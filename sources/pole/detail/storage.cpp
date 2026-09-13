/* POLE - Portable C++ library to access OLE Storage 
   Copyright (C) 2002-2004 Ariya Hidayat <ariya@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, US
*/


#include <iostream>
#include <string.h>
#include "../../../includes/pole/detail/util.hpp"
#include "../../../includes/pole/detail/header.hpp"
#include "../../../includes/pole/detail/dirtree.hpp"
#include "../../../includes/pole/detail/alloctable.hpp"
#include "../../../includes/pole/detail/storage.hpp"
#include "../../../includes/pole/detail/stream.hpp"

#if defined(WIN32)
#include <windows.h>
#include <cstring>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif

namespace POLE
{

// =========== PositionalFile ==========

#if defined(WIN32)

namespace
{
	// One manual-reset event per thread, reused across reads. FILE_FLAG_OVERLAPPED
	// makes every ReadFile asynchronous, and an OVERLAPPED with no event of its own
	// has the kernel signal the file handle instead -- which is ambiguous the moment
	// two reads are in flight on one handle. The event holds no file state, so it is
	// safe as a thread_local where a descriptor would not be; slideio::FileReader
	// keeps the identical one for the identical reason. It is wrapped in a struct
	// with a destructor because a raw thread_local HANDLE has none, and would leak
	// the kernel object for every thread that ever read.
	struct ReadEvent
	{
		HANDLE handle;
		ReadEvent() : handle( CreateEventW( NULL, TRUE, FALSE, NULL ) ) {}
		~ReadEvent() { if( handle ) CloseHandle( handle ); }
	private:
		// no copy or assign
		ReadEvent( const ReadEvent& );
		ReadEvent& operator=( const ReadEvent& );
	};

	ReadEvent& read_event()
	{
		thread_local ReadEvent event;
		return event;
	}
}

#endif

PositionalFile::PositionalFile( const char* filename )
{
#if defined(WIN32)
	// The flags mirror slideio::FileReader. FILE_FLAG_OVERLAPPED is what makes the
	// per-read offset usable from several threads at once: without it the read does
	// still start at the OVERLAPPED offset, but it also moves the handle's shared
	// file pointer and the operations are serialised, which is the very thing this
	// class exists to avoid.
	// The share flags are FileReader's three, so the two copies of this primitive
	// cannot silently diverge -- but note that they do not decide the question here.
	// StorageIO holds the same file open through its std::fstream as well, and that
	// handle's share mode is the binding one: FILE_SHARE_DELETE on this descriptor
	// will not make a delete-while-open succeed while the fstream is also open.
	// FILE_SHARE_READ | FILE_SHARE_WRITE is what MSVC's std::fstream itself asks
	// for, so this second descriptor cannot be refused where the fstream was
	// granted.
	_handle = CreateFileA( filename, GENERIC_READ,
	                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                       NULL, OPEN_EXISTING,
	                       FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL );
#else
	_fd = ::open( filename, O_RDONLY | O_CLOEXEC );
#endif
}

#if defined(WIN32)
PositionalFile::PositionalFile( const wchar_t* filename )
{
	_handle = CreateFileW( filename, GENERIC_READ,
	                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                       NULL, OPEN_EXISTING,
	                       FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL );
}
#endif

PositionalFile::~PositionalFile()
{
#if defined(WIN32)
	if( _handle != INVALID_HANDLE_VALUE ) CloseHandle( _handle );
#else
	if( _fd >= 0 ) ::close( _fd );
#endif
}

bool PositionalFile::good() const
{
#if defined(WIN32)
	return _handle != INVALID_HANDLE_VALUE;
#else
	return _fd >= 0;
#endif
}

ULONG32 PositionalFile::read_at( ULONG32 offset, unsigned char* dst, ULONG32 n ) const
{
	if( !good() || !dst ) return 0;
	_read_calls.fetch_add( 1, std::memory_order_relaxed );

#if !defined(WIN32)
	// A read that makes no progress is retried a bounded number of times. An
	// unbounded loop would turn a pathological source of EINTR -- a signal
	// arriving faster than the read completes -- into a hang inside a library
	// call; a real interruption succeeds on the next attempt.
	const int max_retries = 1000;
	int retries = 0;
#endif

	ULONG32 done = 0;
	while( done < n )
	{
#if defined(WIN32)
		ReadEvent& event = read_event();
		if( !event.handle ) break;

		OVERLAPPED ov;
		memset( &ov, 0, sizeof(ov) );
		// pole is already 32-bit-offset-limited: ULONG32 is unsigned long
		// (util.hpp) and StorageIO::_size is assigned (ULONG32)tellg(), so a
		// compound document over 4 GB is unsupported here already. Do not
		// widen it in this change.
		ov.Offset = (DWORD)( offset + done );
		ov.OffsetHigh = 0;
		ov.hEvent = event.handle;
		ResetEvent( event.handle );

		DWORD got = 0;
		if( !ReadFile( _handle, dst + done, (DWORD)( n - done ), &got, &ov ) )
		{
			// Anything but ERROR_IO_PENDING ends the read. End-of-file is one of
			// those: an overlapped read that starts at or past the end reports
			// ERROR_HANDLE_EOF rather than a zero-byte success. A genuine failure
			// ends it too, and the caller sees the short count.
			if( GetLastError() != ERROR_IO_PENDING ) break;
			// bWait = TRUE, so this returns only once the operation has completed
			// or failed and the kernel no longer refers to this OVERLAPPED. It
			// fails with ERROR_HANDLE_EOF when the pending read hit the end.
			if( !GetOverlappedResult( _handle, &ov, &got, TRUE ) ) break;
		}
		if( got == 0 ) break;
		done += got;
#else
		ssize_t got = ::pread( _fd, dst + done, (size_t)( n - done ), (off_t)( offset + done ) );
		if( got < 0 )
		{
			if( errno == EINTR && ++retries <= max_retries ) continue;
			break;
		}
		if( got == 0 ) break;
		retries = 0;
		done += (ULONG32)got;
#endif
	}
	return done;
}

// =========== StorageIO ==========

StorageIO::StorageIO( const char* filename )
{
	m_dtmodified = false;
	init();

	// open the file, check for error
	_result = OpenFailed;
	std::fstream* file = new std::fstream( filename, std::ios::binary | std::ios::in | std::ios::out);
	if( !file || file->fail() ) return;
	_file = file;
	_stream = file;
	// A second, read-only descriptor, opened alongside the fstream rather than in
	// place of it: the fstream's open mode is what decides whether this document
	// opens at all, and that is deliberately left alone here. If the positional
	// open fails, the read path falls back to seekg under a mutex, so no file that
	// opens today stops opening.
	_pread = new PositionalFile( filename );
	if( !_pread->good() ) { delete _pread; _pread = NULL; }
	load();
}

#if defined(WIN32)
StorageIO::StorageIO(const wchar_t* filename)
{
	m_dtmodified = false;
	init();

	// open the file, check for error
	_result = OpenFailed;
	std::fstream* file = new std::fstream(filename, std::ios::binary | std::ios::in | std::ios::out);
	if (!file || file->fail()) return;
	_file = file;
	_stream = file;
	_pread = new PositionalFile( filename );
	if( !_pread->good() ) { delete _pread; _pread = NULL; }
	load();
}
#endif

StorageIO::StorageIO( std::iostream* stream )
{
	init();
	_result = OpenFailed;
	_stream = stream;
	load();
}

StorageIO::~StorageIO()
{
	flush();
	close();
	if (_sbat) delete _sbat;
	if (_bbat) delete _bbat;
	delete _dirtree;
	delete _header;
}

void StorageIO::init()
{
	_result = Ok;
	_file = NULL;
	_stream = NULL;
	_pread = NULL;

	_header = new Header();
	_dirtree = new DirTree();
	_bbat = new AllocTable(1 << _header->b_shift());
	_sbat = new AllocTable(1 << _header->s_shift());

	_size = 0;
}

bool StorageIO::load()
{
	if (!_stream) return false;

	// find size of input file
	_stream->seekg( 0, std::ios::end );
	_size = (ULONG32)_stream->tellg();

	// load header
	unsigned char* buffer = new unsigned char[512];
	_stream->seekg( 0 ); 
	_stream->read( (char*)buffer, 512 );
	bool res = _header->load( buffer, 512 );
	delete[] buffer;
	if (!res)
		return false;

	// check OLE magic id
	_result = NotOLE;
	if (!_header->is_ole())
		return false;

	// sanity checks
	_result = BadOLE;
	if (!_header->valid())
		return false;

	// important block size
	_bbat->set_block_size(1 << _header->b_shift());
	_sbat->set_block_size(1 << _header->s_shift());

	// find blocks allocated to store big bat
	// the first 109 blocks are in header, the rest in meta bat
	std::vector<ULONG32> blocks;
	blocks.resize( _header->num_bat() );
	for( unsigned i = 0; i < 109; i++ )
	{
		if( i >= _header->num_bat() ) 
			break;
		else 
			blocks[i] = _header->bb_blocks()[i];
	}
	if( (_header->num_bat() > 109) && (_header->num_mbat() > 0) )
	{
		unsigned char* buffer2 = new unsigned char[ _bbat->block_size() ];
		unsigned k = 109;
		for( unsigned r = 0; r < _header->num_mbat(); r++ )
		{
			loadBigBlock( _header->mbat_start()+r, buffer2, _bbat->block_size() );
			for( unsigned s=0; s < _bbat->block_size(); s+=4 )
			{
				if( k >= _header->num_bat() ) 
					break;
				else  
					blocks[k++] = readU32( buffer2 + s );
			}  
		}    
		delete[] buffer2;
	}

	// load big bat
	ULONG32 buflen = (ULONG32)(blocks.size()*_bbat->block_size());
	if( buflen > 0 )
	{
		buffer = new unsigned char[ buflen ];  
		loadBigBlocks( blocks, buffer, buflen );
		_bbat->load( buffer, buflen );
		delete[] buffer;
	}  

	// load small bat
	blocks.clear();
	if (!_bbat->follow( _header->sbat_start(), blocks ))
		return false;
	buflen = (ULONG32)(blocks.size()*_bbat->block_size());
	if( buflen > 0 )
	{
		buffer = new unsigned char[ buflen ];  
		loadBigBlocks( blocks, buffer, buflen );
		_sbat->load( buffer, buflen );
		delete[] buffer;
	}  
		
	// load directory tree
	blocks.clear();
	if (!_bbat->follow( _header->dirent_start(), blocks ))
		return false;
	buflen = (ULONG32)(blocks.size()*_bbat->block_size());
	buffer = new unsigned char[ buflen ];  
	loadBigBlocks( blocks, buffer, buflen );
	if (!_dirtree->load( buffer, buflen ))
		return false;
	unsigned sb_start = readU32( buffer + 0x74 );
	delete[] buffer;
		
	// fetch block chain as data for small-files
	if (!_bbat->follow( sb_start, _sb_blocks ))// small files
		return false;

	// for troubleshooting, just enable this block
	#if 0
	_header->debug();
	_sbat->debug();
	_bbat->debug();
	_dirtree->debug();
	#endif

	// so far so good
	_result = Ok;
	return true;
}

bool StorageIO::create( const char* filename )
{
  std::fstream* file = new std::fstream(filename, std::ios::out|std::ios::binary);
  if( !file || file->fail() )
  {
    _result = OpenFailed;
	if (file)
		delete file;
    return false;
  }
  
  // so far so good
  _result = Ok;
  _file = file;
  _stream = file;
  return true;
}

void StorageIO::close()
{
	flush();
	std::list<StreamImpl*>::iterator it;
	for( it = _streams.begin(); it != _streams.end(); ++it )
	delete *it;
	_streams.clear();

	if (_file)
	{
	_file->close();
	delete _file;
	_file = NULL;
	}

	if (_pread)
	{
		delete _pread;
		_pread = NULL;
	}
}

ULONG32 StorageIO::loadBigBlocks( const std::vector<ULONG32>& blocks, unsigned char* data, ULONG32 maxlen ) const
{
  // sentinel
  if( !data ) return 0;
  // A positionally opened document must not be turned away on the state of an
  // fstream that nothing on this path reads any more.
  if( !_pread && ( !_stream || !_stream->good() ) ) return 0;
  if( blocks.size() < 1 ) return 0;
  if( maxlen == 0 ) return 0;

  // read block one by one, seems fast enough
  ULONG32 bytes = 0;
  for( ULONG32 i=0; (i < blocks.size() ) && ( bytes < maxlen ); i++ )
  {
    ULONG32 block = blocks[i];
	ULONG32 pos =  _bbat->block_size() * ( block+1 );
    ULONG32 p = (_bbat->block_size() < maxlen-bytes) ? _bbat->block_size() : maxlen-bytes;
    if( pos + p > _size ) 
		p = _size - pos;
    if( _pread )
    {
      // The count actually read, where the fstream branch below advances by the
      // requested p whether or not that many bytes were there. StreamImpl::read
      // checks the total against the size it expected, so the true count is what
      // it wants.
      bytes += _pread->read_at( pos, data + bytes, p );
    }
    else
    {
      std::lock_guard<std::mutex> lock( _stream_mutex );
      _stream->seekg( pos );
      _stream->read( (char*)data + bytes, p );
      bytes += p;
    }
  }

  return bytes;
}

ULONG32 StorageIO::loadBigBlockRun( ULONG32 firstBlock, ULONG32 nBlocks, ULONG32 offsetInFirst,
                                    unsigned char* dst, ULONG32 n ) const
{
  // sentinel
  if( !dst ) return 0;
  if( !_pread && ( !_stream || !_stream->good() ) ) return 0;
  if( nBlocks == 0 ) return 0;
  if( n == 0 ) return 0;
  if( !_bbat ) return 0;

  const ULONG32 bs = _bbat->block_size();
  if( bs == 0 || offsetInFirst >= bs ) return 0;

  // The +1 is the header block, exactly as loadBigBlocks computes it: block
  // number b begins at bs * (b+1).
  const ULONG32 pos = bs * ( firstBlock + 1 ) + offsetInFirst;
  const ULONG32 avail = nBlocks * bs - offsetInFirst;
  ULONG32 p = ( n < avail ) ? n : avail;

  // Same clamp loadBigBlocks applies per block, hoisted to the run. Tested
  // before the subtraction rather than after, since these are unsigned.
  if( pos >= _size ) return 0;
  if( pos + p > _size ) p = _size - pos;

  if( _pread )
    return _pread->read_at( pos, dst, p );

  std::lock_guard<std::mutex> lock( _stream_mutex );
  _stream->seekg( pos );
  _stream->read( (char*)dst, p );
  return p;
}

ULONG32 StorageIO::loadBigBlock( ULONG32 block, unsigned char* data, ULONG32 maxlen ) const
{
  // sentinel
  if( !data ) return 0;
  if( !_pread && ( !_stream || !_stream->good() ) ) return 0;
  
  // wraps call for loadBigBlocks
  std::vector<ULONG32> blocks;
  blocks.resize( 1 );
  blocks[ 0 ] = block;
  
  return loadBigBlocks( blocks, data, maxlen );
}

// return number of bytes which has been read
ULONG32 StorageIO::loadSmallBlocks( const std::vector<ULONG32>& blocks, unsigned char* data, ULONG32 maxlen ) const
{
  // sentinel
  if( !data ) return 0;
  if( !_pread && ( !_stream || !_stream->good() ) ) return 0;
  if( blocks.size() < 1 ) return 0;
  if( maxlen == 0 ) return 0;

  // our own local buffer
  unsigned char* buf = new unsigned char[ _bbat->block_size() ];

  // read small block one by one
  ULONG32 bytes = 0;
  for( ULONG32 i=0; ( i<blocks.size() ) && ( bytes<maxlen ); i++ )
  {
    ULONG32 block = blocks[i];

    // find where the small-block exactly is
    ULONG32 pos = block * _sbat->block_size();
    ULONG32 bbindex = pos / _bbat->block_size();
    if( bbindex >= _sb_blocks.size() ) break;

    ULONG32 read = loadBigBlock( _sb_blocks[ bbindex ], buf, _bbat->block_size() );
	if (read != _bbat->block_size())
		break;

    // copy the data
    ULONG32 offset = pos % _bbat->block_size();
    ULONG32 p = (maxlen-bytes < _bbat->block_size()-offset ) ? maxlen-bytes : _bbat->block_size()-offset;
    if (p > _sbat->block_size())
		p = _sbat->block_size();
    memcpy( data + bytes, buf + offset, p );
    bytes += p;
  }
  
  delete[] buf;

  return bytes;
}

ULONG32 StorageIO::loadSmallBlockRun( const std::vector<ULONG32>& blocks, size_t firstIndex,
                                      ULONG32 offsetInFirst, unsigned char* dst, ULONG32 n ) const
{
  // sentinel
  if( !dst ) return 0;
  if( !_pread && ( !_stream || !_stream->good() ) ) return 0;
  if( blocks.empty() || firstIndex >= blocks.size() ) return 0;
  if( n == 0 ) return 0;
  if( !_bbat || !_sbat ) return 0;

  const ULONG32 bbs = _bbat->block_size();
  const ULONG32 sbs = _sbat->block_size();
  if( bbs == 0 || sbs == 0 || offsetInFirst >= sbs ) return 0;

  unsigned char* buf = new unsigned char[ bbs ];
  // Which big block of the container buf currently holds. The container is
  // indexed through _sb_blocks, so this is an index into that, and no valid
  // index equals the sentinel.
  size_t cached = (size_t)-1;

  ULONG32 bytes = 0;
  ULONG32 offset = offsetInFirst;
  for( size_t i = firstIndex; i < blocks.size() && bytes < n; ++i )
  {
    // Where this small block sits inside the container stream, and which of
    // the container's big blocks that lands in.
    const ULONG32 pos = blocks[i] * sbs;
    const size_t bbindex = pos / bbs;
    if( bbindex >= _sb_blocks.size() ) break;

    if( bbindex != cached )
    {
      if( loadBigBlock( _sb_blocks[ bbindex ], buf, bbs ) != bbs ) break;
      cached = bbindex;
    }

    const ULONG32 inBlock = pos % bbs + offset;
    // Never past the end of this small block, of the big block holding it, or
    // of what the caller asked for.
    ULONG32 p = sbs - offset;
    if( p > bbs - inBlock ) p = bbs - inBlock;
    if( p > n - bytes ) p = n - bytes;
    if( p == 0 ) break;

    memcpy( dst + bytes, buf + inBlock, p );
    bytes += p;
    offset = 0;
  }

  delete[] buf;
  return bytes;
}

ULONG32 StorageIO::loadSmallBlock( ULONG32 block, unsigned char* data, ULONG32 maxlen ) const
{
  // sentinel
  if( !data ) return 0;
  if( !_pread && ( !_stream || !_stream->good() ) ) return 0;

  // wraps call for loadSmallBlocks
  std::vector<ULONG32> blocks;
  blocks.resize( 1 );
  blocks.assign( 1, block );

  return loadSmallBlocks( blocks, data, maxlen );
}

// list all files and subdirs in current path
void StorageIO::listDirectory(std::list<std::string>& result) const
{
  std::vector<const DirEntry*> entries;
  _dirtree->listDirectory(entries);
  for( unsigned i = 0; i < entries.size(); i++ )
    result.push_back( entries[i]->name() );
}

void StorageIO::listEntries(std::vector<const DirEntry*>& result) const
{
  _dirtree->listDirectory(result);
}


// Write a bigblock
ULONG32 StorageIO::saveBlock(ULONG32 fisical_offset, const unsigned char* data, ULONG32 len)
{
	_file->seekp(fisical_offset);
	_file->write((const char*)data, len);
	return len;
}

void StorageIO::flush()
{
	if (m_dtmodified && _bbat && _header)
	{
		std::vector<ULONG32> blocks;
		if (!_bbat->follow( _header->dirent_start(), blocks ))
			return;
		ULONG32 bufflen = (ULONG32)(blocks.size() * _bbat->block_size());
		unsigned char *buffer = new unsigned char[bufflen];
		if (!_dirtree->save(buffer, bufflen))
			return;
		for (ULONG32 ndx = 0; ndx < blocks.size(); ++ndx)
		{
			ULONG32 fisical_offset = (blocks[ndx] * big_block_size()) + big_block_size();
			saveBlock(fisical_offset, buffer, big_block_size());
			buffer += big_block_size();
		}
		m_dtmodified = false;
	}
}

}

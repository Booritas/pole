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

namespace POLE
{

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
	load();
}

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
	load();
}

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
}

ULONG32 StorageIO::loadBigBlocks( const std::vector<ULONG32>& blocks, unsigned char* data, ULONG32 maxlen )
{
  // sentinel
  if( !_stream ) return 0; 
  if( !data ) return 0;
  if( !_stream->good() ) return 0;
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
	_stream->seekg( pos );
	_stream->read( (char*)data + bytes, p );
    bytes += p;
  }

  return bytes;
}

ULONG32 StorageIO::loadBigBlock( ULONG32 block, unsigned char* data, ULONG32 maxlen )
{
  // sentinel
  if( !data ) return 0;
  if( !_stream || !_stream->good() ) return 0;
  
  // wraps call for loadBigBlocks
  std::vector<ULONG32> blocks;
  blocks.resize( 1 );
  blocks[ 0 ] = block;
  
  return loadBigBlocks( blocks, data, maxlen );
}

// return number of bytes which has been read
ULONG32 StorageIO::loadSmallBlocks( const std::vector<ULONG32>& blocks, unsigned char* data, ULONG32 maxlen )
{
  // sentinel
  if( !data ) return 0;
  if( !_stream || !_stream->good() ) return 0;
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

ULONG32 StorageIO::loadSmallBlock( ULONG32 block, unsigned char* data, ULONG32 maxlen )
{
  // sentinel
  if( !data ) return 0;
  if( !_stream || !_stream->good() ) return 0;

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
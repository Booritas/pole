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

// storage header
#pragma once

#include <fstream>
#include <list>
#include <mutex>
#include "header.hpp"
#include "dirtree.hpp"
#include "alloctable.hpp"

namespace POLE
{

class StreamImpl;

// A read-only file handle whose reads carry their own offset, so any number of
// threads may read one file through one descriptor with no shared cursor.
//
// This duplicates slideio::FileReader (src/slideio/core/tools/filereader.hpp in
// the slideio repository) on purpose: pole is vendored as a submodule precisely
// because it depends on nothing but the standard library, so it cannot use it.
// Keep the two in step -- in particular the short-read retry loop, which exists
// because pread is permitted to return fewer bytes than requested, and the
// Windows open flags, which FileReader documents at length.
class PositionalFile
{
public:
	PositionalFile( const char* filename );
#if defined(WIN32)
	PositionalFile( const wchar_t* filename );
#endif
	~PositionalFile();

	bool good() const;
	// Reads up to n bytes from offset. Returns bytes actually read.
	ULONG32 read_at( ULONG32 offset, unsigned char* dst, ULONG32 n ) const;

private:
#if defined(WIN32)
	void* _handle;
#else
	int _fd;
#endif

	// no copy or assign
	PositionalFile( const PositionalFile& );
	PositionalFile& operator=( const PositionalFile& );
};

class StorageIO
{
public:
	enum { Ok, OpenFailed, NotOLE, BadOLE, UnknownError, StupidWorkaroundForBrokenCompiler=255 };

// Construction/destruction  
public:
    StorageIO( const char* filename );
#if defined(WIN32)
	StorageIO(const wchar_t* filename);
#endif	
	StorageIO( std::iostream* stream );
    ~StorageIO();
    
// Attributes
public:
	int result() const { return _result; }
	const Header* header() const { return _header; }
	const DirEntry* entry(const std::string& path, bool create = false) const { return _dirtree->entry(path, create); }
	void path( std::string& result) const { _dirtree->path(result); }
	void listDirectory(std::list<std::string>&) const;
	void listEntries(std::vector<const DirEntry*>& result) const;
	ULONG32 small_block_size() const { return (_sbat) ? _sbat->block_size() : 0; }
	ULONG32 big_block_size() const { return (_bbat) ? _bbat->block_size() : 0; }
	bool follow_small_block_table( ULONG32 start, std::vector<ULONG32>& chain ) const 
	{ 
		if (_sbat) 
			return _sbat->follow(start, chain); 
		return false;
	}
	bool follow_big_block_table( ULONG32 start, std::vector<ULONG32>& chain ) const 
	{ 
		if (_bbat) 
			return _bbat->follow(start, chain); 
		return false;
	}

	const std::vector<ULONG32>& sb_blocks() const
	{
		return _sb_blocks;
	}

	void get_entry_childrens(size_t index, std::vector<size_t> result)
	{
		_dirtree->children(index, result);
	}

	void debug() {_dirtree->debug();}
	void children( size_t index, std::vector<size_t>& result ) const
	{
		if (_dirtree)
			_dirtree->children(index, result);
	}

// Operations
public:
    bool create( const char* filename );
	bool enterDirectory( const std::string& directory ) { return _dirtree->enterDirectory( directory ); }
	void leaveDirectory() { return _dirtree->leaveDirectory(); }
	ULONG32 loadSmallBlock(ULONG32 block, unsigned char* buffer, ULONG32 maxlen) const;
    ULONG32 loadBigBlock(ULONG32 block, unsigned char* buffer, ULONG32 maxlen) const;
	ULONG32 saveBlock(ULONG32 block, const unsigned char* buffer, ULONG32 maxlen);
	// Delete an entry identified by path, then save changes 
	// made to the document by calling flush
	bool delete_entry(const std::string& path) 
	{ 
		m_dtmodified = true;
		if (_dirtree && _dirtree->delete_entry(path)) 
		{
			flush();
			return true; 
		}
		return false; 
	}
	// Save changes made to the documment
	void flush();

// Implementation
private:  
    void init();
    bool load();
    void close();

	ULONG32 loadSmallBlocks( const std::vector<ULONG32>& blocks, unsigned char* buffer, ULONG32 maxlen ) const;
	ULONG32 loadBigBlocks( const std::vector<ULONG32>& blocks, unsigned char* buffer, ULONG32 maxlen ) const;
//	ULONG32 saveBigBlock(ULONG32 fisical_offset, const unsigned char* data, ULONG32 len);

    std::iostream* _stream;
    std::fstream* _file;
	PositionalFile* _pread;           // read path; NULL for the iostream* ctor
	mutable std::mutex _stream_mutex; // guards _stream when _pread is NULL
	ULONG32 _size;   // size of the storage stream
    int _result;     // result of last operation
    std::list<StreamImpl*> _streams; // current streams
    std::vector<ULONG32> _sb_blocks; // blocks for "small" files
	
    Header* _header;           // storage header 
    DirTree* _dirtree;         // directory tree
    AllocTable* _bbat;         // allocation table for big blocks
    AllocTable* _sbat;         // allocation table for small blocks
	bool m_dtmodified;

	// no copy or assign
    StorageIO( const StorageIO& );
    StorageIO& operator=( const StorageIO& );
};

}

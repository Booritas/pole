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
#include "header.hpp"
#include "dirtree.hpp"
#include "alloctable.hpp"

namespace POLE
{

class StreamImpl;

class StorageIO
{
public:
	enum { Ok, OpenFailed, NotOLE, BadOLE, UnknownError, StupidWorkaroundForBrokenCompiler=255 };

// Construction/destruction  
public:
    StorageIO( const char* filename );
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
	ULONG32 loadSmallBlock(ULONG32 block, unsigned char* buffer, ULONG32 maxlen);
    ULONG32 loadBigBlock(ULONG32 block, unsigned char* buffer, ULONG32 maxlen);
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

	ULONG32 loadSmallBlocks( const std::vector<ULONG32>& blocks, unsigned char* buffer, ULONG32 maxlen );
	ULONG32 loadBigBlocks( const std::vector<ULONG32>& blocks, unsigned char* buffer, ULONG32 maxlen );
//	ULONG32 saveBigBlock(ULONG32 fisical_offset, const unsigned char* data, ULONG32 len);

    std::iostream* _stream;
    std::fstream* _file;
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

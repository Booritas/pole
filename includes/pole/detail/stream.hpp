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

namespace POLE
{

class StorageIO;

class StreamImpl
{
public:
	enum {Eof = 1, Bad = 2};
	static const std::string null_path;

// Construction/destruction  
public:
    StreamImpl( const StreamImpl& );
	StreamImpl(StorageIO* io, const DirEntry* e): _entry(e), _io(io) { init(); }
	StreamImpl(StorageIO* io, const std::string& path): _entry(io->entry(path)), _io(io) { init(); }
    ~StreamImpl() { delete[] _cache_data; }

// Attributes
public:
	const std::string& path() const { return _entry ? _entry->name() : null_path; }
	ULONG32 size() const { return (_entry) ? _entry->size() : 0; }
    std::streamsize tell() const { return _pos; }
	bool fail() const { return ((_state & StreamImpl::Bad) != 0); }
	bool eof() const { return ((_state & StreamImpl::Eof) != 0); }

// Operations
public:
    void seek( unsigned long pos ) 
	{
		if (pos > _entry->size())
		{
			_state |= StreamImpl::Eof;
			return;
		}
		else
		{
			_state &= StreamImpl::Eof;
		}

		_pos = pos;
	}

    int getch();
    std::streamsize read( unsigned char* data, std::streamsize maxlen );

	POLE::ULONG32 write(const unsigned char* data, POLE::ULONG32 maxlen);

// Implementation
private:
	void init();
	std::streamsize read( size_t pos, unsigned char* data, std::streamsize maxlen );
	void update_cache();

	StorageIO* _io; 
    const DirEntry* _entry; 
    std::vector<ULONG32> _blocks;
    std::streamsize _pos; // pointer for read

	// simple cache system to speed-up getch()
	unsigned char* _cache_data; 
    std::streamsize _cache_size;
    std::streamsize _cache_pos;
	int _state;

    // no default, copy or assign
    StreamImpl( );
    StreamImpl& operator=( const StreamImpl& );
};

}

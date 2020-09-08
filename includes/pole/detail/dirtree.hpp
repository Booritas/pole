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

// dirtree header
#pragma once

#include <vector>
#include <string>

namespace POLE
{

class DirEntry
{
public:
    static const unsigned End;
  
// Construction/destruction  
public:
	DirEntry():  
		_name("Root Entry"),
		_type(5),
		_size(0),
		_start(End),
		_prev(End),
		_next(End),
		_child(End),
		_index(0),
		_modif(false)
	{}
	DirEntry(const std::string& name, ULONG16 len, ULONG8 type, ULONG32 size, ULONG32 start, ULONG32 prev, ULONG32 next, ULONG32 child, size_t index)
	{ 
		set(name, len, type, size, start, prev, next, child, index);
	}

// Attributes
public:
	bool valid() const;
	const std::string& name() const { return _name; }
	ULONG8 type() const { return _type; }
	bool root() const { return (_type == 5); }
	bool dir() const { return ((_type == 1) || (_type == 5)); }
	bool file() const { return (_type == 2); }
	ULONG32 size() const { return _size; }
	ULONG32 start() const { return _start; }
	ULONG32 prev() const { return _prev; }
	ULONG32 next() const { return _next; }
	ULONG32 child() const { return _child; }
	bool modified() const { return _modif; }
	size_t index() const { return _index; }

// Operations
public:
	void set(const std::string& name, ULONG16 len, ULONG8 type, ULONG32 size, ULONG32 start, ULONG32 prev, ULONG32 next, ULONG32 child, size_t index, bool modif = false)
	{
		_name = name;
		_name_len = len;
		_type = type;
		_size = size;
		_start = start;
		_prev = prev;
		_next = next;
		_child = child;
		_index = index;
		_modif = modif;
	}
	void set_name(const std::string& name) { _name = name; set_modif(); }
	void set_name_len(ULONG16 len) { _name_len = len; set_modif(); }
	void set_type(ULONG8 type) { _type = type; set_modif(); }
	void set_size(ULONG32 size) { _size = size; set_modif(); }
	void set_start(ULONG32 start) { _start = start; set_modif(); }
	void set_prev(ULONG32 prev) { _prev = prev; set_modif(); }
	void set_next(ULONG32 next) { _next = next; set_modif(); }
	void set_child(ULONG32 child) { _child = child; set_modif(); }
	void set_modif(bool modif = true) { _modif = modif; }

		
// Implementation
private:
    std::string _name;  // the name, not in unicode anymore 
	ULONG16 _name_len;  // name length 
    ULONG8 _type;       // true if directory   
    ULONG32 _size;		// size (not valid if directory)
    ULONG32 _start;		// starting block
    ULONG32 _prev;      // previous sibling
    ULONG32 _next;      // next sibling
    ULONG32 _child;     // first child
	size_t _index;		// index of the entry in the directory
	bool _modif;
};

class DirTree
{
// Construction/destruction  
public:
	DirTree(): _current(0) { clear(); }

// Attributes
public:
    size_t entryCount() const { return _entries.size(); }
    size_t indexOf( const DirEntry* e ) const;
    size_t parent( size_t index ) const;
    void fullName( size_t index, std::string& ) const;
    void path( std::string& result) const {  fullName( _current, result ); }
    void children( size_t index, std::vector<size_t>& ) const;
    void listDirectory(std::vector<const DirEntry*>&) const;
    const DirEntry* entry( size_t index ) const;
	
// Operations
public:
	void clear();
	const DirEntry* entry( const std::string& name, bool create=false ) { return _entry(name, create); }
    bool enterDirectory( const std::string& dir );
    void leaveDirectory();
    
	bool delete_entry(const std::string& path, int level = 0 );
	bool load( unsigned char* buffer, size_t len );
    bool save( unsigned char* buffer, size_t len );
    void debug();
  
// Implementation
private:
	DirEntry* _entry( size_t index );
	DirEntry* _entry( const std::string& name, bool create=false );
	void find_siblings( std::vector<size_t>& result, ULONG32 index ) const;
	size_t search_prev_link( size_t entry );
	size_t find_rightmost_sibling(size_t left_sib);
	bool set_prev_link(size_t prev_link, size_t entry, ULONG32 value);


	size_t _current;
    std::vector<DirEntry> _entries;
    
	DirTree( const DirTree& );
    DirTree& operator=( const DirTree& );
};

}

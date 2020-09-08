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

// alloctable header
#pragma once

#include <vector>

namespace POLE
{

class AllocTable
{
public:
    static const ULONG32 Eof;
    static const ULONG32 Avail;
    static const ULONG32 Bat;    
    static const ULONG32 MetaBat;    

// Construction/destruction  
public:
    AllocTable(ULONG32 block_size = 4096) { set_block_size(block_size); }

// Attributes
public:
	size_t count() const { return _data.size(); } // number of blocks
	ULONG32 block_size() const { return _block_size; } // block size
    ULONG32 operator[]( size_t index ) const { return _data[index]; }
    bool follow( ULONG32 start, std::vector<ULONG32>& chain ) const;

// Operations
public:
	void set_block_size(ULONG32 size) { _block_size = size; _data.clear(); resize( 128 ); }
    void set_chain( const std::vector<ULONG32>& chain );

    bool load( const unsigned char* buffer, size_t len );
    bool save( unsigned char* buffer, size_t len );
    void debug() const;

// Implementation
private:
	void resize( size_t newsize ) { _data.resize( newsize,  Avail); }
    size_t unused();
	void preserve( size_t n ) { unused(); }
    void set( size_t index, ULONG32 val );

    std::vector<ULONG32> _data;
    ULONG32 _block_size;
    
	AllocTable( const AllocTable& ); // No copy construction
    AllocTable& operator=( const AllocTable& ); // No copy operator
};

}

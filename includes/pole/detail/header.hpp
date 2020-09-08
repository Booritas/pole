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

// header.hpp
#pragma once
#include "util.hpp"

namespace POLE
{
class Header
{
// Construction/destruction
public:
     Header();

// Attributes
public:
	const unsigned char* id() const { return _id; }
	unsigned b_shift() const { return _b_shift; }
	unsigned s_shift() const { return _s_shift; }
	unsigned num_bat() const { return _num_bat; }
	unsigned dirent_start() const { return _dirent_start; }
	unsigned threshold() const { return _threshold; }
	unsigned sbat_start() const { return _sbat_start; }
	unsigned num_sbat() const { return _num_sbat; }
	unsigned mbat_start() const { return _mbat_start; }
	unsigned num_mbat() const { return _num_mbat; }
	const ULONG32* bb_blocks() const { return _bb_blocks; }
	bool valid() const;
	bool is_ole() const;
    
// Operations
public:
    bool load( const unsigned char* buffer, size_t len );
    bool save( unsigned char* buffer, size_t len );
    void debug();

// Implementation
private:
    unsigned char _id[8];     // signature, or magic identifier
    unsigned _b_shift;        // bbat->blockSize = 1 << b_shift [_uSectorShift]
    unsigned _s_shift;        // sbat->blockSize = 1 << s_shift [_uMiniSectorShift]
    unsigned _num_bat;        // blocks allocated for big bat   [_csectFat]
    unsigned _dirent_start;   // starting block for directory info  [_secDirStart]
    unsigned _threshold;      // switch from small to big file (usually 4K)  [_ulMiniSectorCutoff]
    unsigned _sbat_start;     // starting block index to store small bat  [_sectMiniFatStart]
    unsigned _num_sbat;       // blocks allocated for small bat  [_csectMiniFat]
    unsigned _mbat_start;     // starting block to store meta bat  [_sectDifStart]
    unsigned _num_mbat;       // blocks allocated for meta bat  [_csectDif]
    ULONG32 _bb_blocks[109];  //                                [_sectFat]
};

}

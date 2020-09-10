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
#include "../../../includes/pole/detail/util.hpp"
#include "../../../includes/pole/detail/alloctable.hpp"
#include "../../../includes/pole/detail/header.hpp"

namespace POLE
{

static const unsigned char pole_magic[] = { 0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1 };

// =========== Header ==========

Header::Header()
{
  _b_shift = 9;
  _s_shift = 6;
  _num_bat = 0;
  _dirent_start = 0;
  _threshold = 4096;
  _sbat_start = 0;
  _num_sbat = 0;
  _mbat_start = 0;
  _num_mbat = 0;

  for( unsigned i = 0; i < 8; i++ )
    _id[i] = pole_magic[i];  
  for( unsigned i=0; i<109; i++ )
    _bb_blocks[i] = AllocTable::Avail;
}

bool Header::valid() const
{
  const uint32_t sector_size((1 << _b_shift) / 4 - 1);
  if( _threshold != 4096 ) return false;
  if( _num_bat == 0 ) return false; //ok
  if( (_num_bat > 109) && (_num_bat > ((_num_mbat * sector_size) + 109))) return false; //ok
  if( (_num_bat < 109) && (_num_mbat != 0) ) return false; //ok
  if( _s_shift > _b_shift ) return false;
  if( _b_shift <= 6 ) return false;
  if( _b_shift >=31 ) return false;
  return true;
}

bool Header::is_ole() const
{
  for( unsigned i=0; i<8; i++ )
    if( _id[i] != pole_magic[i] )
      return false;
  return true;
}

bool Header::load( const unsigned char* buffer, size_t len )
{
  if (len < 0x4C+109 * 4 || !buffer)
	  return false;

  _b_shift     = readU16( buffer + 0x1e );
  _s_shift     = readU16( buffer + 0x20 );
  _num_bat      = readU32( buffer + 0x2c );
  _dirent_start = readU32( buffer + 0x30 );
  _threshold    = readU32( buffer + 0x38 );
  _sbat_start   = readU32( buffer + 0x3c );
  _num_sbat     = readU32( buffer + 0x40 );
  _mbat_start   = readU32( buffer + 0x44 );
  _num_mbat     = readU32( buffer + 0x48 );
  
  for( unsigned i = 0; i < 8; i++ )
    _id[i] = buffer[i];  
  for( unsigned i=0; i<109; i++ )
    _bb_blocks[i] = readU32( buffer + 0x4C+i*4 );

  return true;
}

bool Header::save( unsigned char* buffer, size_t len )
{
  if (len<0x4C+109*4 || !buffer)
	  return false;

  memset( buffer, 0, 0x4c );
  memcpy( buffer, pole_magic, 8 );        // ole signature
  writeU32( buffer + 8, 0 );              // unknown 
  writeU32( buffer + 12, 0 );             // unknown
  writeU32( buffer + 16, 0 );             // unknown
  writeU16( buffer + 24, 0x003e );        // revision ?
  writeU16( buffer + 26, 3 );             // version ?
  writeU16( buffer + 28, 0xfffe );        // unknown
  writeU16( buffer + 0x1e, _b_shift );
  writeU16( buffer + 0x20, _s_shift );
  writeU32( buffer + 0x2c, _num_bat );
  writeU32( buffer + 0x30, _dirent_start );
  writeU32( buffer + 0x38, _threshold );
  writeU32( buffer + 0x3c, _sbat_start );
  writeU32( buffer + 0x40, _num_sbat );
  writeU32( buffer + 0x44, _mbat_start );
  writeU32( buffer + 0x48, _num_mbat );
  
  for( unsigned i=0; i<109; i++ )
    writeU32( buffer + 0x4C+i*4, _bb_blocks[i] );

  return true;
}

void Header::debug()
{
  std::cout << std::endl;
  std::cout << "b_shift " << _b_shift << std::endl;
  std::cout << "s_shift " << _s_shift << std::endl;
  std::cout << "num_bat " << _num_bat << std::endl;
  std::cout << "dirent_start " << _dirent_start << std::endl;
  std::cout << "threshold " << _threshold << std::endl;
  std::cout << "sbat_start " << _sbat_start << std::endl;
  std::cout << "num_sbat " << _num_sbat << std::endl;
  std::cout << "mbat_start " << _mbat_start << std::endl;
  std::cout << "num_mbat " << _num_mbat << std::endl;
  
  unsigned s = (_num_bat<=109) ? _num_bat : 109;
  std::cout << "bat blocks: ";
  for( unsigned i = 0; i < s; i++ )
    std::cout << _bb_blocks[i] << " ";
  std::cout << std::endl;
}
 

}
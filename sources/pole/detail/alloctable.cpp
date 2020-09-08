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

namespace POLE
{


 
// =========== AllocTable ==========

const ULONG32 AllocTable::Avail = 0xffffffff;
const ULONG32 AllocTable::Eof = 0xfffffffe;
const ULONG32 AllocTable::Bat = 0xfffffffd;
const ULONG32 AllocTable::MetaBat = 0xfffffffc;


void AllocTable::set( size_t index, ULONG32 value )
{
  if ( index >= count() )
	  resize( index + 1);
  _data[ index ] = value;
}

void AllocTable::set_chain( const std::vector<ULONG32>& chain )
{
  if ( chain.size() )
  {
    for( size_t i = 0; i<chain.size()-1; i++ )
      set( chain[i], chain[i+1] );
    set( chain[ chain.size()-1 ], Eof );
  }
}

// follow 
bool AllocTable::follow( ULONG32 start, std::vector<ULONG32>& chain ) const
{
  if( start >= count() ) 
	  return false; 

  bool result = true;
  ULONG32 p = start;
  size_t loop_control = 0;
  while( p < count() )
  {
    if (!(loop_control < count()))
	{
		result = false;
		break;
	}
    chain.push_back( p );
    p = _data[ p ];
	++loop_control;
  }

  return result;
}

size_t AllocTable::unused()
{
  // find first available block
  for( size_t i = 0; i < _data.size(); i++ )
    if( _data[i] == Avail )
      return i;
  
  // completely full, so enlarge the table
  size_t block = _data.size();
  resize( _data.size() + 10 );
  return block;      
}

bool AllocTable::load( const unsigned char* buffer, size_t len )
{
  if (len%4 || !buffer)
    return false;

  resize( len / 4 );
  for( size_t i = 0; i < count(); i++ )
    set( i, readU32( buffer + i*4 ) );
  
  return true;
}

bool AllocTable::save( unsigned char* buffer, size_t len )
{
  if (len < 4*count() || !buffer)
	  return false;

  for( size_t i = 0; i < count(); i++ )
    writeU32( buffer + i*4, _data[i] );

  return true;
}

void AllocTable::debug() const
{
	std::cout << "block size " << _data.size() << std::endl;
	for( size_t i=0; i< _data.size(); i++ )
	{
		if( _data[i] == Avail ) 
			continue;
        std::cout << i << ": ";
        if( _data[i] == Eof ) 
			std::cout << "[eof]";
        else if( _data[i] == Bat ) 
			std::cout << "[bat]";
        else if( _data[i] == MetaBat ) 
			std::cout << "[metabat]";
        else std::cout << _data[i];
			std::cout << std::endl;
	}
}

}
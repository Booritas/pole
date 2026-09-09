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
#include <cstdlib>
#include <string.h>
#include "../../../includes/pole/detail/util.hpp"
#include "../../../includes/pole/detail/header.hpp"
#include "../../../includes/pole/detail/dirtree.hpp"
#include "../../../includes/pole/detail/alloctable.hpp"
#include "../../../includes/pole/detail/storage.hpp"
#include "../../../includes/pole/detail/stream.hpp"

namespace POLE
{

// =========== StreamImpl ==========

const std::string StreamImpl::null_path;

StreamImpl::StreamImpl( const StreamImpl& stream)
{
	_io = stream._io; 
    _entry = stream._entry; 
	_blocks = stream._blocks;
	_pos = stream._pos;

	_cache_size = stream._cache_size;
    _cache_pos = stream._cache_pos;
	_cache_data = new unsigned char[4096];
	for (std::streamsize i = 0; i<_cache_size; i++)
		_cache_data[i] = stream._cache_data[i];

	_state = stream._state;
}

void StreamImpl::init()
{
  _pos = 0;
  _state = 0;
  // prepare cache
  _cache_pos = 0;
  _cache_size = 4096; // optimal ?
  _cache_data = new unsigned char[_cache_size];

  // sanity check
  if (!_entry) return;

  if( _entry->size() >= _io->header()->threshold() ) 
  {
	  if (!_io->follow_big_block_table( _entry->start(), _blocks ))
		  _state = StreamImpl::Bad;
  }
  else
    if (!_io->follow_small_block_table( _entry->start(), _blocks ))
		_state = StreamImpl::Bad;

  //update_cache();
}

int StreamImpl::getch()
{
  // sanity check
  if (!_entry) 
	  return 0;

  // past end-of-file ?
  if( _pos > static_cast<std::streamsize>(_entry->size()) ) 
	  return -1;

  // need to update cache ?
  if( !_cache_size || ( _pos < _cache_pos ) ||
    ( _pos >= _cache_pos + _cache_size ) )
      update_cache();

  // something bad if we don't get good cache
  if( !_cache_size ) 
	  return -1;

  int data = _cache_data[_pos - _cache_pos];
  _pos++;

  return data;
}

std::streamsize StreamImpl::read( size_t pos, unsigned char* data,
                                  std::streamsize maxlen, bool* hit_eof ) const
{
  if (hit_eof)
	  *hit_eof = false;
  // sanity checks
  if (!_entry)
	  return 0;
  if( !data )
	  return 0;
  if( maxlen == 0 )
	  return 0;
  if ((maxlen + pos) > _entry->size())
  {
	  maxlen = _entry->size() - pos;
	  if (hit_eof)
		  *hit_eof = true;
  }

  std::streamsize totalbytes = 0;
  
  if ( _entry->size() < _io->header()->threshold() )
  {
    // small file
    const ULONG32 small_block_size = _io->small_block_size();
    if( small_block_size == 0 )
      return 0;

    size_t index = pos / small_block_size;

    if( index >= _blocks.size() ) 
		return 0;

    unsigned char* buf = new unsigned char[ small_block_size ];
    size_t offset = pos % small_block_size;
    while( totalbytes < maxlen )
    {
      if( index >= _blocks.size() ) break;
      ULONG32 read = _io->loadSmallBlock( _blocks[index], buf, small_block_size );
	  if (read != small_block_size)
		  break;
      std::streamsize count = small_block_size - offset;
      if(count > (maxlen - totalbytes)) 
		  count = maxlen - totalbytes;
      memcpy(data + totalbytes, buf + offset, count);
      totalbytes += count;
      offset = 0;
      index++;
    }
    delete[] buf;

  }
  else
  {
    // big file
    const ULONG32 big_block_size = _io->big_block_size();
    if( big_block_size == 0 )
      return 0;

    size_t index = pos / big_block_size;
    
    if( index >= _blocks.size() ) 
		return 0;
    
    unsigned char* buf = new unsigned char[ big_block_size ];
    size_t offset = pos % big_block_size;
    while( totalbytes < maxlen )
    {
      if( index >= _blocks.size() ) break;
      ULONG32 read = _io->loadBigBlock( _blocks[index], buf, big_block_size );
	  if (read != big_block_size)
		  break;
      std::streamsize count = big_block_size - offset;
      if( count > maxlen-totalbytes ) count = maxlen-totalbytes;
      memcpy( data+totalbytes, buf + offset, count );
      totalbytes += count;
      index++;
      offset = 0;
    }
    delete [] buf;

  }

  return totalbytes;
}

std::streamsize StreamImpl::read( unsigned char* data, std::streamsize maxlen )
{
  bool hit_eof = false;
  std::streamsize bytes = read( (size_t)tell(), data, maxlen, &hit_eof );
  _pos += bytes;

  if (hit_eof)
	  _state |= StreamImpl::Eof;
  else
	  _state &= StreamImpl::Eof;

  if (_pos == _entry->size())
	  _state |= StreamImpl::Eof;

  return bytes;
}

void StreamImpl::update_cache()
{
  // sanity checks
  if (!_entry) 
	  return;
  if( !_cache_data ) 
	  return;

  _cache_pos = _pos - ( _pos % _cache_size );
  size_t bytes = _cache_size;
  if( _cache_pos + bytes > _entry->size() ) 
	  bytes = _entry->size() - _cache_pos;
  _cache_size = read( _cache_pos, _cache_data, bytes );
}

/*
para escribir:
1. El entry de cada stream contiene el bloque en que comienza, hay que cargarlos todos en un vector utilizando el follow de la clase AllocTable.
2. Para conocer en que bloque del stream se debe comenzar a escribir, esto se hace dividiendo el offset con respecto al inicio del stream entre el tama�o de cada bloque.
3. Para calcular el offset dentro de ese bloque se toma el reto de la divisi�n anterior.
4. Para conocer el desplazamiento f�sico donde hay que escribir en el fichero se suma el offset dentro del bloque y se suma con el offset del bloque en el que se va a escribir, este �ltimo se calcula como el (bloque*512)+512.
5. Para saber la cantidad de bytes afectados en un bloque dado se resta el tama�o del bloque entre el offset dentro del bloque. Si es menos, entonces todo pertenece a ese bloque, si es mayor, entonces se escriben los bytes desde el desplazamiento inicial hasta el final del bloque y el resto pertenece al bloque adyacente.
6. Es necesario incrementar el buffer de datos en la cantidad de bytes copiados y restar de la cantidad total de bytes lo que se copi�.
7. Repetir la operaci�n con otro bloque si es necesario.
*/
POLE::ULONG32 StreamImpl::write(const unsigned char* data, POLE::ULONG32 maxlen)
{
	// Sanity checks
	if(! _entry) 
		return 0;
	if(!data) 
		return 0;
	if(maxlen == 0) 
		return 0;
	if((maxlen + _pos) > _entry->size())
	{
		maxlen = (ULONG32)(_entry->size() - _pos);
		_state |= StreamImpl::Eof;
	}
	else
	{
		_state &= StreamImpl::Eof;
	}

	// Amount of written byes
	ULONG32 written = 0;
	ULONG32 count = 0;
	ULONG32 data_len = maxlen;
	
	if (_entry->size() < _io->header()->threshold())
	{// small file
		std::vector<ULONG32> _sbroot_entry = _io->sb_blocks();
		const ULONG32 small_block_size = _io->small_block_size();
		const ULONG32 big_block_size = _io->big_block_size();
		if( small_block_size == 0 || big_block_size == 0 )
			return 0;

		ULONG32 index = (ULONG32)(_pos / small_block_size);

		if(index > _sbroot_entry.size()) 
			return 0;

		ULONG32 offset = _pos % small_block_size;


		for (; ((index < _blocks.size()) && (count < maxlen)); ++index)
		{
			// Take the minifat sector index
			ULONG32 minifat_index = _blocks[index];
			ULONG32 sbindex_offset = minifat_index % 8;
			// Calculate the the root entry's big block index
			ULONG32 position = minifat_index * small_block_size;
			ULONG32 bbindex = position / big_block_size;
			// Fisical offset inside the file
			ULONG32 bbindice = _sbroot_entry[bbindex];
			ULONG32 fisical_offset = (((bbindice * big_block_size) + big_block_size) + 
								     (sbindex_offset * small_block_size)) + offset;

			// Amount of bytes that can actually be written
			ULONG32 canwrite = small_block_size - offset;
			if (canwrite > data_len )
				canwrite = data_len;

			written = _io->saveBlock(fisical_offset, data, canwrite);
			// HACK: In case of error what to do? quit?
			if (written < canwrite)
				return 0;
			data += written;
			data_len -= written;
			count += written;
			offset = 0;
		}
	}
	else
	{// big file
		// Ordinal of the first block for writing
		const ULONG32 big_block_size = _io->big_block_size();
		if( big_block_size == 0 )
			return 0;

		ULONG32 index = (ULONG32)(_pos / big_block_size);
		if(index > _blocks.size()) 
			return 0;
		// Offset inside this block
		ULONG32 offset = _pos % big_block_size;

		for (; ((index < _blocks.size()) && (count < maxlen)); ++index)
		{
			// Fisical offset inside the file
			ULONG32 fisical_offset = ((_blocks[index] * big_block_size) + big_block_size + offset);

			// Amount of bytes that can actually be written
			ULONG32 canwrite = big_block_size - offset;
			if (canwrite > data_len )
				canwrite = data_len;

			written = _io->saveBlock(fisical_offset, data, canwrite);
			// HACK: In case of error what to do? quit?
			if (written < canwrite)
				return 0;
			data += written;
			data_len -= written;
			count += written;
			offset = 0;
		}
	}
	return written;
}


}
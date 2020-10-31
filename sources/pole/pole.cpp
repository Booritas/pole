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


#pragma warning( disable : 4267 ) // conversion from 'size_t' to 'unsigned int'
#include "detail/header.cpp"
#include "detail/alloctable.cpp"
#include "detail/dirtree.cpp"
#include "detail/storage.cpp"
#include "detail/stream.cpp"
#pragma warning( default : 4267 ) // conversion from 'size_t' to 'unsigned int'

#include "../../includes/pole/pole.h"

namespace POLE
{

// =========== Storage ==========

Storage::Storage( const char* filename )
{
  io = new StorageIO( filename );
}

Storage::~Storage()
{
  delete io;
  std::list<Stream*>::iterator it;
  for( it = streams.begin(); it != streams.end(); ++it )
    delete *it;
}

int Storage::result() const
{
  return io->result();
}

void Storage::listEntries(std::vector<const DirEntry*>& result) const
{
	return io->listEntries(result);
}

// enters a sub-directory, returns false if not a directory or not found
bool Storage::enterDirectory( const std::string& directory )
{
	return io->enterDirectory( directory );
}

// goes up one level (like cd ..)
void Storage::leaveDirectory()
{
  io->leaveDirectory();
}

void Storage::path( std::string& result) const
{
  io->path(result);
}

Stream* Storage::stream( const std::string& name, bool reuse )
{
  // sanity check
  if( !name.length() ) return (Stream*)0;
  if( !io ) return (Stream*)0;

  // make absolute if necesary
  std::string fullName = name;
  std::string path_;
  path(path_);
  if( name[0] != '/' ) fullName.insert( 0, path_ + "/" );
  
  // If a stream for this path already exists return it
  if (reuse)
  {
	std::list<Stream*>::iterator it;
	for( it = streams.begin(); it != streams.end(); ++it )
		if ((*it)->path() == name)
			return *it;
  }

  const DirEntry* entry = io->entry( name );
  if( !entry ) return (Stream*)0;

  // We create the StreamImpl here instead of in the constructor to avoid 
  // passing implementation parameters in the constructor.
  // The created StreamImpl will be deleted in the Stream destructor.
  StreamImpl* i = new StreamImpl( io, entry );
  Stream* s = new Stream(i); 
  streams.push_back( s );
  
  return s;
}

void Storage::debug()
{
	io->debug();
}

bool Storage::delete_entry(const std::string& path) 
{ 
	if (io) 
		return io->delete_entry(path); 
	return false; 
}


// =========== Stream ==========

// FIXME tell parent storage we're gone
Stream::~Stream()
{
  delete impl;
}

// Returns the path for this stream.
const std::string& Stream::path() const
{
	return impl ? impl->path() : StreamImpl::null_path;
}

unsigned long Stream::tell() const
{
#pragma warning( disable : 4267 ) // conversion from 'size_t' to 'unsigned int'
  return impl ? (unsigned long)impl->tell() : 0;
#pragma warning( default : 4267 ) // conversion from 'size_t' to 'unsigned int'
}

bool Stream::eof() const 
{ 
	return impl ? impl->eof() : true; 
}

bool Stream::fail() const 
{ 
	return impl ? impl->fail() : true; 
}

void Stream::seek( unsigned long newpos )
{
  if( impl ) impl->seek( newpos );
}

unsigned long Stream::size() const
{
  return impl ? impl->size() : 0;
}

int Stream::getch()
{
  return impl ? impl->getch() : 0;
}

unsigned long Stream::read( unsigned char* data, unsigned long maxlen )
{
#pragma warning( disable : 4267 ) // conversion from 'size_t' to 'unsigned int'
  return impl ? (unsigned long)impl->read( data, maxlen ) : 0;
#pragma warning( default : 4267 ) // conversion from 'size_t' to 'unsigned int'
}

Stream& Stream::write(const unsigned char* data, POLE::ULONG32 len)
{
  //#pragma warning( disable : 4267 ) // conversion from 'size_t' to 'unsigned int'
	if (impl) 
		impl->write( data, len );

	return *this;
  //#pragma warning( default : 4267 ) // conversion from 'size_t' to 'unsigned int'
}

}

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

#ifndef POLE_H
#define POLE_H

#pragma once

#include <string>
#include <list>
#include <vector>
#include "./detail/util.hpp"

namespace POLE
{

class DirEntry;
class StorageIO;
class Stream;
class StreamImpl;

class Storage
{
public:
  enum { Ok, OpenFailed, NotOLE, BadOLE, UnknownError, StupidWorkaroundForBrokenCompiler=255 };

  // Constructs a storage with name filename.
  Storage( const char* filename );
#if defined(WIN32)
  Storage( const wchar_t* filename);
#endif
  // Destroys the storage.
  ~Storage();
// Attributes
public:
  // Returns the error code of last operation.
  int result() const;

  // Returns the current path.
  void path( std::string& result) const;

  void listEntries(std::vector<const DirEntry*>& result) const;

// Operations
public:
  // Changes path to directory. Returns true if no error occurs.
  bool enterDirectory( const std::string& directory );

  // Goes to one directory up.
  void leaveDirectory();

  // Finds and returns a stream with the specified name.
  Stream* stream( const std::string& name, bool reuse = false );

  bool delete_entry(const std::string& path);

  void debug();
  
private:
  StorageIO* io;
  std::list<Stream*> streams;
  
  // no copy or assign
  Storage( const Storage& );
  Storage& operator=( const Storage& );
};

class Stream
{
  friend class Storage;

public: // Attributes

  // Returns the path for this stream.
  const std::string& path() const;

  // Returns the stream size.
  unsigned long size() const;

  // Returns the read pointer.
  unsigned long tell() const;

  // Return the Eof state of the stream
  bool eof() const; 

  // Return the fail state of the Stream
  bool fail() const;

public: // Operations

  // Sets the read position.
  void seek( unsigned long pos ); 

  // Reads a byte.
  int getch();

  // Reads a block of data.
  unsigned long read( unsigned char* data, unsigned long maxlen );

  // Write a block of data
  Stream& write(const unsigned char* data, ULONG32 len);

private:
	Stream(StreamImpl* i) { impl = i; }
	~Stream();


  // no default, copy or assign
  Stream();
  Stream( const Stream& );
  Stream& operator=( const Stream& );
    
  StreamImpl* impl;
};


}

#endif // POLE_H

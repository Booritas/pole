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

// olestream header
#pragma once

#include "pole/pole.h"
#include <iostream>

namespace ole
{
	// Forward declaration
	class stream_path;

	class basic_stream
	{
	// Construction (only the paths can construct new streams)
	public:	
		basic_stream(const basic_stream& other) : _stream(other._stream) {}
	// Attributes
	public:
		bool fail() const { return _stream ? _stream->fail() : true; }
		bool eof() const { return _stream ? _stream->eof() : true; }
		bool operator == (const ole::basic_stream& other) const { return _stream == other._stream; }
		bool operator != (const ole::basic_stream& other) const { return _stream != other._stream; }
	
	// Operations
	public:
		std::streamsize read(char* buf, std::streamsize n) { return _stream ? _stream->read((unsigned char *)buf, n): 0; }
		ole::basic_stream& write(const char* buf, std::streamsize n) { _stream->write((unsigned char *)buf, n); return *this; }
		ole::basic_stream& operator=( const ole::basic_stream& other ) { _stream = other._stream; return *this; }
		std::streamoff seek(std::streamoff off, std::ios::seekdir way, std::ios::openmode which = std::ios::in) 
		{
			// Error. Is a correct return value in case of error?
			if (!_stream) return -1;

			std::streamoff _position = 0 ;
			switch (way)
			{
			case std::ios::cur:
					_position = _stream->tell() + off;
					break;
			case std::ios::beg: 
					_position = off;
					break;
			case std::ios::end:
					_position = _stream->size() - off;
					break;
			}
			if (_stream)
				_stream->seek(_position);

			return _stream->tell();
		}

	// Implementation
	private:
		friend class ole::stream_path; // To allow construction

		POLE::Stream* _stream;
		basic_stream(); // No default construction
		basic_stream(POLE::Stream* str) : _stream(str) {}
	};
}
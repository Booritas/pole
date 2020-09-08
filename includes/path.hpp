/* POLE - Portable C++ library to access OLE Storage 
   Copyright (C) 2004 Jorge Lodos Vigil <lodos@segurmatica.com>
   Copyright (C) 2004 Israel Fdez. Cabrera <israel@segurmatica.com>

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

// path header
#pragma once

#include <vector>
#include "stream.hpp"

namespace ole
{
	// Forward declaration
	class compound_document;

	class stream_path
	{
	// Construction (only the storage can construct new paths)
	public:
		stream_path(const ole::stream_path& other): 
		   _path(other._path), _stream(other._stream), _ref_count(0) {}
		stream_path(POLE::Stream* st, const std::string& src): 
		   _stream(st), _path(src), _ref_count(0) {}
		stream_path(POLE::Stream* st, const char* src):
		   _stream(st), _path(src), _ref_count(0) {}
	    
	// Attributes
	public:
		// Return the full path of the stream
		const std::string& string() const { return _path; }
		// Return only the name of the stream
		const std::string string_name() const 
		{ 
			size_t pos = _path.find_last_of("/");
			if (pos == 0 || pos == std::string::npos)
				return _path;
			return _path.substr(pos + 1, _path.size() - pos); 
		}

		// relational operators
		bool operator==( const stream_path& other ) const { return (_path == other._path && 
						_stream == other._stream); }
		bool operator!=( const stream_path& other ) const { return (_path != other._path || 
						_stream != other._stream); }
		// True is the contained stream is in use, if so the entry can't be delete.
		bool used() { return (_ref_count > 0); }
	
	// Operations
	public:
		// Return the contained stream
		// TODO: Que hacer en caso que se pase de 255??
		ole::basic_stream& stream() { _ref_count++; return _stream; }
		// Decrement the Reference Count varaible
		void close() { if (_ref_count > 0) _ref_count--; }

		ole::stream_path& operator=( const ole::stream_path& other ) { _path = other._path; _stream = other._stream; return *this; }

	// Implementation
	private:
		friend class ole::compound_document; // To allow construction
		friend class storage_path; // To allow construction

		ole::basic_stream _stream;
		std::string _path;
		unsigned char _ref_count;
		
	
		stream_path(); // no default construction
	};

	// Storage path contains child stream's paths as members
	class storage_path
	{
	// Constructor
	public:
		storage_path(const std::string& path_str):
		  _path(path_str) {}
    // Operations
	public:
		typedef std::vector<ole::stream_path>::iterator stream_iterator;
		std::vector<ole::stream_path>::iterator begin() { return _streams.begin(); }
		std::vector<ole::stream_path>::iterator end() { return _streams.end(); }
		void add_child(const ole::stream_path& path) { _streams.push_back(path); }
		// Find a stream in the storage
		std::vector<ole::stream_path>::iterator find_stream(const std::string& path)
		{
			for (std::vector<ole::stream_path>::iterator it = _streams.begin();
				it != _streams.end(); 
				++it)
			{
				if (it->string() == path)
					return it;
			}

			return _streams.end();
		}

		// Returns a specific stream iterator 
		std::vector<ole::stream_path>::iterator open_stream(const std::string& path)
		{ return find_stream(path); }
		// Find if a stream's path match with path
		bool path_exist(const std::string& path)
		{
			return (find_stream(path) != _streams.end());
		}
    // Attributes
	public:
		// Return the full path inside the document
		const std::string string() const { return _path; }
		// Return only the name of the storage
		const std::string string_name() const 
		{ 
			size_t pos = _path.find_last_of("/");
			if (pos == 0 || pos == std::string::npos)
				return _path;

			return _path.substr(pos + 1, _path.size() - pos); 
		}
	// Members
	private:
		friend class ole::compound_document; // To allow construction
		std::string _path;
		std::vector<ole::stream_path> _streams;
	};
}// end namespace ole

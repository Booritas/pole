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

/*
 compound_document represents an OLE Compound Document.
 This class may iterate all the document's storages in various ways based on preorder,
 postorder, and sibling iterators.
 Preorder and postorder can iterate the whole document's storages; siblig_iterator
 iterates only storages's child storages.
 Dereferencing any storage_iterator returns an storage_path, a concept tha encapsulate the
 name, path and child streams and streams iterators for the storage. See path.hpp for 
 more information.
 compound_document can also determine if a given storage or stream path exist through the 
 path_exist method.
 Entry deletion is also possible with delete_entry method.
*/

// compound_document header
#pragma once
// #include <vector>
#include "pole/detail/util.hpp"
#include "pole/pole.h"
#include "path.hpp"
#include "tree.hh"

namespace ole
{
	class compound_document
	{
	public:
		// storage_path iterator
		typedef tree<ole::storage_path>::iterator storage_iterator;
		typedef tree<ole::storage_path>::pre_order_iterator storage_preorder_iterator;
		typedef tree<ole::storage_path>::post_order_iterator storage_postorder_iterator;
		typedef tree<ole::storage_path>::sibling_iterator storage_sibling_iterator;
		// Construction/destruction
		compound_document(): _storage(NULL), _good(false) {}
		compound_document(const std::string& filename);
		~compound_document() { if (_storage) delete _storage; }

	// Attributes
	public:
		bool good() const { return _good; }		
		tree<ole::storage_path>::iterator begin() { return tree<ole::storage_path>::iterator(_storages.begin()); }
		tree<ole::storage_path>::iterator end() { return tree<ole::storage_path>::iterator(_storages.end()); }
		void debug(){_storage->debug();}
	// Operations
	public:
		// Find a storage in the document
		tree<ole::storage_path>::pre_order_iterator find_storage(const std::string& storage_path);
		// Returns a specific storage iterator
		tree<ole::storage_path>::pre_order_iterator open_storage(const std::string& storage_path)
		{ return find_storage(storage_path); }
		// To know if certain paths exist in the compound document
		bool path_exist(const std::string& path);
		// Delete an entry in the document bases on a string 
		bool delete_entry(const std::string& path) 
		{ 
			if (_storage && entry_can_be_deleted(path)) 
				return _storage->delete_entry(path); 
			return false; 
		}
		// Delete an entry in the document bases on an storage iterator
		bool delete_entry(const ole::compound_document::storage_iterator& entry) 
		{ return delete_entry(entry->string()); }
		// Delete an entry in the document bases on an stream iterator
		bool delete_entry(const ole::storage_path::stream_iterator entry) 
		{ return delete_entry(entry->string()); }
		

	// Implementation
	private:
		void init();
		void listEntries(std::vector<const POLE::DirEntry*>& result)
		{ if (_storage) _storage->listEntries(result); }
		// Determines if an entry identified by path can be deleted
		// A stream should not be deleted if there is a stream using it
		bool entry_can_be_deleted(const std::string& path);

		POLE::Storage* _storage;
		tree<ole::storage_path> _storages;
		bool _good;

		ole::compound_document& operator=(const ole::compound_document&); // no assignment operator
	};

	typedef ole::compound_document::storage_postorder_iterator storage_postorder_iterator;
	typedef ole::compound_document::storage_preorder_iterator storage_preorder_iterator;
	//Short name for a preorder iterator
	typedef tree<ole::storage_path>::iterator storage_iterator; 
	typedef ole::compound_document::storage_sibling_iterator storage_sibling_iterator;
	typedef ole::storage_path::stream_iterator stream_iterator;
}

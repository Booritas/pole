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


#include "../includes/pole/pole.h"

#include <algorithm>
#include <vector>

#include "../includes/pole/detail/dirtree.hpp"
#include "../includes/storage.hpp"

namespace ole
{
	namespace detail
	{
		std::vector<std::string> inc_splitstr(const std::string& str, const std::string& sep)
		{
			std::vector<std::string> vec;
			size_t pos = str.size();
			vec.push_back(str);

			while (pos != 0)
			{
				pos = str.find_last_of(sep, pos - 1);
				std::string s = str.substr(0, pos);
				if (s != sep && s != "")
					vec.push_back(s);
			}
			vec.push_back("/");
			return vec;
		}
	}// namespce detail

//=============compound_document===============

	compound_document::compound_document(const std::string& filename): _storage(NULL), _good(false) 
	{
		if (filename.empty())
			return;

		_storage = new POLE::Storage(filename.c_str());
		if (!_storage || _storage->result() != POLE::Storage::Ok)
			return;

		init(_storages.begin());
		/*if (_paths.size() == 0)
			return;*/
		_good = true;
	}

	void compound_document::init(tree<ole::storage_path>::sibling_iterator it, size_t index)
	{
		if (index == 0)
			it = _storages.insert(_storages.begin(), ole::storage_path("/"));

		std::vector<const POLE::DirEntry*> entries;
		_storage->listEntries(entries);

		for (std::vector<const POLE::DirEntry*>::iterator entry_it = entries.begin();
			entry_it != entries.end(); entry_it++)
		{
			std::string _defpath;
			_storage->path(_defpath);
			if (_defpath[_defpath.size() - 1] != '/')
				_defpath += "/";
			_defpath += (*entry_it)->name();
			if ((*entry_it)->type() == 1)
			{
				ole::storage_path p(_defpath);
				tree<ole::storage_path>::sibling_iterator _newit = _storages.append_child(it, p);
				_storage->enterDirectory(_defpath);
				init(_newit, ++index);
				_storage->leaveDirectory();
			}
			else
			{
				POLE::Stream* _defstream = _storage->stream(_defpath, true);
				(*it).add_child(ole::stream_path(_defstream, _defpath));
			}
		}
	}

	tree<ole::storage_path>::pre_order_iterator compound_document::find_storage(const std::string& storage_path)
	{
		ole::storage_preorder_iterator bit;
		for(bit = _storages.begin();
			bit != _storages.end();
			bit++)
		{
			if (bit->string() == storage_path)
				break;
		}

		return bit;
	}

	bool compound_document::path_exist(const std::string& path)
	{
		bool retval = false;
		ole::storage_preorder_iterator it = find_storage(path);
		if (it != _storages.end())// If the path match a storage
			return true;
		else// Path is not a storage, let try a "cd .." to find a storage again
		{
			std::string _child_tmp;
			size_t pos = pos = path.find_last_of("/");
			if (pos == 0)
			{
				if ((it = find_storage("/")) != _storages.end())
					return it->path_exist(path);
			}
			if (pos > 0)
			{
				_child_tmp = path.substr(0, path.size() - ++pos);
				if ((it = find_storage(_child_tmp)) != _storages.end())
				{
					return it->path_exist(path);
				}
			}
		}
		
		return retval;
	}

	bool compound_document::entry_can_be_deleted(const std::string& path)
	{
		bool retval = false;
		ole::storage_preorder_iterator it = find_storage(path);
		if (it != _storages.end())// If the path match a storage
			return true;
		else// Path is not a storage, let try a "cd .." to find a storage again
		{
			std::string _child_tmp;
			size_t pos = pos = path.find_last_of("/");
			if (pos == 0)
			{
				if ((it = find_storage("/")) != _storages.end())
				{
					for (ole::stream_iterator sit = it->begin();
						sit != it->end(); sit++)
					{
						if (sit->string() == path)
							return !sit->used();
					}
				}
			}
			if (pos > 0)
			{
				_child_tmp = path.substr(0, path.size() - pos);
				if ((it = find_storage(_child_tmp)) != _storages.end())
				{
					for (ole::stream_iterator sit = it->begin();
						sit != it->end(); sit++)
					{
						if (sit->string() == path)
							return !sit->used();
					}
				}
			}
		}
		
		return retval;
	}
}
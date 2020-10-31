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


#include <list>
#include <iostream>
#include "../../../includes/pole/detail/util.hpp"
#include "../../../includes/pole/detail/dirtree.hpp"

namespace POLE
{

const unsigned DirEntry::End = 0xffffffff;

bool DirEntry::valid() const
{ 
	//if ((_type == 1 ) && (_size != 0 ))
	//	return false;
	if ((_type != 1 ) && (_type != 2 ) && (_type != 5 ))
		return false;
	if (_name.empty())
		return false;

	return true;
}

// =========== DirTree ==========

void DirTree::clear()
{
  // leave only the root entry
  _entries.resize( 1 );
  _current = 0;
}

const DirEntry* DirTree::entry( size_t index ) const
{
  if( index >= entryCount() ) return NULL;
  return &_entries[ index ];
}

DirEntry* DirTree::_entry( size_t index )
{
  if( index >= entryCount() ) return NULL;
  return &_entries[ index ];
}

size_t DirTree::indexOf( const DirEntry* e ) const
{
  for( size_t i = 0; i < entryCount(); i++ )
    if( entry( i ) == e ) return i;
    
  return -1;
}

size_t DirTree::parent( size_t index ) const
{
	// brute-force, basically we iterate for each entries, find its children
	// and check if one of the children is 'index'
	for( size_t j = 0; j < entryCount(); j++ )
	{
		/*DirEntry e = _entries[j];
		if (e.child() == index)
			return j;*/
		std::vector<size_t> chi;
		children( j, chi );
		for( size_t i=0; i<chi.size();i++ )
		if( chi[i] == index )
			return j;
	}
	        
	return -1;
}

void DirTree::fullName( size_t index, std::string& result ) const
{
	// don't use root name ("Root Entry"), just give "/"
	if( index == 0 )
	{
		result = "/";
		return;
	}

	result = entry( index )->name();
	result.insert( 0,  "/" );
	size_t p = parent( index );
	if (p == -1)
		return;
	const DirEntry * _entry = 0;
	while( p > 0 )
	{
		_entry = entry( p );
		if (_entry && _entry->dir() && _entry->valid())
		{
		result.insert( 0,  _entry->name());
		result.insert( 0,  "/" );
		}
		p = parent(p);
		if (p == -1)
			break;
	}
}

// given a fullname (e.g "/ObjectPool/_1020961869"), find the entry
// if not found and create is false, return 0
// if create is true, a new entry is returned
DirEntry* DirTree::_entry( const std::string& name, bool create )
{
	if( name.empty() ) return NULL;
   
   // quick check for "/" (that's root)
   if( name == "/" ) return &_entries[0];
   
   // split the names, e.g  "/ObjectPool/_1020961869" will become:
   // "ObjectPool" and "_1020961869" 
   std::list<std::string> names;
   std::string::size_type start = 0, end = 0;
   while( start < name.length() )
   {
     end = name.find_first_of( '/', start );
	 if (end == 0)
	 {
		 ++start;
		 continue;
	 }
     if( end == std::string::npos ) end = name.length();
     names.push_back( name.substr( start, end-start ) );
     start = end+1;
   }
  
   // start from root when name is absolute
   // or current directory when name is relative
   size_t index = (name[0] == '/' ) ? 0 : _current;

   // trace one by one   
   std::list<std::string>::iterator it; 
   for( it = names.begin(); it != names.end(); ++it )
   {
     // find among the children of index
     std::vector<size_t> chi;
	 children( index, chi );
     size_t child = 0;
     for( size_t i = 0; i < chi.size(); i++ )
     {
       const DirEntry* ce = entry( chi[i] );
       if( ce ) 
         if( ce->valid() && ( ce->name().length()>1 ) )
		 {
           if( ce->name() == *it )
		   {
             child = chi[i];
			 break;
		   }
		 }
     }
     
     // traverse to the child
     if( child > 0 ) index = child;
     else
     {
       // not found among children
       if( !create ) return NULL;
       
       // create a new entry
       DirEntry* parent = &_entries[index];
	   DirEntry e(*it, (ULONG16)(((*it).size()*2) + 2), 2, 0, 0, DirEntry::End, DirEntry::End, parent->child(), entryCount() + 1);
	   _entries.push_back( e );
       index = entryCount()-1;
       parent->set_child((ULONG32)index);
     }
   }

   return _entry( index );
}

void DirTree::children( size_t index, std::vector<size_t>& result ) const
{ 
  const DirEntry* e = entry( index );
  if( e && ( e->valid() && e->child() < entryCount() ) )
    find_siblings( result, e->child() );
}

void DirTree::listDirectory(std::vector<const DirEntry*>& result) const
{
  std::vector<size_t> chi;
  children( _current, chi );
  for( unsigned i = 0; i < chi.size(); i++ )
    result.push_back( entry( chi[i] ) );
}

bool DirTree::enterDirectory( const std::string& dir )
{
  const DirEntry* e = entry( dir );
  if( !e ) 
	  return false;
  if( !e->valid() ) 
	  return false;
  if( !e->dir() ) 
	  return false;
  
  size_t index = e->index();
  if( index == -1 ) return false;
    
  _current = index;
  return true;
}

void DirTree::leaveDirectory()
{
  // already at root ?
  if( _current == 0 ) return;

  size_t p = parent( _current );
  if( p != -1 ) _current = p;
}

bool DirTree::load( unsigned char* buffer, size_t size )
{
  _entries.clear();
  _current = 0;
  size_t init_count = size / 128;
  
  for( unsigned i = 0; i < init_count; i++ )
  {
    unsigned p = i * 128;
    
    // would be < 32 if first char in the name isn't printable
    unsigned prefix = 32;
    
    // parse name of this entry, which stored as Unicode 16-bit
    std::string name;
	// Name's length 
    int name_len = readU16( buffer + 0x40+p );
	// Read name char by char
    if( name_len > 64 ) name_len = 64;
    for( int j=0; ( buffer[j + p]) && (j < name_len); j+= 2 )
      name.append( 1, buffer[j + p] );
      
    // first char isn't printable ? remove it...
    /*if( buffer[p] < 32 )
    { 
      prefix = buffer[0]; 
      name.erase( 0,1 ); 
    }*/
    
    // 1 = directory (aka storage), 2 = file (aka stream),  5 = root
    ULONG8 type = buffer[ 0x42 + p];
	ULONG16 len = readU16( buffer + 0x40+p );
    ULONG32 start = readU32( buffer + 0x74+p );
    ULONG32 size = readU32( buffer + 0x78+p );
    ULONG32 prev = readU32( buffer + 0x44+p );
    ULONG32 next = readU32( buffer + 0x48+p );
    ULONG32 child = readU32( buffer + 0x4C+p );
    
	DirEntry e(name, len, type, size, start, prev, next, child, _entries.size());

	_entries.push_back( e );
	
  }  
  return true;
}

bool DirTree::save( unsigned char* buffer, size_t len )
{
  size_t size = 128*entryCount();
  if (len < size)
    return false;
  
  memset( buffer, 0, size );
  
  for( unsigned i = 0; i < entryCount(); i++ )
  {
    const DirEntry* e = entry( i );
    if( !e ) return false;
	//if (!e->modified()) continue;
    
    // max length for name is 32 chars
    std::string name = e->name();
    if( name.length() > 32 )
      name.erase( 32, name.length() );
      
    // write name as Unicode 16-bit
    for( unsigned j = 0; j < name.length(); j++ )
      buffer[ i*128 + j*2 ] = name[j];

    writeU16( buffer + i*128 + 0x40, (ULONG16)(name.length()*2 + 2) );    
    writeU32( buffer + i*128 + 0x74, e->start() );
    writeU32( buffer + i*128 + 0x78, e->size() );
    writeU32( buffer + i*128 + 0x44, e->prev() );
    writeU32( buffer + i*128 + 0x48, e->next() );
    writeU32( buffer + i*128 + 0x4c, e->child() );
    buffer[ i*128 + 0x42 ] = e->type();
    buffer[ i*128 + 0x43 ] = 1; // always black
  }  
  return true;
}

void DirTree::debug()
{
  for( unsigned i = 0; i < entryCount(); i++ )
  {
    const DirEntry* e = entry( i );
    if( !e ) continue;
    std::cout << i << ": ";
    if( !e->valid() ) std::cout << "INVALID ";
    std::cout << e->name() << " ";
    if( e->dir() ) std::cout << "(Dir) ";
    else std::cout << "(File) ";
    std::cout << e->size() << " ";
    std::cout << "s:" << e->start() << " ";
    std::cout << "(";
	if( e->child() == DirEntry::End ) std::cout << "-"; else std::cout << e->child();
    std::cout << " ";
	if( e->prev() == DirEntry::End ) std::cout << "-"; else std::cout << e->prev();
    std::cout << ":";
	if( e->next() == DirEntry::End ) std::cout << "-"; else std::cout << e->next();
    std::cout << ")";    
    std::cout << std::endl;
  }
  std::vector<size_t> res;
  children(0, res);
  std::cout << std::endl << std::endl << "--------------------------" << std::endl;
  for (unsigned int i = 0; i < res.size(); ++i)
  {
	  std::cout << res[i] << std::endl;
  }
}

// helper function: recursively find siblings of index
void DirTree::find_siblings( std::vector<size_t>& result, ULONG32 index ) const
{
  const DirEntry* e = entry( index );
  if( !e ) return;
  if( !e->valid() ) return;

  // prevent infinite loop  
  for( unsigned i = 0; i < result.size(); i++ )
    if( result[i] == index ) return;

  // add myself    
  result.push_back( index );
  
  // visit previous sibling, don't go infinitely
  ULONG32 prev = e->prev();
  if( ( prev > 0 ) && ( prev < entryCount() ) )
  {
    for( unsigned i = 0; i < result.size(); i++ )
      if( result[i] == prev ) prev = 0;
    if( prev ) find_siblings( result, prev );
  }
    
  // visit next sibling, don't go infinitely
  ULONG32 next = e->next();
  if( ( next > 0 ) && ( next < entryCount() ) )
  {
    for( unsigned i = 0; i < result.size(); i++ )
      if( result[i] == next ) next = 0;
    if( next ) find_siblings( result, next );

  }
}

size_t DirTree::search_prev_link( size_t _entry )
{
	// Find parent
	size_t par_index = parent(_entry);
	if (par_index == -1)
		return false;
	if (_entries[par_index].child() == _entry)
		return par_index;
	else
	{
		std::vector<size_t> brothers;
		children(par_index, brothers);
		if (brothers.size() == 0)
			return false;
		for (size_t ndx = 0; ndx < brothers.size(); ++ndx)
		{
			if (_entries[brothers[ndx]].next() == _entry || 
				_entries[brothers[ndx]].prev() == _entry)
			{
				return brothers[ndx];
			}
		}
	}

	return -1;
}

bool DirTree::set_prev_link(size_t prev_link, size_t entry, ULONG32 value)
{
	DirEntry *pl = _entry(prev_link);
	if (!pl) return false;

	if (pl->prev() == entry)
		pl->set_prev(value);
	if (pl->next() == entry)
		pl->set_next(value);
	if (pl->child() == entry)
		pl->set_child(value);

	return true;
}

size_t DirTree::find_rightmost_sibling(size_t sib)
{
	DirEntry * _ent = _entry(sib);
	if (!_ent)
		return -1;
	if (_ent->prev() == DirEntry::End)
		return _ent->index();
	else
		return find_rightmost_sibling(_ent->prev());

	return -1;
}

bool DirTree::delete_entry(const std::string& path, int level)
{
	// Deletion is not posible over Root Entry
	if (path == "/")
		return false;
	DirEntry *e = _entry(path);
	// Hack: Should an invalid path throw and exception?
	if (!e)
		return false;
	// Is a Storgage?
 	if (e->type() == 1)
	{
		// Has Child?
		if (e->child() != DirEntry::End)
		{
			DirEntry *child = _entry(e->child());
			std::string _child_path = path + ((path[path.size()] == '/') ? child->name() : std::string("/") + child->name());
			level++;
			if (!delete_entry(_child_path, level))
				return false;
			e->set_child(DirEntry::End);
			level--;
		}
	}
	// This is not the first call. Searching for brothers
	if (level)
	{
		// Has right sibling?
		if (e->prev() != DirEntry::End)
		{
			// Find the previous entry
			DirEntry *pe = _entry(e->prev());
			if (!pe)
				return false;
			std::string _child_path;
			std::string::size_type _pos = path.find_last_of("/");

			if (_pos != std::string::npos)
				_child_path = path.substr(0, path.size() - ((path.size() - _pos) - 1)) + pe->name();
			else
				return false;

			level++;
			if (!delete_entry(_child_path, level))
				return false;
			e->set_prev(DirEntry::End);
			level--;
		}
		// Has left sibling?
		if (e->next() != DirEntry::End)
		{
			// Find the previous entry
			DirEntry *pe = _entry(e->next());
			if (!pe)
				return false;
			std::string _child_path;
			std::string::size_type _pos = path.find_last_of("/");

			if (_pos != std::string::npos)
				_child_path = path.substr(0, path.size() - ((path.size() - _pos) - 1)) + pe->name();
			else
				return false;

			level++;
			if (!delete_entry(_child_path, level))
				return false;
			e->set_next(DirEntry::End);
			level--;
		}
	}

	// Delete the entry
	if (level == 0)
		// Decrement and delete
		//level--;
	//else
	{// This is not an recursive call
		// Find the entry that points the entry is being deleted
		size_t prev_link = search_prev_link(e->index());
		if (prev_link == -1)
			return false;
		// If is the last entry...
		if (e->next() == DirEntry::End &&
			e->prev() == DirEntry::End)
		{
			// Set the end of chain mark in the entry that point to the entry
			// is being deleted
			if (!set_prev_link(prev_link, e->index(), DirEntry::End))
				return false;
		}
		else 
		{
			// It is not the last one, but has a brother
			if(e->next() == DirEntry::End ||
			   e->prev() == DirEntry::End)
			{
				// If hasn't a previous brother, set previous link entry to point
				// the next field of the entry is being deleted
				if (e->prev() == DirEntry::End)
				{
					if (!set_prev_link(prev_link, e->index(), e->next()))
						return false;
				}
				else
					// If hasn't a next brother, set previous link entry to point
					// the prev field of the entry is being deleted
					if (!set_prev_link(prev_link, e->index(), e->prev()))
						return false;
			}
			else
			{
				// If has a previous and a next brother, find the right most sibling
				// pointed by the next field of the entry is being deleted, set this entry's previous field
				// to point the prev field of the entry is being deleted.
				// Then set previous link entry to point the next field of the entry is being deleted.
				size_t right_most = find_rightmost_sibling(e->next());
				if (right_most == -1)
					return false;
				DirEntry *_right = _entry(right_most);
				if (!_right) return false;
				_right->set_prev(e->prev());

				if (!set_prev_link(prev_link, e->index(), e->next()))
					return false;
			}
		}
	}
	e->set("", 0, 0, 0, 0, DirEntry::End, DirEntry::End, DirEntry::End, 0, true);

	return true;
}

}
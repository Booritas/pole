/* POLE++ - Portable C++ library to access OLE Storage 
   Copyright (C) 2002-2004 Jorge Lodos Vigil <lodos@segurmatica.com>
   Copyright (C) 2002-2004 Israel Fdez. Cabrera <israel@segurmatica.com>

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

//pole++ main source file
//
// This used to be the single translation unit of the library: it #included
// pole/pole.cpp and storage.cpp, which in turn #included the five files under
// pole/detail/. All of those are listed in sources/CMakeLists.txt and so were
// also compiled on their own, leaving every symbol in the archive two or three
// times and the linker reporting 152 LNK4006s. The standalone objects were the
// ones it kept, so they are now the only ones: each source file is its own
// translation unit and this aggregator is no longer part of the build.

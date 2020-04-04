////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   types.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __TYPES_H__
#define __TYPES_H__

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------Basic Types--------------------------------------------------//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef signed char        S8;      ///< Compiler independent Signed Char
typedef unsigned char      U8;      ///< Compiler independent Unsigned Char

typedef signed short       S16;     ///< Compiler independent Signed 16-bit short
typedef unsigned short     U16;     ///< Compiler independent Unsigned 16-bit short

typedef signed int         S32;     ///< Compiler independent Signed 32-bit integer
typedef unsigned int       U32;     ///< Compiler independent Unsigned 32-bit integer

typedef signed long        S64;     ///< Compiler independent Signed 64-bit integer
typedef unsigned long      U64;     ///< Compiler independent Unsigned 64-bit integer

typedef float              F32;     ///< Compiler independent 32-bit float
typedef double             F64;     ///< Compiler independent 64-bit float

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------String Types--------------------------------------------------//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef char           UTF8;        ///< Compiler independent 8  bit Unicode encoded character
typedef const char*    StringEntry;

#endif //__TYPES_H__

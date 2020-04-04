////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   LibraryTemplates.hpp
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __LIBRARY_TEMPLATE_H__
#define __LIBRARY_TEMPLATE_H__

/*
=============
Q_vsnprintf
=============
*/
#if defined (_WIN32)
template<typename T, typename Y, typename P>
size_t Q_vsnprintf( T* str, Y size, P format, va_list ap )
{
    size_t len;
    
    len = _vsnprintf( str, size, format, ap );
    str[size - 1] = '\0';
#ifdef _DEBUG
    if ( len == -1 )
    {
        Com_Printf( "Q_vsnprintf: string (%.32s...) was truncated (%i) - target buffer too small ( %Iu )\n", str, len, size );
    }
#endif
    
    return len;
}
#endif

/*
=================
Q_strncpyz
=================
*/
template<typename T, typename Y, typename P>
void Q_strncpyz( T* dest, Y src, P destsize )
{
    if ( !dest )
    {
        Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
    }
    
    if ( !src )
    {
        Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
    }
    
    if ( destsize < 1 )
    {
        Com_Error( ERR_FATAL, "Q_strncpyz: destsize < 1" );
    }
    
    ::strncpy( dest, src, destsize - 1 );
    dest[destsize - 1] = 0;
}

/*
=================
Q_strcat
=================
*/
template<typename T, typename Y, typename P>
void Q_strcat( T* dest, Y destsize, P format ... )
{
    size_t l1;
    va_list argptr;
    UTF8 text[1024];
    
    va_start( argptr, format );
    Q_vsnprintf( text, sizeof( text ), format, argptr );
    va_end( argptr );
    
    l1 = ::strlen( dest );
    
    if ( l1 >= destsize )
    {
        Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
    }
    
    Q_strncpyz( dest + l1, text, destsize - l1 );
}

/*
=============
Q_snprintf
=============
*/
template<typename T, typename Y, typename P>
size_t Q_snprintf( T* dest, Y destsize, P format, ... )
{
    size_t len;
    va_list argptr;
    
    va_start( argptr, format );
    len = Q_vsnprintf( dest, destsize, format, argptr );
    va_end( argptr );
    
    if ( len >= destsize )
    {
        Com_Printf( "Q_snprintf: Output length %d too short, require %d bytes.\n", destsize, len + 1 );
    }
    
    if ( len == -1 )
    {
        Com_Printf( "Q_snprintf: overflow of %i bytes buffer\n", destsize );
    }
    
    return len;
}

#endif //!__LIBRARY_TEMPLATE_H__

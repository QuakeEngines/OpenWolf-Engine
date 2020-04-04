////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 id Software, Inc.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   r_glimp.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_GLIMP_H__
#define __R_GLIMP_H__

#pragma once

typedef enum
{
    RSERR_OK,
    
    RSERR_INVALID_FULLSCREEN,
    RSERR_INVALID_MODE,
    
    RSERR_UNKNOWN
} rserr_t;

#define R_MODE_FALLBACK 3 // 640 * 480

//
// idRenderSystemGlimpLocal
//
class idRenderSystemGlimpLocal
{
public:
    idRenderSystemGlimpLocal();
    ~idRenderSystemGlimpLocal();
    
    static void Minimize( void );
    static void LogComment( StringEntry comment );
    static S32 CompareModes( const void* a, const void* b );
    static void DetectAvailableModes( void );
    static bool GetProcAddresses( bool fixedFunction );
    static void ClearProcAddresses( void );
    static S32 SetMode( S32 mode, bool fullscreen, bool noborder, bool fixedFunction );
    static bool StartDriverAndSetMode( S32 mode, bool fullscreen, bool noborder, bool gl3Core );
    static bool HaveExtension( StringEntry ext );
    static void InitExtensions( void );
    static void Splash( void );
    static void Init( bool fixedFunction );
    static void EndFrame( void );
    static void Shutdown( void );
};

extern idRenderSystemGlimpLocal renderSystemGlimpLocal;

#endif //!__R_GLIMP_H__
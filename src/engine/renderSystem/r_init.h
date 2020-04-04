////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
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
// File name:   r_image_jpg.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_INIT_H__
#define __R_INIT_H__

#pragma once

//
// idRenderSystemInitLocal
//
class idRenderSystemInitLocal
{
public:
    idRenderSystemInitLocal();
    ~idRenderSystemInitLocal();
    
    static void InitOpenGL( void );
    static void CheckErrors( StringEntry file, S32 line );
    static bool GetModeInfo( S32* width, S32* height, F32* windowAspect, S32 mode );
    static void ModeList_f( void );
    static U8* ReadPixels( S32 x, S32 y, S32 width, S32 height, U64* offset, S32* padlen );
    static void TakeScreenshot( S32 x, S32 y, S32 width, S32 height, UTF8* fileName );
    static void TakeScreenshotJPEG( S32 x, S32 y, S32 width, S32 height, UTF8* fileName );
    static const void* TakeScreenshotCmd( const void* data );
    static void TakeScreenshot( S32 x, S32 y, S32 width, S32 height, UTF8* name, bool jpeg );
    static void ScreenshotFilename( S32 lastNumber, UTF8* fileName );
    static void ScreenshotFilenameJPEG( S32 lastNumber, UTF8* fileName );
    static void LevelShot( void );
    static void ScreenShot_f( void );
    static void ScreenShotJPEG_f( void );
    static void ExportCubemaps( void );
    static void ExportCubemaps_f( void );
    static const void* TakeVideoFrameCmd( const void* data );
    static void SetDefaultState( void );
    static void PrintLongString( StringEntry string );
    static void GfxInfo_f( void );
    static void Register( void );
    static void InitQueries( void );
    static void ShutDownQueries( void );
    static void Init( void );
};

extern idRenderSystemInitLocal renderSystemInitLocal;

#endif //!__R_INIT_H__
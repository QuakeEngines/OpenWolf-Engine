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
// File name:   r_backend.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_BACKEND_H__
#define __R_BACKEND_H__

//
// idRenderSystemBackendLocal
//
class idRenderSystemBackendLocal
{
public:
    idRenderSystemBackendLocal();
    ~idRenderSystemBackendLocal();
    
    static void BindToTMU( image_t* image, S32 tmu );
    static void Cull( S32 cullType );
    static void State( U64 stateBits );
    static void SetProjectionMatrix( mat4_t matrix );
    static void SetModelviewMatrix( mat4_t matrix );
    static void Hyperspace( void );
    static void SetViewportAndScissor( void );
    static void BeginDrawingView( void );
    static void RenderDrawSurfList( drawSurf_t* drawSurfs, S32 numDrawSurfs );
    static void SetGL2D( void );
    static const void* SetColor( const void* data );
    static const void* StretchPic( const void* data );
    static const void* PrefilterEnvMap( const void* data );
    static const void* DrawSurfs( const void* data );
    static const void* DrawBuffer( const void* data );
    static void ShowImages( void );
    static const void* ColorMask( const void* data );
    static const void* ClearDepth( const void* data );
    static const void* SwapBuffers( const void* data );
    static const void* PostProcess( const void* data );
    static const void* ExportCubemaps( const void* data );
    static const void* Finish( const void* data );
    static void ExecuteRenderCommands( const void* data );
};

extern idRenderSystemBackendLocal renderSystemBackendLocal;

#endif //!__R_ANIMATION_H__

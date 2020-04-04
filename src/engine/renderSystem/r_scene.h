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
// File name:   r_sky.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_SCENE_H__
#define __R_SCENE_H__

#pragma once

//
// idRenderSystemSceneLocal
//
class idRenderSystemSceneLocal
{
public:
    idRenderSystemSceneLocal();
    ~idRenderSystemSceneLocal();
    
    static void InitNextFrame( void );
    static void AddPolygonSurfaces( void );
    static void AddDynamicLightToScene( const vec3_t org, F32 intensity, F32 r, F32 g, F32 b, S32 additive );
    static void BeginScene( const refdef_t* fd );
    static void EndScene( void );
};

extern idRenderSystemSceneLocal renderSystemSceneLocal;

#endif //!__R_SCENE_H__

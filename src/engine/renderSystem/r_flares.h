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
// File name:   r_flares.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_FLARES_H__
#define __R_FLARES_H__

#pragma once

// flare states maintain visibility over multiple frames for fading
// layers: view, mirror, menu
typedef struct flare_s
{
    struct flare_s* next; // for active chain
    S32 addedFrame;
    S32 windowX, windowY;
    S32 frameSceneNum;
    S32 fogNum;
    S32 fadeTime;
    F32 drawIntensity;		// may be non 0 even if !visible due to fading
    F32 eyeZ;
    vec3_t origin;
    vec3_t color;
    bool visible; // state of last test
    bool inPortal; // true if in a portal view of the scene
    void* surface;
} flare_t;

#define	MAX_FLARES 128

//
// idRenderSystemFlaresLocal
//
class idRenderSystemFlaresLocal
{
public:
    idRenderSystemFlaresLocal();
    ~idRenderSystemFlaresLocal();
    
    static void SetFlareCoeff( void );
    static void ClearFlares( void );
    static void AddFlare( void* surface, S32 fogNum, vec3_t point, vec3_t color, vec3_t normal );
    static void AddDlightFlares( void );
    static void TestFlare( flare_t* f );
    static void RenderFlare( flare_t* f );
    static void RenderFlares( void );
};

extern idRenderSystemFlaresLocal renderSystemFlaresLocal;

#endif //!__R_FLARES_H__
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

#ifndef __R_SKY_H__
#define __R_SKY_H__

#pragma once

#define SKY_SUBDIVISIONS		8
#define HALF_SKY_SUBDIVISIONS	(SKY_SUBDIVISIONS/2)

static F32 s_cloudTexCoords[6][SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1][2];
static F32 s_cloudTexP[6][SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1];

static vec3_t sky_clip[6] =
{
    {1, 1, 0},
    {1, -1, 0},
    {0, -1, 1},
    {0, 1, 1},
    {1, 0, 1},
    { -1, 0, 1}
};

static F32 sky_mins[2][6], sky_maxs[2][6];
static F32 sky_min, sky_max;

#define	ON_EPSILON		0.1f			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64

static vec3_t s_skyPoints[SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1];
static F32 s_skyTexCoords[SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1][2];
static S32 sky_texorder[6] = {0, 2, 1, 3, 4, 5};

//
// idRenderSystemSkyLocal
//
class idRenderSystemSkyLocal
{
public:
    idRenderSystemSkyLocal();
    ~idRenderSystemSkyLocal();
    
    static void ClipSkyPolygon( S32 nump, vec3_t vecs, S32 stage );
    static void ClearSkyBox( void );
    static void AddSkyPolygon( S32 nump, vec3_t vecs );
    static void MakeSkyVec( F32 s, F32 t, S32 axis, F32 outSt[2], vec3_t outXYZ );
    static void DrawSkySide( struct image_s* image, const S32 mins[2], const S32 maxs[2] );
    static void DrawSkyBox( shader_t* shader );
    static void FillCloudySkySide( const S32 mins[2], const S32 maxs[2], bool addIndexes );
    static void FillCloudBox( const shader_t* shader, S32 stage );
    static void ClipSkyPolygons( shaderCommands_t* input );
    static void BuildCloudData( shaderCommands_t* input );
    static void InitSkyTexCoords( F32 heightCloud );
    static void DrawSun( F32 scale, shader_t* shader );
    static void StageIteratorSky( void );
};

extern idRenderSystemSkyLocal renderSystemSkyLocal;

#endif //!__R_SKY_H__

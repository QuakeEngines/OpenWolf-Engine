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

#ifndef __R_SURFACE_H__
#define __R_SURFACE_H__

#pragma once

//
// idRenderSystemSurfaceLocal
//
class idRenderSystemSurfaceLocal
{
public:
    idRenderSystemSurfaceLocal();
    ~idRenderSystemSurfaceLocal();
    
    static void CheckOverflow( S32 verts, S32 indexes );
    static void CheckVao( vao_t* vao );
    static void AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, F32 color[4], F32 s1, F32 t1, F32 s2, F32 t2 );
    static void AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, F32 color[4] );
    static void InstantQuad2( vec4_t quadVerts[4], vec2_t texCoords[4] );
    static void InstantQuad( vec4_t quadVerts[4] );
    static void SurfaceSprite( void );
    static void SurfacePolychain( srfPoly_t* p );
    static void SurfaceVertsAndIndexes( S32 numVerts, srfVert_t* verts, S32 numIndexes, U32* indexes, S32 dlightBits, S32 pshadowBits );
    static bool SurfaceVaoCached( S32 numVerts, srfVert_t* verts, S32 numIndexes, U32* indexes, S32 dlightBits, S32 pshadowBits );
    static void SurfaceTriangles( srfBspSurface_t* srf );
    static void SurfaceBeam( void );
    static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, F32 len, F32 spanWidth );
    static void DoRailDiscs( S32 numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up );
    static void SurfaceRailRings( void );
    static void SurfaceRailCore( void );
    static void SurfaceLightningBolt( void );
    static void LerpMeshVertexes( mdvSurface_t* surf, F32 backlerp );
    static void SurfaceMesh( mdvSurface_t* surface );
    static void SurfaceFace( srfBspSurface_t* srf );
    static F32 LodErrorForVolume( vec3_t local, F32 radius );
    static void SurfaceGrid( srfBspSurface_t* srf );
    static void SurfaceAxis( void );
    static void SurfaceEntity( surfaceType_t* surfType );
    static void SurfaceBad( surfaceType_t* surfType );
    static void SurfaceFlare( srfFlare_t* surf );
    static void SurfaceVaoMdvMesh( srfVaoMdvMesh_t* surface );
    static void SurfaceSkip( void* surf );
};

extern idRenderSystemSurfaceLocal renderSystemSurfaceLocal;

#endif //!__R_SURFACE_H__

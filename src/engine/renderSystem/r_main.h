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

#ifndef __R_MAIN_H__
#define __R_MAIN_H__

#pragma once

//
// idRenderSystemMainLocal
//
class idRenderSystemMainLocal
{
public:
    idRenderSystemMainLocal();
    ~idRenderSystemMainLocal();
    
    static bool CompareVert( srfVert_t* v1, srfVert_t* v2, bool checkST );
    static void CalcTexDirs( vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3 );
    static F32 CalcTangentSpace( vec3_t tangent, vec3_t bitangent, const vec3_t normal, const vec3_t sdir, const vec3_t tdir );
    static bool CalcTangentVectors( srfVert_t* dv[3] );
    static S32 CullLocalBox( vec3_t localBounds[2] );
    static S32 CullBox( vec3_t worldBounds[2] );
    static S32 CullLocalPointAndRadius( const vec3_t pt, F32 radius );
    static S32 CullPointAndRadiusEx( const vec3_t pt, F32 radius, const cplane_t* frustum, S32 numPlanes );
    static S32 CullPointAndRadius( const vec3_t pt, F32 radius );
    static void LocalNormalToWorld( const vec3_t local, vec3_t world );
    static void LocalPointToWorld( const vec3_t local, vec3_t world );
    static void WorldToLocal( const vec3_t world, vec3_t local );
    static void TransformModelToClip( const vec3_t src, const F32* modelMatrix, const F32* projectionMatrix, vec4_t eye, vec4_t dst );
    static void TransformClipToWindow( const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window );
    static void myGlMultMatrix( const F32* a, const F32* b, F32* out );
    static void RotateForEntity( const trRefEntity_t* ent, const viewParms_t* viewParms, orientationr_t* orientation );
    static void RotateForViewer( void );
    static void SetFarClip( void );
    static void SetupFrustum( viewParms_t* dest, F32 xmin, F32 xmax, F32 ymax, F32 zProj, F32 zFar, F32 stereoSep );
    static void SetupProjection( viewParms_t* dest, F32 zProj, F32 zFar, bool computeFrustum );
    static void SetupProjectionZ( viewParms_t* dest );
    static void SetupProjectionOrtho( viewParms_t* dest, vec3_t viewBounds[2] );
    static void MirrorPoint( vec3_t in, orientation_t* surface, orientation_t* camera, vec3_t out );
    static void MirrorVector( vec3_t in, orientation_t* surface, orientation_t* camera, vec3_t out );
    static void PlaneForSurface( surfaceType_t* surfType, cplane_t* plane );
    static bool GetPortalOrientations( drawSurf_t* drawSurf, S32 entityNum, orientation_t* surface, orientation_t* camera, vec3_t pvsOrigin, bool* mirror );
    static bool IsMirror( const drawSurf_t* drawSurf, S32 entityNum );
    static bool SurfIsOffscreen( const drawSurf_t* drawSurf, vec4_t clipDest[128] );
    static bool MirrorViewBySurface( drawSurf_t* drawSurf, S32 entityNum );
    static S32 SpriteFogNum( trRefEntity_t* ent );
    static void Radix( S32 byte, S32 size, drawSurf_t* source, drawSurf_t* dest );
    static void RadixSort( drawSurf_t* source, S32 size );
    static void AddDrawSurf( surfaceType_t* surface, shader_t* shader, S32 fogIndex, S32 dlightMap, S32 pshadowMap, S32 cubemap );
    static void DecomposeSort( U32 sort, S32* entityNum, shader_t** shader, S32* fogNum, S32* dlightMap, S32* pshadowMap );
    static void SortDrawSurfs( drawSurf_t* drawSurfs, S32 numDrawSurfs );
    static void AddEntitySurface( S32 entityNum );
    static void AddEntitySurfaces( void );
    static void GenerateDrawSurfs( void );
    static void DebugPolygon( S32 color, S32 numPoints, F32* points );
    static void DebugGraphics( void );
    static void RenderView( viewParms_t* parms );
    static void RenderDlightCubemaps( const refdef_t* fd );
    static void RenderPshadowMaps( const refdef_t* fd );
    static F32 CalcSplit( F32 n, F32 f, F32 i, F32 m );
    static void RenderSunShadowMaps( const refdef_t* fd, S32 level );
    static void RenderCubemapSide( cubemap_t* cubemap, S32 cubemapIndex, S32 cubemapSide, bool subscene, bool bounce );
};

extern idRenderSystemMainLocal renderSystemMainLocal;

#endif //!__R_MAIN_H__
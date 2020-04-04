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
// File name:   r_bsp.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: Loads and prepares a map file for scene rendering.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_BSP_TECH3_H__
#define __R_BSP_TECH3_H__

#pragma once

#define	DEFAULT_LIGHTMAP_SIZE	128

#ifndef MAX_SPAWN_VARS
#define MAX_SPAWN_VARS 64
#endif

//
// idRenderSystemBSPTechLocal
//
class idRenderSystemBSPTechLocal
{
public:
    idRenderSystemBSPTechLocal();
    ~idRenderSystemBSPTechLocal();
    
    static void HSVtoRGB( F32 h, F32 s, F32 v, F32 rgb[3] );
    static void ColorShiftLightingBytes( U8 in[4], U8 out[4] );
    static void ColorShiftLightingFloats( F32 in[4], F32 out[4] );
    static void ColorToRGBM( const vec3_t color, U8 rgbm[4] );
    static void ColorToRGB16( const vec3_t color, U16 rgb16[3] );
    static void ColorShiftLightingFloats( F32 in[4], F32 out[4], F32 scale );
    static void ColorToRGBA16F( const vec3_t color, U16 rgba16f[4] );
    static	void LoadLightmaps( lump_t* l, lump_t* surfs );
    static F32 FatPackU( F32 input, S32 lightmapnum );
    static F32 FatPackV( F32 input, S32 lightmapnum );
    static S32 FatLightmap( S32 lightmapnum );
    static	void LoadVisibility( lump_t* l );
    static shader_t* ShaderForShaderNum( S32 shaderNum, S32 lightmapNum );
    static void LoadDrawVertToSrfVert( srfVert_t* s, drawVert_t* d, S32 realLightmapNum, F32 hdrVertColors[3], vec3_t* bounds );
    static void ParseFace( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf, S32* indexes );
    static void ParseMesh( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf );
    static void ParseTriSurf( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf, S32* indexes );
    static void ParseFlare( dsurface_t* ds, drawVert_t* verts, msurface_t* surf, S32* indexes );
    static S32 MergedWidthPoints( srfBspSurface_t* grid, S32 offset );
    static S32 MergedHeightPoints( srfBspSurface_t* grid, S32 offset );
    static void FixSharedVertexLodError_r( S32 start, srfBspSurface_t* grid1 );
    static void FixSharedVertexLodError( void );
    static S32 StitchPatches( S32 grid1num, S32 grid2num );
    static S32 TryStitchingPatch( S32 grid1num );
    static void StitchAllPatches( void );
    static void MovePatchSurfacesToHunk( void );
    static void LoadSurfaces( lump_t* surfs, lump_t* verts, lump_t* indexLump );
    static void LoadSubmodels( lump_t* l );
    static void BSPSetParent( mnode_t* node, mnode_t* parent );
    static void LoadNodesAndLeafs( lump_t* nodeLump, lump_t* leafLump );
    static void LoadShaders( lump_t* l );
    static void LoadMarksurfaces( lump_t* l );
    static void LoadPlanes( lump_t* l );
    static void LoadFogs( lump_t* l, lump_t* brushesLump, lump_t* sidesLump );
    static void LoadLightGrid( lump_t* l );
    static void LoadEntities( lump_t* l );
    static void LoadCubemapEntities( StringEntry cubemapEntityName );
    static void AssignCubemapsToWorldSurfaces( void );
    static void LoadCubemaps( void );
    static void RenderMissingCubemaps( void );
    static void CalcVertexLightDirs( void );
    static bool ParseSpawnVars( UTF8* spawnVarChars, S32 maxSpawnVarChars, S32* numSpawnVars, UTF8* spawnVars[MAX_SPAWN_VARS][2] );
};

extern idRenderSystemBSPTechLocal renderSystemBSPTechLocal;

#endif //!__R_BSP_TECH3_H__

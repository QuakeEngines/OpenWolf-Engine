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

#ifndef __R_SHADER_H__
#define __R_SHADER_H__

#pragma once

static UTF8* s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t	shader;
static texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define SHADER_FILE_HASH_SIZE 1024
static shader_t* shaderHashTable[SHADER_FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH	2048
static UTF8** shaderTextHashTable[MAX_SHADERTEXT_HASH];

#define GLS_BLEND_BITS (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)
#define	MAX_SHADER_FILES	4096

//
// idRenderSystemShaderLocal
//
class idRenderSystemShaderLocal
{
public:
    idRenderSystemShaderLocal();
    ~idRenderSystemShaderLocal();
    
    static S64 generateHashValue( StringEntry fname, const S32 size );
    static bool ParseVector( UTF8** text, S32 count, F32* v );
    static U32 NameToAFunc( StringEntry funcname );
    static S32 NameToSrcBlendMode( StringEntry name );
    static S32 NameToDstBlendMode( StringEntry name );
    static genFunc_t NameToGenFunc( StringEntry funcname );
    static void ParseWaveForm( UTF8** text, waveForm_t* wave );
    static void ParseTexMod( UTF8* _text, shaderStage_t* stage );
    static bool ParseStage( shaderStage_t* stage, UTF8** text );
    static void ParseDeform( UTF8** text );
    static void ParseSkyParms( UTF8** text );
    static void ParseSort( UTF8** text );
    static void ParseSurfaceParm( UTF8** text );
    static bool HaveSurfaceType( S32 surfaceFlags );
    static bool StringsContainWord( StringEntry heystack, StringEntry heystack2, UTF8* needle );
    static bool ParseShader( StringEntry name, UTF8** text );
    static void ComputeStageIteratorFunc( void );
    static void ComputeVertexAttribs( void );
    static void CollapseStagesToLightall( shaderStage_t* diffuse, shaderStage_t* normal, shaderStage_t* specular,
                                          shaderStage_t* lightmap, bool useLightVector, bool useLightVertex, bool parallax, bool tcgen );
    static S32 CollapseStagesToGLSL( void );
    static void FixRenderCommandList( S32 newShader );
    static void SortNewShader( void );
    static shader_t* GeneratePermanentShader( void );
    static void FindLightingStages( void );
    static void VertexLightingCollapse( void );
    static void InitShader( StringEntry name, S32 lightmapIndex );
    static shader_t* FinishShader( void );
    static UTF8* FindShaderInShaderText( StringEntry shadername );
    static shader_t* FindShaderByName( StringEntry name );
    static shader_t* FindShader( StringEntry name, S32 lightmapIndex, bool mipRawImage );
    static qhandle_t RegisterShaderFromImage( StringEntry name, S32 lightmapIndex, image_t* image, bool mipRawImage );
    static qhandle_t RegisterShaderLightMap( StringEntry name, S32 lightmapIndex );
    static shader_t* GetShaderByHandle( qhandle_t hShader );
    static void ShaderList_f( void );
    static void ScanAndLoadShaderFiles( void );
    static void CreateInternalShaders( void );
    static void CreateExternalShaders( void );
    static void InitShaders( void );
    
};

extern idRenderSystemShaderLocal renderSystemShaderLocal;

#endif //!__R_SHADER_H__

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

#ifndef __R_SHADE_H__
#define __R_SHADE_H__

#pragma once

//
// idRenderSystemShadeLocal
//
class idRenderSystemShadeLocal
{
public:
    idRenderSystemShadeLocal();
    ~idRenderSystemShadeLocal();
    
    static void DrawElements( S32 numIndexes, S32 firstIndex );
    static S64 ftol( F32 f );
    static void BindAnimatedImageToTMU( textureBundle_t* bundle, S32 tmu );
    static void DrawTris( shaderCommands_t* input );
    static void DrawNormals( shaderCommands_t* input );
    static void BeginSurface( shader_t* shader, S32 fogNum, S32 cubemapIndex );
    static void ComputeTexMods( shaderStage_t* pStage, S32 bundleNum, F32* outMatrix, F32* outOffTurb );
    static void ComputeDeformValues( S32* deformGen, vec5_t deformParams );
    static void ProjectDlightTexture( void );
    static void ComputeShaderColors( shaderStage_t* pStage, vec4_t baseColor, vec4_t vertColor, S32 blend );
    static void ComputeFogValues( vec4_t fogDistanceVector, vec4_t fogDepthVector, F32* eyeT );
    static void ComputeFogColorMask( shaderStage_t* pStage, vec4_t fogColorMask );
    static void ForwardDlight( void );
    static void ProjectPshadowVBOGLSL( void );
    static void FogPass( void );
    static U32 CalcShaderVertexAttribs( shaderCommands_t* input );
    static void SetMaterialBasedProperties( shaderProgram_t* sp, shaderStage_t* pStage );
    static void SetStageImageDimensions( shaderProgram_t* sp, shaderStage_t* pStage );
    static cullType_t GetCullType( const viewParms_t* viewParms, const trRefEntity_t* refEntity, cullType_t shaderCullType );
    static void IterateStagesGeneric( shaderCommands_t* input );
    static void RenderShadowmap( shaderCommands_t* input );
    static void StageIteratorGeneric( void );
    static void EndSurface( void );
    static F64 WaveValue( const F32* table, F64 base, F64 amplitude, F64 phase, F64 freq );
    static const F32* TableForFunc( genFunc_t func );
    static F32 EvalWaveForm( const waveForm_t* wf );
    static F32 EvalWaveFormClamped( const waveForm_t* wf );
    static void CalcStretchTexMatrix( const waveForm_t* wf, F32* matrix );
    static void CalcDeformVertexes( deformStage_t* ds );
    static void CalcDeformNormals( deformStage_t* ds );
    static void CalcBulgeVertexes( deformStage_t* ds );
    static void CalcMoveVertexes( deformStage_t* ds );
    static void DeformText( StringEntry text );
    static void GlobalVectorToLocal( const vec3_t in, vec3_t out );
    static void AutospriteDeform( void );
    static void Autosprite2Deform( void );
    static void DeformTessGeometry( void );
    static F32 CalcWaveColorSingle( const waveForm_t* wf );
    static F32 CalcWaveAlphaSingle( const waveForm_t* wf );
    static void CalcModulateColorsByFog( U8* colors );
    static void CalcFogTexCoords( F32* st );
    static void CalcTurbulentFactors( const waveForm_t* wf, F32* amplitude, F32* now );
    static void CalcScaleTexMatrix( const F32 scale[2], F32* matrix );
    static void CalcScrollTexMatrix( const F32 scrollSpeed[2], F32* matrix );
    static void CalcTransformTexMatrix( const texModInfo_t* tmi, F32* matrix );
    static void CalcRotateTexMatrix( F32 degsPerSecond, F32* matrix );
    static bool ShaderRequiresCPUDeforms( const shader_t* shader );
};

extern idRenderSystemShadeLocal renderSystemShadeLocal;

#endif //!__R_SHADER_H__

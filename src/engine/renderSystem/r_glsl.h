////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2006 - 2008 Robert Beckebans <trebor_7@users.sourceforge.net>
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
// File name:   r_glsl.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_GLSL_H__
#define __R_GLSL_H__

#pragma once

struct uniformInfo_t
{
    StringEntry name;
    S32 type;
    S32 size;
};

// These must be in the same order as in uniform_t in r_local.h.
static uniformInfo_t uniformsInfo[] =
{
    { "u_DiffuseMap",  GLSL_INT, 1 },
    { "u_LightMap",    GLSL_INT, 1 },
    { "u_NormalMap",   GLSL_INT, 1 },
    { "u_DeluxeMap",   GLSL_INT, 1},
    { "u_SpecularMap", GLSL_INT, 1},
    { "u_GlowMap",     GLSL_INT, 1},
    
    { "u_TextureMap", GLSL_INT, 1},
    { "u_LevelsMap",  GLSL_INT, 1 },
    { "u_CubeMap",    GLSL_INT, 1 },
    { "u_EnvBrdfMap", GLSL_INT, 1 },
    
    { "u_ScreenImageMap", GLSL_INT , 1},
    { "u_ScreenDepthMap", GLSL_INT , 1},
    
    { "u_ShadowMap",  GLSL_INT, 1 },
    { "u_ShadowMap2", GLSL_INT, 1 },
    { "u_ShadowMap3", GLSL_INT, 1 },
    { "u_ShadowMap4", GLSL_INT, 1 },
    
    { "u_ShadowMvp",  GLSL_MAT16, 1 },
    { "u_ShadowMvp2", GLSL_MAT16, 1 },
    { "u_ShadowMvp3", GLSL_MAT16, 1 },
    { "u_ShadowMvp4", GLSL_MAT16, 1 },
    
    { "u_EnableTextures", GLSL_VEC4, 1 },
    
    { "u_DiffuseTexMatrix",  GLSL_VEC4 , 1},
    { "u_DiffuseTexOffTurb", GLSL_VEC4 , 1},
    
    { "u_TCGen0",        GLSL_INT , 1},
    { "u_TCGen0Vector0", GLSL_VEC3 , 1},
    { "u_TCGen0Vector1", GLSL_VEC3, 1 },
    
    { "u_DeformGen",    GLSL_INT, 1 },
    { "u_DeformParams", GLSL_FLOAT5 , 1},
    
    { "u_ColorGen",  GLSL_INT , 1},
    { "u_AlphaGen",  GLSL_INT , 1},
    { "u_Color",     GLSL_VEC4, 1 },
    { "u_BaseColor", GLSL_VEC4 , 1},
    { "u_VertColor", GLSL_VEC4, 1 },
    
    { "u_DlightInfo",    GLSL_VEC4 , 1},
    { "u_LightForward",  GLSL_VEC3 , 1},
    { "u_LightUp",       GLSL_VEC3 , 1},
    { "u_LightRight",    GLSL_VEC3 , 1},
    { "u_LightOrigin",   GLSL_VEC4 , 1},
    { "u_LightOrigin1",   GLSL_VEC4 , 1},
    { "u_LightColor",   GLSL_VEC4, 1 },
    { "u_LightColor1",   GLSL_VEC4, 1, },
    { "u_ModelLightDir", GLSL_VEC3, 1 },
    { "u_LightRadius",   GLSL_FLOAT , 1},
    { "u_AmbientLight",  GLSL_VEC3 , 1},
    { "u_DirectedLight", GLSL_VEC3 , 1},
    
    { "u_PortalRange", GLSL_FLOAT, 1 },
    
    { "u_FogDistance",  GLSL_VEC4 , 1},
    { "u_FogDepth",     GLSL_VEC4, 1 },
    { "u_FogEyeT",      GLSL_FLOAT , 1},
    { "u_FogColorMask", GLSL_VEC4, 1 },
    
    { "u_ModelMatrix",               GLSL_MAT16, 1 },
    { "u_ModelViewProjectionMatrix", GLSL_MAT16, 1 },
    
    { "u_invProjectionMatrix", GLSL_MAT16, 1 },
    { "u_invEyeProjectionMatrix", GLSL_MAT16 , 1},
    
    { "u_Time",          GLSL_FLOAT, 1 },
    { "u_VertexLerp",   GLSL_FLOAT, 1 },
    { "u_NormalScale",   GLSL_VEC4, 1 },
    { "u_SpecularScale", GLSL_VEC4, 1 },
    
    { "u_ViewInfo",        GLSL_VEC4, 1 },
    { "u_ViewOrigin",      GLSL_VEC3, 1 },
    { "u_LocalViewOrigin", GLSL_VEC3, 1 },
    { "u_ViewForward",     GLSL_VEC3, 1 },
    { "u_ViewLeft",        GLSL_VEC3, 1 },
    { "u_ViewUp",          GLSL_VEC3, 1 },
    
    { "u_InvTexRes",           GLSL_VEC2, 1 },
    { "u_AutoExposureMinMax",  GLSL_VEC2, 1 },
    { "u_ToneMinAvgMaxLinear", GLSL_VEC3, 1 },
    
    { "u_PrimaryLightOrigin",  GLSL_VEC4 , 1 },
    { "u_PrimaryLightColor",   GLSL_VEC3, 1  },
    { "u_PrimaryLightAmbient", GLSL_VEC3, 1  },
    { "u_PrimaryLightRadius",  GLSL_FLOAT, 1 },
    
    { "u_CubeMapInfo", GLSL_VEC4, 1 },
    
    { "u_AlphaTest", GLSL_INT, 1 },
    
    { "u_BoneMatrix", GLSL_MAT16_BONEMATRIX , 1},
    
    { "u_Brightness",	GLSL_FLOAT, 1 },
    { "u_Contrast",		GLSL_FLOAT, 1 },
    { "u_Gamma",		GLSL_FLOAT, 1 },
    
    { "u_Dimensions",           GLSL_VEC2, 1 },
    { "u_HeightMap",			GLSL_INT, 1 },
    { "u_Local0",				GLSL_VEC4, 1 },
    { "u_Local1",				GLSL_VEC4 , 1 },
    { "u_Local2",				GLSL_VEC4, 1 },
    { "u_Local3",				GLSL_VEC4, 1 },
    { "u_Texture0",				GLSL_INT, 1 },
    { "u_Texture1",				GLSL_INT, 1 },
    { "u_Texture2",				GLSL_INT, 1 },
    { "u_Texture3",				GLSL_INT, 1 },
};

typedef enum
{
    GLSL_PRINTLOG_PROGRAM_INFO,
    GLSL_PRINTLOG_SHADER_INFO,
    GLSL_PRINTLOG_SHADER_SOURCE
}
glslPrintLog_t;

//
// idRenderSystemGLSLLocal
//
class idRenderSystemGLSLLocal
{
public:
    idRenderSystemGLSLLocal();
    ~idRenderSystemGLSLLocal();
    
    static void PrintLog( U32 programOrShader, glslPrintLog_t type, bool developerOnly );
    static void GetShaderHeader( GLenum shaderType, StringEntry extra, UTF8* dest, S32 size );
    static S32 CompileGPUShader( U32 program, U32* prevShader, StringEntry buffer, S32 size, U32 shaderType );
    static S32 LoadGPUShaderText( StringEntry name, U32 shaderType, UTF8* dest, S32 destSize );
    static void LinkProgram( U32 program );
    static void ValidateProgram( U32 program );
    static void ShowProgramUniforms( U32 program );
    static S32 InitGPUShader2( shaderProgram_t* program, StringEntry name, S32 attribs, StringEntry vpCode, StringEntry fpCode );
    static S32 InitGPUShader( shaderProgram_t* program, StringEntry name, S32 attribs, bool fragmentShader, StringEntry extra, bool addHeader );
    static void InitUniforms( shaderProgram_t* program );
    static void FinishGPUShader( shaderProgram_t* program );
    static void SetUniformInt( shaderProgram_t* program, S32 uniformNum, S32 value );
    static void SetUniformFloat( shaderProgram_t* program, S32 uniformNum, F32 value );
    static void SetUniformVec2( shaderProgram_t* program, S32 uniformNum, const vec2_t v );
    static void SetUniformVec3( shaderProgram_t* program, S32 uniformNum, const vec3_t v );
    static void SetUniformVec4( shaderProgram_t* program, S32 uniformNum, const vec4_t v );
    static void SetUniformFloat5( shaderProgram_t* program, S32 uniformNum, const vec5_t v );
    static void SetUniformMat4( shaderProgram_t* program, S32 uniformNum, const mat4_t matrix );
    static void SetUniformMat4BoneMatrix( shaderProgram_t* program, S32 uniformNum, mat4_t* matrix, S32 numMatricies );
    static void DeleteGPUShader( shaderProgram_t* program );
    static void InitGPUShaders( void );
    static void ShutdownGPUShaders( void );
    static void BindProgram( shaderProgram_t* program );
    static shaderProgram_t* GetGenericShaderProgram( S32 stage );
};

extern idRenderSystemGLSLLocal renderSystemGLSLLocal;

#endif //!__R_GLSL_H__
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
// File name:   r_shade.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: THIS ENTIRE FILE IS BACK END
//              This file deals with applying shaders to surface data in the tess struct.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

shaderCommands_t tess;

idRenderSystemShadeLocal renderSystemShadeLocal;

/*
===============
idRenderSystemShadeLocal::idRenderSystemShadeLocal
===============
*/
idRenderSystemShadeLocal::idRenderSystemShadeLocal( void )
{
}

/*
===============
idRenderSystemShadeLocal::~idRenderSystemShadeLocal
===============
*/
idRenderSystemShadeLocal::~idRenderSystemShadeLocal( void )
{
}

/*
===============
edgeVerts
===============
*/
static S32 edgeVerts[6][2] =
{
    { 0, 1 },
    { 0, 2 },
    { 0, 3 },
    { 1, 2 },
    { 1, 3 },
    { 2, 3 }
};

/*
==================
idRenderSystemShadeLocal::DrawElements
==================
*/
void idRenderSystemShadeLocal::DrawElements( S32 numIndexes, S32 firstIndex )
{
    qglDrawElements( GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET( firstIndex * sizeof( U32 ) ) );
}

/*
=================
BindAnimatedImageToTMU
=================
*/
S64 idRenderSystemShadeLocal::ftol( F32 f )
{
    return ( S64 )f;
}

void idRenderSystemShadeLocal::BindAnimatedImageToTMU( textureBundle_t* bundle, S32 tmu )
{
    S64 i;
    
    if ( bundle->isVideoMap )
    {
        trap_CIN_RunCinematic( bundle->videoMapHandle );
        trap_CIN_UploadCinematic( bundle->videoMapHandle );
        idRenderSystemBackendLocal::BindToTMU( tr.scratchImage[bundle->videoMapHandle], tmu );
        return;
    }
    
    if ( bundle->numImageAnimations <= 1 )
    {
        idRenderSystemBackendLocal::BindToTMU( bundle->image[0], tmu );
        return;
    }
    
    // it is necessary to do this messy calc to make sure animations line up
    // exactly with waveforms of the same frequency
    i = ( S64 )( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
    i >>= FUNCTABLE_SIZE2;
    
    if ( i < 0 )
    {
        i = 0;	// may happen with shader time offsets
    }
    
    // Windows x86 doesn't load renderer DLL with 64 bit modulus
    //index %= bundle->numImageAnimations;
    while ( i >= bundle->numImageAnimations )
    {
        i -= bundle->numImageAnimations;
    }
    
    
    //i %= bundle->numImageAnimations;
    
    idRenderSystemBackendLocal::BindToTMU( bundle->image[ i ], tmu );
}


/*
================
idRenderSystemShadeLocal::DrawTris

Draws triangle outlines for debugging
================
*/
void idRenderSystemShadeLocal::DrawTris( shaderCommands_t* input )
{
    idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_COLORMAP );
    
    idRenderSystemBackendLocal::State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
    qglDepthRange( 0, 0 );
    
    {
        shaderProgram_t* sp = &tr.textureColorShader;
        vec4_t color;
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        VectorSet4( color, 1, 1, 1, 1 );
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_COLOR, color );
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
        
        DrawElements( input->numIndexes, input->firstIndex );
    }
    
    qglDepthRange( 0, 1 );
}

/*
================
idRenderSystemShadeLocal::DrawNormals

Draws vertex normals for debugging
================
*/
void idRenderSystemShadeLocal::DrawNormals( shaderCommands_t* input )
{
    //FIXME: implement this
}

/*
==============
idRenderSystemShadeLocal::BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a idRenderSystemShadeLocal::End due
to overflow.
==============
*/
void idRenderSystemShadeLocal::BeginSurface( shader_t* shader, S32 fogNum, S32 cubemapIndex )
{
    shader_t* state = ( shader->remappedShader ) ? shader->remappedShader : shader;
    
    tess.numIndexes = 0;
    tess.firstIndex = 0;
    tess.numVertexes = 0;
    tess.shader = state;
    tess.fogNum = fogNum;
    tess.cubemapIndex = cubemapIndex;
    tess.dlightBits = 0;		// will be OR'd in by surface functions
    tess.pshadowBits = 0;       // will be OR'd in by surface functions
    tess.xstages = state->stages;
    tess.numPasses = state->numUnfoggedPasses;
    tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
    tess.useInternalVao = true;
    tess.useCacheVao = false;
    
    tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
    if ( tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime )
    {
        tess.shaderTime = tess.shader->clampTime;
    }
    
    if ( backEnd.viewParms.flags & VPF_SHADOWMAP )
    {
        tess.currentStageIteratorFunc = StageIteratorGeneric;
    }
}

/*
==================
idRenderSystemShadeLocal::ComputeTexMods
==================
*/
void idRenderSystemShadeLocal::ComputeTexMods( shaderStage_t* pStage, S32 bundleNum, F32* outMatrix, F32* outOffTurb )
{
    S32 tm;
    F32 matrix[6], currentmatrix[6];
    textureBundle_t* bundle = &pStage->bundle[bundleNum];
    
    matrix[0] = 1.0f;
    matrix[2] = 0.0f;
    matrix[4] = 0.0f;
    matrix[1] = 0.0f;
    matrix[3] = 1.0f;
    matrix[5] = 0.0f;
    
    currentmatrix[0] = 1.0f;
    currentmatrix[2] = 0.0f;
    currentmatrix[4] = 0.0f;
    currentmatrix[1] = 0.0f;
    currentmatrix[3] = 1.0f;
    currentmatrix[5] = 0.0f;
    
    outMatrix[0] = 1.0f;
    outMatrix[2] = 0.0f;
    outMatrix[1] = 0.0f;
    outMatrix[3] = 1.0f;
    
    outOffTurb[0] = 0.0f;
    outOffTurb[1] = 0.0f;
    outOffTurb[2] = 0.0f;
    outOffTurb[3] = 0.0f;
    
    for ( tm = 0; tm < bundle->numTexMods ; tm++ )
    {
        switch ( bundle->texMods[tm].type )
        {
        
            case TMOD_NONE:
                // break out of for loop
                tm = TR_MAX_TEXMODS;
                break;
                
            case TMOD_TURBULENT:
                CalcTurbulentFactors( &bundle->texMods[tm].wave, &outOffTurb[2], &outOffTurb[3] );
                break;
                
            case TMOD_ENTITY_TRANSLATE:
                CalcScrollTexMatrix( backEnd.currentEntity->e.shaderTexCoord, matrix );
                break;
                
            case TMOD_SCROLL:
                CalcScrollTexMatrix( bundle->texMods[tm].scroll, matrix );
                break;
                
            case TMOD_SCALE:
                CalcScaleTexMatrix( bundle->texMods[tm].scale, matrix );
                break;
                
            case TMOD_STRETCH:
                CalcStretchTexMatrix( &bundle->texMods[tm].wave, matrix );
                break;
                
            case TMOD_TRANSFORM:
                CalcTransformTexMatrix( &bundle->texMods[tm], matrix );
                break;
                
            case TMOD_ROTATE:
                CalcRotateTexMatrix( bundle->texMods[tm].rotateSpeed, matrix );
                break;
                
            default:
                Com_Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", bundle->texMods[tm].type, tess.shader->name );
                break;
        }
        
        switch ( bundle->texMods[tm].type )
        {
            case TMOD_NONE:
            case TMOD_TURBULENT:
            default:
                break;
                
            case TMOD_ENTITY_TRANSLATE:
            case TMOD_SCROLL:
            case TMOD_SCALE:
            case TMOD_STRETCH:
            case TMOD_TRANSFORM:
            case TMOD_ROTATE:
                outMatrix[0] = matrix[0] * currentmatrix[0] + matrix[2] * currentmatrix[1];
                outMatrix[1] = matrix[1] * currentmatrix[0] + matrix[3] * currentmatrix[1];
                
                outMatrix[2] = matrix[0] * currentmatrix[2] + matrix[2] * currentmatrix[3];
                outMatrix[3] = matrix[1] * currentmatrix[2] + matrix[3] * currentmatrix[3];
                
                outOffTurb[0] = matrix[0] * currentmatrix[4] + matrix[2] * currentmatrix[5] + matrix[4];
                outOffTurb[1] = matrix[1] * currentmatrix[4] + matrix[3] * currentmatrix[5] + matrix[5];
                
                currentmatrix[0] = outMatrix[0];
                currentmatrix[1] = outMatrix[1];
                currentmatrix[2] = outMatrix[2];
                currentmatrix[3] = outMatrix[3];
                currentmatrix[4] = outOffTurb[0];
                currentmatrix[5] = outOffTurb[1];
                break;
        }
    }
}

/*
==================
idRenderSystemShadeLocal::ComputeDeformValues
==================
*/
void idRenderSystemShadeLocal::ComputeDeformValues( S32* deformGen, vec5_t deformParams )
{
    // u_DeformGen
    *deformGen = DGEN_NONE;
    if ( !ShaderRequiresCPUDeforms( tess.shader ) )
    {
        deformStage_t*  ds;
        
        // only support the first one
        ds = &tess.shader->deforms[0];
        
        switch ( ds->deformation )
        {
            case DEFORM_WAVE:
                *deformGen = ds->deformationWave.func;
                
                deformParams[0] = ds->deformationWave.base;
                deformParams[1] = ds->deformationWave.amplitude;
                deformParams[2] = ds->deformationWave.phase;
                deformParams[3] = ds->deformationWave.frequency;
                deformParams[4] = ds->deformationSpread;
                break;
                
            case DEFORM_BULGE:
                *deformGen = DGEN_BULGE;
                
                deformParams[0] = 0;
                // amplitude
                deformParams[1] = ds->bulgeHeight;
                // phase
                deformParams[2] = ds->bulgeWidth;
                // frequency
                deformParams[3] = ds->bulgeSpeed;
                deformParams[4] = 0;
                break;
                
            default:
                break;
        }
    }
}

/*
==================
idRenderSystemShadeLocal::ProjectDlightTexture
==================
*/
void idRenderSystemShadeLocal::ProjectDlightTexture( void )
{
    S32	l;
    vec3_t	origin;
    F32	scale;
    F32	radius;
    S32 deformGen;
    vec5_t deformParams;
    
    if ( r_dynamiclight->integer == 0 )
    {
        return;
    }
    
    if ( !backEnd.refdef.num_dlights )
    {
        return;
    }
    
    ComputeDeformValues( &deformGen, deformParams );
    
    for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ )
    {
        dlight_t*	dl;
        shaderProgram_t* sp;
        vec4_t vector;
        
        if ( !( tess.dlightBits & ( 1 << l ) ) )
        {
            // this surface definately doesn't have any of this light
            continue;
        }
        
        dl = &backEnd.refdef.dlights[l];
        VectorCopy( dl->transformed, origin );
        radius = dl->radius;
        scale = 1.0f / radius;
        
        sp = &tr.dlightShader[deformGen == DGEN_NONE ? 0 : 1];
        
        backEnd.pc.c_dlightDraws++;
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation );
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_DEFORMGEN, deformGen );
        if ( deformGen != DGEN_NONE )
        {
            idRenderSystemGLSLLocal::SetUniformFloat5( sp, UNIFORM_DEFORMPARAMS, deformParams );
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
        }
        
        vector[0] = dl->color[0];
        vector[1] = dl->color[1];
        vector[2] = dl->color[2];
        vector[3] = 1.0f;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_COLOR, vector );
        
        vector[0] = origin[0];
        vector[1] = origin[1];
        vector[2] = origin[2];
        vector[3] = scale;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DLIGHTINFO, vector );
        
        idRenderSystemBackendLocal::BindToTMU( tr.dlightImage, TB_COLORMAP );
        
        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        if ( dl->additive )
        {
            idRenderSystemBackendLocal::State( GLS_ATEST_GT_0 | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
        }
        else
        {
            idRenderSystemBackendLocal::State( GLS_ATEST_GT_0 | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
        }
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 1 );
        
        DrawElements( tess.numIndexes, tess.firstIndex );
        
        backEnd.pc.c_totalIndexes += tess.numIndexes;
        backEnd.pc.c_dlightIndexes += tess.numIndexes;
        backEnd.pc.c_dlightVertexes += tess.numVertexes;
    }
}

/*
==================
idRenderSystemShadeLocal::ComputeShaderColors
==================
*/
void idRenderSystemShadeLocal::ComputeShaderColors( shaderStage_t* pStage, vec4_t baseColor, vec4_t vertColor, S32 blend )
{
    bool isBlend = ( ( blend & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR )
                   || ( ( blend & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR )
                   || ( ( blend & GLS_DSTBLEND_BITS ) == GLS_DSTBLEND_SRC_COLOR )
                   || ( ( blend & GLS_DSTBLEND_BITS ) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
                   
    bool is2DDraw = backEnd.currentEntity == &backEnd.entity2D;
    
    F32 overbright = ( isBlend || is2DDraw ) ? 1.0f : ( F32 )( 1 << tr.overbrightBits );
    
    fog_t* fog;
    
    baseColor[0] =
        baseColor[1] =
            baseColor[2] =
                baseColor[3] = 1.0f;
                
    vertColor[0] =
        vertColor[1] =
            vertColor[2] =
                vertColor[3] = 0.0f;
                
    // rgbGen
    switch ( pStage->rgbGen )
    {
        case CGEN_EXACT_VERTEX:
        case CGEN_EXACT_VERTEX_LIT:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] =
                        baseColor[3] = 0.0f;
                        
            vertColor[0] =
                vertColor[1] =
                    vertColor[2] = overbright;
            vertColor[3] = 1.0f;
            break;
        case CGEN_CONST:
            baseColor[0] = pStage->constantColor[0] / 255.0f;
            baseColor[1] = pStage->constantColor[1] / 255.0f;
            baseColor[2] = pStage->constantColor[2] / 255.0f;
            baseColor[3] = pStage->constantColor[3] / 255.0f;
            break;
        case CGEN_VERTEX:
        case CGEN_VERTEX_LIT:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] =
                        baseColor[3] = 0.0f;
                        
            vertColor[0] =
                vertColor[1] =
                    vertColor[2] =
                        vertColor[3] = 1.0f;
            break;
        case CGEN_ONE_MINUS_VERTEX:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] = 1.0f;
                    
            vertColor[0] =
                vertColor[1] =
                    vertColor[2] = -1.0f;
            break;
        case CGEN_FOG:
            fog = tr.world->fogs + tess.fogNum;
            
            baseColor[0] = ( ( U8* )( &fog->colorInt ) )[0] / 255.0f;
            baseColor[1] = ( ( U8* )( &fog->colorInt ) )[1] / 255.0f;
            baseColor[2] = ( ( U8* )( &fog->colorInt ) )[2] / 255.0f;
            baseColor[3] = ( ( U8* )( &fog->colorInt ) )[3] / 255.0f;
            break;
        case CGEN_WAVEFORM:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] = CalcWaveColorSingle( &pStage->rgbWave );
            break;
        case CGEN_ENTITY:
            if ( backEnd.currentEntity )
            {
                baseColor[0] = ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[0] / 255.0f;
                baseColor[1] = ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[1] / 255.0f;
                baseColor[2] = ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[2] / 255.0f;
                baseColor[3] = ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[3] / 255.0f;
            }
            break;
        case CGEN_ONE_MINUS_ENTITY:
            if ( backEnd.currentEntity )
            {
                baseColor[0] = 1.0f - ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[0] / 255.0f;
                baseColor[1] = 1.0f - ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[1] / 255.0f;
                baseColor[2] = 1.0f - ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[2] / 255.0f;
                baseColor[3] = 1.0f - ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[3] / 255.0f;
            }
            break;
        case CGEN_IDENTITY:
        case CGEN_LIGHTING_DIFFUSE:
            baseColor[0] =
                baseColor[1] =
                    baseColor[2] = overbright;
            break;
        case CGEN_IDENTITY_LIGHTING:
        case CGEN_BAD:
            break;
    }
    
    // alphaGen
    switch ( pStage->alphaGen )
    {
        case AGEN_SKIP:
            break;
        case AGEN_CONST:
            baseColor[3] = pStage->constantColor[3] / 255.0f;
            vertColor[3] = 0.0f;
            break;
        case AGEN_WAVEFORM:
            baseColor[3] = CalcWaveAlphaSingle( &pStage->alphaWave );
            vertColor[3] = 0.0f;
            break;
        case AGEN_ENTITY:
            if ( backEnd.currentEntity )
            {
                baseColor[3] = ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[3] / 255.0f;
            }
            vertColor[3] = 0.0f;
            break;
        case AGEN_ONE_MINUS_ENTITY:
            if ( backEnd.currentEntity )
            {
                baseColor[3] = 1.0f - ( ( U8* )backEnd.currentEntity->e.shaderRGBA )[3] / 255.0f;
            }
            vertColor[3] = 0.0f;
            break;
        case AGEN_VERTEX:
            baseColor[3] = 0.0f;
            vertColor[3] = 1.0f;
            break;
        case AGEN_ONE_MINUS_VERTEX:
            baseColor[3] = 1.0f;
            vertColor[3] = -1.0f;
            break;
        case AGEN_IDENTITY:
        case AGEN_LIGHTING_SPECULAR:
        case AGEN_PORTAL:
            // Done entirely in vertex program
            baseColor[3] = 1.0f;
            vertColor[3] = 0.0f;
            break;
    }
    
    // FIXME: find some way to implement this.
#if 0
    // if in greyscale rendering mode turn all color values into greyscale.
    if ( r_greyscale->integer )
    {
        S32 scale;
        
        for ( i = 0; i < tess.numVertexes; i++ )
        {
            scale = ( tess.svars.colors[i][0] + tess.svars.colors[i][1] + tess.svars.colors[i][2] ) / 3;
            tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2] = scale;
        }
    }
#endif
}

/*
==================
idRenderSystemShadeLocal::ComputeFogValues
==================
*/
void idRenderSystemShadeLocal::ComputeFogValues( vec4_t fogDistanceVector, vec4_t fogDepthVector, F32* eyeT )
{
    // from CalcFogTexCoords()
    fog_t*  fog;
    vec3_t  local;
    
    if ( !tess.fogNum )
        return;
        
    fog = tr.world->fogs + tess.fogNum;
    
    VectorSubtract( backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local );
    fogDistanceVector[0] = -backEnd.orientation.modelMatrix[2];
    fogDistanceVector[1] = -backEnd.orientation.modelMatrix[6];
    fogDistanceVector[2] = -backEnd.orientation.modelMatrix[10];
    fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.orientation.axis[0] );
    
    // scale the fog vectors based on the fog's thickness
    VectorScale4( fogDistanceVector, fog->tcScale, fogDistanceVector );
    
    // rotate the gradient vector for this orientation
    if ( fog->hasSurface )
    {
        fogDepthVector[0] = fog->surface[0] * backEnd.orientation.axis[0][0] +
                            fog->surface[1] * backEnd.orientation.axis[0][1] + fog->surface[2] * backEnd.orientation.axis[0][2];
        fogDepthVector[1] = fog->surface[0] * backEnd.orientation.axis[1][0] +
                            fog->surface[1] * backEnd.orientation.axis[1][1] + fog->surface[2] * backEnd.orientation.axis[1][2];
        fogDepthVector[2] = fog->surface[0] * backEnd.orientation.axis[2][0] +
                            fog->surface[1] * backEnd.orientation.axis[2][1] + fog->surface[2] * backEnd.orientation.axis[2][2];
        fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.orientation.origin, fog->surface );
        
        *eyeT = DotProduct( backEnd.orientation.viewOrigin, fogDepthVector ) + fogDepthVector[3];
    }
    else
    {
        // non-surface fog always has eye inside
        *eyeT = 1;
    }
}

/*
==================
idRenderSystemShadeLocal::ComputeFogColorMask
==================
*/
void idRenderSystemShadeLocal::ComputeFogColorMask( shaderStage_t* pStage, vec4_t fogColorMask )
{
    switch ( pStage->adjustColorsForFog )
    {
        case ACFF_MODULATE_RGB:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] = 1.0f;
            fogColorMask[3] = 0.0f;
            break;
        case ACFF_MODULATE_ALPHA:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] = 0.0f;
            fogColorMask[3] = 1.0f;
            break;
        case ACFF_MODULATE_RGBA:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] =
                        fogColorMask[3] = 1.0f;
            break;
        default:
            fogColorMask[0] =
                fogColorMask[1] =
                    fogColorMask[2] =
                        fogColorMask[3] = 0.0f;
            break;
    }
}

/*
==================
idRenderSystemShadeLocal::ForwardDlight
==================
*/
void idRenderSystemShadeLocal::ForwardDlight( void )
{
    S32	l;
    F32	radius;
    
    S32 deformGen;
    vec5_t deformParams;
    
    vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
    F32 eyeT = 0;
    if ( r_dynamiclight->integer == 0 )
    {
        return;
    }
    
    shaderCommands_t* input = &tess;
    shaderStage_t* pStage = tess.xstages[0];
    
    if ( !pStage )
    {
        return;
    }
    
    if ( !backEnd.refdef.num_dlights )
    {
        return;
    }
    
    ComputeDeformValues( &deformGen, deformParams );
    
    ComputeFogValues( fogDistanceVector, fogDepthVector, &eyeT );
    
    for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ )
    {
        dlight_t*	dl;
        shaderProgram_t* sp;
        vec4_t vector;
        vec4_t texMatrix;
        vec4_t texOffTurb;
        
        if ( !( tess.dlightBits & ( 1 << l ) ) )
        {
            // this surface definitely doesn't have any of this light
            continue;
        }
        
        dl = &backEnd.refdef.dlights[l];
        //VectorCopy( dl->transformed, origin );
        radius = dl->radius;
        //scale = 1.0f / radius;
        
        //if (pStage->glslShaderGroup == tr.lightallShader)
        {
            S32 index = pStage->glslShaderIndex;
            
            index &= ~LIGHTDEF_LIGHTTYPE_MASK;
            index |= LIGHTDEF_USE_LIGHT_VECTOR;
            
            sp = &tr.lightallShader[index];
        }
        
        backEnd.pc.c_lightallDraws++;
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_LOCALVIEWORIGIN, backEnd.orientation.viewOrigin );
        
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation );
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_DEFORMGEN, deformGen );
        if ( deformGen != DGEN_NONE )
        {
            idRenderSystemGLSLLocal::SetUniformFloat5( sp, UNIFORM_DEFORMPARAMS, deformParams );
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
        }
        
        if ( input->fogNum )
        {
            vec4_t fogColorMask;
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGDISTANCE, fogDistanceVector );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGDEPTH, fogDepthVector );
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_FOGEYET, eyeT );
            
            ComputeFogColorMask( pStage, fogColorMask );
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGCOLORMASK, fogColorMask );
        }
        
        {
            vec4_t baseColor;
            vec4_t vertColor;
            
            ComputeShaderColors( pStage, baseColor, vertColor, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_BASECOLOR, baseColor );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_VERTCOLOR, vertColor );
        }
        
        if ( pStage->alphaGen == AGEN_PORTAL )
        {
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_PORTALRANGE, tess.shader->portalRange );
        }
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_COLORGEN, pStage->rgbGen );
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHAGEN, pStage->alphaGen );
        
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_DIRECTEDLIGHT, dl->color );
        
        VectorSet( vector, 0, 0, 0 );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_AMBIENTLIGHT, vector );
        
        VectorCopy( dl->origin, vector );
        vector[3] = 1.0f;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_LIGHTORIGIN, vector );
        
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_LIGHTRADIUS, radius );
        
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_NORMALSCALE, pStage->normalScale );
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_SPECULARSCALE, pStage->specularScale );
        
        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        idRenderSystemBackendLocal::State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
        
        if ( pStage->bundle[TB_DIFFUSEMAP].image[0] )
        {
            BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP );
        }
        
        // bind textures that are sampled and used in the glsl shader, and
        // bind whiteImage to textures that are sampled but zeroed in the glsl shader
        //
        // alternatives:
        //  - use the last bound texture
        //     -> costs more to sample a higher res texture then throw out the result
        //  - disable texture sampling in glsl shader with #ifdefs, as before
        //     -> increases the number of shaders that must be compiled
        //
        
        if ( pStage->bundle[TB_NORMALMAP].image[0] )
        {
            BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP], TB_NORMALMAP );
        }
        else if ( r_normalMapping->integer )
        {
            idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_NORMALMAP );
        }
        
        if ( pStage->bundle[TB_SPECULARMAP].image[0] )
        {
            BindAnimatedImageToTMU( &pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP );
        }
        else if ( r_specularMapping->integer )
        {
            idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_SPECULARMAP );
        }
        
        {
            vec4_t enableTextures;
            
            VectorSet4( enableTextures, 0.0f, 0.0f, 0.0f, 0.0f );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_ENABLETEXTURES, enableTextures );
        }
        
        if ( r_dlightMode->integer >= 2 )
        {
            idRenderSystemBackendLocal::BindToTMU( tr.shadowCubemaps[l].image, TB_SHADOWMAP2 );
        }
        
        ComputeTexMods( pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb );
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXMATRIX, texMatrix );
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXOFFTURB, texOffTurb );
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_TCGEN0, pStage->bundle[0].tcGen );
        
        // draw
        DrawElements( input->numIndexes, input->firstIndex );
        
        backEnd.pc.c_totalIndexes += tess.numIndexes;
        backEnd.pc.c_dlightIndexes += tess.numIndexes;
        backEnd.pc.c_dlightVertexes += tess.numVertexes;
    }
}

/*
==================
idRenderSystemShadeLocal::ProjectPshadowVBOGLSL
==================
*/
void idRenderSystemShadeLocal::ProjectPshadowVBOGLSL( void )
{
    S32	l, deformGen;
    F32	radius;
    vec3_t origin;
    vec5_t deformParams;
    
    shaderCommands_t* input = &tess;
    
    if ( !backEnd.refdef.num_pshadows )
    {
        return;
    }
    
    ComputeDeformValues( &deformGen, deformParams );
    
    for ( l = 0 ; l < backEnd.refdef.num_pshadows ; l++ )
    {
        pshadow_t*	ps;
        shaderProgram_t* sp;
        vec4_t vector;
        
        if ( !( tess.pshadowBits & ( 1 << l ) ) )
        {
            // this surface definately doesn't have any of this shadow
            continue;
        }
        
        ps = &backEnd.refdef.pshadows[l];
        VectorCopy( ps->lightOrigin, origin );
        radius = ps->lightRadius;
        
        sp = &tr.pshadowShader;
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        
        VectorCopy( origin, vector );
        vector[3] = 1.0f;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_LIGHTORIGIN, vector );
        
        VectorScale( ps->lightViewAxis[0], 1.0f / ps->viewRadius, vector );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_LIGHTFORWARD, vector );
        
        VectorScale( ps->lightViewAxis[1], 1.0f / ps->viewRadius, vector );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_LIGHTRIGHT, vector );
        
        VectorScale( ps->lightViewAxis[2], 1.0f / ps->viewRadius, vector );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_LIGHTUP, vector );
        
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_LIGHTRADIUS, radius );
        
        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        idRenderSystemBackendLocal::State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
        
        idRenderSystemBackendLocal::BindToTMU( tr.pshadowMaps[l], TB_DIFFUSEMAP );
        
        // draw
        DrawElements( input->numIndexes, input->firstIndex );
        
        backEnd.pc.c_totalIndexes += tess.numIndexes;
        //backEnd.pc.c_dlightIndexes += tess.numIndexes;
    }
}

/*
===================
idRenderSystemShadeLocal::FogPass

Blends a fog texture on top of everything else
===================
*/
void idRenderSystemShadeLocal::FogPass( void )
{
    S32 deformGen;
    F32	eyeT = 0;
    fog_t* fog;
    vec4_t color, fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
    shaderProgram_t* sp;
    vec5_t deformParams;
    
    ComputeDeformValues( &deformGen, deformParams );
    
    {
        S32 index = 0;
        
        if ( deformGen != DGEN_NONE )
        {
            index |= FOGDEF_USE_DEFORM_VERTEXES;
        }
        
        if ( glState.vertexAnimation )
        {
            index |= FOGDEF_USE_VERTEX_ANIMATION;
        }
        else if ( glState.boneAnimation )
        {
            index |= FOGDEF_USE_BONE_ANIMATION;
        }
        
        sp = &tr.fogShader[index];
    }
    
    backEnd.pc.c_fogDraws++;
    
    idRenderSystemGLSLLocal::BindProgram( sp );
    
    fog = tr.world->fogs + tess.fogNum;
    
    idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation );
    
    if ( glState.boneAnimation )
    {
        idRenderSystemGLSLLocal::SetUniformMat4BoneMatrix( sp, UNIFORM_BONEMATRIX, glState.boneMatrix, glState.boneAnimation );
    }
    
    idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_DEFORMGEN, deformGen );
    if ( deformGen != DGEN_NONE )
    {
        idRenderSystemGLSLLocal::SetUniformFloat5( sp, UNIFORM_DEFORMPARAMS, deformParams );
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
    }
    
    color[0] = ( ( U8* )( &fog->colorInt ) )[0] / 255.0f;
    color[1] = ( ( U8* )( &fog->colorInt ) )[1] / 255.0f;
    color[2] = ( ( U8* )( &fog->colorInt ) )[2] / 255.0f;
    color[3] = ( ( U8* )( &fog->colorInt ) )[3] / 255.0f;
    
    idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_COLOR, color );
    
    ComputeFogValues( fogDistanceVector, fogDepthVector, &eyeT );
    
    idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGDISTANCE, fogDistanceVector );
    idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGDEPTH, fogDepthVector );
    idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_FOGEYET, eyeT );
    
    if ( tess.shader->fogPass == FP_EQUAL )
    {
        idRenderSystemBackendLocal::State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
    }
    else
    {
        idRenderSystemBackendLocal::State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    }
    
    idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
    
    DrawElements( tess.numIndexes, tess.firstIndex );
}

/*
==================
idRenderSystemShadeLocal::CalcShaderVertexAttribs
==================
*/
U32 idRenderSystemShadeLocal::CalcShaderVertexAttribs( shaderCommands_t* input )
{
    U32 vertexAttribs = input->shader->vertexAttribs;
    
    if ( glState.vertexAnimation )
    {
        vertexAttribs |= ATTR_POSITION2;
        if ( vertexAttribs & ATTR_NORMAL )
        {
            vertexAttribs |= ATTR_NORMAL2;
            vertexAttribs |= ATTR_TANGENT2;
        }
    }
    
    return vertexAttribs;
}

/*
==================
idRenderSystemShadeLocal::SetMaterialBasedProperties
==================
*/
void idRenderSystemShadeLocal::SetMaterialBasedProperties( shaderProgram_t* sp, shaderStage_t* pStage )
{
    vec4_t local1;
    F32	specularScale = 1.0,  materialType = 0.0, parallaxScale = 1.0, cubemapScale = 0.0, isMetalic = 0.0;
    
    if ( pStage->isWater )
    {
        specularScale = 1.5;
        materialType = ( F32 )MATERIAL_WATER;
        parallaxScale = 2.0;
    }
    else
    {
        switch ( tess.shader->surfaceFlags & MATERIAL_MASK )
        {
            case MATERIAL_WATER:			// 13			// light covering of water on a surface
                specularScale = 1.0f;
                cubemapScale = 1.5f;
                materialType = ( F32 )MATERIAL_WATER;
                parallaxScale = 2.0f;
                break;
            case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
                specularScale = 0.53f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_SHORTGRASS;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_LONGGRASS:		// 6			// long jungle grass
                specularScale = 0.5f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_LONGGRASS;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_SAND:				// 8			// sandy beach
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_SAND;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_CARPET:			// 27			// lush carpet
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_CARPET;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_GRAVEL:			// 9			// lots of small stones
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_GRAVEL;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_ROCK:				// 23			//
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_ROCK;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_TILES:			// 26			// tiled floor
                specularScale = 0.86f;
                cubemapScale = 0.9f;
                materialType = ( F32 )MATERIAL_TILES;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_SOLIDWOOD;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_HOLLOWWOOD;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_SOLIDMETAL:		// 3			// solid girders
                specularScale = 0.92f;
                cubemapScale = 0.92f;
                materialType = ( F32 )MATERIAL_SOLIDMETAL;
                parallaxScale = 0.005f;
                isMetalic = 1.0f;
                break;
            case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- Used for weapons to force lower parallax...
                specularScale = 0.92f;
                cubemapScale = 0.92f;
                materialType = ( F32 )MATERIAL_HOLLOWMETAL;
                parallaxScale = 2.0f;
                isMetalic = 1.0f;
                break;
            case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_DRYLEAVES;
                parallaxScale = 0.0f;
                break;
            case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
                specularScale = 0.75f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_GREENLEAVES;
                parallaxScale = 0.0f; // GreenLeaves should NEVER be parallaxed.. It's used for surfaces with an alpha channel and parallax screws it up...
                break;
            case MATERIAL_FABRIC:			// 21			// Cotton sheets
                specularScale = 0.48f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_FABRIC;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_CANVAS:			// 22			// tent material
                specularScale = 0.45f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_CANVAS;
                parallaxScale = 2.5f;
                break;
            case MATERIAL_MARBLE:			// 12			// marble floors
                specularScale = 0.86f;
                cubemapScale = 1.0f;
                materialType = ( F32 )MATERIAL_MARBLE;
                parallaxScale = 2.0f;
                break;
            case MATERIAL_SNOW:				// 14			// freshly laid snow
                specularScale = 0.65f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_SNOW;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_MUD:				// 17			// wet soil
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_MUD;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_DIRT:				// 7			// hard mud
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_DIRT;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
                specularScale = 0.3f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_CONCRETE;
                parallaxScale = 3.0f;
                break;
            case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
                specularScale = 0.2f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_FLESH;
                parallaxScale = 1.0f;
                break;
            case MATERIAL_RUBBER:			// 24			// hard tire like rubber
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_RUBBER;
                parallaxScale = 1.0f;
                break;
            case MATERIAL_PLASTIC:			// 25			//
                specularScale = 0.88f;
                cubemapScale = 0.5f;
                materialType = ( F32 )MATERIAL_PLASTIC;
                parallaxScale = 1.0f;
                break;
            case MATERIAL_PLASTER:			// 28			// drywall style plaster
                specularScale = 0.4f;
                cubemapScale = 0.0f;
                materialType = ( F32 )MATERIAL_PLASTER;
                parallaxScale = 2.0f;
                break;
            case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
                specularScale = 0.88f;
                cubemapScale = 1.0f;
                materialType = ( F32 )MATERIAL_SHATTERGLASS;
                parallaxScale = 1.0f;
                break;
            case MATERIAL_ARMOR:			// 30			// body armor
                specularScale = 0.4f;
                cubemapScale = 2.0f;
                materialType = ( F32 )MATERIAL_ARMOR;
                parallaxScale = 2.0f;
                isMetalic = 1.0f;
                break;
            case MATERIAL_ICE:				// 15			// packed snow/solid ice
                specularScale = 0.9f;
                cubemapScale = 0.8f;
                parallaxScale = 2.0f;
                materialType = ( F32 )MATERIAL_ICE;
                break;
            case MATERIAL_GLASS:			// 10			//
                specularScale = 0.95f;
                cubemapScale = 1.0f;
                materialType = ( F32 )MATERIAL_GLASS;
                parallaxScale = 1.0f;
                break;
            case MATERIAL_BPGLASS:			// 18			// bulletproof glass
                specularScale = 0.93f;
                cubemapScale = 0.93f;
                materialType = ( F32 )MATERIAL_BPGLASS;
                parallaxScale = 1.0f;
                break;
            case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
                specularScale = 0.92f;
                cubemapScale = 0.92f;
                materialType = ( F32 )MATERIAL_COMPUTER;
                parallaxScale = 2.0f;
                break;
            default:
                specularScale = 0.0f;
                cubemapScale = 0.0f;
                materialType = 0.0f;
                parallaxScale = 1.0f;
                break;
        }
    }
    bool realNormalMap = false;
    
    if ( pStage->bundle[TB_NORMALMAP].image[0] )
    {
        realNormalMap = true;
    }
    
    VectorSet4( local1, parallaxScale, ( F32 )pStage->hasSpecular, specularScale, materialType );
    idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_LOCAL1, local1 );
    
    //idRenderSystemGLSLLocal::SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
    idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )backEnd.refdef.floatTime );
}

/*
==================
idRenderSystemShadeLocal::SetStageImageDimensions
==================
*/
void idRenderSystemShadeLocal::SetStageImageDimensions( shaderProgram_t* sp, shaderStage_t* pStage )
{
    vec2_t dimensions;
    
    if ( !pStage->bundle[0].image[0] )
    {
        // argh!
        pStage->bundle[0].image[0] = tr.whiteImage;
    }
    
    dimensions[0] = ( F32 )pStage->bundle[0].image[0]->width;
    dimensions[1] = ( F32 )pStage->bundle[0].image[0]->height;
    
    if ( pStage->bundle[TB_DIFFUSEMAP].image[0] )
    {
        dimensions[0] = ( F32 )pStage->bundle[TB_DIFFUSEMAP].image[0]->width;
        dimensions[1] = ( F32 )pStage->bundle[TB_DIFFUSEMAP].image[0]->height;
    }
    else if ( pStage->bundle[TB_NORMALMAP].image[0] )
    {
        dimensions[0] = ( F32 )pStage->bundle[TB_NORMALMAP].image[0]->width;
        dimensions[1] = ( F32 )pStage->bundle[TB_NORMALMAP].image[0]->height;
    }
    else if ( pStage->bundle[TB_SPECULARMAP].image[0] )
    {
        dimensions[0] = ( F32 )pStage->bundle[TB_SPECULARMAP].image[0]->width;
        dimensions[1] = ( F32 )pStage->bundle[TB_SPECULARMAP].image[0]->height;
    }
    
    idRenderSystemGLSLLocal::SetUniformVec2( sp, UNIFORM_DIMENSIONS, dimensions );
}

/*
==================
idRenderSystemShadeLocal::GetCullType
==================
*/
cullType_t idRenderSystemShadeLocal::GetCullType( const viewParms_t* viewParms, const trRefEntity_t* refEntity, cullType_t shaderCullType )
{
    assert( refEntity );
    
    cullType_t cullType = CT_TWO_SIDED;
    if ( !backEnd.projection2D )
    {
        if ( shaderCullType != CT_TWO_SIDED )
        {
            bool cullFront = ( shaderCullType == CT_FRONT_SIDED );
            if ( viewParms->isMirror )
            {
                cullFront = !cullFront;
            }
            
            if ( refEntity->mirrored )
            {
                cullFront = !cullFront;
            }
            
            if ( viewParms->flags & VPF_DEPTHSHADOW )
            {
                cullFront = !cullFront;
            }
            
            cullType = ( cullFront ? CT_FRONT_SIDED : CT_BACK_SIDED );
        }
    }
    
    return cullType;
}

/*
==================
idRenderSystemShadeLocal::IterateStagesGeneric
==================
*/
void idRenderSystemShadeLocal::IterateStagesGeneric( shaderCommands_t* input )
{
    S32 stage;
    
    vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
    F32 eyeT = 0;
    
    S32 deformGen;
    vec5_t deformParams;
    
    bool renderToCubemap = tr.renderCubeFbo && glState.currentFBO == tr.renderCubeFbo;
    cullType_t cullType = GetCullType( &backEnd.viewParms, backEnd.currentEntity, input->shader->cullType );
    
    if ( renderToCubemap )
    {
        cullType = CT_TWO_SIDED;
    }
    
    ComputeDeformValues( &deformGen, deformParams );
    
    ComputeFogValues( fogDistanceVector, fogDepthVector, &eyeT );
    
    for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
    {
        shaderStage_t* pStage = input->xstages[stage];
        shaderProgram_t* sp;
        vec4_t texMatrix, texOffTurb;
        
        if ( !pStage )
        {
            break;
        }
        
        if ( backEnd.depthFill )
        {
            if ( pStage->glslShaderGroup == tr.lightallShader )
            {
                S32 index = 0;
                
                if ( backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity )
                {
                    if ( glState.boneAnimation )
                    {
                        index |= LIGHTDEF_ENTITY_BONE_ANIMATION;
                    }
                    else
                    {
                        index |= LIGHTDEF_ENTITY_VERTEX_ANIMATION;
                    }
                }
                
                if ( pStage->stateBits & GLS_ATEST_BITS )
                {
                    index |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
                }
                
                sp = &pStage->glslShaderGroup[index];
            }
            else
            {
                S32 shaderAttribs = 0;
                
                if ( tess.shader->numDeforms && !ShaderRequiresCPUDeforms( tess.shader ) )
                {
                    shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
                }
                
                if ( glState.vertexAnimation )
                {
                    shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
                }
                else if ( glState.boneAnimation )
                {
                    shaderAttribs |= GENERICDEF_USE_BONE_ANIMATION;
                }
                
                if ( pStage->stateBits & GLS_ATEST_BITS )
                {
                    shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
                    shaderAttribs |= GENERICDEF_USE_RGBAGEN;
                }
                
                sp = &tr.genericShader[shaderAttribs];
            }
        }
        else if ( pStage->glslShaderGroup == tr.lightallShader )
        {
            S32 index = pStage->glslShaderIndex;
            
            if ( backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity )
            {
                if ( glState.boneAnimation )
                {
                    index |= LIGHTDEF_ENTITY_BONE_ANIMATION;
                }
                else
                {
                    index |= LIGHTDEF_ENTITY_VERTEX_ANIMATION;
                }
            }
            
            if ( r_sunlightMode->integer && ( backEnd.viewParms.flags & VPF_USESUNLIGHT ) && ( index & LIGHTDEF_LIGHTTYPE_MASK ) )
            {
                index |= LIGHTDEF_USE_SHADOWMAP;
            }
            
            if ( r_lightmap->integer && ( ( index & LIGHTDEF_LIGHTTYPE_MASK ) == LIGHTDEF_USE_LIGHTMAP ) )
            {
                index = LIGHTDEF_USE_TCGEN_AND_TCMOD;
            }
            
            sp = &pStage->glslShaderGroup[index];
            
            backEnd.pc.c_lightallDraws++;
        }
        else
        {
            sp = idRenderSystemGLSLLocal::GetGenericShaderProgram( stage );
            
            backEnd.pc.c_genericDraws++;
        }
        
        if ( pStage->isWater )
        {
            sp = &tr.waterShader;
            pStage->glslShaderGroup = &tr.waterShader;
            idRenderSystemGLSLLocal::BindProgram( sp );
            
            SetMaterialBasedProperties( sp, pStage );
            
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
        }
        
        if ( r_proceduralSun->integer && tess.shader == tr.sunShader )
        {
            // Special case for procedural sun...
            sp = &tr.sunPassShader;
            idRenderSystemGLSLLocal::BindProgram( sp );
            
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
        }
        
        SetMaterialBasedProperties( sp, pStage );
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        SetStageImageDimensions( sp, pStage );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin );
        idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_LOCALVIEWORIGIN, backEnd.orientation.viewOrigin );
        
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation );
        
        if ( glState.boneAnimation )
        {
            idRenderSystemGLSLLocal::SetUniformMat4BoneMatrix( sp, UNIFORM_BONEMATRIX, glState.boneMatrix, glState.boneAnimation );
        }
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_DEFORMGEN, deformGen );
        if ( deformGen != DGEN_NONE )
        {
            idRenderSystemGLSLLocal::SetUniformFloat5( sp, UNIFORM_DEFORMPARAMS, deformParams );
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
        }
        
        if ( input->fogNum )
        {
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGDISTANCE, fogDistanceVector );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGDEPTH, fogDepthVector );
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_FOGEYET, eyeT );
        }
        
        idRenderSystemBackendLocal::State( pStage->stateBits );
        if ( ( pStage->stateBits & GLS_ATEST_BITS ) == GLS_ATEST_GT_0 )
        {
            idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 1 );
        }
        else if ( ( pStage->stateBits & GLS_ATEST_BITS ) == GLS_ATEST_LT_80 )
        {
            idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 2 );
        }
        else if ( ( pStage->stateBits & GLS_ATEST_BITS ) == GLS_ATEST_GE_80 )
        {
            idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 3 );
        }
        else
        {
            idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
        }
        
        {
            vec4_t baseColor;
            vec4_t vertColor;
            
            ComputeShaderColors( pStage, baseColor, vertColor, pStage->stateBits );
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_BASECOLOR, baseColor );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_VERTCOLOR, vertColor );
        }
        
        if ( pStage->rgbGen == CGEN_LIGHTING_DIFFUSE )
        {
            vec4_t vec;
            
            VectorScale( backEnd.currentEntity->ambientLight, 1.0f / 255.0f, vec );
            idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_AMBIENTLIGHT, vec );
            
            VectorScale( backEnd.currentEntity->directedLight, 1.0f / 255.0f, vec );
            idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_DIRECTEDLIGHT, vec );
            
            VectorCopy( backEnd.currentEntity->lightDir, vec );
            vec[3] = 0.0f;
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_LIGHTORIGIN, vec );
            idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_MODELLIGHTDIR, backEnd.currentEntity->modelLightDir );
            
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_LIGHTRADIUS, 0.0f );
        }
        
        if ( pStage->alphaGen == AGEN_PORTAL )
        {
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_PORTALRANGE, tess.shader->portalRange );
        }
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_COLORGEN, pStage->rgbGen );
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHAGEN, pStage->alphaGen );
        
        if ( input->fogNum )
        {
            vec4_t fogColorMask;
            
            ComputeFogColorMask( pStage, fogColorMask );
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_FOGCOLORMASK, fogColorMask );
        }
        
        if ( r_lightmap->integer )
        {
            vec4_t v;
            VectorSet4( v, 1.0f, 0.0f, 0.0f, 1.0f );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXMATRIX, v );
            VectorSet4( v, 0.0f, 0.0f, 0.0f, 0.0f );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXOFFTURB, v );
            
            idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_TCGEN0, TCGEN_LIGHTMAP );
        }
        else
        {
            ComputeTexMods( pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXMATRIX, texMatrix );
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXOFFTURB, texOffTurb );
            
            idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_TCGEN0, pStage->bundle[0].tcGen );
            if ( pStage->bundle[0].tcGen == TCGEN_VECTOR )
            {
                vec3_t vec;
                
                VectorCopy( pStage->bundle[0].tcGenVectors[0], vec );
                idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_TCGEN0VECTOR0, vec );
                VectorCopy( pStage->bundle[0].tcGenVectors[1], vec );
                idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_TCGEN0VECTOR1, vec );
            }
        }
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
        
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_NORMALSCALE, pStage->normalScale );
        
        {
            vec4_t specularScale;
            Vector4Copy( pStage->specularScale, specularScale );
            
            if ( renderToCubemap )
            {
                // force specular to nonmetal if rendering cubemaps
                if ( r_pbr->integer )
                {
                    specularScale[1] = 0.0f;
                }
            }
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_SPECULARSCALE, specularScale );
        }
        
        // do multitexture
        bool enableCubeMaps = ( r_cubeMapping->integer && !( tr.viewParms.flags & VPF_NOCUBEMAPS ) && input->cubemapIndex );
        
        if ( backEnd.depthFill )
        {
            if ( !( pStage->stateBits & GLS_ATEST_BITS ) )
            {
                idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_COLORMAP );
            }
            else if ( pStage->bundle[TB_COLORMAP].image[0] != 0 )
            {
                BindAnimatedImageToTMU( &pStage->bundle[TB_COLORMAP], TB_COLORMAP );
            }
        }
        
        else if ( pStage->glslShaderGroup == tr.lightallShader )
        {
            S32 i;
            vec4_t enableTextures;
            
            if ( r_sunlightMode->integer && ( backEnd.viewParms.flags & VPF_USESUNLIGHT ) && ( pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK ) )
            {
                // FIXME: screenShadowImage is NULL if no framebuffers
                if ( tr.screenShadowImage )
                {
                    idRenderSystemBackendLocal::BindToTMU( tr.screenShadowImage, TB_SHADOWMAP );
                }
                
                idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol );
                
                if ( r_pbr->integer )
                {
                    vec3_t color;
                    
                    color[0] = backEnd.refdef.sunCol[0] * backEnd.refdef.sunCol[0];
                    color[1] = backEnd.refdef.sunCol[1] * backEnd.refdef.sunCol[1];
                    color[2] = backEnd.refdef.sunCol[2] * backEnd.refdef.sunCol[2];
                    idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_PRIMARYLIGHTCOLOR, color );
                }
                else
                {
                    idRenderSystemGLSLLocal::SetUniformVec3( sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol );
                }
                
                idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_PRIMARYLIGHTORIGIN,  backEnd.refdef.sunDir );
            }
            
            VectorSet4( enableTextures, 0, 0, 0, 0 );
            if ( ( r_lightmap->integer == 1 || r_lightmap->integer == 2 ) && pStage->bundle[TB_LIGHTMAP].image[0] )
            {
                for ( i = 0; i < NUM_TEXTURE_BUNDLES; i++ )
                {
                    if ( i == TB_COLORMAP )
                    {
                        BindAnimatedImageToTMU( &pStage->bundle[TB_LIGHTMAP], i );
                    }
                    else
                    {
                        idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, i );
                    }
                }
            }
            else if ( r_lightmap->integer == 3 && pStage->bundle[TB_DELUXEMAP].image[0] )
            {
                for ( i = 0; i < NUM_TEXTURE_BUNDLES; i++ )
                {
                    if ( i == TB_COLORMAP )
                    {
                        BindAnimatedImageToTMU( &pStage->bundle[TB_DELUXEMAP], i );
                    }
                    else
                    {
                        idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, i );
                    }
                }
            }
            else
            {
                bool light = ( pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK ) != 0;
                bool fastLight = !( r_normalMapping->integer || r_specularMapping->integer );
                
                if ( pStage->bundle[TB_DIFFUSEMAP].image[0] )
                {
                    BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP );
                }
                
                if ( pStage->bundle[TB_LIGHTMAP].image[0] )
                {
                    BindAnimatedImageToTMU( &pStage->bundle[TB_LIGHTMAP], TB_LIGHTMAP );
                }
                
                // bind textures that are sampled and used in the glsl shader, and
                // bind whiteImage to textures that are sampled but zeroed in the glsl shader
                //
                // alternatives:
                //  - use the last bound texture
                //     -> costs more to sample a higher res texture then throw out the result
                //  - disable texture sampling in glsl shader with #ifdefs, as before
                //     -> increases the number of shaders that must be compiled
                //
                if ( ( light ) && !fastLight )
                {
                    if ( pStage->bundle[TB_NORMALMAP].image[0] )
                    {
                        BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP], TB_NORMALMAP );
                        enableTextures[0] = 1.0f;
                    }
                    else if ( r_normalMapping->integer )
                    {
                        idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_NORMALMAP );
                    }
                    
                    if ( pStage->bundle[TB_DELUXEMAP].image[0] )
                    {
                        BindAnimatedImageToTMU( &pStage->bundle[TB_DELUXEMAP], TB_DELUXEMAP );
                        enableTextures[1] = 1.0f;
                    }
                    else if ( r_deluxeMapping->integer )
                    {
                        idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_DELUXEMAP );
                    }
                    
                    if ( pStage->bundle[TB_SPECULARMAP].image[0] )
                    {
                        BindAnimatedImageToTMU( &pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP );
                        enableTextures[2] = 1.0f;
                    }
                    else if ( r_specularMapping->integer )
                    {
                        idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_SPECULARMAP );
                    }
                }
                
                enableTextures[3] = ( r_cubeMapping->integer && !( tr.viewParms.flags & VPF_NOCUBEMAPS ) && input->cubemapIndex ) ? 1.0f : 0.0f;
                if ( enableCubeMaps )
                {
                    enableTextures[3] = 1.0f;
                }
                
            }
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_ENABLETEXTURES, enableTextures );
        }
        else if ( pStage->bundle[1].image[0] != 0 )
        {
            BindAnimatedImageToTMU( &pStage->bundle[0], 0 );
            BindAnimatedImageToTMU( &pStage->bundle[1], 1 );
        }
        else
        {
            // set state
            BindAnimatedImageToTMU( &pStage->bundle[0], 0 );
        }
        
        // testing cube map
        if ( enableCubeMaps )
        {
            vec4_t vec;
            cubemap_t* cubemap = &tr.cubemaps[input->cubemapIndex - 1];
            
            // FIXME: cubemap image could be NULL if cubemap isn't renderer or loaded
            if ( cubemap->image )
            {
                idRenderSystemBackendLocal::BindToTMU( cubemap->image, TB_CUBEMAP );
            }
            
            idRenderSystemBackendLocal::BindToTMU( tr.envBrdfImage, TB_ENVBRDFMAP );
            
            vec[0] = cubemap->origin[0] - backEnd.viewParms.orientation.origin[0];
            vec[1] = cubemap->origin[1] - backEnd.viewParms.orientation.origin[1];
            vec[2] = cubemap->origin[2] - backEnd.viewParms.orientation.origin[2];
            vec[3] = 1.0f;
            
            VectorScale4( vec, 1.0f / cubemap->parallaxRadius, vec );
            
            idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_CUBEMAPINFO, vec );
        }
        
        // draw
        DrawElements( input->numIndexes, input->firstIndex );
        
        // allow skipping out to show just lightmaps during development
        if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap ) )
        {
            break;
        }
        
        if ( backEnd.depthFill )
        {
            break;
        }
    }
}

/*
==================
idRenderSystemShadeLocal::RenderShadowmap
==================
*/
void idRenderSystemShadeLocal::RenderShadowmap( shaderCommands_t* input )
{
    S32 deformGen;
    vec5_t deformParams;
    
    ComputeDeformValues( &deformGen, deformParams );
    
    {
        shaderProgram_t* sp = &tr.shadowmapShader[0];
        
        if ( glState.vertexAnimation )
        {
            sp = &tr.shadowmapShader[SHADOWMAPDEF_USE_VERTEX_ANIMATION];
        }
        else if ( glState.boneAnimation )
        {
            sp = &tr.shadowmapShader[SHADOWMAPDEF_USE_BONE_ANIMATION];
        }
        
        vec4_t vector;
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
        
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation );
        
        if ( glState.boneAnimation )
        {
            idRenderSystemGLSLLocal::SetUniformMat4BoneMatrix( sp, UNIFORM_BONEMATRIX, glState.boneMatrix, glState.boneAnimation );
        }
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_DEFORMGEN, deformGen );
        if ( deformGen != DGEN_NONE )
        {
            idRenderSystemGLSLLocal::SetUniformFloat5( sp, UNIFORM_DEFORMPARAMS, deformParams );
            idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_TIME, ( F32 )tess.shaderTime );
        }
        
        VectorCopy( backEnd.viewParms.orientation.origin, vector );
        vector[3] = 1.0f;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_LIGHTORIGIN, vector );
        idRenderSystemGLSLLocal::SetUniformFloat( sp, UNIFORM_LIGHTRADIUS, backEnd.viewParms.zFar );
        
        idRenderSystemBackendLocal::State( 0 );
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
        
        //
        // do multitexture
        //
        //if ( pStage->glslShaderGroup )
        {
            //
            // draw
            //
            
            DrawElements( input->numIndexes, input->firstIndex );
        }
    }
}

/*
==================
idRenderSystemShadeLocal::StageIteratorGeneric
==================
*/
void idRenderSystemShadeLocal::StageIteratorGeneric( void )
{
    shaderCommands_t* input;
    U32 vertexAttribs = 0;
    
    input = &tess;
    
    if ( !input->numVertexes || !input->numIndexes )
    {
        return;
    }
    
    if ( tess.useInternalVao )
    {
        DeformTessGeometry();
    }
    
    vertexAttribs = CalcShaderVertexAttribs( input );
    
    if ( tess.useInternalVao )
    {
        idRenderSystemVaoLocal::UpdateTessVao( vertexAttribs );
    }
    else
    {
        backEnd.pc.c_staticVaoDraws++;
    }
    
    // log this call
    if ( r_logFile->integer )
    {
        // don't just call LogComment, or we will get
        // a call to va() every frame!
        idRenderSystemGlimpLocal::LogComment( reinterpret_cast< UTF8* >( va( "--- idRenderSystemShadeLocal::StageIteratorGeneric( %s ) ---\n", tess.shader->name ) ) );
    }
    
    // set face culling appropriately
    if ( input->shader->cullType == CT_TWO_SIDED )
    {
        idRenderSystemBackendLocal::Cull( CT_TWO_SIDED );
    }
    else
    {
        bool cullFront = ( input->shader->cullType == CT_FRONT_SIDED );
        
        if ( backEnd.viewParms.flags & VPF_DEPTHSHADOW )
        {
            cullFront = !cullFront;
        }
        
        if ( backEnd.viewParms.isMirror )
        {
            cullFront = !cullFront;
        }
        
        if ( backEnd.currentEntity && backEnd.currentEntity->mirrored )
        {
            cullFront = !cullFront;
        }
        
        if ( cullFront )
        {
            idRenderSystemBackendLocal::Cull( CT_FRONT_SIDED );
        }
        else
        {
            idRenderSystemBackendLocal::Cull( CT_BACK_SIDED );
        }
        
        if ( tr.renderCubeFbo && glState.currentFBO == tr.renderCubeFbo )
        {
            cullFront = ( bool )CT_TWO_SIDED;
        }
    }
    
    // set polygon offset if necessary
    if ( input->shader->polygonOffset )
    {
        qglEnable( GL_POLYGON_OFFSET_FILL );
    }
    
    // Set up any special shaders needed for this surface/contents type...
    if ( ( tess.shader->contentFlags & CONTENTS_WATER ) )
    {
        // In case it is already set, no need looping more then once on the same shader...
        if ( input->xstages[0]->isWater != true )
        {
            for ( S32 stage = 0; stage < MAX_SHADER_STAGES; stage++ )
            {
                if ( input->xstages[stage] )
                {
                    input->xstages[stage]->isWater = true;
                }
            }
        }
    }
    
    // render depth if in depthfill mode
    if ( backEnd.depthFill )
    {
        IterateStagesGeneric( input );
        
        // reset polygon offset
        if ( input->shader->polygonOffset )
        {
            qglDisable( GL_POLYGON_OFFSET_FILL );
        }
        
        return;
    }
    
    // render shadowmap if in shadowmap mode
    if ( backEnd.viewParms.flags & VPF_SHADOWMAP )
    {
        if ( input->shader->sort == SS_OPAQUE )
        {
            RenderShadowmap( input );
        }
        //
        // reset polygon offset
        //
        if ( input->shader->polygonOffset )
        {
            qglDisable( GL_POLYGON_OFFSET_FILL );
        }
        
        return;
    }
    
    // call shader function
    IterateStagesGeneric( input );
    
    // pshadows!
    if ( glRefConfig.framebufferObject && r_shadows->integer == 4 && tess.pshadowBits
            && tess.shader->sort <= SS_OPAQUE && !( tess.shader->surfaceFlags & ( /*SURF_NODLIGHT | */SURF_SKY ) ) )
    {
        ProjectPshadowVBOGLSL();
    }
    
    //
    // now do any dynamic lighting needed
    //
    if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE && r_lightmap->integer == 0 && !( tess.shader->surfaceFlags & ( SURF_NODLIGHT | SURF_SKY ) ) )
    {
        if ( tess.shader->numUnfoggedPasses == 1 && tess.xstages[0]->glslShaderGroup == tr.lightallShader
                && ( tess.xstages[0]->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK ) && r_dlightMode->integer )
        {
            ForwardDlight();
        }
        else
        {
            ProjectDlightTexture();
        }
    }
    
    // now do fog
    if ( tess.fogNum && tess.shader->fogPass )
    {
        FogPass();
    }
    
    // reset polygon offset
    if ( input->shader->polygonOffset )
    {
        qglDisable( GL_POLYGON_OFFSET_FILL );
    }
}

/*
==================
idRenderSystemShadeLocal::EndSurface
==================
*/
void idRenderSystemShadeLocal::EndSurface( void )
{
    shaderCommands_t* input;
    
    input = &tess;
    
    if ( input->numIndexes == 0 || input->numVertexes == 0 )
    {
        return;
    }
    
    if ( input->indexes[SHADER_MAX_INDEXES - 1] != 0 )
    {
        Com_Error( ERR_DROP, "idRenderSystemShadeLocal::EndSurface() - SHADER_MAX_INDEXES hit" );
    }
    if ( input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0 )
    {
        Com_Error( ERR_DROP, "idRenderSystemShadeLocal::EndSurface() - SHADER_MAX_VERTEXES hit" );
    }
    
    if ( tess.shader == tr.shadowShader )
    {
        idRenderSystemShadowsLocal::ShadowTessEnd();
        return;
    }
    
    // for debugging of sort order issues, stop rendering after a given sort value
    if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort )
    {
        return;
    }
    
    if ( tess.useCacheVao )
    {
        // upload indexes now
        idRenderSystemVaoLocal::CacheCommit();
    }
    
    // update performance counters
    backEnd.pc.c_shaders++;
    backEnd.pc.c_vertexes += tess.numVertexes;
    backEnd.pc.c_indexes += tess.numIndexes;
    backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;
    
    //
    // call off to shader specific tess end function
    //
    tess.currentStageIteratorFunc();
    
    //
    // draw debugging stuff
    //
    if ( r_showtris->integer )
    {
        DrawTris( input );
    }
    if ( r_shownormals->integer )
    {
        DrawNormals( input );
    }
    // clear shader so we can tell we don't have any unclosed surfaces
    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.firstIndex = 0;
    
    idRenderSystemGlimpLocal::LogComment( "----------\n" );
}

/*
==================
idRenderSystemShadeLocal::WaveValue
==================
*/
F64 idRenderSystemShadeLocal::WaveValue( const F32* table, F64 base, F64 amplitude, F64 phase, F64 freq )
{
    // the original code did a double to 32-bit int conversion of x
    const F64 x = ( phase + tess.shaderTime * freq ) * FUNCTABLE_SIZE;
    const S32 i = ( S32 )( ( int64_t )x & ( int64_t )FUNCTABLE_MASK );
    const F64 r = base + table[i] * amplitude;
    
    return r;
}

/*
==================
idRenderSystemShadeLocal::TableForFunc
==================
*/
const F32* idRenderSystemShadeLocal::TableForFunc( genFunc_t func )
{
    switch ( func )
    {
        case GF_SIN:
            return tr.sinTable;
        case GF_TRIANGLE:
            return tr.triangleTable;
        case GF_SQUARE:
            return tr.squareTable;
        case GF_SAWTOOTH:
            return tr.sawToothTable;
        case GF_INVERSE_SAWTOOTH:
            return tr.inverseSawToothTable;
        case GF_NOISE:
            return tr.noiseTable;
        case GF_NONE:
        default:
            break;
    }
    
    Com_Error( ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'", func, tess.shader->name );
    return NULL;
}

/*
==================
idRenderSystemShadeLocal::EvalWaveForm

Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
==================
*/
F32 idRenderSystemShadeLocal::EvalWaveForm( const waveForm_t* wf )
{
    return ( F32 )WaveValue( TableForFunc( wf->func ), wf->base, wf->amplitude, wf->phase, wf->frequency );
}

/*
==================
idRenderSystemShadeLocal::EvalWaveFormClamped
==================
*/
F32 idRenderSystemShadeLocal::EvalWaveFormClamped( const waveForm_t* wf )
{
    F32 glow = EvalWaveForm( wf );
    
    if ( glow < 0 )
    {
        return 0;
    }
    
    if ( glow > 1 )
    {
        return 1;
    }
    
    return glow;
}

/*
==================
idRenderSystemShadeLocal::CalcStretchTexMatrix
==================
*/
void idRenderSystemShadeLocal::CalcStretchTexMatrix( const waveForm_t* wf, F32* matrix )
{
    F32 p;
    
    p = 1.0f / EvalWaveForm( wf );
    
    matrix[0] = p;
    matrix[2] = 0;
    matrix[4] = 0.5f - 0.5f * p;
    matrix[1] = 0;
    matrix[3] = p;
    matrix[5] = 0.5f - 0.5f * p;
}


/*
========================
idRenderSystemShadeLocal::CalcDeformVertexes
========================
*/
void idRenderSystemShadeLocal::CalcDeformVertexes( deformStage_t* ds )
{
    F32* xyz = ( F32* )tess.xyz;
    S16* normal = ( S16* )tess.normal;
    vec3_t offset;
    
    if ( ds->deformationWave.frequency == 0 )
    {
        const F32 scale = EvalWaveForm( &ds->deformationWave );
        
        for ( S32 i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
        {
            idRenderSystemVaoLocal::VaoUnpackNormal( offset, normal );
            
            xyz[0] += offset[0] * scale;
            xyz[1] += offset[1] * scale;
            xyz[2] += offset[2] * scale;
        }
    }
    else
    {
        const F32* table = TableForFunc( ds->deformationWave.func );
        
        for ( S32 i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
        {
            F32 off = ( xyz[0] + xyz[1] + xyz[2] ) * ds->deformationSpread;
            
            const F32 scale = ( F32 )WaveValue( table, ds->deformationWave.base,
                                                ds->deformationWave.amplitude,
                                                ds->deformationWave.phase + off,
                                                ds->deformationWave.frequency );
                                                
            idRenderSystemVaoLocal::VaoUnpackNormal( offset, normal );
            
            xyz[0] += offset[0] * scale;
            xyz[1] += offset[1] * scale;
            xyz[2] += offset[2] * scale;
        }
    }
}

/*
=========================
idRenderSystemShadeLocal::CalcDeformNormals

Wiggle the normals for wavy environment mapping
=========================
*/
void idRenderSystemShadeLocal::CalcDeformNormals( deformStage_t* ds )
{
    S32 i;
    F32	scale;
    const F32* xyz = ( const F32* )tess.xyz;
    S16* normal = tess.normal[0];
    
    for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
    {
        vec3_t fNormal;
        
        idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, normal );
        
        scale = 0.98f;
        scale = idRenderSystemNoiseLocal::NoiseGet4f( xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
        fNormal[0] += ds->deformationWave.amplitude * scale;
        
        scale = 0.98f;
        scale = idRenderSystemNoiseLocal::NoiseGet4f( 100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
        fNormal[1] += ds->deformationWave.amplitude * scale;
        
        scale = 0.98f;
        scale = idRenderSystemNoiseLocal::NoiseGet4f( 200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
        fNormal[2] += ds->deformationWave.amplitude * scale;
        
        VectorNormalizeFast( fNormal );
        
        idRenderSystemVaoLocal::VaoPackNormal( normal, fNormal );
    }
}

/*
========================
idRenderSystemShadeLocal::CalcBulgeVertexes
========================
*/
void idRenderSystemShadeLocal::CalcBulgeVertexes( deformStage_t* ds )
{
    S32 i;
    const F32* st = ( const F32* )tess.texCoords[0];
    F32* xyz = ( F32* )tess.xyz;
    S16* normal = tess.normal[0];
    
    F64 now = backEnd.refdef.time * 0.001 * ds->bulgeSpeed;
    
    for ( i = 0; i < tess.numVertexes; i++, xyz += 4, st += 2, normal += 4 )
    {
        S64	off;
        F32 scale;
        vec3_t fNormal;
        
        idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, normal );
        
        off = ( S64 )( ( FUNCTABLE_SIZE / ( M_PI * 2 ) ) * ( st[0] * ds->bulgeWidth + now ) );
        
        scale = tr.sinTable[off & FUNCTABLE_MASK] * ds->bulgeHeight;
        
        xyz[0] += fNormal[0] * scale;
        xyz[1] += fNormal[1] * scale;
        xyz[2] += fNormal[2] * scale;
    }
}


/*
======================
idRenderSystemShadeLocal::CalcMoveVertexes

A deformation that can move an entire surface along a wave path
======================
*/
void idRenderSystemShadeLocal::CalcMoveVertexes( deformStage_t* ds )
{
    const F32* table = TableForFunc( ds->deformationWave.func );
    const F64 scale = WaveValue( table, ds->deformationWave.base,
                                 ds->deformationWave.amplitude,
                                 ds->deformationWave.phase,
                                 ds->deformationWave.frequency );
                                 
    vec3_t offset;
    VectorScale( ds->moveVector, ( F32 )scale, offset );
    
    F32* xyz = ( F32* )tess.xyz;
    for ( S32 i = 0; i < tess.numVertexes; i++, xyz += 4 )
    {
        VectorAdd( xyz, offset, xyz );
    }
}

/*
=============
idRenderSystemShadeLocal::DeformText

Change a polygon into a bunch of text polygons
=============
*/
void idRenderSystemShadeLocal::DeformText( StringEntry text )
{
    S32	i, len, ch;
    F32	color[4], bottom, top;
    vec3_t origin, width, height, mid, fNormal;
    
    height[0] = 0;
    height[1] = 0;
    height[2] = -1;
    
    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, tess.normal[0] );
    CrossProduct( fNormal, height, width );
    
    // find the midpoint of the box
    VectorClear( mid );
    bottom = 999999;
    top = -999999;
    for ( i = 0; i < 4; i++ )
    {
        VectorAdd( tess.xyz[i], mid, mid );
        if ( tess.xyz[i][2] < bottom )
        {
            bottom = tess.xyz[i][2];
        }
        if ( tess.xyz[i][2] > top )
        {
            top = tess.xyz[i][2];
        }
    }
    VectorScale( mid, 0.25f, origin );
    
    // determine the individual character size
    height[0] = 0;
    height[1] = 0;
    height[2] = ( top - bottom ) * 0.5f;
    
    VectorScale( width, height[2] * -0.75f, width );
    
    // determine the starting position
    len = ( S32 )::strlen( text );
    VectorMA( origin, ( len - 1 ), width, origin );
    
    // clear the shader indexes
    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.firstIndex = 0;
    
    color[0] = color[1] = color[2] = color[3] = 1.0f;
    
    // draw each character
    for ( i = 0; i < len; i++ )
    {
        ch = text[i];
        ch &= 255;
        
        if ( ch != ' ' )
        {
            S32	row, col;
            F32	frow, fcol, size;
            
            row = ch >> 4;
            col = ch & 15;
            
            frow = row * 0.0625f;
            fcol = col * 0.0625f;
            size = 0.0625f;
            
            idRenderSystemSurfaceLocal::AddQuadStampExt( origin, width, height, color, fcol, frow, fcol + size, frow + size );
        }
        
        VectorMA( origin, -2, width, origin );
    }
}

/*
==================
idRenderSystemShadeLocal::GlobalVectorToLocal
==================
*/
void idRenderSystemShadeLocal::GlobalVectorToLocal( const vec3_t in, vec3_t out )
{
    out[0] = DotProduct( in, backEnd.orientation.axis[0] );
    out[1] = DotProduct( in, backEnd.orientation.axis[1] );
    out[2] = DotProduct( in, backEnd.orientation.axis[2] );
}

/*
=====================
idRenderSystemShadeLocal::AutospriteDeform

Assuming all the triangles for this shader are independant
quads, rebuild them as forward facing sprites
=====================
*/
void idRenderSystemShadeLocal::AutospriteDeform( void )
{
    S32	i, oldVerts;
    F32* xyz, radius;
    vec3_t mid, delta, left, up, leftDir, upDir;
    
    if ( tess.numVertexes & 3 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Autosprite shader %s had odd vertex count\n", tess.shader->name );
    }
    
    if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Autosprite shader %s had odd index count\n", tess.shader->name );
    }
    
    oldVerts = tess.numVertexes;
    tess.numVertexes = 0;
    tess.numIndexes = 0;
    tess.firstIndex = 0;
    
    if ( backEnd.currentEntity != &tr.worldEntity )
    {
        GlobalVectorToLocal( backEnd.viewParms.orientation.axis[1], leftDir );
        GlobalVectorToLocal( backEnd.viewParms.orientation.axis[2], upDir );
    }
    else
    {
        VectorCopy( backEnd.viewParms.orientation.axis[1], leftDir );
        VectorCopy( backEnd.viewParms.orientation.axis[2], upDir );
    }
    
    for ( i = 0; i < oldVerts; i += 4 )
    {
        vec4_t color;
        // find the midpoint
        xyz = tess.xyz[i];
        
        mid[0] = 0.25f * ( xyz[0] + xyz[4] + xyz[8] + xyz[12] );
        mid[1] = 0.25f * ( xyz[1] + xyz[5] + xyz[9] + xyz[13] );
        mid[2] = 0.25f * ( xyz[2] + xyz[6] + xyz[10] + xyz[14] );
        
        VectorSubtract( xyz, mid, delta );
        radius = VectorLength( delta ) * 0.707f;		// / sqrt(2)
        
        VectorScale( leftDir, radius, left );
        VectorScale( upDir, radius, up );
        
        if ( backEnd.viewParms.isMirror )
        {
            VectorSubtract( vec3_origin, left, left );
        }
        
        // compensate for scale in the axes if necessary
        if ( backEnd.currentEntity->e.nonNormalizedAxes )
        {
            F32 axisLength;
            
            axisLength = VectorLength( backEnd.currentEntity->e.axis[0] );
            
            if ( !axisLength )
            {
                axisLength = 0;
            }
            else
            {
                axisLength = 1.0f / axisLength;
            }
            
            VectorScale( left, axisLength, left );
            VectorScale( up, axisLength, up );
        }
        
        VectorScale4( tess.color[i], 1.0f / 65535.0f, color );
        idRenderSystemSurfaceLocal::AddQuadStamp( mid, left, up, color );
    }
}

/*
==================
idRenderSystemShadeLocal::Autosprite2Deform
==================
*/
void idRenderSystemShadeLocal::Autosprite2Deform( void )
{
    S32 i, j, k, indexes;
    F32* xyz;
    vec3_t	forward;
    
    if ( tess.numVertexes & 3 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count\n", tess.shader->name );
    }
    
    if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Autosprite2 shader %s had odd index count\n", tess.shader->name );
    }
    
    if ( backEnd.currentEntity != &tr.worldEntity )
    {
        GlobalVectorToLocal( backEnd.viewParms.orientation.axis[0], forward );
    }
    else
    {
        VectorCopy( backEnd.viewParms.orientation.axis[0], forward );
    }
    
    // this is a lot of work for two triangles...
    // we could precalculate a lot of it is an issue, but it would mess up
    // the shader abstraction
    for ( i = 0, indexes = 0; i < tess.numVertexes; i += 4, indexes += 6 )
    {
        S32	nums[2];
        F32	lengths[2], * v1, * v2;
        vec3_t mid[2], major, minor;
        
        // find the midpoint
        xyz = tess.xyz[i];
        
        // identify the two shortest edges
        nums[0] = nums[1] = 0;
        lengths[0] = lengths[1] = 999999;
        
        for ( j = 0; j < 6; j++ )
        {
            F32	l;
            vec3_t	temp;
            
            v1 = xyz + 4 * edgeVerts[j][0];
            v2 = xyz + 4 * edgeVerts[j][1];
            
            VectorSubtract( v1, v2, temp );
            
            l = DotProduct( temp, temp );
            if ( l < lengths[0] )
            {
                nums[1] = nums[0];
                lengths[1] = lengths[0];
                nums[0] = j;
                lengths[0] = l;
            }
            else if ( l < lengths[1] )
            {
                nums[1] = j;
                lengths[1] = l;
            }
        }
        
        for ( j = 0; j < 2; j++ )
        {
            v1 = xyz + 4 * edgeVerts[nums[j]][0];
            v2 = xyz + 4 * edgeVerts[nums[j]][1];
            
            mid[j][0] = 0.5f * ( v1[0] + v2[0] );
            mid[j][1] = 0.5f * ( v1[1] + v2[1] );
            mid[j][2] = 0.5f * ( v1[2] + v2[2] );
        }
        
        // find the vector of the major axis
        VectorSubtract( mid[1], mid[0], major );
        
        // cross this with the view direction to get minor axis
        CrossProduct( major, forward, minor );
        VectorNormalize( minor );
        
        // re-project the points
        for ( j = 0; j < 2; j++ )
        {
            F32	l;
            
            v1 = xyz + 4 * edgeVerts[nums[j]][0];
            v2 = xyz + 4 * edgeVerts[nums[j]][1];
            
            l = 0.5f * sqrtf( lengths[j] );
            
            // we need to see which direction this edge
            // is used to determine direction of projection
            for ( k = 0; k < 5; k++ )
            {
                if ( tess.indexes[indexes + k] == i + edgeVerts[nums[j]][0]
                        && tess.indexes[indexes + k + 1] == i + edgeVerts[nums[j]][1] )
                {
                    break;
                }
            }
            
            if ( k == 5 )
            {
                VectorMA( mid[j], l, minor, v1 );
                VectorMA( mid[j], -l, minor, v2 );
            }
            else
            {
                VectorMA( mid[j], -l, minor, v1 );
                VectorMA( mid[j], l, minor, v2 );
            }
        }
    }
}

/*
=====================
idRenderSystemShadeLocal::DeformTessGeometry
=====================
*/
void idRenderSystemShadeLocal::DeformTessGeometry( void )
{
    S32	i;
    deformStage_t* ds;
    
    if ( !ShaderRequiresCPUDeforms( tess.shader ) )
    {
        // we don't need the following CPU deforms
        return;
    }
    
    for ( i = 0; i < tess.shader->numDeforms; i++ )
    {
        ds = &tess.shader->deforms[i];
        
        switch ( ds->deformation )
        {
            case DEFORM_NONE:
                break;
            case DEFORM_NORMALS:
                CalcDeformNormals( ds );
                break;
            case DEFORM_WAVE:
                CalcDeformVertexes( ds );
                break;
            case DEFORM_BULGE:
                CalcBulgeVertexes( ds );
                break;
            case DEFORM_MOVE:
                CalcMoveVertexes( ds );
                break;
            case DEFORM_PROJECTION_SHADOW:
                idRenderSystemShadowsLocal::ProjectionShadowDeform();
                break;
            case DEFORM_AUTOSPRITE:
                AutospriteDeform();
                break;
            case DEFORM_AUTOSPRITE2:
                Autosprite2Deform();
                break;
            case DEFORM_TEXT0:
            case DEFORM_TEXT1:
            case DEFORM_TEXT2:
            case DEFORM_TEXT3:
            case DEFORM_TEXT4:
            case DEFORM_TEXT5:
            case DEFORM_TEXT6:
            case DEFORM_TEXT7:
                DeformText( backEnd.refdef.text[ds->deformation - DEFORM_TEXT0] );
                break;
        }
    }
}

/*
==================
idRenderSystemShadeLocal::CalcWaveColorSingle
==================
*/
F32 idRenderSystemShadeLocal::CalcWaveColorSingle( const waveForm_t* wf )
{
    F32 glow;
    
    if ( wf->func == GF_NOISE )
    {
        glow = wf->base + idRenderSystemNoiseLocal::NoiseGet4f( 0, 0, 0, ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
    }
    else if ( wf->func == GF_RANDOM )
    {
        glow = wf->base + idRenderSystemNoiseLocal::RandomOn( ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
    }
    else
    {
        glow = EvalWaveForm( wf ) * tr.identityLight;
    }
    
    if ( glow < 0 )
    {
        glow = 0;
    }
    else if ( glow > 1 )
    {
        glow = 1;
    }
    
    return glow;
}

/*
==================
idRenderSystemShadeLocal::CalcWaveAlphaSingle
==================
*/
F32 idRenderSystemShadeLocal::CalcWaveAlphaSingle( const waveForm_t* wf )
{
    return EvalWaveFormClamped( wf );
}

/*
==================
idRenderSystemShadeLocal::CalcModulateColorsByFog
==================
*/
void idRenderSystemShadeLocal::CalcModulateColorsByFog( U8* colors )
{
    S32	i;
    F32	texCoords[SHADER_MAX_VERTEXES][2] = { {0.0f} };
    
    // calculate texcoords so we can derive density
    // this is not wasted, because it would only have
    // been previously called if the surface was opaque
    CalcFogTexCoords( texCoords[0] );
    
    for ( i = 0; i < tess.numVertexes; i++, colors += 4 )
    {
        F32 f = 1.0f - idRenderSystemImageLocal::FogFactor( texCoords[i][0], texCoords[i][1] );
        colors[0] *= ( U8 )f;
        colors[1] *= ( U8 )f;
        colors[2] *= ( U8 )f;
    }
}

/*
========================
idRenderSystemShadeLocal::CalcFogTexCoords

To do the clipped fog plane really correctly, we should use
projected textures, but I don't trust the drivers and it
doesn't fit our shader data.
========================
*/
void idRenderSystemShadeLocal::CalcFogTexCoords( F32* st )
{
    S32	i;
    F32* v, s, t, eyeT;
    fog_t* fog;
    vec3_t local;
    vec4_t fogDistanceVector, fogDepthVector = { 0, 0, 0, 0 };
    bool eyeOutside;
    
    fog = tr.world->fogs + tess.fogNum;
    
    // all fogging distance is based on world Z units
    VectorSubtract( backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local );
    fogDistanceVector[0] = -backEnd.orientation.modelMatrix[2];
    fogDistanceVector[1] = -backEnd.orientation.modelMatrix[6];
    fogDistanceVector[2] = -backEnd.orientation.modelMatrix[10];
    fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.orientation.axis[0] );
    
    // scale the fog vectors based on the fog's thickness
    fogDistanceVector[0] *= fog->tcScale;
    fogDistanceVector[1] *= fog->tcScale;
    fogDistanceVector[2] *= fog->tcScale;
    fogDistanceVector[3] *= fog->tcScale;
    
    // rotate the gradient vector for this orientation
    if ( fog->hasSurface )
    {
        fogDepthVector[0] = fog->surface[0] * backEnd.orientation.axis[0][0] +
                            fog->surface[1] * backEnd.orientation.axis[0][1] + fog->surface[2] * backEnd.orientation.axis[0][2];
        fogDepthVector[1] = fog->surface[0] * backEnd.orientation.axis[1][0] +
                            fog->surface[1] * backEnd.orientation.axis[1][1] + fog->surface[2] * backEnd.orientation.axis[1][2];
        fogDepthVector[2] = fog->surface[0] * backEnd.orientation.axis[2][0] +
                            fog->surface[1] * backEnd.orientation.axis[2][1] + fog->surface[2] * backEnd.orientation.axis[2][2];
        fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.orientation.origin, fog->surface );
        
        eyeT = DotProduct( backEnd.orientation.viewOrigin, fogDepthVector ) + fogDepthVector[3];
    }
    else
    {
        // non-surface fog always has eye inside
        eyeT = 1;
    }
    
    // see if the viewpoint is outside
    // this is needed for clipping distance even for constant fog
    
    if ( eyeT < 0 )
    {
        eyeOutside = true;
    }
    else
    {
        eyeOutside = false;
    }
    
    fogDistanceVector[3] += 1.0f / 512;
    
    // calculate density for each point
    for ( i = 0, v = tess.xyz[0]; i < tess.numVertexes; i++, v += 4 )
    {
        // calculate the length in fog
        s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[3];
        t = DotProduct( v, fogDepthVector ) + fogDepthVector[3];
        
        // partially clipped fogs use the T axis
        if ( eyeOutside )
        {
            if ( t < 1.0 )
            {
                // point is outside, so no fogging
                t = 1.0f / 32;
            }
            else
            {
                // cut the distance at the fog plane
                t = 1.0f / 32 + 30.0f / 32 * t / ( t - eyeT );
            }
        }
        else
        {
            if ( t < 0 )
            {
                // point is outside, so no fogging
                t = 1.0f / 32;
            }
            else
            {
                t = 31.0f / 32;
            }
        }
        
        st[0] = s;
        st[1] = t;
        st += 2;
    }
}

/*
==================
idRenderSystemShadeLocal::CalcTurbulentFactors
==================
*/
void idRenderSystemShadeLocal::CalcTurbulentFactors( const waveForm_t* wf, F32* amplitude, F32* now )
{
    *now = ( F32 )( wf->phase + tess.shaderTime * wf->frequency );
    *amplitude = wf->amplitude;
}

/*
==================
idRenderSystemShadeLocal::CalcScaleTexMatrix
==================
*/
void idRenderSystemShadeLocal::CalcScaleTexMatrix( const F32 scale[2], F32* matrix )
{
    matrix[0] = scale[0];
    matrix[2] = 0.0f;
    matrix[4] = 0.0f;
    matrix[1] = 0.0f;
    matrix[3] = scale[1];
    matrix[5] = 0.0f;
}

/*
==================
idRenderSystemShadeLocal::CalcScrollTexMatrix
==================
*/
void idRenderSystemShadeLocal::CalcScrollTexMatrix( const F32 scrollSpeed[2], F32* matrix )
{
    F64 timeScale = tess.shaderTime;
    F64 adjustedScrollS = scrollSpeed[0] * timeScale;
    F64 adjustedScrollT = scrollSpeed[1] * timeScale;
    
    // clamp so coordinates don't continuously get larger, causing problems
    // with hardware limits
    adjustedScrollS -= floor( adjustedScrollS );
    adjustedScrollT -= floor( adjustedScrollT );
    
    matrix[0] = 1.0f;
    matrix[2] = 0.0f;
    matrix[4] = ( F32 )adjustedScrollS;
    matrix[1] = 0.0f;
    matrix[3] = 1.0f;
    matrix[5] = ( F32 )adjustedScrollT;
}

/*
==================
idRenderSystemShadeLocal::CalcTransformTexMatrix
==================
*/
void idRenderSystemShadeLocal::CalcTransformTexMatrix( const texModInfo_t* tmi, F32* matrix )
{
    matrix[0] = tmi->matrix[0][0];
    matrix[2] = tmi->matrix[1][0];
    matrix[4] = tmi->translate[0];
    matrix[1] = tmi->matrix[0][1];
    matrix[3] = tmi->matrix[1][1];
    matrix[5] = tmi->translate[1];
}

/*
==================
idRenderSystemShadeLocal::CalcRotateTexMatrix
==================
*/
void idRenderSystemShadeLocal::CalcRotateTexMatrix( F32 degsPerSecond, F32* matrix )
{
    F32 sinValue, cosValue;
    F64 timeScale = tess.shaderTime, degs;
    
    degs = -degsPerSecond * timeScale;
    S32 i = ( S32 )( degs * ( FUNCTABLE_SIZE / 360.0f ) );
    
    sinValue = tr.sinTable[i & FUNCTABLE_MASK];
    cosValue = tr.sinTable[( i + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK];
    
    matrix[0] = cosValue;
    matrix[2] = -sinValue;
    matrix[4] = 0.5f - 0.5f * cosValue + 0.5f * sinValue;
    matrix[1] = sinValue;
    matrix[3] = cosValue;
    matrix[5] = 0.5f - 0.5f * sinValue - 0.5f * cosValue;
}

/*
==================
idRenderSystemShadeLocal::ShaderRequiresCPUDeforms
==================
*/
bool idRenderSystemShadeLocal::ShaderRequiresCPUDeforms( const shader_t* shader )
{
    if ( shader->numDeforms )
    {
        const deformStage_t* ds = &shader->deforms[0];
        
        if ( shader->numDeforms > 1 )
        {
            return true;
        }
        
        switch ( ds->deformation )
        {
            case DEFORM_WAVE:
            case DEFORM_BULGE:
                // need CPU deforms at high level-times to avoid floating point percision loss
                return ( backEnd.refdef.floatTime != ( F32 )backEnd.refdef.floatTime );
                
            default:
                return true;
        }
    }
    
    return false;
}

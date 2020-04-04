////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete
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
// File name:   r_postprocess.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemPostProcessLocal renderSystemPostProcessLocal;

/*
===============
idRenderSystemPostProcessLocal::idRenderSystemPostProcessLocal
===============
*/
idRenderSystemPostProcessLocal::idRenderSystemPostProcessLocal( void )
{
}

/*
===============
idRenderSystemPostProcessLocal::~idRenderSystemPostProcessLocal
===============
*/
idRenderSystemPostProcessLocal::~idRenderSystemPostProcessLocal( void )
{
}

/*
===============
idRenderSystemPostProcessLocal::ToneMap
===============
*/
void idRenderSystemPostProcessLocal::ToneMap( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox, S32 autoExposure )
{
    ivec4_t srcBox, dstBox;
    vec4_t color;
    static S32 lastFrameCount = 0;
    
    if ( autoExposure )
    {
        if ( lastFrameCount == 0 || tr.frameCount < lastFrameCount || tr.frameCount - lastFrameCount > 5 )
        {
            // determine average log luminance
            FBO_t* srcFbo, *dstFbo, *tmp;
            S32 size = 256;
            
            lastFrameCount = tr.frameCount;
            
            VectorSet4( dstBox, 0, 0, size, size );
            
            idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, tr.textureScratchFbo[0], dstBox, &tr.calclevels4xShader[0], NULL, 0 );
            
            srcFbo = tr.textureScratchFbo[0];
            dstFbo = tr.textureScratchFbo[1];
            
            // downscale to 1x1 texture
            while ( size > 1 )
            {
                VectorSet4( srcBox, 0, 0, size, size );
                //size >>= 2;
                size >>= 1;
                VectorSet4( dstBox, 0, 0, size, size );
                
                if ( size == 1 )
                {
                    dstFbo = tr.targetLevelsFbo;
                }
                
                idRenderSystemFBOLocal::FastBlit( srcFbo, srcBox, dstFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
                
                tmp = srcFbo;
                srcFbo = dstFbo;
                dstFbo = tmp;
            }
        }
        
        // blend with old log luminance for gradual change
        VectorSet4( srcBox, 0, 0, 0, 0 );
        
        color[0] =
            color[1] =
                color[2] = 1.0f;
                
        if ( glRefConfig.textureFloat )
        {
            color[3] = 0.03f;
        }
        else
        {
            color[3] = 0.1f;
        }
        
        idRenderSystemFBOLocal::Blit( tr.targetLevelsFbo, srcBox, NULL, tr.calcLevelsFbo, NULL,  NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    }
    
    // tonemap
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    if ( autoExposure )
    {
        idRenderSystemBackendLocal::BindToTMU( tr.calcLevelsImage, TB_LEVELSMAP );
    }
    else
    {
        idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.tonemapShader, color, 0 );
}

/*
=============
idRenderSystemPostProcessLocal::BokehBlur

Blurs a part of one framebuffer to another.

Framebuffers can be identical.
=============
*/
void idRenderSystemPostProcessLocal::BokehBlur( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox, F32 blur )
{
    vec4_t color;
    
    blur *= 10.0f;
    
    if ( blur < 0.004f )
    {
        return;
    }
    
    if ( glRefConfig.framebufferObject )
    {
        // bokeh blur
        if ( blur > 0.0f )
        {
            ivec4_t quarterBox;
            
            quarterBox[0] = 0;
            quarterBox[1] = tr.quarterFbo[0]->height;
            quarterBox[2] = tr.quarterFbo[0]->width;
            quarterBox[3] = -tr.quarterFbo[0]->height;
            
            // create a quarter texture
            //idRenderSystemFBOLocal::Blit(NULL, NULL, NULL, tr.quarterFbo[0], NULL, NULL, NULL, 0);
            idRenderSystemFBOLocal::FastBlit( src, srcBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
        
#ifndef HQ_BLUR
        if ( blur > 1.0f )
        {
            // create a 1/16th texture
            //idRenderSystemFBOLocal::Blit(tr.quarterFbo[0], NULL, NULL, tr.textureScratchFbo[0], NULL, NULL, NULL, 0);
            idRenderSystemFBOLocal::FastBlit( tr.quarterFbo[0], NULL, tr.textureScratchFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
#endif
        
        if ( blur > 0.0f && blur <= 1.0f )
        {
            // Crossfade original with quarter texture
            VectorSet4( color, 1, 1, 1, blur );
            
            idRenderSystemFBOLocal::Blit( tr.quarterFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
        }
#ifndef HQ_BLUR
        // ok blur, but can see some pixelization
        else if ( blur > 1.0f && blur <= 2.0f )
        {
            // crossfade quarter texture with 1/16th texture
            idRenderSystemFBOLocal::Blit( tr.quarterFbo[0], NULL, NULL, dst, dstBox, NULL, NULL, 0 );
            
            VectorSet4( color, 1, 1, 1, blur - 1.0f );
            
            idRenderSystemFBOLocal::Blit( tr.textureScratchFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
        }
        else if ( blur > 2.0f )
        {
            // blur 1/16th texture then replace
            S32 i;
            
            for ( i = 0; i < 2; i++ )
            {
                vec2_t blurTexScale;
                F32 subblur;
                
                subblur = ( ( blur - 2.0f ) / 2.0f ) / 3.0f * ( F32 )( i + 1 );
                
                blurTexScale[0] =
                    blurTexScale[1] = subblur;
                    
                color[0] =
                    color[1] =
                        color[2] = 0.5f;
                color[3] = 1.0f;
                
                if ( i != 0 )
                {
                    idRenderSystemFBOLocal::Blit( tr.textureScratchFbo[0], NULL, blurTexScale, tr.textureScratchFbo[1], NULL, &tr.bokehShader, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
                }
                else
                {
                    idRenderSystemFBOLocal::Blit( tr.textureScratchFbo[0], NULL, blurTexScale, tr.textureScratchFbo[1], NULL, &tr.bokehShader, color, 0 );
                }
            }
            
            idRenderSystemFBOLocal::Blit( tr.textureScratchFbo[1], NULL, NULL, dst, dstBox, NULL, NULL, 0 );
        }
#else // higher quality blur, but slower
        else if ( blur > 1.0f )
        {
            // blur quarter texture then replace
            S32 i;
        
            src = tr.quarterFbo[0];
            dst = tr.quarterFbo[1];
        
            VectorSet4( color, 0.5f, 0.5f, 0.5f, 1 );
        
            for ( i = 0; i < 2; i++ )
            {
                vec2_t blurTexScale;
                F32 subblur;
        
                subblur = ( blur - 1.0f ) / 2.0f * ( F32 )( i + 1 );
        
                blurTexScale[0] =
                    blurTexScale[1] = subblur;
        
                color[0] =
                    color[1] =
                        color[2] = 1.0f;
                if ( i != 0 )
                    color[3] = 1.0f;
                else
                    color[3] = 0.5f;
        
                idRenderSystemFBOLocal::Blit( tr.quarterFbo[0], NULL, blurTexScale, tr.quarterFbo[1], NULL, &tr.bokehShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
            }
        
            idRenderSystemFBOLocal::Blit( tr.quarterFbo[1], NULL, NULL, dst, dstBox, NULL, NULL, 0 );
        }
#endif
    }
}

/*
===============
idRenderSystemPostProcessLocal::RadialBlur
===============
*/
void idRenderSystemPostProcessLocal::RadialBlur( FBO_t* srcFbo, FBO_t* dstFbo, S32 passes, F32 stretch, F32 x, F32 y, F32 w, F32 h, F32 xcenter, F32 ycenter, F32 alpha )
{
    ivec4_t srcBox, dstBox;
    S32 srcWidth, srcHeight;
    vec4_t color;
    const F32 inc = 1.f / passes;
    const F32 mul = powf( stretch, inc );
    F32 scale;
    
    alpha *= inc;
    VectorSet4( color, alpha, alpha, alpha, 1.0f );
    
    srcWidth  = srcFbo ? srcFbo->width  : glConfig.vidWidth;
    srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;
    
    VectorSet4( srcBox, 0, 0, srcWidth, srcHeight );
    
    VectorSet4( dstBox, ( S32 )x, ( S32 )y, ( S32 )w, ( S32 )h );
    idRenderSystemFBOLocal::Blit( srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, 0 );
    
    --passes;
    scale = mul;
    while ( passes > 0 )
    {
        F32 iscale = 1.f / scale;
        F32 s0 = xcenter * ( 1.f - iscale );
        F32 t0 = ( 1.0f - ycenter ) * ( 1.f - iscale );
        
        srcBox[0] = ( S32 )( s0 * srcWidth );
        srcBox[1] = ( S32 )( t0 * srcHeight );
        srcBox[2] = ( S32 )( iscale * srcWidth );
        srcBox[3] = ( S32 )( iscale * srcHeight );
        
        idRenderSystemFBOLocal::Blit( srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
        
        scale *= mul;
        --passes;
    }
}

/*
===============
idRenderSystemPostProcessLocal::UpdateSunFlareVis
===============
*/
bool idRenderSystemPostProcessLocal::UpdateSunFlareVis( void )
{
    U32 sampleCount = 0;
    if ( !glRefConfig.occlusionQuery )
    {
        return true;
    }
    
    tr.sunFlareQueryIndex ^= 1;
    if ( !tr.sunFlareQueryActive[tr.sunFlareQueryIndex] )
    {
        return true;
    }
    
    /* debug code */
    if ( 0 )
    {
        S32 iter;
        for ( iter = 0 ; ; ++iter )
        {
            S32 available = 0;
            qglGetQueryObjectiv( tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT_AVAILABLE, &available );
            if ( available )
                break;
        }
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Waited %d iterations\n", iter );
    }
    
    qglGetQueryObjectuiv( tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT, &sampleCount );
    return sampleCount > 0;
}

/*
===============
idRenderSystemPostProcessLocal::SunRays
===============
*/
void idRenderSystemPostProcessLocal::SunRays( FBO_t* srcFbo, ivec4_t srcBox, FBO_t* dstFbo, ivec4_t dstBox )
{
    vec4_t color;
    F32 dot;
    const F32 cutoff = 0.25f;
    bool colorize = true;
    
    //	F32 w, h, w2, h2;
    mat4_t mvp;
    vec4_t pos, hpos;
    
    dot = DotProduct( tr.sunDirection, backEnd.viewParms.orientation.axis[0] );
    if ( dot < cutoff )
    {
        return;
    }
    
    if ( !UpdateSunFlareVis() )
    {
        return;
    }
    
    // From idRenderSystemSkyLocal::DrawSun()
    {
        F32 dist;
        mat4_t trans, model;
        
        idRenderSystemMathsLocal::Mat4Translation( backEnd.viewParms.orientation.origin, trans );
        idRenderSystemMathsLocal::Mat4Multiply( backEnd.viewParms.world.modelMatrix, trans, model );
        idRenderSystemMathsLocal::Mat4Multiply( backEnd.viewParms.projectionMatrix, model, mvp );
        
        dist = backEnd.viewParms.zFar / 1.75f;
        
        VectorScale( tr.sunDirection, dist, pos );
    }
    
    // project sun point
    //idRenderSystemMathsLocal::Mat4Multiply(backEnd.viewParms.projectionMatrix, backEnd.viewParms.world.modelMatrix, mvp);
    idRenderSystemMathsLocal::Mat4Transform( mvp, pos, hpos );
    
    // transform to UV coords
    hpos[3] = 0.5f / hpos[3];
    
    pos[0] = 0.5f + hpos[0] * hpos[3];
    pos[1] = 0.5f + hpos[1] * hpos[3];
    
    // initialize quarter buffers
    {
        F32 mul = 1.f;
        ivec4_t rayBox, quarterBox;
        S32 srcWidth  = srcFbo ? srcFbo->width  : glConfig.vidWidth;
        S32 srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;
        
        VectorSet4( color, mul, mul, mul, 1 );
        
        rayBox[0] = srcBox[0] * tr.sunRaysFbo->width  / srcWidth;
        rayBox[1] = srcBox[1] * tr.sunRaysFbo->height / srcHeight;
        rayBox[2] = srcBox[2] * tr.sunRaysFbo->width  / srcWidth;
        rayBox[3] = srcBox[3] * tr.sunRaysFbo->height / srcHeight;
        
        quarterBox[0] = 0;
        quarterBox[1] = tr.quarterFbo[0]->height;
        quarterBox[2] = tr.quarterFbo[0]->width;
        quarterBox[3] = -tr.quarterFbo[0]->height;
        
        // first, downsample the framebuffer
        if ( colorize )
        {
            idRenderSystemFBOLocal::FastBlit( srcFbo, srcBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            idRenderSystemFBOLocal::Blit( tr.sunRaysFbo, rayBox, NULL, tr.quarterFbo[0], quarterBox, NULL, color, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );
        }
        else
        {
            idRenderSystemFBOLocal::FastBlit( tr.sunRaysFbo, rayBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
    }
    
    // radial blur passes, ping-ponging between the two quarter-size buffers
    {
        const F32 stretch_add = 2.f / 3.f;
        F32 stretch = 1.f + stretch_add;
        S32 i;
        for ( i = 0; i < 2; ++i )
        {
            RadialBlur( tr.quarterFbo[i & 1], tr.quarterFbo[( ~i ) & 1], 5, stretch, 0.f, 0.f, ( F32 )tr.quarterFbo[0]->width, ( F32 )tr.quarterFbo[0]->height,
                        pos[0], pos[1], 1.125f );
            stretch += stretch_add;
        }
    }
    
    // add result back on top of the main buffer
    {
        F32 mul = 1.f;
        
        VectorSet4( color, mul, mul, mul, 1 );
        
        idRenderSystemFBOLocal::Blit( tr.quarterFbo[0], NULL, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
    }
}

/*
===============
idRenderSystemPostProcessLocal::DarkExpand
===============
*/
void idRenderSystemPostProcessLocal::DarkExpand( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.darkexpandShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.darkexpandShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.darkexpandShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

/*
===============
idRenderSystemPostProcessLocal::Anamorphic
===============
*/
void idRenderSystemPostProcessLocal::Anamorphic( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t	color;
    ivec4_t halfBox;
    vec2_t	texScale, texHalfScale, texDoubleScale;
    
    texScale[0] = texScale[1] = 1.0f;
    texHalfScale[0] = texHalfScale[1] = texScale[0] / 8.0f;
    texDoubleScale[0] = texDoubleScale[1] = texScale[0] * 8.0f;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    halfBox[0] = backEnd.viewParms.viewportX      * tr.anamorphicRenderFBOImage[0]->width / glConfig.vidWidth;
    halfBox[1] = backEnd.viewParms.viewportY      * tr.anamorphicRenderFBOImage[0]->height / glConfig.vidHeight;
    halfBox[2] = backEnd.viewParms.viewportWidth  * tr.anamorphicRenderFBOImage[0]->width / glConfig.vidWidth;
    halfBox[3] = backEnd.viewParms.viewportHeight * tr.anamorphicRenderFBOImage[0]->height / glConfig.vidHeight;
    
    //
    // Darken to VBO...
    //
    
    idRenderSystemGLSLLocal::BindProgram( &tr.anamorphicDarkenShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.anamorphicDarkenShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_anamorphicDarkenPower->value, 0.0, 0.0, 0.0 );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.anamorphicDarkenShader, UNIFORM_LOCAL0, local0 );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, NULL, texHalfScale, tr.anamorphicRenderFBO[1], NULL, &tr.anamorphicDarkenShader, color, 0 );
    idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
    
    //
    // Blur the new darken'ed VBO...
    //
    
    for ( S32 i = 0; i < r_bloomPasses->integer; i++ )
    {
        //
        // Bloom X axis... (to VBO 1)
        //
        
        //for (S32 width = 1; width < 12 ; width++)
        {
            idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
            
            idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
            
            {
                vec2_t screensize;
                screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                
                idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
            }
            
            {
                vec4_t local0;
                VectorSet4( local0, 1.0f, 0.0f, 16.0f, 0.0f );
                idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
            }
            
            idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
            idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
    }
    
    //
    // Copy (and upscale) the bloom image to our full screen image...
    //
    
    idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, texDoubleScale, tr.anamorphicRenderFBO[2], NULL, &tr.ssgiBlurShader, color, 0 );
    
    //
    // Combine the screen with the bloom'ed VBO...
    //
    
    
    idRenderSystemGLSLLocal::BindProgram( &tr.anamorphicCombineShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.anamorphicCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.anamorphicCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    
    idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[2], TB_NORMALMAP );
    
    {
        vec4_t local0;
        VectorSet4( local0, 0.6f, 0.0f, 0.0f, 0.0f );
        
        VectorSet4( local0, 1.0f, 0.0f, 0.0f, 0.0f ); // Account for already added glow...
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.anamorphicCombineShader, UNIFORM_LOCAL0, local0 );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.anamorphicCombineShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    
    //
    // Render the results now...
    //
    
    idRenderSystemFBOLocal::FastBlit( ldrFbo, NULL, hdrFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
}

/*
===============
idRenderSystemPostProcessLocal::LensFlare
===============
*/
void idRenderSystemPostProcessLocal::LensFlare( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.lensflareShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.lensflareShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.lensflareShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::MultiPost
===============
*/
void idRenderSystemPostProcessLocal::MultiPost( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.multipostShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.multipostShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.multipostShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::HDR
===============
*/
void idRenderSystemPostProcessLocal::HDR( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.hdrShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0f, 0.0f );
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.hdrShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.hdrShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.hdrShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

/*
===============
idRenderSystemPostProcessLocal::Anaglyph
===============
*/
void idRenderSystemPostProcessLocal::Anaglyph( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.anaglyphShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    //qglUseProgramObjectARB(tr.fakedepthShader.program);
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0f, 0.0f );
        //VectorSet4(viewInfo, zmin, zmax, 0.0f, 0.0f);
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.anaglyphShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.anaglyphShader, UNIFORM_DIMENSIONS, screensize );
        
        //clientMainSystem->RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_trueAnaglyphSeparation->value, r_trueAnaglyphRed->value, r_trueAnaglyphGreen->value, r_trueAnaglyphBlue->value );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.anaglyphShader, UNIFORM_LOCAL0, local0 );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.anaglyphShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

/*
===============
idRenderSystemPostProcessLocal::TextureClean
===============
*/
void idRenderSystemPostProcessLocal::TextureClean( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.texturecleanShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        //VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.texturecleanShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.texturecleanShader, UNIFORM_DIMENSIONS, screensize );
        
        //clientMainSystem->RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_textureCleanSigma->value, r_textureCleanBSigma->value, r_textureCleanMSize->value, 0 );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.texturecleanShader, UNIFORM_LOCAL0, local0 );
    }
    
    //idRenderSystemFBOLocal::Blit(hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.texturecleanShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.texturecleanShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::ESharpening
===============
*/
void idRenderSystemPostProcessLocal::ESharpening( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.esharpeningShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    //qglUseProgramObjectARB(tr.esharpeningShader.program);
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0f, 0.0f );
        //VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.esharpeningShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.esharpeningShader, UNIFORM_DIMENSIONS, screensize );
        
        //clientMainSystem->RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_textureCleanSigma->value, r_textureCleanBSigma->value, 0, 0 );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.texturecleanShader, UNIFORM_LOCAL0, local0 );
    }
    
    //idRenderSystemFBOLocal::Blit(hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.esharpeningShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.esharpeningShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::ESharpening2
===============
*/
void idRenderSystemPostProcessLocal::ESharpening2( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.esharpening2Shader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.esharpening2Shader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.esharpening2Shader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.esharpeningShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::DOF
===============
*/
void idRenderSystemPostProcessLocal::DOF( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.dofShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.dofShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.dofShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.dofShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, 0.0f );
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.dofShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.dofShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

/*
===============
idRenderSystemPostProcessLocal::Vibrancy
===============
*/
void idRenderSystemPostProcessLocal::Vibrancy( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.vibrancyShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.vibrancyShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t info;
        
        info[0] = r_vibrancy->value;
        info[1] = 0.0f;
        info[2] = 0.0f;
        info[3] = 0.0f;
        
        VectorSet4( info, info[0], info[1], info[2], info[3] );
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.vibrancyShader, UNIFORM_LOCAL0, info );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.vibrancyShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

/*
===============
idRenderSystemPostProcessLocal::TextureDetail
===============
*/
void idRenderSystemPostProcessLocal::TextureDetail( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.texturedetailShader );
    
    idRenderSystemBackendLocal::BindToTMU( hdrFbo->colorImage[0], TB_LEVELSMAP );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.texturedetailShader, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.texturedetailShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.texturedetailShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, zmin, zmax );
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.texturedetailShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_textureDetailStrength->value, 0.0f, 0.0f, 0.0f );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.texturedetailShader, UNIFORM_LOCAL0, local0 );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.texturedetailShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::RBM
===============
*/
void idRenderSystemPostProcessLocal::RBM( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t		color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.rbmShader );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.rbmShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.rbmShader, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
    
    idRenderSystemGLSLLocal::SetUniformVec3( &tr.rbmShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.rbmShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    idRenderSystemBackendLocal::BindToTMU( hdrFbo->colorImage[0], TB_DIFFUSEMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.rbmShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.normalDetailedImage, TB_NORMALMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.rbmShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    
    {
        vec4_t local0;
        VectorSet4( local0, r_rbmStrength->value, 0.0f, 0.0f, 0.0f );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.rbmShader, UNIFORM_LOCAL0, local0 );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.rbmShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        F32 zmax = backEnd.viewParms.zFar;
        F32 ymax = zmax * tanf( backEnd.viewParms.fovY * M_PI / 360.0f );
        F32 xmax = zmax * tanf( backEnd.viewParms.fovX * M_PI / 360.0f );
        F32 zmin = r_znear->value;
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, 0.0f );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.rbmShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.rbmShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::Contrast
===============
*/
void idRenderSystemPostProcessLocal::Contrast( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox )
{
    F32 brightness = 2;
    
    if ( !glRefConfig.framebufferObject )
    {
        return;
    }
    
    idRenderSystemGLSLLocal::SetUniformFloat( &tr.contrastShader, UNIFORM_BRIGHTNESS, r_brightness->value );
    idRenderSystemGLSLLocal::SetUniformFloat( &tr.contrastShader, UNIFORM_CONTRAST, r_contrast->value );
    idRenderSystemGLSLLocal::SetUniformFloat( &tr.contrastShader, UNIFORM_GAMMA, r_gamma->value );
    
    idRenderSystemFBOLocal::FastBlit( src, srcBox, tr.screenScratchFbo, srcBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
    idRenderSystemFBOLocal::Blit( tr.screenScratchFbo, srcBox, NULL, dst, dstBox, &tr.contrastShader, NULL, 0 );
}


/*
===============
idRenderSystemPostProcessLocal::FXAA
===============
*/
void idRenderSystemPostProcessLocal::FXAA( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.fxaaShader );
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.fxaaShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.fxaaShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.fxaaShader, color, 0 );
}

/*
===============
idRenderSystemPostProcessLocal::Bloom
===============
*/
void idRenderSystemPostProcessLocal::Bloom( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t	color;
    ivec4_t halfBox;
    vec2_t	texScale, texHalfScale, texDoubleScale;
    
    texScale[0] = texScale[1] = 1.0f;
    texHalfScale[0] = texHalfScale[1] = texScale[0] / 2.0f;
    texDoubleScale[0] = texDoubleScale[1] = texScale[0] * 2.0f;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    halfBox[0] = ( S32 )( backEnd.viewParms.viewportX * tr.bloomRenderFBOImage[0]->width / glConfig.vidWidth );
    halfBox[1] = ( S32 )( backEnd.viewParms.viewportY * tr.bloomRenderFBOImage[0]->height / glConfig.vidHeight );
    halfBox[2] = ( S32 )( backEnd.viewParms.viewportWidth * tr.bloomRenderFBOImage[0]->width / glConfig.vidWidth );
    halfBox[3] = ( S32 )( backEnd.viewParms.viewportHeight * tr.bloomRenderFBOImage[0]->height / glConfig.vidHeight );
    
    //
    // Darken to VBO...
    //
    
    idRenderSystemGLSLLocal::BindProgram( &tr.bloomDarkenShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.bloomDarkenShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_bloomDarkenPower->value, 0.0f, 0.0f, 0.0f );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.bloomDarkenShader, UNIFORM_LOCAL0, local0 );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, NULL, texHalfScale, tr.bloomRenderFBO[1], NULL, &tr.bloomDarkenShader, color, 0 );
    idRenderSystemFBOLocal::FastBlit( tr.bloomRenderFBO[1], NULL, tr.bloomRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
    
    //
    // Blur the new darken'ed VBO...
    //
    
    for ( S32 i = 0; i < r_bloomPasses->integer; i++ )
    {
#ifdef ___BLOOM_AXIS_UNCOMBINED_SHADER___
        //
        // Bloom X axis... (to VBO 1)
        //
        
        idRenderSystemGLSLLocal::BindProgram( &tr.bloomBlurShader );
        
        idRenderSystemBackendLocal::BindToTMU( tr.bloomRenderFBOImage[0], TB_DIFFUSEMAP );
        
        {
            vec2_t screensize;
            screensize[0] = tr.bloomRenderFBOImage[0]->width;
            screensize[1] = tr.bloomRenderFBOImage[0]->height;
            
            idRenderSystemGLSLLocal::SetUniformVec2( &tr.bloomBlurShader, UNIFORM_DIMENSIONS, screensize );
        }
        
        {
            vec4_t local0;
            VectorSet4( local0, 1.0f, 0.0f, 0.0f, 0.0f );
            idRenderSystemGLSLLocal::SetUniformVec4( &tr.bloomBlurShader, UNIFORM_LOCAL0, local0 );
        }
        
        idRenderSystemFBOLocal::Blit( tr.bloomRenderFBO[0], NULL, NULL, tr.bloomRenderFBO[1], NULL, &tr.bloomBlurShader, color, 0 );
        idRenderSystemFBOLocal::FastBlit( tr.bloomRenderFBO[1], NULL, tr.bloomRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        
        //
        // Bloom Y axis... (back to VBO 0)
        //
        
        idRenderSystemGLSLLocal::BindProgram( &tr.bloomBlurShader );
        
        idRenderSystemBackendLocal::BindToTMU( tr.bloomRenderFBOImage[1], TB_DIFFUSEMAP );
        
        {
            vec2_t screensize;
            screensize[0] = tr.bloomRenderFBOImage[1]->width;
            screensize[1] = tr.bloomRenderFBOImage[1]->height;
            
            idRenderSystemGLSLLocal::SetUniformVec2( &tr.bloomBlurShader, UNIFORM_DIMENSIONS, screensize );
        }
        
        {
            vec4_t local0;
            VectorSet4( local0, 0.0, 1.0, 0.0, 0.0 );
            idRenderSystemGLSLLocal::SetUniformVec4( &tr.bloomBlurShader, UNIFORM_LOCAL0, local0 );
        }
        
        idRenderSystemFBOLocal::Blit( tr.bloomRenderFBO[0], NULL, NULL, tr.bloomRenderFBO[1], NULL, &tr.bloomBlurShader, color, 0 );
        idRenderSystemFBOLocal::FastBlit( tr.bloomRenderFBO[1], NULL, tr.bloomRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
#else //___BLOOM_AXIS_UNCOMBINED_SHADER___
        
        //
        // Bloom X and Y axis... (to VBO 1)
        //
        
        idRenderSystemGLSLLocal::BindProgram( &tr.bloomBlurShader );
        
        idRenderSystemBackendLocal::BindToTMU( tr.bloomRenderFBOImage[0], TB_DIFFUSEMAP );
        
        {
            vec2_t screensize;
            screensize[0] = ( F32 )tr.bloomRenderFBOImage[0]->width;
            screensize[1] = ( F32 )tr.bloomRenderFBOImage[0]->height;
        
            idRenderSystemGLSLLocal::SetUniformVec2( &tr.bloomBlurShader, UNIFORM_DIMENSIONS, screensize );
        }
        
        {
            vec4_t local0;
            VectorSet4( local0, 0.0f, 0.0f, /* bloom width */ 3.0f, 0.0f );
            idRenderSystemGLSLLocal::SetUniformVec4( &tr.bloomBlurShader, UNIFORM_LOCAL0, local0 );
        }
        
        idRenderSystemFBOLocal::Blit( tr.bloomRenderFBO[0], NULL, NULL, tr.bloomRenderFBO[1], NULL, &tr.bloomBlurShader, color, 0 );
        idRenderSystemFBOLocal::FastBlit( tr.bloomRenderFBO[1], NULL, tr.bloomRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        
#endif //___BLOOM_AXIS_UNCOMBINED_SHADER___
    }
    
    //
    // Copy (and upscale) the bloom image to our full screen image...
    //
    
    idRenderSystemFBOLocal::Blit( tr.bloomRenderFBO[0], NULL, texDoubleScale, tr.bloomRenderFBO[2], NULL, &tr.bloomBlurShader, color, 0 );
    
    //
    // Combine the screen with the bloom'ed VBO...
    //
    idRenderSystemGLSLLocal::BindProgram( &tr.bloomCombineShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.bloomCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.bloomCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    
    idRenderSystemBackendLocal::BindToTMU( tr.bloomRenderFBOImage[2], TB_NORMALMAP );
    
    {
        vec4_t local0;
        VectorSet4( local0, r_bloomScale->value, 0.0f, 0.0f, 0.0f );
        VectorSet4( local0, 0.5f * r_bloomScale->value, 0.0f, 0.0f, 0.0f );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.bloomCombineShader, UNIFORM_LOCAL0, local0 );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.bloomCombineShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    
    //
    // Render the results now...
    //
    
    idRenderSystemFBOLocal::FastBlit( ldrFbo, NULL, hdrFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
}

/*
===============
idRenderSystemPostProcessLocal::SSGI
===============
*/
void idRenderSystemPostProcessLocal::SSGI( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t	color;
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    {
        ivec4_t halfBox;
        vec2_t	texScale, texHalfScale, texDoubleScale;
        
        texScale[0] = texScale[1] = 1.0f;
        texHalfScale[0] = texHalfScale[1] = texScale[0] / 8.0f;
        texDoubleScale[0] = texDoubleScale[1] = texScale[0] * 8.0f;
        
        halfBox[0] = ( S32 )( backEnd.viewParms.viewportX * tr.anamorphicRenderFBOImage[0]->width / glConfig.vidWidth );
        halfBox[1] = ( S32 )( backEnd.viewParms.viewportY * tr.anamorphicRenderFBOImage[0]->height / glConfig.vidHeight );
        halfBox[2] = ( S32 )( backEnd.viewParms.viewportWidth * tr.anamorphicRenderFBOImage[0]->width / glConfig.vidWidth );
        halfBox[3] = ( S32 )( backEnd.viewParms.viewportHeight * tr.anamorphicRenderFBOImage[0]->height / glConfig.vidHeight );
        
        //
        // Darken to VBO...
        //
        
        {
            idRenderSystemGLSLLocal::BindProgram( &tr.anamorphicDarkenShader );
            
            idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_DIFFUSEMAP );
            
            {
                vec2_t screensize;
                screensize[0] = ( F32 )glConfig.vidWidth;
                screensize[1] = ( F32 )glConfig.vidHeight;
                
                idRenderSystemGLSLLocal::SetUniformVec2( &tr.anamorphicDarkenShader, UNIFORM_DIMENSIONS, screensize );
            }
            
            {
                vec4_t local0;
                VectorSet4( local0, r_anamorphicDarkenPower->value, 0.0f, 0.0f, 0.0f );
                idRenderSystemGLSLLocal::SetUniformVec4( &tr.anamorphicDarkenShader, UNIFORM_LOCAL0, local0 );
            }
            
            idRenderSystemFBOLocal::Blit( hdrFbo, NULL, texHalfScale, tr.anamorphicRenderFBO[1], NULL, &tr.anamorphicDarkenShader, color, 0 );
            idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
        }
        
        //
        // Blur the new darken'ed VBO...
        //
        
        F32 SCAN_WIDTH = r_ssgiWidth->value;//8.0;
        
        {
            //
            // Bloom +-X axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0f, 0.0f, SCAN_WIDTH, 3.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
            
            //
            // Bloom +-Y axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 0.0, 1.0, SCAN_WIDTH, 3.0 );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
            
            //
            // Bloom XY & -X-Y axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0f, 1.0f, SCAN_WIDTH, 3.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
            
            //
            // Bloom -X+Y & +X-Y axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, -1.0f, 1.0f, SCAN_WIDTH, 3.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        }
        
        //
        // Do a final blur pass - but this time don't mark it as a ssgi one - so that it uses darkness as well...
        //
        
        {
            //
            // Bloom +-X axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    VectorSet4( local0, 1.0f, 0.0f, SCAN_WIDTH, 0.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
            
            //
            // Bloom +-Y axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 0.0f, 1.0f, SCAN_WIDTH, 0.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
            
            //
            // Bloom XY & -X-Y axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, 1.0f, 1.0f, SCAN_WIDTH, 0.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
            
            //
            // Bloom -X+Y & +X-Y axis... (to VBO 1)
            //
            
            {
                idRenderSystemGLSLLocal::BindProgram( &tr.ssgiBlurShader );
                
                idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_DIFFUSEMAP );
                
                {
                    vec2_t screensize;
                    screensize[0] = ( F32 )tr.anamorphicRenderFBOImage[0]->width;
                    screensize[1] = ( F32 )tr.anamorphicRenderFBOImage[0]->height;
                    
                    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiBlurShader, UNIFORM_DIMENSIONS, screensize );
                }
                
                {
                    vec4_t local0;
                    //VectorSet4(local0, (F32)width, 0.0, 0.0, 0.0);
                    VectorSet4( local0, -1.0f, 1.0f, SCAN_WIDTH, 0.0f );
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiBlurShader, UNIFORM_LOCAL0, local0 );
                }
                
                idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, NULL, tr.anamorphicRenderFBO[1], NULL, &tr.ssgiBlurShader, color, 0 );
                idRenderSystemFBOLocal::FastBlit( tr.anamorphicRenderFBO[1], NULL, tr.anamorphicRenderFBO[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
            }
        }
        
        //
        // Copy (and upscale) the bloom image to our full screen image...
        //
        
        idRenderSystemFBOLocal::Blit( tr.anamorphicRenderFBO[0], NULL, texDoubleScale, tr.anamorphicRenderFBO[2], NULL, &tr.ssgiBlurShader, color, 0 );
    }
    
    //
    // Do the SSAO/SSGI...
    //
    
    idRenderSystemGLSLLocal::BindProgram( &tr.ssgiShader );
    
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssgiShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssgiShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssgiShader, UNIFORM_NORMALMAP, TB_NORMALMAP ); // really scaled down dynamic glow map
    idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[2], TB_NORMALMAP ); // really scaled down dynamic glow map
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssgiShader, UNIFORM_DIMENSIONS, screensize );
        
        //clientMainSystem->RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        //F32 zmin = backEnd.viewParms.zNear;
        
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, 0.0 );
        
        //clientMainSystem->RefPrintf(PRINT_WARNING, "Sent zmin %f, zmax %f, zmax/zmin %f.\n", zmin, zmax, zmax / zmin);
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec4_t local0;
        local0[0] = r_ssgi->value;
        local0[1] = r_ssgiSamples->value;
        local0[2] = 0.0;
        local0[3] = 0.0;
        
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssgiShader, UNIFORM_LOCAL0, local0 );
        
        //clientMainSystem->RefPrintf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.ssgiShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
}

/*
===============
idRenderSystemPostProcessLocal::ScreenSpaceReflections
===============
*/
void idRenderSystemPostProcessLocal::ScreenSpaceReflections( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.ssrShader );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssrShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    idRenderSystemBackendLocal::BindToTMU( hdrFbo->colorImage[0], TB_DIFFUSEMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssrShader, UNIFORM_GLOWMAP, TB_GLOWMAP );
    
    // Use the most blurred version of glow...
    if ( r_anamorphic->integer )
    {
        idRenderSystemBackendLocal::BindToTMU( tr.anamorphicRenderFBOImage[0], TB_GLOWMAP );
    }
    else if ( r_bloom->integer )
    {
        idRenderSystemBackendLocal::BindToTMU( tr.bloomRenderFBOImage[0], TB_GLOWMAP );
    }
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssrShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.renderDepthImage, TB_LIGHTMAP );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.ssrShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    vec4_t local1;
    VectorSet4( local1, ( F32 )r_ssr->integer, ( F32 )r_sse->integer, r_ssrStrength->value, r_sseStrength->value );
    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssrShader, UNIFORM_LOCAL1, local1 );
    
    vec4_t local3;
    VectorSet4( local3, 1.0f, 0.0f, 0.0f, 1.0f );
    idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssrShader, UNIFORM_LOCAL3, local3 );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssrShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t viewInfo;
        F32 zmax = backEnd.viewParms.zFar;
        F32 ymax = zmax * tanf( backEnd.viewParms.fovY * M_PI / 360.0f );
        F32 xmax = zmax * tanf( backEnd.viewParms.fovX * M_PI / 360.0f );
        F32 zmin = r_znear->value;
        VectorSet4( viewInfo, zmin, zmax, zmax / zmin, backEnd.viewParms.fovX );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssrShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, NULL, NULL, tr.genericFbo2, NULL, &tr.ssrShader, color, 0 );
    
    // Combine render
    idRenderSystemGLSLLocal::BindProgram( &tr.ssrCombineShader );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.ssrCombineShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.ssrCombineShader, UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssrCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    idRenderSystemBackendLocal::BindToTMU( hdrFbo->colorImage[0], TB_DIFFUSEMAP );
    
    idRenderSystemGLSLLocal::SetUniformInt( &tr.ssrCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    idRenderSystemBackendLocal::BindToTMU( tr.genericFBO2Image, TB_NORMALMAP );
    
    vec2_t screensize;
    screensize[0] = ( F32 )tr.genericFBO2Image->width;
    screensize[1] = ( F32 )tr.genericFBO2Image->height;
    idRenderSystemGLSLLocal::SetUniformVec2( &tr.ssrCombineShader, UNIFORM_DIMENSIONS, screensize );
    
    idRenderSystemFBOLocal::Blit( hdrFbo, NULL, NULL, ldrFbo, NULL, &tr.ssrCombineShader, color, 0 );
}

void idRenderSystemPostProcessLocal::Underwater( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox )
{
    vec4_t color;
    
    // bloom
    color[0] =
        color[1] =
            color[2] = powf( 2, r_cameraExposure->value );
    color[3] = 1.0f;
    
    idRenderSystemGLSLLocal::BindProgram( &tr.underWaterShader );
    idRenderSystemBackendLocal::BindToTMU( tr.fixedLevelsImage, TB_LEVELSMAP );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.underWaterShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    
    idRenderSystemGLSLLocal::SetUniformFloat( &tr.underWaterShader, UNIFORM_TIME, ( F32 )( backEnd.refdef.floatTime * 5.0f )/*tr.refdef.floatTime*/ );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        idRenderSystemGLSLLocal::SetUniformVec2( &tr.underWaterShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    idRenderSystemFBOLocal::Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.underWaterShader, color, 0 );
}
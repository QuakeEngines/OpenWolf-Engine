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
// File name:   tr_init.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: functions that are not called every frame
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

vidconfig_t glConfig;
glRefConfig_t glRefConfig;
bool textureFilterAnisotropic = false;
S32 maxAnisotropy = 0;
F32 displayAspect = 0.0f;

glstate_t glState;

convar_t* r_flareSize;
convar_t* r_flareFade;
convar_t* r_flareCoeff;

convar_t* r_railWidth;
convar_t* r_railCoreWidth;
convar_t* r_railSegmentLength;

convar_t* r_verbose;

convar_t* r_detailTextures;

convar_t* r_znear;
convar_t* r_zproj;
convar_t* r_stereoSeparation;

convar_t* r_skipBackEnd;

convar_t* r_stereoEnabled;
convar_t* r_anaglyphMode;

convar_t* r_greyscale;

convar_t* r_ignorehwgamma;
convar_t* r_measureOverdraw;

convar_t* r_inGameVideo;
convar_t* r_fastsky;
convar_t* r_drawSun;
convar_t* r_dynamiclight;

convar_t* r_lodbias;
convar_t* r_lodscale;

convar_t* r_norefresh;
convar_t* r_drawentities;
convar_t* r_drawworld;
convar_t* r_speeds;
convar_t* r_fullbright;
convar_t* r_novis;
convar_t* r_nocull;
convar_t* r_facePlaneCull;
convar_t* r_showcluster;
convar_t* r_nocurves;

convar_t* r_allowExtensions;

convar_t* r_ext_compressed_textures;
convar_t* r_ext_multitexture;
convar_t* r_ext_compiled_vertex_array;
convar_t* r_ext_texture_env_add;
convar_t* r_ext_texture_filter_anisotropic;
convar_t* r_ext_max_anisotropy;

convar_t* r_ext_framebuffer_object;
convar_t* r_ext_texture_float;
convar_t* r_ext_framebuffer_multisample;
convar_t* r_arb_seamless_cube_map;
convar_t* r_arb_vertex_array_object;
convar_t* r_ext_direct_state_access;

convar_t* r_cameraExposure;

convar_t* r_hdr;
convar_t* r_truehdr;
convar_t* r_floatLightmap;
convar_t* r_postProcess;

convar_t* r_toneMap;
convar_t* r_forceToneMap;
convar_t* r_forceToneMapMin;
convar_t* r_forceToneMapAvg;
convar_t* r_forceToneMapMax;

convar_t* r_autoExposure;
convar_t* r_forceAutoExposure;
convar_t* r_forceAutoExposureMin;
convar_t* r_forceAutoExposureMax;

convar_t* r_depthPrepass;
convar_t* r_ssao;

convar_t* r_normalMapping;
convar_t* r_specularMapping;
convar_t* r_deluxeMapping;
convar_t* r_parallaxMapping;
convar_t* r_parallaxMapShadows;
convar_t* r_cubeMapping;
convar_t* r_horizonFade;
convar_t* r_cubemapSize;
convar_t* r_deluxeSpecular;
convar_t* r_pbr;
convar_t* r_baseNormalX;
convar_t* r_baseNormalY;
convar_t* r_baseParallax;
convar_t* r_baseSpecular;
convar_t* r_baseGloss;
convar_t* r_glossType;
convar_t* r_mergeLightmaps;
convar_t* r_dlightMode;
convar_t* r_pshadowDist;
convar_t* r_imageUpsample;
convar_t* r_imageUpsampleMaxSize;
convar_t* r_imageUpsampleType;
convar_t* r_genNormalMaps;
convar_t* r_forceSun;
convar_t* r_forceSunLightScale;
convar_t* r_forceSunAmbientScale;
convar_t* r_sunlightMode;
convar_t* r_drawSunRays;
convar_t* r_sunShadows;
convar_t* r_shadowFilter;
convar_t* r_shadowBlur;
convar_t* r_shadowMapSize;
convar_t* r_shadowCascadeZNear;
convar_t* r_shadowCascadeZFar;
convar_t* r_shadowCascadeZBias;
convar_t* r_ignoreDstAlpha;

convar_t* r_ignoreGLErrors;
convar_t* r_logFile;

convar_t* r_stencilbits;
convar_t* r_depthbits;
convar_t* r_colorbits;
convar_t* r_texturebits;
convar_t* r_ext_multisample;

convar_t* r_drawBuffer;
convar_t* r_lightmap;
convar_t* r_vertexLight;
convar_t* r_uiFullScreen;
convar_t* r_shadows;
convar_t* r_flares;
convar_t* r_mode;
convar_t* r_nobind;
convar_t* r_singleShader;
convar_t* r_roundImagesDown;
convar_t* r_colorMipLevels;
convar_t* r_picmip;
convar_t* r_showtris;
convar_t* r_showsky;
convar_t* r_shownormals;
convar_t* r_finish;
convar_t* r_clear;
convar_t* r_swapInterval;
convar_t* r_textureMode;
convar_t* r_offsetFactor;
convar_t* r_offsetUnits;
convar_t* r_intensity;
convar_t* r_lockpvs;
convar_t* r_noportals;
convar_t* r_portalOnly;

convar_t* r_subdivisions;
convar_t* r_lodCurveError;

convar_t* r_fullscreen;
convar_t* r_noborder;

convar_t* r_customwidth;
convar_t* r_customheight;
convar_t* r_pixelAspect;

convar_t* r_overBrightBits;
convar_t* r_mapOverBrightBits;

convar_t* r_debugSurface;
convar_t* r_simpleMipMaps;

convar_t* r_showImages;

convar_t* r_ambientScale;
convar_t* r_directedScale;
convar_t* r_debugLight;
convar_t* r_debugSort;
convar_t* r_printShaders;
convar_t* r_saveFontData;

convar_t* r_marksOnTriangleMeshes;

convar_t* r_aviMotionJpegQuality;
convar_t* r_screenshotJpegQuality;

convar_t* r_lensflare;
convar_t* r_anamorphic;
convar_t* r_anamorphicDarkenPower;
convar_t* r_ssgi;
convar_t* r_ssgiWidth;
convar_t* r_ssgiSamples;
convar_t* r_ssr;
convar_t* r_ssrStrength;
convar_t* r_sse;
convar_t* r_sseStrength;
convar_t* r_darkexpand;
convar_t* r_dof;
convar_t* r_esharpening;
convar_t* r_esharpening2;
convar_t* r_multipost;
convar_t* r_textureClean;
convar_t* r_textureCleanSigma;
convar_t* r_textureCleanBSigma;
convar_t* r_textureCleanMSize;
convar_t* r_trueAnaglyph;
convar_t* r_trueAnaglyphSeparation;
convar_t* r_trueAnaglyphRed;
convar_t* r_trueAnaglyphGreen;
convar_t* r_trueAnaglyphBlue;
convar_t* r_vibrancy;
convar_t* r_textureDetail;
convar_t* r_textureDetailStrength;
convar_t* r_rbm;
convar_t* r_rbmStrength;
convar_t* r_screenblur;
convar_t* r_brightness;
convar_t* r_contrast;
convar_t* r_gamma;
convar_t* r_bloom;
convar_t* r_bloomPasses;
convar_t* r_bloomDarkenPower;
convar_t* r_bloomScale;
convar_t* r_fxaa;
convar_t* r_proceduralSun;
convar_t* r_proceduralSunScale;

convar_t*	r_maxpolys;
S32	max_polys;
convar_t* r_maxpolyverts;
S32	max_polyverts;

idRenderSystemInitLocal renderSystemInitLocal;

/*
===============
idRenderSystemInitLocal::idRenderSystemInitLocal
===============
*/
idRenderSystemInitLocal::idRenderSystemInitLocal( void )
{
}

/*
===============
idRenderSystemInitLocal::~idRenderSystemInitLocal
===============
*/
idRenderSystemInitLocal::~idRenderSystemInitLocal( void )
{
}

/*
** idRenderSystemInitLocal::InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
void idRenderSystemInitLocal::InitOpenGL( void )
{
    //
    // initialize OS specific portions of the renderer
    //
    // GLimp_Init directly or indirectly references the following cvars:
    //		- r_fullscreen
    //		- r_mode
    //		- r_(color|depth|stencil)bits
    //		- r_ignorehwgamma
    //		- r_gamma
    //
    
    if ( glConfig.vidWidth == 0 )
    {
        S32		temp;
        
        idRenderSystemGlimpLocal::Init( false );
        idRenderSystemExtensionsLocal::InitExtraExtensions();
        
        glConfig.textureEnvAddAvailable = true;
        
        // OpenGL driver constants
        qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
        glConfig.maxTextureSize = temp;
        
        // stubbed or broken drivers may have reported 0...
        if ( glConfig.maxTextureSize <= 0 )
        {
            glConfig.maxTextureSize = 0;
        }
        
        qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &temp );
        glConfig.numTextureUnits = temp;
        
        // reserve 160 components for other uniforms
        qglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS, &temp );
        glRefConfig.glslMaxAnimatedBones = ( S32 )Com_Clamp( 0, IQM_MAX_JOINTS, ( F32 )( ( temp - 160 ) / 16 ) );
        if ( glRefConfig.glslMaxAnimatedBones < 12 )
        {
            glRefConfig.glslMaxAnimatedBones = 0;
        }
    }
    
    // set default state
    SetDefaultState();
}

/*
==================
idRenderSystemInitLocal::CheckErrors
==================
*/
void idRenderSystemInitLocal::CheckErrors( StringEntry file, S32 line )
{
    S32 err;
    UTF8 s[64];
    
    err = qglGetError();
    if ( err == GL_NO_ERROR )
    {
        return;
    }
    if ( r_ignoreGLErrors->integer )
    {
        return;
    }
    switch ( err )
    {
        case GL_INVALID_ENUM:
            strcpy( s, "GL_INVALID_ENUM" );
            break;
        case GL_INVALID_VALUE:
            strcpy( s, "GL_INVALID_VALUE" );
            break;
        case GL_INVALID_OPERATION:
            strcpy( s, "GL_INVALID_OPERATION" );
            break;
        case GL_STACK_OVERFLOW:
            strcpy( s, "GL_STACK_OVERFLOW" );
            break;
        case GL_STACK_UNDERFLOW:
            strcpy( s, "GL_STACK_UNDERFLOW" );
            break;
        case GL_OUT_OF_MEMORY:
            strcpy( s, "GL_OUT_OF_MEMORY" );
            break;
        default:
            Q_snprintf( s, sizeof( s ), "%i", err );
            break;
    }
    
    Com_Error( ERR_FATAL, "idRenderSystemInitLocal::CheckErrors: %s in %s at line %d", s, file, line );
}

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    StringEntry description;
    S32        width, height;
    F32      pixelAspect; // pixel width / height
} vidmode_t;

//Dushan
static const vidmode_t r_vidModes[] =
{
    { "1280x720  (16:9)",  1280, 720, 1 },
    { "1280x768  (5:4)",   1280, 768, 1 },
    { "1280x800  (16:10)", 1280, 800, 1 },
    { "1280x854  (3:2)",   1280, 854, 1 },
    { "1280x1024 (5:4) ",  1280, 1024, 1 },
    { "1366x768  (16:9)",  1366, 768, 1 },
    { "1440x900  (16:10)", 1440, 900, 1 },
    { "1440x960  (3:2)",   1440, 960, 1 },
    { "1600x900  (16:9)",  1600, 900, 1 },
    { "1680x1050 (16:10)", 1680, 1050, 1 },
    { "1600x1200 (4:3)",   1600, 1200, 1 },
    { "1920x1080 (16:9)",  1920, 1080, 1 },
    { "1920x1200 (16:10)", 1920, 1200, 1 },
    { "2048x1080 (17:9)",  2048, 1080, 1 },
    { "2048x1536 (4:3)",   2048, 1536, 1 },
    { "2560x1080 (21:9)",  2560, 1080, 1 },
    { "2560x1600 (16:10)", 2560, 1600, 1 },
    { "2560x2048 (5:4)",   2560, 2048, 1 },
    { "2880x1920 (3:2)",   2880, 1920, 1 },
    { "3440x1440 (21:9)",  3440, 1440, 1 },
    { "3840x2400 (16:10)", 3840, 2400, 1 },
    { "4096x2160 (1.89:1)", 4096, 2160, 1 },
    { "5120x4096 (5:4)",   5120, 4096, 1 },
    { "7680x4800 (16:10)", 7680, 4800, 1 },
};

static const S32 s_numVidModes = ARRAY_LEN( r_vidModes );

bool idRenderSystemInitLocal::GetModeInfo( S32* width, S32* height, F32* windowAspect, S32 mode )
{
    const vidmode_t* vm;
    
    if ( mode < -2 )
    {
        return false;
    }
    
    if ( mode >= s_numVidModes )
    {
        return false;
    }
    
    if ( mode == -2 )
    {
        // Must set width and height to display size before calling this function!
        *windowAspect = ( F32 ) * width / *height;
    }
    else if ( mode == -1 )
    {
        *width = r_customwidth->integer;
        *height = r_customheight->integer;
    }
    else
    {
        vm = &r_vidModes[ mode ];
        
        *width = vm->width;
        *height = vm->height;
        *windowAspect = ( F32 ) vm->width / ( vm->height * vm->pixelAspect );
    }
    
    return true;
}

/*
** R_ModeList_f
*/
void idRenderSystemInitLocal::ModeList_f( void )
{
    S32 i;
    
    clientMainSystem->RefPrintf( PRINT_ALL, "\n" );
    
    for ( i = 0; i < s_numVidModes; i++ )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "Mode %-2d: %s\n", i, r_vidModes[ i ].description );
    }
    
    clientMainSystem->RefPrintf( PRINT_ALL, "\n" );
}


/*
==============================================================================

						SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with fileSystem->FileExists / fileSystem->FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with memorySystem->FreeTempMemory()
==================
*/

U8* idRenderSystemInitLocal::ReadPixels( S32 x, S32 y, S32 width, S32 height, U64* offset, S32* padlen )
{
    U8* buffer = nullptr, *bufstart;
    S32 padwidth, linelen, packAlign;
    
    qglGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );
    
    linelen = width * 3;
    padwidth = PAD( linelen, packAlign );
    
    // Allocate a few more bytes so that we can choose an alignment we like
    buffer = ( U8* )memorySystem->AllocateTempMemory( padwidth * height + *offset + packAlign - 1 );
    
    bufstart = ( U8* )PADP( ( intptr_t ) buffer + *offset, packAlign );
    
    qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart );
    
    *offset = ( U64 )( bufstart - buffer );
    *padlen = padwidth - linelen;
    
    return buffer;
}

/*
==================
idRenderSystemInitLocal::TakeScreenshot
==================
*/
void idRenderSystemInitLocal::TakeScreenshot( S32 x, S32 y, S32 width, S32 height, UTF8* fileName )
{
    U8* allbuf, *buffer, * srcptr, *destptr, * endline, *endmem, temp;
    
    S32 linelen, padlen;
    U64 offset = 18, memcount;
    
    allbuf = ReadPixels( x, y, width, height, &offset, &padlen );
    buffer = allbuf + offset - 18;
    
    ::memset( buffer, 0, 18 );
    // uncompressed type
    buffer[2] = 2;
    buffer[12] = width & 255;
    buffer[13] = width >> 8;
    buffer[14] = height & 255;
    buffer[15] = height >> 8;
    // pixel size
    buffer[16] = 24;
    
    // swap rgb to bgr and remove padding from line endings
    linelen = width * 3;
    
    srcptr = destptr = allbuf + offset;
    endmem = srcptr + ( linelen + padlen ) * height;
    
    while ( srcptr < endmem )
    {
        endline = srcptr + linelen;
        
        while ( srcptr < endline )
        {
            temp = srcptr[0];
            *destptr++ = srcptr[2];
            *destptr++ = srcptr[1];
            *destptr++ = temp;
            
            srcptr += 3;
        }
        
        // Skip the pad
        srcptr += padlen;
    }
    
    memcount = linelen * height;
    
    fileSystem->WriteFile( fileName, buffer, memcount + 18 );
    
    memorySystem->FreeTempMemory( allbuf );
}

/*
==================
idRenderSystemInitLocal::TakeScreenshotJPEG
==================
*/
void idRenderSystemInitLocal::TakeScreenshotJPEG( S32 x, S32 y, S32 width, S32 height, UTF8* fileName )
{
    S32 padlen;
    U64 offset = 0, memcount;
    U8* buffer;
    
    buffer = ReadPixels( x, y, width, height, &offset, &padlen );
    memcount = ( width * 3 + padlen ) * height;
    
    idRenderSystemImageJPEGLocal::SaveJPG( fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen );
    memorySystem->FreeTempMemory( buffer );
}

/*
==================
idRenderSystemInitLocal::TakeScreenshotCmd
==================
*/
const void* idRenderSystemInitLocal::TakeScreenshotCmd( const void* data )
{
    const screenshotCommand_t*	cmd;
    
    cmd = ( const screenshotCommand_t* )data;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    if ( cmd->jpeg )
    {
        TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
    }
    else
    {
        TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
    }
    
    return ( const void* )( cmd + 1 );
}

/*
==================
idRenderSystemInitLocal::TakeScreenshot
==================
*/
void idRenderSystemInitLocal::TakeScreenshot( S32 x, S32 y, S32 width, S32 height, UTF8* name, bool jpeg )
{
    // bad things if two screenshots per frame?
    static UTF8	fileName[MAX_OSPATH];
    screenshotCommand_t*	cmd = nullptr;
    
    cmd = GetCommandBuffer( sizeof * cmd , cmd );
    if ( !cmd )
    {
        return;
    }
    cmd->commandId = RC_SCREENSHOT;
    
    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    Q_strncpyz( fileName, name, sizeof( fileName ) );
    cmd->fileName = fileName;
    cmd->jpeg = jpeg;
}

/*
==================
idRenderSystemInitLocal::ScreenshotFilename
==================
*/
void idRenderSystemInitLocal::ScreenshotFilename( S32 lastNumber, UTF8* fileName )
{
    S32	a, b, c, d;
    
    if ( lastNumber < 0 || lastNumber > 9999 )
    {
        Q_snprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
        return;
    }
    
    a = lastNumber / 1000;
    lastNumber -= a * 1000;
    
    b = lastNumber / 100;
    lastNumber -= b * 100;
    
    c = lastNumber / 10;
    lastNumber -= c * 10;
    
    d = lastNumber;
    
    Q_snprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga", a, b, c, d );
}

/*
==================
idRenderSystemInitLocal::ScreenshotFilename
==================
*/
void idRenderSystemInitLocal::ScreenshotFilenameJPEG( S32 lastNumber, UTF8* fileName )
{
    S32 a, b, c, d;
    
    if ( lastNumber < 0 || lastNumber > 9999 )
    {
        Q_snprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
        return;
    }
    
    a = lastNumber / 1000;
    lastNumber -= a * 1000;
    
    b = lastNumber / 100;
    lastNumber -= b * 100;
    
    c = lastNumber / 10;
    lastNumber -= c * 10;
    
    d = lastNumber;
    
    Q_snprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
                , a, b, c, d );
}

/*
====================
idRenderSystemInitLocal::LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void idRenderSystemInitLocal::LevelShot( void )
{
    S32	padlen, x, y, r, g, b, xx, yy;
    U64 offset = 0;
    UTF8 checkname[MAX_OSPATH];
    U8* buffer = nullptr, * source, *allsource, * src, *dst;
    F32	 xScale, yScale;
    
    Q_snprintf( checkname, sizeof( checkname ), "levelshots/%s.tga", tr.world->baseName );
    
    allsource = ReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen );
    source = allsource + offset;
    
    buffer = ( U8* )memorySystem->AllocateTempMemory( 128 * 128 * 3 + 18 );
    ::memset( buffer, 0, 18 );
    // uncompressed type
    buffer[2] = 2;
    buffer[12] = 128;
    buffer[14] = 128;
    // pixel size
    buffer[16] = 24;
    
    // resample from source
    xScale = glConfig.vidWidth / 512.0f;
    yScale = glConfig.vidHeight / 384.0f;
    
    for ( y = 0 ; y < 128 ; y++ )
    {
        for ( x = 0 ; x < 128 ; x++ )
        {
            r = g = b = 0;
            for ( yy = 0 ; yy < 3 ; yy++ )
            {
                for ( xx = 0 ; xx < 4 ; xx++ )
                {
                    src = source + ( 3 * glConfig.vidWidth + padlen ) * ( S32 )( ( y * 3 + yy ) * yScale ) + 3 * ( S32 )( ( x * 4 + xx ) * xScale );
                    r += src[0];
                    g += src[1];
                    b += src[2];
                }
            }
            dst = buffer + 18 + 3 * ( y * 128 + x );
            dst[0] = b / 12;
            dst[1] = g / 12;
            dst[2] = r / 12;
        }
    }
    
    fileSystem->WriteFile( checkname, buffer, 128 * 128 * 3 + 18 );
    
    memorySystem->FreeTempMemory( buffer );
    memorySystem->FreeTempMemory( allsource );
    
    clientMainSystem->RefPrintf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
idRenderSystemInitLocal::ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void idRenderSystemInitLocal::ScreenShot_f( void )
{
    static S32 lastNumber = -1;
    UTF8 checkname[MAX_OSPATH];
    bool silent;
    
    if ( !::strcmp( cmdSystem->Argv( 1 ), "levelshot" ) )
    {
        LevelShot();
        return;
    }
    
    if ( !::strcmp( cmdSystem->Argv( 1 ), "silent" ) )
    {
        silent = true;
    }
    else
    {
        silent = false;
    }
    
    if ( cmdSystem->Argc() == 2 && !silent )
    {
        // explicit filename
        Q_snprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", cmdSystem->Argv( 1 ) );
    }
    else
    {
        // scan for a free filename
        
        // if we have saved a previous screenshot, don't scan
        // again, because recording demo avis can involve
        // thousands of shots
        if ( lastNumber == -1 )
        {
            lastNumber = 0;
        }
        // scan for a free number
        for ( ; lastNumber <= 9999 ; lastNumber++ )
        {
            ScreenshotFilename( lastNumber, checkname );
            
            if ( !fileSystem->FileExists( checkname ) )
            {
                // file doesn't exist
                break;
            }
        }
        
        if ( lastNumber >= 9999 )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
            return;
        }
        
        lastNumber++;
    }
    
    TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, false );
    
    if ( !silent )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "Wrote %s\n", checkname );
    }
}

void idRenderSystemInitLocal::ScreenShotJPEG_f( void )
{
    UTF8		checkname[MAX_OSPATH];
    static	S32	lastNumber = -1;
    bool	silent;
    
    if ( !::strcmp( cmdSystem->Argv( 1 ), "levelshot" ) )
    {
        LevelShot();
        return;
    }
    
    if ( !::strcmp( cmdSystem->Argv( 1 ), "silent" ) )
    {
        silent = true;
    }
    else
    {
        silent = false;
    }
    
    if ( cmdSystem->Argc() == 2 && !silent )
    {
        // explicit filename
        Q_snprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", cmdSystem->Argv( 1 ) );
    }
    else
    {
        // scan for a free filename
        
        // if we have saved a previous screenshot, don't scan
        // again, because recording demo avis can involve
        // thousands of shots
        if ( lastNumber == -1 )
        {
            lastNumber = 0;
        }
        
        // scan for a free number
        for ( ; lastNumber <= 9999 ; lastNumber++ )
        {
            ScreenshotFilenameJPEG( lastNumber, checkname );
            
            if ( !fileSystem->FileExists( checkname ) )
            {
                break; // file doesn't exist
            }
        }
        
        if ( lastNumber == 10000 )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
            return;
        }
        
        lastNumber++;
    }
    
    TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, true );
    
    if ( !silent )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "Wrote %s\n", checkname );
    }
}

/*
==================
idRenderSystemInitLocal::ExportCubemaps
==================
*/
void idRenderSystemInitLocal::ExportCubemaps( void )
{
    exportCubemapsCommand_t* cmd = nullptr;
    
    cmd = GetCommandBuffer( sizeof * cmd , cmd );
    if ( !cmd )
    {
        return;
    }
    cmd->commandId = RC_EXPORT_CUBEMAPS;
}

/*
==================
R_ExportCubemaps_f
==================
*/
void idRenderSystemInitLocal::ExportCubemaps_f( void )
{
    ExportCubemaps();
}

/*
==================
idRenderSystemInitLocal::TakeVideoFrameCmd
==================
*/
const void* idRenderSystemInitLocal::TakeVideoFrameCmd( const void* data )
{
    S32	padwidth, avipadwidth, padlen, avipadlen, packAlign;
    size_t memcount;
    U64 linelen;
    U8* cBuf;
    const videoFrameCommand_t*	cmd;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    cmd = ( const videoFrameCommand_t* )data;
    
    qglGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );
    
    linelen = cmd->width * 3;
    
    // Alignment stuff for qglReadPixels
    padwidth = PAD( linelen, packAlign );
    padlen = padwidth - linelen;
    
    // AVI line padding
    avipadwidth = PAD( linelen, AVI_LINE_PADDING );
    avipadlen = avipadwidth - linelen;
    
    cBuf = ( U8* )PADP( cmd->captureBuffer, packAlign );
    
    qglReadPixels( 0, 0, cmd->width, cmd->height, GL_RGB, GL_UNSIGNED_BYTE, cBuf );
    
    memcount = padwidth * cmd->height;
    
    if ( cmd->motionJpeg )
    {
        memcount = idRenderSystemImageJPEGLocal::SaveJPGToBuffer( cmd->encodeBuffer, linelen * cmd->height, r_aviMotionJpegQuality->integer,
                   cmd->width, cmd->height, cBuf, padlen );
        clientAVISystem->WriteAVIVideoFrame( cmd->encodeBuffer, ( S32 )memcount );
    }
    else
    {
        U8* lineend, *memend, * srcptr, *destptr;
        
        srcptr = cBuf;
        destptr = cmd->encodeBuffer;
        memend = srcptr + memcount;
        
        // swap R and B and remove line paddings
        while ( srcptr < memend )
        {
            lineend = srcptr + linelen;
            
            while ( srcptr < lineend )
            {
                *destptr++ = srcptr[2];
                *destptr++ = srcptr[1];
                *destptr++ = srcptr[0];
                srcptr += 3;
            }
            
            ::memset( destptr, '\0', avipadlen );
            destptr += avipadlen;
            
            srcptr += padlen;
        }
        
        clientAVISystem->WriteAVIVideoFrame( cmd->encodeBuffer, avipadwidth * cmd->height );
    }
    
    return ( const void* )( cmd + 1 );
}

/*
** idRenderSystemInitLocal::SetDefaultState
*/
void idRenderSystemInitLocal::SetDefaultState( void )
{
    qglClearDepth( 1.0f );
    
    qglCullFace( GL_FRONT );
    
    idRenderSystemDSALocal::BindNullTextures();
    
    if ( glRefConfig.framebufferObject )
    {
        idRenderSystemDSALocal::BindNullFramebuffers();
    }
    
    idRenderSystemImageLocal::TextureMode( r_textureMode->string );
    
    idRenderSystemInitLocal::CheckErrors( __FILE__, __LINE__ );
    
    //qglShadeModel( GL_SMOOTH );
    qglDepthFunc( GL_LEQUAL );
    
    ::memset( &glState, 0, sizeof( glState ) );
    
    //
    // make sure our GL state vector is set correctly
    //
    glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
    glState.storedGlState = 0;
    glState.faceCulling = CT_TWO_SIDED;
    glState.faceCullFront = true;
    
    idRenderSystemDSALocal::BindNullProgram();
    
    if ( glRefConfig.vertexArrayObject )
    {
        qglBindVertexArray( 0 );
    }
    
    qglBindBuffer( GL_ARRAY_BUFFER, 0 );
    qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glState.currentVao = NULL;
    glState.vertexAttribsEnabled = 0;
    
    qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    qglDepthMask( GL_TRUE );
    qglDisable( GL_DEPTH_TEST );
    qglEnable( GL_SCISSOR_TEST );
    qglDisable( GL_CULL_FACE );
    qglDisable( GL_BLEND );
    
    if ( glRefConfig.seamlessCubeMap )
    {
        qglEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
    }
    
    // GL_POLYGON_OFFSET_FILL will be glEnable()d when this is used
    qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
    
    // FIXME: get color of sky
    qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

/*
================
idRenderSystemInitLocal::PrintLongString

Workaround for clientMainSystem->RefPrintf's 1024 characters buffer limit.
================
*/
void idRenderSystemInitLocal::PrintLongString( StringEntry string )
{
    S32 size = ( S32 )::strlen( string );
    UTF8 buffer[1024];
    StringEntry p;
    
    p = string;
    while ( size > 0 )
    {
        Q_strncpyz( buffer, p, sizeof( buffer ) );
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "%s", buffer );
        
        p += 1023;
        size -= 1023;
    }
}

/*
================
idRenderSystemInitLocal::GfxInfo_f
================
*/
void idRenderSystemInitLocal::GfxInfo_f( void )
{
    StringEntry enablestrings[] =
    {
        "disabled",
        "enabled"
    };
    
    StringEntry fsstrings[] =
    {
        "windowed",
        "fullscreen"
    };
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_VENDOR: %s\n", glConfig.vendor_string );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_RENDERER: %s\n", glConfig.renderer_string );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_VERSION: %s\n", glConfig.version_string );
    
    if ( qglGetStringi )
    {
        GLint numExtensions;
        S32 i;
        
        qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
        for ( i = 0; i < numExtensions; i++ )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "%s ", qglGetStringi( GL_EXTENSIONS, i ) );
        }
    }
    else
    {
        PrintLongString( glConfig.extensions_string );
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "PIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
    
    if ( glConfig.displayFrequency )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "%d\n", glConfig.displayFrequency );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "N/A\n" );
    }
    
    if ( glConfig.deviceSupportsGamma )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "texturemode: %s\n", r_textureMode->string );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "picmip: %d\n", r_picmip->integer );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "texture bits: %d\n", r_texturebits->integer );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );
    
    if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "HACK: using vertex lightmap approximation\n" );
    }
    
    if ( glConfig.hardwareType == GLHW_RAGEPRO )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "HACK: ragePro approximations\n" );
    }
    
    if ( glConfig.hardwareType == GLHW_RIVA128 )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "HACK: riva128 approximations\n" );
    }
    
    if ( r_finish->integer )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Forcing glFinish\n" );
    }
}

/*
===============
idRenderSystemInitLocal::Register
===============
*/
void idRenderSystemInitLocal::Register( void )
{
    // latched and archived variables
    r_allowExtensions = cvarSystem->Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_compressed_textures = cvarSystem->Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_multitexture = cvarSystem->Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_compiled_vertex_array = cvarSystem->Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_texture_env_add = cvarSystem->Get( "r_ext_texture_env_add", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    r_ext_framebuffer_object = cvarSystem->Get( "r_ext_framebuffer_object", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_texture_float = cvarSystem->Get( "r_ext_texture_float", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_framebuffer_multisample = cvarSystem->Get( "r_ext_framebuffer_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_arb_seamless_cube_map = cvarSystem->Get( "r_arb_seamless_cube_map", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_arb_vertex_array_object = cvarSystem->Get( "r_arb_vertex_array_object", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_direct_state_access = cvarSystem->Get( "r_ext_direct_state_access", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    r_ext_texture_filter_anisotropic = cvarSystem->Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_max_anisotropy = cvarSystem->Get( "r_ext_max_anisotropy", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    r_picmip = cvarSystem->Get( "r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_roundImagesDown = cvarSystem->Get( "r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_colorMipLevels = cvarSystem->Get( "r_colorMipLevels", "0", CVAR_LATCH, "description" );
    cvarSystem->CheckRange( r_picmip, 0, 16, true );
    r_detailTextures = cvarSystem->Get( "r_detailTextures", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_texturebits = cvarSystem->Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_colorbits = cvarSystem->Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_stencilbits = cvarSystem->Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_depthbits = cvarSystem->Get( "r_depthbits", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ext_multisample = cvarSystem->Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    cvarSystem->CheckRange( r_ext_multisample, 0, 4, true );
    r_overBrightBits = cvarSystem->Get( "r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ignorehwgamma = cvarSystem->Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_mode = cvarSystem->Get( "r_mode", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_fullscreen = cvarSystem->Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_noborder = cvarSystem->Get( "r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_customwidth = cvarSystem->Get( "r_customwidth", "1680", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_customheight = cvarSystem->Get( "r_customheight", "1050", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_pixelAspect = cvarSystem->Get( "r_pixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_simpleMipMaps = cvarSystem->Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_vertexLight = cvarSystem->Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_uiFullScreen = cvarSystem->Get( "r_uifullscreen", "0", 0, "description" );
    r_subdivisions = cvarSystem->Get( "r_subdivisions", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_stereoEnabled = cvarSystem->Get( "r_stereoEnabled", "0", CVAR_CHEAT, "description" );
    r_greyscale = cvarSystem->Get( "r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    cvarSystem->CheckRange( r_greyscale, 0, 1, false );
    
    r_hdr = cvarSystem->Get( "r_hdr", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_truehdr = cvarSystem->Get( "r_truehdr", "1", CVAR_ARCHIVE, "description" );
    r_floatLightmap = cvarSystem->Get( "r_floatLightmap", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_postProcess = cvarSystem->Get( "r_postProcess", "1", CVAR_ARCHIVE, "description" );
    
    r_toneMap = cvarSystem->Get( "r_toneMap", "1", CVAR_ARCHIVE, "description" );
    r_forceToneMap = cvarSystem->Get( "r_forceToneMap", "0", CVAR_CHEAT, "description" );
    r_forceToneMapMin = cvarSystem->Get( "r_forceToneMapMin", "-8.0", CVAR_CHEAT, "description" );
    r_forceToneMapAvg = cvarSystem->Get( "r_forceToneMapAvg", "-2.0", CVAR_CHEAT, "description" );
    r_forceToneMapMax = cvarSystem->Get( "r_forceToneMapMax", "0.0", CVAR_CHEAT, "description" );
    
    r_autoExposure = cvarSystem->Get( "r_autoExposure", "1", CVAR_ARCHIVE, "description" );
    r_forceAutoExposure = cvarSystem->Get( "r_forceAutoExposure", "0", CVAR_CHEAT, "description" );
    r_forceAutoExposureMin = cvarSystem->Get( "r_forceAutoExposureMin", "-2.0", CVAR_CHEAT, "description" );
    r_forceAutoExposureMax = cvarSystem->Get( "r_forceAutoExposureMax", "2.0", CVAR_CHEAT, "description" );
    
    r_cameraExposure = cvarSystem->Get( "r_cameraExposure", "0", CVAR_CHEAT, "description" );
    
    r_depthPrepass = cvarSystem->Get( "r_depthPrepass", "1", CVAR_ARCHIVE, "description" );
    r_ssao = cvarSystem->Get( "r_ssao", "0", CVAR_LATCH | CVAR_ARCHIVE, "description" );
    
    r_normalMapping = cvarSystem->Get( "r_normalMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_specularMapping = cvarSystem->Get( "r_specularMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_deluxeMapping = cvarSystem->Get( "r_deluxeMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_parallaxMapping = cvarSystem->Get( "r_parallaxMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_parallaxMapShadows = cvarSystem->Get( "r_parallaxMapShadows", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_cubeMapping = cvarSystem->Get( "r_cubeMapping", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_horizonFade = cvarSystem->Get( "r_horizonFade", "3", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_cubemapSize = cvarSystem->Get( "r_cubemapSize", "128", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_deluxeSpecular = cvarSystem->Get( "r_deluxeSpecular", "0.3", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_pbr = cvarSystem->Get( "r_pbr", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_baseNormalX = cvarSystem->Get( "r_baseNormalX", "1.0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_baseNormalY = cvarSystem->Get( "r_baseNormalY", "1.0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_baseParallax = cvarSystem->Get( "r_baseParallax", "0.001", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_baseSpecular = cvarSystem->Get( "r_baseSpecular", "0.04", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_baseGloss = cvarSystem->Get( "r_baseGloss", "0.3", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_glossType = cvarSystem->Get( "r_glossType", "2", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_dlightMode = cvarSystem->Get( "r_dlightMode", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_pshadowDist = cvarSystem->Get( "r_pshadowDist", "128", CVAR_ARCHIVE, "description" );
    r_mergeLightmaps = cvarSystem->Get( "r_mergeLightmaps", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_imageUpsample = cvarSystem->Get( "r_imageUpsample", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_imageUpsampleMaxSize = cvarSystem->Get( "r_imageUpsampleMaxSize", "1024", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_imageUpsampleType = cvarSystem->Get( "r_imageUpsampleType", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_genNormalMaps = cvarSystem->Get( "r_genNormalMaps", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    r_forceSun = cvarSystem->Get( "r_forceSun", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_forceSunLightScale = cvarSystem->Get( "r_forceSunLightScale", "1.0", CVAR_CHEAT, "description" );
    r_forceSunAmbientScale = cvarSystem->Get( "r_forceSunAmbientScale", "0.5", CVAR_CHEAT, "description" );
    r_drawSunRays = cvarSystem->Get( "r_drawSunRays", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_sunlightMode = cvarSystem->Get( "r_sunlightMode", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    r_sunShadows = cvarSystem->Get( "r_sunShadows", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_shadowFilter = cvarSystem->Get( "r_shadowFilter", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_shadowBlur = cvarSystem->Get( "r_shadowBlur", "1", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_shadowMapSize = cvarSystem->Get( "r_shadowMapSize", "1024", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_shadowCascadeZNear = cvarSystem->Get( "r_shadowCascadeZNear", "8", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_shadowCascadeZFar = cvarSystem->Get( "r_shadowCascadeZFar", "1024", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_shadowCascadeZBias = cvarSystem->Get( "r_shadowCascadeZBias", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_ignoreDstAlpha = cvarSystem->Get( "r_ignoreDstAlpha", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    // temporary latched variables that can only change over a restart
    r_fullbright = cvarSystem->Get( "r_fullbright", "0", CVAR_LATCH | CVAR_CHEAT, "description" );
    r_mapOverBrightBits = cvarSystem->Get( "r_mapOverBrightBits", "1", CVAR_LATCH, "description" );
    r_intensity = cvarSystem->Get( "r_intensity", "1", CVAR_LATCH, "description" );
    r_singleShader = cvarSystem->Get( "r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH, "description" );
    
    // archived variables that can change at any time
    r_lodCurveError = cvarSystem->Get( "r_lodCurveError", "250", CVAR_ARCHIVE | CVAR_CHEAT, "description" );
    r_lodbias = cvarSystem->Get( "r_lodbias", "0", CVAR_ARCHIVE, "description" );
    r_flares = cvarSystem->Get( "r_flares", "0", CVAR_ARCHIVE, "description" );
    r_znear = cvarSystem->Get( "r_znear", "4", CVAR_CHEAT, "description" );
    cvarSystem->CheckRange( r_znear, 0.001f, 200, false );
    r_zproj = cvarSystem->Get( "r_zproj", "64", CVAR_ARCHIVE, "description" );
    r_stereoSeparation = cvarSystem->Get( "r_stereoSeparation", "64", CVAR_ARCHIVE, "description" );
    r_ignoreGLErrors = cvarSystem->Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE, "description" );
    r_fastsky = cvarSystem->Get( "r_fastsky", "0", CVAR_ARCHIVE, "description" );
    r_inGameVideo = cvarSystem->Get( "r_inGameVideo", "1", CVAR_ARCHIVE, "description" );
    r_drawSun = cvarSystem->Get( "r_drawSun", "1", CVAR_ARCHIVE, "description" );
    r_dynamiclight = cvarSystem->Get( "r_dynamiclight", "1", CVAR_ARCHIVE, "description" );
    r_finish = cvarSystem->Get( "r_finish", "0", CVAR_ARCHIVE, "description" );
    r_textureMode = cvarSystem->Get( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE, "description" );
    r_swapInterval = cvarSystem->Get( "r_swapInterval", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_facePlaneCull = cvarSystem->Get( "r_facePlaneCull", "1", CVAR_ARCHIVE, "description" );
    
    r_railWidth = cvarSystem->Get( "r_railWidth", "16", CVAR_ARCHIVE, "description" );
    r_railCoreWidth = cvarSystem->Get( "r_railCoreWidth", "6", CVAR_ARCHIVE, "description" );
    r_railSegmentLength = cvarSystem->Get( "r_railSegmentLength", "32", CVAR_ARCHIVE, "description" );
    
    r_ambientScale = cvarSystem->Get( "r_ambientScale", "0.6", CVAR_CHEAT, "description" );
    r_directedScale = cvarSystem->Get( "r_directedScale", "1", CVAR_CHEAT, "description" );
    
    r_anaglyphMode = cvarSystem->Get( "r_anaglyphMode", "0", CVAR_CHEAT, "description" );
    
    // temporary variables that can change at any time
    r_showImages = cvarSystem->Get( "r_showImages", "0", CVAR_CHEAT | CVAR_TEMP, "description" );
    
    r_debugLight = cvarSystem->Get( "r_debuglight", "0", CVAR_TEMP, "description" );
    r_debugSort = cvarSystem->Get( "r_debugSort", "0", CVAR_CHEAT, "description" );
    r_printShaders = cvarSystem->Get( "r_printShaders", "0", 0, "description" );
    r_saveFontData = cvarSystem->Get( "r_saveFontData", "0", 0, "description" );
    
    r_nocurves = cvarSystem->Get( "r_nocurves", "0", CVAR_CHEAT, "description" );
    r_drawworld = cvarSystem->Get( "r_drawworld", "1", CVAR_CHEAT, "description" );
    r_lightmap = cvarSystem->Get( "r_lightmap", "0", 0, "description" );
    r_portalOnly = cvarSystem->Get( "r_portalOnly", "0", CVAR_CHEAT, "description" );
    
    r_flareSize = cvarSystem->Get( "r_flareSize", "40", CVAR_CHEAT, "description" );
    r_flareFade = cvarSystem->Get( "r_flareFade", "7", CVAR_CHEAT, "description" );
    r_flareCoeff = cvarSystem->Get( "r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT, "description" );
    
    r_skipBackEnd = cvarSystem->Get( "r_skipBackEnd", "0", CVAR_CHEAT, "description" );
    
    r_measureOverdraw = cvarSystem->Get( "r_measureOverdraw", "0", CVAR_CHEAT, "description" );
    r_lodscale = cvarSystem->Get( "r_lodscale", "5", CVAR_CHEAT, "description" );
    r_norefresh = cvarSystem->Get( "r_norefresh", "0", CVAR_CHEAT, "description" );
    r_drawentities = cvarSystem->Get( "r_drawentities", "1", CVAR_CHEAT, "description" );
    r_nocull = cvarSystem->Get( "r_nocull", "0", CVAR_CHEAT, "description" );
    r_novis = cvarSystem->Get( "r_novis", "0", CVAR_CHEAT, "description" );
    r_showcluster = cvarSystem->Get( "r_showcluster", "0", CVAR_CHEAT, "description" );
    r_speeds = cvarSystem->Get( "r_speeds", "0", CVAR_CHEAT, "description" );
    r_verbose = cvarSystem->Get( "r_verbose", "0", CVAR_CHEAT, "description" );
    r_logFile = cvarSystem->Get( "r_logFile", "0", CVAR_CHEAT, "description" );
    r_debugSurface = cvarSystem->Get( "r_debugSurface", "0", CVAR_CHEAT, "description" );
    r_nobind = cvarSystem->Get( "r_nobind", "0", CVAR_CHEAT, "description" );
    r_showtris = cvarSystem->Get( "r_showtris", "0", CVAR_CHEAT, "description" );
    r_showsky = cvarSystem->Get( "r_showsky", "0", CVAR_CHEAT, "description" );
    r_shownormals = cvarSystem->Get( "r_shownormals", "0", CVAR_CHEAT, "description" );
    r_clear = cvarSystem->Get( "r_clear", "0", CVAR_CHEAT, "description" );
    r_offsetFactor = cvarSystem->Get( "r_offsetfactor", "-1", CVAR_CHEAT, "description" );
    r_offsetUnits = cvarSystem->Get( "r_offsetunits", "-2", CVAR_CHEAT, "description" );
    r_drawBuffer = cvarSystem->Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT, "description" );
    r_lockpvs = cvarSystem->Get( "r_lockpvs", "0", CVAR_CHEAT, "description" );
    r_noportals = cvarSystem->Get( "r_noportals", "0", CVAR_CHEAT, "description" );
    r_shadows = cvarSystem->Get( "cg_shadows", "4", 0, "description" );
    
    r_marksOnTriangleMeshes = cvarSystem->Get( "r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE, "description" );
    
    r_aviMotionJpegQuality = cvarSystem->Get( "r_aviMotionJpegQuality", "90", CVAR_ARCHIVE, "description" );
    r_screenshotJpegQuality = cvarSystem->Get( "r_screenshotJpegQuality", "90", CVAR_ARCHIVE, "description" );
    
    //postprocess effects
    r_lensflare = cvarSystem->Get( "r_lensflare", "0", CVAR_ARCHIVE, "description" );
    r_anamorphic = cvarSystem->Get( "r_anamorphic", "0", CVAR_ARCHIVE, "description" );
    r_anamorphicDarkenPower = cvarSystem->Get( "r_anamorphicDarkenPower", "256.0", CVAR_ARCHIVE, "description" );
    r_ssgi = cvarSystem->Get( "r_ssgi", "0", CVAR_ARCHIVE, "description" );
    r_ssgiWidth = cvarSystem->Get( "r_ssgiWidth", "8.0", CVAR_ARCHIVE, "description" );
    r_ssgiSamples = cvarSystem->Get( "r_ssgiSamples", "2", CVAR_ARCHIVE, "description" );
    r_ssr = cvarSystem->Get( "r_ssr", "0", CVAR_ARCHIVE, "description" );
    r_ssrStrength = cvarSystem->Get( "r_ssrStrength", "0.02", CVAR_ARCHIVE, "description" );
    r_sse = cvarSystem->Get( "r_sse", "0", CVAR_ARCHIVE, "description" );
    r_sseStrength = cvarSystem->Get( "rr_sseStrength", "0.02", CVAR_ARCHIVE, "description" );
    r_darkexpand = cvarSystem->Get( "r_darkexpand", "1", CVAR_ARCHIVE, "description" );
    r_dof = cvarSystem->Get( "r_dof", "0", CVAR_ARCHIVE, "description" );
    r_esharpening = cvarSystem->Get( "r_esharpening", "0", CVAR_ARCHIVE, "description" );
    r_esharpening2 = cvarSystem->Get( "r_esharpening2", "0", CVAR_ARCHIVE, "description" );
    r_multipost = cvarSystem->Get( "r_multipost", "0", CVAR_ARCHIVE, "description" );
    r_textureClean = cvarSystem->Get( "r_textureClean", "0", CVAR_ARCHIVE, "description" );
    r_textureCleanSigma = cvarSystem->Get( "r_textureCleanSigma", "1.2", CVAR_ARCHIVE, "description" );
    r_textureCleanBSigma = cvarSystem->Get( "r_textureCleanBSigma", "0.1", CVAR_ARCHIVE, "description" );
    r_textureCleanMSize = cvarSystem->Get( "r_textureCleanMSize", "6.0", CVAR_ARCHIVE, "description" );
    r_trueAnaglyph = cvarSystem->Get( "r_trueAnaglyph", "0", CVAR_ARCHIVE, "description" );
    r_trueAnaglyphSeparation = cvarSystem->Get( "r_trueAnaglyphSeparation", "8.0", CVAR_ARCHIVE, "description" );
    r_trueAnaglyphRed = cvarSystem->Get( "r_trueAnaglyphRed", "0.0", CVAR_ARCHIVE, "description" );
    r_trueAnaglyphGreen = cvarSystem->Get( "r_trueAnaglyphGreen", "0.0", CVAR_ARCHIVE, "description" );
    r_trueAnaglyphBlue = cvarSystem->Get( "r_trueAnaglyphBlue", "0.0", CVAR_ARCHIVE, "description" );
    r_vibrancy = cvarSystem->Get( "r_vibrancy", "1.04", CVAR_ARCHIVE, "description" );
    r_bloom = cvarSystem->Get( "r_bloom", "0", CVAR_ARCHIVE, "description" );
    r_bloomPasses = cvarSystem->Get( "r_bloomPasses", "2", CVAR_ARCHIVE, "description" );
    r_bloomDarkenPower = cvarSystem->Get( "r_bloomDarkenPower", "6.5", CVAR_ARCHIVE, "description" );
    r_bloomScale = cvarSystem->Get( "r_bloomScale", "0.5", CVAR_ARCHIVE, "description" );
    r_fxaa = cvarSystem->Get( "r_fxaa", "0", CVAR_ARCHIVE, "description" );
    r_textureDetail = cvarSystem->Get( "r_textureDetail", "0", CVAR_ARCHIVE, "description" );
    r_textureDetailStrength = cvarSystem->Get( "r_textureDetailStrength", "0.004", CVAR_ARCHIVE, "description" );
    r_rbm = cvarSystem->Get( "r_rbm", "0", CVAR_ARCHIVE, "description" );
    r_rbmStrength = cvarSystem->Get( "r_rbmStrength", "0.015", CVAR_ARCHIVE, "description" );
    r_screenblur = cvarSystem->Get( "r_screenBlur", "0", CVAR_ARCHIVE, "description" );
    r_brightness = cvarSystem->Get( "r_brightness", "0.0", CVAR_ARCHIVE, "description" );
    r_contrast = cvarSystem->Get( "r_contrast", "1.0", CVAR_ARCHIVE, "description" );
    r_gamma = cvarSystem->Get( "r_gamma", "1.0", CVAR_ARCHIVE, "description" );
    r_proceduralSun = cvarSystem->Get( "r_proceduralSun", "0", CVAR_ARCHIVE, "description" );
    r_proceduralSunScale = cvarSystem->Get( "r_proceduralSunScale", "1.75", CVAR_ARCHIVE, "description" );
    
    r_maxpolys = cvarSystem->Get( "r_maxpolys", va( "%d", MAX_POLYS ), 0, "description" );
    r_maxpolyverts = cvarSystem->Get( "r_maxpolyverts", va( "%d", MAX_POLYVERTS ), 0, "description" );
    
    // make sure all the commands added here are also
    // removed in Shutdown
    cmdSystem->AddCommand( "imagelist", &idRenderSystemImageLocal::ImageList_f, "description" );
    cmdSystem->AddCommand( "shaderlist", &idRenderSystemShaderLocal::ShaderList_f, "description" );
    cmdSystem->AddCommand( "skinlist", &idRenderSystemSkinsLocal::SkinList_f, "description" );
    cmdSystem->AddCommand( "modellist", &idRenderSystemModelLocal::Modellist_f, "description" );
    cmdSystem->AddCommand( "modelist", ModeList_f, "description" );
    cmdSystem->AddCommand( "screenshot", ScreenShot_f, "description" );
    cmdSystem->AddCommand( "screenshotJPEG", ScreenShotJPEG_f, "description" );
    cmdSystem->AddCommand( "gfxinfo", GfxInfo_f, "description" );
    //cmdSystem->AddCommand( "minimize", Minimize , "description");
    cmdSystem->AddCommand( "exportCubemaps", ExportCubemaps_f, "description" );
}

void idRenderSystemInitLocal::InitQueries( void )
{
    if ( !glRefConfig.occlusionQuery )
    {
        return;
    }
    
    if ( r_drawSunRays->integer )
    {
        qglGenQueries( ARRAY_LEN( tr.sunFlareQuery ), tr.sunFlareQuery );
    }
}

void idRenderSystemInitLocal::ShutDownQueries( void )
{
    if ( !glRefConfig.occlusionQuery )
    {
        return;
    }
    
    if ( r_drawSunRays->integer )
    {
        qglDeleteQueries( ARRAY_LEN( tr.sunFlareQuery ), tr.sunFlareQuery );
    }
}

/*
===============
idRenderSystemInitLocal::Init
===============
*/
void idRenderSystemInitLocal::Init( void )
{
    S32	i, err;
    U8* ptr = nullptr;
    
    clientMainSystem->RefPrintf( PRINT_ALL, "----- idRenderSystemInitLocal::Init -----\n" );
    
    // clear all our internal state
    ::memset( &tr, 0, sizeof( tr ) );
    ::memset( &backEnd, 0, sizeof( backEnd ) );
    ::memset( &tess, 0, sizeof( tess ) );
    
    if ( ( intptr_t )tess.xyz & 15 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "tess.xyz not 16 byte aligned\n" );
    }
    
    idRenderSystemNoiseLocal::NoiseInit();
    
    // init function tables
    for ( i = 0; i < FUNCTABLE_SIZE; i++ )
    {
        tr.sinTable[i] = sin( DEG2RAD( i * 360.0f / ( ( F32 )( FUNCTABLE_SIZE - 1 ) ) ) );
        tr.squareTable[i] = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
        tr.sawToothTable[i] = ( F32 )i / FUNCTABLE_SIZE;
        tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];
        
        if ( i < FUNCTABLE_SIZE / 2 )
        {
            if ( i < FUNCTABLE_SIZE / 4 )
            {
                tr.triangleTable[i] = ( F32 ) i / ( FUNCTABLE_SIZE / 4 );
            }
            else
            {
                tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
            }
        }
        else
        {
            tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
        }
    }
    
    idRenderSystemImageLocal::InitFogTable();
    
    Register();
    
    max_polys = r_maxpolys->integer;
    if ( max_polys < MAX_POLYS )
    {
        max_polys = MAX_POLYS;
    }
    
    max_polyverts = r_maxpolyverts->integer;
    if ( max_polyverts < MAX_POLYVERTS )
    {
        max_polyverts = MAX_POLYVERTS;
    }
    
    ptr = ( U8* )( memorySystem->Alloc( sizeof( *backEndData ) + sizeof( srfPoly_t ) * max_polys + sizeof( polyVert_t ) * max_polyverts, h_low ) );
    backEndData = ( backEndData_t* )ptr;
    backEndData->polys = ( srfPoly_t* )( reinterpret_cast<UTF8*>( ptr ) + sizeof( *backEndData ) );
    backEndData->polyVerts = ( polyVert_t* )( reinterpret_cast<UTF8*>( ptr ) + sizeof( *backEndData ) + sizeof( srfPoly_t ) * max_polys );
    
    idRenderSystemSceneLocal::InitNextFrame();
    
    InitOpenGL();
    
    idRenderSystemImageLocal::InitImages();
    
    if ( glRefConfig.framebufferObject )
    {
        idRenderSystemFBOLocal::Init();
    }
    
    //Init GLSL
    idRenderSystemGLSLLocal::InitGPUShaders();
    
    idRenderSystemVaoLocal::InitVaos();
    idRenderSystemShaderLocal::InitShaders();
    
    idRenderSystemSkinsLocal::InitSkins();
    
    idRenderSystemModelLocal::ModelInit();
    
    idRenderSystemFontLocal::InitFreeType();
    
    InitQueries();
    
    err = qglGetError();
    if ( err != GL_NO_ERROR )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "glGetError() = 0x%x\n", err );
    }
    
    // print info
    GfxInfo_f();
    
    clientMainSystem->RefPrintf( PRINT_ALL, "----- finished idRenderSystemLocal::Init -----\n" );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown( bool destroyWindow )
{
    clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemLocal::Shutdown( %i )\n", destroyWindow );
    
    cmdSystem->RemoveCommand( "exportCubemaps" );
    cmdSystem->RemoveCommand( "gfxinfo" );
    cmdSystem->RemoveCommand( "gfxmeminfo" );
    cmdSystem->RemoveCommand( "imagelist" );
    cmdSystem->RemoveCommand( "minimize" );
    cmdSystem->RemoveCommand( "modellist" );
    cmdSystem->RemoveCommand( "screenshot" );
    cmdSystem->RemoveCommand( "screenshotJPEG" );
    cmdSystem->RemoveCommand( "shaderlist" );
    cmdSystem->RemoveCommand( "skinlist" );
    cmdSystem->RemoveCommand( "modellist" );
    cmdSystem->RemoveCommand( "modelist" );
    
    if ( tr.registered )
    {
        idRenderSystemCmdsLocal::IssuePendingRenderCommands();
        idRenderSystemInitLocal::ShutDownQueries();
        
        if ( glRefConfig.framebufferObject )
        {
            idRenderSystemFBOLocal::Shutdown();
        }
        
        idRenderSystemImageLocal::DeleteTextures();
        
        idRenderSystemVaoLocal::ShutdownVaos();
        
        idRenderSystemGLSLLocal::ShutdownGPUShaders();
    }
    
    idRenderSystemFontLocal::DoneFreeType();
    
    // shut down platform specific OpenGL stuff
    if ( destroyWindow )
    {
        idRenderSystemGlimpLocal::Shutdown();
        
        ::memset( &glConfig, 0, sizeof( glConfig ) );
        ::memset( &glState, 0, sizeof( glState ) );
        ::memset( &glRefConfig, 0, sizeof( glRefConfig ) );
        
        textureFilterAnisotropic = false;
        maxAnisotropy = 0;
        displayAspect = 0.0f;
        
        // wipe all render alloc'd zone memory
        clientMainSystem->RefTagFree();
    }
    
    tr.registered = false;
}

/*
=============
idRenderSystemLocal::EndRegistration

Touch all images to make sure they are resident
=============
*/
void idRenderSystemLocal::EndRegistration( void )
{
    idRenderSystemCmdsLocal::IssuePendingRenderCommands();
    
    if ( !idsystem->LowPhysicalMemory() )
    {
        idRenderSystemBackendLocal::ShowImages();
    }
}

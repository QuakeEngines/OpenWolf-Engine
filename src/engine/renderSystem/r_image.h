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
// File name:   r_image.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_IMAGE_H__
#define __R_IMAGE_H__

#pragma once

#define IMAGE_FILE_HASH_SIZE 1024
#define COPYSAMPLE(a,b) *(U32 *)(a) = *(U32 *)(b)
// size must be even
#define WORKBLOCK_SIZE     128
#define WORKBLOCK_BORDER   4
#define WORKBLOCK_REALSIZE (WORKBLOCK_SIZE + WORKBLOCK_BORDER * 2)
#define	DLIGHT_SIZE	16
#define	FOG_S	256
#define	FOG_T	32
#define	LUT_WIDTH	128
#define	LUT_HEIGHT	128
#define	DEFAULT_SIZE	16
#define COPYSAMPLE(a,b) *(U32 *)(a) = *(U32 *)(b)
// size must be even
#define WORKBLOCK_SIZE     128
#define WORKBLOCK_BORDER   4
#define WORKBLOCK_REALSIZE (WORKBLOCK_SIZE + WORKBLOCK_BORDER * 2)


typedef enum
{
    IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
    IMGTYPE_NORMAL,
    IMGTYPE_NORMALHEIGHT,
    IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
    IMGFLAG_NONE = 0x0000,
    IMGFLAG_MIPMAP = 0x0001,
    IMGFLAG_PICMIP = 0x0002,
    IMGFLAG_CUBEMAP = 0x0004,
    IMGFLAG_NO_COMPRESSION = 0x0010,
    IMGFLAG_NOLIGHTSCALE = 0x0020,
    IMGFLAG_CLAMPTOEDGE = 0x0040,
    IMGFLAG_GENNORMALMAP = 0x0100,
    IMGFLAG_MUTABLE = 0x0200,
    IMGFLAG_SRGB = 0x0400,
} imgFlags_t;

typedef struct image_s
{
    UTF8		imgName[MAX_QPATH];		// game path, including extension
    S32			width, height;				// source image
    S32			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
    U32			texnum;					// gl texture binding
    
    S32			frameUsed;			// for texture usage in frame statistics
    
    S32			internalFormat;
    S32			TMU;				// only needed for voodoo2
    
    imgType_t   type;
    S32 /*imgFlags_t*/  flags;
    
    struct image_s* next;
} image_t;

//
// idRenderSystemImageLocal
//
class idRenderSystemImageLocal
{
public:
    idRenderSystemImageLocal();
    ~idRenderSystemImageLocal();
    
    static S64 generateHashValue( StringEntry fname );
    static void TextureMode( StringEntry string );
    static S32 SumOfUsedImages( void );
    static void ImageList_f( void );
    static void ResampleTexture( U8* in, S32 inwidth, S32 inheight, U8* out, S32 outwidth, S32 outheight );
    static void RGBAtoYCoCgA( const U8* in, U8* out, S32 width, S32 height );
    static void YCoCgAtoRGBA( const U8* in, U8* out, S32 width, S32 height );
    static void RGBAtoNormal( const U8* in, U8* out, S32 width, S32 height, bool clampToEdge );
    static void DoFCBI( U8* in, U8* out, S32 width, S32 height, S32 component );
    static void DoFCBIQuick( U8* in, U8* out, S32 width, S32 height, S32 component );
    static void DoLinear( U8* in, U8* out, S32 width, S32 height );
    static void ExpandHalfTextureToGrid( U8* data, S32 width, S32 height );
    static void FillInNormalizedZ( const U8* in, U8* out, S32 width, S32 height );
    static void FCBIByBlock( U8* data, S32 width, S32 height, bool clampToEdge, bool normalized );
    static void LightScaleTexture( U8* in, S32 inwidth, S32 inheight, bool only_gamma );
    static void MipMapsRGB( U8* in, S32 inWidth, S32 inHeight );
    static void MipMapNormalHeight( const U8* in, U8* out, S32 width, S32 height, bool swizzle );
    static void BlendOverTexture( U8* data, S32 pixelCount, U8 blend[4] );
    static void RawImage_SwizzleRA( U8* data, S32 width, S32 height );
    static bool RawImage_ScaleToPower2( U8** data, S32* inout_width, S32* inout_height, imgType_t type, S32 flags, U8** resampledBuffer );
    static bool RawImage_HasAlpha( const U8* scan, S32 numPixels );
    static U32 RawImage_GetFormat( const U8* data, S32 numPixels, U32 picFormat, bool lightMap, imgType_t type, S32 flags );
    static void CompressMonoBlock( U8 outdata[8], const U8 indata[16] );
    static void RawImage_UploadToRgtc2Texture( U32 texture, S32 miplevel, S32 x, S32 y, S32 width, S32 height, U8* data );
    static S32 CalculateMipSize( S32 width, S32 height, U32 picFormat );
    static U32 PixelDataFormatFromInternalFormat( U32 internalFormat );
    static void RawImage_UploadTexture( U32 texture, U8* data, S32 x, S32 y, S32 width, S32 height, U32 target, U32 picFormat, S32 numMips, U32 internalFormat,
                                        imgType_t type, S32 flags, bool subtexture );
    static void Upload32( U8* data, S32 x, S32 y, S32 width, S32 height, U32 picFormat, S32 numMips, image_t* image, bool scaled );
    static image_t* CreateImage2( StringEntry name, U8* pic, S32 width, S32 height, U32 picFormat, S32 numMips, imgType_t type, S32 flags, S32 internalFormat );
    static image_t* CreateImage( StringEntry name, U8* pic, S32 width, S32 height, imgType_t type, S32 flags, S32 internalFormat );
    static void UpdateSubImage( image_t* image, U8* pic, S32 x, S32 y, S32 width, S32 height, U32 picFormat );
    static void idLoadImage( StringEntry name, U8** pic, S32* width, S32* height, U32* picFormat, S32* numMips );
    static image_t* FindImageFile( StringEntry name, imgType_t type, S32 flags );
    static void CreateDlightImage( void );
    static void InitFogTable( void );
    static F32	FogFactor( F32 s, F32 t );
    static void CreateFogImage( void );
    static void CreateEnvBrdfLUT( void );
    static void CreateDefaultImage( void );
    static void CreateBuiltinImages( void );
    static void SetColorMappings( void );
    static void InitImages( void );
    static void DeleteTextures( void );
    static S32 NextPowerOfTwo( S32 in );
};

extern idRenderSystemImageLocal renderSystemImageParseLocal;

#endif //!__R_IMAGE_H__
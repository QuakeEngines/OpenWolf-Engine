////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2007 - 2008 Joerg Dietrich
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
// File name:   r_image_png.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_IMAGE_PNG_H__
#define __R_IMAGE_PNG_H__

#pragma once

#define Q3IMAGE_BYTESPERPIXEL (4)

#define PNG_Signature "\x89\x50\x4E\x47\xD\xA\x1A\xA"
#define PNG_Signature_Size (8)

struct PNG_ChunkHeader
{
    U32 Length;
    U32 Type;
};

#define PNG_ChunkHeader_Size (8)

typedef U32 PNG_ChunkCRC;

#define PNG_ChunkCRC_Size (4)

#define MAKE_CHUNKTYPE(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d)))

#define PNG_ChunkType_IHDR MAKE_CHUNKTYPE('I', 'H', 'D', 'R')
#define PNG_ChunkType_PLTE MAKE_CHUNKTYPE('P', 'L', 'T', 'E')
#define PNG_ChunkType_IDAT MAKE_CHUNKTYPE('I', 'D', 'A', 'T')
#define PNG_ChunkType_IEND MAKE_CHUNKTYPE('I', 'E', 'N', 'D')
#define PNG_ChunkType_tRNS MAKE_CHUNKTYPE('t', 'R', 'N', 'S')


struct PNG_Chunk_IHDR
{
    U32 Width;
    U32 Height;
    U8  BitDepth;
    U8  ColourType;
    U8  CompressionMethod;
    U8  FilterMethod;
    U8  InterlaceMethod;
};

#define PNG_Chunk_IHDR_Size (13)


#define PNG_ColourType_Grey      (0)
#define PNG_ColourType_True      (2)
#define PNG_ColourType_Indexed   (3)
#define PNG_ColourType_GreyAlpha (4)
#define PNG_ColourType_TrueAlpha (6)

#define PNG_NumColourComponents_Grey      (1)
#define PNG_NumColourComponents_True      (3)
#define PNG_NumColourComponents_Indexed   (1)
#define PNG_NumColourComponents_GreyAlpha (2)
#define PNG_NumColourComponents_TrueAlpha (4)

#define PNG_BitDepth_1  ( 1)
#define PNG_BitDepth_2  ( 2)
#define PNG_BitDepth_4  ( 4)
#define PNG_BitDepth_8  ( 8)
#define PNG_BitDepth_16 (16)

#define PNG_CompressionMethod_0 (0)

#define PNG_FilterMethod_0 (0)

#define PNG_FilterType_None    (0)
#define PNG_FilterType_Sub     (1)
#define PNG_FilterType_Up      (2)
#define PNG_FilterType_Average (3)
#define PNG_FilterType_Paeth   (4)

#define PNG_InterlaceMethod_NonInterlaced (0)
#define PNG_InterlaceMethod_Interlaced    (1)


#define PNG_Adam7_NumPasses (7)

struct PNG_ZlibHeader
{
    U8 CompressionMethod;
    U8 Flags;
};

#define PNG_ZlibHeader_Size (2)

#define PNG_ZlibCheckValue_Size (4)

struct BufferedFile
{
    U8* Buffer;
    S32   Length;
    U8* Ptr;
    S32   BytesLeft;
};

//
// idRenderSystemImagePNGLocal
//
class idRenderSystemImagePNGLocal
{
public:
    idRenderSystemImagePNGLocal();
    ~idRenderSystemImagePNGLocal();
    
    static struct BufferedFile* ReadBufferedFile( StringEntry name );
    static void CloseBufferedFile( struct BufferedFile* BF );
    static void* BufferedFileRead( struct BufferedFile* BF, U32 Length );
    static bool BufferedFileRewind( struct BufferedFile* BF, U32 Offset );
    static bool BufferedFileSkip( struct BufferedFile* BF, U32 Offset );
    static bool FindChunk( struct BufferedFile* BF, U32 ChunkType );
    static U32 DecompressIDATs( struct BufferedFile* BF, U8** Buffer );
    static U8 PredictPaeth( U8 a, U8 b, U8 c );
    static bool UnfilterImage( U8* DecompressedData, U32  ImageHeight, U32  BytesPerScanline, U32  BytesPerPixel );
    static bool ConvertPixel( struct PNG_Chunk_IHDR* IHDR, U8* OutPtr, U8* DecompPtr, bool HasTransparentColour, U8* TransparentColour, U8* OutPal );
    static bool DecodeImageNonInterlaced( struct PNG_Chunk_IHDR* IHDR, U8* OutBuffer, U8* DecompressedData, U32 DecompressedDataLength,
                                          bool HasTransparentColour, U8* TransparentColour, U8* OutPal );
    static bool DecodeImageInterlaced( struct PNG_Chunk_IHDR* IHDR, U8* OutBuffer, U8* DecompressedData, U32 DecompressedDataLength,
                                       bool HasTransparentColour, U8* TransparentColour, U8* OutPal );
    static void LoadPNG( StringEntry name, U8** pic, S32* width, S32* height );
};

extern idRenderSystemImagePNGLocal renderSystemImagePNGLocal;

#endif //!__R_IMAGE_PNG_H__
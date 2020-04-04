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

#ifndef __R_IMAGE_JPG_H__
#define __R_IMAGE_JPG_H__

#pragma once

/* Catching errors, as done in libjpeg's example.c */
typedef struct q_jpeg_error_mgr_s
{
    struct jpeg_error_mgr pub;  /* "public" fields */
    
    jmp_buf setjmp_buffer;  /* for return to caller */
} q_jpeg_error_mgr_t;

typedef struct
{
    struct jpeg_destination_mgr pub; /* public fields */
    
    U8* outfile;		/* target stream */
    S32	size;
} my_destination_mgr;

typedef my_destination_mgr* my_dest_ptr;

//
// idRenderSystemImageDDSLocal
//
class idRenderSystemImageJPEGLocal
{
public:
    idRenderSystemImageJPEGLocal();
    ~idRenderSystemImageJPEGLocal();
    
    static void JPGErrorExit( j_common_ptr cinfo );
    static void JPGOutputMessage( j_common_ptr cinfo );
    static void LoadJPG( StringEntry filename, U8** pic, S32* width, S32* height );
    static void voidinit_destination( j_compress_ptr cinfo );
    static boolean empty_output_buffer( j_compress_ptr cinfo );
    static void term_destination( j_compress_ptr cinfo );
    static void jpegDest( j_compress_ptr cinfo, U8* outfile, S32 size );
    static size_t SaveJPGToBuffer( U8* buffer, U64 bufSize, S32 quality, S32 image_width, S32 image_height, U8* image_buffer, S32 padding );
    static void SaveJPG( UTF8* filename, S32 quality, S32 image_width, S32 image_height, U8* image_buffer, S32 padding );
};

extern idRenderSystemImageJPEGLocal renderSystemImageJPEGLocal;

#endif //!__R_IMAGE_DDS_H__
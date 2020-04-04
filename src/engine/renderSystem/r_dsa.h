////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2016 James Canete
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
// File name:   r_dsa.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_DSA_H__
#define __R_DSA_H__

#pragma once

static struct
{
    U32 textures[NUM_TEXTURE_BUNDLES];
    U32 texunit;
    
    U32 program;
    
    U32 drawFramebuffer;
    U32 readFramebuffer;
    U32 renderbuffer;
} glDsaState;

//
// idRenderSystemDSALocal
//
class idRenderSystemDSALocal
{
public:
    idRenderSystemDSALocal();
    ~idRenderSystemDSALocal();
    
    static void BindNullTextures( void );
    static S32 BindMultiTexture( U32 texunit, U32 target, U32 texture );
    static void BindMultiTextureEXT( U32 texunit, U32 target, U32 texture );
    static void TextureParameterfEXT( U32 texture, U32 target, U32 pname, F32 param );
    static void TextureParameteriEXT( U32 texture, U32 target, U32 pname, S32 param );
    static void TextureImage2DEXT( U32 texture, U32 target, S32 level, S32 internalformat, S32 width, S32 height, S32 border, U32 format, U32 type, const void* pixels );
    static void TextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 width, S32 height, U32 format, U32 type, const void* pixels );
    static void CopyTextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 x, S32 y, S32 width, S32 height );
    static void CompressedTextureImage2DEXT( U32 texture, U32 target, S32 level, U32 internalformat, S32 width, S32 height, S32 border, S32 imageSize, const void* data );
    static void CompressedTextureSubImage2DEXT( U32 texture, U32 target, S32 level, S32 xoffset, S32 yoffset, S32 width, S32 height, U32 format, S32 imageSize, const void* data );
    static void GenerateTextureMipmapEXT( U32 texture, U32 target );
    static void BindNullProgram( void );
    static S32 UseProgram( U32 program );
    static void ProgramUniform1iEXT( U32 program, S32 location, S32 v0 );
    static void ProgramUniform1fEXT( U32 program, S32 location, F32 v0 );
    static void ProgramUniform2fEXT( U32 program, S32 location, F32 v0, F32 v1 );
    static void ProgramUniform2fvEXT( U32 program, S32 location, S32 count, const F32* value );
    static void ProgramUniform3fEXT( U32 program, S32 location, F32 v0, F32 v1, F32 v2 );
    static void ProgramUniform4fEXT( U32 program, S32 location, F32 v0, F32 v1, F32 v2, F32 v3 );
    static void ProgramUniform1fvEXT( U32 program, S32 location, S32 count, const F32* value );
    static void ProgramUniformMatrix4fvEXT( U32 program, S32 location, S32 count, U8 transpose, const F32* value );
    static void ProgramUniform3fEXT( U32 program, S32 location, S32 count, const F32* value );
    static void BindNullFramebuffers( void );
    static void BindFramebuffer( U32 target, U32 framebuffer );
    static void BindRenderbuffer( U32 renderbuffer );
    static void BindFragDataLocation( U32 program, U32 color, StringEntry name );
    static void FramebufferTexture( U32 framebuffer, U32 attachment, U32 texture, S32 level );
    static void NamedRenderbufferStorageEXT( U32 renderbuffer, U32 internalformat, S32 width, S32 height );
    static void NamedRenderbufferStorageMultisampleEXT( U32 renderbuffer, S32 samples, U32 internalformat, S32 width, S32 height );
    static U32 CheckNamedFramebufferStatusEXT( U32 framebuffer, U32 target );
    static void NamedFramebufferTexture2DEXT( U32 framebuffer, U32 attachment, U32 textarget, U32 texture, S32 level );
    static void NamedFramebufferRenderbufferEXT( U32 framebuffer, U32 attachment, U32 renderbuffertarget, U32 renderbuffer );
    static void PatchParameteri( U32 pname, S32 value );
};

extern idRenderSystemDSALocal renderSystemDSALocal;

#endif

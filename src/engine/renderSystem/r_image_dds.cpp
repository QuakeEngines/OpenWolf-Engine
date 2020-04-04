////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2015 James Canete
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
// File name:   r_image_dds.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemImageDDSLocal renderSystemImageDDSLocal;

/*
===============
idRenderSystemImageDDSLocal::idRenderSystemImageDDSLocal
===============
*/
idRenderSystemImageDDSLocal::idRenderSystemImageDDSLocal( void )
{
}

/*
===============
idRenderSystemImageDDSLocal::~idRenderSystemImageDDSLocal
===============
*/
idRenderSystemImageDDSLocal::~idRenderSystemImageDDSLocal( void )
{
}

void idRenderSystemImageDDSLocal::LoadDDS( StringEntry filename, U8** pic, S32* width, S32* height, U32* picFormat, S32* numMips )
{
    union
    {
        U8* b;
        void* v;
    } buffer;
    S32 len;
    ddsHeader_t* ddsHeader = NULL;
    ddsHeaderDxt10_t* ddsHeaderDxt10 = NULL;
    U8* data;
    
    if ( !picFormat )
    {
        clientMainSystem->RefPrintf( PRINT_ERROR, "idRenderSystemImageDDSLocal::LoadDDS() called without picFormat parameter!" );
        return;
    }
    
    if ( width )
    {
        *width = 0;
    }
    
    if ( height )
    {
        *height = 0;
    }
    
    if ( picFormat )
    {
        *picFormat = GL_RGBA8;
    }
    
    if ( numMips )
    {
        *numMips = 1;
    }
    
    *pic = NULL;
    
    // load the file
    len = fileSystem->ReadFile( const_cast< UTF8* >( filename ), &buffer.v );
    if ( !buffer.b || len < 0 )
    {
        return;
    }
    
    // reject files that are too small to hold even a header
    if ( len < 4 + sizeof( *ddsHeader ) )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "File %s is too small to be a DDS file.\n", filename );
        fileSystem->FreeFile( buffer.v );
        return;
    }
    
    // reject files that don't start with "DDS "
    if ( *( ( U32* )( buffer.b ) ) != EncodeFourCC( "DDS " ) )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "File %s is not a DDS file.\n", filename );
        fileSystem->FreeFile( buffer.v );
        return;
    }
    
    // parse header and dx10 header if available
    ddsHeader = ( ddsHeader_t* )( buffer.b + 4 );
    if ( ( ddsHeader->pixelFormatFlags & DDSPF_FOURCC ) && ddsHeader->fourCC == EncodeFourCC( "DX10" ) )
    {
        if ( len < 4 + sizeof( *ddsHeader ) + sizeof( *ddsHeaderDxt10 ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "File %s indicates a DX10 header it is too small to contain.\n", filename );
            fileSystem->FreeFile( buffer.v );
            return;
        }
        
        ddsHeaderDxt10 = ( ddsHeaderDxt10_t* )( buffer.b + 4 + sizeof( ddsHeader_t ) );
        data = buffer.b + 4 + sizeof( *ddsHeader ) + sizeof( *ddsHeaderDxt10 );
        len -= 4 + sizeof( *ddsHeader ) + sizeof( *ddsHeaderDxt10 );
    }
    else
    {
        data = buffer.b + 4 + sizeof( *ddsHeader );
        len -= 4 + sizeof( *ddsHeader );
    }
    
    if ( width )
    {
        *width = ddsHeader->width;
    }
    
    if ( height )
    {
        *height = ddsHeader->height;
    }
    
    if ( numMips )
    {
        if ( ddsHeader->flags & _DDSFLAGS_MIPMAPCOUNT )
        {
            *numMips = ddsHeader->numMips;
        }
        else
        {
            *numMips = 1;
        }
    }
    
    // cubemap
    if ( ( ddsHeader->caps2 & DDSCAPS2_CUBEMAP ) == DDSCAPS2_CUBEMAP )
    {
        if ( ddsHeader->width != ddsHeader->height )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemImageDDSLocal::LoadDDS: invalid dds image \"%s\"\n", filename );
            fileSystem->FreeFile( buffer.v );
            return;
        }
        
        if ( width )
        {
            *width = ddsHeader->width;
        }
        
        if ( height )
        {
            *height = 0;
        }
        
        if ( *width & ( *width - 1 ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_LoadDDS: cubemap images must be power of two \"%s\"\n", filename );
            fileSystem->FreeFile( buffer.v );
            return;
        }
    }
    else
    {
        // 2D texture
        if ( width )
        {
            *width = ddsHeader->width;
        }
        
        if ( height )
        {
            *height = ddsHeader->height;
        }
        
        if ( ( *width & ( *width - 1 ) ) || ( *height & ( *height - 1 ) ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_LoadDDSImage: 2D texture images must be power of two \"%s\"\n", filename );
            fileSystem->FreeFile( buffer.v );
            return;
        }
    }
    
    // Convert DXGI format/FourCC into OpenGL format
    if ( ddsHeaderDxt10 )
    {
        switch ( ddsHeaderDxt10->dxgiFormat )
        {
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
                // FIXME: check for GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
                *picFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                break;
                
            case DXGI_FORMAT_BC1_UNORM_SRGB:
                // FIXME: check for GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
                *picFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
                break;
                
            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
                
            case DXGI_FORMAT_BC2_UNORM_SRGB:
                *picFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
                break;
                
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
                
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                *picFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                break;
                
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
                *picFormat = GL_COMPRESSED_RED_RGTC1;
                break;
                
            case DXGI_FORMAT_BC4_SNORM:
                *picFormat = GL_COMPRESSED_SIGNED_RED_RGTC1;
                break;
                
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
                *picFormat = GL_COMPRESSED_RG_RGTC2;
                break;
                
            case DXGI_FORMAT_BC5_SNORM:
                *picFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
                break;
                
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
                *picFormat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
                break;
                
            case DXGI_FORMAT_BC6H_SF16:
                *picFormat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
                break;
                
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
                *picFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                break;
                
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                *picFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
                break;
                
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                *picFormat = GL_SRGB8_ALPHA8_EXT;
                break;
                
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
                *picFormat = GL_RGBA8;
                break;
                
            default:
                clientMainSystem->RefPrintf( PRINT_ALL, "DDS File %s has unsupported DXGI format %d.", filename, ddsHeaderDxt10->dxgiFormat );
                fileSystem->FreeFile( buffer.v );
                return;
                break;
        }
    }
    else
    {
        if ( ddsHeader->pixelFormatFlags & DDSPF_FOURCC )
        {
            if ( ddsHeader->fourCC == EncodeFourCC( "DXT1" ) )
            {
                *picFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "DXT2" ) )
            {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "DXT3" ) )
            {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "DXT4" ) )
            {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "DXT5" ) )
            {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "ATI1" ) )
            {
                *picFormat = GL_COMPRESSED_RED_RGTC1;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "BC4U" ) )
            {
                *picFormat = GL_COMPRESSED_RED_RGTC1;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "BC4S" ) )
            {
                *picFormat = GL_COMPRESSED_SIGNED_RED_RGTC1;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "ATI2" ) )
            {
                *picFormat = GL_COMPRESSED_RG_RGTC2;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "BC5U" ) )
            {
                *picFormat = GL_COMPRESSED_RG_RGTC2;
            }
            else if ( ddsHeader->fourCC == EncodeFourCC( "BC5S" ) )
            {
                *picFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
            }
            else
            {
                clientMainSystem->RefPrintf( PRINT_ALL, "DDS File %s has unsupported FourCC.", filename );
                fileSystem->FreeFile( buffer.v );
                return;
            }
        }
        else if ( ddsHeader->pixelFormatFlags == ( DDSPF_RGB | DDSPF_ALPHAPIXELS ) && ddsHeader->rgbBitCount == 32 && ddsHeader->rBitMask == 0x000000ff &&
                  ddsHeader->gBitMask == 0x0000ff00 && ddsHeader->bBitMask == 0x00ff0000 && ddsHeader->aBitMask == 0xff000000 )
        {
            *picFormat = GL_RGBA8;
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "DDS File %s has unsupported RGBA format.", filename );
            fileSystem->FreeFile( buffer.v );
            return;
        }
    }
    
    *pic = ( U8* )memorySystem->Malloc( len );
    ::memcpy( *pic, data, len );
    
    fileSystem->FreeFile( buffer.v );
}

void idRenderSystemImageDDSLocal::SaveDDS( StringEntry filename, U8* pic, S32 width, S32 height, S32 depth )
{
    S32 picSize, size;
    U8* data;
    ddsHeader_t* ddsHeader;
    
    if ( !depth )
    {
        depth = 1;
    }
    
    picSize = width * height * depth * 4;
    size = 4 + sizeof( *ddsHeader ) + picSize;
    data = ( U8* )memorySystem->Malloc( size );
    
    data[0] = 'D';
    data[1] = 'D';
    data[2] = 'S';
    data[3] = ' ';
    
    ddsHeader = ( ddsHeader_t* )( data + 4 );
    ::memset( ddsHeader, 0, sizeof( ddsHeader_t ) );
    
    ddsHeader->headerSize = 0x7c;
    ddsHeader->flags = _DDSFLAGS_REQUIRED;
    ddsHeader->height = height;
    ddsHeader->width = width;
    ddsHeader->always_0x00000020 = 0x00000020;
    ddsHeader->caps = DDSCAPS_COMPLEX | DDSCAPS_REQUIRED;
    
    if ( depth == 6 )
    {
        ddsHeader->caps2 = DDSCAPS2_CUBEMAP;
    }
    
    ddsHeader->pixelFormatFlags = DDSPF_RGB | DDSPF_ALPHAPIXELS;
    ddsHeader->rgbBitCount = 32;
    ddsHeader->rBitMask = 0x000000ff;
    ddsHeader->gBitMask = 0x0000ff00;
    ddsHeader->bBitMask = 0x00ff0000;
    ddsHeader->aBitMask = 0xff000000;
    
    ::memcpy( data + 4 + sizeof( *ddsHeader ), pic, picSize );
    
    fileSystem->WriteFile( filename, data, size );
    
    memorySystem->Free( data );
}

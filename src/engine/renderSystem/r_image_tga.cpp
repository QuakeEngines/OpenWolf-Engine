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
// File name:   r_image_tga.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemImageTGALocal renderSystemImageTGALocal;

/*
===============
idRenderSystemImageTGALocal::idRenderSystemImageTGALocal
===============
*/
idRenderSystemImageTGALocal::idRenderSystemImageTGALocal( void )
{
}

/*
===============
idRenderSystemImageTGALocal::~idRenderSystemImageTGALocal
===============
*/
idRenderSystemImageTGALocal::~idRenderSystemImageTGALocal( void )
{
}

/*
========================================================================
TGA files are used for 24/32 bit images
========================================================================
*/
void idRenderSystemImageTGALocal::LoadTGA( StringEntry name, U8** pic, S32* width, S32* height )
{
    S32 row;
    U32 column, columns, rows, numPixels;
    U8* pixbuf, * buf_p, * end;
    union
    {
        U8* b;
        void* v;
    } buffer;
    TargaHeader	targa_header;
    U8* targa_rgba;
    S32 length;
    
    *pic = NULL;
    
    if ( width )
    {
        *width = 0;
    }
    
    if ( height )
    {
        *height = 0;
    }
    
    // load the file
    length = fileSystem->ReadFile( const_cast<UTF8*>( name ), &buffer.v );
    if ( !buffer.b || length < 0 )
    {
        return;
    }
    
    if ( length < 18 )
    {
        Com_Error( ERR_DROP, "LoadTGA: header too short (%s)", name );
    }
    
    buf_p = buffer.b;
    end = buffer.b + length;
    
    targa_header.id_length = buf_p[0];
    targa_header.colormap_type = buf_p[1];
    targa_header.image_type = buf_p[2];
    
    memcpy( &targa_header.colormap_index, &buf_p[3], 2 );
    memcpy( &targa_header.colormap_length, &buf_p[5], 2 );
    targa_header.colormap_size = buf_p[7];
    memcpy( &targa_header.x_origin, &buf_p[8], 2 );
    memcpy( &targa_header.y_origin, &buf_p[10], 2 );
    memcpy( &targa_header.width, &buf_p[12], 2 );
    memcpy( &targa_header.height, &buf_p[14], 2 );
    targa_header.pixel_size = buf_p[16];
    targa_header.attributes = buf_p[17];
    
    targa_header.colormap_index = LittleShort( targa_header.colormap_index );
    targa_header.colormap_length = LittleShort( targa_header.colormap_length );
    targa_header.x_origin = LittleShort( targa_header.x_origin );
    targa_header.y_origin = LittleShort( targa_header.y_origin );
    targa_header.width = LittleShort( targa_header.width );
    targa_header.height = LittleShort( targa_header.height );
    
    buf_p += 18;
    
    if ( targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3 )
    {
        Com_Error( ERR_DROP, "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported" );
    }
    
    if ( targa_header.colormap_type != 0 )
    {
        Com_Error( ERR_DROP, "LoadTGA: colormaps not supported" );
    }
    
    if ( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 )
    {
        Com_Error( ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)" );
    }
    
    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows * 4;
    
    if ( !columns || !rows || numPixels > 0x7FFFFFFF || numPixels / columns / 4 != rows )
    {
        Com_Error( ERR_DROP, "LoadTGA: %s has an invalid image size", name );
    }
    
    targa_rgba = ( U8* )memorySystem->Malloc( numPixels );
    
    if ( targa_header.id_length != 0 )
    {
        if ( buf_p + targa_header.id_length > end )
            Com_Error( ERR_DROP, "LoadTGA: header too short (%s)", name );
            
        buf_p += targa_header.id_length;  // skip TARGA image comment
    }
    
    if ( targa_header.image_type == 2 || targa_header.image_type == 3 )
    {
        if ( buf_p + columns * rows * targa_header.pixel_size / 8 > end )
        {
            Com_Error( ERR_DROP, "LoadTGA: file truncated (%s)", name );
        }
        
        // Uncompressed RGB or gray scale image
        for ( row = rows - 1; row >= 0; row-- )
        {
            pixbuf = targa_rgba + row * columns * 4;
            for ( column = 0; column < columns; column++ )
            {
                U8 red, green, blue, alphabyte;
                switch ( targa_header.pixel_size )
                {
                
                    case 8:
                        blue = *buf_p++;
                        green = blue;
                        red = blue;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = 255;
                        break;
                        
                    case 24:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = 255;
                        break;
                    case 32:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        alphabyte = *buf_p++;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        break;
                    default:
                        Com_Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'", targa_header.pixel_size, name );
                        break;
                }
            }
        }
    }
    // Runlength encoded RGB images
    else if ( targa_header.image_type == 10 )
    {
        U8 red, green, blue, alphabyte, packetHeader, packetSize, j;
        
        for ( row = rows - 1; row >= 0; row-- )
        {
            pixbuf = targa_rgba + row * columns * 4;
            for ( column = 0; column < columns; )
            {
                if ( buf_p + 1 > end )
                {
                    Com_Error( ERR_DROP, "LoadTGA: file truncated (%s)", name );
                }
                
                packetHeader = *buf_p++;
                packetSize = 1 + ( packetHeader & 0x7f );
                
                if ( packetHeader & 0x80 )        // run-length packet
                {
                    if ( buf_p + targa_header.pixel_size / 8 > end )
                    {
                        Com_Error( ERR_DROP, "LoadTGA: file truncated (%s)", name );
                    }
                    
                    switch ( targa_header.pixel_size )
                    {
                        case 24:
                            blue = *buf_p++;
                            green = *buf_p++;
                            red = *buf_p++;
                            alphabyte = 255;
                            break;
                        case 32:
                            blue = *buf_p++;
                            green = *buf_p++;
                            red = *buf_p++;
                            alphabyte = *buf_p++;
                            break;
                        default:
                            Com_Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'", targa_header.pixel_size, name );
                            break;
                    }
                    
                    for ( j = 0; j < packetSize; j++ )
                    {
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        column++;
                        
                        if ( column == columns ) // run spans across rows
                        {
                            column = 0;
                            if ( row > 0 )
                            {
                                row--;
                            }
                            else
                            {
                                goto breakOut;
                            }
                            
                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
                // non run-length packet
                else
                {
                
                    if ( buf_p + targa_header.pixel_size / 8 * packetSize > end )
                    {
                        Com_Error( ERR_DROP, "LoadTGA: file truncated (%s)", name );
                    }
                    
                    for ( j = 0; j < packetSize; j++ )
                    {
                        switch ( targa_header.pixel_size )
                        {
                            case 24:
                                blue = *buf_p++;
                                green = *buf_p++;
                                red = *buf_p++;
                                *pixbuf++ = red;
                                *pixbuf++ = green;
                                *pixbuf++ = blue;
                                *pixbuf++ = 255;
                                break;
                            case 32:
                                blue = *buf_p++;
                                green = *buf_p++;
                                red = *buf_p++;
                                alphabyte = *buf_p++;
                                *pixbuf++ = red;
                                *pixbuf++ = green;
                                *pixbuf++ = blue;
                                *pixbuf++ = alphabyte;
                                break;
                            default:
                                Com_Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'", targa_header.pixel_size, name );
                                break;
                        }
                        column++;
                        if ( column == columns ) // pixel packet run spans across rows
                        {
                            column = 0;
                            if ( row > 0 )
                                row--;
                            else
                                goto breakOut;
                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
            }
breakOut:
            ;
        }
    }
    
    // instead we just print a warning
    if ( targa_header.attributes & 0x20 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: '%s' TGA file header declares top-down image, ignoring\n", name );
    }
    
    if ( width )
    {
        *width = columns;
    }
    
    if ( height )
    {
        *height = rows;
    }
    
    *pic = targa_rgba;
    
    fileSystem->FreeFile( buffer.v );
}

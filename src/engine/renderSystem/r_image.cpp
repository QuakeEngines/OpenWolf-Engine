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

#include <renderSystem/r_precompiled.h>

static U8 s_intensitytable[256];
static U8 s_gammatable[256];

static S32 gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
static S32	gl_filter_max = GL_LINEAR;

static image_t* imageHashTable[IMAGE_FILE_HASH_SIZE];

struct textureMode_t
{
    StringEntry name;
    S32	minimize, maximize;
};

textureMode_t modes[] =
{
    {"GL_NEAREST", GL_NEAREST, GL_NEAREST},
    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

idRenderSystemImageLocal renderSystemImageLocal;

/*
===============
idRenderSystemImageLocal::idRenderSystemImageLocal
===============
*/
idRenderSystemImageLocal::idRenderSystemImageLocal( void )
{
}

/*
===============
idRenderSystemImageLocal::~idRenderSystemImageLocal
===============
*/
idRenderSystemImageLocal::~idRenderSystemImageLocal( void )
{
}

/*
================
idRenderSystemImageLocal::generateHashValue

return a hash value for the filename
================
*/
S64 idRenderSystemImageLocal::generateHashValue( StringEntry fname )
{
    S32 i = 0;
    S64	hash = 0;
    UTF8 letter;
    
    while ( fname[i] != '\0' )
    {
        letter = ::tolower( fname[i] );
        
        // don't include extension
        if ( letter == '.' )
        {
            break;
        }
        
        // damn path names
        if ( letter == '\\' )
        {
            letter = '/';
        }
        
        hash += ( S64 )( letter ) * ( i + 119 );
        i++;
    }
    
    hash &= ( IMAGE_FILE_HASH_SIZE - 1 );
    return hash;
}

/*
===============
idRenderSystemImageLocal::TextureMode
===============
*/
void idRenderSystemImageLocal::TextureMode( StringEntry string )
{
    S32	i;
    image_t* glt;
    
    for ( i = 0 ; i < 6 ; i++ )
    {
        if ( !Q_stricmp( modes[i].name, string ) )
        {
            break;
        }
    }
    
    // hack to prevent trilinear from being set on voodoo,
    // because their driver freaks...
    if ( i == 5 && glConfig.hardwareType == GLHW_3DFX_2D3D )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "Refusing to set trilinear on a voodoo.\n" );
        i = 3;
    }
    
    
    if ( i == 6 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "bad filter name\n" );
        return;
    }
    
    gl_filter_min = modes[i].minimize;
    gl_filter_max = modes[i].maximize;
    
    // change all the existing mipmap texture objects
    for ( i = 0 ; i < tr.numImages ; i++ )
    {
        glt = tr.images[ i ];
        
        if ( glt->flags & IMGFLAG_MIPMAP && !( glt->flags & IMGFLAG_CUBEMAP ) )
        {
            qglTextureParameterfEXT( glt->texnum, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ( F32 )gl_filter_min );
            qglTextureParameterfEXT( glt->texnum, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ( F32 )gl_filter_max );
        }
    }
}

/*
===============
idRenderSystemImageLocal::SumOfUsedImages
===============
*/
S32 idRenderSystemImageLocal::SumOfUsedImages( void )
{
    S32	i, total = 0;
    
    for ( i = 0; i < tr.numImages; i++ )
    {
        if ( tr.images[i]->frameUsed == tr.frameCount )
        {
            total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
        }
    }
    
    return total;
}

/*
===============
idRenderSystemImageLocal::ImageList_f
===============
*/
void idRenderSystemImageLocal::ImageList_f( void )
{
    S32 i, estTotalSize = 0;
    
    clientMainSystem->RefPrintf( PRINT_ALL, "\n      -w-- -h-- -type-- -size- --name-------\n" );
    
    for ( i = 0 ; i < tr.numImages ; i++ )
    {
        image_t* image = tr.images[i];
        StringEntry format = "????   ", sizeSuffix;
        S32 estSize, displaySize;
        
        estSize = image->uploadHeight * image->uploadWidth;
        
        switch ( image->internalFormat )
        {
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
                format = "sDXT1  ";
                // 64 bits per 16 pixels, so 4 bits per pixel
                estSize /= 2;
                break;
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
                format = "sDXT5  ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
                format = "sBPTC  ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;
            case GL_COMPRESSED_RG_RGTC2:
                format = "RGTC2  ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
                format = "DXT1   ";
                // 64 bits per 16 pixels, so 4 bits per pixel
                estSize /= 2;
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                format = "DXT1a  ";
                // 64 bits per 16 pixels, so 4 bits per pixel
                estSize /= 2;
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                format = "DXT5   ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;
            case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
                format = "BPTC   ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;
            case GL_RGB4_S3TC:
                format = "S3TC   ";
                // same as DXT1?
                estSize /= 2;
                break;
            case GL_RGBA16F:
                format = "RGBA16F";
                // 8 bytes per pixel
                estSize *= 8;
                break;
            case GL_RGBA16:
                format = "RGBA16 ";
                // 8 bytes per pixel
                estSize *= 8;
                break;
            case GL_RGBA4:
            case GL_RGBA8:
            case GL_RGBA:
                format = "RGBA   ";
                // 4 bytes per pixel
                estSize *= 4;
                break;
            case GL_LUMINANCE8:
            case GL_LUMINANCE:
                format = "L      ";
                // 1 byte per pixel?
                break;
            case GL_RGB5:
            case GL_RGB8:
            case GL_RGB:
                format = "RGB    ";
                // 3 bytes per pixel?
                estSize *= 3;
                break;
            case GL_LUMINANCE8_ALPHA8:
            case GL_LUMINANCE_ALPHA:
                format = "LA     ";
                // 2 bytes per pixel?
                estSize *= 2;
                break;
            case GL_SRGB_EXT:
            case GL_SRGB8_EXT:
                format = "sRGB   ";
                // 3 bytes per pixel?
                estSize *= 3;
                break;
            case GL_SRGB_ALPHA_EXT:
            case GL_SRGB8_ALPHA8_EXT:
                format = "sRGBA  ";
                // 4 bytes per pixel?
                estSize *= 4;
                break;
            case GL_SLUMINANCE_EXT:
            case GL_SLUMINANCE8_EXT:
                format = "sL     ";
                // 1 byte per pixel?
                break;
            case GL_SLUMINANCE_ALPHA_EXT:
            case GL_SLUMINANCE8_ALPHA8_EXT:
                format = "sLA    ";
                // 2 byte per pixel?
                estSize *= 2;
                break;
            case GL_DEPTH_COMPONENT16:
                format = "Depth16";
                // 2 bytes per pixel
                estSize *= 2;
                break;
            case GL_DEPTH_COMPONENT24:
                format = "Depth24";
                // 3 bytes per pixel
                estSize *= 3;
                break;
            case GL_DEPTH_COMPONENT:
            case GL_DEPTH_COMPONENT32:
                format = "Depth32";
                // 4 bytes per pixel
                estSize *= 4;
                break;
        }
        
        // mipmap adds about 50%
        if ( image->flags & IMGFLAG_MIPMAP )
        {
            estSize += estSize / 2;
        }
        
        sizeSuffix = "b ";
        displaySize = estSize;
        
        if ( displaySize > 1024 )
        {
            displaySize /= 1024;
            sizeSuffix = "kb";
        }
        
        if ( displaySize > 1024 )
        {
            displaySize /= 1024;
            sizeSuffix = "Mb";
        }
        
        if ( displaySize > 1024 )
        {
            displaySize /= 1024;
            sizeSuffix = "Gb";
        }
        
        clientMainSystem->RefPrintf( PRINT_ALL, "%4i: %4ix%4i %s %4i%s %s\n", i, image->uploadWidth, image->uploadHeight, format, displaySize, sizeSuffix, image->imgName );
        estTotalSize += estSize;
    }
    
    clientMainSystem->RefPrintf( PRINT_ALL, " ---------\n" );
    clientMainSystem->RefPrintf( PRINT_ALL, " approx %i bytes\n", estTotalSize );
    clientMainSystem->RefPrintf( PRINT_ALL, " %i total images\n\n", tr.numImages );
}

/*
================
idRenderSystemImageLocal::ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
void idRenderSystemImageLocal::ResampleTexture( U8* in, S32 inwidth, S32 inheight, U8* out, S32 outwidth, S32 outheight )
{
    S32	i, j, frac, fracstep, p1[2048], p2[2048];
    U8*	inrow, *inrow2, *pix1, *pix2, *pix3, *pix4;
    
    if ( outwidth > 2048 )
    {
        Com_Error( ERR_DROP, "idRenderSystemImageLocal::ResampleTexture: max width" );
    }
    
    fracstep = inwidth * 0x10000 / outwidth;
    
    frac = fracstep >> 2;
    
    for ( i = 0 ; i < outwidth ; i++ )
    {
        p1[i] = 4 * ( frac >> 16 );
        frac += fracstep;
    }
    
    frac = 3 * ( fracstep >> 2 );
    
    for ( i = 0 ; i < outwidth ; i++ )
    {
        p2[i] = 4 * ( frac >> 16 );
        frac += fracstep;
    }
    
    for ( i = 0 ; i < outheight ; i++ )
    {
        inrow = in + 4 * inwidth * ( S32 )( ( i + 0.25 ) * inheight / outheight );
        inrow2 = in + 4 * inwidth * ( S32 )( ( i + 0.75 ) * inheight / outheight );
        
        for ( j = 0 ; j < outwidth ; j++ )
        {
            pix1 = inrow + p1[j];
            pix2 = inrow + p2[j];
            pix3 = inrow2 + p1[j];
            pix4 = inrow2 + p2[j];
            *out++ = ( pix1[0] + pix2[0] + pix3[0] + pix4[0] ) >> 2;
            *out++ = ( pix1[1] + pix2[1] + pix3[1] + pix4[1] ) >> 2;
            *out++ = ( pix1[2] + pix2[2] + pix3[2] + pix4[2] ) >> 2;
            *out++ = ( pix1[3] + pix2[3] + pix3[3] + pix4[3] ) >> 2;
        }
    }
}

void idRenderSystemImageLocal::RGBAtoYCoCgA( const U8* in, U8* out, S32 width, S32 height )
{
    S32 x, y;
    
    for ( y = 0; y < height; y++ )
    {
        const U8* inbyte  = in  + y * width * 4;
        U8* outbyte = out + y * width * 4;
        
        for ( x = 0; x < width; x++ )
        {
            U8 r, g, b, a, rb2;
            
            r = *inbyte++;
            g = *inbyte++;
            b = *inbyte++;
            a = *inbyte++;
            rb2 = ( r + b ) >> 1;
            
            *outbyte++ = ( g + rb2 ) >> 1;     // Y  =  R/4 + G/2 + B/4
            *outbyte++ = ( r - b + 256 ) >> 1; // Co =  R/2       - B/2
            *outbyte++ = ( g - rb2 + 256 ) >> 1; // Cg = -R/4 + G/2 - B/4
            *outbyte++ = a;
        }
    }
}

void idRenderSystemImageLocal::YCoCgAtoRGBA( const U8* in, U8* out, S32 width, S32 height )
{
    S32 x, y;
    
    for ( y = 0; y < height; y++ )
    {
        const U8* inbyte  = in  + y * width * 4;
        U8* outbyte = out + y * width * 4;
        
        for ( x = 0; x < width; x++ )
        {
            U8 _Y, Co, Cg, a;
            
            _Y = *inbyte++;
            Co = *inbyte++;
            Cg = *inbyte++;
            a  = *inbyte++;
            
            // R = Y + Co - Cg
            *outbyte++ = CLAMP( _Y + Co - Cg, 0, 255 );
            // G = Y + Cg
            *outbyte++ = CLAMP( _Y      + Cg - 128, 0, 255 );
            // B = Y - Co - Cg
            *outbyte++ = CLAMP( _Y - Co - Cg + 256, 0, 255 );
            *outbyte++ = a;
        }
    }
}


// uses a sobel filter to change a texture to a normal map
void idRenderSystemImageLocal::RGBAtoNormal( const U8* in, U8* out, S32 width, S32 height, bool clampToEdge )
{
    S32 x, y, max = 1;
    
    // convert to heightmap, storing in alpha
    // same as converting to Y in YCoCg
    for ( y = 0; y < height; y++ )
    {
        const U8* inbyte  = in  + y * width * 4;
        U8* outbyte = out + y * width * 4 + 3;
        
        for ( x = 0; x < width; x++ )
        {
            U8 result = ( inbyte[0] >> 2 ) + ( inbyte[1] >> 1 ) + ( inbyte[2] >> 2 );
            // Make linear
            result = result * result / 255;
            *outbyte = result;
            max = MAX( max, *outbyte );
            outbyte += 4;
            inbyte  += 4;
        }
    }
    
    // level out heights
    if ( max < 255 )
    {
        for ( y = 0; y < height; y++ )
        {
            U8* outbyte = out + y * width * 4 + 3;
            
            for ( x = 0; x < width; x++ )
            {
                *outbyte = *outbyte + ( 255 - max );
                outbyte += 4;
            }
        }
    }
    
    // now run sobel filter over height values to generate X and Y
    // then normalize
    for ( y = 0; y < height; y++ )
    {
        U8* outbyte = out + y * width * 4;
        
        for ( x = 0; x < width; x++ )
        {
            // 0 1 2
            // 3 4 5
            // 6 7 8
            
            U8 s[9];
            S32 x2, y2, i;
            vec3_t normal;
            
            i = 0;
            for ( y2 = -1; y2 <= 1; y2++ )
            {
                S32 src_y = y + y2;
                
                if ( clampToEdge )
                {
                    src_y = CLAMP( src_y, 0, height - 1 );
                }
                else
                {
                    src_y = ( src_y + height ) % height;
                }
                
                
                for ( x2 = -1; x2 <= 1; x2++ )
                {
                    S32 src_x = x + x2;
                    
                    if ( clampToEdge )
                    {
                        src_x = CLAMP( src_x, 0, width - 1 );
                    }
                    else
                    {
                        src_x = ( src_x + width ) % width;
                    }
                    
                    s[i++] = *( out + ( src_y * width + src_x ) * 4 + 3 );
                }
            }
            
            normal[0] = ( F32 )( s[0] - s[2] + 2 * s[3] - 2 * s[5] + s[6] - s[8] );
            normal[1] = ( F32 )( s[0] + 2 * s[1] + s[2] - s[6] - 2 * s[7] - s[8] );
            normal[2] = ( F32 )( s[4] * 4 );
            
            if ( !VectorNormalize2( normal, normal ) )
            {
                VectorSet( normal, 0, 0, 1 );
            }
            
            *outbyte++ = FloatToOffsetByte( normal[0] );
            *outbyte++ = FloatToOffsetByte( normal[1] );
            *outbyte++ = FloatToOffsetByte( normal[2] );
            
            outbyte++;
        }
    }
}

// based on Fast Curve Based Interpolation
// from Fast Artifacts-Free Image Interpolation (http://www.andreagiachetti.it/icbi/)
// assumes data has a 2 pixel thick border of clamped or wrapped data
// expects data to be a grid with even (0, 0), (2, 0), (0, 2), (2, 2) etc pixels filled
// only performs FCBI on specified component
void idRenderSystemImageLocal::DoFCBI( U8* in, U8* out, S32 width, S32 height, S32 component )
{
    S32 x, y;
    U8* outbyte, *inbyte;
    
    // copy in to out
    for ( y = 2; y < height - 2; y += 2 )
    {
        inbyte  = in  + ( y * width + 2 ) * 4 + component;
        outbyte = out + ( y * width + 2 ) * 4 + component;
        
        for ( x = 2; x < width - 2; x += 2 )
        {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }
    
    for ( y = 3; y < height - 3; y += 2 )
    {
        // diagonals
        //
        // NWp  - northwest interpolated pixel
        // NEp  - northeast interpolated pixel
        // NWd  - northwest first derivative
        // NEd  - northeast first derivative
        // NWdd - northwest second derivative
        // NEdd - northeast second derivative
        //
        // Uses these samples:
        //
        //         0
        //   - - a - b - -
        //   - - - - - - -
        //   c - d - e - f
        // 0 - - - - - - -
        //   g - h - i - j
        //   - - - - - - -
        //   - - k - l - -
        //
        // x+2 uses these samples:
        //
        //         0
        //   - - - - a - b - -
        //   - - - - - - - - -
        //   - - c - d - e - f
        // 0 - - - - - - - - -
        //   - - g - h - i - j
        //   - - - - - - - - -
        //   - - - - k - l - -
        //
        // so we can reuse 8 of them on next iteration
        //
        // a=b, c=d, d=e, e=f, g=h, h=i, i=j, k=l
        //
        // only b, f, j, and l need to be sampled on next iteration
        
        U8 sa, sb, sc, sd, se, sf, sg, sh, si, sj, sk, sl;
        U8* line1, *line2, *line3, *line4;
        
        x = 3;
        
        // optimization one
        //                       SAMPLE2(sa, x-1, y-3);
        //SAMPLE2(sc, x-3, y-1); SAMPLE2(sd, x-1, y-1); SAMPLE2(se, x+1, y-1);
        //SAMPLE2(sg, x-3, y+1); SAMPLE2(sh, x-1, y+1); SAMPLE2(si, x+1, y+1);
        //                       SAMPLE2(sk, x-1, y+3);
        
        // optimization two
        line1 = in + ( ( y - 3 ) * width + ( x - 1 ) ) * 4 + component;
        line2 = in + ( ( y - 1 ) * width + ( x - 3 ) ) * 4 + component;
        line3 = in + ( ( y + 1 ) * width + ( x - 3 ) ) * 4 + component;
        line4 = in + ( ( y + 3 ) * width + ( x - 1 ) ) * 4 + component;
        
        //                                   COPYSAMPLE(sa, line1); line1 += 8;
        //COPYSAMPLE(sc, line2); line2 += 8; COPYSAMPLE(sd, line2); line2 += 8; COPYSAMPLE(se, line2); line2 += 8;
        //COPYSAMPLE(sg, line3); line3 += 8; COPYSAMPLE(sh, line3); line3 += 8; COPYSAMPLE(si, line3); line3 += 8;
        //                                   COPYSAMPLE(sk, line4); line4 += 8;
        
        sa = *line1;
        line1 += 8;
        sc = *line2;
        line2 += 8;
        sd = *line2;
        line2 += 8;
        se = *line2;
        line2 += 8;
        sg = *line3;
        line3 += 8;
        sh = *line3;
        line3 += 8;
        si = *line3;
        line3 += 8;
        sk = *line4;
        line4 += 8;
        
        outbyte = out + ( y * width + x ) * 4 + component;
        
        for ( ; x < width - 3; x += 2 )
        {
            S32 NWd, NEd, NWp, NEp;
            
            // original
            //                       SAMPLE2(sa, x-1, y-3); SAMPLE2(sb, x+1, y-3);
            //SAMPLE2(sc, x-3, y-1); SAMPLE2(sd, x-1, y-1); SAMPLE2(se, x+1, y-1); SAMPLE2(sf, x+3, y-1);
            //SAMPLE2(sg, x-3, y+1); SAMPLE2(sh, x-1, y+1); SAMPLE2(si, x+1, y+1); SAMPLE2(sj, x+3, y+1);
            //                       SAMPLE2(sk, x-1, y+3); SAMPLE2(sl, x+1, y+3);
            
            // optimization one
            //SAMPLE2(sb, x+1, y-3);
            //SAMPLE2(sf, x+3, y-1);
            //SAMPLE2(sj, x+3, y+1);
            //SAMPLE2(sl, x+1, y+3);
            
            // optimization two
            //COPYSAMPLE(sb, line1); line1 += 8;
            //COPYSAMPLE(sf, line2); line2 += 8;
            //COPYSAMPLE(sj, line3); line3 += 8;
            //COPYSAMPLE(sl, line4); line4 += 8;
            
            sb = *line1;
            line1 += 8;
            sf = *line2;
            line2 += 8;
            sj = *line3;
            line3 += 8;
            sl = *line4;
            line4 += 8;
            
            NWp = sd + si;
            NEp = se + sh;
            NWd = abs( sd - si );
            NEd = abs( se - sh );
            
            if ( NWd > 100 || NEd > 100 || abs( NWp - NEp ) > 200 )
            {
                if ( NWd < NEd )
                {
                    *outbyte = NWp >> 1;
                }
                else
                {
                    *outbyte = NEp >> 1;
                }
            }
            else
            {
                S32 NWdd, NEdd;
                
                //NEdd = abs(sg + sd + sb - 3 * (se + sh) + sk + si + sf);
                //NWdd = abs(sa + se + sj - 3 * (sd + si) + sc + sh + sl);
                NEdd = ::abs( sg + sb - 3 * NEp + sk + sf + NWp );
                NWdd = ::abs( sa + sj - 3 * NWp + sc + sl + NEp );
                
                if ( NWdd > NEdd )
                {
                    *outbyte = NWp >> 1;
                }
                else
                {
                    *outbyte = NEp >> 1;
                }
            }
            
            outbyte += 8;
            
            //                    COPYSAMPLE(sa, sb);
            //COPYSAMPLE(sc, sd); COPYSAMPLE(sd, se); COPYSAMPLE(se, sf);
            //COPYSAMPLE(sg, sh); COPYSAMPLE(sh, si); COPYSAMPLE(si, sj);
            //                    COPYSAMPLE(sk, sl);
            
            sa = sb;
            sc = sd;
            sd = se;
            se = sf;
            sg = sh;
            sh = si;
            si = sj;
            sk = sl;
        }
    }
    
    // hack: copy out to in again
    for ( y = 3; y < height - 3; y += 2 )
    {
        inbyte = out + ( y * width + 3 ) * 4 + component;
        outbyte = in + ( y * width + 3 ) * 4 + component;
        
        for ( x = 3; x < width - 3; x += 2 )
        {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }
    
    for ( y = 2; y < height - 3; y++ )
    {
        // horizontal & vertical
        //
        // hp  - horizontally interpolated pixel
        // vp  - vertically interpolated pixel
        // hd  - horizontal first derivative
        // vd  - vertical first derivative
        // hdd - horizontal second derivative
        // vdd - vertical second derivative
        // Uses these samples:
        //
        //       0
        //   - a - b -
        //   c - d - e
        // 0 - f - g -
        //   h - i - j
        //   - k - l -
        //
        // x+2 uses these samples:
        //
        //       0
        //   - - - a - b -
        //   - - c - d - e
        // 0 - - - f - g -
        //   - - h - i - j
        //   - - - k - l -
        //
        // so we can reuse 7 of them on next iteration
        //
        // a=b, c=d, d=e, f=g, h=i, i=j, k=l
        //
        // only b, e, g, j, and l need to be sampled on next iteration
        
        U8 sa, sb, sc, sd, se, sf, sg, sh, si, sj, sk, sl;
        U8* line1, *line2, *line3, *line4, *line5;
        
        //x = (y + 1) % 2;
        x = ( y + 1 ) % 2 + 2;
        
        // optimization one
        //            SAMPLE2(sa, x-1, y-2);
        //SAMPLE2(sc, x-2, y-1); SAMPLE2(sd, x,   y-1);
        //            SAMPLE2(sf, x-1, y  );
        //SAMPLE2(sh, x-2, y+1); SAMPLE2(si, x,   y+1);
        //            SAMPLE2(sk, x-1, y+2);
        
        line1 = in + ( ( y - 2 ) * width + ( x - 1 ) ) * 4 + component;
        line2 = in + ( ( y - 1 ) * width + ( x - 2 ) ) * 4 + component;
        line3 = in + ( ( y ) * width + ( x - 1 ) ) * 4 + component;
        line4 = in + ( ( y + 1 ) * width + ( x - 2 ) ) * 4 + component;
        line5 = in + ( ( y + 2 ) * width + ( x - 1 ) ) * 4 + component;
        
        //                 COPYSAMPLE(sa, line1); line1 += 8;
        //COPYSAMPLE(sc, line2); line2 += 8; COPYSAMPLE(sd, line2); line2 += 8;
        //                 COPYSAMPLE(sf, line3); line3 += 8;
        //COPYSAMPLE(sh, line4); line4 += 8; COPYSAMPLE(si, line4); line4 += 8;
        //                 COPYSAMPLE(sk, line5); line5 += 8;
        
        sa = *line1;
        line1 += 8;
        sc = *line2;
        line2 += 8;
        sd = *line2;
        line2 += 8;
        sf = *line3;
        line3 += 8;
        sh = *line4;
        line4 += 8;
        si = *line4;
        line4 += 8;
        sk = *line5;
        line5 += 8;
        
        outbyte = out + ( y * width + x ) * 4 + component;
        
        for ( ; x < width - 3; x += 2 )
        {
            S32 hd, vd, hp, vp;
            
            //            SAMPLE2(sa, x-1, y-2); SAMPLE2(sb, x+1, y-2);
            //SAMPLE2(sc, x-2, y-1); SAMPLE2(sd, x,   y-1); SAMPLE2(se, x+2, y-1);
            //            SAMPLE2(sf, x-1, y  ); SAMPLE2(sg, x+1, y  );
            //SAMPLE2(sh, x-2, y+1); SAMPLE2(si, x,   y+1); SAMPLE2(sj, x+2, y+1);
            //            SAMPLE2(sk, x-1, y+2); SAMPLE2(sl, x+1, y+2);
            
            // optimization one
            //SAMPLE2(sb, x+1, y-2);
            //SAMPLE2(se, x+2, y-1);
            //SAMPLE2(sg, x+1, y  );
            //SAMPLE2(sj, x+2, y+1);
            //SAMPLE2(sl, x+1, y+2);
            
            //COPYSAMPLE(sb, line1); line1 += 8;
            //COPYSAMPLE(se, line2); line2 += 8;
            //COPYSAMPLE(sg, line3); line3 += 8;
            //COPYSAMPLE(sj, line4); line4 += 8;
            //COPYSAMPLE(sl, line5); line5 += 8;
            
            sb = *line1;
            line1 += 8;
            se = *line2;
            line2 += 8;
            sg = *line3;
            line3 += 8;
            sj = *line4;
            line4 += 8;
            sl = *line5;
            line5 += 8;
            
            hp = sf + sg;
            vp = sd + si;
            hd = abs( sf - sg );
            vd = abs( sd - si );
            
            if ( hd > 100 || vd > 100 || abs( hp - vp ) > 200 )
            {
                if ( hd < vd )
                {
                    *outbyte = hp >> 1;
                }
                else
                {
                    *outbyte = vp >> 1;
                }
            }
            else
            {
                S32 hdd, vdd;
                
                //hdd = abs(sc[i] + sd[i] + se[i] - 3 * (sf[i] + sg[i]) + sh[i] + si[i] + sj[i]);
                //vdd = abs(sa[i] + sf[i] + sk[i] - 3 * (sd[i] + si[i]) + sb[i] + sg[i] + sl[i]);
                
                hdd = ::abs( sc + se - 3 * hp + sh + sj + vp );
                vdd = ::abs( sa + sk - 3 * vp + sb + sl + hp );
                
                if ( hdd > vdd )
                {
                    *outbyte = hp >> 1;
                }
                else
                {
                    *outbyte = vp >> 1;
                }
            }
            
            outbyte += 8;
            
            //          COPYSAMPLE(sa, sb);
            //COPYSAMPLE(sc, sd); COPYSAMPLE(sd, se);
            //          COPYSAMPLE(sf, sg);
            //COPYSAMPLE(sh, si); COPYSAMPLE(si, sj);
            //          COPYSAMPLE(sk, sl);
            sa = sb;
            sc = sd;
            sd = se;
            sf = sg;
            sh = si;
            si = sj;
            sk = sl;
        }
    }
}

// Similar to FCBI, but throws out the second order derivatives for speed
void idRenderSystemImageLocal::DoFCBIQuick( U8* in, U8* out, S32 width, S32 height, S32 component )
{
    S32 x, y;
    U8* outbyte, *inbyte;
    
    // copy in to out
    for ( y = 2; y < height - 2; y += 2 )
    {
        inbyte  = in  + ( y * width + 2 ) * 4 + component;
        outbyte = out + ( y * width + 2 ) * 4 + component;
        
        for ( x = 2; x < width - 2; x += 2 )
        {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }
    
    for ( y = 3; y < height - 4; y += 2 )
    {
        U8 sd, se, sh, si;
        U8* line2, *line3;
        
        x = 3;
        
        line2 = in + ( ( y - 1 ) * width + ( x - 1 ) ) * 4 + component;
        line3 = in + ( ( y + 1 ) * width + ( x - 1 ) ) * 4 + component;
        
        sd = *line2;
        line2 += 8;
        sh = *line3;
        line3 += 8;
        
        outbyte = out + ( y * width + x ) * 4 + component;
        
        for ( ; x < width - 4; x += 2 )
        {
            S32 NWd, NEd, NWp, NEp;
            
            se = *line2;
            line2 += 8;
            si = *line3;
            line3 += 8;
            
            NWp = sd + si;
            NEp = se + sh;
            NWd = abs( sd - si );
            NEd = abs( se - sh );
            
            if ( NWd < NEd )
            {
                *outbyte = NWp >> 1;
            }
            else
            {
                *outbyte = NEp >> 1;
            }
            
            outbyte += 8;
            
            sd = se;
            sh = si;
        }
    }
    
    // hack: copy out to in again
    for ( y = 3; y < height - 3; y += 2 )
    {
        inbyte  = out + ( y * width + 3 ) * 4 + component;
        outbyte = in  + ( y * width + 3 ) * 4 + component;
        
        for ( x = 3; x < width - 3; x += 2 )
        {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }
    
    for ( y = 2; y < height - 3; y++ )
    {
        U8 sd, sf, sg, si, * line2, *line3, *line4;
        
        x = ( y + 1 ) % 2 + 2;
        
        line2 = in + ( ( y - 1 ) * width + ( x ) ) * 4 + component;
        line3 = in + ( ( y ) * width + ( x - 1 ) ) * 4 + component;
        line4 = in + ( ( y + 1 ) * width + ( x ) ) * 4 + component;
        
        outbyte = out + ( y * width + x ) * 4 + component;
        
        sf = *line3;
        line3 += 8;
        
        for ( ; x < width - 3; x += 2 )
        {
            S32 hd, vd, hp, vp;
            
            sd = *line2;
            line2 += 8;
            sg = *line3;
            line3 += 8;
            si = *line4;
            line4 += 8;
            
            hp = sf + sg;
            vp = sd + si;
            hd = abs( sf - sg );
            vd = abs( sd - si );
            
            if ( hd < vd )
            {
                *outbyte = hp >> 1;
            }
            else
            {
                *outbyte = vp >> 1;
            }
            
            outbyte += 8;
            
            sf = sg;
        }
    }
}

// Similar to DoFCBIQuick, but just takes the average instead of checking derivatives
// as well, this operates on all four components
void idRenderSystemImageLocal::DoLinear( U8* in, U8* out, S32 width, S32 height )
{
    S32 x, y, i;
    U8* outbyte, *inbyte;
    
    // copy in to out
    for ( y = 2; y < height - 2; y += 2 )
    {
        x = 2;
        
        inbyte  = in  + ( y * width + x ) * 4;
        outbyte = out + ( y * width + x ) * 4;
        
        for ( ; x < width - 2; x += 2 )
        {
            COPYSAMPLE( outbyte, inbyte );
            outbyte += 8;
            inbyte += 8;
        }
    }
    
    for ( y = 1; y < height - 1; y += 2 )
    {
        U8 sd[4] = {0}, se[4] = {0}, sh[4] = {0}, si[4] = {0};
        U8* line2, *line3;
        
        x = 1;
        
        line2 = in + ( ( y - 1 ) * width + ( x - 1 ) ) * 4;
        line3 = in + ( ( y + 1 ) * width + ( x - 1 ) ) * 4;
        
        COPYSAMPLE( sd, line2 );
        line2 += 8;
        COPYSAMPLE( sh, line3 );
        line3 += 8;
        
        outbyte = out + ( y * width + x ) * 4;
        
        for ( ; x < width - 1; x += 2 )
        {
            COPYSAMPLE( se, line2 );
            line2 += 8;
            COPYSAMPLE( si, line3 );
            line3 += 8;
            
            for ( i = 0; i < 4; i++ )
            {
                *outbyte++ = ( sd[i] + si[i] + se[i] + sh[i] ) >> 2;
            }
            
            outbyte += 4;
            
            COPYSAMPLE( sd, se );
            COPYSAMPLE( sh, si );
        }
    }
    
    // hack: copy out to in again
    for ( y = 1; y < height - 1; y += 2 )
    {
        x = 1;
        
        inbyte  = out + ( y * width + x ) * 4;
        outbyte = in  + ( y * width + x ) * 4;
        
        for ( ; x < width - 1; x += 2 )
        {
            COPYSAMPLE( outbyte, inbyte );
            outbyte += 8;
            inbyte += 8;
        }
    }
    
    for ( y = 1; y < height - 1; y++ )
    {
        U8 sd[4], sf[4], sg[4], si[4];
        U8* line2, *line3, *line4;
        
        x = y % 2 + 1;
        
        line2 = in + ( ( y - 1 ) * width + ( x ) ) * 4;
        line3 = in + ( ( y ) * width + ( x - 1 ) ) * 4;
        line4 = in + ( ( y + 1 ) * width + ( x ) ) * 4;
        
        COPYSAMPLE( sf, line3 );
        line3 += 8;
        
        outbyte = out + ( y * width + x ) * 4;
        
        for ( ; x < width - 1; x += 2 )
        {
            COPYSAMPLE( sd, line2 );
            line2 += 8;
            COPYSAMPLE( sg, line3 );
            line3 += 8;
            COPYSAMPLE( si, line4 );
            line4 += 8;
            
            for ( i = 0; i < 4; i++ )
            {
                *outbyte++ = ( sf[i] + sg[i] + sd[i] + si[i] ) >> 2;
            }
            
            outbyte += 4;
            
            COPYSAMPLE( sf, sg );
        }
    }
}

void idRenderSystemImageLocal::ExpandHalfTextureToGrid( U8* data, S32 width, S32 height )
{
    S32 x, y;
    
    for ( y = height / 2; y > 0; y-- )
    {
        U8* outbyte = data + ( ( y * 2 - 1 ) * ( width )     - 2 ) * 4;
        U8* inbyte  = data + ( y * ( width / 2 ) - 1 ) * 4;
        
        for ( x = width / 2; x > 0; x-- )
        {
            COPYSAMPLE( outbyte, inbyte );
            
            outbyte -= 8;
            inbyte -= 4;
        }
    }
}

void idRenderSystemImageLocal::FillInNormalizedZ( const U8* in, U8* out, S32 width, S32 height )
{
    S32 x, y;
    
    for ( y = 0; y < height; y++ )
    {
        const U8* inbyte  = in  + y * width * 4;
        U8* outbyte = out + y * width * 4;
        
        for ( x = 0; x < width; x++ )
        {
            U8 nx, ny, nz, h;
            F32 fnx, fny, fll, fnz;
            
            nx = *inbyte++;
            ny = *inbyte++;
            inbyte++;
            h  = *inbyte++;
            
            fnx = OffsetByteToFloat( nx );
            fny = OffsetByteToFloat( ny );
            fll = 1.0f - fnx * fnx - fny * fny;
            
            if ( fll >= 0.0f )
            {
                fnz = ( F32 )sqrt( fll );
            }
            else
            {
                fnz = 0.0f;
            }
            
            nz = FloatToOffsetByte( fnz );
            
            *outbyte++ = nx;
            *outbyte++ = ny;
            *outbyte++ = nz;
            *outbyte++ = h;
        }
    }
}

// assumes that data has already been expanded into a 2x2 grid
void idRenderSystemImageLocal::FCBIByBlock( U8* data, S32 width, S32 height, bool clampToEdge, bool normalized )
{
    U8 workdata[WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4], outdata[WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4], * inbyte, *outbyte;
    S32 x, y, srcx, srcy;
    
    ExpandHalfTextureToGrid( data, width, height );
    
    for ( y = 0; y < height; y += WORKBLOCK_SIZE )
    {
        for ( x = 0; x < width; x += WORKBLOCK_SIZE )
        {
            S32 x2, y2, workwidth, workheight, fullworkwidth, fullworkheight;
            
            workwidth = MIN( WORKBLOCK_SIZE, width - x );
            workheight = MIN( WORKBLOCK_SIZE, height - y );
            
            fullworkwidth = workwidth + WORKBLOCK_BORDER * 2;
            fullworkheight = workheight + WORKBLOCK_BORDER * 2;
            
            //memset(workdata, 0, WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4);
            
            // fill in work block
            for ( y2 = 0; y2 < fullworkheight; y2 += 2 )
            {
                srcy = y + y2 - WORKBLOCK_BORDER;
                
                if ( clampToEdge )
                {
                    srcy = CLAMP( srcy, 0, height - 2 );
                }
                else
                {
                    srcy = ( srcy + height ) % height;
                }
                
                outbyte = workdata + y2 * fullworkwidth * 4;
                inbyte = data + srcy * width * 4;
                
                for ( x2 = 0; x2 < fullworkwidth; x2 += 2 )
                {
                    srcx = x + x2 - WORKBLOCK_BORDER;
                    
                    if ( clampToEdge )
                    {
                        srcx = CLAMP( srcx, 0, width - 2 );
                    }
                    else
                    {
                        srcx = ( srcx + width ) % width;
                    }
                    
                    COPYSAMPLE( outbyte, inbyte + srcx * 4 );
                    outbyte += 8;
                }
            }
            
            // submit work block
            DoLinear( workdata, outdata, fullworkwidth, fullworkheight );
            
            if ( !normalized )
            {
                switch ( r_imageUpsampleType->integer )
                {
                    case 0:
                        break;
                    case 1:
                        DoFCBIQuick( workdata, outdata, fullworkwidth, fullworkheight, 0 );
                        break;
                    case 2:
                    default:
                        DoFCBI( workdata, outdata, fullworkwidth, fullworkheight, 0 );
                        break;
                }
            }
            else
            {
                switch ( r_imageUpsampleType->integer )
                {
                    case 0:
                        break;
                    case 1:
                        DoFCBIQuick( workdata, outdata, fullworkwidth, fullworkheight, 0 );
                        DoFCBIQuick( workdata, outdata, fullworkwidth, fullworkheight, 1 );
                        break;
                    case 2:
                    default:
                        DoFCBI( workdata, outdata, fullworkwidth, fullworkheight, 0 );
                        DoFCBI( workdata, outdata, fullworkwidth, fullworkheight, 1 );
                        break;
                }
            }
            
            // copy back work block
            for ( y2 = 0; y2 < workheight; y2++ )
            {
                inbyte = outdata + ( ( y2 + WORKBLOCK_BORDER ) * fullworkwidth + WORKBLOCK_BORDER ) * 4;
                outbyte = data + ( ( y + y2 ) * width + x ) * 4;
                for ( x2 = 0; x2 < workwidth; x2++ )
                {
                    COPYSAMPLE( outbyte, inbyte );
                    outbyte += 4;
                    inbyte += 4;
                }
            }
        }
    }
}
#undef COPYSAMPLE

/*
================
idRenderSystemImageLocal::LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void idRenderSystemImageLocal::LightScaleTexture( U8* in, S32 inwidth, S32 inheight, bool only_gamma )
{
    if ( only_gamma )
    {
        return;
    }
    else
    {
        S32	i, c;
        U8*	p;
        
        p = in;
        
        c = inwidth * inheight;
        
        for ( i = 0; i < c; i++, p += 4 )
        {
            p[0] = s_intensitytable[p[0]];
            p[1] = s_intensitytable[p[1]];
            p[2] = s_intensitytable[p[2]];
        }
    }
}

/*
================
idRenderSystemImageLocal::MipMapsRGB

Operates in place, quartering the size of the texture
Colors are gamma correct
================
*/
void idRenderSystemImageLocal::MipMapsRGB( U8* in, S32 inWidth, S32 inHeight )
{
    S32 x, y, c, stride;
    F32 total;
    U8* out = in;
    const U8* in2;
    static F32 downmipSrgbLookup[256];
    static S32 downmipSrgbLookupSet = 0;
    
    if ( !downmipSrgbLookupSet )
    {
        for ( x = 0; x < 256; x++ )
        {
            downmipSrgbLookup[x] = powf( x / 255.0f, 2.2f ) * 0.25f;
        }
        
        downmipSrgbLookupSet = 1;
    }
    
    if ( inWidth == 1 && inHeight == 1 )
    {
        return;
    }
    
    if ( inWidth == 1 || inHeight == 1 )
    {
        for ( x = ( inWidth * inHeight ) >> 1; x; x-- )
        {
            for ( c = 3; c; c--, in++ )
            {
                total  = ( downmipSrgbLookup[*( in )] + downmipSrgbLookup[*( in + 4 )] ) * 2.0f;
                
                *out++ = ( U8 )( powf( total, 1.0f / 2.2f ) * 255.0f );
            }
            *out++ = ( *( in ) + * ( in + 4 ) ) >> 1;
            in += 5;
        }
        
        return;
    }
    
    stride = inWidth * 4;
    inWidth >>= 1;
    inHeight >>= 1;
    
    in2 = in + stride;
    for ( y = inHeight; y; y--, in += stride, in2 += stride )
    {
        for ( x = inWidth; x; x-- )
        {
            for ( c = 3; c; c--, in++, in2++ )
            {
                total = downmipSrgbLookup[*( in )]  + downmipSrgbLookup[*( in + 4 )]
                        + downmipSrgbLookup[*( in2 )] + downmipSrgbLookup[*( in2 + 4 )];
                        
                *out++ = ( U8 )( powf( total, 1.0f / 2.2f ) * 255.0f );
            }
            
            *out++ = ( *( in ) + * ( in + 4 ) + * ( in2 ) + * ( in2 + 4 ) ) >> 2;
            in += 5, in2 += 5;
        }
    }
}

void idRenderSystemImageLocal::MipMapNormalHeight( const U8* in, U8* out, S32 width, S32 height, bool swizzle )
{
    S32	i, j, row, sx = swizzle ? 3 : 0, sa = swizzle ? 0 : 3;
    
    if ( width == 1 && height == 1 )
    {
        return;
    }
    
    row = width * 4;
    width >>= 1;
    height >>= 1;
    
    for ( i = 0 ; i < height ; i++, in += row )
    {
        for ( j = 0 ; j < width ; j++, out += 4, in += 8 )
        {
            vec3_t v;
            
            v[0] =  OffsetByteToFloat( in[sx      ] );
            v[1] =  OffsetByteToFloat( in[       1] );
            v[2] =  OffsetByteToFloat( in[       2] );
            
            v[0] += OffsetByteToFloat( in[sx    + 4] );
            v[1] += OffsetByteToFloat( in[       5] );
            v[2] += OffsetByteToFloat( in[       6] );
            
            v[0] += OffsetByteToFloat( in[sx + row  ] );
            v[1] += OffsetByteToFloat( in[   row + 1] );
            v[2] += OffsetByteToFloat( in[   row + 2] );
            
            v[0] += OffsetByteToFloat( in[sx + row + 4] );
            v[1] += OffsetByteToFloat( in[   row + 5] );
            v[2] += OffsetByteToFloat( in[   row + 6] );
            
            VectorNormalizeFast( v );
            
            //v[0] *= 0.25f;
            //v[1] *= 0.25f;
            //v[2] = 1.0f - v[0] * v[0] - v[1] * v[1];
            //v[2] = sqrt(MAX(v[2], 0.0f));
            
            out[sx] = FloatToOffsetByte( v[0] );
            out[1 ] = FloatToOffsetByte( v[1] );
            out[2 ] = FloatToOffsetByte( v[2] );
            out[sa] = MAX( MAX( in[sa], in[sa + 4] ), MAX( in[sa + row], in[sa + row + 4] ) );
        }
    }
}

/*
==================
idRenderSystemImageLocal::BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
void idRenderSystemImageLocal::BlendOverTexture( U8* data, S32 pixelCount, U8 blend[4] )
{
    S32 i, inverseAlpha, premult[3];
    
    inverseAlpha = 255 - blend[3];
    premult[0] = blend[0] * blend[3];
    premult[1] = blend[1] * blend[3];
    premult[2] = blend[2] * blend[3];
    
    for ( i = 0 ; i < pixelCount ; i++, data += 4 )
    {
        data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
        data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
        data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
    }
}

U8	mipBlendColors[16][4] =
{
    {0, 0, 0, 0},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
};

void idRenderSystemImageLocal::RawImage_SwizzleRA( U8* data, S32 width, S32 height )
{
    S32 i;
    U8* ptr = data, swap;
    
    for ( i = 0; i < width * height; i++, ptr += 4 )
    {
        // swap red and alpha
        swap = ptr[0];
        ptr[0] = ptr[3];
        ptr[3] = swap;
    }
}

S32 idRenderSystemImageLocal::NextPowerOfTwo( S32 in )
{
    S32 out;
    
    for ( out = 1; out < in; out <<= 1 )
        ;
        
    return out;
}

/*
===============
RawImage_ScaleToPower2
===============
*/
bool idRenderSystemImageLocal::RawImage_ScaleToPower2( U8** data, S32* inout_width, S32* inout_height, imgType_t type, S32 flags, U8** resampledBuffer )
{
    S32 width = *inout_width, height = *inout_height, scaled_width, scaled_height;
    bool picmip = flags & IMGFLAG_PICMIP;
    bool mipmap = flags & IMGFLAG_MIPMAP;
    bool clampToEdge = flags & IMGFLAG_CLAMPTOEDGE;
    bool scaled;
    
    //
    // convert to exact power of 2 sizes
    //
    if ( !mipmap )
    {
        scaled_width = width;
        scaled_height = height;
    }
    else
    {
        scaled_width = NextPowerOfTwo( width );
        scaled_height = NextPowerOfTwo( height );
    }
    
    if ( r_roundImagesDown->integer && scaled_width > width )
        scaled_width >>= 1;
    if ( r_roundImagesDown->integer && scaled_height > height )
        scaled_height >>= 1;
        
    if ( picmip && data && resampledBuffer && r_imageUpsample->integer &&
            scaled_width < r_imageUpsampleMaxSize->integer && scaled_height < r_imageUpsampleMaxSize->integer )
    {
        S32 finalwidth, finalheight;
        //S32 startTime, endTime;
        
        //startTime = ri.Milliseconds();
        
        finalwidth = scaled_width << r_imageUpsample->integer;
        finalheight = scaled_height << r_imageUpsample->integer;
        
        while ( finalwidth > r_imageUpsampleMaxSize->integer
                || finalheight > r_imageUpsampleMaxSize->integer )
        {
            finalwidth >>= 1;
            finalheight >>= 1;
        }
        
        while ( finalwidth > glConfig.maxTextureSize
                || finalheight > glConfig.maxTextureSize )
        {
            finalwidth >>= 1;
            finalheight >>= 1;
        }
        
        *resampledBuffer = ( U8* )memorySystem->AllocateTempMemory( finalwidth * finalheight * 4 );
        
        if ( scaled_width != width || scaled_height != height )
            ResampleTexture( *data, width, height, *resampledBuffer, scaled_width, scaled_height );
        else
            ::memcpy( *resampledBuffer, *data, width * height * 4 );
            
        if ( type == IMGTYPE_COLORALPHA )
            RGBAtoYCoCgA( *resampledBuffer, *resampledBuffer, scaled_width, scaled_height );
            
        while ( scaled_width < finalwidth || scaled_height < finalheight )
        {
            scaled_width <<= 1;
            scaled_height <<= 1;
            
            FCBIByBlock( *resampledBuffer, scaled_width, scaled_height, clampToEdge, ( type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT ) );
        }
        
        if ( type == IMGTYPE_COLORALPHA )
            YCoCgAtoRGBA( *resampledBuffer, *resampledBuffer, scaled_width, scaled_height );
        else if ( type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT )
            FillInNormalizedZ( *resampledBuffer, *resampledBuffer, scaled_width, scaled_height );
            
        //endTime = ri.Milliseconds();
        
        //ri.Printf(PRINT_ALL, "upsampled %dx%d to %dx%d in %dms\n", width, height, scaled_width, scaled_height, endTime - startTime);
        
        *data = *resampledBuffer;
    }
    else if ( scaled_width != width || scaled_height != height )
    {
        if ( data && resampledBuffer )
        {
            *resampledBuffer = ( U8* )memorySystem->AllocateTempMemory( scaled_width * scaled_height * 4 );
            ResampleTexture( *data, width, height, *resampledBuffer, scaled_width, scaled_height );
            *data = *resampledBuffer;
        }
    }
    
    width = scaled_width;
    height = scaled_height;
    
    //
    // perform optional picmip operation
    //
    if ( picmip )
    {
        scaled_width >>= r_picmip->integer;
        scaled_height >>= r_picmip->integer;
    }
    
    //
    // clamp to the current upper OpenGL limit
    // scale both axis down equally so we don't have to
    // deal with a half mip resampling
    //
    while ( scaled_width > glConfig.maxTextureSize
            || scaled_height > glConfig.maxTextureSize )
    {
        scaled_width >>= 1;
        scaled_height >>= 1;
    }
    
    //
    // clamp to minimum size
    //
    scaled_width = MAX( 1, scaled_width );
    scaled_height = MAX( 1, scaled_height );
    
    scaled = ( width != scaled_width ) || ( height != scaled_height );
    
    //
    // rescale texture to new size using existing mipmap functions
    //
    if ( data )
    {
        while ( width > scaled_width || height > scaled_height )
        {
            if ( type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT )
                MipMapNormalHeight( *data, *data, width, height, false );
            else
                MipMapsRGB( *data, width, height );
                
            width = MAX( 1, width >> 1 );
            height = MAX( 1, height >> 1 );
        }
    }
    
    *inout_width = width;
    *inout_height = height;
    
    return scaled;
}


bool idRenderSystemImageLocal::RawImage_HasAlpha( const U8* scan, S32 numPixels )
{
    S32 i;
    
    if ( !scan )
    {
        return true;
    }
    
    for ( i = 0; i < numPixels; i++ )
    {
        if ( scan[i * 4 + 3] != 255 )
        {
            return true;
        }
    }
    
    return false;
}

U32 idRenderSystemImageLocal::RawImage_GetFormat( const U8* data, S32 numPixels, U32 picFormat, bool lightMap, imgType_t type, S32 flags )
{
    S32 samples = 3;
    U32 internalFormat = GL_RGB;
    bool forceNoCompression = ( flags & IMGFLAG_NO_COMPRESSION );
    bool normalmap = ( type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT );
    
    if ( picFormat != GL_RGBA8 )
        return picFormat;
        
    if ( normalmap )
    {
        if ( ( type == IMGTYPE_NORMALHEIGHT ) && RawImage_HasAlpha( data, numPixels ) && r_parallaxMapping->integer )
        {
            if ( !forceNoCompression && glRefConfig.textureCompression & TCR_BPTC )
            {
                internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            }
            else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
            {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            }
            else if ( r_texturebits->integer == 16 )
            {
                internalFormat = GL_RGBA4;
            }
            else if ( r_texturebits->integer == 32 )
            {
                internalFormat = GL_RGBA8;
            }
            else
            {
                internalFormat = GL_RGBA;
            }
        }
        else
        {
            if ( !forceNoCompression && glRefConfig.textureCompression & TCR_RGTC )
            {
                internalFormat = GL_COMPRESSED_RG_RGTC2;
            }
            else if ( !forceNoCompression && glRefConfig.textureCompression & TCR_BPTC )
            {
                internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            }
            else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
            {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            }
            else if ( r_texturebits->integer == 16 )
            {
                internalFormat = GL_RGB5;
            }
            else if ( r_texturebits->integer == 32 )
            {
                internalFormat = GL_RGB8;
            }
            else
            {
                internalFormat = GL_RGB;
            }
        }
    }
    else if ( lightMap )
    {
        if ( r_greyscale->integer )
            internalFormat = GL_LUMINANCE;
        else
            internalFormat = GL_RGBA;
    }
    else
    {
        if ( RawImage_HasAlpha( data, numPixels ) )
        {
            samples = 4;
        }
        
        // select proper internal format
        if ( samples == 3 )
        {
            if ( r_greyscale->integer )
            {
                if ( r_texturebits->integer == 16 || r_texturebits->integer == 32 )
                    internalFormat = GL_LUMINANCE8;
                else
                    internalFormat = GL_LUMINANCE;
            }
            else
            {
                if ( !forceNoCompression && ( glRefConfig.textureCompression & TCR_BPTC ) )
                {
                    internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
                {
                    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                }
                else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC )
                {
                    internalFormat = GL_RGB4_S3TC;
                }
                else if ( r_texturebits->integer == 16 )
                {
                    internalFormat = GL_RGB5;
                }
                else if ( r_texturebits->integer == 32 )
                {
                    internalFormat = GL_RGB8;
                }
                else
                {
                    internalFormat = GL_RGB;
                }
            }
        }
        else if ( samples == 4 )
        {
            if ( r_greyscale->integer )
            {
                if ( r_texturebits->integer == 16 || r_texturebits->integer == 32 )
                    internalFormat = GL_LUMINANCE8_ALPHA8;
                else
                    internalFormat = GL_LUMINANCE_ALPHA;
            }
            else
            {
                if ( !forceNoCompression && ( glRefConfig.textureCompression & TCR_BPTC ) )
                {
                    internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else if ( !forceNoCompression && glConfig.textureCompression == TC_S3TC_ARB )
                {
                    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                }
                else if ( r_texturebits->integer == 16 )
                {
                    internalFormat = GL_RGBA4;
                }
                else if ( r_texturebits->integer == 32 )
                {
                    internalFormat = GL_RGBA8;
                }
                else
                {
                    internalFormat = GL_RGBA;
                }
            }
        }
    }
    
    return internalFormat;
}

void idRenderSystemImageLocal::CompressMonoBlock( U8 outdata[8], const U8 indata[16] )
{
    S32 hi, lo, diff, bias, outbyte, shift, i;
    U8* p = outdata;
    
    hi = lo = indata[0];
    
    for ( i = 1; i < 16; i++ )
    {
        hi = MAX( indata[i], hi );
        lo = MIN( indata[i], lo );
    }
    
    *p++ = hi;
    *p++ = lo;
    
    diff = hi - lo;
    
    if ( diff == 0 )
    {
        outbyte = ( hi == 255 ) ? 255 : 0;
        
        for ( i = 0; i < 6; i++ )
        {
            *p++ = outbyte;
        }
        
        return;
    }
    
    bias = diff / 2 - lo * 7;
    outbyte = shift = 0;
    
    for ( i = 0; i < 16; i++ )
    {
        const U8 fixIndex[8] = { 1, 7, 6, 5, 4, 3, 2, 0 };
        U8 index = fixIndex[( indata[i] * 7 + bias ) / diff];
        
        outbyte |= index << shift;
        shift += 3;
        
        if ( shift >= 8 )
        {
            *p++ = outbyte & 0xff;
            shift -= 8;
            outbyte >>= 8;
        }
    }
}

void idRenderSystemImageLocal::RawImage_UploadToRgtc2Texture( U32 texture, S32 miplevel, S32 x, S32 y, S32 width, S32 height, U8* data )
{
    S32 wBlocks, hBlocks, iy, ix, size;
    U8* compressedData, *p = nullptr;
    
    wBlocks = ( width + 3 ) / 4;
    hBlocks = ( height + 3 ) / 4;
    size = wBlocks * hBlocks * 16;
    
    p = compressedData = ( U8* )memorySystem->AllocateTempMemory( size );
    for ( iy = 0; iy < height; iy += 4 )
    {
        S32 oh = MIN( 4, height - iy );
        
        for ( ix = 0; ix < width; ix += 4 )
        {
            U8 workingData[16];
            S32 component;
            
            S32 ow = MIN( 4, width - ix );
            
            for ( component = 0; component < 2; component++ )
            {
                S32 ox, oy;
                
                for ( oy = 0; oy < oh; oy++ )
                {
                    for ( ox = 0; ox < ow; ox++ )
                    {
                        workingData[oy * 4 + ox] = data[( ( iy + oy ) * width + ix + ox ) * 4 + component];
                    }
                }
                
                // dupe data to fill
                for ( oy = 0; oy < 4; oy++ )
                {
                    for ( ox = ( oy < oh ) ? ow : 0; ox < 4; ox++ )
                    {
                        workingData[oy * 4 + ox] = workingData[( oy % oh ) * 4 + ox % ow];
                    }
                }
                
                CompressMonoBlock( p, workingData );
                p += 8;
            }
        }
    }
    
    // FIXME: Won't work for x/y that aren't multiples of 4.
    qglCompressedTextureSubImage2DEXT( texture, GL_TEXTURE_2D, miplevel, x, y, width, height, GL_COMPRESSED_RG_RGTC2, size, compressedData );
    
    memorySystem->FreeTempMemory( compressedData );
}

S32 idRenderSystemImageLocal::CalculateMipSize( S32 width, S32 height, U32 picFormat )
{
    S32 numBlocks = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ), numPixels = width * height;
    
    switch ( picFormat )
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RED_RGTC1:
        case GL_COMPRESSED_SIGNED_RED_RGTC1:
            return numBlocks * 8;
            
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_RG_RGTC2:
        case GL_COMPRESSED_SIGNED_RG_RGTC2:
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
        case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
            return numBlocks * 16;
            
        case GL_RGBA8:
        case GL_SRGB8_ALPHA8_EXT:
            return numPixels * 4;
            
        case GL_RGBA16:
            return numPixels * 8;
            
        default:
            clientMainSystem->RefPrintf( PRINT_ALL, "Unsupported texture format %08x\n", picFormat );
            return 0;
    }
    
    return 0;
}

U32 idRenderSystemImageLocal::PixelDataFormatFromInternalFormat( U32 internalFormat )
{
    switch ( internalFormat )
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            return GL_DEPTH_COMPONENT;
        default:
            return GL_RGBA;
            break;
    }
}

void idRenderSystemImageLocal::RawImage_UploadTexture( U32 texture, U8* data, S32 x, S32 y, S32 width, S32 height, U32 target, U32 picFormat,
        S32 numMips, U32 internalFormat, imgType_t type, S32 flags, bool subtexture )
{
    U32 dataFormat, dataType;
    S32 size, miplevel;
    bool rgtc = internalFormat == GL_COMPRESSED_RG_RGTC2;
    bool rgba8 = picFormat == GL_RGBA8 || picFormat == GL_SRGB8_ALPHA8_EXT;
    bool rgba = rgba8 || picFormat == GL_RGBA16;
    bool mipmap = !!( flags & IMGFLAG_MIPMAP );
    bool lastMip = false;
    
    dataFormat = PixelDataFormatFromInternalFormat( internalFormat );
    dataType = picFormat == GL_RGBA16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
    
    miplevel = 0;
    do
    {
        lastMip = ( width == 1 && height == 1 ) || !mipmap;
        size = CalculateMipSize( width, height, picFormat );
        
        if ( !rgba )
        {
            qglCompressedTextureSubImage2DEXT( texture, target, miplevel, x, y, width, height, picFormat, size, data );
        }
        else
        {
            if ( rgba8 && miplevel != 0 && r_colorMipLevels->integer )
                BlendOverTexture( ( U8* )data, width * height, mipBlendColors[miplevel] );
                
            if ( rgba8 && rgtc )
                RawImage_UploadToRgtc2Texture( texture, miplevel, x, y, width, height, data );
            else
                qglTextureSubImage2DEXT( texture, target, miplevel, x, y, width, height, dataFormat, dataType, data );
        }
        
        if ( !lastMip && numMips < 2 )
        {
            if ( glRefConfig.framebufferObject )
            {
                qglGenerateTextureMipmapEXT( texture, target );
                break;
            }
            else if ( rgba8 )
            {
                if ( type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT )
                    MipMapNormalHeight( data, data, width, height, glRefConfig.swizzleNormalmap );
                else
                    MipMapsRGB( data, width, height );
            }
        }
        
        x >>= 1;
        y >>= 1;
        width = MAX( 1, width >> 1 );
        height = MAX( 1, height >> 1 );
        miplevel++;
        
        if ( numMips > 1 )
        {
            data += size;
            numMips--;
        }
    }
    while ( !lastMip );
}


/*
===============
idRenderSystemImageLocal::Upload32
===============
*/
void idRenderSystemImageLocal::Upload32( U8* data, S32 x, S32 y, S32 width, S32 height, U32 picFormat, S32 numMips, image_t* image, bool scaled )
{
    S32 i, c;
    U8*	scan;
    
    imgType_t type = image->type;
    S32 flags = image->flags;
    U32 internalFormat = image->internalFormat;
    bool rgba8 = picFormat == GL_RGBA8 || picFormat == GL_SRGB8_ALPHA8_EXT;
    bool mipmap = !!( flags & IMGFLAG_MIPMAP ) && ( rgba8 || numMips > 1 );
    bool cubemap = !!( flags & IMGFLAG_CUBEMAP );
    
    // These operations cannot be performed on non-rgba8 images.
    if ( rgba8 && !cubemap )
    {
        c = width * height;
        scan = data;
        
        if ( type == IMGTYPE_COLORALPHA )
        {
            if ( r_greyscale->integer )
            {
                for ( i = 0; i < c; i++ )
                {
                    U8 luma = ( U8 )( LUMA( scan[i * 4], scan[i * 4 + 1], scan[i * 4 + 2] ) );
                    scan[i * 4] = luma;
                    scan[i * 4 + 1] = luma;
                    scan[i * 4 + 2] = luma;
                }
            }
            else if ( r_greyscale->value )
            {
                for ( i = 0; i < c; i++ )
                {
                    F32 luma = LUMA( scan[i * 4], scan[i * 4 + 1], scan[i * 4 + 2] );
                    scan[i * 4] = ( U8 )( LERP( scan[i * 4], luma, r_greyscale->value ) );
                    scan[i * 4 + 1] = ( U8 )( LERP( scan[i * 4 + 1], luma, r_greyscale->value ) );
                    scan[i * 4 + 2] = ( U8 )( LERP( scan[i * 4 + 2], luma, r_greyscale->value ) );
                }
            }
            
            // This corresponds to what the OpenGL1 renderer does.
            if ( !( flags & IMGFLAG_NOLIGHTSCALE ) && ( scaled || mipmap ) )
            {
                LightScaleTexture( data, width, height, !mipmap );
            }
        }
        
        if ( glRefConfig.swizzleNormalmap && ( type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT ) )
        {
            RawImage_SwizzleRA( data, width, height );
        }
    }
    
    if ( cubemap )
    {
        for ( i = 0; i < 6; i++ )
        {
            S32 w2 = width, h2 = height;
            RawImage_UploadTexture( image->texnum, data, x, y, width, height, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, picFormat, numMips, internalFormat, type, flags, false );
            for ( c = numMips; c; c-- )
            {
                data += CalculateMipSize( w2, h2, picFormat );
                w2 = MAX( 1, w2 >> 1 );
                h2 = MAX( 1, h2 >> 1 );
            }
        }
    }
    else
    {
        RawImage_UploadTexture( image->texnum, data, x, y, width, height, GL_TEXTURE_2D, picFormat, numMips, internalFormat, type, flags, false );
    }
    
    idRenderSystemInitLocal::CheckErrors( __FILE__, __LINE__ );
}

/*
================
idRenderSystemImageLocal::CreateImage2

This is the only way any image_t are created
================
*/
image_t* idRenderSystemImageLocal::CreateImage2( StringEntry name, U8* pic, S32 width, S32 height, U32 picFormat, S32 numMips,
        imgType_t type, S32 flags, S32 internalFormat )
{
    U8* resampledBuffer = nullptr;
    image_t* image = nullptr;
    bool isLightmap = false, scaled = false;
    S64 hash;
    S32 glWrapClampMode, mipWidth, mipHeight, miplevel;
    bool rgba8 = picFormat == GL_RGBA8 || picFormat == GL_SRGB8_ALPHA8_EXT;
    bool mipmap = !!( flags & IMGFLAG_MIPMAP );
    bool cubemap = !!( flags & IMGFLAG_CUBEMAP );
    bool picmip = !!( flags & IMGFLAG_PICMIP );
    bool lastMip;
    U32 textureTarget = cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, dataFormat;
    
    if ( ::strlen( name ) >= MAX_QPATH )
    {
        Com_Error( ERR_DROP, "idRenderSystemImageLocal::CreateImage: \"%s\" is too long", name );
    }
    
    if ( !::strncmp( name, "*lightmap", 9 ) )
    {
        isLightmap = true;
    }
    
    if ( tr.numImages == MAX_DRAWIMAGES )
    {
        Com_Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit" );
        return nullptr;
    }
    
    image = tr.images[tr.numImages] = reinterpret_cast<image_t*>( memorySystem->Alloc( sizeof( image_t ), h_low ) );
    qglGenTextures( 1, &image->texnum );
    tr.numImages++;
    
    image->type = type;
    image->flags = flags;
    
    Q_strncpyz( image->imgName, name, sizeof( image->imgName ) );
    
    image->width = width;
    image->height = height;
    
    if ( flags & IMGFLAG_CLAMPTOEDGE )
    {
        glWrapClampMode = GL_CLAMP_TO_EDGE;
    }
    else
    {
        glWrapClampMode = GL_REPEAT;
    }
    
    if ( !internalFormat )
    {
        internalFormat = RawImage_GetFormat( pic, width * height, picFormat, isLightmap, image->type, image->flags );
    }
    
    image->internalFormat = internalFormat;
    
    // lightmaps are always allocated on TMU 1
    if ( isLightmap )
    {
        image->TMU = 1;
    }
    else
    {
        image->TMU = 0;
    }
    
    // Possibly scale image before uploading.
    // if not rgba8 and uploading an image, skip picmips.
    if ( !cubemap )
    {
        if ( rgba8 )
        {
            scaled = RawImage_ScaleToPower2( &pic, &width, &height, type, flags, &resampledBuffer );
        }
        else if ( pic && picmip )
        {
            for ( miplevel = r_picmip->integer; miplevel > 0 && numMips > 1; miplevel--, numMips-- )
            {
                S32 size = CalculateMipSize( width, height, picFormat );
                width = MAX( 1, width >> 1 );
                height = MAX( 1, height >> 1 );
                pic += size;
            }
        }
    }
    
    image->uploadWidth = width;
    image->uploadHeight = height;
    
    S32 format = GL_BGRA;
    if ( internalFormat == GL_DEPTH_COMPONENT24 )
    {
        format = GL_DEPTH_COMPONENT;
    }
    
    // Allocate texture storage so we don't have to worry about it later.
    dataFormat = PixelDataFormatFromInternalFormat( internalFormat );
    mipWidth = width;
    mipHeight = height;
    miplevel = 0;
    
    do
    {
        lastMip = !mipmap || ( mipWidth == 1 && mipHeight == 1 );
        if ( cubemap )
        {
            S32 i;
            
            for ( i = 0; i < 6; i++ )
            {
                qglTextureImage2DEXT( image->texnum, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, miplevel,
                                      internalFormat, mipWidth, mipHeight, 0, format, GL_UNSIGNED_BYTE, NULL );
            }
        }
        else
        {
            qglTextureImage2DEXT( image->texnum, GL_TEXTURE_2D, miplevel, internalFormat, mipWidth, mipHeight, 0, format, GL_UNSIGNED_BYTE, NULL );
        }
        
        mipWidth = MAX( 1U, mipWidth >> 1 );
        mipHeight = MAX( 1U, mipHeight >> 1 );
        miplevel++;
    }
    while ( !lastMip );
    
    // Upload data.
    if ( pic )
    {
        Upload32( pic, 0, 0, width, height, picFormat, numMips, image, scaled );
    }
    
    if ( resampledBuffer != NULL )
    {
        memorySystem->FreeTempMemory( resampledBuffer );
    }
    
    // Set all necessary texture parameters.
    qglTextureParameterfEXT( image->texnum, textureTarget, GL_TEXTURE_WRAP_S, ( F32 )glWrapClampMode );
    qglTextureParameterfEXT( image->texnum, textureTarget, GL_TEXTURE_WRAP_T, ( F32 )glWrapClampMode );
    
    if ( cubemap )
    {
        qglTextureParameteriEXT( image->texnum, textureTarget, GL_TEXTURE_WRAP_R, glWrapClampMode );
    }
    
    if ( glConfig.textureFilterAnisotropic && !cubemap )
    {
        qglTextureParameteriEXT( image->texnum, textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                 mipmap ? ( S32 )Com_Clamp( 1, ( F32 )glConfig.maxAnisotropy, r_ext_max_anisotropy->value ) : 1 );
    }
    
    switch ( internalFormat )
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16_ARB:
        case GL_DEPTH_COMPONENT24_ARB:
        case GL_DEPTH_COMPONENT32_ARB:
            // Fix for sampling depth buffer on old nVidia cards.
            // from http://www.idevgames.com/forums/thread-4141-post-34844.html#pid34844
            qglTextureParameterfEXT( image->texnum, textureTarget, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
            qglTextureParameterfEXT( image->texnum, textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
            qglTextureParameterfEXT( image->texnum, textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
            break;
        default:
            qglTextureParameterfEXT( image->texnum, textureTarget, GL_TEXTURE_MIN_FILTER, ( F32 )( mipmap ? gl_filter_min : GL_LINEAR ) );
            qglTextureParameterfEXT( image->texnum, textureTarget, GL_TEXTURE_MAG_FILTER, ( F32 )( mipmap ? gl_filter_max : GL_LINEAR ) );
            break;
    }
    
    idRenderSystemInitLocal::CheckErrors( __FILE__, __LINE__ );
    
    hash = generateHashValue( name );
    image->next = imageHashTable[hash];
    imageHashTable[hash] = image;
    
    return image;
}

/*
================
idRenderSystemImageLocal::CreateImage

Wrapper for R_CreateImage2(), for the old parameters.
================
*/
image_t* idRenderSystemImageLocal::CreateImage( StringEntry name, U8* pic, S32 width, S32 height, imgType_t type, S32 flags, S32 internalFormat )
{
    return CreateImage2( name, pic, width, height, GL_RGBA8, 0, type, flags, internalFormat );
}


void idRenderSystemImageLocal::UpdateSubImage( image_t* image, U8* pic, S32 x, S32 y, S32 width, S32 height, U32 picFormat )
{
    Upload32( pic, x, y, width, height, picFormat, 0, image, false );
}

typedef struct
{
    StringEntry ext;
    void ( *ImageLoader )( StringEntry, U8**, S32*, S32* );
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static imageExtToLoaderMap_t imageLoaders[ ] =
{
    { "png", idRenderSystemImagePNGLocal::LoadPNG },
    { "tga", idRenderSystemImageTGALocal::LoadTGA },
    { "jpg", idRenderSystemImageJPEGLocal::LoadJPG },
    { "jpeg", idRenderSystemImageJPEGLocal::LoadJPG }
};

/*
=================
idRenderSystemImageLocal::LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
static S32 numImageLoaders = ARRAY_LEN( imageLoaders );
void idRenderSystemImageLocal::idLoadImage( StringEntry name, U8** pic, S32* width, S32* height, U32* picFormat, S32* numMips )
{
    S32 i, orgLoader = -1;
    UTF8 localName[ MAX_QPATH ];
    StringEntry ext;
    StringEntry altName;
    bool orgNameFailed = false;
    
    *pic = NULL;
    *width = 0;
    *height = 0;
    *picFormat = GL_RGBA8;
    *numMips = 0;
    
    Q_strncpyz( localName, name, MAX_QPATH );
    
    ext = COM_GetExtension( localName );
    
    // If compressed textures are enabled, try loading a DDS first, it'll load fastest
    if ( r_ext_compressed_textures->integer )
    {
        UTF8 ddsName[MAX_QPATH];
        
        COM_StripExtension3( name, ddsName, MAX_QPATH );
        Q_strcat( ddsName, MAX_QPATH, ".dds" );
        
        idRenderSystemImageDDSLocal::LoadDDS( ddsName, pic, width, height, picFormat, numMips );
        
        // If loaded, we're done.
        if ( *pic )
        {
            return;
        }
    }
    
    if ( *ext )
    {
        // Look for the correct loader and use it
        for ( i = 0; i < numImageLoaders; i++ )
        {
            if ( !Q_stricmp( ext, imageLoaders[ i ].ext ) )
            {
                // Load
                imageLoaders[ i ].ImageLoader( localName, pic, width, height );
                break;
            }
        }
        
        // A loader was found
        if ( i < numImageLoaders )
        {
            if ( *pic == NULL )
            {
                // Loader failed, most likely because the file isn't there;
                // try again without the extension
                orgNameFailed = true;
                orgLoader = i;
                COM_StripExtension3( name, localName, MAX_QPATH );
            }
            else
            {
                // Something loaded
                return;
            }
        }
    }
    
    // Try and find a suitable match using all
    // the image formats supported
    for ( i = 0; i < numImageLoaders; i++ )
    {
        if ( i == orgLoader )
        {
            continue;
        }
        
        altName = va( "%s.%s", localName, imageLoaders[ i ].ext );
        
        // Load
        imageLoaders[ i ].ImageLoader( altName, pic, width, height );
        
        if ( *pic )
        {
            if ( orgNameFailed )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n", name, altName );
            }
            
            break;
        }
    }
}

/*
===============
idRenderSystemImageLocal::FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t* idRenderSystemImageLocal::FindImageFile( StringEntry name, imgType_t type, S32 flags )
{
    S32	width, height, picNumMips, checkFlagsTrue, checkFlagsFalse;
    S64	hash;
    U32  picFormat;
    image_t*	image;
    U8*	pic;
    
    if ( !name )
    {
        return NULL;
    }
    
    hash = generateHashValue( name );
    
    // see if the image is already loaded
    for ( image = imageHashTable[hash]; image; image = image->next )
    {
        if ( !::strcmp( name, image->imgName ) )
        {
            // the white image can be used with any set of parms, but other mismatches are errors
            if ( ::strcmp( name, "*white" ) )
            {
                if ( image->flags != flags )
                {
                    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags );
                }
            }
            return image;
        }
    }
    
    // load the pic from disk
    idLoadImage( name, &pic, &width, &height, &picFormat, &picNumMips );
    if ( pic == NULL )
    {
        return NULL;
    }
    
    checkFlagsTrue = IMGFLAG_PICMIP | IMGFLAG_MIPMAP | IMGFLAG_GENNORMALMAP;
    checkFlagsFalse = IMGFLAG_CUBEMAP;
    
    if ( r_normalMapping->integer && ( picFormat == GL_RGBA8 ) && ( type == IMGTYPE_COLORALPHA ) &&
            ( ( flags & checkFlagsTrue ) == checkFlagsTrue ) && !( flags & checkFlagsFalse ) )
    {
        S32 normalFlags, normalWidth, normalHeight;
        UTF8 normalName[MAX_QPATH];
        image_t* normalImage;
        
        normalFlags = ( flags & ~IMGFLAG_GENNORMALMAP ) | IMGFLAG_NOLIGHTSCALE;
        
        COM_StripExtension3( name, normalName, MAX_QPATH );
        Q_strcat( normalName, MAX_QPATH, "_n" );
        
        // find normalmap in case it's there
        normalImage = FindImageFile( normalName, IMGTYPE_NORMAL, normalFlags );
        
        // if not, generate it
        if ( normalImage == NULL )
        {
            S32 x, y;
            U8* normalPic;
            
            normalWidth = width;
            normalHeight = height;
            normalPic = ( U8* )clientMainSystem->RefMalloc( width * height * 4 );
            RGBAtoNormal( pic, normalPic, width, height, flags & IMGFLAG_CLAMPTOEDGE );
            
            // Brighten up the original image to work with the normal map
            RGBAtoYCoCgA( pic, pic, width, height );
            for ( y = 0; y < height; y++ )
            {
                U8* picbyte  = pic       + y * width * 4;
                U8* normbyte = normalPic + y * width * 4;
                for ( x = 0; x < width; x++ )
                {
                    S32 div = MAX( normbyte[2] - 127, 16 );
                    picbyte[0] = CLAMP( picbyte[0] * 128 / div, 0, 255 );
                    picbyte  += 4;
                    normbyte += 4;
                }
            }
            YCoCgAtoRGBA( pic, pic, width, height );
            
            CreateImage( normalName, normalPic, normalWidth, normalHeight, IMGTYPE_NORMAL, normalFlags, 0 );
            memorySystem->Free( normalPic );
        }
    }
    
    // force mipmaps off if image is compressed but doesn't have enough mips
    if ( ( flags & IMGFLAG_MIPMAP ) && picFormat != GL_RGBA8 && picFormat != GL_SRGB8_ALPHA8_EXT )
    {
        S32 wh = MAX( width, height ), neededMips = 0;
        
        while ( wh )
        {
            neededMips++;
            wh >>= 1;
        }
        
        if ( neededMips > picNumMips )
        {
            flags &= ~IMGFLAG_MIPMAP;
        }
    }
    
    image = CreateImage2( const_cast< UTF8* >( name ), pic, width, height, picFormat, picNumMips, type, flags, 0 );
    memorySystem->Free( pic );
    return image;
}


/*
================
idRenderSystemImageLocal::reateDlightImage
================
*/
void idRenderSystemImageLocal::CreateDlightImage( void )
{
    S32 x, y, b;
    U8 data[DLIGHT_SIZE][DLIGHT_SIZE][4];
    
    // make a centered inverse-square falloff blob for dynamic lighting
    for ( x = 0; x < DLIGHT_SIZE; x++ )
    {
        for ( y = 0; y < DLIGHT_SIZE; y++ )
        {
            F32 d;
            
            d = ( DLIGHT_SIZE / 2 - 0.5f - x ) * ( DLIGHT_SIZE / 2 - 0.5f - x ) +
                ( DLIGHT_SIZE / 2 - 0.5f - y ) * ( DLIGHT_SIZE / 2 - 0.5f - y );
            b = ( S32 )( 4000 / d );
            
            if ( b > 255 )
            {
                b = 255;
            }
            else if ( b < 75 )
            {
                b = 0;
            }
            
            data[y][x][0] =
                data[y][x][1] =
                    data[y][x][2] = b;
            data[y][x][3] = 255;
        }
    }
    
    tr.dlightImage = CreateImage( "*dlight", ( U8* )data, DLIGHT_SIZE, DLIGHT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
}

/*
=================
idRenderSystemImageLocal::InitFogTable
=================
*/
void idRenderSystemImageLocal::InitFogTable( void )
{
    S32 i;
    F32	d, exp;
    
    exp = 0.5;
    
    for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ )
    {
        d = pow( ( F32 )i / ( FOG_TABLE_SIZE - 1 ), exp );
        
        tr.fogTable[i] = d;
    }
}

/*
================
idRenderSystemImageLocal::FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
F32	idRenderSystemImageLocal::FogFactor( F32 s, F32 t )
{
    F32	d;
    
    s -= 1.0f / 512;
    
    if ( s < 0 )
    {
        return 0;
    }
    
    if ( t < 1.0 / 32 )
    {
        return 0;
    }
    
    if ( t < 31.0 / 32 )
    {
        s *= ( t - 1.0f / 32.0f ) / ( 30.0f / 32.0f );
    }
    
    // we need to leave a lot of clamp range
    s *= 8;
    
    if ( s > 1.0 )
    {
        s = 1.0;
    }
    
    d = tr.fogTable[( S32 )( s * ( FOG_TABLE_SIZE - 1 ) ) ];
    
    return d;
}

/*
================
idRenderSystemImageLocal::CreateFogImage
================
*/
void idRenderSystemImageLocal::CreateFogImage( void )
{
    S32	x, y;
    F32	d;
    U8*	data = nullptr;
    
    data = ( U8* )memorySystem->AllocateTempMemory( FOG_S * FOG_T * 4 );
    
    // S is distance, T is depth
    for ( x = 0 ; x < FOG_S ; x++ )
    {
        for ( y = 0 ; y < FOG_T ; y++ )
        {
            d = FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );
            
            data[( y * FOG_S + x ) * 4 + 0] = data[( y * FOG_S + x ) * 4 + 1] = data[( y * FOG_S + x ) * 4 + 2] = 255;
            data[( y * FOG_S + x ) * 4 + 3] = ( U8 )( 255 * d );
        }
    }
    tr.fogImage = CreateImage( "*fog", ( U8* )data, FOG_S, FOG_T, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0 );
    memorySystem->FreeTempMemory( data );
}

/*
================
idRenderSystemImageLocal::CreateEnvBrdfLUT

from https://github.com/knarkowicz/IntegrateDFG
================
*/
void idRenderSystemImageLocal::CreateEnvBrdfLUT( void )
{
    if ( !r_cubeMapping->integer )
    {
        return;
    }
    
    U16	data[LUT_WIDTH][LUT_HEIGHT][2];
    
    F32 const MATH_PI = 3.14159f;
    U32 const sampleNum = 1024;
    
    for ( U32 y = 0; y < LUT_HEIGHT; ++y )
    {
        F32 const ndotv = ( y + 0.5f ) / LUT_HEIGHT;
        
        for ( U32 x = 0; x < LUT_WIDTH; ++x )
        {
            F32 const roughness = ( x + 0.5f ) / LUT_WIDTH;
            F32 const m = roughness * roughness;
            F32 const m2 = m * m;
            
            F32 const vx = sqrtf( 1.0f - ndotv * ndotv );
            F32 const vy = 0.0f;
            F32 const vz = ndotv;
            
            F32 scale = 0.0f;
            F32 bias = 0.0f;
            
            for ( U32 i = 0; i < sampleNum; ++i )
            {
                F32 const e1 = ( F32 )i / sampleNum;
                F32 const e2 = ( F32 )( ( F64 )idRenderSystemMathsLocal::ReverseBits( i ) / ( F64 )0x100000000LL );
                
                F32 const phi = 2.0f * MATH_PI * e1;
                F32 const cosPhi = cosf( phi );
                F32 const sinPhi = sinf( phi );
                F32 const cosTheta = sqrtf( ( 1.0f - e2 ) / ( 1.0f + ( roughness * roughness * roughness * roughness - 1.0f ) * e2 ) );
                F32 const sinTheta = sqrtf( 1.0f - cosTheta * cosTheta );
                
                F32 const hx = sinTheta * cosf( phi );
                F32 const hy = sinTheta * sinf( phi );
                F32 const hz = cosTheta;
                
                F32 const vdh = vx * hx + vy * hy + vz * hz;
                F32 const lx = 2.0f * vdh * hx - vx;
                F32 const ly = 2.0f * vdh * hy - vy;
                F32 const lz = 2.0f * vdh * hz - vz;
                
                F32 const ndotl = MAX( lz, 0.0f );
                F32 const ndoth = MAX( hz, 0.0f );
                F32 const vdoth = MAX( vdh, 0.0f );
                
                if ( ndotl > 0.0f )
                {
                    F32 const visibility = idRenderSystemMathsLocal::GSmithCorrelated( roughness, ndotv, ndotl );
                    F32 const ndotlVisPDF = ndotl * visibility * ( 4.0f * vdoth / ndoth );
                    F32 const fresnel = powf( 1.0f - vdoth, 5.0f );
                    
                    scale += ndotlVisPDF * ( 1.0f - fresnel );
                    bias += ndotlVisPDF * fresnel;
                }
            }
            scale /= sampleNum;
            bias /= sampleNum;
            
            data[y][x][0] = idRenderSystemMathsLocal::FloatToHalf( scale );
            data[y][x][1] = idRenderSystemMathsLocal::FloatToHalf( bias );
        }
    }
    
    tr.envBrdfImage = CreateImage( "*envBrdfLUT", ( U8* )data, LUT_WIDTH, LUT_HEIGHT, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                   GL_RGBA16F );
}

/*
==================
CreateDefaultImage
==================
*/
void idRenderSystemImageLocal::CreateDefaultImage( void )
{
    S32 x;
    U8 data[DEFAULT_SIZE][DEFAULT_SIZE][4];
    
    // the default image will be a box, to allow you to see the mapping coordinates
    ::memset( data, 32, sizeof( data ) );
    
    for ( x = 0 ; x < DEFAULT_SIZE ; x++ )
    {
        data[0][x][0] =
            data[0][x][1] =
                data[0][x][2] =
                    data[0][x][3] = 255;
                    
        data[x][0][0] =
            data[x][0][1] =
                data[x][0][2] =
                    data[x][0][3] = 255;
                    
        data[DEFAULT_SIZE - 1][x][0] =
            data[DEFAULT_SIZE - 1][x][1] =
                data[DEFAULT_SIZE - 1][x][2] =
                    data[DEFAULT_SIZE - 1][x][3] = 255;
                    
        data[x][DEFAULT_SIZE - 1][0] =
            data[x][DEFAULT_SIZE - 1][1] =
                data[x][DEFAULT_SIZE - 1][2] =
                    data[x][DEFAULT_SIZE - 1][3] = 255;
    }
    
    tr.defaultImage = CreateImage( "*default", ( U8* )data, DEFAULT_SIZE, DEFAULT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP, 0 );
}

/*
==================
idRenderSystemImageLocal::CreateBuiltinImages
==================
*/
void idRenderSystemImageLocal::CreateBuiltinImages( void )
{
    S32 x, y;
    U8 data[DEFAULT_SIZE][DEFAULT_SIZE][4];
    
    CreateDefaultImage();
    
    // we use a solid white image instead of disabling texturing
    ::memset( data, 255, sizeof( data ) );
    tr.whiteImage = CreateImage( "*white", ( U8* )data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0 );
    
    if ( r_dlightMode->integer >= 2 )
    {
        for ( x = 0; x < MAX_DLIGHTS; x++ )
        {
            tr.shadowCubemaps[x].image = CreateImage( va( "*shadowcubemap%i", x ), NULL, PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE,
                                         IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_CUBEMAP, GL_DEPTH_COMPONENT24 );
            idRenderSystemBackendLocal::BindToTMU( tr.shadowCubemaps[x].image, GL_DEPTH_COMPONENT24 );
            qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
            qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
        }
    }
    
    // with overbright bits active, we need an image which is some fraction of full color,
    // for default lightmaps, etc
    for ( x = 0 ; x < DEFAULT_SIZE ; x++ )
    {
        for ( y = 0 ; y < DEFAULT_SIZE ; y++ )
        {
            data[y][x][0] = data[y][x][1] = data[y][x][2] = tr.identityLightByte;
            data[y][x][3] = 255;
        }
    }
    
    tr.identityLightImage = CreateImage( "*identityLight", ( U8* )data, 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0 );
    
    for ( x = 0; x < 32; x++ )
    {
        // scratchimage is usually used for cinematic drawing
        tr.scratchImage[x] = CreateImage( "*scratch", ( U8* )data, DEFAULT_SIZE, DEFAULT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE, 0 );
    }
    
    CreateDlightImage();
    CreateFogImage();
    CreateEnvBrdfLUT();
    
    if ( glRefConfig.framebufferObject )
    {
        S32 width, height, hdrFormat, rgbFormat;
        
        width = glConfig.vidWidth;
        height = glConfig.vidHeight;
        
        hdrFormat = GL_RGBA8;
        
        if ( r_hdr->integer && glRefConfig.textureFloat )
        {
            hdrFormat = GL_RGBA16F_ARB;
        }
        
        rgbFormat = GL_RGBA8;
        
        tr.renderImage = CreateImage( "_render", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        tr.glowImage = CreateImage( "*glow", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        
        tr.normalDetailedImage = CreateImage( "*normaldetailed", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        
        tr.screenScratchImage = CreateImage( "screenScratch", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, rgbFormat );
        
        if ( r_shadowBlur->integer || r_ssao->integer )
        {
            tr.hdrDepthImage = CreateImage( "*hdrDepth", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_R32F );
        }
        
        if ( r_drawSunRays->integer )
        {
            tr.sunRaysImage = CreateImage( "*sunRays", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, rgbFormat );
        }
        
        tr.renderDepthImage  = CreateImage( "*renderdepth",  NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH_COMPONENT24 );
        tr.textureDepthImage = CreateImage( "*texturedepth", NULL, PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH_COMPONENT24 );
        
        tr.genericFBOImage = CreateImage( "_generic", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        tr.genericFBO2Image = CreateImage( "_generic2", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        
        tr.bloomRenderFBOImage[0] = CreateImage( "_bloom0", NULL, width / 2, height / 2, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        tr.bloomRenderFBOImage[1] = CreateImage( "_bloom1", NULL, width / 2, height / 2, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        tr.bloomRenderFBOImage[2] = CreateImage( "_bloom2", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        
        tr.anamorphicRenderFBOImage[0] = CreateImage( "_anamorphic0", NULL, width / 8, height / 8, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        tr.anamorphicRenderFBOImage[1] = CreateImage( "_anamorphic1", NULL, width / 8, height / 8, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        tr.anamorphicRenderFBOImage[2] = CreateImage( "_anamorphic2", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        
        {
            U8* p;
            
            data[0][0][0] = 0;
            data[0][0][1] = ( U8 )( 0.45f * 255 );
            data[0][0][2] = 255;
            data[0][0][3] = 255;
            p = ( U8* )data;
            
            tr.calcLevelsImage =   CreateImage( "*calcLevels",    p, 1, 1, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
            tr.targetLevelsImage = CreateImage( "*targetLevels",  p, 1, 1, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
            tr.fixedLevelsImage =  CreateImage( "*fixedLevels",   p, 1, 1, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        }
        
        for ( x = 0; x < 2; x++ )
        {
            tr.textureScratchImage[x] = CreateImage( va( "*textureScratch%d", x ), NULL, 256, 256, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8 );
        }
        for ( x = 0; x < 2; x++ )
        {
            tr.quarterImage[x] = CreateImage( va( "*quarter%d", x ), NULL, width / 2, height / 2, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8 );
        }
        
        if ( r_ssao->integer )
        {
            tr.screenSsaoImage = CreateImage( "*screenSsao", NULL, width / 2, height / 2, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8 );
        }
        
        for ( x = 0; x < MAX_DRAWN_PSHADOWS; x++ )
        {
            tr.pshadowMaps[x] = CreateImage( va( "*shadowmap%i", x ), NULL, PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH_COMPONENT24 );
            qglTextureParameterfEXT( tr.pshadowMaps[x]->texnum, GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
            qglTextureParameterfEXT( tr.pshadowMaps[x]->texnum, GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
        }
        
        if ( r_sunlightMode->integer )
        {
            for ( x = 0; x < 4; x++ )
            {
                tr.sunShadowDepthImage[x] = CreateImage( va( "*sunshadowdepth%i", x ), NULL, r_shadowMapSize->integer, r_shadowMapSize->integer, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH_COMPONENT24 );
                qglTextureParameterfEXT( tr.sunShadowDepthImage[x]->texnum, GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
                qglTextureParameterfEXT( tr.sunShadowDepthImage[x]->texnum, GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
            }
            
            tr.screenShadowImage = CreateImage( "*screenShadow", NULL, width, height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8 );
        }
        
        if ( r_cubeMapping->integer )
        {
            tr.renderCubeImage = CreateImage( "*renderCube", NULL, r_cubemapSize->integer, r_cubemapSize->integer, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, hdrFormat );
            tr.prefilterEnvMapImage = CreateImage( "*prefilterEnvMapFbo", NULL, r_cubemapSize->integer / 2, r_cubemapSize->integer / 2, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat );
        }
    }
}

/*
===============
idRenderSystemImageLocal::SetColorMappings
===============
*/
void idRenderSystemImageLocal::SetColorMappings( void )
{
    S32	i, j;
    
    // setup the overbright lighting
    tr.overbrightBits = r_overBrightBits->integer;
    
    // allow 2 overbright bits
    if ( tr.overbrightBits > 2 )
    {
        tr.overbrightBits = 2;
    }
    else if ( tr.overbrightBits < 0 )
    {
        tr.overbrightBits = 0;
    }
    
    // don't allow more overbright bits than map overbright bits
    if ( tr.overbrightBits > r_mapOverBrightBits->integer )
    {
        tr.overbrightBits = r_mapOverBrightBits->integer;
    }
    
    tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
    tr.identityLightByte = ( S32 )( 255 * tr.identityLight );
    
    if ( r_intensity->value <= 1 )
    {
        cvarSystem->Set( "r_intensity", "1" );
    }
    
    if ( r_gamma->value < 0.5f )
    {
        cvarSystem->Set( "r_gamma", "0.5" );
    }
    else if ( r_gamma->value > 3.0f )
    {
        cvarSystem->Set( "r_gamma", "3.0" );
    }
    
    for ( i = 0 ; i < 256 ; i++ )
    {
        j = ( S32 )( i * r_intensity->value );
        
        if ( j > 255 )
        {
            j = 255;
        }
        
        s_intensitytable[i] = j;
    }
}

/*
===============
idRenderSystemImageLocal::InitImages
===============
*/
void idRenderSystemImageLocal::InitImages( void )
{
    ::memset( imageHashTable, 0, sizeof( imageHashTable ) );
    
    // build brightness translation tables
    SetColorMappings();
    
    // create default texture and white texture
    CreateBuiltinImages();
}

/*
===============
idRenderSystemImageLocal::DeleteTextures
===============
*/
void idRenderSystemImageLocal::DeleteTextures( void )
{
    S32		i;
    
    for ( i = 0; i < tr.numImages ; i++ )
    {
        qglDeleteTextures( 1, &tr.images[i]->texnum );
    }
    
    ::memset( tr.images, 0, sizeof( tr.images ) );
    
    tr.numImages = 0;
    
    idRenderSystemDSALocal::BindNullTextures();
}

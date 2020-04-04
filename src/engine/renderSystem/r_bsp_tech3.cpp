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
// File name:   r_bsp.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: Loads and prepares a map file for scene rendering.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemBSPTechLocal renderSystemBSPTechLocal;

/*
===============
idRenderSystemBSPTechLocal::idRenderSystemBSPTechLocal
===============
*/
idRenderSystemBSPTechLocal::idRenderSystemBSPTechLocal( void )
{
}

/*
===============
idRenderSystemBSPTechLocal::~idRenderSystemBSPTechLocal
===============
*/
idRenderSystemBSPTechLocal::~idRenderSystemBSPTechLocal( void )
{
}

static world_t s_worldData;
static U8* fileBase;

S32	c_subdivisions, c_gridVerts;

void idRenderSystemBSPTechLocal::HSVtoRGB( F32 h, F32 s, F32 v, F32 rgb[3] )
{
    S32 i;
    F32 f, p, q, t;
    
    h *= 5;
    
    i = ( S32 )floorf( h );
    f = h - i;
    
    p = v * ( 1 - s );
    q = v * ( 1 - s * f );
    t = v * ( 1 - s * ( 1 - f ) );
    
    switch ( i )
    {
        case 0:
            rgb[0] = v;
            rgb[1] = t;
            rgb[2] = p;
            break;
        case 1:
            rgb[0] = q;
            rgb[1] = v;
            rgb[2] = p;
            break;
        case 2:
            rgb[0] = p;
            rgb[1] = v;
            rgb[2] = t;
            break;
        case 3:
            rgb[0] = p;
            rgb[1] = q;
            rgb[2] = v;
            break;
        case 4:
            rgb[0] = t;
            rgb[1] = p;
            rgb[2] = v;
            break;
        case 5:
            rgb[0] = v;
            rgb[1] = p;
            rgb[2] = q;
            break;
    }
}

/*
===============
idRenderSystemBSPTechLocal::ColorShiftLightingBytes
===============
*/
void idRenderSystemBSPTechLocal::ColorShiftLightingBytes( U8 in[4], U8 out[4] )
{
    S32	shift, r, g, b;
    
    // shift the color data based on overbright range
    shift = r_mapOverBrightBits->integer - tr.overbrightBits;
    
    // shift the data based on overbright range
    r = in[0] << shift;
    g = in[1] << shift;
    b = in[2] << shift;
    
    // normalize by color instead of saturating to white
    if ( ( r | g | b ) > 255 )
    {
        S32		max;
        
        max = r > g ? r : g;
        max = max > b ? max : b;
        r = r * 255 / max;
        g = g * 255 / max;
        b = b * 255 / max;
    }
    
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = in[3];
}


/*
===============
idRenderSystemBSPTechLocal::ColorShiftLightingFloats
===============
*/
void idRenderSystemBSPTechLocal::ColorShiftLightingFloats( F32 in[4], F32 out[4] )
{
    F32	r, g, b, scale = ( 1 << ( r_mapOverBrightBits->integer - tr.overbrightBits ) ) / 255.0f;
    
    r = in[0] * scale;
    g = in[1] * scale;
    b = in[2] * scale;
    
    // normalize by color instead of saturating to white
    if ( !glRefConfig.textureFloat )
    {
        if ( r > 1 || g > 1 || b > 1 )
        {
            F32	max;
            
            max = r > g ? r : g;
            max = max > b ? max : b;
            r = r / max;
            g = g / max;
            b = b / max;
        }
    }
    
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = in[3];
}

// Modified from http://graphicrants.blogspot.jp/2009/04/rgbm-color-encoding.html
void idRenderSystemBSPTechLocal::ColorToRGBM( const vec3_t color, U8 rgbm[4] )
{
    F32	maxComponent;
    vec3_t sample;
    
    VectorCopy( color, sample );
    
    maxComponent = MAX( sample[0], sample[1] );
    maxComponent = MAX( maxComponent, sample[2] );
    maxComponent = CLAMP( maxComponent, 1.0f / 255.0f, 1.0f );
    
    rgbm[3] = ( U8 )ceil( maxComponent * 255.0f );
    maxComponent = 255.0f / rgbm[3];
    
    VectorScale( sample, maxComponent, sample );
    
    rgbm[0] = ( U8 )( sample[0] * 255 );
    rgbm[1] = ( U8 )( sample[1] * 255 );
    rgbm[2] = ( U8 )( sample[2] * 255 );
}

void idRenderSystemBSPTechLocal::ColorToRGB16( const vec3_t color, U16 rgb16[3] )
{
    rgb16[0] = ( U16 )( color[0] * 65535.0f + 0.5f );
    rgb16[1] = ( U16 )( color[1] * 65535.0f + 0.5f );
    rgb16[2] = ( U16 )( color[2] * 65535.0f + 0.5f );
}

/*
===============
idRenderSystemBSPTechLocal::ColorShiftLightingFloats
===============
*/
void idRenderSystemBSPTechLocal::ColorShiftLightingFloats( F32 in[4], F32 out[4], F32 scale )
{
    F32 r, g, b;
    
    scale *= pow( 2.0f, r_mapOverBrightBits->integer - tr.overbrightBits );
    
    r = in[0] * scale;
    g = in[1] * scale;
    b = in[2] * scale;
    
    //if (!glRefConfig.floatLightmap)
    {
        if ( r > 1.0f || g > 1.0f || b > 1.0f )
        {
            F32 high = Q_max( Q_max( r, g ), b );
            
            r /= high;
            g /= high;
            b /= high;
        }
    }
    
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = in[3];
}

void idRenderSystemBSPTechLocal::ColorToRGBA16F( const vec3_t color, U16 rgba16f[4] )
{
    rgba16f[0] = idRenderSystemMathsLocal::FloatToHalf( color[0] );
    rgba16f[1] = idRenderSystemMathsLocal::FloatToHalf( color[1] );
    rgba16f[2] = idRenderSystemMathsLocal::FloatToHalf( color[2] );
    rgba16f[3] = idRenderSystemMathsLocal::FloatToHalf( 1.0f );
}

/*
===============
idRenderSystemBSPTechLocal::LoadLightmaps
===============
*/
void idRenderSystemBSPTechLocal::LoadLightmaps( lump_t* l, lump_t* surfs )
{
    S32 imgFlags = IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE;
    S32	len, imageSize, i, j, numLightmaps, textureInternalFormat = 0, numLightmapsPerPage = 16;
    U8* buf, * buf_p, * image;
    F32 maxIntensity = 0;
    F64 sumIntensity = 0;
    dsurface_t* surf;
    
    len = l->filelen;
    if ( !len )
    {
        return;
    }
    
    buf = fileBase + l->fileofs;
    
    // we are about to upload textures
    idRenderSystemCmdsLocal::IssuePendingRenderCommands();
    
    tr.lightmapSize = DEFAULT_LIGHTMAP_SIZE;
    numLightmaps = len / ( tr.lightmapSize * tr.lightmapSize * 3 );
    
    // check for deluxe mapping
    if ( numLightmaps <= 1 )
    {
        tr.worldDeluxeMapping = false;
    }
    else
    {
        tr.worldDeluxeMapping = true;
        
        for ( i = 0, surf = ( dsurface_t* )( fileBase + surfs->fileofs );  i < surfs->filelen / sizeof( dsurface_t ); i++, surf++ )
        {
            S32 lightmapNum = LittleLong( surf->lightmapNum );
            
            if ( lightmapNum >= 0 && ( lightmapNum & 1 ) != 0 )
            {
                tr.worldDeluxeMapping = false;
                break;
            }
        }
    }
    
    imageSize = tr.lightmapSize * tr.lightmapSize * 4 * 2;
    image = ( U8* )memorySystem->Malloc( imageSize );
    
    if ( tr.worldDeluxeMapping )
    {
        numLightmaps >>= 1;
    }
    
    // Use fat lightmaps of an appropriate size.
    if ( r_mergeLightmaps->integer )
    {
        S32 maxLightmapsPerAxis = glConfig.maxTextureSize / tr.lightmapSize;
        S32 lightmapCols = 4, lightmapRows = 4;
        
        // Increase width at first, then height.
        while ( lightmapCols * lightmapRows < numLightmaps && lightmapCols != maxLightmapsPerAxis )
        {
            lightmapCols <<= 1;
        }
        
        while ( lightmapCols * lightmapRows < numLightmaps && lightmapRows != maxLightmapsPerAxis )
        {
            lightmapRows <<= 1;
        }
        
        tr.fatLightmapCols = lightmapCols;
        tr.fatLightmapRows = lightmapRows;
        numLightmapsPerPage = lightmapCols * lightmapRows;
        
        tr.numLightmaps = ( numLightmaps + ( numLightmapsPerPage - 1 ) ) / numLightmapsPerPage;
    }
    else
    {
        tr.numLightmaps = numLightmaps;
    }
    
    tr.lightmaps = reinterpret_cast<image_t**>( memorySystem->Alloc( tr.numLightmaps * sizeof( image_t* ), h_low ) );
    
    if ( tr.worldDeluxeMapping )
    {
        tr.deluxemaps = reinterpret_cast<image_t**>( memorySystem->Alloc( tr.numLightmaps * sizeof( image_t* ), h_low ) );
    }
    
    textureInternalFormat = GL_RGBA8;
    if ( r_hdr->integer )
    {
        // Check for the first hdr lightmap, if it exists, use GL_RGBA16 for textures.
        UTF8 filename[MAX_QPATH];
        
        Q_snprintf( filename, sizeof( filename ), "maps/%s/lm_0000.hdr", s_worldData.baseName );
        
        if ( fileSystem->FileExists( filename ) )
        {
            textureInternalFormat = GL_RGBA16;
        }
    }
    
    if ( r_mergeLightmaps->integer )
    {
        S32 width = tr.fatLightmapCols * tr.lightmapSize;
        S32 height = tr.fatLightmapRows * tr.lightmapSize;
        
        for ( i = 0; i < tr.numLightmaps; i++ )
        {
            tr.lightmaps[i] = idRenderSystemImageLocal::CreateImage( va( "_fatlightmap%d", i ), NULL, width, height, IMGTYPE_COLORALPHA,
                              imgFlags, textureInternalFormat );
                              
            if ( tr.worldDeluxeMapping )
            {
                tr.deluxemaps[i] = idRenderSystemImageLocal::CreateImage( va( "_fatdeluxemap%d", i ), NULL, width, height, IMGTYPE_DELUXE, imgFlags, 0 );
            }
        }
    }
    
    for ( i = 0; i < numLightmaps; i++ )
    {
        S32 xoff = 0, yoff = 0, lightmapnum = i;
        
        // expand the 24 bit on-disk to 32 bit
        if ( r_mergeLightmaps->integer )
        {
            S32 lightmaponpage = i % numLightmapsPerPage;
            xoff = ( lightmaponpage % tr.fatLightmapCols ) * tr.lightmapSize;
            yoff = ( lightmaponpage / tr.fatLightmapCols ) * tr.lightmapSize;
            
            lightmapnum /= numLightmapsPerPage;
        }
        
        // if (tr.worldLightmapping)
        {
            S32 lightmapWidth = tr.lightmapSize, lightmapHeight = tr.lightmapSize, picNumMips;
            U32 picFormat;
            U8* hdrLightmap = NULL;
            F32* hdrL = NULL;
            UTF8 filename[MAX_QPATH];
            
            // look for hdr lightmaps
            if ( textureInternalFormat == GL_RGBA16 )
            {
                Q_snprintf( filename, sizeof( filename ), "maps/%s/lm_%04d.hdr", s_worldData.baseName, i * ( tr.worldDeluxeMapping ? 2 : 1 ) );
                
                idRenderSystemImageLocal::idLoadImage( filename, &hdrLightmap, &lightmapWidth, &lightmapHeight, &picFormat, &picNumMips );
                
                if ( hdrLightmap )
                {
                    S32 newImageSize = lightmapWidth * lightmapHeight * 4 * 2;
                    hdrL = ( F32* )hdrLightmap;
                    
                    if ( r_mergeLightmaps->integer && ( lightmapWidth != tr.lightmapSize || lightmapHeight != tr.lightmapSize ) )
                    {
                        clientMainSystem->RefPrintf( PRINT_ALL, "Error loading %s: non %dx%d lightmaps require r_mergeLightmaps 0.\n", filename, tr.lightmapSize, tr.lightmapSize );
                        memorySystem->Free( hdrLightmap );
                        hdrLightmap = NULL;
                    }
                    else if ( newImageSize > imageSize )
                    {
                        memorySystem->Free( image );
                        imageSize = newImageSize;
                        image = ( U8* )memorySystem->Malloc( imageSize );
                    }
                }
                
                if ( !hdrLightmap )
                {
                    lightmapWidth = tr.lightmapSize;
                    lightmapHeight = tr.lightmapSize;
                }
            }
            
            if ( hdrLightmap )
            {
                buf_p = hdrLightmap;
                
            }
            else
            {
                if ( tr.worldDeluxeMapping )
                {
                    buf_p = buf + ( i * 2 ) * tr.lightmapSize * tr.lightmapSize * 3;
                }
                else
                {
                    buf_p = buf + i * tr.lightmapSize * tr.lightmapSize * 3;
                }
            }
            
            for ( j = 0; j < lightmapWidth * lightmapHeight; j++ )
            {
                if ( hdrLightmap )
                {
                    vec4_t color;
                    
                    memcpy( color, &hdrL[j * 3], 12 );
                    
                    color[0] = LittleFloat( color[0] );
                    color[1] = LittleFloat( color[1] );
                    color[2] = LittleFloat( color[2] );
                    color[3] = 1.0f;
                    
                    renderSystemBSPTechLocal.ColorShiftLightingFloats( color, color );
                    
                    color[0] = sqrtf( color[0] );
                    color[1] = sqrtf( color[1] );
                    color[2] = sqrtf( color[2] );
                    
                    renderSystemBSPTechLocal.ColorToRGBA16F( color, ( uint16_t* )( &image[j * 8] ) );
                }
                else if ( textureInternalFormat == GL_RGBA16 )
                {
                    vec4_t color;
                    
                    //hack: convert LDR lightmap to HDR one
                    color[0] = MAX( buf_p[j * 3 + 0], 0.499f );
                    color[1] = MAX( buf_p[j * 3 + 1], 0.499f );
                    color[2] = MAX( buf_p[j * 3 + 2], 0.499f );
                    
                    // if under an arbitrary value (say 12) grey it out
                    // this prevents weird splotches in dimly lit areas
                    if ( color[0] + color[1] + color[2] < 12.0f )
                    {
                        F32 avg = ( color[0] + color[1] + color[2] ) * 0.3333f;
                        color[0] = avg;
                        color[1] = avg;
                        color[2] = avg;
                    }
                    color[3] = 1.0f;
                    
                    renderSystemBSPTechLocal.ColorShiftLightingFloats( color, color );
                    
                    renderSystemBSPTechLocal.ColorToRGB16( color, ( U16* )( &image[j * 8] ) );
                    ( ( U16* )( &image[j * 8] ) )[3] = 65535;
                }
                else
                {
                    if ( r_lightmap->integer == 2 )
                    {
                        // color code by intensity as development tool	(FIXME: check range)
                        F32 r = buf_p[j * 3 + 0];
                        F32 g = buf_p[j * 3 + 1];
                        F32 b = buf_p[j * 3 + 2];
                        F32 intensity;
                        F32 out[3] = { 0.0, 0.0, 0.0 };
                        
                        intensity = 0.33f * r + 0.685f * g + 0.063f * b;
                        
                        if ( intensity > 255 )
                        {
                            intensity = 1.0f;
                        }
                        else
                        {
                            intensity /= 255.0f;
                        }
                        
                        if ( intensity > maxIntensity )
                        {
                            maxIntensity = intensity;
                        }
                        
                        renderSystemBSPTechLocal.HSVtoRGB( intensity, 1.00f, 0.50f, out );
                        
                        image[j * 4 + 0] = ( U8 )( out[0] * 255 );
                        image[j * 4 + 1] = ( U8 )( out[1] * 255 );
                        image[j * 4 + 2] = ( U8 )( out[2] * 255 );
                        image[j * 4 + 3] = 255;
                        
                        sumIntensity += intensity;
                    }
                    else
                    {
                        renderSystemBSPTechLocal.ColorShiftLightingBytes( &buf_p[j * 3], &image[j * 4] );
                        image[j * 4 + 3] = 255;
                    }
                }
            }
            
            if ( r_mergeLightmaps->integer )
            {
                idRenderSystemImageLocal::UpdateSubImage( tr.lightmaps[lightmapnum], image, xoff, yoff, lightmapWidth, lightmapHeight, textureInternalFormat );
            }
            else
            {
                tr.lightmaps[i] = idRenderSystemImageLocal::CreateImage( va( "*lightmap%d", i ), image, lightmapWidth, lightmapHeight, IMGTYPE_COLORALPHA, imgFlags, textureInternalFormat );
            }
            
            if ( hdrLightmap )
            {
                fileSystem->FreeFile( hdrLightmap );
            }
        }
        
        if ( tr.worldDeluxeMapping )
        {
            buf_p = buf + ( i * 2 + 1 ) * tr.lightmapSize * tr.lightmapSize * 3;
            
            for ( j = 0; j < tr.lightmapSize * tr.lightmapSize; j++ )
            {
                image[j * 4 + 0] = buf_p[j * 3 + 0];
                image[j * 4 + 1] = buf_p[j * 3 + 1];
                image[j * 4 + 2] = buf_p[j * 3 + 2];
                
                // make 0,0,0 into 127,127,127
                if ( ( image[j * 4 + 0] == 0 ) && ( image[j * 4 + 1] == 0 ) && ( image[j * 4 + 2] == 0 ) )
                {
                    image[j * 4 + 0] = image[j * 4 + 1] = image[j * 4 + 2] = 127;
                }
                
                image[j * 4 + 3] = 255;
            }
            
            if ( r_mergeLightmaps->integer )
            {
                idRenderSystemImageLocal::UpdateSubImage( tr.deluxemaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize, GL_RGBA8 );
            }
            else
            {
                tr.deluxemaps[i] = idRenderSystemImageLocal::CreateImage( va( "*deluxemap%d", i ), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_DELUXE, imgFlags, 0 );
            }
        }
    }
    
    if ( r_lightmap->integer == 2 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "Brightest lightmap value: %d\n", ( S32 )( maxIntensity * 255 ) );
    }
    
    memorySystem->Free( image );
}

F32 idRenderSystemBSPTechLocal::FatPackU( F32 input, S32 lightmapnum )
{
    if ( lightmapnum < 0 )
    {
        return input;
    }
    
    if ( tr.worldDeluxeMapping )
    {
        lightmapnum >>= 1;
    }
    
    if ( tr.fatLightmapCols > 0 )
    {
        lightmapnum %= ( tr.fatLightmapCols * tr.fatLightmapRows );
        return ( input + ( lightmapnum % tr.fatLightmapCols ) ) / ( F32 )( tr.fatLightmapCols );
    }
    
    return input;
}

F32 idRenderSystemBSPTechLocal::FatPackV( F32 input, S32 lightmapnum )
{
    if ( lightmapnum < 0 )
    {
        return input;
    }
    
    if ( tr.worldDeluxeMapping )
    {
        lightmapnum >>= 1;
    }
    
    if ( tr.fatLightmapCols > 0 )
    {
        lightmapnum %= ( tr.fatLightmapCols * tr.fatLightmapRows );
        return ( input + ( lightmapnum / tr.fatLightmapCols ) ) / ( F32 )( tr.fatLightmapRows );
    }
    
    return input;
}

S32 idRenderSystemBSPTechLocal::FatLightmap( S32 lightmapnum )
{
    if ( lightmapnum < 0 )
    {
        return lightmapnum;
    }
    
    if ( tr.worldDeluxeMapping )
    {
        lightmapnum >>= 1;
    }
    
    if ( tr.fatLightmapCols > 0 )
    {
        return lightmapnum / ( tr.fatLightmapCols * tr.fatLightmapRows );
    }
    
    return lightmapnum;
}

/*
=================
idRenderSystemLocal::SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void idRenderSystemLocal::SetWorldVisData( const U8* vis )
{
    tr.externalVisData = vis;
}

/*
=================
idRenderSystemBSPTechLocal::LoadVisibility
=================
*/
void idRenderSystemBSPTechLocal::LoadVisibility( lump_t* l )
{
    S32	len;
    U8* buf;
    
    len = l->filelen;
    if ( !len )
    {
        return;
    }
    buf = fileBase + l->fileofs;
    
    s_worldData.numClusters = LittleLong( ( ( S32* )buf )[0] );
    s_worldData.clusterBytes = LittleLong( ( ( S32* )buf )[1] );
    
    // CM_Load should have given us the vis data to share, so
    // we don't need to allocate another copy
    if ( tr.externalVisData )
    {
        s_worldData.vis = tr.externalVisData;
    }
    else
    {
        U8* dest = nullptr;
        dest = reinterpret_cast<U8*>( memorySystem->Alloc( len - 8, h_low ) );
        ::memcpy( dest, buf + 8, len - 8 );
        s_worldData.vis = dest;
    }
}

/*
===============
SidRenderSystemBSPTechLocal::haderForShaderNum
===============
*/
shader_t* idRenderSystemBSPTechLocal::ShaderForShaderNum( S32 shaderNum, S32 lightmapNum )
{
    shader_t* _shader;
    dshader_t* dsh;
    
    S32 _shaderNum = LittleLong( shaderNum );
    if ( _shaderNum < 0 || _shaderNum >= s_worldData.numShaders )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::ShaderForShaderNum: bad num %i", _shaderNum );
    }
    dsh = &s_worldData.shaders[_shaderNum];
    
    if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 )
    {
        lightmapNum = LIGHTMAP_BY_VERTEX;
    }
    
    if ( r_fullbright->integer )
    {
        lightmapNum = LIGHTMAP_WHITEIMAGE;
    }
    
    _shader = idRenderSystemShaderLocal::FindShader( dsh->shader, lightmapNum, true );
    
    // if the shader had errors, just use default shader
    if ( _shader->defaultShader )
    {
        return tr.defaultShader;
    }
    
    return _shader;
}

void idRenderSystemBSPTechLocal::LoadDrawVertToSrfVert( srfVert_t* s, drawVert_t* d, S32 realLightmapNum, F32 hdrVertColors[3], vec3_t* bounds )
{
    vec4_t v;
    
    s->xyz[0] = LittleFloat( d->xyz[0] );
    s->xyz[1] = LittleFloat( d->xyz[1] );
    s->xyz[2] = LittleFloat( d->xyz[2] );
    
    if ( bounds )
    {
        AddPointToBounds( s->xyz, bounds[0], bounds[1] );
    }
    
    s->st[0] = LittleFloat( d->st[0] );
    s->st[1] = LittleFloat( d->st[1] );
    
    if ( realLightmapNum >= 0 )
    {
        s->lightmap[0] = FatPackU( LittleFloat( d->lightmap[0] ), realLightmapNum );
        s->lightmap[1] = FatPackV( LittleFloat( d->lightmap[1] ), realLightmapNum );
    }
    else
    {
        s->lightmap[0] = LittleFloat( d->lightmap[0] );
        s->lightmap[1] = LittleFloat( d->lightmap[1] );
    }
    
    v[0] = LittleFloat( d->normal[0] );
    v[1] = LittleFloat( d->normal[1] );
    v[2] = LittleFloat( d->normal[2] );
    
    idRenderSystemVaoLocal::VaoPackNormal( s->normal, v );
    
    if ( hdrVertColors )
    {
        v[0] = hdrVertColors[0];
        v[1] = hdrVertColors[1];
        v[2] = hdrVertColors[2];
    }
    else
    {
        //hack: convert LDR vertex colors to HDR
        if ( r_hdr->integer )
        {
            v[0] = MAX( d->color[0], 0.499f );
            v[1] = MAX( d->color[1], 0.499f );
            v[2] = MAX( d->color[2], 0.499f );
        }
        else
        {
            v[0] = d->color[0];
            v[1] = d->color[1];
            v[2] = d->color[2];
        }
        
    }
    v[3] = d->color[3] / 255.0f;
    
    ColorShiftLightingFloats( v, v );
    idRenderSystemVaoLocal::VaoPackColor( s->color, v );
}

/*
===============
idRenderSystemBSPTechLocal::ParseFace
===============
*/
void idRenderSystemBSPTechLocal::ParseFace( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf, S32* indexes )
{
    S32	i, j, numVerts, numIndexes, badTriangles, realLightmapNum;
    U32* tri;
    srfBspSurface_t* cv;
    
    realLightmapNum = LittleLong( ds->lightmapNum );
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader value
    surf->shader = ShaderForShaderNum( ds->shaderNum, FatLightmap( realLightmapNum ) );
    if ( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    numVerts = LittleLong( ds->numVerts );
    if ( numVerts > 2048 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts );
        numVerts = 2048;
        surf->shader = tr.defaultShader;
    }
    
    numIndexes = LittleLong( ds->numIndexes );
    
    //cv = memorySystem->Alloc(sizeof(*cv), h_low);
    cv = ( srfBspSurface_t* )surf->data;
    cv->surfaceType = SF_FACE;
    
    cv->numIndexes = numIndexes;
    cv->indexes = reinterpret_cast<U32*>( memorySystem->Alloc( numIndexes * sizeof( cv->indexes[0] ), h_low ) );
    
    cv->numVerts = numVerts;
    cv->verts = reinterpret_cast<srfVert_t*>( memorySystem->Alloc( numVerts * sizeof( cv->verts[0] ), h_low ) );
    
    // copy vertexes
    surf->cullinfo.type = CULLINFO_PLANE | CULLINFO_BOX;
    ClearBounds( surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );
    verts += LittleLong( ds->firstVert );
    
    for ( i = 0; i < numVerts; i++ )
    {
        LoadDrawVertToSrfVert( &cv->verts[i], &verts[i], realLightmapNum, hdrVertColors ? hdrVertColors + ( ds->firstVert + i ) * 3 : NULL,
                               surf->cullinfo.bounds );
    }
    
    // copy triangles
    badTriangles = 0;
    indexes += LittleLong( ds->firstIndex );
    for ( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
    {
        for ( j = 0; j < 3; j++ )
        {
            tri[j] = LittleLong( indexes[i + j] );
            
            if ( tri[j] >= ( U32 )numVerts )
            {
                Com_Error( ERR_DROP, "Bad index in face surface" );
            }
        }
        
        if ( ( tri[0] == tri[1] ) || ( tri[1] == tri[2] ) || ( tri[0] == tri[2] ) )
        {
            tri -= 3;
            badTriangles++;
        }
    }
    
    if ( badTriangles )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Face has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3,
                                     numVerts, numIndexes / 3 - badTriangles );
        cv->numIndexes -= badTriangles * 3;
    }
    
    // take the plane information from the lightmap vector
    for ( i = 0; i < 3; i++ )
    {
        cv->cullPlane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
    }
    cv->cullPlane.dist = DotProduct( cv->verts[0].xyz, cv->cullPlane.normal );
    SetPlaneSignbits( &cv->cullPlane );
    cv->cullPlane.type = PlaneTypeForNormal( cv->cullPlane.normal );
    surf->cullinfo.plane = cv->cullPlane;
    
    surf->data = ( surfaceType_t* )cv;
    
    // Calculate tangent spaces
    {
        srfVert_t* dv[3];
        
        for ( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
        {
            dv[0] = &cv->verts[tri[0]];
            dv[1] = &cv->verts[tri[1]];
            dv[2] = &cv->verts[tri[2]];
            
            idRenderSystemMainLocal::CalcTangentVectors( dv );
        }
    }
}

/*
===============
idRenderSystemBSPTechLocal::ParseMesh
===============
*/
void idRenderSystemBSPTechLocal::ParseMesh( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf )
{
    S32 i, width, height, numPoints, realLightmapNum;
    srfBspSurface_t* grid = ( srfBspSurface_t* )surf->data;
    srfVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
    vec3_t bounds[2], tmpVec;
    static surfaceType_t skipData = SF_SKIP;
    
    realLightmapNum = LittleLong( ds->lightmapNum );
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader value
    surf->shader = ShaderForShaderNum( ds->shaderNum, FatLightmap( realLightmapNum ) );
    if ( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    // we may have a nodraw surface, because they might still need to
    // be around for movement clipping
    if ( s_worldData.shaders[LittleLong( ds->shaderNum )].surfaceFlags & SURF_NODRAW )
    {
        surf->data = &skipData;
        return;
    }
    
    width = LittleLong( ds->patchWidth );
    height = LittleLong( ds->patchHeight );
    
    if ( width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE )
    {
        Com_Error( ERR_DROP, "ParseMesh: bad size" );
    }
    
    verts += LittleLong( ds->firstVert );
    numPoints = width * height;
    for ( i = 0; i < numPoints; i++ )
    {
        LoadDrawVertToSrfVert( &points[i], &verts[i], realLightmapNum, hdrVertColors ? hdrVertColors + ( ds->firstVert + i ) * 3 : NULL, NULL );
    }
    
    // pre-tesseleate
    idRenderSystemCurveLocal::SubdividePatchToGrid( grid, width, height, points );
    
    // copy the level of detail origin, which is the center
    // of the group of all curves that must subdivide the same
    // to avoid cracking
    for ( i = 0; i < 3; i++ )
    {
        bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
        bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
    }
    
    VectorAdd( bounds[0], bounds[1], bounds[1] );
    VectorScale( bounds[1], 0.5f, grid->lodOrigin );
    VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
    grid->lodRadius = VectorLength( tmpVec );
    
    surf->cullinfo.type = CULLINFO_BOX | CULLINFO_SPHERE;
    VectorCopy( grid->cullBounds[0], surf->cullinfo.bounds[0] );
    VectorCopy( grid->cullBounds[1], surf->cullinfo.bounds[1] );
    VectorCopy( grid->cullOrigin, surf->cullinfo.localOrigin );
    surf->cullinfo.radius = grid->cullRadius;
}

/*
===============
idRenderSystemBSPTechLocal::ParseTriSurf
===============
*/
void idRenderSystemBSPTechLocal::ParseTriSurf( dsurface_t* ds, drawVert_t* verts, F32* hdrVertColors, msurface_t* surf, S32* indexes )
{
    S32 i, j, numVerts, numIndexes, badTriangles;
    U32* tri;
    srfBspSurface_t* cv;
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader
    surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
    if ( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    numVerts = LittleLong( ds->numVerts );
    numIndexes = LittleLong( ds->numIndexes );
    
    //cv = memorySystem->Alloc(sizeof(*cv), h_low);
    cv = ( srfBspSurface_t* )surf->data;
    cv->surfaceType = SF_TRIANGLES;
    
    cv->numIndexes = numIndexes;
    cv->indexes = reinterpret_cast<U32*>( memorySystem->Alloc( numIndexes * sizeof( cv->indexes[0] ), h_low ) );
    
    cv->numVerts = numVerts;
    cv->verts = reinterpret_cast<srfVert_t*>( memorySystem->Alloc( numVerts * sizeof( cv->verts[0] ), h_low ) );
    surf->data = ( surfaceType_t* )cv;
    
    // copy vertexes
    surf->cullinfo.type = CULLINFO_BOX;
    ClearBounds( surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );
    verts += LittleLong( ds->firstVert );
    for ( i = 0; i < numVerts; i++ )
    {
        LoadDrawVertToSrfVert( &cv->verts[i], &verts[i], -1, hdrVertColors ? hdrVertColors + ( ds->firstVert + i ) * 3 : NULL, surf->cullinfo.bounds );
    }
    
    // copy triangles
    badTriangles = 0;
    indexes += LittleLong( ds->firstIndex );
    for ( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
    {
        for ( j = 0; j < 3; j++ )
        {
            tri[j] = LittleLong( indexes[i + j] );
            
            if ( tri[j] >= ( U32 )numVerts )
            {
                Com_Error( ERR_DROP, "Bad index in face surface" );
            }
        }
        
        if ( ( tri[0] == tri[1] ) || ( tri[1] == tri[2] ) || ( tri[0] == tri[2] ) )
        {
            tri -= 3;
            badTriangles++;
        }
    }
    
    if ( badTriangles )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Trisurf has bad triangles, originally shader %s %d tris %d verts, now %d tris\n",
                                     surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles );
        cv->numIndexes -= badTriangles * 3;
        numIndexes = cv->numIndexes;
    }
    
    // Calculate tangent spaces
    {
        srfVert_t* dv[3];
        
        for ( i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3 )
        {
            dv[0] = &cv->verts[tri[0]];
            dv[1] = &cv->verts[tri[1]];
            dv[2] = &cv->verts[tri[2]];
            
            idRenderSystemMainLocal::CalcTangentVectors( dv );
        }
    }
}

/*
===============
ParseFlare
===============
*/
void idRenderSystemBSPTechLocal::ParseFlare( dsurface_t* ds, drawVert_t* verts, msurface_t* surf, S32* indexes )
{
    srfFlare_t* flare;
    S32				i;
    
    // get fog volume
    surf->fogIndex = LittleLong( ds->fogNum ) + 1;
    
    // get shader
    surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
    if ( r_singleShader->integer && !surf->shader->isSky )
    {
        surf->shader = tr.defaultShader;
    }
    
    //flare = memorySystem->Alloc( sizeof( *flare ), h_low );
    flare = ( srfFlare_t* )surf->data;
    flare->surfaceType = SF_FLARE;
    
    surf->data = ( surfaceType_t* )flare;
    
    for ( i = 0; i < 3; i++ )
    {
        flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
        flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
        flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
    }
    
    surf->cullinfo.type = CULLINFO_NONE;
}

/*
=================
idRenderSystemBSPTechLocal::MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
S32 idRenderSystemBSPTechLocal::MergedWidthPoints( srfBspSurface_t* grid, S32 offset )
{
    S32 i, j;
    
    for ( i = 1; i < grid->width - 1; i++ )
    {
        for ( j = i + 1; j < grid->width - 1; j++ )
        {
            if ( Q_fabs( grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0] ) > .1 )
            {
                continue;
            }
            
            if ( Q_fabs( grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1] ) > .1 )
            {
                continue;
            }
            
            if ( Q_fabs( grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2] ) > .1 )
            {
                continue;
            }
            
            return true;
        }
    }
    return false;
}

/*
=================
idRenderSystemBSPTechLocal::MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
S32 idRenderSystemBSPTechLocal::MergedHeightPoints( srfBspSurface_t* grid, S32 offset )
{
    S32 i, j;
    
    for ( i = 1; i < grid->height - 1; i++ )
    {
        for ( j = i + 1; j < grid->height - 1; j++ )
        {
            if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0] ) > .1 )
            {
                continue;
            }
            
            if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1] ) > .1 )
            {
                continue;
            }
            
            if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2] ) > .1 )
            {
                continue;
            }
            
            return true;
        }
    }
    
    return false;
}

/*
=================
idRenderSystemBSPTechLocal::FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void idRenderSystemBSPTechLocal::FixSharedVertexLodError_r( S32 start, srfBspSurface_t* grid1 )
{
    S32 j, k, l, m, n, offset1, offset2, touch;
    srfBspSurface_t* grid2;
    
    for ( j = start; j < s_worldData.numsurfaces; j++ )
    {
        grid2 = ( srfBspSurface_t* )s_worldData.surfaces[j].data;
        
        // if this surface is not a grid
        if ( grid2->surfaceType != SF_GRID )
        {
            continue;
        }
        
        // if the LOD errors are already fixed for this patch
        if ( grid2->lodFixed == 2 )
        {
            continue;
        }
        
        // grids in the same LOD group should have the exact same lod radius
        if ( grid1->lodRadius != grid2->lodRadius )
        {
            continue;
        }
        
        // grids in the same LOD group should have the exact same lod origin
        if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] )
        {
            continue;
        }
        
        if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] )
        {
            continue;
        }
        
        if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] )
        {
            continue;
        }
        
        touch = false;
        for ( n = 0; n < 2; n++ )
        {
            if ( n )
            {
                offset1 = ( grid1->height - 1 ) * grid1->width;
            }
            else
            {
                offset1 = 0;
            }
            
            if ( MergedWidthPoints( grid1, offset1 ) )
            {
                continue;
            }
            for ( k = 1; k < grid1->width - 1; k++ )
            {
                for ( m = 0; m < 2; m++ )
                {
                    if ( m )
                    {
                        offset2 = ( grid2->height - 1 ) * grid2->width;
                    }
                    else
                    {
                        offset2 = 0;
                    }
                    
                    if ( MergedWidthPoints( grid2, offset2 ) )
                    {
                        continue;
                    }
                    
                    for ( l = 1; l < grid2->width - 1; l++ )
                    {
                        if ( Q_fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 )
                        {
                            continue;
                        }
                        
                        if ( Q_fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 )
                        {
                            continue;
                        }
                        if ( Q_fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 )
                        {
                            continue;
                        }
                        
                        // ok the points are equal and should have the same lod error
                        grid2->widthLodError[l] = grid1->widthLodError[k];
                        touch = true;
                    }
                }
                for ( m = 0; m < 2; m++ )
                {
                    if ( m )
                    {
                        offset2 = grid2->width - 1;
                    }
                    else
                    {
                        offset2 = 0;
                    }
                    
                    if ( MergedHeightPoints( grid2, offset2 ) )
                    {
                        continue;
                    }
                    
                    for ( l = 1; l < grid2->height - 1; l++ )
                    {
                        if ( Q_fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 )
                        {
                            continue;
                        }
                        
                        if ( Q_fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 )
                        {
                            continue;
                        }
                        if ( Q_fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 )
                        {
                            continue;
                        }
                        
                        // ok the points are equal and should have the same lod error
                        grid2->heightLodError[l] = grid1->widthLodError[k];
                        touch = true;
                    }
                }
            }
        }
        for ( n = 0; n < 2; n++ )
        {
            if ( n )
            {
                offset1 = grid1->width - 1;
            }
            else
            {
                offset1 = 0;
            }
            
            if ( MergedHeightPoints( grid1, offset1 ) )
            {
                continue;
            }
            
            for ( k = 1; k < grid1->height - 1; k++ )
            {
                for ( m = 0; m < 2; m++ )
                {
                    if ( m )
                    {
                        offset2 = ( grid2->height - 1 ) * grid2->width;
                    }
                    else
                    {
                        offset2 = 0;
                    }
                    
                    if ( MergedWidthPoints( grid2, offset2 ) )
                    {
                        continue;
                    }
                    
                    for ( l = 1; l < grid2->width - 1; l++ )
                    {
                        if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 )
                        {
                            continue;
                        }
                        
                        if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 )
                        {
                            continue;
                        }
                        
                        if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 )
                        {
                            continue;
                        }
                        
                        // ok the points are equal and should have the same lod error
                        grid2->widthLodError[l] = grid1->heightLodError[k];
                        touch = true;
                    }
                }
                
                for ( m = 0; m < 2; m++ )
                {
                    if ( m )
                    {
                        offset2 = grid2->width - 1;
                    }
                    else
                    {
                        offset2 = 0;
                    }
                    
                    if ( MergedHeightPoints( grid2, offset2 ) )
                    {
                        continue;
                    }
                    
                    for ( l = 1; l < grid2->height - 1; l++ )
                    {
                        if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 )
                        {
                            continue;
                        }
                        
                        if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 )
                        {
                            continue;
                        }
                        
                        if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 )
                        {
                            continue;
                        }
                        
                        // ok the points are equal and should have the same lod error
                        grid2->heightLodError[l] = grid1->heightLodError[k];
                        touch = true;
                    }
                }
            }
        }
        if ( touch )
        {
            grid2->lodFixed = 2;
            FixSharedVertexLodError_r( start, grid2 );
            
            //NOTE: this would be correct but makes things really slow
            grid2->lodFixed = 1;
        }
    }
}

/*
=================
idRenderSystemBSPTechLocal::FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void idRenderSystemBSPTechLocal::FixSharedVertexLodError( void )
{
    S32 i;
    srfBspSurface_t* grid1;
    
    for ( i = 0; i < s_worldData.numsurfaces; i++ )
    {
        grid1 = ( srfBspSurface_t* )s_worldData.surfaces[i].data;
        
        // if this surface is not a grid
        if ( grid1->surfaceType != SF_GRID )
        {
            continue;
        }
        
        if ( grid1->lodFixed )
        {
            continue;
        }
        
        grid1->lodFixed = 2;
        
        // recursively fix other patches in the same LOD group
        FixSharedVertexLodError_r( i + 1, grid1 );
    }
}

/*
===============
idRenderSystemBSPTechLocal::StitchPatches
===============
*/
S32 idRenderSystemBSPTechLocal::StitchPatches( S32 grid1num, S32 grid2num )
{
    S32 k, l, m, n, offset1, offset2, row, column;
    F32* v1, * v2;
    srfBspSurface_t* grid1, * grid2;
    
    grid1 = ( srfBspSurface_t* )s_worldData.surfaces[grid1num].data;
    grid2 = ( srfBspSurface_t* )s_worldData.surfaces[grid2num].data;
    
    for ( n = 0; n < 2; n++ )
    {
        if ( n )
        {
            offset1 = ( grid1->height - 1 ) * grid1->width;
        }
        else
        {
            offset1 = 0;
        }
        
        if ( MergedWidthPoints( grid1, offset1 ) )
        {
            continue;
        }
        
        for ( k = 0; k < grid1->width - 2; k += 2 )
        {
            for ( m = 0; m < 2; m++ )
            {
                if ( grid2->width >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = ( grid2->height - 1 ) * grid2->width;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->width - 1; l++ )
                {
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[k + 2 + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert column into grid2 right after after column l
                    if ( m )
                    {
                        row = grid2->height - 1;
                    }
                    else
                    {
                        row = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertColumn( grid2, l + 1, row, grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    
                    grid2->lodStitched = false;
                    
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    return true;
                }
            }
            
            for ( m = 0; m < 2; m++ )
            {
                if ( grid2->height >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = grid2->width - 1;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->height - 1; l++ )
                {
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[k + 2 + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert row into grid2 right after after row l
                    if ( m )
                    {
                        column = grid2->width - 1;
                    }
                    else
                    {
                        column = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertRow( grid2, l + 1, column, grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    return true;
                }
            }
        }
    }
    
    for ( n = 0; n < 2; n++ )
    {
        if ( n )
        {
            offset1 = grid1->width - 1;
        }
        else
        {
            offset1 = 0;
        }
        
        if ( MergedHeightPoints( grid1, offset1 ) )
        {
            continue;
        }
        
        for ( k = 0; k < grid1->height - 2; k += 2 )
        {
            for ( m = 0; m < 2; m++ )
            {
                if ( grid2->width >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = ( grid2->height - 1 ) * grid2->width;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->width - 1; l++ )
                {
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert column into grid2 right after after column l
                    if ( m )
                    {
                        row = grid2->height - 1;
                    }
                    else
                    {
                        row = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertColumn( grid2, l + 1, row, grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    
                    grid2->lodStitched = false;
                    
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    
                    return true;
                }
            }
            
            for ( m = 0; m < 2; m++ )
            {
            
                if ( grid2->height >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = grid2->width - 1;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->height - 1; l++ )
                {
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert row into grid2 right after after row l
                    if ( m )
                    {
                        column = grid2->width - 1;
                    }
                    else
                    {
                        column = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertRow( grid2, l + 1, column, grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    return true;
                }
            }
        }
    }
    
    for ( n = 0; n < 2; n++ )
    {
        if ( n )
        {
            offset1 = ( grid1->height - 1 ) * grid1->width;
        }
        else
        {
            offset1 = 0;
        }
        
        if ( MergedWidthPoints( grid1, offset1 ) )
        {
            continue;
        }
        
        for ( k = grid1->width - 1; k > 1; k -= 2 )
        {
            for ( m = 0; m < 2; m++ )
            {
                if ( !grid2 || grid2->width >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = ( grid2->height - 1 ) * grid2->width;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->width - 1; l++ )
                {
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[k - 2 + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert column into grid2 right after after column l
                    if ( m )
                    {
                        row = grid2->height - 1;
                    }
                    else
                    {
                        row = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertColumn( grid2, l + 1, row, grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    
                    grid2->lodStitched = false;
                    
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    
                    return true;
                }
            }
            for ( m = 0; m < 2; m++ )
            {
                if ( !grid2 || grid2->height >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = grid2->width - 1;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->height - 1; l++ )
                {
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    v1 = grid1->verts[k - 2 + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert row into grid2 right after after row l
                    if ( m )
                    {
                        column = grid2->width - 1;
                    }
                    else
                    {
                        column = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertRow( grid2, l + 1, column, grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
                    
                    if ( !grid2 )
                    {
                        break;
                    }
                    
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    return true;
                }
            }
        }
    }
    
    for ( n = 0; n < 2; n++ )
    {
        if ( n )
        {
            offset1 = grid1->width - 1;
        }
        else
        {
            offset1 = 0;
        }
        
        if ( MergedHeightPoints( grid1, offset1 ) )
        {
            continue;
        }
        
        for ( k = grid1->height - 1; k > 1; k -= 2 )
        {
            for ( m = 0; m < 2; m++ )
            {
                if ( !grid2 || grid2->width >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = ( grid2->height - 1 ) * grid2->width;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->width - 1; l++ )
                {
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert column into grid2 right after after column l
                    if ( m )
                    {
                        row = grid2->height - 1;
                    }
                    else
                    {
                        row = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertColumn( grid2, l + 1, row, grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    
                    grid2->lodStitched = false;
                    
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    
                    return true;
                }
            }
            
            for ( m = 0; m < 2; m++ )
            {
                if ( !grid2 || grid2->height >= MAX_GRID_SIZE )
                {
                    break;
                }
                
                if ( m )
                {
                    offset2 = grid2->width - 1;
                }
                else
                {
                    offset2 = 0;
                }
                
                for ( l = 0; l < grid2->height - 1; l++ )
                {
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[1] - v2[1] ) > .1 )
                    {
                        continue;
                    }
                    
                    if ( Q_fabs( v1[2] - v2[2] ) > .1 )
                    {
                        continue;
                    }
                    
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
                    
                    if ( Q_fabs( v1[0] - v2[0] ) < .01 && Q_fabs( v1[1] - v2[1] ) < .01 && Q_fabs( v1[2] - v2[2] ) < .01 )
                    {
                        continue;
                    }
                    
                    //clientMainSystem->RefPrintf( PRINT_ALL, "found highest LoD crack between two patches\n" );
                    
                    // insert row into grid2 right after after row l
                    if ( m )
                    {
                        column = grid2->width - 1;
                    }
                    else
                    {
                        column = 0;
                    }
                    
                    idRenderSystemCurveLocal::GridInsertRow( grid2, l + 1, column, grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = ( surfaceType_t* )grid2;
                    return true;
                }
            }
        }
    }
    
    return false;
}

/*
===============
idRenderSystemBSPTechLocal::TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
S32 idRenderSystemBSPTechLocal::TryStitchingPatch( S32 grid1num )
{
    S32 j, numstitches;
    srfBspSurface_t* grid1, * grid2;
    
    numstitches = 0;
    grid1 = ( srfBspSurface_t* )s_worldData.surfaces[grid1num].data;
    
    for ( j = 0; j < s_worldData.numsurfaces; j++ )
    {
        grid2 = ( srfBspSurface_t* )s_worldData.surfaces[j].data;
        
        // if this surface is not a grid
        if ( grid2->surfaceType != SF_GRID )
        {
            continue;
        }
        
        // grids in the same LOD group should have the exact same lod radius
        if ( grid1->lodRadius != grid2->lodRadius )
        {
            continue;
        }
        
        // grids in the same LOD group should have the exact same lod origin
        if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] )
        {
            continue;
        }
        
        if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] )
        {
            continue;
        }
        
        if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] )
        {
            continue;
        }
        
        while ( StitchPatches( grid1num, j ) )
        {
            numstitches++;
        }
    }
    
    return numstitches;
}

/*
===============
idRenderSystemBSPTechLocal::StitchAllPatches
===============
*/
void idRenderSystemBSPTechLocal::StitchAllPatches( void )
{
    S32 i, stitched, numstitches;
    srfBspSurface_t* grid1;
    
    numstitches = 0;
    do
    {
        stitched = false;
        
        for ( i = 0; i < s_worldData.numsurfaces; i++ )
        {
            grid1 = ( srfBspSurface_t* )s_worldData.surfaces[i].data;
            
            // if this surface is not a grid
            if ( grid1->surfaceType != SF_GRID )
            {
                continue;
            }
            
            if ( grid1->lodStitched )
            {
                continue;
            }
            
            grid1->lodStitched = true;
            stitched = true;
            
            numstitches += TryStitchingPatch( i );
        }
    }
    while ( stitched );
    
    clientMainSystem->RefPrintf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
idRenderSystemBSPTechLocal::MovePatchSurfacesToHunk
===============
*/
void idRenderSystemBSPTechLocal::MovePatchSurfacesToHunk( void )
{
    S32 i;
    srfBspSurface_t* grid;
    
    for ( i = 0; i < s_worldData.numsurfaces; i++ )
    {
        void* copyFrom;
        
        grid = ( srfBspSurface_t* )s_worldData.surfaces[i].data;
        
        // if this surface is not a grid
        if ( grid->surfaceType != SF_GRID )
        {
            continue;
        }
        
        copyFrom = grid->widthLodError;
        grid->widthLodError = reinterpret_cast<F32*>( memorySystem->Alloc( grid->width * 4, h_low ) );
        ::memcpy( grid->widthLodError, copyFrom, grid->width * 4 );
        memorySystem->Free( copyFrom );
        
        copyFrom = grid->heightLodError;
        grid->heightLodError = reinterpret_cast<F32*>( memorySystem->Alloc( grid->height * 4, h_low ) );
        ::memcpy( grid->heightLodError, copyFrom, grid->height * 4 );
        memorySystem->Free( copyFrom );
        
        copyFrom = grid->indexes;
        grid->indexes = reinterpret_cast<U32*>( memorySystem->Alloc( grid->numIndexes * sizeof( U32 ), h_low ) );
        ::memcpy( grid->indexes, copyFrom, grid->numIndexes * sizeof( U32 ) );
        memorySystem->Free( copyFrom );
        
        copyFrom = grid->verts;
        grid->verts = reinterpret_cast<srfVert_t*>( memorySystem->Alloc( grid->numVerts * sizeof( srfVert_t ), h_low ) );
        ::memcpy( grid->verts, copyFrom, grid->numVerts * sizeof( srfVert_t ) );
        memorySystem->Free( copyFrom );
    }
}

/*
===============
idRenderSystemBSPTechLocal::LoadSurfaces
===============
*/
void idRenderSystemBSPTechLocal::LoadSurfaces( lump_t* surfs, lump_t* verts, lump_t* indexLump )
{
    S32* indexes, count, numFaces, numMeshes, numTriSurfs, numFlares, i;
    F32* hdrVertColors = NULL;
    dsurface_t* in;
    msurface_t* out = nullptr;
    drawVert_t* dv;
    
    numFaces = 0;
    numMeshes = 0;
    numTriSurfs = 0;
    numFlares = 0;
    
    if ( surfs->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadSurfaces: funny lump size in %s", s_worldData.name );
    }
    
    count = surfs->filelen / sizeof( *in );
    
    dv = ( drawVert_t* )( fileBase + verts->fileofs );
    if ( verts->filelen % sizeof( *dv ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadSurfaces: funny lump size in %s", s_worldData.name );
    }
    
    indexes = ( S32* )( fileBase + indexLump->fileofs );
    if ( indexLump->filelen % sizeof( *indexes ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadSurfaces: funny lump size in %s", s_worldData.name );
    }
    
    out = reinterpret_cast<msurface_t*>( memorySystem->Alloc( count * sizeof( *out ), h_low ) );
    
    s_worldData.surfaces = out;
    s_worldData.numsurfaces = count;
    s_worldData.surfacesViewCount = reinterpret_cast<S32*>( memorySystem->Alloc( count * sizeof( *s_worldData.surfacesViewCount ), h_low ) );
    s_worldData.surfacesDlightBits = reinterpret_cast<S32*>( memorySystem->Alloc( count * sizeof( *s_worldData.surfacesDlightBits ), h_low ) );
    s_worldData.surfacesPshadowBits = reinterpret_cast<S32*>( memorySystem->Alloc( count * sizeof( *s_worldData.surfacesPshadowBits ), h_low ) );
    
    // load hdr vertex colors
    if ( r_hdr->integer )
    {
        UTF8 filename[MAX_QPATH];
        S32 size;
        
        Q_snprintf( filename, sizeof( filename ), "maps/%s/vertlight.raw", s_worldData.baseName );
        //clientMainSystem->RefPrintf(PRINT_ALL, "looking for %s\n", filename);
        
        size = fileSystem->ReadFile( filename, ( void** )&hdrVertColors );
        
        if ( hdrVertColors )
        {
            //clientMainSystem->RefPrintf(PRINT_ALL, "Found!\n");
            if ( size != sizeof( F32 ) * 3 * ( verts->filelen / sizeof( *dv ) ) )
            {
                Com_Error( ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, ( S32 )( ( sizeof( F32 ) ) * 3 * ( verts->filelen / sizeof( *dv ) ) ) );
            }
        }
    }
    
    // Two passes, allocate surfaces first, then load them full of data
    // This ensures surfaces are close together to reduce L2 cache misses when using VAOs,
    // which don't actually use the verts and indexes
    in = ( dsurface_t* )( fileBase + surfs->fileofs );
    out = s_worldData.surfaces;
    
    for ( i = 0; i < count; i++, in++, out++ )
    {
        switch ( LittleLong( in->surfaceType ) )
        {
            case MST_PATCH:
                out->data = reinterpret_cast<surfaceType_t*>( memorySystem->Alloc( sizeof( srfBspSurface_t ), h_low ) );
                break;
            case MST_TRIANGLE_SOUP:
                out->data = reinterpret_cast<surfaceType_t*>( memorySystem->Alloc( sizeof( srfBspSurface_t ), h_low ) );
                break;
            case MST_PLANAR:
                out->data = reinterpret_cast<surfaceType_t*>( memorySystem->Alloc( sizeof( srfBspSurface_t ), h_low ) );
                break;
            case MST_FLARE:
                out->data = reinterpret_cast<surfaceType_t*>( memorySystem->Alloc( sizeof( srfFlare_t ), h_low ) );
                break;
            default:
                break;
        }
    }
    
    in = ( dsurface_t* )( fileBase + surfs->fileofs );
    out = s_worldData.surfaces;
    for ( i = 0; i < count; i++, in++, out++ )
    {
        switch ( LittleLong( in->surfaceType ) )
        {
            case MST_PATCH:
                ParseMesh( in, dv, hdrVertColors, out );
                numMeshes++;
                break;
            case MST_TRIANGLE_SOUP:
                ParseTriSurf( in, dv, hdrVertColors, out, indexes );
                numTriSurfs++;
                break;
            case MST_PLANAR:
                ParseFace( in, dv, hdrVertColors, out, indexes );
                numFaces++;
                break;
            case MST_FLARE:
                ParseFlare( in, dv, out, indexes );
                numFlares++;
                break;
            default:
                Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::Bad surfaceType" );
        }
    }
    
    if ( hdrVertColors )
    {
        fileSystem->FreeFile( hdrVertColors );
    }
    
    StitchAllPatches();
    
    FixSharedVertexLodError();
    
    MovePatchSurfacesToHunk();
    
    clientMainSystem->RefPrintf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", numFaces, numMeshes, numTriSurfs, numFlares );
}

/*
=================
idRenderSystemBSPTechLocal::LoadSubmodels
=================
*/
void idRenderSystemBSPTechLocal::LoadSubmodels( lump_t* l )
{
    S32 i, j, count;
    dmodel_t* in;
    bmodel_t* out;
    
    in = ( dmodel_t* )( fileBase + l->fileofs );
    if ( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
    }
    
    count = l->filelen / sizeof( *in );
    
    s_worldData.numBModels = count;
    s_worldData.bmodels = out = reinterpret_cast<bmodel_t*>( memorySystem->Alloc( count * sizeof( *out ), h_low ) );
    
    for ( i = 0; i < count; i++, in++, out++ )
    {
        model_t* model;
        
        model = idRenderSystemModelLocal::AllocModel();
        
        // this should never happen
        assert( model != NULL );
        
        if ( model == NULL )
        {
            Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadSubmodels: idRenderSystemModelLocal::AllocModel() failed" );
        }
        
        model->type = MOD_BRUSH;
        model->bmodel = out;
        Q_snprintf( model->name, sizeof( model->name ), "*%d", i );
        
        for ( j = 0; j < 3; j++ )
        {
            out->bounds[0][j] = LittleFloat( in->mins[j] );
            out->bounds[1][j] = LittleFloat( in->maxs[j] );
        }
        
        out->firstSurface = LittleLong( in->firstSurface );
        out->numSurfaces = LittleLong( in->numSurfaces );
        
        if ( i == 0 )
        {
            // Add this for limiting VAO surface creation
            s_worldData.numWorldSurfaces = out->numSurfaces;
        }
    }
}

/*
=================
idRenderSystemBSPTechLocal::BSPSetParent
=================
*/
void idRenderSystemBSPTechLocal::BSPSetParent( mnode_t* node, mnode_t* parent )
{
    node->parent = parent;
    if ( node->contents != -1 )
    {
        return;
    }
    
    BSPSetParent( node->children[0], node );
    BSPSetParent( node->children[1], node );
}

/*
=================
idRenderSystemBSPTechLocal::LoadNodesAndLeafs
=================
*/
void idRenderSystemBSPTechLocal::LoadNodesAndLeafs( lump_t* nodeLump, lump_t* leafLump )
{
    S32	i, j, p, numNodes, numLeafs;
    dnode_t* in;
    dleaf_t* inLeaf;
    mnode_t* out = nullptr;
    
    in = ( dnode_t* )( fileBase + nodeLump->fileofs );
    if ( nodeLump->filelen % sizeof( dnode_t ) || leafLump->filelen % sizeof( dleaf_t ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadNodesAndLeafs: funny lump size in %s", s_worldData.name );
    }
    
    numNodes = nodeLump->filelen / sizeof( dnode_t );
    numLeafs = leafLump->filelen / sizeof( dleaf_t );
    
    out = reinterpret_cast<mnode_t*>( memorySystem->Alloc( ( numNodes + numLeafs ) * sizeof( *out ), h_low ) );
    
    s_worldData.nodes = out;
    s_worldData.numnodes = numNodes + numLeafs;
    s_worldData.numDecisionNodes = numNodes;
    
    // load nodes
    for ( i = 0; i < numNodes; i++, in++, out++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            out->mins[j] = ( F32 )LittleLong( in->mins[j] );
            out->maxs[j] = ( F32 )LittleLong( in->maxs[j] );
        }
        
        p = LittleLong( in->planeNum );
        out->plane = s_worldData.planes + p;
        
        // differentiate from leafs
        out->contents = CONTENTS_NODE;
        
        for ( j = 0; j < 2; j++ )
        {
            p = LittleLong( in->children[j] );
            if ( p >= 0 )
            {
                out->children[j] = s_worldData.nodes + p;
            }
            else
            {
                out->children[j] = s_worldData.nodes + numNodes + ( -1 - p );
            }
        }
    }
    
    // load leafs
    inLeaf = ( dleaf_t* )( fileBase + leafLump->fileofs );
    for ( i = 0; i < numLeafs; i++, inLeaf++, out++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            out->mins[j] = ( F32 )LittleLong( inLeaf->mins[j] );
            out->maxs[j] = ( F32 )LittleLong( inLeaf->maxs[j] );
        }
        
        out->cluster = LittleLong( inLeaf->cluster );
        out->area = LittleLong( inLeaf->area );
        
        if ( out->cluster >= s_worldData.numClusters )
        {
            s_worldData.numClusters = out->cluster + 1;
        }
        
        out->firstmarksurface = LittleLong( inLeaf->firstLeafSurface );
        out->nummarksurfaces = LittleLong( inLeaf->numLeafSurfaces );
    }
    
    // chain decendants
    BSPSetParent( s_worldData.nodes, NULL );
}

/*
=================
idRenderSystemBSPTechLocal::LoadShaders
=================
*/
void idRenderSystemBSPTechLocal::LoadShaders( lump_t* l )
{
    S32 i, count;
    dshader_t* in, * out = nullptr;
    
    in = ( dshader_t* )( fileBase + l->fileofs );
    if ( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadShaders: funny lump size in %s", s_worldData.name );
    }
    
    count = l->filelen / sizeof( *in );
    out = reinterpret_cast<dshader_t*>( memorySystem->Alloc( count * sizeof( *out ), h_low ) );
    
    s_worldData.shaders = out;
    s_worldData.numShaders = count;
    
    ::memcpy( out, in, count * sizeof( *out ) );
    
    for ( i = 0; i < count; i++ )
    {
        out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
        out[i].contentFlags = LittleLong( out[i].contentFlags );
    }
}

/*
=================
idRenderSystemBSPTechLocal::LoadMarksurfaces
=================
*/
void idRenderSystemBSPTechLocal::LoadMarksurfaces( lump_t* l )
{
    S32	i, j, count, * in, * out = nullptr;
    
    in = ( S32* )( fileBase + l->fileofs );
    if ( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadMarksurfaces: funny lump size in %s", s_worldData.name );
    }
    
    count = l->filelen / sizeof( *in );
    out = reinterpret_cast<S32*>( memorySystem->Alloc( count * sizeof( *out ), h_low ) );
    
    s_worldData.marksurfaces = out;
    s_worldData.nummarksurfaces = count;
    
    for ( i = 0; i < count; i++ )
    {
        j = LittleLong( in[i] );
        out[i] = j;
    }
}

/*
=================
idRenderSystemBSPTechLocal::LoadPlanes
=================
*/
void idRenderSystemBSPTechLocal::LoadPlanes( lump_t* l )
{
    S32	i, j, bits, count;
    cplane_t* out = nullptr;
    dplane_t* in;
    
    in = ( dplane_t* )( fileBase + l->fileofs );
    if ( l->filelen % sizeof( *in ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadPlanes: funny lump size in %s", s_worldData.name );
    }
    
    count = l->filelen / sizeof( *in );
    out = reinterpret_cast<cplane_t*>( memorySystem->Alloc( count * 2 * sizeof( *out ), h_low ) );
    
    s_worldData.planes = out;
    s_worldData.numplanes = count;
    
    for ( i = 0; i < count; i++, in++, out++ )
    {
        bits = 0;
        
        for ( j = 0; j < 3; j++ )
        {
            out->normal[j] = LittleFloat( in->normal[j] );
            if ( out->normal[j] < 0 )
            {
                bits |= 1 << j;
            }
        }
        
        out->dist = LittleFloat( in->dist );
        out->type = PlaneTypeForNormal( out->normal );
        out->signbits = bits;
    }
}

/*
=================
idRenderSystemBSPTechLocal::LoadFogs
=================
*/
void idRenderSystemBSPTechLocal::LoadFogs( lump_t* l, lump_t* brushesLump, lump_t* sidesLump )
{
    S32 i, count, brushesCount, sidesCount, sideNum, planeNum, firstSide;
    F32 d;
    fog_t* out;
    dfog_t* fogs;
    dbrush_t* brushes, * brush;
    dbrushside_t* sides;
    shader_t* shader;
    
    fogs = ( dfog_t* )( fileBase + l->fileofs );
    if ( l->filelen % sizeof( *fogs ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadFogs: funny lump size in %s", s_worldData.name );
    }
    count = l->filelen / sizeof( *fogs );
    
    // create fog strucutres for them
    s_worldData.numfogs = count + 1;
    s_worldData.fogs = reinterpret_cast<fog_t*>( memorySystem->Alloc( s_worldData.numfogs * sizeof( *out ), h_low ) );
    out = s_worldData.fogs + 1;
    
    if ( !count )
    {
        return;
    }
    
    brushes = ( dbrush_t* )( fileBase + brushesLump->fileofs );
    if ( brushesLump->filelen % sizeof( *brushes ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadFogs: funny lump size in %s", s_worldData.name );
    }
    
    brushesCount = brushesLump->filelen / sizeof( *brushes );
    
    sides = ( dbrushside_t* )( fileBase + sidesLump->fileofs );
    if ( sidesLump->filelen % sizeof( *sides ) )
    {
        Com_Error( ERR_DROP, "idRenderSystemBSPTechLocal::LoadFogs: funny lump size in %s", s_worldData.name );
    }
    
    sidesCount = sidesLump->filelen / sizeof( *sides );
    
    for ( i = 0; i < count; i++, fogs++ )
    {
        out->originalBrushNumber = LittleLong( fogs->brushNum );
        
        if ( out->originalBrushNumber >= brushesCount )
        {
            Com_Error( ERR_DROP, "fog brushNumber out of range" );
        }
        brush = brushes + out->originalBrushNumber;
        
        firstSide = LittleLong( brush->firstSide );
        
        if ( firstSide > sidesCount - 6 )
        {
            Com_Error( ERR_DROP, "fog brush sideNumber out of range" );
        }
        
        // brushes are always sorted with the axial sides first
        sideNum = firstSide + 0;
        planeNum = LittleLong( sides[sideNum].planeNum );
        out->bounds[0][0] = -s_worldData.planes[planeNum].dist;
        
        sideNum = firstSide + 1;
        planeNum = LittleLong( sides[sideNum].planeNum );
        out->bounds[1][0] = s_worldData.planes[planeNum].dist;
        
        sideNum = firstSide + 2;
        planeNum = LittleLong( sides[sideNum].planeNum );
        out->bounds[0][1] = -s_worldData.planes[planeNum].dist;
        
        sideNum = firstSide + 3;
        planeNum = LittleLong( sides[sideNum].planeNum );
        out->bounds[1][1] = s_worldData.planes[planeNum].dist;
        
        sideNum = firstSide + 4;
        planeNum = LittleLong( sides[sideNum].planeNum );
        out->bounds[0][2] = -s_worldData.planes[planeNum].dist;
        
        sideNum = firstSide + 5;
        planeNum = LittleLong( sides[sideNum].planeNum );
        out->bounds[1][2] = s_worldData.planes[planeNum].dist;
        
        // get information from the shader for fog parameters
        shader = idRenderSystemShaderLocal::FindShader( fogs->shader, LIGHTMAP_NONE, true );
        
        out->parms = shader->fogParms;
        
        out->colorInt = ColorBytes4( shader->fogParms.color[0], shader->fogParms.color[1], shader->fogParms.color[2], 1.0 );
        
        d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
        out->tcScale = 1.0f / ( d * 8 );
        
        // set the gradient vector
        sideNum = LittleLong( fogs->visibleSide );
        
        if ( sideNum == -1 )
        {
            out->hasSurface = false;
        }
        else
        {
            out->hasSurface = true;
            planeNum = LittleLong( sides[firstSide + sideNum].planeNum );
            VectorSubtract( vec3_origin, s_worldData.planes[planeNum].normal, out->surface );
            out->surface[3] = -s_worldData.planes[planeNum].dist;
        }
        
        out++;
    }
}

/*
================
idRenderSystemBSPTechLocal::LoadLightGrid
================
*/
void idRenderSystemBSPTechLocal::LoadLightGrid( lump_t* l )
{
    S32	i, numGridPoints;
    vec3_t	maxs;
    world_t* w;
    F32* wMins, * wMaxs;
    
    w = &s_worldData;
    
    w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
    w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
    w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];
    
    wMins = w->bmodels[0].bounds[0];
    wMaxs = w->bmodels[0].bounds[1];
    
    for ( i = 0; i < 3; i++ )
    {
        w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
        maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
        w->lightGridBounds[i] = ( S32 )( ( maxs[i] - w->lightGridOrigin[i] ) / w->lightGridSize[i] + 1 );
    }
    
    numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];
    
    if ( l->filelen != numGridPoints * 8 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
        w->lightGridData = NULL;
        return;
    }
    
    w->lightGridData = reinterpret_cast<U8*>( memorySystem->Alloc( l->filelen, h_low ) );
    ::memcpy( w->lightGridData, ( void* )( fileBase + l->fileofs ), l->filelen );
    
    // deal with overbright bits
    for ( i = 0; i < numGridPoints; i++ )
    {
        ColorShiftLightingBytes( &w->lightGridData[i * 8], &w->lightGridData[i * 8] );
        ColorShiftLightingBytes( &w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3] );
    }
    
    // load hdr lightgrid
    if ( r_hdr->integer )
    {
        UTF8 filename[MAX_QPATH];
        F32* hdrLightGrid;
        S32 size;
        
        Q_snprintf( filename, sizeof( filename ), "maps/%s/lightgrid.raw", s_worldData.baseName );
        //clientMainSystem->RefPrintf(PRINT_ALL, "looking for %s\n", filename);
        
        size = fileSystem->ReadFile( filename, ( void** )&hdrLightGrid );
        
        if ( hdrLightGrid )
        {
            //clientMainSystem->RefPrintf(PRINT_ALL, "found!\n");
            
            if ( size != sizeof( F32 ) * 6 * numGridPoints )
            {
                Com_Error( ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, ( S32 )( sizeof( F32 ) ) * 6 * numGridPoints );
            }
            
            w->lightGrid16 = reinterpret_cast<U16*>( memorySystem->Alloc( sizeof( w->lightGrid16 ) * 6 * numGridPoints, h_low ) );
            
            for ( i = 0; i < numGridPoints; i++ )
            {
                vec4_t c;
                
                c[0] = hdrLightGrid[i * 6];
                c[1] = hdrLightGrid[i * 6 + 1];
                c[2] = hdrLightGrid[i * 6 + 2];
                c[3] = 1.0f;
                
                ColorShiftLightingFloats( c, c );
                ColorToRGB16( c, &w->lightGrid16[i * 6] );
                
                c[0] = hdrLightGrid[i * 6 + 3];
                c[1] = hdrLightGrid[i * 6 + 4];
                c[2] = hdrLightGrid[i * 6 + 5];
                c[3] = 1.0f;
                
                ColorShiftLightingFloats( c, c );
                ColorToRGB16( c, &w->lightGrid16[i * 6 + 3] );
            }
        }
        else if ( 0 )
        {
            // promote 8-bit lightgrid to 16-bit
            w->lightGrid16 = reinterpret_cast<U16*>( memorySystem->Alloc( sizeof( w->lightGrid16 ) * 6 * numGridPoints, h_low ) );
            
            for ( i = 0; i < numGridPoints; i++ )
            {
                w->lightGrid16[i * 6] = w->lightGridData[i * 8] * 257;
                w->lightGrid16[i * 6 + 1] = w->lightGridData[i * 8 + 1] * 257;
                w->lightGrid16[i * 6 + 2] = w->lightGridData[i * 8 + 2] * 257;
                w->lightGrid16[i * 6 + 3] = w->lightGridData[i * 8 + 3] * 257;
                w->lightGrid16[i * 6 + 4] = w->lightGridData[i * 8 + 4] * 257;
                w->lightGrid16[i * 6 + 5] = w->lightGridData[i * 8 + 5] * 257;
            }
        }
        
        if ( hdrLightGrid )
        {
            fileSystem->FreeFile( hdrLightGrid );
        }
    }
}

/*
================
idRenderSystemBSPTechLocal::LoadEntities
================
*/
void idRenderSystemBSPTechLocal::LoadEntities( lump_t* l )
{
    UTF8* p, * token, * s, keyname[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];
    world_t* w;
    
    w = &s_worldData;
    w->lightGridSize[0] = 64;
    w->lightGridSize[1] = 64;
    w->lightGridSize[2] = 128;
    
    p = reinterpret_cast<UTF8*>( ( fileBase + l->fileofs ) );
    
    // store for reference by the cgame
    w->entityString = reinterpret_cast<UTF8*>( ( memorySystem->Alloc( l->filelen + 1, h_low ) ) );
    ::strcpy( w->entityString, p );
    w->entityParsePoint = w->entityString;
    
    p = w->entityString;
    
    token = COM_ParseExt2( &p, true );
    if ( !*token || *token != '{' )
    {
        return;
    }
    
    // only parse the world spawn
    while ( 1 )
    {
        // parse key
        token = COM_ParseExt2( &p, true );
        
        if ( !*token || *token == '}' )
        {
            break;
        }
        Q_strncpyz( keyname, token, sizeof( keyname ) );
        
        // parse value
        token = COM_ParseExt2( &p, true );
        
        if ( !*token || *token == '}' )
        {
            break;
        }
        
        Q_strncpyz( value, token, sizeof( value ) );
        
        // check for remapping of shaders for vertex lighting
        if ( !Q_strncmp( keyname, "vertexremapshader", ( S32 )::strlen( "vertexremapshader" ) ) )
        {
            s = ::strchr( value, ';' );
            
            if ( !s )
            {
                clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
                break;
            }
            
            *s++ = 0;
            
            if ( r_vertexLight->integer )
            {
                renderSystemLocal.RemapShader( value, s, "0" );
            }
            
            continue;
        }
        
        // check for remapping of shaders
        if ( !Q_strncmp( keyname, "remapshader", ( S32 )::strlen( "remapshader" ) ) )
        {
            s = ::strchr( value, ';' );
            if ( !s )
            {
                clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
                break;
            }
            
            *s++ = 0;
            renderSystemLocal.RemapShader( value, s, "0" );
            
            continue;
        }
        
        // check for a different grid size
        if ( !Q_stricmp( keyname, "gridsize" ) )
        {
            ::sscanf( value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
            continue;
        }
        
        // check for auto exposure
        if ( !Q_stricmp( keyname, "autoExposureMinMax" ) )
        {
            ::sscanf( value, "%f %f", &tr.autoExposureMinMax[0], &tr.autoExposureMinMax[1] );
            continue;
        }
    }
}

/*
=================
idRenderSystemLocal::GetEntityToken
=================
*/
bool idRenderSystemLocal::GetEntityToken( UTF8* buffer, S32 size )
{
    UTF8* s;
    world_t* worldData = &s_worldData;
    
    if ( size == -1 )
    {
        //force reset
        worldData->entityParsePoint = worldData->entityString;
        return true;
    }
    
    s = COM_Parse2( &worldData->entityParsePoint );
    Q_strncpyz( buffer, s, size );
    
    if ( !worldData->entityParsePoint && !s[0] )
    {
        worldData->entityParsePoint = worldData->entityString;
        return false;
    }
    else
    {
        return true;
    }
}

bool idRenderSystemBSPTechLocal::ParseSpawnVars( UTF8* spawnVarChars, S32 maxSpawnVarChars, S32* numSpawnVars, UTF8* spawnVars[MAX_SPAWN_VARS][2] )
{
    S32	numSpawnVarChars = 0;
    UTF8 keyname[MAX_TOKEN_CHARS];
    UTF8 com_token[MAX_TOKEN_CHARS];
    
    *numSpawnVars = 0;
    
    // parse the opening brace
    if ( !renderSystemLocal.GetEntityToken( com_token, sizeof( com_token ) ) )
    {
        // end of spawn string
        return false;
    }
    
    if ( com_token[0] != '{' )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemBSPTechLocal::ParseSpawnVars: found %s when expecting {\n", com_token );
        return false;
    }
    
    // go through all the key / value pairs
    while ( 1 )
    {
        S32 keyLength, tokenLength;
        
        // parse key
        if ( !renderSystemLocal.GetEntityToken( keyname, sizeof( keyname ) ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
            return false;
        }
        
        if ( keyname[0] == '}' )
        {
            break;
        }
        
        // parse value
        if ( !renderSystemLocal.GetEntityToken( com_token, sizeof( com_token ) ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
            return false;
        }
        
        if ( com_token[0] == '}' )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_ParseSpawnVars: closing brace without data\n" );
            return false;
        }
        
        if ( *numSpawnVars == MAX_SPAWN_VARS )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VARS\n" );
            return false;
        }
        
        keyLength = ( S32 )::strlen( keyname ) + 1;
        tokenLength = ( S32 )::strlen( com_token ) + 1;
        
        if ( numSpawnVarChars + keyLength + tokenLength > maxSpawnVarChars )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VAR_CHARS\n" );
            return false;
        }
        
        strcpy( spawnVarChars + numSpawnVarChars, keyname );
        spawnVars[*numSpawnVars][0] = spawnVarChars + numSpawnVarChars;
        numSpawnVarChars += keyLength;
        
        strcpy( spawnVarChars + numSpawnVarChars, com_token );
        spawnVars[*numSpawnVars][1] = spawnVarChars + numSpawnVarChars;
        numSpawnVarChars += tokenLength;
        
        ( *numSpawnVars )++;
    }
    
    return true;
}

void idRenderSystemBSPTechLocal::LoadCubemapEntities( StringEntry cubemapEntityName )
{
    S32 numSpawnVars, numCubemaps = 0;
    UTF8 spawnVarChars[2048], * spawnVars[MAX_SPAWN_VARS][2];
    
    if ( !Q_strncmp( cubemapEntityName, "misc_skyportal", ( S32 )::strlen( "misc_skyportal" ) ) )
    {
        ::memset( &tr.skyboxCubemap, 0, sizeof( tr.skyboxCubemap ) );
        
        while ( ParseSpawnVars( spawnVarChars, sizeof( spawnVarChars ), &numSpawnVars, spawnVars ) )
        {
            S32 i;
            F32 parallaxRadius = 1000.0f;
            UTF8 name[MAX_QPATH];
            vec3_t origin;
            bool isCubemap = false;
            bool originSet = false;
            
            name[0] = '\0';
            
            for ( i = 0; i < numSpawnVars; i++ )
            {
                if ( !Q_stricmp( spawnVars[i][0], "classname" ) && !Q_stricmp( spawnVars[i][1], cubemapEntityName ) )
                {
                    isCubemap = true;
                }
                
                if ( !Q_stricmp( spawnVars[i][0], "origin" ) )
                {
                    sscanf( spawnVars[i][1], "%f %f %f", &origin[0], &origin[1], &origin[2] );
                    originSet = true;
                }
            }
            
            if ( isCubemap && originSet )
            {
                cubemap_t* cubemap = &tr.cubemaps[numCubemaps];
                Q_strncpyz( cubemap->name, "SKYBOX_CUBEMAP", MAX_QPATH );
                VectorCopy( origin, cubemap->origin );
                cubemap->parallaxRadius = parallaxRadius;
            }
        }
        return;
    }
    
    // count cubemaps
    numCubemaps = 0;
    
    while ( ParseSpawnVars( spawnVarChars, sizeof( spawnVarChars ), &numSpawnVars, spawnVars ) )
    {
        S32 i;
        
        for ( i = 0; i < numSpawnVars; i++ )
        {
            if ( !Q_stricmp( spawnVars[i][0], "classname" ) && !Q_stricmp( spawnVars[i][1], cubemapEntityName ) )
            {
                numCubemaps++;
            }
        }
    }
    
    if ( !numCubemaps )
    {
        return;
    }
    
    tr.numCubemaps = numCubemaps;
    tr.cubemaps = reinterpret_cast<cubemap_t*>( memorySystem->Alloc( tr.numCubemaps * sizeof( *tr.cubemaps ), h_low ) );
    ::memset( tr.cubemaps, 0, tr.numCubemaps * sizeof( *tr.cubemaps ) );
    
    numCubemaps = 0;
    while ( ParseSpawnVars( spawnVarChars, sizeof( spawnVarChars ), &numSpawnVars, spawnVars ) )
    {
        S32 i;
        F32 parallaxRadius = 1000.0f;
        UTF8 name[MAX_QPATH];
        vec3_t origin;
        bool originSet = false, isCubemap = false;
        
        name[0] = '\0';
        for ( i = 0; i < numSpawnVars; i++ )
        {
            if ( !Q_stricmp( spawnVars[i][0], "classname" ) && !Q_stricmp( spawnVars[i][1], cubemapEntityName ) )
            {
                isCubemap = true;
            }
            
            if ( !Q_stricmp( spawnVars[i][0], "name" ) )
            {
                Q_strncpyz( name, spawnVars[i][1], MAX_QPATH );
            }
            
            if ( !Q_stricmp( spawnVars[i][0], "origin" ) )
            {
                ::sscanf( spawnVars[i][1], "%f %f %f", &origin[0], &origin[1], &origin[2] );
                originSet = true;
            }
            else if ( !Q_stricmp( spawnVars[i][0], "radius" ) )
            {
                ::sscanf( spawnVars[i][1], "%f", &parallaxRadius );
            }
        }
        
        if ( isCubemap && originSet )
        {
            cubemap_t* cubemap = &tr.cubemaps[numCubemaps];
            Q_strncpyz( cubemap->name, name, MAX_QPATH );
            VectorCopy( origin, cubemap->origin );
            cubemap->parallaxRadius = parallaxRadius;
            numCubemaps++;
        }
    }
}

void idRenderSystemBSPTechLocal::AssignCubemapsToWorldSurfaces( void )
{
    S32 i;
    world_t* w;
    
    w = &s_worldData;
    
    for ( i = 0; i < w->numsurfaces; i++ )
    {
        msurface_t* surf = &w->surfaces[i];
        vec3_t surfOrigin;
        
        if ( surf->cullinfo.type & CULLINFO_SPHERE )
        {
            VectorCopy( surf->cullinfo.localOrigin, surfOrigin );
        }
        else if ( surf->cullinfo.type & CULLINFO_BOX )
        {
            surfOrigin[0] = ( surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0] ) * 0.5f;
            surfOrigin[1] = ( surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1] ) * 0.5f;
            surfOrigin[2] = ( surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2] ) * 0.5f;
        }
        else
        {
            //clientMainSystem->RefPrintf(PRINT_ALL, "surface %d has no cubemap\n", i);
            continue;
        }
        
        surf->cubemapIndex = idRenderSystemLightLocal::CubemapForPoint( surfOrigin );
        //clientMainSystem->RefPrintf(PRINT_ALL, "surface %d has cubemap %d\n", i, surf->cubemapIndex);
    }
}

void idRenderSystemBSPTechLocal::LoadCubemaps( void )
{
    S32 i, flags = IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_NOLIGHTSCALE | IMGFLAG_CUBEMAP;
    
    for ( i = 0; i < tr.numCubemaps; i++ )
    {
        UTF8 filename[MAX_QPATH];
        cubemap_t* cubemap = &tr.cubemaps[i];
        
        Q_snprintf( filename, MAX_QPATH, "cubemaps/%s/%03d.dds", tr.world->baseName, i );
        
        cubemap->image = idRenderSystemImageLocal::FindImageFile( filename, IMGTYPE_COLORALPHA, flags );
    }
}

void idRenderSystemBSPTechLocal::RenderMissingCubemaps( void )
{
    if ( tr.cubemaps[0].image )
    {
        return;
    }
    
    U32 cubemapFormat = GL_RGBA8;
    
    if ( r_hdr->integer )
    {
        cubemapFormat = GL_RGBA16F;
    }
    
    S32 numberOfBounces = 2;
    for ( S32 k = 0; k <= numberOfBounces; k++ )
    {
        bool bounce = bool( k != 0 );
        for ( S32 i = 0; i < tr.numCubemaps; i++ )
        {
            if ( !bounce )
            {
                tr.cubemaps[i].image = idRenderSystemImageLocal::CreateImage( va( "*cubeMap%d", i ), NULL, r_cubemapSize->integer, r_cubemapSize->integer,
                                       IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat );
            }
            
            for ( S32 j = 0; j < 6; j++ )
            {
                renderSystemLocal.ClearScene();
                idRenderSystemMainLocal::RenderCubemapSide( &tr.cubemaps[i], i, j, false, bounce );
                idRenderSystemCmdsLocal::IssuePendingRenderCommands();
                idRenderSystemSceneLocal::InitNextFrame();
            }
            
            for ( S32 j = 0; j < 6; j++ )
            {
                renderSystemLocal.ClearScene();
                idRenderSystemCmdsLocal::AddConvolveCubemapCmd( &tr.cubemaps[i], i, j );
                idRenderSystemCmdsLocal::IssuePendingRenderCommands();
                idRenderSystemSceneLocal::InitNextFrame();
            }
        }
    }
}

void idRenderSystemBSPTechLocal::CalcVertexLightDirs( void )
{
    S32 i, k;
    msurface_t* surface;
    
    for ( k = 0, surface = &s_worldData.surfaces[0]; k < s_worldData.numsurfaces /* s_worldData.numWorldSurfaces */; k++, surface++ )
    {
        srfBspSurface_t* bspSurf = ( srfBspSurface_t* )surface->data;
        
        switch ( bspSurf->surfaceType )
        {
            case SF_FACE:
            case SF_GRID:
            case SF_TRIANGLES:
                for ( i = 0; i < bspSurf->numVerts; i++ )
                {
                    vec3_t lightDir;
                    vec3_t normal;
                    
                    idRenderSystemVaoLocal::VaoUnpackNormal( normal, bspSurf->verts[i].normal );
                    idRenderSystemLightLocal::LightDirForPoint( bspSurf->verts[i].xyz, lightDir, normal, &s_worldData );
                    idRenderSystemVaoLocal::VaoPackNormal( bspSurf->verts[i].lightdir, lightDir );
                }
                
                break;
                
            default:
                break;
        }
    }
}


/*
=================
idRenderSystemLocal::LoadWorld

Called directly from cgame
=================
*/
void idRenderSystemLocal::LoadWorld( StringEntry name )
{
    S32			i;
    dheader_t* header;
    union
    {
        U8* b;
        void* v;
    } buffer;
    U8* startMarker;
    
    if ( tr.worldMapLoaded )
    {
        Com_Error( ERR_DROP, "ERROR: attempted to redundantly load world map" );
    }
    
    // set default map light scale
    tr.sunShadowScale = 0.5f;
    
    // set default sun direction to be used if it isn't
    // overridden by a shader
    tr.sunDirection[0] = 0.45f;
    tr.sunDirection[1] = 0.3f;
    tr.sunDirection[2] = 0.9f;
    
    tr.sunShader = nullptr;
    
    VectorNormalize( tr.sunDirection );
    
    // set default autoexposure settings
    tr.autoExposureMinMax[0] = -2.0f;
    tr.autoExposureMinMax[1] = 2.0f;
    
    // set default tone mapping settings
    tr.toneMinAvgMaxLevel[0] = -8.0f;
    tr.toneMinAvgMaxLevel[1] = -2.0f;
    tr.toneMinAvgMaxLevel[2] = 0.0f;
    
    // reset last cascade sun direction so last shadow cascade is rerendered
    VectorClear( tr.lastCascadeSunDirection );
    
    tr.worldMapLoaded = true;
    
    // load it
    fileSystem->ReadFile( name, &buffer.v );
    if ( !buffer.b )
    {
        Com_Error( ERR_DROP, "idRenderSystemLocal::LoadWorldMap: %s not found", name );
    }
    
    // clear tr.world so if the level fails to load, the next
    // try will not look at the partially loaded version
    tr.world = NULL;
    
    ::memset( &s_worldData, 0, sizeof( s_worldData ) );
    Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );
    
    COM_StripExtension2( COM_SkipPath( s_worldData.name ), s_worldData.baseName, sizeof( s_worldData.baseName ) );
    
    startMarker = ( U8* )memorySystem->Alloc( 0, h_low );
    c_gridVerts = 0;
    
    header = ( dheader_t* )buffer.b;
    fileBase = ( U8* )header;
    
    i = LittleLong( header->version );
#if 0
    if ( i != BSP_VERSION )
    {
        Com_Error( ERR_DROP, "idRenderSystemLocal::LoadWorldMap: %s has wrong version number (%i should be %i)", name, i, BSP_VERSION );
    }
#endif
    // swap all the lumps
    for ( i = 0; i < sizeof( dheader_t ) / 4; i++ )
    {
        ( ( S32* )header )[i] = LittleLong( ( ( S32* )header )[i] );
    }
    
    // load into heap
    renderSystemBSPTechLocal.LoadEntities( &header->lumps[LUMP_ENTITIES] );
    renderSystemBSPTechLocal.LoadShaders( &header->lumps[LUMP_SHADERS] );
    renderSystemBSPTechLocal.LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES] );
    renderSystemBSPTechLocal.LoadPlanes( &header->lumps[LUMP_PLANES] );
    renderSystemBSPTechLocal.LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
    renderSystemBSPTechLocal.LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
    renderSystemBSPTechLocal.LoadMarksurfaces( &header->lumps[LUMP_LEAFSURFACES] );
    renderSystemBSPTechLocal.LoadNodesAndLeafs( &header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS] );
    renderSystemBSPTechLocal.LoadSubmodels( &header->lumps[LUMP_MODELS] );
    renderSystemBSPTechLocal.LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
    renderSystemBSPTechLocal.LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );
    
    // determine vertex light directions
    renderSystemBSPTechLocal.CalcVertexLightDirs();
    
    // determine which parts of the map are in sunlight
    if ( 0 )
    {
        world_t* w;
        U8* primaryLightGrid, * data;
        S32 lightGridSize;
        S32 i;
        
        w = &s_worldData;
        
        lightGridSize = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];
        primaryLightGrid = ( U8* )memorySystem->Malloc( lightGridSize * sizeof( *primaryLightGrid ) );
        
        memset( primaryLightGrid, 0, lightGridSize * sizeof( *primaryLightGrid ) );
        
        data = w->lightGridData;
        for ( i = 0; i < lightGridSize; i++, data += 8 )
        {
            S32 lat, lng;
            vec3_t gridLightDir, gridLightCol;
            
            // skip samples in wall
            if ( !( data[0] + data[1] + data[2] + data[3] + data[4] + data[5] ) )
            {
                continue;
            }
            
            gridLightCol[0] = ByteToFloat( data[3] );
            gridLightCol[1] = ByteToFloat( data[4] );
            gridLightCol[2] = ByteToFloat( data[5] );
            // Suppress unused-but-set-variable warning
            ( void )gridLightCol;
            
            lat = data[7];
            lng = data[6];
            lat *= ( FUNCTABLE_SIZE / 256 );
            lng *= ( FUNCTABLE_SIZE / 256 );
            
            // decode X as cos( lat ) * sin( long )
            // decode Y as sin( lat ) * sin( long )
            // decode Z as cos( long )
            
            gridLightDir[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK] * tr.sinTable[lng];
            gridLightDir[1] = tr.sinTable[lat] * tr.sinTable[lng];
            gridLightDir[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK];
            
            // FIXME: magic number for determining if light direction is close enough to sunlight
            if ( DotProduct( gridLightDir, tr.sunDirection ) > 0.75f )
            {
                primaryLightGrid[i] = 1;
            }
            else
            {
                primaryLightGrid[i] = 255;
            }
        }
        
        if ( 0 )
        {
            S32 i;
            U8* buffer = ( U8* )memorySystem->Malloc( w->lightGridBounds[0] * w->lightGridBounds[1] * 3 + 18 );
            U8* out;
            U8* in;
            UTF8 fileName[MAX_QPATH];
            
            ::memset( buffer, 0, 18 );
            buffer[2] = 2;		// uncompressed type
            buffer[12] = w->lightGridBounds[0] & 255;
            buffer[13] = w->lightGridBounds[0] >> 8;
            buffer[14] = w->lightGridBounds[1] & 255;
            buffer[15] = w->lightGridBounds[1] >> 8;
            buffer[16] = 24;	// pixel size
            
            in = primaryLightGrid;
            for ( i = 0; i < w->lightGridBounds[2]; i++ )
            {
                S32 j;
                
                Q_snprintf( fileName, sizeof( fileName ), "primarylg%d.tga", i );
                
                out = buffer + 18;
                for ( j = 0; j < w->lightGridBounds[0] * w->lightGridBounds[1]; j++ )
                {
                    if ( *in == 1 )
                    {
                        *out++ = 255;
                        *out++ = 255;
                        *out++ = 255;
                    }
                    else if ( *in == 255 )
                    {
                        *out++ = 64;
                        *out++ = 64;
                        *out++ = 64;
                    }
                    else
                    {
                        *out++ = 0;
                        *out++ = 0;
                        *out++ = 0;
                    }
                    in++;
                }
                
                fileSystem->WriteFile( fileName, buffer, w->lightGridBounds[0] * w->lightGridBounds[1] * 3 + 18 );
            }
            
            memorySystem->Free( buffer );
        }
        
        for ( i = 0; i < w->numWorldSurfaces; i++ )
        {
            msurface_t* surf = w->surfaces + i;
            cullinfo_t* ci = &surf->cullinfo;
            
            if ( ci->type & CULLINFO_PLANE )
            {
                if ( DotProduct( ci->plane.normal, tr.sunDirection ) <= 0.0f )
                {
                    //clientMainSystem->RefPrintf(PRINT_ALL, "surface %d is not oriented towards sunlight\n", i);
                    continue;
                }
            }
            
            if ( ci->type & CULLINFO_BOX )
            {
                S32 ibounds[2][3], x, y, z, goodSamples, numSamples;
                vec3_t lightOrigin;
                
                VectorSubtract( ci->bounds[0], w->lightGridOrigin, lightOrigin );
                
                ibounds[0][0] = ( S32 )floorf( lightOrigin[0] * w->lightGridInverseSize[0] );
                ibounds[0][1] = ( S32 )floorf( lightOrigin[1] * w->lightGridInverseSize[1] );
                ibounds[0][2] = ( S32 )floorf( lightOrigin[2] * w->lightGridInverseSize[2] );
                
                VectorSubtract( ci->bounds[1], w->lightGridOrigin, lightOrigin );
                
                ibounds[1][0] = ( S32 )ceilf( lightOrigin[0] * w->lightGridInverseSize[0] );
                ibounds[1][1] = ( S32 )ceilf( lightOrigin[1] * w->lightGridInverseSize[1] );
                ibounds[1][2] = ( S32 )ceilf( lightOrigin[2] * w->lightGridInverseSize[2] );
                
                ibounds[0][0] = ( S32 )CLAMP( ibounds[0][0], 0, w->lightGridSize[0] );
                ibounds[0][1] = ( S32 )CLAMP( ibounds[0][1], 0, w->lightGridSize[1] );
                ibounds[0][2] = ( S32 )CLAMP( ibounds[0][2], 0, w->lightGridSize[2] );
                
                ibounds[1][0] = ( S32 )CLAMP( ibounds[1][0], 0, w->lightGridSize[0] );
                ibounds[1][1] = ( S32 )CLAMP( ibounds[1][1], 0, w->lightGridSize[1] );
                ibounds[1][2] = ( S32 )CLAMP( ibounds[1][2], 0, w->lightGridSize[2] );
                
                /*
                clientMainSystem->RefPrintf(PRINT_ALL, "surf %d bounds (%f %f %f)-(%f %f %f) ibounds (%d %d %d)-(%d %d %d)\n", i,
                    ci->bounds[0][0], ci->bounds[0][1], ci->bounds[0][2],
                    ci->bounds[1][0], ci->bounds[1][1], ci->bounds[1][2],
                    ibounds[0][0], ibounds[0][1], ibounds[0][2],
                    ibounds[1][0], ibounds[1][1], ibounds[1][2]);
                */
                
                goodSamples = 0;
                numSamples = 0;
                for ( x = ibounds[0][0]; x <= ibounds[1][0]; x++ )
                {
                    for ( y = ibounds[0][1]; y <= ibounds[1][1]; y++ )
                    {
                        for ( z = ibounds[0][2]; z <= ibounds[1][2]; z++ )
                        {
                            U8 primaryLight = primaryLightGrid[x * 8 + y * 8 * w->lightGridBounds[0] + z * 8 * w->lightGridBounds[0] * w->lightGridBounds[2]];
                            
                            if ( primaryLight == 0 )
                            {
                                continue;
                            }
                            
                            numSamples++;
                            
                            if ( primaryLight == 1 )
                            {
                                goodSamples++;
                            }
                        }
                    }
                }
                
                // FIXME: magic number for determining whether object is mostly in sunlight
                if ( goodSamples > numSamples * 0.75f )
                {
                    //clientMainSystem->RefPrintf(PRINT_ALL, "surface %d is in sunlight\n", i);
                    //surf->primaryLight = 1;
                }
            }
        }
        
        memorySystem->Free( primaryLightGrid );
    }
    
    // load cubemaps
    if ( r_cubeMapping->integer )
    {
        renderSystemBSPTechLocal.LoadCubemapEntities( "misc_cubemap" );
        if ( !tr.numCubemaps )
        {
            // location names are an assured way to get an even distribution
            renderSystemBSPTechLocal.LoadCubemapEntities( "target_location" );
        }
        
        if ( !tr.numCubemaps )
        {
            // try misc_models
            renderSystemBSPTechLocal.LoadCubemapEntities( "misc_model" );
        }
        
        if ( tr.numCubemaps )
        {
            renderSystemBSPTechLocal.AssignCubemapsToWorldSurfaces();
        }
    }
    
    s_worldData.dataSize = ( S32 )( ( U8* )memorySystem->Alloc( 0, h_low ) - startMarker );
    
    clientMainSystem->RefPrintf( PRINT_ALL, "total world data size: %d.%02d MB\n", s_worldData.dataSize / ( 1024 * 1024 ),
                                 ( s_worldData.dataSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
                                 
    // only set tr.world now that we know the entire level has loaded properly
    tr.world = &s_worldData;
    
    // make sure the VAO glState entry is safe
    idRenderSystemVaoLocal::BindNullVao();
    
    // Render all cubemaps
    if ( r_cubeMapping->integer && tr.numCubemaps )
    {
        renderSystemBSPTechLocal.LoadCubemaps();
        renderSystemBSPTechLocal.RenderMissingCubemaps();
    }
    
    fileSystem->FreeFile( buffer.v );
}

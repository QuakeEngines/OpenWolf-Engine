////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   cl_cin.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: video and cinematic playback
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

//#define ADAPTED_TO_STREAMING_SOUND
// (SA) MISSIONPACK MERGE
// s_rawend for wolf is [] and for q3 is just a single value
// I need to ask Ryan if it's as simple as a constant index or
// if some more coding needs to be done.

#include <framework/precompiled.h>

#define MAXSIZE             8
#define MINSIZE             4

#define DEFAULT_CIN_WIDTH   512
#define DEFAULT_CIN_HEIGHT  512

#define ROQ_QUAD            0x1000
#define ROQ_QUAD_INFO       0x1001
#define ROQ_CODEBOOK        0x1002
#define ROQ_QUAD_VQ         0x1011
#define ROQ_QUAD_JPEG       0x1012
#define ROQ_QUAD_HANG       0x1013
#define ROQ_PACKET          0x1030
#define ZA_SOUND_MONO       0x1020
#define ZA_SOUND_STEREO     0x1021

#define MAX_VIDEO_HANDLES   16

extern S32      s_soundtime;

#define CIN_STREAM 0    //DAJ const for the sound stream used for cinematics
static void     RoQ_init( void );

/******************************************************************************
*
* Class:		trFMV
*
* Description:	RoQ/RnR manipulation routines
*				not entirely complete for first run
*
******************************************************************************/

static S64     ROQ_YY_tab[256];
static S64     ROQ_UB_tab[256];
static S64     ROQ_UG_tab[256];
static S64     ROQ_VG_tab[256];
static S64     ROQ_VR_tab[256];
static U16 vq2[256 * 16 * 4];
static U16 vq4[256 * 64 * 4];
static U16 vq8[256 * 256 * 4];

typedef enum
{
    FT_ROQ = 0,					// normal roq (vq3 stuff)
} filetype_t;

typedef struct
{
    U8            linbuf[DEFAULT_CIN_WIDTH* DEFAULT_CIN_HEIGHT * 4 * 2];
    U8            file[65536];
    S16           sqrTable[256];
    
    S32             mcomp[256];
    U8*           qStatus[2][32768];
    
    S64            oldXOff, oldYOff, oldysize, oldxsize;
    
    S32             currentHandle;
} cinematics_t;

typedef struct
{
    UTF8            fileName[MAX_OSPATH];
    S32             CIN_WIDTH, CIN_HEIGHT;
    S32             xpos, ypos, width, height;
    bool        looping, holdAtEnd, dirty, alterGameState, silent, shader;
    fileHandle_t    iFile;
    e_status        status;
    U32    startTime;
    U32    lastTime;
    S64            tfps;
    S64            RoQPlayed;
    S64            ROQSize;
    U32    RoQFrameSize;
    S64            onQuad;
    S64            numQuads;
    S64            samplesPerLine;
    U32    roq_id;
    S64            screenDelta;
    
    void ( *VQ0 )( U8* status, void* qdata );
    void ( *VQ1 )( U8* status, void* qdata );
    void ( *VQNormal )( U8* status, void* qdata );
    void ( *VQBuffer )( U8* status, void* qdata );
    
    S64            samplesPerPixel;	// defaults to 2
    U8*           gray;
    U32    xsize, ysize, maxsize, minsize;
    
    bool        half, smootheddouble, inMemory;
    S64            normalBuffer0;
    S64            roq_flags;
    S64            roqF0;
    S64            roqF1;
    S64            t[2];
    S64            roqFPS;
    S32             playonwalls;
    U8*           buf;
    S64            drawX, drawY;
    filetype_t      fileType;
} cin_cache;

static cinematics_t cin;
static cin_cache cinTable[MAX_VIDEO_HANDLES];
static S32      currentHandle = -1;
static S32      CL_handle = -1;

void CIN_CloseAllVideos( void )
{
    S32		i;
    
    for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ )
    {
        if ( cinTable[i].fileName[0] != 0 )
        {
            CIN_StopCinematic( i );
        }
    }
}


static S32 CIN_HandleForVideo( void )
{
    S32		i;
    
    for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ )
    {
        if ( cinTable[i].fileName[0] == 0 )
        {
            return i;
        }
    }
    Com_Error( ERR_DROP, "CIN_HandleForVideo: none free" );
    return -1;
}

//-----------------------------------------------------------------------------
// RllSetupTable
//
// Allocates and initializes the square table.
//
// Parameters:  None
//
// Returns:     Nothing
//-----------------------------------------------------------------------------
static void RllSetupTable( void )
{
    S32 z;
    
    for ( z = 0; z < 128; z++ )
    {
        cin.sqrTable[z] = ( S16 )( z * z );
        cin.sqrTable[z + 128] = ( S16 )( -cin.sqrTable[z] );
    }
}


//-----------------------------------------------------------------------------
// RllDecodeMonoToMono
//
// Decode mono source data into a mono buffer.
//
// Parameters:  from -> buffer holding encoded data
//              to ->   buffer to hold decoded data
//              size =  number of bytes of input (= # of shorts of output)
//              signedOutput = 0 for unsigned output, non-zero for signed output
//              flag = flags from asset header
//
// Returns:     Number of samples placed in output buffer
//-----------------------------------------------------------------------------
S64 RllDecodeMonoToMono( U8* from, S16* to, U32 size, UTF8 signedOutput , U16 flag )
{
    U32 z;
    S32 prev;
    
    if ( signedOutput )
        prev =  flag - 0x8000;
    else
        prev = flag;
        
    for ( z = 0; z < size; z++ )
    {
        prev = to[z] = ( S16 )( prev + cin.sqrTable[from[z]] );
    }
    return size;	//*sizeof(S16));
}


//-----------------------------------------------------------------------------
// RllDecodeMonoToStereo
//
// Decode mono source data into a stereo buffer. Output is 4 times the number
// of bytes in the input.
//
// Parameters:  from -> buffer holding encoded data
//              to ->   buffer to hold decoded data
//              size =  number of bytes of input (= 1/4 # of bytes of output)
//              signedOutput = 0 for unsigned output, non-zero for signed output
//              flag = flags from asset header
//
// Returns:     Number of samples placed in output buffer
//-----------------------------------------------------------------------------
S64 RllDecodeMonoToStereo( U8* from, S16* to, U32 size, UTF8 signedOutput, U16 flag )
{
    U32 z;
    S32 prev;
    
    if ( signedOutput )
        prev =  flag - 0x8000;
    else
        prev = flag;
        
    for ( z = 0; z < size; z++ )
    {
        prev = ( S16 )( prev + cin.sqrTable[from[z]] );
        to[z * 2 + 0] = to[z * 2 + 1] = ( S16 )( prev );
    }
    
    return size;	// * 2 * sizeof(S16));
}


//-----------------------------------------------------------------------------
// RllDecodeStereoToStereo
//
// Decode stereo source data into a stereo buffer.
//
// Parameters:  from -> buffer holding encoded data
//              to ->   buffer to hold decoded data
//              size =  number of bytes of input (= 1/2 # of bytes of output)
//              signedOutput = 0 for unsigned output, non-zero for signed output
//              flag = flags from asset header
//
// Returns:     Number of samples placed in output buffer
//-----------------------------------------------------------------------------
S64 RllDecodeStereoToStereo( U8* from, S16* to, U32 size, UTF8 signedOutput, U16 flag )
{
    U32 z;
    U8* zz = from;
    S32	prevL, prevR;
    
    if ( signedOutput )
    {
        prevL = ( flag & 0xff00 ) - 0x8000;
        prevR = ( ( flag & 0x00ff ) << 8 ) - 0x8000;
    }
    else
    {
        prevL = flag & 0xff00;
        prevR = ( flag & 0x00ff ) << 8;
    }
    
    for ( z = 0; z < size; z += 2 )
    {
        prevL = ( S16 )( prevL + cin.sqrTable[*zz++] );
        prevR = ( S16 )( prevR + cin.sqrTable[*zz++] );
        to[z + 0] = ( S16 )( prevL );
        to[z + 1] = ( S16 )( prevR );
    }
    
    return ( size >> 1 );	//*sizeof(S16));
}


//-----------------------------------------------------------------------------
// RllDecodeStereoToMono
//
// Decode stereo source data into a mono buffer.
//
// Parameters:  from -> buffer holding encoded data
//              to ->   buffer to hold decoded data
//              size =  number of bytes of input (= # of bytes of output)
//              signedOutput = 0 for unsigned output, non-zero for signed output
//              flag = flags from asset header
//
// Returns:     Number of samples placed in output buffer
//-----------------------------------------------------------------------------
S64 RllDecodeStereoToMono( U8* from, S16* to, U32 size, UTF8 signedOutput, U16 flag )
{
    U32 z;
    S32 prevL, prevR;
    
    if ( signedOutput )
    {
        prevL = ( flag & 0xff00 ) - 0x8000;
        prevR = ( ( flag & 0x00ff ) << 8 ) - 0x8000;
    }
    else
    {
        prevL = flag & 0xff00;
        prevR = ( flag & 0x00ff ) << 8;
    }
    
    for ( z = 0; z < size; z += 1 )
    {
        prevL = prevL + cin.sqrTable[from[z * 2]];
        prevR = prevR + cin.sqrTable[from[z * 2 + 1]];
        to[z] = ( S16 )( ( prevL + prevR ) / 2 );
    }
    
    return size;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void move8_32( U8* src, U8* dst, S32 spl )
{
    S32 i;
    
    for ( i = 0; i < 8; ++i )
    {
        memcpy( dst, src, 32 );
        src += spl;
        dst += spl;
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void move4_32( U8* src, U8* dst, S32 spl )
{
    S32 i;
    
    for ( i = 0; i < 4; ++i )
    {
        memcpy( dst, src, 16 );
        src += spl;
        dst += spl;
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blit8_32( U8* src, U8* dst, S32 spl )
{
    S32 i;
    
    for ( i = 0; i < 8; ++i )
    {
        memcpy( dst, src, 32 );
        src += 32;
        dst += spl;
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
static void blit4_32( U8* src, U8* dst, S32 spl )
{
    S32             i;
    
    for ( i = 0; i < 4; ++i )
    {
        memmove( dst, src, 16 );
        src += 16;
        dst += spl;
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blit2_32( U8* src, U8* dst, S32 spl )
{
    memcpy( dst, src, 8 );
    memcpy( dst + spl, src + 8, 8 );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blitVQQuad32fs( U8** status, U8* data )
{
    U16	newd, celdata, code;
    U32	index, i;
    S32		spl;
    
    newd	= 0;
    celdata = 0;
    index	= 0;
    
    spl = cinTable[currentHandle].samplesPerLine;
    
    do
    {
        if ( !newd )
        {
            newd = 7;
            celdata = data[0] + data[1] * 256;
            data += 2;
        }
        else
        {
            newd--;
        }
        
        code = ( U16 )( celdata & 0xc000 );
        celdata <<= 2;
        
        switch ( code )
        {
            case	0x8000:													// vq code
                blit8_32( ( U8* )&vq8[( *data ) * 128], status[index], spl );
                data++;
                index += 5;
                break;
            case	0xc000:													// drop
                index++;													// skip 8x8
                for ( i = 0; i < 4; i++ )
                {
                    if ( !newd )
                    {
                        newd = 7;
                        celdata = data[0] + data[1] * 256;
                        data += 2;
                    }
                    else
                    {
                        newd--;
                    }
                    
                    code = ( U16 )( celdata & 0xc000 );
                    celdata <<= 2;
                    
                    switch ( code )  											// code in top two bits of code
                    {
                        case	0x8000:										// 4x4 vq code
                            blit4_32( ( U8* )&vq4[( *data ) * 32], status[index], spl );
                            data++;
                            break;
                        case	0xc000:										// 2x2 vq code
                            blit2_32( ( U8* )&vq2[( *data ) * 8], status[index], spl );
                            data++;
                            blit2_32( ( U8* )&vq2[( *data ) * 8], status[index] + 8, spl );
                            data++;
                            blit2_32( ( U8* )&vq2[( *data ) * 8], status[index] + spl * 2, spl );
                            data++;
                            blit2_32( ( U8* )&vq2[( *data ) * 8], status[index] + spl * 2 + 8, spl );
                            data++;
                            break;
                        case	0x4000:										// motion compensation
                            move4_32( status[index] + cin.mcomp[( *data )], status[index], spl );
                            data++;
                            break;
                    }
                    index++;
                }
                break;
            case	0x4000:													// motion compensation
                move8_32( status[index] + cin.mcomp[( *data )], status[index], spl );
                data++;
                index += 5;
                break;
            case	0x0000:
                index += 5;
                break;
        }
    }
    while ( status[index] != nullptr );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

void ROQ_GenYUVTables( void )
{
    F32 t_ub, t_vr, t_ug, t_vg;
    S64 i;
    
    t_ub = ( 1.77200f / 2.0f ) * ( F32 )( 1 << 6 ) + 0.5f;
    t_vr = ( 1.40200f / 2.0f ) * ( F32 )( 1 << 6 ) + 0.5f;
    t_ug = ( 0.34414f / 2.0f ) * ( F32 )( 1 << 6 ) + 0.5f;
    t_vg = ( 0.71414f / 2.0f ) * ( F32 )( 1 << 6 ) + 0.5f;
    for ( i = 0; i < 256; i++ )
    {
        F32 x = ( F32 )( 2 * i - 255 );
        
        ROQ_UB_tab[i] = ( S64 )( ( t_ub * x ) + ( 1 << 5 ) );
        ROQ_VR_tab[i] = ( S64 )( ( t_vr * x ) + ( 1 << 5 ) );
        ROQ_UG_tab[i] = ( S64 )( ( -t_ug * x ) );
        ROQ_VG_tab[i] = ( S64 )( ( -t_vg * x ) + ( 1 << 5 ) );
        ROQ_YY_tab[i] = ( S64 )( ( i << 6 ) | ( i >> 2 ) );
    }
}

#define VQ2TO4(a,b,c,d) { \
    	*c++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*c++ = a[1];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*c++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*c++ = b[1];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	a += 2; b += 2; }

#define VQ2TO2(a,b,c,d) { \
	*c++ = *a;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*c++ = *b;	\
	*d++ = *b;	\
	*d++ = *b;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*d++ = *b;	\
	*d++ = *b;	\
	a++; b++; }

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static U16 yuv_to_rgb( S64 y, S64 u, S64 v )
{
    S64 r, g, b, YY = ( S64 )( ROQ_YY_tab[( y )] );
    
    r = ( YY + ROQ_VR_tab[v] ) >> 9;
    g = ( YY + ROQ_UG_tab[u] + ROQ_VG_tab[v] ) >> 8;
    b = ( YY + ROQ_UB_tab[u] ) >> 9;
    
    if ( r < 0 ) r = 0;
    if ( g < 0 ) g = 0;
    if ( b < 0 ) b = 0;
    if ( r > 31 ) r = 31;
    if ( g > 63 ) g = 63;
    if ( b > 31 ) b = 31;
    
    return ( U16 )( ( r << 11 ) + ( g << 5 ) + ( b ) );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
static U32 yuv_to_rgb24( S64 y, S64 u, S64 v )
{
    S64 r, g, b, YY = ( S64 )( ROQ_YY_tab[( y )] );
    
    r = ( YY + ROQ_VR_tab[v] ) >> 6;
    g = ( YY + ROQ_UG_tab[u] + ROQ_VG_tab[v] ) >> 6;
    b = ( YY + ROQ_UB_tab[u] ) >> 6;
    
    if ( r < 0 ) r = 0;
    if ( g < 0 ) g = 0;
    if ( b < 0 ) b = 0;
    if ( r > 255 ) r = 255;
    if ( g > 255 ) g = 255;
    if ( b > 255 ) b = 255;
    
    return LittleLong( ( r ) | ( g << 8 ) | ( b << 16 ) | ( 255 << 24 ) );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void decodeCodeBook( U8* input, U16 roq_flags )
{
    S64	i, j, two, four;
    U16*	aptr, *bptr, *cptr, *dptr;
    S64	y0, y1, y2, y3, cr, cb;
    U8*	bbptr, *baptr, *bcptr, *bdptr;
    union
    {
        U32* i;
        U16* s;
    } iaptr, ibptr, icptr, idptr;
    
    if ( !roq_flags )
    {
        two = four = 256;
    }
    else
    {
        two  = roq_flags >> 8;
        if ( !two ) two = 256;
        four = roq_flags & 0xff;
    }
    
    four *= 2;
    
    bptr = ( U16* )vq2;
    
    if ( !cinTable[currentHandle].half )
    {
        if ( !cinTable[currentHandle].smootheddouble )
        {
            //
            // normal height
            //
            if ( cinTable[currentHandle].samplesPerPixel == 2 )
            {
                for ( i = 0; i < two; i++ )
                {
                    y0 = ( S64 ) * input++;
                    y1 = ( S64 ) * input++;
                    y2 = ( S64 ) * input++;
                    y3 = ( S64 ) * input++;
                    cr = ( S64 ) * input++;
                    cb = ( S64 ) * input++;
                    *bptr++ = yuv_to_rgb( y0, cr, cb );
                    *bptr++ = yuv_to_rgb( y1, cr, cb );
                    *bptr++ = yuv_to_rgb( y2, cr, cb );
                    *bptr++ = yuv_to_rgb( y3, cr, cb );
                }
                
                cptr = ( U16* )vq4;
                dptr = ( U16* )vq8;
                
                for ( i = 0; i < four; i++ )
                {
                    aptr = ( U16* )vq2 + ( *input++ ) * 4;
                    bptr = ( U16* )vq2 + ( *input++ ) * 4;
                    for ( j = 0; j < 2; j++ )
                        VQ2TO4( aptr, bptr, cptr, dptr );
                }
            }
            else if ( cinTable[currentHandle].samplesPerPixel == 4 )
            {
                ibptr.s = bptr;
                for ( i = 0; i < two; i++ )
                {
                    y0 = ( S64 ) * input++;
                    y1 = ( S64 ) * input++;
                    y2 = ( S64 ) * input++;
                    y3 = ( S64 ) * input++;
                    cr = ( S64 ) * input++;
                    cb = ( S64 ) * input++;
                    *ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
                }
                
                icptr.s = vq4;
                idptr.s = vq8;
                
                for ( i = 0; i < four; i++ )
                {
                    iaptr.s = vq2;
                    iaptr.i += ( *input++ ) * 4;
                    ibptr.s = vq2;
                    ibptr.i += ( *input++ ) * 4;
                    for ( j = 0; j < 2; j++ )
                        VQ2TO4( iaptr.i, ibptr.i, icptr.i, idptr.i );
                }
            }
            else if ( cinTable[currentHandle].samplesPerPixel == 1 )
            {
                bbptr = ( U8* )bptr;
                for ( i = 0; i < two; i++ )
                {
                    *bbptr++ = cinTable[currentHandle].gray[*input++];
                    *bbptr++ = cinTable[currentHandle].gray[*input++];
                    *bbptr++ = cinTable[currentHandle].gray[*input++];
                    *bbptr++ = cinTable[currentHandle].gray[*input];
                    input += 3;
                }
                
                bcptr = ( U8* )vq4;
                bdptr = ( U8* )vq8;
                
                for ( i = 0; i < four; i++ )
                {
                    baptr = ( U8* )vq2 + ( *input++ ) * 4;
                    bbptr = ( U8* )vq2 + ( *input++ ) * 4;
                    for ( j = 0; j < 2; j++ )
                        VQ2TO4( baptr, bbptr, bcptr, bdptr );
                }
            }
        }
        else
        {
            //
            // double height, smoothed
            //
            if ( cinTable[currentHandle].samplesPerPixel == 2 )
            {
                for ( i = 0; i < two; i++ )
                {
                    y0 = ( S64 ) * input++;
                    y1 = ( S64 ) * input++;
                    y2 = ( S64 ) * input++;
                    y3 = ( S64 ) * input++;
                    cr = ( S64 ) * input++;
                    cb = ( S64 ) * input++;
                    *bptr++ = yuv_to_rgb( y0, cr, cb );
                    *bptr++ = yuv_to_rgb( y1, cr, cb );
                    *bptr++ = yuv_to_rgb( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
                    *bptr++ = yuv_to_rgb( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
                    *bptr++ = yuv_to_rgb( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
                    *bptr++ = yuv_to_rgb( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
                    *bptr++ = yuv_to_rgb( y2, cr, cb );
                    *bptr++ = yuv_to_rgb( y3, cr, cb );
                }
                
                cptr = ( U16* )vq4;
                dptr = ( U16* )vq8;
                
                for ( i = 0; i < four; i++ )
                {
                    aptr = ( U16* )vq2 + ( *input++ ) * 8;
                    bptr = ( U16* )vq2 + ( *input++ ) * 8;
                    for ( j = 0; j < 2; j++ )
                    {
                        VQ2TO4( aptr, bptr, cptr, dptr );
                        VQ2TO4( aptr, bptr, cptr, dptr );
                    }
                }
            }
            else if ( cinTable[currentHandle].samplesPerPixel == 4 )
            {
                ibptr.s = bptr;
                for ( i = 0; i < two; i++ )
                {
                    y0 = ( S64 ) * input++;
                    y1 = ( S64 ) * input++;
                    y2 = ( S64 ) * input++;
                    y3 = ( S64 ) * input++;
                    cr = ( S64 ) * input++;
                    cb = ( S64 ) * input++;
                    *ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
                    *ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
                }
                
                icptr.s = vq4;
                idptr.s = vq8;
                
                for ( i = 0; i < four; i++ )
                {
                    iaptr.s = vq2;
                    iaptr.i += ( *input++ ) * 8;
                    ibptr.s = vq2;
                    ibptr.i += ( *input++ ) * 8;
                    for ( j = 0; j < 2; j++ )
                    {
                        VQ2TO4( iaptr.i, ibptr.i, icptr.i, idptr.i );
                        VQ2TO4( iaptr.i, ibptr.i, icptr.i, idptr.i );
                    }
                }
            }
            else if ( cinTable[currentHandle].samplesPerPixel == 1 )
            {
                bbptr = ( U8* )bptr;
                for ( i = 0; i < two; i++ )
                {
                    y0 = ( S64 ) * input++;
                    y1 = ( S64 ) * input++;
                    y2 = ( S64 ) * input++;
                    y3 = ( S64 ) * input;
                    input += 3;
                    *bbptr++ = cinTable[currentHandle].gray[y0];
                    *bbptr++ = cinTable[currentHandle].gray[y1];
                    *bbptr++ = cinTable[currentHandle].gray[( ( y0 * 3 ) + y2 ) / 4];
                    *bbptr++ = cinTable[currentHandle].gray[( ( y1 * 3 ) + y3 ) / 4];
                    *bbptr++ = cinTable[currentHandle].gray[( y0 + ( y2 * 3 ) ) / 4];
                    *bbptr++ = cinTable[currentHandle].gray[( y1 + ( y3 * 3 ) ) / 4];
                    *bbptr++ = cinTable[currentHandle].gray[y2];
                    *bbptr++ = cinTable[currentHandle].gray[y3];
                }
                
                bcptr = ( U8* )vq4;
                bdptr = ( U8* )vq8;
                
                for ( i = 0; i < four; i++ )
                {
                    baptr = ( U8* )vq2 + ( *input++ ) * 8;
                    bbptr = ( U8* )vq2 + ( *input++ ) * 8;
                    for ( j = 0; j < 2; j++ )
                    {
                        VQ2TO4( baptr, bbptr, bcptr, bdptr );
                        VQ2TO4( baptr, bbptr, bcptr, bdptr );
                    }
                }
            }
        }
    }
    else
    {
        //
        // 1/4 screen
        //
        if ( cinTable[currentHandle].samplesPerPixel == 2 )
        {
            for ( i = 0; i < two; i++ )
            {
                y0 = ( S64 ) * input;
                input += 2;
                y2 = ( S64 ) * input;
                input += 2;
                cr = ( S64 ) * input++;
                cb = ( S64 ) * input++;
                *bptr++ = yuv_to_rgb( y0, cr, cb );
                *bptr++ = yuv_to_rgb( y2, cr, cb );
            }
            
            cptr = ( U16* )vq4;
            dptr = ( U16* )vq8;
            
            for ( i = 0; i < four; i++ )
            {
                aptr = ( U16* )vq2 + ( *input++ ) * 2;
                bptr = ( U16* )vq2 + ( *input++ ) * 2;
                for ( j = 0; j < 2; j++ )
                {
                    VQ2TO2( aptr, bptr, cptr, dptr );
                }
            }
        }
        else if ( cinTable[currentHandle].samplesPerPixel == 1 )
        {
            bbptr = ( U8* )bptr;
            
            for ( i = 0; i < two; i++ )
            {
                *bbptr++ = cinTable[currentHandle].gray[*input];
                input += 2;
                *bbptr++ = cinTable[currentHandle].gray[*input];
                input += 4;
            }
            
            bcptr = ( U8* )vq4;
            bdptr = ( U8* )vq8;
            
            for ( i = 0; i < four; i++ )
            {
                baptr = ( U8* )vq2 + ( *input++ ) * 2;
                bbptr = ( U8* )vq2 + ( *input++ ) * 2;
                for ( j = 0; j < 2; j++ )
                {
                    VQ2TO2( baptr, bbptr, bcptr, bdptr );
                }
            }
        }
        else if ( cinTable[currentHandle].samplesPerPixel == 4 )
        {
            ibptr.s = bptr;
            for ( i = 0; i < two; i++ )
            {
                y0 = ( S64 ) * input;
                input += 2;
                y2 = ( S64 ) * input;
                input += 2;
                cr = ( S64 ) * input++;
                cb = ( S64 ) * input++;
                *ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
                *ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
            }
            
            icptr.s = vq4;
            idptr.s = vq8;
            
            for ( i = 0; i < four; i++ )
            {
                iaptr.s = vq2;
                iaptr.i += ( *input++ ) * 2;
                ibptr.s = vq2 + ( *input++ ) * 2;
                ibptr.i += ( *input++ ) * 2;
                for ( j = 0; j < 2; j++ )
                {
                    VQ2TO2( iaptr.i, ibptr.i, icptr.i, idptr.i );
                }
            }
        }
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void recurseQuad( S64 startX, S64 startY, S64 quadSize, S64 xOff, S64 yOff )
{
    U8* scroff;
    S64 bigx, bigy, lowx, lowy, useY;
    S64 offset;
    
    offset = cinTable[currentHandle].screenDelta;
    
    lowx = lowy = 0;
    bigx = cinTable[currentHandle].xsize;
    bigy = cinTable[currentHandle].ysize;
    
    if ( bigx > cinTable[currentHandle].CIN_WIDTH ) bigx = cinTable[currentHandle].CIN_WIDTH;
    if ( bigy > cinTable[currentHandle].CIN_HEIGHT ) bigy = cinTable[currentHandle].CIN_HEIGHT;
    
    if ( ( startX >= lowx ) && ( startX + quadSize ) <= ( bigx ) && ( startY + quadSize ) <= ( bigy ) && ( startY >= lowy ) && quadSize <= MAXSIZE )
    {
        useY = startY;
        scroff = cin.linbuf + ( useY + ( ( cinTable[currentHandle].CIN_HEIGHT - bigy ) >> 1 ) + yOff ) * ( cinTable[currentHandle].samplesPerLine ) + ( ( ( startX + xOff ) ) * cinTable[currentHandle].samplesPerPixel );
        
        cin.qStatus[0][cinTable[currentHandle].onQuad  ] = scroff;
        cin.qStatus[1][cinTable[currentHandle].onQuad++] = scroff + offset;
    }
    
    if ( quadSize != MINSIZE )
    {
        quadSize >>= 1;
        recurseQuad( startX,		  startY		  , quadSize, xOff, yOff );
        recurseQuad( startX + quadSize, startY		  , quadSize, xOff, yOff );
        recurseQuad( startX,		  startY + quadSize , quadSize, xOff, yOff );
        recurseQuad( startX + quadSize, startY + quadSize , quadSize, xOff, yOff );
    }
}


/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void setupQuad( S64 xOff, S64 yOff )
{
    S64 numQuadCels, i, x, y;
    U8* temp;
    
    if ( xOff == cin.oldXOff && yOff == cin.oldYOff && cinTable[currentHandle].ysize == cin.oldysize && cinTable[currentHandle].xsize == cin.oldxsize )
    {
        return;
    }
    
    cin.oldXOff = xOff;
    cin.oldYOff = yOff;
    cin.oldysize = cinTable[currentHandle].ysize;
    cin.oldxsize = cinTable[currentHandle].xsize;
    
    numQuadCels  = ( cinTable[currentHandle].CIN_WIDTH * cinTable[currentHandle].CIN_HEIGHT ) / ( 16 );
    numQuadCels += numQuadCels / 4 + numQuadCels / 16;
    numQuadCels += 64;							  // for overflow
    
    numQuadCels  = ( cinTable[currentHandle].xsize * cinTable[currentHandle].ysize ) / ( 16 );
    numQuadCels += numQuadCels / 4;
    numQuadCels += 64;							  // for overflow
    
    cinTable[currentHandle].onQuad = 0;
    
    for ( y = 0; y < ( S64 )cinTable[currentHandle].ysize; y += 16 )
        for ( x = 0; x < ( S64 )cinTable[currentHandle].xsize; x += 16 )
            recurseQuad( x, y, 16, xOff, yOff );
            
    temp = nullptr;
    
    for ( i = ( numQuadCels - 64 ); i < numQuadCels; i++ )
    {
        cin.qStatus[0][i] = temp;			  // eoq
        cin.qStatus[1][i] = temp;			  // eoq
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void readQuadInfo( U8* qData )
{
    if ( currentHandle < 0 ) return;
    
    cinTable[currentHandle].xsize    = qData[0] + qData[1] * 256;
    cinTable[currentHandle].ysize    = qData[2] + qData[3] * 256;
    cinTable[currentHandle].maxsize  = qData[4] + qData[5] * 256;
    cinTable[currentHandle].minsize  = qData[6] + qData[7] * 256;
    
    cinTable[currentHandle].CIN_HEIGHT = cinTable[currentHandle].ysize;
    cinTable[currentHandle].CIN_WIDTH  = cinTable[currentHandle].xsize;
    
    cinTable[currentHandle].samplesPerLine = cinTable[currentHandle].CIN_WIDTH * cinTable[currentHandle].samplesPerPixel;
    cinTable[currentHandle].screenDelta = cinTable[currentHandle].CIN_HEIGHT * cinTable[currentHandle].samplesPerLine;
    
    cinTable[currentHandle].half = false;
    cinTable[currentHandle].smootheddouble = false;
    
    cinTable[currentHandle].VQ0 = cinTable[currentHandle].VQNormal;
    cinTable[currentHandle].VQ1 = cinTable[currentHandle].VQBuffer;
    
    cinTable[currentHandle].t[0] = cinTable[currentHandle].screenDelta;
    cinTable[currentHandle].t[1] = -cinTable[currentHandle].screenDelta;
    
    cinTable[currentHandle].drawX = cinTable[currentHandle].CIN_WIDTH;
    cinTable[currentHandle].drawY = cinTable[currentHandle].CIN_HEIGHT;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQPrepMcomp( S64 xoff, S64 yoff )
{
    S64 i, j, x, y, temp, temp2;
    
    i = cinTable[currentHandle].samplesPerLine;
    j = cinTable[currentHandle].samplesPerPixel;
    if ( cinTable[currentHandle].xsize == ( cinTable[currentHandle].ysize * 4 ) && !cinTable[currentHandle].half )
    {
        j = j + j;
        i = i + i;
    }
    
    for ( y = 0; y < 16; y++ )
    {
        temp2 = ( y + yoff - 8 ) * i;
        for ( x = 0; x < 16; x++ )
        {
            temp = ( x + xoff - 8 ) * j;
            cin.mcomp[( x * 16 ) + y] = cinTable[currentHandle].normalBuffer0 - ( temp2 + temp );
        }
    }
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void initRoQ( void )
{
    if ( currentHandle < 0 ) return;
    
    cinTable[currentHandle].VQNormal = ( void (* )( U8*, void* ) )blitVQQuad32fs;
    cinTable[currentHandle].VQBuffer = ( void (* )( U8*, void* ) )blitVQQuad32fs;
    cinTable[currentHandle].samplesPerPixel = 4;
    ROQ_GenYUVTables();
    RllSetupTable();
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
/*
static U8* RoQFetchInterlaced( U8 *source ) {
	S32 x, *src, *dst;

	if (currentHandle < 0) return nullptr;

	src = (S32 *)source;
	dst = (S32 *)cinTable[currentHandle].buf2;

	for(x=0;x<256*256;x++) {
		*dst = *src;
		dst++; src += 2;
	}
	return cinTable[currentHandle].buf2;
}
*/
static void RoQReset( void )
{

    if ( currentHandle < 0 ) return;
    
    fileSystem->FCloseFile( cinTable[currentHandle].iFile );
    fileSystem->FOpenFileRead( cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, true );
    // let the background thread start reading ahead
    fileSystem->Read( cin.file, 16, cinTable[currentHandle].iFile );
    RoQ_init();
    cinTable[currentHandle].status = FMV_LOOPED;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQInterrupt( void )
{
    U8*				framedata;
    S16		sbuf[32768];
    S32		ssize;
    
    if ( currentHandle < 0 ) return;
    
    fileSystem->Read( cin.file, cinTable[currentHandle].RoQFrameSize + 8, cinTable[currentHandle].iFile );
    if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize )
    {
        if ( cinTable[currentHandle].holdAtEnd == false )
        {
            if ( cinTable[currentHandle].looping )
            {
                RoQReset();
            }
            else
            {
                cinTable[currentHandle].status = FMV_EOF;
            }
        }
        else
        {
            cinTable[currentHandle].status = FMV_IDLE;
        }
        return;
    }
    
    framedata = cin.file;
    //
    // new frame is ready
    //
redump:
    switch ( cinTable[currentHandle].roq_id )
    {
        case	ROQ_QUAD_VQ:
            if ( ( cinTable[currentHandle].numQuads & 1 ) )
            {
                cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[1];
                RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
                cinTable[currentHandle].VQ1( ( U8* )cin.qStatus[1], framedata );
                cinTable[currentHandle].buf = 	cin.linbuf + cinTable[currentHandle].screenDelta;
            }
            else
            {
                cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[0];
                RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
                cinTable[currentHandle].VQ0( ( U8* )cin.qStatus[0], framedata );
                cinTable[currentHandle].buf = 	cin.linbuf;
            }
            if ( cinTable[currentHandle].numQuads == 0 )  		// first frame
            {
                ::memcpy( cin.linbuf + cinTable[currentHandle].screenDelta, cin.linbuf, cinTable[currentHandle].samplesPerLine * cinTable[currentHandle].ysize );
            }
            cinTable[currentHandle].numQuads++;
            cinTable[currentHandle].dirty = true;
            break;
        case	ROQ_CODEBOOK:
            decodeCodeBook( framedata, ( U16 )cinTable[currentHandle].roq_flags );
            break;
        case	ZA_SOUND_MONO:
            if ( !cinTable[currentHandle].silent )
            {
                ssize = RllDecodeMonoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, ( U16 )cinTable[currentHandle].roq_flags );
                soundSystem->RawSamples( ssize, 22050, 2, 1, reinterpret_cast< U8*>( sbuf ), 1.0f );
            }
            break;
        case	ZA_SOUND_STEREO:
            if ( !cinTable[currentHandle].silent )
            {
                if ( cinTable[currentHandle].numQuads == -1 )
                {
                    soundSystem->Update();
                    s_rawend = s_soundtime;
                }
                ssize = RllDecodeStereoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, ( U16 )cinTable[currentHandle].roq_flags );
                soundSystem->RawSamples( ssize, 22050, 2, 2, reinterpret_cast< U8*>( sbuf ), 1.0f );
            }
            break;
        case	ROQ_QUAD_INFO:
            if ( cinTable[currentHandle].numQuads == -1 )
            {
                readQuadInfo( framedata );
                setupQuad( 0, 0 );
                // we need to use clientMainSystem->ScaledMilliseconds because of the smp mode calls from the renderer
                cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = ( U32 )( clientMainSystem->ScaledMilliseconds() * com_timescale->value );
            }
            if ( cinTable[currentHandle].numQuads != 1 ) cinTable[currentHandle].numQuads = 0;
            break;
        case	ROQ_PACKET:
            cinTable[currentHandle].inMemory = ( bool )cinTable[currentHandle].roq_flags;
            cinTable[currentHandle].RoQFrameSize = 0;           // for header
            break;
        case	ROQ_QUAD_HANG:
            cinTable[currentHandle].RoQFrameSize = 0;
            break;
        case	ROQ_QUAD_JPEG:
            break;
        default:
            cinTable[currentHandle].status = FMV_EOF;
            break;
    }
    //
    // read in next frame data
    //
    if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize )
    {
        if ( cinTable[currentHandle].holdAtEnd == false )
        {
            if ( cinTable[currentHandle].looping )
            {
                RoQReset();
            }
            else
            {
                cinTable[currentHandle].status = FMV_EOF;
            }
        }
        else
        {
            cinTable[currentHandle].status = FMV_IDLE;
        }
        return;
    }
    
    framedata		 += cinTable[currentHandle].RoQFrameSize;
    cinTable[currentHandle].roq_id		 = framedata[0] + framedata[1] * 256;
    cinTable[currentHandle].RoQFrameSize = framedata[2] + framedata[3] * 256 + framedata[4] * 65536;
    cinTable[currentHandle].roq_flags	 = framedata[6] + framedata[7] * 256;
    cinTable[currentHandle].roqF0		 = ( S8 )framedata[7];
    cinTable[currentHandle].roqF1		 = ( S8 )framedata[6];
    
    if ( cinTable[currentHandle].RoQFrameSize > 65536 || cinTable[currentHandle].roq_id == 0x1084 )
    {
        Com_DPrintf( "roq_size>65536||roq_id==0x1084\n" );
        cinTable[currentHandle].status = FMV_EOF;
        if ( cinTable[currentHandle].looping )
        {
            RoQReset();
        }
        return;
    }
    if ( cinTable[currentHandle].inMemory && ( cinTable[currentHandle].status != FMV_EOF ) )
    {
        cinTable[currentHandle].inMemory = false;
        framedata += 8;
        goto redump;
    }
    //
    // one more frame hits the dust
    //
    //	assert(cinTable[currentHandle].RoQFrameSize <= 65536);
    //	r = fileSystem->Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
    cinTable[currentHandle].RoQPlayed	+= cinTable[currentHandle].RoQFrameSize + 8;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQ_init( void )
{
    // we need to use clientMainSystem->ScaledMilliseconds because of the smp mode calls from the renderer
    cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = ( U32 )( clientMainSystem->ScaledMilliseconds() * com_timescale->value );
    
    cinTable[currentHandle].RoQPlayed = 24;
    
    /*	get frame rate */
    cinTable[currentHandle].roqFPS	 = cin.file[ 6] + cin.file[ 7] * 256;
    
    if ( !cinTable[currentHandle].roqFPS ) cinTable[currentHandle].roqFPS = 30;
    
    cinTable[currentHandle].numQuads = -1;
    
    cinTable[currentHandle].roq_id		= cin.file[ 8] + cin.file[ 9] * 256;
    cinTable[currentHandle].RoQFrameSize	= cin.file[10] + cin.file[11] * 256 + cin.file[12] * 65536;
    cinTable[currentHandle].roq_flags	= cin.file[14] + cin.file[15] * 256;
    
    if ( cinTable[currentHandle].RoQFrameSize > 65536 || !cinTable[currentHandle].RoQFrameSize )
    {
        return;
    }
    
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

//FIXME: this isn't realy a "roq-shutdown" (it's more a CIN-shutdown, beside the file-closing)
static void RoQShutdown( void )
{
    StringEntry s;
    
    if ( !cinTable[currentHandle].buf )
    {
        //FIXME: there could be something that should be "shutdowned" even if we don't have a output frame (at least in the ogm code)
        return;
    }
    
    if ( cinTable[currentHandle].status == FMV_IDLE )
    {
        return;
    }
    Com_DPrintf( "finished cinematic\n" );
    cinTable[currentHandle].status = FMV_IDLE;
    
    if ( cinTable[currentHandle].iFile )
    {
        fileSystem->FCloseFile( cinTable[currentHandle].iFile );
        cinTable[currentHandle].iFile = 0;
    }
    
    if ( cinTable[currentHandle].alterGameState )
    {
        cls.state = CA_DISCONNECTED;
        // we can't just do a vstr nextmap, because
        // if we are aborting the intro cinematic with
        // a devmap command, nextmap would be valid by
        // the time it was referenced
        s = cvarSystem->VariableString( "nextmap" );
        if ( s[0] )
        {
            cmdBufferSystem->ExecuteText( EXEC_APPEND, va( "%s\n", s ) );
            cvarSystem->Set( "nextmap", "" );
        }
        CL_handle = -1;
    }
    cinTable[currentHandle].fileName[0] = 0;
    currentHandle = -1;
}

/*
==================
CIN_StopCinematic
==================
*/
e_status CIN_StopCinematic( S32 handle )
{

    if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) return FMV_EOF;
    currentHandle = handle;
    
    Com_DPrintf( "trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName );
    
    if ( !cinTable[currentHandle].buf )
    {
        return FMV_EOF;
    }
    
    if ( cinTable[currentHandle].alterGameState )
    {
        if ( cls.state != CA_CINEMATIC )
        {
            return cinTable[currentHandle].status;
        }
    }
    
    cinTable[currentHandle].status = FMV_EOF;
    RoQShutdown();
    
    return FMV_EOF;
}


/*
==================
CIN_RunCinematic

Fetch and decompress the pending frame
==================
*/
e_status CIN_RunCinematic( S32 handle )
{
    S32	start = 0;
    S32     thisTime = 0;
    
    if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) return FMV_EOF;
    
    if ( cin.currentHandle != handle )
    {
        currentHandle = handle;
        cin.currentHandle = currentHandle;
        cinTable[currentHandle].status = FMV_EOF;
        RoQReset();
    }
    
    if ( cinTable[handle].playonwalls < -1 )
    {
        return cinTable[handle].status;
    }
    
    currentHandle = handle;
    
    if ( cinTable[currentHandle].alterGameState )
    {
        if ( cls.state != CA_CINEMATIC )
        {
            return cinTable[currentHandle].status;
        }
    }
    
    if ( cinTable[currentHandle].status == FMV_IDLE )
    {
        return cinTable[currentHandle].status;
    }
    
    //FIXME? clientMainSystem->ScaledMilliseconds already uses com_timescale (so I can't see that the com_timescale in here makes any sense at all O_o)
    // we need to use clientMainSystem->ScaledMilliseconds because of the smp mode calls from the renderer
    thisTime = ( U32 )( clientMainSystem->ScaledMilliseconds() * com_timescale->value );
    if ( cinTable[currentHandle].shader && ( ( F32 )abs( ( F32 )thisTime - cinTable[currentHandle].lastTime ) ) > 100 )
    {
        cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
    }
    // we need to use clientMainSystem->ScaledMilliseconds because of the smp mode calls from the renderer
    cinTable[currentHandle].tfps = ( ( ( ( ( U32 )( clientMainSystem->ScaledMilliseconds() * com_timescale->value ) ) - cinTable[currentHandle].startTime ) * 3 ) / 100 );
    
    start = cinTable[currentHandle].startTime;
    while ( ( cinTable[currentHandle].tfps != cinTable[currentHandle].numQuads )
            && ( cinTable[currentHandle].status == FMV_PLAY ) )
    {
        RoQInterrupt();
        if ( start != cinTable[currentHandle].startTime )
        {
            // we need to use clientMainSystem->ScaledMilliseconds because of the smp mode calls from the renderer
            cinTable[currentHandle].tfps = ( ( ( ( ( U32 )( clientMainSystem->ScaledMilliseconds() * com_timescale->value ) )
                                                 - cinTable[currentHandle].startTime ) * 3 ) / 100 );
            start = cinTable[currentHandle].startTime;
        }
    }
    
    cinTable[currentHandle].lastTime = thisTime;
    
    if ( cinTable[currentHandle].status == FMV_LOOPED )
    {
        cinTable[currentHandle].status = FMV_PLAY;
    }
    
    if ( cinTable[currentHandle].status == FMV_EOF )
    {
        if ( cinTable[currentHandle].looping )
        {
            RoQReset();
        }
        else
        {
            RoQShutdown();
        }
    }
    
    return cinTable[currentHandle].status;
}

/*
=================
S_FileExtension
=================
*/
static UTF8* S_FileExtension( StringEntry fni )
{
    // we should search from the ending to the last '/'
    UTF8* fn = ( UTF8* )fni + strlen( fni ) - 1;
    UTF8* eptr = nullptr;
    
    while ( *fn != '/' && fn != fni )
    {
        if ( *fn == '.' )
        {
            eptr = fn;
            break;
        }
        fn--;
    }
    
    return eptr;
}

/*
==================
CIN_PlayCinematic
==================
*/
S32 CIN_PlayCinematic( StringEntry arg, S32 x, S32 y, S32 w, S32 h, S32 systemBits )
{
    U16 RoQID;
    UTF8	name[MAX_OSPATH];
    S32		i;
    UTF8*   fileextPtr;
    
    if ( strstr( arg, "/" ) == nullptr && strstr( arg, "\\" ) == nullptr )
    {
        Q_snprintf( name, sizeof( name ), "video/%s", arg );
    }
    else
    {
        Q_snprintf( name, sizeof( name ), "%s", arg );
    }
    
    if ( !( systemBits & CIN_system ) )
    {
        for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ )
        {
            if ( !strcmp( cinTable[i].fileName, name ) )
            {
                return i;
            }
        }
    }
    
    Com_DPrintf( "SCR_PlayCinematic( %s )\n", arg );
    
    ::memset( &cin, 0, sizeof( cinematics_t ) );
    currentHandle = CIN_HandleForVideo();
    
    cin.currentHandle = currentHandle;
    
    strcpy( cinTable[currentHandle].fileName, name );
    
    fileextPtr = S_FileExtension( name );   // using the function from soundfile/audiocodec-detection
    
    cinTable[currentHandle].ROQSize = 0;
    cinTable[currentHandle].ROQSize = fileSystem->FOpenFileRead( cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, true );
    
    if ( cinTable[currentHandle].ROQSize <= 0 )
    {
        Com_DPrintf( "play(%s), ROQSize<=0\n", arg );
        cinTable[currentHandle].fileName[0] = 0;
        return -1;
    }
    
    CIN_SetExtents( currentHandle, x, y, w, h );
    CIN_SetLooping( currentHandle, ( bool )( ( systemBits & CIN_loop ) != 0 ) );
    
    cinTable[currentHandle].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
    cinTable[currentHandle].CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
    cinTable[currentHandle].holdAtEnd = ( bool )( ( systemBits & CIN_hold ) != 0 );
    cinTable[currentHandle].alterGameState = ( bool )( ( systemBits & CIN_system ) != 0 );
    cinTable[currentHandle].playonwalls = 1;
    cinTable[currentHandle].silent = ( bool )( ( systemBits & CIN_silent ) != 0 );
    cinTable[currentHandle].shader = ( bool )( ( systemBits & CIN_shader ) != 0 );
    
    if ( cinTable[currentHandle].alterGameState )
    {
        // close the menu
        if ( uivm )
        {
            uiManager->SetActiveMenu( UIMENU_NONE );
            //VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
        }
    }
    else
    {
        cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
    }
    
    initRoQ();
    
    fileSystem->Read( cin.file, 16, cinTable[currentHandle].iFile );
    
    RoQID = ( U16 )( cin.file[0] ) + ( U16 )( cin.file[1] ) * 256;
    if ( RoQID == 0x1084 )
    {
        RoQ_init();
        //		fileSystem->Read (cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);
        
        cinTable[currentHandle].status = FMV_PLAY;
        Com_DPrintf( "trFMV::play(), playing %s\n", arg );
        
        if ( cinTable[currentHandle].alterGameState )
        {
            cls.state = CA_CINEMATIC;
        }
        
        Con_Close();
        
        s_rawend = s_soundtime;
        
        return currentHandle;
    }
    Com_DPrintf( "trFMV::play(), invalid RoQ ID\n" );
    
    RoQShutdown();
    return -1;
}

void CIN_SetExtents( S32 handle, S32 x, S32 y, S32 w, S32 h )
{
    if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) return;
    cinTable[handle].xpos = x;
    cinTable[handle].ypos = y;
    cinTable[handle].width = w;
    cinTable[handle].height = h;
    cinTable[handle].dirty = true;
}

void CIN_SetLooping( S32 handle, bool loop )
{
    if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) return;
    cinTable[handle].looping = loop;
}

/*
==================
CIN_DrawCinematic
==================
*/
void CIN_DrawCinematic( S32 handle )
{
    F32	x, y, w, h;
    U8*	buf;
    
    if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) return;
    
    if ( !cinTable[handle].buf )
    {
        return;
    }
    
    x = ( F32 )cinTable[handle].xpos;
    y = ( F32 )cinTable[handle].ypos;
    w = ( F32 )cinTable[handle].width;
    h = ( F32 )cinTable[handle].height;
    buf = cinTable[handle].buf;
    
    idClientScreenSystemLocal::AdjustFrom640( &x, &y, &w, &h );
    
    if ( cinTable[handle].dirty && ( cinTable[handle].CIN_WIDTH != cinTable[handle].drawX || cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY ) )
    {
        S32 ix, iy, *buf2, *buf3, xm, ym, ll;
        
        xm = cinTable[handle].CIN_WIDTH / 256;
        ym = cinTable[handle].CIN_HEIGHT / 256;
        ll = 8;
        if ( cinTable[handle].CIN_WIDTH == 512 )
        {
            ll = 9;
        }
        
        buf3 = ( S32* )buf;
        buf2 = ( S32* )memorySystem->AllocateTempMemory( 256 * 256 * 4 );
        if ( xm == 2 && ym == 2 )
        {
            U8* bc2, *bc3;
            S32	ic, iiy;
            
            bc2 = ( U8* )buf2;
            bc3 = ( U8* )buf3;
            for ( iy = 0; iy < 256; iy++ )
            {
                iiy = iy << 12;
                for ( ix = 0; ix < 2048; ix += 8 )
                {
                    for ( ic = ix; ic < ( ix + 4 ); ic++ )
                    {
                        *bc2 = ( bc3[iiy + ic] + bc3[iiy + 4 + ic] + bc3[iiy + 2048 + ic] + bc3[iiy + 2048 + 4 + ic] ) >> 2;
                        bc2++;
                    }
                }
            }
        }
        else if ( xm == 2 && ym == 1 )
        {
            U8* bc2, *bc3;
            S32	ic, iiy;
            
            bc2 = ( U8* )buf2;
            bc3 = ( U8* )buf3;
            for ( iy = 0; iy < 256; iy++ )
            {
                iiy = iy << 11;
                for ( ix = 0; ix < 2048; ix += 8 )
                {
                    for ( ic = ix; ic < ( ix + 4 ); ic++ )
                    {
                        *bc2 = ( bc3[iiy + ic] + bc3[iiy + 4 + ic] ) >> 1;
                        bc2++;
                    }
                }
            }
        }
        else
        {
            for ( iy = 0; iy < 256; iy++ )
            {
                for ( ix = 0; ix < 256; ix++ )
                {
                    buf2[( iy << 8 ) + ix] = buf3[( ( iy * ym ) << ll ) + ( ix * xm )];
                }
            }
        }
        renderSystem->DrawStretchRaw( ( S32 )x, ( S32 )y, ( S32 )w, ( S32 )h, 256, 256, ( U8* )buf2, handle, true );
        cinTable[handle].dirty = false;
        memorySystem->FreeTempMemory( buf2 );
        return;
    }
    
    renderSystem->DrawStretchRaw( ( S32 )x, ( S32 )y, ( S32 )w, ( S32 )h, cinTable[handle].drawX, cinTable[handle].drawY, buf, handle, cinTable[handle].dirty );
    cinTable[handle].dirty = false;
}

void CL_PlayCinematic_f( void )
{
    UTF8*	arg, *s;
    bool	holdatend;
    S32 bits = CIN_system;
    
    // don't allow this while on server
    if ( cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE )
    {
        return;
    }
    
    Com_DPrintf( "CL_PlayCinematic_f\n" );
    if ( cls.state == CA_CINEMATIC )
    {
        SCR_StopCinematic();
    }
    
    arg = cmdSystem->Argv( 1 );
    s = cmdSystem->Argv( 2 );
    
    holdatend = false;
    if ( ( s && s[0] == '1' ) || Q_stricmp( arg, "demoend.roq" ) == 0 || Q_stricmp( arg, "end.roq" ) == 0 )
    {
        bits |= CIN_hold;
    }
    if ( s && s[0] == '2' )
    {
        bits |= CIN_loop;
    }
    
    soundSystem->StopAllSounds();
    
    CL_handle = CIN_PlayCinematic( arg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits );
    if ( CL_handle >= 0 )
    {
        do
        {
            SCR_RunCinematic();
        }
        while ( cinTable[currentHandle].buf == nullptr && cinTable[currentHandle].status == FMV_PLAY );		// wait for first frame (load codebook and sound)
    }
}


void SCR_DrawCinematic( void )
{
    if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
    {
        CIN_DrawCinematic( CL_handle );
    }
}

void SCR_RunCinematic( void )
{
    if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
    {
        CIN_RunCinematic( CL_handle );
    }
}

void SCR_StopCinematic( void )
{
    if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
    {
        CIN_StopCinematic( CL_handle );
        soundSystem->StopAllSounds();
        CL_handle = -1;
    }
}

void CIN_UploadCinematic( S32 handle )
{
    if ( handle >= 0 && handle < MAX_VIDEO_HANDLES )
    {
        if ( !cinTable[handle].buf )
        {
            return;
        }
        if ( cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty )
        {
            if ( cinTable[handle].playonwalls == 0 )
            {
                cinTable[handle].playonwalls = -1;
            }
            else
            {
                if ( cinTable[handle].playonwalls == -1 )
                {
                    cinTable[handle].playonwalls = -2;
                }
                else
                {
                    cinTable[handle].dirty = false;
                }
            }
        }
        renderSystem->UploadCinematic( 256, 256, 256, 256, cinTable[handle].buf, handle, cinTable[handle].dirty );
        if ( cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1 )
        {
            cinTable[handle].playonwalls--;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2016 - 2017 OGS Dev Team
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
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
// File name:   q_math.cpp
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description: stateless support routines that are included in each code module
//              Some of the vector functions are static inline in q_shared.h. q3asm
//              doesn't understand static functions though, so we only want them in
//              one file. That's what this is about.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#if defined (UPDATE_SERVER)
#include <null/null_autoprecompiled.h>
#elif defined (DEDICATED)
#include <null/null_serverprecompiled.h>
#elif defined (GUI)
#include <GUI/gui_precompiled.h>
#elif defined (CGAMEDLL)
#include <cgame/cgame_precompiled.h>
#elif defined (GAMEDLL)
#include <sgame/sgame_precompiled.h>
#elif defined (RENDERSYSTEM)
#include <renderSystem/r_precompiled.h>
#else
#include <framework/precompiled.h>
#endif // !GAMEDLL

// *INDENT-OFF*
#if !defined (Q3MAP2) || defined (_WIN32)
vec3_t vec3_origin = {0, 0, 0};
#endif
vec3_t axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

matrix_t matrixIdentity = {	1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 1
                          };

quat_t quatIdentity = { 0, 0, 0, 1 };


vec4_t colorBlack      =   {0, 0, 0, 1};
vec4_t colorRed        =   {1, 0, 0, 1};
vec4_t colorGreen      =   {0, 1, 0, 1};
vec4_t colorBlue       =   {0, 0, 1, 1};
vec4_t colorYellow     =   {1, 1, 0, 1};
vec4_t colorOrange     =   {1, 0.5, 0, 1};
vec4_t colorMagenta    =   {1, 0, 1, 1};
vec4_t colorCyan       =   {0, 1, 1, 1};
vec4_t colorWhite      =   {1, 1, 1, 1};
vec4_t colorLtGrey     =   {0.75, 0.75, 0.75, 1};
vec4_t colorMdGrey     =   {0.5, 0.5, 0.5, 1};
vec4_t colorDkGrey     =   {0.25, 0.25, 0.25, 1};
vec4_t colorMdRed      =   {0.5, 0, 0, 1};
vec4_t colorMdGreen    =   {0, 0.5, 0, 1};

vec4_t clrBrown =          {0.68f,         0.68f,          0.56f,          1.f};
vec4_t clrBrownDk =        {0.58f * 0.75f, 0.58f * 0.75f,  0.46f * 0.75f,  1.f};
vec4_t clrBrownLine =      {0.0525f,       0.05f,          0.025f,         0.2f};
vec4_t clrBrownLineFull =  {0.0525f,       0.05f,          0.025f,         1.f};

vec4_t g_color_table[MAX_CCODES] =
{
    { 0.00000f, 0.00000f, 0.00000f, 1.00000f },	//0
    { 1.00000f, 0.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 1.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 1.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.50000f, 0.00000f, 1.00000f },
    { 0.60000f, 0.60000f, 1.00000f, 1.00000f },	//9
    { 1.00000f, 0.00000f, 0.00000f, 1.00000f },	//a
    { 1.00000f, 0.13239f, 0.00000f, 1.00000f },
    { 1.00000f, 0.26795f, 0.00000f, 1.00000f },
    { 1.00000f, 0.37829f, 0.00000f, 1.00000f },
    { 1.00000f, 0.50000f, 0.00000f, 1.00000f },
    { 1.00000f, 0.60633f, 0.00000f, 1.00000f },
    { 1.00000f, 0.73205f, 0.00000f, 1.00000f },
    { 1.00000f, 0.84990f, 0.00000f, 1.00000f },
    { 1.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.86761f, 1.00000f, 0.00000f, 1.00000f },
    { 0.73205f, 1.00000f, 0.00000f, 1.00000f },
    { 0.62171f, 1.00000f, 0.00000f, 1.00000f },
    { 0.50000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.39367f, 1.00000f, 0.00000f, 1.00000f },
    { 0.26795f, 1.00000f, 0.00000f, 1.00000f },
    { 0.15010f, 1.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.00000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.13239f, 1.00000f },
    { 0.00000f, 1.00000f, 0.26795f, 1.00000f },
    { 0.00000f, 1.00000f, 0.37829f, 1.00000f },
    { 0.00000f, 1.00000f, 0.50000f, 1.00000f },
    { 0.00000f, 1.00000f, 0.60633f, 1.00000f },
    { 0.00000f, 1.00000f, 0.73205f, 1.00000f },
    { 0.00000f, 1.00000f, 0.84990f, 1.00000f },
    { 0.00000f, 1.00000f, 1.00000f, 1.00000f },
    { 0.00000f, 0.86761f, 1.00000f, 1.00000f },
    { 0.00000f, 0.73205f, 1.00000f, 1.00000f },	//A
    { 0.00000f, 0.62171f, 1.00000f, 1.00000f },
    { 0.00000f, 0.50000f, 1.00000f, 1.00000f },
    { 0.00000f, 0.39367f, 1.00000f, 1.00000f },
    { 0.00000f, 0.26795f, 1.00000f, 1.00000f },
    { 0.00000f, 0.15010f, 1.00000f, 1.00000f },
    { 0.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 0.13239f, 0.00000f, 1.00000f, 1.00000f },
    { 0.26795f, 0.00000f, 1.00000f, 1.00000f },
    { 0.37829f, 0.00000f, 1.00000f, 1.00000f },
    { 0.50000f, 0.00000f, 1.00000f, 1.00000f },
    { 0.60633f, 0.00000f, 1.00000f, 1.00000f },
    { 0.73205f, 0.00000f, 1.00000f, 1.00000f },
    { 0.84990f, 0.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.00000f, 1.00000f, 1.00000f },
    { 1.00000f, 0.00000f, 0.86761f, 1.00000f },
    { 1.00000f, 0.00000f, 0.73205f, 1.00000f },
    { 1.00000f, 0.00000f, 0.62171f, 1.00000f },
    { 1.00000f, 0.00000f, 0.50000f, 1.00000f },
    { 1.00000f, 0.00000f, 0.39367f, 1.00000f },
    { 1.00000f, 0.00000f, 0.26795f, 1.00000f },
    { 1.00000f, 0.00000f, 0.15010f, 1.00000f },
    { 0.75000f, 0.75000f, 0.75000f, 1.00000f },
    { 0.50000f, 0.50000f, 0.50000f, 1.00000f },
    { 0.25000f, 0.25000f, 0.25000f, 1.00000f },
    { 1.00000f, 0.50000f, 1.00000f, 1.00000f },
};

vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
    { -0.525731f, 0.000000f, 0.850651f }, { -0.442863f, 0.238856f, 0.864188f },
    { -0.295242f, 0.000000f, 0.955423f }, { -0.309017f, 0.500000f, 0.809017f },
    { -0.162460f, 0.262866f, 0.951056f }, { 0.000000f, 0.000000f, 1.000000f },
    { 0.000000f, 0.850651f, 0.525731f }, { -0.147621f, 0.716567f, 0.681718f },
    { 0.147621f, 0.716567f, 0.681718f }, { 0.000000f, 0.525731f, 0.850651f },
    { 0.309017f, 0.500000f, 0.809017f }, { 0.525731f, 0.000000f, 0.850651f },
    { 0.295242f, 0.000000f, 0.955423f }, { 0.442863f, 0.238856f, 0.864188f },
    { 0.162460f, 0.262866f, 0.951056f }, { -0.681718f, 0.147621f, 0.716567f },
    { -0.809017f, 0.309017f, 0.500000f }, { -0.587785f, 0.425325f, 0.688191f },
    { -0.850651f, 0.525731f, 0.000000f }, { -0.864188f, 0.442863f, 0.238856f },
    { -0.716567f, 0.681718f, 0.147621f }, { -0.688191f, 0.587785f, 0.425325f },
    { -0.500000f, 0.809017f, 0.309017f }, { -0.238856f, 0.864188f, 0.442863f },
    { -0.425325f, 0.688191f, 0.587785f }, { -0.716567f, 0.681718f, -0.147621f },
    { -0.500000f, 0.809017f, -0.309017f }, { -0.525731f, 0.850651f, 0.000000f },
    { 0.000000f, 0.850651f, -0.525731f }, { -0.238856f, 0.864188f, -0.442863f },
    { 0.000000f, 0.955423f, -0.295242f }, { -0.262866f, 0.951056f, -0.162460f },
    { 0.000000f, 1.000000f, 0.000000f }, { 0.000000f, 0.955423f, 0.295242f },
    { -0.262866f, 0.951056f, 0.162460f }, { 0.238856f, 0.864188f, 0.442863f },
    { 0.262866f, 0.951056f, 0.162460f }, { 0.500000f, 0.809017f, 0.309017f },
    { 0.238856f, 0.864188f, -0.442863f }, { 0.262866f, 0.951056f, -0.162460f },
    { 0.500000f, 0.809017f, -0.309017f }, { 0.850651f, 0.525731f, 0.000000f },
    { 0.716567f, 0.681718f, 0.147621f }, { 0.716567f, 0.681718f, -0.147621f },
    { 0.525731f, 0.850651f, 0.000000f }, { 0.425325f, 0.688191f, 0.587785f },
    { 0.864188f, 0.442863f, 0.238856f }, { 0.688191f, 0.587785f, 0.425325f },
    { 0.809017f, 0.309017f, 0.500000f }, { 0.681718f, 0.147621f, 0.716567f },
    { 0.587785f, 0.425325f, 0.688191f }, { 0.955423f, 0.295242f, 0.000000f },
    { 1.000000f, 0.000000f, 0.000000f }, { 0.951056f, 0.162460f, 0.262866f },
    { 0.850651f, -0.525731f, 0.000000f }, { 0.955423f, -0.295242f, 0.000000f },
    { 0.864188f, -0.442863f, 0.238856f }, { 0.951056f, -0.162460f, 0.262866f },
    { 0.809017f, -0.309017f, 0.500000f }, { 0.681718f, -0.147621f, 0.716567f },
    { 0.850651f, 0.000000f, 0.525731f }, { 0.864188f, 0.442863f, -0.238856f },
    { 0.809017f, 0.309017f, -0.500000f }, { 0.951056f, 0.162460f, -0.262866f },
    { 0.525731f, 0.000000f, -0.850651f }, { 0.681718f, 0.147621f, -0.716567f },
    { 0.681718f, -0.147621f, -0.716567f }, { 0.850651f, 0.000000f, -0.525731f },
    { 0.809017f, -0.309017f, -0.500000f }, { 0.864188f, -0.442863f, -0.238856f },
    { 0.951056f, -0.162460f, -0.262866f }, { 0.147621f, 0.716567f, -0.681718f },
    { 0.309017f, 0.500000f, -0.809017f }, { 0.425325f, 0.688191f, -0.587785f },
    { 0.442863f, 0.238856f, -0.864188f }, { 0.587785f, 0.425325f, -0.688191f },
    { 0.688191f, 0.587785f, -0.425325f }, { -0.147621f, 0.716567f, -0.681718f },
    { -0.309017f, 0.500000f, -0.809017f }, { 0.000000f, 0.525731f, -0.850651f },
    { -0.525731f, 0.000000f, -0.850651f }, { -0.442863f, 0.238856f, -0.864188f },
    { -0.295242f, 0.000000f, -0.955423f }, { -0.162460f, 0.262866f, -0.951056f },
    { 0.000000f, 0.000000f, -1.000000f }, { 0.295242f, 0.000000f, -0.955423f },
    { 0.162460f, 0.262866f, -0.951056f }, { -0.442863f, -0.238856f, -0.864188f },
    { -0.309017f, -0.500000f, -0.809017f }, { -0.162460f, -0.262866f, -0.951056f },
    { 0.000000f, -0.850651f, -0.525731f }, { -0.147621f, -0.716567f, -0.681718f },
    { 0.147621f, -0.716567f, -0.681718f }, { 0.000000f, -0.525731f, -0.850651f },
    { 0.309017f, -0.500000f, -0.809017f }, { 0.442863f, -0.238856f, -0.864188f },
    { 0.162460f, -0.262866f, -0.951056f }, { 0.238856f, -0.864188f, -0.442863f },
    { 0.500000f, -0.809017f, -0.309017f }, { 0.425325f, -0.688191f, -0.587785f },
    { 0.716567f, -0.681718f, -0.147621f }, { 0.688191f, -0.587785f, -0.425325f },
    { 0.587785f, -0.425325f, -0.688191f }, { 0.000000f, -0.955423f, -0.295242f },
    { 0.000000f, -1.000000f, 0.000000f }, { 0.262866f, -0.951056f, -0.162460f },
    { 0.000000f, -0.850651f, 0.525731f }, { 0.000000f, -0.955423f, 0.295242f },
    { 0.238856f, -0.864188f, 0.442863f }, { 0.262866f, -0.951056f, 0.162460f },
    { 0.500000f, -0.809017f, 0.309017f }, { 0.716567f, -0.681718f, 0.147621f },
    { 0.525731f, -0.850651f, 0.000000f }, { -0.238856f, -0.864188f, -0.442863f },
    { -0.500000f, -0.809017f, -0.309017f }, { -0.262866f, -0.951056f, -0.162460f },
    { -0.850651f, -0.525731f, 0.000000f }, { -0.716567f, -0.681718f, -0.147621f },
    { -0.716567f, -0.681718f, 0.147621f }, { -0.525731f, -0.850651f, 0.000000f },
    { -0.500000f, -0.809017f, 0.309017f }, { -0.238856f, -0.864188f, 0.442863f },
    { -0.262866f, -0.951056f, 0.162460f }, { -0.864188f, -0.442863f, 0.238856f },
    { -0.809017f, -0.309017f, 0.500000f }, { -0.688191f, -0.587785f, 0.425325f },
    { -0.681718f, -0.147621f, 0.716567f }, { -0.442863f, -0.238856f, 0.864188f },
    { -0.587785f, -0.425325f, 0.688191f }, { -0.309017f, -0.500000f, 0.809017f },
    { -0.147621f, -0.716567f, 0.681718f }, { -0.425325f, -0.688191f, 0.587785f },
    { -0.162460f, -0.262866f, 0.951056f }, { 0.442863f, -0.238856f, 0.864188f },
    { 0.162460f, -0.262866f, 0.951056f }, { 0.309017f, -0.500000f, 0.809017f },
    { 0.147621f, -0.716567f, 0.681718f }, { 0.000000f, -0.525731f, 0.850651f },
    { 0.425325f, -0.688191f, 0.587785f }, { 0.587785f, -0.425325f, 0.688191f },
    { 0.688191f, -0.587785f, 0.425325f }, { -0.955423f, 0.295242f, 0.000000f },
    { -0.951056f, 0.162460f, 0.262866f }, { -1.000000f, 0.000000f, 0.000000f },
    { -0.850651f, 0.000000f, 0.525731f }, { -0.955423f, -0.295242f, 0.000000f },
    { -0.951056f, -0.162460f, 0.262866f }, { -0.864188f, 0.442863f, -0.238856f },
    { -0.951056f, 0.162460f, -0.262866f }, { -0.809017f, 0.309017f, -0.500000f },
    { -0.864188f, -0.442863f, -0.238856f }, { -0.951056f, -0.162460f, -0.262866f },
    { -0.809017f, -0.309017f, -0.500000f }, { -0.681718f, 0.147621f, -0.716567f },
    { -0.681718f, -0.147621f, -0.716567f }, { -0.850651f, 0.000000f, -0.525731f },
    { -0.688191f, 0.587785f, -0.425325f }, { -0.587785f, 0.425325f, -0.688191f },
    { -0.425325f, 0.688191f, -0.587785f }, { -0.425325f, -0.688191f, -0.587785f },
    { -0.587785f, -0.425325f, -0.688191f }, { -0.688191f, -0.587785f, -0.425325f }
};

//==============================================================

S32 Q_rand( S32* seed )
{
    *seed = ( 69069 * *seed + 1 );
    return *seed;
}

F32 Q_random( S32* seed )
{
    return ( Q_rand( seed ) & 0xffff ) / ( F32 )0x10000;
}

F32 Q_crandom( S32* seed )
{
    return 2.0f * ( Q_random( seed ) - 0.5f );
}

bool Q_IsColorString( StringEntry p )
{
    if ( !p )
    {
        return false;
    }
    
    if ( p[0] != Q_COLOR_ESCAPE )
    {
        return false;
    }
    
    if ( p[1] == 0 )
    {
        return false;
    }
    
    // isalnum expects a signed integer in the range -1 (EOF) to 255, or it might assert on undefined behaviour
    // a dereferenced char pointer has the range -128 to 127, so we just need to rangecheck the negative part
    if ( p[1] < 0 )
    {
        return false;
    }
    
    if ( isalnum( p[1] ) == 0 )
    {
        return false;
    }
    
    return true;
}

//=======================================================

U8 ClampByte( S32 i )
{
    if ( i < 0 )
    {
        return 0;
    }
    if ( i > 255 )
    {
        return 255;
    }
    return i;
}

S8 ClampChar( S32 i )
{
    if ( i < -128 )
    {
        return -128;
    }
    if ( i > 127 )
    {
        return 127;
    }
    return i;
}

S16 ClampShort( S32 i )
{
    if ( i < -32768 )
    {
        return -32768;
    }
    if ( i > 0x7fff )
    {
        return 0x7fff;
    }
    return i;
}

// this isn't a real cheap function to call!
S32 DirToByte( vec3_t dir )
{
    S32             i, best;
    F32           d, bestd;
    
    if ( !dir )
    {
        return 0;
    }
    
    bestd = 0;
    best = 0;
    for ( i = 0; i < NUMVERTEXNORMALS; i++ )
    {
        d = DotProduct( dir, bytedirs[i] );
        if ( d > bestd )
        {
            bestd = d;
            best = i;
        }
    }
    
    return best;
}

void ByteToDir( S32 b, vec3_t dir )
{
    if ( b < 0 || b >= NUMVERTEXNORMALS )
    {
        VectorCopy( vec3_origin, dir );
        return;
    }
    VectorCopy( bytedirs[b], dir );
}


U32 ColorBytes3( F32 r, F32 g, F32 b )
{
    U32 i;
    
    ( ( U8* ) & i )[0] = ( U8 )( r * 255 );
    ( ( U8* ) & i )[1] = ( U8 )( g * 255 );
    ( ( U8* ) & i )[2] = ( U8 )( b * 255 );
    
    return i;
}

U32 ColorBytes4( F32 r, F32 g, F32 b, F32 a )
{
    U32        i;
    
    ( ( U8* ) & i )[0] = ( U8 )( r * 255 );
    ( ( U8* ) & i )[1] = ( U8 )( g * 255 );
    ( ( U8* ) & i )[2] = ( U8 )( b * 255 );
    ( ( U8* ) & i )[3] = ( U8 )( a * 255 );
    
    return i;
}

F32 NormalizeColor( const vec3_t in, vec3_t out )
{
    F32           max;
    
    max = in[0];
    if ( in[1] > max )
    {
        max = in[1];
    }
    if ( in[2] > max )
    {
        max = in[2];
    }
    
    if ( !max )
    {
        VectorClear( out );
    }
    else
    {
        out[0] = in[0] / max;
        out[1] = in[1] / max;
        out[2] = in[2] / max;
    }
    return max;
}

void ClampColor( vec4_t color )
{
    S32             i;
    
    for ( i = 0; i < 4; i++ )
    {
        if ( color[i] < 0 )
            color[i] = 0;
            
        if ( color[i] > 1 )
            color[i] = 1;
    }
}

F32 PlaneNormalize( vec4_t plane )
{
    F32 length, ilength;
    
    length = sqrtf( plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2] );
    if ( length == 0 )
    {
        VectorClear( plane );
        return 0;
    }
    
    ilength = 1.0f / length;
    plane[0] = plane[0] * ilength;
    plane[1] = plane[1] * ilength;
    plane[2] = plane[2] * ilength;
    plane[3] = plane[3] * ilength;
    
    return length;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
bool PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw )
{
    vec3_t          d1, d2;
    
    VectorSubtract( b, a, d1 );
    VectorSubtract( c, a, d2 );
    
    if ( cw )
    {
        CrossProduct( d2, d1, plane );
    }
    else
    {
        CrossProduct( d1, d2, plane );
    }
    
    if ( VectorNormalize( plane ) == 0 )
    {
        return false;
    }
    
    plane[3] = DotProduct( a, plane );
    return true;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
=====================
*/
bool PlaneFromPointsOrder( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw )
{
    vec3_t          d1, d2;
    
    VectorSubtract( b, a, d1 );
    VectorSubtract( c, a, d2 );
    
    if ( cw )
    {
        CrossProduct( d2, d1, plane );
    }
    else
    {
        CrossProduct( d1, d2, plane );
    }
    
    if ( VectorNormalize( plane ) == 0 )
    {
        return false;
    }
    
    plane[3] = DotProduct( a, plane );
    return true;
}

void PlaneIntersectRay( const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res )
{
    vec3_t dir;
    F32 sect;
    F32 distToPlane;
    F32 planeDotRay;
    
    VectorNormalize2( rayDir, dir );
    
    distToPlane = DotProduct( plane, rayPos ) - plane[3];
    planeDotRay = DotProduct( plane, dir );
    sect = -( distToPlane ) / planeDotRay;
    
    VectorMA( rayPos, sect, dir, res );
}
/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
#ifndef Q3MAP2
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, F32 degrees )
{
    F32           m[3][3];
    F32           im[3][3];
    F32           zrot[3][3];
    F32           tmpmat[3][3];
    F32           rot[3][3];
    S32             i;
    vec3_t          vr, vup, vf;
    F32           rad;
    
    vf[0] = dir[0];
    vf[1] = dir[1];
    vf[2] = dir[2];
    
    if ( VectorNormalize( vf ) == 0 || degrees == 0.0f )
    {
        // degenerate case
        VectorCopy( point, dst );
        return;
    }
    
    PerpendicularVector( vr, dir );
    CrossProduct( vr, vf, vup );
    
    m[0][0] = vr[0];
    m[1][0] = vr[1];
    m[2][0] = vr[2];
    
    m[0][1] = vup[0];
    m[1][1] = vup[1];
    m[2][1] = vup[2];
    
    m[0][2] = vf[0];
    m[1][2] = vf[1];
    m[2][2] = vf[2];
    
    ::memcpy( im, m, sizeof( im ) );
    
    im[0][1] = m[1][0];
    im[0][2] = m[2][0];
    im[1][0] = m[0][1];
    im[1][2] = m[2][1];
    im[2][0] = m[0][2];
    im[2][1] = m[1][2];
    
    ::memset( zrot, 0, sizeof( zrot ) );
    zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;
    
    rad = ( F32 )DEG2RAD( degrees );
    zrot[0][0] = cosf( rad );
    zrot[0][1] = sinf( rad );
    zrot[1][0] = -sinf( rad );
    zrot[1][1] = cosf( rad );
    
    AxisMultiply( m, zrot, tmpmat );
    AxisMultiply( tmpmat, im, rot );
    
    for ( i = 0; i < 3; i++ )
    {
        dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
    }
}
#endif

/*
===============
RotatePointArountVertex

Rotate a point around a vertex
===============
*/
void RotatePointAroundVertex( vec3_t pnt, F32 rot_x, F32 rot_y, F32 rot_z, const vec3_t origin )
{
    F32 tmp[11];
    
    //F32 rad_x, rad_y, rad_z;
    
    /*rad_x = DEG2RAD( rot_x );
       rad_y = DEG2RAD( rot_y );
       rad_z = DEG2RAD( rot_z ); */
    
    // move pnt to rel{0,0,0}
    VectorSubtract( pnt, origin, pnt );
    
    // init temp values
    tmp[0] = sinf( rot_x );
    tmp[1] = cosf( rot_x );
    tmp[2] = sinf( rot_y );
    tmp[3] = cosf( rot_y );
    tmp[4] = sinf( rot_z );
    tmp[5] = cosf( rot_z );
    tmp[6] = pnt[1] * tmp[5];
    tmp[7] = pnt[0] * tmp[4];
    tmp[8] = pnt[0] * tmp[5];
    tmp[9] = pnt[1] * tmp[4];
    tmp[10] = pnt[2] * tmp[3];
    
    // rotate point
    pnt[0] = ( tmp[3] * ( tmp[8] - tmp[9] ) + pnt[3] * tmp[2] );
    pnt[1] = ( tmp[0] * ( tmp[2] * tmp[8] - tmp[2] * tmp[9] - tmp[10] ) + tmp[1] * ( tmp[7] + tmp[6] ) );
    pnt[2] = ( tmp[1] * ( -tmp[2] * tmp[8] + tmp[2] * tmp[9] + tmp[10] ) + tmp[0] * ( tmp[7] + tmp[6] ) );
    
    // move pnt back
    VectorAdd( pnt, origin, pnt );
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], F32 yaw )
{

    // create an arbitrary axis[1]
    PerpendicularVector( axis[1], axis[0] );
    
    // rotate it around axis[0] by yaw
    if ( yaw )
    {
        vec3_t          temp;
        
        VectorCopy( axis[1], temp );
        RotatePointAroundVector( axis[1], axis[0], temp, yaw );
    }
    
    // cross to get axis[2]
    CrossProduct( axis[0], axis[1], axis[2] );
}

/*
================
Q_isnan

Don't pass doubles to this
================
*/
S32 Q_isnan( F32 x )
{
    union
    {
        F32 f;
        U32 i;
    } t;
    
    t.f = x;
    t.i &= 0x7FFFFFFF;
    t.i = 0x7F800000 - t.i;
    
    return ( S32 )( ( U32 )t.i >> 31 );
}

void vectoangles( const vec3_t value1, vec3_t angles )
{
    F32           forward;
    F32           yaw, pitch;
    
    if ( value1[1] == 0 && value1[0] == 0 )
    {
        yaw = 0;
        if ( value1[2] > 0 )
        {
            pitch = 90;
        }
        else
        {
            pitch = 270;
        }
    }
    else
    {
        if ( value1[0] )
        {
            yaw = ( atan2f( value1[1], value1[0] ) * 180.0f / M_PI );
        }
        else if ( value1[1] > 0 )
        {
            yaw = 90;
        }
        else
        {
            yaw = 270;
        }
        
        if ( yaw < 0 )
        {
            yaw += 360;
        }
        
        forward = sqrtf( value1[0] * value1[0] + value1[1] * value1[1] );
        pitch = ( atan2f( value1[2], forward ) * 180.0f / M_PI );
        
        if ( pitch < 0 )
        {
            pitch += 360;
        }
    }
    
    angles[PITCH] = -pitch;
    angles[YAW] = yaw;
    angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] )
{
    vec3_t          right;
    
    // angle vectors returns "right" instead of "y axis"
    AngleVectors( angles, axis[0], right, axis[2] );
    VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] )
{
    axis[0][0] = 1;
    axis[0][1] = 0;
    axis[0][2] = 0;
    axis[1][0] = 0;
    axis[1][1] = 1;
    axis[1][2] = 0;
    axis[2][0] = 0;
    axis[2][1] = 0;
    axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] )
{
    VectorCopy( in[0], out[0] );
    VectorCopy( in[1], out[1] );
    VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
    F32           d;
    vec3_t          n;
    F32           inv_denom;
    
    inv_denom = 1.0F / DotProduct( normal, normal );
    d = DotProduct( normal, p ) * inv_denom;
    
    n[0] = normal[0] * inv_denom;
    n[1] = normal[1] * inv_denom;
    n[2] = normal[2] * inv_denom;
    
    dst[0] = p[0] - d * n[0];
    dst[1] = p[1] - d * n[1];
    dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up )
{
    F32           d;
    
    // this rotate and negate guarantees a vector
    // not colinear with the original
    right[1] = -forward[0];
    right[2] = forward[1];
    right[0] = forward[2];
    
    d = DotProduct( right, forward );
    VectorMA( right, -d, forward, right );
    VectorNormalize( right );
    CrossProduct( right, forward, up );
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
    out[0] = DotProduct( in, matrix[0] );
    out[1] = DotProduct( in, matrix[1] );
    out[2] = DotProduct( in, matrix[2] );
}

/*
===============
LerpAngle
===============
*/
F32 LerpAngle( F32 from, F32 to, F32 frac )
{
    if ( to - from > 180 )
    {
        to -= 360;
    }
    if ( to - from < -180 )
    {
        to += 360;
    }
    
    return ( from + frac * ( to - from ) );
}

/*
=================
LerpPosition

=================
*/

void LerpPosition( vec3_t start, vec3_t end, F32 frac, vec3_t out )
{
    vec3_t          dist;
    
    VectorSubtract( end, start, dist );
    VectorMA( start, frac, dist, out );
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
F32 AngleSubtract( F32 a1, F32 a2 )
{
    F32           a = a1 - a2;
    
    while ( a > 180 )
    {
        a -= 360;
    }
    while ( a < -180 )
    {
        a += 360;
    }
    return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
    v3[0] = AngleSubtract( v1[0], v2[0] );
    v3[1] = AngleSubtract( v1[1], v2[1] );
    v3[2] = AngleSubtract( v1[2], v2[2] );
}


F32 AngleMod( F32 a )
{
    return ( ( 360.0f / 65536 ) * ( ( S32 )( a * ( 65536 / 360.0f ) ) & 65535 ) );
}

/*
=================
AngleNormalize2Pi

returns angle normalized to the range [0 <= angle < 2*M_PI]
=================
*/
F32 AngleNormalize2Pi( F32 angle )
{
    return DEG2RAD( AngleNormalize360( RAD2DEG( angle ) ) );
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
F32 AngleNormalize360( F32 angle )
{
    return ( 360.0f / 65536 ) * ( ( S32 )( angle * ( 65536 / 360.0f ) ) & 65535 );
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
F32 AngleNormalize180( F32 angle )
{
    angle = AngleNormalize360( angle );
    if ( angle > 180.0 )
    {
        angle -= 360.0;
    }
    return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
F32 AngleDelta( F32 angle1, F32 angle2 )
{
    return AngleNormalize180( angle1 - angle2 );
}


/*
=================
AngleBetweenVectors

returns the angle between two vectors normalized to the range [0 <= angle <= 180]
=================
*/
F32 AngleBetweenVectors( const vec3_t a, const vec3_t b )
{
    F32           alen, blen;
    
    alen = VectorLength( a );
    blen = VectorLength( b );
    
    if ( !alen || !blen )
        return 0;
        
    // complete dot product of two vectors a, b is |a| * |b| * cos(angle)
    // this results in:
    //
    // angle = acos( (a * b) / (|a| * |b|) )
    return RAD2DEG( Q_acos( DotProduct( a, b ) / ( alen * blen ) ) );
}


//============================================================

/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits( cplane_t* out )
{
    S32             bits, j;
    
    // for fast box on planeside test
    bits = 0;
    for ( j = 0; j < 3; j++ )
    {
        if ( out->normal[j] < 0 )
        {
            bits |= 1 << j;
        }
    }
    out->signbits = bits;
}

/*
=================
RadiusFromBounds
=================
*/
F32 RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
    S32             i;
    vec3_t          corner;
    F32           a, b;
    
    for ( i = 0; i < 3; i++ )
    {
        a = Q_fabs( mins[i] );
        b = Q_fabs( maxs[i] );
        corner[i] = a > b ? a : b;
    }
    
    return VectorLength( corner );
}

void ZeroBounds( vec3_t mins, vec3_t maxs )
{
    mins[0] = mins[1] = mins[2] = 0;
    maxs[0] = maxs[1] = maxs[2] = 0;
}

#ifndef Q3MAP2
void ClearBounds( vec3_t mins, vec3_t maxs )
{
    mins[0] = mins[1] = mins[2] = 99999;
    maxs[0] = maxs[1] = maxs[2] = -99999;
}
#endif

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
    if ( v[0] < mins[0] )
    {
        mins[0] = v[0];
    }
    if ( v[0] > maxs[0] )
    {
        maxs[0] = v[0];
    }
    
    if ( v[1] < mins[1] )
    {
        mins[1] = v[1];
    }
    if ( v[1] > maxs[1] )
    {
        maxs[1] = v[1];
    }
    
    if ( v[2] < mins[2] )
    {
        mins[2] = v[2];
    }
    if ( v[2] > maxs[2] )
    {
        maxs[2] = v[2];
    }
}

bool PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs )
{
    if ( v[0] < mins[0] )
    {
        return false;
    }
    if ( v[0] > maxs[0] )
    {
        return false;
    }
    
    if ( v[1] < mins[1] )
    {
        return false;
    }
    if ( v[1] > maxs[1] )
    {
        return false;
    }
    
    if ( v[2] < mins[2] )
    {
        return false;
    }
    if ( v[2] > maxs[2] )
    {
        return false;
    }
    
    return true;
}

void BoundsAdd( vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
    if ( mins2[0] < mins[0] )
    {
        mins[0] = mins2[0];
    }
    if ( mins2[1] < mins[1] )
    {
        mins[1] = mins2[1];
    }
    if ( mins2[2] < mins[2] )
    {
        mins[2] = mins2[2];
    }
    
    if ( maxs2[0] > maxs[0] )
    {
        maxs[0] = maxs2[0];
    }
    if ( maxs2[1] > maxs[1] )
    {
        maxs[1] = maxs2[1];
    }
    if ( maxs2[2] > maxs[2] )
    {
        maxs[2] = maxs2[2];
    }
}

bool BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
    // Though collision code is likely to use its own BoundsIntersect() version
    // optimized even further, this code is fine too for the rest of the code base
    
    __m128 xmmMins1 = _mm_set_ps( mins[0], mins[1], mins[2], 0 );
    __m128 xmmMaxs1 = _mm_set_ps( maxs[0], maxs[1], maxs[2], 1 );
    __m128 xmmMins2 = _mm_set_ps( mins2[0], mins2[1], mins2[2], 0 );
    __m128 xmmMaxs2 = _mm_set_ps( maxs2[0], maxs2[1], maxs2[2], 1 );
    
    __m128 cmp1 = _mm_cmpge_ps( xmmMins1, xmmMaxs2 );
    __m128 cmp2 = _mm_cmpge_ps( xmmMins2, xmmMaxs1 );
    __m128 orCmp = _mm_or_ps( cmp1, cmp2 );
    
    return _mm_movemask_epi8( _mm_cmpeq_epi32( _mm_castps_si128( orCmp ), _mm_setzero_si128() ) ) == 0xFFFF;
}

bool BoundsIntersectSphere( const vec3_t mins, const vec3_t maxs, const vec3_t origin, F32 radius )
{
    if ( origin[0] - radius > maxs[0] ||
            origin[0] + radius < mins[0] ||
            origin[1] - radius > maxs[1] ||
            origin[1] + radius < mins[1] || origin[2] - radius > maxs[2] || origin[2] + radius < mins[2] )
    {
        return false;
    }
    
    return true;
}

bool BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t origin )
{
    if ( origin[0] > maxs[0] ||
            origin[0] < mins[0] || origin[1] > maxs[1] || origin[1] < mins[1] || origin[2] > maxs[2] || origin[2] < mins[2] )
    {
        return false;
    }
    
    return true;
}

S32 VectorCompare( const vec3_t v1, const vec3_t v2 )
{
    __m128 cmp = _mm_cmpneq_ps( _mm_loadu_ps( v1 ), _mm_loadu_ps( v2 ) );
    return !( _mm_movemask_epi8( _mm_castps_si128( cmp ) ) & 0xFFF );
}

F32 VectorNormalize( vec3_t v )
{
    // NOTE: This is necessary to prevent an memory overwrite...
    // sice vec only has 3 floats, we can't "movaps" directly into it.
#if defined(_WIN32) || defined(_WIN64)
    __declspec( align( 16 ) ) F32 result[4];
#elif __LINUX__
    F32 result[4] __attribute__( ( aligned( 16 ) ) );
#endif
    
    F32* r = &result[0];
    
    F32 radius = 0.f;
    if ( v[0] || v[1] || v[2] )
    {
        __m128 _v = _mm_loadu_ps( v );
        __m128 res = _mm_mul_ps( _v, _v );
        
        // Shuffle
        __m128 v2 = _mm_shuffle_ps( res, res, 1 );
        __m128 v3 = _mm_shuffle_ps( res, res, 2 );
        
        // Adda all together & store
        res = _mm_add_ps( res, _mm_add_ps( v3, v2 ) );
        
        // Sqrt and store mag in res
        res = _mm_sqrt_ps( res );
        
        // Need to duplicate to all elems
        res = _mm_shuffle_ps( res, res, 0 );
        
        // Compute components again
        _v = _mm_div_ps( _v, res );
        
        // Store in result & set vec
        _mm_store_ps( r, _v );
        v[0] = r[0];
        v[1] = r[1];
        v[2] = r[2];
        
        float ret;
        _mm_store_ss( &ret, res );
        return ret;
    }
    
    return radius;
}

const __m128	f1 = _mm_set_ss( 1.0f );
const __m128	f3 = _mm_set_ss( 3.0f ); // 3 as SSE value
const __m128	f05 = _mm_set_ss( 0.5f ); // 0.5 as SSE value
const __m128	f0 = _mm_set_ss( 0.0f );
const __m128	fac2 = _mm_set_ss( 2.0f );
const __m128	fac3 = _mm_set_ss( 6.0f );
const __m128	fac4 = _mm_set_ss( 24.0f );
const __m128	fac5 = _mm_set_ss( 120.0f );
const __m128	fac6 = _mm_set_ss( 720.0f );
const __m128	fac7 = _mm_set_ss( 5040.0f );
const __m128	fac8 = _mm_set_ss( 40320.0f );
const __m128	fac9 = _mm_set_ss( 362880.0f );

F32 RSqrtAccurate( F32 a )
{
    __m128  xx = _mm_load_ss( &a );
    __m128  xr = _mm_rsqrt_ss( xx );
    __m128  xt;
    
    xt = _mm_mul_ss( xr, xr );
    xt = _mm_mul_ss( xt, xx );
    xt = _mm_sub_ss( f3, xt );
    xt = _mm_mul_ss( xt, f05 );
    xr = _mm_mul_ss( xr, xt );
    
    _mm_store_ss( &a, xr );
    return a;
}

//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast( vec3_t v )
{
    F32 ool = RSqrtAccurate( FLT_EPSILON + v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
    
    v[0] *= ool;
    v[1] *= ool;
    v[2] *= ool;
}

/* Used to compare floats when rounding errors could occur  */
#ifndef EQUAL
#define EQUAL(a,b) (fabsf((a)-(b))<0.0000000001f)
#endif

F32 VectorNormalize2( const vec3_t v, vec3_t out )
{
    F32 length;
    
    length = DotProduct( v, v );
    length = sqrtf( length );
    if ( !EQUAL( length, 0.0f ) )
    {
        const F32 ilength = 1.0f / length;
        out[0] = v[0] * ilength;
        out[1] = v[1] * ilength;
        out[2] = v[2] * ilength;
    }
    
    return length;
}

void _VectorMA( const vec3_t veca, F32 scale, const vec3_t vecb, vec3_t vecc )
{
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
}

inline __m128 dotProduct3D( __m128 v1, __m128 v2 )
{
    return _mm_dp_ps( v1, v2, 0x71 );
}

F32 DotProduct( const vec3_t v1, const vec3_t v2 )
{
    return _mm_cvtss_f32( dotProduct3D( _mm_loadu_ps( v1 ), _mm_loadu_ps( v2 ) ) );
}

void VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
    __m128 xmm_veca, xmm_vecb, xmm_out;
    
    xmm_veca = _mm_load_ss( &veca[0] );
    xmm_vecb = _mm_load_ss( &vecb[0] );
    xmm_out = _mm_sub_ss( xmm_veca, xmm_vecb );
    _mm_store_ss( &out[0], xmm_out );
    
    xmm_veca = _mm_load_ss( &veca[1] );
    xmm_vecb = _mm_load_ss( &vecb[1] );
    xmm_out = _mm_sub_ss( xmm_veca, xmm_vecb );
    _mm_store_ss( &out[1], xmm_out );
    
    xmm_veca = _mm_load_ss( &veca[2] );
    xmm_vecb = _mm_load_ss( &vecb[2] );
    xmm_out = _mm_sub_ss( xmm_veca, xmm_vecb );
    _mm_store_ss( &out[2], xmm_out );
}

void VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
    __m128 xmm_veca, xmm_vecb, xmm_out;
    
    xmm_veca = _mm_load_ss( &veca[0] );
    xmm_vecb = _mm_load_ss( &vecb[0] );
    xmm_out = _mm_add_ss( xmm_veca, xmm_vecb );
    _mm_store_ss( &out[0], xmm_out );
    
    xmm_veca = _mm_load_ss( &veca[1] );
    xmm_vecb = _mm_load_ss( &vecb[1] );
    xmm_out = _mm_add_ss( xmm_veca, xmm_vecb );
    _mm_store_ss( &out[1], xmm_out );
    
    xmm_veca = _mm_load_ss( &veca[2] );
    xmm_vecb = _mm_load_ss( &vecb[2] );
    xmm_out = _mm_add_ss( xmm_veca, xmm_vecb );
    _mm_store_ss( &out[2], xmm_out );
}

void _VectorCopy( const vec3_t in, vec3_t out )
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

void _VectorScale( const vec3_t in, F32 scale, vec3_t out )
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

#ifndef Q3MAP2
F32 VectorLength( const vec3_t v )
{
    return sqrtf( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}
#endif

F32 VectorLengthSquared( const vec3_t v )
{
    return ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

F32 Distance( const vec3_t p1, const vec3_t p2 )
{
    vec3_t          v;
    
    VectorSubtract( p2, p1, v );
    return VectorLength( v );
}

F32 DistanceSquared( const vec3_t p1, const vec3_t p2 )
{
    vec3_t          v;
    
    VectorSubtract( p2, p1, v );
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

#ifndef Q3MAP2
void VectorInverse( vec3_t v )
{
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}
#endif

void Vector4Scale( const vec4_t in, F32 scale, vec4_t out )
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
    out[3] = in[3] * scale;
}

S32 NearestPowerOfTwo( S32 val )
{
    S32             answer;
    
    for ( answer = 1; answer < val; answer <<= 1 )
        ;
    return answer;
}

S32 Q_log2( S32 val )
{
    S32             answer;
    
    answer = 0;
    while ( ( val >>= 1 ) != 0 )
    {
        answer++;
    }
    return answer;
}

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

S32 i;
i = 1065353246;
acos(*(F32*) &i) == -1.#IND0
=====================
*/
F32 Q_acos( F32 c )
{
    F32           angle;
    
    angle = acosf( c );
    
    if ( angle > M_PI )
    {
        return ( F32 )M_PI;
    }
    
    if ( angle < -M_PI )
    {
        return ( F32 )M_PI;
    }
    
    return angle;
}

/*
================
AxisMultiply
================
*/
void AxisMultiply( F32 in1[3][3], F32 in2[3][3], F32 out[3][3] )
{
    out[0][0] = ( in1[0][0] * in2[0][0] ) + ( in1[0][1] * in2[1][0] ) + ( in1[0][2] * in2[2][0] );
    out[0][1] = ( in1[0][0] * in2[0][1] ) + ( in1[0][1] * in2[1][1] ) + ( in1[0][2] * in2[2][1] );
    out[0][2] = ( in1[0][0] * in2[0][2] ) + ( in1[0][1] * in2[1][2] ) + ( in1[0][2] * in2[2][2] );
    out[1][0] = ( in1[1][0] * in2[0][0] ) + ( in1[1][1] * in2[1][0] ) + ( in1[1][2] * in2[2][0] );
    out[1][1] = ( in1[1][0] * in2[0][1] ) + ( in1[1][1] * in2[1][1] ) + ( in1[1][2] * in2[2][1] );
    out[1][2] = ( in1[1][0] * in2[0][2] ) + ( in1[1][1] * in2[1][2] ) + ( in1[1][2] * in2[2][2] );
    out[2][0] = ( in1[2][0] * in2[0][0] ) + ( in1[2][1] * in2[1][0] ) + ( in1[2][2] * in2[2][0] );
    out[2][1] = ( in1[2][0] * in2[0][1] ) + ( in1[2][1] * in2[1][1] ) + ( in1[2][2] * in2[2][1] );
    out[2][2] = ( in1[2][0] * in2[0][2] ) + ( in1[2][1] * in2[1][2] ) + ( in1[2][2] * in2[2][2] );
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
    const F32 deg2Rad = ( F32 )( M_PI ) / 180.0f;
    
    const F32 yaw = deg2Rad * angles[YAW];
    const F32 sy = sinf( yaw );
    const F32 cy = cosf( yaw );
    
    const F32 pitch = deg2Rad * angles[PITCH];
    const F32 sp = sinf( pitch );
    const F32 cp = cosf( pitch );
    
    const F32 roll = deg2Rad * angles[ROLL];
    const F32 sr = sinf( roll );
    const F32 cr = cosf( roll );
    
    if ( forward )
    {
        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;
    }
    
    if ( right )
    {
        const F32 t = sr * sp;
        right[0] = ( -1 * t * cy + -1 * cr * -sy );
        right[1] = ( -1 * t * sy + -1 * cr * cy );
        right[2] = -1 * sr * cp;
    }
    
    if ( up )
    {
        const F32 t = cr * sp;
        up[0] = ( t * cy + -sr * -sy );
        up[1] = ( t * sy + -sr * cy );
        up[2] = cr * cp;
    }
}

/*
=================
PerpendicularVector

assumes "src" is normalized
=================
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
    S32             pos;
    S32             i;
    F32           minelem = 1.0F;
    vec3_t          tempvec;
    
    /*
     ** find the smallest magnitude axially aligned vector
     */
    for ( pos = 0, i = 0; i < 3; i++ )
    {
        if ( Q_fabs( src[i] ) < minelem )
        {
            pos = i;
            minelem = Q_fabs( src[i] );
        }
    }
    tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
    tempvec[pos] = 1.0F;
    
    /*
     ** project the point onto the plane defined by src
     */
    ProjectPointOnPlane( dst, tempvec, src );
    
    /*
     ** normalize the result
     */
    VectorNormalize( dst );
}

// Ridah
/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
    vec3_t          v1, v2;
    
    VectorSubtract( point, p1, v1 );
    VectorNormalize( v1 );
    
    VectorSubtract( point, p2, v2 );
    VectorNormalize( v2 );
    
    CrossProduct( v1, v2, up );
    VectorNormalize( up );
}

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
    vec3_t          pVec, vec;
    
    VectorSubtract( point, vStart, pVec );
    VectorSubtract( vEnd, vStart, vec );
    VectorNormalize( vec );
    // project onto the directional vector for this segment
    VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

#define LINE_DISTANCE_EPSILON 1e-05f

/*
================
DistanceBetweenLineSegmentsSquared
Return the smallest distance between two line segments, squared
================
*/

F32 DistanceBetweenLineSegmentsSquared( const vec3_t sP0, const vec3_t sP1,
                                        const vec3_t tP0, const vec3_t tP1, F32* s, F32* t )
{
    vec3_t          sMag, tMag, diff;
    F32           a, b, c, d, e;
    F32           D;
    F32           sN, sD;
    F32           tN, tD;
    vec3_t          separation;
    
    VectorSubtract( sP1, sP0, sMag );
    VectorSubtract( tP1, tP0, tMag );
    VectorSubtract( sP0, tP0, diff );
    a = DotProduct( sMag, sMag );
    b = DotProduct( sMag, tMag );
    c = DotProduct( tMag, tMag );
    d = DotProduct( sMag, diff );
    e = DotProduct( tMag, diff );
    sD = tD = D = a * c - b * b;
    
    if ( D < LINE_DISTANCE_EPSILON )
    {
        // the lines are almost parallel
        sN = 0.0;				// force using point P0 on segment S1
        sD = 1.0;				// to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    }
    else
    {
        // get the closest points on the infinite  lines
        sN = ( b * e - c * d );
        tN = ( a * e - b * d );
        
        if ( sN < 0.0 )
        {
            // sN < 0 => the s=0 edge is visible
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if ( sN > sD )
        {
            // sN > sD => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }
    
    if ( tN < 0.0 )
    {
        // tN < 0 => the t=0 edge is visible
        tN = 0.0;
        
        // recompute sN for this edge
        if ( -d < 0.0 )
            sN = 0.0;
        else if ( -d > a )
            sN = sD;
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if ( tN > tD )
    {
        // tN > tD => the t=1 edge is visible
        tN = tD;
        
        // recompute sN for this edge
        if ( ( -d + b ) < 0.0 )
            sN = 0;
        else if ( ( -d + b ) > a )
            sN = sD;
        else
        {
            sN = ( -d + b );
            sD = a;
        }
    }
    
    // finally do the division to get *s and *t
    *s = ( fabsf( sN ) < LINE_DISTANCE_EPSILON ? 0.0f : sN / sD );
    *t = ( fabsf( tN ) < LINE_DISTANCE_EPSILON ? 0.0f : tN / tD );
    
    // get the difference of the two closest points
    VectorScale( sMag, *s, sMag );
    VectorScale( tMag, *t, tMag );
    VectorAdd( diff, sMag, separation );
    VectorSubtract( separation, tMag, separation );
    
    return VectorLengthSquared( separation );
    
}

/*
================
DistanceBetweenLineSegments

Return the smallest distance between two line segments
================
*/

F32 DistanceBetweenLineSegments( const vec3_t sP0, const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, F32* s, F32* t )
{
    return ( F32 ) sqrt( DistanceBetweenLineSegmentsSquared( sP0, sP1, tP0, tP1, s, t ) );
}

/*
================
ProjectPointOntoVectorBounded
================
*/
void ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
    vec3_t          pVec, vec;
    S32             j;
    
    VectorSubtract( point, vStart, pVec );
    VectorSubtract( vEnd, vStart, vec );
    VectorNormalize( vec );
    // project onto the directional vector for this segment
    VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
    // check bounds
    for ( j = 0; j < 3; j++ )
        if ( ( vProj[j] > vStart[j] && vProj[j] > vEnd[j] ) || ( vProj[j] < vStart[j] && vProj[j] < vEnd[j] ) )
        {
            break;
        }
    if ( j < 3 )
    {
        if ( Q_fabs( vProj[j] - vStart[j] ) < Q_fabs( vProj[j] - vEnd[j] ) )
        {
            VectorCopy( vStart, vProj );
        }
        else
        {
            VectorCopy( vEnd, vProj );
        }
    }
}

/*
================
DistanceFromLineSquared
================
*/
F32 DistanceFromLineSquared( vec3_t p, vec3_t lp1, vec3_t lp2 )
{
    vec3_t          proj, t;
    S32             j;
    
    ProjectPointOntoVector( p, lp1, lp2, proj );
    for ( j = 0; j < 3; j++ )
        if ( ( proj[j] > lp1[j] && proj[j] > lp2[j] ) || ( proj[j] < lp1[j] && proj[j] < lp2[j] ) )
        {
            break;
        }
    if ( j < 3 )
    {
        if ( Q_fabs( proj[j] - lp1[j] ) < Q_fabs( proj[j] - lp2[j] ) )
        {
            VectorSubtract( p, lp1, t );
        }
        else
        {
            VectorSubtract( p, lp2, t );
        }
        return VectorLengthSquared( t );
    }
    VectorSubtract( p, proj, t );
    return VectorLengthSquared( t );
}

/*
================
DistanceFromVectorSquared
================
*/
F32 DistanceFromVectorSquared( vec3_t p, vec3_t lp1, vec3_t lp2 )
{
    vec3_t          proj, t;
    
    ProjectPointOntoVector( p, lp1, lp2, proj );
    VectorSubtract( p, proj, t );
    return VectorLengthSquared( t );
}

F32 vectoyaw( const vec3_t vec )
{
    F32           yaw;
    
    if ( vec[YAW] == 0 && vec[PITCH] == 0 )
    {
        yaw = 0;
    }
    else
    {
        if ( vec[PITCH] )
        {
            yaw = ( atan2f( vec[YAW], vec[PITCH] ) * 180 / M_PI );
        }
        else if ( vec[YAW] > 0 )
        {
            yaw = 90;
        }
        else
        {
            yaw = 270;
        }
        if ( yaw < 0 )
        {
            yaw += 360;
        }
    }
    
    return yaw;
}

/*
=================
AxisToAngles

  Used to convert the MD3 tag axis to MDC tag angles, which are much smaller

  This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
  using this during gameplay
=================
*/
void AxisToAngles( /*const*/ vec3_t axis[3], vec3_t angles )
{
    F32 length1;
    F32 yaw, pitch, roll = 0.0f;
    
    if ( axis[0][1] == 0 && axis[0][0] == 0 )
    {
        yaw = 0;
        if ( axis[0][2] > 0 )
        {
            pitch = 90;
        }
        else
        {
            pitch = 270;
        }
    }
    else
    {
        if ( axis[0][0] )
        {
            yaw = ( atan2f( axis[0][1], axis[0][0] ) * 180 / M_PI );
        }
        else if ( axis[0][1] > 0 )
        {
            yaw = 90;
        }
        else
        {
            yaw = 270;
        }
        if ( yaw < 0 )
        {
            yaw += 360;
        }
        
        length1 = sqrtf( axis[0][0] * axis[0][0] + axis[0][1] * axis[0][1] );
        pitch = ( atan2f( axis[0][2], length1 ) * 180 / M_PI );
        if ( pitch < 0 )
        {
            pitch += 360;
        }
        
        roll = ( atan2f( axis[1][2], axis[2][2] ) * 180 / M_PI );
        if ( roll < 0 )
        {
            roll += 360;
        }
    }
    
    angles[PITCH] = -pitch;
    angles[YAW] = yaw;
    angles[ROLL] = roll;
}

F32 VectorDistance( vec3_t v1, vec3_t v2 )
{
    vec3_t          dir;
    
    VectorSubtract( v2, v1, dir );
    return VectorLength( dir );
}

F32 VectorDistanceSquared( vec3_t v1, vec3_t v2 )
{
    vec3_t          dir;
    
    VectorSubtract( v2, v1, dir );
    return VectorLengthSquared( dir );
}

// done.
/*
================
VectorMaxComponent

Return the biggest component of some vector
================
*/
F32 VectorMaxComponent( vec3_t v )
{
    F32 biggest = v[ 0 ];
    
    if ( v[ 1 ] > biggest )
        biggest = v[ 1 ];
        
    if ( v[ 2 ] > biggest )
        biggest = v[ 2 ];
        
    return biggest;
}

/*
================
VectorMinComponent

Return the smallest component of some vector
================
*/
F32 VectorMinComponent( vec3_t v )
{
    F32 smallest = v[ 0 ];
    
    if ( v[ 1 ] < smallest )
        smallest = v[ 1 ];
        
    if ( v[ 2 ] < smallest )
        smallest = v[ 2 ];
        
    return smallest;
}

void MatrixFromAngles( matrix_t m, F32 pitch, F32 yaw, F32 roll )
{
    static F32    sr, sp, sy, cr, cp, cy;
    
    // static to help MS compiler fp bugs
    sp = sinf( DEG2RAD( pitch ) );
    cp = cosf( DEG2RAD( pitch ) );
    
    sy = sinf( DEG2RAD( yaw ) );
    cy = cosf( DEG2RAD( yaw ) );
    
    sr = sinf( DEG2RAD( roll ) );
    cr = cosf( DEG2RAD( roll ) );
    
    m[ 0] = cp * cy;
    m[ 4] = ( sr * sp * cy + cr * -sy );
    m[ 8] = ( cr * sp * cy + -sr * -sy );
    m[12] = 0;
    m[ 1] = cp * sy;
    m[ 5] = ( sr * sp * sy + cr * cy );
    m[ 9] = ( cr * sp * sy + -sr * cy );
    m[13] = 0;
    m[ 2] = -sp;
    m[ 6] = sr * cp;
    m[10] = cr * cp;
    m[14] = 0;
    m[ 3] = 0;
    m[ 7] = 0;
    m[11] = 0;
    m[15] = 1;
}

void MatrixSetupTransformFromRotation( matrix_t m, const matrix_t rot, const vec3_t origin )
{
    m[ 0] = rot[ 0];
    m[ 4] = rot[ 4];
    m[ 8] = rot[ 8];
    m[12] = origin[0];
    m[ 1] = rot[ 1];
    m[ 5] = rot[ 5];
    m[ 9] = rot[ 9];
    m[13] = origin[1];
    m[ 2] = rot[ 2];
    m[ 6] = rot[ 6];
    m[10] = rot[10];
    m[14] = origin[2];
    m[ 3] = 0;
    m[ 7] = 0;
    m[11] = 0;
    m[15] = 1;
}

void MatrixAffineInverse( const matrix_t in, matrix_t out )
{
#if 0
    MatrixCopy( in, out );
    MatrixInverse( out );
#else
    // Tr3B - cleaned up
    out[ 0] = in[ 0];
    out[ 4] = in[ 1];
    out[ 8] = in[ 2];
    out[ 1] = in[ 4];
    out[ 5] = in[ 5];
    out[ 9] = in[ 6];
    out[ 2] = in[ 8];
    out[ 6] = in[ 9];
    out[10] = in[10];
    out[ 3] = 0;
    out[ 7] = 0;
    out[11] = 0;
    out[15] = 1;
    
    out[12] = -( in[12] * out[ 0] + in[13] * out[ 4] + in[14] * out[ 8] );
    out[13] = -( in[12] * out[ 1] + in[13] * out[ 5] + in[14] * out[ 9] );
    out[14] = -( in[12] * out[ 2] + in[13] * out[ 6] + in[14] * out[10] );
#endif
}

void MatrixTransformNormal( const matrix_t m, const vec3_t in, vec3_t out )
{
    out[ 0] = m[ 0] * in[ 0] + m[ 4] * in[ 1] + m[ 8] * in[ 2];
    out[ 1] = m[ 1] * in[ 0] + m[ 5] * in[ 1] + m[ 9] * in[ 2];
    out[ 2] = m[ 2] * in[ 0] + m[ 6] * in[ 1] + m[10] * in[ 2];
}

void MatrixTransformNormal2( const matrix_t m, vec3_t inout )
{
    vec3_t          tmp;
    
    tmp[ 0] = m[ 0] * inout[ 0] + m[ 4] * inout[ 1] + m[ 8] * inout[ 2];
    tmp[ 1] = m[ 1] * inout[ 0] + m[ 5] * inout[ 1] + m[ 9] * inout[ 2];
    tmp[ 2] = m[ 2] * inout[ 0] + m[ 6] * inout[ 1] + m[10] * inout[ 2];
    
    VectorCopy( tmp, inout );
}

void MatrixTransformPoint( const matrix_t m, const vec3_t in, vec3_t out )
{
    out[ 0] = m[ 0] * in[ 0] + m[ 4] * in[ 1] + m[ 8] * in[ 2] + m[12];
    out[ 1] = m[ 1] * in[ 0] + m[ 5] * in[ 1] + m[ 9] * in[ 2] + m[13];
    out[ 2] = m[ 2] * in[ 0] + m[ 6] * in[ 1] + m[10] * in[ 2] + m[14];
}

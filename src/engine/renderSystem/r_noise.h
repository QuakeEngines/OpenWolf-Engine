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
// File name:   r_sky.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_NOISE_H__
#define __R_NOISE_H__

#pragma once

#define NOISE_SIZE 256
#define NOISE_MASK ( NOISE_SIZE - 1 )

#define VAL( a ) s_noise_perm[ ( a ) & ( NOISE_MASK )]
#define VALR( a ) s_random[ ( a ) & FUNCTABLE_MASK ]
#define INDEX( x, y, z, t ) VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )

static F32 s_noise_table[NOISE_SIZE];
static S32 s_noise_perm[NOISE_SIZE];
static S32 s_random[FUNCTABLE_SIZE];

//
// idRenderSystemNoiseLocal
//
class idRenderSystemNoiseLocal
{
public:
    idRenderSystemNoiseLocal();
    ~idRenderSystemNoiseLocal();
    
    static F32 GetNoiseValue( S32 x, S32 y, S32 z, S32 t );
    static void NoiseInit( void );
    static F32 NoiseGet4f( F32 x, F32 y, F32 z, F64 t );
    static S32 RandomOn( F64 t );
};

extern idRenderSystemNoiseLocal renderSystemNoiseLocal;

#endif //!__R_NOISE_H__

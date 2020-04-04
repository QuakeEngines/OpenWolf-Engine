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
// File name:   r_animation.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_ANIMATION_H__
#define __R_ANIMATION_H__

#pragma once

#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT (1.0f/(F32)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))

//
// idRenderSystemAnimationLocal
//
class idRenderSystemAnimationLocal
{
public:
    idRenderSystemAnimationLocal();
    ~idRenderSystemAnimationLocal();
    
    static S32 MDRCullModel( mdrHeader_t* header, trRefEntity_t* ent );
    static S32 MDRComputeFogNum( mdrHeader_t* header, trRefEntity_t* ent );
    static void MDRAddAnimSurfaces( trRefEntity_t* ent );
    static void MDRSurfaceAnim( mdrSurface_t* surface );
    static void UnCompress( F32 mat[3][4], const U8* comp );
};

extern idRenderSystemAnimationLocal renderSystemAnimaionLocal;

#endif //!__R_ANIMATION_H__
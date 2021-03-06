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

#ifndef __R_WORLD_H__
#define __R_WORLD_H__

#pragma once

//
// idRenderSystemWorldLocal
//
class idRenderSystemWorldLocal
{
public:
    idRenderSystemWorldLocal();
    ~idRenderSystemWorldLocal();
    
    static bool	CullSurface( msurface_t* surf, S32 entityNum );
    static S32 DlightSurface( msurface_t* surf, S32 dlightBits );
    static S32 PshadowSurface( msurface_t* surf, S32 pshadowBits );
    static bool IsPostRenderEntity( S32 refEntityNum, const trRefEntity_t* refEntity );
    static void AddWorldSurface( msurface_t* surf, S32 entityNum, S32 dlightBits, S32 pshadowBits, bool dontCache );
    static void AddBrushModelSurfaces( trRefEntity_t* ent );
    static void RecursiveWorldNode( mnode_t* node, U32 planeBits, U32 dlightBits, U32 pshadowBits );
    static mnode_t* PointInLeaf( const vec3_t p );
    static const U8* ClusterPVS( S32 cluster );
    static void MarkLeaves( void );
    static void AddWorldSurfaces( void );
};

extern idRenderSystemWorldLocal renderSystemWorldLocal;

#endif //!__R_WORLD_H__

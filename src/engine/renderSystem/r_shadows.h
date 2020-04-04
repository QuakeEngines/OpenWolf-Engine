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

#ifndef __R_SHADOWS_H__
#define __R_SHADOWS_H__

#pragma once

typedef struct
{
    S32 i2;
    S32 facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static edgeDef_t edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static S32 numEdgeDefs[SHADER_MAX_VERTEXES];
static S32 facing[SHADER_MAX_INDEXES / 3];
static vec3_t shadowXyz[SHADER_MAX_VERTEXES];

//
// idRenderSystemShadowsLocal
//
class idRenderSystemShadowsLocal
{
public:
    idRenderSystemShadowsLocal();
    ~idRenderSystemShadowsLocal();
    
    static void AddEdgeDef( S32 i1, S32 i2, S32 facing );
    static void RenderShadowEdges( void );
    static void ShadowTessEnd( void );
    static void ShadowFinish( void );
    static void ProjectionShadowDeform( void );
};

extern idRenderSystemShadowsLocal renderSystemShadowsLocal;

#endif //!__R_SHADOWS_H__

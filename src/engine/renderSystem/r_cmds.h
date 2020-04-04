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
// File name:   r_cmds.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_CMDS_H__
#define __R_CMDS_H__

#pragma once

#define MODE_RED_CYAN	1
#define MODE_RED_BLUE	2
#define MODE_RED_GREEN	3
#define MODE_GREEN_MAGENTA 4
#define MODE_MAX MODE_GREEN_MAGENTA

//
// idRenderSystemCmdsLocal
//
class idRenderSystemCmdsLocal
{
public:
    idRenderSystemCmdsLocal();
    ~idRenderSystemCmdsLocal();
    
    static void PerformanceCounters( void );
    static void IssueRenderCommands( bool runPerformanceCounters );
    static void IssuePendingRenderCommands( void );
    static void AddDrawSurfCmd( drawSurf_t* drawSurfs, S32 numDrawSurfs );
    static void AddConvolveCubemapCmd( cubemap_t* cubemaps, S32 cubemap, S32 cubeSide );
    static void AddPostProcessCmd( void );
    static bool ClipRegion( F32* x, F32* y, F32* w, F32* h, F32* s1, F32* t1, F32* s2, F32* t2 );
    static void SetColorMode( GLboolean* rgba, stereoFrame_t stereoFrame, S32 colormode );
};

template<typename B, typename T>
T GetCommandBuffer( B bytes, T type );

extern idRenderSystemCmdsLocal renderSystemCmdsLocal;

#endif //!__R_BSP_TECH3_H__

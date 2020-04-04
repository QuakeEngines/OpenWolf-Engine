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
// File name:   r_image_jpg.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_MARKS_H__
#define __R_MARKS_H__

#pragma once

#define MAX_VERTS_ON_POLY 64
#define MARKER_OFFSET 0	// 1
#define	SIDE_FRONT 0
#define	SIDE_BACK 1
#define	SIDE_ON	2

//
// idRenderSystemMarksLocal
//
class idRenderSystemMarksLocal
{
public:
    idRenderSystemMarksLocal();
    ~idRenderSystemMarksLocal();
    
    static void ChopPolyBehindPlane( S32 numInPoints, vec3_t inPoints[MAX_VERTS_ON_POLY], S32* numOutPoints,
                                     vec3_t outPoints[MAX_VERTS_ON_POLY], vec3_t normal, F32 dist, F32 epsilon );
    static void BoxSurfaces_r( mnode_t* node, vec3_t mins, vec3_t maxs, surfaceType_t** list, S32 listsize, S32* listlength, vec3_t dir );
    static void AddMarkFragments( S32 numClipPoints, vec3_t clipPoints[2][MAX_VERTS_ON_POLY], S32 numPlanes, vec3_t* normals, F32* dists,
                                  S32 maxPoints, vec3_t pointBuffer, S32 maxFragments, markFragment_t* fragmentBuffer, S32* returnedPoints, S32* returnedFragments,
                                  vec3_t mins, vec3_t maxs );
                                  
};

extern idRenderSystemMarksLocal renderSystemMarksLocal;

#endif //!__R_IMAGE_TGA_H__
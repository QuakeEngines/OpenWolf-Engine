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
// File name:   r_curve.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: This file does all of the processing necessary to turn a raw grid of points
//              read from the map file into a srfBspSurface_t ready for rendering.
//
//              The level of detail solution is direction independent, based only on subdivided
//              distance from the true curve.
//
//              Only a single entry point :
//
//              srfBspSurface_t * R_SubdividePatchToGrid(S32 width, S32 height, srfVert_t
//              points[MAX_PATCH_SIZE * MAX_PATCH_SIZE]) {
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_CURVE_H__
#define __R_CURVE_H__

#pragma once

#define	MAX_FACE_POINTS		2048

#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

//
// idRenderSystemCurveLocal
//
class idRenderSystemCurveLocal
{
public:
    idRenderSystemCurveLocal();
    ~idRenderSystemCurveLocal();
    
    static void LerpDrawVert( srfVert_t* a, srfVert_t* b, srfVert_t* out );
    static void Transpose( S32 width, S32 height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] );
    static void MakeMeshNormals( S32 width, S32 height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] );
    static void MakeMeshTangentVectors( S32 width, S32 height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], S32 numIndexes, U32 indexes[( MAX_GRID_SIZE - 1 ) * ( MAX_GRID_SIZE - 1 ) * 2 * 3] );
    static S32 MakeMeshIndexes( S32 width, S32 height, U32 indexes[( MAX_GRID_SIZE - 1 ) * ( MAX_GRID_SIZE - 1 ) * 2 * 3] );
    static void InvertCtrl( S32 width, S32 height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] );
    static void InvertErrorTable( F32 errorTable[2][MAX_GRID_SIZE], S32 width, S32 height );
    static void PutPointsOnCurve( srfVert_t	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], S32 width, S32 height );
    static void CreateSurfaceGridMesh( srfBspSurface_t* grid, S32 width, S32 height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], F32 errorTable[2][MAX_GRID_SIZE], S32 numIndexes, U32 indexes[( MAX_GRID_SIZE - 1 ) * ( MAX_GRID_SIZE - 1 ) * 2 * 3] );
    static void FreeSurfaceGridMeshData( srfBspSurface_t* grid );
    static void SubdividePatchToGrid( srfBspSurface_t* grid, S32 width, S32 height, srfVert_t points[MAX_PATCH_SIZE* MAX_PATCH_SIZE] );
    static void GridInsertColumn( srfBspSurface_t* grid, S32 column, S32 row, vec3_t point, F32 loderror );
    static void GridInsertRow( srfBspSurface_t* grid, S32 row, S32 column, vec3_t point, F32 loderror );
};

extern idRenderSystemCurveLocal renderSystemCurveLocal;

#endif //!__R_CURVE_H__
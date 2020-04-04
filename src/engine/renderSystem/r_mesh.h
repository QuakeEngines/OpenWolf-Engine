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

#ifndef __R_MESH_H__
#define __R_MESH_H__

#pragma once

//
// idRenderSystemImageTGALocal
//
class idRenderSystemMeshLocal
{
public:
    idRenderSystemMeshLocal();
    ~idRenderSystemMeshLocal();
    
    static F32 ProjectRadius( F32 r, vec3_t location );
    static S32 CullModel( mdvModel_t* model, trRefEntity_t* ent );
    static S32 ComputeLOD( trRefEntity_t* ent );
    static S32 ComputeFogNum( mdvModel_t* model, trRefEntity_t* ent );
    static void AddMD3Surfaces( trRefEntity_t* ent );
};

extern idRenderSystemMeshLocal renderSystemMeshLocal;

#endif //!__R_MESH_H__
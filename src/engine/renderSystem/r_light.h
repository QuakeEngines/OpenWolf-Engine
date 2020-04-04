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

#ifndef __R_LIGHT_H__
#define __R_LIGHT_H__

#pragma once

#define	DLIGHT_AT_RADIUS		16
// at the edge of a dlight's influence, this amount of light will be added

#define	DLIGHT_MINIMUM_RADIUS	16
// never calculate a range less than this to prevent huge light numbers

#define DLIGHT_SHADOW_MINZDIST 30
// so the shadows won't get to long.

extern convar_t* r_ambientScale;
extern convar_t* r_directedScale;
extern convar_t* r_debugLight;

//
// idRenderSystemLightLocal
//
class idRenderSystemLightLocal
{
public:
    idRenderSystemLightLocal();
    ~idRenderSystemLightLocal();
    
    static void TransformDlights( S32 count, dlight_t* dl, orientationr_t* orientation );
    static void DlightBmodel( bmodel_t* bmodel );
    static void LogLight( trRefEntity_t* ent );
    static void AddLightToEntity( trRefEntity_t* ent, dlight_t* light, F32 dist );
    static void CalcLightsForEntity( trRefEntity_t* ent, dlight_t* lights, S32 numdlights, vec3_t lightOrigin, vec3_t lightDir );
    static bool LightDirForPoint( vec3_t point, vec3_t lightDir, vec3_t normal, world_t* world );
    static S32 CubemapForPoint( vec3_t point );
    static void SetupEntityLightingGrid( trRefEntity_t* ent, world_t* world );
    static void SetupEntityLighting( const trRefdef_t* refdef, trRefEntity_t* ent );
};

extern idRenderSystemLightLocal renderSystemLightLocal;

#endif //!__R_LIGHT_H__
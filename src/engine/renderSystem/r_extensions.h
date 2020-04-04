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
//              srfBspSurface_t * idRenderSystemCurveLocal::SubdividePatchToGrid(S32 width,
//              S32 height, srfVert_tpoints[MAX_PATCH_SIZE * MAX_PATCH_SIZE]) {
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_EXTENSIONS_H__
#define __R_EXTENSIONS_H__

#pragma once

//
// idRenderSystemCurveLocal
//
class idRenderSystemExtensionsLocal
{
public:
    idRenderSystemExtensionsLocal();
    ~idRenderSystemExtensionsLocal();
    
    static void InitExtraExtensions( void );
};

extern idRenderSystemExtensionsLocal renderSystemExtensionsLocal;

#endif //!__R_EXTENSIONS_H__
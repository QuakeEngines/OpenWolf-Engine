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

#ifndef __R_MODEL_H__
#define __R_MODEL_H__

#pragma once

//
// idRenderSystemModelLocal
//
class idRenderSystemModelLocal
{
public:
    idRenderSystemModelLocal();
    ~idRenderSystemModelLocal();
    
    static qhandle_t RegisterMD3( StringEntry name, model_t* mod );
    static qhandle_t RegisterMDR( StringEntry name, model_t* mod );
    static qhandle_t RegisterIQM( StringEntry name, model_t* mod );
    static model_t* GetModelByHandle( qhandle_t index );
    static model_t* AllocModel( void );
    static bool LoadMD3( model_t* mod, S32 lod, void* buffer, S32 bufferSize, StringEntry modName );
    static bool LoadMDR( model_t* mod, void* buffer, S32 filesize, StringEntry mod_name );
    static void ModelInit( void );
    static void Modellist_f( void );
    static mdvTag_t* GetTag( mdvModel_t* mod, S32 frame, StringEntry _tagName );
    static mdvTag_t* GetAnimTag( mdrHeader_t* mod, S32 framenum, StringEntry tagName, mdvTag_t* dest );
};

extern idRenderSystemModelLocal renderSystemModelLocal;

#endif //!__R_MODEL_H__
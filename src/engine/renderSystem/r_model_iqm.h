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

#ifndef __R_MODEL_IQM_H__
#define __R_MODEL_IQM_H__

#pragma once

//
// idRenderSystelModelIQMLocal
//
class idRenderSystelModelIQMLocal
{
public:
    idRenderSystelModelIQMLocal();
    ~idRenderSystelModelIQMLocal();
    
    static bool CheckRange( iqmHeader_t* header, U32 offset, S32 count, S32 size );
    static void Matrix34Multiply( const F32* a, const F32* b, F32* out );
    static void JointToMatrix( const quat_t rot, const vec3_t scale, const vec3_t trans, F32* mat );
    static void Matrix34Invert( const F32* inMat, F32* outMat );
    static void QuatSlerp( const quat_t from, const quat_t _to, F32 fraction, quat_t out );
    static F32 QuatNormalize2( const quat_t v, quat_t out );
    static bool LoadIQM( model_t* mod, void* buffer, U32 filesize, StringEntry mod_name );
    static S32 CullIQM( iqmData_t* data, trRefEntity_t* ent );
    static S32 ComputeIQMFogNum( iqmData_t* data, trRefEntity_t* ent );
    static void AddIQMSurfaces( trRefEntity_t* ent );
    static void ComputePoseMats( iqmData_t* data, S32 frame, S32 oldframe, F32 backlerp, F32* poseMats );
    static void ComputeJointMats( iqmData_t* data, S32 frame, S32 oldframe, F32 backlerp, F32* mat );
    static void IQMSurfaceAnim( surfaceType_t* surface );
    static void IQMSurfaceAnimVao( srfVaoIQModel_t* surface );
    static S32 IQMLerpTag( orientation_t* tag, iqmData_t* data, S32 startFrame, S32 endFrame, F32 frac, StringEntry tagName );
};

extern idRenderSystelModelIQMLocal renderSystemIQMModelLocal;

#endif //!__R_MODEL_IQM_H__

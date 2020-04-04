////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 Andrei Drexler, Richard Allen, James Canete
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
// File name:   r_postprocess.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_POSTPROCESS_H__
#define __R_POSTPROCESS_H__

#pragma once

//
// idRenderSystemPostProcessLocal
//
class idRenderSystemPostProcessLocal
{
public:
    idRenderSystemPostProcessLocal();
    ~idRenderSystemPostProcessLocal();
    
    static void ToneMap( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox, S32 autoExposure );
    static void BokehBlur( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox, F32 blur );
    static void RadialBlur( FBO_t* srcFbo, FBO_t* dstFbo, S32 passes, F32 stretch, F32 x, F32 y, F32 w, F32 h, F32 xcenter, F32 ycenter, F32 alpha );
    static bool UpdateSunFlareVis( void );
    static void SunRays( FBO_t* srcFbo, ivec4_t srcBox, FBO_t* dstFbo, ivec4_t dstBox );
    static void TextureDetail( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void RBM( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void Contrast( FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox );
    static void DarkExpand( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void Anamorphic( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void LensFlare( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void HDR( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void Anaglyph( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void FXAA( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void ESharpening( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void ESharpening2( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void TextureClean( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void DOF( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void MultiPost( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void Vibrancy( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void Bloom( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void SSGI( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void ScreenSpaceReflections( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
    static void Underwater( FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox );
};

#endif //!__R_POSTPROCESS_H__

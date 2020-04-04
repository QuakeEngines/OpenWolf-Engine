////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2013 - 2016 SomaZ
// Copyright(C) 2018 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_allocator.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_GLSL_PARSE_H__
#define __R_GLSL_PARSE_H__

#pragma once

//
// idRenderSystemGLSLParseLocal
//
class idRenderSystemGLSLParseLocal
{
public:
    idRenderSystemGLSLParseLocal();
    ~idRenderSystemGLSLParseLocal();
    
    //static Block* FindBlock( StringEntry name, Block* blocks, U64 numBlocks );
    static GPUProgramDesc ParseProgramSource( idRenderSystemAllocator& allocator, StringEntry text );
};

extern idRenderSystemGLSLParseLocal renderSystemGLSLParseLocal;

#endif //!__R_GLSL_PARSE_H__
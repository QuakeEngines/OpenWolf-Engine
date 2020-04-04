////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2013 - 2016 SomaZ
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
// File name:   r_allocator.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_ALOCATOR_H__
#define __R_ALOCATOR_H__

#pragma once

class idRenderSystemAllocator
{
public:
    idRenderSystemAllocator( void* memory, size_t memorySize, size_t alignment = 16 );
    idRenderSystemAllocator( size_t memorySize, size_t alignment = 16 );
    ~idRenderSystemAllocator();
    
    idRenderSystemAllocator( const idRenderSystemAllocator& ) = delete;
    idRenderSystemAllocator& operator=( const idRenderSystemAllocator& ) = delete;
    
    size_t GetSize() const;
    void* Base() const;
    void* Alloc( size_t allocSize );
    void* Mark() const;
    void Reset();
    void ResetTo( void* mark );
    
private:
    size_t alignment;
    bool ownMemory;
    void* unalignedBase;
    void* alignedBase;
    void* mark;
    void* end;
};

template<typename T>
T* RenderSystemAllocArray( idRenderSystemAllocator& allocator, U64 count )
{
    return static_cast<T*>( allocator.Alloc( sizeof( T ) * count ) );
}

inline UTF8* RenderSystemAllocString( idRenderSystemAllocator& allocator, U64 stringLength )
{
    return RenderSystemAllocArray<UTF8>( allocator, stringLength + 1 );
}

template<typename T>
T* RenderSystemAlloc( idRenderSystemAllocator& allocator )
{
    return RenderSystemAllocArray<T>( allocator, 1 );
}

#endif //!__R_ALOCATOR_H__
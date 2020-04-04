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
// File name:   r_allocator.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

/*
=================
idRenderSystemAllocator::idRenderSystemAllocator
=================
*/
idRenderSystemAllocator::idRenderSystemAllocator( void* memory, size_t memorySize, size_t alignment )
    : alignment( alignment )
    , ownMemory( false )
    , unalignedBase( memory )
    , alignedBase( PADP( unalignedBase, alignment ) )
    , mark( alignedBase )
    , end( ( char* )unalignedBase + memorySize )
{
    assert( unalignedBase );
    assert( memorySize );
    assert( alignment );
}

idRenderSystemAllocator::idRenderSystemAllocator( size_t memorySize, size_t alignment )
    : alignment( alignment )
    , ownMemory( true )
#if defined(GLSL_BUILDTOOL)
    , unalignedBase( malloc( memorySize ) )
#else
    , unalignedBase( memorySystem->Malloc( memorySize ) )
#endif
    , alignedBase( PADP( unalignedBase, alignment ) )
    , mark( alignedBase )
    , end( ( char* )unalignedBase + memorySize )
{
    assert( unalignedBase );
    assert( memorySize );
    assert( alignment );
}

idRenderSystemAllocator::~idRenderSystemAllocator()
{
    if ( ownMemory )
    {
#if defined(GLSL_BUILDTOOL)
        free( unalignedBase );
#else
        memorySystem->Free( unalignedBase );
#endif
    }
}

/*
=================
RenderSystemAllocator::Alloc
=================
*/
void* idRenderSystemAllocator::Alloc( size_t allocSize )
{
    if ( ( size_t )( ( char* )end - ( char* )mark ) < allocSize )
    {
        assert( !"Allocator is out of memory" );
        return nullptr;
    }
    
    void* result = mark;
    size_t alignedSize = PAD( allocSize, alignment );
    
    mark = ( char* )mark + alignedSize;
    
    return result;
}

/*
=================
idRenderSystemAllocator::Mark
=================
*/
void* idRenderSystemAllocator::Mark( void ) const
{
    return mark;
}

/*
=================
idRenderSystemAllocator::Reset
=================
*/
void idRenderSystemAllocator::Reset( void )
{
    mark = alignedBase;
}

/*
=================
idRenderSystemAllocator::ResetTo
=================
*/
void idRenderSystemAllocator::ResetTo( void* m )
{
    mark = m;
}
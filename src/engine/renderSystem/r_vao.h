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

#ifndef __R_VAO_H__
#define __R_VAO_H__

#pragma once

typedef struct bufferCacheEntry_s
{
    void* data;
    S32 size;
    S32 bufferOffset;
}
bufferCacheEntry_t;

typedef struct queuedSurface_s
{
    srfVert_t* vertexes;
    S32 numVerts;
    U32* indexes;
    S32 numIndexes;
}
queuedSurface_t;

#define VAOCACHE_MAX_BUFFERED_SURFACES (1 << 16)
#if GL_INDEX_TYPE == GL_UNSIGNED_SHORT
#define VAOCACHE_VERTEX_BUFFER_SIZE (sizeof(srfVert_t) * USHRT_MAX)
#define VAOCACHE_INDEX_BUFFER_SIZE (sizeof(glIndex_t) * USHRT_MAX * 4)
#else // GL_UNSIGNED_INT
#define VAOCACHE_VERTEX_BUFFER_SIZE (16 * 1024 * 1024)
#define VAOCACHE_INDEX_BUFFER_SIZE (5 * 1024 * 1024)
#endif

#define VAOCACHE_MAX_QUEUED_VERTEXES (1 << 16)
#define VAOCACHE_MAX_QUEUED_INDEXES (VAOCACHE_MAX_QUEUED_VERTEXES * 6 / 4)

#define VAOCACHE_MAX_QUEUED_SURFACES 4096

static struct
{
    vao_t* vao;
    bufferCacheEntry_t indexEntries[VAOCACHE_MAX_BUFFERED_SURFACES];
    S32 indexChainLengths[VAOCACHE_MAX_BUFFERED_SURFACES];
    S32 numIndexEntries;
    S32 vertexOffset;
    S32 indexOffset;
    
    srfVert_t vertexes[VAOCACHE_MAX_QUEUED_VERTEXES];
    S32 vertexCommitSize;
    
    U32 indexes[VAOCACHE_MAX_QUEUED_INDEXES];
    S32 indexCommitSize;
    
    queuedSurface_t surfaceQueue[VAOCACHE_MAX_QUEUED_SURFACES];
    S32 numSurfacesQueued;
}
vc = { 0 };

//
// idRenderSystemVaoLocal
//
class idRenderSystemVaoLocal
{
public:
    idRenderSystemVaoLocal();
    ~idRenderSystemVaoLocal();
    static void VaoPackTangent( S16* out, vec4_t v );
    static void VaoPackNormal( S16* out, vec3_t v );
    static void VaoPackColor( U16* out, vec4_t c );
    static void VaoUnpackTangent( vec4_t v, S16* pack );
    static void VaoUnpackNormal( vec3_t v, S16* pack );
    static void SetVertexPointers( vao_t* vao );
    static vao_t* CreateVao( StringEntry name, U8* vertexes, S32 vertexesSize, U8* indexes, S32 indexesSize, vaoUsage_t usage );
    static vao_t* CreateVao2( StringEntry name, S32 numVertexes, srfVert_t* verts, S32 numIndexes, U32* indexes );
    static void BindVao( vao_t* vao );
    static void BindNullVao( void );
    static void InitVaos( void );
    static void ShutdownVaos( void );
    static void VaoList_f( void );
    static void UpdateTessVao( U32 attribBits );
    static void CacheCommit( void );
    static void CacheInit( void );
    static void CacheBindVao( void );
    static void CacheCheckAdd( bool* endSurface, bool* recycleVertexBuffer, bool* recycleIndexBuffer, S32 numVerts, S32 numIndexes );
    static void CacheRecycleVertexBuffer( void );
    static void CacheRecycleIndexBuffer( void );
    static void CacheInitNewSurfaceSet( void );
    static void CacheAddSurface( srfVert_t* verts, S32 numVerts, U32* indexes, S32 numIndexes );
};

extern idRenderSystemVaoLocal renderSystemVaoLocal;

#endif //!__R_VAO_H__

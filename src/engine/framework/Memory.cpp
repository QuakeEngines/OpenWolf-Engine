////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 - 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   Threads.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idMemorySystemLocal memorySystemLocal;
idMemorySystem* memorySystem = &memorySystemLocal;

/*
===============
idMemorySystemLocal::idMemorySystemLocal
===============
*/
idMemorySystemLocal::idMemorySystemLocal( void )
{
}

/*
===============
idMemorySystemLocal::idMemorySystemLocal
===============
*/
idMemorySystemLocal::~idMemorySystemLocal( void )
{
}

//qmutex_t* zone_mutex = nullptr;

/*
========================
idMemorySystemLocal::ClearZone
========================
*/
void idMemorySystemLocal::ClearZone( memzone_t* zone, S32 size )
{
    memblock_t* block;
    
    // set the entire zone to one free block
    zone->blocklist.next = zone->blocklist.prev = block = ( memblock_t* )( ( U8* ) zone + sizeof( memzone_t ) );
    // in use block
    zone->blocklist.tag = 1;
    zone->blocklist.id = 0;
    zone->blocklist.size = 0;
    zone->rover = block;
    zone->size = size;
    zone->used = 0;
    
    block->prev = block->next = &zone->blocklist;
    // free block
    block->tag = 0;
    block->id = ZONEID;
    block->size = size - sizeof( memzone_t );
}

/*
========================
idMemorySystemLocal::Free
========================
*/
void idMemorySystemLocal::Free( void* ptr )
{
    memblock_t* block, *other;
    memzone_t* zone;
    
    if ( !ptr )
    {
        Com_Error( ERR_DROP, "idMemorySystemLocal::Free: NULL pointer" );
    }
    
    block = ( memblock_t* )( ( U8* ) ptr - sizeof( memblock_t ) );
    if ( block->id != ZONEID )
    {
        Com_Error( ERR_FATAL, "idMemorySystemLocal::Free: freed a pointer without ZONEID" );
    }
    if ( block->tag == 0 )
    {
        Com_Error( ERR_FATAL, "idMemorySystemLocal::Free: freed a freed pointer" );
    }
    
    //threadsSystem->Mutex_Lock( zone_mutex );
    
    // if static memory
    if ( block->tag == TAG_STATIC )
    {
        return;
    }
    
    // check the memory trash tester
    if ( *( S32* )( ( U8* ) block + block->size - 4 ) != ZONEID )
    {
        Com_Error( ERR_FATAL, "idMemorySystemLocal:: memory block wrote past end" );
    }
    
    if ( block->tag == TAG_SMALL )
    {
        zone = smallzone;
    }
    else
    {
        zone = mainzone;
    }
    
    zone->used -= block->size;
    
    // set the block to something that should cause problems
    // if it is referenced...
    ::memset( ptr, 0xaa, block->size - sizeof( *block ) );
    
    // mark as free
    block->tag = 0;
    
    other = block->prev;
    if ( !other->tag )
    {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;
        
        if ( block == zone->rover )
        {
            zone->rover = other;
        }
        
        block = other;
    }
    
    zone->rover = block;
    
    other = block->next;
    
    if ( !other->tag )
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;
    }
    
    //threadsSystem->Mutex_Unlock( zone_mutex );
}

/*
================
idMemorySystemLocal::FreeTags
================
*/
void idMemorySystemLocal::FreeTags( memtag_t tag )
{
    S32 count;
    memzone_t* zone;
    
    if ( tag == TAG_SMALL )
    {
        zone = smallzone;
    }
    else
    {
        zone = mainzone;
    }
    count = 0;
    
    // use the rover as our pointer, because
    // memorySystem->Free automatically adjusts it
    zone->rover = zone->blocklist.next;
    do
    {
        if ( zone->rover->tag == tag )
        {
            count++;
            Free( ( void* )( zone->rover + 1 ) );
            continue;
        }
        
        zone->rover = zone->rover->next;
    }
    while ( zone->rover != &zone->blocklist );
}

/*
================
idMemorySystemLocal::TagMalloc
================
*/
void* idMemorySystemLocal::TagMalloc( size_t size, memtag_t tag )
{
    S32 extra;
    memblock_t* start, *rover, *_new, *base;
    memzone_t* zone;
    
    //threadsSystem->Mutex_Lock( zone_mutex );
    
    if ( !tag )
    {
        Com_Error( ERR_FATAL, "idMemorySystemLocal::TagMalloc: tried to use a 0 tag" );
    }
    
    if ( tag == TAG_SMALL )
    {
        zone = smallzone;
    }
    else
    {
        zone = mainzone;
    }
    
    // scan through the block list looking for the first free block
    // of sufficient size
    // account for size of block header
    size += sizeof( memblock_t );
    
    // space for memory trash tester
    size += 4;
    
    // align to 32/64 bit boundary
    size = PAD( size, sizeof( intptr_t ) );
    
    base = rover = zone->rover;
    start = base->prev;
    
    do
    {
        if ( rover == start )
        {
            // scaned all the way around the list
            Com_Error( ERR_FATAL, "idMemorySystemLocal::Malloc: failed on allocation of %i bytes from the %s zone",
                       size, zone == smallzone ? "small" : "main" );
            return NULL;
        }
        
        if ( rover->tag )
        {
            base = rover = rover->next;
        }
        else
        {
            rover = rover->next;
        }
    }
    while ( base->tag || base->size < size );
    
    // found a block big enough
    extra = ( S32 )( base->size - size );
    if ( extra > MINFRAGMENT )
    {
        // there will be a free fragment after the allocated block
        _new = ( memblock_t* )( ( U8* ) base + size );
        _new->size = extra;
        // free block
        _new->tag = 0;
        _new->prev = base;
        _new->id = ZONEID;
        _new->next = base->next;
        _new->next->prev = _new;
        base->next = _new;
        base->size = ( S32 )size;
    }
    
    // no longer a free block
    base->tag = tag;
    
    // next allocation will start looking here
    zone->rover = base->next;
    zone->used += base->size;
    
    base->id = ZONEID;
    
    // marker for memory trash testing
    *( S32* )( ( U8* ) base + base->size - 4 ) = ZONEID;
    
    //threadsSystem->Mutex_Unlock( zone_mutex );
    
    return ( void* )( ( U8* ) base + sizeof( memblock_t ) );
}

/*
========================
idMemorySystemLocal::Malloc
========================
*/
void* idMemorySystemLocal::Malloc( size_t size )
{
    void* buf;
    
    buf = TagMalloc( size, TAG_GENERAL );
    ::memset( buf, 0, size );
    
    return buf;
}

/*
========================
idMemorySystemLocal::SMalloc
========================
*/
void* idMemorySystemLocal::SMalloc( size_t size )
{
    return TagMalloc( size, TAG_SMALL );
}

/*
========================
idMemorySystemLocal::CheckHeap
========================
*/
void idMemorySystemLocal::CheckHeap( void )
{
    memblock_t* block;
    
    for ( block = mainzone->blocklist.next;; block = block->next )
    {
        if ( block->next == &mainzone->blocklist )
        {
            // all blocks have been hit
            break;
        }
        
        if ( ( U8* ) block + block->size != ( U8* ) block->next )
        {
            Com_Error( ERR_FATAL, "idMemorySystemLocal::CheckHeap: block size does not touch the next block\n" );
        }
        
        if ( block->next->prev != block )
        {
            Com_Error( ERR_FATAL, "idMemorySystemLocal::CheckHeap: next block doesn't have proper back link\n" );
        }
        
        if ( !block->tag && !block->next->tag )
        {
            Com_Error( ERR_FATAL, "idMemorySystemLocal::CheckHeap: two consecutive free blocks\n" );
        }
    }
}

/*
========================
idMemorySystemLocal::LogZoneHeap
========================
*/
void idMemorySystemLocal::LogZoneHeap( memzone_t* zone, UTF8* name )
{
    S32 size, numBlocks;
    size_t allocSize;
    UTF8 buf[4096];
    memblock_t* block;
    
    if ( !logfile_ || !fileSystem->Initialized() )
    {
        return;
    }
    
    size = numBlocks = 0;
    
    Q_snprintf( buf, sizeof( buf ), "\r\n================\r\n%s log\r\n================\r\n", name );
    
    fileSystem->Write( buf, ( S32 )::strlen( buf ), logfile_ );
    
    for ( block = zone->blocklist.next; block->next != &zone->blocklist; block = block->next )
    {
        if ( block->tag )
        {
            size += block->size;
            numBlocks++;
        }
    }
    
    // + 32 bit alignment
    allocSize = numBlocks * sizeof( memblock_t );
    
    Q_snprintf( buf, sizeof( buf ), "%d %s memory in %d blocks\r\n", size, name, numBlocks );
    fileSystem->Write( buf, ( S32 )::strlen( buf ), logfile_ );
    
    Q_snprintf( buf, sizeof( buf ), "%d %s memory overhead\r\n", size - allocSize, name );
    fileSystem->Write( buf, ( S32 )::strlen( buf ), logfile_ );
}

/*
========================
idMemorySystemLocal::AvailableZoneMemory
========================
*/
S32 idMemorySystemLocal::AvailableZoneMemory( const memzone_t* zone )
{
    return zone->size - zone->used;
}

/*
========================
idMemorySystemLocal::LogHeap
========================
*/
void idMemorySystemLocal::LogHeap( void )
{
    LogZoneHeap( mainzone, "MAIN" );
    LogZoneHeap( smallzone, "SMALL" );
}

static hunkblock_t* hunkblocks;

static hunkUsed_t hunk_low, hunk_high;
static hunkUsed_t* hunk_permanent, *hunk_temp;

static U8*    s_hunkData = NULL;
static S32      s_hunkTotal;

static S32      s_zoneTotal;
static S32      s_smallZoneTotal;

/*
=================
idMemorySystemLocal::Meminfo_f
=================
*/
void idMemorySystemLocal::Meminfo_f( void )
{
    S32 zoneBytes, zoneBlocks, smallZoneBytes, smallZoneBlocks, botlibBytes, rendererBytes, otherBytes, staticBytes, generalBytes;
    memblock_t*	block;
    
    zoneBytes = 0;
    botlibBytes = 0;
    rendererBytes = 0;
    otherBytes = 0;
    staticBytes = 0;
    generalBytes = 0;
    zoneBlocks = 0;
    
    for ( block = mainzone->blocklist.next ; ; block = block->next )
    {
        if ( cmdSystem->Argc() != 1 )
        {
            Com_Printf( "block:%p    size:%7i    tag:%3i\n", ( void* )block, block->size, block->tag );
        }
        
        if ( block->tag )
        {
            zoneBytes += block->size;
            zoneBlocks++;
            
            if ( block->tag == TAG_BOTLIB )
            {
                botlibBytes += block->size;
            }
            else if ( block->tag == TAG_RENDERER )
            {
                rendererBytes += block->size;
            }
            else if ( block->tag == TAG_STATIC )
            {
                staticBytes += block->size;
            }
            else if ( block->tag == TAG_GENERAL )
            {
                generalBytes += block->size;
            }
            else
                otherBytes += block->size;
        }
        
        if ( block->next == &mainzone->blocklist )
        {
            // all blocks have been hit
            break;
        }
        
        if ( ( U8* )block + block->size != ( U8* )block->next )
        {
            Com_Printf( "ERROR: block size does not touch the next block\n" );
        }
        
        if ( block->next->prev != block )
        {
            Com_Printf( "ERROR: next block doesn't have proper back link\n" );
        }
        
        if ( !block->tag && !block->next->tag )
        {
            Com_Printf( "ERROR: two consecutive free blocks\n" );
        }
    }
    
    smallZoneBytes = 0;
    smallZoneBlocks = 0;
    
    for ( block = smallzone->blocklist.next ; ; block = block->next )
    {
        if ( block->tag )
        {
            smallZoneBytes += block->size;
            smallZoneBlocks++;
        }
        
        if ( block->next == &smallzone->blocklist )
        {
            // all blocks have been hit
            break;
        }
    }
    
    Com_Printf( "%8i K total hunk\n", s_hunk.memSize / 1024 );
    Com_Printf( "%8i K total zone\n", s_zoneTotal / 1024 );
    Com_Printf( "\n" );
    
    Com_Printf( "%8i K used hunk (permanent)\n", s_hunk.permTop / 1024 );
    Com_Printf( "%8i K used hunk (temp)\n", s_hunk.tempTop / 1024 );
    Com_Printf( "%8i K used hunk (TOTAL)\n", ( s_hunk.permTop + s_hunk.tempTop ) / 1024 );
    Com_Printf( "\n" );
    
    Com_Printf( "%8i K max hunk (permanent)\n", s_hunk.permMax / 1024 );
    Com_Printf( "%8i K max hunk (temp)\n", s_hunk.tempMax / 1024 );
    Com_Printf( "%8i K max hunk (TOTAL)\n", ( s_hunk.permMax + s_hunk.tempMax ) / 1024 );
    Com_Printf( "\n" );
    
    Com_Printf( "%8i K max hunk since last Clear\n", s_hunk.maxEver / 1024 );
    Com_Printf( "%8i K hunk mem never touched\n", ( s_hunk.memSize - s_hunk.maxEver ) / 1024 );
    Com_Printf( "%8i hunk mark value\n", s_hunk.mark );
    Com_Printf( "\n" );
    
    Com_Printf( "\n" );
    Com_Printf( "%8i bytes in %i zone blocks\n", zoneBytes, zoneBlocks	);
    Com_Printf( "        %8i bytes in dynamic botlib\n", botlibBytes );
    Com_Printf( "        %8i bytes in dynamic renderer\n", rendererBytes );
    Com_Printf( "        %8i bytes in dynamic other\n", otherBytes );
    Com_Printf( "        %8i bytes in small Zone memory\n", smallZoneBytes );
    Com_Printf( "        %8i bytes in static server memory\n", staticBytes );
    Com_Printf( "        %8i bytes in general common memory\n", generalBytes );
}

/*
===============
idMemorySystemLocal::TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void idMemorySystemLocal::TouchMemory( void )
{
    S32	i, j, start, end, sum;
    memblock_t*	block;
    
    CheckHeap();
    
    start = idsystem->Milliseconds();
    
    sum = 0;
    
    for ( block = mainzone->blocklist.next ; ; block = block->next )
    {
        if ( block->tag )
        {
            j = block->size >> 2;
            
            // only need to touch each page
            for ( i = 0 ; i < j ; i += 64 )
            {
                sum += ( ( S32* )block )[i];
            }
        }
        
        if ( block->next == &mainzone->blocklist )
        {
            // all blocks have been hit
            break;
        }
    }
    
    end = idsystem->Milliseconds();
    
    Com_Printf( "idMemorySystemLocal::TouchMemory: %i msec\n", end - start );
}

/*
=================
idMemorySystemLocal::InitSmallZoneMemory
=================
*/
void idMemorySystemLocal::InitSmallZoneMemory( void )
{
    s_smallZoneTotal = 512 * 1024;
    
    //zone_mutex = ( qmutex_t* )threadsSystem->Mutex_Create();
    
    // bk001205 - was malloc
    smallzone = ( memzone_t* )calloc( s_smallZoneTotal, 1 );
    if ( !smallzone )
    {
        Com_Error( ERR_FATAL, "Small zone data failed to allocate %1.1f megs", ( F32 )s_smallZoneTotal / ( 1024 * 1024 ) );
    }
    
    ClearZone( smallzone, s_smallZoneTotal );
    
    return;
}

/*
=================
idMemorySystemLocal::InitZoneMemory
=================
*/
void idMemorySystemLocal::InitZoneMemory( void )
{
    convar_t* cv;
    
    // allocate the random block zone
    // config files and command line options haven't been taken in account yet
    Com_StartupVariable( "com_zoneMegs" );
    cv = cvarSystem->Get( "com_zoneMegs", DEF_COMZONEMEGS_S, CVAR_INIT, "description" );
    
    if ( cv->integer < DEF_COMZONEMEGS )
    {
        s_zoneTotal = 1024 * 1024 * DEF_COMZONEMEGS;
    }
    else
    {
        s_zoneTotal = cv->integer * 1024 * 1024;
    }
    
    // bk001205 - was malloc
    mainzone = ( memzone_t* )calloc( s_zoneTotal, 1 );
    if ( !mainzone )
    {
        Com_Error( ERR_FATAL, "Zone data failed to allocate %i megs", s_zoneTotal / ( 1024 * 1024 ) );
    }
    
    ClearZone( mainzone, s_zoneTotal );
    
}

/*
=================
idMemorySystemLocal::InitHunkMemory
=================
*/
void idMemorySystemLocal::InitHunkMemory( void )
{
    S32 nMinAlloc;
    UTF8* pMsg = nullptr;
    convar_t* cv;
    
    ::memset( &s_hunk, 0, sizeof( s_hunk ) );
    
    // make sure the file system has allocated and "not" freed any temp blocks
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if ( fileSystem->LoadStack() != 0 )
    {
        Com_Error( ERR_FATAL, "Hunk initialization failed. File system load stack not zero" );
    }
    
    // allocate the stack based hunk allocator
    cv = cvarSystem->Get( "com_hunkMegs", DEF_COMHUNKMEGS_S, CVAR_LATCH | CVAR_ARCHIVE, "description" );
    
    // if we are not dedicated min allocation is 56, otherwise min is 1
    if ( com_dedicated && com_dedicated->integer )
    {
        nMinAlloc = MIN_DEDICATED_COMHUNKMEGS;
        pMsg = "Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n";
    }
    else
    {
        nMinAlloc = MIN_COMHUNKMEGS;
        pMsg = "Minimum com_hunkMegs is %i, allocating %i megs.\n";
    }
    
    if ( cv->integer < nMinAlloc )
    {
        s_hunk.memSize = 1024 * 1024 * nMinAlloc;
        Com_Printf( pMsg, nMinAlloc, s_hunk.memSize / ( 1024 * 1024 ) );
    }
    else
    {
        s_hunk.memSize = cv->integer * 1024 * 1024;
    }
    
    // bk001205 - was malloc
    s_hunk.original = ( U8* )calloc( s_hunk.memSize + 31, 1 );
    if ( !s_hunk.original )
    {
        Com_Error( ERR_FATAL, "Hunk data failed to allocate %i megs", s_hunk.memSize / ( 1024 * 1024 ) );
    }
    
    // cacheline align
    s_hunk.mem = ( U8* )( ( ( intptr_t )s_hunk.original + 31 ) & ~31 );
    
    Clear();
    
    cmdSystem->AddCommand( "meminfo", &idMemorySystemLocal::Meminfo_f, "description" );
}

/*
====================
idMemorySystemLocal::MemoryRemaining
====================
*/
S32	idMemorySystemLocal::MemoryRemaining( void )
{
    return s_hunk.memSize - s_hunk.permTop - s_hunk.tempTop;
}

/*
===================
idMemorySystemLocal::SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void idMemorySystemLocal::SetMark( void )
{
    s_hunk.mark = s_hunk.permTop;
}

/*
=================
idMemorySystemLocal::ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void idMemorySystemLocal::ClearToMark( void )
{
    s_hunk.permTop = s_hunk.mark;
    s_hunk.permMax = s_hunk.permTop;
    
    s_hunk.tempMax = s_hunk.tempTop = 0;
}

/*
=================
idMemorySystemLocal::Clear

The server calls this before shutting down or loading a new map
=================
*/
void idMemorySystemLocal::Clear( void )
{
#ifndef DEDICATED
    clientGameSystem->ShutdownCGame();
    clientGUISystem->ShutdownGUI();
#endif
    
    serverGameSystem->ShutdownGameProgs();
    
#ifndef DEDICATED
    CIN_CloseAllVideos();
#endif
    
    s_hunk.permTop = 0;
    s_hunk.permMax = 0;
    s_hunk.tempTop = 0;
    s_hunk.tempMax = 0;
    s_hunk.maxEver = 0;
    s_hunk.mark = 0;
    
    Com_Printf( "Clear: reset the hunk ok\n" );
    
    //stake out a chunk for the frame temp data
    FrameInit();
}

/*
=================
idMemorySystemLocal::Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
void* idMemorySystemLocal::Alloc( size_t size, ha_pref preference )
{
    void*	buf;
    
    if ( s_hunk.mem == NULL )
    {
        Com_Error( ERR_FATAL, "idMemorySystemLocal::Alloc: Hunk memory system not initialized" );
    }
    
    // round to cacheline
    size = ( size + 31 ) & ~31;
    
    if ( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
    {
        Com_Error( ERR_DROP, "idMemorySystemLocal::Alloc failed on %i", size );
    }
    
    buf = s_hunk.mem + s_hunk.permTop;
    s_hunk.permTop += ( U64 )size;
    
    if ( s_hunk.permTop > s_hunk.permMax )
    {
        s_hunk.permMax = s_hunk.permTop;
    }
    
    if ( s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver )
    {
        s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;
    }
    
    ::memset( buf, 0, size );
    
    return buf;
}

/*
=================
idMemorySystemLocal::AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void* idMemorySystemLocal::AllocateTempMemory( size_t size )
{
    void* buf;
    hunkHeader_t* hdr;
    
    // return a Z_Malloc'd block if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if ( s_hunk.mem == NULL )
    {
        return Malloc( size );
    }
    
    size = PAD( size, sizeof( intptr_t ) ) + sizeof( hunkHeader_t );
    
    if ( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
    {
        Com_Error( ERR_DROP, "idMemorySystemLocal::AllocateTempMemory: failed on %i", size );
    }
    
    s_hunk.tempTop += ( U64 )size;
    buf = s_hunk.mem + s_hunk.memSize - s_hunk.tempTop;
    
    if ( s_hunk.tempTop > s_hunk.tempMax )
    {
        s_hunk.tempMax = s_hunk.tempTop;
    }
    
    if ( s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver )
    {
        s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;
    }
    
    hdr = ( hunkHeader_t* )buf;
    buf = ( void* )( hdr + 1 );
    
    hdr->magic = HUNK_MAGIC;
    hdr->size = size;
    
    // don't bother clearing, because we are going to load a file over it
    return buf;
}

/*
==================
idMemorySystemLocal::FreeTempMemory
==================
*/
void idMemorySystemLocal::FreeTempMemory( void* buf )
{
    hunkHeader_t* hdr;
    
    // free with memorySystem->Free if the hunk has not been initialized
    // this allows the config and product id files ( journal files too ) to be loaded
    // by the file system without redunant routines in the file system utilizing different
    // memory systems
    if ( !s_hunk.mem )
    {
        Free( buf );
        return;
    }
    
    
    hdr = ( hunkHeader_t* )buf - 1;
    if ( hdr->magic != HUNK_MAGIC )
    {
        Com_Error( ERR_FATAL, "idMemorySystemLocal::FreeTempMemory: bad magic" );
    }
    
    hdr->magic = HUNK_FREE_MAGIC;
    
    // this only works if the files are freed in stack order,
    // otherwise the memory will stay around until TempMemory
    if ( ( U8* )hdr == s_hunk.mem + s_hunk.memSize - s_hunk.tempTop )
    {
        s_hunk.tempTop -= ( U64 )hdr->size;
    }
}

/*
=================
idMemorySystemLocal::ClearTempMemory

The temp space is no longer needed.  If we have left more
touched but unused memory on this side, have future
permanent allocs use this side.
=================
*/
void idMemorySystemLocal::ClearTempMemory( void )
{
    if ( s_hunk.mem )
    {
        s_hunk.tempTop = 0;
        s_hunk.tempMax = 0;
    }
}

/*
=================
idMemorySystemLocal::FrameInit
=================
*/
void idMemorySystemLocal::FrameInit( void )
{
    S32 megs = cvarSystem->Get( "com_hunkFrameMegs", "1", CVAR_LATCH | CVAR_ARCHIVE, "description" )->integer;
    U64 cb;
    
    if ( megs < 1 )
    {
        megs = 1;
    }
    
    cb = 1024 * 1024 * megs;
    
    s_frameStackBase = ( U8* )memorySystemLocal.Alloc( cb, h_low );
    s_frameStackEnd = s_frameStackBase + cb;
    
    s_frameStackLoc = s_frameStackBase;
}

/*
========================
idMemorySystemLocal::CopyString

never write over the memory CopyString returns because
memory from a memstatic_t might be returned
========================
*/
UTF8* idMemorySystemLocal::CopyString( StringEntry in )
{
    UTF8* out;
    
    if ( !in[0] )
    {
        return ( ( UTF8* )&emptystring ) + sizeof( memblock_t );
    }
    else if ( !in[1] )
    {
        if ( in[0] >= '0' && in[0] <= '9' )
        {
            return ( ( UTF8* )&numberstring[in[0] - '0'] ) + sizeof( memblock_t );
        }
    }
    
    out = ( UTF8* )memorySystem->SMalloc( ::strlen( in ) + 1 );
    ::strcpy( out, in );
    
    return out;
}

/*
========================
idMemorySystemLocal::Shutdown
========================
*/
void idMemorySystemLocal::Shutdown( void )
{
    //threadsSystem->Mutex_Destroy( &zone_mutex );
}

/*
========================
idMemorySystemLocal::Shutdown
========================
*/
void idMemorySystemLocal::GetHunkInfo( S32* hunkused, S32* hunkexpected )
{
    *hunkused = com_hunkusedvalue;
    *hunkexpected = com_expectedhunkusage;
}

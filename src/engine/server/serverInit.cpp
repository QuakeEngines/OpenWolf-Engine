////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverInit.cpp
// Version:     v1.01
// Created:     12/27/2018
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

idServerInitSystemLocal serverInitSystemLocal;
idServerInitSystem* serverInitSystem = &serverInitSystemLocal;

/*
===============
idServerInitSystemLocal::idServerInitSystemLocal
===============
*/
idServerInitSystemLocal::idServerInitSystemLocal( void )
{
}

/*
===============
idServerBotSystemLocal::~idServerBotSystemLocal
===============
*/
idServerInitSystemLocal::~idServerInitSystemLocal( void )
{
}

/*
===============
idServerInitSystemLocal::SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
void idServerInitSystemLocal::SendConfigstring( client_t* client, S32 index )
{
    S32 maxChunkSize = MAX_STRING_CHARS - 24, len;
    
    if ( sv.configstrings[index].restricted && Com_ClientListContains( &sv.configstrings[index].clientList, ( S32 )( client - svs.clients ) ) )
    {
        // Send a blank config string for this client if it's listed
        serverMainSystem->SendServerCommand( client, "cs %i \"\"\n", index );
        return;
    }
    
    len = ( S32 )::strlen( sv.configstrings[index].s );
    
    if ( len >= maxChunkSize )
    {
        S32	sent = 0, remaining = len;
        UTF8* cmd, buf[MAX_STRING_CHARS];
        
        while ( remaining > 0 )
        {
            if ( sent == 0 )
            {
                cmd = "bcs0";
            }
            else if ( remaining < maxChunkSize )
            {
                cmd = "bcs2";
            }
            else
            {
                cmd = "bcs1";
            }
            
            Q_strncpyz( buf, &sv.configstrings[index].s[sent], maxChunkSize );
            
            serverMainSystem->SendServerCommand( client, "%s %i \"%s\"\n", cmd, index, buf );
            
            sent += ( maxChunkSize - 1 );
            remaining -= ( maxChunkSize - 1 );
        }
    }
    else
    {
        // standard cs, just send it
        serverMainSystem->SendServerCommand( client, "cs %i \"%s\"\n", index, sv.configstrings[index].s );
    }
}

/*
===============
idServerInitSystemLocal::UpdateConfigStrings
===============
*/
void idServerInitSystemLocal::UpdateConfigStrings( void )
{
    S32 len, i, index, maxChunkSize = MAX_STRING_CHARS - 24;
    client_t* client;
    
    for ( index = 0; index < MAX_CONFIGSTRINGS; index++ )
    {
        if ( !sv.configstringsmodified[index] )
        {
            continue;
        }
        
        sv.configstringsmodified[index] = false;
        
        // send it to all the clients if we aren't
        // spawning a new server
        if ( sv.state == SS_GAME || sv.restarting )
        {
            // send the data to all relevent clients
            for ( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
            {
                if ( client->state < CS_ACTIVE )
                {
                    if ( client->state == CS_PRIMED )
                    {
                        client->csUpdated[index] = true;
                    }
                    
                    continue;
                }
                
                // do not always send server info to all clients
                if ( index == CS_SERVERINFO && client->gentity && ( client->gentity->r.svFlags & SVF_NOSERVERINFO ) )
                {
                    continue;
                }
                
                // RF, don't send to bot/AI
                // Gordon: Note: might want to re-enable later for bot support
                // RF, re-enabled
                // Arnout: removed hardcoded gametype
                // Arnout: added coop
                if ( ( serverGameSystem->GameIsSinglePlayer() || serverGameSystem->GameIsCoop() ) && client->gentity && ( client->gentity->r.svFlags & SVF_BOT ) )
                {
                    continue;
                }
                
                len = ( S32 )::strlen( sv.configstrings[index].s );
                
                SendConfigstring( client, index );
            }
        }
    }
}

/*
===============
idServerInitSystemLocal::SetConfigstringNoUpdate
===============
*/
void idServerInitSystemLocal::SetConfigstringNoUpdate( S32 index, StringEntry val )
{
    if ( index < 0 || index >= MAX_CONFIGSTRINGS )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::SetConfigstring: bad index %i\n", index );
    }
    
    if ( !val )
    {
        val = "";
    }
    
    if ( ::strlen( val ) >= BIG_INFO_STRING )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::SetConfigstring: CS %d is too long\n", index );
    }
    
    // don't bother broadcasting an update if no change
    if ( !strcmp( val, sv.configstrings[index].s ) )
    {
        return;
    }
    
    // change the string in sv
    memorySystem->Free( sv.configstrings[index].s );
    sv.configstrings[index].s = memorySystem->CopyString( val );
}

/*
===============
idServerInitSystemLocal::SetConfigstring
===============
*/
void idServerInitSystemLocal::SetConfigstring( S32 index, StringEntry val )
{
    S32 i;
    client_t* client;
    
    if ( index < 0 || index >= MAX_CONFIGSTRINGS )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::SetConfigstring: bad index %i\n", index );
    }
    
    if ( !val )
    {
        val = "";
    }
    
    // don't bother broadcasting an update if no change
    if ( !strcmp( val, sv.configstrings[ index ].s ) )
    {
        return;
    }
    
    // change the string in sv
    memorySystem->Free( sv.configstrings[index].s );
    sv.configstrings[index].s = memorySystem->CopyString( val );
    
    // send it to all the clients if we aren't
    // spawning a new server
    if ( sv.state == SS_GAME || sv.restarting )
    {
        // send the data to all relevent clients
        for ( i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++ )
        {
            if ( client->state < CS_ACTIVE )
            {
                if ( client->state == CS_PRIMED )
                {
                    client->csUpdated[index] = true;
                }
                continue;
            }
            
            // do not always send server info to all clients
            if ( index == CS_SERVERINFO && client->gentity && ( client->gentity->r.svFlags & SVF_NOSERVERINFO ) )
            {
                continue;
            }
            
            SendConfigstring( client, index );
        }
    }
}

/*
===============
idServerInitSystemLocal::GetConfigstring
===============
*/
void idServerInitSystemLocal::GetConfigstring( S32 index, UTF8* buffer, S32 bufferSize )
{
    if ( bufferSize < 1 )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::GetConfigstring: bufferSize == %i", bufferSize );
    }
    
    if ( index < 0 || index >= MAX_CONFIGSTRINGS )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::GetConfigstring: bad index %i\n", index );
    }
    
    if ( !sv.configstrings[index].s )
    {
        buffer[0] = 0;
        return;
    }
    
    Q_strncpyz( buffer, sv.configstrings[index].s, bufferSize );
}

/*
===============
idServerInitSystemLocal::SetConfigstringRestrictions
===============
*/
void idServerInitSystemLocal::SetConfigstringRestrictions( S32 index, const clientList_t* clientList )
{
    S32	i;
    clientList_t oldClientList = sv.configstrings[index].clientList;
    
    sv.configstrings[index].clientList = *clientList;
    sv.configstrings[index].restricted = true;
    
    for ( i = 0 ; i < sv_maxclients->integer ; i++ )
    {
        if ( svs.clients[i].state >= CS_CONNECTED )
        {
            if ( Com_ClientListContains( &oldClientList, i ) != Com_ClientListContains( clientList, i ) )
            {
                // A client has left or joined the restricted list, so update
                SendConfigstring( &svs.clients[i], index );
            }
        }
    }
}

/*
===============
idServerInitSystemLocal::SetUserinfo
===============
*/
void idServerInitSystemLocal::SetUserinfo( S32 index, StringEntry val )
{
    if ( index < 0 || index >= sv_maxclients->integer )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::SetUserinfo: bad index %i\n", index );
    }
    
    if ( !val )
    {
        val = "";
    }
    
    Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[index].userinfo ) );
    Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof( svs.clients[index].name ) );
}

/*
===============
idServerInitSystemLocal::GetUserinfo
===============
*/
void idServerInitSystemLocal::GetUserinfo( S32 index, UTF8* buffer, S32 bufferSize )
{
    if ( bufferSize < 1 )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::GetUserinfo: bufferSize == %i", bufferSize );
    }
    
    if ( index < 0 || index >= sv_maxclients->integer )
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::GetUserinfo: bad index %i\n", index );
    }
    
    Q_strncpyz( buffer, svs.clients[index].userinfo, bufferSize );
}

/*
================
idServerInitSystemLocal::CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void idServerInitSystemLocal::CreateBaseline( void )
{
    S32 entnum;
    sharedEntity_t* svent;
    
    for ( entnum = 0; entnum < sv.num_entities; entnum++ )
    {
        svent = serverGameSystem->GentityNum( entnum );
        
        if ( !svent->r.linked )
        {
            continue;
        }
        
        svent->s.number = entnum;
        
        // take current state as baseline
        sv.svEntities[entnum].baseline = svent->s;
    }
}

/*
===============
idServerInitSystemLocal::BoundMaxClients
===============
*/
void idServerInitSystemLocal::BoundMaxClients( S32 minimum )
{
    // get the current maxclients value
    cvarSystem->Get( "sv_maxclients", "20", 0, "description" );
    
    // START    xkan, 10/03/2002
    // allow many bots in single player. note that this pretty much means all previous
    // settings will be ignored (including the one set through "seta sv_maxclients <num>"
    // in user profile's wolfconfig_mp.cfg). also that if the user subsequently start
    // the server in multiplayer mode, the number of clients will still be the number
    // set here, which may be wrong - we can certainly just set it to a sensible number
    // when it is not in single player mode in the else part of the if statement when
    // necessary
    if ( serverGameSystem->GameIsSinglePlayer() || serverGameSystem->GameIsCoop() )
    {
        cvarSystem->Set( "sv_maxclients", "64" );
    }
    // END xkan, 10/03/2002
    
    sv_maxclients->modified = false;
    
    if ( sv_maxclients->integer < minimum )
    {
        cvarSystem->Set( "sv_maxclients", va( "%i", minimum ) );
    }
    else if ( sv_maxclients->integer > MAX_CLIENTS )
    {
        cvarSystem->Set( "sv_maxclients", va( "%i", MAX_CLIENTS ) );
    }
}

/*
===============
idServerInitSystemLocal::Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
void idServerInitSystemLocal::Startup( void )
{
    if ( svs.initialized )
    {
        Com_Error( ERR_FATAL, "idServerInitSystemLocal::Startup: svs.initialized" );
    }
    
    BoundMaxClients( 1 );
    
    // RF, avoid trying to allocate large chunk on a fragmented zone
    svs.clients = static_cast<client_t*>( calloc( sizeof( client_t ) * sv_maxclients->integer, 1 ) );
    if ( !svs.clients )
    {
        Com_Error( ERR_FATAL, "idServerInitSystemLocal::Startup: unable to allocate svs.clients" );
    }
    
    if ( com_dedicated->integer )
    {
        svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
    }
    else
    {
        // we don't need nearly as many when playing locally
        svs.numSnapshotEntities = sv_maxclients->integer * 8 * MAX_SNAPSHOT_ENTITIES;
    }
    
    svs.initialized = true;
    
    // Don't respect sv_killserver unless a server is actually running
    if ( sv_killserver->integer )
    {
        cvarSystem->Set( "sv_killserver", "0" );
    }
    
    cvarSystem->Set( "sv_running", "1" );
    
    // Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network.
    networkSystem->JoinMulticast6();
}

/*
==================
idServerInitSystemLocal::ChangeMaxClients
==================
*/
void idServerInitSystemLocal::ChangeMaxClients( void )
{
    S32 oldMaxClients, i, count;
    client_t* oldClients;
    
    // get the highest client number in use
    count = 0;
    
    for ( i = 0; i < sv_maxclients->integer; i++ )
    {
        if ( svs.clients[i].state >= CS_CONNECTED )
        {
            if ( i > count )
            {
                count = i;
            }
        }
    }
    count++;
    
    oldMaxClients = sv_maxclients->integer;
    
    // never go below the highest client number in use
    BoundMaxClients( count );
    
    // if still the same
    if ( sv_maxclients->integer == oldMaxClients )
    {
        return;
    }
    
    oldClients = static_cast<client_t*>( memorySystem->AllocateTempMemory( count * sizeof( client_t ) ) );
    
    // copy the clients to hunk memory
    for ( i = 0; i < count; i++ )
    {
        if ( svs.clients[i].state >= CS_CONNECTED )
        {
            oldClients[i] = svs.clients[i];
        }
        else
        {
            ::memset( &oldClients[i], 0, sizeof( client_t ) );
        }
    }
    
    // free old clients arrays
    //memorySystem->Free( svs.clients );
    free( svs.clients ); // RF, avoid trying to allocate large chunk on a fragmented zone
    
    // allocate new clients
    // RF, avoid trying to allocate large chunk on a fragmented zone
    svs.clients = static_cast<client_t*>( calloc( sizeof( client_t ) * sv_maxclients->integer, 1 ) );
    if ( !svs.clients )
    {
        Com_Error( ERR_FATAL, "idServerInitSystemLocal::Startup: unable to allocate svs.clients" );
    }
    
    ::memset( svs.clients, 0, sv_maxclients->integer * sizeof( client_t ) );
    
    // copy the clients over
    for ( i = 0; i < count; i++ )
    {
        if ( oldClients[i].state >= CS_CONNECTED )
        {
            svs.clients[i] = oldClients[i];
        }
    }
    
    // free the old clients on the hunk
    memorySystem->FreeTempMemory( oldClients );
    
    // allocate new snapshot entities
    if ( com_dedicated->integer )
    {
        svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
    }
    else
    {
        // we don't need nearly as many when playing locally
        svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
    }
}

/*
====================
SV_SetExpectedHunkUsage

Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void idServerInitSystemLocal::SetExpectedHunkUsage( UTF8* mapname )
{
    S32 handle, len;
    UTF8* memlistfile = "hunkusage.dat", *buf, *buftrav, *token;
    
    len = fileSystem->FOpenFileByMode( memlistfile, &handle, FS_READ );
    if ( len >= 0 ) // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
    {
        buf = ( UTF8* )memorySystem->Malloc( len + 1 );
        ::memset( buf, 0, len + 1 );
        
        fileSystem->Read( static_cast<void*>( buf ), len, handle );
        fileSystem->FCloseFile( handle );
        
        // now parse the file, filtering out the current map
        buftrav = buf;
        while ( ( token = COM_Parse( &buftrav ) ) != NULL && token[0] )
        {
            if ( !Q_stricmp( token, mapname ) )
            {
                // found a match
                token = COM_Parse( &buftrav );	// read the size
                
                if ( token && token[0] )
                {
                    // this is the usage
                    com_expectedhunkusage = atoi( token );
                    memorySystem->Free( buf );
                    return;
                }
            }
        }
        
        memorySystem->Free( buf );
    }
    
    // just set it to a negative number,so the cgame knows not to draw the percent bar
    com_expectedhunkusage = -1;
}

/*
================
idServerInitSystemLocal::ClearServer
================
*/
void idServerInitSystemLocal::ClearServer( void )
{
    S32 i;
    
    for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
    {
        if ( sv.configstrings[i].s )
        {
            memorySystem->Free( sv.configstrings[i].s );
        }
    }
    
    ::memset( &sv, 0, sizeof( sv ) );
}

/*
================
idServerInitSystemLocal::TouchCGameDLL

touch the cgame DLL so that a pure client (with DLL sv_pure support) can load do the correct checks
================
*/
void idServerInitSystemLocal::TouchCGameDLL( void )
{
    fileHandle_t f;
    UTF8* filename;
    
    filename = idsystem->GetDLLName( "cgame" );
    fileSystem->FOpenFileRead_Filtered( filename, &f, false, FS_EXCLUDE_DIR );
    
    if ( f )
    {
        fileSystem->FCloseFile( f );
    }
    else if ( sv_pure->integer ) // ydnar: so we can work the damn game
    {
        Com_Error( ERR_DROP, "idServerInitSystemLocal::TouchCGameDLL - Failed to locate cgame DLL for pure server mode" );
    }
}

/*
================
idServerInitSystemLocal::SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void idServerInitSystemLocal::SpawnServer( UTF8* server, bool killBots )
{
    S32 i, checksum;
    StringEntry p;
    bool isBot;
    
    svs.queryDone = 0;
    
    // ydnar: broadcast a level change to all connected clients
    if ( svs.clients && !com_errorEntered )
    {
        FinalCommand( "spawnserver", false );
    }
    
    // shut down the existing game if it is running
    serverGameSystem->ShutdownGameProgs();
    svs.gameStarted = false;
    
    Com_Printf( "------ Server Initialization ------\n" );
    Com_Printf( "Server: %s\n", server );
    
    if ( svs.snapshotEntities )
    {
        delete[] svs.snapshotEntities;
        svs.snapshotEntities = nullptr;
    }
    
    // if not running a dedicated server CL_MapLoading will connect the client to the server
    // also print some status stuff
#ifndef DEDICATED
    clientMainSystem->MapLoading();
    
    // make sure all the client stuff is unloaded
    clientMainSystem->ShutdownAll( false );
#endif
    // clear the whole hunk because we're (re)loading the server
    memorySystem->Clear();
    
    // (SA) NOTE: TODO: used in missionpack
    // clear collision map data
    collisionModelManager->ClearMap();
    
    // wipe the entire per-level structure
    ClearServer();
    
    // allocate empty config strings
    for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
    {
        sv.configstrings[i].s = memorySystem->CopyString( "" );
        sv.configstringsmodified[i] = false;
    }
    
    // init client structures and svs.numSnapshotEntities
    if ( !cvarSystem->VariableValue( "sv_running" ) )
    {
        Startup();
    }
    else
    {
        // check for maxclients change
        if ( sv_maxclients->modified )
        {
            ChangeMaxClients();
        }
    }
    
    // clear pak references
    fileSystem->ClearPakReferences( 0 );
    
    // allocate the snapshot entities on the hunk
    svs.nextSnapshotEntities = 0;
    
    svs.snapshotEntities = new entityState_s[svs.numSnapshotEntities];
    ::memset( svs.snapshotEntities, 0, sizeof( entityState_t ) * svs.numSnapshotEntities );
    
    // toggle the server bit so clients can detect that a
    // server has changed
    svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;
    
    // set nextmap to the same map, but it may be overriden
    // by the game startup or another console command
    cvarSystem->Set( "nextmap", "map_restart 0" );
    
    for ( i = 0; i < sv_maxclients->integer; i++ )
    {
        // save when the server started for each client already connected
        if ( svs.clients[i].state >= CS_CONNECTED )
        {
            svs.clients[i].oldServerTime = sv.time;
        }
    }
    
    // Ridah
    // DHM - Nerve :: We want to use the completion bar in multiplayer as well
    // Arnout: just always use it
    if ( !serverGameSystem->GameIsSinglePlayer() )
    {
        SetExpectedHunkUsage( va( "maps/%s.world", server ) );
    }
    else
    {
        // just set it to a negative number,so the cgame knows not to draw the percent bar
        cvarSystem->Set( "com_expectedhunkusage", "-1" );
    }
    
    // make sure we are not paused
    cvarSystem->Set( "cl_paused", "0" );
    
    // get a new checksum feed and restart the file system
    srand( idsystem->Milliseconds() );
    sv.checksumFeed = ( ( ( S32 )rand() << 16 ) ^ rand() ) ^ idsystem->Milliseconds();
    
    // DO_LIGHT_DEDICATED
    // only comment out when you need a new pure checksum string and it's associated random feed
    //Com_DPrintf("SV_SpawnServer checksum feed: %p\n", sv.checksumFeed);
    
    fileSystem->Restart( sv.checksumFeed );
    
    collisionModelManager->LoadMap( va( "maps/%s.world", server ), false, &checksum );
    
    // set serverinfo visible name
    cvarSystem->Set( "mapname", server );
    
    cvarSystem->Set( "sv_mapChecksum", va( "%i", checksum ) );
    
    sv_newGameShlib = cvarSystem->Get( "sv_newGameShlib", "", CVAR_TEMP, "description" );
    
    // serverid should be different each time
    sv.serverId = com_frameTime;
    sv.restartedServerId = sv.serverId;
    sv.checksumFeedServerId = sv.serverId;
    cvarSystem->Set( "sv_serverid", va( "%i", sv.serverId ) );
    
    // media configstring setting should be done during
    // the loading stage, so connected clients don't have
    // to load during actual gameplay
    sv.state = SS_LOADING;
    
    cvarSystem->Set( "sv_serverRestarting", "1" );
    
    // load and spawn all other entities
    serverGameSystem->InitGameProgs();
    
    // don't allow a map_restart if game is modified
    // Arnout: there isn't any check done against this, obsolete
    //  sv_gametype->modified = false;
    
    // run a few frames to allow everything to settle
    for ( i = 0; i < GAME_INIT_FRAMES; i++ )
    {
        sgame->RunFrame( sv.time );
        sv.time += 100;
        svs.time += FRAMETIME;
    }
    
    // create a baseline for more efficient communications
    CreateBaseline();
    
    for ( i = 0; i < sv_maxclients->integer; i++ )
    {
        // send the new gamestate to all connected clients
        if ( svs.clients[i].state >= CS_CONNECTED )
        {
            UTF8* denied;
            
            if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT )
            {
                if ( killBots || serverGameSystem->GameIsSinglePlayer() || serverGameSystem->GameIsCoop() )
                {
                    serverClientSystem->DropClient( &svs.clients[i], "" );
                    continue;
                }
                isBot = true;
            }
            else
            {
                isBot = false;
            }
            
            // connect the client again
            denied = static_cast<UTF8*>( sgame->ClientConnect( i, false ) ); // firstTime = false
            if ( denied )
            {
                // this generally shouldn't happen, because the client
                // was connected before the level change
                serverClientSystem->DropClient( &svs.clients[i], denied );
            }
            else
            {
                if ( !isBot )
                {
                    // when we get the next packet from a connected client,
                    // the new gamestate will be sent
                    svs.clients[i].state = CS_CONNECTED;
                }
                else
                {
                    client_t* client;
                    sharedEntity_t* ent;
                    
                    client = &svs.clients[i];
                    client->state = CS_ACTIVE;
                    ent = serverGameSystem->GentityNum( i );
                    ent->s.number = i;
                    client->gentity = ent;
                    
                    client->deltaMessage = -1;
                    client->nextSnapshotTime = svs.time;	// generate a snapshot immediately
                    
                    sgame->ClientBegin( i );
                }
            }
        }
    }
    
    // run another frame to allow things to look at all the players
    sgame->RunFrame( sv.time );
    sv.time += 100;
    svs.time += FRAMETIME;
    
    if ( sv_pure->integer )
    {
        // the server sends these to the clients so they will only
        // load pk3s also loaded at the server
        p = fileSystem->LoadedPakChecksums();
        cvarSystem->Set( "sv_paks", p );
        if ( strlen( p ) == 0 )
        {
            Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
        }
        p = fileSystem->LoadedPakNames();
        cvarSystem->Set( "sv_pakNames", p );
    }
    else
    {
        cvarSystem->Set( "sv_paks", "" );
        cvarSystem->Set( "sv_pakNames", "" );
    }
    // the server sends these to the clients so they can figure
    // out which pk3s should be auto-downloaded
    // NOTE: we consider the referencedPaks as 'required for operation'
    
    // we want the server to reference the bin pk3 that the client is expected to load from
    TouchCGameDLL();
    
    p = fileSystem->ReferencedPakChecksums();
    cvarSystem->Set( "sv_referencedPaks", p );
    p = fileSystem->ReferencedPakNames();
    cvarSystem->Set( "sv_referencedPakNames", p );
    
    // save systeminfo and serverinfo strings
    cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
    SetConfigstring( CS_SYSTEMINFO, cvarSystem->InfoString_Big( CVAR_SYSTEMINFO ) );
    
    SetConfigstring( CS_SERVERINFO, cvarSystem->InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ) );
    cvar_modifiedFlags &= ~CVAR_SERVERINFO;
    
    // NERVE - SMF
    SetConfigstring( CS_WOLFINFO, cvarSystem->InfoString( CVAR_WOLFINFO ) );
    cvar_modifiedFlags &= ~CVAR_WOLFINFO;
    
    // any media configstring setting now should issue a warning
    // and any configstring changes should be reliably transmitted
    // to all clients
    sv.state = SS_GAME;
    
    // send a heartbeat now so the master will get up to date info
    serverCcmdsLocal.Heartbeat_f();
    
    memorySystem->SetMark();
    
    UpdateConfigStrings();
    
#ifndef DEDICATED
    if ( com_dedicated->integer )
    {
        // restart renderer in order to show console for dedicated servers launched through the regular binary
        clientMainSystem->StartHunkUsers( true );
    }
#endif
    
    cvarSystem->Set( "sv_serverRestarting", "0" );
    
    Com_Printf( "-----------------------------------\n" );
}

// Update Server
/*
====================
idServerInitSystemLocal::ParseVersionMapping

Reads versionmap.cfg which sets up a mapping of client version to installer to download
====================
*/
void idServerInitSystemLocal::ParseVersionMapping( void )
{
#if defined (UPDATE_SERVER)
    S32 handle, len;
    UTF8* filename = "versionmap.cfg", *buf, *buftrav, *token;
    
    len = fileSystem->SV_FOpenFileRead( filename, &handle );
    
    // the file exists
    if ( len >= 0 )
    {
        buf = ( UTF8* )memorySystem->Malloc( len + 1 );
        memset( buf, 0, len + 1 );
        
        fileSystem->Read( ( void* )buf, len, handle );
        fileSystem->FCloseFile( handle );
        
        // now parse the file, setting the version table info
        buftrav = buf;
        
        token = COM_Parse( &buftrav );
        if ( strcmp( token, "OpenWolf-VersionMap" ) )
        {
            memorySystem->Free( buf );
            Com_Error( ERR_FATAL, "invalid versionmap.cfg" );
            return;
        }
        
        Com_Printf( "\n------------Update Server-------------\n\nParsing version map...\n" );
        
        while ( ( token = COM_Parse( &buftrav ) ) && token[0] )
        {
            // read the version number
            ::strcpy( versionMap[ numVersions ].version, token );
            
            // read the platform
            token = COM_Parse( &buftrav );
            if ( token && token[0] )
            {
                ::strcpy( versionMap[ numVersions ].platform, token );
            }
            else
            {
                memorySystem->Free( buf );
                Com_Error( ERR_FATAL, "error parsing versionmap.cfg, after %s", versionMap[numVersions].platform );
                return;
            }
            
            // read the installer name
            token = COM_Parse( &buftrav );
            if ( token && token[0] )
            {
                ::strcpy( versionMap[ numVersions ].installer, token );
            }
            else
            {
                memorySystem->Free( buf );
                Com_Error( ERR_FATAL, "error parsing versionmap.cfg, after %s", versionMap[numVersions].installer );
                return;
            }
            
            numVersions++;
            if ( numVersions >= MAX_UPDATE_VERSIONS )
            {
                memorySystem->Free( buf );
                Com_Error( ERR_FATAL, "Exceeded maximum number of mappings(%d)", MAX_UPDATE_VERSIONS );
                return;
            }
            
        }
        
        Com_Printf( " found %d mapping%c\n--------------------------------------\n\n", numVersions, numVersions > 1 ? 's' : ' ' );
        
        memorySystem->Free( buf );
    }
    else
    {
        Com_Error( ERR_FATAL, "Couldn't open versionmap.cfg" );
    }
#endif
}

/*
===============
idServerInitSystemLocal::Init

Only called at main exe startup, not for each game
===============
*/
void idServerInitSystemLocal::Init( void )
{
    S32 index;
    
    serverCcmdsLocal.AddOperatorCommands();
    
    // serverinfo vars
    cvarSystem->Get( "dmflags", "0", /*CVAR_SERVERINFO */ 0, "description" );
    cvarSystem->Get( "fraglimit", "0", /*CVAR_SERVERINFO */ 0, "description" );
    cvarSystem->Get( "timelimit", "0", CVAR_SERVERINFO, "description" );
    
    // Rafael gameskill
    //  sv_gameskill = cvarSystem->Get ("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH, "description" );
    // done
    
    cvarSystem->Get( "sv_keywords", "", CVAR_SERVERINFO, "description" );
    cvarSystem->Get( "protocol", va( "%i", PROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_ARCHIVE, "description" );
    sv_mapname = cvarSystem->Get( "mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM, "description" );
    sv_privateClients = cvarSystem->Get( "sv_privateClients", "0", CVAR_SERVERINFO, "description" );
    sv_hostname = cvarSystem->Get( "sv_hostname", "OpenWolf Host", CVAR_SERVERINFO | CVAR_ARCHIVE, "description" );
    //
    sv_maxclients = cvarSystem->Get( "sv_maxclients", "20", CVAR_SERVERINFO | CVAR_LATCH, "description" );	// NERVE - SMF - changed to 20 from 8
    sv_democlients = cvarSystem->Get( "sv_democlients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, "description" );
    
    sv_maxRate = cvarSystem->Get( "sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, "description" );
    sv_minPing = cvarSystem->Get( "sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, "description" );
    sv_maxPing = cvarSystem->Get( "sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, "description" );
    sv_floodProtect = cvarSystem->Get( "sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, "description" );
    sv_allowAnonymous = cvarSystem->Get( "sv_allowAnonymous", "0", CVAR_SERVERINFO, "description" );
    sv_friendlyFire = cvarSystem->Get( "g_friendlyFire", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, "description" );	// NERVE - SMF
    sv_maxlives = cvarSystem->Get( "g_maxlives", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO, "description" );	// NERVE - SMF
    sv_needpass = cvarSystem->Get( "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, "description" );
    
    // systeminfo
    //bani - added convar_t for sv_cheats so server engine can reference it
    sv_cheats = cvarSystem->Get( "sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM, "description" );
    sv_serverid = cvarSystem->Get( "sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM, "description" );
    sv_pure = cvarSystem->Get( "sv_pure", "0", CVAR_SYSTEMINFO, "description" );
    
    cvarSystem->Get( "sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM, "description" );
    cvarSystem->Get( "sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM, "description" );
    cvarSystem->Get( "sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM, "description" );
    cvarSystem->Get( "sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM, "description" );
    
    // server vars
    sv_rconPassword = cvarSystem->Get( "rconPassword", "", CVAR_TEMP, "description" );
    sv_privatePassword = cvarSystem->Get( "sv_privatePassword", "", CVAR_TEMP, "description" );
    sv_fps = cvarSystem->Get( "sv_fps", "60", CVAR_TEMP, "description" );
    sv_timeout = cvarSystem->Get( "sv_timeout", "240", CVAR_TEMP, "description" );
    sv_zombietime = cvarSystem->Get( "sv_zombietime", "2", CVAR_TEMP, "description" );
    cvarSystem->Get( "nextmap", "", CVAR_TEMP, "description" );
    
    sv_allowDownload = cvarSystem->Get( "sv_allowDownload", "1", CVAR_ARCHIVE, "description" );
    
    sv_master[0] = cvarSystem->Get( "sv_master1", MASTER_SERVER_NAME, 0, "description" );
    for ( index = 1; index < MAX_MASTER_SERVERS; index++ )
    {
        sv_master[index] = cvarSystem->Get( va( "sv_master%d", index + 1 ), "", CVAR_ARCHIVE, "description" );
    }
    
    sv_reconnectlimit = cvarSystem->Get( "sv_reconnectlimit", "3", 0, "description" );
    sv_tempbanmessage = cvarSystem->Get( "sv_tempbanmessage", "You have been kicked and are temporarily banned from joining this server.", 0, "description" );
    sv_showloss = cvarSystem->Get( "sv_showloss", "0", 0, "description" );
    sv_padPackets = cvarSystem->Get( "sv_padPackets", "0", 0, "description" );
    sv_killserver = cvarSystem->Get( "sv_killserver", "0", 0, "description" );
    sv_mapChecksum = cvarSystem->Get( "sv_mapChecksum", "", CVAR_ROM, "description" );
    
    sv_reloading = cvarSystem->Get( "g_reloading", "0", CVAR_ROM, "description" );
    
    sv_lanForceRate = cvarSystem->Get( "sv_lanForceRate", "1", CVAR_ARCHIVE, "description" );
    
    sv_onlyVisibleClients = cvarSystem->Get( "sv_onlyVisibleClients", "0", 0, "description" );	// DHM - Nerve
    
    sv_showAverageBPS = cvarSystem->Get( "sv_showAverageBPS", "0", 0, "description" );	// NERVE - SMF - net debugging
    
    // NERVE - SMF - create user set cvars
    cvarSystem->Get( "g_userTimeLimit", "0", 0, "description" );
    cvarSystem->Get( "g_userAlliedRespawnTime", "0", 0, "description" );
    cvarSystem->Get( "g_userAxisRespawnTime", "0", 0, "description" );
    cvarSystem->Get( "g_maxlives", "0", 0, "description" );
    cvarSystem->Get( "g_altStopwatchMode", "0", CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "g_minGameClients", "8", CVAR_SERVERINFO, "description" );
    cvarSystem->Get( "g_complaintlimit", "6", CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM, "description" );
    cvarSystem->Get( "g_currentRound", "0", CVAR_WOLFINFO, "description" );
    cvarSystem->Get( "g_nextTimeLimit", "0", CVAR_WOLFINFO, "description" );
    // -NERVE - SMF
    
    // TTimo - some UI additions
    // NOTE: sucks to have this hardcoded really, I suppose this should be in UI
    cvarSystem->Get( "g_axismaxlives", "0", 0, "description" );
    cvarSystem->Get( "g_alliedmaxlives", "0", 0, "description" );
    cvarSystem->Get( "g_fastres", "0", CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "g_fastResMsec", "1000", CVAR_ARCHIVE, "description" );
    
    // ATVI Tracker Wolfenstein Misc #273
    cvarSystem->Get( "g_voteFlags", "0", CVAR_ROM | CVAR_SERVERINFO, "description" );
    
    // ATVI Tracker Wolfenstein Misc #263
    cvarSystem->Get( "g_antilag", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, "description" );
    
    cvarSystem->Get( "g_needpass", "0", CVAR_SERVERINFO, "description" );
    
    g_gameType = cvarSystem->Get( "g_gametype", va( "%i", com_gameInfo.defaultGameType ), CVAR_SERVERINFO | CVAR_LATCH, "description" );
    
#if !defined (UPDATE_SERVER)
    // the download netcode tops at 18/20 kb/s, no need to make you think you can go above
    sv_dl_maxRate = cvarSystem->Get( "sv_dl_maxRate", "42000", CVAR_ARCHIVE, "description" );
#else
    // the update server is on steroids, sv_fps 60 and no snapshotMsec limitation, it can go up to 30 kb/s
    sv_dl_maxRate = cvarSystem->Get( "sv_dl_maxRate", "60000", CVAR_ARCHIVE, "description" );
#endif
    
    sv_wwwDownload = cvarSystem->Get( "sv_wwwDownload", "0", CVAR_ARCHIVE, "description" );
    sv_wwwBaseURL = cvarSystem->Get( "sv_wwwBaseURL", "", CVAR_ARCHIVE, "description" );
    sv_wwwDlDisconnected = cvarSystem->Get( "sv_wwwDlDisconnected", "0", CVAR_ARCHIVE, "description" );
    sv_wwwFallbackURL = cvarSystem->Get( "sv_wwwFallbackURL", "", CVAR_ARCHIVE, "description" );
    
    //bani
    sv_packetloss = cvarSystem->Get( "sv_packetloss", "0", CVAR_CHEAT, "description" );
    sv_packetdelay = cvarSystem->Get( "sv_packetdelay", "0", CVAR_CHEAT, "description" );
    
    // fretn - note: redirecting of clients to other servers relies on this,
    // ET://someserver.com
    sv_fullmsg = cvarSystem->Get( "sv_fullmsg", "Server is full.", CVAR_ARCHIVE, "description" );
    
    sv_hibernateTime = cvarSystem->Get( "sv_hibernateTime", "0", CVAR_ARCHIVE, "description" );
    svs.hibernation.sv_fps = sv_fps->value;
    
    // oacs extended recording variables
    sv_oacsEnable = cvarSystem->Get( "sv_oacsEnable", "0", CVAR_ARCHIVE, "description" );
    sv_oacsPlayersTableEnable = cvarSystem->Get( "sv_oacsPlayersTableEnable", "1", CVAR_ARCHIVE, "description" );
    sv_oacsTypesFile = cvarSystem->Get( "sv_oacsTypesFile", "oacs/types.txt", CVAR_ARCHIVE, "description" );
    sv_oacsDataFile = cvarSystem->Get( "sv_oacsDataFile", "oacs/data.txt", CVAR_ARCHIVE, "description" );
    sv_oacsPlayersTable = cvarSystem->Get( "sv_oacsPlayersTable", "oacs/playerstable.txt", CVAR_ARCHIVE, "description" );
    sv_oacsMinPlayers = cvarSystem->Get( "sv_oacsMinPlayers", "1", CVAR_ARCHIVE, "description" );
    sv_oacsLabelPassword = cvarSystem->Get( "sv_oacsLabelPassword", "", CVAR_TEMP, "description" );
    sv_oacsMaxPing = cvarSystem->Get( "sv_oacsMaxPing", "700", CVAR_ARCHIVE, "description" );
    sv_oacsMaxLastPacketTime = cvarSystem->Get( "sv_oacsMaxLastPacketTime", "10000", CVAR_ARCHIVE, "description" );
    
    sv_wh_active = cvarSystem->Get( "sv_wh_active", "0", CVAR_ARCHIVE, "description" );
    sv_wh_bbox_horz = cvarSystem->Get( "sv_wh_bbox_horz", "60", CVAR_ARCHIVE, "description" );
    
    if ( sv_wh_bbox_horz->integer < 20 )
    {
        cvarSystem->Set( "sv_wh_bbox_horz", "10" );
    }
    if ( sv_wh_bbox_horz->integer > 50 )
    {
        cvarSystem->Set( "sv_wh_bbox_horz", "50" );
    }
    
    sv_wh_bbox_vert = cvarSystem->Get( "sv_wh_bbox_vert", "60", CVAR_ARCHIVE, "description" );
    
    if ( sv_wh_bbox_vert->integer < 10 )
    {
        cvarSystem->Set( "sv_wh_bbox_vert", "30" );
    }
    if ( sv_wh_bbox_vert->integer > 50 )
    {
        cvarSystem->Set( "sv_wh_bbox_vert", "80" );
    }
    
    sv_wh_check_fov = cvarSystem->Get( "wh_check_fov", "0", CVAR_ARCHIVE, "description" );
    
    idServerWallhackSystemLocal::InitWallhack();
    
    // can't ResolveAuthHost() here since NET_Init() hasn't been
    // called yet; works if we move ResolveAuthHost() to platform/systemMain.cpp
    // but that's introducing another #ifdef DEDICATED there, kinda sad; seems
    // that Rambetter added his stuff in Frame() but that incurs (at least
    // some) overhead on each frame; the overhead may not be so bad since the
    // HEARTBEAT stuff is also handled in Frame(), who knows; but we found
    // a better way: Startup() and SpawnServer(), check the code there!
    
    svs.serverLoad = -1;
    
#if defined(UPDATE_SERVER)
    Startup();
    ParseVersionMapping();
    
    // serverid should be different each time
    sv.serverId = com_frameTime + 100;
    sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
    sv.checksumFeedServerId = sv.serverId;
    cvarSystem->Set( "sv_serverid", va( "%i", sv.serverId ) );
    cvarSystem->Set( "mapname", "Update" );
    
    // allocate empty config strings
    {
        S32 i;
        
        for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ )
        {
            sv.configstrings[i].s = memorySystem->CopyString( "" );
        }
    }
#endif
    
    serverCryptoSystem->InitCrypto();
    
    // OACS: Initialize the interframe/features variable and write down the extended records structures (types)
    if ( sv_oacsEnable->integer == 1 )
    {
        idServerOACSSystemLocal::ExtendedRecordInit();
    }
}


/*
==================
idServerInitSystemLocal::FinalCommand

Used by idServerInitSystemLocal::Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void idServerInitSystemLocal::FinalCommand( UTF8* cmd, bool disconnect )
{
    S32 i, j;
    client_t* cl;
    
    // send it twice, ignoring rate
    for ( j = 0; j < 2; j++ )
    {
        for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
        {
            if ( cl->state >= CS_CONNECTED )
            {
                // don't send a disconnect to a local client
                if ( cl->netchan.remoteAddress.type != NA_LOOPBACK )
                {
                    //serverMainSystem->SendServerCommand( cl, "print \"%s\"", message );
                    serverMainSystem->SendServerCommand( cl, "%s", cmd );
                    
                    // ydnar: added this so map changes can use this functionality
                    if ( disconnect )
                    {
                        serverMainSystem->SendServerCommand( cl, "disconnect" );
                    }
                }
                
                // force a snapshot to be sent
                cl->nextSnapshotTime = -1;
                serverSnapshotSystemLocal.SendClientSnapshot( cl );
            }
        }
    }
}

/*
================
idServerInitSystemLocal::Shutdown

Called when each game quits,
before idSystemLocal::Quit or idSystemLocal::Error
================
*/
void idServerInitSystemLocal::Shutdown( UTF8* finalmsg )
{
    if ( !com_sv_running || !com_sv_running->integer )
    {
        return;
    }
    
    Com_Printf( "----- Server Shutdown -----\n" );
    
    networkSystem->LeaveMulticast6();
    
    if ( svs.clients && !com_errorEntered )
    {
        FinalCommand( va( "print \"%s\"", finalmsg ), true );
    }
    
    serverMainSystem->MasterShutdown();
    serverGameSystem->ShutdownGameProgs();
    svs.gameStarted = false;
    
    // stop any demos
    if ( sv.demoState == DS_RECORDING )
    {
        serverDemoSystem->DemoStopRecord();
    }
    
    if ( sv.demoState == DS_PLAYBACK )
    {
        serverDemoSystem->DemoStopPlayback();
    }
    
    // OACS: commit any remaining interframe
    idServerOACSSystemLocal::ExtendedRecordShutdown();
    
    // de allocate the snapshot entities
    if ( svs.snapshotEntities )
    {
        delete[] svs.snapshotEntities;
        svs.snapshotEntities = nullptr;
    }
    
    // free current level
    ClearServer();
    collisionModelManager->ClearMap();
    
    // free server static data
    if ( svs.clients )
    {
        S32 index;
        
        for ( index = 0; index < sv_maxclients->integer; index++ )
        {
            serverClientSystem->FreeClient( &svs.clients[index] );
        }
        
        //memorySystem->Free( svs.clients );
        free( svs.clients ); // RF, avoid trying to allocate large chunk on a fragmented zone
    }
    
    ::memset( &svs, 0, sizeof( svs ) );
    svs.serverLoad = -1;
    
    cvarSystem->Set( "sv_running", "0" );
    
    Com_Printf( "---------------------------\n" );
    
    // disconnect any local clients
    clientMainSystem->Disconnect( false );
}

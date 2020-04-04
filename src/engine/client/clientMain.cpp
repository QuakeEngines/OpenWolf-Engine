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
// File name:   clientMain.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: client main loop
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

convar_t* cl_wavefilerecord;
convar_t* cl_nodelta;
convar_t* cl_debugMove;
convar_t* cl_noprint;
convar_t* cl_motd;
convar_t* cl_autoupdate;
convar_t* rcon_client_password;
convar_t* rconAddress;
convar_t* cl_timeout;
convar_t* cl_maxpackets;
convar_t* cl_packetdup;
convar_t* cl_timeNudge;
convar_t* cl_showTimeDelta;
convar_t* cl_freezeDemo;
//This is referenced in msg.c and we need to make sure it is nullptr
convar_t* cl_shownet = nullptr;
convar_t* cl_shownuments;
convar_t* cl_visibleClients;
convar_t* cl_showSend;
convar_t* cl_showServerCommands;
convar_t* cl_timedemo;
convar_t* cl_aviFrameRate;
convar_t* cl_forceavidemo;
convar_t* cl_freelook;
convar_t* cl_sensitivity;
convar_t* cl_xbox360ControllerAvailable;
convar_t* cl_mouseAccelOffset;
convar_t* cl_mouseAccel;
convar_t* cl_showMouseRate;
convar_t* m_pitch;
convar_t* m_yaw;
convar_t* m_forward;
convar_t* m_side;
convar_t* m_filter;
convar_t* j_pitch;
convar_t* j_yaw;
convar_t* j_forward;
convar_t* j_side;
convar_t* j_up;
convar_t* j_pitch_axis;
convar_t* j_yaw_axis;
convar_t* j_forward_axis;
convar_t* j_side_axis;
convar_t* j_up_axis;
convar_t* cl_activeAction;
convar_t* cl_autorecord;
convar_t* cl_motdString;
convar_t* cl_allowDownload;
convar_t* cl_wwwDownload;
convar_t* cl_conXOffset;
convar_t* cl_inGameVideo;
convar_t* cl_serverStatusResendTime;
convar_t* cl_trn;
convar_t* cl_missionStats;
convar_t* cl_waitForFire;
convar_t* cl_language;
convar_t* cl_debugTranslation;
convar_t* cl_updateavailable;
convar_t* cl_updatefiles;
convar_t* cl_authserver;
convar_t* cl_profile;
convar_t* cl_defaultProfile;
convar_t* cl_demorecording;
convar_t* cl_demofilename;
convar_t* cl_demooffset;
convar_t* cl_waverecording;
convar_t* cl_wavefilename;
convar_t* cl_waveoffset;
convar_t* cl_packetloss;
convar_t* cl_packetdelay;
convar_t* cl_consoleKeys;
convar_t* cl_consoleFont;
convar_t* cl_consoleFontSize;
convar_t* cl_consoleFontKerning;
convar_t* cl_consolePrompt;
convar_t* cl_gamename;
convar_t* cl_altTab;
convar_t* cl_aviMotionJpeg;
convar_t* cl_guidServerUniq;

clientActive_t cl;
clientConnection_t clc;
clientStatic_t cls;
void* cgvm;
netadr_t rcon_address;

idRenderSystem* renderSystem;
idRenderSystem* ( *rendererEntry )( rendererImports_t* imports );

void* rendererName;
static rendererImports_t exports;

// NERVE - SMF - Localization code
#define FILE_HASH_SIZE 1024
#define MAX_VA_STRING 32000

// "updates" shifted from -7
#define AUTOUPDATE_DIR "ni]Zm^l"
#define AUTOUPDATE_DIR_SHIFT 7
#define MAX_AVI_BUFFER 2048

static trans_t* transTable[FILE_HASH_SIZE];

typedef struct wav_hdr_s
{
    U32 ChunkID;	    // big endian
    U32 ChunkSize;	    // little endian
    U32 Format;		    // big endian
    U32 Subchunk1ID;	// big endian
    U32 Subchunk1Size;	// little endian
    U16 AudioFormat;	// little endian
    U16 NumChannels;	// little endian
    U32 SampleRate;	    // little endian
    U32 ByteRate;	    // little endian
    U16 BlockAlign;	    // little endian
    U16 BitsPerSample;	// little endian
    U32 Subchunk2ID;	// big endian
    U32 Subchunk2Size;	// little indian ;)
    U32  NumSamples;
} wav_hdr_t;

wav_hdr_t hdr;

typedef struct
{
    UTF8            name[MAX_QPATH];
    S32             hits;
    S32             lastSetIndex;
} cacheItem_t;
typedef enum
{
    CACHE_SOUNDS,
    CACHE_MODELS,
    CACHE_IMAGES,
    
    CACHE_NUMGROUPS
} cacheGroup_t;
static cacheItem_t cacheGroups[CACHE_NUMGROUPS] =
{
    {{'s', 'o', 'u', 'n', 'd', 0}, CACHE_SOUNDS},
    {{'m', 'o', 'd', 'e', 'l', 0}, CACHE_MODELS},
    {{'i', 'm', 'a', 'g', 'e', 0}, CACHE_IMAGES},
};

#define MAX_CACHE_ITEMS 4096
// if hit on this percentage of maps, it'll get cached
#define CACHE_HIT_RATIO 0.75f

static S32 cacheIndex;
static cacheItem_t cacheItems[CACHE_NUMGROUPS][MAX_CACHE_ITEMS];

// Have we heard from the auto-update server this session?
bool autoupdateChecked;
bool autoupdateStarted;
// compiler bug workaround
static UTF8 demoName[MAX_QPATH];

// moved from UTF8* to array (was getting the UTF8* from va(), broke on big downloads)
UTF8 autoupdateFilename[MAX_QPATH];

static U8 buffer[MAX_AVI_BUFFER];
static S32 bufIndex;
#define MAX_RCON_MESSAGE 1024
// compiler bug workaround
static UTF8 wavName[MAX_QPATH];

idClientMainSystemLocal clientMainLocal;
idClientMainSystem* clientMainSystem = &clientMainLocal;

/*
===============
idClientMainSystemLocal::idClientMainSystemLocal
===============
*/
idClientMainSystemLocal::idClientMainSystemLocal( void )
{
}

/*
===============
idClientMainSystemLocal::~idClientMainSystemLocal
===============
*/
idClientMainSystemLocal::~idClientMainSystemLocal( void )
{
}

/*
===============
idClientMainSystemLocal::PurgeCache
===============
*/
void idClientMainSystemLocal::PurgeCache( void )
{
    cls.doCachePurge = true;
}

/*
===============
idClientMainSystemLocal::DoPurgeCache
===============
*/
void idClientMainSystemLocal::DoPurgeCache( void )
{
    if ( !cls.doCachePurge )
    {
        return;
    }
    
    cls.doCachePurge = false;
    
    if ( !com_cl_running )
    {
        return;
    }
    
    if ( !com_cl_running->integer )
    {
        return;
    }
    
    if ( !cls.rendererStarted )
    {
        return;
    }
}

/*
=======================================================================
CLIENT RELIABLE COMMAND COMMUNICATION
=======================================================================
*/

/*
======================
idClientMainSystemLocal::AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void idClientMainSystemLocal::AddReliableCommand( StringEntry cmd )
{
    S32 index;
    
    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    if ( clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS )
    {
        Com_Error( ERR_DROP, "Client command overflow" );
    }
    
    clc.reliableSequence++;
    
    index = clc.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
    
    Q_strncpyz( clc.reliableCommands[index], cmd, sizeof( clc.reliableCommands[index] ) );
}

/*
=======================================================================
CLIENT SIDE DEMO RECORDING
=======================================================================
*/

/*
====================
idClientMainSystemLocal::WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void idClientMainSystemLocal::WriteDemoMessage( msg_t* msg, S32 headerBytes )
{
    S32 len, swlen;
    
    // write the packet sequence
    len = clc.serverMessageSequence;
    swlen = LittleLong( len );
    fileSystem->Write( &swlen, 4, clc.demofile );
    
    // skip the packet sequencing information
    len = msg->cursize - headerBytes;
    swlen = LittleLong( len );
    fileSystem->Write( &swlen, 4, clc.demofile );
    fileSystem->Write( msg->data + headerBytes, len, clc.demofile );
}

/*
====================
idClientMainSystemLocal::StopRecording_f

stop recording a demo
====================
*/
void idClientMainSystemLocal::StopRecord_f( void )
{
    S32 len;
    
    if ( !clc.demorecording )
    {
        Com_Printf( "Not recording a demo.\n" );
        return;
    }
    
    // finish up
    len = -1;
    fileSystem->Write( &len, 4, clc.demofile );
    fileSystem->Write( &len, 4, clc.demofile );
    fileSystem->FCloseFile( clc.demofile );
    clc.demofile = 0;
    
    clc.demorecording = false;
    
    cvarSystem->Set( "cl_demorecording", "0" );
    cvarSystem->Set( "cl_demofilename", "" );
    cvarSystem->Set( "cl_demooffset", "0" );
    
    Com_Printf( "Stopped demo.\n" );
}

/*
==================
idClientMainSystemLocal::DemoFilename
==================
*/
void idClientMainSystemLocal::DemoFilename( S32 number, UTF8* fileName )
{
    if ( number < 0 || number > 9999 )
    {
        Q_snprintf( fileName, MAX_OSPATH, "demo9999" );
        return;
    }
    
    Q_snprintf( fileName, MAX_OSPATH, "demo%04i", number );
}

/*
====================
idClientMainSystemLocal::Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/

void idClientMainSystemLocal::Record_f( void )
{
    S32 len;
    UTF8 name[MAX_OSPATH], *s;
    
    if ( cmdSystem->Argc() > 2 )
    {
        Com_Printf( "record <demoname>\n" );
        return;
    }
    
    if ( clc.demorecording )
    {
        Com_Printf( "Already recording.\n" );
        return;
    }
    
    if ( cls.state != CA_ACTIVE )
    {
        Com_Printf( "You must be in a level to record.\n" );
        return;
    }
    
    // ATVI Wolfenstein Misc #479 - changing this to a warning
    // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
    if ( networkSystem->IsLocalAddress( clc.serverAddress ) && !cvarSystem->VariableValue( "g_synchronousClients" ) )
    {
        Com_Printf( S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n" );
    }
    
    if ( cmdSystem->Argc() == 2 )
    {
        s = cmdSystem->Argv( 1 );
        Q_strncpyz( demoName, s, sizeof( demoName ) );
        Q_snprintf( name, sizeof( name ), "demos/%s.dm_%d", demoName, com_protocol->integer );
    }
    else
    {
        S32 number;
        
        // scan for a free demo name
        for ( number = 0; number <= 9999; number++ )
        {
            DemoFilename( number, demoName );
            Q_snprintf( name, sizeof( name ), "demos/%s.dm_%d", demoName, com_protocol->integer );
            
            len = fileSystem->ReadFile( name, nullptr );
            if ( len <= 0 )
            {
                // file doesn't exist
                break;
            }
        }
    }
    
    Record( name );
}

/*
===============
idClientMainSystemLocal::Record
===============
*/
void idClientMainSystemLocal::Record( StringEntry name )
{
    S32 i, len;
    U8 bufData[MAX_MSGLEN];
    UTF8* s;
    msg_t buf;
    entityState_t* ent, nullstate;
    
    // open the demo file
    Com_Printf( "recording to %s.\n", name );
    clc.demofile = fileSystem->FOpenFileWrite( name );
    
    if ( !clc.demofile )
    {
        Com_Printf( "ERROR: couldn't open.\n" );
        return;
    }
    
    clc.demorecording = true;
    cvarSystem->Set( "cl_demorecording", "1" );
    Q_strncpyz( clc.demoName, demoName, sizeof( clc.demoName ) );
    cvarSystem->Set( "cl_demofilename", clc.demoName );
    cvarSystem->Set( "cl_demooffset", "0" );
    
    // don't start saving messages until a non-delta compressed message is received
    clc.demowaiting = true;
    
    // write out the gamestate message
    MSG_Init( &buf, bufData, sizeof( bufData ) );
    MSG_Bitstream( &buf );
    
    // NOTE, MRE: all server->client messages now acknowledge
    MSG_WriteLong( &buf, clc.reliableSequence );
    
    MSG_WriteByte( &buf, svc_gamestate );
    MSG_WriteLong( &buf, clc.serverCommandSequence );
    
    // configstrings
    for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
    {
        if ( !cl.gameState.stringOffsets[i] )
        {
            continue;
        }
        
        s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
        
        MSG_WriteByte( &buf, svc_configstring );
        MSG_WriteShort( &buf, i );
        MSG_WriteBigString( &buf, s );
    }
    
    // baselines
    ::memset( &nullstate, 0, sizeof( nullstate ) );
    
    for ( i = 0; i < MAX_GENTITIES; i++ )
    {
        ent = &cl.entityBaselines[i];
        if ( !ent->number )
        {
            continue;
        }
        
        MSG_WriteByte( &buf, svc_baseline );
        MSG_WriteDeltaEntity( &buf, &nullstate, ent, true );
    }
    
    MSG_WriteByte( &buf, svc_EOF );
    
    // finished writing the gamestate stuff
    // write the client num
    MSG_WriteLong( &buf, clc.clientNum );
    // write the checksum feed
    MSG_WriteLong( &buf, clc.checksumFeed );
    
    // finished writing the client packet
    MSG_WriteByte( &buf, svc_EOF );
    
    // write it to the demo file
    len = LittleLong( clc.serverMessageSequence - 1 );
    fileSystem->Write( &len, 4, clc.demofile );
    
    len = LittleLong( buf.cursize );
    fileSystem->Write( &len, 4, clc.demofile );
    fileSystem->Write( buf.data, buf.cursize, clc.demofile );
    
    // the rest of the demo file will be copied from net messages
}

/*
=======================================================================
CLIENT SIDE DEMO PLAYBACK
=======================================================================
*/

/*
=================
idClientMainSystemLocal::DemoCompleted
=================
*/

void idClientMainSystemLocal::DemoCompleted( void )
{
    if ( cl_timedemo && cl_timedemo->integer )
    {
        S32 time;
        
        time = idsystem->Milliseconds() - clc.timeDemoStart;
        if ( time > 0 )
        {
            Com_Printf( "%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames, time / 1000.0, clc.timeDemoFrames * 1000.0 / time );
        }
    }
    
    if ( clc.waverecording )
    {
        WriteWaveClose();
        clc.waverecording = false;
    }
    
    clientMainLocal.Disconnect( true );
    NextDemo();
}

/*
=================
idClientMainSystemLocal::ReadDemoMessage
=================
*/

void idClientMainSystemLocal::ReadDemoMessage( void )
{
    S32 r, s;
    U8 bufData[MAX_MSGLEN];
    msg_t buf;
    
    if ( !clc.demofile )
    {
        DemoCompleted();
        return;
    }
    
    // get the sequence number
    r = fileSystem->Read( &s, 4, clc.demofile );
    if ( r != 4 )
    {
        DemoCompleted();
        return;
    }
    
    clc.serverMessageSequence = LittleLong( s );
    
    // init the message
    MSG_Init( &buf, bufData, sizeof( bufData ) );
    
    // get the length
    r = fileSystem->Read( &buf.cursize, 4, clc.demofile );
    
    if ( r != 4 )
    {
        DemoCompleted();
        return;
    }
    
    buf.cursize = LittleLong( buf.cursize );
    
    if ( buf.cursize == -1 )
    {
        DemoCompleted();
        return;
    }
    
    if ( buf.cursize > buf.maxsize )
    {
        Com_Error( ERR_DROP, "idClientMainSystemLocal::ReadDemoMessage: demoMsglen > MAX_MSGLEN" );
    }
    
    r = fileSystem->Read( buf.data, buf.cursize, clc.demofile );
    
    if ( r != buf.cursize )
    {
        Com_Printf( "Demo file was truncated.\n" );
        DemoCompleted();
        return;
    }
    
    clc.lastPacketTime = cls.realtime;
    buf.readcount = 0;
    
    idClientParseSystemLocal::ParseServerMessage( &buf );
}

/*
====================
Wave file saving functions

FIXME: make this actually work
====================
*/

/*
==================
idClientMainSystemLocal::DemoFilename
==================
*/
void idClientMainSystemLocal::WavFilename( S32 number, UTF8* fileName )
{
    if ( number < 0 || number > 9999 )
    {
        Q_snprintf( fileName, MAX_OSPATH, "wav9999" );
        return;
    }
    
    Q_snprintf( fileName, MAX_OSPATH, "wav%04i", number );
}

/*
===============
idClientMainSystemLocal::WriteWaveHeader
===============
*/
void idClientMainSystemLocal::WriteWaveHeader( void )
{
    memset( &hdr, 0, sizeof( hdr ) );
    
    // "RIFF"
    hdr.ChunkID = 0x46464952;
    // total filesize - 8 bytes
    hdr.ChunkSize = 0;
    // "WAVE"
    hdr.Format = 0x45564157;
    
    // "fmt "
    hdr.Subchunk1ID = 0x20746d66;
    // 16 = pcm
    hdr.Subchunk1Size = 16;
    // 1 = linear quantization
    hdr.AudioFormat = 1;
    // 2 = stereo
    hdr.NumChannels = 2;
    
    hdr.SampleRate = dma.speed;
    
    // 16bits
    hdr.BitsPerSample = 16;
    
    // SampleRate * NumChannels * BitsPerSample/8
    hdr.ByteRate = hdr.SampleRate * hdr.NumChannels * ( hdr.BitsPerSample / 8 );
    
    // NumChannels * BitsPerSample/8
    hdr.BlockAlign = hdr.NumChannels * ( hdr.BitsPerSample / 8 );
    
    // "data"
    hdr.Subchunk2ID = 0x61746164;
    
    // NumSamples * NumChannels * BitsPerSample/8
    hdr.Subchunk2Size = 0;
    
    // ...
    fileSystem->Write( &hdr.ChunkID, 44, clc.wavefile );
}

/*
===============
idClientMainSystemLocal::WriteWaveHeader
===============
*/
void idClientMainSystemLocal::WriteWaveOpen( void )
{
    // we will just save it as a 16bit stereo 22050kz pcm file
    S32 len;
    UTF8 name[MAX_OSPATH], *s;
    
    if ( cmdSystem->Argc() > 2 )
    {
        Com_Printf( "wav_record <wavname>\n" );
        return;
    }
    
    if ( clc.waverecording )
    {
        Com_Printf( "Already recording a wav file\n" );
        return;
    }
    
    // yes ... no ? leave it up to them imo
    //if (cl_avidemo.integer)
    //{
    //    return;
    //}
    
    if ( cmdSystem->Argc() == 2 )
    {
        s = cmdSystem->Argv( 1 );
        
        Q_strncpyz( wavName, s, sizeof( wavName ) );
        Q_snprintf( name, sizeof( name ), "wav/%s.wav", wavName );
    }
    else
    {
        S32  number;
        
        // I STOLE THIS
        for ( number = 0; number <= 9999; number++ )
        {
            WavFilename( number, wavName );
            Q_snprintf( name, sizeof( name ), "wav/%s.wav", wavName );
            
            len = fileSystem->FileExists( name );
            if ( len <= 0 )
            {
                // file doesn't exist
                break;
            }
        }
    }
    
    Com_Printf( "recording to %s.\n", name );
    clc.wavefile = fileSystem->FOpenFileWrite( name );
    
    if ( !clc.wavefile )
    {
        Com_Printf( "ERROR: couldn't open %s for writing.\n", name );
        return;
    }
    
    WriteWaveHeader();
    clc.wavetime = -1;
    
    clc.waverecording = true;
    
    cvarSystem->Set( "cl_waverecording", "1" );
    cvarSystem->Set( "cl_wavefilename", wavName );
    cvarSystem->Set( "cl_waveoffset", "0" );
}

/*
===============
idClientMainSystemLocal::WriteWaveHeader
===============
*/
void idClientMainSystemLocal::WriteWaveClose( void )
{
    Com_Printf( "Stopped recording\n" );
    
    hdr.Subchunk2Size = hdr.NumSamples * hdr.NumChannels * ( hdr.BitsPerSample / 8 );
    hdr.ChunkSize = 36 + hdr.Subchunk2Size;
    
    fileSystem->Seek( clc.wavefile, 4, FS_SEEK_SET );
    fileSystem->Write( &hdr.ChunkSize, 4, clc.wavefile );
    fileSystem->Seek( clc.wavefile, 40, FS_SEEK_SET );
    fileSystem->Write( &hdr.Subchunk2Size, 4, clc.wavefile );
    
    // and we're outta here
    fileSystem->FCloseFile( clc.wavefile );
    clc.wavefile = 0;
}

/*
====================
idClientMainSystemLocal::CompleteDemoName
====================
*/
void idClientMainSystemLocal::CompleteDemoName( UTF8* args, S32 argNum )
{
    if ( argNum == 2 )
    {
        UTF8 demoExt[ 16 ];
        
        Q_snprintf( demoExt, sizeof( demoExt ), ".dm_%d", com_protocol->integer );
        Field_CompleteFilename( "demos", demoExt, true );
    }
}

/*
====================
idClientMainSystemLocal::PlayDemo_f

demo <demoname>
====================
*/
void idClientMainSystemLocal::PlayDemo_f( void )
{
    S32 prot_ver;
    UTF8 name[MAX_OSPATH], extension[32], * arg;
    
    if ( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "playdemo <demoname>\n" );
        return;
    }
    
    // make sure a local server is killed
    cvarSystem->Set( "sv_killserver", "1" );
    
    clientMainLocal.Disconnect( true );
    
    // open the demo file
    arg = cmdSystem->Argv( 1 );
    prot_ver = com_protocol->integer - 1;
    
    while ( prot_ver <= com_protocol->integer && !clc.demofile )
    {
        Q_snprintf( extension, sizeof( extension ), ".dm_%d", prot_ver );
        
        if ( !Q_stricmp( arg + strlen( arg ) - strlen( extension ), extension ) )
        {
            Q_snprintf( name, sizeof( name ), "demos/%s", arg );
        }
        else
        {
            Q_snprintf( name, sizeof( name ), "demos/%s.dm_%d", arg, prot_ver );
        }
        
        fileSystem->FOpenFileRead( name, &clc.demofile, true );
        
        prot_ver++;
    }
    
    if ( !clc.demofile )
    {
        Com_Error( ERR_DROP, "couldn't open %s", name );
        return;
    }
    
    Q_strncpyz( clc.demoName, cmdSystem->Argv( 1 ), sizeof( clc.demoName ) );
    
    Con_Close();
    
    cls.state = CA_CONNECTED;
    clc.demoplaying = true;
    
    if ( cvarSystem->VariableValue( "cl_wavefilerecord" ) )
    {
        WriteWaveOpen();
    }
    
    Q_strncpyz( cls.servername, cmdSystem->Argv( 1 ), sizeof( cls.servername ) );
    
    // read demo messages until connected
    while ( cls.state >= CA_CONNECTED && cls.state < CA_PRIMED )
    {
        ReadDemoMessage();
    }
    
    // don't get the first snapshot this frame, to prevent the long
    // time from the gamestate load from messing causing a time skip
    clc.firstDemoFrameSkipped = false;
    
}

/*
====================
idClientMainSystemLocal::StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void idClientMainSystemLocal::StartDemoLoop( void )
{
    // start the demo loop again
    cmdBufferSystem->AddText( "d1\n" );
    clientGUISystem->SetCatcher( 0 );
}

/*
==================
idClientMainSystemLocal::DemoState

Returns the current state of the demo system
==================
*/
demoState_t idClientMainSystemLocal::DemoState( void )
{
    if ( clc.demoplaying )
    {
        return DS_PLAYBACK;
    }
    else if ( clc.demorecording )
    {
        return DS_RECORDING;
    }
    else
    {
        return DS_NONE;
    }
}

/*
==================
idClientMainSystemLocal::DemoPos

Returns the current position of the demo
==================
*/
S32 idClientMainSystemLocal::DemoPos( void )
{
    if ( clc.demoplaying || clc.demorecording )
    {
        return fileSystem->FTell( clc.demofile );
    }
    else
    {
        return 0;
    }
}

/*
==================
idClientMainSystemLocal::NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void idClientMainSystemLocal::NextDemo( void )
{
    UTF8 v[MAX_STRING_CHARS];
    
    Q_strncpyz( v, cvarSystem->VariableString( "nextdemo" ), sizeof( v ) );
    v[MAX_STRING_CHARS - 1] = 0;
    
    Com_DPrintf( "idClientMainSystemLocal::NextDemo: %s\n", v );
    
    if ( !v[0] )
    {
        clientMainLocal.FlushMemory();
        return;
    }
    
    cvarSystem->Set( "nextdemo", "" );
    
    cmdBufferSystem->AddText( v );
    cmdBufferSystem->AddText( "\n" );
    cmdBufferSystem->Execute();
}

/*
==================
idClientMainSystemLocal::DemoName

Returns the name of the demo
==================
*/
void idClientMainSystemLocal::DemoName( UTF8* buffer, S32 size )
{
    if ( clc.demoplaying || clc.demorecording )
    {
        Q_strncpyz( buffer, clc.demoName, size );
    }
    else if ( size >= 1 )
    {
        buffer[ 0 ] = '\0';
    }
}

/*
=====================
idClientMainSystemLocal::ShutdownAll
=====================
*/
void idClientMainSystemLocal::ShutdownAll( bool shutdownRef )
{
    // clear sounds
    soundSystem->DisableSounds();
    
    // download subsystem
    downloadSystem->Shutdown();
    
    // shutdown CGame
    clientGameSystem->ShutdownCGame();
    
    // shutdown GUI
    clientGUISystem->ShutdownGUI();
    
    // shutdown the renderer
    if ( shutdownRef )
    {
        ShutdownRef();
    }
    else
    {
        // don't destroy window or context
        renderSystem->Shutdown( false );
    }
    
    DoPurgeCache();
    
    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.rendererStarted = false;
    cls.soundRegistered = false;
    
    // Stop recording on map change etc, demos aren't valid over map changes anyway
    if ( clc.demorecording )
    {
        StopRecord_f();
    }
    
    if ( clc.waverecording )
    {
        WavStopRecord_f();
    }
}

/*
=================
idClientMainSystemLocal::ClearMemory

Called by Com_GameRestart
=================
*/
void idClientMainSystemLocal::ClearMemory( bool shutdownRef )
{
    // shutdown all the client stuff
    clientMainLocal.ShutdownAll( shutdownRef );
    
    
    // if not running a server clear the whole hunk
    if ( !com_sv_running->integer )
    {
        // clear the whole hunk
        memorySystem->Clear();
        
        // clear collision map data
        collisionModelManager->ClearMap();
    }
    else
    {
        // clear all the client data on the hunk
        memorySystem->ClearToMark();
    }
}

/*
=================
idClientMainSystemLocal::FlushMemory

Called by MapLoading, Connect_f, PlayDemo_f, and ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void idClientMainSystemLocal::FlushMemory( void )
{
    ClearMemory( false );
    StartHunkUsers( false );
}

/*
=====================
idClientMainSystemLocal::MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void idClientMainSystemLocal::MapLoading( void )
{
    if ( com_dedicated->integer )
    {
        cls.state = CA_DISCONNECTED;
        clientGUISystem->SetCatcher( KEYCATCH_CONSOLE );
        return;
    }
    
    if ( !com_cl_running->integer )
    {
        return;
    }
    
    Con_Close();
    
    cls.keyCatchers = 0;
    
    // if we are already connected to the local host, stay connected
    if ( cls.state >= CA_CONNECTED && !Q_stricmp( cls.servername, "localhost" ) )
    {
        // so the connect screen is drawn
        cls.state = CA_CONNECTED;
        
        ::memset( cls.updateInfoString, 0, sizeof( cls.updateInfoString ) );
        ::memset( clc.serverMessage, 0, sizeof( clc.serverMessage ) );
        ::memset( &cl.gameState, 0, sizeof( cl.gameState ) );
        
        clc.lastPacketSentTime = -9999;
        clientScreenSystem->UpdateScreen();
    }
    else
    {
        // clear nextmap so the cinematic shutdown doesn't execute it
        cvarSystem->Set( "nextmap", "" );
        Disconnect( true );
        
        Q_strncpyz( cls.servername, "localhost", sizeof( cls.servername ) );
        
        // so the connect screen is drawn
        cls.state = CA_CHALLENGING;
        cls.keyCatchers = 0;
        
        clientScreenSystem->UpdateScreen();
        
        clc.connectTime = -RETRANSMIT_TIMEOUT;
        
        networkChainSystem->StringToAdr( cls.servername, &clc.serverAddress, NA_UNSPEC );
        
        // we don't need a challenge on the localhost
        CheckForResend();
    }
}

/*
=====================
idClientMainSystemLocal::ClearState

Called before parsing a gamestate
=====================
*/
void idClientMainSystemLocal::ClearState( void )
{
    soundSystem->StopAllSounds();
    ::memset( &cl, 0, sizeof( cl ) );
}

/*
====================
idClientMainSystemLocal::UpdateGUID

update cl_guid using ETKEY_FILE and optional prefix
====================
*/
void idClientMainSystemLocal::UpdateGUID( StringEntry prefix, S32 prefix_len )
{
    S32 len;
    fileHandle_t f;
    
    len = fileSystem->SV_FOpenFileRead( GUIDKEY_FILE, &f );
    fileSystem->FCloseFile( f );
    
    if ( len != GUIDKEY_SIZE )
    {
        cvarSystem->Set( "cl_guid", "" );
    }
    else
    {
        cvarSystem->Set( "cl_guid", Com_MD5File( GUIDKEY_FILE, GUIDKEY_SIZE, prefix, prefix_len ) );
    }
}
/*
=====================
idClientMainSystemLocal::ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void idClientMainSystemLocal::ClearStaticDownload( void )
{
    // reset before calling
    assert( !cls.bWWWDlDisconnected );
    
    cls.downloadRestart = false;
    cls.downloadTempName[0] = '\0';
    cls.downloadName[0] = '\0';
    cls.originalDownloadName[0] = '\0';
}

/*
=====================
idClientMainSystemLocal::Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void idClientMainSystemLocal::Disconnect( bool showMainMenu )
{
    if ( !com_cl_running || !com_cl_running->integer )
    {
        return;
    }
    
    if ( clc.demorecording )
    {
        StopRecord_f();
    }
    
    if ( !cls.bWWWDlDisconnected )
    {
        if ( clc.download )
        {
            fileSystem->FCloseFile( clc.download );
            clc.download = 0;
        }
        
        *cls.downloadTempName = *cls.downloadName = 0;
        
        cvarSystem->Set( "cl_downloadName", "" );
        
        autoupdateStarted = false;
        autoupdateFilename[0] = '\0';
    }
    
    if ( clc.demofile )
    {
        fileSystem->FCloseFile( clc.demofile );
        clc.demofile = 0;
    }
    
    if ( uivm && showMainMenu )
    {
        uiManager->SetActiveMenu( UIMENU_NONE );
    }
    
    SCR_StopCinematic();
    soundSystemLocal.ClearSoundBuffer();
    
    // send a disconnect message to the server
    // send it a few times in case one is dropped
    if ( cls.state >= CA_CONNECTED )
    {
        AddReliableCommand( "disconnect" );
        CL_WritePacket();
        CL_WritePacket();
        CL_WritePacket();
    }
    
    ClearState();
    
    // wipe the client connection
    ::memset( &clc, 0, sizeof( clc ) );
    
    if ( !cls.bWWWDlDisconnected )
    {
        ClearStaticDownload();
    }
    
    // not connected to a pure server anymore
    cl_connectedToPureServer = false;
    
    // stop recording any video
    if ( clientAVISystem->VideoRecording() )
    {
        // finish rendering current frame
        //idClientScreenSystemLocal::UpdateScreen();
        clientAVISystem->CloseAVI();
    }
    
    // show_bug.cgi?id=589
    // don't try a restart if uivm is nullptr, as we might be in the middle of a restart already
    if ( uivm && cls.state > CA_DISCONNECTED )
    {
        // restart the GUI
        cls.state = CA_DISCONNECTED;
        
        // shutdown the GUI
        clientGUISystem->ShutdownGUI();
        
        // init the UI
        clientGUISystem->InitGUI();
    }
    else
    {
        cls.state = CA_DISCONNECTED;
    }
    
    UpdateGUID( nullptr, 0 );
}

/*
===================
idClientMainSystemLocal::ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void idClientMainSystemLocal::ForwardCommandToServer( StringEntry string )
{
    UTF8* cmd;
    
    cmd = cmdSystem->Argv( 0 );
    
    // ignore key up commands
    if ( cmd[0] == '-' )
    {
        return;
    }
    
    // no userinfo updates from command line
    if ( !strcmp( cmd, "userinfo" ) )
    {
        return;
    }
    
    if ( clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+' )
    {
        Com_Printf( "Unknown command \"%s\"\n", cmd );
        return;
    }
    
    if ( cmdSystem->Argc() > 1 )
    {
        AddReliableCommand( string );
    }
    else
    {
        AddReliableCommand( cmd );
    }
}

/*
==================
idClientMainSystemLocal::OpenUrl_f
==================
*/
void idClientMainSystemLocal::OpenUrl_f( void )
{
    StringEntry url;
    
    if ( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: openurl <url>\n" );
        return;
    }
    
    url = cmdSystem->Argv( 1 );
    
    {
        //FixMe: URL sanity checks.
        //Random sanity checks. Scott: if you've got some magic URL
        //parsing and validating functions USE THEM HERE, this code
        //is a placeholder!!!
        
        S32 i;
        StringEntry u;
        
        StringEntry allowPrefixes[] = { "http://", "https://", "" };
        StringEntry allowDomains[2] = { "localhost", 0 };
        
        u = url;
        for ( i = 0; i < lengthof( allowPrefixes ); i++ )
        {
            StringEntry p = allowPrefixes[i];
            S32 len = ( S32 )::strlen( p );
            
            if ( Q_strncmp( u, p, len ) == 0 )
            {
                u += len;
                break;
            }
        }
        
        if ( i == lengthof( allowPrefixes ) )
        {
            //This really won't ever hit because of the "" at the end
            //of the allowedPrefixes array. As I said above, placeholder
            //code: fix it later!
            
            Com_Printf( "Invalid URL prefix.\n" );
            return;
        }
        
        for ( i = 0; i < lengthof( allowDomains ); i++ )
        {
            S32 len;
            StringEntry d = allowDomains[i];
            
            if ( !d )
            {
                break;
            }
            
            len = ( S32 )::strlen( d );
            
            if ( Q_strncmp( u, d, len ) == 0 )
            {
                u += len;
                break;
            }
        }
        
        if ( i == lengthof( allowDomains ) )
        {
            Com_Printf( "Invalid domain.\n" );
            return;
        }
        /* my kingdom for a regex */
        for ( i = 0; i < strlen( url ); i++ )
        {
            if ( !(
                        // lower case alpha
                        ( url[i] >= 'a' && url[i] <= 'z' ) ||
                        // upper case alpha
                        ( url[i] >= 'A' && url[i] <= 'Z' ) ||
                        //numeric
                        ( url[i] >= '0' && url[i] <= '9' ) ||
                        // / and : chars
                        ( url[i] == '/' ) || ( url[i] == ':' ) ||
                        // . and & chars
                        ( url[i] == '.' ) || ( url[i] == '&' ) ||
                        // ; char
                        ( url[i] == ';' )
                    ) )
            {
                Com_Printf( "Invalid URL\n" );
                return;
            }
        }
    }
    
    if ( !idsystem->OpenUrl( url ) )
    {
        Com_Printf( "System error opening URL\n" );
    }
}

/*
===================
idClientMainSystemLocal::RequestMotd
===================
*/
void idClientMainSystemLocal::RequestMotd( void )
{
    UTF8 info[MAX_INFO_STRING];
    
    if ( !cl_motd->integer )
    {
        return;
    }
    
    Com_DPrintf( "Resolving %s\n", MASTER_SERVER_NAME );
    
    switch ( networkChainSystem->StringToAdr( MASTER_SERVER_NAME, &cls.updateServer, NA_UNSPEC ) )
    {
        case 0:
            Com_Printf( "Couldn't resolve master address\n" );
            return;
            
        case 2:
            cls.updateServer.port = BigShort( PORT_MASTER );
        default:
            break;
    }
    
    Com_DPrintf( "%s resolved to %s\n", MASTER_SERVER_NAME, networkSystem->AdrToString( cls.updateServer ) );
    
    info[0] = 0;
    
    Q_snprintf( cls.updateChallenge, sizeof( cls.updateChallenge ), "%i", ( ( rand() << 16 ) ^ rand() ) ^ Com_Milliseconds() );
    
    Info_SetValueForKey( info, "challenge", cls.updateChallenge );
    Info_SetValueForKey( info, "renderer", cls.glconfig.renderer_string );
    Info_SetValueForKey( info, "version", com_version->string );
    
    networkChainSystem->OutOfBandPrint( NS_CLIENT, cls.updateServer, "getmotd%s", info );
}

/*
======================================================================
CONSOLE COMMANDS
======================================================================
*/

/*
==================
idClientMainSystemLocal::ForwardToServer_f
==================
*/
void idClientMainSystemLocal::ForwardToServer_f( void )
{
    if ( cls.state != CA_ACTIVE || clc.demoplaying )
    {
        Com_Printf( "Not connected to a server.\n" );
        return;
    }
    
    // don't forward the first argument
    if ( cmdSystem->Argc() > 1 )
    {
        clientMainLocal.AddReliableCommand( cmdSystem->Args() );
    }
}

/*
==================
idClientMainSystemLocal::Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void idClientMainSystemLocal::Setenv_f( void )
{
    S32 argc = cmdSystem->Argc();
    
    if ( argc > 2 )
    {
        UTF8 buffer[1024];
        S32 i;
        
        ::strcpy( buffer, cmdSystem->Argv( 1 ) );
        ::strcat( buffer, "=" );
        
        for ( i = 2; i < argc; i++ )
        {
            ::strcat( buffer, cmdSystem->Argv( i ) );
            ::strcat( buffer, " " );
        }
        
        Q_putenv( buffer );
    }
    else if ( argc == 2 )
    {
        UTF8* env = getenv( cmdSystem->Argv( 1 ) );
        
        if ( env )
        {
            Com_Printf( "%s=%s\n", cmdSystem->Argv( 1 ), env );
        }
        else
        {
            Com_Printf( "%s undefined\n", cmdSystem->Argv( 1 ) );
        }
    }
}

/*
==================
idClientMainSystemLocal::Disconnect_f
==================
*/
void idClientMainSystemLocal::Disconnect_f( void )
{
    SCR_StopCinematic();
    
    cvarSystem->Set( "savegame_loading", "0" );
    cvarSystem->Set( "g_reloading", "0" );
    
    if ( cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC )
    {
        Com_Printf( "Disconnected from server" );
        clientMainLocal.Disconnect( true );
    }
}


/*
================
idClientMainSystemLocal::Reconnect_f
================
*/
void idClientMainSystemLocal::Reconnect_f( void )
{
    if ( !::strlen( cls.servername ) || !::strcmp( cls.servername, "localhost" ) )
    {
        Com_Printf( "Can't reconnect to localhost.\n" );
        return;
    }
    
    cmdBufferSystem->AddText( va( "connect %s\n", cls.servername ) );
}

/*
================
idClientMainSystemLocal::Connect_f
================
*/
void idClientMainSystemLocal::Connect_f( void )
{
    S32 argc = cmdSystem->Argc();
    UTF8* server;
    StringEntry serverString;
    netadrtype_t family = NA_UNSPEC;
    
    if ( argc == 2 )
    {
        server = cmdSystem->Argv( 1 );
    }
    else
    {
        if ( !strcmp( cmdSystem->Argv( 1 ), "-4" ) )
            family = NA_IP;
        else if ( !strcmp( cmdSystem->Argv( 1 ), "-6" ) )
            family = NA_IP6;
        else
            Com_Printf( "warning: only -4 or -6 as address type understood.\n" );
            
        server = cmdSystem->Argv( 2 );
    }
    
    soundSystem->StopAllSounds();
    
    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set( "r_uiFullScreen", "0" );
    cvarSystem->Set( "ui_connecting", "1" );
    
    // fire a message off to the motd server
    RequestMotd();
    
    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;
    
    if ( com_sv_running->integer && !strcmp( server, "localhost" ) )
    {
        // if running a local server, kill it
        serverInitSystem->Shutdown( "Server quit\n" );
    }
    
    // make sure a local server is killed
    cvarSystem->Set( "sv_killserver", "1" );
    serverMainSystem->Frame( 0 );
    
    clientMainLocal.Disconnect( true );
    Con_Close();
    
    Q_strncpyz( cls.servername, server, sizeof( cls.servername ) );
    
    if ( !networkChainSystem->StringToAdr( cls.servername, &clc.serverAddress, family ) )
    {
        Com_Printf( "Bad server address\n" );
        cls.state = CA_DISCONNECTED;
        cvarSystem->Set( "ui_connecting", "0" );
        return;
    }
    
    if ( clc.serverAddress.port == 0 )
    {
        clc.serverAddress.port = BigShort( PORT_SERVER );
    }
    
    serverString = networkSystem->AdrToString( clc.serverAddress );
    Com_Printf( "%s resolved to %s\n", cls.servername, serverString );
    
    if ( cl_guidServerUniq->integer )
    {
        UpdateGUID( serverString, ( S32 )::strlen( serverString ) );
    }
    else
    {
        UpdateGUID( nullptr, 0 );
    }
    
    // if we aren't playing on a lan, we needto authenticate
    // with the cd key
    if ( networkSystem->IsLocalAddress( clc.serverAddress ) )
    {
        cls.state = CA_CHALLENGING;
    }
    else
    {
        cls.state = CA_CONNECTING;
    }
    
    cvarSystem->Set( "cl_avidemo", "0" );
    
    // show_bug.cgi?id=507
    // prepare to catch a connection process that would turn bad
    cvarSystem->Set( "com_errorDiagnoseIP", networkSystem->AdrToString( clc.serverAddress ) );
    
    // ATVI Wolfenstein Misc #439
    // we need to setup a correct default for this, otherwise the first val we set might reappear
    cvarSystem->Set( "com_errorMessage", "" );
    
    cls.keyCatchers = 0;
    clc.connectTime = -99999;   // CheckForResend() will fire immediately
    clc.connectPacketCount = 0;
    
    // server connection string
    cvarSystem->Set( "cl_currentServerAddress", server );
    cvarSystem->Set( "cl_currentServerIP", serverString );
    
    // Gordon: um, couldnt this be handled
    // NERVE - SMF - reset some cvars
    cvarSystem->Set( "mp_playerType", "0" );
    cvarSystem->Set( "mp_currentPlayerType", "0" );
    cvarSystem->Set( "mp_weapon", "0" );
    cvarSystem->Set( "mp_team", "0" );
    cvarSystem->Set( "mp_currentTeam", "0" );
    
    cvarSystem->Set( "ui_limboOptions", "0" );
    cvarSystem->Set( "ui_limboPrevOptions", "0" );
    cvarSystem->Set( "ui_limboObjective", "0" );
    // -NERVE - SMF
    
}

/*
=====================
idClientMainSystemLocal::Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void idClientMainSystemLocal::Rcon_f( void )
{
    UTF8 message[MAX_RCON_MESSAGE];
    
    if ( !::strlen( rcon_client_password->string ) )
    {
        Com_Printf( "You must set 'rcon_password' before\n"
                    "issuing an rcon command.\n" );
        return;
    }
    
    message[0] = -1;
    message[1] = -1;
    message[2] = -1;
    message[3] = -1;
    message[4] = 0;
    
    Q_strcat( message, MAX_RCON_MESSAGE, "rcon " );
    
    Q_strcat( message, MAX_RCON_MESSAGE, rcon_client_password->string );
    Q_strcat( message, MAX_RCON_MESSAGE, " " );
    
    // ATVI Wolfenstein Misc #284
    Q_strcat( message, MAX_RCON_MESSAGE, cmdSystem->Cmd() + 5 );
    
    if ( cls.state >= CA_CONNECTED )
    {
        rcon_address = clc.netchan.remoteAddress;
    }
    else
    {
        if ( !::strlen( rconAddress->string ) )
        {
            Com_Printf( "You must either be connected,\n"
                        "or set the 'rconAddress' cvar\n"
                        "to issue rcon commands\n" );
                        
            return;
        }
        
        networkChainSystem->StringToAdr( rconAddress->string, &rcon_address, NA_UNSPEC );
        
        if ( rcon_address.port == 0 )
        {
            rcon_address.port = BigShort( PORT_SERVER );
        }
    }
    
    networkChainSystem->SendPacket( NS_CLIENT, ( S32 )::strlen( message ) + 1, message, rcon_address );
}

/*
=================
idClientMainSystemLocal::SendPureChecksums
=================
*/
void idClientMainSystemLocal::SendPureChecksums( void )
{
    S32 i;
    UTF8 cMsg[MAX_INFO_VALUE];
    StringEntry pChecksums;
    
    // if we are pure we need to send back a command with our referenced pk3 checksums
    pChecksums = fileSystem->ReferencedPakPureChecksums();
    
    // "cp"
    Q_snprintf( cMsg, sizeof( cMsg ), "Va " );
    Q_strcat( cMsg, sizeof( cMsg ), "%d ", cl.serverId );
    Q_strcat( cMsg, sizeof( cMsg ), pChecksums );
    
    for ( i = 0; i < 2; i++ )
    {
        cMsg[i] += 13 + ( i * 2 );
    }
    
    clientMainLocal.AddReliableCommand( cMsg );
}

/*
=================
idClientMainSystemLocal::ResetPureClientAtServer
=================
*/
void idClientMainSystemLocal::ResetPureClientAtServer( void )
{
    clientMainLocal.AddReliableCommand( va( "vdr" ) );
}

/*
=================
idClientMainSystemLocal::Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void idClientMainSystemLocal::Vid_Restart_f( void )
{
    if ( cls.lastVidRestart )
    {
        if ( abs( cls.lastVidRestart - idsystem->Milliseconds() ) < 500 )
        {
            // do not allow vid restart righ after cgame init
            return;
        }
    }
    
    // settings may have changed so stop recording now
    if ( clientAVISystem->VideoRecording() )
    {
        cvarSystem->Set( "cl_avidemo", "0" );
        clientAVISystem->CloseAVI();
    }
    
    // RF, don't show percent bar, since the memory usage will just sit at the same level anyway
    com_expectedhunkusage = -1;
    
    // don't let them loop during the restart
    soundSystem->StopAllSounds();
    
    // if not running a server clear the whole hunk
    if ( com_sv_running->integer )
    {
        // clear all the client data on the hunk
        memorySystem->ClearToMark();
    }
    else
    {
        // clear the whole hunk
        memorySystem->Clear();
    }
    
    // shutdown the UI
    clientGUISystem->ShutdownGUI();
    
    // shutdown the CGame
    clientGameSystem->ShutdownCGame();
    
    // shutdown the renderer and clear the renderer interface
    ShutdownRef();
    
    // client is no longer pure untill new checksums are sent
    ResetPureClientAtServer();
    
    // clear pak references
    fileSystem->ClearPakReferences( FS_UI_REF | FS_CGAME_REF );
    
    // reinitialize the filesystem if the game directory or checksum has changed
    fileSystem->ConditionalRestart( clc.checksumFeed );
    
    // all sound handles are now invalid
    soundSystem->BeginRegistration();
    
    cls.rendererStarted = false;
    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.soundRegistered = false;
    autoupdateChecked = false;
    
    // if not running a server clear the whole hunk
    if ( !com_sv_running->integer )
    {
        collisionModelManager->ClearMap();
        
        // clear the whole hunk
        memorySystem->Clear();
    }
    else
    {
        // clear all the client data on the hunk
        memorySystem->ClearToMark();
    }
    
    // initialize the renderer interface
    InitRef();
    
    // startup all the client stuff
    clientMainLocal.StartHunkUsers( false );
    
#ifdef _WIN32
    idsystem->Restart_f();
#endif
    
    // unpause so the cgame definately gets a snapshot and renders a frame
    cvarSystem->Set( "cl_paused", "0" );
    
    // start the cgame if connected
    if ( cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC )
    {
        cls.cgameStarted = true;
        clientGameSystem->InitCGame();
        
        // send pure checksums
        SendPureChecksums();
    }
}

/*
=================
idClientMainSystemLocal::UI_Restart_f

Restart the ui subsystem
=================
*/
void idClientMainSystemLocal::UI_Restart_f( void )
{
    // NERVE - SMF
    // shutdown the GUI
    clientGUISystem->ShutdownGUI();
    
    autoupdateChecked = false;
    
    // init the UI
    clientGUISystem->InitGUI();
}

/*
=================
idClientMainSystemLocal::Snd_Reload_f

Reloads sounddata from disk, retains soundhandles.
=================
*/
void idClientMainSystemLocal::Snd_Reload_f( void )
{
    soundSystem->Reload();
}

/*
=================
idClientMainSystemLocal::Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void idClientMainSystemLocal::Snd_Restart_f( void )
{
    soundSystem->Shutdown();
    soundSystem->Init();
    
    Vid_Restart_f();
}

/*
==================
idClientMainSystemLocal::PK3List_f
==================
*/
void idClientMainSystemLocal::OpenedPK3List_f( void )
{
    Com_Printf( "Opened PK3 Names: %s\n", fileSystem->LoadedPakNames() );
}

/*
==================
idClientMainSystemLocal::PureList_f
==================
*/
void idClientMainSystemLocal::ReferencedPK3List_f( void )
{
    Com_Printf( "Referenced PK3 Names: %s\n", fileSystem->ReferencedPakNames() );
}

/*
==================
idClientMainSystemLocal::Configstrings_f
==================
*/
void idClientMainSystemLocal::Configstrings_f( void )
{
    S32 i, ofs;
    
    if ( cls.state != CA_ACTIVE )
    {
        Com_Printf( "Not connected to a server.\n" );
        return;
    }
    
    for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
    {
        ofs = cl.gameState.stringOffsets[i];
        
        if ( !ofs )
        {
            continue;
        }
        
        Com_Printf( "%4i: %s\n", i, cl.gameState.stringData + ofs );
    }
}

/*
==============
idClientMainSystemLocal::Clientinfo_f
==============
*/
void idClientMainSystemLocal::Clientinfo_f( void )
{
    Com_Printf( "--------- Client Information ---------\n" );
    Com_Printf( "state: %i\n", cls.state );
    Com_Printf( "Server: %s\n", cls.servername );
    Com_Printf( "User info settings:\n" );
    Info_Print( cvarSystem->InfoString( CVAR_USERINFO ) );
    Com_Printf( "--------------------------------------\n" );
}

/*
==============
idClientMainSystemLocal::WavRecord_f
==============
*/
void idClientMainSystemLocal::WavRecord_f( void )
{
    if ( clc.wavefile )
    {
        Com_Printf( "Already recording a wav file\n" );
        return;
    }
    
    WriteWaveOpen();
}

/*
==============
idClientMainSystemLocal::WavStopRecord_f
==============
*/
void idClientMainSystemLocal::WavStopRecord_f( void )
{
    if ( !clc.wavefile )
    {
        Com_Printf( "Not recording a wav file\n" );
        return;
    }
    
    WriteWaveClose();
    
    cvarSystem->Set( "cl_waverecording", "0" );
    cvarSystem->Set( "cl_wavefilename", "" );
    cvarSystem->Set( "cl_waveoffset", "0" );
    
    clc.waverecording = false;
}

/*
===============
idClientMainSystemLocal::Video_f

video
video [filename]
===============
*/
void idClientMainSystemLocal::Video_f( void )
{
    S32 i, last;
    UTF8 filename[MAX_OSPATH];
    
    if ( !clc.demoplaying )
    {
        Com_Printf( "The video command can only be used when playing back demos\n" );
        return;
    }
    
    if ( cmdSystem->Argc() == 2 )
    {
        // explicit filename
        Q_snprintf( filename, MAX_OSPATH, "videos/%s.avi", cmdSystem->Argv( 1 ) );
    }
    else
    {
        // scan for a free filename
        for ( i = 0; i <= 9999; i++ )
        {
            S32 a, b, c, d;
            
            last = i;
            
            a = last / 1000;
            last -= a * 1000;
            b = last / 100;
            last -= b * 100;
            c = last / 10;
            last -= c * 10;
            d = last;
            
            Q_snprintf( filename, MAX_OSPATH, "videos/video%d%d%d%d.avi", a, b, c, d );
            
            if ( !fileSystem->FileExists( filename ) )
            {
                // file doesn't exist
                break;
            }
        }
        
        if ( i > 9999 )
        {
            Com_Printf( S_COLOR_RED "ERROR: no free file names to create video\n" );
            return;
        }
    }
    
    clientAVISystem->OpenAVIForWriting( filename );
}

/*
===============
idClientMainSystemLocal::StopVideo_f
===============
*/
void idClientMainSystemLocal::StopVideo_f( void )
{
    clientAVISystem->CloseAVI();
}

/*
=================
idClientMainSystemLocal::DownloadsComplete

Called when all downloading has been completed
=================
*/
void idClientMainSystemLocal::DownloadsComplete( void )
{
#ifndef _WIN32
    UTF8*    fs_write_path;
#endif
    UTF8*    fn;
    
    if ( cls.state < CA_CONNECTING )
    {
        //	a download has completed outside of a game
        return;
    }
    
    if ( cls.state == CA_ACTIVE )
    {
        //	a download has completed while the game is playing
        //	inform the client that its download is complete
        cmdBufferSystem->ExecuteText( EXEC_INSERT, "donedl" );
        return;
    }
    
    // DHM - Nerve :: Auto-update (not finished yet)
    if ( autoupdateStarted )
    {
    
        if ( autoupdateFilename[0] && ( strlen( autoupdateFilename ) > 4 ) )
        {
#ifdef _WIN32
            // win32's Sys_StartProcess prepends the current dir
            fn = va( "%s/%s", fileSystem->ShiftStr( AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT ), autoupdateFilename );
#else
            fs_write_path = cvarSystem->VariableString( "fs_homepath" );
            fn = fileSystem->BuildOSPath( fs_write_path, fileSystem->ShiftStr( AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT ), autoupdateFilename );
#ifdef __LINUX__
            idsystem->Chmod( fn, S_IXUSR );
#endif
#endif
            idsystem->StartProcess( fn, true );
        }
        
        autoupdateStarted = false;
        clientMainLocal.Disconnect( true );
        return;
    }
    
    // if we downloaded files we need to restart the file system
    if ( cls.downloadRestart )
    {
        cls.downloadRestart = false;
        
        // We possibly downloaded a pak, restart the file system to load it
        fileSystem->Restart( clc.checksumFeed );
        
        if ( !cls.bWWWDlDisconnected )
        {
            // inform the server so we get new gamestate info
            clientMainLocal.AddReliableCommand( "donedl" );
        }
        
        // we can reset that now
        cls.bWWWDlDisconnected = false;
        ClearStaticDownload();
        
        // by sending the donedl command we request a new gamestate
        // so we don't want to load stuff yet
        return;
    }
    
    if ( cls.bWWWDlDisconnected )
    {
        cls.bWWWDlDisconnected = false;
        ClearStaticDownload();
        return;
    }
    
    // let the client game init and load data
    cls.state = CA_LOADING;
    
    // Pump the loop, this may change gamestate!
    Com_EventLoop();
    
    // if the gamestate was changed by calling Com_EventLoop
    // then we loaded everything already and we don't want to do it again.
    if ( cls.state != CA_LOADING )
    {
        return;
    }
    
    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set( "r_uiFullScreen", "0" );
    
    // flush client memory and start loading stuff
    // this will also (re)load the UI
    // if this is a local client then only the client part of the hunk
    // will be cleared, note that this is done after the hunk mark has been set
    clientMainLocal.FlushMemory();
    
    // initialize the CGame
    cls.cgameStarted = true;
    clientGameSystem->InitCGame();
    
    // set pure checksums
    SendPureChecksums();
    
    CL_WritePacket();
    CL_WritePacket();
    CL_WritePacket();
}

/*
=================
idClientMainSystemLocal::BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void idClientMainSystemLocal::BeginDownload( StringEntry localName, StringEntry remoteName )
{

    Com_DPrintf( "***** idClientMainSystemLocal::BeginDownload *****\n"
                 "Localname: %s\n" "Remotename: %s\n" "****************************\n", localName, remoteName );
                 
    Q_strncpyz( cls.downloadName, localName, sizeof( cls.downloadName ) );
    Q_snprintf( cls.downloadTempName, sizeof( cls.downloadTempName ), "%s.tmp", localName );
    
    // Set so UI gets access to it
    cvarSystem->Set( "cl_downloadName", remoteName );
    cvarSystem->Set( "cl_downloadSize", "0" );
    cvarSystem->Set( "cl_downloadCount", "0" );
    cvarSystem->SetValue( "cl_downloadTime", ( F32 )cls.realtime );
    
    clc.downloadBlock = 0;		// Starting new file
    clc.downloadCount = 0;
    
    clientMainLocal.AddReliableCommand( va( "download %s", remoteName ) );
}

/*
=================
idClientMainSystemLocal::NextDownload

A download completed or failed
=================
*/
void idClientMainSystemLocal::NextDownload( void )
{
    UTF8* s, *remoteName, *localName;
    
    // We are looking to start a download here
    if ( *clc.downloadList )
    {
        s = clc.downloadList;
        
        //format is:
        //@remotename@localname@remotename@localname, etc.
        
        if ( *s == '@' )
        {
            s++;
        }
        
        remoteName = s;
        
        if ( ( s = ::strchr( s, '@' ) ) == nullptr )
        {
            DownloadsComplete();
            return;
        }
        
        *s++ = 0;
        localName = s;
        
        if ( ( s = ::strchr( s, '@' ) ) != nullptr )
        {
            *s++ = 0;
        }
        else
        {
            // point at the nul byte
            s = localName + ::strlen( localName );
            
        }
        
        BeginDownload( localName, remoteName );
        
        cls.downloadRestart = true;
        
        // move over the rest
        ::memmove( clc.downloadList, s, strlen( s ) + 1 );
        
        return;
    }
    
    DownloadsComplete();
}

/*
=================
idClientMainSystemLocal::InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void idClientMainSystemLocal::InitDownloads( void )
{
#ifndef PRE_RELEASE_DEMO
    UTF8 missingfiles[1024], *dir = fileSystem->ShiftStr( AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT );
    
    // TTimo
    // init some of the www dl data
    clc.bWWWDl = false;
    clc.bWWWDlAborting = false;
    cls.bWWWDlDisconnected = false;
    
    ClearStaticDownload();
    
    if ( autoupdateStarted && networkSystem->CompareAdr( cls.autoupdateServer, clc.serverAddress ) )
    {
        if ( ::strlen( cl_updatefiles->string ) > 4 )
        {
            Q_strncpyz( autoupdateFilename, cl_updatefiles->string, sizeof( autoupdateFilename ) );
            Q_strncpyz( clc.downloadList, va( "@%s/%s@%s/%s", dir, cl_updatefiles->string, dir, cl_updatefiles->string ), MAX_INFO_STRING );
            
            cls.state = CA_CONNECTED;
            
            NextDownload();
            return;
        }
    }
    else
    {
        // whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
        if ( fileSystem->ComparePaks( missingfiles, sizeof( missingfiles ), false ) )
        {
            cvarSystem->Set( "com_missingFiles", missingfiles );
        }
        else
        {
            cvarSystem->Set( "com_missingFiles", "" );
        }
        
        // reset the redirect checksum tracking
        clc.redirectedList[0] = '\0';
        
        if ( cl_allowDownload->integer && fileSystem->ComparePaks( clc.downloadList, sizeof( clc.downloadList ), true ) )
        {
            // this gets printed to UI, i18n
            Com_Printf( TranslateStringBuf( "Need paks: %s\n" ), clc.downloadList );
            
            if ( *clc.downloadList )
            {
                // if autodownloading is not enabled on the server
                cls.state = CA_CONNECTED;
                NextDownload();
                return;
            }
        }
    }
#endif
    
    DownloadsComplete();
}

/*
=================
idClientMainSystemLocal::CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void idClientMainSystemLocal::CheckForResend( void )
{
    S32 port, i, pktlen;
    UTF8 info[MAX_INFO_STRING], data[MAX_INFO_STRING], pkt[1024 + 1];
    
    // don't send anything if playing back a demo
    if ( clc.demoplaying )
    {
        return;
    }
    
    // resend if we haven't gotten a reply yet
    if ( cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING )
    {
        return;
    }
    
    if ( cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT )
    {
        return;
    }
    
    // for retransmit requests
    clc.connectTime = cls.realtime;
    clc.connectPacketCount++;
    
    switch ( cls.state )
    {
        case CA_CONNECTING:
            ::strcpy( pkt, "getchallenge" );
            pktlen = ( S32 )::strlen( pkt );
            networkChainSystem->OutOfBandPrint( NS_CLIENT, clc.serverAddress, "%s", pkt );
            break;
            
        case CA_CHALLENGING:
            // sending back the challenge
            port = ( S32 )cvarSystem->VariableValue( "net_qport" );
            
            Q_strncpyz( info, cvarSystem->InfoString( CVAR_USERINFO ), sizeof( info ) );
            Info_SetValueForKey( info, "protocol", va( "%i", com_protocol->integer ) );
            Info_SetValueForKey( info, "qport", va( "%i", port ) );
            Info_SetValueForKey( info, "challenge", va( "%i", clc.challenge ) );
            
            ::strcpy( data, "connect " );
            
            // NERVE - SMF - spaces in name bugfix
            data[8] = '\"';
            
            for ( i = 0; i < strlen( info ); i++ )
            {
                data[9 + i] = info[i];
            }
            
            // NERVE - SMF - spaces in name bugfix
            data[9 + i] = '\"';
            data[10 + i] = 0;
            
            // EVEN BALANCE - T.RAY
            pktlen = i + 10;
            ::memcpy( pkt, &data[0], pktlen );
            
            networkChainSystem->OutOfBandData( NS_CLIENT, clc.serverAddress, ( U8* ) pkt, pktlen );
            // the most current userinfo has been sent, so watch for any
            // newer changes to userinfo variables
            cvar_modifiedFlags &= ~CVAR_USERINFO;
            break;
            
        default:
            Com_Error( ERR_FATAL, "idClientMainSystemLocal::CheckForResend: bad cls.state" );
    }
}

/*
===================
idClientMainSystemLocal::DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void idClientMainSystemLocal::DisconnectPacket( netadr_t from )
{
    StringEntry  message;
    
    if ( cls.state < CA_AUTHORIZING )
    {
        return;
    }
    
    // if not from our server, ignore it
    if ( !networkSystem->CompareAdr( from, clc.netchan.remoteAddress ) )
    {
        return;
    }
    
    // if we have received packets within three seconds, ignore (it might be a malicious spoof)
    // NOTE TTimo:
    // there used to be a  clc.lastPacketTime = cls.realtime; line in PacketEvent before calling ConnectionLessPacket
    // therefore .. packets never got through this check, clients never disconnected
    // switched the clc.lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
    // you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
    if ( cls.realtime - clc.lastPacketTime < 3000 )
    {
        return;
    }
    
    // if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
    if ( !cls.bWWWDlDisconnected )
    {
        // drop the connection
        message = "Server disconnected for unknown reason";
        Com_Printf( "%s", message );
        cvarSystem->Set( "com_errorMessage", message );
        clientMainLocal.Disconnect( true );
    }
    else
    {
        clientMainLocal.Disconnect( false );
        cvarSystem->Set( "ui_connecting", "1" );
        cvarSystem->Set( "ui_dl_running", "1" );
    }
}

/*
=================
idClientMainSystemLocal::str_replace
=================
*/
UTF8* idClientMainSystemLocal::str_replace( StringEntry string, StringEntry substr, StringEntry replacement )
{
    UTF8* tok = nullptr;
    UTF8* newstr = nullptr;
    UTF8* oldstr = nullptr;
    
    /* if either substr or replacement is nullptr, duplicate string a let caller handle it */
    if ( substr == nullptr || replacement == nullptr )
    {
        return ::strdup( string );
    }
    
    newstr = ::strdup( string );
    
    while ( ( tok = ::strstr( newstr, substr ) ) )
    {
        oldstr = newstr;
        newstr = ( UTF8* )::malloc( ::strlen( oldstr ) - ::strlen( substr ) + ::strlen( replacement ) + 1 );
        
        /*failed to alloc mem, free old string and return nullptr */
        if ( newstr == nullptr )
        {
            ::free( oldstr );
            return nullptr;
        }
        
        ::memcpy( newstr, oldstr, tok - oldstr );
        ::memcpy( newstr + ( tok - oldstr ), replacement, ::strlen( replacement ) );
        ::memcpy( newstr + ( tok - oldstr ) + ::strlen( replacement ), tok + ::strlen( substr ), strlen( oldstr ) - ::strlen( substr ) - ( tok - oldstr ) );
        ::memset( newstr + ::strlen( oldstr ) - ::strlen( substr ) + ::strlen( replacement ) , 0, 1 );
        ::free( oldstr );
    }
    
    return newstr;
}

/*
===================
idClientMainSystemLocal::MotdPacket
===================
*/
void idClientMainSystemLocal::MotdPacket( netadr_t from, StringEntry info )
{
    UTF8* w;
    StringEntry v;
    
    // if not from our server, ignore it
    if ( !networkSystem->CompareAdr( from, cls.updateServer ) )
    {
        Com_DPrintf( "MOTD packet from unexpected source\n" );
        return;
    }
    
    Com_DPrintf( "MOTD packet: %s\n", info );
    
    while ( *info != '\\' )
    {
        info++;
    }
    
    // check challenge
    v = Info_ValueForKey( info, "challenge" );
    
    if ( ::strcmp( v, cls.updateChallenge ) )
    {
        Com_DPrintf( "MOTD packet mismatched challenge: "
                     "'%s' != '%s'\n", v, cls.updateChallenge );
        return;
    }
    
    v = Info_ValueForKey( info, "motd" );
    w = str_replace( v, "|", "\n" );
    
    Q_strncpyz( cls.updateInfoString, info, sizeof( cls.updateInfoString ) );
    
    cvarSystem->Set( "cl_newsString", w );
    
    free( w );
}

/*
===================
idClientMainSystemLocal::PrintPackets
an OOB message from server, with potential markups
print OOB are the only messages we handle markups in
[err_dialog]: used to indicate that the connection should be aborted
  no further information, just do an error diagnostic screen afterwards
[err_prot]: HACK. This is a protocol error. The client uses a custom
  protocol error message (client sided) in the diagnostic window.
  The space for the error message on the connection screen is limited
  to 256 chars.
===================
*/
void idClientMainSystemLocal::PrintPacket( netadr_t from, msg_t* msg )
{
    UTF8* s;
    
    s = MSG_ReadBigString( msg );
    if ( !Q_stricmpn( s, "[err_dialog]", 12 ) )
    {
        Q_strncpyz( clc.serverMessage, s + 12, sizeof( clc.serverMessage ) );
        // cvarSystem->Set("com_errorMessage", clc.serverMessage );
        
        Com_Error( ERR_DROP, "%s", clc.serverMessage );
    }
    else if ( !Q_stricmpn( s, "[err_prot]", 10 ) )
    {
        Q_strncpyz( clc.serverMessage, s + 10, sizeof( clc.serverMessage ) );
        // cvarSystem->Set("com_errorMessage", TranslateStringBuf( PROTOCOL_MISMATCH_ERROR_LONG ) );
        
        Com_Error( ERR_DROP, "%s", TranslateStringBuf( PROTOCOL_MISMATCH_ERROR_LONG ) );
    }
    else if ( !Q_stricmpn( s, "[err_update]", 12 ) )
    {
        Q_strncpyz( clc.serverMessage, s + 12, sizeof( clc.serverMessage ) );
        
        Com_Error( ERR_AUTOUPDATE, "%s", clc.serverMessage );
    }
    else if ( !Q_stricmpn( s, "ET://", 5 ) )
    {
        // fretn
        Q_strncpyz( clc.serverMessage, s, sizeof( clc.serverMessage ) );
        cvarSystem->Set( "com_errorMessage", clc.serverMessage );
        
        Com_Error( ERR_DROP, "%s", clc.serverMessage );
    }
    else
    {
        Q_strncpyz( clc.serverMessage, s, sizeof( clc.serverMessage ) );
    }
    
    Com_Printf( "%s", clc.serverMessage );
}

/*
=================
idClientMainSystemLocal::ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void idClientMainSystemLocal::ConnectionlessPacket( netadr_t from, msg_t* msg )
{
    UTF8* s, *c;
    S32 challenge = 0;
    
    MSG_BeginReadingOOB( msg );
    // skip the -1
    MSG_ReadLong( msg );
    
    s = MSG_ReadStringLine( msg );
    
    cmdSystem->TokenizeString( s );
    
    c = cmdSystem->Argv( 0 );
    
    Com_DPrintf( "CL packet %s: %s\n", networkSystem->AdrToString( from ), c );
    
    if ( !Q_stricmp( c, "disconnectResponse" ) )
    {
        if ( cls.state == CA_CONNECTED )
        {
            clientMainLocal.Disconnect( true );
        }
        
        return;
    }
    
    // challenge from the server we are connecting to
    if ( !Q_stricmp( c, "challengeResponse" ) )
    {
        if ( cls.state != CA_CONNECTING )
        {
            Com_Printf( "Unwanted challenge response received.  Ignored.\n" );
            return;
        }
        
        c = cmdSystem->Argv( 2 );
        if ( *c )
        {
            challenge = atoi( c );
        }
        
        if ( !networkSystem->CompareAdr( from, clc.serverAddress ) )
        {
            // This challenge response is not coming from the expected address.
            // Check whether we have a matching client challenge to prevent
            // connection hi-jacking.
            if ( !*c || challenge != clc.challenge )
            {
                Com_DPrintf( "Challenge response received from unexpected source. Ignored.\n" );
                return;
            }
        }
        
        // start sending challenge response instead of challenge request packets
        clc.challenge = atoi( cmdSystem->Argv( 1 ) );
        if ( cmdSystem->Argc() > 2 )
        {
            clc.onlyVisibleClients = atoi( cmdSystem->Argv( 2 ) );
        }
        else
        {
            clc.onlyVisibleClients = 0;
        }
        cls.state = CA_CHALLENGING;
        clc.connectPacketCount = 0;
        clc.connectTime = -99999;
        
        // take this address as the new server address.  This allows
        // a server proxy to hand off connections to multiple servers
        clc.serverAddress = from;
        Com_DPrintf( "challengeResponse: %d\n", clc.challenge );
        return;
    }
    
    // server connection
    if ( !Q_stricmp( c, "connectResponse" ) )
    {
        if ( cls.state >= CA_CONNECTED )
        {
            Com_Printf( "Dup connect received.  Ignored.\n" );
            return;
        }
        
        if ( cls.state != CA_CHALLENGING )
        {
            Com_Printf( "connectResponse packet while not connecting.  Ignored.\n" );
            return;
        }
        
        if ( !networkSystem->CompareAdr( from, clc.serverAddress ) )
        {
            Com_Printf( "connectResponse from a different address.  Ignored.\n" );
            Com_Printf( "%s should have been %s\n", networkSystem->AdrToString( from ), networkSystem->AdrToString( clc.serverAddress ) );
            return;
        }
        
        // DHM - Nerve :: If we have completed a connection to the Auto-Update server...
        if ( autoupdateChecked && networkSystem->CompareAdr( cls.autoupdateServer, clc.serverAddress ) )
        {
            // Mark the client as being in the process of getting an update
            if ( cl_updateavailable->integer )
            {
                autoupdateStarted = true;
            }
        }
        
        networkChainSystem->Setup( NS_CLIENT, &clc.netchan, from, ( S32 )cvarSystem->VariableValue( "net_qport" ) );
        
        cls.state = CA_CONNECTED;
        // send first packet immediately
        clc.lastPacketSentTime = -9999;
        return;
    }
    
    // server responding to an info broadcast
    if ( !Q_stricmp( c, "infoResponse" ) )
    {
        idClientBrowserSystemLocal::ServerInfoPacket( from, msg );
        return;
    }
    
    // server responding to a get playerlist
    if ( !Q_stricmp( c, "statusResponse" ) )
    {
        idClientBrowserSystemLocal::ServerStatusResponse( from, msg );
        return;
    }
    
    // a disconnect message from the server, which will happen if the server
    // dropped the connection but it is still getting packets from us
    if ( !Q_stricmp( c, "disconnect" ) )
    {
        DisconnectPacket( from );
        return;
    }
    
    // echo request from server
    if ( !Q_stricmp( c, "echo" ) )
    {
        networkChainSystem->OutOfBandPrint( NS_CLIENT, from, "%s", cmdSystem->Argv( 1 ) );
        return;
    }
    
    // cd check
    if ( !Q_stricmp( c, "keyAuthorize" ) )
    {
        // we don't use these now, so dump them on the floor
        return;
    }
    
    // global MOTD from id
    if ( !Q_stricmp( c, "motd" ) )
    {
        MotdPacket( from, s );
        return;
    }
    
    // echo request from server
    if ( !Q_stricmp( c, "print" ) )
    {
        if ( networkSystem->CompareAdr( from, clc.serverAddress ) || networkSystem->CompareAdr( from, rcon_address ) )
        {
            PrintPacket( from, msg );
        }
        
        return;
    }
    
    // DHM - Nerve :: Auto-update server response message
    if ( !Q_stricmp( c, "updateResponse" ) )
    {
        UpdateInfoPacket( from );
        return;
    }
    // DHM - Nerve
    
    // NERVE - SMF - bugfix, make this compare first n chars so it doesnt bail if token is parsed incorrectly
    // echo request from server
    if ( !Q_strncmp( c, "getserversResponse", 18 ) )
    {
        idClientBrowserSystemLocal::ServersResponsePacket( &from, msg, false );
        return;
    }
    
    // list of servers sent back by a master server (extended)
    if ( !Q_strncmp( c, "getserversExtResponse", 21 ) )
    {
        idClientBrowserSystemLocal::ServersResponsePacket( &from, msg, true );
        return;
    }
    
    Com_DPrintf( "Unknown connectionless packet command.\n" );
}

/*
=================
idClientMainSystemLocal::PacketEvent

A packet has arrived from the main event loop
=================
*/
void idClientMainSystemLocal::PacketEvent( netadr_t from, msg_t* msg )
{
    S32 headerBytes;
    
    if ( msg->cursize >= 4 && *( S32* )msg->data == -1 )
    {
        ConnectionlessPacket( from, msg );
        return;
    }
    
    clc.lastPacketTime = cls.realtime;
    
    if ( cls.state < CA_CONNECTED )
    {
        // can't be a valid sequenced packet
        return;
    }
    
    if ( msg->cursize < 4 )
    {
        Com_Printf( "%s: Runt packet\n", networkSystem->AdrToString( from ) );
        return;
    }
    
    // packet from server
    if ( !networkSystem->CompareAdr( from, clc.netchan.remoteAddress ) )
    {
        Com_DPrintf( "%s:sequenced packet without connection\n", networkSystem->AdrToString( from ) );
        // FIXME: send a client disconnect?
        return;
    }
    
    if ( !idClientNetworkChainSystemLocal::Netchan_Process( &clc.netchan, msg ) )
    {
        // out of order, duplicated, etc
        return;
    }
    
    // the header is different lengths for reliable and unreliable messages
    headerBytes = msg->readcount;
    
    // track the last message received so it can be returned in
    // client messages, allowing the server to detect a dropped
    // gamestate
    clc.serverMessageSequence = LittleLong( *( S32* )msg->data );
    
    clc.lastPacketTime = cls.realtime;
    idClientParseSystemLocal::ParseServerMessage( msg );
    
    // we don't know if it is ok to save a demo message until
    // after we have parsed the frame
    if ( clc.demorecording && !clc.demowaiting )
    {
        WriteDemoMessage( msg, headerBytes );
    }
}

/*
==================
idClientMainSystemLocal::CheckTimeout
==================
*/
void idClientMainSystemLocal::CheckTimeout( void )
{
    // check timeout
    if ( ( !cl_paused->integer || !sv_paused->integer )
            && cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC && cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000 )
    {
        if ( ++cl.timeoutcount > 5 )
        {
            // timeoutcount saves debugger
            cvarSystem->Set( "com_errorMessage", "Server connection timed out." );
            clientMainLocal.Disconnect( true );
            
            return;
        }
    }
    else
    {
        cl.timeoutcount = 0;
    }
}

/*
=================
idClientMainSystemLocal::CheckPaused
=================
*/
bool idClientMainSystemLocal::CheckPaused( void )
{
    // if cl_paused->modified is set, the cvar has only been changed in
    // this frame. Keep paused in this frame to ensure the server doesn't
    // lag behind.
    if ( cl_paused->integer || cl_paused->modified )
    {
        return true;
    }
    
    return false;
}

/*
==================
idClientMainSystemLocal::CheckUserinfo
==================
*/
void idClientMainSystemLocal::CheckUserinfo( void )
{
    // don't add reliable commands when not yet connected
    if ( cls.state < CA_CONNECTED )
    {
        return;
    }
    
    // don't overflow the reliable command buffer when paused
    if ( sv_paused->integer && com_sv_running->integer && CheckPaused() )
    {
        return;
    }
    
    // send a reliable userinfo update if needed
    if ( cvar_modifiedFlags & CVAR_USERINFO )
    {
        cvar_modifiedFlags &= ~CVAR_USERINFO;
        clientMainLocal.AddReliableCommand( va( "userinfo \"%s\"", cvarSystem->InfoString( CVAR_USERINFO ) ) );
    }
}

/*
==================
idClientMainSystemLocal::WWWDownload
==================
*/
void idClientMainSystemLocal::WWWDownload( void )
{
    UTF8* to_ospath;
    dlStatus_t      ret;
    static bool bAbort = false;
    
    if ( clc.bWWWDlAborting )
    {
        if ( !bAbort )
        {
            Com_DPrintf( "idClientMainSystemLocal::WWWDownload: WWWDlAborting\n" );
            bAbort = true;
        }
        
        return;
    }
    
    if ( bAbort )
    {
        Com_DPrintf( "idClientMainSystemLocal::WWWDownload: WWWDlAborting done\n" );
        bAbort = false;
    }
    
    ret = downloadSystem->DownloadLoop();
    
    if ( ret == DL_CONTINUE )
    {
        return;
    }
    
    if ( ret == DL_DONE )
    {
        // taken from ParseDownload
        // we work with OS paths
        clc.download = 0;
        
        to_ospath = fileSystem->BuildOSPath( cvarSystem->VariableString( "fs_homepath" ), cls.originalDownloadName, "" );
        to_ospath[strlen( to_ospath ) - 1] = '\0';
        
        if ( rename( cls.downloadTempName, to_ospath ) )
        {
            fileSystem->FSCopyFile( cls.downloadTempName, to_ospath );
            ::remove( cls.downloadTempName );
        }
        
        *cls.downloadTempName = *cls.downloadName = 0;
        
        cvarSystem->Set( "cl_downloadName", "" );
        
        if ( cls.bWWWDlDisconnected )
        {
            // for an auto-update in disconnected mode, we'll be spawning the setup in CL_DownloadsComplete
            if ( !autoupdateStarted )
            {
                // reconnect to the server, which might send us to a new disconnected download
                cmdBufferSystem->ExecuteText( EXEC_APPEND, "reconnect\n" );
            }
        }
        else
        {
            clientMainLocal.AddReliableCommand( "wwwdl done" );
            
            // tracking potential web redirects leading us to wrong checksum - only works in connected mode
            if ( ::strlen( clc.redirectedList ) + ::strlen( cls.originalDownloadName ) + 1 >= sizeof( clc.redirectedList ) )
            {
                // just to be safe
                Com_Printf( "ERROR: redirectedList overflow (%s)\n", clc.redirectedList );
            }
            else
            {
                ::strcat( clc.redirectedList, "@" );
                ::strcat( clc.redirectedList, cls.originalDownloadName );
            }
        }
    }
    else
    {
        if ( cls.bWWWDlDisconnected )
        {
            // in a connected download, we'd tell the server about failure and wait for a reply
            // but in this case we can't get anything from server
            // if we just reconnect it's likely we'll get the same disconnected download message, and error out again
            // this may happen for a regular dl or an auto update
            // get the msg before clearing structs
            StringEntry error = va( "Download failure while getting '%s'\n", cls.downloadName );
            
            // need clearing structs before ERR_DROP, or it goes into endless reload
            cls.bWWWDlDisconnected = false;
            
            ClearStaticDownload();
            
            Com_Error( ERR_DROP, "%s", error );
        }
        else
        {
            // see ParseDownload, same abort strategy
            Com_Printf( "Download failure while getting '%s'\n", cls.downloadName );
            clientMainLocal.AddReliableCommand( "wwwdl fail" );
            clc.bWWWDlAborting = true;
        }
        
        return;
    }
    
    clc.bWWWDl = false;
    
    NextDownload();
}

/*
==================
idClientMainSystemLocal::WWWBadChecksum

FS code calls this when doing fileSystem->ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
bool idClientMainSystemLocal::WWWBadChecksum( StringEntry pakname )
{
    if ( ::strstr( clc.redirectedList, va( "@%s", pakname ) ) )
    {
        Com_Printf( "WARNING: file %s obtained through download redirect has wrong checksum\n", pakname );
        Com_Printf( "         this likely means the server configuration is broken\n" );
        
        if ( ::strlen( clc.badChecksumList ) + ::strlen( pakname ) + 1 >= sizeof( clc.badChecksumList ) )
        {
            Com_Printf( "ERROR: badChecksumList overflowed (%s)\n", clc.badChecksumList );
            return false;
        }
        
        ::strcat( clc.badChecksumList, "@" );
        ::strcat( clc.badChecksumList, pakname );
        
        Com_DPrintf( "bad checksums: %s\n", clc.badChecksumList );
        
        return true;
    }
    
    return false;
}

/*
==================
idClientMainSystemLocal::Frame
==================
*/
void idClientMainSystemLocal::Frame( S32 msec )
{
    if ( !com_cl_running->integer )
    {
        soundSystem->Update();
        clientScreenSystem->UpdateScreen();
        return;
    }
    
    if ( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_UI ) && !com_sv_running->integer && clc.demoplaying )
    {
        // if disconnected, bring up the menu
        soundSystem->StopAllSounds();
        uiManager->SetActiveMenu( UIMENU_MAIN );
    }
    
    // if recording an avi, lock to a fixed fps
    if ( clientAVISystem->VideoRecording() && cl_aviFrameRate->integer && msec )
    {
        if ( !clientAVISystem->VideoRecording() )
        {
            Video_f();
        }
        
        // save the current screen
        if ( cls.state == CA_ACTIVE || cl_forceavidemo->integer )
        {
            clientAVISystem->TakeVideoFrame();
            
            // fixed time for next frame'
            msec = ( S32 )ceil( ( 1000.0f / cl_aviFrameRate->value ) * com_timescale->value );
            if ( msec == 0 )
            {
                msec = 1;
            }
        }
    }
    
    // save the msec before checking pause
    cls.realFrametime = msec;
    
    // decide the simulation time
    cls.frametime = msec;
    
    cls.realtime += cls.frametime;
    
    if ( cl_timegraph->integer )
    {
        idClientScreenSystemLocal::DebugGraph( cls.realFrametime * 0.25f, 0 );
    }
    
    // see if we need to update any userinfo
    CheckUserinfo();
    
    // if we haven't gotten a packet in a long time,
    // drop the connection
    CheckTimeout();
    
    // wwwdl download may survive a server disconnect
    if ( ( cls.state == CA_CONNECTED && clc.bWWWDl ) || cls.bWWWDlDisconnected )
    {
        WWWDownload();
    }
    
    // send intentions now
    CL_SendCmd();
    
    // resend a connection request if necessary
    CheckForResend();
    
    // decide on the serverTime to render
    clientGameSystem->SetCGameTime();
    
    // update the screen
    clientScreenSystem->UpdateScreen();
    
    // update the sound
    soundSystem->Update();
    
    // advance local effects for next frame
    SCR_RunCinematic();
    
    Con_RunConsole();
    
    cls.framecount++;
}

/*
=================
idClientMainSystemLocal::Cache_StartGather_f
=================
*/
void idClientMainSystemLocal::Cache_StartGather_f( void )
{
    cacheIndex = 0;
    memset( cacheItems, 0, sizeof( cacheItems ) );
    
    cvarSystem->Set( "cl_cacheGathering", "1" );
}

/*
=================
idClientMainSystemLocal::Cache_UsedFile_f
=================
*/
void idClientMainSystemLocal::Cache_UsedFile_f( void )
{
    S32 i, group;
    UTF8 groupStr[MAX_QPATH], itemStr[MAX_QPATH];
    cacheItem_t* item;
    
    if ( cmdSystem->Argc() < 2 )
    {
        Com_Error( ERR_DROP, "usedfile without enough parameters\n" );
        return;
    }
    
    ::strcpy( groupStr, cmdSystem->Argv( 1 ) );
    
    ::strcpy( itemStr, cmdSystem->Argv( 2 ) );
    
    for ( i = 3; i < cmdSystem->Argc(); i++ )
    {
        ::strcat( itemStr, " " );
        ::strcat( itemStr, cmdSystem->Argv( i ) );
    }
    
    Q_strlwr( itemStr );
    
    // find the cache group
    for ( i = 0; i < CACHE_NUMGROUPS; i++ )
    {
        if ( !Q_strncmp( groupStr, cacheGroups[i].name, MAX_QPATH ) )
        {
            break;
        }
    }
    
    if ( i == CACHE_NUMGROUPS )
    {
        Com_Error( ERR_DROP, "usedfile without a valid cache group\n" );
        return;
    }
    
    // see if it's already there
    group = i;
    
    for ( i = 0, item = cacheItems[group]; i < MAX_CACHE_ITEMS; i++, item++ )
    {
        if ( !item->name[0] )
        {
            // didn't find it, so add it here
            Q_strncpyz( item->name, itemStr, MAX_QPATH );
            
            if ( cacheIndex > 9999 )
            {
                // hack, but yeh
                item->hits = cacheIndex;
            }
            else
            {
                item->hits++;
            }
            
            item->lastSetIndex = cacheIndex;
            
            break;
        }
        
        if ( item->name[0] == itemStr[0] && !Q_strncmp( item->name, itemStr, MAX_QPATH ) )
        {
            if ( item->lastSetIndex != cacheIndex )
            {
                item->hits++;
                item->lastSetIndex = cacheIndex;
            }
            
            break;
        }
    }
}

/*
=================
idClientMainSystemLocal::Cache_SetIndex_f
=================
*/
void idClientMainSystemLocal::Cache_SetIndex_f( void )
{
    if ( cmdSystem->Argc() < 2 )
    {
        Com_Error( ERR_DROP, "setindex needs an index\n" );
        return;
    }
    
    cacheIndex = atoi( cmdSystem->Argv( 1 ) );
}

/*
=================
idClientMainSystemLocal::Cache_MapChange_f
=================
*/
void idClientMainSystemLocal::Cache_MapChange_f( void )
{
    cacheIndex++;
}

/*
=================
idClientMainSystemLocal::Cache_EndGather_f
=================
*/
void idClientMainSystemLocal::Cache_EndGather_f( void )
{
    // save the frequently used files to the cache list file
    S32 i, j, handle, cachePass;
    UTF8 filename[MAX_QPATH];
    
    cachePass = ( S32 )floor( ( F32 )cacheIndex * CACHE_HIT_RATIO );
    
    for ( i = 0; i < CACHE_NUMGROUPS; i++ )
    {
        Q_strncpyz( filename, cacheGroups[i].name, MAX_QPATH );
        Q_strcat( filename, MAX_QPATH, ".cache" );
        
        handle = fileSystem->FOpenFileWrite( filename );
        
        for ( j = 0; j < MAX_CACHE_ITEMS; j++ )
        {
            // if it's a valid filename, and it's been hit enough times, cache it
            if ( cacheItems[i][j].hits >= cachePass && strstr( cacheItems[i][j].name, "/" ) )
            {
                fileSystem->Write( cacheItems[i][j].name, ( S32 )::strlen( cacheItems[i][j].name ), handle );
                fileSystem->Write( "\n", 1, handle );
            }
        }
        
        fileSystem->FCloseFile( handle );
    }
    
    cvarSystem->Set( "cl_cacheGathering", "0" );
}

// done.
//============================================================================

/*
================
idClientMainSystemLocal::SetRecommended_f
================
*/
void idClientMainSystemLocal::SetRecommended_f( void )
{
    Com_SetRecommended();
}

/*
================
idClientMainSystemLocal::RefPrintf

DLL glue
================
*/
void idClientMainSystemLocal::RefPrintf( S32 print_level, StringEntry fmt, ... )
{
    va_list argptr;
    UTF8  msg[MAXPRINTMSG];
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
    va_end( argptr );
    
    if ( print_level == PRINT_ALL )
    {
        Com_Printf( "%s", msg );
    }
    else if ( print_level == PRINT_WARNING )
    {
        // yellow
        Com_Printf( S_COLOR_YELLOW "%s", msg );
    }
    else if ( print_level == PRINT_DEVELOPER )
    {
        // red
        Com_DPrintf( S_COLOR_RED "%s", msg );
    }
}

/*
============
idClientMainSystemLocal::InitRenderer
============
*/
void idClientMainSystemLocal::InitRenderer( void )
{
    fileHandle_t f;
    
    cls.rendererStarted = true;
    
    // this sets up the renderer and calls R_Init
    renderSystem->Init( &cls.glconfig );
    
    // load character sets
    cls.charSetShader = renderSystem->RegisterShader( "gfx/2d/bigchars" );
    
    cls.useLegacyConsoleFont = true;
    
    // Register console font specified by cl_consoleFont, if any
    // filehandle is unused but forces fileSystem->FOpenFileRead() to heed purecheck because it does not when filehandle is nullptr
    if ( cl_consoleFont->string[0] )
    {
        if ( fileSystem->FOpenFileByMode( cl_consoleFont->string, &f, FS_READ ) >= 0 )
        {
            renderSystem->RegisterFont( cl_consoleFont->string, cl_consoleFontSize->integer, &cls.consoleFont );
            cls.useLegacyConsoleFont = false;
        }
        
        fileSystem->FCloseFile( f );
    }
    
    cls.whiteShader = renderSystem->RegisterShader( "white" );
    cls.consoleShader = renderSystem->RegisterShader( "console" );
    
    g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
    g_consoleField.widthInChars = g_console_field_width;
}

/*
============================
idClientMainSystemLocal::StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void idClientMainSystemLocal::StartHunkUsers( bool rendererOnly )
{
    if ( !com_cl_running )
    {
        return;
    }
    
    if ( !com_cl_running->integer )
    {
        return;
    }
    
    if ( !cls.rendererStarted )
    {
        cls.rendererStarted = true;
        InitRenderer();
    }
    
    if ( rendererOnly )
    {
        return;
    }
    
    if ( !cls.soundStarted )
    {
        cls.soundStarted = true;
        soundSystem->Init();
    }
    
    if ( !cls.soundRegistered )
    {
        cls.soundRegistered = true;
        soundSystem->BeginRegistration();
    }
    
    if ( com_dedicated->integer )
    {
        return;
    }
    
    if ( !cls.uiStarted )
    {
        cls.uiStarted = true;
        clientGUISystem->InitGUI();
    }
}

/*
=================
idClientMainSystemLocal::CheckAutoUpdate
=================
*/
void idClientMainSystemLocal::CheckAutoUpdate( void )
{
#ifndef PRE_RELEASE_DEMO

    if ( !cl_autoupdate->integer )
    {
        return;
    }
    
    // Only check once per session
    if ( autoupdateChecked )
    {
        return;
    }
    
    srand( Com_Milliseconds() );
    
    // Resolve update server
    if ( !networkChainSystem->StringToAdr( cls.autoupdateServerNames[0], &cls.autoupdateServer, NA_IP ) )
    {
        Com_DPrintf( "Failed to resolve any Auto-update servers.\n" );
        
        cls.autoUpdateServerChecked[0] = true;
        
        autoupdateChecked = true;
        return;
    }
    
    cls.autoupdatServerIndex = 0;
    
    cls.autoupdatServerFirstIndex = cls.autoupdatServerIndex;
    
    cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;
    
    cls.autoupdateServer.port = BigShort( PORT_SERVER );
    Com_DPrintf( "autoupdate server at: %i.%i.%i.%i:%i\n", cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                 cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                 BigShort( cls.autoupdateServer.port ) );
                 
    networkChainSystem->OutOfBandPrint( NS_CLIENT, cls.autoupdateServer, "getUpdateInfo \"%s\" \"%s\"\n", Q3_VERSION, ARCH_STRING );
    
#endif // !PRE_RELEASE_DEMO
    
    RequestMotd();
    
    autoupdateChecked = true;
}

/*
=================
idClientMainSystemLocal::NextUpdateServer
=================
*/
bool idClientMainSystemLocal::NextUpdateServer( void )
{
    UTF8*        servername;
    
#ifdef PRE_RELEASE_DEMO
    return false;
#endif // PRE_RELEASE_DEMO
    
    if ( !cl_autoupdate->integer )
    {
        return false;
    }
    
#if 0 //def _DEBUG
    Com_Printf( S_COLOR_MAGENTA "Autoupdate hardcoded OFF in debug build\n" );
    return false;
#endif
    
    while ( cls.autoUpdateServerChecked[cls.autoupdatServerFirstIndex] )
    {
        cls.autoupdatServerIndex++;
        
        if ( cls.autoupdatServerIndex > MAX_AUTOUPDATE_SERVERS )
        {
            cls.autoupdatServerIndex = 0;
        }
        
        if ( cls.autoupdatServerIndex == cls.autoupdatServerFirstIndex )
        {
            // went through all of them already
            return false;
        }
    }
    
    servername = cls.autoupdateServerNames[cls.autoupdatServerIndex];
    
    Com_DPrintf( "Resolving AutoUpdate Server... " );
    
    if ( !networkChainSystem->StringToAdr( servername, &cls.autoupdateServer, NA_IP ) )
    {
        Com_DPrintf( "Couldn't resolve address, trying next one..." );
        
        cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;
        
        return clientMainLocal.NextUpdateServer();
    }
    
    cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = true;
    
    cls.autoupdateServer.port = BigShort( PORT_SERVER );
    Com_DPrintf( "%i.%i.%i.%i:%i\n", cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                 cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                 BigShort( cls.autoupdateServer.port ) );
                 
    return true;
}

/*
=================
idClientMainSystemLocal::GetAutoUpdate
=================
*/
void idClientMainSystemLocal::GetAutoUpdate( void )
{
    // Don't try and get an update if we haven't checked for one
    if ( !autoupdateChecked )
    {
        return;
    }
    
    // Make sure there's a valid update file to request
    if ( ::strlen( cl_updatefiles->string ) < 5 )
    {
        return;
    }
    
    Com_DPrintf( "Connecting to auto-update server...\n" );
    
    soundSystem->StopAllSounds();
    
    // starting to load a map so we get out of full screen ui mode
    cvarSystem->Set( "r_uiFullScreen", "0" );
    
    // toggle on all the download related cvars
    // general flag
    cvarSystem->Set( "cl_allowDownload", "1" );
    // ftp/http support
    cvarSystem->Set( "cl_wwwDownload", "1" );
    
    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;
    
    if ( com_sv_running->integer )
    {
        // if running a local server, kill it
        serverInitSystem->Shutdown( "Server quit\n" );
    }
    
    // make sure a local server is killed
    cvarSystem->Set( "sv_killserver", "1" );
    serverMainSystem->Frame( 0 );
    
    Disconnect( true );
    Con_Close();
    
    Q_strncpyz( cls.servername, "Auto-Updater", sizeof( cls.servername ) );
    
    if ( cls.autoupdateServer.type == NA_BAD )
    {
        Com_Printf( "Bad server address\n" );
        cls.state = CA_DISCONNECTED;
        cvarSystem->Set( "ui_connecting", "0" );
        return;
    }
    
    // Copy auto-update server address to Server connect address
    ::memcpy( &clc.serverAddress, &cls.autoupdateServer, sizeof( netadr_t ) );
    
    Com_DPrintf( "%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
                 clc.serverAddress.ip[0], clc.serverAddress.ip[1],
                 clc.serverAddress.ip[2], clc.serverAddress.ip[3], BigShort( clc.serverAddress.port ) );
                 
    cls.state = CA_CONNECTING;
    
    cls.keyCatchers = 0;
    // CheckForResend() will fire immediately
    clc.connectTime = -99999;
    clc.connectPacketCount = 0;
    
    // server connection string
    cvarSystem->Set( "cl_currentServerAddress", "Auto-Updater" );
}

/*
============
idClientMainSystemLocal::RefMalloc
============
*/
void* idClientMainSystemLocal::RefMalloc( S32 size )
{
    return memorySystem->TagMalloc( size, TAG_RENDERER );
}

/*
============
idClientMainSystemLocal::RefTagFree
============
*/
void idClientMainSystemLocal::RefTagFree( void )
{
    memorySystem->FreeTags( TAG_RENDERER );
    return;
}

/*
====================
idClientMainSystemLocal::InitExportTable
====================
*/
S32 idClientMainSystemLocal::ScaledMilliseconds( void )
{
    return ( S32 )( idsystem->Milliseconds() * com_timescale->value );
}


/*
====================
idClientMainSystemLocal::InitExportTable
====================
*/
void idClientMainSystemLocal::InitExportTable( void )
{
    exports.Printf = Com_Printf;
    exports.Error = Com_Error;
    exports.collisionModelManager = collisionModelManager;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdBufferSystem = cmdBufferSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.UploadCinematic = CIN_UploadCinematic;
    exports.PlayCinematic = CIN_PlayCinematic;
    exports.RunCinematic = CIN_RunCinematic;
    exports.clientAVISystem = clientAVISystem;
    exports.clientMainSystem = clientMainSystem;
    exports.memorySystem = memorySystem;
}

/*
============
idClientMainSystemLocal::InitRef
============
*/
static void* rendererLib = nullptr;
void idClientMainSystemLocal::InitRef( void )
{
    Com_Printf( "----- Initializing Renderer ----\n" );
    UTF8 dllName[MAX_OSPATH];
    
    Q_snprintf( dllName, sizeof( dllName ), "renderSystem" ARCH_STRING DLL_EXT );
    
    Com_Printf( "Loading \"%s\"...\n", dllName );
    
    Com_Printf( "OK\n" );
    
    if ( ( rendererLib = SDL_LoadObject( dllName ) ) == 0 )
    {
        UTF8 fn[1024];
        
        Q_strncpyz( fn, idsystem->Cwd(), sizeof( fn ) );
        
        ::strncat( fn, "/", sizeof( fn ) - ( S32 )::strlen( fn ) - 1 );
        ::strncat( fn, dllName, sizeof( fn ) - ( S32 )::strlen( fn ) - 1 );
        
        Com_Printf( "Loading \"%s\"...", fn );
        
        if ( ( rendererLib = SDL_LoadObject( fn ) ) == 0 )
        {
            Com_Error( ERR_FATAL, "failed:\n\"%s\"", SDL_GetError() );
        }
    }
    
    // Get the entry point.
    rendererEntry = ( idRenderSystem * ( QDECL* )( rendererImports_t* ) )idsystem->GetProcAddress( rendererLib, "rendererEntry" );
    if ( !rendererEntry )
    {
        Com_Error( ERR_FATAL, "rendererEntry on RenderSystem failed.\n" );
    }
    
    // Init the export table.
    InitExportTable();
    
    renderSystem = rendererEntry( &exports );
    
    Com_Printf( "-------------------------------\n" );
    
    // unpause so the cgame definately gets a snapshot and renders a frame
    cvarSystem->Set( "cl_paused", "0" );
}

/*
============
idClientMainSystemLocal::ShutdownRef
============
*/
void idClientMainSystemLocal::ShutdownRef( void )
{
    renderSystem->Shutdown( true );
    
    if ( rendererLib )
    {
        idsystem->UnloadDll( rendererLib );
        rendererLib = nullptr;
    }
}

/*
=================
idClientMainSystemLocal::SaveTranslations_f
=================
*/
void idClientMainSystemLocal::SaveTranslations_f( void )
{
    SaveTransTable( "scripts/translation.lang", false );
}

/*
=================
idClientMainSystemLocal::SaveNewTranslations_f
=================
*/
void idClientMainSystemLocal::SaveNewTranslations_f( void )
{
    UTF8 fileName[512];
    
    if ( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "usage: SaveNewTranslations <filename>\n" );
        return;
    }
    
    ::strcpy( fileName, va( "translations/%s.lang", cmdSystem->Argv( 1 ) ) );
    
    SaveTransTable( fileName, true );
}

/*
=================
idClientMainSystemLocal::SaveNewTranslations_f
=================
*/
void idClientMainSystemLocal::LoadTranslations_f( void )
{
    ReloadTranslation();
}

/*
===============
idClientMainSystemLocal::GenerateQKey

test to see if a valid ETKEY_FILE exists.  If one does not, try to generate
it by filling it with 2048 bytes of random data.
===============
*/
void idClientMainSystemLocal::GenerateGUIDKey( void )
{
    S32 len = 0;
    U8 buff[GUIDKEY_SIZE];
    fileHandle_t f;
    
    len = fileSystem->SV_FOpenFileRead( GUIDKEY_FILE, &f );
    
    fileSystem->FCloseFile( f );
    
    if ( len == GUIDKEY_SIZE )
    {
        Com_Printf( "GUIDKEY found.\n" );
        return;
    }
    else
    {
        if ( len > 0 )
        {
            Com_Printf( "GUIDKEY file size != %d, regenerating\n", GUIDKEY_SIZE );
        }
        
        Com_Printf( "GUIDKEY building random string\n" );
        Com_RandomBytes( buff, sizeof( buff ) );
        
        f = fileSystem->SV_FOpenFileWrite( GUIDKEY_FILE );
        if ( !f )
        {
            Com_Printf( "GUIDKEY could not open %s for write\n", GUIDKEY_FILE );
            return;
        }
        
        fileSystem->Write( buff, sizeof( buff ), f );
        fileSystem->FCloseFile( f );
        
        Com_Printf( "GUIDKEY generated\n" );
    }
}

/*
==============
idClientMainSystemLocal::Userinfo_f
==============
*/
void idClientMainSystemLocal::Userinfo_f( void )
{
    //do nothing kthxbye
}

/*
====================
idClientMainSystemLocal::Init
====================
*/
void idClientMainSystemLocal::Init( void )
{
    Com_Printf( "----- idClientMainSystemLocal::Init -----\n" );
    
    Con_Init();
    
    if ( !com_fullyInitialized )
    {
        ClearState();
        
        // no longer CA_UNINITIALIZED
        cls.state = CA_DISCONNECTED;
    }
    
    cls.realtime = 0;
    
    CL_InitInput();
    
    // register our variables
    cl_noprint = cvarSystem->Get( "cl_noprint", "0", 0, "description" );
    cl_motd = cvarSystem->Get( "cl_motd", "1", 0, "description" );
    cl_autoupdate = cvarSystem->Get( "cl_autoupdate", "1", CVAR_ARCHIVE, "description" );
    
    cl_timeout = cvarSystem->Get( "cl_timeout", "200", 0, "description" );
    
    cl_wavefilerecord = cvarSystem->Get( "cl_wavefilerecord", "0", CVAR_TEMP, "description" );
    
    cl_timeNudge = cvarSystem->Get( "cl_timeNudge", "0", CVAR_TEMP, "description" );
    cl_shownet = cvarSystem->Get( "cl_shownet", "0", CVAR_TEMP, "description" );
    cl_shownuments = cvarSystem->Get( "cl_shownuments", "0", CVAR_TEMP, "description" );
    cl_visibleClients = cvarSystem->Get( "cl_visibleClients", "0", CVAR_TEMP, "description" );
    cl_showServerCommands = cvarSystem->Get( "cl_showServerCommands", "0", 0, "description" );
    cl_showSend = cvarSystem->Get( "cl_showSend", "0", CVAR_TEMP, "description" );
    cl_showTimeDelta = cvarSystem->Get( "cl_showTimeDelta", "0", CVAR_TEMP, "description" );
    cl_freezeDemo = cvarSystem->Get( "cl_freezeDemo", "0", CVAR_TEMP, "description" );
    rcon_client_password = cvarSystem->Get( "rconPassword", "", CVAR_TEMP, "description" );
    cl_activeAction = cvarSystem->Get( "activeAction", "", CVAR_TEMP, "description" );
    cl_autorecord = cvarSystem->Get( "cl_autorecord", "0", CVAR_TEMP, "description" );
    
    cl_timedemo = cvarSystem->Get( "timedemo", "0", 0, "description" );
    cl_forceavidemo = cvarSystem->Get( "cl_forceavidemo", "0", 0, "description" );
    cl_aviFrameRate = cvarSystem->Get( "cl_aviFrameRate", "25", CVAR_ARCHIVE, "description" );
    
    cl_aviMotionJpeg = cvarSystem->Get( "cl_aviMotionJpeg", "1", CVAR_ARCHIVE, "description" );
    
    rconAddress = cvarSystem->Get( "rconAddress", "", 0, "description" );
    
    cl_yawspeed = cvarSystem->Get( "cl_yawspeed", "140", CVAR_ARCHIVE, "description" );
    cl_pitchspeed = cvarSystem->Get( "cl_pitchspeed", "140", CVAR_ARCHIVE, "description" );
    cl_anglespeedkey = cvarSystem->Get( "cl_anglespeedkey", "1.5", 0, "description" );
    
    cl_maxpackets = cvarSystem->Get( "cl_maxpackets", "30", CVAR_ARCHIVE, "description" );
    cl_packetdup = cvarSystem->Get( "cl_packetdup", "1", CVAR_ARCHIVE, "description" );
    
    cl_run = cvarSystem->Get( "cl_run", "1", CVAR_ARCHIVE, "description" );
    cl_sensitivity = cvarSystem->Get( "sensitivity", "5", CVAR_ARCHIVE, "description" );
    cl_mouseAccel = cvarSystem->Get( "cl_mouseAccel", "0", CVAR_ARCHIVE, "description" );
    cl_freelook = cvarSystem->Get( "cl_freelook", "1", CVAR_ARCHIVE, "description" );
    
    cl_xbox360ControllerAvailable = cvarSystem->Get( "in_xbox360ControllerAvailable", "0", CVAR_ROM, "description" );
    
    // offset for the power function (for style 1, ignored otherwise)
    // this should be set to the max rate value
    cl_mouseAccelOffset = cvarSystem->Get( "cl_mouseAccelOffset", "5", CVAR_ARCHIVE, "description" );
    
    cl_showMouseRate = cvarSystem->Get( "cl_showmouserate", "0", 0, "description" );
    
    cl_allowDownload = cvarSystem->Get( "cl_allowDownload", "1", CVAR_ARCHIVE, "description" );
    cl_wwwDownload = cvarSystem->Get( "cl_wwwDownload", "1", CVAR_USERINFO | CVAR_ARCHIVE, "description" );
    
    cl_profile = cvarSystem->Get( "cl_profile", "", CVAR_ROM, "description" );
    cl_defaultProfile = cvarSystem->Get( "cl_defaultProfile", "", CVAR_ROM, "description" );
    
    // init autoswitch so the ui will have it correctly even
    // if the cgame hasn't been started
    // -NERVE - SMF - disabled autoswitch by default
    cvarSystem->Get( "cg_autoswitch", "0", CVAR_ARCHIVE, "description" );
    
    // Rafael - particle switch
    cvarSystem->Get( "cg_wolfparticles", "1", CVAR_ARCHIVE, "description" );
    // done
    
    cl_conXOffset = cvarSystem->Get( "cl_conXOffset", "3", 0, "description" );
    cl_inGameVideo = cvarSystem->Get( "r_inGameVideo", "1", CVAR_ARCHIVE, "description" );
    
    cl_serverStatusResendTime = cvarSystem->Get( "cl_serverStatusResendTime", "750", 0, "description" );
    
    // RF
    cl_recoilPitch = cvarSystem->Get( "cg_recoilPitch", "0", CVAR_ROM, "description" );
    
    cl_bypassMouseInput = cvarSystem->Get( "cl_bypassMouseInput", "0", 0, "description" );
    
    cl_doubletapdelay = cvarSystem->Get( "cl_doubletapdelay", "100", CVAR_ARCHIVE, "description" );
    
    m_pitch = cvarSystem->Get( "m_pitch", "0.022", CVAR_ARCHIVE, "description" );
    m_yaw = cvarSystem->Get( "m_yaw", "0.022", CVAR_ARCHIVE, "description" );
    m_forward = cvarSystem->Get( "m_forward", "0.25", CVAR_ARCHIVE, "description" );
    m_side = cvarSystem->Get( "m_side", "0.25", CVAR_ARCHIVE, "description" );
    m_filter = cvarSystem->Get( "m_filter", "0", CVAR_ARCHIVE, "description" );
    
    j_pitch = cvarSystem->Get( "j_pitch", "0.022", CVAR_ARCHIVE, "description" );
    j_yaw = cvarSystem->Get( "j_yaw", "-0.022", CVAR_ARCHIVE, "description" );
    j_forward = cvarSystem->Get( "j_forward", "-0.25", CVAR_ARCHIVE, "description" );
    j_side = cvarSystem->Get( "j_side", "0.25", CVAR_ARCHIVE, "description" );
    j_up = cvarSystem->Get( "j_up", "1", CVAR_ARCHIVE, "description" );
    j_pitch_axis = cvarSystem->Get( "j_pitch_axis", "3", CVAR_ARCHIVE, "description" );
    j_yaw_axis = cvarSystem->Get( "j_yaw_axis", "4", CVAR_ARCHIVE, "description" );
    j_forward_axis = cvarSystem->Get( "j_forward_axis", "1", CVAR_ARCHIVE, "description" );
    j_side_axis = cvarSystem->Get( "j_side_axis", "0", CVAR_ARCHIVE, "description" );
    j_up_axis = cvarSystem->Get( "j_up_axis", "2", CVAR_ARCHIVE, "description" );
    
    cvarSystem->CheckRange( j_pitch_axis, 0, MAX_JOYSTICK_AXIS - 1, true );
    cvarSystem->CheckRange( j_yaw_axis, 0, MAX_JOYSTICK_AXIS - 1, true );
    cvarSystem->CheckRange( j_forward_axis, 0, MAX_JOYSTICK_AXIS - 1, true );
    cvarSystem->CheckRange( j_side_axis, 0, MAX_JOYSTICK_AXIS - 1, true );
    cvarSystem->CheckRange( j_up_axis, 0, MAX_JOYSTICK_AXIS - 1, true );
    
    cl_motdString = cvarSystem->Get( "cl_motdString", "", CVAR_ROM, "description" );
    
    // ~ and `, as keys and characters
    cl_consoleKeys = cvarSystem->Get( "cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE, "description" );
    
    cl_consoleFont = cvarSystem->Get( "cl_consoleFont", "", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    cl_consoleFontSize = cvarSystem->Get( "cl_consoleFontSize", "16", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    cl_consoleFontKerning = cvarSystem->Get( "cl_consoleFontKerning", "0", CVAR_ARCHIVE, "description" );
    cl_consolePrompt = cvarSystem->Get( "cl_consolePrompt", "^3->", CVAR_ARCHIVE, "description" );
    
    cl_gamename = cvarSystem->Get( "cl_gamename", GAMENAME_FOR_MASTER, CVAR_TEMP, "description" );
    cl_altTab = cvarSystem->Get( "cl_altTab", "1", CVAR_ARCHIVE, "description" );
    
    //bani - make these cvars visible to cgame
    cl_demorecording = cvarSystem->Get( "cl_demorecording", "0", CVAR_ROM, "description" );
    cl_demofilename = cvarSystem->Get( "cl_demofilename", "", CVAR_ROM, "description" );
    cl_demooffset = cvarSystem->Get( "cl_demooffset", "0", CVAR_ROM, "description" );
    cl_waverecording = cvarSystem->Get( "cl_waverecording", "0", CVAR_ROM, "description" );
    cl_wavefilename = cvarSystem->Get( "cl_wavefilename", "", CVAR_ROM, "description" );
    cl_waveoffset = cvarSystem->Get( "cl_waveoffset", "0", CVAR_ROM, "description" );
    
    //bani
    cl_packetloss = cvarSystem->Get( "cl_packetloss", "0", CVAR_CHEAT, "description" );
    cl_packetdelay = cvarSystem->Get( "cl_packetdelay", "0", CVAR_CHEAT, "description" );
    
    cvarSystem->Get( "cl_maxPing", "800", CVAR_ARCHIVE, "description" );
    
    cl_guidServerUniq = cvarSystem->Get( "cl_guidServerUniq", "1", CVAR_ARCHIVE, "description" );
    
    // userinfo
    cvarSystem->Get( "name", idsystem->GetCurrentUser(), CVAR_USERINFO | CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "cg_version", PRODUCT_NAME, CVAR_ROM | CVAR_USERINFO, "description" );
    cvarSystem->Get( "password", "", CVAR_USERINFO, "description" );
    cvarSystem->Get( "cg_predictItems", "1", CVAR_ARCHIVE, "description" );
    cvarSystem->Get( "cg_autoactivate", "1", CVAR_ARCHIVE, "description" );
    
    // cgame might not be initialized before menu is used
    cvarSystem->Get( "cg_viewsize", "100", CVAR_ARCHIVE, "description" );
    
    cvarSystem->Get( "cg_autoReload", "1", CVAR_ARCHIVE, "description" );
    
    cl_missionStats = cvarSystem->Get( "g_missionStats", "0", CVAR_ROM, "description" );
    cl_waitForFire = cvarSystem->Get( "cl_waitForFire", "0", CVAR_ROM, "description" );
    
    // NERVE - SMF - localization
    cl_language = cvarSystem->Get( "cl_language", "0", CVAR_ARCHIVE, "description" );
    cl_debugTranslation = cvarSystem->Get( "cl_debugTranslation", "0", 0, "description" );
    // -NERVE - SMF
    
    // DHM - Nerve :: Auto-update
    cl_updateavailable = cvarSystem->Get( "cl_updateavailable", "0", CVAR_ROM, "description" );
    cl_updatefiles = cvarSystem->Get( "cl_updatefiles", "", CVAR_ROM, "description" );
    
    Q_strncpyz( cls.autoupdateServerNames[0], AUTOUPDATE_SERVER1_NAME, MAX_QPATH );
    Q_strncpyz( cls.autoupdateServerNames[1], AUTOUPDATE_SERVER2_NAME, MAX_QPATH );
    Q_strncpyz( cls.autoupdateServerNames[2], AUTOUPDATE_SERVER3_NAME, MAX_QPATH );
    Q_strncpyz( cls.autoupdateServerNames[3], AUTOUPDATE_SERVER4_NAME, MAX_QPATH );
    Q_strncpyz( cls.autoupdateServerNames[4], AUTOUPDATE_SERVER5_NAME, MAX_QPATH );
    // DHM - Nerve
    
    //
    // register our commands
    //
    cmdSystem->AddCommand( "cmd", &idClientMainSystemLocal::ForwardToServer_f, "description" );
    cmdSystem->AddCommand( "configstrings", &idClientMainSystemLocal::Configstrings_f, "description" );
    cmdSystem->AddCommand( "clientinfo", &idClientMainSystemLocal::Clientinfo_f, "description" );
    cmdSystem->AddCommand( "snd_reload", &idClientMainSystemLocal::Snd_Reload_f, "description" );
    cmdSystem->AddCommand( "snd_restart", &idClientMainSystemLocal::Snd_Restart_f, "description" );
    cmdSystem->AddCommand( "vid_restart", &idClientMainSystemLocal::Vid_Restart_f, "description" );
    cmdSystem->AddCommand( "ui_restart", &idClientMainSystemLocal::UI_Restart_f, "description" );
    cmdSystem->AddCommand( "disconnect", &idClientMainSystemLocal::Disconnect_f, "description" );
    cmdSystem->AddCommand( "record", &idClientMainSystemLocal::Record_f, "description" );
    cmdSystem->AddCommand( "demo", &idClientMainSystemLocal::PlayDemo_f, "description" );
    cmdSystem->SetCommandCompletionFunc( "demo", &idClientMainSystemLocal::CompleteDemoName );
    cmdSystem->AddCommand( "cinematic", CL_PlayCinematic_f, "description" );
    cmdSystem->AddCommand( "stoprecord", &idClientMainSystemLocal::StopRecord_f, "description" );
    cmdSystem->AddCommand( "connect", &idClientMainSystemLocal::Connect_f, "description" );
    cmdSystem->AddCommand( "reconnect", &idClientMainSystemLocal::Reconnect_f, "description" );
    cmdSystem->AddCommand( "localservers", &idClientBrowserSystemLocal::LocalServers, "description" );
    cmdSystem->AddCommand( "globalservers", &idClientBrowserSystemLocal::GlobalServers, "description" );
    cmdSystem->AddCommand( "openurl", &idClientMainSystemLocal::OpenUrl_f, "description" );
    cmdSystem->AddCommand( "rcon", &idClientMainSystemLocal::Rcon_f, "description" );
    cmdSystem->AddCommand( "setenv", &idClientMainSystemLocal::Setenv_f, "description" );
    cmdSystem->AddCommand( "ping", &idClientBrowserSystemLocal::Ping, "description" );
    cmdSystem->AddCommand( "serverstatus", &idClientBrowserSystemLocal::ServerStatus, "description" );
    cmdSystem->AddCommand( "showip", &idClientBrowserSystemLocal::ShowIP, "description" );
    cmdSystem->AddCommand( "fs_openedList", &idClientMainSystemLocal::OpenedPK3List_f, "description" );
    cmdSystem->AddCommand( "fs_referencedList", &idClientMainSystemLocal::ReferencedPK3List_f, "description" );
    
    // Ridah, startup-caching system
    cmdSystem->AddCommand( "cache_startgather", &idClientMainSystemLocal::Cache_StartGather_f, "description" );
    cmdSystem->AddCommand( "cache_usedfile", &idClientMainSystemLocal::Cache_UsedFile_f, "description" );
    cmdSystem->AddCommand( "cache_setindex", &idClientMainSystemLocal::Cache_SetIndex_f, "description" );
    cmdSystem->AddCommand( "cache_mapchange", &idClientMainSystemLocal::Cache_MapChange_f, "description" );
    cmdSystem->AddCommand( "cache_endgather", &idClientMainSystemLocal::Cache_EndGather_f, "description" );
    
    cmdSystem->AddCommand( "updatehunkusage", &idClientGameSystemLocal::UpdateLevelHunkUsage, "Update hunk memory usage" );
    // done.
    
    cmdSystem->AddCommand( "SaveTranslations", &idClientMainSystemLocal::SaveTranslations_f, "description" );
    cmdSystem->AddCommand( "SaveNewTranslations", &idClientMainSystemLocal::SaveNewTranslations_f, "description" );
    cmdSystem->AddCommand( "LoadTranslations", &idClientMainSystemLocal::LoadTranslations_f, "description" );
    
    cmdSystem->AddCommand( "setRecommended", &idClientMainSystemLocal::SetRecommended_f, "description" );
    cmdSystem->AddCommand( "userinfo", &idClientMainSystemLocal::Userinfo_f, "description" );
    cmdSystem->AddCommand( "wav_record", &idClientMainSystemLocal::WavRecord_f, "description" );
    cmdSystem->AddCommand( "wav_stoprecord", &idClientMainSystemLocal::WavStopRecord_f, "description" );
    
    cmdSystem->AddCommand( "video", &idClientMainSystemLocal::Video_f, "description" );
    cmdSystem->AddCommand( "stopvideo", &idClientMainSystemLocal::StopVideo_f, "description" );
    
    InitRef();
    
    idClientScreenSystemLocal::Init();
    
    cmdBufferSystem->Execute();
    
    cvarSystem->Set( "cl_running", "1" );
    
    GenerateGUIDKey();
    cvarSystem->Get( "cl_guid", "", CVAR_USERINFO | CVAR_ROM, "description" );
    UpdateGUID( nullptr, 0 );
    
    // DHM - Nerve
    autoupdateChecked = false;
    autoupdateStarted = false;
    
    InitTranslation(); // NERVE - SMF - localization
    
    Com_Printf( "----- Client Initialization Complete -----\n" );
}


/*
===============
idClientMainSystemLocal::Shutdown
===============
*/
void idClientMainSystemLocal::Shutdown( void )
{
    static bool recursive = false;
    
    // check whether the client is running at all.
    if ( !( com_cl_running && com_cl_running->integer ) )
    {
        return;
    }
    
    Com_Printf( "----- idClientMainSystemLocal::Shutdown -----\n" );
    
    if ( recursive )
    {
        printf( "recursive shutdown\n" );
        return;
    }
    recursive = true;
    
    // write wav header when we quit
    if ( clc.waverecording )
    {
        WavStopRecord_f();
    }
    
    Disconnect( true );
    
    downloadSystem->Shutdown();
    ShutdownRef();
    
    clientGUISystem->ShutdownGUI();
    
    soundSystem->Shutdown();
    
    cmdSystem->RemoveCommand( "cmd" );
    cmdSystem->RemoveCommand( "configstrings" );
    cmdSystem->RemoveCommand( "userinfo" );
    cmdSystem->RemoveCommand( "snd_reload" );
    cmdSystem->RemoveCommand( "snd_restart" );
    cmdSystem->RemoveCommand( "vid_restart" );
    cmdSystem->RemoveCommand( "disconnect" );
    cmdSystem->RemoveCommand( "record" );
    cmdSystem->RemoveCommand( "demo" );
    cmdSystem->RemoveCommand( "cinematic" );
    cmdSystem->RemoveCommand( "stoprecord" );
    cmdSystem->RemoveCommand( "connect" );
    cmdSystem->RemoveCommand( "reconnect" );
    cmdSystem->RemoveCommand( "localservers" );
    cmdSystem->RemoveCommand( "globalservers" );
    cmdSystem->RemoveCommand( "rcon" );
    cmdSystem->RemoveCommand( "setenv" );
    cmdSystem->RemoveCommand( "ping" );
    cmdSystem->RemoveCommand( "serverstatus" );
    cmdSystem->RemoveCommand( "showip" );
    cmdSystem->RemoveCommand( "model" );
    
    // Ridah, startup-caching system
    cmdSystem->RemoveCommand( "cache_startgather" );
    cmdSystem->RemoveCommand( "cache_usedfile" );
    cmdSystem->RemoveCommand( "cache_setindex" );
    cmdSystem->RemoveCommand( "cache_mapchange" );
    cmdSystem->RemoveCommand( "cache_endgather" );
    
    cmdSystem->RemoveCommand( "updatehunkusage" );
    cmdSystem->RemoveCommand( "wav_record" );
    cmdSystem->RemoveCommand( "wav_stoprecord" );
    // done.
    
    consoleCursesSystem->Shutdown();
    
    cvarSystem->Set( "cl_running", "0" );
    
    recursive = false;
    
    ::memset( &cls, 0, sizeof( cls ) );
    clientGUISystem->SetCatcher( 0 );
    
    Com_Printf( "-----------------------\n" );
}

/*
=======================
idClientMainSystemLocal::AllocTrans
=======================
*/
trans_t* idClientMainSystemLocal::AllocTrans( UTF8* original, UTF8* translated[MAX_LANGUAGES] )
{
    S32 i;
    trans_t* t;
    
    t = ( trans_t* )::malloc( sizeof( trans_t ) );
    ::memset( t, 0, sizeof( trans_t ) );
    
    if ( original )
    {
        ::strncpy( t->original, original, MAX_TRANS_STRING );
    }
    
    if ( translated )
    {
        for ( i = 0; i < MAX_LANGUAGES; i++ )
        {
            ::strncpy( t->translated[i], translated[i], MAX_TRANS_STRING );
        }
    }
    
    return t;
}

/*
=======================
idClientMainSystemLocal::generateHashValue
=======================
*/
S64 idClientMainSystemLocal::generateHashValue( StringEntry fname )
{
    S32 i;
    S64 hash;
    UTF8 letter;
    
    hash = 0;
    i = 0;
    
    while ( fname[i] != '\0' )
    {
        letter = tolower( fname[i] );
        hash += ( S64 )( letter ) * ( i + 119 );
        i++;
    }
    
    hash &= ( FILE_HASH_SIZE - 1 );
    
    return hash;
}

/*
=======================
idClientMainSystemLocal::LookupTrans
=======================
*/
trans_t* idClientMainSystemLocal::LookupTrans( UTF8* original, UTF8* translated[MAX_LANGUAGES], bool isLoading )
{
    S64 hash;
    trans_t* t, *newt, *prev = nullptr;
    
    hash = generateHashValue( original );
    
    for ( t = transTable[hash]; t; prev = t, t = t->next )
    {
        if ( !Q_stricmp( original, t->original ) )
        {
            if ( isLoading )
            {
                Com_DPrintf( S_COLOR_YELLOW "WARNING: Duplicate string found: \"%s\"\n", original );
            }
            
            return t;
        }
    }
    
    newt = AllocTrans( original, translated );
    
    if ( prev )
    {
        prev->next = newt;
    }
    else
    {
        transTable[hash] = newt;
    }
    
    if ( cl_debugTranslation->integer >= 1 && !isLoading )
    {
        Com_Printf( "Missing translation: \'%s\'\n", original );
    }
    
    // see if we want to save out the translation table everytime a string is added
    if ( cl_debugTranslation->integer == 2 && !isLoading )
    {
        SaveTransTable( "new", true );
    }
    
    return newt;
}

/*
=======================
idClientMainSystemLocal::SaveTransTable
=======================
*/
void idClientMainSystemLocal::SaveTransTable( StringEntry fileName, bool newOnly )
{
    S32 bucketlen, bucketnum, maxbucketlen, avebucketlen, untransnum, transnum, i, j, len;
    fileHandle_t f;
    trans_t* t;
    StringEntry buf;
    
    if ( cl.corruptedTranslationFile )
    {
        Com_Printf( S_COLOR_YELLOW "WARNING: Cannot save corrupted translation file. Please reload first." );
        return;
    }
    
    fileSystem->FOpenFileByMode( fileName, &f, FS_WRITE );
    
    bucketnum = 0;
    maxbucketlen = 0;
    avebucketlen = 0;
    transnum = 0;
    untransnum = 0;
    
    // write out version, if one
    if ( strlen( cl.translationVersion ) )
    {
        buf = va( "#version\t\t\"%s\"\n", cl.translationVersion );
    }
    else
    {
        buf = va( "#version\t\t\"1.0 01/01/01\"\n" );
    }
    
    len = ( S32 )::strlen( buf );
    
    fileSystem->Write( buf, len, f );
    
    // write out translated strings
    for ( j = 0; j < 2; j++ )
    {
        for ( i = 0; i < FILE_HASH_SIZE; i++ )
        {
            t = transTable[i];
            
            if ( !t || ( newOnly && t->fromFile ) )
            {
                continue;
            }
            
            bucketlen = 0;
            
            for ( ; t; t = t->next )
            {
                bucketlen++;
                
                if ( ::strlen( t->translated[0] ) )
                {
                    if ( j )
                    {
                        continue;
                    }
                    transnum++;
                }
                else
                {
                    if ( !j )
                    {
                        continue;
                    }
                    untransnum++;
                }
                
                buf = va( "{\n\tenglish\t\t\"%s\"\n", t->original );
                len = ( S32 )::strlen( buf );
                fileSystem->Write( buf, len, f );
                
                buf = va( "\tfrench\t\t\"%s\"\n", t->translated[LANGUAGE_FRENCH] );
                len = ( S32 )::strlen( buf );
                fileSystem->Write( buf, len, f );
                
                buf = va( "\tgerman\t\t\"%s\"\n", t->translated[LANGUAGE_GERMAN] );
                len = ( S32 )::strlen( buf );
                fileSystem->Write( buf, len, f );
                
                buf = va( "\titalian\t\t\"%s\"\n", t->translated[LANGUAGE_ITALIAN] );
                len = ( S32 )::strlen( buf );
                fileSystem->Write( buf, len, f );
                
                buf = va( "\tspanish\t\t\"%s\"\n", t->translated[LANGUAGE_SPANISH] );
                len = ( S32 )::strlen( buf );
                fileSystem->Write( buf, len, f );
                
                buf = "}\n";
                len = ( S32 )::strlen( buf );
                fileSystem->Write( buf, len, f );
            }
            
            if ( bucketlen > maxbucketlen )
            {
                maxbucketlen = bucketlen;
            }
            
            if ( bucketlen )
            {
                bucketnum++;
                avebucketlen += bucketlen;
            }
        }
    }
    
    Com_Printf( "Saved translation table.\nTotal = %i, Translated = %i, Untranslated = %i, aveblen = %2.2f, maxblen = %i\n",
                transnum + untransnum, transnum, untransnum, ( F32 )avebucketlen / bucketnum, maxbucketlen );
                
    fileSystem->FCloseFile( f );
}

/*
=======================
idClientMainSystemLocal::CheckTranslationString

NERVE - SMF - compare formatting UTF8acters
=======================
*/
bool idClientMainSystemLocal::CheckTranslationString( UTF8* original, UTF8* translated )
{
    S32 len, i;
    UTF8 format_org[128], format_trans[128];
    
    ::memset( format_org, 0, 128 );
    ::memset( format_trans, 0, 128 );
    
    // generate formatting string for original
    len = ( S32 )::strlen( original );
    
    for ( i = 0; i < len; i++ )
    {
        if ( original[i] != '%' )
        {
            continue;
        }
        
        strcat( format_org, va( "%c%c ", '%', original[i + 1] ) );
    }
    
    // generate formatting string for translated
    len = ( S32 )::strlen( translated );
    if ( !len )
    {
        return true;
    }
    
    for ( i = 0; i < len; i++ )
    {
        if ( translated[i] != '%' )
        {
            continue;
        }
        
        strcat( format_trans, va( "%c%c ", '%', translated[i + 1] ) );
    }
    
    // compare
    len = ( S32 )::strlen( format_org );
    
    if ( len != ::strlen( format_trans ) )
    {
        return false;
    }
    
    for ( i = 0; i < len; i++ )
    {
        if ( format_org[i] != format_trans[i] )
        {
            return false;
        }
    }
    
    return true;
}

/*
=======================
idClientMainSystemLocal::LoadTransTable
=======================
*/
void idClientMainSystemLocal::LoadTransTable( StringEntry fileName )
{
    S32 len, i, count;
    UTF8 translated[MAX_LANGUAGES][MAX_VA_STRING], original[MAX_VA_STRING], * text, * text_p, * token;
    fileHandle_t f;
    trans_t* t;
    bool aborted;
    
    count = 0;
    aborted = false;
    cl.corruptedTranslationFile = false;
    
    len = fileSystem->FOpenFileByMode( fileName, &f, FS_READ );
    if ( len <= 0 )
    {
        return;
    }
    
    // Gordon: shouldn't this be a memorySystem->Malloc or something?
    text = ( UTF8* )malloc( len + 1 );
    if ( !text )
    {
        return;
    }
    
    fileSystem->Read( text, len, f );
    text[len] = 0;
    fileSystem->FCloseFile( f );
    
    // parse the text
    text_p = text;
    
    do
    {
        token = COM_Parse( &text_p );
        if ( Q_stricmp( "{", token ) )
        {
            // parse version number
            if ( !Q_stricmp( "#version", token ) )
            {
                token = COM_Parse( &text_p );
                ::strcpy( cl.translationVersion, token );
                continue;
            }
            
            break;
        }
        
        // english
        token = COM_Parse( &text_p );
        if ( Q_stricmp( "english", token ) )
        {
            aborted = true;
            break;
        }
        
        token = COM_Parse( &text_p );
        ::strcpy( original, token );
        
        if ( cl_debugTranslation->integer == 3 )
        {
            Com_Printf( "%i Loading: \"%s\"\n", count, original );
        }
        
        // french
        token = COM_Parse( &text_p );
        if ( Q_stricmp( "french", token ) )
        {
            aborted = true;
            break;
        }
        
        token = COM_Parse( &text_p );
        ::strcpy( translated[LANGUAGE_FRENCH], token );
        if ( !CheckTranslationString( original, translated[LANGUAGE_FRENCH] ) )
        {
            Com_Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
            aborted = true;
            break;
        }
        
        // german
        token = COM_Parse( &text_p );
        if ( Q_stricmp( "german", token ) )
        {
            aborted = true;
            break;
        }
        
        token = COM_Parse( &text_p );
        ::strcpy( translated[LANGUAGE_GERMAN], token );
        if ( !CheckTranslationString( original, translated[LANGUAGE_GERMAN] ) )
        {
            Com_Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
            aborted = true;
            break;
        }
        
        // italian
        token = COM_Parse( &text_p );
        if ( Q_stricmp( "italian", token ) )
        {
            aborted = true;
            break;
        }
        
        token = COM_Parse( &text_p );
        ::strcpy( translated[LANGUAGE_ITALIAN], token );
        if ( !CheckTranslationString( original, translated[LANGUAGE_ITALIAN] ) )
        {
            Com_Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
            aborted = true;
            break;
        }
        
        // spanish
        token = COM_Parse( &text_p );
        if ( Q_stricmp( "spanish", token ) )
        {
            aborted = true;
            break;
        }
        
        token = COM_Parse( &text_p );
        ::strcpy( translated[LANGUAGE_SPANISH], token );
        if ( !CheckTranslationString( original, translated[LANGUAGE_SPANISH] ) )
        {
            Com_Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
            aborted = true;
            break;
        }
        
        // do lookup
        t = LookupTrans( original, nullptr, true );
        
        if ( t )
        {
            t->fromFile = true;
            
            for ( i = 0; i < MAX_LANGUAGES; i++ )
            {
                ::strncpy( t->translated[i], translated[i], MAX_TRANS_STRING );
            }
        }
        
        token = COM_Parse( &text_p );
        
        // set offset if we have one
        if ( !Q_stricmp( "offset", token ) )
        {
            token = COM_Parse( &text_p );
            t->x_offset = ( F32 )::atof( token );
            
            token = COM_Parse( &text_p );
            t->y_offset = ( F32 )::atof( token );
            
            token = COM_Parse( &text_p );
        }
        
        if ( Q_stricmp( "}", token ) )
        {
            aborted = true;
            break;
        }
        
        count++;
    }
    while ( token );
    
    if ( aborted )
    {
        S32 i, line = 1;
        
        for ( i = 0; i < len && ( text + i ) < text_p; i++ )
        {
            if ( text[i] == '\n' )
            {
                line++;
            }
        }
        
        Com_Printf( S_COLOR_YELLOW "WARNING: Problem loading %s on line %i\n", fileName, line );
        cl.corruptedTranslationFile = true;
    }
    else
    {
        Com_Printf( "Loaded %i translation strings from %s\n", count, fileName );
    }
    
    // cleanup
    free( text );
}

/*
=======================
idClientMainSystemLocal::ReloadTranslation
=======================
*/
void idClientMainSystemLocal::ReloadTranslation( void )
{
    S32 numFiles, i;
    UTF8** fileList;
    
    for ( i = 0; i < FILE_HASH_SIZE; i++ )
    {
        if ( transTable[i] )
        {
            free( transTable[i] );
        }
    }
    
    memset( transTable, 0, sizeof( trans_t* ) * FILE_HASH_SIZE );
    LoadTransTable( "scripts/translation.lang" );
    
    fileList = fileSystem->ListFiles( "translations", ".lang", &numFiles );
    
    for ( i = 0; i < numFiles; i++ )
    {
        LoadTransTable( va( "translations/%s", fileList[i] ) );
    }
}

/*
=======================
idClientMainSystemLocal::InitTranslation
=======================
*/
void idClientMainSystemLocal::InitTranslation( void )
{
    S32 numFiles, i;
    UTF8** fileList;
    
    ::memset( transTable, 0, sizeof( trans_t* ) * FILE_HASH_SIZE );
    LoadTransTable( "scripts/translation.lang" );
    
    fileList = fileSystem->ListFiles( "translations", ".lang", &numFiles );
    
    for ( i = 0; i < numFiles; i++ )
    {
        LoadTransTable( va( "translations/%s", fileList[i] ) );
    }
}

/*
=======================
idClientMainSystemLocal::TranslateString
=======================
*/
void idClientMainSystemLocal::TranslateString( StringEntry string, UTF8* dest_buffer )
{
    S32 i, count, currentLanguage;
    UTF8* buf;
    trans_t* t;
    bool newline = false;
    
    buf = dest_buffer;
    currentLanguage = cl_language->integer - 1;
    
    // early bail if we only want english or bad language type
    if ( !string )
    {
        ::strcpy( buf, "(null)" );
        return;
    }
    else if ( currentLanguage < 0 || currentLanguage >= MAX_LANGUAGES || !::strlen( string ) )
    {
        ::strcpy( buf, string );
        return;
    }
    
    // ignore newlines
    if ( string[::strlen( string ) - 1] == '\n' )
    {
        newline = true;
    }
    
    for ( i = 0, count = 0; string[i] != '\0'; i++ )
    {
        if ( string[i] != '\n' )
        {
            buf[count++] = string[i];
        }
    }
    buf[count] = '\0';
    
    t = LookupTrans( buf, nullptr, false );
    
    if ( t && ::strlen( t->translated[currentLanguage] ) )
    {
        S32 offset = 0;
        
        if ( cl_debugTranslation->integer >= 1 )
        {
            buf[0] = '^';
            buf[1] = '1';
            buf[2] = '[';
            offset = 3;
        }
        
        ::strcpy( buf + offset, t->translated[currentLanguage] );
        
        if ( cl_debugTranslation->integer >= 1 )
        {
            S32 len2 = ( S32 )::strlen( buf );
            
            buf[len2] = ']';
            buf[len2 + 1] = '^';
            buf[len2 + 2] = '7';
            buf[len2 + 3] = '\0';
        }
        
        if ( newline )
        {
            S32 len2 = ( S32 )::strlen( buf );
            
            buf[len2] = '\n';
            buf[len2 + 1] = '\0';
        }
    }
    else
    {
        S32 offset = 0;
        
        if ( cl_debugTranslation->integer >= 1 )
        {
            buf[0] = '^';
            buf[1] = '1';
            buf[2] = '[';
            offset = 3;
        }
        
        ::strcpy( buf + offset, string );
        
        if ( cl_debugTranslation->integer >= 1 )
        {
            S32 len2 = ( S32 )::strlen( buf );
            bool addnewline = false;
            
            if ( buf[len2 - 1] == '\n' )
            {
                len2--;
                addnewline = true;
            }
            
            buf[len2] = ']';
            buf[len2 + 1] = '^';
            buf[len2 + 2] = '7';
            buf[len2 + 3] = '\0';
            
            if ( addnewline )
            {
                buf[len2 + 3] = '\n';
                buf[len2 + 4] = '\0';
            }
        }
    }
}

/*
=======================
idClientMainSystemLocal::TranslateStringBuf

TTimo - handy, stores in a static buf, converts \n to chr(13)
=======================
*/
StringEntry idClientMainSystemLocal::TranslateStringBuf( StringEntry string )
{
    S32 i, l;
    UTF8* p;
    static UTF8 buf[MAX_VA_STRING];
    
    clientMainLocal.TranslateString( string, buf );
    
    while ( ( p = strstr( buf, "\\n" ) ) != nullptr )
    {
        *p = '\n';
        p++;
        
        // ::memcpy(p, p+1, strlen(p) ); b0rks on win32
        l = ( S32 )::strlen( p );
        
        for ( i = 0; i < l; i++ )
        {
            *p = *( p + 1 );
            p++;
        }
    }
    
    return buf;
}

/*
=======================
idClientMainSystemLocal::OpenURLForCvar
=======================
*/
void idClientMainSystemLocal::OpenURL( StringEntry url )
{
    if ( !url || !strlen( url ) )
    {
        Com_Printf( "%s", TranslateStringBuf( "invalid/empty URL\n" ) );
        return;
    }
    
    idsystem->OpenURL( url, true );
}

/*
==================
BotImport_DrawPolygon
==================
*/
void idClientMainSystemLocal::BotImport_DrawPolygon( S32 color, S32 numpoints, F32* points )
{
    //renderSystem->DrawDebugPolygon( color, numpoints, points );
}

/*
===================
idClientMainSystemLocal::UpdateInfoPacket
===================
*/
void idClientMainSystemLocal::UpdateInfoPacket( netadr_t from )
{

    if ( cls.autoupdateServer.type == NA_BAD )
    {
        Com_DPrintf( "idClientMainSystemLocal::UpdateInfoPacket:  Auto-Updater has bad address\n" );
        return;
    }
    
    Com_DPrintf( "Auto-Updater resolved to %i.%i.%i.%i:%i\n",
                 cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
                 cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
                 BigShort( cls.autoupdateServer.port ) );
                 
    if ( !networkSystem->CompareAdr( from, cls.autoupdateServer ) )
    {
        Com_DPrintf( "idClientMainSystemLocal::UpdateInfoPacket:  Received packet from %i.%i.%i.%i:%i\n",
                     from.ip[0], from.ip[1], from.ip[2], from.ip[3],
                     BigShort( from.port ) );
        return;
    }
    
    cvarSystem->Set( "cl_updateavailable", cmdSystem->Argv( 1 ) );
    
    if ( !Q_stricmp( cl_updateavailable->string, "1" ) )
    {
        cvarSystem->Set( "cl_updatefiles", cmdSystem->Argv( 2 ) );
        uiManager->SetActiveMenu( UIMENU_WM_AUTOUPDATE );
    }
}

/*
=======================
idClientMainSystemLocal::AddToLimboChat
=======================
*/
void idClientMainSystemLocal::AddToLimboChat( StringEntry str )
{
    S32 i, len, lastcolor, chatHeight;
    UTF8* p, * ls;
    
    chatHeight = LIMBOCHAT_HEIGHT;
    cl.limboChatPos = LIMBOCHAT_HEIGHT - 1;
    len = 0;
    
    // copy old strings
    for ( i = cl.limboChatPos; i > 0; i-- )
    {
        ::strcpy( cl.limboChatMsgs[i], cl.limboChatMsgs[i - 1] );
    }
    
    // copy new string
    p = cl.limboChatMsgs[0];
    *p = 0;
    
    lastcolor = '7';
    
    ls = nullptr;
    
    while ( *str )
    {
        if ( len > LIMBOCHAT_WIDTH - 1 )
        {
            break;
        }
        
        if ( Q_IsColorString( str ) )
        {
            *p++ = *str++;
            lastcolor = *str;
            *p++ = *str++;
            continue;
        }
        
        if ( *str == ' ' )
        {
            ls = p;
        }
        
        *p++ = *str++;
        len++;
    }
    
    *p = 0;
}

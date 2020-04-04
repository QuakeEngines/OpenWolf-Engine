////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cl_ui.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

void* uivm;

idUserInterfaceManager* uiManager;
idUserInterfaceManager* ( *dllEntry )( guiImports_t* guimports );

static guiImports_t exports;

idClientGUISystemLocal clientGUILocal;
idClientGUISystem* clientGUISystem = &clientGUILocal;

/*
===============
idClientGUISystemLocal::idClientGUISystemLocal
===============
*/
idClientGUISystemLocal::idClientGUISystemLocal( void )
{
}

/*
===============
idClientGUISystemLocal::~idClientGUISystemLocal
===============
*/
idClientGUISystemLocal::~idClientGUISystemLocal( void )
{
}

/*
====================
idClientGUISystemLocal::GetClientState
====================
*/
void idClientGUISystemLocal::GetClientState( uiClientState_t* state )
{
    state->connectPacketCount = clc.connectPacketCount;
    state->connState = cls.state;
    Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
    Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
    Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
    state->clientNum = cl.snap.ps.clientNum;
}

/*
====================
idClientGUISystemLocal::GetNews
====================
*/
bool idClientGUISystemLocal::GetNews( bool begin )
{
    bool finished = false;
    S32 readSize;
    static UTF8 newsFile[MAX_QPATH] = "";
    
    if ( !newsFile[0] )
    {
        Q_strncpyz( newsFile, fileSystem->BuildOSPath( cvarSystem->VariableString( "fs_homepath" ), "", "news.dat" ), MAX_QPATH );
        newsFile[MAX_QPATH - 1] = 0;
    }
    
    if ( begin )  // if not already using curl, start the download
    {
        if ( !clc.bWWWDl )
        {
            clc.bWWWDl = true;
            downloadSystem->BeginDownload( newsFile, "http://tremulous.net/clientnews.txt", com_developer->integer );
            cls.bWWWDlDisconnected = true;
            return false;
        }
    }
    
    if ( fileSystem->SV_FOpenFileRead( newsFile, &clc.download ) )
    {
        readSize = fileSystem->Read( clc.newsString, sizeof( clc.newsString ), clc.download );
        clc.newsString[ readSize ] = '\0';
        if ( readSize > 0 )
        {
            finished = true;
            clc.bWWWDl = false;
            cls.bWWWDlDisconnected = false;
        }
    }
    
    fileSystem->FCloseFile( clc.download );
    
    if ( !finished )
    {
        ::strcpy( clc.newsString, "Retrieving..." );
    }
    
    cvarSystem->Set( "cl_newsString", clc.newsString );
    return finished;
}

/*
====================
idClientGUISystemLocal::GetGlConfig
====================
*/
void idClientGUISystemLocal::GetGlconfig( vidconfig_t* config )
{
    *config = cls.glconfig;
}

/*
====================
idClientGUISystemLocal::GUIGetClipboarzdData
====================
*/
void idClientGUISystemLocal::GUIGetClipboardData( UTF8* buf, S32 buflen )
{
    UTF8* cbd;
    
    cbd = idsystem->SysGetClipboardData();
    
    if ( !cbd )
    {
        *buf = 0;
        return;
    }
    
    Q_strncpyz( buf, cbd, buflen );
    
    memorySystem->Free( cbd );
}

/*
====================
idClientGUISystemLocal::KeynumToStringBuf
====================
*/
void idClientGUISystemLocal::KeynumToStringBuf( S32 keynum, UTF8* buf, S32 buflen )
{
    Q_strncpyz( buf, clientKeysSystem->Key_KeynumToString( keynum ), buflen );
}

/*
====================
idClientGUISystemLocal::GetBindingBuf
====================
*/
void idClientGUISystemLocal::GetBindingBuf( S32 keynum, UTF8* buf, S32 buflen )
{
    UTF8* value;
    
    value = idClientKeysSystemLocal::Key_GetBinding( keynum );
    if ( value )
    {
        Q_strncpyz( buf, value, buflen );
    }
    else
    {
        *buf = 0;
    }
}

/*
====================
idClientGUISystemLocal::GetCatcher
====================
*/
S32 idClientGUISystemLocal::GetCatcher( void )
{
    return cls.keyCatchers;
}

/*
====================
idClientGUISystemLocal::SetCatcher
====================
*/
void idClientGUISystemLocal::SetCatcher( S32 catcher )
{
    // console overrides everything
    if ( cls.keyCatchers & KEYCATCH_CONSOLE )
    {
        cls.keyCatchers = catcher | KEYCATCH_CONSOLE;
    }
    else
    {
        cls.keyCatchers = catcher;
    }
    
}

/*
====================
idClientGUISystemLocal::GetConfigString
====================
*/
S32 idClientGUISystemLocal::GetConfigString( S32 index, UTF8* buf, S32 size )
{
    S32 offset;
    
    if ( index < 0 || index >= MAX_CONFIGSTRINGS )
    {
        return false;
    }
    
    offset = cl.gameState.stringOffsets[index];
    
    if ( !offset )
    {
        if ( size )
        {
            buf[0] = 0;
        }
        return false;
    }
    
    Q_strncpyz( buf, cl.gameState.stringData + offset, size );
    
    return true;
}

/*
====================
idClientGUISystemLocal::ShutdownGUI
====================
*/
void idClientGUISystemLocal::ShutdownGUI( void )
{
    cls.keyCatchers &= ~KEYCATCH_UI;
    cls.uiStarted = false;
    
    if ( uiManager == nullptr || uivm == nullptr )
    {
        return;
    }
    
    uiManager->Shutdown();
    uiManager = nullptr;
    
    idsystem->UnloadDll( uivm );
    uivm = nullptr;
}

/*
====================
idClientGUISystemLocal::CreateExportTable
====================
*/
void idClientGUISystemLocal::CreateExportTable( void )
{
    exports.Print = Com_Printf;
    exports.Error = Com_Error;
    
    exports.RealTime = Com_RealTime;
    exports.RealTime = Com_RealTime;
    exports.PlayCinematic = CIN_PlayCinematic;
    exports.StopCinematic = CIN_StopCinematic;
    exports.RunCinematic = CIN_RunCinematic;
    exports.DrawCinematic = CIN_DrawCinematic;
    exports.SetExtents = CIN_SetExtents;
    
    exports.renderSystem = renderSystem;
    exports.soundSystem = soundSystem;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdBufferSystem = cmdBufferSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.idcgame = cgame;
    exports.idLANSystem = clientLANSystem;
    exports.idGUISystem = clientGUISystem;
    exports.clientScreenSystem = clientScreenSystem;
    exports.parseSystem = ParseSystem;
    exports.clientMainSystem = clientMainSystem;
    exports.clientKeysSystem = clientKeysSystem;
    exports.memorySystem = memorySystem;
}

/*
====================
idClientGUISystemLocal::InitGUI
====================
*/

void idClientGUISystemLocal::InitGUI( void )
{
    // load the GUI module
    uivm = idsystem->LoadDll( "gui" );
    if ( !uivm )
    {
        Com_Error( ERR_DROP, "vm on gui failed" );
    }
    
    // Load in the entry point.
    dllEntry = ( idUserInterfaceManager * ( QDECL* )( guiImports_t* ) )idsystem->GetProcAddress( uivm, "dllEntry" );
    if ( !dllEntry )
    {
        Com_Error( ERR_DROP, "cgdllEntry on cgame failed" );
    }
    
    // Create the export table.
    CreateExportTable();
    
    // Call the dll entry point.
    uiManager = dllEntry( &exports );
    
    uiManager->Init( cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE );
}

/*
====================
idClientGUISystemLocal::checkKeyExec
====================
*/
bool idClientGUISystemLocal::checkKeyExec( S32 key )
{
    if ( uivm )
    {
        return uiManager->CheckExecKey( key );
    }
    else
    {
        return false;
    }
}

/*
====================
idClientGUISystemLocal::GameCommand

See if the current console command is claimed by the ui
====================
*/
bool idClientGUISystemLocal::GameCommand( void )
{
    if ( !cls.uiStarted )
    {
        return false;
    }
    
    return uiManager->ConsoleCommand( cls.realtime );
}

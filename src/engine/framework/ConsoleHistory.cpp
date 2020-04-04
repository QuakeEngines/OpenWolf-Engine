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

idConsoleHistorySystemLocal consoleHistorySystemLocal;
idConsoleHistorySystem* consoleHistorySystem = &consoleHistorySystemLocal;

/*
===============
idConsoleHistorySystemLocal::idConsoleHistorySystemLocal
===============
*/
idConsoleHistorySystemLocal::idConsoleHistorySystemLocal( void )
{
}

/*
===============
idConsoleHistorySystemLocal::~idConsoleHistorySystemLocal
===============
*/
idConsoleHistorySystemLocal::~idConsoleHistorySystemLocal( void )
{
}

/*
==================
idHistorySystemLocal::HistoryLoad
==================
*/
void idConsoleHistorySystemLocal::HistoryLoad( void )
{
    S32 i;
    UTF8* buf, * end, buffer[sizeof( history )];
    fileHandle_t f;
    
    fileSystem->SV_FOpenFileRead( CON_HISTORY_FILE, &f );
    if ( !f )
    {
        Com_Printf( "Couldn't read %s.\n", CON_HISTORY_FILE );
        return;
    }
    
    fileSystem->Read( buffer, sizeof( buffer ), f );
    fileSystem->FCloseFile( f );
    
    buf = buffer;
    for ( i = 0; i < CON_HISTORY; i++ )
    {
        end = ::strchr( buf, '\n' );
        if ( !end )
        {
            end = buf + ::strlen( buf );
            Q_strncpyz( history[i], buf, sizeof( history[0] ) );
            break;
        }
        else
        {
            *end = '\0';
        }
        
        Q_strncpyz( history[i], buf, sizeof( history[0] ) );
        
        buf = end + 1;
        if ( !*buf )
        {
            break;
        }
    }
    
    if ( i > CON_HISTORY )
    {
        i = CON_HISTORY;
    }
    
    hist_current = hist_next = i + 1;
}

/*
==================
idHistorySystemLocal::HistorySave
==================
*/
void idConsoleHistorySystemLocal::HistorySave( void )
{
    S32 i;
    fileHandle_t f;
    
    f = fileSystem->SV_FOpenFileWrite( CON_HISTORY_FILE );
    if ( !f )
    {
        Com_Printf( "Couldn't write %s.\n", CON_HISTORY_FILE );
        return;
    }
    
    i = ( hist_next + 1 ) % CON_HISTORY;
    do
    {
        if ( history[i][0] )
        {
            fileSystem->Write( history[i], ( S32 )::strlen( history[i] ), f );
            fileSystem->Write( "\n", 1, f );
        }
        
        i = ( i + 1 ) % CON_HISTORY;
        
    }
    while ( i != hist_next % CON_HISTORY );
    
    fileSystem->FCloseFile( f );
}

/*
==================
idHistorySystemLocal::HistoryAdd
==================
*/
void idConsoleHistorySystemLocal::HistoryAdd( StringEntry field )
{
    StringEntry prev = history[( hist_next - 1 ) % CON_HISTORY];
    
    if ( !field[0] || ( ( field[0] == '/' || field[0] == '\\' ) && !field[1] ) )
    {
        hist_current = hist_next;
        return;
    }
    
    if ( ( *field == *prev || ( *field == '/' && *prev == '\\' ) || ( *field == '\\' && *prev == '/' ) ) && !::strcmp( &field[1], &prev[1] ) )
    {
        hist_current = hist_next;
        return;
    }
    
    Q_strncpyz( history[hist_next % CON_HISTORY], field, sizeof( history[0] ) );
    
    hist_next++;
    hist_current = hist_next;
    
    HistorySave();
}

/*
==================
Hist_Prev
==================
*/
StringEntry idConsoleHistorySystemLocal::HistoryPrev( void )
{
    if ( ( hist_current - 1 ) % CON_HISTORY != hist_next % CON_HISTORY && history[( hist_current - 1 ) % CON_HISTORY][0] )
    {
        hist_current--;
    }
    
    return history[hist_current % CON_HISTORY];
}

/*
==================
Hist_Next
==================
*/
StringEntry idConsoleHistorySystemLocal::HistoryNext( void )
{
    if ( hist_current % CON_HISTORY != hist_next % CON_HISTORY )
    {
        hist_current++;
    }
    
    if ( hist_current % CON_HISTORY == hist_next % CON_HISTORY )
    {
        return nullptr;
    }
    
    return history[hist_current % CON_HISTORY];
}

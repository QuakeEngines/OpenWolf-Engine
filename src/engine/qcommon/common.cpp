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
// File name:   common.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: misc functions used in client and server
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

S32 demo_protocols[] = { 1, 0 };

#define MAX_NUM_ARGVS   50

S32             com_argc;
UTF8*           com_argv[MAX_NUM_ARGVS + 1];

jmp_buf         abortframe;		// an ERR_DROP occured, exit the entire frame

static qmutex_t* com_print_mutex;

fileHandle_t logfile_;
fileHandle_t    com_journalFile;	// events are written here
fileHandle_t    com_journalDataFile;	// config files are written here

convar_t*         com_crashed = NULL;	// ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

//bani - explicit NULL to make win32 teh happy

convar_t*         com_ignorecrash = NULL;	// bani - let experienced users ignore crashes, explicit NULL to make win32 teh happy
convar_t*         com_pid;		// bani - process id

convar_t*         com_viewlog;
convar_t*         com_speeds;
convar_t*         com_developer;
convar_t*         com_dedicated;
convar_t*         com_timescale;
convar_t*         com_fixedtime;
convar_t*         com_dropsim;	// 0.0 to 1.0, simulated packet drops
convar_t*         com_journal;
convar_t*         com_maxfps;
convar_t*         com_timedemo;
convar_t*         com_sv_running;
convar_t*         com_cl_running;
convar_t*         com_showtrace;
convar_t*         com_version;
convar_t* com_logfile;		// 1 = buffer log, 2 = flush after each print
convar_t* com_logfilename;
//convar_t    *com_blood;
convar_t*         com_buildScript;	// for automated data building scripts
convar_t*         con_drawnotify;
convar_t*         com_ansiColor;

convar_t*         com_unfocused;
convar_t*         com_minimized;

convar_t* com_affinity;

convar_t*         com_introPlayed;
convar_t*         com_logosPlaying;
convar_t*         cl_paused;
convar_t*         sv_paused;
#if defined (DEDICATED)
convar_t*	   	cl_packetdelay;
#endif
//convar_t		   *sv_packetdelay;
convar_t*         com_cameraMode;
convar_t*         com_maxfpsUnfocused;
convar_t*         com_maxfpsMinimized;
convar_t*         com_abnormalExit;

#if defined( _WIN32 ) && defined( _DEBUG )
convar_t*         com_noErrorInterrupt;
#endif
convar_t*         com_recommendedSet;

convar_t*         com_watchdog;
convar_t*         com_watchdog_cmd;

// Rafael Notebook
convar_t*         cl_notebook;

convar_t*         com_hunkused;	// Ridah
convar_t*         com_protocol;

// com_speeds times
S32             time_game;
S32             time_frontend;	// renderer frontend time
S32             time_backend;	// renderer backend time

S32             com_frameTime;
S32             com_frameMsec;
S32             com_frameNumber;
S32             com_expectedhunkusage;
S32             com_hunkusedvalue;

bool com_errorEntered = false;
bool com_fullyInitialized = false;
bool com_gameRestarting = false;

UTF8            com_errorMessage[MAXPRINTMSG];
void Com_WriteConfiguration( void );
void            Com_WriteConfig_f( void );
void            CIN_CloseAllVideos();

//============================================================================

static UTF8*    rd_buffer;
static S32      rd_buffersize;
static bool rd_flushing = false;
static void ( *rd_flush )( UTF8* buffer );

void Com_BeginRedirect( UTF8* buffer, S32 buffersize, void ( *flush )( UTF8* ) )
{
    if ( !buffer || !buffersize || !flush )
    {
        return;
    }
    rd_buffer = buffer;
    rd_buffersize = buffersize;
    rd_flush = flush;
    
    *rd_buffer = 0;
}

void Com_EndRedirect( void )
{
    if ( rd_flush )
    {
        rd_flushing = true;
        rd_flush( rd_buffer );
        rd_flushing = false;
    }
    
    rd_buffer = NULL;
    rd_buffersize = 0;
    rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void Com_Printf( StringEntry fmt, ... )
{
    va_list	argptr;
    UTF8 msg[MAXPRINTMSG];
    static bool openingLogFile = false;
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
    va_end( argptr );
    
    threadsSystem->Mutex_Lock( com_print_mutex );
    
    if ( rd_buffer && !rd_flushing )
    {
        if ( ( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) )
        {
            rd_flushing = true;
            rd_flush( rd_buffer );
            rd_flushing = false;
            *rd_buffer = 0;
        }
        Q_strcat( rd_buffer, rd_buffersize, msg );
        // show_bug.cgi?id=51
        // only flush the rcon buffer when it's necessary, avoid fragmenting
        //rd_flush(rd_buffer);
        //*rd_buffer = 0;
        
        threadsSystem->Mutex_Unlock( com_print_mutex );
        return;
    }
    
    threadsSystem->Mutex_Unlock( com_print_mutex );
    
#ifndef DEDICATED
    CL_ConsolePrint( msg );
#endif
    
    // echo to dedicated console and early console
    idsystem->Print( msg );
    
    // logfile
    if ( com_logfile && com_logfile->integer )
    {
        // TTimo: only open the qconsole.log if the filesystem is in an initialized state
        //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
        if ( !logfile_ && fileSystem->Initialized() && !openingLogFile )
        {
            struct tm*	newtime;
            time_t		aclock;
            UTF8*		filename;
            
            filename = com_logfilename->string;
            
            if ( !filename )
            {
                filename = "qconsole.log";
            }
            
            openingLogFile = true;
            
            time( &aclock );
            newtime = localtime( &aclock );
            
            logfile_ = fileSystem->FOpenFileWrite( filename );
            
            if ( logfile_ )
            {
                Com_Printf( "Opened logfile %s on %s \n", filename, asctime( newtime ) );
                
                if ( com_logfile->integer > 1 )
                {
                    // force it to not buffer so we get valid
                    // data even if we are crashing
                    fileSystem->ForceFlush( logfile_ );
                }
            }
            else
            {
                Com_Printf( "Failed to open logfile %s" S_COLOR_WHITE "\n", filename );
                cvarSystem->SetValue( "logfile", 0 );
            }
            
            openingLogFile = false;
        }
        if ( logfile_ && fileSystem->Initialized() )
        {
            fileSystem->Write( msg, ( S32 )::strlen( msg ), logfile_ );
        }
    }
}

void Com_FatalError( StringEntry error, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    
    va_start( argptr, error );
    Q_vsnprintf( msg, sizeof( msg ), error, argptr );
    va_end( argptr );
    
    Com_Error( ERR_FATAL, msg );
}

void Com_DropError( StringEntry error, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    
    va_start( argptr, error );
    Q_vsnprintf( msg, sizeof( msg ), error, argptr );
    va_end( argptr );
    
    Com_Error( ERR_DROP, msg );
}

void Com_Warning( StringEntry error, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    
    va_start( argptr, error );
    Q_vsnprintf( msg, sizeof( msg ), error, argptr );
    va_end( argptr );
    
    Com_Printf( msg );
}



/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf( StringEntry fmt, ... )
{
    va_list         argptr;
    UTF8            msg[MAXPRINTMSG];
    
    if ( !com_developer || com_developer->integer != 1 )
    {
        return;					// don't confuse non-developers with techie stuff...
    }
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
    va_end( argptr );
    
    Com_Printf( "%s", msg );
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate thing.
=============
*/
// *INDENT-OFF*
void Com_Error( S32 code, StringEntry fmt, ... )
{
    va_list         argptr;
    static S32      lastErrorTime;
    static S32      errorCount;
    S32             currentTime;
    static bool calledSysError = false;
    
    if ( com_errorEntered )
    {
        if ( !calledSysError )
        {
            calledSysError = true;
            idsystem->Error( "recursive error after: %s", com_errorMessage );
        }
    }
    
    com_errorEntered = true;
    
    cvarSystem->Set( "com_errorCode", va( "%i", code ) );
    
    // when we are running automated scripts, make sure we
    // know if anything failed
    if ( com_buildScript && com_buildScript->integer )
    {
        code = ERR_FATAL;
    }
    
    // make sure we can get at our local stuff
    fileSystem->PureServerSetLoadedPaks( "", "" );
    
    // if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
    currentTime = idsystem->Milliseconds();
    if ( currentTime - lastErrorTime < 100 )
    {
        if ( ++errorCount > 3 )
        {
            code = ERR_FATAL;
        }
    }
    else
    {
        errorCount = 0;
    }
    lastErrorTime = currentTime;
    
    if ( com_dedicated && com_dedicated->integer > 0 )
    {
        code = ERR_FATAL;
    }
    
    va_start( argptr, fmt );
    Q_vsnprintf( com_errorMessage, sizeof( com_errorMessage ), fmt, argptr );
    va_end( argptr );
    
    switch ( code )
    {
        case ERR_FATAL:
        case ERR_DROP:
            idsystem->WriteDump( "Debug Dump\nCom_Error: %s", com_errorMessage );
            break;
    }
    
    if ( code == ERR_SERVERDISCONNECT )
    {
#ifndef DEDICATED
        clientMainSystem->Disconnect( true );
        clientMainSystem->FlushMemory();
#endif
        com_errorEntered = false;
        longjmp( abortframe, -1 );
    }
    else if ( code == ERR_DROP || code == ERR_DISCONNECT )
    {
        Com_Printf( "********************\nERROR: %s\n********************\n", com_errorMessage );
        serverInitSystem->Shutdown( va( "Server crashed: %s\n", com_errorMessage ) );
#ifndef DEDICATED
        clientMainSystem->Disconnect( true );
        clientMainSystem->FlushMemory();
#endif
        com_errorEntered = false;
        longjmp( abortframe, -1 );
    }
#ifndef DEDICATED
    else if ( code == ERR_AUTOUPDATE )
    {
        clientMainSystem->Disconnect( true );
        clientMainSystem->FlushMemory();
        com_errorEntered = false;
        if ( !Q_stricmpn( com_errorMessage, "Server is full", 14 ) && clientMainSystem->NextUpdateServer() )
        {
            clientMainSystem->GetAutoUpdate();
        }
        else
        {
            longjmp( abortframe, -1 );
        }
    }
#endif
    else
    {
        serverInitSystem->Shutdown( va( "Server fatal crashed: %s\n", com_errorMessage ) );
#ifndef DEDICATED
        clientMainSystem->Shutdown();
#endif
    }
    
    Com_Shutdown( code == ERR_VID_FATAL ? true : false );
    
    calledSysError = true;
    idsystem->Error( "%s", com_errorMessage );
}

/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f( void )
{
    // don't try to shutdown if we are in a recursive error
    if ( !com_errorEntered )
    {
        // Some VMs might execute "quit" command directly,
        // which would trigger an unload of active VM error.
        // idSystemLocal::Quit will kill this process anyways, so
        // a corrupt call stack makes no difference
        serverInitSystem->Shutdown( "Server quit\n" );
        //bani
#ifndef DEDICATED
        clientGameSystem->ShutdownCGame();
        clientMainSystem->Shutdown();
#endif
        Com_Shutdown( false );
        fileSystem->Shutdown( true );
        cmdSystem->Shutdown();
        cvarSystem->Shutdown();
        memorySystem->Shutdown();
    }
    idsystem->Quit();
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES   32
S32             com_numConsoleLines;
UTF8*           com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( UTF8* commandLine )
{
    com_consoleLines[0] = commandLine;
    com_numConsoleLines = 1;
    
    while ( *commandLine )
    {
        // look for a + seperating character
        // if commandLine came from a file, we might have real line seperators
        if ( *commandLine == '+' || *commandLine == '\n' )
        {
            if ( com_numConsoleLines == MAX_CONSOLE_LINES )
            {
                return;
            }
            com_consoleLines[com_numConsoleLines] = commandLine + 1;
            com_numConsoleLines++;
            *commandLine = 0;
        }
        commandLine++;
    }
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of wolfconfig.cfg
===================
*/
bool Com_SafeMode( void )
{
    S32             i;
    
    for ( i = 0; i < com_numConsoleLines; i++ )
    {
        cmdSystem->TokenizeString( com_consoleLines[i] );
        if ( !Q_stricmp( cmdSystem->Argv( 0 ), "safe" ) || !Q_stricmp( cmdSystem->Argv( 0 ), "cvar_restart" ) )
        {
            com_consoleLines[i][0] = 0;
            return true;
        }
    }
    return false;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable( StringEntry match )
{
    S32             i;
    UTF8*           s;
    
    for ( i = 0; i < com_numConsoleLines; i++ )
    {
        cmdSystem->TokenizeString( com_consoleLines[i] );
        
        if ( ::strcmp( cmdSystem->Argv( 0 ), "set" ) )
        {
            continue;
        }
        
        s = cmdSystem->Argv( 1 );
        
        if ( !match || !strcmp( s, match ) )
        {
            if ( cvarSystem->Flags( s ) == CVAR_NONEXISTENT )
            {
                cvarSystem->Get( s, cmdSystem->Argv( 2 ), CVAR_USER_CREATED, nullptr );
            }
            else
            {
                cvarSystem->Set( s, cmdSystem->Argv( 2 ) );
            }
        }
    }
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
bool Com_AddStartupCommands( void )
{
    S32             i;
    bool        added;
    
    added = false;
    // quote every token, so args with semicolons can work
    for ( i = 0; i < com_numConsoleLines; i++ )
    {
        if ( !com_consoleLines[i] || !com_consoleLines[i][0] )
        {
            continue;
        }
        
        // set commands won't override menu startup
        if ( !Q_stricmpn( com_consoleLines[i], "set", 3 ) )
        {
            continue;
        }
        
        added = true;
        cmdBufferSystem->AddText( com_consoleLines[i] );
        cmdBufferSystem->AddText( "\n" );
    }
    
    return added;
}


//============================================================================

void Info_Print( StringEntry s )
{
    UTF8            key[BIG_INFO_VALUE];
    UTF8            value[BIG_INFO_VALUE];
    UTF8*           o;
    S32             l;
    
    if ( *s == '\\' )
    {
        s++;
    }
    while ( *s )
    {
        o = key;
        while ( *s && *s != '\\' )
            *o++ = *s++;
            
        l = ( S32 )( o - key );
        if ( l < 20 )
        {
            memset( o, ' ', 20 - l );
            key[20] = 0;
        }
        else
        {
            *o = 0;
        }
        Com_Printf( "%s ", key );
        
        if ( !*s )
        {
            Com_Printf( "MISSING VALUE\n" );
            return;
        }
        
        o = value;
        s++;
        while ( *s && *s != '\\' )
            *o++ = *s++;
        *o = 0;
        
        if ( *s )
        {
            s++;
        }
        Com_Printf( "%s\n", value );
    }
}

/*
============
Com_Filter
============
*/
S32 Com_Filter( UTF8* filter, UTF8* name, S32 casesensitive )
{
    UTF8            buf[MAX_TOKEN_CHARS];
    UTF8*           ptr;
    S32             i, found;
    
    while ( *filter )
    {
        if ( *filter == '*' )
        {
            filter++;
            for ( i = 0; *filter; i++ )
            {
                if ( *filter == '*' || *filter == '?' )
                {
                    break;
                }
                buf[i] = *filter;
                filter++;
            }
            buf[i] = '\0';
            if ( strlen( buf ) )
            {
                ptr = Com_StringContains( name, buf, casesensitive );
                if ( !ptr )
                {
                    return false;
                }
                name = ptr + strlen( buf );
            }
        }
        else if ( *filter == '?' )
        {
            filter++;
            name++;
        }
        else if ( *filter == '[' && *( filter + 1 ) == '[' )
        {
            filter++;
        }
        else if ( *filter == '[' )
        {
            filter++;
            found = false;
            while ( *filter && !found )
            {
                if ( *filter == ']' && *( filter + 1 ) != ']' )
                {
                    break;
                }
                if ( *( filter + 1 ) == '-' && *( filter + 2 ) && ( *( filter + 2 ) != ']' || *( filter + 3 ) == ']' ) )
                {
                    if ( casesensitive )
                    {
                        if ( *name >= *filter && *name <= *( filter + 2 ) )
                        {
                            found = true;
                        }
                    }
                    else
                    {
                        if ( toupper( *name ) >= toupper( *filter ) && toupper( *name ) <= toupper( *( filter + 2 ) ) )
                        {
                            found = true;
                        }
                    }
                    filter += 3;
                }
                else
                {
                    if ( casesensitive )
                    {
                        if ( *filter == *name )
                        {
                            found = true;
                        }
                    }
                    else
                    {
                        if ( toupper( *filter ) == toupper( *name ) )
                        {
                            found = true;
                        }
                    }
                    filter++;
                }
            }
            if ( !found )
            {
                return false;
            }
            while ( *filter )
            {
                if ( *filter == ']' && *( filter + 1 ) != ']' )
                {
                    break;
                }
                filter++;
            }
            filter++;
            name++;
        }
        else
        {
            if ( casesensitive )
            {
                if ( *filter != *name )
                {
                    return false;
                }
            }
            else
            {
                if ( toupper( *filter ) != toupper( *name ) )
                {
                    return false;
                }
            }
            filter++;
            name++;
        }
    }
    return true;
}

/*
============
Com_FilterPath
============
*/
S32 Com_FilterPath( UTF8* filter, UTF8* name, S32 casesensitive )
{
    S32             i;
    UTF8            new_filter[MAX_QPATH];
    UTF8            new_name[MAX_QPATH];
    
    for ( i = 0; i < MAX_QPATH - 1 && filter[i]; i++ )
    {
        if ( filter[i] == '\\' || filter[i] == ':' )
        {
            new_filter[i] = '/';
        }
        else
        {
            new_filter[i] = filter[i];
        }
    }
    new_filter[i] = '\0';
    for ( i = 0; i < MAX_QPATH - 1 && name[i]; i++ )
    {
        if ( name[i] == '\\' || name[i] == ':' )
        {
            new_name[i] = '/';
        }
        else
        {
            new_name[i] = name[i];
        }
    }
    new_name[i] = '\0';
    return Com_Filter( new_filter, new_name, casesensitive );
}



/*
================
Com_RealTime
================
*/
S32 Com_RealTime( qtime_t* qtime )
{
    time_t          t;
    struct tm*      tms;
    
    t = time( NULL );
    if ( !qtime )
    {
        return ( S32 )t;
    }
    tms = localtime( &t );
    if ( tms )
    {
        qtime->tm_sec = tms->tm_sec;
        qtime->tm_min = tms->tm_min;
        qtime->tm_hour = tms->tm_hour;
        qtime->tm_mday = tms->tm_mday;
        qtime->tm_mon = tms->tm_mon;
        qtime->tm_year = tms->tm_year;
        qtime->tm_wday = tms->tm_wday;
        qtime->tm_yday = tms->tm_yday;
        qtime->tm_isdst = tms->tm_isdst;
    }
    return ( S32 )t;
}

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
// Dushan, 512
#define MAX_PUSHED_EVENTS               512
// bk001129 - init, also static
static S32      com_pushedEventsHead = 0;
static S32      com_pushedEventsTail = 0;

// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void )
{
    S32 i;
    
    Com_StartupVariable( "journal" );
    com_journal = cvarSystem->Get( "journal", "0", CVAR_INIT, "description" );
    if ( !com_journal->integer )
    {
        if ( com_journal->string && com_journal->string[ 0 ] == '_' )
        {
            Com_Printf( "Replaying journaled events\n" );
            fileSystem->FOpenFileRead( va( "journal%s.dat", com_journal->string ), &com_journalFile, true );
            fileSystem->FOpenFileRead( va( "journal_data%s.dat", com_journal->string ), &com_journalDataFile, true );
            com_journal->integer = 2;
        }
        else
            return;
    }
    else
    {
        for ( i = 0; i <= 9999 ; i++ )
        {
            UTF8 f[MAX_OSPATH];
            Q_snprintf( f, sizeof( f ), "journal_%04d.dat", i );
            if ( !fileSystem->FileExists( f ) )
                break;
        }
        
        if ( com_journal->integer == 1 )
        {
            Com_Printf( "Journaling events\n" );
            com_journalFile		= fileSystem->FOpenFileWrite( va( "journal_%04d.dat", i ) );
            com_journalDataFile	= fileSystem->FOpenFileWrite( va( "journal_data_%04d.dat", i ) );
        }
        else if ( com_journal->integer == 2 )
        {
            i--;
            Com_Printf( "Replaying journaled events\n" );
            fileSystem->FOpenFileRead( va( "journal_%04d.dat", i ), &com_journalFile, true );
            fileSystem->FOpenFileRead( va( "journal_data_%04d.dat", i ), &com_journalDataFile, true );
        }
    }
    
    if ( !com_journalFile || !com_journalDataFile )
    {
        cvarSystem->Set( "journal", "0" );
        
        if ( com_journalFile )
        {
            fileSystem->FCloseFile( com_journalFile );
        }
        
        if ( com_journalDataFile )
        {
            fileSystem->FCloseFile( com_journalDataFile );
        }
        
        com_journalFile = 0;
        com_journalDataFile = 0;
        Com_Printf( "Couldn't open journal files\n" );
    }
}

/*
========================================================================
EVENT LOOP
========================================================================
*/

#define MAX_QUEUED_EVENTS  256
#define MASK_QUEUED_EVENTS ( MAX_QUEUED_EVENTS - 1 )

static sysEvent_t  eventQueue[ MAX_QUEUED_EVENTS ];
static S32         eventHead = 0;
static S32         eventTail = 0;
static U8        sys_packetReceived[ MAX_MSGLEN ];

/*
================
Com_QueueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Com_QueueEvent( S32 time, sysEventType_t type, S32 value, S32 value2, S32 ptrLength, void* ptr )
{
    sysEvent_t*  ev;
    
    ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];
    
    if ( eventHead - eventTail >= MAX_QUEUED_EVENTS )
    {
        Com_DPrintf( "Com_QueueEvent: overflow\n" );
        // we are discarding an event, but don't leak memory
        if ( ev->evPtr )
        {
            memorySystem->Free( ev->evPtr );
        }
        eventTail++;
    }
    
    eventHead++;
    
    if ( time == 0 )
    {
        time = idsystem->Milliseconds();
    }
    
    ev->evTime = time;
    ev->evType = type;
    ev->evValue = value;
    ev->evValue2 = value2;
    ev->evPtrLength = ptrLength;
    ev->evPtr = ptr;
}

/*
================
Com_GetSystemEvent

================
*/
sysEvent_t Com_GetSystemEvent( void )
{
    sysEvent_t  ev;
    UTF8*        s;
    msg_t       netmsg;
    netadr_t    adr;
    
    // return if we have data
    if ( eventHead > eventTail )
    {
        eventTail++;
        return eventQueue[( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
    }
    
    // check for console commands
    s = idsystem->ConsoleInput();
    
    if ( s )
    {
        UTF8*  b;
        S32   len;
        
        len = ( S32 )::strlen( s ) + 1;
        b = ( UTF8* )memorySystem->Malloc( len );
        Q_strncpyz( b, s, len - 1 );
        Com_QueueEvent( 0, SYSE_CONSOLE, 0, 0, len, b );
    }
    
    // check for network packets
    MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
    
    adr.type = NA_IP;
    if ( networkSystem->GetPacket( &adr, &netmsg ) )
    {
        netadr_t*  buf;
        S32       len;
        
        // copy out to a seperate buffer for qeueing
        len = sizeof( netadr_t ) + netmsg.cursize;
        buf = ( netadr_t* )memorySystem->Malloc( len );
        *buf = adr;
        memcpy( buf + 1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
        Com_QueueEvent( 0, SYSE_PACKET, 0, 0, len, buf );
    }
    
    // return if we have data
    if ( eventHead > eventTail )
    {
        eventTail++;
        return eventQueue[( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
    }
    
    // create an empty event to return
    memset( &ev, 0, sizeof( ev ) );
    ev.evTime = idsystem->Milliseconds();
    
    return ev;
}


/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void )
{
    S32			r;
    sysEvent_t	ev;
    
    // either get an event from the system or the journal file
    if ( com_journal->integer == 2 )
    {
        r = fileSystem->Read( &ev, sizeof( ev ), com_journalFile );
        if ( r != sizeof( ev ) )
        {
            //Com_Error( ERR_FATAL, "Error reading from journal file" );
            com_journal->integer = 0;
            ev.evType = SYSE_NONE;
        }
        if ( ev.evPtrLength )
        {
            ev.evPtr = memorySystem->Malloc( ev.evPtrLength );
            r = fileSystem->Read( ev.evPtr, ev.evPtrLength, com_journalFile );
            if ( r != ev.evPtrLength )
            {
                //Com_Error( ERR_FATAL, "Error reading from journal file" );
                com_journal->integer = 0;
                ev.evType = SYSE_NONE;
            }
        }
    }
    else
    {
        ev = Com_GetSystemEvent();
        
        // write the journal value out if needed
        if ( com_journal->integer == 1 )
        {
            r = fileSystem->Write( &ev, sizeof( ev ), com_journalFile );
            if ( r != sizeof( ev ) )
            {
                Com_Error( ERR_FATAL, "Error writing to journal file" );
            }
            if ( ev.evPtrLength )
            {
                r = fileSystem->Write( ev.evPtr, ev.evPtrLength, com_journalFile );
                if ( r != ev.evPtrLength )
                {
                    Com_Error( ERR_FATAL, "Error writing to journal file" );
                }
            }
        }
    }
    
    return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void )
{
    // clear the static buffer array
    // this requires SYSE_NONE to be accepted as a valid but NOP event
    memset( com_pushedEvents, 0, sizeof( com_pushedEvents ) );
    // reset counters while we are at it
    // beware: GetEvent might still return an SYSE_NONE from the buffer
    com_pushedEventsHead = 0;
    com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t* _event )
{
    sysEvent_t*     ev;
    static S32      printedWarning = 0;	// bk001129 - init, bk001204 - explicit S32
    
    ev = &com_pushedEvents[com_pushedEventsHead & ( MAX_PUSHED_EVENTS - 1 )];
    
    if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS )
    {
    
        // don't print the warning constantly, or it can give time for more...
        if ( !printedWarning )
        {
            printedWarning = true;
            Com_Printf( "WARNING: Com_PushEvent overflow\n" );
        }
        
        if ( ev->evPtr )
        {
            memorySystem->Free( ev->evPtr );
        }
        com_pushedEventsTail++;
    }
    else
    {
        printedWarning = false;
    }
    
    *ev = *_event;
    com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t Com_GetEvent( void )
{
    if ( com_pushedEventsHead > com_pushedEventsTail )
    {
        com_pushedEventsTail++;
        return com_pushedEvents[( com_pushedEventsTail - 1 ) & ( MAX_PUSHED_EVENTS - 1 )];
    }
    return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t* evFrom, msg_t* buf )
{
    S32             t1, t2, msec;
    
    t1 = 0;
    
    if ( com_speeds->integer )
    {
        t1 = idsystem->Milliseconds();
    }
    
    serverMainSystem->PacketEvent( *evFrom, buf );
    
    if ( com_speeds->integer )
    {
        t2 = idsystem->Milliseconds();
        msec = t2 - t1;
        if ( com_speeds->integer == 3 )
        {
            Com_Printf( "idServerMainSystemLocal::PacketEvent time: %i\n", msec );
        }
    }
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/

#ifndef DEDICATED
extern bool consoleButtonWasPressed;
#endif

S32 Com_EventLoop( void )
{
    sysEvent_t      ev;
    netadr_t        evFrom;
    static U8       bufData[MAX_MSGLEN];
    msg_t           buf;
    
    MSG_Init( &buf, bufData, sizeof( bufData ) );
    
    while ( 1 )
    {
        networkChainSystem->FlushPacketQueue();
        
        ev = Com_GetEvent();
        
        // if no more events are available
        if ( ev.evType == SYSE_NONE )
        {
            // manually send packet events for the loopback channel
            while ( networkChainSystem->GetLoopPacket( NS_CLIENT, &evFrom, &buf ) )
            {
#ifndef DEDICATED
                clientMainSystem->PacketEvent( evFrom, &buf );
#endif
            }
            
            while ( networkChainSystem->GetLoopPacket( NS_SERVER, &evFrom, &buf ) )
            {
                // if the server just shut down, flush the events
                if ( com_sv_running->integer )
                {
                    Com_RunAndTimeServerPacket( &evFrom, &buf );
                }
            }
            
            return ev.evTime;
        }
        
        
        switch ( ev.evType )
        {
            case SYSE_NONE:
                break;
            case SYSE_KEY:
                clientKeysSystem->KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
                break;
            case SYSE_CHAR:
#ifndef DEDICATED
                // fretn
                // we just pressed the console button,
                // so ignore this event
                // this prevents chars appearing at console input
                // when you just opened it
                if ( consoleButtonWasPressed )
                {
                    consoleButtonWasPressed = false;
                    break;
                }
#endif
                clientKeysSystem->CharEvent( ev.evValue );
                break;
            case SYSE_MOUSE:
                CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
                break;
            case SYSE_JOYSTICK_AXIS:
                CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
                break;
            case SYSE_CONSOLE:
                //cmdBufferSystem->AddText( "\n" );
                if ( ( ( UTF8* )ev.evPtr )[0] == '\\' || ( ( UTF8* )ev.evPtr )[0] == '/' )
                {
                    cmdBufferSystem->AddText( ( UTF8* )ev.evPtr + 1 );
                }
                else
                {
                    cmdBufferSystem->AddText( ( UTF8* )ev.evPtr );
                }
                break;
            case SYSE_PACKET:
                // this cvar allows simulation of connections that
                // drop a lot of packets.  Note that loopback connections
                // don't go through here at all.
                if ( com_dropsim->value > 0 )
                {
                    static S32      seed;
                    
                    if ( Q_random( &seed ) < com_dropsim->value )
                    {
                        break;	// drop this packet
                    }
                }
                
                evFrom = *( netadr_t* ) ev.evPtr;
                buf.cursize = ev.evPtrLength - sizeof( evFrom );
                
                // we must copy the contents of the message out, because
                // the event buffers are only large enough to hold the
                // exact payload, but channel messages need to be large
                // enough to hold fragment reassembly
                if ( buf.cursize > buf.maxsize )
                {
                    Com_Printf( "Com_EventLoop: oversize packet\n" );
                    continue;
                }
                memcpy( buf.data, ( U8* )( ( netadr_t* ) ev.evPtr + 1 ), buf.cursize );
                if ( com_sv_running->integer )
                {
                    Com_RunAndTimeServerPacket( &evFrom, &buf );
                }
                else
                {
#ifndef DEDICATED
                    clientMainSystem->PacketEvent( evFrom, &buf );
#endif
                }
                break;
                
            default:
                Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
                break;
        }
        
        // free any block data
        if ( ev.evPtr )
        {
            memorySystem->Free( ev.evPtr );
        }
    }
    
    return 0;					// never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
S32 Com_Milliseconds( void )
{
    sysEvent_t      ev;
    
    // get events and push them until we get a null event with the current time
    do
    {
    
        ev = Com_GetRealEvent();
        if ( ev.evType != SYSE_NONE )
        {
            Com_PushEvent( &ev );
        }
    }
    while ( ev.evType != SYSE_NONE );
    
    return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f( void )
{
    if ( cmdSystem->Argc() > 1 )
    {
        Com_Error( ERR_DROP, "Testing drop error" );
    }
    else
    {
        Com_Error( ERR_FATAL, "Testing fatal error" );
    }
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f( void )
{
    F32           s;
    S32             start, now;
    
    if ( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "freeze <seconds>\n" );
        return;
    }
    s = ( F32 )atof( cmdSystem->Argv( 1 ) );
    
    start = Com_Milliseconds();
    
    while ( 1 )
    {
        now = Com_Milliseconds();
        if ( ( now - start ) / 1000.0f > s )
        {
            break;
        }
    }
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f( void )
{
    *( volatile S32* )0 = 0x12345678;
}

void Com_SetRecommended( void )
{
    convar_t*         r_highQualityVideo, *com_recommended;
    bool        goodVideo;
    
    // will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
    r_highQualityVideo = cvarSystem->Get( "r_highQualityVideo", "1", CVAR_ARCHIVE, "description" );
    com_recommended = cvarSystem->Get( "com_recommended", "-1", CVAR_ARCHIVE, "description" );
    goodVideo = ( bool )( r_highQualityVideo && r_highQualityVideo->integer );
    
    if ( goodVideo )
    {
        Com_Printf( "Found high quality video and slow CPU\n" );
        cmdBufferSystem->AddText( "exec preset_fast.cfg\n" );
        cvarSystem->Set( "com_recommended", "2" );
    }
    else
    {
        Com_Printf( "Found low quality video and slow CPU\n" );
        cmdBufferSystem->AddText( "exec preset_fastest.cfg\n" );
        cvarSystem->Set( "com_recommended", "3" );
    }
    
}

// Arnout: gameinfo, to let the engine know which gametypes are SP and if we should use profiles.
// This can't be dependant on gamecode as we sometimes need to know about it when no game-modules
// are loaded
gameInfo_t      com_gameInfo;

void Com_GetGameInfo( void )
{
    UTF8*           f, *buf;
    UTF8*           token;
    
    memset( &com_gameInfo, 0, sizeof( com_gameInfo ) );
    
    if ( fileSystem->ReadFile( "gameinfo.dat", ( void** )&f ) > 0 )
    {
        buf = f;
        
        while ( ( token = COM_Parse( &buf ) ) != NULL && token[0] )
        {
            if ( !Q_stricmp( token, "spEnabled" ) )
            {
                com_gameInfo.spEnabled = true;
            }
            else if ( !Q_stricmp( token, "spGameTypes" ) )
            {
                while ( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.spGameTypes |= ( 1 << atoi( token ) );
                }
            }
            else if ( !Q_stricmp( token, "defaultSPGameType" ) )
            {
                if ( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.defaultSPGameType = atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else if ( !Q_stricmp( token, "coopGameTypes" ) )
            {
            
                while ( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.coopGameTypes |= ( 1 << atoi( token ) );
                }
            }
            else if ( !Q_stricmp( token, "defaultCoopGameType" ) )
            {
                if ( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.defaultCoopGameType = atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else if ( !Q_stricmp( token, "defaultGameType" ) )
            {
                if ( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.defaultGameType = atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else if ( !Q_stricmp( token, "usesProfiles" ) )
            {
                if ( ( token = COM_ParseExt( &buf, false ) ) != NULL && token[0] )
                {
                    com_gameInfo.usesProfiles = ( bool )atoi( token );
                }
                else
                {
                    fileSystem->FreeFile( f );
                    Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
                }
            }
            else
            {
                fileSystem->FreeFile( f );
                Com_Error( ERR_FATAL, "Com_GetGameInfo: bad syntax." );
            }
        }
        
        // all is good
        fileSystem->FreeFile( f );
    }
}

// bani - checks if profile.pid is valid
// return true if it is
// return false if it isn't(!)
bool Com_CheckProfile( UTF8* profile_path )
{
    fileHandle_t    f;
    UTF8            f_data[32] = { 0 };
    S32             f_pid;
    
    //let user override this
    if ( com_ignorecrash->integer )
    {
        return true;
    }
    
    if ( fileSystem->FOpenFileRead( profile_path, &f, true ) < 0 )
    {
        //no profile found, we're ok
        return true;
    }
    
    if ( fileSystem->Read( &f_data, sizeof( f_data ) - 1, f ) < 0 )
    {
        //b0rk3d!
        fileSystem->FCloseFile( f );
        //try to delete corrupted pid file
        fileSystem->Delete( profile_path );
        return false;
    }
    
    f_pid = atoi( f_data );
    if ( f_pid != com_pid->integer )
    {
        //pid doesn't match
        fileSystem->FCloseFile( f );
        return false;
    }
    
    //we're all ok
    fileSystem->FCloseFile( f );
    return true;
}

//bani - from files.c
extern UTF8     fs_gamedir[MAX_OSPATH];
UTF8            last_fs_gamedir[MAX_OSPATH];
UTF8            last_profile_path[MAX_OSPATH];

//bani - track profile changes, delete old profile.pid if we change fs_game(dir)
//hackish, we fiddle with fs_gamedir to make fileSystem->* calls work "right"
void Com_TrackProfile( UTF8* profile_path )
{
    UTF8            temp_fs_gamedir[MAX_OSPATH];
    
    //  Com_Printf( "Com_TrackProfile: Tracking profile [%s] [%s]\n", fs_gamedir, profile_path );
    //have we changed fs_game(dir)?
    if ( strcmp( last_fs_gamedir, fs_gamedir ) )
    {
        if ( strlen( last_fs_gamedir ) && strlen( last_profile_path ) )
        {
            //save current fs_gamedir
            Q_strncpyz( temp_fs_gamedir, fs_gamedir, sizeof( temp_fs_gamedir ) );
            //set fs_gamedir temporarily to make fileSystem->* stuff work "right"
            Q_strncpyz( fs_gamedir, last_fs_gamedir, sizeof( fs_gamedir ) );
            if ( fileSystem->FileExists( last_profile_path ) )
            {
                Com_Printf( "Com_TrackProfile: Deleting old pid file [%s] [%s]\n", fs_gamedir, last_profile_path );
                fileSystem->Delete( last_profile_path );
            }
            //restore current fs_gamedir
            Q_strncpyz( fs_gamedir, temp_fs_gamedir, sizeof( fs_gamedir ) );
        }
        //and save current vars for future reference
        Q_strncpyz( last_fs_gamedir, fs_gamedir, sizeof( last_fs_gamedir ) );
        Q_strncpyz( last_profile_path, profile_path, sizeof( last_profile_path ) );
    }
}

// bani - writes pid to profile
// returns true if successful
// returns false if not(!!)
bool Com_WriteProfile( UTF8* profile_path )
{
    fileHandle_t    f;
    
    if ( fileSystem->FileExists( profile_path ) )
    {
        fileSystem->Delete( profile_path );
    }
    
    f = fileSystem->FOpenFileWrite( profile_path );
    if ( f < 0 )
    {
        Com_Printf( "Com_WriteProfile: Can't write %s.\n", profile_path );
        return false;
    }
    
    fileSystem->Printf( f, "%d", com_pid->integer );
    
    fileSystem->FCloseFile( f );
    
    //track profile changes
    Com_TrackProfile( profile_path );
    
    return true;
}

/*
=================
Com_InitRand
Seed the random number generator, if possible with an OS supplied random seed.
=================
*/
static void Com_InitRand( void )
{
    U32 seed;
    
    if ( idsystem->RandomBytes( ( U8* )&seed, sizeof( seed ) ) )
    {
        srand( seed );
    }
    else
    {
        srand( ( U32 )time( NULL ) );
    }
}

/*
=================
Com_Init
=================
*/
void Com_Init( UTF8* commandLine )
{
    S32 pid;
    UTF8* s;
    
    // TTimo gcc warning: variable `safeMode' might be clobbered by `longjmp' or `vfork'
    volatile bool safeMode = true;
    
    threadsSystem->Threads_Init();
    
    com_print_mutex = ( qmutex_t* )threadsSystem->Mutex_Create();
    
    if ( setjmp( abortframe ) )
    {
        idsystem->Error( "Error during initialization" );
    }
    
    Com_Printf( S_COLOR_ALPHA_ORANGERED "%s " S_COLOR_WHITE "%s " S_COLOR_GREY70 "%s " S_COLOR_WHITE "\n ", Q3_VERSION, PLATFORM_STRING, __DATE__ );
    Com_Printf( S_COLOR_GREY70 "--------------------------------" S_COLOR_WHITE "\n" );
    
    // Clear queues
    ::memset( &eventQueue[0], 0, MAX_QUEUED_EVENTS * sizeof( sysEvent_t ) );
    ::memset( &sys_packetReceived[0], 0, MAX_MSGLEN * sizeof( U8 ) );
    
    // initialize the weak pseudo-random number generator for use later.
    Com_InitRand();
    
    // bk001129 - do this before anything else decides to push events
    Com_InitPushEvent();
    
    memorySystem->InitSmallZoneMemory();
    
    cvarSystem->Init();
    
    // prepare enough of the subsystems to handle
    // cvar and command buffer management
    Com_ParseCommandLine( commandLine );
    
    cmdBufferSystem->Init();
    
    // override anything from the config files with command line args
    Com_StartupVariable( NULL );
    
    memorySystem->InitZoneMemory();
    cmdSystem->Init();
    
    // get the developer cvar set as early as possible
    Com_StartupVariable( "developer" );
    
    // bani: init this early
    Com_StartupVariable( "com_ignorecrash" );
    com_ignorecrash = cvarSystem->Get( "com_ignorecrash", "0", 0, "description" );
    
    // ydnar: init crashed variable as early as possible
    com_crashed = cvarSystem->Get( "com_crashed", "0", CVAR_TEMP, "description" );
    
    // bani: init pid
#ifdef _WIN32
    pid = GetCurrentProcessId();
#else
    pid = getpid();
#endif
    s = va( "%d", pid );
    com_pid = cvarSystem->Get( "com_pid", s, CVAR_ROM, "description" );
    
    // done early so bind command exists
#ifndef DEDICATED
    clientKeysSystem->InitKeyCommands();
#endif
    
#ifdef _WIN32
    _setmaxstdio( 2048 );
#endif
    
    fileSystem->InitFilesystem();
    
    Com_InitJournaling();
    
    Com_GetGameInfo();
    
#ifndef UPDATE_SERVER
    cmdBufferSystem->AddText( "exec default.cfg\n" );
    cmdBufferSystem->AddText( "exec language.lang\n" );	// NERVE - SMF
    
    // skip the q3config.cfg if "safe" is on the command line
    if ( !Com_SafeMode() )
    {
        UTF8*           cl_profileStr = cvarSystem->VariableString( "cl_profile" );
        
        safeMode = false;
        if ( com_gameInfo.usesProfiles )
        {
            if ( !cl_profileStr[0] )
            {
                UTF8*           defaultProfile = NULL;
                
                fileSystem->ReadFile( "profiles/defaultprofile.dat", ( void** )&defaultProfile );
                
                if ( defaultProfile )
                {
                    UTF8*           text_p = defaultProfile;
                    UTF8*           token = COM_Parse( &text_p );
                    
                    if ( token && *token )
                    {
                        cvarSystem->Set( "cl_defaultProfile", token );
                        cvarSystem->Set( "cl_profile", token );
                    }
                    
                    fileSystem->FreeFile( defaultProfile );
                    
                    cl_profileStr = cvarSystem->VariableString( "cl_defaultProfile" );
                }
            }
            
            if ( cl_profileStr[0] )
            {
                // bani - check existing pid file and make sure it's ok
                if ( !Com_CheckProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
                {
#ifndef _DEBUG
                    Com_Printf( "^3WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n",
                                cl_profileStr );
                    // ydnar: set crashed state
                    cmdBufferSystem->AddText( "set com_crashed 1\n" );
#endif
                }
                
                // bani - write a new one
                if ( !Com_WriteProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
                {
                    Com_Printf( "^3WARNING: couldn't write profiles/%s/profile.pid\n", cl_profileStr );
                }
                
                // exec the config
                cmdBufferSystem->AddText( va( "exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME ) );
            }
        }
        else
        {
            cmdBufferSystem->AddText( va( "exec %s\n", CONFIG_NAME ) );
        }
    }
    
    cmdBufferSystem->AddText( "exec autoexec.cfg\n" );
#endif
    
    // ydnar: reset crashed state
    cmdBufferSystem->AddText( "set com_crashed 0\n" );
    
    // execute the queued commands
    cmdBufferSystem->Execute();
    
    // override anything from the config files with command line args
    Com_StartupVariable( NULL );
    
#ifdef UPDATE_SERVER
    com_dedicated = cvarSystem->Get( "dedicated", "1", CVAR_LATCH, "description" );
#elif DEDICATED
    // TTimo: default to internet dedicated, not LAN dedicated
    com_dedicated = cvarSystem->Get( "dedicated", "2", CVAR_ROM, "description" );
    cvarSystem->CheckRange( com_dedicated, 1, 2, true );
#else
    com_dedicated = cvarSystem->Get( "dedicated", "0", CVAR_LATCH, "description" );
    cvarSystem->CheckRange( com_dedicated, 0, 2, true );
#endif
    
    // allocate the stack based hunk allocator
    memorySystem->InitHunkMemory();
    
    // if any archived cvars are modified after this, we will trigger a writing
    // of the config file
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;
    
    //
    // init commands and vars
    //
    
    com_logfile = cvarSystem->Get( "logfile", "0", CVAR_TEMP, "" );
    
    // Gordon: no need to latch this in ET, our recoil is framerate independant
    com_maxfps = cvarSystem->Get( "com_maxfps", "125", CVAR_ARCHIVE /*|CVAR_LATCH */, "description" );
    //  com_blood = cvarSystem->Get ("com_blood", "1", CVAR_ARCHIVE, "description"); // Gordon: no longer used?
    
    com_developer = cvarSystem->Get( "developer", "0", CVAR_TEMP, "description" );
    
    com_timescale = cvarSystem->Get( "timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO, "description" );
    com_fixedtime = cvarSystem->Get( "fixedtime", "0", CVAR_CHEAT, "description" );
    com_showtrace = cvarSystem->Get( "com_showtrace", "0", CVAR_CHEAT, "description" );
    com_dropsim = cvarSystem->Get( "com_dropsim", "0", CVAR_CHEAT, "description" );
    com_viewlog = cvarSystem->Get( "viewlog", "0", CVAR_CHEAT, "description" );
    com_speeds = cvarSystem->Get( "com_speeds", "0", 0, "description" );
    com_timedemo = cvarSystem->Get( "timedemo", "0", CVAR_CHEAT, "description" );
    com_cameraMode = cvarSystem->Get( "com_cameraMode", "0", CVAR_CHEAT, "description" );
    
    com_watchdog = cvarSystem->Get( "com_watchdog", "60", CVAR_ARCHIVE, "description" );
    com_watchdog_cmd = cvarSystem->Get( "com_watchdog_cmd", "", CVAR_ARCHIVE, "description" );
    
    cl_paused = cvarSystem->Get( "cl_paused", "0", CVAR_ROM, "description" );
    sv_paused = cvarSystem->Get( "sv_paused", "0", CVAR_ROM, "description" );
    com_sv_running = cvarSystem->Get( "sv_running", "0", CVAR_ROM, "description" );
    com_cl_running = cvarSystem->Get( "cl_running", "0", CVAR_ROM, "description" );
    com_buildScript = cvarSystem->Get( "com_buildScript", "0", 0, "description" );
    
    con_drawnotify = cvarSystem->Get( "con_drawnotify", "0", CVAR_CHEAT, "description" );
    
    com_introPlayed = cvarSystem->Get( "com_introplayed", "0", CVAR_ARCHIVE, "description" );
    com_ansiColor = cvarSystem->Get( "com_ansiColor", "0", CVAR_ARCHIVE, "description" );
    com_logosPlaying = cvarSystem->Get( "com_logosPlaying", "0", CVAR_ROM, "description" );
    com_recommendedSet = cvarSystem->Get( "com_recommendedSet", "0", CVAR_ARCHIVE, "description" );
    
    com_unfocused = cvarSystem->Get( "com_unfocused", "0", CVAR_ROM, "description" );
    com_minimized = cvarSystem->Get( "com_minimized", "0", CVAR_ROM, "description" );
    com_affinity = cvarSystem->Get( "com_affinity", "1", CVAR_ARCHIVE, "description" );
    com_maxfpsUnfocused = cvarSystem->Get( "com_maxfpsUnfocused", "0", CVAR_ARCHIVE, "description" );
    com_maxfpsMinimized = cvarSystem->Get( "com_maxfpsMinimized", "0", CVAR_ARCHIVE, "description" );
    com_abnormalExit = cvarSystem->Get( "com_abnormalExit", "0", CVAR_ROM, "description" );
    
    cvarSystem->Get( "savegame_loading", "0", CVAR_ROM, "description" );
    
#if defined( _WIN32 ) && defined( _DEBUG )
    com_noErrorInterrupt = cvarSystem->Get( "com_noErrorInterrupt", "0", 0, "description" );
#endif
    
    com_hunkused = cvarSystem->Get( "com_hunkused", "0", 0, "description" );
    com_hunkusedvalue = 0;
    
    if ( com_dedicated->integer )
    {
        if ( !com_viewlog->integer )
        {
            cvarSystem->Set( "viewlog", "0" );
        }
    }
    
    if ( com_developer && com_developer->integer )
    {
        cmdSystem->AddCommand( "error", Com_Error_f, "description" );
        cmdSystem->AddCommand( "crash", Com_Crash_f, "description" );
        cmdSystem->AddCommand( "freeze", Com_Freeze_f, "description" );
    }
    cmdSystem->AddCommand( "quit", Com_Quit_f, "description" );
    cmdSystem->AddCommand( "writeconfig", Com_WriteConfig_f, "Writes current settings to a file in your cfg folder, assumes .cfg as default file extension" );
    
    s = va( "%s %s %s %s", Q3_VERSION, ARCH_STRING, OS_STRING, __DATE__ );
    com_version = cvarSystem->Get( "version", s, CVAR_ROM | CVAR_SERVERINFO, "description" );
    com_protocol = cvarSystem->Get( "protocol", va( "%i", PROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_ARCHIVE, "description" );
    
    com_logfilename = cvarSystem->Get( "com_logfilename", "logs/console.log", CVAR_ARCHIVE, "description" );
    
    idsystem->Init();
    
    if ( idsystem->WritePIDFile( ) )
    {
#ifndef DEDICATED
        StringEntry message = "The last time " CLIENT_WINDOW_TITLE " ran, "
                              "it didn't exit properly. This may be due to inappropriate video "
                              "settings. Would you like to start with \"safe\" video settings?";
                              
        if ( idsystem->Dialog( DT_YES_NO, message, "Abnormal Exit" ) == DR_YES )
        {
            cvarSystem->Set( "com_abnormalExit", "1" );
        }
#endif
    }
    
    // Pick a random port value
    Com_RandomBytes( ( U8* )&qport, sizeof( int ) );
    
    networkChainSystem->Init( Com_Milliseconds() & 0xffff );	// pick a port value that should be nice and random
    
    serverInitSystem->Init();
    
    idConsoleHistorySystemLocal::HistoryLoad();
    
    com_dedicated->modified = false;
    if ( !com_dedicated->integer )
    {
#ifndef DEDICATED
        clientMainSystem->Init();
#endif
    }
    
    // set com_frameTime so that if a map is started on the
    // command line it will still be able to count on com_frameTime
    // being random enough for a serverid
    com_frameTime = Com_Milliseconds();
    
    // add + commands from command line
    if ( !Com_AddStartupCommands() )
    {
        // if the user didn't give any commands, run default action
    }
    
    // NERVE - SMF - force recommendedSet and don't do vid_restart if in safe mode
    if ( !com_recommendedSet->integer && !safeMode )
    {
        Com_SetRecommended();
        cmdBufferSystem->ExecuteText( EXEC_APPEND, "vid_restart\n" );
    }
    cvarSystem->Set( "com_recommendedSet", "1" );
    
    if ( !com_dedicated->integer && !Com_AddStartupCommands() )
    {
        cvarSystem->Set( "com_logosPlaying", "1" );
        
        cmdBufferSystem->AddText( "cinematic splash.roq\n" );
        
        cvarSystem->Set( "nextmap", "cinematic avlogo.roq" );
        
        if ( !com_introPlayed->integer )
        {
            cvarSystem->Set( com_introPlayed->name, "1" );
            cvarSystem->Set( "nextmap", "cinematic splash.roq" );
        }
    }
    
#ifndef DEDICATED
    clientMainSystem->StartHunkUsers( false );
#endif
    
    com_fullyInitialized = true;
    
    Com_Printf( "--- Common Initialization Complete ---\n" );
}

void Com_WriteConfigToFile( StringEntry filename )
{
    fileHandle_t    f;
    
    f = fileSystem->FOpenFileWrite( filename );
    if ( !f )
    {
        Com_Printf( "Couldn't write %s.\n", filename );
        return;
    }
    
    fileSystem->Printf( f, "// generated by OpenWolf, do not modify\n" );
    fileSystem->Printf( f, "//\n" );
    fileSystem->Printf( f, "// Key Bindings\n" );
    fileSystem->Printf( f, "//\n" );
#ifndef DEDICATED
    clientKeysSystem->Key_WriteBindings( f );
#endif
    fileSystem->Printf( f, "//\n" );
    fileSystem->Printf( f, "// Cvars\n" );
    fileSystem->Printf( f, "//\n" );
    cvarSystem->WriteVariables( f );
    fileSystem->Printf( f, "//\n" );
    fileSystem->Printf( f, "// Aliases\n" );
    fileSystem->Printf( f, "//\n" );
    cmdSystem->WriteAliases( f );
    
    //close file
    fileSystem->FCloseFile( f );
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void )
{
    // if we are quiting without fully initializing, make sure
    // we don't write out anything
    if ( !com_fullyInitialized )
    {
        return;
    }
    
    if ( !( cvar_modifiedFlags & CVAR_ARCHIVE ) )
    {
        return;
    }
    
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;
    
    {
        UTF8* cl_profileStr = cvarSystem->VariableString( "cl_profile" );
        
        if ( cl_profileStr[0] )
        {
            Com_WriteConfigToFile( va( "profiles/%s/%s", cl_profileStr, CONFIG_NAME ) );
        }
        else
        {
            Com_WriteConfigToFile( CONFIG_NAME );
        }
    }
    
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void )
{
    UTF8 filename[MAX_QPATH];
    UTF8* path = cmdSystem->Argv( 1 );
    
    if ( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: writeconfig <filename>\n" );
        Com_Printf( "Description", "Writes current settings to a file in your cfg folder, assumes .cfg as default file extension", "" );
        return;
    }
    
    Q_strncpyz( filename, cmdSystem->Argv( 1 ), sizeof( filename ) );
    COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
    Com_Printf( "Writing %s.\n", filename );
    
    Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
S32 Com_ModifyMsec( S32 msec )
{
    S32             clampTime;
    
    //
    // modify time for debugging values
    //
    if ( com_fixedtime->integer )
    {
        msec = com_fixedtime->integer;
    }
    else if ( com_timescale->value )
    {
        msec *= ( S32 )com_timescale->value;
        //  } else if (com_cameraMode->integer) {
        //      msec *= com_timescale->value;
    }
    
    // don't let it scale below 1 msec
    if ( msec < 1 && ( S32 )com_timescale->value )
    {
        msec = 1;
    }
    
    if ( com_dedicated->integer )
    {
        // dedicated servers don't want to clamp for a much longer
        // period, because it would mess up all the client's views
        // of time.
        if ( !svs.hibernation.enabled && com_sv_running->integer && msec > 500 )
        {
            Com_Printf( "Hitch warning: %i msec frame time\n", msec );
        }
        
        clampTime = 5000;
    }
    else if ( !com_sv_running->integer )
    {
        // clients of remote servers do not want to clamp time, because
        // it would skew their view of the server's time temporarily
        clampTime = 5000;
    }
    else
    {
        // for local single player gaming
        // we may want to clamp the time to prevent players from
        // flying off edges when something hitches.
        clampTime = 200;
    }
    
    if ( msec > clampTime )
    {
        msec = clampTime;
    }
    
    return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_Frame( void )
{
    S32             msec, minMsec;
    static S32      lastTime;
    S32             key;
    S32             timeBeforeFirstEvents;
    S32             timeBeforeServer;
    S32             timeBeforeEvents;
    S32             timeBeforeClient;
    S32             timeAfter;
    static S32      watchdogTime = 0;
    static bool		watchWarn = false;
    
    if ( setjmp( abortframe ) )
    {
        return;					// an ERR_DROP was thrown
    }
    
    // bk001204 - init to zero.
    //  also:  might be clobbered by `longjmp' or `vfork'
    timeBeforeFirstEvents = 0;
    timeBeforeServer = 0;
    timeBeforeEvents = 0;
    timeBeforeClient = 0;
    timeAfter = 0;
    
    // old net chan encryption key
    key = 0x87243987;
    
    // Don't write config on Update Server
#if !defined (UPDATE_SERVER)
    // write config file if anything changed
    Com_WriteConfiguration();
#endif
    
#ifdef _WiN32
    // if "viewlog" has been modified, show or hide the log console
    if ( com_viewlog->modified )
    {
        if ( !com_dedicated->value )
        {
            Sys_ShowConsole( com_viewlog->integer, false );
        }
        com_viewlog->modified = false;
    }
#endif
    
    // main event loop
    if ( com_speeds->integer )
    {
        timeBeforeFirstEvents = idsystem->Milliseconds();
    }
    
    // we may want to spin here if things are going too fast
    if ( !com_dedicated->integer && !com_timedemo->integer )
    {
        if ( com_minimized->integer && com_maxfpsMinimized->integer > 0 )
        {
            minMsec = 1000 / com_maxfpsMinimized->integer;
        }
        else if ( com_unfocused->integer && com_maxfpsUnfocused->integer > 0 )
        {
            minMsec = 1000 / com_maxfpsUnfocused->integer;
        }
        else if ( com_maxfps->integer > 0 )
        {
            minMsec = 1000 / com_maxfps->integer;
        }
        else
        {
            minMsec = 1;
        }
    }
    else
    {
        minMsec = 1;
    }
    
    msec = minMsec;
    do
    {
        S32 timeRemaining = minMsec - msec;
        
        // The existing idSystemLocal::Sleep implementations aren't really
        // precise enough to be of use beyond 100fps
        // FIXME: implement a more precise sleep (RDTSC or something)
        if ( timeRemaining >= 10 )
        {
            idsystem->SysSleep( timeRemaining );
        }
        
        com_frameTime = Com_EventLoop();
        if ( lastTime > com_frameTime )
        {
            // possible on first frame
            lastTime = com_frameTime;
        }
        msec = com_frameTime - lastTime;
    }
    while ( msec < minMsec );
    
    cmdBufferSystem->Execute();
    cmdDelaySystem->Frame();
    
    lastTime = com_frameTime;
    
    // mess with msec if needed
    com_frameMsec = msec;
    msec = Com_ModifyMsec( msec );
    
    //
    // server side
    //
    if ( com_speeds->integer )
    {
        timeBeforeServer = idsystem->Milliseconds();
    }
    
    serverMainSystem->Frame( msec );
    
    // if "dedicated" has been modified, start up
    // or shut down the client system.
    // Do this after the server may have started,
    // but before the client tries to auto-connect
#ifndef DEDICATED
    if ( com_dedicated->modified )
    {
        // get the latched value
        cvarSystem->Get( "dedicated", "0", 0, "description" );
        
        com_dedicated->modified = false;
        
        if ( !com_dedicated->integer )
        {
            clientMainSystem->Init();
        }
        else
        {
            clientMainSystem->Shutdown();
        }
    }
#endif
    
    //
    // client system
    //
    if ( !com_dedicated->integer )
    {
        //
        // run event loop a second time to get server to client packets
        // without a frame of latency
        //
        if ( com_speeds->integer )
        {
            timeBeforeEvents = idsystem->Milliseconds();
        }
        Com_EventLoop();
        cmdBufferSystem->Execute();
        
        //
        // client side
        //
        if ( com_speeds->integer )
        {
            timeBeforeClient = idsystem->Milliseconds();
        }
        
#ifndef DEDICATED
        clientMainSystem->Frame( msec );
#endif
        
        if ( com_speeds->integer )
        {
            timeAfter = idsystem->Milliseconds();
        }
    }
    else
    {
        timeAfter = idsystem->Milliseconds();
    }
    
    //
    // watchdog
    //
    if ( com_dedicated->integer && !com_sv_running->integer && com_watchdog->integer )
    {
        if ( watchdogTime == 0 )
        {
            watchdogTime = idsystem->Milliseconds();
        }
        else
        {
            if ( !watchWarn && idsystem->Milliseconds() - watchdogTime > ( com_watchdog->integer - 4 ) * 1000 )
            {
                Com_Printf( "WARNING: watchdog will trigger in 4 seconds\n" );
                watchWarn = true;
            }
            else if ( idsystem->Milliseconds() - watchdogTime > com_watchdog->integer * 1000 )
            {
                Com_Printf( "Idle Server with no map - triggering watchdog\n" );
                watchdogTime = 0;
                watchWarn = false;
                if ( com_watchdog_cmd->string[0] == '\0' )
                {
                    cmdBufferSystem->AddText( "quit\n" );
                }
                else
                {
                    cmdBufferSystem->AddText( va( "%s\n", com_watchdog_cmd->string ) );
                }
            }
        }
    }
    
    networkChainSystem->FlushPacketQueue();
    
    //
    // report timing information
    //
    if ( com_speeds->integer )
    {
        S32             all, sv, sev, cev, cl;
        
        all = timeAfter - timeBeforeServer;
        sv = timeBeforeEvents - timeBeforeServer;
        sev = timeBeforeServer - timeBeforeFirstEvents;
        cev = timeBeforeClient - timeBeforeEvents;
        cl = timeAfter - timeBeforeClient;
        sv -= time_game;
        cl -= time_frontend + time_backend;
        
        Com_Printf( "frame:%i all:%3i sv:%3i sev:%3i cev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
                    com_frameNumber, all, sv, sev, cev, cl, time_game, time_frontend, time_backend );
    }
    
    //
    // trace optimization tracking
    //
    if ( com_showtrace->integer )
    {
    
        extern S32 c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
        extern S32 c_pointcontents;
        
        Com_Printf( "%4i traces  (%ib %ip %it) %4i points\n", c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces, c_pointcontents );
        c_traces = 0;
        c_brush_traces = 0;
        c_patch_traces = 0;
        c_trisoup_traces = 0;
        c_pointcontents = 0;
    }
    
    com_frameNumber++;
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown( bool badProfile )
{
    UTF8* cl_profileStr = cvarSystem->VariableString( "cl_profile" );
    
    collisionModelManager->ClearMap();
    
    networkSystem->Shutdown();
    
    if ( logfile_ )
    {
        fileSystem->FCloseFile( logfile_ );
        logfile_ = 0;
        com_logfile->integer = 0;//don't open up the log file again!!
    }
    
    // write config file if anything changed
    Com_WriteConfiguration();
    
    // delete pid file
    if ( com_gameInfo.usesProfiles && cl_profileStr[0] && !badProfile )
    {
        if ( fileSystem->FileExists( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
        {
            fileSystem->Delete( va( "profiles/%s/profile.pid", cl_profileStr ) );
        }
    }
    
    if ( com_journalFile )
    {
        fileSystem->FCloseFile( com_journalFile );
        com_journalFile = 0;
    }
    
    threadsSystem->Mutex_Destroy( &com_print_mutex );
    
    threadsSystem->Threads_Shutdown();
}




//------------------------------------------------------------------------


/*
===========================================
command line completion
===========================================
*/

/*
==================
Field_Clear
==================
*/
void Field_Clear( field_t* edit )
{
    ::memset( edit->buffer, 0, MAX_EDIT_LINE );
    edit->cursor = 0;
    edit->scroll = 0;
}

/*
==================
Field_Set
==================
*/
void Field_Set( field_t* edit, StringEntry content )
{
    ::memset( edit->buffer, 0, MAX_EDIT_LINE );
    ::strncpy( edit->buffer, content, MAX_EDIT_LINE );
    edit->cursor = ( S32 )::strlen( edit->buffer );
    
    if ( edit->cursor >= edit->widthInChars )
    {
        edit->scroll = edit->cursor - edit->widthInChars + 1;
    }
    else
    {
        edit->scroll = 0;
    }
}

/*
==================
Field_WordDelete
==================
*/
void Field_WordDelete( field_t* edit )
{
    while ( edit->cursor )
    {
        if ( edit->buffer[edit->cursor - 1] != ' ' )
        {
            edit->buffer[edit->cursor - 1] = 0;
            edit->cursor--;
        }
        else
        {
            edit->cursor--;
            if ( edit->buffer[edit->cursor - 1] != ' ' )
                return;
        }
    }
}


StringEntry completionString;
static UTF8     shortestMatch[MAX_TOKEN_CHARS];
static S32      matchCount;

// field we are working on, passed to Field_CompleteCommand (&g_consoleCommand for instance)
static field_t* completionField;
static StringEntry completionPrompt;

/*
===============
FindMatches
===============
*/
static void FindMatches( StringEntry s )
{
    S32             i;
    
    if ( Q_stricmpn( s, completionString, ( S32 )::strlen( completionString ) ) )
    {
        return;
    }
    matchCount++;
    if ( matchCount == 1 )
    {
        Q_strncpyz( shortestMatch, s, sizeof( shortestMatch ) );
        return;
    }
    
    // cut shortestMatch to the amount common with s
    // was wrong when s had fewer chars than shortestMatch
    i = 0;
    do
    {
        if ( tolower( shortestMatch[i] ) != tolower( s[i] ) )
        {
            shortestMatch[i] = 0;
        }
    }
    while ( s[i++] );
}


/*
===============
PrintMatches
===============
*/
static void PrintMatches( StringEntry s )
{
    if ( !Q_stricmpn( s, shortestMatch, ( S32 )::strlen( shortestMatch ) ) )
    {
        Com_Printf( "    %s\n", s );
    }
}

/*
===============
PrintCvarMatches
===============
*/
static void PrintCvarMatches( StringEntry s )
{
    if ( !Q_stricmpn( s, shortestMatch, ( S32 )::strlen( shortestMatch ) ) )
    {
        Com_Printf( "    %-32s ^7\"%s^7\"\n", s, cvarSystem->VariableString( s ) );
    }
}


/*
===============
Field_FindFirstSeparator
===============
*/
static UTF8* Field_FindFirstSeparator( UTF8* s )
{
    S32 i;
    
    for ( i = 0; i < strlen( s ); i++ )
    {
        if ( s[ i ] == ';' )
            return &s[ i ];
    }
    
    return NULL;
}

/*
===============
Field_Complete
===============
*/
static bool Field_Complete( void )
{
    S32 completionOffset;
    
    if ( matchCount == 0 )
        return true;
        
    completionOffset = ( S32 )::strlen( completionField->buffer ) - ( S32 )::strlen( completionString );
    
    Q_strncpyz( &completionField->buffer[ completionOffset ], shortestMatch, sizeof( completionField->buffer ) - completionOffset );
    
    completionField->cursor = ( S32 )::strlen( completionField->buffer );
    
    if ( matchCount == 1 )
    {
        Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
        completionField->cursor++;
        return true;
    }
    
    Com_Printf( "%s^7%s\n", completionPrompt, completionField->buffer );
    
    return false;
}

#ifndef DEDICATED
/*
===============
Field_CompleteKeyname
===============
*/
void Field_CompleteKeyname( void )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    clientKeysSystem->Key_KeynameCompletion( FindMatches );
    
    if ( !Field_Complete() )
    {
        clientKeysSystem->Key_KeynameCompletion( PrintMatches );
    }
}

/*
===============
Field_CompleteCgame
===============
*/
void Field_CompleteCgame( S32 argNum )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    clientGameSystem->CgameCompletion( FindMatches, argNum );
    
    if ( !Field_Complete() )
    {
        clientGameSystem->CgameCompletion( PrintMatches, argNum );
    }
}
#endif

/*
===============
Field_CompleteFilename
===============
*/
void Field_CompleteFilename( StringEntry dir,
                             StringEntry ext, bool stripExt )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    fileSystem->FilenameCompletion( dir, ext, stripExt, FindMatches );
    
    if ( !Field_Complete( ) )
        fileSystem->FilenameCompletion( dir, ext, stripExt, PrintMatches );
}

/*
===============
Field_CompleteAlias
===============
*/
void Field_CompleteAlias( void )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    cmdSystem->AliasCompletion( FindMatches );
    
    if ( !Field_Complete( ) )
        cmdSystem->AliasCompletion( PrintMatches );
}

/*
===============
Field_CompleteDelay
===============
*/
void Field_CompleteDelay( void )
{
    matchCount = 0;
    shortestMatch[ 0 ] = 0;
    
    cmdSystem->DelayCompletion( FindMatches );
    
    if ( !Field_Complete( ) )
        cmdSystem->DelayCompletion( PrintMatches );
}

/*
===============
Field_CompleteCommand
===============
*/
void Field_CompleteCommand( UTF8* cmd, bool doCommands, bool doCvars )
{
    S32 completionArgument = 0;
    
    // Skip leading whitespace and quotes
    cmd = Com_SkipCharset( cmd, " \"" );
    
    cmdSystem->TokenizeStringIgnoreQuotes( cmd );
    completionArgument = cmdSystem->Argc( );
    
    // If there is trailing whitespace on the cmd
    if ( *( cmd + strlen( cmd ) - 1 ) == ' ' )
    {
        completionString = "";
        completionArgument++;
    }
    else
        completionString = cmdSystem->Argv( completionArgument - 1 );
        
#ifndef DEDICATED
    // Unconditionally add a '\' to the start of the buffer
    if ( completionField->buffer[ 0 ] &&
            completionField->buffer[ 0 ] != '\\' )
    {
        if ( completionField->buffer[ 0 ] != '/' )
        {
            // Buffer is full, refuse to complete
            if ( strlen( completionField->buffer ) + 1 >= sizeof( completionField->buffer ) )
                return;
                
            memmove( &completionField->buffer[ 1 ],
                     &completionField->buffer[ 0 ],
                     strlen( completionField->buffer ) + 1 );
            completionField->cursor++;
        }
        
        completionField->buffer[ 0 ] = '\\';
    }
#endif
    
    if ( completionArgument > 1 )
    {
        StringEntry baseCmd = cmdSystem->Argv( 0 );
        UTF8* p;
        
#ifndef DEDICATED
        // This should always be true
        if ( baseCmd[ 0 ] == '\\' || baseCmd[ 0 ] == '/' )
            baseCmd++;
#endif
            
        if ( ( p = Field_FindFirstSeparator( cmd ) ) )
            Field_CompleteCommand( p + 1, true, true ); // Compound command
        else
            cmdSystem->CompleteArgument( baseCmd, cmd, completionArgument );
    }
    else
    {
        if ( completionString )
        {
            if ( completionString[0] == '\\' || completionString[0] == '/' )
                completionString++;
                
            matchCount = 0;
            shortestMatch[0] = 0;
            
            if ( strlen( completionString ) == 0 )
                return;
                
            if ( doCommands )
                cmdSystem->CommandCompletion( FindMatches );
                
            if ( doCvars )
                cvarSystem->CommandCompletion( FindMatches );
                
            if ( !Field_Complete() )
            {
                // run through again, printing matches
                if ( doCommands )
                    cmdSystem->CommandCompletion( PrintMatches );
                    
                if ( doCvars )
                    cvarSystem->CommandCompletion( PrintCvarMatches );
            }
        }
    }
}

/*
===============
Field_AutoComplete

Perform Tab expansion
===============
*/
void Field_AutoComplete( field_t* field, StringEntry prompt )
{
    completionField = field;
    completionPrompt = prompt;
    
    Field_CompleteCommand( completionField->buffer, true, true );
}

/*
==================
Com_RandomBytes

fills string array with len radom bytes, peferably from the OS randomizer
==================
*/
void Com_RandomBytes( U8* string, S32 len )
{
    S32 i;
    
    if ( idsystem->RandomBytes( string, len ) )
    {
        return;
    }
    
    Com_Printf( "Com_RandomBytes: using weak randomization\n" );
    
    for ( i = 0; i < len; i++ )
    {
        string[i] = ( U8 )( rand() % 255 );
    }
}

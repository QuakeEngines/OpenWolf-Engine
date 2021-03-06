////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   files.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description: handle based filesystem for Quake III Arena
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

FILE* missingFiles = NULL;

#define MP_LEGACY_PAK 0x7776DC09

/*
=============================================================================

QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created reletive to the base path, so base path should usually be writable.

The "cd path" is the path to an alternate hierarchy that will be searched if a file
is not located in the base path.  A user can do a partial install that copies some
data to a base path created on their hard drive and leave the rest on the cd.  Files
are never writen to the cd path.  It defaults to a value set by the installer, like
"e:\quake3", but it can be overridden with "+set ds_cdpath g:\quake3".

If a user runs the game directly from a CD, the base path would be on the CD.  This
should still function correctly, but all file writes will fail (harmlessly).

The "home path" is the path used for all write access. On win32 systems we have "base path"
== "home path", but on *nix systems the base installation is usually readonly, and
"home path" points to ~/.q3a or similar

The user can also install custom mods and content in "home path", so it should be searched
along with "home path" and "cd path" for game content.


The "base game" is the directory under the paths where data comes from by default, and
can be either "baseq3" or "demoq3".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes.  Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

After the paths are initialized, quake will look for the product.txt file.  If not
found and verified, the game will run in restricted mode.  In restricted mode, only
files contained in demoq3/pak0.pk3 will be available for loading, and only if the zip header is
verified to not have been modified.  A single exception is made for q3config.cfg.  Files
can still be written out in restricted mode, so screenshots and demos are allowed.
Restricted mode can be tested by setting "+set fs_restrict 1" on the command line, even
if there is a valid product.txt under the basepath or cdpath.

If not running in restricted mode, and a file is not found in any local filesystem,
an attempt will be made to download it and save it under the base path.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the cd
path, it will be copied over to the base path.  This is a development aid to help build
test releases and to copy working sets over slow network links.

File search order: when idFileSystemLocal::FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to idFileSystemLocal::AddGameDirectory

Additionaly, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as baseq3 + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("baseq3")

e.g. the qpath "sound/newstuff/test.ogg" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory
cd path + current game's zip files
cd path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory
cd path + base game's zip file
cd path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory
cd path + BASEGAME's zip file
cd path + BASEGAME's directory

server download, to be written to home path + current game's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.

TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.

How to prevent downloading zip files?
Pass pk3 file names in systeminfo, and download before idFileSystemLocal::Restart()?

Aborting a download disconnects the client from the server.

How to mark files as downloadable?  Commercial add-ons won't be downloadable.

Non-commercial downloads will want to download the entire zip file.
the game would have to be reset to actually read the zip in

Auto-update information

Path separators

Casing

  separate server gamedir and client gamedir, so if the user starts
  a local game after having connected to a network game, it won't stick
  with the network game.

  allow menu options for game selection?

Read / write config to floppy option.

Different version coexistance?

When building a pak file, make sure a wolfconfig.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting

=============================================================================
*/

//bani - made fs_gamedir non-static
UTF8 fs_gamedir[MAX_OSPATH]; // this will be a single file name with no separators
static convar_t* fs_debug;
static convar_t* fs_homepath;
static convar_t* fs_basepath;
static convar_t* fs_libpath;

#ifdef MACOS_X
// Also search the .app bundle for .pk3 files
static convar_t* fs_apppath;
#endif

static convar_t* fs_buildpath;
static convar_t* fs_buildgame;
static convar_t* fs_basegame;
static convar_t* fs_copyfiles;
static convar_t* fs_gamedirvar;
static convar_t* fs_missing;
static convar_t* fs_restrict;
static searchpath_t* fs_searchpaths;

static S32 fs_readCount; // total bytes read
static S32 fs_loadCount; // total files read
static S32 fs_loadStack; // total files in memory
static S32 fs_packFiles; // total number of files in packs
static S32 fs_fakeChkSum;
static S32 fs_checksumFeed;

idFileSystemLocal fileSystemLocal;
idFileSystem* fileSystem = &fileSystemLocal;

/*
===============
idFileSystemLocal::idFileSystemLocal
===============
*/
idFileSystemLocal::idFileSystemLocal( void )
{
}

/*
===============
idFileSystemLocal::~idFileSystemLocal
===============
*/
idFileSystemLocal::~idFileSystemLocal( void )
{
}

/*
==============
idFileSystemLocal::Initialized
==============
*/
bool idFileSystemLocal::Initialized( void )
{
    //return ( bool )( fs_searchpaths != NULL );
    return fs_searchpaths != NULL;
}

/*
=================
idFileSystemLocal::PakIsPure
=================
*/
bool idFileSystemLocal::PakIsPure( pack_t* pack )
{
    S32 i;
    
    if ( fs_numServerPaks )
    {
        for ( i = 0; i < fs_numServerPaks; i++ )
        {
            // FIXME: also use hashed file names
            // NOTE TTimo: a pk3 with same checksum but different name would be validated too
            // I don't see this as allowing for any exploit, it would only happen if the client does manips of it's file names 'not a bug'
            if ( pack->checksum == fs_serverPaks[i] )
            {
                return true; // on the approved list
            }
        }
        
        return false; // not on the pure server pak list
    }
    return true;
}


/*
=================
idFileSystemLocal::LoadStack

return load stack
=================
*/
S32 idFileSystemLocal::LoadStack( void )
{
    return fs_loadStack;
}

/*
================
idFileSystemLocal::HashFileName

return a hash value for the filename
================
*/
S64 idFileSystemLocal::HashFileName( StringEntry fname, size_t hashSize )
{
    S32 i;
    S64 hash;
    UTF8 letter;
    
    hash = 0;
    i = 0;
    
    while ( fname[i] != '\0' )
    {
        letter = tolower( fname[i] );
        if ( letter == '.' )
        {
            break; // don't include extension
        }
        if ( letter == '\\' )
        {
            letter = '/'; // damn path names
        }
        if ( letter == PATH_SEP )
        {
            letter = '/'; // damn path names
        }
        hash += ( S64 )( letter ) * ( i + 119 );
        i++;
    }
    
    hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
    hash &= ( hashSize - 1 );
    
    return hash;
}

/*
================
idFileSystemLocal::HandleForFile
================
*/
fileHandle_t idFileSystemLocal::HandleForFile( void )
{
    for ( S32 i = 1; i < MAX_FILE_HANDLES; i++ )
    {
        if ( fsh[i].handleFiles.file.o == NULL )
        {
            return i;
        }
    }
    Com_Error( ERR_DROP, "idFileSystemLocal::HandleForFile: none free" );
    return 0;
}

/*
================
idFileSystemLocal::FileForHandle
================
*/
FILE* idFileSystemLocal::FileForHandle( fileHandle_t f )
{
    if ( f < 0 || f > MAX_FILE_HANDLES )
    {
        Com_Error( ERR_DROP, "idFileSystemLocal::FileForHandle: %d out of range", f );
    }
    
    if ( fsh[f].zipFile == true )
    {
        Com_Error( ERR_DROP, "idFileSystemLocal::FileForHandle: can't get FILE on zip file" );
    }
    
    if ( !fsh[f].handleFiles.file.o )
    {
        Com_Error( ERR_DROP, "idFileSystemLocal::FileForHandle: NULL" );
    }
    
    return fsh[f].handleFiles.file.o;
}

/*
================
idFileSystemLocal::ForceFlush
================
*/
void idFileSystemLocal::ForceFlush( fileHandle_t f )
{
    FILE* file = FileForHandle( f );
    setvbuf( file, NULL, _IONBF, 0 );
}

/*
================
idFileSystemLocal::filelength

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
S32 idFileSystemLocal::filelength( fileHandle_t f )
{
    S32 pos, end;
    FILE* h;
    
    h = FileForHandle( f );
    pos = ftell( h );
    
    fseek( h, 0, SEEK_END );
    end = ftell( h );
    
    fseek( h, pos, SEEK_SET );
    
    return end;
}

/*
====================
idFileSystemLocal::ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void idFileSystemLocal::ReplaceSeparators( UTF8* path )
{
    UTF8* s;
    
    for ( s = path; *s; s++ )
    {
        if ( *s == '/' || *s == '\\' )
        {
            *s = PATH_SEP;
        }
    }
}

/*
===================
idFileSystemLocal::BuildOSPath

Qpath may have either forward or backwards slashes
===================
*/
UTF8* idFileSystemLocal::BuildOSPath( StringEntry base, StringEntry game, StringEntry qpath )
{
    UTF8 temp[MAX_OSPATH];
    static UTF8 ospath[2][MAX_OSPATH];
    static S32 toggle;
    
    toggle ^= 1;        // flip-flop to allow two returns without clash
    
    if ( !game || !game[0] )
    {
        game = fs_gamedir;
    }
    
    Q_snprintf( temp, sizeof( temp ), "/%s/%s", game, qpath );
    
    ReplaceSeparators( temp );
    
    Q_snprintf( ospath[toggle], sizeof( ospath[0] ), "%s%s", base, temp );
    
    return ospath[toggle];
}

/*
=====================
idFileSystemLocal::BuildOSHomePath

return a path to a file in the users homepath
=====================
*/
void idFileSystemLocal::BuildOSHomePath( UTF8* ospath, S32 size, S32 qpath )
{
    Q_snprintf( ospath, size, "%s/%s/%s", fs_homepath->string, fs_gamedir, qpath );
    ReplaceSeparators( ospath );
}

/*
============
idFileSystemLocal::CreatePath

Creates any directories needed to store the given filename
============
*/
S32 idFileSystemLocal::CreatePath( StringEntry OSPath_ )
{
    // use va() to have a clean StringEntry prototype
    UTF8* OSPath = va( "%s", OSPath_ );
    UTF8* ofs;
    
    // make absolutely sure that it can't back up the path
    // FIXME: is c: allowed???
    if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) )
    {
        Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
        return true;
    }
    
    for ( ofs = OSPath + 1; *ofs; ofs++ )
    {
        if ( *ofs == PATH_SEP )
        {
            // create the directory
            *ofs = 0;
            idsystem->Mkdir( OSPath );
            *ofs = PATH_SEP;
        }
    }
    return false;
}

/*
=================
idFileSystemLocal::FSCopyFile

Copy a fully specified file from one place to another
=================
*/
void idFileSystemLocal::FSCopyFile( UTF8* fromOSPath, UTF8* toOSPath )
{
    FILE* f;
    S32 len;
    U8* buf;
    
    Com_Printf( "copy %s to %s\n", fromOSPath, toOSPath );
    
    if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) )
    {
        Com_Printf( "Ignoring journal files\n" );
        return;
    }
    
    f = fopen( fromOSPath, "rb" );
    if ( !f )
    {
        return;
    }
    
    fseek( f, 0, SEEK_END );
    len = ftell( f );
    fseek( f, 0, SEEK_SET );
    
    buf = ( U8* )memorySystem->Malloc( len );
    
    if ( fread( buf, 1, len, f ) != len )
    {
        Com_Error( ERR_FATAL, "Short read in idFileSystemLocal::Copyfiles()\n" );
    }
    
    fclose( f );
    
    if ( CreatePath( toOSPath ) )
    {
        memorySystem->Free( buf );
        return;
    }
    
    f = fopen( toOSPath, "wb" );
    if ( !f )
    {
        memorySystem->Free( buf );  //DAJ free as well
        return;
    }
    
    if ( fwrite( buf, 1, len, f ) != len )
    {
        Com_Error( ERR_FATAL, "Short write in idFileSystemLocal::Copyfiles()\n" );
    }
    
    fclose( f );
    
    memorySystem->Free( buf );
}

/*
===========
idFileSystemLocal::Remove
===========
*/
bool idFileSystemLocal::Remove( StringEntry osPath )
{
    return ( bool )!remove( osPath );
}

/*
===========
idFileSystemLocal::HomeRemove
===========
*/
void idFileSystemLocal::HomeRemove( StringEntry homePath )
{
    remove( BuildOSPath( fs_homepath->string, fs_gamedir, homePath ) );
}

/*
================
idFileSystemLocal::FileExists

Tests if the file exists in the current gamedir, this DOES NOT
search the paths.  This is to determine if opening a file to write
(which always goes into the current gamedir) will cause any overwrites.
NOTE TTimo: this goes with idFileSystemLocal::FOpenFileWrite for opening the file afterwards
================
*/
bool idFileSystemLocal::FileExists( StringEntry file )
{
    FILE* f;
    UTF8* testpath;
    
    testpath = BuildOSPath( fs_homepath->string, fs_gamedir, file );
    
    f = fopen( testpath, "rb" );
    
    if ( f )
    {
        fclose( f );
        return true;
    }
    
    return false;
}

/*
================
idFileSystemLocal::SV_FileExists

Tests if the file exists
================
*/
bool idFileSystemLocal::SV_FileExists( StringEntry file )
{
    FILE* f;
    UTF8* testpath;
    
    testpath = fileSystemLocal.BuildOSPath( fs_homepath->string, file, "" );
    testpath[strlen( testpath ) - 1] = '\0';
    
    f = fopen( testpath, "rb" );
    
    if ( f )
    {
        fclose( f );
        return true;
    }
    
    return false;
}

/*
===========
idFileSystemLocal::SV_FOpenFileWrite
===========
*/
fileHandle_t idFileSystemLocal::SV_FOpenFileWrite( StringEntry filename )
{
    UTF8* ospath;
    fileHandle_t f;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::SV_FOpenFileWrite: Filesystem call made without initialization\n" );
    }
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, filename, "" );
    ospath[strlen( ospath ) - 1] = '\0';
    
    f = HandleForFile();
    fsh[f].zipFile = false;
    
    if ( fs_debug->integer )
    {
        Com_Printf( "FS_SV_FOpenFileWrite: %s\n", ospath );
    }
    
    if ( CreatePath( ospath ) )
    {
        return 0;
    }
    
    Com_DPrintf( "idFileSystemLocal::SV_FOpenFileWrite: writing to: %s\n", ospath );
    fsh[f].handleFiles.file.o = fopen( ospath, "wb" );
    
    Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );
    
    fsh[f].handleSync = false;
    
    if ( !fsh[f].handleFiles.file.o )
    {
        f = 0;
    }
    
    return f;
}

/*
===========
idFileSystemLocal::SV_FOpenFileRead
search for a file somewhere below the home path, base path or cd path
we search in that order, matching idFileSystemLocal::SV_FOpenFileRead order
===========
*/
S32 idFileSystemLocal::SV_FOpenFileRead( StringEntry filename, fileHandle_t* fp )
{
    UTF8* ospath;
    fileHandle_t f = 0;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::SV_FOpenFileRead: Filesystem call made without initialization\n" );
    }
    
    f = HandleForFile();
    fsh[f].zipFile = false;
    
    Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );
    
    // don't let sound stutter
    //S_ClearSoundBuffer();
    
    // search homepath
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, filename, "" );
    // remove trailing slash
    ospath[strlen( ospath ) - 1] = '\0';
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::SV_FOpenFileRead (fs_homepath): %s\n", ospath );
    }
    
    fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
    fsh[f].handleSync = false;
    
    if ( !fsh[f].handleFiles.file.o )
    {
        // NOTE TTimo on non *nix systems, fs_homepath == fs_basepath, might want to avoid
        if ( Q_stricmp( fs_homepath->string, fs_basepath->string ) )
        {
            // search basepath
            ospath = fileSystemLocal.BuildOSPath( fs_basepath->string, filename, "" );
            ospath[strlen( ospath ) - 1] = '\0';
            
            if ( fs_debug->integer )
            {
                Com_Printf( "idFileSystemLocal::SV_FOpenFileRead (fs_basepath): %s\n", ospath );
            }
            
            fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
            fsh[f].handleSync = false;
            
            if ( !fsh[f].handleFiles.file.o )
            {
                f = 0;
            }
        }
    }
    
    *fp = f;
    if ( f )
    {
        return filelength( f );
    }
    
    return 0;
}

/*
===========
idFileSystemLocal::SV_Rename
===========
*/
void idFileSystemLocal::SV_Rename( StringEntry from, StringEntry to )
{
    UTF8* from_ospath, * to_ospath;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::SV_Rename: Filesystem call made without initialization\n" );
    }
    
    // don't let sound stutter
    //S_ClearSoundBuffer();
    
    from_ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, from, "" );
    to_ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, to, "" );
    from_ospath[strlen( from_ospath ) - 1] = '\0';
    to_ospath[strlen( to_ospath ) - 1] = '\0';
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::SV_Rename: %s --> %s\n", from_ospath, to_ospath );
    }
    
    if ( rename( from_ospath, to_ospath ) )
    {
        // Failed, try copying it and deleting the original
        FSCopyFile( from_ospath, to_ospath );
        Remove( from_ospath );
    }
}

/*
===========
idFileSystemLocal::Rename
===========
*/
void idFileSystemLocal::Rename( StringEntry from, StringEntry to )
{
    UTF8* from_ospath, * to_ospath;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::Rename: Filesystem call made without initialization\n" );
    }
    
    // don't let sound stutter
    //S_ClearSoundBuffer();
    
    from_ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, from );
    to_ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, to );
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::Rename: %s --> %s\n", from_ospath, to_ospath );
    }
    
    if ( rename( from_ospath, to_ospath ) )
    {
        // Failed, try copying it and deleting the original
        FSCopyFile( from_ospath, to_ospath );
        Remove( from_ospath );
    }
}

/*
==============
idFileSystemLocal::FCloseFile

If the FILE pointer is an open pak file, leave it open.

For some reason, other dll's can't just cal fclose()
on files returned by idFileSystemLocal::FOpenFile...
==============
*/
void idFileSystemLocal::FCloseFile( fileHandle_t f )
{
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FCloseFile: Filesystem call made without initialization\n" );
    }
    
    if ( fsh[f].zipFile == true )
    {
        unzCloseCurrentFile( fsh[f].handleFiles.file.z );
        
        if ( fsh[f].handleFiles.unique )
        {
            unzClose( fsh[f].handleFiles.file.z );
        }
        ::memset( &fsh[f], 0, sizeof( fsh[f] ) );
        
        return;
    }
    
    // we didn't find it as a pak, so close it as a unique file
    if ( fsh[f].handleFiles.file.o )
    {
        fclose( fsh[f].handleFiles.file.o );
    }
    
    ::memset( &fsh[f], 0, sizeof( fsh[f] ) );
}

/*
===========
idFileSystemLocal::FOpenFileWrite
===========
*/
fileHandle_t idFileSystemLocal::FOpenFileWrite( StringEntry filename )
{
    UTF8* ospath;
    fileHandle_t f;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileWrite: Filesystem call made without initialization\n" );
    }
    
    f = HandleForFile();
    fsh[f].zipFile = false;
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, filename );
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::FOpenFileWrite: %s\n", ospath );
    }
    
    if ( CreatePath( ospath ) )
    {
        return 0;
    }
    
    // enabling the following line causes a recursive function call loop
    // when running with +set logfile 1 +set developer 1
    //Com_DPrintf( "writing to: %s\n", ospath );
    fsh[f].handleFiles.file.o = fopen( ospath, "wb" );
    
    Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );
    
    fsh[f].handleSync = false;
    
    if ( !fsh[f].handleFiles.file.o )
    {
        f = 0;
    }
    
    return f;
}

/*
===========
idFileSystemLocal::FOpenFileAppend
===========
*/
fileHandle_t idFileSystemLocal::FOpenFileAppend( StringEntry filename )
{
    UTF8* ospath;
    fileHandle_t f;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
    }
    
    f = HandleForFile();
    fsh[f].zipFile = false;
    
    Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );
    
    // don't let sound stutter
    //S_ClearSoundBuffer();
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, filename );
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::FOpenFileAppend: %s\n", ospath );
    }
    
    if ( CreatePath( ospath ) )
    {
        return 0;
    }
    
    fsh[f].handleFiles.file.o = fopen( ospath, "ab" );
    fsh[f].handleSync = false;
    
    if ( !fsh[f].handleFiles.file.o )
    {
        f = 0;
    }
    
    return f;
}

/*
===========
idFileSystemLocal::FOpenFileWrite
===========
*/
S32 idFileSystemLocal::FOpenFileDirect( StringEntry filename, fileHandle_t* f )
{
    S32 r;
    UTF8* ospath;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileDirect: Filesystem call made without initialization\n" );
    }
    
    *f = HandleForFile();
    fsh[*f].zipFile = false;
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, filename );
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::FOpenFileDirect: %s\n", ospath );
    }
    
    // enabling the following line causes a recursive function call loop
    // when running with +set logfile 1 +set developer 1
    //Com_DPrintf( "writing to: %s\n", ospath );
    fsh[*f].handleFiles.file.o = fopen( ospath, "rb" );
    
    if ( !fsh[*f].handleFiles.file.o )
    {
        *f = 0;
        return 0;
    }
    
    fseek( fsh[*f].handleFiles.file.o, 0, SEEK_END );
    r = ftell( fsh[*f].handleFiles.file.o );
    fseek( fsh[*f].handleFiles.file.o, 0, SEEK_SET );
    
    return r;
}

/*
===========
idFileSystemLocal::FOpenFileUpdate
===========
*/
fileHandle_t idFileSystemLocal::FOpenFileUpdate( StringEntry filename, S32* length )
{
    UTF8* ospath;
    fileHandle_t f;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileUpdate: Filesystem call made without initialization\n" );
    }
    
    f = HandleForFile();
    fsh[f].zipFile = false;
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, filename );
    
    if ( fs_debug->integer )
    {
        Com_Printf( "idFileSystemLocal::FOpenFileWrite: %s\n", ospath );
    }
    
    if ( fileSystemLocal.CreatePath( ospath ) )
    {
        return 0;
    }
    
    // enabling the following line causes a recursive function call loop
    // when running with +set logfile 1 +set developer 1
    //Com_DPrintf( "writing to: %s\n", ospath );
    fsh[f].handleFiles.file.o = fopen( ospath, "wb" );
    
    if ( !fsh[f].handleFiles.file.o )
    {
        f = 0;
    }
    
    return f;
}

/*
===========
idFileSystemLocal::FilenameCompare

Ignore case and seprator UTF8 distinctions
===========
*/
bool idFileSystemLocal::FilenameCompare( StringEntry s1, StringEntry s2 )
{
    S32 c1, c2;
    
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        
        if ( c1 >= 'a' && c1 <= 'z' )
        {
            c1 -= ( 'a' - 'A' );
        }
        if ( c2 >= 'a' && c2 <= 'z' )
        {
            c2 -= ( 'a' - 'A' );
        }
        
        if ( c1 == '\\' || c1 == ':' )
        {
            c1 = '/';
        }
        if ( c2 == '\\' || c2 == ':' )
        {
            c2 = '/';
        }
        
        if ( c1 != c2 )
        {
            return ( bool ) - 1; // strings not equal
        }
    }
    while ( c1 );
    
    return false; // strings are equal
}

/*
===========
idFileSystemLocal::ShiftedStrStr
===========
*/
UTF8* idFileSystemLocal::ShiftedStrStr( StringEntry string, StringEntry substring, S32 shift )
{
    UTF8 buf[MAX_STRING_TOKENS];
    S32 i;
    
    for ( i = 0; substring[i]; i++ )
    {
        buf[i] = substring[i] + shift;
    }
    buf[i] = '\0';
    
    return ( UTF8* )strstr( string, buf );
}

/*
==========
idFileSystemLocal::ShiftStr

perform simple string shifting to avoid scanning from the exe
==========
*/
UTF8* idFileSystemLocal::ShiftStr( StringEntry string, S32 shift )
{
    static UTF8 buf[MAX_STRING_CHARS];
    S32 i, l;
    
    l = ( S32 )::strlen( string );
    
    for ( i = 0; i < l; i++ )
    {
        buf[i] = string[i] + shift;
    }
    
    buf[i] = '\0';
    
    return buf;
}

/*
===========
idFileSystemLocal::FOpenFileRead

Finds the file in the search path.
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
S32 idFileSystemLocal::FOpenFileRead( StringEntry filename, fileHandle_t* file, bool uniqueFILE )
{
    searchpath_t* search;
    UTF8* netpath;
    pack_t* pak;
    fileInPack_t* pakFile;
    directory_t* dir;
    S64 hash = 0;
    FILE* temp;
    S32 l;
    UTF8 demoExt[16];
    
    hash = 0;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileRead: Filesystem call made without initialization\n" );
    }
    
    // TTimo - NOTE
    // when checking for file existence, it's probably safer to use idFileSystemLocal::FileExists, as I'm not
    // sure this chunk of code is really up to date with everything
    if ( file == NULL )
    {
        // just wants to see if file is there
        for ( search = fs_searchpaths; search; search = search->next )
        {
            //
            if ( search->pack )
            {
                hash = HashFileName( filename, search->pack->hashSize );
            }
            // is the element a pak file?
            if ( search->pack && search->pack->hashTable[hash] )
            {
                if ( fs_filter_flag & FS_EXCLUDE_PK3 )
                {
                    continue;
                }
                
                // look through all the pak file elements
                pak = search->pack;
                pakFile = pak->hashTable[hash];
                do
                {
                    // case and separator insensitive comparisons
                    if ( !FilenameCompare( pakFile->name, filename ) )
                    {
                        // found it!
                        return true;
                    }
                    pakFile = pakFile->next;
                }
                while ( pakFile != NULL );
            }
            else if ( search->dir )
            {
                if ( fs_filter_flag & FS_EXCLUDE_DIR )
                {
                    continue;
                }
                
                dir = search->dir;
                
                netpath = fileSystemLocal.BuildOSPath( dir->path, dir->gamedir, filename );
                temp = fopen( netpath, "rb" );
                
                if ( !temp )
                {
                    continue;
                }
                
                fclose( temp );
                
                return true;
            }
        }
        return false;
    }
    
    if ( !filename )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileRead: NULL 'filename' parameter passed\n" );
    }
    
    //Q_snprintf( demoExt, sizeof( demoExt ), ".dm_%d",PROTOCOL_VERSION );
    // qpaths are not supposed to have a leading slash
    if ( filename[0] == '/' || filename[0] == '\\' )
    {
        filename++;
    }
    
    // make absolutely sure that it can't back up the path.
    // The searchpaths do guarantee that something will always
    // be prepended, so we don't need to worry about "c:" or "//limbo"
    if ( strstr( filename, ".." ) || strstr( filename, "::" ) )
    {
        *file = 0;
        return -1;
    }
    
    // make sure the q3key file is only readable by the quake3.exe at initialization
    // any other time the key should only be accessed in memory using the provided functions
    if ( com_fullyInitialized && strstr( filename, "owkey" ) )
    {
        *file = 0;
        return -1;
    }
    
    // search through the path, one element at a time
    *file = HandleForFile();
    fsh[*file].handleFiles.unique = uniqueFILE;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( search->pack )
        {
            hash = HashFileName( filename, search->pack->hashSize );
        }
        
        // is the element a pak file?
        if ( search->pack && search->pack->hashTable[hash] )
        {
            if ( fs_filter_flag & FS_EXCLUDE_PK3 )
            {
                continue;
            }
            
            // disregard if it doesn't match one of the allowed pure pak files
            if ( !PakIsPure( search->pack ) )
            {
                continue;
            }
            
            // look through all the pak file elements
            pak = search->pack;
            pakFile = pak->hashTable[hash];
            do
            {
                // case and separator insensitive comparisons
                if ( !FilenameCompare( pakFile->name, filename ) )
                {
                    // found it!
                    
                    // mark the pak as having been referenced and mark specifics on cgame and ui
                    // shaders, txt, arena files  by themselves do not count as a reference as
                    // these are loaded from all pk3s
                    // from every pk3 file..
                    l = ( S32 )::strlen( filename );
                    if ( !( pak->referenced & FS_GENERAL_REF ) )
                    {
                        if ( Q_stricmp( filename + l - 7, ".shader" ) != 0 &&
                                Q_stricmp( filename + l - 4, ".mtr" ) != 0 &&
                                Q_stricmp( filename + l - 4, ".txt" ) != 0 &&
                                Q_stricmp( filename + l - 4, ".ttf" ) != 0 &&
                                Q_stricmp( filename + l - 4, ".otf" ) != 0 &&
                                Q_stricmp( filename + l - 4, ".cfg" ) != 0 &&
                                Q_stricmp( filename + l - 7, ".config" ) != 0 &&
                                strstr( filename, "levelshots" ) == NULL &&
                                Q_stricmp( filename + l - 4, ".bot" ) != 0 &&
                                Q_stricmp( filename + l - 6, ".arena" ) != 0 &&
                                Q_stricmp( filename + l - 5, ".menu" ) != 0 )
                        {
                            // hack to work around issue of com_logfile set and this being first thing logged
                            fsh[*file].handleFiles.file.z = ( unzFile ) - 1;
                            Com_DPrintf( "Referencing %s due to file %s opened\n", pak->pakFilename, filename );
                            fsh[*file].handleFiles.file.z = ( unzFile )0;
                            pak->referenced |= FS_GENERAL_REF;
                        }
                    }
                    
                    // for OS client/server interoperability, we expect binaries for .so and .dll to be in the same pk3
                    // so that when we reference the DLL files on any platform, this covers everyone else
                    
                    // qagame dll
                    if ( !( pak->referenced & FS_QAGAME_REF ) && !Q_stricmp( filename, idsystem->GetDLLName( "sgame" ) ) )
                    {
                        pak->referenced |= FS_QAGAME_REF;
                    }
                    // cgame dll
                    if ( !( pak->referenced & FS_CGAME_REF ) && !Q_stricmp( filename, idsystem->GetDLLName( "cgame" ) ) )
                    {
                        pak->referenced |= FS_CGAME_REF;
                    }
                    
                    if ( uniqueFILE )
                    {
                        // open a new file on the pakfile
                        fsh[*file].handleFiles.file.z = unzOpen( pak->pakFilename );
                        if ( fsh[*file].handleFiles.file.z == NULL )
                        {
                            Com_Error( ERR_FATAL, "Couldn't reopen %s", pak->pakFilename );
                        }
                    }
                    else
                    {
                        fsh[*file].handleFiles.file.z = pak->handle;
                    }
                    
                    Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
                    fsh[*file].zipFile = true;
                    
                    // set the file position in the zip file (also sets the current file info)
                    unzSetOffset( fsh[*file].handleFiles.file.z, pakFile->pos );
                    
                    // open the file in the zip
                    unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
                    fsh[*file].zipFilePos = pakFile->pos;
                    
                    if ( fs_debug->integer )
                    {
                        Com_Printf( "idFileSystemLocal::FOpenFileRead: %s (found in '%s')\n", filename, pak->pakFilename );
                    }
                    
                    return pakFile->len;
                }
                pakFile = pakFile->next;
                
            }
            while ( pakFile != NULL );
        }
        else if ( search->dir )
        {
            if ( fs_filter_flag & FS_EXCLUDE_DIR )
            {
                continue;
            }
            
            // check a file in the directory tree
            
            // if we are running restricted, or if the filesystem is configured for pure (fs_numServerPaks)
            // the only files we will allow to come from the directory are .cfg files
            l = ( S32 )::strlen( filename );
            if ( fs_restrict->integer || fs_numServerPaks )
            {
            
                if ( Q_stricmp( filename + l - 4, ".cfg" )     // for config files
                        && Q_stricmp( filename + l - 4, ".ttf" )
                        && Q_stricmp( filename + l - 4, ".otf" )
                        && Q_stricmp( filename + l - 5, ".menu" ) // menu files
                        && Q_stricmp( filename + l - 5, ".game" ) // menu files
                        && Q_stricmp( filename + l - ( S32 )::strlen( demoExt ), demoExt )	// menu files
                        && Q_stricmp( filename + l - 4, ".dat" ) // for journal files
                        && Q_stricmp( filename + l - 8, "bots.txt" )
                        && Q_stricmp( filename + l - 8, ".botents" )
#ifdef __MACOS__
                        // even when pure is on, let the server game be loaded
                        && Q_stricmp( filename, "qagame_mac" ) // Dushan - this is wrong now
#endif
                   )
                {
                    continue;
                }
            }
            
            dir = search->dir;
            
            netpath = fileSystemLocal.BuildOSPath( dir->path, dir->gamedir, filename );
            fsh[*file].handleFiles.file.o = fopen( netpath, "rb" );
            
            if ( !fsh[*file].handleFiles.file.o )
            {
                continue;
            }
            
            if ( Q_stricmp( filename + l - 4, ".cfg" )     // for config files
                    && Q_stricmp( filename + l - 4, ".ttf" ) != 0
                    && Q_stricmp( filename + l - 4, ".otf" ) != 0
                    && Q_stricmp( filename + l - 5, ".menu" ) // menu files
                    && Q_stricmp( filename + l - 5, ".game" ) // menu files
                    && Q_stricmp( filename + l - ( S32 )::strlen( demoExt ), demoExt ) // menu files
                    && Q_stricmp( filename + l - 4, ".dat" )
                    && Q_stricmp( filename + l - 8, ".botents" )
                    && !strstr( filename, "botfiles" ) ) // RF, need this for dev
            {
                fs_fakeChkSum = ( S32 )random();
            }
            
            Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
            fsh[*file].zipFile = false;
            if ( fs_debug->integer )
            {
                Com_Printf( "idFileSystemLocal::FOpenFileRead: %s (found in '%s/%s')\n", filename,
                            dir->path, dir->gamedir );
            }
            
            return filelength( *file );
        }
    }
    
    Com_DPrintf( "Can't find %s\n", filename );
    
    if ( fs_missing->integer && missingFiles )
    {
        fprintf( missingFiles, "%s\n", filename );
    }
    
    *file = 0;
    return -1;
}

/*
==========
idFileSystemLocal::FOpenFileRead_Filtered
perform simple string shifting to avoid scanning from the exe
==========
*/
S32 idFileSystemLocal::FOpenFileRead_Filtered( StringEntry qpath, fileHandle_t* file, bool uniqueFILE, S32 filter_flag )
{
    S32 ret;
    
    fs_filter_flag = filter_flag;
    ret = FOpenFileRead( qpath, file, uniqueFILE );
    fs_filter_flag = 0;
    
    return ret;
}

// TTimo
// relevant to client only
/*
==================
idFileSystemLocal::CL_ExtractFromPakFile

NERVE - SMF - Extracts the latest file from a pak file.

Compares packed file against extracted file. If no differences, does not copy.
This is necessary for exe/dlls which may or may not be locked.

NOTE TTimo:
  fullpath gives the full OS path to the dll that will potentially be loaded
    on win32 it's always in fs_basepath/<fs_game>/
    on linux it can be in fs_homepath/<fs_game>/ or fs_basepath/<fs_game>/
  the dll is extracted to fs_homepath (== fs_basepath on win32) if needed

  the return value doesn't tell wether file was extracted or not, it just says wether it's ok to continue
  (i.e. either the right file was extracted successfully, or it was already present)

  cvar_lastVersion is the optional name of a CVAR_ARCHIVE used to store the wolf version for the last extracted .so
  show_bug.cgi?id=463

==================
*/
bool idFileSystemLocal::CL_ExtractFromPakFile( StringEntry base, StringEntry gamedir, StringEntry filename )
{
    S32 srcLength, destLength;
    U8* srcData, * destData;
    bool needToCopy;
    FILE* destHandle;
    UTF8* fn;
    
    fn = fileSystemLocal.BuildOSPath( base, gamedir, filename );
    needToCopy = true;
    
    // read in compressed file
    srcLength = ReadFile( filename, ( void** )&srcData );
    
    // if its not in the pak, we bail
    if ( srcLength == -1 )
    {
        return false;
    }
    
    // read in local file
    destHandle = fopen( fn, "rb" );
    
    // if we have a local file, we need to compare the two
    if ( destHandle )
    {
        fseek( destHandle, 0, SEEK_END );
        destLength = ftell( destHandle );
        fseek( destHandle, 0, SEEK_SET );
        
        if ( destLength > 0 )
        {
            destData = ( U8* )memorySystem->Malloc( destLength );
            
            fread( destData, destLength, 1, destHandle );
            
            // compare files
            if ( destLength == srcLength )
            {
                S32 i;
                
                for ( i = 0; i < destLength; i++ )
                {
                    if ( destData[i] != srcData[i] )
                    {
                        break;
                    }
                }
                
                if ( i == destLength )
                {
                    needToCopy = false;
                }
            }
            
            memorySystem->Free( destData ); // TTimo
        }
        
        fclose( destHandle );
    }
    
    // write file
    if ( needToCopy )
    {
        fileHandle_t f;
        
        f = FOpenFileWrite( filename );
        if ( !f )
        {
            Com_Printf( "Failed to open %s\n", filename );
            return false;
        }
        
        Write( srcData, srcLength, f );
        
        FCloseFile( f );
    }
    
    FreeFile( srcData );
    return true;
}

/*
==============
idFileSystemLocal::AllowDeletion
==============
*/
bool idFileSystemLocal::AllowDeletion( UTF8* filename )
{
    // for safety, only allow deletion from the save, profiles and demo directory
    if ( Q_strncmp( filename, "save/", 5 ) != 0 && Q_strncmp( filename, "profiles/", 9 ) != 0 && Q_strncmp( filename, "demos/", 6 ) != 0 )
    {
        return false;
    }
    
    return true;
}

/*
==============
idFileSystemLocal::DeleteDir
==============
*/
S32 idFileSystemLocal::DeleteDir( UTF8* dirname, bool nonEmpty, bool recursive )
{
    UTF8* ospath;
    UTF8** pFiles = NULL;
    S32 i, nFiles = 0;
    // Dushan
    static UTF8* root = "/";
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::DeleteDir: Filesystem call made without initialization\n" );
    }
    
    if ( !dirname || dirname[0] == 0 )
    {
        return 0;
    }
    
    if ( !AllowDeletion( dirname ) )
    {
        return 0;
    }
    
    if ( recursive )
    {
        ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, dirname );
        pFiles = idsystem->ListFiles( ospath, root, NULL, &nFiles, false );
        for ( i = 0; i < nFiles; i++ )
        {
            UTF8 temp[MAX_OSPATH];
            
            if ( !Q_stricmp( pFiles[i], ".." ) || !Q_stricmp( pFiles[i], "." ) )
            {
                continue;
            }
            
            Q_snprintf( temp, sizeof( temp ), "%s/%s", dirname, pFiles[i] );
            
            if ( !DeleteDir( temp, nonEmpty, recursive ) )
            {
                return 0;
            }
        }
        idsystem->FreeFileList( pFiles );
    }
    
    if ( nonEmpty )
    {
        ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, dirname );
        pFiles = idsystem->ListFiles( ospath, NULL, NULL, &nFiles, false );
        for ( i = 0; i < nFiles; i++ )
        {
            ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, va( "%s/%s", dirname, pFiles[i] ) );
            
            if ( remove( ospath ) == -1 ) // failure
            {
                return 0;
            }
        }
        idsystem->FreeFileList( pFiles );
    }
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, dirname );
    
    if ( Q_rmdir( ospath ) == 0 )
    {
        return 1;
    }
    
    return 0;
}

/*
==============
idFileSystemLocal::OSStatFile
Test an file given OS path:
returns -1 if not found
returns 1 if directory
returns 0 otherwise
==============
*/
S32 idFileSystemLocal::OSStatFile( UTF8* ospath )
{
#ifdef _WIN32
    struct _stat stat;
    if ( _stat( ospath, &stat ) == -1 )
#else
    struct stat stat_buf;
    if ( stat( ospath, &stat_buf ) == -1 )
#endif
    {
        return -1;
    }
#ifdef _WIN32
    if ( stat.st_mode & _S_IFDIR )
#else
    if ( S_ISDIR( stat_buf.st_mode ) )
#endif
    {
        return 1;
    }
    return 0;
}

/*
==============
idFileSystemLocal::Delete
TTimo - this was not in the 1.30 filesystem code
using fs_homepath for the file to remove
==============
*/
S32 idFileSystemLocal::Delete( UTF8* filename )
{
    UTF8* ospath;
    S32 stat;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::Delete: Filesystem call made without initialization\n" );
    }
    
    if ( !filename || filename[0] == 0 )
    {
        return 0;
    }
    
    if ( !AllowDeletion( filename ) )
    {
        return 0;
    }
    
    ospath = fileSystemLocal.BuildOSPath( fs_homepath->string, fs_gamedir, filename );
    
    stat = OSStatFile( ospath );
    if ( stat == -1 )
    {
        return 0;
    }
    
    if ( stat == 1 )
    {
        return( DeleteDir( filename, true, true ) );
    }
    else
    {
        if ( remove( ospath ) != -1 ) // success
        {
            return 1;
        }
    }
    
    return 0;
}

/*
================
idFileSystemLocal::FPrintf
================
*/
S32 idFileSystemLocal::FPrintf( fileHandle_t f, StringEntry fmt, ... )
{
    va_list argptr;
    UTF8 msg[8192];
    S32 l, r;
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
    va_end( argptr );
    
    l = ( S32 )::strlen( msg );
    
    r = fileSystemLocal.Write( msg, l, f );
    return r;
}

/*
=================
idFileSystemLocal::Read2

Properly handles partial reads
=================
*/
S32 idFileSystemLocal::Read2( void* buffer, S32 len, fileHandle_t f )
{
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::Read2: Filesystem call made without initialization\n" );
    }
    
    if ( !f )
    {
        return 0;
    }
    if ( fsh[f].streamed )
    {
        S32 r;
        
        fsh[f].streamed = false;
        
        r = Read( buffer, len, f );
        
        fsh[f].streamed = true;
        
        return r;
    }
    else
    {
        return Read( buffer, len, f );
    }
}

/*
================
idFileSystemLocal::Read
================
*/
S32 idFileSystemLocal::Read( void* buffer, S32 len, fileHandle_t f )
{
    S32 block, remaining, read, tries;
    U8* buf;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::Read: Filesystem call made without initialization\n" );
    }
    
    if ( !f )
    {
        return 0;
    }
    
    buf = ( U8* )buffer;
    fs_readCount += len;
    
    if ( fsh[f].zipFile == false )
    {
        remaining = len;
        tries = 0;
        
        while ( remaining )
        {
            block = remaining;
            
            //			read = fread (buf, block, 1, fsh[f].handleFiles.file.o);
            read = ( S32 )fread( buf, 1, block, fsh[f].handleFiles.file.o );
            
            if ( read == 0 )
            {
                // we might have been trying to read from a CD, which
                // sometimes returns a 0 read on windows
                if ( !tries )
                {
                    tries = 1;
                }
                else
                {
                    return len - remaining;   //Com_Error (ERR_FATAL, "idFileSystemLocal::Read: 0 bytes read");
                }
            }
            
            if ( read == -1 )
            {
                Com_Error( ERR_FATAL, "idFileSystemLocal::Read: -1 bytes read" );
            }
            
            remaining -= read;
            buf += read;
        }
        return len;
    }
    else
    {
        return unzReadCurrentFile( fsh[f].handleFiles.file.z, buffer, len );
    }
}

/*
=================
idFileSystemLocal::Write

Properly handles partial writes
=================
*/
S32 idFileSystemLocal::Write( const void* buffer, S32 len, fileHandle_t h )
{
    S32 block, remaining, written, tries;
    U8* buf;
    FILE* f;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::Write: Filesystem call made without initialization\n" );
    }
    
    if ( !h )
    {
        return 0;
    }
    
    f = FileForHandle( h );
    buf = ( U8* )buffer;
    
    remaining = len;
    tries = 0;
    
    while ( remaining )
    {
        block = remaining;
        written = ( S32 )fwrite( buf, 1, block, f );
        if ( written == 0 )
        {
            if ( !tries )
            {
                tries = 1;
            }
            else
            {
                Com_Printf( "idFileSystemLocal::Write: 0 bytes written (%d attempted)\n", block );
                return 0;
            }
        }
        
        if ( written < 0 )
        {
            Com_Printf( "idFileSystemLocal::Write: %d bytes written (%d attempted)\n", written, block );
            return 0;
        }
        
        remaining -= written;
        buf += written;
    }
    
    if ( fsh[h].handleSync )
    {
        fflush( f );
    }
    
    return len;
}

/*
================
idFileSystemLocal::Printf
================
*/
void idFileSystemLocal::Printf( fileHandle_t h, StringEntry fmt, ... )
{
    va_list argptr;
    UTF8 msg[MAXPRINTMSG];
    
    va_start( argptr, fmt );
    Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
    va_end( argptr );
    
    Write( msg, ( S32 )::strlen( msg ), h );
}

/*
=================
idFileSystemLocal::Seek
=================
*/
S32 idFileSystemLocal::Seek( fileHandle_t f, S64 offset, S32 origin )
{
    S32 _origin;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::Seek: Filesystem call made without initialization\n" );
        return -1;
    }
    
    if ( fsh[f].streamed )
    {
        fsh[f].streamed = false;
        Seek( f, offset, origin );
        fsh[f].streamed = true;
    }
    
    if ( fsh[f].zipFile == true )
    {
        //FIXME: this is incomplete and really, really
        //crappy (but better than what was here before)
        U8 buffer[PK3_SEEK_BUFFER_SIZE];
        S32 remainder = offset;
        
        if ( offset < 0 || origin == FS_SEEK_END )
        {
            Com_Error( ERR_FATAL, "Negative offsets and FS_SEEK_END not implemented "
                       "for idFileSystemLocal::Seek on pk3 file contents\n" );
            return -1;
        }
        
        switch ( origin )
        {
            case FS_SEEK_SET:
                unzSetOffset( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
                unzOpenCurrentFile( fsh[f].handleFiles.file.z );
                //fallthrough
                
            case FS_SEEK_CUR:
                while ( remainder > PK3_SEEK_BUFFER_SIZE )
                {
                    Read( buffer, PK3_SEEK_BUFFER_SIZE, f );
                    remainder -= PK3_SEEK_BUFFER_SIZE;
                }
                
                Read( buffer, remainder, f );
                
                return offset;
                break;
                
            default:
                Com_Error( ERR_FATAL, "Bad origin in idFileSystemLocal::Seek\n" );
                return -1;
                break;
        }
    }
    else
    {
        FILE* file;
        file = FileForHandle( f );
        
        switch ( origin )
        {
            case FS_SEEK_CUR:
                _origin = SEEK_CUR;
                break;
                
            case FS_SEEK_END:
                _origin = SEEK_END;
                break;
                
            case FS_SEEK_SET:
                _origin = SEEK_SET;
                break;
                
            default:
                _origin = SEEK_CUR;
                Com_Error( ERR_FATAL, "Bad origin in idFileSystemLocal::Seek\n" );
                break;
        }
        
        return fseek( file, offset, _origin );
    }
}

/*
======================================================================================
CONVENIENCE FUNCTIONS FOR ENTIRE FILES
======================================================================================
*/

/*
================
idFileSystemLocal::FileIsInPAK
================
*/
S32 idFileSystemLocal::FileIsInPAK( StringEntry filename, S32* pChecksum )
{
    searchpath_t* search;
    pack_t* pak;
    fileInPack_t* pakFile;
    S64 hash = 0;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FileIsInPAK: Filesystem call made without initialization\n" );
    }
    
    if ( !filename )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileRead: NULL 'filename' parameter passed\n" );
    }
    
    // qpaths are not supposed to have a leading slash
    if ( filename[0] == '/' || filename[0] == '\\' )
    {
        filename++;
    }
    
    // make absolutely sure that it can't back up the path.
    // The searchpaths do guarantee that something will always
    // be prepended, so we don't need to worry about "c:" or "//limbo"
    if ( strstr( filename, ".." ) || strstr( filename, "::" ) )
    {
        return -1;
    }
    
    //
    // search through the path, one element at a time
    //
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        //
        if ( search->pack )
        {
            hash = HashFileName( filename, search->pack->hashSize );
        }
        
        // is the element a pak file?
        if ( search->pack && search->pack->hashTable[hash] )
        {
            // disregard if it doesn't match one of the allowed pure pak files
            if ( !PakIsPure( search->pack ) )
            {
                continue;
            }
            
            // look through all the pak file elements
            pak = search->pack;
            pakFile = pak->hashTable[hash];
            do
            {
                // case and separator insensitive comparisons
                if ( !FilenameCompare( pakFile->name, filename ) )
                {
                    if ( pChecksum )
                    {
                        *pChecksum = pak->pure_checksum;
                    }
                    // Mac hack
                    if ( pak->checksum == MP_LEGACY_PAK )
                    {
                        legacy_bin = true;
                    }
                    else
                    {
                        legacy_bin = false;
                    }
                    return 1;
                }
                pakFile = pakFile->next;
            }
            while ( pakFile != NULL );
        }
    }
    return -1;
}

/*
============
idFileSystemLocal::ReadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
S32 idFileSystemLocal::ReadFile( StringEntry qpath, void** buffer )
{
    fileHandle_t h;
    U8* buf;
    bool isConfig;
    S32 len;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::ReadFile: Filesystem call made without initialization\n" );
    }
    
    if ( !qpath || !qpath[0] )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::ReadFile with empty name\n" );
    }
    
    buf = NULL; // quiet compiler warning
    
    // if this is a .cfg file and we are playing back a journal, read
    // it from the journal file
    if ( strstr( qpath, ".cfg" ) )
    {
        isConfig = true;
        if ( com_journal && com_journal->integer == 2 )
        {
            S32 r;
            
            Com_DPrintf( "Loading %s from journal file.\n", qpath );
            
            r = Read( &len, sizeof( len ), com_journalDataFile );
            
            if ( r != sizeof( len ) )
            {
                if ( buffer != NULL )
                {
                    *buffer = NULL;
                }
                return -1;
            }
            
            // if the file didn't exist when the journal was created
            if ( !len )
            {
                if ( buffer == NULL )
                {
                    return 1;           // hack for old journal files
                }
                *buffer = NULL;
                
                return -1;
            }
            
            if ( buffer == NULL )
            {
                return len;
            }
            
            //buf = ( U8* )AllocateTempMemory( len + 1 );
            buf = ( U8* )memorySystem->Malloc( len + 1 );
            buf[len] = '\0';	// because we're not calling memorySystem->Malloc with optional trailing 'bZeroIt' bool
            *buffer = buf;
            
            r = Read( buf, len, com_journalDataFile );
            if ( r != len )
            {
                Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
            }
            
            fs_loadCount++;
            fs_loadStack++;
            
            // guarantee that it will have a trailing 0 for string operations
            buf[len] = 0;
            
            return len;
        }
    }
    else
    {
        isConfig = false;
    }
    
    // look for it in the filesystem or pack files
    len = FOpenFileRead( qpath, &h, false );
    if ( h == 0 )
    {
        if ( buffer )
        {
            *buffer = NULL;
        }
        // if we are journalling and it is a config file, write a zero to the journal file
        if ( isConfig && com_journal && com_journal->integer == 1 )
        {
            Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
            len = 0;
            Write( &len, sizeof( len ), com_journalDataFile );
            Flush( com_journalDataFile );
        }
        return -1;
    }
    
    if ( !buffer )
    {
        if ( isConfig && com_journal && com_journal->integer == 1 )
        {
            Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
            Write( &len, sizeof( len ), com_journalDataFile );
            Flush( com_journalDataFile );
        }
        FCloseFile( h );
        return len;
    }
    
    buf = ( U8* )memorySystem->AllocateTempMemory( len + 1 );
    *buffer = buf;
    
    Read( buf, len, h );
    
    fs_loadCount++;
    fs_loadStack++;
    
    // guarantee that it will have a trailing 0 for string operations
    buf[len] = 0;
    FCloseFile( h );
    
    // if we are journalling and it is a config file, write it to the journal file
    if ( isConfig && com_journal && com_journal->integer == 1 )
    {
        Com_DPrintf( "Writing %s to journal file.\n", qpath );
        Write( &len, sizeof( len ), com_journalDataFile );
        Write( buf, len, com_journalDataFile );
        Flush( com_journalDataFile );
    }
    return len;
}

/*
=============
idFileSystemLocal::FreeFile
=============
*/
void idFileSystemLocal::FreeFile( void* buffer )
{
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FreeFile: Filesystem call made without initialization\n" );
    }
    
    if ( !buffer )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FreeFile( NULL )" );
    }
    
    fs_loadStack--;
    
    memorySystem->FreeTempMemory( buffer );
    
    // if all of our temp files are free, clear all of our space
    if ( fs_loadStack == 0 )
    {
        memorySystem->ClearTempMemory();
    }
}

/*
============
idFileSystemLocal::WriteFile

Filename are reletive to the quake search path
============
*/
void idFileSystemLocal::WriteFile( StringEntry qpath, const void* buffer, S32 size )
{
    fileHandle_t f;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
    }
    
    if ( !qpath || !buffer )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::WriteFile: NULL parameter" );
    }
    
    f = FOpenFileWrite( qpath );
    if ( !f )
    {
        Com_Printf( "Failed to open %s\n", qpath );
        return;
    }
    
    Write( buffer, size, f );
    
    FCloseFile( f );
}

/*
==========================================================================
ZIP FILE LOADING
==========================================================================
*/

/*
=================
idFileSystemLocal::LoadZipFile

Creates a new pak_t in the search chain for the contents
of a zip file.
=================
*/
pack_t* idFileSystemLocal::LoadZipFile( StringEntry zipfile, StringEntry basename )
{
    S32 err, fs_numHeaderLongs, * fs_headerLongs, sizeOfHeaderLongs;
    size_t i, len;
    fileInPack_t* buildBuffer;
    pack_t* pack;
    unzFile uf;
    unz_global_info gi;
    UTF8 filename_inzip[MAX_ZPATH];
    unz_file_info file_info;
    S64	hash;
    UTF8* namePtr;
    
    fs_numHeaderLongs = 0;
    
    uf = unzOpen( zipfile );
    err = unzGetGlobalInfo( uf, &gi );
    
    if ( err != UNZ_OK )
    {
        return NULL;
    }
    
    len = 0;
    unzGoToFirstFile( uf );
    
    for ( i = 0; i < gi.number_entry; i++ )
    {
        err = unzGetCurrentFileInfo( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );
        
        if ( err != UNZ_OK )
        {
            // it's better to fail and have the user notified than to have a half-loaded pk3,
            // or worse a failed pack referenced that results in further failures (yes it does happen)
            Com_Error( ERR_FATAL, "Corrupted pk3 file \'%s\'", basename );
            break;
        }
        
        filename_inzip[sizeof( filename_inzip ) - 1] = '\0';
        len += ( S32 )::strlen( filename_inzip ) + 1;
        
        unzGoToNextFile( uf );
    }
    
    buildBuffer = ( fileInPack_t* )memorySystem->Malloc( ( gi.number_entry * sizeof( fileInPack_t ) ) + len );
    namePtr = ( ( UTF8* )buildBuffer ) + gi.number_entry * sizeof( fileInPack_t );
    fs_headerLongs = ( S32* )memorySystem->Malloc( ( gi.number_entry + 1 ) * sizeof( S32 ) );
    fs_headerLongs[fs_numHeaderLongs++] = LittleLong( fs_checksumFeed );
    
    // get the hash table size from the number of files in the zip
    // because lots of custom pk3 files have less than 32 or 64 files
    for ( i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1 )
    {
        if ( i > gi.number_entry )
        {
            break;
        }
    }
    
    pack = ( pack_t* )memorySystem->Malloc( sizeof( pack_t ) + i * sizeof( fileInPack_t* ) );
    pack->hashSize = i;
    pack->hashTable = ( fileInPack_t** )( ( ( UTF8* )pack ) + sizeof( pack_t ) );
    
    for ( i = 0; i < pack->hashSize; i++ )
    {
        pack->hashTable[i] = NULL;
    }
    
    Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );
    Q_strncpyz( pack->pakBasename, basename, sizeof( pack->pakBasename ) );
    
    // strip .pk3 if needed
    if ( ( S32 )::strlen( pack->pakBasename ) > 4 && !Q_stricmp( pack->pakBasename + ( S32 )::strlen( pack->pakBasename ) - 4, ".pk3" ) )
    {
        pack->pakBasename[strlen( pack->pakBasename ) - 4] = 0;
    }
    
    pack->handle = uf;
    pack->numfiles = gi.number_entry;
    unzGoToFirstFile( uf );
    
    for ( i = 0; i < gi.number_entry; i++ )
    {
        err = unzGetCurrentFileInfo( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );
        
        if ( err != UNZ_OK )
        {
            Com_Error( ERR_FATAL, "Corrupted pk3 file \'%s\'", basename );
            break;
        }
        
        if ( file_info.uncompressed_size > 0 )
        {
            fs_headerLongs[fs_numHeaderLongs++] = LittleLong( file_info.crc );
        }
        
        Q_strlwr( filename_inzip );
        
        hash = HashFileName( filename_inzip, pack->hashSize );
        buildBuffer[i].name = namePtr;
        
        strcpy( buildBuffer[i].name, filename_inzip );
        
        namePtr += ( S32 )::strlen( filename_inzip ) + 1;
        
        // store the file position in the zip
        buildBuffer[i].pos = unzGetOffset( uf );
        buildBuffer[i].len = file_info.uncompressed_size;
        buildBuffer[i].next = pack->hashTable[hash];
        
        pack->hashTable[hash] = &buildBuffer[i];
        
        unzGoToNextFile( uf );
    }
    
    sizeOfHeaderLongs = sizeof( *fs_headerLongs );
    
    pack->checksum = MD4System->BlockChecksum( &fs_headerLongs[1], sizeof( *fs_headerLongs ) * ( fs_numHeaderLongs - 1 ) );
    pack->pure_checksum = MD4System->BlockChecksum( fs_headerLongs, sizeof( *fs_headerLongs ) * fs_numHeaderLongs );
    pack->checksum = LittleLong( pack->checksum );
    pack->pure_checksum = LittleLong( pack->pure_checksum );
    
    memorySystem->Free( fs_headerLongs );
    
    pack->buildBuffer = buildBuffer;
    return pack;
}

/*
=================================================================================
DIRECTORY SCANNING FUNCTIONS
=================================================================================
*/
S32 idFileSystemLocal::ReturnPath( StringEntry zname, UTF8* zpath, S32* depth )
{
    S32 len, at, newdep;
    
    newdep = 0;
    zpath[0] = 0;
    len = 0;
    at = 0;
    
    while ( zname[at] != 0 )
    {
        if ( zname[at] == '/' || zname[at] == '\\' )
        {
            len = at;
            newdep++;
        }
        at++;
    }
    
    strcpy( zpath, zname );
    
    zpath[len] = 0;
    *depth = newdep;
    
    return len;
}

/*
==================
idFileSystemLocal::AddFileToList
==================
*/
S32 idFileSystemLocal::AddFileToList( UTF8* name, UTF8* list[MAX_FOUND_FILES], S32 nfiles )
{
    S32 i;
    
    if ( nfiles == MAX_FOUND_FILES - 1 )
    {
        return nfiles;
    }
    
    for ( i = 0; i < nfiles; i++ )
    {
        if ( !Q_stricmp( name, list[i] ) )
        {
            return nfiles;      // allready in list
        }
    }
    
    list[nfiles] = memorySystem->CopyString( name );
    nfiles++;
    
    return nfiles;
}

/*
===============
idFileSystemLocal::ListFilteredFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
UTF8** idFileSystemLocal::ListFilteredFiles( StringEntry path, StringEntry extension, UTF8* filter, S32* numfiles )
{
    S32 nfiles, i, pathLength, extensionLength, length, pathDepth, temp;
    UTF8** listCopy, * list[MAX_FOUND_FILES];
    searchpath_t* search;
    pack_t* pak;
    fileInPack_t* buildBuffer;
    UTF8 zpath[MAX_ZPATH];
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::ListFilteredFiles: Filesystem call made without initialization\n" );
    }
    
    if ( !path )
    {
        *numfiles = 0;
        return NULL;
    }
    
    if ( !extension )
    {
        extension = "";
    }
    
    pathLength = ( S32 )::strlen( path );
    
    if ( path[pathLength - 1] == '\\' || path[pathLength - 1] == '/' )
    {
        pathLength--;
    }
    
    extensionLength = ( S32 )::strlen( extension );
    nfiles = 0;
    ReturnPath( path, zpath, &pathDepth );
    
    // search through the path, one element at a time, adding to list
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( search->pack )
        {
            //ZOID:  If we are pure, don't search for files on paks that
            // aren't on the pure list
            if ( !PakIsPure( search->pack ) )
            {
                continue;
            }
            
            // look through all the pak file elements
            pak = search->pack;
            buildBuffer = pak->buildBuffer;
            for ( i = 0; i < pak->numfiles; i++ )
            {
                UTF8* name;
                S32 zpathLen, depth;
                
                // check for directory match
                name = buildBuffer[i].name;
                
                if ( filter )
                {
                    // case insensitive
                    if ( !Com_FilterPath( filter, name, false ) )
                    {
                        continue;
                    }
                    // unique the match
                    nfiles = AddFileToList( name, list, nfiles );
                }
                else
                {
                    zpathLen = ReturnPath( name, zpath, &depth );
                    
                    if ( ( depth - pathDepth ) > 2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) )
                    {
                        continue;
                    }
                    
                    // check for extension match
                    length = ( S32 )::strlen( name );
                    
                    if ( length < extensionLength )
                    {
                        continue;
                    }
                    
                    if ( Q_stricmp( name + length - extensionLength, extension ) )
                    {
                        continue;
                    }
                    
                    // unique the match
                    temp = pathLength;
                    if ( pathLength )
                    {
                        temp++; // include the '/'
                    }
                    
                    nfiles = AddFileToList( name + temp, list, nfiles );
                }
            }
        }
        else if ( search->dir ) // scan for files in the filesystem
        {
            UTF8* netpath;
            S32 numSysFiles;
            UTF8** sysFiles;
            UTF8* name;
            
            // don't scan directories for files if we are pure or restricted
            if ( fs_numServerPaks )
            {
                continue;
            }
            else if ( fs_restrict->integer && ( !com_gameInfo.usesProfiles || ( com_gameInfo.usesProfiles && Q_stricmpn( path, "profiles", 8 ) ) ) &&
                      Q_stricmpn( path, "demos", 5 ) )
            {
                continue;
            }
            else
            {
                netpath = fileSystemLocal.BuildOSPath( search->dir->path, search->dir->gamedir, path );
                sysFiles = idsystem->ListFiles( netpath, extension, filter, &numSysFiles, false );
                
                for ( i = 0; i < numSysFiles; i++ )
                {
                    // unique the match
                    name = sysFiles[i];
                    nfiles = AddFileToList( name, list, nfiles );
                }
                
                idsystem->FreeFileList( sysFiles );
            }
        }
    }
    
    // return a copy of the list
    *numfiles = nfiles;
    
    if ( !nfiles )
    {
        return NULL;
    }
    
    listCopy = ( UTF8** )memorySystem->Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
    
    for ( i = 0; i < nfiles; i++ )
    {
        listCopy[i] = list[i];
    }
    listCopy[i] = NULL;
    
    return listCopy;
}

/*
=================
idFileSystemLocal::ListFiles
=================
*/
UTF8** idFileSystemLocal::ListFiles( StringEntry path, StringEntry extension, S32* numfiles )
{
    return ListFilteredFiles( path, extension, NULL, numfiles );
}

/*
=================
idFileSystemLocal::FreeFileList
=================
*/
void idFileSystemLocal::FreeFileList( UTF8** list )
{
    S32 i;
    
    if ( !fs_searchpaths )
    {
        Com_Error( ERR_FATAL, "idFileSystemLocal::FreeFileList: Filesystem call made without initialization\n" );
    }
    
    if ( !list )
    {
        return;
    }
    
    for ( i = 0; list[i]; i++ )
    {
        memorySystem->Free( list[i] );
    }
    
    memorySystem->Free( list );
}

/*
================
idFileSystemLocal::GetFileList
================
*/
S32 idFileSystemLocal::GetFileList( StringEntry path, StringEntry extension, UTF8* listbuf, S32 bufsize )
{
    S32 nFiles, i, nTotal, nLen;
    UTF8** pFiles = NULL;
    
    *listbuf = 0;
    nFiles = 0;
    nTotal = 0;
    
    if ( Q_stricmp( path, "$modlist" ) == 0 )
    {
        return GetModList( listbuf, bufsize );
    }
    
    pFiles = ListFiles( path, extension, &nFiles );
    
    for ( i = 0; i < nFiles; i++ )
    {
        nLen = ( S32 )::strlen( pFiles[i] ) + 1;
        
        if ( nTotal + nLen + 1 < bufsize )
        {
            strcpy( listbuf, pFiles[i] );
            listbuf += nLen;
            nTotal += nLen;
        }
        else
        {
            nFiles = i;
            break;
        }
    }
    
    FreeFileList( pFiles );
    
    return nFiles;
}

/*
=======================
idFileSystemLocal::ConcatenateFileLists

mkv: Naive implementation. Concatenates three lists into a
new list, and frees the old lists from the heap.
bk001129 - from cvs1.17 (mkv)

FIXME TTimo those two should move to common.c next to idServerLocal::ListFiles
=======================
*/
U32 idFileSystemLocal::CountFileList( UTF8** list )
{
    S32 i = 0;
    
    if ( list )
    {
        while ( *list )
        {
            list++;
            i++;
        }
    }
    return i;
}

/*
=======================
idFileSystemLocal::ConcatenateFileLists
=======================
*/
UTF8** idFileSystemLocal::ConcatenateFileLists( UTF8** list0, UTF8** list1, UTF8** list2 )
{
    S32 totalLength = 0;
    UTF8** cat = NULL, ** dst, ** src;
    
    totalLength += CountFileList( list0 );
    totalLength += CountFileList( list1 );
    totalLength += CountFileList( list2 );
    
    /* Create new list. */
    dst = cat = ( UTF8** )memorySystem->Malloc( ( totalLength + 1 ) * sizeof( UTF8* ) );
    
    /* Copy over lists. */
    if ( list0 )
    {
        for ( src = list0; *src; src++, dst++ )
        {
            *dst = *src;
        }
    }
    
    if ( list1 )
    {
        for ( src = list1; *src; src++, dst++ )
        {
            *dst = *src;
        }
    }
    
    if ( list2 )
    {
        for ( src = list2; *src; src++, dst++ )
        {
            *dst = *src;
        }
    }
    
    // Terminate the list
    *dst = NULL;
    
    // Free our old lists.
    // NOTE: not freeing their content, it's been merged in dst and still being used
    if ( list0 )
    {
        memorySystem->Free( list0 );
    }
    
    if ( list1 )
    {
        memorySystem->Free( list1 );
    }
    
    if ( list2 )
    {
        memorySystem->Free( list2 );
    }
    
    return cat;
}

/*
================
idFileSystemLocal::GetModList

Returns a list of mod directory names
A mod directory is a peer to baseq3 with a pk3 in it
The directories are searched in base path, cd path and home path
================
*/
S32 idFileSystemLocal::GetModList( UTF8* listbuf, S32 bufsize )
{
    S32 nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
    UTF8** pFiles = NULL;
    UTF8** pPaks = NULL;
    UTF8* name, * path;
    UTF8 descPath[MAX_OSPATH];
    fileHandle_t descHandle;
    
    S32 dummy;
    UTF8** pFiles0 = NULL;
    UTF8** pFiles1 = NULL;
    UTF8** pFiles2 = NULL;
    bool bDrop = false;
    
    *listbuf = 0;
    nMods = nPotential = nTotal = 0;
    
    pFiles0 = idsystem->ListFiles( fs_homepath->string, NULL, NULL, &dummy, true );
    pFiles1 = idsystem->ListFiles( fs_basepath->string, NULL, NULL, &dummy, true );
    
    // we searched for mods in the three paths
    // it is likely that we have duplicate names now, which we will cleanup below
    pFiles = ConcatenateFileLists( pFiles0, pFiles1, pFiles2 );
    nPotential = CountFileList( pFiles );
    
    for ( i = 0; i < nPotential; i++ )
    {
        name = pFiles[i];
        // NOTE: cleaner would involve more changes
        // ignore duplicate mod directories
        if ( i != 0 )
        {
            bDrop = false;
            
            for ( j = 0; j < i; j++ )
            {
                if ( Q_stricmp( pFiles[j], name ) == 0 )
                {
                    // this one can be dropped
                    bDrop = true;
                    break;
                }
            }
        }
        
        if ( bDrop )
        {
            continue;
        }
        
        // we drop "baseq3" "." and ".."
        if ( Q_stricmp( name, BASEGAME ) && Q_stricmpn( name, ".", 1 ) )
        {
            // now we need to find some .pk3 files to validate the mod
            // NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
            // we didn't keep the information when we merged the directory names, as to what OS Path it was found under
            // so it could be in base path, cd path or home path
            // we will try each three of them here (yes, it's a bit messy)
            // NOTE Arnout: what about dropping the current loaded mod as well?
            path = fileSystemLocal.BuildOSPath( fs_basepath->string, name, "" );
            nPaks = 0;
            pPaks = idsystem->ListFiles( path, ".pk3", NULL, &nPaks, false );
            idsystem->FreeFileList( pPaks ); // we only use idServerLocal::ListFiles to check wether .pk3 files are present
            
            /* try on home path */
            if ( nPaks <= 0 )
            {
                path = fileSystemLocal.BuildOSPath( fs_homepath->string, name, "" );
                nPaks = 0;
                pPaks = idsystem->ListFiles( path, ".pk3", NULL, &nPaks, false );
                idsystem->FreeFileList( pPaks );
            }
            
            if ( nPaks > 0 )
            {
                nLen = ( S32 )::strlen( name ) + 1;
                // nLen is the length of the mod path
                // we need to see if there is a description available
                descPath[0] = '\0';
                strcpy( descPath, name );
                strcat( descPath, "/description.txt" );
                nDescLen = fileSystemLocal.SV_FOpenFileRead( descPath, &descHandle );
                
                if ( nDescLen > 0 && descHandle )
                {
                    FILE* file;
                    file = FileForHandle( descHandle );
                    ::memset( descPath, 0, sizeof( descPath ) );
                    nDescLen = ( S32 )::fread( descPath, 1, 48, file );
                    
                    if ( nDescLen >= 0 )
                    {
                        descPath[nDescLen] = '\0';
                    }
                    
                    fileSystemLocal.FCloseFile( descHandle );
                }
                else
                {
                    strcpy( descPath, name );
                }
                nDescLen = ( S32 )::strlen( descPath ) + 1;
                
                if ( nTotal + nLen + 1 + nDescLen + 1 < bufsize )
                {
                    strcpy( listbuf, name );
                    listbuf += nLen;
                    
                    strcpy( listbuf, descPath );
                    listbuf += nDescLen;
                    
                    nTotal += nLen + nDescLen;
                    nMods++;
                }
                else
                {
                    break;
                }
            }
        }
    }
    
    idsystem->FreeFileList( pFiles );
    
    return nMods;
}

/*
================
idFileSystemLocal::Dir_f
================
*/
void idFileSystemLocal::Dir_f( void )
{
    UTF8* path;
    UTF8* extension;
    UTF8** dirnames;
    S32 ndirs, i;
    
    if ( cmdSystem->Argc() < 2 || cmdSystem->Argc() > 3 )
    {
        Com_Printf( "usage: dir <directory> [extension]\n" );
        return;
    }
    
    if ( cmdSystem->Argc() == 2 )
    {
        path = cmdSystem->Argv( 1 );
        extension = "";
    }
    else
    {
        path = cmdSystem->Argv( 1 );
        extension = cmdSystem->Argv( 2 );
    }
    
    Com_Printf( "Directory of %s %s\n", path, extension );
    Com_Printf( "---------------\n" );
    
    dirnames = fileSystemLocal.ListFiles( path, extension, &ndirs );
    
    for ( i = 0; i < ndirs; i++ )
    {
        Com_Printf( "%s\n", dirnames[i] );
    }
    
    fileSystemLocal.FreeFileList( dirnames );
}

/*
===========
idFileSystemLocal::ConvertPath
===========
*/
void idFileSystemLocal::ConvertPath( UTF8* s )
{
    while ( *s )
    {
        if ( *s == '\\' || *s == ':' )
        {
            *s = '/';
        }
        s++;
    }
}

/*
===========
idFileSystemLocal::PathCmp

Ignore case and seprator UTF8 distinctions
===========
*/
S32 idFileSystemLocal::PathCmp( StringEntry s1, StringEntry s2 )
{
    S32 c1, c2;
    
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        
        if ( c1 >= 'a' && c1 <= 'z' )
        {
            c1 -= ( 'a' - 'A' );
        }
        
        if ( c2 >= 'a' && c2 <= 'z' )
        {
            c2 -= ( 'a' - 'A' );
        }
        
        if ( c1 == '\\' || c1 == ':' )
        {
            c1 = '/';
        }
        
        if ( c2 == '\\' || c2 == ':' )
        {
            c2 = '/';
        }
        
        if ( c1 < c2 )
        {
            return -1; // strings not equal
        }
        
        if ( c1 > c2 )
        {
            return 1;
        }
    }
    while ( c1 );
    
    return 0; // strings are equal
}

/*
================
idFileSystemLocal::SortFileList
================
*/
void idFileSystemLocal::SortFileList( UTF8** filelist, S32 numfiles )
{
    S32 i, j, k, numsortedfiles;
    UTF8** sortedlist;
    
    sortedlist = ( UTF8** )memorySystem->Malloc( ( numfiles + 1 ) * sizeof( *sortedlist ) );
    sortedlist[0] = NULL;
    numsortedfiles = 0;
    
    for ( i = 0; i < numfiles; i++ )
    {
        for ( j = 0; j < numsortedfiles; j++ )
        {
            if ( PathCmp( filelist[i], sortedlist[j] ) < 0 )
            {
                break;
            }
        }
        
        for ( k = numsortedfiles; k > j; k-- )
        {
            sortedlist[k] = sortedlist[k - 1];
        }
        
        sortedlist[j] = filelist[i];
        numsortedfiles++;
    }
    
    ::memcpy( filelist, sortedlist, numfiles * sizeof( *filelist ) );
    memorySystem->Free( sortedlist );
}

/*
================
idFileSystemLocal::NewDir_f
================
*/
void idFileSystemLocal::NewDir_f( void )
{
    UTF8* filter;
    UTF8** dirnames;
    S32 ndirs;
    S32 i;
    
    if ( cmdSystem->Argc() < 2 )
    {
        Com_Printf( "usage: fdir <filter>\n" );
        Com_Printf( "example: fdir *q3dm*.bsp\n" );
        return;
    }
    
    filter = cmdSystem->Argv( 1 );
    
    Com_Printf( "---------------\n" );
    
    dirnames = fileSystemLocal.ListFilteredFiles( "", "", filter, &ndirs );
    
    fileSystemLocal.SortFileList( dirnames, ndirs );
    
    for ( i = 0; i < ndirs; i++ )
    {
        fileSystemLocal.ConvertPath( dirnames[i] );
        Com_Printf( "%s\n", dirnames[i] );
    }
    
    Com_Printf( "%d files listed\n", ndirs );
    
    fileSystemLocal.FreeFileList( dirnames );
}

/*
============
idFileSystemLocal::Path_f
============
*/
void idFileSystemLocal::Path_f( void )
{
    searchpath_t* s;
    S32 i;
    
    Com_Printf( "Current search path:\n" );
    
    for ( s = fs_searchpaths; s; s = s->next )
    {
        if ( s->pack )
        {
            //Com_Printf( "%s %X (%i files)\n", s->pack->pakFilename, s->pack->checksum, s->pack->numfiles );
            Com_Printf( "%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles );
            
            if ( fs_numServerPaks )
            {
                if ( !fileSystemLocal.PakIsPure( s->pack ) )
                {
                    Com_Printf( "    not on the pure list\n" );
                }
                else
                {
                    Com_Printf( "    on the pure list\n" );
                }
            }
        }
        else
        {
            Com_Printf( "%s/%s\n", s->dir->path, s->dir->gamedir );
        }
    }
    
    Com_Printf( "\n" );
    
    for ( i = 1; i < MAX_FILE_HANDLES; i++ )
    {
        if ( fsh[i].handleFiles.file.o )
        {
            Com_Printf( "handle %i: %s\n", i, fsh[i].name );
        }
    }
}

/*
============
idFileSystemLocal::TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run.
============
*/
void idFileSystemLocal::TouchFile_f( void )
{
    fileHandle_t f;
    
    if ( cmdSystem->Argc() != 2 )
    {
        Com_Printf( "Usage: touchFile <file>\n" );
        return;
    }
    
    fileSystemLocal.FOpenFileRead( cmdSystem->Argv( 1 ), &f, false );
    
    if ( f )
    {
        fileSystemLocal.FCloseFile( f );
    }
}

/*
============
idFileSystemLocal::Which_f
============
*/
void idFileSystemLocal::Which_f( void )
{
    searchpath_t* search;
    UTF8* netpath;
    pack_t* pak;
    fileInPack_t* pakFile;
    directory_t* dir;
    S64 hash;
    FILE* temp;
    UTF8* filename;
    UTF8 buf[MAX_OSPATH];
    
    hash = 0;
    filename = cmdSystem->Argv( 1 );
    
    if ( !filename[0] )
    {
        Com_Printf( "Usage: which <file>\n" );
        return;
    }
    
    // qpaths are not supposed to have a leading slash
    if ( filename[0] == '/' || filename[0] == '\\' )
    {
        filename++;
    }
    
    // just wants to see if file is there
    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( search->pack )
        {
            hash = fileSystemLocal.HashFileName( filename, search->pack->hashSize );
        }
        
        // is the element a pak file?
        if ( search->pack && search->pack->hashTable[hash] )
        {
            // look through all the pak file elements
            pak = search->pack;
            pakFile = pak->hashTable[hash];
            
            do
            {
                // case and separator insensitive comparisons
                if ( !fileSystemLocal.FilenameCompare( pakFile->name, filename ) )
                {
                    // found it!
                    Com_Printf( "File \"%s\" found in \"%s\"\n", filename, pak->pakFilename );
                    return;
                }
                pakFile = pakFile->next;
            }
            while ( pakFile != NULL );
        }
        else if ( search->dir )
        {
            dir = search->dir;
            
            netpath = fileSystemLocal.BuildOSPath( dir->path, dir->gamedir, filename );
            temp = fopen( netpath, "rb" );
            
            if ( !temp )
            {
                continue;
            }
            
            fclose( temp );
            
            Q_snprintf( buf, sizeof( buf ), "%s/%s", dir->path, dir->gamedir );
            
            fileSystemLocal.ReplaceSeparators( buf );
            
            Com_Printf( "File \"%s\" found at \"%s\"\n", filename, buf );
            return;
        }
    }
    Com_Printf( "File not found: \"%s\"\n", filename );
    return;
}

/*
===========
idFileSystemLocal::paksort
===========
*/
S32 idFileSystemLocal::paksort( const void* a, const void* b )
{
    UTF8* aa, * bb;
    
    aa = *( UTF8** )a;
    bb = *( UTF8** )b;
    
    return fileSystemLocal.PathCmp( aa, bb );
}

/*
===========
idFileSystemLocal::IsExt

Return true if ext matches file extension filename
===========
*/
bool idFileSystemLocal::IsExt( StringEntry filename, StringEntry ext, S32 namelen )
{
    S32 extlen;
    
    extlen = ( S32 )::strlen( ext );
    
    if ( extlen > namelen )
    {
        return false;
    }
    
    filename += namelen - extlen;
    
    return ( bool )!Q_stricmp( filename, ext );
}


/*
================
idFileSystemLocal::AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads the zip headers
================
*/
void idFileSystemLocal::AddGameDirectory( StringEntry path, StringEntry dir )
{
    searchpath_t* sp;
    searchpath_t* search;
    pack_t* pak;
    UTF8 curpath[MAX_OSPATH + 1], * pakfile;
    S32 numfiles;
    UTF8** pakfiles;
    S32 pakfilesi;
    UTF8** pakfilestmp;
    S32 numdirs;
    UTF8** pakdirs;
    S32 pakdirsi;
    UTF8** pakdirstmp;
    S32 pakwhich;
    S32 len;
    
    // Unique
    for ( sp = fs_searchpaths; sp; sp = sp->next )
    {
        if ( sp->dir && !Q_stricmp( sp->dir->path, path ) && !Q_stricmp( sp->dir->gamedir, dir ) )
        {
            return; // we've already got this one
        }
    }
    
    Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );
    
    // find all pak files in this directory
    Q_strncpyz( curpath, fileSystemLocal.BuildOSPath( path, dir, "" ), sizeof( curpath ) );
    curpath[strlen( curpath ) - 1] = '\0';	// strip the trailing slash
    
    // Get .pk3 files
    pakfiles = idsystem->ListFiles( curpath, ".pk3", NULL, &numfiles, false );
    
    // Get top level directories (we'll filter them later since the idSystemLocal::ListFiles filtering is terrible)
    pakdirs = idsystem->ListFiles( curpath, "/", NULL, &numdirs, false );
    
    qsort( pakfiles, numfiles, sizeof( UTF8* ), fileSystemLocal.paksort );
    qsort( pakdirs, numdirs, sizeof( UTF8* ), fileSystemLocal.paksort );
    
    pakfilesi = 0;
    pakdirsi = 0;
    
    // Log may not be initialized at this point, but it will still show in the console.
    Com_Printf( "idFileSystemLocal::AddGameDirectory: \"%s\" \"%s\"\n", path, dir );
    
    while ( ( pakfilesi < numfiles ) || ( pakdirsi < numdirs ) )
    {
        // Check if a pakfile or pakdir comes next
        if ( pakfilesi >= numfiles )
        {
            // We've used all the pakfiles, it must be a pakdir.
            pakwhich = 0;
        }
        else if ( pakdirsi >= numdirs )
        {
            // We've used all the pakdirs, it must be a pakfile.
            pakwhich = 1;
        }
        else
        {
            // Could be either, compare to see which name comes first
            // Need tmp variables for appropriate indirection for paksort()
            pakfilestmp = &pakfiles[pakfilesi];
            pakdirstmp = &pakdirs[pakdirsi];
            pakwhich = ( fileSystemLocal.paksort( pakfilestmp, pakdirstmp ) < 0 );
        }
        
        if ( pakwhich )
        {
            // The next .pk3 file is before the next .pk3dir
            pakfile = fileSystemLocal.BuildOSPath( path, dir, pakfiles[pakfilesi] );
            Com_Printf( "    pk3: %s\n", pakfile );
            
            if ( ( pak = fileSystemLocal.LoadZipFile( pakfile, pakfiles[pakfilesi] ) ) == 0 )
            {
                // This isn't a .pk3! Next!
                pakfilesi++;
                continue;
            }
            
            Q_strncpyz( pak->pakPathname, curpath, sizeof( pak->pakPathname ) );
            
            // store the game name for downloading
            Q_strncpyz( pak->pakGamename, dir, sizeof( pak->pakGamename ) );
            
            fs_packFiles += pak->numfiles;
            
            search = ( searchpath_t* )memorySystem->Malloc( sizeof( searchpath_t ) );
            search->pack = pak;
            search->next = fs_searchpaths;
            fs_searchpaths = search;
            
            pakfilesi++;
        }
        else
        {
            // The next .pk3dir is before the next .pk3 file
            // But wait, this could be any directory, we're filtering to only ending with ".pk3dir" here.
            len = ( S32 )::strlen( pakdirs[pakdirsi] );
            
            if ( !fileSystemLocal.IsExt( pakdirs[pakdirsi], ".pk3dir", len ) )
            {
                // This isn't a .pk3dir! Next!
                pakdirsi++;
                continue;
            }
            
            pakfile = fileSystemLocal.BuildOSPath( path, dir, pakdirs[pakdirsi] );
            Com_Printf( " pk3dir: %s\n", pakfile );
            
            // add the directory to the search path
            search = ( searchpath_t* )memorySystem->Malloc( sizeof( searchpath_t ) );
            search->dir = ( directory_t* )memorySystem->Malloc( sizeof( *search->dir ) );
            
            Q_strncpyz( search->dir->path, curpath, sizeof( search->dir->path ) );	// c:\xreal\base
            Q_strncpyz( search->dir->fullpath, pakfile, sizeof( search->dir->fullpath ) );	// c:\xreal\base\mypak.pk3dir
            Q_strncpyz( search->dir->gamedir, pakdirs[pakdirsi], sizeof( search->dir->gamedir ) ); // mypak.pk3dir
            
            search->next = fs_searchpaths;
            fs_searchpaths = search;
            
            pakdirsi++;
        }
    }
    
    // done
    idsystem->FreeFileList( pakfiles );
    idsystem->FreeFileList( pakdirs );
    
    // add the directory to the search path
    search = ( searchpath_t* )memorySystem->Malloc( sizeof( searchpath_t ) );
    search->dir = ( directory_t* )memorySystem->Malloc( sizeof( *search->dir ) );
    
    Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
    Q_strncpyz( search->dir->fullpath, curpath, sizeof( search->dir->fullpath ) );
    Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
    
    search->next = fs_searchpaths;
    fs_searchpaths = search;
}

/*
================
idFileSystemLocal::idPak
================
*/
bool idFileSystemLocal::idPak( UTF8* pak, UTF8* base )
{
    S32 i;
    
    if ( !FilenameCompare( pak, va( "%s/mp_bin", base ) ) )
    {
        return true;
    }
    
    for ( i = 0; i < NUM_ID_PAKS; i++ )
    {
        if ( !FilenameCompare( pak, va( "%s/pak%d", base, i ) ) )
        {
            break;
        }
        
        // JPW NERVE -- this fn prevents external sources from downloading/overwriting official files, so exclude both SP and MP files from this list as well
        //if ( !FilenameCompare(pak, va("%s/mp_pak%d",base,i)) )
        //{
        //	break;
        //}
        
        //if ( !FilenameCompare(pak, va("%s/sp_pak%d",base,i)) )
        //{
        //	break;
        //}
        // jpw
        
    }
    
    if ( i < NUM_ID_PAKS )
    {
        return true;
    }
    
    return false;
}

/*
================
idFileSystemLocal::VerifyOfficialPaks
================
*/
bool idFileSystemLocal::VerifyOfficialPaks( void )
{
    S32 i, j;
    searchpath_t* sp;
    S32 numOfficialPaksOnServer = 0;
    S32 numOfficialPaksLocal = 0;
    officialpak_t officialpaks[64];
    
    if ( !fs_numServerPaks )
    {
        return true;
    }
    
    for ( i = 0; i < fs_numServerPaks; i++ )
    {
        if ( idPak( fs_serverPakNames[i], BASEGAME ) )
        {
            Q_strncpyz( officialpaks[numOfficialPaksOnServer].pakname, fs_serverPakNames[i], sizeof( officialpaks[0].pakname ) );
            officialpaks[numOfficialPaksOnServer].ok = false;
            numOfficialPaksOnServer++;
        }
    }
    
    for ( i = 0; i < fs_numServerPaks; i++ )
    {
        for ( sp = fs_searchpaths; sp; sp = sp->next )
        {
            if ( sp->pack && sp->pack->checksum == fs_serverPaks[i] )
            {
                UTF8 packPath[MAX_QPATH];
                
                Q_snprintf( packPath, sizeof( packPath ), "%s/%s", sp->pack->pakGamename, sp->pack->pakBasename );
                
                if ( idPak( packPath, BASEGAME ) )
                {
                    for ( j = 0; j < numOfficialPaksOnServer; j++ )
                    {
                        if ( !Q_stricmp( packPath, officialpaks[j].pakname ) )
                        {
                            officialpaks[j].ok = true;
                        }
                    }
                    numOfficialPaksLocal++;
                }
                break;
            }
        }
    }
    
    if ( numOfficialPaksOnServer != numOfficialPaksLocal )
    {
        for ( i = 0; i < numOfficialPaksOnServer; i++ )
        {
            if ( officialpaks[i].ok != true )
            {
                Com_Printf( "ERROR: Missing/corrupt official pak file %s\n", officialpaks[i].pakname );
            }
        }
        
        return false;
    }
    else
    {
        return true;
    }
}

/*
================
idFileSystemLocal::ComparePaks

----------------
dlstring == true

Returns a list of pak files that we should download from the server. They all get stored
in the current gamedir and an idFileSystemLocal::Restart will be fired up after we download them all.

The string is the format:

@remotename@localname [repeat]

static S32		fs_numServerReferencedPaks;
static S32		fs_serverReferencedPaks[MAX_SEARCH_PATHS];
static UTF8		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];

----------------
dlstring == false

we are not interested in a download string format, we want something human-readable
(this is used for diagnostics while connecting to a pure server)

================
*/
bool idFileSystemLocal::ComparePaks( UTF8* neededpaks, S32 len, bool dlstring )
{
    searchpath_t* sp;
    bool havepak, badchecksum;
    S32 i;
    
    if ( !fs_numServerReferencedPaks )
    {
        return false; // Server didn't send any pack information along
    }
    
    *neededpaks = 0;
    
    for ( i = 0; i < fs_numServerReferencedPaks; i++ )
    {
        // Ok, see if we have this pak file
        badchecksum = false;
        havepak = false;
        
        // never autodownload any of the id paks
        if ( idPak( fs_serverReferencedPakNames[i], BASEGAME ) )
        {
            continue;
        }
        
        for ( sp = fs_searchpaths; sp; sp = sp->next )
        {
            if ( sp->pack && sp->pack->checksum == fs_serverReferencedPaks[i] )
            {
                havepak = true; // This is it!
                break;
            }
        }
        
        if ( !havepak && fs_serverReferencedPakNames[i] && *fs_serverReferencedPakNames[i] )
        {
            // Don't got it
            
            if ( dlstring )
            {
                // Remote name
                Q_strcat( neededpaks, len, "@" );
                Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
                Q_strcat( neededpaks, len, ".pk3" );
                
                // Local name
                Q_strcat( neededpaks, len, "@" );
                // Do we have one with the same name?
                if ( SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) )
                {
                    UTF8 st[MAX_ZPATH];
                    // We already have one called this, we need to download it to another name
                    // Make something up with the checksum in it
                    Q_snprintf( st, sizeof( st ), "%s.%08x.pk3", fs_serverReferencedPakNames[i], fs_serverReferencedPaks[i] );
                    Q_strcat( neededpaks, len, st );
                }
                else
                {
                    Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
                    Q_strcat( neededpaks, len, ".pk3" );
                }
            }
            else
            {
                Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
                Q_strcat( neededpaks, len, ".pk3" );
                // Do we have one with the same name?
                if ( SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) )
                {
                    Q_strcat( neededpaks, len, " (local file exists with wrong checksum)" );
#ifndef DEDICATED
                    // let the client subsystem track bad download redirects (dl file with wrong checksums)
                    // this is a bit ugly but the only other solution would have been callback passing..
                    if ( clientMainSystem->WWWBadChecksum( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) )
                    {
                        // remove a potentially malicious download file
                        // (this is also intended to avoid expansion of the pk3 into a file with different checksum .. messes up wwwdl chkfail)
                        UTF8* rmv = BuildOSPath( fs_homepath->string, va( "%s.pk3", fs_serverReferencedPakNames[i] ), "" );
                        rmv[strlen( rmv ) - 1] = '\0';
                        Remove( rmv );
                    }
#endif
                }
                Q_strcat( neededpaks, len, "\n" );
            }
        }
    }
    
    if ( *neededpaks )
    {
        Com_Printf( "Need paks: %s\n", neededpaks );
        return true;
    }
    
    return false; // We have them all
}

/*
================
idFileSystemLocal::Shutdown

Frees all resources and closes all files
================
*/
void idFileSystemLocal::Shutdown( bool closemfp )
{
    searchpath_t* p, * next;
    S32 i;
    
    for ( i = 0; i < MAX_FILE_HANDLES; i++ )
    {
        if ( fsh[i].fileSize )
        {
            FCloseFile( i );
        }
    }
    
    // free everything
    for ( p = fs_searchpaths; p; p = next )
    {
        next = p->next;
        
        if ( p->pack )
        {
            unzClose( p->pack->handle );
            memorySystem->Free( p->pack->buildBuffer );
            memorySystem->Free( p->pack );
        }
        
        if ( p->dir )
        {
            memorySystem->Free( p->dir );
        }
        
        memorySystem->Free( p );
    }
    
    // any idFileSystemLocal:: calls will now be an error until reinitialized
    fs_searchpaths = NULL;
    
    cmdSystem->RemoveCommand( "path" );
    cmdSystem->RemoveCommand( "dir" );
    cmdSystem->RemoveCommand( "fdir" );
    cmdSystem->RemoveCommand( "touchFile" );
    cmdSystem->RemoveCommand( "which" );
    
    if ( closemfp )
    {
        fclose( missingFiles );
    }
}

/*
================
idFileSystemLocal::ReorderPurePaks

NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
this can lead to misleading situations, see show_bug.cgi?id=540
================
*/
void idFileSystemLocal::ReorderPurePaks( void )
{
    searchpath_t* s;
    S32 i;
    searchpath_t** p_insert_index, // for linked list reordering
                 ** p_previous;     // when doing the scan
                 
    // only relevant when connected to pure server
    if ( !fs_numServerPaks )
    {
        return;
    }
    
    fs_reordered = false;
    
    p_insert_index = &fs_searchpaths; // we insert in order at the beginning of the list
    
    for ( i = 0; i < fs_numServerPaks; i++ )
    {
        p_previous = p_insert_index; // track the pointer-to-current-item
        
        for ( s = *p_insert_index; s; s = s->next )  // the part of the list before p_insert_index has been sorted already
        {
            if ( s->pack && fs_serverPaks[i] == s->pack->checksum )
            {
                fs_reordered = true;
                
                // move this element to the insert list
                *p_previous = s->next;
                s->next = *p_insert_index;
                *p_insert_index = s;
                
                // increment insert list
                p_insert_index = &s->next;
                
                break; // iterate to next server pack
            }
            p_previous = &s->next;
        }
    }
}

/*
================
idFileSystemLocal::Startup
================
*/
void idFileSystemLocal::Startup( StringEntry gameName )
{
    StringEntry homePath;
    UTF8 tmp[MAX_OSPATH];
    
    Com_Printf( "----- idFileSystemLocal::Startup -----\n" );
    
    fs_debug = cvarSystem->Get( "fs_debug", "0", 0, "enables the display of file system messages to the console." );
    fs_copyfiles = cvarSystem->Get( "fs_copyfiles", "0", CVAR_INIT, "Relic/obsolete.!" );
    fs_basepath = cvarSystem->Get( "fs_basepath", idsystem->DefaultInstallPath(), CVAR_INIT, "Holds the logical path to install folder." );
    fs_buildpath = cvarSystem->Get( "fs_buildpath", "", CVAR_INIT, "description" );
    fs_buildgame = cvarSystem->Get( "fs_buildgame", BASEGAME, CVAR_INIT, "Relic/obsolete.!" );
    fs_basegame = cvarSystem->Get( "fs_basegame", "", CVAR_INIT, "Allows people to base mods upon mods syntax to follow." );
    fs_libpath = cvarSystem->Get( "fs_libpath", idsystem->DefaultLibPath(), CVAR_INIT, "Default binary directory." );
#ifdef MACOS_X
    fs_apppath = cvarSystem->Get( "fs_apppath", idsystem->DefaultAppPath(), CVAR_INIT, "description" );
#endif
    homePath = idsystem->DefaultHomePath( tmp, sizeof( tmp ) );
    
    if ( !homePath || !homePath[0] )
    {
        homePath = fs_basepath->string;
    }
    
    fs_homepath = cvarSystem->Get( "fs_homepath", homePath, CVAR_INIT, "description" );
    fs_gamedirvar = cvarSystem->Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO, "description" );
    fs_restrict = cvarSystem->Get( "fs_restrict", "", CVAR_INIT, "description" );
    fs_missing = cvarSystem->Get( "fs_missing", "", CVAR_INIT, "description" );
    
    // add search path elements in reverse priority order
    if ( fs_basepath->string[0] )
    {
        AddGameDirectory( fs_basepath->string, gameName );
    }
    // fs_homepath is somewhat particular to *nix systems, only add if relevant
    
#ifdef MACOS_X
    // Make MacOSX also include the base path included with the .app bundle
    if ( fs_apppath->string[0] )
        AddGameDirectory( fs_apppath->string, gameName );
#endif
        
    // NOTE: same filtering below for mods and basegame
    if ( fs_basepath->string[0] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
    {
        AddGameDirectory( fs_homepath->string, gameName );
    }
    
#ifndef PRE_RELEASE_DEMO
    // check for additional base game so mods can be based upon other mods
    if ( fs_basegame->string[0] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_basegame->string, gameName ) )
    {
        if ( fs_basepath->string[0] )
        {
            AddGameDirectory( fs_basepath->string, fs_basegame->string );
        }
        if ( fs_homepath->string[0] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
        {
            AddGameDirectory( fs_homepath->string, fs_basegame->string );
        }
    }
    
    // check for additional game folder for mods
    if ( fs_gamedirvar->string[0] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_gamedirvar->string, gameName ) )
    {
        if ( fs_basepath->string[0] )
        {
            AddGameDirectory( fs_basepath->string, fs_gamedirvar->string );
        }
        if ( fs_homepath->string[0] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
        {
            AddGameDirectory( fs_homepath->string, fs_gamedirvar->string );
        }
    }
#endif // PRE_RELEASE_DEMO
    
    // add our commands
    cmdSystem->AddCommand( "path", Path_f, "description" );
    cmdSystem->AddCommand( "dir", Dir_f, "description" );
    cmdSystem->AddCommand( "fdir", NewDir_f, "description" );
    cmdSystem->AddCommand( "touchFile", TouchFile_f, "description" );
    cmdSystem->AddCommand( "which", Which_f, "description" );
    
    // show_bug.cgi?id=506
    // reorder the pure pk3 files according to server order
    ReorderPurePaks();
    
    //print the current search paths
    //idFileSystemLocal::Path_f();
    
    fs_gamedirvar->modified = false; // We just loaded, it's not modified
    
    Com_Printf( "----------------------\n" );
    
    if ( missingFiles == NULL )
    {
        missingFiles = fopen( "\\missing.txt", "ab" );
    }
    Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}


/*
=====================
idFileSystemLocal::GamePureChecksum
Returns the checksum of the pk3 from which the server loaded the qagame.qvm
NOTE TTimo: this is not used in RTCW so far
=====================
*/
StringEntry idFileSystemLocal::GamePureChecksum( void )
{
    static UTF8 info[MAX_STRING_TOKENS];
    searchpath_t* search;
    
    info[0] = 0;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( search->pack )
        {
            if ( search->pack->referenced & FS_QAGAME_REF )
            {
                Q_snprintf( info, sizeof( info ), "%d", search->pack->checksum );
            }
        }
    }
    
    return info;
}

/*
=====================
idFileSystemLocal::LoadedPakChecksums

Returns a space separated string containing the checksums of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
StringEntry idFileSystemLocal::LoadedPakChecksums( void )
{
    static UTF8 info[BIG_INFO_STRING];
    searchpath_t* search;
    
    info[0] = 0;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( !search->pack )
        {
            continue;
        }
        
        Q_strcat( info, sizeof( info ), va( "%i ", search->pack->checksum ) );
    }
    
    return info;
}

/*
=====================
idFileSystemLocal::LoadedPakNames

Returns a space separated string containing the names of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
StringEntry idFileSystemLocal::LoadedPakNames( void )
{
    static UTF8 info[BIG_INFO_STRING];
    searchpath_t* search;
    
    info[0] = 0;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( !search->pack )
        {
            continue;
        }
        
        if ( *info )
        {
            Q_strcat( info, sizeof( info ), " " );
        }
        
        // Arnout: changed to have the full path
        //Q_strcat( info, sizeof( info ), search->pack->pakBasename );
        Q_strcat( info, sizeof( info ), search->pack->pakGamename );
        Q_strcat( info, sizeof( info ), "/" );
        Q_strcat( info, sizeof( info ), search->pack->pakBasename );
    }
    
    return info;
}

/*
=====================
idFileSystemLocal::LoadedPakPureChecksums

Returns a space separated string containing the pure checksums of all loaded pk3 files.
Servers with sv_pure use these checksums to compare with the checksums the clients send
back to the server.
=====================
*/
StringEntry idFileSystemLocal::LoadedPakPureChecksums( void )
{
    static UTF8 info[BIG_INFO_STRING];
    searchpath_t* search;
    
    info[0] = 0;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( !search->pack )
        {
            continue;
        }
        
        Q_strcat( info, sizeof( info ), va( "%i ", search->pack->pure_checksum ) );
    }
    
    // DO_LIGHT_DEDICATED
    // only comment out when you need a new pure checksums string
    //Com_DPrintf("idFileSystemLocal::LoadPakPureChecksums: %s\n", info);
    
    return info;
}

/*
=====================
idFileSystemLocal::ReferencedPakChecksums

Returns a space separated string containing the checksums of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.
=====================
*/
StringEntry idFileSystemLocal::ReferencedPakChecksums( void )
{
    static UTF8 info[BIG_INFO_STRING];
    searchpath_t* search;
    
    info[0] = 0;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( search->pack )
        {
            if ( search->pack->referenced || Q_stricmpn( search->pack->pakGamename, BASEGAME, ( S32 )::strlen( BASEGAME ) ) )
            {
                Q_strcat( info, sizeof( info ), va( "%i ", search->pack->checksum ) );
            }
        }
    }
    
    return info;
}

/*
=====================
idFileSystemLocal::ReferencedPakNames

Returns a space separated string containing the names of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.
=====================
*/
StringEntry idFileSystemLocal::ReferencedPakNames( void )
{
    static UTF8 info[BIG_INFO_STRING];
    searchpath_t* search;
    
    info[0] = 0;
    
    // we want to return ALL pk3's from the fs_game path
    // and referenced one's from baseq3
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file?
        if ( search->pack )
        {
            if ( *info )
            {
                Q_strcat( info, sizeof( info ), " " );
            }
            
            if ( search->pack->referenced || Q_stricmpn( search->pack->pakGamename, BASEGAME, ( S32 )::strlen( BASEGAME ) ) )
            {
                Q_strcat( info, sizeof( info ), search->pack->pakGamename );
                Q_strcat( info, sizeof( info ), "/" );
                Q_strcat( info, sizeof( info ), search->pack->pakBasename );
            }
        }
    }
    
    return info;
}

/*
=====================
idFileSystemLocal::ReferencedPakPureChecksums

Returns a space separated string containing the pure checksums of all referenced pk3 files.
Servers with sv_pure set will get this string back from clients for pure validation

The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."

NOTE TTimo - DO_LIGHT_DEDICATED
this function is only used by the client to build the string sent back to server
we don't have any need of overriding it for light, but it's useless in dedicated
=====================
*/
StringEntry idFileSystemLocal::ReferencedPakPureChecksums( void )
{
    static UTF8 info[BIG_INFO_STRING];
    searchpath_t* search;
    S32 nFlags, numPaks, checksum;
    
    info[0] = 0;
    
    checksum = fs_checksumFeed;
    
    numPaks = 0;
    for ( nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1 )
    {
        if ( nFlags & FS_GENERAL_REF )
        {
            // add a delimter between must haves and general refs
            //Q_strcat(info, sizeof(info), "@ ");
            info[strlen( info ) + 1] = '\0';
            info[strlen( info ) + 2] = '\0';
            info[strlen( info )] = '@';
            info[strlen( info )] = ' ';
        }
        for ( search = fs_searchpaths; search; search = search->next )
        {
            // is the element a pak file and has it been referenced based on flag?
            if ( search->pack && ( search->pack->referenced & nFlags ) )
            {
                Q_strcat( info, sizeof( info ), va( "%i ", search->pack->pure_checksum ) );
                if ( nFlags & ( FS_CGAME_REF | FS_UI_REF ) )
                {
                    break;
                }
                checksum ^= search->pack->pure_checksum;
                numPaks++;
            }
        }
        if ( fs_fakeChkSum != 0 )
        {
            // only added if a non-pure file is referenced
            Q_strcat( info, sizeof( info ), va( "%i ", fs_fakeChkSum ) );
        }
    }
    
    // last checksum is the encoded number of referenced pk3s
    checksum ^= numPaks;
    Q_strcat( info, sizeof( info ), va( "%i ", checksum ) );
    
    return info;
}

/*
=====================
idFileSystemLocal::ClearPakReferences
=====================
*/
void idFileSystemLocal::ClearPakReferences( S32 flags )
{
    searchpath_t* search;
    
    if ( !flags )
    {
        flags = -1;
    }
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        // is the element a pak file and has it been referenced?
        if ( search->pack )
        {
            search->pack->referenced &= ~flags;
        }
    }
}

/*
=====================
idFileSystemLocal::PureServerSetLoadedPaks

If the string is empty, all data sources will be allowed.
If not empty, only pk3 files that match one of the space
separated checksums will be checked for files, with the
exception of .cfg and .dat files.
=====================
*/
void idFileSystemLocal::PureServerSetLoadedPaks( StringEntry pakSums, StringEntry pakNames )
{
    S32 i, c, d;
    
    cmdSystem->TokenizeString( pakSums );
    
    c = cmdSystem->Argc();
    if ( c > MAX_SEARCH_PATHS )
    {
        c = MAX_SEARCH_PATHS;
    }
    
    fs_numServerPaks = c;
    
    for ( i = 0; i < c; i++ )
    {
        fs_serverPaks[i] = atoi( cmdSystem->Argv( i ) );
    }
    
    if ( fs_numServerPaks )
    {
        Com_DPrintf( "Connected to a pure server.\n" );
    }
    else
    {
        if ( fs_reordered )
        {
            // show_bug.cgi?id=540
            // force a restart to make sure the search order will be correct
            Com_DPrintf( "FS search reorder is required\n" );
            Restart( fs_checksumFeed );
            return;
        }
    }
    
    for ( i = 0; i < c; i++ )
    {
        if ( fs_serverPakNames[i] )
        {
            memorySystem->Free( fs_serverPakNames[i] );
        }
        fs_serverPakNames[i] = NULL;
    }
    
    if ( pakNames && *pakNames )
    {
        cmdSystem->TokenizeString( pakNames );
        
        d = cmdSystem->Argc();
        
        if ( d > MAX_SEARCH_PATHS )
        {
            d = MAX_SEARCH_PATHS;
        }
        
        for ( i = 0; i < d; i++ )
        {
            fs_serverPakNames[i] = memorySystem->CopyString( cmdSystem->Argv( i ) );
        }
    }
}

/*
=====================
idFileSystemLocal::PureServerSetReferencedPaks

The checksums and names of the pk3 files referenced at the server
are sent to the client and stored here. The client will use these
checksums to see if any pk3 files need to be auto-downloaded.
=====================
*/
void idFileSystemLocal::PureServerSetReferencedPaks( StringEntry pakSums, StringEntry pakNames )
{
    S32 i, c, d;
    
    cmdSystem->TokenizeString( pakSums );
    
    c = cmdSystem->Argc();
    if ( c > MAX_SEARCH_PATHS )
    {
        c = MAX_SEARCH_PATHS;
    }
    
    fs_numServerReferencedPaks = c;
    
    for ( i = 0; i < c; i++ )
    {
        fs_serverReferencedPaks[i] = atoi( cmdSystem->Argv( i ) );
    }
    
    for ( i = 0; i < c; i++ )
    {
        if ( fs_serverReferencedPakNames[i] )
        {
            memorySystem->Free( fs_serverReferencedPakNames[i] );
        }
        
        fs_serverReferencedPakNames[i] = NULL;
    }
    
    if ( pakNames && *pakNames )
    {
        cmdSystem->TokenizeString( pakNames );
        
        d = cmdSystem->Argc();
        
        if ( d > MAX_SEARCH_PATHS )
        {
            d = MAX_SEARCH_PATHS;
        }
        
        for ( i = 0; i < d; i++ )
        {
            fs_serverReferencedPakNames[i] = memorySystem->CopyString( cmdSystem->Argv( i ) );
        }
    }
}

/*
================
idFileSystemLocal::InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void idFileSystemLocal::InitFilesystem( void )
{
    // allow command line parms to override our defaults
    // we have to specially handle this, because normal command
    // line variable sets don't happen until after the filesystem
    // has already been initialized
    Com_StartupVariable( "fs_basepath" );
    Com_StartupVariable( "fs_buildpath" );
    Com_StartupVariable( "fs_buildgame" );
    Com_StartupVariable( "fs_homepath" );
    Com_StartupVariable( "fs_game" );
    Com_StartupVariable( "fs_copyfiles" );
    Com_StartupVariable( "fs_restrict" );
    
    // try to start up normally
    Startup( BASEGAME );
    
#ifndef UPDATE_SERVER
    // if we can't find default.cfg, assume that the paths are
    // busted and error out now, rather than getting an unreadable
    // graphics screen when the font fails to load
    // Arnout: we want the nice error message here as well
    if ( ReadFile( "default.cfg", NULL ) <= 0 )
    {
        Com_Error( ERR_FATAL, "Couldn't load default.cfg - I am missing essential files - verify your installation?" );
    }
#endif
    
    Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
    Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}


/*
================
idFileSystemLocal::Restart
================
*/
//void CL_PurgeCache( void );
void idFileSystemLocal::Restart( S32 checksumFeed )
{

#ifndef DEDICATED
    // Arnout: big hack to clear the image cache on a FS_Restart
    //	CL_PurgeCache();
#endif
    
    // free anything we currently have loaded
    Shutdown( false );
    
    // set the checksum feed
    fs_checksumFeed = checksumFeed;
    
    // clear pak references
    ClearPakReferences( 0 );
    
    // try to start up normally
    Startup( BASEGAME );
    
    // if we can't find default.cfg, assume that the paths are
    // busted and error out now, rather than getting an unreadable
    // graphics screen when the font fails to load
    if ( ReadFile( "default.cfg", NULL ) <= 0 )
    {
        // this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
        // (for instance a TA demo server)
        if ( lastValidBase[0] )
        {
            PureServerSetLoadedPaks( "", "" );
            cvarSystem->Set( "fs_basepath", lastValidBase );
            cvarSystem->Set( "fs_gamedirvar", lastValidGame );
            lastValidBase[0] = '\0';
            lastValidGame[0] = '\0';
            cvarSystem->Set( "fs_restrict", "0" );
            Restart( checksumFeed );
            Com_Error( ERR_DROP, "Invalid game folder\n" );
            return;
        }
        
        // TTimo - added some verbosity, 'couldn't load default.cfg' confuses the hell out of users
        Com_Error( ERR_FATAL, "Couldn't load default.cfg - I am missing essential files - verify your installation?" );
    }
    
    // bk010116 - new check before safeMode
    if ( Q_stricmp( fs_gamedirvar->string, lastValidGame ) )
    {
        // skip the wolfconfig.cfg if "safe" is on the command line
        if ( !Com_SafeMode() )
        {
            UTF8* cl_profileStr = cvarSystem->VariableString( "cl_profile" );
            
            if ( com_gameInfo.usesProfiles && cl_profileStr[0] )
            {
                // bani - check existing pid file and make sure it's ok
                if ( !Com_CheckProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
                {
#ifndef _DEBUG
                    Com_Printf( "^3WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n", cl_profileStr );
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
            else
            {
                cmdBufferSystem->AddText( va( "exec %s\n", CONFIG_NAME ) );
            }
        }
    }
    
    Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
    Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
    
}

/*
=================
idFileSystemLocal::ConditionalRestart
restart if necessary

FIXME TTimo
this doesn't catch all cases where an idFileSystemLocal::Restart is necessary
see show_bug.cgi?id=478
=================
*/
bool idFileSystemLocal::ConditionalRestart( S32 checksumFeed )
{
    if ( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed )
    {
        Restart( checksumFeed );
        return true;
    }
    return false;
}

/*
========================================================================================
Handle based file calls for virtual machines
========================================================================================
*/

/*
=================
idFileSystemLocal::FOpenFileByMode
=================
*/
S32 idFileSystemLocal::FOpenFileByMode( StringEntry qpath, fileHandle_t* f, fsMode_t mode )
{
    S32 r;
    bool sync;
    
    sync = false;
    
    switch ( mode )
    {
        case FS_READ:
            r = FOpenFileRead( qpath, f, true );
            break;
            
        case FS_WRITE:
            *f = FOpenFileWrite( qpath );
            r = 0;
            
            if ( *f == 0 )
            {
                r = -1;
            }
            break;
            
        case FS_APPEND_SYNC:
            sync = true;
            
        case FS_APPEND:
            *f = FOpenFileAppend( qpath );
            r = 0;
            if ( *f == 0 )
            {
                r = -1;
            }
            break;
            
        case FS_READ_DIRECT:
            r = FOpenFileDirect( qpath, f );
            break;
            
        case FS_UPDATE:
            *f = FOpenFileUpdate( qpath, &r );
            r = 0;
            
            if ( *f == 0 )
            {
                r = -1;
            }
            break;
            
        default:
            Com_Error( ERR_FATAL, "idFileSystemLocal::FOpenFileByMode: bad mode" );
            return -1;
    }
    
    if ( !f )
    {
        return r;
    }
    
    if ( *f )
    {
        if ( fsh[*f].zipFile == true )
        {
            fsh[*f].baseOffset = unztell( fsh[*f].handleFiles.file.z );
        }
        else
        {
            fsh[*f].baseOffset = ftell( fsh[*f].handleFiles.file.o );
        }
        
        fsh[*f].fileSize = r;
        fsh[*f].streamed = false;
        
        // uncommenting this makes fs_reads
        // use the background threads --
        // MAY be faster for loading levels depending on the use of file io
        // q3a not faster
        // wolf not faster
        
        //		if (mode == FS_READ) {
        //			Sys_BeginStreamedFile( *f, 0x4000 );
        //			fsh[*f].streamed = true;
        //		}
    }
    
    fsh[*f].handleSync = sync;
    
    return r;
}

/*
=================
idFileSystemLocal::FOpenFileByMode
=================
*/
S32 idFileSystemLocal::FTell( fileHandle_t f )
{
    S32 pos;
    if ( fsh[f].zipFile == true )
    {
        pos = unztell( fsh[f].handleFiles.file.z );
    }
    else
    {
        pos = ftell( fsh[f].handleFiles.file.o );
    }
    return pos;
}

/*
=================
idFileSystemLocal::FOpenFileByMode
=================
*/
void idFileSystemLocal::Flush( fileHandle_t f )
{
    fflush( fsh[f].handleFiles.file.o );
}

/*
=================
idFileSystemLocal::VerifyPak

CVE-2006-2082
compared requested pak against the names as we built them in idFileSystemLocal::ReferencedPakNames
=================
*/
bool idFileSystemLocal::VerifyPak( StringEntry pak )
{
    UTF8 teststring[BIG_INFO_STRING];
    searchpath_t* search;
    
    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( search->pack )
        {
            Q_strncpyz( teststring, search->pack->pakGamename, sizeof( teststring ) );
            Q_strcat( teststring, sizeof( teststring ), "/" );
            Q_strcat( teststring, sizeof( teststring ), search->pack->pakBasename );
            Q_strcat( teststring, sizeof( teststring ), ".pk3" );
            
            if ( !Q_stricmp( teststring, pak ) )
            {
                return true;
            }
        }
    }
    
    return false;
}

/*
=================
idFileSystemLocal::FOpenFileByMode
=================
*/
void idFileSystemLocal::FilenameCompletion( StringEntry dir, StringEntry ext, bool stripExt, void( *callback )( StringEntry s ) )
{
    UTF8** filenames;
    S32 i, nfiles;
    UTF8 filename[MAX_STRING_CHARS];
    
    filenames = ListFilteredFiles( dir, ext, NULL, &nfiles );
    
    SortFileList( filenames, nfiles );
    
    for ( i = 0; i < nfiles; i++ )
    {
        ConvertPath( filenames[i] );
        Q_strncpyz( filename, filenames[i], MAX_STRING_CHARS );
        
        if ( stripExt )
        {
            COM_StripExtension2( filename, filename, sizeof( filename ) );
        }
        
        callback( filename );
    }
    FreeFileList( filenames );
}

// Check if a file is empty
bool idFileSystemLocal::IsFileEmpty( UTF8* filename )
{
    bool result;
    fileHandle_t file;
    UTF8 buf[1];
    
    // Open the file (we need to use ioquake3 functions because it will build the full path relative to homepath and gamedir, see fileSystem->BuildOSPath)
    FOpenFileRead( filename, &file, false ); // the length returned by fileSystem->FOpenFileRead is equal to 1 when the file has no content or when it has 1 character, so this length can't be used to know if the file is empty or not
    
    // Get the first character from the file, but what we are interested in is the number of characters read, which we store in the var c
    S32 c = Read( buf, 1, file );
    
    // If the number of characters that were read is 0, then the file is empty
    if ( c == 0 )
    {
        result = true;
        // Else if we have read 1 character, then the file is not empty
    }
    else
    {
        result = false;
    }
    
    // Close the file
    fileSystem->FCloseFile( file );
    
    // Return the result
    return result;
}

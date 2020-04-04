////////////////////////////////////////////////////////////////////////////////////////
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
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   system_api.h
// Created:     12/25/2018
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SYSTEM_API_H__
#define __SYSTEM_API_H__

typedef enum
{
    DR_YES = 0,
    DR_NO = 1,
    DR_OK = 0,
    DR_CANCEL = 1
} dialogResult_t;

typedef enum
{
    DT_INFO,
    DT_WARNING,
    DT_ERROR,
    DT_YES_NO,
    DT_OK_CANCEL
} dialogType_t;

//
// idServerSnapshotSystem
//
class idSystem
{
public:
    virtual void Restart_f( void ) = 0;
    virtual void Shutdown( void ) = 0;
    virtual void InputInit( void* windowData ) = 0;
    virtual void SysSnapVector( F32* v ) = 0;
    virtual void* GetProcAddress( void* dllhandle, StringEntry name ) = 0;
    virtual void* LoadDll( StringEntry name ) = 0;
    virtual void UnloadDll( void* dllHandle ) = 0;
    virtual UTF8* GetDLLName( StringEntry name ) = 0;
    virtual void Error( StringEntry error, ... ) = 0;
    virtual void Print( StringEntry msg ) = 0;
    virtual void WriteDump( StringEntry fmt, ... ) = 0;
    virtual void Quit( void ) = 0;
    virtual void Init( void ) = 0;
    virtual bool WritePIDFile( void ) = 0;
    virtual UTF8* ConsoleInput( void ) = 0;
    virtual UTF8* DefaultAppPath( void ) = 0;
    virtual UTF8* DefaultLibPath( void ) = 0;
    virtual UTF8* DefaultInstallPath( void ) = 0;
    virtual UTF8* SysGetClipboardData( void ) = 0;
    virtual void Chmod( UTF8* file, S32 mode ) = 0;
    virtual bool IsNumLockDown( void ) = 0;
    virtual void OpenURL( StringEntry url, bool doexit ) = 0;
    virtual void StartProcess( UTF8* exeName, bool doexit ) = 0;
    virtual void GLimpSafeInit( void ) = 0;
    virtual void GLimpInit( void ) = 0;
    virtual dialogResult_t Dialog( dialogType_t type, StringEntry message, StringEntry title ) = 0;
    virtual bool OpenUrl( StringEntry url ) = 0;
    virtual void SysSleep( S32 msec ) = 0;
    virtual void FreeFileList( UTF8** list ) = 0;
    virtual UTF8** ListFiles( StringEntry directory, StringEntry extension, UTF8* filter, S32* numfiles, bool wantsubs ) = 0;
    virtual bool Mkdir( StringEntry path ) = 0;
    virtual bool LowPhysicalMemory( void ) = 0;
    virtual UTF8* GetCurrentUser( void ) = 0;
    virtual bool RandomBytes( U8* string, S32 len ) = 0;
    virtual S32 Milliseconds( void ) = 0;
    virtual UTF8* DefaultHomePath( UTF8* buffer, S32 size ) = 0;
#ifndef DEDICATED
    virtual void DeactivateMouse( void ) = 0;
#endif
    virtual UTF8* Cwd( void ) = 0;
};

extern idSystem* idsystem;

#endif //!__SYSTEM_API_H__

////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientMain_api.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTMAIN_API_H__
#define __CLIENTMAIN_API_H__

#if defined (GUI) || defined (CGAMEDLL)
typedef struct netadr_s netadr_t;
typedef struct msg_s msg_t;
#endif

//
// idClientMainSystem
//
class idClientMainSystem
{
public:
    virtual void AddReliableCommand( StringEntry cmd ) = 0;
    virtual demoState_t DemoState( void ) = 0;
    virtual S32 DemoPos( void ) = 0;
    virtual void DemoName( UTF8* buffer, S32 size ) = 0;
    virtual void ShutdownAll( bool shutdownRef ) = 0;
    virtual void FlushMemory( void ) = 0;
    virtual void MapLoading( void ) = 0;
    virtual void Disconnect( bool showMainMenu ) = 0;
#ifndef GAMEDLL
    virtual void PacketEvent( netadr_t from, msg_t* msg ) = 0;
#endif
    virtual void ForwardCommandToServer( StringEntry string ) = 0;
    virtual bool WWWBadChecksum( StringEntry pakname ) = 0;
    virtual void Frame( S32 msec ) = 0;
    virtual void RefPrintf( S32 print_level, StringEntry fmt, ... ) = 0;
    virtual void StartHunkUsers( bool rendererOnly ) = 0;
    virtual void CheckAutoUpdate( void ) = 0;
    virtual void GetAutoUpdate( void ) = 0;
    virtual void* RefMalloc( S32 size ) = 0;
    virtual void RefTagFree( void ) = 0;
    virtual S32 ScaledMilliseconds( void ) = 0;
    virtual void Init( void ) = 0;
    virtual void Shutdown( void ) = 0;
    virtual void TranslateString( StringEntry string, UTF8* dest_buffer ) = 0;
    virtual bool NextUpdateServer( void ) = 0;
    virtual void AddToLimboChat( StringEntry str ) = 0;
    virtual void OpenURL( StringEntry url ) = 0;
};

extern idClientMainSystem* clientMainSystem;

#endif // !__CLIENTMAIN_API_H__

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
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   clientGUI_api.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTGUI_API_H__
#define __CLIENTGUI_API_H__

typedef struct
{
    S32 connectPacketCount;
    S32 clientNum;
    UTF8 servername[MAX_STRING_CHARS];
    UTF8 updateInfoString[MAX_STRING_CHARS];
    UTF8 messageString[MAX_STRING_CHARS];
    connstate_t connState;
} uiClientState_t;

//
// idClientGUISystem
//
class idClientGUISystem
{
public:
    virtual void GetClientState( uiClientState_t* state ) = 0;
    virtual void GetGlconfig( vidconfig_t* config ) = 0;
    virtual void GUIGetClipboardData( UTF8* buf, S32 buflen ) = 0;
    virtual S32 GetConfigString( S32 index, UTF8* buf, S32 size ) = 0;
    virtual bool GetNews( bool begin ) = 0;
    virtual void KeynumToStringBuf( S32 keynum, UTF8* buf, S32 buflen ) = 0;
    virtual void GetBindingBuf( S32 keynum, UTF8* buf, S32 buflen ) = 0;
    virtual S32 GetCatcher( void ) = 0;
    virtual void SetCatcher( S32 catcher ) = 0;
    virtual bool checkKeyExec( S32 key ) = 0;
    virtual bool GameCommand( void ) = 0;
    virtual void InitGUI( void ) = 0;
    virtual void ShutdownGUI( void ) = 0;
};

extern idClientGUISystem* clientGUISystem;

#endif // !__CLIENTGUI_API_H__


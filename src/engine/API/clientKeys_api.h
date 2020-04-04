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
// File name:   clientKeys_api.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTKEYS_API_H__
#define __CLIENTKEYS_API_H__

typedef struct
{
    bool down;
    // if > 1, it is autorepeating
    S32 repeats;
    UTF8* binding;
    S32 hash;
} qkey_t;

extern bool key_overstrikeMode;
extern qkey_t keys[MAX_KEYS];

#if defined (GUI) || defined (CGAMEDLL)
typedef struct field_s field_t;
#endif

extern S32 anykeydown;
extern field_t g_consoleField;
extern field_t chatField;
extern bool chat_team;
extern bool chat_buddy;

//
// idClientKeysSystem
//
class idClientKeysSystem
{
public:
    virtual bool Key_GetOverstrikeMode( void ) = 0;
    virtual void Key_SetOverstrikeMode( bool state ) = 0;
    virtual bool Key_IsDown( S32 keynum ) = 0;
    virtual S32 Key_StringToKeynum( UTF8* str ) = 0;
    virtual void Key_SetBinding( S32 keynum, StringEntry binding ) = 0;
    virtual void Key_GetBindingByString( StringEntry binding, S32* key1, S32* key2 ) = 0;
    virtual S32 Key_GetKey( StringEntry binding ) = 0;
    virtual void InitKeyCommands( void ) = 0;
    virtual void KeyEvent( S32 key, S32 down, S32 time ) = 0;
    virtual void CharEvent( S32 key ) = 0;
    virtual void Key_ClearStates( void ) = 0;
    virtual void Key_WriteBindings( fileHandle_t f ) = 0;
    virtual void Key_KeynameCompletion( void( *callback )( StringEntry s ) ) = 0;
    virtual UTF8* Key_KeynumToString( S32 keynum ) = 0;
};

extern idClientKeysSystem* clientKeysSystem;

#endif // !__CLIENTKEYS_API_H__


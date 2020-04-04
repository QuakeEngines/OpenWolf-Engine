////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf Engine.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   clientAVI.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTKEYS_LOCAL_H__
#define __CLIENTKEYS_LOCAL_H__

//
// idClientKeysSystemLocal
//
class idClientKeysSystemLocal : public idClientKeysSystem
{
public:
    idClientKeysSystemLocal();
    ~idClientKeysSystemLocal();
    
    virtual bool Key_GetOverstrikeMode( void );
    virtual void Key_SetOverstrikeMode( bool state );
    virtual bool Key_IsDown( S32 keynum );
    virtual S32 Key_StringToKeynum( UTF8* str );
    virtual void Key_SetBinding( S32 keynum, StringEntry binding );
    virtual void Key_GetBindingByString( StringEntry binding, S32* key1, S32* key2 );
    virtual S32 Key_GetKey( StringEntry binding );
    virtual void InitKeyCommands( void );
    virtual void KeyEvent( S32 key, S32 down, S32 time );
    virtual void CharEvent( S32 key );
    virtual void Key_ClearStates( void );
    virtual void Key_WriteBindings( fileHandle_t f );
    virtual void Key_KeynameCompletion( void( *callback )( StringEntry s ) );
    virtual UTF8* Key_KeynumToString( S32 keynum );
    
    static S64 generateHashValue( StringEntry fname );
    static void Field_VariableSizeDraw( field_t* edit, S32 x, S32 y, S32 size, bool showCursor, bool noColorEscape, F32 alpha );
    static void Field_Draw( field_t* edit, S32 x, S32 y, bool showCursor, bool noColorEscape, F32 alpha );
    static void Field_BigDraw( field_t* edit, S32 x, S32 y, bool showCursor, bool noColorEscape );
    static void Field_Paste( field_t* edit );
    static void Field_KeyDownEvent( field_t* edit, S32 key );
    static void Field_CharEvent( field_t* edit, S32 ch );
    static void CompleteCommand( void );
    static void Console_Key( S32 key );
    static void Message_Key( S32 key );
    static UTF8* Key_GetBinding( S32 keynum );
    static void Key_Unbindall_f( void );
    static void Key_Bind_f( void );
    static void Key_EditBind_f( void );
    static void Key_Bindlist_f( void );
    static void Key_Unbind_f( void );
    static void Key_CompleteUnbind( UTF8* args, S32 argNum );
    static void Key_CompleteBind( UTF8* args, S32 argNum );
    static void Key_CompleteEditbind( UTF8* args, S32 argNum );
    static void AddKeyUpCommands( S32 key, UTF8* kb, S32 time );
private:
    bool consoleButtonWasPressed = false;
    const S32 BIND_HASH_SIZE = 1024;
};

extern idClientKeysSystemLocal clientKeysLocal;

#endif // !__CLIENTKEYS_LOCAL_H__
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

#ifndef __CLIENTMAIN_LOCAL_H__
#define __CLIENTMAIN_LOCAL_H__

// NERVE - SMF - localization
typedef enum
{
    LANGUAGE_FRENCH = 0,
    LANGUAGE_GERMAN,
    LANGUAGE_ITALIAN,
    LANGUAGE_SPANISH,
    MAX_LANGUAGES
} languages_t;

#define MAX_TRANS_STRING    4096

typedef struct trans_s
{
    UTF8            original[MAX_TRANS_STRING];
    UTF8            translated[MAX_LANGUAGES][MAX_TRANS_STRING];
    struct trans_s* next;
    F32           x_offset;
    F32           y_offset;
    bool        fromFile;
} trans_t;

//
// idClientMainSystemLocal
//
class idClientMainSystemLocal : public idClientMainSystem
{
public:
    idClientMainSystemLocal();
    ~idClientMainSystemLocal();
    
    virtual void AddReliableCommand( StringEntry cmd );
    virtual demoState_t DemoState( void );
    virtual S32 DemoPos( void );
    virtual void DemoName( UTF8* buffer, S32 size );
    virtual void ShutdownAll( bool shutdownRef );
    virtual void FlushMemory( void );
    virtual void MapLoading( void );
    virtual void Disconnect( bool showMainMenu );
    virtual void PacketEvent( netadr_t from, msg_t* msg );
    virtual void ForwardCommandToServer( StringEntry string );
    virtual bool WWWBadChecksum( StringEntry pakname );
    virtual void Frame( S32 msec );
    virtual void RefPrintf( S32 print_level, StringEntry fmt, ... );
    virtual void StartHunkUsers( bool rendererOnly );
    virtual void CheckAutoUpdate( void );
    virtual void GetAutoUpdate( void );
    virtual void* RefMalloc( S32 size );
    virtual void RefTagFree( void );
    virtual S32 ScaledMilliseconds( void );
    virtual void Init( void );
    virtual void Shutdown( void );
    virtual void TranslateString( StringEntry string, UTF8* dest_buffer );
    virtual bool NextUpdateServer( void );
    virtual void AddToLimboChat( StringEntry str );
    virtual void OpenURL( StringEntry url );
    
    static void PurgeCache( void );
    static void DoPurgeCache( void );
    static void WriteDemoMessage( msg_t* msg, S32 headerBytes );
    static void StopRecord_f( void );
    static void DemoFilename( S32 number, UTF8* fileName );
    static void Record_f( void );
    static void Record( StringEntry name );
    static void DemoCompleted( void );
    static void ReadDemoMessage( void );
    static void WavFilename( S32 number, UTF8* fileName );
    static void WriteWaveHeader( void );
    static void WriteWaveOpen( void );
    static void WriteWaveClose( void );
    static void CompleteDemoName( UTF8* args, S32 argNum );
    static void PlayDemo_f( void );
    static void StartDemoLoop( void );
    static void NextDemo( void );
    static void ClearMemory( bool shutdownRef );
    static void ClearState( void );
    static void UpdateGUID( StringEntry prefix, S32 prefix_len );
    static void ClearStaticDownload( void );
    static void OpenUrl_f( void );
    static void RequestMotd( void );
    static void ForwardToServer_f( void );
    static void Setenv_f( void );
    static void Disconnect_f( void );
    static void Reconnect_f( void );
    static void Connect_f( void );
    static void Rcon_f( void );
    static void SendPureChecksums( void );
    static void ResetPureClientAtServer( void );
    static void Vid_Restart_f( void );
    static void UI_Restart_f( void );
    static void Snd_Reload_f( void );
    static void Snd_Restart_f( void );
    static void OpenedPK3List_f( void );
    static void ReferencedPK3List_f( void );
    static void Configstrings_f( void );
    static void Clientinfo_f( void );
    static void WavRecord_f( void );
    static void WavStopRecord_f( void );
    static void Video_f( void );
    static void StopVideo_f( void );
    static void DownloadsComplete( void );
    static void BeginDownload( StringEntry localName, StringEntry remoteName );
    static void NextDownload( void );
    static void InitDownloads( void );
    static void CheckForResend( void );
    static void DisconnectPacket( netadr_t from );
    static UTF8* str_replace( StringEntry string, StringEntry substr, StringEntry replacement );
    static void MotdPacket( netadr_t from, StringEntry info );
    static void PrintPacket( netadr_t from, msg_t* msg );
    static void ConnectionlessPacket( netadr_t from, msg_t* msg );
    static void CheckTimeout( void );
    static bool CheckPaused( void );
    static void CheckUserinfo( void );
    static void WWWDownload( void );
    static void Cache_StartGather_f( void );
    static void Cache_UsedFile_f( void );
    static void Cache_SetIndex_f( void );
    static void Cache_MapChange_f( void );
    static void Cache_EndGather_f( void );
    static void SetRecommended_f( void );
    static void InitRenderer( void );
    static void InitRef( void );
    static void ShutdownRef( void );
    static void SaveTranslations_f( void );
    static void SaveNewTranslations_f( void );
    static void LoadTranslations_f( void );
    static void GenerateGUIDKey( void );
    static void Userinfo_f( void );
    static trans_t* AllocTrans( UTF8* original, UTF8* translated[MAX_LANGUAGES] );
    static S64 generateHashValue( StringEntry fname );
    static trans_t* LookupTrans( UTF8* original, UTF8* translated[MAX_LANGUAGES], bool isLoading );
    static void SaveTransTable( StringEntry fileName, bool newOnly );
    static bool CheckTranslationString( UTF8* original, UTF8* translated );
    static void LoadTransTable( StringEntry fileName );
    static void ReloadTranslation( void );
    static void InitTranslation( void );
    static StringEntry TranslateStringBuf( StringEntry string );
    static void BotImport_DrawPolygon( S32 color, S32 numpoints, F32* points );
    static void UpdateInfoPacket( netadr_t from );
    static void InitExportTable( void );
private:
    const S32 MAX_RCON_MESSAGE = 1024;
};

extern idClientMainSystemLocal clientMainLocal;

#endif // !__CLIENTMAIN_LOCAL_H__
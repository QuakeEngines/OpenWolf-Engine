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
// File name:   FileSystem_api.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __FILESYSTEM_API_H__
#define __FILESYSTEM_API_H__

// Dushan
// INFO: Little crapy, but its working
// Basically PAK3 files are ZIP files with BZ2 compression

typedef struct fileInPack_s
{
    UTF8* name; // name of the file
    U64 pos; // file info position in zip
    U64	len;// uncompress file size
    struct fileInPack_s* next; // next file in the hash
} fileInPack_t;

typedef struct pack_s
{
    UTF8 pakFilename[MAX_OSPATH]; // c:\quake3\baseq3\pak0.pk3
    UTF8 pakPathname[MAX_OSPATH];
    UTF8 pakBasename[MAX_OSPATH]; // pak0
    UTF8 pakGamename[MAX_OSPATH]; // baseq3
    void* handle; // handle to zip file
    S32 checksum; // regular checksum
    S32 pure_checksum; // checksum for pure
    S32 numfiles; // number of files in pk3
    S32 referenced; // referenced file flags
    size_t hashSize; // hash table size (power of 2)
    fileInPack_t** hashTable; // hash table
    fileInPack_t* buildBuffer; // buffer with the filenames etc.
} pack_t;

#define MAX_FOUND_FILES 0x1000

// idFileSystem
class idFileSystem
{
public:
    virtual S32 LoadStack( void ) = 0;
    virtual void ForceFlush( fileHandle_t f ) = 0;
    virtual UTF8* BuildOSPath( StringEntry base, StringEntry game, StringEntry qpath ) = 0;
    virtual S32 CreatePath( StringEntry OSPath_ ) = 0;
    virtual void FSCopyFile( UTF8* fromOSPath, UTF8* toOSPath ) = 0;
    virtual void HomeRemove( StringEntry homePath ) = 0;
    virtual bool FileExists( StringEntry file ) = 0;
    virtual fileHandle_t SV_FOpenFileWrite( StringEntry filename ) = 0;
    virtual S32 SV_FOpenFileRead( StringEntry filename, fileHandle_t* fp ) = 0;
    virtual void SV_Rename( StringEntry from, StringEntry to ) = 0;
    virtual void Rename( StringEntry from, StringEntry to ) = 0;
    virtual void FCloseFile( fileHandle_t f ) = 0;
    virtual fileHandle_t FOpenFileWrite( StringEntry filename ) = 0;
    virtual fileHandle_t FOpenFileAppend( StringEntry filename ) = 0;
    virtual S32 FOpenFileRead( StringEntry filename, fileHandle_t* file, bool uniqueFILE ) = 0;
    virtual S32 FOpenFileRead_Filtered( StringEntry qpath, fileHandle_t* file, bool uniqueFILE, S32 filter_flag ) = 0;
    virtual bool CL_ExtractFromPakFile( StringEntry base, StringEntry gamedir, StringEntry filename ) = 0;
    virtual S32 Delete( UTF8* filename ) = 0;
    virtual S32 Read2( void* buffer, S32 len, fileHandle_t f ) = 0;
    virtual S32 Read( void* buffer, S32 len, fileHandle_t f ) = 0;
    virtual S32 Write( const void* buffer, S32 len, fileHandle_t h ) = 0;
    virtual void Printf( fileHandle_t h, StringEntry fmt, ... ) = 0;
    virtual S32 Seek( fileHandle_t f, S64 offset, S32 origin ) = 0;
    virtual S32 FileIsInPAK( StringEntry filename, S32* pChecksum ) = 0;
    virtual S32 ReadFile( StringEntry qpath, void** buffer ) = 0;
    virtual void FreeFile( void* buffer ) = 0;
    virtual void WriteFile( StringEntry qpath, const void* buffer, S32 size ) = 0;
    virtual UTF8** ListFiles( StringEntry path, StringEntry extension, S32* numfiles ) = 0;
    virtual void FreeFileList( UTF8** list ) = 0;
    virtual S32 GetFileList( StringEntry path, StringEntry extension, UTF8* listbuf, S32 bufsize ) = 0;
    virtual bool idPak( UTF8* pak, UTF8* base ) = 0;
    virtual bool VerifyOfficialPaks( void ) = 0;
    virtual bool ComparePaks( UTF8* neededpaks, S32 len, bool dlstring ) = 0;
    virtual void Shutdown( bool closemfp ) = 0;
    virtual StringEntry LoadedPakNames( void ) = 0;
    virtual StringEntry GamePureChecksum( void ) = 0;
    virtual StringEntry LoadedPakChecksums( void ) = 0;
    virtual StringEntry LoadedPakPureChecksums( void ) = 0;
    virtual StringEntry ReferencedPakChecksums( void ) = 0;
    virtual StringEntry ReferencedPakNames( void ) = 0;
    virtual StringEntry ReferencedPakPureChecksums( void ) = 0;
    virtual void ClearPakReferences( S32 flags ) = 0;
    virtual void PureServerSetLoadedPaks( StringEntry pakSums, StringEntry pakNames ) = 0;
    virtual void PureServerSetReferencedPaks( StringEntry pakSums, StringEntry pakNames ) = 0;
    virtual void InitFilesystem( void ) = 0;
    virtual void Restart( S32 checksumFeed ) = 0;
    virtual bool ConditionalRestart( S32 checksumFeed ) = 0;
    virtual S32 FOpenFileByMode( StringEntry qpath, fileHandle_t* f, fsMode_t mode ) = 0;
    virtual S32 FTell( fileHandle_t f ) = 0;
    virtual void Flush( fileHandle_t f ) = 0;
    virtual bool VerifyPak( StringEntry pak ) = 0;
    virtual void FilenameCompletion( StringEntry dir, StringEntry ext, bool stripExt, void( *callback )( StringEntry s ) ) = 0;
    virtual bool IsFileEmpty( UTF8* filename ) = 0;
    virtual bool Initialized( void ) = 0;
    virtual UTF8* ShiftStr( StringEntry string, S32 shift ) = 0;
};

extern idFileSystem* fileSystem;

#endif // !__FILESYSTEM_API_H__

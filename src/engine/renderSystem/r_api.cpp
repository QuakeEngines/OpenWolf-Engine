////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
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
// File name:   cgame_api.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2017, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

rendererImports_t* imports;
idCollisionModelManager* collisionModelManager;
idFileSystem* fileSystem;
idCVarSystem* cvarSystem;
idCmdBufferSystem* cmdBufferSystem;
idCmdSystem* cmdSystem;
idSystem* idsystem;
idClientAVISystemAPI* clientAVISystem;
idClientMainSystem* clientMainSystem;
idMemorySystem* memorySystem;

#ifdef __LINUX__
extern "C" idRenderSystem* rendererEntry( rendererImports_t* renimports )
#else
Q_EXPORT idRenderSystem* rendererEntry( rendererImports_t* renimports )
#endif
{
    imports = renimports;
    
    collisionModelManager = imports->collisionModelManager;
    fileSystem = imports->fileSystem;
    cvarSystem = imports->cvarSystem;
    cmdBufferSystem = imports->cmdBufferSystem;
    cmdSystem = imports->cmdSystem;
    idsystem = imports->idsystem;
    memorySystem = imports->memorySystem;
    clientAVISystem = imports->clientAVISystem;
    clientMainSystem = imports->clientMainSystem;
    
    return renderSystem;
}

void QDECL Com_Printf( StringEntry msg, ... )
{
    va_list argptr;
    UTF8 text[1024];
    
    va_start( argptr, msg );
    Q_vsnprintf( text, sizeof( text ), msg, argptr );
    va_end( argptr );
    
    imports->Printf( "%s", text );
}

void QDECL Com_Error( S32 level, StringEntry error, ... )
{
    va_list argptr;
    UTF8 text[1024];
    
    va_start( argptr, error );
    Q_vsnprintf( text, sizeof( text ), error, argptr );
    va_end( argptr );
    
    imports->Error( level, "%s", text );
}

e_status trap_CIN_RunCinematic( S32 handle )
{
    return imports->RunCinematic( handle );
}

S32 trap_CIN_PlayCinematic( StringEntry arg0, S32 xpos, S32 ypos, S32 width, S32 height, S32 bits )
{
    return imports->PlayCinematic( arg0, xpos, ypos, width, height, bits );
}

void trap_CIN_UploadCinematic( S32 handle )
{
    imports->UploadCinematic( handle );
}

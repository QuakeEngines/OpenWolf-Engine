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
// File name:   cmd.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: Quake script command processing module
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idCmdDelaySystemLocal CmdDelaySystemLocal;
idCmdDelaySystem* cmdDelaySystem = &CmdDelaySystemLocal;

delayedCommands_s delayedCommands[MAX_DELAYED_COMMANDS];

/*
===============
idCmdDelaySystemLocal::idCmdDelaySystemLocal
===============
*/
idCmdDelaySystemLocal::idCmdDelaySystemLocal( void )
{
}

/*
===============
idCmdDelaySystemLocal::~idCmdDelaySystemLocal
===============
*/
idCmdDelaySystemLocal::~idCmdDelaySystemLocal( void )
{
}

/*
===============
idCmdDelaySystemLocal::Frame
===============
*/
void idCmdDelaySystemLocal::Frame( void )
{
    S32 i;
    bool RunIt;
    
    for ( i = 0; ( i < MAX_DELAYED_COMMANDS ); i++ )
    {
        RunIt = false;
        
        if ( delayedCommands[i].delay == CMD_DELAY_UNUSED )
        {
            continue;
        }
        
        //check if we should run the command (both type)
        if ( delayedCommands[i].type == CMD_DELAY_MSEC && delayedCommands[i].delay < idsystem->Milliseconds() )
        {
            RunIt = true;
        }
        else if ( delayedCommands[i].type == CMD_DELAY_FRAME )
        {
            delayedCommands[i].delay -= 1;
            if ( delayedCommands[i].delay == CMD_DELAY_FRAME_FIRE )
            {
                RunIt = true;
            }
        }
        
        if ( RunIt )
        {
            delayedCommands[i].delay = CMD_DELAY_UNUSED;
            cmdBufferLocal.ExecuteText( EXEC_NOW, delayedCommands[i].text );
        }
    }
}

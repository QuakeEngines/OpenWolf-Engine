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
// File name:   Threads.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CONSOLE_HISTORY_H__
#define __CONSOLE_HISTORY_H__

#define CON_HISTORY 64
#define CON_HISTORY_FILE "conhistory"

static UTF8 history[CON_HISTORY][MAX_EDIT_LINE];
static S32 hist_current, hist_next;

//
// idHistorySystemLocal
//
class idConsoleHistorySystemLocal : public idConsoleHistorySystem
{
public:
    idConsoleHistorySystemLocal( void );
    ~idConsoleHistorySystemLocal( void );
    
    virtual void HistoryAdd( StringEntry field );
    virtual StringEntry HistoryPrev( void );
    virtual StringEntry HistoryNext( void );
    
    static void HistorySave( void );
    static void HistoryLoad( void );
    
};

extern idConsoleHistorySystemLocal consoleHistorySystemLocal;

#endif //!__HISTORY_H__
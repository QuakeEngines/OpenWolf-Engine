////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   serverNetChan.h
// Version:     v1.00
// Created:     12/25/2018
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERNETCHAN_H__
#define __SERVERNETCHAN_H__

//
// idServerGameSystemLocal
//
class idServerNetChanSystemLocal : public idServerNetChanSystem
{
public:
    virtual void NetchanFreeQueue( client_t* client );
    virtual void NetchanTransmitNextFragment( client_t* client );
    virtual void NetchanTransmit( client_t* client, msg_t* msg );
    virtual bool NetchanProcess( client_t* client, msg_t* msg );
public:
    idServerNetChanSystemLocal();
    ~idServerNetChanSystemLocal();
    
    static void NetchanEncode( client_t* client, msg_t* msg, UTF8* commandString );
    static void NetchanDecode( client_t* client, msg_t* msg );
    
};

extern idServerNetChanSystemLocal serverNetChanSystemLocal;

#endif //!__SERVERNETCHAN_H__

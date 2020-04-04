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
// File name:   clientAVI_api.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTAVI_API_H__
#define __CLIENTAVI_API_H__

//
// idClientAVISystemAPI
//
class idClientAVISystemAPI
{
public:
    virtual void WriteAVIVideoFrame( const U8* imageBuffer, S32 size ) = 0;
    virtual void WriteAVIAudioFrame( const U8* pcmBuffer, S32 size ) = 0;
    virtual void TakeVideoFrame( void ) = 0;
    virtual bool CloseAVI( void ) = 0;
    virtual bool VideoRecording( void ) = 0;
    virtual bool OpenAVIForWriting( StringEntry fileName ) = 0;
};

extern idClientAVISystemAPI* clientAVISystem;

#endif // !__CLIENTAVI_API_H__


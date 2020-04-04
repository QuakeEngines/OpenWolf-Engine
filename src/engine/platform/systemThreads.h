////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   systemThreads.h
// Version:     v1.00
// Created:     11/06/2019
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SYSTEMTHREADS_H__
#define __SYSTEMTHREADS_H__

//
// idConsoleCursesLocal
//
class idSystemThreadsLocal : public idSystemThreadsSystem
{
public:
    idSystemThreadsLocal();
    ~idSystemThreadsLocal();
    
    virtual S32 Mutex_Create( qmutex_t** pmutex );
    virtual void Mutex_Destroy( qmutex_t* mutex );
    virtual void Mutex_Lock( qmutex_t* mutex );
    virtual void Mutex_Unlock( qmutex_t* mutex );
    virtual S32 Thread_Create( qthread_t** pthread, void * ( *routine )( void* ), void* param );
    virtual void Thread_Join( qthread_t* thread );
    virtual void Thread_Yield( void );
    virtual S32 Thread_Cancel( qthread_t* thread );
};

extern idSystemThreadsLocal systemThreadsLocal;

#endif // __SYSTEMTHREADS_H__
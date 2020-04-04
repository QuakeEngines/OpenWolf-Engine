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
// File name:   Threads.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idThreadsSystemLocal threadsSystemLocal;
idThreadsSystem* threadsSystem = &threadsSystemLocal;

/*
===============
idThreadsSystemLocal::idThreadsSystemLocal
===============
*/
idThreadsSystemLocal::idThreadsSystemLocal( void )
{
}

/*
===============
idThreadsSystemLocal::~idThreadsSystemLocal
===============
*/
idThreadsSystemLocal::~idThreadsSystemLocal( void )
{
}

qmutex_t* global_mutex;

/*
* QMutex_Create
*/
qmutex_t* idThreadsSystemLocal::Mutex_Create( void )
{
    S32 ret;
    qmutex_t* mutex;
    
    ret = systemThreadsSystem->Mutex_Create( &mutex );
    if ( ret != 0 )
    {
        return nullptr;
    }
    return mutex;
}

/*
* QMutex_Destroy
*/
void idThreadsSystemLocal::Mutex_Destroy( qmutex_t** pmutex )
{
    assert( pmutex != nullptr );
    if ( pmutex && *pmutex )
    {
        systemThreadsSystem->Mutex_Destroy( *pmutex );
        *pmutex = nullptr;
    }
}

/*
* QMutex_Lock
*/
void idThreadsSystemLocal::Mutex_Lock( qmutex_t* mutex )
{
    assert( mutex != nullptr );
    systemThreadsSystem->Mutex_Lock( mutex );
}

/*
* QMutex_Unlock
*/
void idThreadsSystemLocal::Mutex_Unlock( qmutex_t* mutex )
{
    assert( mutex != nullptr );
    systemThreadsSystem->Mutex_Unlock( mutex );
}

/*
* QThread_Create
*/
qthread_t* idThreadsSystemLocal::Thread_Create( void * ( *routine )( void* ), void* param )
{
    S32 ret;
    qthread_t* thread;
    
    ret = systemThreadsSystem->Thread_Create( &thread, routine, param );
    if ( ret != 0 )
    {
        return nullptr;
    }
    return thread;
}

/*
* QThread_Create
*/
void idThreadsSystemLocal::Thread_Join( qthread_t* thread )
{
    systemThreadsSystem->Thread_Join( thread );
}

/*
* QThreads_Init
*/
void idThreadsSystemLocal::Threads_Init( void )
{
    S32 ret;
    
    global_mutex = nullptr;
    
    ret = systemThreadsSystem->Mutex_Create( &global_mutex );
    if ( ret != 0 )
    {
        return;
    }
}

/*
* QThreads_Shutdown
*/
void idThreadsSystemLocal::Threads_Shutdown( void )
{
    if ( global_mutex != nullptr )
    {
        systemThreadsSystem->Mutex_Destroy( global_mutex );
        global_mutex = nullptr;
    }
}
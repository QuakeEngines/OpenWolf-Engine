////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2013 Victor Luchits
// Copyright(C) 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
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
// File name:   systemThreads.cpp
// Version:     v1.00
// Created:     11/06/2019
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

struct qthread_s
{
    SDL_Thread* t;
};

struct qmutex_s
{
    SDL_mutex* m;
};

struct qcondvar_s
{
    SDL_cond* c;
};

idSystemThreadsLocal systemThreadsLocal;
idSystemThreadsSystem* systemThreadsSystem = &systemThreadsLocal;

/*
===============
idConsoleCursesLocal::idConsoleCursesLocal
===============
*/
idSystemThreadsLocal::idSystemThreadsLocal( void )
{
}

/*
===============
idConsoleCursesLocal::~idConsoleCursesLocal
===============
*/
idSystemThreadsLocal::~idSystemThreadsLocal( void )
{
}

/*
* Sys_Mutex_Create
*/
int idSystemThreadsLocal::Mutex_Create( qmutex_t** pmutex )
{
    qmutex_t* mutex;
    
    mutex = ( qmutex_t* )malloc( sizeof( *mutex ) );
    mutex->m = SDL_CreateMutex();
    
    *pmutex = mutex;
    return 0;
}

/*
* Sys_Mutex_Destroy
*/
void idSystemThreadsLocal::Mutex_Destroy( qmutex_t* mutex )
{
    if ( !mutex )
    {
        return;
    }
    
    SDL_DestroyMutex( mutex->m );
    free( mutex );
}

/*
* Sys_Mutex_Lock
*/
void idSystemThreadsLocal::Mutex_Lock( qmutex_t* mutex )
{
    SDL_LockMutex( mutex->m );
}

/*
* Sys_Mutex_Unlock
*/
void idSystemThreadsLocal::Mutex_Unlock( qmutex_t* mutex )
{
    SDL_UnlockMutex( mutex->m );
}

/*
* Sys_Thread_Create
*/
int idSystemThreadsLocal::Thread_Create( qthread_t** pthread, void * ( *routine )( void* ), void* param )
{
    qthread_t* thread;
    
    thread = ( qthread_t* )memorySystem->Malloc( sizeof( *thread ) );
    thread->t = SDL_CreateThread( ( SDL_ThreadFunction )routine, nullptr, param );
    
    *pthread = thread;
    return 0;
}

/*
* Sys_Thread_Join
*/
void idSystemThreadsLocal::Thread_Join( qthread_t* thread )
{
    S32 status = 0;
    
    if ( thread )
    {
        SDL_WaitThread( thread->t, &status );
        free( thread );
    }
}

/*
* Sys_Thread_Yield
*/
void idSystemThreadsLocal::Thread_Yield( void )
{
    idsystem->SysSleep( 0 );
}

/*
* Sys_Thread_Cancel
*/
S32 idSystemThreadsLocal::Thread_Cancel( qthread_t* thread )
{
    assert( false && "NOT IMPLEMENTED" );
    return -1;
}

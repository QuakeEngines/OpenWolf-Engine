////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 id Software, Inc.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_glimp.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

S32 qglMajorVersion, qglMinorVersion;
S32 qglesMajorVersion, qglesMinorVersion;

idRenderSystemGlimpLocal renderSystemGlimpLocal;

/*
===============
idRenderSystemGlimpLocal::idRenderSystemGlimpLocal
===============
*/
idRenderSystemGlimpLocal::idRenderSystemGlimpLocal( void )
{
}

/*
===============
idRenderSystemGlimpLocal::~idRenderSystemGlimpLocal
===============
*/
idRenderSystemGlimpLocal::~idRenderSystemGlimpLocal( void )
{
}

static SDL_Window* SDL_window = nullptr;
static SDL_GLContext SDL_glContext = nullptr;

void ( APIENTRYP qglActiveTextureARB )( U32 texture );
void ( APIENTRYP qglClientActiveTextureARB )( U32 texture );
void ( APIENTRYP qglMultiTexCoord2fARB )( U32 target, F32 s, F32 t );
void ( APIENTRYP qglLockArraysEXT )( S32 first, S32 count );
void ( APIENTRYP qglUnlockArraysEXT )( void );

#define GLE(ret, name, ...) name##proc * qgl##name;
QGL_1_1_PROCS;
QGL_1_1_FIXED_FUNCTION_PROCS;
QGL_DESKTOP_1_1_PROCS;
QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
QGL_ES_1_1_PROCS;
QGL_ES_1_1_FIXED_FUNCTION_PROCS;
QGL_1_3_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
QGL_3_0_PROCS;
QGL_4_0_PROCS;
QGL_ARB_occlusion_query_PROCS;
QGL_ARB_framebuffer_object_PROCS;
QGL_ARB_vertex_array_object_PROCS;
QGL_EXT_direct_state_access_PROCS;
#undef GLE

convar_t* r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
convar_t* r_allowResize; // make window resizable
convar_t* r_centerWindow;
convar_t* r_sdlDriver;

/*
===============
idRenderSystemLocal::MainWindow
===============
*/
void* idRenderSystemLocal::MainWindow( void )
{
    return SDL_window;
}

/*
===============
idRenderSystemGlimpLocal::Minimize

Minimize the game so that user is back at the desktop
===============
*/
void idRenderSystemGlimpLocal::Minimize( void )
{
    SDL_MinimizeWindow( SDL_window );
}

/*
===============
idRenderSystemGlimpLocal::LogComment
===============
*/
void idRenderSystemGlimpLocal::LogComment( StringEntry comment )
{
}

/*
===============
idRenderSystemGlimpLocal::CompareModes
===============
*/
S32 idRenderSystemGlimpLocal::CompareModes( const void* a, const void* b )
{
    const F32 ASPECT_EPSILON = 0.001f;
    SDL_Rect* modeA = ( SDL_Rect* )a;
    SDL_Rect* modeB = ( SDL_Rect* )b;
    F32 aspectA = ( F32 )modeA->w / ( F32 )modeA->h;
    F32 aspectB = ( F32 )modeB->w / ( F32 )modeB->h;
    S32 areaA = modeA->w * modeA->h;
    S32 areaB = modeB->w * modeB->h;
    F32 aspectDiffA = fabs( aspectA - displayAspect );
    F32 aspectDiffB = fabs( aspectB - displayAspect );
    F32 aspectDiffsDiff = aspectDiffA - aspectDiffB;
    
    if ( aspectDiffsDiff > ASPECT_EPSILON )
    {
        return 1;
    }
    else if ( aspectDiffsDiff < -ASPECT_EPSILON )
    {
        return -1;
    }
    else
    {
        return areaA - areaB;
    }
}

/*
===============
idRenderSystemGlimpLocal::DetectAvailableModes
===============
*/
void idRenderSystemGlimpLocal::DetectAvailableModes( void )
{
    S32 i, j, numSDLModes, numModes = 0;
    UTF8 buf[ MAX_STRING_CHARS ] = { 0 };
    SDL_Rect* modes;
    SDL_DisplayMode windowMode;
    
    S32 display = SDL_GetWindowDisplayIndex( SDL_window );
    if ( display < 0 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Couldn't get window display index, no resolutions detected: %s\n", SDL_GetError() );
        return;
    }
    numSDLModes = SDL_GetNumDisplayModes( display );
    
    if ( SDL_GetWindowDisplayMode( SDL_window, &windowMode ) < 0 || numSDLModes <= 0 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError() );
        return;
    }
    
    modes = ( SDL_Rect* )SDL_calloc( ( size_t )numSDLModes, sizeof( SDL_Rect ) );
    if ( !modes )
    {
        Com_Error( ERR_FATAL, "Out of memory" );
    }
    
    for ( i = 0; i < numSDLModes; i++ )
    {
        SDL_DisplayMode mode;
        
        if ( SDL_GetDisplayMode( display, i, &mode ) < 0 )
            continue;
            
        if ( !mode.w || !mode.h )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Display supports any resolution\n" );
            SDL_free( modes );
            return;
        }
        
        if ( windowMode.format != mode.format )
        {
            continue;
        }
        
        // SDL can give the same resolution with different refresh rates.
        // Only list resolution once.
        for ( j = 0; j < numModes; j++ )
        {
            if ( mode.w == modes[j].w && mode.h == modes[j].h )
            {
                break;
            }
        }
        
        if ( j != numModes )
        {
            continue;
        }
        
        modes[ numModes ].w = mode.w;
        modes[ numModes ].h = mode.h;
        numModes++;
    }
    
    if ( numModes > 1 )
    {
        qsort( modes, numModes, sizeof( SDL_Rect ), CompareModes );
    }
    
    for ( i = 0; i < numModes; i++ )
    {
        StringEntry newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );
        
        if ( ::strlen( newModeString ) < ( S32 )sizeof( buf ) - ::strlen( buf ) )
        {
            Q_strcat( buf, sizeof( buf ), newModeString );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_WARNING, "Skipping mode %ux%u, buffer too small\n", modes[ i ].w, modes[ i ].h );
        }
    }
    
    if ( *buf )
    {
        buf[ strlen( buf ) - 1 ] = 0;
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Available modes: '%s'\n", buf );
        cvarSystem->Set( "r_availableModes", buf );
    }
    
    SDL_free( modes );
}

/*
===============
idRenderSystemGlimpLocal::GetProcAddresses

Get addresses for OpenGL functions.
===============
*/
bool idRenderSystemGlimpLocal::GetProcAddresses( bool fixedFunction )
{
    bool success = true;
    StringEntry version;
    
#ifdef __SDL_NOGETPROCADDR__
#define GLE( ret, name, ... ) qgl##name = gl#name;
#else
#define GLE( ret, name, ... ) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name); \
	if ( qgl##name == nullptr ) { \
		clientMainSystem->RefPrintf( PRINT_DEVELOPER, "ERROR: Missing OpenGL function %s\n", "gl" #name ); \
		success = false; \
	}
#endif
    
    // OpenGL 1.0 and OpenGL ES 1.0
    GLE( const U8*, GetString, U32 name )
    
    if ( !qglGetString )
    {
        Com_Error( ERR_FATAL, "glGetString is nullptr" );
    }
    
    version = ( StringEntry )qglGetString( GL_VERSION );
    
    if ( !version )
    {
        Com_Error( ERR_FATAL, "GL_VERSION is nullptr\n" );
    }
    
    if ( Q_stricmpn( "OpenGL ES", version, 9 ) == 0 )
    {
        UTF8 profile[6]; // ES, ES-CM, or ES-CL
        
        ::sscanf( version, "OpenGL %5s %d.%d", profile, &qglesMajorVersion, &qglesMinorVersion );
        
        // common lite profile (no floating point) is not supported
        if ( Q_stricmp( profile, "ES-CL" ) == 0 )
        {
            qglesMajorVersion = 0;
            qglesMinorVersion = 0;
        }
    }
    else
    {
        ::sscanf( version, "%d.%d", &qglMajorVersion, &qglMinorVersion );
    }
    
    if ( fixedFunction )
    {
        if ( QGL_VERSION_ATLEAST( 1, 2 ) )
        {
            QGL_1_1_PROCS;
            QGL_1_1_FIXED_FUNCTION_PROCS;
            QGL_DESKTOP_1_1_PROCS;
            QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
        }
        else if ( qglesMajorVersion == 1 && qglesMinorVersion >= 1 )
        {
            // OpenGL ES 1.1 (2.0 is not backward compatible)
            QGL_1_1_PROCS;
            QGL_1_1_FIXED_FUNCTION_PROCS;
            QGL_ES_1_1_PROCS;
            QGL_ES_1_1_FIXED_FUNCTION_PROCS;
            QGL_1_3_PROCS;
            
            // error so this doesn't segfault due to nullptr desktop GL functions being used
            Com_Error( ERR_FATAL, "Unsupported OpenGL Version: %s\n", version );
        }
        else
        {
            Com_Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 1.2 is required\n", version );
        }
    }
    else
    {
        if ( QGL_VERSION_ATLEAST( 2, 0 ) )
        {
            QGL_1_1_PROCS;
            QGL_DESKTOP_1_1_PROCS;
            QGL_1_3_PROCS;
            QGL_1_5_PROCS;
            QGL_2_0_PROCS;
        }
        else if ( QGLES_VERSION_ATLEAST( 2, 0 ) )
        {
            QGL_1_1_PROCS;
            QGL_ES_1_1_PROCS;
            QGL_1_3_PROCS;
            QGL_1_5_PROCS;
            QGL_2_0_PROCS;
            QGL_ARB_occlusion_query_PROCS;
            
            // error so this doesn't segfault due to nullptr desktop GL functions being used
            Com_Error( ERR_FATAL, "Unsupported OpenGL Version: %s\n", version );
        }
        else
        {
            Com_Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 2.0 is required\n", version );
        }
    }
    
    if ( QGL_VERSION_ATLEAST( 3, 0 ) || QGLES_VERSION_ATLEAST( 3, 0 ) )
    {
        QGL_3_0_PROCS;
    }
    
    if ( QGL_VERSION_ATLEAST( 4, 0 ) || QGLES_VERSION_ATLEAST( 4, 0 ) )
    {
        QGL_4_0_PROCS;
    }
    
#undef GLE
    
    return success;
}

/*
===============
idRenderSystemGlimpLocal::ClearProcAddresses

Clear addresses for OpenGL functions.
===============
*/
void idRenderSystemGlimpLocal::ClearProcAddresses( void )
{
#define GLE( ret, name, ... ) qgl##name = nullptr;

    qglMajorVersion = 0;
    qglMinorVersion = 0;
    qglesMajorVersion = 0;
    qglesMinorVersion = 0;
    
    QGL_1_1_PROCS;
    QGL_1_1_FIXED_FUNCTION_PROCS;
    QGL_DESKTOP_1_1_PROCS;
    QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
    QGL_ES_1_1_PROCS;
    QGL_ES_1_1_FIXED_FUNCTION_PROCS;
    QGL_1_3_PROCS;
    QGL_1_5_PROCS;
    QGL_2_0_PROCS;
    QGL_3_0_PROCS;
    QGL_4_0_PROCS;
    QGL_ARB_occlusion_query_PROCS;
    QGL_ARB_framebuffer_object_PROCS;
    QGL_ARB_vertex_array_object_PROCS;
    QGL_EXT_direct_state_access_PROCS;
    
    qglActiveTextureARB = nullptr;
    qglClientActiveTextureARB = nullptr;
    qglMultiTexCoord2fARB = nullptr;
    
    qglLockArraysEXT = nullptr;
    qglUnlockArraysEXT = nullptr;
    
#undef GLE
}

/*
===============
idRenderSystemGlimpLocal::SetMode
===============
*/
S32 idRenderSystemGlimpLocal::SetMode( S32 mode, bool fullscreen, bool noborder, bool fixedFunction )
{
    S32 perChannelColorBits, colorBits, depthBits, stencilBits, samples, i = 0, display = 0, x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
    U32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    StringEntry glstring;
    SDL_Surface* icon = nullptr;
    SDL_DisplayMode desktopMode;
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Initializing OpenGL display\n" );
    
    if ( r_allowResize->integer )
    {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    
    // If a window exists, note its display index
    if ( SDL_window != nullptr )
    {
        display = SDL_GetWindowDisplayIndex( SDL_window );
        
        if ( display < 0 )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
        }
    }
    
    if ( display >= 0 && SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
    {
        displayAspect = ( F32 )desktopMode.w / ( F32 )desktopMode.h;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Display aspect: %.3f\n", displayAspect );
    }
    else
    {
        ::memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Cannot determine display aspect, assuming 1.333\n" );
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...setting mode %d:", mode );
    
    if ( mode == -2 )
    {
        // use desktop video resolution
        if ( desktopMode.h > 0 )
        {
            glConfig.vidWidth = desktopMode.w;
            glConfig.vidHeight = desktopMode.h;
        }
        else
        {
            glConfig.vidWidth = 640;
            glConfig.vidHeight = 480;
            
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Cannot determine display resolution, assuming 640x480\n" );
        }
        
        glConfig.windowAspect = ( F32 )glConfig.vidWidth / ( F32 )glConfig.vidHeight;
    }
    else if ( !idRenderSystemInitLocal::GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, " invalid mode\n" );
        return RSERR_INVALID_MODE;
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );
    
    // Center window
    if ( r_centerWindow->integer && !fullscreen )
    {
        x = ( desktopMode.w / 2 ) - ( glConfig.vidWidth / 2 );
        y = ( desktopMode.h / 2 ) - ( glConfig.vidHeight / 2 );
    }
    
    // Destroy existing state if it exists
    if ( SDL_glContext != nullptr )
    {
        ClearProcAddresses();
        SDL_GL_DeleteContext( SDL_glContext );
        SDL_glContext = nullptr;
    }
    
    if ( SDL_window != nullptr )
    {
        SDL_GetWindowPosition( SDL_window, &x, &y );
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y );
        SDL_DestroyWindow( SDL_window );
        SDL_window = nullptr;
    }
    
    if ( fullscreen )
    {
        flags |= SDL_WINDOW_FULLSCREEN;
        glConfig.isFullscreen = true;
    }
    else
    {
        if ( noborder )
        {
            flags |= SDL_WINDOW_BORDERLESS;
        }
        
        glConfig.isFullscreen = false;
    }
    
    colorBits = r_colorbits->integer;
    if ( ( !colorBits ) || ( colorBits >= 32 ) )
    {
        colorBits = 24;
    }
    
    if ( !r_depthbits->value )
    {
        depthBits = 24;
    }
    else
    {
        depthBits = r_depthbits->integer;
    }
    
    stencilBits = r_stencilbits->integer;
    samples = r_ext_multisample->integer;
    
    for ( i = 0; i < 16; i++ )
    {
        S32 testColorBits, testDepthBits, testStencilBits;
        S32 realColorBits[3];
        
        // 0 - default
        // 1 - minus colorBits
        // 2 - minus depthBits
        // 3 - minus stencil
        if ( ( i % 4 ) == 0 && i )
        {
            // one pass, reduce
            switch ( i / 4 )
            {
                case 2:
                    if ( colorBits == 24 )
                    {
                        colorBits = 16;
                    }
                    break;
                case 1:
                    if ( depthBits == 24 )
                    {
                        depthBits = 16;
                    }
                    else if ( depthBits == 16 )
                    {
                        depthBits = 8;
                    }
                case 3:
                    if ( stencilBits == 24 )
                    {
                        stencilBits = 16;
                    }
                    else if ( stencilBits == 16 )
                    {
                        stencilBits = 8;
                    }
            }
        }
        
        testColorBits = colorBits;
        testDepthBits = depthBits;
        testStencilBits = stencilBits;
        
        if ( ( i % 4 ) == 3 )
        {
            // reduce colorBits
            if ( testColorBits == 24 )
            {
                testColorBits = 16;
            }
        }
        
        if ( ( i % 4 ) == 2 )
        {
            // reduce depthBits
            if ( testDepthBits == 24 )
            {
                testDepthBits = 16;
            }
            else if ( testDepthBits == 16 )
            {
                testDepthBits = 8;
            }
        }
        
        if ( ( i % 4 ) == 1 )
        {
            // reduce stencilBits
            if ( testStencilBits == 24 )
            {
                testStencilBits = 16;
            }
            else if ( testStencilBits == 16 )
            {
                testStencilBits = 8;
            }
            else
            {
                testStencilBits = 0;
            }
        }
        
        if ( testColorBits == 24 )
        {
            perChannelColorBits = 8;
        }
        else
        {
            perChannelColorBits = 4;
        }
        
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );
        
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );
        
        if ( r_stereoEnabled->integer )
        {
            glConfig.stereoEnabled = true;
            SDL_GL_SetAttribute( SDL_GL_STEREO, 1 );
        }
        else
        {
            glConfig.stereoEnabled = false;
            SDL_GL_SetAttribute( SDL_GL_STEREO, 0 );
        }
        
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        
        
        if ( ( SDL_window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y, glConfig.vidWidth, glConfig.vidHeight, flags ) ) == nullptr )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_CreateWindow failed: %s\n", SDL_GetError() );
            continue;
        }
        
        if ( fullscreen )
        {
            SDL_DisplayMode vidMode;
            
            switch ( testColorBits )
            {
                case 16:
                    vidMode.format = SDL_PIXELFORMAT_RGB565;
                    break;
                case 24:
                    vidMode.format = SDL_PIXELFORMAT_RGB24;
                    break;
                default:
                    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits );
                    continue;
            }
            
            if ( mode == -1 )
            {
                SDL_SetWindowFullscreen( SDL_window, SDL_WINDOW_FULLSCREEN_DESKTOP );
                SDL_GL_GetDrawableSize( SDL_window, &glConfig.vidWidth, &glConfig.vidHeight );
            }
            
            vidMode.w = glConfig.vidWidth;
            vidMode.h = glConfig.vidHeight;
            vidMode.refresh_rate = glConfig.displayFrequency = cvarSystem->VariableIntegerValue( "r_displayRefresh" );
            vidMode.driverdata = nullptr;
            
            if ( SDL_SetWindowDisplayMode( SDL_window, &vidMode ) < 0 )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError() );
                continue;
            }
        }
        
        SDL_SetWindowIcon( SDL_window, icon );
        
        if ( !fixedFunction )
        {
            S32 profileMask, majorVersion, minorVersion;
            
            SDL_GL_GetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, &profileMask );
            SDL_GL_GetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion );
            SDL_GL_GetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion );
            
            clientMainSystem->RefPrintf( PRINT_ALL, "Trying to get an OpenGL 3.3 core context\n" );
            
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
            
            if ( ( SDL_glContext = SDL_GL_CreateContext( SDL_window ) ) == nullptr )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Reverting to default context\n" );
                
                SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, profileMask );
                SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion );
                SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, minorVersion );
            }
            else
            {
                StringEntry renderer;
                
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_GL_CreateContext succeeded.\n" );
                
                if ( GetProcAddresses( fixedFunction ) )
                {
                    renderer = ( StringEntry )qglGetString( GL_RENDERER );
                }
                else
                {
                    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "idRenderSystemGlimpLocal::GetProcAdvedresses() failed for OpenGL 3.3 core context\n" );
                    renderer = nullptr;
                }
                
                if ( !renderer || ( strstr( renderer, "Software Renderer" ) || strstr( renderer, "Software Rasterizer" ) ) )
                {
                    if ( renderer )
                    {
                        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_RENDERER is %s, rejecting context\n", renderer );
                    }
                    
                    ClearProcAddresses();
                    SDL_GL_DeleteContext( SDL_glContext );
                    SDL_glContext = nullptr;
                    
                    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, profileMask );
                    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion );
                    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, minorVersion );
                }
            }
        }
        else
        {
            SDL_glContext = nullptr;
        }
        
        if ( !SDL_glContext )
        {
            if ( ( SDL_glContext = SDL_GL_CreateContext( SDL_window ) ) == nullptr )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
                SDL_DestroyWindow( SDL_window );
                SDL_window = nullptr;
                continue;
            }
            
            if ( !GetProcAddresses( fixedFunction ) )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "idRenderSystemGlimpLocal::GetProcAddresses() failed\n" );
                ClearProcAddresses();
                SDL_GL_DeleteContext( SDL_glContext );
                SDL_glContext = nullptr;
                SDL_DestroyWindow( SDL_window );
                SDL_window = nullptr;
                continue;
            }
        }
        
        qglClearColor( 0, 0, 0, 1 );
        qglClear( GL_COLOR_BUFFER_BIT );
        SDL_GL_SwapWindow( SDL_window );
        
        idRenderSystemCmdsLocal::IssuePendingRenderCommands();
        if ( SDL_GL_SetSwapInterval( r_swapInterval->integer ) == -1 )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError() );
        }
        
        SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &realColorBits[0] );
        SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &realColorBits[1] );
        SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &realColorBits[2] );
        SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &glConfig.depthBits );
        SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &glConfig.stencilBits );
        
        glConfig.colorBits = realColorBits[0] + realColorBits[1] + realColorBits[2];
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Using %d color bits, %d depth, %d stencil display.\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
        break;
    }
    
    if ( SDL_glContext == nullptr )
    {
        SDL_FreeSurface( icon );
        return RSERR_UNKNOWN;
    }
    
    if ( !SDL_window )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Couldn't get a visual\n" );
        return RSERR_INVALID_MODE;
    }
    
    DetectAvailableModes();
    
    glstring = ( UTF8* )qglGetString( GL_RENDERER );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "GL_RENDERER: %s\n", glstring );
    
    // fix mouse when unfocused/minimized
    SDL_MinimizeWindow( SDL_window );
    SDL_RestoreWindow( SDL_window );
    
    return RSERR_OK;
}

/*
===============
idRenderSystemGlimpLocal::StartDriverAndSetMode
===============
*/
bool idRenderSystemGlimpLocal::StartDriverAndSetMode( S32 mode, bool fullscreen, bool noborder, bool gl3Core )
{
    S32 num_displays, dpy;
    rserr_t err;
    SDL_DisplayMode modeSDL;
    
    if ( fullscreen && cvarSystem->VariableIntegerValue( "in_nograb" ) )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Fullscreen not allowed with in_nograb 1\n" );
        cvarSystem->Set( "r_fullscreen", "0" );
        r_fullscreen->modified = false;
        fullscreen = false;
    }
    
    if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
    {
        StringEntry driverName;
        
        if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError() );
            return false;
        }
        
        driverName = SDL_GetCurrentVideoDriver();
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "SDL using driver \"%s\"\n", driverName );
        cvarSystem->Set( "r_sdlDriver", driverName );
    }
    
    num_displays = SDL_GetNumVideoDisplays();
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "See %d displays.\n", num_displays );
    
    for ( dpy = 0; dpy < num_displays; dpy++ )
    {
        const S32 num_modes = SDL_GetNumDisplayModes( dpy );
        SDL_Rect rect = { 0, 0, 0, 0 };
        F32 ddpi, hdpi, vdpi;
        S32 m;
        
        SDL_GetDisplayBounds( dpy, &rect );
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "%d: \"%s\" (%dx%d, (%d, %d)), %d modes.\n", dpy, SDL_GetDisplayName( dpy ), rect.w, rect.h, rect.x, rect.y, num_modes );
        
        if ( SDL_GetDisplayDPI( dpy, &ddpi, &hdpi, &vdpi ) == -1 )
        {
            Com_Error( ERR_DROP, " DPI: failed to query (%s)\n", SDL_GetError() );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "DPI: ddpi=%f; hdpi=%f; vdpi=%f\n", ddpi, hdpi, vdpi );
        }
        
        if ( SDL_GetCurrentDisplayMode( dpy, &modeSDL ) == -1 )
        {
            Com_Error( ERR_DROP, " CURRENT: failed to query (%s)\n", SDL_GetError() );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "CURRENT", &modeSDL );
        }
        
        if ( SDL_GetDesktopDisplayMode( dpy, &modeSDL ) == -1 )
        {
            Com_Error( ERR_DROP, " DESKTOP: failed to query (%s)\n", SDL_GetError() );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "DESKTOP", &modeSDL );
        }
        
        for ( m = 0; m < num_modes; m++ )
        {
            if ( SDL_GetDisplayMode( dpy, m, &modeSDL ) == -1 )
            {
                Com_Error( ERR_DROP, " MODE %d: failed to query (%s)\n", m, SDL_GetError() );
            }
            else
            {
                UTF8 prefix[64];
                snprintf( prefix, sizeof( prefix ), " MODE %d", m );
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, prefix, &modeSDL );
            }
        }
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "\n" );
    }
    
    if ( r_stereoEnabled->integer )
    {
        glConfig.stereoEnabled = true;
    }
    else
    {
        glConfig.stereoEnabled = false;
    }
    
    err = ( rserr_t )SetMode( mode, fullscreen, noborder, gl3Core );
    
    switch ( err )
    {
        case RSERR_INVALID_FULLSCREEN:
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...WARNING: fullscreen unavailable in this mode\n" );
            return false;
            
        case RSERR_INVALID_MODE:
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...WARNING: could not set the given mode (%d)\n", mode );
            return false;
            
        default:
            break;
    }
    
    return true;
}

/*
===============
idRenderSystemGlimpLocal::HaveExtension
===============
*/
bool idRenderSystemGlimpLocal::HaveExtension( StringEntry ext )
{
    StringEntry ptr = Q_stristr( glConfig.extensions_string, ext );
    
    if ( ptr == nullptr )
    {
        return false;
    }
    
    ptr += strlen( ext );
    // verify it's complete string.
    return ( ( *ptr == ' ' ) || ( *ptr == '\0' ) );
}

/*
===============
idRenderSystemGlimpLocal::InitExtensions
===============
*/
void idRenderSystemGlimpLocal::InitExtensions( void )
{
    if ( !r_allowExtensions->integer )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "* IGNORING OPENGL EXTENSIONS *\n" );
        return;
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Initializing OpenGL extensions\n" );
    
    glConfig.textureCompression = TC_NONE;
    
    // GL_EXT_texture_compression_s3tc
    if ( HaveExtension( "GL_ARB_texture_compression" ) && HaveExtension( "GL_EXT_texture_compression_s3tc" ) )
    {
        if ( r_ext_compressed_textures->value )
        {
            glConfig.textureCompression = TC_S3TC_ARB;
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GL_EXT_texture_compression_s3tc\n" );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...ignoring GL_EXT_texture_compression_s3tc\n" );
        }
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_EXT_texture_compression_s3tc not found\n" );
    }
    
    // GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
    if ( glConfig.textureCompression == TC_NONE )
    {
        if ( HaveExtension( "GL_S3_s3tc" ) )
        {
            if ( r_ext_compressed_textures->value )
            {
                glConfig.textureCompression = TC_S3TC;
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GL_S3_s3tc\n" );
            }
            else
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...ignoring GL_S3_s3tc\n" );
            }
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_S3_s3tc not found\n" );
        }
    }
    
    // GL_EXT_texture_env_add
    glConfig.textureEnvAddAvailable = false;
    if ( HaveExtension( "EXT_texture_env_add" ) )
    {
        if ( r_ext_texture_env_add->integer )
        {
            glConfig.textureEnvAddAvailable = true;
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GL_EXT_texture_env_add\n" );
        }
        else
        {
            glConfig.textureEnvAddAvailable = false;
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...ignoring GL_EXT_texture_env_add\n" );
        }
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_EXT_texture_env_add not found\n" );
    }
    
    // GL_ARB_multitexture
    qglMultiTexCoord2fARB = nullptr;
    qglActiveTextureARB = nullptr;
    qglClientActiveTextureARB = nullptr;
    if ( HaveExtension( "GL_ARB_multitexture" ) )
    {
        if ( r_ext_multitexture->value )
        {
            qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC )SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
            qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC )SDL_GL_GetProcAddress( "glActiveTextureARB" );
            qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC )SDL_GL_GetProcAddress( "glClientActiveTextureARB" );
            
            if ( qglActiveTextureARB )
            {
                S32 glint = 16; //Dushan
                qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
                glConfig.numTextureUnits = ( S32 )glint;
                
                if ( glConfig.numTextureUnits > 1 )
                {
                    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GL_ARB_multitexture\n" );
                }
                else
                {
                    qglMultiTexCoord2fARB = nullptr;
                    qglActiveTextureARB = nullptr;
                    qglClientActiveTextureARB = nullptr;
                    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...not using GL_ARB_multitexture, < 2 texture units\n" );
                }
            }
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...ignoring GL_ARB_multitexture\n" );
        }
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_ARB_multitexture not found\n" );
    }
    
    
    // GL_EXT_compiled_vertex_array
    if ( HaveExtension( "GL_EXT_compiled_vertex_array" ) )
    {
        if ( r_ext_compiled_vertex_array->value )
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GL_EXT_compiled_vertex_array\n" );
            qglLockArraysEXT = ( void ( APIENTRY* )( S32, S32 ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
            qglUnlockArraysEXT = ( void ( APIENTRY* )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
            if ( !qglLockArraysEXT || !qglUnlockArraysEXT )
            {
                Com_Error( ERR_FATAL, "bad getprocaddress" );
            }
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...ignoring GL_EXT_compiled_vertex_array\n" );
        }
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_EXT_compiled_vertex_array not found\n" );
    }
    
    glConfig.textureFilterAnisotropic = false;
    if ( HaveExtension( "GL_EXT_texture_filter_anisotropic" ) )
    {
        if ( r_ext_texture_filter_anisotropic->integer )
        {
            qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, ( S32* )&glConfig.maxAnisotropy );
            if ( glConfig.maxAnisotropy <= 0 )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
                glConfig.maxAnisotropy = 0;
            }
            else
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GL_EXT_texture_filter_anisotropic (max: %i)\n", glConfig.maxAnisotropy );
                glConfig.textureFilterAnisotropic = true;
            }
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
        }
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...GL_EXT_texture_filter_anisotropic not found\n" );
    }
}

/*
===============
idRenderSystemGlimpLocal::Splash
===============
*/
void idRenderSystemGlimpLocal::Splash( void )
{
    U8 splashData[144000]; // width * height * bytes_per_pixel
    SDL_Surface* splashImage = nullptr;
    
    // decode splash image
    SPLASH_IMAGE_RUN_LENGTH_DECODE( splashData,
                                    CLIENT_WINDOW_SPLASH.rle_pixel_data,
                                    CLIENT_WINDOW_SPLASH.width * CLIENT_WINDOW_SPLASH.height,
                                    CLIENT_WINDOW_SPLASH.bytes_per_pixel );
                                    
    // get splash image
    splashImage = SDL_CreateRGBSurfaceFrom(
                      ( void* )splashData,
                      CLIENT_WINDOW_SPLASH.width,
                      CLIENT_WINDOW_SPLASH.height,
                      CLIENT_WINDOW_SPLASH.bytes_per_pixel * 8,
                      CLIENT_WINDOW_SPLASH.bytes_per_pixel * CLIENT_WINDOW_SPLASH.width,
#ifdef Q3_LITTLE_ENDIAN
                      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
                      0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
                  );
                  
    SDL_Rect dstRect;
    dstRect.x = glConfig.vidWidth / 2 - splashImage->w / 2;
    dstRect.y = glConfig.vidHeight / 2 - splashImage->h / 2;
    dstRect.w = splashImage->w;
    dstRect.h = splashImage->h;
    
    // apply image on surface
    SDL_BlitSurface( splashImage, nullptr, SDL_GetWindowSurface( SDL_window ), &dstRect );
    SDL_UpdateWindowSurface( SDL_window );
    
    SDL_FreeSurface( splashImage );
}

/*
===============
idRenderSystemGlimpLocal::Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void idRenderSystemGlimpLocal::Init( bool fixedFunction )
{
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "idRenderSystemGlimpLocal::Init( )\n" );
    
    r_allowSoftwareGL = cvarSystem->Get( "r_allowSoftwareGL", "0", CVAR_LATCH, "description" );
    r_sdlDriver = cvarSystem->Get( "r_sdlDriver", "", CVAR_ROM, "description" );
    r_allowResize = cvarSystem->Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    r_centerWindow = cvarSystem->Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH, "description" );
    
    if ( cvarSystem->VariableIntegerValue( "com_abnormalExit" ) )
    {
        cvarSystem->Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
        cvarSystem->Set( "r_fullscreen", "0" );
        cvarSystem->Set( "r_centerWindow", "0" );
        cvarSystem->Set( "com_abnormalExit", "0" );
    }
    
    idsystem->GLimpInit();
    
    // Create the window and set up the context
    if ( StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, r_noborder->integer, fixedFunction ) )
    {
        goto success;
    }
    
    // Try again, this time in a platform specific "safe mode"
    idsystem->GLimpSafeInit();
    
    if ( StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, false, fixedFunction ) )
    {
        goto success;
    }
    
    // Finally, try the default screen resolution
    if ( r_mode->integer != R_MODE_FALLBACK )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK );
        
        if ( StartDriverAndSetMode( R_MODE_FALLBACK, false, false, fixedFunction ) )
        {
            goto success;
        }
    }
    
    // Nothing worked, give up
    Com_Error( ERR_FATAL, "idRenderSystemGlimpLocal::Init() - could not load OpenGL subsystem" );
    
success:
    // These values force the UI to disable driver selection
    glConfig.driverType = GLDRV_ICD;
    glConfig.hardwareType = GLHW_GENERIC;
    
    // Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
    glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer && SDL_SetWindowBrightness( SDL_window, 1.0f ) >= 0;
    
    // get our config strings
    Q_strncpyz( glConfig.vendor_string, ( UTF8* ) qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
    Q_strncpyz( glConfig.renderer_string, ( UTF8* ) qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
    if ( *glConfig.renderer_string && glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] == '\n' )
    {
        glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] = 0;
    }
    Q_strncpyz( glConfig.version_string, ( UTF8* ) qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
    
    // manually create extension list if using OpenGL 4
    if ( qglGetStringi )
    {
        S32 i, numExtensions, extensionLength, listLength;
        StringEntry extension;
        
        qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
        listLength = 0;
        
        for ( i = 0; i < numExtensions; i++ )
        {
            extension = ( UTF8* ) qglGetStringi( GL_EXTENSIONS, i );
            extensionLength = ( S32 )::strlen( extension );
            
            if ( ( listLength + extensionLength + 1 ) >= sizeof( glConfig.extensions_string ) )
            {
                break;
            }
            
            if ( i > 0 )
            {
                Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), " " );
                listLength++;
            }
            
            Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), extension );
            listLength += extensionLength;
        }
    }
    else
    {
        Q_strncpyz( glConfig.extensions_string, ( UTF8* ) qglGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
    }
    
    // initialize extensions
    InitExtensions();
    
    cvarSystem->Get( "r_availableModes", "", CVAR_ROM, "description" );
    
    // Display splash screen
    Splash();
    
    // This depends on SDL_INIT_VIDEO, hence having it here
    idsystem->InputInit( SDL_window );
}

/*
===============
idRenderSystemGlimpLocal::EndFrame

Responsible for doing a swapbuffers
===============
*/
void idRenderSystemGlimpLocal::EndFrame( void )
{
    // don't flip if drawing to front buffer
    if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
    {
        SDL_GL_SwapWindow( SDL_window );
    }
    
    if ( r_fullscreen->modified )
    {
        S32 fullscreen;
        bool needToToggle;
        bool sdlToggled = false;
        
        // Find out the current state
        fullscreen = !!( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN );
        
        if ( r_fullscreen->integer && cvarSystem->VariableIntegerValue( "in_nograb" ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
            
            cvarSystem->Set( "r_fullscreen", "0" );
            
            r_fullscreen->modified = false;
        }
        
        // Is the state we want different from the current state?
        needToToggle = !!r_fullscreen->integer != fullscreen;
        
        if ( needToToggle )
        {
            sdlToggled = SDL_SetWindowFullscreen( SDL_window, r_fullscreen->integer ) >= 0;
            
            // SDL_WM_ToggleFullScreen didn't work, so do it the slow way
            if ( !sdlToggled )
            {
                cmdBufferSystem->ExecuteText( EXEC_APPEND, "vid_restart\n" );
            }
            
            idsystem->Restart_f();
        }
        
        r_fullscreen->modified = false;
    }
}

/*
===============
idRenderSystemGlimpLocal::Shutdown
===============
*/
void idRenderSystemGlimpLocal::Shutdown( void )
{
    idsystem->Shutdown();
    
    if ( SDL_glContext )
    {
        SDL_GL_DeleteContext( SDL_glContext );
        SDL_glContext = nullptr;
    }
    
    if ( SDL_window )
    {
        SDL_DestroyWindow( SDL_window );
        SDL_window = nullptr;
    }
    
    SDL_QuitSubSystem( SDL_INIT_VIDEO );
    
    ::memset( &glConfig, 0, sizeof( glConfig ) );
    ::memset( &glState, 0, sizeof( glState ) );
}

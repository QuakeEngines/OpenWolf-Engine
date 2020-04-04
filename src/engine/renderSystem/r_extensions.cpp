
////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011 James Canete (use.less01@gmail.com)
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
// File name:   r_extensions.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: extensions needed by the renderer not in r_glimp.cpp
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemExtensionsLocal renderSystemExtensionsLocal;

/*
===============
idRenderSystemExtensionsLocal::idRenderSystemExtensionsLocal
===============
*/
idRenderSystemExtensionsLocal::idRenderSystemExtensionsLocal( void )
{
}

/*
===============
idRenderSystemExtensionsLocal::~idRenderSystemExtensionsLocal
===============
*/
idRenderSystemExtensionsLocal::~idRenderSystemExtensionsLocal( void )
{
}

/*
===============
idRenderSystemExtrensionsLocal::InitExtraExtensions
===============
*/
void idRenderSystemExtensionsLocal::InitExtraExtensions( void )
{
    //S32 len;
    UTF8* extension;
    const UTF8* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };
    bool q_gl_version_at_least_3_0;
    bool q_gl_version_at_least_3_2;
    
    // Check OpenGL version
    if ( !QGL_VERSION_ATLEAST( 2, 0 ) )
    {
        Com_Error( ERR_FATAL, "OpenGL 2.0 required!" );
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using OpenGL %s\n", glConfig.version_string );
    
    q_gl_version_at_least_3_0 = QGL_VERSION_ATLEAST( 3, 0 );
    q_gl_version_at_least_3_2 = QGL_VERSION_ATLEAST( 3, 2 );
    
    // Check if we need Intel graphics specific fixes.
    glRefConfig.intelGraphics = false;
    if ( ::strstr( ( UTF8* )qglGetString( GL_RENDERER ), "Intel" ) )
    {
        glRefConfig.intelGraphics = true;
    }
    
#define GLE(ret, name, ...) qgl##name = &idRenderSystemDSALocal::name;
    // set DSA fallbacks
    QGL_EXT_direct_state_access_PROCS;
#undef GLE
    
#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name);
    QGL_1_1_PROCS
    QGL_1_1_FIXED_FUNCTION_PROCS;
    QGL_DESKTOP_1_1_PROCS;
    QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
    
    // OpenGL 1.3, was GL_ARB_texture_compression
    QGL_1_3_PROCS;
    
    // OpenGL 1.5, was GL_ARB_vertex_buffer_object and GL_ARB_occlusion_query
    QGL_1_5_PROCS;
    QGL_ARB_occlusion_query_PROCS;
    glRefConfig.occlusionQuery = true;
    
    // OpenGL 2.0, was GL_ARB_shading_language_100, GL_ARB_vertex_program, GL_ARB_shader_objects, and GL_ARB_vertex_shader
    QGL_2_0_PROCS;
    QGL_3_0_PROCS;
    
    //Dushan
    QGL_4_0_PROCS;
    
    // OpenGL 3.0 - GL_ARB_framebuffer_object
    extension = "GL_ARB_framebuffer_object";
    glRefConfig.framebufferObject = false;
    glRefConfig.framebufferBlit = false;
    glRefConfig.framebufferMultisample = false;
    if ( q_gl_version_at_least_3_0 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.framebufferObject = !!r_ext_framebuffer_object->integer;
        glRefConfig.framebufferBlit = true;
        glRefConfig.framebufferMultisample = true;
        
        qglGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glRefConfig.maxRenderbufferSize );
        qglGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glRefConfig.maxColorAttachments );
        
        QGL_ARB_framebuffer_object_PROCS;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[glRefConfig.framebufferObject], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    // OpenGL 3.0 - GL_ARB_vertex_array_object
    extension = "GL_ARB_vertex_array_object";
    glRefConfig.vertexArrayObject = false;
    if ( q_gl_version_at_least_3_0 || SDL_GL_ExtensionSupported( extension ) )
    {
        if ( q_gl_version_at_least_3_0 )
        {
            // force VAO, core context requires it
            glRefConfig.vertexArrayObject = true;
        }
        else
        {
            glRefConfig.vertexArrayObject = !!r_arb_vertex_array_object->integer;
        }
        
        QGL_ARB_vertex_array_object_PROCS;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[glRefConfig.vertexArrayObject], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    // OpenGL 3.0 - GL_ARB_texture_float
    extension = "GL_ARB_texture_float";
    glRefConfig.textureFloat = false;
    if ( q_gl_version_at_least_3_0 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.textureFloat = !!r_ext_texture_float->integer;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[glRefConfig.textureFloat], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    // OpenGL 3.2 - GL_ARB_depth_clamp
    extension = "GL_ARB_depth_clamp";
    glRefConfig.depthClamp = false;
    if ( q_gl_version_at_least_3_2 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.depthClamp = true;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[glRefConfig.depthClamp], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    // OpenGL 3.2 - GL_ARB_seamless_cube_map
    extension = "GL_ARB_seamless_cube_map";
    glRefConfig.seamlessCubeMap = false;
    if ( q_gl_version_at_least_3_2 || SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.seamlessCubeMap = !!r_arb_seamless_cube_map->integer;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[glRefConfig.seamlessCubeMap], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    if ( !qglGetString )
    {
        Com_Error( ERR_FATAL, "glGetString is NULL" );
    }
    
    // Determine GLSL version
    if ( 1 )
    {
        UTF8 version[256];
        
        Q_strncpyz( version, ( UTF8* )qglGetString( GL_SHADING_LANGUAGE_VERSION ), sizeof( version ) );
        
        sscanf( version, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion );
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...using GLSL version %s\n", version );
    }
    
    glRefConfig.memInfo = MI_NONE;
    
    // GL_NVX_gpu_memory_info
    extension = "GL_NVX_gpu_memory_info";
    if ( SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.memInfo = MI_NVX;
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[1], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    // GL_ATI_meminfo
    extension = "GL_ATI_meminfo";
    if ( SDL_GL_ExtensionSupported( extension ) )
    {
        if ( glRefConfig.memInfo == MI_NONE )
        {
            glRefConfig.memInfo = MI_ATI;
            
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[1], extension );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[0], extension );
        }
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    glRefConfig.textureCompression = TCR_NONE;
    
    // GL_ARB_texture_compression_rgtc
    extension = "GL_ARB_texture_compression_rgtc";
    if ( SDL_GL_ExtensionSupported( extension ) )
    {
        bool useRgtc = r_ext_compressed_textures->integer >= 1;
        
        if ( useRgtc )
        {
            glRefConfig.textureCompression |= TCR_RGTC;
        }
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[useRgtc], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    glRefConfig.swizzleNormalmap = r_ext_compressed_textures->integer && !( glRefConfig.textureCompression & TCR_RGTC );
    
    // GL_ARB_texture_compression_bptc
    extension = "GL_ARB_texture_compression_bptc";
    if ( SDL_GL_ExtensionSupported( extension ) )
    {
        bool useBptc = r_ext_compressed_textures->integer >= 2;
        
        if ( useBptc )
        {
            glRefConfig.textureCompression |= TCR_BPTC;
        }
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[useBptc], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    extension = "GL_EXT_shadow_samplers";
    if ( SDL_GL_ExtensionSupported( extension ) )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[1], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
    // GL_EXT_direct_state_access
    extension = "GL_EXT_direct_state_access";
    glRefConfig.directStateAccess = false;
    if ( SDL_GL_ExtensionSupported( extension ) )
    {
        glRefConfig.directStateAccess = !!r_ext_direct_state_access->integer;
        
        // QGL_*_PROCS becomes several functions, do not remove {}
        if ( glRefConfig.directStateAccess )
        {
            QGL_EXT_direct_state_access_PROCS;
        }
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[glRefConfig.directStateAccess], extension );
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, result[2], extension );
    }
    
#undef GLE
}

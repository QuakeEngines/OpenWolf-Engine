////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
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
// File name:   r_cmds.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemCmdsLocal renderSystemCmdsLocal;

/*
===============
idRenderSystemCmdsLocal::idRenderSystemCmdsLocal
===============
*/
idRenderSystemCmdsLocal::idRenderSystemCmdsLocal( void )
{
}

/*
===============
idRenderSystemCmdsLocal::~idRenderSystemCmdsLocal
===============
*/
idRenderSystemCmdsLocal::~idRenderSystemCmdsLocal( void )
{
}

/*
=====================
idRenderSystemCmdsLocal::PerformanceCounters
=====================
*/
void idRenderSystemCmdsLocal::PerformanceCounters( void )
{
    if ( !r_speeds->integer )
    {
        // clear the counters even if we aren't printing
        ::memset( &tr.pc, 0, sizeof( tr.pc ) );
        ::memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
        return;
    }
    
    if ( r_speeds->integer == 1 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "%i/%i/%i shaders/batches/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
                                     backEnd.pc.c_shaders, backEnd.pc.c_surfBatches, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
                                     backEnd.pc.c_indexes / 3, backEnd.pc.c_totalIndexes / 3,
                                     idRenderSystemImageLocal::SumOfUsedImages() / ( 1000000.0f ), backEnd.pc.c_overDraw / ( F32 )( glConfig.vidWidth * glConfig.vidHeight ) );
    }
    else if ( r_speeds->integer == 2 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
                                     tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
                                     tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
        clientMainSystem->RefPrintf( PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
                                     tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
                                     tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
    }
    else if ( r_speeds->integer == 3 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
    }
    else if ( r_speeds->integer == 4 )
    {
        if ( backEnd.pc.c_dlightVertexes )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n",
                                         tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
                                         backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
        }
    }
    else if ( r_speeds->integer == 5 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
    }
    else if ( r_speeds->integer == 6 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n",
                                     backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
    }
    else if ( r_speeds->integer == 7 )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "VAO draws: static %i dynamic %i\n",
                                     backEnd.pc.c_staticVaoDraws, backEnd.pc.c_dynamicVaoDraws );
        clientMainSystem->RefPrintf( PRINT_ALL, "GLSL binds: %i  draws: gen %i light %i fog %i dlight %i\n",
                                     backEnd.pc.c_glslShaderBinds, backEnd.pc.c_genericDraws, backEnd.pc.c_lightallDraws, backEnd.pc.c_fogDraws, backEnd.pc.c_dlightDraws );
    }
    
    ::memset( &tr.pc, 0, sizeof( tr.pc ) );
    ::memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

/*
====================
idRenderSystemCmdsLocal::IssueRenderCommands
====================
*/
void idRenderSystemCmdsLocal::IssueRenderCommands( bool runPerformanceCounters )
{
    renderCommandList_t* cmdList = &backEndData->commands;
    assert( cmdList );
    
    // add an end-of-list command
    *( S32* )( cmdList->cmds + cmdList->used ) = RC_END_OF_LIST;
    
    // clear it out, in case this is a sync and not a buffer flip
    cmdList->used = 0;
    
    // at this point, the back end thread is idle, so it is ok
    // to look at its performance counters
    if ( runPerformanceCounters )
    {
        PerformanceCounters();
    }
    
    // actually start the commands going
    if ( !r_skipBackEnd->integer ) //&& !com_minimized->integer )
    {
        // let it start on the new batch
        idRenderSystemBackendLocal::ExecuteRenderCommands( cmdList->cmds );
    }
}

/*
====================
idRenderSystemCmdsLocal::IssuePendingRenderCommands

Issue any pending commands and wait for them to complete.
====================
*/
void idRenderSystemCmdsLocal::IssuePendingRenderCommands( void )
{
    if ( !tr.registered )
    {
        return;
    }
    
    IssueRenderCommands( false );
}

/*
=============
idRenderSystemCmdsLocal::AddDrawSurfCmd
=============
*/
void idRenderSystemCmdsLocal::AddDrawSurfCmd( drawSurf_t* drawSurfs, S32 numDrawSurfs )
{
    drawSurfsCommand_t*	cmd = nullptr;
    
    cmd = GetCommandBuffer( sizeof * cmd , cmd );
    if ( !cmd )
    {
        return;
    }
    cmd->commandId = RC_DRAW_SURFS;
    
    cmd->drawSurfs = drawSurfs;
    cmd->numDrawSurfs = numDrawSurfs;
    
    cmd->refdef = tr.refdef;
    cmd->viewParms = tr.viewParms;
}

/*
=============
idRenderSystemCmdsLocal::AddConvolveCubemapsCmd
=============
*/
void idRenderSystemCmdsLocal::AddConvolveCubemapCmd( cubemap_t* cubemaps, S32 cubemap, S32 cubeSide )
{
    convolveCubemapCommand_t* cmd = nullptr;
    
    cmd = GetCommandBuffer( sizeof * cmd , cmd );
    if ( !cmd )
    {
        return;
    }
    
    cmd->commandId = RC_CONVOLVECUBEMAP;
    
    cmd->cubemap = cubemap;
    cmd->cubeSide = cubeSide;
    cmd->cubemaps = cubemaps;
}

/*
=============
idRenderSystemCmdsLocal::PostProcessingCmd
=============
*/
void idRenderSystemCmdsLocal::AddPostProcessCmd( void )
{
    postProcessCommand_t* cmd = nullptr;
    
    cmd = GetCommandBuffer( sizeof * cmd, cmd );
    if ( !cmd )
    {
        return;
    }
    
    cmd->commandId = RC_POSTPROCESS;
    
    cmd->refdef = tr.refdef;
    cmd->viewParms = tr.viewParms;
}

/*
=============
idRenderSystemLocal::SetColor

Passing NULL will set the color to white
=============
*/
void idRenderSystemLocal::SetColor( const F32* rgba )
{
    setColorCommand_t*	cmd = nullptr;
    
    if ( !tr.registered )
    {
        return;
    }
    
    cmd = GetCommandBuffer( sizeof * cmd, cmd );
    if ( !cmd )
    {
        return;
    }
    cmd->commandId = RC_SET_COLOR;
    
    if ( !rgba )
    {
        static F32 colorWhite[4] = { 1, 1, 1, 1 };
        
        rgba = colorWhite;
    }
    
    cmd->color[0] = rgba[0];
    cmd->color[1] = rgba[1];
    cmd->color[2] = rgba[2];
    cmd->color[3] = rgba[3];
}

/*
=============
idRenderSystemCmdsLocal::ClipRegion
=============
*/
bool idRenderSystemCmdsLocal::ClipRegion( F32* x, F32* y, F32* w, F32* h, F32* s1, F32* t1, F32* s2, F32* t2 )
{
    F32 left, top, right, bottom, _s1, _t1, _s2, _t2, clipLeft, clipTop, clipRight, clipBottom;
    
    if ( tr.clipRegion[2] <= tr.clipRegion[0] || tr.clipRegion[3] <= tr.clipRegion[1] )
    {
        return false;
    }
    
    left = *x;
    top = *y;
    right = *x + *w;
    bottom = *y + *h;
    
    _s1 = *s1;
    _t1 = *t1;
    _s2 = *s2;
    _t2 = *t2;
    
    clipLeft = tr.clipRegion[0];
    clipTop = tr.clipRegion[1];
    clipRight = tr.clipRegion[2];
    clipBottom = tr.clipRegion[3];
    
    // Completely clipped away
    if ( right <= clipLeft || left >= clipRight || bottom <= clipTop || top >= clipBottom )
    {
        return true;
    }
    
    // Clip left edge
    if ( left < clipLeft )
    {
        F32 f = ( clipLeft - left ) / ( right - left );
        *s1 = ( f * ( _s2 - _s1 ) ) + _s1;
        *x = clipLeft;
        *w -= ( clipLeft - left );
    }
    
    // Clip right edge
    if ( right > clipRight )
    {
        F32 f = ( clipRight - right ) / ( left - right );
        *s2 = ( f * ( _s1 - _s2 ) ) + _s2;
        *w = clipRight - *x;
    }
    
    // Clip top edge
    if ( top < clipTop )
    {
        F32 f = ( clipTop - top ) / ( bottom - top );
        *t1 = ( f * ( _t2 - _t1 ) ) + _t1;
        *y = clipTop;
        *h -= ( clipTop - top );
    }
    
    // Clip bottom edge
    if ( bottom > clipBottom )
    {
        F32 f = ( clipBottom - bottom ) / ( top - bottom );
        *t2 = ( f * ( _t1 - _t2 ) ) + _t2;
        *h = clipBottom - *y;
    }
    
    return false;
}

/*
=============
idRenderSystemLocal::SetClipRegion
=============
*/
void idRenderSystemLocal::SetClipRegion( const F32* region )
{
    if ( region == NULL )
    {
        ::memset( tr.clipRegion, 0, sizeof( vec4_t ) );
    }
    else
    {
        Vector4Copy( region, tr.clipRegion );
    }
}

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
void idRenderSystemLocal::DrawStretchPic( F32 x, F32 y, F32 w, F32 h, F32 s1, F32 t1, F32 s2, F32 t2, qhandle_t hShader )
{
    stretchPicCommand_t*	cmd = nullptr;
    
    if ( !tr.registered )
    {
        return;
    }
    
    if ( idRenderSystemCmdsLocal::ClipRegion( &x, &y, &w, &h, &s1, &t1, &s2, &t2 ) )
    {
        return;
    }
    
    cmd = GetCommandBuffer( sizeof * cmd, cmd );
    if ( !cmd )
    {
        return;
    }
    
    cmd->commandId = RC_STRETCH_PIC;
    cmd->shader = idRenderSystemShaderLocal::GetShaderByHandle( hShader );
    cmd->x = x;
    cmd->y = y;
    cmd->w = w;
    cmd->h = h;
    cmd->s1 = s1;
    cmd->t1 = t1;
    cmd->s2 = s2;
    cmd->t2 = t2;
}


void idRenderSystemCmdsLocal::SetColorMode( GLboolean* rgba, stereoFrame_t stereoFrame, S32 colormode )
{
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = GL_TRUE;
    
    if ( colormode > MODE_MAX )
    {
        if ( stereoFrame == STEREO_LEFT )
        {
            stereoFrame = STEREO_RIGHT;
        }
        else if ( stereoFrame == STEREO_RIGHT )
        {
            stereoFrame = STEREO_LEFT;
        }
        
        colormode -= MODE_MAX;
    }
    
    if ( colormode == MODE_GREEN_MAGENTA )
    {
        if ( stereoFrame == STEREO_LEFT )
        {
            rgba[0] = rgba[2] = GL_FALSE;
        }
        else if ( stereoFrame == STEREO_RIGHT )
        {
            rgba[1] = GL_FALSE;
        }
    }
    else
    {
        if ( stereoFrame == STEREO_LEFT )
        {
            rgba[1] = rgba[2] = GL_FALSE;
        }
        else if ( stereoFrame == STEREO_RIGHT )
        {
            rgba[0] = GL_FALSE;
            
            if ( colormode == MODE_RED_BLUE )
            {
                rgba[1] = GL_FALSE;
            }
            else if ( colormode == MODE_RED_GREEN )
            {
                rgba[2] = GL_FALSE;
            }
        }
    }
}

/*
====================
idRenderSystemLocal::BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void idRenderSystemLocal::BeginFrame( stereoFrame_t stereoFrame )
{
    drawBufferCommand_t* cmd = nullptr;
    colorMaskCommand_t* colcmd = nullptr;
    
    if ( !tr.registered )
    {
        return;
    }
    
    glState.finishCalled = false;
    
    tr.frameCount++;
    tr.frameSceneNum = 0;
    
    //
    // do overdraw measurement
    //
    if ( r_measureOverdraw->integer )
    {
        if ( glConfig.stencilBits < 4 )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
            cvarSystem->Set( "r_measureOverdraw", "0" );
            r_measureOverdraw->modified = false;
        }
        else if ( r_shadows->integer == 2 )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "Warning: stencil shadows and overdraw measurement are mutually exclusive\n" );
            cvarSystem->Set( "r_measureOverdraw", "0" );
            r_measureOverdraw->modified = false;
        }
        else
        {
            idRenderSystemCmdsLocal::IssuePendingRenderCommands();
            qglEnable( GL_STENCIL_TEST );
            qglStencilMask( ~0U );
            qglClearStencil( 0U );
            qglStencilFunc( GL_ALWAYS, 0U, ~0U );
            qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
        }
        
        r_measureOverdraw->modified = false;
    }
    else
    {
        // this is only reached if it was on and is now off
        if ( r_measureOverdraw->modified )
        {
            idRenderSystemCmdsLocal::IssuePendingRenderCommands();
            qglDisable( GL_STENCIL_TEST );
        }
        
        r_measureOverdraw->modified = false;
    }
    
    //
    // texturemode stuff
    //
    if ( r_textureMode->modified )
    {
        idRenderSystemCmdsLocal::IssuePendingRenderCommands();
        idRenderSystemImageLocal::TextureMode( r_textureMode->string );
        r_textureMode->modified = false;
    }
    
    //
    // gamma stuff
    //
    if ( r_gamma->modified )
    {
        r_gamma->modified = false;
        
        idRenderSystemCmdsLocal::IssuePendingRenderCommands();
        idRenderSystemImageLocal::SetColorMappings();
    }
    
    // check for errors
    if ( !r_ignoreGLErrors->integer )
    {
        S32	err;
        
        idRenderSystemCmdsLocal::IssuePendingRenderCommands();
        if ( ( err = qglGetError() ) != GL_NO_ERROR )
        {
            Com_Error( ERR_FATAL, "idRenderSystemLocal::BeginFrame() - glGetError() failed (0x%x)!", err );
        }
    }
    
    if ( glConfig.stereoEnabled )
    {
        if ( !( cmd = GetCommandBuffer( sizeof * cmd, cmd ) ) )
        {
            return;
        }
        
        cmd->commandId = RC_DRAW_BUFFER;
        
        if ( stereoFrame == STEREO_LEFT )
        {
            cmd->buffer = ( S32 )GL_BACK_LEFT;
        }
        else if ( stereoFrame == STEREO_RIGHT )
        {
            cmd->buffer = ( S32 )GL_BACK_RIGHT;
        }
        else
        {
            Com_Error( ERR_FATAL, "idRenderSystemLocal::BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
        }
    }
    else
    {
        if ( r_anaglyphMode->integer )
        {
            if ( r_anaglyphMode->modified )
            {
                // clear both, front and backbuffer.
                qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
                backEnd.colorMask[0] = GL_FALSE;
                backEnd.colorMask[1] = GL_FALSE;
                backEnd.colorMask[2] = GL_FALSE;
                backEnd.colorMask[3] = GL_FALSE;
                
                if ( glRefConfig.framebufferObject )
                {
                
                    if ( tr.renderFbo )
                    {
                        idRenderSystemFBOLocal::Bind( tr.renderFbo );
                        qglClear( GL_COLOR_BUFFER_BIT );
                    }
                    
                    idRenderSystemFBOLocal::Bind( NULL );
                }
                
                qglDrawBuffer( GL_FRONT );
                qglClear( GL_COLOR_BUFFER_BIT );
                qglDrawBuffer( GL_BACK );
                qglClear( GL_COLOR_BUFFER_BIT );
                
                r_anaglyphMode->modified = false;
            }
            
            if ( stereoFrame == STEREO_LEFT )
            {
                if ( !( cmd = GetCommandBuffer( sizeof * cmd, cmd ) ) )
                {
                    return;
                }
                
                if ( !( colcmd = GetCommandBuffer( sizeof * colcmd , colcmd ) ) )
                {
                    return;
                }
            }
            else if ( stereoFrame == STEREO_RIGHT )
            {
                clearDepthCommand_t* cldcmd = nullptr;
                
                if ( !( cldcmd = GetCommandBuffer( sizeof * cldcmd , cldcmd ) ) )
                {
                    return;
                }
                
                cldcmd->commandId = RC_CLEARDEPTH;
                
                if ( !( colcmd = GetCommandBuffer( sizeof * colcmd , colcmd ) ) )
                {
                    return;
                }
            }
            else
            {
                Com_Error( ERR_FATAL, "idRenderSystemLocal::BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
            }
            
            idRenderSystemCmdsLocal::SetColorMode( colcmd->rgba, stereoFrame, r_anaglyphMode->integer );
            colcmd->commandId = RC_COLORMASK;
        }
        else
        {
            if ( stereoFrame != STEREO_CENTER )
            {
                Com_Error( ERR_FATAL, "idRenderSystemLocal::BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );
            }
            
            if ( !( cmd = GetCommandBuffer( sizeof * cmd , cmd ) ) )
            {
                return;
            }
        }
        
        if ( cmd )
        {
            cmd->commandId = RC_DRAW_BUFFER;
            
            if ( r_anaglyphMode->modified )
            {
                qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
                backEnd.colorMask[0] = 0;
                backEnd.colorMask[1] = 0;
                backEnd.colorMask[2] = 0;
                backEnd.colorMask[3] = 0;
                r_anaglyphMode->modified = false;
            }
            
            if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) )
            {
                cmd->buffer = ( S32 )GL_FRONT;
            }
            else
            {
                cmd->buffer = ( S32 )GL_BACK;
            }
        }
    }
    
    tr.refdef.stereoFrame = stereoFrame;
}

/*
=============
idRenderSystemLocal::EndFrame

Returns the number of msec spent in the back end
=============
*/
void idRenderSystemLocal::EndFrame( S32* frontEndMsec, S32* backEndMsec )
{
    swapBuffersCommand_t*	cmd = nullptr;
    
    if ( !tr.registered )
    {
        return;
    }
    cmd = GetCommandBuffer( sizeof * cmd , cmd );
    if ( !cmd )
    {
        return;
    }
    cmd->commandId = RC_SWAP_BUFFERS;
    
    idRenderSystemCmdsLocal::IssueRenderCommands( true );
    
    idRenderSystemSceneLocal::InitNextFrame();
    
    if ( frontEndMsec )
    {
        *frontEndMsec = tr.frontEndMsec;
    }
    
    tr.frontEndMsec = 0;
    
    if ( backEndMsec )
    {
        *backEndMsec = backEnd.pc.msec;
    }
    
    backEnd.pc.msec = 0;
}

/*
=============
idRenderSystemLocal::TakeVideoFrame
=============
*/
void idRenderSystemLocal::TakeVideoFrame( S32 width, S32 height, U8* captureBuffer, U8* encodeBuffer, bool motionJpeg )
{
    videoFrameCommand_t*	cmd = nullptr;
    
    if ( !tr.registered )
    {
        return;
    }
    
    cmd = GetCommandBuffer( sizeof * cmd , cmd );
    if ( !cmd )
    {
        return;
    }
    
    cmd->commandId = RC_VIDEOFRAME;
    
    cmd->width = width;
    cmd->height = height;
    cmd->captureBuffer = captureBuffer;
    cmd->encodeBuffer = encodeBuffer;
    cmd->motionJpeg = motionJpeg;
}


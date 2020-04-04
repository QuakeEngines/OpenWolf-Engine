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
// File name:   r_backend.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

backEndData_t*	backEndData;
backEndState_t	backEnd;
idRenderSystemBackendLocal renderSystemBackendLocal;

/*
===============
idRenderSystemBackendLocal::idRenderSystemBackendLocal
===============
*/
idRenderSystemBackendLocal::idRenderSystemBackendLocal( void )
{
}

/*
===============
idRenderSystemBackendLocal::~idRenderSystemBackendLocal
===============
*/
idRenderSystemBackendLocal::~idRenderSystemBackendLocal( void )
{
}

static F32 s_flipMatrix[16] =
{
    // convert from our coordinate system (looking down X)
    // to OpenGL's coordinate system (looking down -Z)
    0, 0, -1, 0,
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1
};

/*
===============
idRenderSystemBackendLocal::BindToTMU
===============
*/
void idRenderSystemBackendLocal::BindToTMU( image_t* image, S32 tmu )
{
    U32 texture = ( tmu == TB_COLORMAP ) ? tr.defaultImage->texnum : 0, target = GL_TEXTURE_2D;
    
    if ( image )
    {
        if ( image->flags & IMGFLAG_CUBEMAP )
        {
            target = GL_TEXTURE_CUBE_MAP;
        }
        
        image->frameUsed = tr.frameCount;
        texture = image->texnum;
    }
    else
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemBackendLocal::BindToTMU: NULL image\n" );
    }
    
    idRenderSystemDSALocal::BindMultiTexture( GL_TEXTURE0 + tmu, target, texture );
}

/*
===============
idRenderSystemBackendLocal::Cull
===============
*/
void idRenderSystemBackendLocal::Cull( S32 cullType )
{
    if ( glState.faceCulling == cullType )
    {
        return;
    }
    
    if ( cullType == CT_TWO_SIDED )
    {
        qglDisable( GL_CULL_FACE );
    }
    else
    {
        S32 cullFront = ( cullType == CT_FRONT_SIDED );
        
        if ( glState.faceCulling == CT_TWO_SIDED )
        {
            qglEnable( GL_CULL_FACE );
        }
        
        if ( glState.faceCullFront != cullFront )
        {
            qglCullFace( cullFront ? GL_FRONT : GL_BACK );
        }
        
        glState.faceCullFront = cullFront;
    }
    
    glState.faceCulling = cullType;
}

/*
===============
idRenderSystemBackendLocal::State

This routine is responsible for setting the most commonly changed state in Q3.
===============
*/
void idRenderSystemBackendLocal::State( U64 stateBits )
{
    U64 diff = stateBits ^ glState.glStateBits;
    
    if ( !diff )
    {
        return;
    }
    
    // check depthFunc bits
    if ( diff & GLS_DEPTHFUNC_BITS )
    {
        if ( stateBits & GLS_DEPTHFUNC_EQUAL )
        {
            qglDepthFunc( GL_EQUAL );
        }
        else if ( stateBits & GLS_DEPTHFUNC_GREATER )
        {
            qglDepthFunc( GL_GREATER );
        }
        else
        {
            qglDepthFunc( GL_LEQUAL );
        }
    }
    
    //
    // check blend bits
    //
    if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
    {
        U32 oldState = glState.glStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
        U32 newState = stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
        U32 storedState = glState.storedGlState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
        
        if ( oldState == 0 )
        {
            qglEnable( GL_BLEND );
        }
        else if ( newState == 0 )
        {
            qglDisable( GL_BLEND );
        }
        
        if ( newState != 0 && storedState != newState )
        {
            U32 srcFactor = GL_ONE, dstFactor = GL_ONE;
            
            glState.storedGlState &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
            glState.storedGlState |= newState;
            
            switch ( stateBits & GLS_SRCBLEND_BITS )
            {
                case GLS_SRCBLEND_ZERO:
                    srcFactor = GL_ZERO;
                    break;
                case GLS_SRCBLEND_ONE:
                    srcFactor = GL_ONE;
                    break;
                case GLS_SRCBLEND_DST_COLOR:
                    srcFactor = GL_DST_COLOR;
                    break;
                case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
                    srcFactor = GL_ONE_MINUS_DST_COLOR;
                    break;
                case GLS_SRCBLEND_SRC_ALPHA:
                    srcFactor = GL_SRC_ALPHA;
                    break;
                case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
                    srcFactor = GL_ONE_MINUS_SRC_ALPHA;
                    break;
                case GLS_SRCBLEND_DST_ALPHA:
                    srcFactor = GL_DST_ALPHA;
                    break;
                case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
                    srcFactor = GL_ONE_MINUS_DST_ALPHA;
                    break;
                case GLS_SRCBLEND_ALPHA_SATURATE:
                    srcFactor = GL_SRC_ALPHA_SATURATE;
                    break;
                default:
                    srcFactor = GL_ONE;
                    Com_Error( ERR_DROP, "idRenderSystemBackendLocal::State: invalid src blend state bits" );
                    break;
            }
            
            switch ( stateBits & GLS_DSTBLEND_BITS )
            {
                case GLS_DSTBLEND_ZERO:
                    dstFactor = GL_ZERO;
                    break;
                case GLS_DSTBLEND_ONE:
                    dstFactor = GL_ONE;
                    break;
                case GLS_DSTBLEND_SRC_COLOR:
                    dstFactor = GL_SRC_COLOR;
                    break;
                case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
                    dstFactor = GL_ONE_MINUS_SRC_COLOR;
                    break;
                case GLS_DSTBLEND_SRC_ALPHA:
                    dstFactor = GL_SRC_ALPHA;
                    break;
                case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
                    dstFactor = GL_ONE_MINUS_SRC_ALPHA;
                    break;
                case GLS_DSTBLEND_DST_ALPHA:
                    dstFactor = GL_DST_ALPHA;
                    break;
                case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
                    dstFactor = GL_ONE_MINUS_DST_ALPHA;
                    break;
                default:
                    dstFactor = GL_ONE;
                    Com_Error( ERR_DROP, "idRenderSystemBackendLocal::State: invalid dst blend state bits" );
                    break;
            }
            
            qglBlendFunc( srcFactor, dstFactor );
        }
    }
    
    // check depthmask
    if ( diff & GLS_DEPTHMASK_TRUE )
    {
        if ( stateBits & GLS_DEPTHMASK_TRUE )
        {
            qglDepthMask( GL_TRUE );
        }
        else
        {
            qglDepthMask( GL_FALSE );
        }
    }
    
    // fill/line mode
    if ( diff & GLS_POLYMODE_LINE )
    {
        if ( stateBits & GLS_POLYMODE_LINE )
        {
            qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        }
        else
        {
            qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }
    }
    
    // depthtest
    if ( diff & GLS_DEPTHTEST_DISABLE )
    {
        if ( stateBits & GLS_DEPTHTEST_DISABLE )
        {
            qglDisable( GL_DEPTH_TEST );
        }
        else
        {
            qglEnable( GL_DEPTH_TEST );
        }
    }
    
    //
    // alpha test
    //
    if ( diff & GLS_ATEST_BITS )
    {
        switch ( stateBits & GLS_ATEST_BITS )
        {
            case 0:
                qglDisable( GL_ALPHA_TEST );
                break;
            case GLS_ATEST_GT_0:
                qglEnable( GL_ALPHA_TEST );
                qglAlphaFunc( GL_GREATER, 0.0f );
                break;
            case GLS_ATEST_LT_80:
                qglEnable( GL_ALPHA_TEST );
                qglAlphaFunc( GL_LESS, 0.5f );
                break;
            case GLS_ATEST_GE_80:
                qglEnable( GL_ALPHA_TEST );
                qglAlphaFunc( GL_GEQUAL, 0.5f );
                break;
            default:
                assert( 0 );
                break;
        }
    }
    
    glState.glStateBits = stateBits;
}

/*
===============
idRenderSystemBackendLocal::CulSetProjectionMatrix
===============
*/
void idRenderSystemBackendLocal::SetProjectionMatrix( mat4_t matrix )
{
    idRenderSystemMathsLocal::Mat4Copy( matrix, glState.projection );
    idRenderSystemMathsLocal::Mat4Multiply( glState.projection, glState.modelview, glState.modelviewProjection );
    idRenderSystemMathsLocal::Mat4SimpleInverse( glState.projection, glState.invProjection );
    idRenderSystemMathsLocal::Mat4SimpleInverse( glState.modelviewProjection, glState.invEyeProjection );
}

/*
===============
idRenderSystemBackendLocal::SetModelviewMatrix
===============
*/
void idRenderSystemBackendLocal::SetModelviewMatrix( mat4_t matrix )
{
    idRenderSystemMathsLocal::Mat4Copy( matrix, glState.modelview );
    idRenderSystemMathsLocal::Mat4Multiply( glState.projection, glState.modelview, glState.modelviewProjection );
    idRenderSystemMathsLocal::Mat4SimpleInverse( glState.projection, glState.invProjection );
    idRenderSystemMathsLocal::Mat4SimpleInverse( glState.modelviewProjection, glState.invEyeProjection );
}

/*
================
idRenderSystemBackendLocal::Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
void idRenderSystemBackendLocal::Hyperspace( void )
{
    F32	c;
    
    if ( !backEnd.isHyperspace )
    {
        // do initialization shit
    }
    
    c = ( backEnd.refdef.time & 255 ) / 255.0f;
    qglClearColor( c, c, c, 1 );
    qglClear( GL_COLOR_BUFFER_BIT );
    qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    
    backEnd.isHyperspace = true;
}

/*
===============
idRenderSystemBackendLocal::SetViewportAndScissor
===============
*/
void idRenderSystemBackendLocal::SetViewportAndScissor( void )
{
    SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
    
    // set the window clipping
    qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
    qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
idRenderSystemBackendLocal::BeginDrawingView

to actually render the visible surfaces for this view
=================
*/
void idRenderSystemBackendLocal::BeginDrawingView( void )
{
    S32 clearBits = 0;
    
    // sync with gl if needed
    if ( r_finish->integer == 1 && !glState.finishCalled )
    {
        qglFinish();
        glState.finishCalled = true;
    }
    
    if ( r_finish->integer == 0 )
    {
        glState.finishCalled = true;
    }
    
    // we will need to change the projection matrix before drawing
    // 2D images again
    backEnd.projection2D = false;
    
    if ( glRefConfig.framebufferObject )
    {
        // FIXME: HUGE HACK: render to the screen fbo if we've already postprocessed the frame and aren't drawing more world
        // drawing more world check is in case of double renders, such as skyportals
        if ( backEnd.viewParms.targetFbo == NULL )
        {
            if ( !tr.renderFbo || ( backEnd.framePostProcessed && ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) ) )
            {
                idRenderSystemFBOLocal::Bind( NULL );
            }
            else
            {
                idRenderSystemFBOLocal::Bind( tr.renderFbo );
            }
        }
        else
        {
            idRenderSystemFBOLocal::Bind( backEnd.viewParms.targetFbo );
            
            // FIXME: hack for cubemap testing
            if ( tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo )
            {
                cubemap_t* cubemap = &tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex];
                
                qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer,
                                         cubemap->image->texnum, 0 );
            }
            else if ( tr.shadowCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.shadowCubeFbo )
            {
                cubemap_t* cubemap = &backEnd.viewParms.cubemapSelection[backEnd.viewParms.targetFboCubemapIndex];
                qglFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubemap->image->texnum, 0 );;
            }
        }
    }
    
    // set the modelview matrix for the viewer
    SetViewportAndScissor();
    
    // ensures that depth writes are enabled for the depth clear
    State( GLS_DEFAULT );
    
    // clear relevant buffers
    clearBits = GL_DEPTH_BUFFER_BIT;
    
    if ( r_measureOverdraw->integer || r_shadows->integer == 2 )
    {
        clearBits |= GL_STENCIL_BUFFER_BIT;
    }
    if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
    {
        // FIXME: only if sky shaders have been used
        clearBits |= GL_COLOR_BUFFER_BIT;
    }
    
    // clear to black for cube maps
    if ( tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo )
    {
        clearBits |= GL_COLOR_BUFFER_BIT;
        qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    }
    
    qglClear( clearBits );
    
    if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
    {
        Hyperspace();
        return;
    }
    else
    {
        backEnd.isHyperspace = false;
    }
    
    // we will only draw a sun if there was sky rendered in this view
    backEnd.skyRenderedThisView = false;
    
    // clip to the plane of the portal
    if ( backEnd.viewParms.isPortal )
    {
#if 1
        F32	plane[4];
        F64	plane2[4];
        
        plane[0] = backEnd.viewParms.portalPlane.normal[0];
        plane[1] = backEnd.viewParms.portalPlane.normal[1];
        plane[2] = backEnd.viewParms.portalPlane.normal[2];
        plane[3] = backEnd.viewParms.portalPlane.dist;
        
        plane2[0] = DotProduct( backEnd.viewParms.orientation.axis[0], plane );
        plane2[1] = DotProduct( backEnd.viewParms.orientation.axis[1], plane );
        plane2[2] = DotProduct( backEnd.viewParms.orientation.axis[2], plane );
        plane2[3] = DotProduct( plane, backEnd.viewParms.orientation.origin ) - plane[3];
        
#endif
        SetModelviewMatrix( s_flipMatrix );
    }
}

/*
==================
idRenderSystemBackendLocal::RenderDrawSurfList
==================
*/
void idRenderSystemBackendLocal::RenderDrawSurfList( drawSurf_t* drawSurfs, S32 numDrawSurfs )
{
    S32 i, fogNum, oldFogNum, entityNum, oldEntityNum, dlighted, oldDlighted, pshadowed, oldPshadowed, cubemapIndex, oldCubemapIndex;
    F32 depth[2];
    drawSurf_t* drawSurf;
    size_t oldSort;
    FBO_t* fbo = NULL;
    shader_t* shader, *oldShader;
    bool inQuery = false, depthRange, oldDepthRange, isCrosshair, wasCrosshair;
    
    // save original time for entity shader offsets
    F64 originalTime = backEnd.refdef.floatTime;
    
    fbo = glState.currentFBO;
    
    // draw everything
    oldEntityNum = -1;
    backEnd.currentEntity = &tr.worldEntity;
    oldShader = NULL;
    oldFogNum = -1;
    oldDepthRange = false;
    wasCrosshair = false;
    oldDlighted = false;
    oldPshadowed = false;
    oldCubemapIndex = -1;
    oldSort = -1;
    
    depth[0] = 0.f;
    depth[1] = 1.f;
    
    backEnd.pc.c_surfaces += numDrawSurfs;
    
    for ( i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++ )
    {
        if ( drawSurf->sort == oldSort && drawSurf->cubemapIndex == oldCubemapIndex )
        {
            if ( backEnd.depthFill && shader && shader->sort != SS_OPAQUE )
            {
                continue;
            }
            
            // fast path, same as previous sort
            rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
            continue;
        }
        
        oldSort = drawSurf->sort;
        idRenderSystemMainLocal::DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed );
        cubemapIndex = drawSurf->cubemapIndex;
        
        // change the tess parameters if needed
        // a "entityMergable" shader is a shader that can have surfaces from seperate
        // entities merged into a single batch, like smoke and blood puff sprites
        if ( shader != NULL && ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted
                                 || pshadowed != oldPshadowed || cubemapIndex != oldCubemapIndex || ( entityNum != oldEntityNum && !shader->entityMergable ) ) )
        {
            if ( oldShader != NULL )
            {
                idRenderSystemShadeLocal::EndSurface();
            }
            
            idRenderSystemShadeLocal::BeginSurface( shader, fogNum, cubemapIndex );
            
            backEnd.pc.c_surfBatches++;
            oldShader = shader;
            oldFogNum = fogNum;
            oldDlighted = dlighted;
            oldPshadowed = pshadowed;
            oldCubemapIndex = cubemapIndex;
        }
        
        if ( backEnd.depthFill && shader && shader->sort != SS_OPAQUE )
        {
            continue;
        }
        
        //
        // change the modelview matrix if needed
        //
        if ( entityNum != oldEntityNum )
        {
            bool sunflare = false;
            depthRange = isCrosshair = false;
            
            if ( entityNum != REFENTITYNUM_WORLD )
            {
                backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
                
                // FIXME: e.shaderTime must be passed as S32 to avoid fp-precision loss issues
                backEnd.refdef.floatTime = originalTime - ( F64 )backEnd.currentEntity->e.shaderTime;
                
                // we have to reset the shaderTime as well otherwise image animations start
                // from the wrong frame
                tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
                
                // set up the transformation matrix
                idRenderSystemMainLocal::RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );
                
                // set up the dynamic lighting if needed
                if ( backEnd.currentEntity->needDlights )
                {
                    idRenderSystemLightLocal::TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
                }
                
                if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
                {
                    // hack the depth range to prevent view model from poking into walls
                    depthRange = true;
                    
                    if ( backEnd.currentEntity->e.renderfx & RF_CROSSHAIR )
                    {
                        isCrosshair = true;
                    }
                }
            }
            else
            {
                backEnd.currentEntity = &tr.worldEntity;
                backEnd.refdef.floatTime = originalTime;
                backEnd.orientation = backEnd.viewParms.world;
                
                // we have to reset the shaderTime as well otherwise image animations on
                // the world (like water) continue with the wrong frame
                tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
                idRenderSystemLightLocal::TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
            }
            
            SetModelviewMatrix( backEnd.orientation.modelMatrix );
            
            // change depthrange. Also change projection matrix so first person weapon does not look like coming
            // out of the screen.
            if ( oldDepthRange != depthRange || wasCrosshair != isCrosshair )
            {
                if ( depthRange )
                {
                    if ( backEnd.viewParms.stereoFrame != STEREO_CENTER )
                    {
                        if ( isCrosshair )
                        {
                            if ( oldDepthRange )
                            {
                                // was not a crosshair but now is, change back proj matrix
                                SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
                            }
                        }
                        else
                        {
                            viewParms_t temp = backEnd.viewParms;
                            
                            idRenderSystemMainLocal::SetupProjection( &temp, r_znear->value, 0, false );
                            
                            SetProjectionMatrix( temp.projectionMatrix );
                        }
                    }
                    
                    if ( !oldDepthRange )
                    {
                        depth[0] = 0;
                        depth[1] = 0.3f;
                        qglDepthRange( depth[0], depth[1] );
                    }
                }
                else
                {
                    if ( !wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER )
                    {
                        SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
                    }
                    
                    if ( !sunflare )
                    {
                        qglDepthRange( 0, 1 );
                    }
                    
                    depth[0] = 0;
                    depth[1] = 1;
                }
                
                oldDepthRange = depthRange;
                wasCrosshair = isCrosshair;
            }
            
            oldEntityNum = entityNum;
        }
        
        // add the triangles for this surface
        rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
    }
    
    backEnd.refdef.floatTime = originalTime;
    
    // draw the contents of the last shader batch
    if ( oldShader != nullptr )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    if ( inQuery )
    {
        qglEndQuery( GL_SAMPLES_PASSED );
    }
    
    if ( glRefConfig.framebufferObject )
    {
        idRenderSystemFBOLocal::Bind( fbo );
    }
    
    // go back to the world modelview matrix
    SetModelviewMatrix( backEnd.viewParms.world.modelMatrix );
    
    qglDepthRange( 0, 1 );
}

/*
============================================================================
RENDER BACK END FUNCTIONS
============================================================================
*/

/*
================
idRenderSystemBackendLocal::SetGL2D
================
*/
void idRenderSystemBackendLocal::SetGL2D( void )
{
    S32 width, height;
    mat4_t matrix;
    
    if ( backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO )
    {
        return;
    }
    
    backEnd.projection2D = true;
    backEnd.last2DFBO = glState.currentFBO;
    
    if ( glState.currentFBO )
    {
        width = glState.currentFBO->width;
        height = glState.currentFBO->height;
    }
    else
    {
        width = glConfig.vidWidth;
        height = glConfig.vidHeight;
    }
    
    // set 2D virtual screen size
    qglViewport( 0, 0, width, height );
    qglScissor( 0, 0, width, height );
    
    idRenderSystemMathsLocal::Mat4Ortho( 0, ( F32 )width, ( F32 )height, 0, 0, 1, matrix );
    SetProjectionMatrix( matrix );
    idRenderSystemMathsLocal::Mat4Identity( matrix );
    SetModelviewMatrix( matrix );
    
    State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    
    //Cull( CT_TWO_SIDED );
    
    qglDisable( GL_CULL_FACE );
    qglDisable( GL_CLIP_PLANE0 );
    
    // set time for 2D shaders
    backEnd.refdef.time = clientMainSystem->ScaledMilliseconds();
    backEnd.refdef.floatTime = backEnd.refdef.time * 0.001;
}

/*
=============
idRenderSystemLocal::StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void idRenderSystemLocal::DrawStretchRaw( S32 x, S32 y, S32 w, S32 h, S32 cols, S32 rows, const U8* data, S32 client, bool dirty )
{
    S32	i, j, start = 0, end;
    vec2_t texCoords[4];
    vec4_t quadVerts[4];
    
    if ( !tr.registered )
    {
        return;
    }
    
    idRenderSystemCmdsLocal::IssuePendingRenderCommands();
    
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    // we definately want to sync every frame for the cinematics
    qglFinish();
    
    if ( r_speeds->integer )
    {
        start = clientMainSystem->ScaledMilliseconds();
    }
    
    // make sure rows and cols are powers of 2
    for ( i = 0 ; ( 1 << i ) < cols ; i++ )
    {
    }
    
    for ( j = 0 ; ( 1 << j ) < rows ; j++ )
    {
    }
    
    if ( ( 1 << i ) != cols || ( 1 << j ) != rows )
    {
        Com_Error( ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows );
    }
    
    renderSystemLocal.UploadCinematic( w, h, cols, rows, data, client, dirty );
    
    renderSystemBackendLocal.BindToTMU( tr.scratchImage[client], TB_COLORMAP );
    
    if ( r_speeds->integer )
    {
        end = clientMainSystem->ScaledMilliseconds();
        clientMainSystem->RefPrintf( PRINT_ALL, "glTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
    }
    
    // FIXME: HUGE hack
    if ( glRefConfig.framebufferObject )
    {
        idRenderSystemFBOLocal::Bind( backEnd.framePostProcessed ? NULL : tr.renderFbo );
    }
    
    renderSystemBackendLocal.SetGL2D();
    
    VectorSet4( quadVerts[0], ( F32 )x, ( F32 )y, 0.0f, 1.0f );
    VectorSet4( quadVerts[1], ( F32 )( x + w ), ( F32 )y, 0.0f, 1.0f );
    VectorSet4( quadVerts[2], ( F32 )( x + w ), ( F32 )( y + h ), 0.0f, 1.0f );
    VectorSet4( quadVerts[3], ( F32 )x, ( F32 )( y + h ), 0.0f, 1.0f );
    
    VectorSet2( texCoords[0], 0.5f / cols, 0.5f / rows );
    VectorSet2( texCoords[1], ( cols - 0.5f ) / cols, 0.5f / rows );
    VectorSet2( texCoords[2], ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows );
    VectorSet2( texCoords[3], 0.5f / cols, ( rows - 0.5f ) / rows );
    
    idRenderSystemGLSLLocal::BindProgram( &tr.textureColorShader );
    
    idRenderSystemGLSLLocal::SetUniformMat4( &tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
    idRenderSystemGLSLLocal::SetUniformVec4( &tr.textureColorShader, UNIFORM_COLOR, colorWhite );
    
    idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
}

void idRenderSystemLocal::UploadCinematic( S32 w, S32 h, S32 cols, S32 rows, const U8* data, S32 client, bool dirty )
{
    U32 texture;
    
    if ( !tr.scratchImage[client] )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemLocal::UploadCinematic: scratch images not initialized\n" );
        return;
    }
    
    texture = tr.scratchImage[client]->texnum;
    
    // if the scratchImage isn't in the format we want, specify it as a new texture
    if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height )
    {
        tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
        tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
        qglTextureImage2DEXT( texture, GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        qglTextureParameterfEXT( texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        qglTextureParameterfEXT( texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        qglTextureParameterfEXT( texture, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        qglTextureParameterfEXT( texture, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    }
    else
    {
        if ( dirty )
        {
            // otherwise, just subimage upload it so that drivers can tell we are going to be changing
            // it and don't try and do a texture compression
            qglTextureSubImage2DEXT( texture, GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
        }
    }
}

/*
=============
idRenderSystemBackendLocal::SetColor
=============
*/
const void* idRenderSystemBackendLocal::SetColor( const void* data )
{
    const setColorCommand_t* cmd = ( const setColorCommand_t* )data;
    
    backEnd.color2D[0] = ( U8 )( cmd->color[0] * 255 );
    backEnd.color2D[1] = ( U8 )( cmd->color[1] * 255 );
    backEnd.color2D[2] = ( U8 )( cmd->color[2] * 255 );
    backEnd.color2D[3] = ( U8 )( cmd->color[3] * 255 );
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::StretchPic
=============
*/
const void* idRenderSystemBackendLocal::StretchPic( const void* data )
{
    S32	numVerts, numIndexes;
    const stretchPicCommand_t* cmd = ( const stretchPicCommand_t* )data;
    shader_t* shader;
    
    // FIXME: HUGE hack
    if ( glRefConfig.framebufferObject )
    {
        idRenderSystemFBOLocal::Bind( backEnd.framePostProcessed ? NULL : tr.renderFbo );
    }
    
    SetGL2D();
    
    shader = cmd->shader;
    if ( shader != tess.shader )
    {
        if ( tess.numIndexes )
        {
            idRenderSystemShadeLocal::EndSurface();
        }
        backEnd.currentEntity = &backEnd.entity2D;
        idRenderSystemShadeLocal::BeginSurface( shader, 0, 0 );
    }
    
    idRenderSystemSurfaceLocal::CheckOverflow( 4, 6 );
    numVerts = tess.numVertexes;
    numIndexes = tess.numIndexes;
    
    tess.numVertexes += 4;
    tess.numIndexes += 6;
    
    tess.indexes[ numIndexes ] = numVerts + 3;
    tess.indexes[ numIndexes + 1 ] = numVerts + 0;
    tess.indexes[ numIndexes + 2 ] = numVerts + 2;
    tess.indexes[ numIndexes + 3 ] = numVerts + 2;
    tess.indexes[ numIndexes + 4 ] = numVerts + 0;
    tess.indexes[ numIndexes + 5 ] = numVerts + 1;
    
    {
        U16 color[4];
        
        VectorScale4( backEnd.color2D, 257, color );
        
        VectorCopy4( color, tess.color[ numVerts ] );
        VectorCopy4( color, tess.color[ numVerts + 1] );
        VectorCopy4( color, tess.color[ numVerts + 2] );
        VectorCopy4( color, tess.color[ numVerts + 3 ] );
    }
    
    tess.xyz[ numVerts ][0] = cmd->x;
    tess.xyz[ numVerts ][1] = cmd->y;
    tess.xyz[ numVerts ][2] = 0;
    
    tess.texCoords[ numVerts ][0] = cmd->s1;
    tess.texCoords[ numVerts ][1] = cmd->t1;
    
    tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
    tess.xyz[ numVerts + 1 ][1] = cmd->y;
    tess.xyz[ numVerts + 1 ][2] = 0;
    
    tess.texCoords[ numVerts + 1 ][0] = cmd->s2;
    tess.texCoords[ numVerts + 1 ][1] = cmd->t1;
    
    tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
    tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
    tess.xyz[ numVerts + 2 ][2] = 0;
    
    tess.texCoords[ numVerts + 2 ][0] = cmd->s2;
    tess.texCoords[ numVerts + 2 ][1] = cmd->t2;
    
    tess.xyz[ numVerts + 3 ][0] = cmd->x;
    tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
    tess.xyz[ numVerts + 3 ][2] = 0;
    
    tess.texCoords[ numVerts + 3 ][0] = cmd->s1;
    tess.texCoords[ numVerts + 3 ][1] = cmd->t2;
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::PrefilterEnvMap
=============
*/
const void* idRenderSystemBackendLocal::PrefilterEnvMap( const void* data )
{
    const convolveCubemapCommand_t* cmd = ( const convolveCubemapCommand_t* )data;
    
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    SetGL2D();
    
    cubemap_t* cubemap = &tr.cubemaps[cmd->cubemap];
    
    if ( !cubemap )
    {
        return ( const void* )( cmd + 1 );
    }
    
    S32 cubeMipSize = r_cubemapSize->integer;
    S32 numMips = 0;
    
    S32 width = cubemap->image->width;
    S32 height = cubemap->image->height;
    
    vec4_t quadVerts[4];
    vec2_t texCoords[4];
    
    while ( cubeMipSize )
    {
        cubeMipSize >>= 1;
        numMips++;
    }
    numMips = MAX( 1, numMips - 4 );
    
    idRenderSystemFBOLocal::Bind( tr.preFilterEnvMapFbo );
    BindToTMU( cubemap->image, TB_CUBEMAP );
    
    idRenderSystemGLSLLocal::BindProgram( &tr.prefilterEnvMapShader );
    
    for ( S32 level = 1; level <= numMips; level++ )
    {
        width = ( S32 )( width / 2.0f );
        height = ( S32 )( height / 2.0f );
        
        qglViewport( 0, 0, width, height );
        qglScissor( 0, 0, width, height );
        
        vec4_t viewInfo;
        VectorSet4( viewInfo, ( F32 )cmd->cubeSide, ( F32 )level, ( F32 )numMips, 0.0f );
        idRenderSystemGLSLLocal::SetUniformVec4( &tr.prefilterEnvMapShader, UNIFORM_VIEWINFO, viewInfo );
        idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
        qglCopyTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, level, 0, 0, 0, 0, width, height );
    }
    
    qglActiveTexture( GL_TEXTURE0 );
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::DrawSurfs
=============
*/
const void* idRenderSystemBackendLocal::DrawSurfs( const void* data )
{
    const drawSurfsCommand_t* cmd = ( const drawSurfsCommand_t* )data;
    bool isShadowView;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    backEnd.refdef = cmd->refdef;
    backEnd.viewParms = cmd->viewParms;
    
    isShadowView = !!( backEnd.viewParms.flags & VPF_DEPTHSHADOW );
    
    // clear the z buffer, set the modelview, etc
    BeginDrawingView();
    
    if ( glRefConfig.framebufferObject && ( backEnd.viewParms.flags & VPF_DEPTHCLAMP ) && glRefConfig.depthClamp )
    {
        qglEnable( GL_DEPTH_CLAMP );
    }
    
    if ( glRefConfig.framebufferObject && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) && ( r_depthPrepass->integer || isShadowView ) )
    {
        FBO_t* oldFbo = glState.currentFBO;
        vec4_t viewInfo;
        
        VectorSet4( viewInfo, backEnd.viewParms.zFar / r_znear->value, backEnd.viewParms.zFar, 0.0, 0.0 );
        
        backEnd.depthFill = true;
        qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
        RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
        qglColorMask( !backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3] );
        backEnd.depthFill = false;
        
        if ( !isShadowView )
        {
            if ( backEnd.viewParms.targetFbo == tr.renderCubeFbo )
            {
                ivec4_t frameBox;
                
                frameBox[0] = backEnd.viewParms.viewportX;
                frameBox[1] = backEnd.viewParms.viewportY;
                frameBox[2] = backEnd.viewParms.viewportWidth;
                frameBox[3] = backEnd.viewParms.viewportHeight;
                idRenderSystemFBOLocal::FastBlit( tr.renderCubeFbo, frameBox, NULL, frameBox, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
            }
            else if ( tr.renderFbo == NULL && tr.renderDepthImage )
            {
                // If we're rendering directly to the screen, copy the depth to a texture
                // This is incredibly slow on Intel Graphics, so just skip it on there
                if ( !glRefConfig.intelGraphics )
                {
                    qglCopyTextureSubImage2DEXT( tr.renderDepthImage->texnum, GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );
                }
            }
            
            if ( tr.hdrDepthFbo )
            {
                // need the depth in a texture we can do GL_LINEAR sampling on, so copy it to an HDR image
                vec4_t srcTexCoords;
                
                VectorSet4( srcTexCoords, 0.0f, 0.0f, 1.0f, 1.0f );
                
                idRenderSystemFBOLocal::BlitFromTexture( tr.renderDepthImage, srcTexCoords, NULL, tr.hdrDepthFbo, NULL, NULL, NULL, 0 );
            }
            
            if ( r_sunlightMode->integer && backEnd.viewParms.flags & VPF_USESUNLIGHT )
            {
                vec2_t texCoords[4];
                vec4_t quadVerts[4], box;
                
                idRenderSystemFBOLocal::Bind( tr.screenShadowFbo );
                
                box[0] = backEnd.viewParms.viewportX * tr.screenShadowFbo->width / ( F32 )glConfig.vidWidth;
                box[1] = backEnd.viewParms.viewportY * tr.screenShadowFbo->height / ( F32 )glConfig.vidHeight;
                box[2] = backEnd.viewParms.viewportWidth * tr.screenShadowFbo->width / ( F32 )glConfig.vidWidth;
                box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / ( F32 )glConfig.vidHeight;
                
                qglViewport( ( S32 )box[0], ( S32 )box[1], ( S32 )box[2], ( S32 )box[3] );
                qglScissor( ( S32 )box[0], ( S32 )box[1], ( S32 )box[2], ( S32 )box[3] );
                
                box[0] = backEnd.viewParms.viewportX / ( F32 )glConfig.vidWidth;
                box[1] = backEnd.viewParms.viewportY / ( F32 )glConfig.vidHeight;
                box[2] = box[0] + backEnd.viewParms.viewportWidth / ( F32 )glConfig.vidWidth;
                box[3] = box[1] + backEnd.viewParms.viewportHeight / ( F32 )glConfig.vidHeight;
                
                texCoords[0][0] = box[0];
                texCoords[0][1] = box[3];
                texCoords[1][0] = box[2];
                texCoords[1][1] = box[3];
                texCoords[2][0] = box[2];
                texCoords[2][1] = box[1];
                texCoords[3][0] = box[0];
                texCoords[3][1] = box[1];
                
                box[0] = -1.0f;
                box[1] = -1.0f;
                box[2] = 1.0f;
                box[3] = 1.0f;
                
                VectorSet4( quadVerts[0], box[0], box[3], 0, 1 );
                VectorSet4( quadVerts[1], box[2], box[3], 0, 1 );
                VectorSet4( quadVerts[2], box[2], box[1], 0, 1 );
                VectorSet4( quadVerts[3], box[0], box[1], 0, 1 );
                
                State( GLS_DEPTHTEST_DISABLE );
                
                idRenderSystemGLSLLocal::BindProgram( &tr.shadowmaskShader );
                
                BindToTMU( tr.renderDepthImage, TB_COLORMAP );
                
                if ( r_shadowCascadeZFar->integer != 0 )
                {
                    BindToTMU( tr.sunShadowDepthImage[0], TB_SHADOWMAP );
                    BindToTMU( tr.sunShadowDepthImage[1], TB_SHADOWMAP2 );
                    BindToTMU( tr.sunShadowDepthImage[2], TB_SHADOWMAP3 );
                    BindToTMU( tr.sunShadowDepthImage[3], TB_SHADOWMAP4 );
                    
                    idRenderSystemGLSLLocal::SetUniformMat4( &tr.shadowmaskShader, UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[0] );
                    idRenderSystemGLSLLocal::SetUniformMat4( &tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1] );
                    idRenderSystemGLSLLocal::SetUniformMat4( &tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2] );
                    idRenderSystemGLSLLocal::SetUniformMat4( &tr.shadowmaskShader, UNIFORM_SHADOWMVP4, backEnd.refdef.sunShadowMvp[3] );
                }
                else
                {
                    BindToTMU( tr.sunShadowDepthImage[3], TB_SHADOWMAP );
                    idRenderSystemGLSLLocal::SetUniformMat4( &tr.shadowmaskShader, UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[3] );
                }
                
                idRenderSystemGLSLLocal::SetUniformVec3( &tr.shadowmaskShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg );
                {
                    vec3_t viewVector;
                    
                    F32 zmax = backEnd.viewParms.zFar;
                    F32 ymax = zmax * tan( backEnd.viewParms.fovY * M_PI / 360.0f );
                    F32 xmax = zmax * tan( backEnd.viewParms.fovX * M_PI / 360.0f );
                    
                    VectorScale( backEnd.refdef.viewaxis[0], zmax, viewVector );
                    idRenderSystemGLSLLocal::SetUniformVec3( &tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewVector );
                    VectorScale( backEnd.refdef.viewaxis[1], xmax, viewVector );
                    idRenderSystemGLSLLocal::SetUniformVec3( &tr.shadowmaskShader, UNIFORM_VIEWLEFT, viewVector );
                    VectorScale( backEnd.refdef.viewaxis[2], ymax, viewVector );
                    idRenderSystemGLSLLocal::SetUniformVec3( &tr.shadowmaskShader, UNIFORM_VIEWUP, viewVector );
                    
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo );
                }
                
                idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords ); //, color, shaderProgram, invTexRes);
                
                if ( r_shadowBlur->integer )
                {
                    viewInfo[2] = 1.0f / ( F32 )( tr.screenScratchFbo->width );
                    viewInfo[3] = 1.0f / ( F32 )( tr.screenScratchFbo->height );
                    
                    idRenderSystemFBOLocal::Bind( tr.screenScratchFbo );
                    
                    idRenderSystemGLSLLocal::BindProgram( &tr.depthBlurShader[0] );
                    
                    BindToTMU( tr.screenShadowImage, TB_COLORMAP );
                    BindToTMU( tr.hdrDepthImage, TB_LIGHTMAP );
                    
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo );
                    
                    idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
                    
                    idRenderSystemFBOLocal::Bind( tr.screenShadowFbo );
                    
                    idRenderSystemGLSLLocal::BindProgram( &tr.depthBlurShader[1] );
                    
                    BindToTMU( tr.screenScratchImage, TB_COLORMAP );
                    BindToTMU( tr.hdrDepthImage, TB_LIGHTMAP );
                    
                    idRenderSystemGLSLLocal::SetUniformVec4( &tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo );
                    
                    idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
                }
            }
            
            if ( r_ssao->integer )
            {
                vec4_t quadVerts[4];
                vec2_t texCoords[4];
                
                viewInfo[2] = 1.0f / ( ( F32 )( tr.quarterImage[0]->width ) * tan( backEnd.viewParms.fovX * M_PI / 360.0f ) * 2.0f );
                viewInfo[3] = 1.0f / ( ( F32 )( tr.quarterImage[0]->height ) * tan( backEnd.viewParms.fovY * M_PI / 360.0f ) * 2.0f );
                viewInfo[3] *= ( F32 )backEnd.viewParms.viewportHeight / ( F32 )backEnd.viewParms.viewportWidth;
                
                idRenderSystemFBOLocal::Bind( tr.quarterFbo[0] );
                
                qglViewport( 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height );
                qglScissor( 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height );
                
                VectorSet4( quadVerts[0], -1, 1, 0, 1 );
                VectorSet4( quadVerts[1], 1, 1, 0, 1 );
                VectorSet4( quadVerts[2], 1, -1, 0, 1 );
                VectorSet4( quadVerts[3], -1, -1, 0, 1 );
                
                texCoords[0][0] = 0;
                texCoords[0][1] = 1;
                texCoords[1][0] = 1;
                texCoords[1][1] = 1;
                texCoords[2][0] = 1;
                texCoords[2][1] = 0;
                texCoords[3][0] = 0;
                texCoords[3][1] = 0;
                
                State( GLS_DEPTHTEST_DISABLE );
                
                idRenderSystemGLSLLocal::BindProgram( &tr.ssaoShader );
                
                BindToTMU( tr.hdrDepthImage, TB_COLORMAP );
                
                idRenderSystemGLSLLocal::SetUniformVec4( &tr.ssaoShader, UNIFORM_VIEWINFO, viewInfo );
                
                idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
                
                viewInfo[2] = 1.0f / ( F32 )( tr.quarterImage[0]->width );
                viewInfo[3] = 1.0f / ( F32 )( tr.quarterImage[0]->height );
                
                idRenderSystemFBOLocal::Bind( tr.quarterFbo[1] );
                
                qglViewport( 0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height );
                qglScissor( 0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height );
                
                idRenderSystemGLSLLocal::BindProgram( &tr.depthBlurShader[0] );
                
                BindToTMU( tr.quarterImage[0], TB_COLORMAP );
                BindToTMU( tr.hdrDepthImage, TB_LIGHTMAP );
                
                idRenderSystemGLSLLocal::SetUniformVec4( &tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo );
                
                idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
                
                idRenderSystemFBOLocal::Bind( tr.screenSsaoFbo );
                
                qglViewport( 0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height );
                qglScissor( 0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height );
                
                idRenderSystemGLSLLocal::BindProgram( &tr.depthBlurShader[1] );
                
                BindToTMU( tr.quarterImage[1], TB_COLORMAP );
                BindToTMU( tr.hdrDepthImage, TB_LIGHTMAP );
                
                idRenderSystemGLSLLocal::SetUniformVec4( &tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo );
                
                idRenderSystemSurfaceLocal::InstantQuad2( quadVerts, texCoords );
            }
        }
        
        // reset viewport and scissor
        idRenderSystemFBOLocal::Bind( oldFbo );
        SetViewportAndScissor();
    }
    
    if ( glRefConfig.framebufferObject && ( backEnd.viewParms.flags & VPF_DEPTHCLAMP ) && glRefConfig.depthClamp )
    {
        qglDisable( GL_DEPTH_CLAMP );
    }
    
    if ( !isShadowView )
    {
        RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
        
        if ( r_drawSun->integer )
        {
            idRenderSystemSkyLocal::DrawSun( 0.1f, tr.sunShader );
        }
        
        if ( glRefConfig.framebufferObject && r_drawSunRays->integer )
        {
            FBO_t* oldFbo = glState.currentFBO;
            idRenderSystemFBOLocal::Bind( tr.sunRaysFbo );
            
            qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
            qglClear( GL_COLOR_BUFFER_BIT );
            
            if ( glRefConfig.occlusionQuery )
            {
                tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = true;
                qglBeginQuery( GL_SAMPLES_PASSED, tr.sunFlareQuery[tr.sunFlareQueryIndex] );
            }
            
            idRenderSystemSkyLocal::DrawSun( 0.3f, tr.sunFlareShader );
            
            if ( glRefConfig.occlusionQuery )
            {
                qglEndQuery( GL_SAMPLES_PASSED );
            }
            
            idRenderSystemFBOLocal::Bind( oldFbo );
        }
        
        // darken down any stencil shadows
        idRenderSystemShadowsLocal::ShadowFinish();
        
        // add light flares on lights that aren't obscured
        idRenderSystemFlaresLocal::RenderFlares();
    }
    
    
    if ( glRefConfig.framebufferObject && tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo )
    {
        cubemap_t* cubemap = &tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex];
        
        idRenderSystemFBOLocal::Bind( NULL );
        
        if ( cubemap && cubemap->image )
        {
            qglGenerateTextureMipmapEXT( cubemap->image->texnum, GL_TEXTURE_CUBE_MAP );
        }
    }
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::DrawBuffer
=============
*/
const void* idRenderSystemBackendLocal::DrawBuffer( const void* data )
{
    const drawBufferCommand_t* cmd = ( const drawBufferCommand_t* )data;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    if ( glRefConfig.framebufferObject )
    {
        idRenderSystemFBOLocal::Bind( NULL );
    }
    
    qglDrawBuffer( cmd->buffer );
    
    // clear screen for debugging
    if ( r_clear->integer )
    {
        qglClearColor( 1, 0, 0.5, 1 );
        qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
    
    return ( const void* )( cmd + 1 );
}

/*
===============
idRenderSystemBackendLocal::ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by idRenderSystemLocal::EndRegistration
===============
*/
void idRenderSystemBackendLocal::ShowImages( void )
{
    S32	i, start, end;
    F32	x, y, w, h;
    image_t* image;
    
    SetGL2D();
    
    qglClear( GL_COLOR_BUFFER_BIT );
    
    qglFinish();
    
    start = clientMainSystem->ScaledMilliseconds();
    
    for ( i = 0 ; i < tr.numImages ; i++ )
    {
        image = tr.images[i];
        
        w = ( F32 )( glConfig.vidWidth / 20 );
        h = ( F32 )( glConfig.vidHeight / 15 );
        x = i % 20 * w;
        y = i / 20 * h;
        
        // show in proportional size in mode 2
        if ( r_showImages->integer == 2 )
        {
            w *= image->uploadWidth / 512.0f;
            h *= image->uploadHeight / 512.0f;
        }
        
        {
            vec4_t quadVerts[4];
            
            BindToTMU( image, TB_COLORMAP );
            
            VectorSet4( quadVerts[0], x, y, 0, 1 );
            VectorSet4( quadVerts[1], x + w, y, 0, 1 );
            VectorSet4( quadVerts[2], x + w, y + h, 0, 1 );
            VectorSet4( quadVerts[3], x, y + h, 0, 1 );
            
            idRenderSystemSurfaceLocal::InstantQuad( quadVerts );
        }
    }
    
    qglFinish();
    
    end = clientMainSystem->ScaledMilliseconds();
    clientMainSystem->RefPrintf( PRINT_ALL, "%i msec to draw all images\n", end - start );
    
}

/*
=============
idRenderSystemBackendLocal::ColorMask
=============
*/
const void* idRenderSystemBackendLocal::ColorMask( const void* data )
{
    const colorMaskCommand_t* cmd = ( colorMaskCommand_t* )data;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    if ( glRefConfig.framebufferObject )
    {
        // reverse color mask, so 0 0 0 0 is the default
        backEnd.colorMask[0] = !cmd->rgba[0];
        backEnd.colorMask[1] = !cmd->rgba[1];
        backEnd.colorMask[2] = !cmd->rgba[2];
        backEnd.colorMask[3] = !cmd->rgba[3];
    }
    
    qglColorMask( cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3] );
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::ClearDepth
=============
*/
const void* idRenderSystemBackendLocal::ClearDepth( const void* data )
{
    const clearDepthCommand_t* cmd = ( clearDepthCommand_t* )data;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    // texture swapping test
    if ( r_showImages->integer )
    {
        ShowImages();
    }
    
    if ( glRefConfig.framebufferObject )
    {
        if ( !tr.renderFbo || backEnd.framePostProcessed )
        {
            idRenderSystemFBOLocal::Bind( NULL );
        }
        else
        {
            idRenderSystemFBOLocal::Bind( tr.renderFbo );
        }
    }
    
    qglClear( GL_DEPTH_BUFFER_BIT );
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::SwapBuffers
=============
*/
const void* idRenderSystemBackendLocal::SwapBuffers( const void* data )
{
    const swapBuffersCommand_t*	cmd;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    // texture swapping test
    if ( r_showImages->integer )
    {
        ShowImages();
    }
    
    cmd = ( const swapBuffersCommand_t* )data;
    
    // we measure overdraw by reading back the stencil buffer and
    // counting up the number of increments that have happened
    if ( r_measureOverdraw->integer )
    {
        S32 i;
        S64 sum = 0;
        U8* stencilReadback = nullptr;
        
        stencilReadback = ( U8* )memorySystem->AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
        qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );
        
        for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ )
        {
            sum += stencilReadback[i];
        }
        
        backEnd.pc.c_overDraw += sum;
        memorySystem->FreeTempMemory( stencilReadback );
    }
    
    if ( glRefConfig.framebufferObject )
    {
        if ( !backEnd.framePostProcessed )
        {
            if ( tr.renderFbo )
            {
                idRenderSystemFBOLocal::FastBlit( tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST );
            }
        }
    }
    
    if ( !glState.finishCalled )
    {
        qglFinish();
    }
    
    idRenderSystemGlimpLocal::LogComment( "***************** idRenderSystemBackendLocal::SwapBuffers *****************\n\n\n" );
    
    idRenderSystemGlimpLocal::EndFrame();
    
    backEnd.framePostProcessed = false;
    backEnd.projection2D = false;
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::PostProcess
=============
*/
const void* idRenderSystemBackendLocal::PostProcess( const void* data )
{
    const postProcessCommand_t* cmd = ( const postProcessCommand_t* )data;
    FBO_t* srcFbo;
    ivec4_t srcBox, dstBox;
    bool autoExposure;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    if ( !glRefConfig.framebufferObject || !r_postProcess->integer )
    {
        // do nothing
        return ( const void* )( cmd + 1 );
    }
    
    if ( cmd )
    {
        backEnd.refdef = cmd->refdef;
        backEnd.viewParms = cmd->viewParms;
    }
    
    srcFbo = tr.renderFbo;
    
    dstBox[0] = backEnd.viewParms.viewportX;
    dstBox[1] = backEnd.viewParms.viewportY;
    dstBox[2] = backEnd.viewParms.viewportWidth;
    dstBox[3] = backEnd.viewParms.viewportHeight;
    
    if ( r_ssao->integer )
    {
        srcBox[0] = backEnd.viewParms.viewportX      * tr.screenSsaoImage->width  / glConfig.vidWidth;
        srcBox[1] = backEnd.viewParms.viewportY      * tr.screenSsaoImage->height / glConfig.vidHeight;
        srcBox[2] = backEnd.viewParms.viewportWidth  * tr.screenSsaoImage->width  / glConfig.vidWidth;
        srcBox[3] = backEnd.viewParms.viewportHeight * tr.screenSsaoImage->height / glConfig.vidHeight;
        
        S32 blendMode = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
        if ( r_ssao->integer == 2 )
        {
            blendMode = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO;
        }
        
        idRenderSystemFBOLocal::Blit( tr.screenSsaoFbo, srcBox, NULL, srcFbo, dstBox, NULL, NULL, blendMode );
    }
    
    srcBox[0] = backEnd.viewParms.viewportX;
    srcBox[1] = backEnd.viewParms.viewportY;
    srcBox[2] = backEnd.viewParms.viewportWidth;
    srcBox[3] = backEnd.viewParms.viewportHeight;
    
    if ( srcFbo )
    {
        {
            if ( backEnd.refdef.rdflags & RDF_UNDERWATER )
            {
                idRenderSystemPostProcessLocal::Underwater( srcFbo, srcBox, tr.genericFbo, dstBox );
                idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
            }
        }
        
        if ( r_lensflare->integer )
        {
            idRenderSystemPostProcessLocal::LensFlare( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_rbm->integer )
        {
            idRenderSystemPostProcessLocal::RBM( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_darkexpand->integer )
        {
            for ( S32 pass = 0; pass < 2; pass++ )
            {
                idRenderSystemPostProcessLocal::DarkExpand( srcFbo, srcBox, tr.genericFbo, dstBox );
                idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
            }
        }
        
        if ( r_textureClean->integer )
        {
            idRenderSystemPostProcessLocal::TextureClean( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_esharpening->integer )
        {
            idRenderSystemPostProcessLocal::ESharpening( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_esharpening2->integer )
        {
            idRenderSystemPostProcessLocal::ESharpening2( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_multipost->integer )
        {
            idRenderSystemPostProcessLocal::MultiPost( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_truehdr->integer )
        {
            idRenderSystemPostProcessLocal::HDR( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_bloom->integer )
        {
            idRenderSystemPostProcessLocal::Bloom( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_ssr->value > 0.0 || r_sse->value > 0.0 )
        {
            idRenderSystemPostProcessLocal::ScreenSpaceReflections( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_textureDetail->integer )
        {
            idRenderSystemPostProcessLocal::TextureDetail( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_anamorphic->integer )
        {
            idRenderSystemPostProcessLocal::Anamorphic( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_ssgi->integer )
        {
            //for (S32 i = 0; i < r_ssgi->integer; i++)
            {
                idRenderSystemPostProcessLocal::SSGI( srcFbo, srcBox, tr.genericFbo, dstBox );
                idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
            }
        }
        
        if ( r_vibrancy->value > 0.0 )
        {
            idRenderSystemPostProcessLocal::Vibrancy( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_dof->integer )
        {
            for ( S32 pass_num = 0; pass_num < 3; pass_num++ )
            {
                idRenderSystemPostProcessLocal::DOF( srcFbo, srcBox, tr.genericFbo, dstBox );
                idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
            }
        }
        
        if ( r_fxaa->integer )
        {
            idRenderSystemPostProcessLocal::FXAA( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_trueAnaglyph->integer )
        {
            idRenderSystemPostProcessLocal::Anaglyph( srcFbo, srcBox, tr.genericFbo, dstBox );
            idRenderSystemFBOLocal::FastBlit( tr.genericFbo, srcBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        
        if ( r_hdr->integer && ( r_toneMap->integer || r_forceToneMap->integer ) )
        {
            autoExposure = r_autoExposure->integer || r_forceAutoExposure->integer;
            idRenderSystemPostProcessLocal::ToneMap( srcFbo, srcBox, NULL, dstBox, autoExposure );
        }
        else if ( r_cameraExposure->value == 0.0f )
        {
            idRenderSystemFBOLocal::FastBlit( srcFbo, srcBox, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }
        else
        {
            vec4_t color;
            
            color[0] =
                color[1] =
                    color[2] = powf( 2, r_cameraExposure->value );
            color[3] = 1.0f;
            
            idRenderSystemFBOLocal::Blit( srcFbo, srcBox, NULL, NULL, dstBox, NULL, color, 0 );
        }
        
    }
    
    if ( r_drawSunRays->integer )
    {
        idRenderSystemPostProcessLocal::SunRays( NULL, srcBox, NULL, dstBox );
    }
    
    if ( 1 )
    {
        idRenderSystemPostProcessLocal::BokehBlur( NULL, srcBox, NULL, dstBox, backEnd.refdef.blurFactor );
    }
    
    idRenderSystemPostProcessLocal::Contrast( NULL, srcBox, NULL, dstBox );
    
    backEnd.framePostProcessed = true;
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::ExportCubemaps
=============
*/
const void* idRenderSystemBackendLocal::ExportCubemaps( const void* data )
{
    const exportCubemapsCommand_t* cmd = ( const exportCubemapsCommand_t* )data;
    
    // finish any 2D drawing if needed
    if ( tess.numIndexes )
    {
        idRenderSystemShadeLocal::EndSurface();
    }
    
    if ( !glRefConfig.framebufferObject || !tr.world || tr.numCubemaps == 0 )
    {
        // do nothing
        clientMainSystem->RefPrintf( PRINT_ALL, "Nothing to export!\n" );
        return ( const void* )( cmd + 1 );
    }
    
    if ( cmd )
    {
        S32 i, j, sideSize = r_cubemapSize->integer * r_cubemapSize->integer * 4;
        U8* cubemapPixels = ( U8* )memorySystem->Malloc( sideSize * 6 );
        FBO_t* oldFbo = glState.currentFBO;
        
        idRenderSystemFBOLocal::Bind( tr.renderCubeFbo );
        
        for ( i = 0; i < tr.numCubemaps; i++ )
        {
            U8* p = cubemapPixels;
            UTF8 filename[MAX_QPATH];
            cubemap_t* cubemap = &tr.cubemaps[i];
            
            for ( j = 0; j < 6; j++ )
            {
                idRenderSystemFBOLocal::AttachImage( tr.renderCubeFbo, cubemap->image, GL_COLOR_ATTACHMENT0, j );
                qglReadPixels( 0, 0, r_cubemapSize->integer, r_cubemapSize->integer, GL_RGBA, GL_UNSIGNED_BYTE, p );
                p += sideSize;
            }
            
            if ( cubemap->name[0] )
            {
                Q_snprintf( filename, MAX_QPATH, "cubemaps/%s/%s.dds", tr.world->baseName, cubemap->name );
            }
            else
            {
                Q_snprintf( filename, MAX_QPATH, "cubemaps/%s/%03d.dds", tr.world->baseName, i );
            }
            
            idRenderSystemImageDDSLocal::SaveDDS( filename, cubemapPixels, r_cubemapSize->integer, r_cubemapSize->integer, 6 );
            
            clientMainSystem->RefPrintf( PRINT_ALL, "Saved cubemap %d as %s\n", i, filename );
        }
        
        idRenderSystemFBOLocal::Bind( oldFbo );
        
        memorySystem->Free( cubemapPixels );
    }
    
    return ( const void* )( cmd + 1 );
}

/*
=============
idRenderSystemBackendLocal::Finish
=============
*/
const void* idRenderSystemBackendLocal::Finish( const void* data )
{
    const renderFinishCommand_t* cmd;
    
    //clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemBackendLocal::Finish\n" );
    
    cmd = ( const renderFinishCommand_t* )data;
    
    qglFinish();
    
    return ( const void* )( cmd + 1 );
}

/*
====================
idRenderSystemBackendLocal::ExecuteRenderCommands
====================
*/
void idRenderSystemBackendLocal::ExecuteRenderCommands( const void* data )
{
    S32		t1, t2;
    
    t1 = clientMainSystem->ScaledMilliseconds();
    
    while ( 1 )
    {
        data = PADP( data, sizeof( void* ) );
        
        switch ( *( const S32* )data )
        {
            case RC_SET_COLOR:
                data = SetColor( data );
                break;
            case RC_STRETCH_PIC:
                data = StretchPic( data );
                break;
            case RC_DRAW_SURFS:
                data = DrawSurfs( data );
                break;
            case RC_DRAW_BUFFER:
                data = DrawBuffer( data );
                break;
            case RC_SWAP_BUFFERS:
                data = SwapBuffers( data );
                break;
            case RC_SCREENSHOT:
                data = idRenderSystemInitLocal::TakeScreenshotCmd( data );
                break;
            case RC_VIDEOFRAME:
                data = idRenderSystemInitLocal::TakeVideoFrameCmd( data );
                break;
            case RC_COLORMASK:
                data = ColorMask( data );
                break;
            case RC_CLEARDEPTH:
                data = ClearDepth( data );
                break;
            case RC_CONVOLVECUBEMAP:
                data = PrefilterEnvMap( data );
                break;
            case RC_POSTPROCESS:
                data = PostProcess( data );
                break;
            case RC_EXPORT_CUBEMAPS:
                data = ExportCubemaps( data );
                break;
            case RC_FINISH:
                data = Finish( data );
                break;
            case RC_END_OF_LIST:
            default:
                // finish any 2D drawing if needed
                if ( tess.numIndexes )
                {
                    idRenderSystemShadeLocal::EndSurface();
                }
                
                // stop rendering
                t2 = clientMainSystem->ScaledMilliseconds();
                backEnd.pc.msec = t2 - t1;
                return;
        }
    }
}

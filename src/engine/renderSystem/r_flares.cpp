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
// File name:   r_flares.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: A light flare is an effect that takes place inside the eye when bright light
//              sources are visible.The size of the flare reletive to the screen is nearly
//              constant, irrespective of distance, but the intensity should be proportional to the
//              projected area of the light source.
//
//              A surface that has been flagged as having a light flare will calculate the depth
//              buffer value that its midpoint should have when the surface is added.
//
//              After all opaque surfaces have been rendered, the depth buffer is read back for
//              each flare in view.If the point has not been obscured by a closer surface, the
//              flare should be drawn.
//
//              Surfaces that have a repeated texture should never be flagged as flaring, because
//              there will only be a single flare added at the midpoint of the polygon.
//
//              To prevent abrupt popping, the intensity of the flare is interpolated upand
//              down as it changes visibility.This involves scene to scene state, unlike almost
//              all other aspects of the renderer, and is complicated by the fact that a single
//              frame may have multiple scenes.
//
//              idRenderSystemFlaresLocal::RenderFlares() will be called once per view(twice in
//              a mirrored scene, potentially up to five or more times in a frame with 3D
//              status bar icons).
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemFlaresLocal renderSystemFlaresLocal;

/*
===============
idRenderSystemFlaresLocal::idRenderSystemFlaresLocal
===============
*/
idRenderSystemFlaresLocal::idRenderSystemFlaresLocal( void )
{
}

/*
===============
idRenderSystemFlaresLocal::~idRenderSystemFlaresLocal
===============
*/
idRenderSystemFlaresLocal::~idRenderSystemFlaresLocal( void )
{
}

flare_t r_flareStructs[MAX_FLARES];
flare_t* r_activeFlares, *r_inactiveFlares;
static F32 flareCoeff;

/*
==================
idRenderSystemFlaresLocal::SetFlareCoeff
==================
*/
void idRenderSystemFlaresLocal::SetFlareCoeff( void )
{

    if ( r_flareCoeff->value == 0.0f )
    {
        flareCoeff = ( F32 )atof( FLARE_STDCOEFF );
    }
    else
    {
        flareCoeff = r_flareCoeff->value;
    }
}

/*
==================
idRenderSystemFlaresLocal::ClearFlares
==================
*/
void idRenderSystemFlaresLocal::ClearFlares( void )
{
    S32	i;
    
    ::memset( r_flareStructs, 0, sizeof( r_flareStructs ) );
    r_activeFlares = nullptr;
    r_inactiveFlares = nullptr;
    
    for ( i = 0 ; i < MAX_FLARES ; i++ )
    {
        r_flareStructs[i].next = r_inactiveFlares;
        r_inactiveFlares = &r_flareStructs[i];
    }
    
    SetFlareCoeff();
}

/*
==================
idRenderSystemFlaresLocal::AddFlare

This is called at surface tesselation time
==================
*/
void idRenderSystemFlaresLocal::AddFlare( void* surface, S32 fogNum, vec3_t point, vec3_t color, vec3_t normal )
{
    S32 i;
    F32 d = 1;
    vec3_t local;
    vec4_t eye, clip, normalized, window;
    flare_t* f;
    
    backEnd.pc.c_flareAdds++;
    
    if ( normal && ( normal[0] || normal[1] || normal[2] ) )
    {
        VectorSubtract( backEnd.viewParms.orientation.origin, point, local );
        VectorNormalize( local );
        d = DotProduct( local, normal );
        
        // If the viewer is behind the flare don't add it.
        if ( d < 0 )
        {
            return;
        }
    }
    
    // if the point is off the screen, don't bother adding it
    // calculate screen coordinates and depth
    idRenderSystemMainLocal::TransformModelToClip( point, backEnd.orientation.modelMatrix, backEnd.viewParms.projectionMatrix, eye, clip );
    
    // check to see if the point is completely off screen
    for ( i = 0 ; i < 3 ; i++ )
    {
        if ( clip[i] >= clip[3] || clip[i] <= -clip[3] )
        {
            return;
        }
    }
    
    idRenderSystemMainLocal::TransformClipToWindow( clip, &backEnd.viewParms, normalized, window );
    
    if ( window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth || window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight )
    {
        // shouldn't happen, since we check the clip[] above, except for FP rounding
        return;
    }
    
    // see if a flare with a matching surface, scene, and view exists
    for ( f = r_activeFlares ; f ; f = f->next )
    {
        if ( f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum
                && f->inPortal == backEnd.viewParms.isPortal )
        {
            break;
        }
    }
    
    // allocate a new one
    if ( !f )
    {
        if ( !r_inactiveFlares )
        {
            // the list is completely full
            return;
        }
        
        f = r_inactiveFlares;
        r_inactiveFlares = r_inactiveFlares->next;
        f->next = r_activeFlares;
        r_activeFlares = f;
        
        f->surface = surface;
        f->frameSceneNum = backEnd.viewParms.frameSceneNum;
        f->inPortal = backEnd.viewParms.isPortal;
        f->addedFrame = -1;
    }
    
    if ( f->addedFrame != backEnd.viewParms.frameCount - 1 )
    {
        f->visible = false;
        f->fadeTime = backEnd.refdef.time - 2000;
    }
    
    f->addedFrame = backEnd.viewParms.frameCount;
    f->fogNum = fogNum;
    
    VectorCopy( point, f->origin );
    VectorCopy( color, f->color );
    
    // fade the intensity of the flare down as the
    // light surface turns away from the viewer
    VectorScale( f->color, d, f->color );
    
    // save info needed to test
    f->windowX = ( S32 )( backEnd.viewParms.viewportX + window[0] );
    f->windowY = ( S32 )( backEnd.viewParms.viewportY + window[1] );
    
    f->eyeZ = eye[2];
}

/*
==================
idRenderSystemFlaresLocal::AddDlightFlares
==================
*/
void idRenderSystemFlaresLocal::AddDlightFlares( void )
{
    S32 i, j, k;
    dlight_t* l;
    fog_t* fog = nullptr;
    
    if ( !r_flares->integer )
    {
        return;
    }
    
    l = backEnd.refdef.dlights;
    
    if ( tr.world )
    {
        fog = tr.world->fogs;
    }
    
    for ( i = 0 ; i < backEnd.refdef.num_dlights ; i++, l++ )
    {
    
        if ( fog )
        {
            // find which fog volume the light is in
            for ( j = 1 ; j < tr.world->numfogs ; j++ )
            {
                fog = &tr.world->fogs[j];
                
                for ( k = 0 ; k < 3 ; k++ )
                {
                    if ( l->origin[k] < fog->bounds[0][k] || l->origin[k] > fog->bounds[1][k] )
                    {
                        break;
                    }
                }
                
                if ( k == 3 )
                {
                    break;
                }
            }
            
            if ( j == tr.world->numfogs )
            {
                j = 0;
            }
        }
        else
        {
            j = 0;
        }
        
        AddFlare( ( void* )l, j, l->origin, l->color, NULL );
    }
}

/*
==================
idRenderSystemFlaresLocal::TestFlare
==================
*/
void idRenderSystemFlaresLocal::TestFlare( flare_t* f )
{
    F32 depth, fade, screenZ;
    FBO_t* oldFbo;
    bool visible;
    
    backEnd.pc.c_flareTests++;
    
    // doing a readpixels is as good as doing a glFinish(), so
    // don't bother with another sync
    glState.finishCalled = false;
    
    // if we're doing multisample rendering, read from the correct FBO
    oldFbo = glState.currentFBO;
    
    // read back the z buffer contents
    qglReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );
    
    screenZ = backEnd.viewParms.projectionMatrix[14] / ( ( 2 * depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );
    
    visible = ( -f->eyeZ - -screenZ ) < 24;
    
    if ( visible )
    {
        if ( !f->visible )
        {
            f->visible = true;
            f->fadeTime = backEnd.refdef.time - 1;
        }
        
        fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
    }
    else
    {
        if ( f->visible )
        {
            f->visible = false;
            f->fadeTime = backEnd.refdef.time - 1;
        }
        
        fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
    }
    
    if ( fade < 0 )
    {
        fade = 0;
    }
    
    if ( fade > 1 )
    {
        fade = 1;
    }
    
    f->drawIntensity = fade;
}

/*
==================
idRenderSystemFlaresLocal::RenderFlare
==================
*/
void idRenderSystemFlaresLocal::RenderFlare( flare_t* f )
{
    S32	iColor[3], iAlpha, srcBlend;
    F32	size, distance, intensity, factor;
    U8 fogFactors[3] = {255, 255, 255};
    vec3_t color;
    
    backEnd.pc.c_flareRenders++;
    
    // We don't want too big values anyways when dividing by distance.
    if ( f->eyeZ > -1.0f )
    {
        distance = 1.0f;
    }
    else
    {
        distance = -f->eyeZ;
    }
    
    // calculate the flare size..
    size = backEnd.viewParms.viewportWidth * ( r_flareSize->value / 640.0f + 8 / distance );
    
    /*
     * This is an alternative to intensity scaling. It changes the size of the flare on screen instead
     * with growing distance. See in the description at the top why this is not the way to go.
    	// size will change ~ 1/r.
    	size = backEnd.viewParms.viewportWidth * (r_flareSize->value / (distance * -2.0f));
    */
    
    /*
     * As flare sizes stay nearly constant with increasing distance we must decrease the intensity
     * to achieve a reasonable visual result. The intensity is ~ (size^2 / distance^2) which can be
     * got by considering the ratio of
     * (flaresurface on screen) : (Surface of sphere defined by flare origin and distance from flare)
     * An important requirement is:
     * intensity <= 1 for all distances.
     *
     * The formula used here to compute the intensity is as follows:
     * intensity = flareCoeff * size^2 / (distance + size*sqrt(flareCoeff))^2
     * As you can see, the intensity will have a max. of 1 when the distance is 0.
     * The coefficient flareCoeff will determine the falloff speed with increasing distance.
     */
    
    factor = distance + size * ( F32 )sqrt( ( F32 )flareCoeff );
    
    intensity = flareCoeff * size * size / ( factor * factor );
    
    // Calculations for RGBA
    srcBlend = tr.flareShader->stages[0] ? ( tr.flareShader->stages[0]->stateBits & GLS_SRCBLEND_BITS ) : 0;
    if ( srcBlend == GLS_SRCBLEND_ONE )
    {
        // Q3 flare, fade color. blendfunc GL_ONE GL_ONE
        VectorScale( f->color, f->drawIntensity * intensity, color );
        iAlpha = 255;
    }
    else
    {
        VectorScale( f->color, intensity, color );
        iAlpha = ( S32 )f->drawIntensity * 255;
    }
    
    // Calculations for fogging
    if ( tr.world && f->fogNum > 0 && f->fogNum < tr.world->numfogs )
    {
        tess.numVertexes = 1;
        VectorCopy( f->origin, tess.xyz[0] );
        tess.fogNum = f->fogNum;
        
        idRenderSystemShadeLocal::CalcModulateColorsByFog( fogFactors );
        
        // We don't need to render the flare if colors are 0 anyways.
        if ( !( fogFactors[0] || fogFactors[1] || fogFactors[2] ) )
        {
            return;
        }
    }
    
    iColor[0] = ( S32 )( color[0] * fogFactors[0] * 257 );
    iColor[1] = ( S32 )( color[1] * fogFactors[1] * 257 );
    iColor[2] = ( S32 )( color[2] * fogFactors[2] * 257 );
    
    idRenderSystemShadeLocal::BeginSurface( tr.flareShader, f->fogNum, 0 );
    
    // FIXME: use quadstamp?
    tess.xyz[tess.numVertexes][0] = f->windowX - size;
    tess.xyz[tess.numVertexes][1] = f->windowY - size;
    tess.texCoords[tess.numVertexes][0] = 0;
    tess.texCoords[tess.numVertexes][1] = 0;
    tess.color[tess.numVertexes][0] = iColor[0];
    tess.color[tess.numVertexes][1] = iColor[1];
    tess.color[tess.numVertexes][2] = iColor[2];
    tess.color[tess.numVertexes][3] = 65535;
    tess.numVertexes++;
    
    tess.xyz[tess.numVertexes][0] = f->windowX - size;
    tess.xyz[tess.numVertexes][1] = f->windowY + size;
    tess.texCoords[tess.numVertexes][0] = 0;
    tess.texCoords[tess.numVertexes][1] = 1;
    tess.color[tess.numVertexes][0] = iColor[0];
    tess.color[tess.numVertexes][1] = iColor[1];
    tess.color[tess.numVertexes][2] = iColor[2];
    tess.color[tess.numVertexes][3] = 65535;
    
    tess.numVertexes++;
    
    tess.xyz[tess.numVertexes][0] = f->windowX + size;
    tess.xyz[tess.numVertexes][1] = f->windowY + size;
    tess.texCoords[tess.numVertexes][0] = 1;
    tess.texCoords[tess.numVertexes][1] = 1;
    tess.color[tess.numVertexes][0] = iColor[0];
    tess.color[tess.numVertexes][1] = iColor[1];
    tess.color[tess.numVertexes][2] = iColor[2];
    tess.color[tess.numVertexes][3] = 65535;
    
    tess.numVertexes++;
    
    tess.xyz[tess.numVertexes][0] = f->windowX + size;
    tess.xyz[tess.numVertexes][1] = f->windowY - size;
    tess.texCoords[tess.numVertexes][0] = 1;
    tess.texCoords[tess.numVertexes][1] = 0;
    tess.color[tess.numVertexes][0] = iColor[0];
    tess.color[tess.numVertexes][1] = iColor[1];
    tess.color[tess.numVertexes][2] = iColor[2];
    tess.color[tess.numVertexes][3] = 65535;
    
    tess.numVertexes++;
    
    tess.indexes[tess.numIndexes++] = 0;
    tess.indexes[tess.numIndexes++] = 1;
    tess.indexes[tess.numIndexes++] = 2;
    tess.indexes[tess.numIndexes++] = 0;
    tess.indexes[tess.numIndexes++] = 2;
    tess.indexes[tess.numIndexes++] = 3;
    
    idRenderSystemShadeLocal::EndSurface();
}

/*
==================
idRenderSystemFlaresLocal::RenderFlares

Because flares are simulating an occular effect, they should be drawn after
everything (all views) in the entire frame has been drawn.

Because of the way portals use the depth buffer to mark off areas, the
needed information would be lost after each view, so we are forced to draw
flares after each view.

The resulting artifact is that flares in mirrors or portals don't dim properly
when occluded by something in the main view, and portal flares that should
extend past the portal edge will be overwritten.
==================
*/
void idRenderSystemFlaresLocal::RenderFlares( void )
{
    flare_t* f, ** prev;
    mat4_t  oldmodelview, oldprojection, matrix;
    bool draw;
    
    if ( !r_flares->integer )
    {
        return;
    }
    
    if ( r_flareCoeff->modified )
    {
        SetFlareCoeff();
        r_flareCoeff->modified = false;
    }
    
    // Reset currentEntity to world so that any previously referenced entities
    // don't have influence on the rendering of these flares (i.e. RF_ renderer flags).
    backEnd.currentEntity = &tr.worldEntity;
    backEnd.orientation = backEnd.viewParms.world;
    
    //AddDlightFlares();
    
    // perform z buffer readback on each flare in this view
    draw = false;
    prev = &r_activeFlares;
    
    while ( ( f = *prev ) != NULL )
    {
        // throw out any flares that weren't added last frame
        if ( f->addedFrame < backEnd.viewParms.frameCount - 1 )
        {
            *prev = f->next;
            f->next = r_inactiveFlares;
            r_inactiveFlares = f;
            continue;
        }
        
        // don't draw any here that aren't from this scene / portal
        f->drawIntensity = 0;
        if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal )
        {
            TestFlare( f );
            
            if ( f->drawIntensity )
            {
                draw = true;
            }
            else
            {
                // this flare has completely faded out, so remove it from the chain
                *prev = f->next;
                f->next = r_inactiveFlares;
                r_inactiveFlares = f;
                continue;
            }
        }
        
        prev = &f->next;
    }
    
    if ( !draw )
    {
        // none visible
        return;
    }
    
    if ( backEnd.viewParms.isPortal )
    {
        qglDisable( GL_CLIP_PLANE0 );
    }
    
    idRenderSystemMathsLocal::Mat4Copy( glState.projection, oldprojection );
    idRenderSystemMathsLocal::Mat4Copy( glState.modelview, oldmodelview );
    idRenderSystemMathsLocal::Mat4Identity( matrix );
    idRenderSystemBackendLocal::SetModelviewMatrix( matrix );
    idRenderSystemMathsLocal::Mat4Ortho( ( F32 )backEnd.viewParms.viewportX, ( F32 )( backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth ),
                                         ( F32 )backEnd.viewParms.viewportY, ( F32 )( backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight ),
                                         -99999, 99999, matrix );
    idRenderSystemBackendLocal::SetModelviewMatrix( matrix );
    
    for ( f = r_activeFlares ; f ; f = f->next )
    {
        if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal && f->drawIntensity )
        {
            RenderFlare( f );
        }
    }
    
    idRenderSystemBackendLocal::SetProjectionMatrix( oldprojection );
    idRenderSystemBackendLocal::SetModelviewMatrix( oldmodelview );
}

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
// File name:   r_main.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: main control flow for each frame
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

trGlobals_t	tr;

idRenderSystemLocal	renderSystemLocal;
idRenderSystem*	renderSystem = &renderSystemLocal;

static F32 s_flipMatrix[16] =
{
    // convert from our coordinate system (looking down X)
    // to OpenGL's coordinate system (looking down -Z)
    0, 0, -1, 0,
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1
};

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t	entitySurface = SF_ENTITY;

idRenderSystemMainLocal renderSystemMainLocal;

/*
===============
idRenderSystemMainLocal::idRenderSystemMainLocal
===============
*/
idRenderSystemMainLocal::idRenderSystemMainLocal( void )
{
}

/*
===============
idRenderSystemMainLocal::~idRenderSystemMainLocal
===============
*/
idRenderSystemMainLocal::~idRenderSystemMainLocal( void )
{
}


/*
================
idRenderSystemMainLocal::CompareVert
================
*/
bool idRenderSystemMainLocal::CompareVert( srfVert_t* v1, srfVert_t* v2, bool checkST )
{
    S32 i;
    
    for ( i = 0; i < 3; i++ )
    {
        if ( floor( v1->xyz[i] + 0.1 ) != floor( v2->xyz[i] + 0.1 ) )
        {
            return false;
        }
        
        if ( checkST && ( ( v1->st[0] != v2->st[0] ) || ( v1->st[1] != v2->st[1] ) ) )
        {
            return false;
        }
    }
    
    return true;
}

/*
http://www.terathon.com/code/tangent.html
*/
void idRenderSystemMainLocal::CalcTexDirs( vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3 )
{
    F32 x1, x2, y1, y2, z1, z2;
    F32	s1, s2, t1, t2, r;
    
    x1 = v2[0] - v1[0];
    x2 = v3[0] - v1[0];
    y1 = v2[1] - v1[1];
    y2 = v3[1] - v1[1];
    z1 = v2[2] - v1[2];
    z2 = v3[2] - v1[2];
    
    s1 = w2[0] - w1[0];
    s2 = w3[0] - w1[0];
    t1 = w2[1] - w1[1];
    t2 = w3[1] - w1[1];
    
    r = s1 * t2 - s2 * t1;
    if ( r ) r = 1.0f / r;
    
    VectorSet( sdir, ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
    VectorSet( tdir, ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );
}

/*
=============
idRenderSystemMainLocal::CalcTangentSpace

Lengyel, Eric. �Computing Tangent Space Basis Vectors for an Arbitrary Mesh�. Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/src/tangent.html
=============
*/
F32 idRenderSystemMainLocal::CalcTangentSpace( vec3_t tangent, vec3_t bitangent, const vec3_t normal, const vec3_t sdir, const vec3_t tdir )
{
    F32 n_dot_t, handedness;
    vec3_t n_cross_t;
    
    // Gram-Schmidt orthogonalize
    n_dot_t = DotProduct( normal, sdir );
    VectorMA( sdir, -n_dot_t, normal, tangent );
    VectorNormalize( tangent );
    
    // Calculate handedness
    CrossProduct( normal, sdir, n_cross_t );
    handedness = ( DotProduct( n_cross_t, tdir ) < 0.0f ) ? -1.0f : 1.0f;
    
    // Calculate orthogonal bitangent, if necessary
    if ( bitangent )
    {
        CrossProduct( normal, tangent, bitangent );
    }
    
    return handedness;
}

bool idRenderSystemMainLocal::CalcTangentVectors( srfVert_t* dv[3] )
{
    S32 i;
    F32 bb, s, t;
    vec3_t bary;
    
    /* calculate barycentric basis for the triangle */
    bb = ( dv[1]->st[0] - dv[0]->st[0] ) * ( dv[2]->st[1] - dv[0]->st[1] ) - ( dv[2]->st[0] - dv[0]->st[0] ) * ( dv[1]->st[1] - dv[0]->st[1] );
    
    if ( fabs( bb ) < 0.00000001f )
    {
        return false;
    }
    
    /* do each vertex */
    for ( i = 0; i < 3; i++ )
    {
        vec4_t tangent;
        vec3_t normal, bitangent, nxt;
        
        // calculate s tangent vector
        s = dv[i]->st[0] + 10.0f;
        t = dv[i]->st[1];
        bary[0] = ( ( dv[1]->st[0] - s ) * ( dv[2]->st[1] - t ) - ( dv[2]->st[0] - s ) * ( dv[1]->st[1] - t ) ) / bb;
        bary[1] = ( ( dv[2]->st[0] - s ) * ( dv[0]->st[1] - t ) - ( dv[0]->st[0] - s ) * ( dv[2]->st[1] - t ) ) / bb;
        bary[2] = ( ( dv[0]->st[0] - s ) * ( dv[1]->st[1] - t ) - ( dv[1]->st[0] - s ) * ( dv[0]->st[1] - t ) ) / bb;
        
        tangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
        tangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
        tangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];
        
        VectorSubtract( tangent, dv[i]->xyz, tangent );
        VectorNormalize( tangent );
        
        // calculate t tangent vector
        s = dv[i]->st[0];
        t = dv[i]->st[1] + 10.0f;
        
        bary[0] = ( ( dv[1]->st[0] - s ) * ( dv[2]->st[1] - t ) - ( dv[2]->st[0] - s ) * ( dv[1]->st[1] - t ) ) / bb;
        bary[1] = ( ( dv[2]->st[0] - s ) * ( dv[0]->st[1] - t ) - ( dv[0]->st[0] - s ) * ( dv[2]->st[1] - t ) ) / bb;
        bary[2] = ( ( dv[0]->st[0] - s ) * ( dv[1]->st[1] - t ) - ( dv[1]->st[0] - s ) * ( dv[0]->st[1] - t ) ) / bb;
        
        bitangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
        bitangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
        bitangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];
        
        VectorSubtract( bitangent, dv[i]->xyz, bitangent );
        VectorNormalize( bitangent );
        
        // store bitangent handedness
        idRenderSystemVaoLocal::VaoUnpackNormal( normal, dv[i]->normal );
        CrossProduct( normal, tangent, nxt );
        tangent[3] = ( DotProduct( nxt, bitangent ) < 0.0f ) ? -1.0f : 1.0f;
        
        idRenderSystemVaoLocal::VaoPackTangent( dv[i]->tangent, tangent );
    }
    
    return true;
}

/*
=================
idRenderSystemMainLocal::CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
S32 idRenderSystemMainLocal::CullLocalBox( vec3_t localBounds[2] )
{
    S32 j;
    vec3_t transformed, v, worldBounds[2];
    
    if ( r_nocull->integer )
    {
        return CULL_CLIP;
    }
    
    // transform into world space
    ClearBounds( worldBounds[0], worldBounds[1] );
    
    for ( j = 0; j < 8; j++ )
    {
        v[0] = localBounds[j & 1][0];
        v[1] = localBounds[( j >> 1 ) & 1][1];
        v[2] = localBounds[( j >> 2 ) & 1][2];
        
        LocalPointToWorld( v, transformed );
        
        AddPointToBounds( transformed, worldBounds[0], worldBounds[1] );
    }
    
    return CullBox( worldBounds );
}

/*
=================
idRenderSystemMainLocal::CullBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
S32 idRenderSystemMainLocal::CullBox( vec3_t worldBounds[2] )
{
    S32 i, r, numPlanes;
    cplane_t* frust;
    bool anyClip;
    
    numPlanes = ( tr.viewParms.flags & VPF_FARPLANEFRUSTUM ) ? 5 : 4;
    
    // check against frustum planes
    anyClip = false;
    
    for ( i = 0; i < numPlanes; i++ )
    {
        frust = &tr.viewParms.frustum[i];
        
        r = collisionModelManager->BoxOnPlaneSide( worldBounds[0], worldBounds[1], frust );
        
        if ( r == 2 )
        {
            // completely outside frustum
            return CULL_OUT;
        }
        
        if ( r == 3 )
        {
            anyClip = true;
        }
    }
    
    if ( !anyClip )
    {
        // completely inside frustum
        return CULL_IN;
    }
    
    // partially clipped
    return CULL_CLIP;
}

/*
** idRenderSystemMainLocal::CullLocalPointAndRadius
*/
S32 idRenderSystemMainLocal::CullLocalPointAndRadius( const vec3_t pt, F32 radius )
{
    vec3_t transformed;
    
    LocalPointToWorld( pt, transformed );
    
    return CullPointAndRadius( transformed, radius );
}

/*
** idRenderSystemMainLocal::CullPointAndRadius
*/
S32 idRenderSystemMainLocal::CullPointAndRadiusEx( const vec3_t pt, F32 radius, const cplane_t* frustum, S32 numPlanes )
{
    S32	i;
    F32	dist;
    const cplane_t*	frust;
    bool mightBeClipped = false;
    
    if ( r_nocull->integer )
    {
        return CULL_CLIP;
    }
    
    // check against frustum planes
    for ( i = 0 ; i < numPlanes ; i++ )
    {
        frust = &frustum[i];
        
        dist = DotProduct( pt, frust->normal ) - frust->dist;
        
        if ( dist < -radius )
        {
            return CULL_OUT;
        }
        else if ( dist <= radius )
        {
            mightBeClipped = true;
        }
    }
    
    if ( mightBeClipped )
    {
        return CULL_CLIP;
    }
    
    // completely inside frustum
    return CULL_IN;
}

/*
** idRenderSystemMainLocal::CullPointAndRadius
*/
S32 idRenderSystemMainLocal::CullPointAndRadius( const vec3_t pt, F32 radius )
{
    return CullPointAndRadiusEx( pt, radius, tr.viewParms.frustum, ( tr.viewParms.flags & VPF_FARPLANEFRUSTUM ) ? 5 : 4 );
}

/*
=================
idRenderSystemMainLocal::LocalNormalToWorld
=================
*/
void idRenderSystemMainLocal::LocalNormalToWorld( const vec3_t local, vec3_t world )
{
    world[0] = local[0] * tr.orientation.axis[0][0] + local[1] * tr.orientation.axis[1][0] + local[2] * tr.orientation.axis[2][0];
    world[1] = local[0] * tr.orientation.axis[0][1] + local[1] * tr.orientation.axis[1][1] + local[2] * tr.orientation.axis[2][1];
    world[2] = local[0] * tr.orientation.axis[0][2] + local[1] * tr.orientation.axis[1][2] + local[2] * tr.orientation.axis[2][2];
}

/*
=================
idRenderSystemMainLocal::LocalPointToWorld
=================
*/
void idRenderSystemMainLocal::LocalPointToWorld( const vec3_t local, vec3_t world )
{
    world[0] = local[0] * tr.orientation.axis[0][0] + local[1] * tr.orientation.axis[1][0] + local[2] * tr.orientation.axis[2][0] + tr.orientation.origin[0];
    world[1] = local[0] * tr.orientation.axis[0][1] + local[1] * tr.orientation.axis[1][1] + local[2] * tr.orientation.axis[2][1] + tr.orientation.origin[1];
    world[2] = local[0] * tr.orientation.axis[0][2] + local[1] * tr.orientation.axis[1][2] + local[2] * tr.orientation.axis[2][2] + tr.orientation.origin[2];
}

/*
=================
idRenderSystemMainLocal::WorldToLocal
=================
*/
void idRenderSystemMainLocal::WorldToLocal( const vec3_t world, vec3_t local )
{
    local[0] = DotProduct( world, tr.orientation.axis[0] );
    local[1] = DotProduct( world, tr.orientation.axis[1] );
    local[2] = DotProduct( world, tr.orientation.axis[2] );
}

/*
==========================
idRenderSystemMainLocal::TransformModelToClip
==========================
*/
void idRenderSystemMainLocal::TransformModelToClip( const vec3_t src, const F32* modelMatrix, const F32* projectionMatrix, vec4_t eye, vec4_t dst )
{
    S32 i;
    
    for ( i = 0 ; i < 4 ; i++ )
    {
        eye[i] =
            src[0] * modelMatrix[ i + 0 * 4 ] +
            src[1] * modelMatrix[ i + 1 * 4 ] +
            src[2] * modelMatrix[ i + 2 * 4 ] +
            1 * modelMatrix[ i + 3 * 4 ];
    }
    
    for ( i = 0 ; i < 4 ; i++ )
    {
        dst[i] =
            eye[0] * projectionMatrix[ i + 0 * 4 ] +
            eye[1] * projectionMatrix[ i + 1 * 4 ] +
            eye[2] * projectionMatrix[ i + 2 * 4 ] +
            eye[3] * projectionMatrix[ i + 3 * 4 ];
    }
}

/*
==========================
idRenderSystemMainLocal::TransformClipToWindow
==========================
*/
void idRenderSystemMainLocal::TransformClipToWindow( const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window )
{
    normalized[0] = clip[0] / clip[3];
    normalized[1] = clip[1] / clip[3];
    normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );
    
    window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
    window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
    window[2] = normalized[2];
    
    window[0] = ( F32 )( window[0] + 0.5f );
    window[1] = ( F32 )( window[1] + 0.5f );
}


/*
==========================
idRenderSystemMainLocal::myGlMultMatrix
==========================
*/
void idRenderSystemMainLocal::myGlMultMatrix( const F32* a, const F32* b, F32* out )
{
    S32	i, j;
    
    for ( i = 0 ; i < 4 ; i++ )
    {
        for ( j = 0 ; j < 4 ; j++ )
        {
            out[ i * 4 + j ] =
                a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
                + a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
                + a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
                + a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
        }
    }
}

/*
=================
idRenderSystemMainLocal::RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void idRenderSystemMainLocal::RotateForEntity( const trRefEntity_t* ent, const viewParms_t* viewParms, orientationr_t* orientation )
{
    F32	glMatrix[16], axisLength;
    vec3_t	delta;
    
    VectorCopy( ent->e.origin, orientation->origin );
    
    VectorCopy( ent->e.axis[0], orientation->axis[0] );
    VectorCopy( ent->e.axis[1], orientation->axis[1] );
    VectorCopy( ent->e.axis[2], orientation->axis[2] );
    
    glMatrix[0] = orientation->axis[0][0];
    glMatrix[4] = orientation->axis[1][0];
    glMatrix[8] = orientation->axis[2][0];
    glMatrix[12] = orientation->origin[0];
    
    glMatrix[1] = orientation->axis[0][1];
    glMatrix[5] = orientation->axis[1][1];
    glMatrix[9] = orientation->axis[2][1];
    glMatrix[13] = orientation->origin[1];
    
    glMatrix[2] = orientation->axis[0][2];
    glMatrix[6] = orientation->axis[1][2];
    glMatrix[10] = orientation->axis[2][2];
    glMatrix[14] = orientation->origin[2];
    
    glMatrix[3] = 0;
    glMatrix[7] = 0;
    glMatrix[11] = 0;
    glMatrix[15] = 1;
    
    idRenderSystemMathsLocal::Mat4Copy( glMatrix, orientation->transformMatrix );
    myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, orientation->modelMatrix );
    
    // calculate the viewer origin in the model's space
    // needed for fog, specular, and environment mapping
    VectorSubtract( viewParms->orientation.origin, orientation->origin, delta );
    
    // compensate for scale in the axes if necessary
    if ( ent->e.nonNormalizedAxes )
    {
        axisLength = VectorLength( ent->e.axis[0] );
        
        if ( !axisLength )
        {
            axisLength = 0;
        }
        else
        {
            axisLength = 1.0f / axisLength;
        }
    }
    else
    {
        axisLength = 1.0f;
    }
    
    orientation->viewOrigin[0] = DotProduct( delta, orientation->axis[0] ) * axisLength;
    orientation->viewOrigin[1] = DotProduct( delta, orientation->axis[1] ) * axisLength;
    orientation->viewOrigin[2] = DotProduct( delta, orientation->axis[2] ) * axisLength;
}

/*
=================
idRenderSystemMainLocal::RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/
void idRenderSystemMainLocal::RotateForViewer( void )
{
    F32	viewerMatrix[16];
    vec3_t origin;
    
    ::memset( &tr.orientation, 0, sizeof( tr.orientation ) );
    tr.orientation.axis[0][0] = 1;
    tr.orientation.axis[1][1] = 1;
    tr.orientation.axis[2][2] = 1;
    VectorCopy( tr.viewParms.orientation.origin, tr.orientation.viewOrigin );
    
    // transform by the camera placement
    VectorCopy( tr.viewParms.orientation.origin, origin );
    
    viewerMatrix[0] = tr.viewParms.orientation.axis[0][0];
    viewerMatrix[4] = tr.viewParms.orientation.axis[0][1];
    viewerMatrix[8] = tr.viewParms.orientation.axis[0][2];
    viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];
    
    viewerMatrix[1] = tr.viewParms.orientation.axis[1][0];
    viewerMatrix[5] = tr.viewParms.orientation.axis[1][1];
    viewerMatrix[9] = tr.viewParms.orientation.axis[1][2];
    viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];
    
    viewerMatrix[2] = tr.viewParms.orientation.axis[2][0];
    viewerMatrix[6] = tr.viewParms.orientation.axis[2][1];
    viewerMatrix[10] = tr.viewParms.orientation.axis[2][2];
    viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];
    
    viewerMatrix[3] = 0;
    viewerMatrix[7] = 0;
    viewerMatrix[11] = 0;
    viewerMatrix[15] = 1;
    
    // convert from our coordinate system (looking down X)
    // to OpenGL's coordinate system (looking down -Z)
    myGlMultMatrix( viewerMatrix, s_flipMatrix, tr.orientation.modelMatrix );
    
    tr.viewParms.world = tr.orientation;
}

/*
** idRenderSystemMainLocal::SetFarClip
*/
void idRenderSystemMainLocal::SetFarClip( void )
{
    S32	i;
    F32	farthestCornerDistance = 0;
    
    // if not rendering the world (icons, menus, etc)
    // set a 2k far clip plane
    if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
    {
        tr.viewParms.zFar = 2048;
        return;
    }
    
    //
    // set far clipping planes dynamically
    //
    farthestCornerDistance = 0;
    
    for ( i = 0; i < 8; i++ )
    {
        vec3_t v, vecTo;
        F32 distance;
        
        if ( i & 1 )
        {
            v[0] = tr.viewParms.visBounds[0][0];
        }
        else
        {
            v[0] = tr.viewParms.visBounds[1][0];
        }
        
        if ( i & 2 )
        {
            v[1] = tr.viewParms.visBounds[0][1];
        }
        else
        {
            v[1] = tr.viewParms.visBounds[1][1];
        }
        
        if ( i & 4 )
        {
            v[2] = tr.viewParms.visBounds[0][2];
        }
        else
        {
            v[2] = tr.viewParms.visBounds[1][2];
        }
        
        VectorSubtract( v, tr.viewParms.orientation.origin, vecTo );
        
        distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];
        
        if ( distance > farthestCornerDistance )
        {
            farthestCornerDistance = distance;
        }
    }
    
    tr.viewParms.zFar = sqrt( farthestCornerDistance );
}

/*
=================
idRenderSystemMainLocal::SetupFrustum

Set up the culling frustum planes for the current view using the results we got from computing the first two rows of
the projection matrix.
=================
*/
void idRenderSystemMainLocal::SetupFrustum( viewParms_t* dest, F32 xmin, F32 xmax, F32 ymax, F32 zProj, F32 zFar, F32 stereoSep )
{
    S32 i;
    F32 oppleg, adjleg, length;
    vec3_t ofsorigin;
    
    if ( stereoSep == 0 && xmin == -xmax )
    {
        // symmetric case can be simplified
        VectorCopy( dest->orientation.origin, ofsorigin );
        
        length = sqrt( xmax * xmax + zProj * zProj );
        oppleg = xmax / length;
        adjleg = zProj / length;
        
        VectorScale( dest->orientation.axis[0], oppleg, dest->frustum[0].normal );
        VectorMA( dest->frustum[0].normal, adjleg, dest->orientation.axis[1], dest->frustum[0].normal );
        
        VectorScale( dest->orientation.axis[0], oppleg, dest->frustum[1].normal );
        VectorMA( dest->frustum[1].normal, -adjleg, dest->orientation.axis[1], dest->frustum[1].normal );
    }
    else
    {
        // In stereo rendering, due to the modification of the projection matrix, dest->orientation.origin is not the
        // actual origin that we're rendering so offset the tip of the view pyramid.
        VectorMA( dest->orientation.origin, stereoSep, dest->orientation.axis[1], ofsorigin );
        
        oppleg = xmax + stereoSep;
        length = sqrt( oppleg * oppleg + zProj * zProj );
        VectorScale( dest->orientation.axis[0], oppleg / length, dest->frustum[0].normal );
        VectorMA( dest->frustum[0].normal, zProj / length, dest->orientation.axis[1], dest->frustum[0].normal );
        
        oppleg = xmin + stereoSep;
        length = sqrt( oppleg * oppleg + zProj * zProj );
        VectorScale( dest->orientation.axis[0], -oppleg / length, dest->frustum[1].normal );
        VectorMA( dest->frustum[1].normal, -zProj / length, dest->orientation.axis[1], dest->frustum[1].normal );
    }
    
    length = sqrt( ymax * ymax + zProj * zProj );
    oppleg = ymax / length;
    adjleg = zProj / length;
    
    VectorScale( dest->orientation.axis[0], oppleg, dest->frustum[2].normal );
    VectorMA( dest->frustum[2].normal, adjleg, dest->orientation.axis[2], dest->frustum[2].normal );
    
    VectorScale( dest->orientation.axis[0], oppleg, dest->frustum[3].normal );
    VectorMA( dest->frustum[3].normal, -adjleg, dest->orientation.axis[2], dest->frustum[3].normal );
    
    for ( i = 0 ; i < 4 ; i++ )
    {
        dest->frustum[i].type = PLANE_NON_AXIAL;
        dest->frustum[i].dist = DotProduct( ofsorigin, dest->frustum[i].normal );
        SetPlaneSignbits( &dest->frustum[i] );
    }
    
    if ( zFar != 0.0f )
    {
        vec3_t farpoint;
        
        VectorMA( ofsorigin, zFar, dest->orientation.axis[0], farpoint );
        VectorScale( dest->orientation.axis[0], -1.0f, dest->frustum[4].normal );
        
        dest->frustum[4].type = PLANE_NON_AXIAL;
        dest->frustum[4].dist = DotProduct( farpoint, dest->frustum[4].normal );
        SetPlaneSignbits( &dest->frustum[4] );
        dest->flags |= VPF_FARPLANEFRUSTUM;
    }
}

/*
===============
idRenderSystemMainLocal::SetupProjection
===============
*/
void idRenderSystemMainLocal::SetupProjection( viewParms_t* dest, F32 zProj, F32 zFar, bool computeFrustum )
{
    F32	xmin, xmax, ymin, ymax, width, height, stereoSep = r_stereoSeparation->value;
    
    /*
     * offset the view origin of the viewer for stereo rendering
     * by setting the projection matrix appropriately.
     */
    
    if ( stereoSep != 0 )
    {
        if ( dest->stereoFrame == STEREO_LEFT )
        {
            stereoSep = zProj / stereoSep;
        }
        else if ( dest->stereoFrame == STEREO_RIGHT )
        {
            stereoSep = zProj / -stereoSep;
        }
        else
        {
            stereoSep = 0;
        }
    }
    
    ymax = zProj * tan( dest->fovY * M_PI / 360.0f );
    ymin = -ymax;
    
    xmax = zProj * tan( dest->fovX * M_PI / 360.0f );
    xmin = -xmax;
    
    width = xmax - xmin;
    height = ymax - ymin;
    
    dest->projectionMatrix[0] = 2 * zProj / width;
    dest->projectionMatrix[4] = 0;
    dest->projectionMatrix[8] = ( xmax + xmin + 2 * stereoSep ) / width;
    dest->projectionMatrix[12] = 2 * zProj * stereoSep / width;
    
    dest->projectionMatrix[1] = 0;
    dest->projectionMatrix[5] = 2 * zProj / height;
    // normally 0
    dest->projectionMatrix[9] = ( ymax + ymin ) / height;
    dest->projectionMatrix[13] = 0;
    
    dest->projectionMatrix[3] = 0;
    dest->projectionMatrix[7] = 0;
    dest->projectionMatrix[11] = -1;
    dest->projectionMatrix[15] = 0;
    
    // Now that we have all the data for the projection matrix we can also setup the view frustum.
    if ( computeFrustum )
    {
        SetupFrustum( dest, xmin, xmax, ymax, zProj, zFar, stereoSep );
    }
}

/*
===============
idRenderSystemMainLocal::SetupProjectionZ

Sets the z-component transformation part in the projection matrix
===============
*/
void idRenderSystemMainLocal::SetupProjectionZ( viewParms_t* dest )
{
    F32 zNear, zFar, depth;
    
    zNear = dest->zNear;
    zFar = dest->zFar;
    
    depth = zFar - zNear;
    
    dest->projectionMatrix[2] = 0;
    dest->projectionMatrix[6] = 0;
    dest->projectionMatrix[10] = -( zFar + zNear ) / depth;
    dest->projectionMatrix[14] = -2 * zFar * zNear / depth;
    
    if ( dest->isPortal )
    {
        F32	plane[4], plane2[4];
        vec4_t q, c;
        
        // transform portal plane into camera space
        plane[0] = dest->portalPlane.normal[0];
        plane[1] = dest->portalPlane.normal[1];
        plane[2] = dest->portalPlane.normal[2];
        plane[3] = dest->portalPlane.dist;
        
        plane2[0] = -DotProduct( dest->orientation.axis[1], plane );
        plane2[1] = DotProduct( dest->orientation.axis[2], plane );
        plane2[2] = -DotProduct( dest->orientation.axis[0], plane );
        plane2[3] = DotProduct( plane, dest->orientation.origin ) - plane[3];
        
        // Lengyel, Eric. "Modifying the Projection Matrix to Perform Oblique Near-plane Clipping".
        // Terathon Software 3D Graphics Library, 2004. http://www.terathon.com/code/oblique.html
        q[0] = ( SGN( plane2[0] ) + dest->projectionMatrix[8] ) / dest->projectionMatrix[0];
        q[1] = ( SGN( plane2[1] ) + dest->projectionMatrix[9] ) / dest->projectionMatrix[5];
        q[2] = -1.0f;
        q[3] = ( 1.0f + dest->projectionMatrix[10] ) / dest->projectionMatrix[14];
        
        VectorScale4( plane2, 2.0f / DotProduct4( plane2, q ), c );
        
        dest->projectionMatrix[2]  = c[0];
        dest->projectionMatrix[6]  = c[1];
        dest->projectionMatrix[10] = c[2] + 1.0f;
        dest->projectionMatrix[14] = c[3];
    }
}

/*
===============
idRenderSystemMainLocal::SetupProjectionOrtho
===============
*/
void idRenderSystemMainLocal::SetupProjectionOrtho( viewParms_t* dest, vec3_t viewBounds[2] )
{
    S32 i;
    F32 xmin, xmax, ymin, ymax, znear, zfar;
    vec3_t pop;
    
    // Quake3:   Projection:
    //
    //    Z  X   Y  Z
    //    | /    | /
    //    |/     |/
    // Y--+      +--X
    
    xmin  =  viewBounds[0][1];
    xmax  =  viewBounds[1][1];
    ymin  = -viewBounds[1][2];
    ymax  = -viewBounds[0][2];
    znear =  viewBounds[0][0];
    zfar  =  viewBounds[1][0];
    
    dest->projectionMatrix[0]  = 2 / ( xmax - xmin );
    dest->projectionMatrix[4]  = 0;
    dest->projectionMatrix[8]  = 0;
    dest->projectionMatrix[12] = ( xmax + xmin ) / ( xmax - xmin );
    
    dest->projectionMatrix[1]  = 0;
    dest->projectionMatrix[5]  = 2 / ( ymax - ymin );
    dest->projectionMatrix[9]  = 0;
    dest->projectionMatrix[13] = ( ymax + ymin ) / ( ymax - ymin );
    
    dest->projectionMatrix[2]  = 0;
    dest->projectionMatrix[6]  = 0;
    dest->projectionMatrix[10] = -2 / ( zfar - znear );
    dest->projectionMatrix[14] = -( zfar + znear ) / ( zfar - znear );
    
    dest->projectionMatrix[3]  = 0;
    dest->projectionMatrix[7]  = 0;
    dest->projectionMatrix[11] = 0;
    dest->projectionMatrix[15] = 1;
    
    VectorScale( dest->orientation.axis[1],  1.0f, dest->frustum[0].normal );
    VectorMA( dest->orientation.origin, viewBounds[0][1], dest->frustum[0].normal, pop );
    dest->frustum[0].dist = DotProduct( pop, dest->frustum[0].normal );
    
    VectorScale( dest->orientation.axis[1], -1.0f, dest->frustum[1].normal );
    VectorMA( dest->orientation.origin, -viewBounds[1][1], dest->frustum[1].normal, pop );
    dest->frustum[1].dist = DotProduct( pop, dest->frustum[1].normal );
    
    VectorScale( dest->orientation.axis[2],  1.0f, dest->frustum[2].normal );
    VectorMA( dest->orientation.origin, viewBounds[0][2], dest->frustum[2].normal, pop );
    dest->frustum[2].dist = DotProduct( pop, dest->frustum[2].normal );
    
    VectorScale( dest->orientation.axis[2], -1.0f, dest->frustum[3].normal );
    VectorMA( dest->orientation.origin, -viewBounds[1][2], dest->frustum[3].normal, pop );
    dest->frustum[3].dist = DotProduct( pop, dest->frustum[3].normal );
    
    VectorScale( dest->orientation.axis[0], -1.0f, dest->frustum[4].normal );
    VectorMA( dest->orientation.origin, -viewBounds[1][0], dest->frustum[4].normal, pop );
    dest->frustum[4].dist = DotProduct( pop, dest->frustum[4].normal );
    
    for ( i = 0; i < 5; i++ )
    {
        dest->frustum[i].type = PLANE_NON_AXIAL;
        SetPlaneSignbits( &dest->frustum[i] );
    }
    
    dest->flags |= VPF_FARPLANEFRUSTUM;
}

/*
=================
idRenderSystemMainLocal::MirrorPoint
=================
*/
void idRenderSystemMainLocal::MirrorPoint( vec3_t in, orientation_t* surface, orientation_t* camera, vec3_t out )
{
    S32	i;
    F32	d;
    vec3_t local, transformed;
    
    VectorSubtract( in, surface->origin, local );
    
    VectorClear( transformed );
    
    for ( i = 0 ; i < 3 ; i++ )
    {
        d = DotProduct( local, surface->axis[i] );
        VectorMA( transformed, d, camera->axis[i], transformed );
    }
    
    VectorAdd( transformed, camera->origin, out );
}

void idRenderSystemMainLocal::MirrorVector( vec3_t in, orientation_t* surface, orientation_t* camera, vec3_t out )
{
    S32	i;
    F32	d;
    
    VectorClear( out );
    for ( i = 0 ; i < 3 ; i++ )
    {
        d = DotProduct( in, surface->axis[i] );
        VectorMA( out, d, camera->axis[i], out );
    }
}

/*
=============
R_PlaneForSurface
=============
*/
void idRenderSystemMainLocal::PlaneForSurface( surfaceType_t* surfType, cplane_t* plane )
{
    srfBspSurface_t* tri;
    srfPoly_t* poly;
    srfVert_t* v1, *v2, *v3;
    vec4_t plane4;
    
    if ( !surfType )
    {
        ::memset( plane, 0, sizeof( *plane ) );
        plane->normal[0] = 1;
        return;
    }
    
    switch ( *surfType )
    {
        case SF_FACE:
            *plane = ( ( srfBspSurface_t* )surfType )->cullPlane;
            return;
        case SF_TRIANGLES:
            tri = ( srfBspSurface_t* )surfType;
            v1 = tri->verts + tri->indexes[0];
            v2 = tri->verts + tri->indexes[1];
            v3 = tri->verts + tri->indexes[2];
            PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz, true );
            VectorCopy( plane4, plane->normal );
            plane->dist = plane4[3];
            return;
        case SF_POLY:
            poly = ( srfPoly_t* )surfType;
            PlaneFromPoints( plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz, true );
            VectorCopy( plane4, plane->normal );
            plane->dist = plane4[3];
            return;
        default:
            ::memset( plane, 0, sizeof( *plane ) );
            plane->normal[0] = 1;
            return;
    }
}

/*
=================
idRenderSystemMainLocal::GetPortalOrientation

entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns true if it should be mirrored
=================
*/
bool idRenderSystemMainLocal::GetPortalOrientations( drawSurf_t* drawSurf, S32 entityNum, orientation_t* surface, orientation_t* camera, vec3_t pvsOrigin, bool* mirror )
{
    S32 i;
    F32	d;
    cplane_t originalPlane, plane;
    trRefEntity_t* e;
    vec3_t transformed;
    
    // create plane axis for the portal we are seeing
    PlaneForSurface( drawSurf->surface, &originalPlane );
    
    // rotate the plane if necessary
    if ( entityNum != REFENTITYNUM_WORLD )
    {
        tr.currentEntityNum = entityNum;
        tr.currentEntity = &tr.refdef.entities[entityNum];
        
        // get the orientation of the entity
        idRenderSystemMainLocal::RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orientation );
        
        // rotate the plane, but keep the non-rotated version for matching
        // against the portalSurface entities
        LocalNormalToWorld( originalPlane.normal, plane.normal );
        
        plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orientation.origin );
        
        // translate the original plane
        originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orientation.origin );
    }
    else
    {
        plane = originalPlane;
    }
    
    VectorCopy( plane.normal, surface->axis[0] );
    PerpendicularVector( surface->axis[1], surface->axis[0] );
    CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );
    
    // locate the portal entity closest to this plane.
    // origin will be the origin of the portal, origin2 will be
    // the origin of the camera
    for ( i = 0 ; i < tr.refdef.num_entities ; i++ )
    {
        e = &tr.refdef.entities[i];
        
        if ( e->e.reType != RT_PORTALSURFACE )
        {
            continue;
        }
        
        d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
        if ( d > 64 || d < -64 )
        {
            continue;
        }
        
        // get the pvsOrigin from the entity
        VectorCopy( e->e.oldorigin, pvsOrigin );
        
        // if the entity is just a mirror, don't use as a camera point
        if ( e->e.oldorigin[0] == e->e.origin[0] && e->e.oldorigin[1] == e->e.origin[1] && e->e.oldorigin[2] == e->e.origin[2] )
        {
            VectorScale( plane.normal, plane.dist, surface->origin );
            VectorCopy( surface->origin, camera->origin );
            VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
            VectorCopy( surface->axis[1], camera->axis[1] );
            VectorCopy( surface->axis[2], camera->axis[2] );
            
            *mirror = true;
            return true;
        }
        
        // project the origin onto the surface plane to get
        // an origin point we can rotate around
        d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
        VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );
        
        // now get the camera origin and orientation
        VectorCopy( e->e.oldorigin, camera->origin );
        AxisCopy( e->e.axis, camera->axis );
        VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
        VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );
        
        // optionally rotate
        if ( e->e.oldframe )
        {
            // if a speed is specified
            if ( e->e.frame )
            {
                // continuous rotate
                d = ( tr.refdef.time / 1000.0f ) * e->e.frame;
                VectorCopy( camera->axis[1], transformed );
                RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
                CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
            }
            else
            {
                // bobbing rotate, with skinNum being the rotation offset
                d = sin( tr.refdef.time * 0.003f );
                d = e->e.skinNum + d * 4;
                VectorCopy( camera->axis[1], transformed );
                RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
                CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
            }
        }
        else if ( e->e.skinNum )
        {
            d = ( F32 )e->e.skinNum;
            VectorCopy( camera->axis[1], transformed );
            RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
            CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
        }
        *mirror = false;
        return true;
    }
    
    // if we didn't locate a portal entity, don't render anything.
    // We don't want to just treat it as a mirror, because without a
    // portal entity the server won't have communicated a proper entity set
    // in the snapshot
    
    // unfortunately, with local movement prediction it is easily possible
    // to see a surface before the server has communicated the matching
    // portal surface entity, so we don't want to print anything here...
    
    //clientMainSystem->RefPrintf( PRINT_ALL, "Portal surface without a portal entity\n" );
    
    return false;
}

bool idRenderSystemMainLocal::IsMirror( const drawSurf_t* drawSurf, S32 entityNum )
{
    S32	i;
    F32	d;
    cplane_t originalPlane, plane;
    trRefEntity_t* e;
    
    // create plane axis for the portal we are seeing
    PlaneForSurface( drawSurf->surface, &originalPlane );
    
    // rotate the plane if necessary
    if ( entityNum != REFENTITYNUM_WORLD )
    {
        tr.currentEntityNum = entityNum;
        tr.currentEntity = &tr.refdef.entities[entityNum];
        
        // get the orientation of the entity
        idRenderSystemMainLocal::RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orientation );
        
        // rotate the plane, but keep the non-rotated version for matching
        // against the portalSurface entities
        LocalNormalToWorld( originalPlane.normal, plane.normal );
        plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orientation.origin );
        
        // translate the original plane
        originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orientation.origin );
    }
    
    // locate the portal entity closest to this plane.
    // origin will be the origin of the portal, origin2 will be
    // the origin of the camera
    for ( i = 0 ; i < tr.refdef.num_entities ; i++ )
    {
        e = &tr.refdef.entities[i];
        if ( e->e.reType != RT_PORTALSURFACE )
        {
            continue;
        }
        
        d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
        if ( d > 64 || d < -64 )
        {
            continue;
        }
        
        // if the entity is just a mirror, don't use as a camera point
        if ( e->e.oldorigin[0] == e->e.origin[0] && e->e.oldorigin[1] == e->e.origin[1] && e->e.oldorigin[2] == e->e.origin[2] )
        {
            return true;
        }
        
        return false;
    }
    
    return false;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
bool idRenderSystemMainLocal::SurfIsOffscreen( const drawSurf_t* drawSurf, vec4_t clipDest[128] )
{
    S32 entityNum, numTriangles, fogNum, pshadowed, dlighted, i;
    U32 pointOr = 0, pointAnd = ( U32 )~0;
    F32 shortest = 100000000;
    shader_t* shader;
    vec4_t clip, eye;
    
    RotateForViewer();
    
    DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed );
    idRenderSystemShadeLocal::BeginSurface( shader, fogNum, drawSurf->cubemapIndex );
    rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
    
    assert( tess.numVertexes < 128 );
    
    for ( i = 0; i < tess.numVertexes; i++ )
    {
        S32 j;
        U32 pointFlags = 0;
        
        TransformModelToClip( tess.xyz[i], tr.orientation.modelMatrix, tr.viewParms.projectionMatrix, eye, clip );
        
        for ( j = 0; j < 3; j++ )
        {
            if ( clip[j] >= clip[3] )
            {
                pointFlags |= ( 1 << ( j * 2 ) );
            }
            else if ( clip[j] <= -clip[3] )
            {
                pointFlags |= ( 1 << ( j * 2 + 1 ) );
            }
        }
        
        pointAnd &= pointFlags;
        pointOr |= pointFlags;
    }
    
    // trivially reject
    if ( pointAnd )
    {
        return true;
    }
    
    // determine if this surface is backfaced and also determine the distance
    // to the nearest vertex so we can cull based on portal range.  Culling
    // based on vertex distance isn't 100% correct (we should be checking for
    // range to the surface), but it's good enough for the types of portals
    // we have in the game right now.
    numTriangles = tess.numIndexes / 3;
    
    for ( i = 0; i < tess.numIndexes; i += 3 )
    {
        vec3_t normal, tNormal;
        
        F32 len;
        
        VectorSubtract( tess.xyz[tess.indexes[i]], tr.viewParms.orientation.origin, normal );
        
        // lose the sqrt
        len = VectorLengthSquared( normal );
        if ( len < shortest )
        {
            shortest = len;
        }
        
        idRenderSystemVaoLocal::VaoUnpackNormal( tNormal, tess.normal[tess.indexes[i]] );
        
        if ( DotProduct( normal, tNormal ) >= 0 )
        {
            numTriangles--;
        }
    }
    
    if ( !numTriangles )
    {
        return true;
    }
    
    // mirrors can early out at this point, since we don't do a fade over distance
    // with them (although we could)
    if ( IsMirror( drawSurf, entityNum ) )
    {
        return false;
    }
    
    if ( shortest > ( tess.shader->portalRange * tess.shader->portalRange ) )
    {
        return true;
    }
    
    return false;
}

/*
========================
idRenderSystemMainLocal::MirrorViewBySurface

Returns true if another view has been rendered
========================
*/
bool idRenderSystemMainLocal::MirrorViewBySurface( drawSurf_t* drawSurf, S32 entityNum )
{
    vec4_t clipDest[128];
    viewParms_t	newParms, oldParms;
    orientation_t surface, camera;
    
    // don't recursively mirror
    if ( tr.viewParms.isPortal )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
        return false;
    }
    
    if ( r_noportals->integer || ( r_fastsky->integer == 1 ) )
    {
        return false;
    }
    
    // trivially reject portal/mirror
    if ( SurfIsOffscreen( drawSurf, clipDest ) )
    {
        return false;
    }
    
    // save old viewParms so we can return to it after the mirror view
    oldParms = tr.viewParms;
    
    newParms = tr.viewParms;
    newParms.isPortal = true;
    newParms.zFar = 0.0f;
    newParms.zNear = r_znear->value;
    newParms.flags &= ~VPF_FARPLANEFRUSTUM;
    
    if ( !GetPortalOrientations( drawSurf, entityNum, &surface, &camera, newParms.pvsOrigin, &newParms.isMirror ) )
    {
        // bad portal, no portalentity
        return false;
    }
    
    // Never draw viewmodels in portal or mirror views.
    newParms.flags |= VPF_NOVIEWMODEL;
    
    MirrorPoint( oldParms.orientation.origin, &surface, &camera, newParms.orientation.origin );
    
    VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
    newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );
    
    MirrorVector( oldParms.orientation.axis[0], &surface, &camera, newParms.orientation.axis[0] );
    MirrorVector( oldParms.orientation.axis[1], &surface, &camera, newParms.orientation.axis[1] );
    MirrorVector( oldParms.orientation.axis[2], &surface, &camera, newParms.orientation.axis[2] );
    
    // OPTIMIZE: restrict the viewport on the mirrored view
    
    // render the mirror view
    RenderView( &newParms );
    
    tr.viewParms = oldParms;
    
    return true;
}

/*
=================
idRenderSystemMainLocal::SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
S32 idRenderSystemMainLocal::SpriteFogNum( trRefEntity_t* ent )
{
    S32 i, j;
    F32 radius;
    fog_t* fog;
    
    if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
    {
        return 0;
    }
    
    if ( ent->e.renderfx & RF_CROSSHAIR )
    {
        return 0;
    }
    
    radius = ent->e.radius;
    
    for ( i = 1; i < tr.world->numfogs; i++ )
    {
        fog = &tr.world->fogs[i];
        
        for ( j = 0; j < 3; j++ )
        {
            if ( ent->e.origin[j] - radius >= fog->bounds[1][j] || ent->e.origin[j] + radius <= fog->bounds[0][j] )
            {
                break;
            }
        }
        
        if ( j == 3 )
        {
            return i;
        }
    }
    
    return 0;
}

/*
==========================================================================================
DRAWSURF SORTING
==========================================================================================
*/

/*
===============
idRenderSystemMainLocal::Radix
===============
*/
ID_INLINE void idRenderSystemMainLocal::Radix( S32 byte, S32 size, drawSurf_t* source, drawSurf_t* dest )
{
    S32 count[ 256 ] = { 0 }, index[ 256 ], i;
    U8* sortKey = nullptr, * end = nullptr;
    
    sortKey = ( ( U8* )&source[ 0 ].sort ) + byte;
    end = sortKey + ( size * sizeof( drawSurf_t ) );
    
    for ( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    {
        ++count[*sortKey];
    }
    
    index[ 0 ] = 0;
    
    for ( i = 1; i < 256; ++i )
    {
        index[i] = index[i - 1] + count[i - 1];
    }
    
    sortKey = ( ( U8* )&source[ 0 ].sort ) + byte;
    
    for ( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    {
        dest[index[*sortKey]++] = source[i];
    }
}

/*
===============
idRenderSystemMainLocal::RadixSort

Radix sort with 4 byte size buckets
===============
*/
void idRenderSystemMainLocal::RadixSort( drawSurf_t* source, S32 size )
{
    static drawSurf_t scratch[ MAX_DRAWSURFS ];
    
#ifdef Q3_LITTLE_ENDIAN
    Radix( 0, size, source, scratch );
    Radix( 1, size, scratch, source );
    Radix( 2, size, source, scratch );
    Radix( 3, size, scratch, source );
#else
    Radix( 3, size, source, scratch );
    Radix( 2, size, scratch, source );
    Radix( 1, size, source, scratch );
    Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
}

/*
=================
idRenderSystemMainLocal::AddDrawSurf
=================
*/
void idRenderSystemMainLocal::AddDrawSurf( surfaceType_t* surface, shader_t* shader, S32 fogIndex, S32 dlightMap, S32 pshadowMap, S32 cubemap )
{
    S32	index;
    
    // instead of checking for overflow, we just mask the index
    // so it wraps around
    index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
    
    // the sort data is packed into a single 32 bit value so it can be
    // compared quickly during the qsorting process
    tr.refdef.drawSurfs[index].sort = ( shader->sortedIndex << QSORT_SHADERNUM_SHIFT )
                                      | tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT )
                                      | ( ( S32 )pshadowMap << QSORT_PSHADOW_SHIFT ) | ( S32 )dlightMap;
    tr.refdef.drawSurfs[index].cubemapIndex = cubemap;
    tr.refdef.drawSurfs[index].surface = surface;
    tr.refdef.numDrawSurfs++;
}

/*
=================
idRenderSystemMainLocal::DecomposeSort
=================
*/
void idRenderSystemMainLocal::DecomposeSort( U32 sort, S32* entityNum, shader_t** shader, S32* fogNum, S32* dlightMap, S32* pshadowMap )
{
    *fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
    *shader = tr.sortedShaders[( sort >> QSORT_SHADERNUM_SHIFT ) & ( MAX_SHADERS - 1 ) ];
    *entityNum = ( sort >> QSORT_REFENTITYNUM_SHIFT ) & REFENTITYNUM_MASK;
    *pshadowMap = ( sort >> QSORT_PSHADOW_SHIFT ) & 1;
    *dlightMap = sort & 1;
}

/*
=================
idRenderSystemMainLocal::SortDrawSurfs
=================
*/
void idRenderSystemMainLocal::SortDrawSurfs( drawSurf_t* drawSurfs, S32 numDrawSurfs )
{
    S32 fogNum, entityNum, dlighted, pshadowed, i;
    shader_t* shader;
    
    //clientMainSystem->RefPrintf(PRINT_ALL, "firstDrawSurf %d numDrawSurfs %d\n", (S32)(drawSurfs - tr.refdef.drawSurfs), numDrawSurfs);
    
    // it is possible for some views to not have any surfaces
    if ( numDrawSurfs < 1 )
    {
        // we still need to add it for hyperspace cases
        idRenderSystemCmdsLocal::AddDrawSurfCmd( drawSurfs, numDrawSurfs );
        return;
    }
    
    // sort the drawsurfs by sort type, then orientation, then shader
    RadixSort( drawSurfs, numDrawSurfs );
    
    // skip pass through drawing if rendering a shadow map
    if ( tr.viewParms.flags & ( VPF_SHADOWMAP | VPF_DEPTHSHADOW ) )
    {
        idRenderSystemCmdsLocal::AddDrawSurfCmd( drawSurfs, numDrawSurfs );
        return;
    }
    
    // check for any pass through drawing, which
    // may cause another view to be rendered first
    for ( i = 0 ; i < numDrawSurfs ; i++ )
    {
        DecomposeSort( ( drawSurfs + i )->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed );
        
        if ( shader->sort > SS_PORTAL )
        {
            break;
        }
        
        // no shader should ever have this sort type
        if ( shader->sort == SS_BAD )
        {
            Com_Error( ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
        }
        
        // if the mirror was completely clipped away, we may need to check another surface
        if ( MirrorViewBySurface( ( drawSurfs + i ), entityNum ) )
        {
            // this is a debug option to see exactly what is being mirrored
            if ( r_portalOnly->integer )
            {
                return;
            }
            
            // only one mirror view at a time
            break;
        }
    }
    
    idRenderSystemCmdsLocal::AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

void idRenderSystemMainLocal::AddEntitySurface( S32 entityNum )
{
    trRefEntity_t* ent;
    shader_t* shader;
    
    tr.currentEntityNum = entityNum;
    
    ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];
    
    ent->needDlights = false;
    
    // preshift the value we are going to OR into the drawsurf sort
    tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;
    
    //
    // the weapon model must be handled special --
    // we don't want the hacked weapon position showing in
    // mirrors, because the true body position will already be drawn
    //
    if ( ( ent->e.renderfx & RF_FIRST_PERSON ) && ( tr.viewParms.flags & VPF_NOVIEWMODEL ) )
    {
        return;
    }
    
    // simple generated models, like sprites and beams, are not culled
    switch ( ent->e.reType )
    {
        case RT_PORTALSURFACE:
            break;		// don't draw anything
        case RT_SPRITE:
        case RT_BEAM:
        case RT_LIGHTNING:
        case RT_RAIL_CORE:
        case RT_RAIL_RINGS:
            // self blood sprites, talk balloons, etc should not be drawn in the primary
            // view.  We can't just do this check for all entities, because md3
            // entities may still want to cast shadows from them
            if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal )
            {
                return;
            }
            
            shader = idRenderSystemShaderLocal::GetShaderByHandle( ent->e.customShader );
            
            AddDrawSurf( &entitySurface, shader, SpriteFogNum( ent ), 0, 0, 0 /*cubeMap*/ );
            
            break;
            
        case RT_MODEL:
            // we must set up parts of tr.or for model culling
            idRenderSystemMainLocal::RotateForEntity( ent, &tr.viewParms, &tr.orientation );
            
            tr.currentModel = idRenderSystemModelLocal::GetModelByHandle( ent->e.hModel );
            if ( !tr.currentModel )
            {
                AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0, 0, 0 /*cubeMap*/ );
            }
            else
            {
                switch ( tr.currentModel->type )
                {
                    case MOD_MESH:
                        idRenderSystemMeshLocal::AddMD3Surfaces( ent );
                        break;
                    case MOD_MDR:
                        idRenderSystemAnimationLocal::MDRAddAnimSurfaces( ent );
                        break;
                    case MOD_IQM:
                        idRenderSystelModelIQMLocal::AddIQMSurfaces( ent );
                        break;
                    case MOD_BRUSH:
                        idRenderSystemWorldLocal::AddBrushModelSurfaces( ent );
                        break;
                        // null model axis
                    case MOD_BAD:
                        if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal )
                        {
                            break;
                        }
                        
                        AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0, 0, 0 );
                        break;
                    default:
                        Com_Error( ERR_DROP, "idRenderSystemMainLocal::AddEntitySurfaces: Bad modeltype" );
                        break;
                }
            }
            break;
        default:
            Com_Error( ERR_DROP, "idRenderSystemLocal::AddRefEntityToScene: bad reType %i", ent->e.reType );
    }
}

/*
=============
idRenderSystemMainLocal::AddEntitySurfaces
=============
*/
void idRenderSystemMainLocal::AddEntitySurfaces( void )
{
    S32 i;
    
    if ( !r_drawentities->integer )
    {
        return;
    }
    
    for ( i = 0; i < tr.refdef.num_entities; i++ )
    {
        AddEntitySurface( i );
    }
}

/*
====================
idRenderSystemMainLocal::GenerateDrawSurfs
====================
*/
void idRenderSystemMainLocal::GenerateDrawSurfs( void )
{
    idRenderSystemWorldLocal::AddWorldSurfaces();
    idRenderSystemSceneLocal::AddPolygonSurfaces();
    
    // set the projection matrix with the minimum zfar
    // now that we have the world bounded
    // this needs to be done before entities are
    // added, because they use the projection
    // matrix for lod calculation
    
    // dynamically compute far clip plane distance
    if ( !( tr.viewParms.flags & VPF_SHADOWMAP ) && !( tr.viewParms.flags & VPF_DEPTHSHADOW ) )
    {
        SetFarClip();
    }
    
    // we know the size of the clipping volume. Now set the rest of the projection matrix.
    SetupProjectionZ( &tr.viewParms );
    
    AddEntitySurfaces();
}

/*
================
idRenderSystemMainLocal::DebugPolygon
================
*/
void idRenderSystemMainLocal::DebugPolygon( S32 color, S32 numPoints, F32* points )
{
    // FIXME: implement this
#if 0
    S32		i;
    
    idRenderSystemBackendLocal::State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
    
    // draw solid shade
    
    qglColor3f( color & 1, ( color >> 1 ) & 1, ( color >> 2 ) & 1 );
    qglBegin( GL_POLYGON );
    for ( i = 0 ; i < numPoints ; i++ )
    {
        glVertex3fv( points + i * 3 );
    }
    qglEnd();
    
    // draw wireframe outline
    idRenderSystemBackendLocal::State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
    qglDepthRange( 0, 0 );
    qglColor3f( 1, 1, 1 );
    qglBegin( GL_POLYGON );
    for ( i = 0 ; i < numPoints ; i++ )
    {
        qglVertex3fv( points + i * 3 );
    }
    qglEnd();
    qglDepthRange( 0, 1 );
#endif
}

/*
====================
idRenderSystemMainLocal::DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void idRenderSystemMainLocal::DebugGraphics( void )
{
    if ( !r_debugSurface->integer )
    {
        return;
    }
    
    idRenderSystemCmdsLocal::IssuePendingRenderCommands();
    
    idRenderSystemBackendLocal::BindToTMU( tr.whiteImage, TB_COLORMAP );
    idRenderSystemBackendLocal::Cull( CT_FRONT_SIDED );
    
    collisionModelManager->DrawDebugSurface( DebugPolygon );
}

/*
================
idRenderSystemMainLocal::RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void idRenderSystemMainLocal::RenderView( viewParms_t* parms )
{
    S32	firstDrawSurf, numDrawSurfs;
    static int lastTime;
    
    if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 )
    {
        return;
    }
    
    tr.viewCount++;
    
    tr.viewParms = *parms;
    tr.viewParms.frameSceneNum = tr.frameSceneNum;
    tr.viewParms.frameCount = tr.frameCount;
    
    firstDrawSurf = tr.refdef.numDrawSurfs;
    
    tr.viewCount++;
    
    // set viewParms.world
    RotateForViewer();
    
    SetupProjection( &tr.viewParms, tr.viewParms.zNear, tr.viewParms.zFar, true );
    
    GenerateDrawSurfs();
    
    // if we overflowed MAX_DRAWSURFS, the drawsurfs
    // wrapped around in the buffer and we will be missing
    // the first surfaces, not the last ones
    numDrawSurfs = tr.refdef.numDrawSurfs;
    
    if ( numDrawSurfs > MAX_DRAWSURFS )
    {
        numDrawSurfs = MAX_DRAWSURFS;
    }
    
    SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, numDrawSurfs - firstDrawSurf );
    
    // draw main system development information (surface outlines, etc)
    DebugGraphics();
}

void idRenderSystemMainLocal::RenderDlightCubemaps( const refdef_t* fd )
{
    S32 i;
    U32 bufferDlightMask = tr.refdef.dlightMask;
    
    for ( i = 0; i < tr.refdef.num_dlights; i++ )
    {
        S32 j;
        viewParms_t	shadowParms;
        
        // use previous frame to determine visible dlights
        if ( ( 1 << i ) & bufferDlightMask )
        {
            continue;
        }
        
        ::memset( &shadowParms, 0, sizeof( shadowParms ) );
        
        shadowParms.viewportX = 0;
        shadowParms.viewportY = 0;
        shadowParms.viewportWidth = PSHADOW_MAP_SIZE;
        shadowParms.viewportHeight = PSHADOW_MAP_SIZE;
        shadowParms.isPortal = false;
        shadowParms.isMirror = true; // because it is
        
        shadowParms.fovX = 90;
        shadowParms.fovY = 90;
        
        shadowParms.flags = VPF_SHADOWMAP | VPF_DEPTHSHADOW | VPF_NOVIEWMODEL;
        shadowParms.zFar = tr.refdef.dlights[i].radius;
        shadowParms.zNear = 1.0f;
        
        VectorCopy( tr.refdef.dlights[i].origin, shadowParms.orientation.origin );
        
        for ( j = 0; j < 6; j++ )
        {
            switch ( j )
            {
                case 0:
                    // -X
                    VectorSet( tr.viewParms.orientation.axis[0], -1,  0,  0 );
                    VectorSet( tr.viewParms.orientation.axis[1],  0,  0, -1 );
                    VectorSet( tr.viewParms.orientation.axis[2],  0,  1,  0 );
                    break;
                case 1:
                    // +X
                    VectorSet( tr.viewParms.orientation.axis[0],  1,  0,  0 );
                    VectorSet( tr.viewParms.orientation.axis[1],  0,  0,  1 );
                    VectorSet( tr.viewParms.orientation.axis[2],  0,  1,  0 );
                    break;
                case 2:
                    // -Y
                    VectorSet( tr.viewParms.orientation.axis[0],  0, -1,  0 );
                    VectorSet( tr.viewParms.orientation.axis[1],  1,  0,  0 );
                    VectorSet( tr.viewParms.orientation.axis[2],  0,  0, -1 );
                    break;
                case 3:
                    // +Y
                    VectorSet( tr.viewParms.orientation.axis[0],  0,  1,  0 );
                    VectorSet( tr.viewParms.orientation.axis[1],  1,  0,  0 );
                    VectorSet( tr.viewParms.orientation.axis[2],  0,  0,  1 );
                    break;
                case 4:
                    // -Z
                    VectorSet( tr.viewParms.orientation.axis[0],  0,  0, -1 );
                    VectorSet( tr.viewParms.orientation.axis[1],  1,  0,  0 );
                    VectorSet( tr.viewParms.orientation.axis[2],  0,  1,  0 );
                    break;
                case 5:
                    // +Z
                    VectorSet( tr.viewParms.orientation.axis[0],  0,  0,  1 );
                    VectorSet( tr.viewParms.orientation.axis[1], -1,  0,  0 );
                    VectorSet( tr.viewParms.orientation.axis[2],  0,  1,  0 );
                    break;
            }
            
            shadowParms.targetFbo = tr.shadowCubeFbo;
            shadowParms.cubemapSelection = tr.shadowCubemaps;
            shadowParms.targetFboLayer = j;
            shadowParms.targetFboCubemapIndex = i;
            
            RenderView( &shadowParms );
        }
    }
}

void idRenderSystemMainLocal::RenderPshadowMaps( const refdef_t* fd )
{
    S32 i;
    viewParms_t	shadowParms;
    
    // first, make a list of shadows
    for ( i = 0; i < tr.refdef.num_entities; i++ )
    {
        trRefEntity_t* ent = &tr.refdef.entities[i];
        
        if ( ( ent->e.renderfx & ( RF_FIRST_PERSON | RF_NOSHADOW ) ) )
        {
            continue;
        }
        
        //if((ent->e.renderfx & RF_THIRD_PERSON))
        //    continue;
        
        if ( ent->e.reType == RT_MODEL )
        {
            model_t* model = idRenderSystemModelLocal::GetModelByHandle( ent->e.hModel );
            pshadow_t shadow;
            F32 radius = 0.0f;
            F32 scale = 1.0f;
            vec3_t diff;
            S32 j;
            
            if ( !model )
            {
                continue;
            }
            
            if ( ent->e.nonNormalizedAxes )
            {
                scale = VectorLength( ent->e.axis[0] );
            }
            
            switch ( model->type )
            {
                case MOD_MESH:
                {
                    mdvFrame_t* frame = &model->mdv[0]->frames[ent->e.frame];
                    
                    radius = frame->radius * scale;
                }
                break;
                
                case MOD_MDR:
                {
                    // FIXME: never actually tested this
                    mdrHeader_t* header = ( mdrHeader_t* )model->modelData;
                    size_t frameSize = ( size_t )( &( ( mdrFrame_t* )0 )->bones[ header->numBones ] );
                    mdrFrame_t* frame = ( mdrFrame_t* )( ( U8* ) header + header->ofsFrames + frameSize * ent->e.frame );
                    
                    radius = frame->radius;
                }
                break;
                case MOD_IQM:
                {
                    // FIXME: never actually tested this
                    iqmData_t* data = ( iqmData_t* )model->modelData;
                    vec3_t diag;
                    F32* framebounds;
                    
                    framebounds = data->bounds + 6 * ent->e.frame;
                    VectorSubtract( framebounds + 3, framebounds, diag );
                    radius = 0.5f * VectorLength( diag );
                }
                break;
                
                default:
                    break;
            }
            
            if ( !radius )
            {
                continue;
            }
            
            // Cull entities that are behind the viewer by more than lightRadius
            VectorSubtract( ent->e.origin, fd->vieworg, diff );
            if ( DotProduct( diff, fd->viewaxis[0] ) < -r_pshadowDist->value )
            {
                continue;
            }
            
            ::memset( &shadow, 0, sizeof( shadow ) );
            
            shadow.numEntities = 1;
            shadow.entityNums[0] = i;
            shadow.viewRadius = radius;
            shadow.lightRadius = r_pshadowDist->value;
            VectorCopy( ent->e.origin, shadow.viewOrigin );
            shadow.sort = DotProduct( diff, diff ) / ( radius * radius );
            VectorCopy( ent->e.origin, shadow.entityOrigins[0] );
            shadow.entityRadiuses[0] = radius;
            
            for ( j = 0; j < MAX_CALC_PSHADOWS; j++ )
            {
                pshadow_t swap;
                
                if ( j + 1 > tr.refdef.num_pshadows )
                {
                    tr.refdef.num_pshadows = j + 1;
                    tr.refdef.pshadows[j] = shadow;
                    break;
                }
                
                // sort shadows by distance from camera divided by radius
                // FIXME: sort better
                if ( tr.refdef.pshadows[j].sort <= shadow.sort )
                {
                    continue;
                }
                
                swap = tr.refdef.pshadows[j];
                tr.refdef.pshadows[j] = shadow;
                shadow = swap;
            }
        }
    }
    
    // next, merge touching pshadows
    for ( i = 0; i < tr.refdef.num_pshadows; i++ )
    {
        S32 j;
        pshadow_t* ps1 = &tr.refdef.pshadows[i];
        
        for ( j = i + 1; j < tr.refdef.num_pshadows; j++ )
        {
            S32 k;
            pshadow_t* ps2 = &tr.refdef.pshadows[j];
            bool touch;
            
            if ( ps1->numEntities == 8 )
            {
                break;
            }
            
            touch = false;
            if ( idRenderSystemMathsLocal::SpheresIntersect( ps1->viewOrigin, ps1->viewRadius, ps2->viewOrigin, ps2->viewRadius ) )
            {
                for ( k = 0; k < ps1->numEntities; k++ )
                {
                    if ( idRenderSystemMathsLocal::SpheresIntersect( ps1->entityOrigins[k], ps1->entityRadiuses[k], ps2->viewOrigin, ps2->viewRadius ) )
                    {
                        touch = true;
                        break;
                    }
                }
            }
            
            if ( touch )
            {
                vec3_t newOrigin;
                F32 newRadius;
                
                idRenderSystemMathsLocal::BoundingSphereOfSpheres( ps1->viewOrigin, ps1->viewRadius, ps2->viewOrigin, ps2->viewRadius, newOrigin, &newRadius );
                VectorCopy( newOrigin, ps1->viewOrigin );
                ps1->viewRadius = newRadius;
                
                ps1->entityNums[ps1->numEntities] = ps2->entityNums[0];
                VectorCopy( ps2->viewOrigin, ps1->entityOrigins[ps1->numEntities] );
                ps1->entityRadiuses[ps1->numEntities] = ps2->viewRadius;
                
                ps1->numEntities++;
                
                for ( k = j; k < tr.refdef.num_pshadows - 1; k++ )
                {
                    tr.refdef.pshadows[k] = tr.refdef.pshadows[k + 1];
                }
                
                j--;
                tr.refdef.num_pshadows--;
            }
        }
    }
    
    // cap number of drawn pshadows
    if ( tr.refdef.num_pshadows > MAX_DRAWN_PSHADOWS )
    {
        tr.refdef.num_pshadows = MAX_DRAWN_PSHADOWS;
    }
    
    // next, fill up the rest of the shadow info
    for ( i = 0; i < tr.refdef.num_pshadows; i++ )
    {
        pshadow_t* shadow = &tr.refdef.pshadows[i];
        vec3_t up, ambientLight, directedLight, lightDir;
        
        VectorSet( lightDir, 0.57735f, 0.57735f, 0.57735f );
        
        renderSystemLocal.LightForPoint( shadow->viewOrigin, ambientLight, directedLight, lightDir );
        
        // sometimes there's no light
        if ( DotProduct( lightDir, lightDir ) < 0.9f )
        {
            VectorSet( lightDir, 0.0f, 0.0f, 1.0f );
        }
        
        if ( shadow->viewRadius * 3.0f > shadow->lightRadius )
        {
            shadow->lightRadius = shadow->viewRadius * 3.0f;
        }
        
        VectorMA( shadow->viewOrigin, shadow->viewRadius, lightDir, shadow->lightOrigin );
        
        // make up a projection, up doesn't matter
        VectorScale( lightDir, -1.0f, shadow->lightViewAxis[0] );
        VectorSet( up, 0, 0, -1 );
        
        if ( fabsf( DotProduct( up, shadow->lightViewAxis[0] ) ) > 0.9f )
        {
            VectorSet( up, -1, 0, 0 );
        }
        
        CrossProduct( shadow->lightViewAxis[0], up, shadow->lightViewAxis[1] );
        VectorNormalize( shadow->lightViewAxis[1] );
        CrossProduct( shadow->lightViewAxis[0], shadow->lightViewAxis[1], shadow->lightViewAxis[2] );
        
        VectorCopy( shadow->lightViewAxis[0], shadow->cullPlane.normal );
        shadow->cullPlane.dist = DotProduct( shadow->cullPlane.normal, shadow->lightOrigin );
        shadow->cullPlane.type = PLANE_NON_AXIAL;
        SetPlaneSignbits( &shadow->cullPlane );
    }
    
    // next, render shadowmaps
    for ( i = 0; i < tr.refdef.num_pshadows; i++ )
    {
        S32 j, firstDrawSurf;
        pshadow_t* shadow = &tr.refdef.pshadows[i];
        
        ::memset( &shadowParms, 0, sizeof( shadowParms ) );
        
        if ( glRefConfig.framebufferObject )
        {
            shadowParms.viewportX = 0;
            shadowParms.viewportY = 0;
        }
        else
        {
            shadowParms.viewportX = tr.refdef.x;
            shadowParms.viewportY = glConfig.vidHeight - ( tr.refdef.y + PSHADOW_MAP_SIZE );
        }
        
        shadowParms.viewportWidth = PSHADOW_MAP_SIZE;
        shadowParms.viewportHeight = PSHADOW_MAP_SIZE;
        shadowParms.isPortal = false;
        shadowParms.isMirror = false;
        
        shadowParms.fovX = 90;
        shadowParms.fovY = 90;
        
        if ( glRefConfig.framebufferObject )
        {
            shadowParms.targetFbo = tr.pshadowFbos[i];
        }
        
        shadowParms.flags = VPF_DEPTHSHADOW | VPF_NOVIEWMODEL;
        shadowParms.zFar = shadow->lightRadius;
        
        VectorCopy( shadow->lightOrigin, shadowParms.orientation.origin );
        
        VectorCopy( shadow->lightViewAxis[0], shadowParms.orientation.axis[0] );
        VectorCopy( shadow->lightViewAxis[1], shadowParms.orientation.axis[1] );
        VectorCopy( shadow->lightViewAxis[2], shadowParms.orientation.axis[2] );
        
        {
            tr.viewCount++;
            
            tr.viewParms = shadowParms;
            tr.viewParms.frameSceneNum = tr.frameSceneNum;
            tr.viewParms.frameCount = tr.frameCount;
            
            firstDrawSurf = tr.refdef.numDrawSurfs;
            
            tr.viewCount++;
            
            // set viewParms.world
            RotateForViewer();
            
            {
                F32 xmin, xmax, ymin, ymax, znear, zfar;
                viewParms_t* dest = &tr.viewParms;
                vec3_t pop;
                
                xmin = ymin = -shadow->viewRadius;
                xmax = ymax = shadow->viewRadius;
                znear = 0;
                zfar = shadow->lightRadius;
                
                dest->projectionMatrix[0] = 2 / ( xmax - xmin );
                dest->projectionMatrix[4] = 0;
                dest->projectionMatrix[8] = ( xmax + xmin ) / ( xmax - xmin );
                dest->projectionMatrix[12] = 0;
                
                dest->projectionMatrix[1] = 0;
                dest->projectionMatrix[5] = 2 / ( ymax - ymin );
                // normally 0
                dest->projectionMatrix[9] = ( ymax + ymin ) / ( ymax - ymin );
                dest->projectionMatrix[13] = 0;
                
                dest->projectionMatrix[2] = 0;
                dest->projectionMatrix[6] = 0;
                dest->projectionMatrix[10] = 2 / ( zfar - znear );
                dest->projectionMatrix[14] = 0;
                
                dest->projectionMatrix[3] = 0;
                dest->projectionMatrix[7] = 0;
                dest->projectionMatrix[11] = 0;
                dest->projectionMatrix[15] = 1;
                
                VectorScale( dest->orientation.axis[1],  1.0f, dest->frustum[0].normal );
                VectorMA( dest->orientation.origin, -shadow->viewRadius, dest->frustum[0].normal, pop );
                dest->frustum[0].dist = DotProduct( pop, dest->frustum[0].normal );
                
                VectorScale( dest->orientation.axis[1], -1.0f, dest->frustum[1].normal );
                VectorMA( dest->orientation.origin, -shadow->viewRadius, dest->frustum[1].normal, pop );
                dest->frustum[1].dist = DotProduct( pop, dest->frustum[1].normal );
                
                VectorScale( dest->orientation.axis[2],  1.0f, dest->frustum[2].normal );
                VectorMA( dest->orientation.origin, -shadow->viewRadius, dest->frustum[2].normal, pop );
                dest->frustum[2].dist = DotProduct( pop, dest->frustum[2].normal );
                
                VectorScale( dest->orientation.axis[2], -1.0f, dest->frustum[3].normal );
                VectorMA( dest->orientation.origin, -shadow->viewRadius, dest->frustum[3].normal, pop );
                dest->frustum[3].dist = DotProduct( pop, dest->frustum[3].normal );
                
                VectorScale( dest->orientation.axis[0], -1.0f, dest->frustum[4].normal );
                VectorMA( dest->orientation.origin, -shadow->lightRadius, dest->frustum[4].normal, pop );
                dest->frustum[4].dist = DotProduct( pop, dest->frustum[4].normal );
                
                for ( j = 0; j < 5; j++ )
                {
                    dest->frustum[j].type = PLANE_NON_AXIAL;
                    SetPlaneSignbits( &dest->frustum[j] );
                }
                
                dest->flags |= VPF_FARPLANEFRUSTUM;
            }
            
            for ( j = 0; j < shadow->numEntities; j++ )
            {
                AddEntitySurface( shadow->entityNums[j] );
            }
            
            SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );
        }
    }
}

F32 idRenderSystemMainLocal::CalcSplit( F32 n, F32 f, F32 i, F32 m )
{
    return ( n * pow( f / n, i / m ) + ( f - n ) * i / m ) / 2.0f;
}

void idRenderSystemMainLocal::RenderSunShadowMaps( const refdef_t* fd, S32 level )
{
    F32 splitZNear, splitZFar, splitBias, viewZNear, viewZFar;
    vec3_t lightViewAxis[3], lightOrigin, lightviewBounds[2];
    vec4_t lightDir, lightCol;
    viewParms_t	shadowParms;
    bool lightViewIndependentOfCameraView = false;
    bool isDlightShadow = false;
    
    if ( r_forceSun->integer == 2 )
    {
        S32 scale = 32768;
        F32 angle = ( fd->time % scale ) / ( F32 )scale * M_PI;
        
        lightDir[0] = cos( angle );
        lightDir[1] = sin( 35.0f * M_PI / 180.0f );
        lightDir[2] = sin( angle ) * cos( 35.0f * M_PI / 180.0f );
        lightDir[3] = 0.0f;
        
        if ( 1 ) //((fd->time % (scale * 2)) < scale)
        {
            lightCol[0] =
                lightCol[1] =
                    lightCol[2] = CLAMP( sin( angle ) * 2.0f, 0.0f, 1.0f ) * 2.0f;
            lightCol[3] = 1.0f;
        }
        else
        {
            lightCol[0] =
                lightCol[1] =
                    lightCol[2] = CLAMP( sin( angle ) * 2.0f * 0.1f, 0.0f, 0.1f );
            lightCol[3] = 1.0f;
        }
        
        VectorCopy4( lightDir, tr.refdef.sunDir );
        VectorCopy4( lightCol, tr.refdef.sunCol );
        VectorScale4( lightCol, 0.2f, tr.refdef.sunAmbCol );
    }
    else
    {
        VectorCopy4( tr.refdef.sunDir, lightDir );
    }
    
    viewZNear = r_shadowCascadeZNear->value;
    viewZFar = r_shadowCascadeZFar->value;
    splitBias = r_shadowCascadeZBias->value;
    
    switch ( level )
    {
        case 0:
        default:
            splitZNear = viewZNear;
            splitZFar = CalcSplit( viewZNear, viewZFar, 1, 3 ) + splitBias;
            break;
        case 1:
            splitZNear = CalcSplit( viewZNear, viewZFar, 1, 3 ) + splitBias;
            splitZFar = CalcSplit( viewZNear, viewZFar, 2, 3 ) + splitBias;
            break;
        case 2:
            splitZNear = CalcSplit( viewZNear, viewZFar, 2, 3 ) + splitBias;
            splitZFar = viewZFar + splitBias;
            break;
        case 3:
            splitZNear = viewZFar + splitBias;
            splitZFar = viewZFar * 2.0f;
            break;
    }
    
    if ( level != 3 )
    {
        VectorCopy( fd->vieworg, lightOrigin );
    }
    else
    {
        VectorCopy( tr.world->lightGridOrigin, lightOrigin );
    }
    
    // Make up a projection
    VectorScale( lightDir, -1.0f, lightViewAxis[0] );
    
    if ( level == 3 || lightViewIndependentOfCameraView )
    {
        // Use world up as light view up
        VectorSet( lightViewAxis[2], 0, 0, 1 );
    }
    else if ( level == 0 )
    {
        // Level 0 tries to use a diamond texture orientation relative to camera view
        // Use halfway between camera view forward and left for light view up
        VectorAdd( fd->viewaxis[0], fd->viewaxis[1], lightViewAxis[2] );
    }
    else
    {
        // Use camera view up as light view up
        VectorCopy( fd->viewaxis[2], lightViewAxis[2] );
    }
    
    // Check if too close to parallel to light direction
    if ( fabsf( DotProduct( lightViewAxis[2], lightViewAxis[0] ) ) > 0.9f )
    {
        if ( level == 3 || lightViewIndependentOfCameraView )
        {
            // Use world left as light view up
            VectorSet( lightViewAxis[2], 0, 1, 0 );
        }
        else if ( level == 0 )
        {
            // Level 0 tries to use a diamond texture orientation relative to camera view
            // Use halfway between camera view forward and up for light view up
            VectorAdd( fd->viewaxis[0], fd->viewaxis[2], lightViewAxis[2] );
        }
        else
        {
            // Use camera view left as light view up
            VectorCopy( fd->viewaxis[1], lightViewAxis[2] );
        }
    }
    
    // clean axes
    CrossProduct( lightViewAxis[2], lightViewAxis[0], lightViewAxis[1] );
    VectorNormalize( lightViewAxis[1] );
    CrossProduct( lightViewAxis[0], lightViewAxis[1], lightViewAxis[2] );
    
    // Create bounds for light projection using slice of view projection
    {
        mat4_t lightViewMatrix;
        vec4_t point, base, lightViewPoint;
        F32 lx, ly;
        
        base[3] = 1;
        point[3] = 1;
        lightViewPoint[3] = 1;
        
        idRenderSystemMathsLocal::Mat4View( lightViewAxis, lightOrigin, lightViewMatrix );
        
        ClearBounds( lightviewBounds[0], lightviewBounds[1] );
        
        if ( level != 3 )
        {
            // add view near plane
            lx = splitZNear * tan( fd->fov_x * M_PI / 360.0f );
            ly = splitZNear * tan( fd->fov_y * M_PI / 360.0f );
            VectorMA( fd->vieworg, splitZNear, fd->viewaxis[0], base );
            
            VectorMA( base,   lx, fd->viewaxis[1], point );
            VectorMA( point,  ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            VectorMA( base,  -lx, fd->viewaxis[1], point );
            VectorMA( point,  ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            VectorMA( base,   lx, fd->viewaxis[1], point );
            VectorMA( point, -ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            VectorMA( base,  -lx, fd->viewaxis[1], point );
            VectorMA( point, -ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            // add view far plane
            lx = splitZFar * tan( fd->fov_x * M_PI / 360.0f );
            ly = splitZFar * tan( fd->fov_y * M_PI / 360.0f );
            VectorMA( fd->vieworg, splitZFar, fd->viewaxis[0], base );
            
            VectorMA( base,   lx, fd->viewaxis[1], point );
            VectorMA( point,  ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            VectorMA( base,  -lx, fd->viewaxis[1], point );
            VectorMA( point,  ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            VectorMA( base,   lx, fd->viewaxis[1], point );
            VectorMA( point, -ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            VectorMA( base,  -lx, fd->viewaxis[1], point );
            VectorMA( point, -ly, fd->viewaxis[2], point );
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
        }
        else
        {
            // use light grid size as level size
            // FIXME: could be tighter
            vec3_t bounds;
            
            bounds[0] = tr.world->lightGridSize[0] * tr.world->lightGridBounds[0];
            bounds[1] = tr.world->lightGridSize[1] * tr.world->lightGridBounds[1];
            bounds[2] = tr.world->lightGridSize[2] * tr.world->lightGridBounds[2];
            
            point[0] = tr.world->lightGridOrigin[0];
            point[1] = tr.world->lightGridOrigin[1];
            point[2] = tr.world->lightGridOrigin[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0] + bounds[0];
            point[1] = tr.world->lightGridOrigin[1];
            point[2] = tr.world->lightGridOrigin[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0];
            point[1] = tr.world->lightGridOrigin[1] + bounds[1];
            point[2] = tr.world->lightGridOrigin[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0] + bounds[0];
            point[1] = tr.world->lightGridOrigin[1] + bounds[1];
            point[2] = tr.world->lightGridOrigin[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0];
            point[1] = tr.world->lightGridOrigin[1];
            point[2] = tr.world->lightGridOrigin[2] + bounds[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0] + bounds[0];
            point[1] = tr.world->lightGridOrigin[1];
            point[2] = tr.world->lightGridOrigin[2] + bounds[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0];
            point[1] = tr.world->lightGridOrigin[1] + bounds[1];
            point[2] = tr.world->lightGridOrigin[2] + bounds[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
            
            point[0] = tr.world->lightGridOrigin[0] + bounds[0];
            point[1] = tr.world->lightGridOrigin[1] + bounds[1];
            point[2] = tr.world->lightGridOrigin[2] + bounds[2];
            idRenderSystemMathsLocal::Mat4Transform( lightViewMatrix, point, lightViewPoint );
            AddPointToBounds( lightViewPoint, lightviewBounds[0], lightviewBounds[1] );
        }
        
        if ( !glRefConfig.depthClamp )
        {
            lightviewBounds[0][0] = lightviewBounds[1][0] - 8192;
        }
        
        // Moving the Light in Texel-Sized Increments
        // from http://msdn.microsoft.com/en-us/library/windows/desktop/ee416324%28v=vs.85%29.aspx
        if ( lightViewIndependentOfCameraView )
        {
            F32 cascadeBound, worldUnitsPerTexel, invWorldUnitsPerTexel;
            
            cascadeBound = MAX( lightviewBounds[1][0] - lightviewBounds[0][0], lightviewBounds[1][1] - lightviewBounds[0][1] );
            cascadeBound = MAX( cascadeBound, lightviewBounds[1][2] - lightviewBounds[0][2] );
            worldUnitsPerTexel = cascadeBound / tr.sunShadowFbo[level]->width;
            invWorldUnitsPerTexel = 1.0f / worldUnitsPerTexel;
            
            VectorScale( lightviewBounds[0], invWorldUnitsPerTexel, lightviewBounds[0] );
            lightviewBounds[0][0] = floor( lightviewBounds[0][0] );
            lightviewBounds[0][1] = floor( lightviewBounds[0][1] );
            lightviewBounds[0][2] = floor( lightviewBounds[0][2] );
            VectorScale( lightviewBounds[0], worldUnitsPerTexel, lightviewBounds[0] );
            
            VectorScale( lightviewBounds[1], invWorldUnitsPerTexel, lightviewBounds[1] );
            lightviewBounds[1][0] = floor( lightviewBounds[1][0] );
            lightviewBounds[1][1] = floor( lightviewBounds[1][1] );
            lightviewBounds[1][2] = floor( lightviewBounds[1][2] );
            VectorScale( lightviewBounds[1], worldUnitsPerTexel, lightviewBounds[1] );
        }
        
        //clientMainSystem->RefPrintf(PRINT_ALL, "level %d znear %f zfar %f\n", level, lightviewBounds[0][0], lightviewBounds[1][0]);
        //clientMainSystem->RefPrintf(PRINT_ALL, "xmin %f xmax %f ymin %f ymax %f\n", lightviewBounds[0][1], lightviewBounds[1][1], -lightviewBounds[1][2], -lightviewBounds[0][2]);
    }
    
    {
        S32 firstDrawSurf;
        
        ::memset( &shadowParms, 0, sizeof( shadowParms ) );
        
        if ( glRefConfig.framebufferObject )
        {
            shadowParms.viewportX = 0;
            shadowParms.viewportY = 0;
        }
        else
        {
            shadowParms.viewportX = tr.refdef.x;
            shadowParms.viewportY = glConfig.vidHeight - ( tr.refdef.y + tr.sunShadowFbo[level]->height );
        }
        
        shadowParms.viewportWidth  = tr.sunShadowFbo[level]->width;
        shadowParms.viewportHeight = tr.sunShadowFbo[level]->height;
        shadowParms.isPortal = false;
        shadowParms.isMirror = false;
        
        shadowParms.fovX = 90;
        shadowParms.fovY = 90;
        
        if ( glRefConfig.framebufferObject )
        {
            shadowParms.targetFbo = tr.sunShadowFbo[level];
        }
        
        shadowParms.flags = VPF_DEPTHSHADOW | VPF_DEPTHCLAMP | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL;
        shadowParms.zFar = lightviewBounds[1][0];
        shadowParms.zNear = r_znear->value;
        
        VectorCopy( lightOrigin, shadowParms.orientation.origin );
        
        VectorCopy( lightViewAxis[0], shadowParms.orientation.axis[0] );
        VectorCopy( lightViewAxis[1], shadowParms.orientation.axis[1] );
        VectorCopy( lightViewAxis[2], shadowParms.orientation.axis[2] );
        
        VectorCopy( lightOrigin, shadowParms.pvsOrigin );
        
        {
            tr.viewCount++;
            
            tr.viewParms = shadowParms;
            tr.viewParms.frameSceneNum = tr.frameSceneNum;
            tr.viewParms.frameCount = tr.frameCount;
            
            firstDrawSurf = tr.refdef.numDrawSurfs;
            
            tr.viewCount++;
            
            // set viewParms.world
            RotateForViewer();
            
            SetupProjectionOrtho( &tr.viewParms, lightviewBounds );
            
            if ( !isDlightShadow )
            {
                idRenderSystemWorldLocal::AddWorldSurfaces();
            }
            
            idRenderSystemSceneLocal::AddPolygonSurfaces();
            
            AddEntitySurfaces();
            
            SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );
        }
        
        idRenderSystemMathsLocal::Mat4Multiply( tr.viewParms.projectionMatrix, tr.viewParms.world.modelMatrix, tr.refdef.sunShadowMvp[level] );
    }
}

void idRenderSystemMainLocal::RenderCubemapSide( cubemap_t* cubemap, S32 cubemapIndex, S32 cubemapSide, bool subscene, bool bounce )
{
    refdef_t refdef;
    viewParms_t	parms;
    
    ::memset( &refdef, 0, sizeof( refdef ) );
    refdef.rdflags = 0;
    VectorCopy( cubemap[cubemapIndex].origin, refdef.vieworg );
    
    switch ( cubemapSide )
    {
        case 0:
            // -X
            VectorSet( refdef.viewaxis[0], -1,  0,  0 );
            VectorSet( refdef.viewaxis[1],  0,  0, -1 );
            VectorSet( refdef.viewaxis[2],  0,  1,  0 );
            break;
        case 1:
            // +X
            VectorSet( refdef.viewaxis[0],  1,  0,  0 );
            VectorSet( refdef.viewaxis[1],  0,  0,  1 );
            VectorSet( refdef.viewaxis[2],  0,  1,  0 );
            break;
        case 2:
            // -Y
            VectorSet( refdef.viewaxis[0],  0, -1,  0 );
            VectorSet( refdef.viewaxis[1],  1,  0,  0 );
            VectorSet( refdef.viewaxis[2],  0,  0, -1 );
            break;
        case 3:
            // +Y
            VectorSet( refdef.viewaxis[0],  0,  1,  0 );
            VectorSet( refdef.viewaxis[1],  1,  0,  0 );
            VectorSet( refdef.viewaxis[2],  0,  0,  1 );
            break;
        case 4:
            // -Z
            VectorSet( refdef.viewaxis[0],  0,  0, -1 );
            VectorSet( refdef.viewaxis[1],  1,  0,  0 );
            VectorSet( refdef.viewaxis[2],  0,  1,  0 );
            break;
        case 5:
            // +Z
            VectorSet( refdef.viewaxis[0],  0,  0,  1 );
            VectorSet( refdef.viewaxis[1], -1,  0,  0 );
            VectorSet( refdef.viewaxis[2],  0,  1,  0 );
            break;
    }
    
    refdef.fov_x = 90;
    refdef.fov_y = 90;
    
    refdef.x = 0;
    refdef.y = 0;
    refdef.width = tr.renderCubeFbo->width;
    refdef.height = tr.renderCubeFbo->height;
    
    refdef.time = 0;
    
    if ( !subscene )
    {
        idRenderSystemSceneLocal::BeginScene( &refdef );
        
        if ( !( refdef.rdflags & RDF_NOWORLDMODEL ) && tr.refdef.num_dlights && r_dlightMode->integer >= 2 )
        {
            RenderDlightCubemaps( &refdef );
        }
        
        if ( glRefConfig.framebufferObject && r_sunlightMode->integer && r_depthPrepass->value && ( ( r_forceSun->integer ) || tr.sunShadows ) )
        {
            RenderSunShadowMaps( &refdef, 0 );
            RenderSunShadowMaps( &refdef, 1 );
            RenderSunShadowMaps( &refdef, 2 );
            RenderSunShadowMaps( &refdef, 3 );
        }
    }
    
    {
        vec3_t ambient, directed, lightDir;
        F32 scale;
        
        renderSystemLocal.LightForPoint( tr.refdef.vieworg, ambient, directed, lightDir );
        scale = directed[0] + directed[1] + directed[2] + ambient[0] + ambient[1] + ambient[2] + 1.0f;
        
        // only print message for first side
        if ( scale < 1.0001f && cubemapSide == 0 )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "cubemap %d %s (%f, %f, %f) is outside the lightgrid or inside a wall!\n", cubemapIndex,
                                         tr.cubemaps[cubemapIndex].name, tr.refdef.vieworg[0], tr.refdef.vieworg[1], tr.refdef.vieworg[2] );
        }
    }
    
    ::memset( &parms, 0, sizeof( parms ) );
    
    parms.viewportX = 0;
    parms.viewportY = 0;
    parms.viewportWidth = tr.renderCubeFbo->width;
    parms.viewportHeight = tr.renderCubeFbo->height;
    parms.isPortal = false;
    parms.isMirror = true;
    parms.flags = VPF_NOVIEWMODEL;
    if ( !bounce )
    {
        parms.flags |= VPF_NOCUBEMAPS;
    }
    
    parms.fovX = 90;
    parms.fovY = 90;
    parms.zNear = r_znear->value;
    
    VectorCopy( refdef.vieworg, parms.orientation.origin );
    VectorCopy( refdef.viewaxis[0], parms.orientation.axis[0] );
    VectorCopy( refdef.viewaxis[1], parms.orientation.axis[1] );
    VectorCopy( refdef.viewaxis[2], parms.orientation.axis[2] );
    
    VectorCopy( refdef.vieworg, parms.pvsOrigin );
    
    if ( glRefConfig.framebufferObject && r_sunlightMode->integer && r_depthPrepass->value && ( ( r_forceSun->integer ) || tr.sunShadows ) )
    {
        parms.flags |= VPF_USESUNLIGHT;
    }
    
    parms.targetFbo = tr.renderCubeFbo;
    parms.cubemapSelection = cubemap;
    parms.targetFboLayer = cubemapSide;
    parms.targetFboCubemapIndex = cubemapIndex;
    
    RenderView( &parms );
    
    if ( !subscene )
    {
        idRenderSystemSceneLocal::EndScene();
    }
}

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
// File name:   r_marks.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: polygon projection on the world polygons
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemMarksLocal renderSystemMarksLocal;

/*
===============
idRenderSystemMarksLocal::idRenderSystemMarksLocal
===============
*/
idRenderSystemMarksLocal::idRenderSystemMarksLocal( void )
{
}

/*
===============
idRenderSystemMarksLocal::~idRenderSystemMarksLocal
===============
*/
idRenderSystemMarksLocal::~idRenderSystemMarksLocal( void )
{
}

/*
=============
idRenderSystemMarksLocal::ChopPolyBehindPlane

Out must have space for two more vertexes than in
=============
*/
void idRenderSystemMarksLocal::ChopPolyBehindPlane( S32 numInPoints, vec3_t inPoints[MAX_VERTS_ON_POLY], S32* numOutPoints,
        vec3_t outPoints[MAX_VERTS_ON_POLY], vec3_t normal, F32 dist, F32 epsilon )
{
    S32	i, j, sides[MAX_VERTS_ON_POLY + 4] = { 0 }, counts[3];
    F32	dists[MAX_VERTS_ON_POLY + 4] = { 0 }, dot, * p1, *p2, *clip, d;
    
    // don't clip if it might overflow
    if ( numInPoints >= MAX_VERTS_ON_POLY - 2 )
    {
        *numOutPoints = 0;
        return;
    }
    
    counts[0] = counts[1] = counts[2] = 0;
    
    // determine sides for each point
    for ( i = 0 ; i < numInPoints ; i++ )
    {
        dot = DotProduct( inPoints[i], normal );
        dot -= dist;
        dists[i] = dot;
        
        if ( dot > epsilon )
        {
            sides[i] = SIDE_FRONT;
        }
        else if ( dot < -epsilon )
        {
            sides[i] = SIDE_BACK;
        }
        else
        {
            sides[i] = SIDE_ON;
        }
        
        counts[sides[i]]++;
    }
    
    sides[i] = sides[0];
    dists[i] = dists[0];
    
    *numOutPoints = 0;
    
    if ( !counts[0] )
    {
        return;
    }
    
    if ( !counts[1] )
    {
        *numOutPoints = numInPoints;
        ::memcpy( outPoints, inPoints, numInPoints * sizeof( vec3_t ) );
        return;
    }
    
    for ( i = 0 ; i < numInPoints ; i++ )
    {
        p1 = inPoints[i];
        clip = outPoints[ *numOutPoints ];
        
        if ( sides[i] == SIDE_ON )
        {
            VectorCopy( p1, clip );
            ( *numOutPoints )++;
            continue;
        }
        
        if ( sides[i] == SIDE_FRONT )
        {
            VectorCopy( p1, clip );
            ( *numOutPoints )++;
            clip = outPoints[ *numOutPoints ];
        }
        
        if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] )
        {
            continue;
        }
        
        // generate a split point
        p2 = inPoints[( i + 1 ) % numInPoints ];
        
        d = dists[i] - dists[i + 1];
        
        if ( d == 0 )
        {
            dot = 0;
        }
        else
        {
            dot = dists[i] / d;
        }
        
        // clip xyz
        
        for ( j = 0 ; j < 3 ; j++ )
        {
            clip[j] = p1[j] + dot * ( p2[j] - p1[j] );
        }
        
        ( *numOutPoints )++;
    }
}

/*
=================
idRenderSystemMarksLocal::BoxSurfaces_r
=================
*/
void idRenderSystemMarksLocal::BoxSurfaces_r( mnode_t* node, vec3_t mins, vec3_t maxs, surfaceType_t** list, S32 listsize, S32* listlength, vec3_t dir )
{
    S32	s, c, * mark;
    msurface_t*	surf;
    
    // do the tail recursion in a loop
    while ( node->contents == -1 )
    {
        s = collisionModelManager->BoxOnPlaneSide( mins, maxs, node->plane );
        if ( s == 1 )
        {
            node = node->children[0];
        }
        else if ( s == 2 )
        {
            node = node->children[1];
        }
        else
        {
            BoxSurfaces_r( node->children[0], mins, maxs, list, listsize, listlength, dir );
            node = node->children[1];
        }
    }
    
    // add the individual surfaces
    mark = tr.world->marksurfaces + node->firstmarksurface;
    c = node->nummarksurfaces;
    
    while ( c-- )
    {
        S32* surfViewCount;
        
        if ( *listlength >= listsize )
        {
            break;
        }
        
        surfViewCount = &tr.world->surfacesViewCount[*mark];
        surf = tr.world->surfaces + *mark;
        
        // check if the surface has NOIMPACT or NOMARKS set
        if ( ( surf->shader->surfaceFlags & ( SURF_NOIMPACT | SURF_NOMARKS ) ) || ( surf->shader->contentFlags & CONTENTS_FOG ) )
        {
            *surfViewCount = tr.viewCount;
        }
        
        // extra check for surfaces to avoid list overflows
        else if ( *( surf->data ) == SF_FACE )
        {
            // the face plane should go through the box
            s = collisionModelManager->BoxOnPlaneSide( mins, maxs, &surf->cullinfo.plane );
            
            if ( s == 1 || s == 2 )
            {
                *surfViewCount = tr.viewCount;
            }
            else if ( DotProduct( surf->cullinfo.plane.normal, dir ) > -0.5 )
            {
                // don't add faces that make sharp angles with the projection direction
                *surfViewCount = tr.viewCount;
            }
        }
        else if ( *( surf->data ) != SF_GRID && *( surf->data ) != SF_TRIANGLES )
        {
            *surfViewCount = tr.viewCount;
        }
        
        // check the viewCount because the surface may have
        // already been added if it spans multiple leafs
        if ( *surfViewCount != tr.viewCount )
        {
            *surfViewCount = tr.viewCount;
            list[*listlength] = surf->data;
            ( *listlength )++;
        }
        
        mark++;
    }
}

/*
=================
idRenderSystemMarksLocal::AddMarkFragments
=================
*/
void idRenderSystemMarksLocal::AddMarkFragments( S32 numClipPoints, vec3_t clipPoints[2][MAX_VERTS_ON_POLY], S32 numPlanes, vec3_t* normals,
        F32* dists, S32 maxPoints, vec3_t pointBuffer, S32 maxFragments, markFragment_t* fragmentBuffer, S32* returnedPoints, S32* returnedFragments,
        vec3_t mins, vec3_t maxs )
{
    S32 pingPong, i;
    markFragment_t*	mf;
    
    // chop the surface by all the bounding planes of the to be projected polygon
    pingPong = 0;
    
    for ( i = 0 ; i < numPlanes ; i++ )
    {
    
        ChopPolyBehindPlane( numClipPoints, clipPoints[pingPong], &numClipPoints, clipPoints[!pingPong], normals[i], dists[i], 0.5 );
        pingPong ^= 1;
        
        if ( numClipPoints == 0 )
        {
            break;
        }
    }
    
    // completely clipped away?
    if ( numClipPoints == 0 )
    {
        return;
    }
    
    // add this fragment to the returned list
    if ( numClipPoints + ( *returnedPoints ) > maxPoints )
    {
        // not enough space for this polygon
        return;
    }
    
    /*
    // all the clip points should be within the bounding box
    for ( i = 0 ; i < numClipPoints ; i++ ) {
    	S32 j;
    	for ( j = 0 ; j < 3 ; j++ ) {
    		if (clipPoints[pingPong][i][j] < mins[j] - 0.5) break;
    		if (clipPoints[pingPong][i][j] > maxs[j] + 0.5) break;
    	}
    	if (j < 3) break;
    }
    if (i < numClipPoints) return;
    */
    
    mf = fragmentBuffer + ( *returnedFragments );
    mf->firstPoint = ( *returnedPoints );
    mf->numPoints = numClipPoints;
    ::memcpy( pointBuffer + ( *returnedPoints ) * 3, clipPoints[pingPong], numClipPoints * sizeof( vec3_t ) );
    
    ( *returnedPoints ) += numClipPoints;
    ( *returnedFragments )++;
}

/*
=================
idRenderSystemLocal::MarkFragments
=================
*/
S32 idRenderSystemLocal::MarkFragments( S32 numPoints, const vec3_t* points, const vec3_t projection, S32 maxPoints, vec3_t pointBuffer,
                                        S32 maxFragments, markFragment_t* fragmentBuffer )
{
    S32 numsurfaces, numPlanes, i, j, k, m, n, returnedFragments, returnedPoints, numClipPoints;
    U32* tri;
    F32*	v, dists[MAX_VERTS_ON_POLY + 2];
    surfaceType_t* surfaces[64];
    srfBspSurface_t* cv;
    srfVert_t* dv;
    vec3_t mins, maxs, normals[MAX_VERTS_ON_POLY + 2], clipPoints[2][MAX_VERTS_ON_POLY], normal, projectionDir, v1, v2;
    
    if ( numPoints <= 0 )
    {
        return 0;
    }
    
    //increment view count for double check prevention
    tr.viewCount++;
    
    VectorNormalize2( projection, projectionDir );
    
    // find all the brushes that are to be considered
    ClearBounds( mins, maxs );
    
    for ( i = 0 ; i < numPoints ; i++ )
    {
        vec3_t	temp;
        
        AddPointToBounds( points[i], mins, maxs );
        VectorAdd( points[i], projection, temp );
        AddPointToBounds( temp, mins, maxs );
        
        // make sure we get all the leafs (also the one(s) in front of the hit surface)
        VectorMA( points[i], -20, projectionDir, temp );
        AddPointToBounds( temp, mins, maxs );
    }
    
    if ( numPoints > MAX_VERTS_ON_POLY )
    {
        numPoints = MAX_VERTS_ON_POLY;
    }
    
    // create the bounding planes for the to be projected polygon
    for ( i = 0 ; i < numPoints ; i++ )
    {
        VectorSubtract( points[( i + 1 ) % numPoints], points[i], v1 );
        VectorAdd( points[i], projection, v2 );
        VectorSubtract( points[i], v2, v2 );
        CrossProduct( v1, v2, normals[i] );
        VectorNormalize( normals[i] );
        dists[i] = DotProduct( normals[i], points[i] );
    }
    
    // add near and far clipping planes for projection
    VectorCopy( projectionDir, normals[numPoints] );
    dists[numPoints] = DotProduct( normals[numPoints], points[0] ) - 32;
    VectorCopy( projectionDir, normals[numPoints + 1] );
    VectorInverse( normals[numPoints + 1] );
    dists[numPoints + 1] = DotProduct( normals[numPoints + 1], points[0] ) - 20;
    numPlanes = numPoints + 2;
    
    numsurfaces = 0;
    renderSystemMarksLocal.BoxSurfaces_r( tr.world->nodes, mins, maxs, surfaces, 64, &numsurfaces, projectionDir );
    //assert(numsurfaces <= 64);
    //assert(numsurfaces != 64);
    
    returnedPoints = 0;
    returnedFragments = 0;
    
    for ( i = 0 ; i < numsurfaces ; i++ )
    {
    
        if ( *surfaces[i] == SF_GRID )
        {
            cv = ( srfBspSurface_t* ) surfaces[i];
            
            for ( m = 0 ; m < cv->height - 1 ; m++ )
            {
                for ( n = 0 ; n < cv->width - 1 ; n++ )
                {
                    // We triangulate the grid and chop all triangles within
                    // the bounding planes of the to be projected polygon.
                    // LOD is not taken into account, not such a big deal though.
                    //
                    // It's probably much nicer to chop the grid itself and deal
                    // with this grid as a normal SF_GRID surface so LOD will
                    // be applied. However the LOD of that chopped grid must
                    // be synced with the LOD of the original curve.
                    // One way to do this; the chopped grid shares vertices with
                    // the original curve. When LOD is applied to the original
                    // curve the unused vertices are flagged. Now the chopped curve
                    // should skip the flagged vertices. This still leaves the
                    // problems with the vertices at the chopped grid edges.
                    //
                    // To avoid issues when LOD applied to "hollow curves" (like
                    // the ones around many jump pads) we now just add a 2 unit
                    // offset to the triangle vertices.
                    // The offset is added in the vertex normal vector direction
                    // so all triangles will still fit together.
                    // The 2 unit offset should avoid pretty much all LOD problems.
                    vec3_t fNormal;
                    
                    numClipPoints = 3;
                    
                    dv = cv->verts + m * cv->width + n;
                    
                    VectorCopy( dv[0].xyz, clipPoints[0][0] );
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, dv[0].normal );
                    VectorMA( clipPoints[0][0], MARKER_OFFSET, fNormal, clipPoints[0][0] );
                    VectorCopy( dv[cv->width].xyz, clipPoints[0][1] );
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, dv[cv->width].normal );
                    VectorMA( clipPoints[0][1], MARKER_OFFSET, fNormal, clipPoints[0][1] );
                    VectorCopy( dv[1].xyz, clipPoints[0][2] );
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, dv[1].normal );
                    VectorMA( clipPoints[0][2], MARKER_OFFSET, fNormal, clipPoints[0][2] );
                    // check the normal of this triangle
                    VectorSubtract( clipPoints[0][0], clipPoints[0][1], v1 );
                    VectorSubtract( clipPoints[0][2], clipPoints[0][1], v2 );
                    CrossProduct( v1, v2, normal );
                    VectorNormalize( normal );
                    if ( DotProduct( normal, projectionDir ) < -0.1 )
                    {
                        // add the fragments of this triangle
                        renderSystemMarksLocal.AddMarkFragments( numClipPoints, clipPoints,
                                numPlanes, normals, dists,
                                maxPoints, pointBuffer,
                                maxFragments, fragmentBuffer,
                                &returnedPoints, &returnedFragments, mins, maxs );
                                
                        if ( returnedFragments == maxFragments )
                        {
                            // not enough space for more fragments
                            return returnedFragments;
                        }
                    }
                    
                    VectorCopy( dv[1].xyz, clipPoints[0][0] );
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, dv[1].normal );
                    VectorMA( clipPoints[0][0], MARKER_OFFSET, fNormal, clipPoints[0][0] );
                    VectorCopy( dv[cv->width].xyz, clipPoints[0][1] );
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, dv[cv->width].normal );
                    VectorMA( clipPoints[0][1], MARKER_OFFSET, fNormal, clipPoints[0][1] );
                    VectorCopy( dv[cv->width + 1].xyz, clipPoints[0][2] );
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, dv[cv->width + 1].normal );
                    VectorMA( clipPoints[0][2], MARKER_OFFSET, fNormal, clipPoints[0][2] );
                    // check the normal of this triangle
                    VectorSubtract( clipPoints[0][0], clipPoints[0][1], v1 );
                    VectorSubtract( clipPoints[0][2], clipPoints[0][1], v2 );
                    CrossProduct( v1, v2, normal );
                    VectorNormalize( normal );
                    if ( DotProduct( normal, projectionDir ) < -0.05 )
                    {
                        // add the fragments of this triangle
                        renderSystemMarksLocal.AddMarkFragments( numClipPoints, clipPoints,
                                numPlanes, normals, dists,
                                maxPoints, pointBuffer,
                                maxFragments, fragmentBuffer,
                                &returnedPoints, &returnedFragments, mins, maxs );
                                
                        if ( returnedFragments == maxFragments )
                        {
                            return returnedFragments;	// not enough space for more fragments
                        }
                    }
                }
            }
        }
        else if ( *surfaces[i] == SF_FACE )
        {
        
            srfBspSurface_t* surf = ( srfBspSurface_t* ) surfaces[i];
            
            // check the normal of this face
            if ( DotProduct( surf->cullPlane.normal, projectionDir ) > -0.5 )
            {
                continue;
            }
            
            for ( k = 0, tri = surf->indexes; k < surf->numIndexes; k += 3, tri += 3 )
            {
                for ( j = 0; j < 3; j++ )
                {
                    v = surf->verts[tri[j]].xyz;
                    VectorMA( v, MARKER_OFFSET, surf->cullPlane.normal, clipPoints[0][j] );
                }
                
                // add the fragments of this face
                renderSystemMarksLocal.AddMarkFragments( 3, clipPoints,
                        numPlanes, normals, dists,
                        maxPoints, pointBuffer,
                        maxFragments, fragmentBuffer,
                        &returnedPoints, &returnedFragments, mins, maxs );
                        
                if ( returnedFragments == maxFragments )
                {
                    // not enough space for more fragments
                    return returnedFragments;
                }
            }
        }
        else if ( *surfaces[i] == SF_TRIANGLES && r_marksOnTriangleMeshes->integer )
        {
        
            srfBspSurface_t* surf = ( srfBspSurface_t* ) surfaces[i];
            
            for ( k = 0, tri = surf->indexes; k < surf->numIndexes; k += 3, tri += 3 )
            {
                for ( j = 0; j < 3; j++ )
                {
                    vec3_t fNormal;
                    v = surf->verts[tri[j]].xyz;
                    idRenderSystemVaoLocal::VaoUnpackNormal( fNormal, surf->verts[tri[j]].normal );
                    VectorMA( v, MARKER_OFFSET, fNormal, clipPoints[0][j] );
                }
                
                // add the fragments of this face
                renderSystemMarksLocal.AddMarkFragments( 3, clipPoints,
                        numPlanes, normals, dists,
                        maxPoints, pointBuffer,
                        maxFragments, fragmentBuffer, &returnedPoints, &returnedFragments, mins, maxs );
                        
                if ( returnedFragments == maxFragments )
                {
                    // not enough space for more fragments
                    return returnedFragments;
                }
            }
        }
    }
    
    return returnedFragments;
}

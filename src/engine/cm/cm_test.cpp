////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   cm_test.cpp
// Version:     v1.01
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

/*
==================
CM_PointLeafnum_r
==================
*/
S32 CM_PointLeafnum_r( const vec3_t p, S32 num )
{
    F32           d;
    cNode_t*        node;
    cplane_t*       plane;
    
    while ( num >= 0 )
    {
        node = cm.nodes + num;
        plane = node->plane;
        
        if ( plane->type < 3 )
        {
            d = p[plane->type] - plane->dist;
        }
        else
        {
            d = DotProduct( plane->normal, p ) - plane->dist;
        }
        if ( d < 0 )
        {
            num = node->children[1];
        }
        else
        {
            num = node->children[0];
        }
    }
    
    c_pointcontents++;			// optimize counter
    
    return -1 - num;
}

/*
==================
idCollisionModelManagerLocal::PointLeafnum
==================
*/
S32 idCollisionModelManagerLocal::PointLeafnum( const vec3_t p )
{
    if ( !cm.numNodes ) // map not loaded
    {
        return 0;
    }
    return CM_PointLeafnum_r( p, 0 );
}


/*
======================================================================
LEAF LISTING
======================================================================
*/

void CM_StoreLeafs( leafList_t* ll, S32 nodenum )
{
    S32 leafNum;
    
    leafNum = -1 - nodenum;
    
    // store the lastLeaf even if the list is overflowed
    if ( cm.leafs[leafNum].cluster != -1 )
    {
        ll->lastLeaf = leafNum;
    }
    
    if ( ll->count >= ll->maxcount )
    {
        ll->overflowed = true;
        return;
    }
    ll->list[ll->count++] = leafNum;
}

/*
==================
CM_StoreBrushes
==================
*/
void CM_StoreBrushes( leafList_t* ll, S32 nodenum )
{
    S32             i, k, leafnum, brushnum;
    cLeaf_t*        leaf;
    cbrush_t*       b;
    
    leafnum = -1 - nodenum;
    
    leaf = &cm.leafs[leafnum];
    
    for ( k = 0; k < leaf->numLeafBrushes; k++ )
    {
        brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];
        b = &cm.brushes[brushnum];
        if ( b->checkcount == cm.checkcount )
        {
            continue; // already checked this brush in another leaf
        }
        b->checkcount = cm.checkcount;
        for ( i = 0; i < 3; i++ )
        {
            if ( b->bounds[0][i] >= ll->bounds[1][i] || b->bounds[1][i] <= ll->bounds[0][i] )
            {
                break;
            }
        }
        if ( i != 3 )
        {
            continue;
        }
        if ( ll->count >= ll->maxcount )
        {
            ll->overflowed = true;
            return;
        }
        ( ( cbrush_t** ) ll->list )[ll->count++] = b;
    }
#if 0
    // store patches?
    for ( k = 0; k < leaf->numLeafSurfaces; k++ )
    {
        patch = cm.surfaces[cm.leafsurfaces[leaf->firstleafsurface + k]];
        if ( !patch )
        {
            continue;
        }
    }
#endif
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
void CM_BoxLeafnums_r( leafList_t* ll, S32 nodenum )
{
    cplane_t*       plane;
    cNode_t*        node;
    S32             s;
    
    while ( 1 )
    {
        if ( nodenum < 0 )
        {
            ll->storeLeafs( ll, nodenum );
            return;
        }
        
        node = &cm.nodes[nodenum];
        plane = node->plane;
        s = collisionModelManagerLocal.BoxOnPlaneSide( ll->bounds[0], ll->bounds[1], plane );
        if ( s == 1 )
        {
            nodenum = node->children[0];
        }
        else if ( s == 2 )
        {
            nodenum = node->children[1];
        }
        else
        {
            // go down both
            CM_BoxLeafnums_r( ll, node->children[0] );
            nodenum = node->children[1];
        }
    }
}

/*
==================
idCollisionModelManagerLocal::BoxLeafnums
==================
*/
S32 idCollisionModelManagerLocal::BoxLeafnums( const vec3_t mins, const vec3_t maxs, S32* list, S32 listsize, S32* lastLeaf )
{
    leafList_t ll;
    
    cm.checkcount++;
    
    VectorCopy( mins, ll.bounds[0] );
    VectorCopy( maxs, ll.bounds[1] );
    ll.count = 0;
    ll.maxcount = listsize;
    ll.list = list;
    ll.storeLeafs = CM_StoreLeafs;
    ll.lastLeaf = 0;
    ll.overflowed = false;
    
    CM_BoxLeafnums_r( &ll, 0 );
    
    *lastLeaf = ll.lastLeaf;
    return ll.count;
}

/*
==================
CM_BoxBrushes
==================
*/
S32 CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t** list, S32 listsize )
{
    leafList_t      ll;
    
    cm.checkcount++;
    
    VectorCopy( mins, ll.bounds[0] );
    VectorCopy( maxs, ll.bounds[1] );
    ll.count = 0;
    ll.maxcount = listsize;
    ll.list = ( S32* )list;
    ll.storeLeafs = CM_StoreBrushes;
    ll.lastLeaf = 0;
    ll.overflowed = false;
    
    CM_BoxLeafnums_r( &ll, 0 );
    
    return ll.count;
}


//====================================================================


/*
==================
idCollisionModelManagerLocal::PointContents
==================
*/
S32 idCollisionModelManagerLocal::PointContents( const vec3_t p, clipHandle_t model )
{
    S32             leafnum, i, k, brushnum, contents;
    cLeaf_t*        leaf;
    cbrush_t*       b;
    F32           d;
    cmodel_t*       clipm;
    
    if ( !cm.numNodes ) // map not loaded
    {
        return 0;
    }
    
    if ( model )
    {
        clipm = CM_ClipHandleToModel( model );
        leaf = &clipm->leaf;
    }
    else
    {
        leafnum = CM_PointLeafnum_r( p, 0 );
        leaf = &cm.leafs[leafnum];
    }
    
    if ( leaf->area == -1 )
    {
        // RB: added this optimization
        // p is in the void and we should return solid so particles can be removed from the void
        return CONTENTS_SOLID;
    }
    
    contents = 0;
    for ( k = 0; k < leaf->numLeafBrushes; k++ )
    {
        brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];
        b = &cm.brushes[brushnum];
        
        if ( !CM_BoundsIntersectPoint( b->bounds[0], b->bounds[1], p ) )
        {
            continue;
        }
        
        // see if the point is in the brush
        for ( i = 0; i < b->numsides; i++ )
        {
            d = DotProduct( p, b->sides[i].plane->normal );
            // FIXME test for Cash
            //          if ( d >= b->sides[i].plane->dist ) {
            if ( d > b->sides[i].plane->dist )
            {
                break;
            }
        }
        
        if ( i == b->numsides )
        {
            contents |= b->contents;
        }
    }
    
    return contents;
}

/*
==================
idCollisionModelManagerLocal::TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
S32 idCollisionModelManagerLocal::TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles )
{
    vec3_t          p_l, temp, forward, right, up;
    
    // subtract origin offset
    VectorSubtract( p, origin, p_l );
    
    // rotate start and end into the models frame of reference
    if ( model != BOX_MODEL_HANDLE && ( angles[0] || angles[1] || angles[2] ) )
    {
        AngleVectors( angles, forward, right, up );
        
        VectorCopy( p_l, temp );
        p_l[0] = DotProduct( temp, forward );
        p_l[1] = -DotProduct( temp, right );
        p_l[2] = DotProduct( temp, up );
    }
    
    return collisionModelManager->PointContents( p_l, model );
}



/*
===============================================================================
PVS
===============================================================================
*/

U8* idCollisionModelManagerLocal::ClusterPVS( S32 cluster )
{
    if ( cluster < 0 || cluster >= cm.numClusters || !cm.vised )
    {
        return cm.visibility;
    }
    
    return cm.visibility + cluster * cm.clusterBytes;
}



/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void CM_FloodArea_r( S32 areaNum, S32 floodnum )
{
    S32             i, *con;
    cArea_t*        area;
    
    area = &cm.areas[areaNum];
    
    if ( area->floodvalid == cm.floodvalid )
    {
        if ( area->floodnum == floodnum )
        {
            return;
        }
        Com_Error( ERR_DROP, "FloodArea_r: reflooded" );
    }
    
    area->floodnum = floodnum;
    area->floodvalid = cm.floodvalid;
    con = cm.areaPortals + areaNum * cm.numAreas;
    for ( i = 0; i < cm.numAreas; i++ )
    {
        if ( con[i] > 0 )
        {
            CM_FloodArea_r( i, floodnum );
        }
    }
}

/*
====================
CM_FloodAreaConnections
====================
*/
void CM_FloodAreaConnections( void )
{
    S32             i, floodnum;
    cArea_t*        area;
    
    // all current floods are now invalid
    cm.floodvalid++;
    floodnum = 0;
    
    area = cm.areas; // Ridah, optimization
    for ( i = 0; i < cm.numAreas; i++, area++ )
    {
        if ( area->floodvalid == cm.floodvalid )
        {
            continue; // already flooded into
        }
        floodnum++;
        CM_FloodArea_r( i, floodnum );
    }
    
}

/*
====================
CM_AdjustAreaPortalState
====================
*/
void idCollisionModelManagerLocal::AdjustAreaPortalState( S32 area1, S32 area2, bool open )
{
    if ( area1 < 0 || area2 < 0 )
    {
        return;
    }
    
    if ( area1 >= cm.numAreas || area2 >= cm.numAreas )
    {
        Com_Error( ERR_DROP, "CM_ChangeAreaPortalState: bad area number" );
    }
    
    if ( open )
    {
        cm.areaPortals[area1 * cm.numAreas + area2]++;
        cm.areaPortals[area2 * cm.numAreas + area1]++;
    }
    else if ( cm.areaPortals[area2 * cm.numAreas + area1] )  // Ridah, fixes loadgame issue
    {
        cm.areaPortals[area1 * cm.numAreas + area2]--;
        cm.areaPortals[area2 * cm.numAreas + area1]--;
        if ( cm.areaPortals[area2 * cm.numAreas + area1] < 0 )
        {
            Com_Error( ERR_DROP, "CM_AdjustAreaPortalState: negative reference count" );
        }
    }
    
    CM_FloodAreaConnections();
}

/*
====================
idCollisionModelManagerLocal::AreasConnected
====================
*/
bool idCollisionModelManagerLocal::AreasConnected( S32 area1, S32 area2 )
{
    if ( cm_noAreas->integer )
    {
        return true;
    }
    
    if ( area1 < 0 || area2 < 0 )
    {
        return false;
    }
    
    if ( area1 >= cm.numAreas || area2 >= cm.numAreas )
    {
        Com_Error( ERR_DROP, "area >= cm.numAreas" );
    }
    
    if ( cm.areas[area1].floodnum == cm.areas[area2].floodnum )
    {
        return true;
    }
    return false;
}


/*
=================
idCollisionModelManagerLocal::WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
S32 idCollisionModelManagerLocal::WriteAreaBits( U8* buffer, S32 area )
{
    S32             i, floodnum, bytes;
    
    bytes = ( cm.numAreas + 7 ) >> 3;
    
#ifndef BSPC
    if ( cm_noAreas->integer || area == -1 )
    {
#else
    if ( area == -1 )
    {
#endif
        // for debugging, send everything
        memset( buffer, 255, bytes );
    }
    else
    {
        floodnum = cm.areas[area].floodnum;
        for ( i = 0; i < cm.numAreas; i++ )
        {
            if ( cm.areas[i].floodnum == floodnum || area == -1 )
            {
                buffer[i >> 3] |= 1 << ( i & 7 );
            }
        }
    }
    
    return bytes;
}

/*
====================
CM_BoundsIntersect
====================
*/
bool CM_BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
    if ( maxs[0] < mins2[0] - SURFACE_CLIP_EPSILON || maxs[1] < mins2[1] - SURFACE_CLIP_EPSILON || maxs[2] < mins2[2] - SURFACE_CLIP_EPSILON || mins[0] > maxs2[0] + SURFACE_CLIP_EPSILON || mins[1] > maxs2[1] + SURFACE_CLIP_EPSILON || mins[2] > maxs2[2] + SURFACE_CLIP_EPSILON )
    {
        return false;
    }
    
    return true;
}

/*
====================
CM_BoundsIntersectPoint
====================
*/
bool CM_BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t point )
{
    if ( maxs[0] < point[0] - SURFACE_CLIP_EPSILON || maxs[1] < point[1] - SURFACE_CLIP_EPSILON || maxs[2] < point[2] - SURFACE_CLIP_EPSILON || mins[0] > point[0] + SURFACE_CLIP_EPSILON || mins[1] > point[1] + SURFACE_CLIP_EPSILON || mins[2] > point[2] + SURFACE_CLIP_EPSILON )
    {
        return false;
    }
    
    return true;
}

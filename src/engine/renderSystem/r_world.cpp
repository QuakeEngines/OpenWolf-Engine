////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_world.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemWorldLocal renderSystemWorldLocal;

/*
===============
idRenderSystemWorldLocal::idRenderSystemWorldLocal
===============
*/
idRenderSystemWorldLocal::idRenderSystemWorldLocal( void )
{
}

/*
===============
idRenderSystemWorldLocal::~idRenderSystemWorldLocal
===============
*/
idRenderSystemWorldLocal::~idRenderSystemWorldLocal( void )
{
}

/*
================
idRenderSystemWorldLocal::CullSurface

Tries to cull surfaces before they are lighted or
added to the sorting list.
================
*/
bool idRenderSystemWorldLocal::CullSurface( msurface_t* surf, S32 entityNum )
{
    if ( r_nocull->integer || surf->cullinfo.type == CULLINFO_NONE )
    {
        return false;
    }
    
    if ( *surf->data == SF_GRID && r_nocurves->integer )
    {
        return true;
    }
    
    if ( surf->cullinfo.type & CULLINFO_PLANE )
    {
        if ( tr.currentModel && tr.currentModel->type == MOD_BRUSH )
        {
            return false;
        }
        
        // Only true for SF_FACE, so treat like its own function
        F32	d;
        cullType_t ct;
        
        if ( !r_facePlaneCull->integer )
        {
            return false;
        }
        
        ct = surf->shader->cullType;
        
        if ( ct == CT_TWO_SIDED )
        {
            return false;
        }
        
        // don't cull for depth shadow
        /*
        if ( tr.viewParms.flags & VPF_DEPTHSHADOW )
        {
        	return false;
        }
        */
        
        // shadowmaps draw back surfaces
        if ( tr.viewParms.flags & ( VPF_SHADOWMAP | VPF_DEPTHSHADOW ) )
        {
            if ( ct == CT_FRONT_SIDED )
            {
                ct = CT_BACK_SIDED;
            }
            else
            {
                ct = CT_FRONT_SIDED;
            }
        }
        
        // do proper cull for orthographic projection
        if ( tr.viewParms.flags & VPF_ORTHOGRAPHIC )
        {
            d = DotProduct( tr.viewParms.orientation.axis[0], surf->cullinfo.plane.normal );
            
            if ( ct == CT_FRONT_SIDED )
            {
                if ( d > 0 )
                {
                    return true;
                }
            }
            else
            {
                if ( d < 0 )
                {
                    return true;
                }
            }
            
            return false;
        }
        
        d = DotProduct( tr.orientation.viewOrigin, surf->cullinfo.plane.normal );
        
        // don't cull exactly on the plane, because there are levels of rounding
        // through the BSP, ICD, and hardware that may cause pixel gaps if an
        // epsilon isn't allowed here
        if ( ct == CT_FRONT_SIDED )
        {
            if ( d < surf->cullinfo.plane.dist - 8 )
            {
                return true;
            }
        }
        else
        {
            if ( d > surf->cullinfo.plane.dist + 8 )
            {
                return true;
            }
        }
        
        return false;
    }
    
    if ( surf->cullinfo.type & CULLINFO_SPHERE )
    {
        S32 sphereCull;
        
        if ( entityNum != REFENTITYNUM_WORLD )
        {
            sphereCull = idRenderSystemMainLocal::CullLocalPointAndRadius( surf->cullinfo.localOrigin, surf->cullinfo.radius );
        }
        else
        {
            sphereCull = idRenderSystemMainLocal::CullPointAndRadius( surf->cullinfo.localOrigin, surf->cullinfo.radius );
        }
        
        if ( sphereCull == CULL_OUT )
        {
            return true;
        }
    }
    
    if ( surf->cullinfo.type & CULLINFO_BOX )
    {
        S32 boxCull;
        
        if ( entityNum != REFENTITYNUM_WORLD )
        {
            boxCull = idRenderSystemMainLocal::CullLocalBox( surf->cullinfo.bounds );
        }
        else
        {
            boxCull = idRenderSystemMainLocal::CullBox( surf->cullinfo.bounds );
        }
        
        if ( boxCull == CULL_OUT )
        {
            return true;
        }
    }
    
    return false;
}

/*
====================
idRenderSystemWorldLocal::DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
S32 idRenderSystemWorldLocal::DlightSurface( msurface_t* surf, S32 dlightBits )
{
    S32 i;
    F32 d;
    dlight_t* dl;
    
    if ( surf->cullinfo.type & CULLINFO_PLANE )
    {
        for ( i = 0 ; i < tr.refdef.num_dlights ; i++ )
        {
            if ( !( dlightBits & ( 1 << i ) ) )
            {
                continue;
            }
            
            dl = &tr.refdef.dlights[i];
            d = DotProduct( dl->origin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
            
            if ( d < -dl->radius || d > dl->radius )
            {
                // dlight doesn't reach the plane
                dlightBits &= ~( 1 << i );
            }
        }
    }
    
    if ( surf->cullinfo.type & CULLINFO_BOX )
    {
        for ( i = 0 ; i < tr.refdef.num_dlights ; i++ )
        {
            if ( !( dlightBits & ( 1 << i ) ) )
            {
                continue;
            }
            
            dl = &tr.refdef.dlights[i];
            
            if ( dl->origin[0] - dl->radius > surf->cullinfo.bounds[1][0]
                    || dl->origin[0] + dl->radius < surf->cullinfo.bounds[0][0]
                    || dl->origin[1] - dl->radius > surf->cullinfo.bounds[1][1]
                    || dl->origin[1] + dl->radius < surf->cullinfo.bounds[0][1]
                    || dl->origin[2] - dl->radius > surf->cullinfo.bounds[1][2]
                    || dl->origin[2] + dl->radius < surf->cullinfo.bounds[0][2] )
            {
                // dlight doesn't reach the bounds
                dlightBits &= ~( 1 << i );
            }
        }
    }
    
    if ( surf->cullinfo.type & CULLINFO_SPHERE )
    {
        for ( i = 0 ; i < tr.refdef.num_dlights ; i++ )
        {
            if ( !( dlightBits & ( 1 << i ) ) )
            {
                continue;
            }
            
            dl = &tr.refdef.dlights[i];
            
            if ( !idRenderSystemMathsLocal::SpheresIntersect( dl->origin, dl->radius, surf->cullinfo.localOrigin, surf->cullinfo.radius ) )
            {
                // dlight doesn't reach the bounds
                dlightBits &= ~( 1 << i );
            }
        }
    }
    
    switch ( *surf->data )
    {
        case SF_FACE:
        case SF_GRID:
        case SF_TRIANGLES:
            ( ( srfBspSurface_t* )surf->data )->dlightBits = dlightBits;
            break;
            
        default:
            dlightBits = 0;
            break;
    }
    
    if ( dlightBits )
    {
        tr.pc.c_dlightSurfaces++;
    }
    else
    {
        tr.pc.c_dlightSurfacesCulled++;
    }
    
    return dlightBits;
}

/*
====================
idRenderSystemWorldLocal::PshadowSurface

Just like idRenderSystemWorldLocal::DlightSurface, cull any we can
====================
*/
S32 idRenderSystemWorldLocal::PshadowSurface( msurface_t* surf, S32 pshadowBits )
{
    S32 i;
    F32 d;
    pshadow_t* ps;
    
    if ( surf->cullinfo.type & CULLINFO_PLANE )
    {
        for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ )
        {
            if ( !( pshadowBits & ( 1 << i ) ) )
            {
                continue;
            }
            
            ps = &tr.refdef.pshadows[i];
            d = DotProduct( ps->lightOrigin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
            
            if ( d < -ps->lightRadius || d > ps->lightRadius )
            {
                // pshadow doesn't reach the plane
                pshadowBits &= ~( 1 << i );
            }
        }
    }
    
    if ( surf->cullinfo.type & CULLINFO_BOX )
    {
        for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ )
        {
            if ( !( pshadowBits & ( 1 << i ) ) )
            {
                continue;
            }
            ps = &tr.refdef.pshadows[i];
            if ( ps->lightOrigin[0] - ps->lightRadius > surf->cullinfo.bounds[1][0]
                    || ps->lightOrigin[0] + ps->lightRadius < surf->cullinfo.bounds[0][0]
                    || ps->lightOrigin[1] - ps->lightRadius > surf->cullinfo.bounds[1][1]
                    || ps->lightOrigin[1] + ps->lightRadius < surf->cullinfo.bounds[0][1]
                    || ps->lightOrigin[2] - ps->lightRadius > surf->cullinfo.bounds[1][2]
                    || ps->lightOrigin[2] + ps->lightRadius < surf->cullinfo.bounds[0][2]
                    || collisionModelManager->BoxOnPlaneSide( surf->cullinfo.bounds[0], surf->cullinfo.bounds[1], &ps->cullPlane ) == 2 )
            {
                // pshadow doesn't reach the bounds
                pshadowBits &= ~( 1 << i );
            }
        }
    }
    
    if ( surf->cullinfo.type & CULLINFO_SPHERE )
    {
        for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ )
        {
            if ( !( pshadowBits & ( 1 << i ) ) )
            {
                continue;
            }
            
            ps = &tr.refdef.pshadows[i];
            
            if ( !idRenderSystemMathsLocal::SpheresIntersect( ps->viewOrigin, ps->viewRadius, surf->cullinfo.localOrigin, surf->cullinfo.radius )
                    || DotProduct( surf->cullinfo.localOrigin, ps->cullPlane.normal ) - ps->cullPlane.dist < -surf->cullinfo.radius )
            {
                // pshadow doesn't reach the bounds
                pshadowBits &= ~( 1 << i );
            }
        }
    }
    
    switch ( *surf->data )
    {
        case SF_FACE:
        case SF_GRID:
        case SF_TRIANGLES:
            ( ( srfBspSurface_t* )surf->data )->pshadowBits = pshadowBits;
            break;
            
        default:
            pshadowBits = 0;
            break;
    }
    
    if ( pshadowBits )
    {
        //tr.pc.c_dlightSurfaces++;
    }
    
    return pshadowBits;
}

/*
================
idRenderSystemWorldLocal::IsPostRenderEntity
================
*/
bool idRenderSystemWorldLocal::IsPostRenderEntity( S32 refEntityNum, const trRefEntity_t* refEntity )
{
    if ( refEntityNum == REFENTITYNUM_WORLD )
    {
        return false;
    }
    
    return ( refEntity->e.renderfx );
}

/*
======================
idRenderSystemWorldLocal::AddWorldSurface
======================
*/
void idRenderSystemWorldLocal::AddWorldSurface( msurface_t* surf, S32 entityNum, S32 dlightBits, S32 pshadowBits, bool dontCache )
{
    // FIXME: bmodel fog?
    S32 cubemapIndex = 0;
    
    // try to cull before dlighting or adding
    if ( CullSurface( surf, entityNum ) )
    {
        return;
    }
    
    // check for dlighting
    /*if ( dlightBits ) */
    {
        dlightBits = DlightSurface( surf, dlightBits );
        dlightBits = ( dlightBits != 0 );
    }
    
    // check for pshadows
    /*if ( pshadowBits ) */
    {
        pshadowBits = PshadowSurface( surf, pshadowBits );
        pshadowBits = ( pshadowBits != 0 );
    }
    
    idRenderSystemMainLocal::AddDrawSurf( surf->data, surf->shader, surf->fogIndex, dlightBits, IsPostRenderEntity( tr.currentEntityNum, tr.currentEntity ), surf->cubemapIndex );
}

/*
=================
idRenderSystemWorldLocal::AddBrushModelSurfaces
=================
*/
void idRenderSystemWorldLocal::AddBrushModelSurfaces( trRefEntity_t* ent )
{
    S32 i, clip;
    bmodel_t* bmodel;
    model_t* pModel;
    
    pModel = idRenderSystemModelLocal::GetModelByHandle( ent->e.hModel );
    bmodel = pModel->bmodel;
    
    clip = idRenderSystemMainLocal::CullLocalBox( bmodel->bounds );
    if ( clip == CULL_OUT )
    {
        return;
    }
    
    idRenderSystemLightLocal::SetupEntityLighting( &tr.refdef, ent );
    idRenderSystemLightLocal::DlightBmodel( bmodel );
    
    for ( i = 0 ; i < bmodel->numSurfaces ; i++ )
    {
        S32 surf = bmodel->firstSurface + i;
        
        if ( tr.world->surfacesViewCount[surf] != tr.viewCount )
        {
            tr.world->surfacesViewCount[surf] = tr.viewCount;
            AddWorldSurface( tr.world->surfaces + surf, tr.currentEntityNum, tr.currentEntity->needDlights, 0, true );
        }
    }
}

/*
================
idRenderSystemWorldLocal::RecursiveWorldNode
================
*/
void idRenderSystemWorldLocal::RecursiveWorldNode( mnode_t* node, U32 planeBits, U32 dlightBits, U32 pshadowBits )
{
    do
    {
        U32 newDlights[2], newPShadows[2];
        
        // if the node wasn't marked as potentially visible, exit
        // pvs is skipped for depth shadows
        if ( !( tr.viewParms.flags & VPF_DEPTHSHADOW ) && node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex] )
        {
            return;
        }
        
        // if the bounding volume is outside the frustum, nothing
        // inside can be visible OPTIMIZE: don't do this all the way to leafs?
        
        if ( !r_nocull->integer )
        {
            S32		r;
            
            if ( planeBits & 1 )
            {
                r = collisionModelManager->BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[0] );
                
                if ( r == 2 )
                {
                    // culled
                    return;
                }
                
                if ( r == 1 )
                {
                    // all descendants will also be in front
                    planeBits &= ~1;
                }
            }
            
            if ( planeBits & 2 )
            {
                r = collisionModelManager->BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[1] );
                if ( r == 2 )
                {
                    // culled
                    return;
                }
                
                if ( r == 1 )
                {
                    // all descendants will also be in front
                    planeBits &= ~2;
                }
            }
            
            if ( planeBits & 4 )
            {
                r = collisionModelManager->BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[2] );
                if ( r == 2 )
                {
                    // culled
                    return;
                }
                
                if ( r == 1 )
                {
                    // all descendants will also be in front
                    planeBits &= ~4;
                }
            }
            
            if ( planeBits & 8 )
            {
                r = collisionModelManager->BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[3] );
                
                if ( r == 2 )
                {
                    // culled
                    return;
                }
                
                if ( r == 1 )
                {
                    // all descendants will also be in front
                    planeBits &= ~8;
                }
            }
            
            if ( planeBits & 16 )
            {
                r = collisionModelManager->BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[4] );
                if ( r == 2 )
                {
                    // culled
                    return;
                }
                
                if ( r == 1 )
                {
                    // all descendants will also be in front
                    planeBits &= ~16;
                }
            }
        }
        
        if ( node->contents != -1 )
        {
            break;
        }
        
        // node is just a decision point, so go down both sides
        // since we don't care about sort orders, just go positive to negative
        
        // determine which dlights are needed
        newDlights[0] = 0;
        newDlights[1] = 0;
        if ( dlightBits )
        {
            S32	i;
            
            for ( i = 0; i < tr.refdef.num_dlights; i++ )
            {
                dlight_t* dl;
                F32		dist;
                
                if ( dlightBits & ( 1 << i ) )
                {
                    dl = &tr.refdef.dlights[i];
                    dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;
                    
                    if ( dist > -dl->radius )
                    {
                        newDlights[0] |= ( 1 << i );
                    }
                    if ( dist < dl->radius )
                    {
                        newDlights[1] |= ( 1 << i );
                    }
                }
            }
        }
        
        newPShadows[0] = 0;
        newPShadows[1] = 0;
        
        if ( pshadowBits )
        {
            S32	i;
            
            for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ )
            {
                F32 dist;
                pshadow_t*	shadow;
                
                if ( pshadowBits & ( 1 << i ) )
                {
                    shadow = &tr.refdef.pshadows[i];
                    dist = DotProduct( shadow->lightOrigin, node->plane->normal ) - node->plane->dist;
                    
                    if ( dist > -shadow->lightRadius )
                    {
                        newPShadows[0] |= ( 1 << i );
                    }
                    
                    if ( dist < shadow->lightRadius )
                    {
                        newPShadows[1] |= ( 1 << i );
                    }
                }
            }
        }
        
        // recurse down the children, front side first
        RecursiveWorldNode( node->children[0], planeBits, newDlights[0], newPShadows[0] );
        
        // tail recurse
        node = node->children[1];
        dlightBits = newDlights[1];
        pshadowBits = newPShadows[1];
    }
    while ( 1 );
    
    {
        // leaf node, so add mark surfaces
        S32 c, surf, *view;
        
        tr.pc.c_leafs++;
        
        // add to z buffer bounds
        if ( node->mins[0] < tr.viewParms.visBounds[0][0] )
        {
            tr.viewParms.visBounds[0][0] = node->mins[0];
        }
        
        if ( node->mins[1] < tr.viewParms.visBounds[0][1] )
        {
            tr.viewParms.visBounds[0][1] = node->mins[1];
        }
        
        if ( node->mins[2] < tr.viewParms.visBounds[0][2] )
        {
            tr.viewParms.visBounds[0][2] = node->mins[2];
        }
        
        if ( node->maxs[0] > tr.viewParms.visBounds[1][0] )
        {
            tr.viewParms.visBounds[1][0] = node->maxs[0];
        }
        
        if ( node->maxs[1] > tr.viewParms.visBounds[1][1] )
        {
            tr.viewParms.visBounds[1][1] = node->maxs[1];
        }
        
        if ( node->maxs[2] > tr.viewParms.visBounds[1][2] )
        {
            tr.viewParms.visBounds[1][2] = node->maxs[2];
        }
        
        // add surfaces
        view = tr.world->marksurfaces + node->firstmarksurface;
        
        c = node->nummarksurfaces;
        while ( c-- )
        {
            // just mark it as visible, so we don't jump out of the cache derefencing the surface
            surf = *view;
            
            if ( tr.world->surfacesViewCount[surf] != tr.viewCount )
            {
                tr.world->surfacesViewCount[surf] = tr.viewCount;
                tr.world->surfacesDlightBits[surf] = dlightBits;
                tr.world->surfacesPshadowBits[surf] = pshadowBits;
            }
            else
            {
                tr.world->surfacesDlightBits[surf] |= dlightBits;
                tr.world->surfacesPshadowBits[surf] |= pshadowBits;
            }
            
            view++;
        }
    }
}

/*
===============
idRenderSystemWorldLocal::PointInLeaf
===============
*/
mnode_t* idRenderSystemWorldLocal::PointInLeaf( const vec3_t p )
{
    F32 d;
    mnode_t* node;
    cplane_t* plane;
    
    if ( !tr.world )
    {
        Com_Error( ERR_DROP, "idRenderSystemWorldLocal::PointInLeaf: bad model" );
    }
    
    node = tr.world->nodes;
    while ( 1 )
    {
        if ( node->contents != -1 )
        {
            break;
        }
        
        plane = node->plane;
        d = DotProduct( p, plane->normal ) - plane->dist;
        
        if ( d > 0 )
        {
            node = node->children[0];
        }
        else
        {
            node = node->children[1];
        }
    }
    
    return node;
}

/*
==============
idRenderSystemWorldLocal::ClusterPVS
==============
*/
const U8* idRenderSystemWorldLocal::ClusterPVS( S32 cluster )
{
    if ( !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters )
    {
        return NULL;
    }
    
    return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
idRenderSystemLocal::inPVS
=================
*/
bool idRenderSystemLocal::inPVS( const vec3_t p1, const vec3_t p2 )
{
    mnode_t* leaf;
    U8*	vis;
    
    leaf = renderSystemWorldLocal.PointInLeaf( p1 );
    // why not idRenderSystemWorldLocal::ClusterPVS ??
    vis = collisionModelManager->ClusterPVS( leaf->cluster );
    leaf = renderSystemWorldLocal.PointInLeaf( p2 );
    
    if ( !( vis[leaf->cluster >> 3] & ( 1 << ( leaf->cluster & 7 ) ) ) )
    {
        return false;
    }
    
    return true;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void idRenderSystemWorldLocal::MarkLeaves( void )
{
    S32 i, cluster;
    const U8*	vis;
    mnode_t*	leaf, *parent;
    
    // lockpvs lets designers walk around to determine the
    // extent of the current pvs
    if ( r_lockpvs->integer )
    {
        return;
    }
    
    // current viewcluster
    leaf = PointInLeaf( tr.viewParms.pvsOrigin );
    cluster = leaf->cluster;
    
    // if the cluster is the same and the area visibility matrix
    // hasn't changed, we don't need to mark everything again
    
    for ( i = 0; i < MAX_VISCOUNTS; i++ )
    {
        // if the areamask or r_showcluster was modified, invalidate all visclusters
        // this caused doors to open into undrawn areas
        if ( tr.refdef.areamaskModified || r_showcluster->modified )
        {
            tr.visClusters[i] = -2;
        }
        else if ( tr.visClusters[i] == cluster )
        {
            if ( tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showcluster->integer )
            {
                clientMainSystem->RefPrintf( PRINT_ALL, "found cluster:%i  area:%i  index:%i\n", cluster, leaf->area, i );
            }
            
            tr.visIndex = i;
            
            return;
        }
    }
    
    tr.visIndex = ( tr.visIndex + 1 ) % MAX_VISCOUNTS;
    tr.visCounts[tr.visIndex]++;
    tr.visClusters[tr.visIndex] = cluster;
    
    if ( r_showcluster->modified || r_showcluster->integer )
    {
        r_showcluster->modified = false;
        
        if ( r_showcluster->integer )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area );
        }
    }
    
    vis = ClusterPVS( tr.visClusters[tr.visIndex] );
    
    for ( i = 0, leaf = tr.world->nodes ; i < tr.world->numnodes ; i++, leaf++ )
    {
        cluster = leaf->cluster;
        if ( cluster < 0 || cluster >= tr.world->numClusters )
        {
            continue;
        }
        
        // check general pvs
        if ( vis && !( vis[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) )
        {
            continue;
        }
        
        // check for door connection
        if ( ( tr.refdef.areamask[leaf->area >> 3] & ( 1 << ( leaf->area & 7 ) ) ) )
        {
            // not visible
            continue;
        }
        
        parent = leaf;
        do
        {
            if ( parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex] )
            {
                break;
            }
            
            parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
            parent = parent->parent;
        }
        while ( parent );
    }
}


/*
=============
idRenderSystemWorldLocal::AddWorldSurfaces
=============
*/
void idRenderSystemWorldLocal::AddWorldSurfaces( void )
{
    U32 planeBits, dlightBits, pshadowBits;
    
    if ( !r_drawworld->integer )
    {
        return;
    }
    
    if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
    {
        return;
    }
    
    tr.currentEntityNum = REFENTITYNUM_WORLD;
    tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;
    
    // determine which leaves are in the PVS / areamask
    if ( !( tr.viewParms.flags & VPF_DEPTHSHADOW ) )
    {
        MarkLeaves();
    }
    
    // clear out the visible min/max
    ClearBounds( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1] );
    
    // perform frustum culling and flag all the potentially visible surfaces
    if ( tr.refdef.num_dlights > MAX_DLIGHTS )
    {
        tr.refdef.num_dlights = MAX_DLIGHTS ;
    }
    
    if ( tr.refdef.num_pshadows > MAX_DRAWN_PSHADOWS )
    {
        tr.refdef.num_pshadows = MAX_DRAWN_PSHADOWS;
    }
    
    planeBits = ( tr.viewParms.flags & VPF_FARPLANEFRUSTUM ) ? 31 : 15;
    
    if ( tr.viewParms.flags & VPF_DEPTHSHADOW )
    {
        dlightBits = 0;
        pshadowBits = 0;
    }
    else if ( !( tr.viewParms.flags & VPF_SHADOWMAP ) )
    {
        dlightBits = ( 1ULL << tr.refdef.num_dlights ) - 1;
        pshadowBits = ( 1ULL << tr.refdef.num_pshadows ) - 1;
    }
    else
    {
        dlightBits = ( 1ULL << tr.refdef.num_dlights ) - 1;
        pshadowBits = 0;
    }
    
    RecursiveWorldNode( tr.world->nodes, planeBits, dlightBits, pshadowBits );
    
    // now add all the potentially visible surfaces
    // also mask invisible dlights for next frame
    {
        S32 i;
        
        tr.refdef.dlightMask = 0;
        
        for ( i = 0; i < tr.world->numWorldSurfaces; i++ )
        {
            if ( tr.world->surfacesViewCount[i] != tr.viewCount )
            {
                continue;
            }
            
            AddWorldSurface( tr.world->surfaces + i, tr.currentEntityNum, tr.world->surfacesDlightBits[i], tr.world->surfacesPshadowBits[i], true );
            tr.refdef.dlightMask |= tr.world->surfacesDlightBits[i];
        }
        
        tr.refdef.dlightMask = ~tr.refdef.dlightMask;
    }
}

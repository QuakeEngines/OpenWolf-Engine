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
// File name:   r_animation.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemAnimationLocal renderSystemAnimationLocal;

/*
===============
idRenderSystemAnimationLocal::idRenderSystemAnimationLocal
===============
*/
idRenderSystemAnimationLocal::idRenderSystemAnimationLocal( void )
{
}

/*
===============
idRenderSystemAnimationLocal::~idRenderSystemAnimationLocal
===============
*/
idRenderSystemAnimationLocal::~idRenderSystemAnimationLocal( void )
{
}

/*
=============
idRenderSystemAnimationLocal::MDRCullModel
=============
*/
S32 idRenderSystemAnimationLocal::MDRCullModel( mdrHeader_t* header, trRefEntity_t* ent )
{
    S32 i;
    size_t frameSize;
    vec3_t bounds[2];
    mdrFrame_t*	oldFrame, *newFrame;
    
    frameSize = ( size_t )( &( ( mdrFrame_t* )0 )->bones[ header->numBones ] );
    
    // compute frame pointers
    newFrame = ( mdrFrame_t* )( ( U8* ) header + header->ofsFrames + frameSize * ent->e.frame );
    oldFrame = ( mdrFrame_t* )( ( U8* ) header + header->ofsFrames + frameSize * ent->e.oldframe );
    
    // cull bounding sphere ONLY if this is not an upscaled entity
    if ( !ent->e.nonNormalizedAxes )
    {
        if ( ent->e.frame == ent->e.oldframe )
        {
            switch ( idRenderSystemMainLocal::CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
            {
                    // Ummm... yeah yeah I know we don't really have an md3 here.. but we pretend
                    // we do. After all, the purpose of mdrs are not that different, are they?
                case CULL_OUT:
                    tr.pc.c_sphere_cull_md3_out++;
                    return CULL_OUT;
                    
                case CULL_IN:
                    tr.pc.c_sphere_cull_md3_in++;
                    return CULL_IN;
                    
                case CULL_CLIP:
                    tr.pc.c_sphere_cull_md3_clip++;
                    break;
            }
        }
        else
        {
            S32 sphereCull, sphereCullB;
            
            sphereCull  = idRenderSystemMainLocal::CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
            if ( newFrame == oldFrame )
            {
                sphereCullB = sphereCull;
            }
            else
            {
                sphereCullB = idRenderSystemMainLocal::CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
            }
            
            if ( sphereCull == sphereCullB )
            {
                if ( sphereCull == CULL_OUT )
                {
                    tr.pc.c_sphere_cull_md3_out++;
                    return CULL_OUT;
                }
                else if ( sphereCull == CULL_IN )
                {
                    tr.pc.c_sphere_cull_md3_in++;
                    return CULL_IN;
                }
                else
                {
                    tr.pc.c_sphere_cull_md3_clip++;
                }
            }
        }
    }
    
    // calculate a bounding box in the current coordinate system
    for ( i = 0 ; i < 3 ; i++ )
    {
        bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
        bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
    }
    
    switch ( idRenderSystemMainLocal::CullLocalBox( bounds ) )
    {
        case CULL_IN:
            tr.pc.c_box_cull_md3_in++;
            return CULL_IN;
        case CULL_CLIP:
            tr.pc.c_box_cull_md3_clip++;
            return CULL_CLIP;
        case CULL_OUT:
        default:
            tr.pc.c_box_cull_md3_out++;
            return CULL_OUT;
    }
}

/*
=================
idRenderSystemAnimationLocal::MDRComputeFogNum
=================
*/
S32 idRenderSystemAnimationLocal::MDRComputeFogNum( mdrHeader_t* header, trRefEntity_t* ent )
{
    S32 i, j;
    size_t frameSize;
    fog_t* fog;
    mdrFrame_t* mdrFrame;
    vec3_t localOrigin;
    
    if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
    {
        return 0;
    }
    
    frameSize = ( size_t )( &( ( mdrFrame_t* )0 )->bones[ header->numBones ] );
    
    // FIXME: non-normalized axis issues
    mdrFrame = ( mdrFrame_t* )( ( U8* ) header + header->ofsFrames + frameSize * ent->e.frame );
    VectorAdd( ent->e.origin, mdrFrame->localOrigin, localOrigin );
    for ( i = 1 ; i < tr.world->numfogs ; i++ )
    {
        fog = &tr.world->fogs[i];
        for ( j = 0 ; j < 3 ; j++ )
        {
            if ( localOrigin[j] - mdrFrame->radius >= fog->bounds[1][j] )
            {
                break;
            }
            
            if ( localOrigin[j] + mdrFrame->radius <= fog->bounds[0][j] )
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
==============
idRenderSystemAnimationLocal::MDRAddAnimSurfaces
==============
*/
void idRenderSystemAnimationLocal::MDRAddAnimSurfaces( trRefEntity_t* ent )
{
    S32	i, j, lodnum = 0, fogNum = 0, cull,  cubemapIndex;
    mdrHeader_t* header;
    mdrSurface_t* surface;
    mdrLOD_t* lod;
    shader_t* shader;
    skin_t*	 skin;
    bool	personalModel;
    
    header = ( mdrHeader_t* ) tr.currentModel->modelData;
    
    personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !( tr.viewParms.isPortal || ( tr.viewParms.flags & ( VPF_SHADOWMAP | VPF_DEPTHSHADOW ) ) );
    
    if ( ent->e.renderfx & RF_WRAP_FRAMES )
    {
        ent->e.frame %= header->numFrames;
        ent->e.oldframe %= header->numFrames;
    }
    
    // Validate the frames so there is no chance of a crash.
    // This will write directly into the entity structure, so
    // when the surfaces are rendered, they don't need to be
    // range checked again.
    if ( ( ent->e.frame >= header->numFrames ) || ( ent->e.frame < 0 ) || ( ent->e.oldframe >= header->numFrames ) || ( ent->e.oldframe < 0 ) )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "R_MDRAddAnimSurfaces: no such frame %d to %d for '%s'\n", ent->e.oldframe, ent->e.frame, tr.currentModel->name );
        ent->e.frame = 0;
        ent->e.oldframe = 0;
    }
    
    // cull the entire model if merged bounding box of both frames
    // is outside the view frustum.
    cull = MDRCullModel( header, ent );
    if ( cull == CULL_OUT )
    {
        return;
    }
    
    // figure out the current LOD of the model we're rendering, and set the lod pointer respectively.
    lodnum = idRenderSystemMeshLocal::ComputeLOD( ent );
    
    // check whether this model has as that many LODs at all. If not, try the closest thing we got.
    if ( header->numLODs <= 0 )
    {
        return;
    }
    
    if ( header->numLODs <= lodnum )
    {
        lodnum = header->numLODs - 1;
    }
    
    lod = ( mdrLOD_t* )( ( U8* )header + header->ofsLODs );
    for ( i = 0; i < lodnum; i++ )
    {
        lod = ( mdrLOD_t* )( ( U8* ) lod + lod->ofsEnd );
    }
    
    // set up lighting
    if ( !personalModel || r_shadows->integer > 1 )
    {
        idRenderSystemLightLocal::SetupEntityLighting( &tr.refdef, ent );
    }
    
    // fogNum?
    fogNum = MDRComputeFogNum( header, ent );
    
    cubemapIndex = idRenderSystemLightLocal::CubemapForPoint( ent->e.origin );
    
    surface = ( mdrSurface_t* )( ( U8* )lod + lod->ofsSurfaces );
    
    for ( i = 0 ; i < lod->numSurfaces ; i++ )
    {
    
        if ( ent->e.customShader )
        {
            shader = idRenderSystemShaderLocal::GetShaderByHandle( ent->e.customShader );
        }
        else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
        {
            skin = idRenderSystemSkinsLocal::GetSkinByHandle( ent->e.customSkin );
            shader = tr.defaultShader;
            
            for ( j = 0; j < skin->numSurfaces; j++ )
            {
                if ( !strcmp( skin->surfaces[j].name, surface->name ) )
                {
                    shader = skin->surfaces[j].shader;
                    break;
                }
            }
        }
        else if ( surface->shaderIndex > 0 )
        {
            shader = idRenderSystemShaderLocal::GetShaderByHandle( surface->shaderIndex );
        }
        else
        {
            shader = tr.defaultShader;
        }
        
        // skip all surfaces that don't matter for lighting only pass
        if ( shader->surfaceFlags & ( SURF_NODLIGHT | SURF_SKY ) )
        {
            continue;
        }
        
        // we will add shadows even if the main object isn't visible in the view
        
        // stencil shadows can't do personal models unless I polyhedron clip
        if ( !personalModel
                && r_shadows->integer == 2
                && fogNum == 0
                && !( ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
                && shader->sort == SS_OPAQUE )
        {
            idRenderSystemMainLocal::AddDrawSurf( ( surfaceType_t* )surface, tr.shadowShader, 0, false, false, 0 );
        }
        
        // projection shadows work fine with personal models
        if ( r_shadows->integer == 3
                && fogNum == 0
                && ( ent->e.renderfx & RF_SHADOW_PLANE )
                && shader->sort == SS_OPAQUE )
        {
            idRenderSystemMainLocal::AddDrawSurf( ( surfaceType_t* )surface, tr.projectionShadowShader, 0, false, false, 0 );
        }
        
        if ( !personalModel )
        {
            idRenderSystemMainLocal::AddDrawSurf( ( surfaceType_t* )surface, shader, fogNum, false, false, cubemapIndex );
        }
        
        surface = ( mdrSurface_t* )( ( U8* )surface + surface->ofsEnd );
    }
}

/*
==============
idRenderSystemAnimationLocal::MDRSurfaceAnim
==============
*/
void idRenderSystemAnimationLocal::MDRSurfaceAnim( mdrSurface_t* surface )
{
    S32 i, j, k, * triangles, indexes, baseIndex, baseVertex, numVerts;
    size_t frameSize;
    F32 frontlerp, backlerp;
    mdrVertex_t* v;
    mdrHeader_t* header;
    mdrFrame_t* frame, * oldFrame;
    mdrBone_t bones[MDR_MAX_BONES], *bonePtr, *bone;
    
    // don't lerp if lerping off, or this is the only frame, or the last frame...
    if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame )
    {
        // if backlerp is 0, lerping is off and frontlerp is never used
        backlerp = 0;
        frontlerp = 1;
    }
    else
    {
        backlerp	= backEnd.currentEntity->e.backlerp;
        frontlerp	= 1.0f - backlerp;
    }
    
    header = ( mdrHeader_t* )( ( U8* )surface + surface->ofsHeader );
    
    frameSize = ( size_t )( &( ( mdrFrame_t* )0 )->bones[ header->numBones ] );
    
    frame = ( mdrFrame_t* )( ( U8* )header + header->ofsFrames + backEnd.currentEntity->e.frame * frameSize );
    oldFrame = ( mdrFrame_t* )( ( U8* )header + header->ofsFrames + backEnd.currentEntity->e.oldframe * frameSize );
    
    idRenderSystemSurfaceLocal::CheckOverflow( surface->numVerts, surface->numTriangles * 3 );
    
    triangles = ( S32* )( ( U8* )surface + surface->ofsTriangles );
    indexes	= surface->numTriangles * 3;
    baseIndex = tess.numIndexes;
    baseVertex = tess.numVertexes;
    
    // Set up all triangles.
    for ( j = 0 ; j < indexes ; j++ )
    {
        tess.indexes[baseIndex + j] = baseVertex + triangles[j];
    }
    tess.numIndexes += indexes;
    
    // lerp all the needed bones
    if ( !backlerp )
    {
        // no lerping needed
        bonePtr = frame->bones;
    }
    else
    {
        bonePtr = bones;
        
        for ( i = 0 ; i < header->numBones * 12 ; i++ )
        {
            ( ( F32* )bonePtr )[i] = frontlerp * ( ( F32* )frame->bones )[i] + backlerp * ( ( F32* )oldFrame->bones )[i];
        }
    }
    
    // deform the vertexes by the lerped bones
    numVerts = surface->numVerts;
    v = ( mdrVertex_t* )( ( U8* )surface + surface->ofsVerts );
    
    for ( j = 0; j < numVerts; j++ )
    {
        vec3_t tempVert, tempNormal;
        mdrWeight_t* w;
        
        VectorClear( tempVert );
        VectorClear( tempNormal );
        
        w = v->weights;
        
        for ( k = 0 ; k < v->numWeights ; k++, w++ )
        {
            bone = bonePtr + w->boneIndex;
            
            tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
            tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
            tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );
            
            tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
            tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
            tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
        }
        
        tess.xyz[baseVertex + j][0] = tempVert[0];
        tess.xyz[baseVertex + j][1] = tempVert[1];
        tess.xyz[baseVertex + j][2] = tempVert[2];
        
        idRenderSystemVaoLocal::VaoPackNormal( tess.normal[baseVertex + j], tempNormal );
        
        tess.texCoords[baseVertex + j][0] = v->texCoords[0];
        tess.texCoords[baseVertex + j][1] = v->texCoords[1];
        
        v = ( mdrVertex_t* )&v->weights[v->numWeights];
    }
    
    tess.numVertexes += surface->numVerts;
}

void idRenderSystemAnimationLocal::UnCompress( F32 mat[3][4], const U8* comp )
{
    S32 val;
    
    val = ( S32 )( ( U16* )( comp ) )[0];
    val -= 1 << ( MC_BITS_X - 1 );
    mat[0][3] = ( ( F32 )( val ) ) * MC_SCALE_X;
    
    val = ( S32 )( ( U16* )( comp ) )[1];
    val -= 1 << ( MC_BITS_Y - 1 );
    mat[1][3] = ( ( F32 )( val ) ) * MC_SCALE_Y;
    
    val = ( S32 )( ( U16* )( comp ) )[2];
    val -= 1 << ( MC_BITS_Z - 1 );
    mat[2][3] = ( ( F32 )( val ) ) * MC_SCALE_Z;
    
    val = ( S32 )( ( U16* )( comp ) )[3];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[0][0] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    val = ( S32 )( ( U16* )( comp ) )[4];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[0][1] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    val = ( S32 )( ( U16* )( comp ) )[5];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[0][2] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    
    val = ( S32 )( ( U16* )( comp ) )[6];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[1][0] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    val = ( S32 )( ( U16* )( comp ) )[7];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[1][1] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    val = ( S32 )( ( U16* )( comp ) )[8];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[1][2] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    
    val = ( S32 )( ( U16* )( comp ) )[9];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[2][0] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    val = ( S32 )( ( U16* )( comp ) )[10];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[2][1] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
    
    val = ( S32 )( ( U16* )( comp ) )[11];
    val -= 1 << ( MC_BITS_VECT - 1 );
    mat[2][2] = ( ( F32 )( val ) ) * MC_SCALE_VECT;
}

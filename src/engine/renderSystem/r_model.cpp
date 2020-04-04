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
// File name:   r_models.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: model loading and caching
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

#define	LL(x) x=LittleLong(x)

idRenderSystemModelLocal renderSystemModelLocal;

/*
===============
idRenderSystemModelLocal::idRenderSystemModelLocal
===============
*/
idRenderSystemModelLocal::idRenderSystemModelLocal( void )
{
}

/*
===============
idRenderSystemModelLocal::~idRenderSystemModelLocal
===============
*/
idRenderSystemModelLocal::~idRenderSystemModelLocal( void )
{
}

/*
====================
idRenderSystemModelLocal::RegisterMD3
====================
*/
qhandle_t idRenderSystemModelLocal::RegisterMD3( StringEntry name, model_t* mod )
{
    union
    {
        U32* u;
        void* v;
    } buf;
    S32 size, lod, ident, numLoaded;
    bool loaded = false;
    UTF8 filename[MAX_QPATH], namebuf[MAX_QPATH + 20], * fext, defex[] = "md3";
    
    numLoaded = 0;
    
    ::strcpy( filename, name );
    
    fext = ::strchr( filename, '.' );
    if ( !fext )
    {
        fext = defex;
    }
    else
    {
        *fext = '\0';
        fext++;
    }
    
    for ( lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod-- )
    {
        if ( lod )
        {
            Q_snprintf( namebuf, sizeof( namebuf ), "%s_%d.%s", filename, lod, fext );
        }
        else
        {
            Q_snprintf( namebuf, sizeof( namebuf ), "%s.%s", filename, fext );
        }
        
        size = fileSystem->ReadFile( namebuf, &buf.v );
        if ( !buf.u )
        {
            continue;
        }
        
        ident = LittleLong( * ( U32* ) buf.u );
        if ( ident == MD3_IDENT )
        {
            loaded = LoadMD3( mod, lod, buf.u, size, name );
        }
        else
        {
            clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::RegisterMD3: unknown fileid for %s\n", name );
        }
        
        fileSystem->FreeFile( buf.v );
        
        if ( loaded )
        {
            mod->numLods++;
            numLoaded++;
        }
        else
        {
            break;
        }
    }
    
    if ( numLoaded )
    {
        // duplicate into higher lod spots that weren't
        // loaded, in case the user changes r_lodbias on the fly
        for ( lod--; lod >= 0; lod-- )
        {
            mod->numLods++;
            mod->mdv[lod] = mod->mdv[lod + 1];
        }
        
        return mod->index;
    }
    
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "idRenderSystemModelLocal::RegisterMD3: couldn't load %s\n", name );
    
    mod->type = MOD_BAD;
    return 0;
}

/*
====================
idRenderSystemModelLocal::RegisterMDR
====================
*/
qhandle_t idRenderSystemModelLocal::RegisterMDR( StringEntry name, model_t* mod )
{
    union
    {
        U32* u;
        void* v;
    } buf;
    S32	ident, filesize;
    bool loaded = false;
    
    filesize = fileSystem->ReadFile( name, ( void** ) &buf.v );
    if ( !buf.u )
    {
        mod->type = MOD_BAD;
        return 0;
    }
    
    ident = LittleLong( *( U32* )buf.u );
    if ( ident == MDR_IDENT )
    {
        loaded = LoadMDR( mod, buf.u, filesize, name );
    }
    
    fileSystem->FreeFile( buf.v );
    
    if ( !loaded )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::RegisterMDR: couldn't load mdr file %s\n", name );
        mod->type = MOD_BAD;
        return 0;
    }
    
    return mod->index;
}

/*
====================
idRenderSystemModelLocal::RegisterIQM
====================
*/
qhandle_t idRenderSystemModelLocal::RegisterIQM( StringEntry name, model_t* mod )
{
    union
    {
        U32* u;
        void* v;
    } buf;
    S32 filesize;
    bool loaded = false;
    
    filesize = fileSystem->ReadFile( name, ( void** ) &buf.v );
    if ( !buf.u )
    {
        mod->type = MOD_BAD;
        return 0;
    }
    
    loaded = idRenderSystelModelIQMLocal::LoadIQM( mod, buf.u, filesize, name );
    
    fileSystem->FreeFile( buf.v );
    
    if ( !loaded )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::RegisterIQM: couldn't load iqm file %s\n", name );
        mod->type = MOD_BAD;
        return 0;
    }
    
    return mod->index;
}


struct modelExtToLoaderMap_t
{
    StringEntry ext;
    qhandle_t ( *ModelLoader )( StringEntry, model_t* );
};

// Note that the ordering indicates the order of preference used
// when there are multiple models of different formats available
static modelExtToLoaderMap_t modelLoaders[ ] =
{
    { "iqm", idRenderSystemModelLocal::RegisterIQM },
    { "mdr", idRenderSystemModelLocal::RegisterMDR },
    { "md3", idRenderSystemModelLocal::RegisterMD3 }
};

static S32 numModelLoaders = ARRAY_LEN( modelLoaders );

/*
** idRenderSystemModelLocal::GetModelByHandle
*/
model_t* idRenderSystemModelLocal::GetModelByHandle( qhandle_t index )
{
    model_t* mod;
    
    // out of range gets the defualt model
    if ( index < 1 || index >= tr.numModels )
    {
        return tr.models[0];
    }
    
    mod = tr.models[index];
    
    return mod;
}

/*
** idRenderSystemModelLocal::AllocModel
*/
model_t* idRenderSystemModelLocal::AllocModel( void )
{
    model_t* mod = nullptr;
    
    if ( tr.numModels == MAX_MOD_KNOWN )
    {
        return NULL;
    }
    
    mod = reinterpret_cast<model_t*>( memorySystem->Alloc( sizeof( *tr.models[tr.numModels] ), h_low ) );
    mod->index = tr.numModels;
    tr.models[tr.numModels] = mod;
    tr.numModels++;
    
    return mod;
}

/*
====================
idRenderSystemLocal::RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t idRenderSystemLocal::RegisterModel( StringEntry name )
{
    S32	i, orgLoader = -1;
    UTF8 localName[ MAX_QPATH ], altName[ MAX_QPATH ];
    StringEntry	ext;
    model_t* mod;
    qhandle_t hModel;
    bool orgNameFailed = false;
    
    if ( !name || !name[0] )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemLocal::RegisterModel: NULL name\n" );
        return 0;
    }
    
    if ( strlen( name ) >= MAX_QPATH )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "Model name exceeds MAX_QPATH\n" );
        return 0;
    }
    
    // search the currently loaded models
    for ( hModel = 1 ; hModel < tr.numModels; hModel++ )
    {
        mod = tr.models[hModel];
        
        if ( !::strcmp( mod->name, name ) )
        {
            if ( mod->type == MOD_BAD )
            {
                return 0;
            }
            
            return hModel;
        }
    }
    
    // allocate a new model_t
    if ( ( mod = renderSystemModelLocal.AllocModel() ) == NULL )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemLocal::RegisterModel: R_AllocModel() failed for '%s'\n", name );
        return 0;
    }
    
    // only set the name after the model has been successfully loaded
    Q_strncpyz( mod->name, name, sizeof( mod->name ) );
    
    idRenderSystemCmdsLocal::IssuePendingRenderCommands();
    
    mod->type = MOD_BAD;
    mod->numLods = 0;
    
    // load the files
    Q_strncpyz( localName, name, MAX_QPATH );
    
    ext = COM_GetExtension( localName );
    
    if ( *ext )
    {
        // Look for the correct loader and use it
        for ( i = 0; i < numModelLoaders; i++ )
        {
            if ( !Q_stricmp( ext, modelLoaders[ i ].ext ) )
            {
                // Load
                hModel = modelLoaders[ i ].ModelLoader( localName, mod );
                break;
            }
        }
        
        // A loader was found
        if ( i < numModelLoaders )
        {
            if ( !hModel )
            {
                // Loader failed, most likely because the file isn't there;
                // try again without the extension
                orgNameFailed = true;
                orgLoader = i;
                COM_StripExtension2( name, localName, MAX_QPATH );
            }
            else
            {
                // Something loaded
                return mod->index;
            }
        }
    }
    
    // Try and find a suitable match using all
    // the model formats supported
    for ( i = 0; i < numModelLoaders; i++ )
    {
        if ( i == orgLoader )
        {
            continue;
        }
        
        Q_snprintf( altName, sizeof( altName ), "%s.%s", localName, modelLoaders[ i ].ext );
        
        // Load
        hModel = modelLoaders[ i ].ModelLoader( altName, mod );
        
        if ( hModel )
        {
            if ( orgNameFailed )
            {
                clientMainSystem->RefPrintf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n", name, altName );
            }
            
            break;
        }
    }
    
    return hModel;
}

/*
=================
idRenderSystemModelLocal::LoadMD3
=================
*/
bool idRenderSystemModelLocal::LoadMD3( model_t* mod, S32 lod, void* buffer, S32 bufferSize, StringEntry modName )
{
    S32 f, i, j, *shaderIndex = nullptr, version, size;
    U32* tri = nullptr;
    md3Header_t* md3Model;
    md3Frame_t* md3Frame;
    md3Surface_t* md3Surf;
    md3Shader_t* md3Shader;
    md3Triangle_t* md3Tri;
    md3St_t* md3st;
    md3XyzNormal_t* md3xyz;
    md3Tag_t* md3Tag;
    mdvModel_t* mdvModel = nullptr;
    mdvFrame_t* frame = nullptr;
    mdvSurface_t* surf = nullptr;//, *surface;
    mdvVertex_t* v = nullptr;
    mdvSt_t* st = nullptr;
    mdvTag_t* tag = nullptr;
    mdvTagName_t* tagName = nullptr;
    
    md3Model = ( md3Header_t* ) buffer;
    
    version = LittleLong( md3Model->version );
    if ( version != MD3_VERSION )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMD3: %s has wrong version (%i should be %i)\n", modName, version, MD3_VERSION );
        return false;
    }
    
    mod->type = MOD_MESH;
    size = LittleLong( md3Model->ofsEnd );
    mod->dataSize += size;
    mdvModel = mod->mdv[lod] = reinterpret_cast<mdvModel_t*>( memorySystem->Alloc( sizeof( mdvModel_t ), h_low ) );
    
    LL( md3Model->ident );
    LL( md3Model->version );
    LL( md3Model->numFrames );
    LL( md3Model->numTags );
    LL( md3Model->numSurfaces );
    LL( md3Model->ofsFrames );
    LL( md3Model->ofsTags );
    LL( md3Model->ofsSurfaces );
    LL( md3Model->ofsEnd );
    
    if ( md3Model->numFrames < 1 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMD3: %s has no frames\n", modName );
        return false;
    }
    
    // swap all the frames
    mdvModel->numFrames = md3Model->numFrames;
    mdvModel->frames = frame = reinterpret_cast<mdvFrame_t*>( memorySystem->Alloc( sizeof( *frame ) * md3Model->numFrames, h_low ) );
    
    md3Frame = ( md3Frame_t* )( ( U8* ) md3Model + md3Model->ofsFrames );
    for ( i = 0; i < md3Model->numFrames; i++, frame++, md3Frame++ )
    {
        frame->radius = LittleFloat( md3Frame->radius );
        for ( j = 0; j < 3; j++ )
        {
            frame->bounds[0][j] = LittleFloat( md3Frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( md3Frame->bounds[1][j] );
            frame->localOrigin[j] = LittleFloat( md3Frame->localOrigin[j] );
        }
    }
    
    // swap all the tags
    mdvModel->numTags = md3Model->numTags;
    mdvModel->tags = tag = reinterpret_cast<mdvTag_t*>( memorySystem->Alloc( sizeof( *tag ) * ( md3Model->numTags * md3Model->numFrames ), h_low ) );
    
    md3Tag = ( md3Tag_t* )( ( U8* ) md3Model + md3Model->ofsTags );
    for ( i = 0; i < md3Model->numTags * md3Model->numFrames; i++, tag++, md3Tag++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            tag->origin[j] = LittleFloat( md3Tag->origin[j] );
            tag->axis[0][j] = LittleFloat( md3Tag->axis[0][j] );
            tag->axis[1][j] = LittleFloat( md3Tag->axis[1][j] );
            tag->axis[2][j] = LittleFloat( md3Tag->axis[2][j] );
        }
    }
    
    mdvModel->tagNames = tagName = reinterpret_cast<mdvTagName_t*>( memorySystem->Alloc( sizeof( *tagName ) * ( md3Model->numTags ), h_low ) );
    
    md3Tag = ( md3Tag_t* )( ( U8* ) md3Model + md3Model->ofsTags );
    
    for ( i = 0; i < md3Model->numTags; i++, tagName++, md3Tag++ )
    {
        Q_strncpyz( tagName->name, md3Tag->name, sizeof( tagName->name ) );
    }
    
    // swap all the surfaces
    mdvModel->numSurfaces = md3Model->numSurfaces;
    mdvModel->surfaces = surf = reinterpret_cast<mdvSurface_t*>( memorySystem->Alloc( sizeof( *surf ) * md3Model->numSurfaces, h_low ) );
    
    md3Surf = ( md3Surface_t* )( ( U8* ) md3Model + md3Model->ofsSurfaces );
    
    for ( i = 0; i < md3Model->numSurfaces; i++ )
    {
        LL( md3Surf->ident );
        LL( md3Surf->flags );
        LL( md3Surf->numFrames );
        LL( md3Surf->numShaders );
        LL( md3Surf->numTriangles );
        LL( md3Surf->ofsTriangles );
        LL( md3Surf->numVerts );
        LL( md3Surf->ofsShaders );
        LL( md3Surf->ofsSt );
        LL( md3Surf->ofsXyzNormals );
        LL( md3Surf->ofsEnd );
        
        if ( md3Surf->numVerts >= SHADER_MAX_VERTEXES )
        {
            clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMD3: %s has more than %i verts on %s (%i).\n",
                                         modName, SHADER_MAX_VERTEXES - 1, md3Surf->name[0] ? md3Surf->name : "a surface",
                                         md3Surf->numVerts );
            return false;
        }
        if ( md3Surf->numTriangles * 3 >= SHADER_MAX_INDEXES )
        {
            clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMD3: %s has more than %i triangles on %s (%i).\n",
                                         modName, ( SHADER_MAX_INDEXES / 3 ) - 1, md3Surf->name[0] ? md3Surf->name : "a surface",
                                         md3Surf->numTriangles );
            return false;
        }
        
        // change to surface identifier
        surf->surfaceType = SF_MDV;
        
        // give pointer to model for Tess_SurfaceMDX
        surf->model = mdvModel;
        
        // copy surface name
        Q_strncpyz( surf->name, md3Surf->name, sizeof( surf->name ) );
        
        // lowercase the surface name so skin compares are faster
        Q_strlwr( surf->name );
        
        // strip off a trailing _1 or _2
        // this is a crutch for q3data being a mess
        j = ( S32 )::strlen( surf->name );
        if ( j > 2 && surf->name[j - 2] == '_' )
        {
            surf->name[j - 2] = 0;
        }
        
        // register the shaders
        surf->numShaderIndexes = md3Surf->numShaders;
        surf->shaderIndexes = shaderIndex = reinterpret_cast<S32*>( memorySystem->Alloc( sizeof( *shaderIndex ) * md3Surf->numShaders, h_low ) );
        
        md3Shader = ( md3Shader_t* )( ( U8* ) md3Surf + md3Surf->ofsShaders );
        for ( j = 0; j < md3Surf->numShaders; j++, shaderIndex++, md3Shader++ )
        {
            shader_t*       sh;
            
            sh = idRenderSystemShaderLocal::FindShader( md3Shader->name, LIGHTMAP_NONE, true );
            if ( sh->defaultShader )
            {
                *shaderIndex = 0;
            }
            else
            {
                *shaderIndex = sh->index;
            }
        }
        
        // swap all the triangles
        surf->numIndexes = md3Surf->numTriangles * 3;
        surf->indexes = tri = reinterpret_cast<U32*>( memorySystem->Alloc( sizeof( *tri ) * 3 * md3Surf->numTriangles, h_low ) );
        
        md3Tri = ( md3Triangle_t* )( ( U8* ) md3Surf + md3Surf->ofsTriangles );
        for ( j = 0; j < md3Surf->numTriangles; j++, tri += 3, md3Tri++ )
        {
            tri[0] = LittleLong( md3Tri->indexes[0] );
            tri[1] = LittleLong( md3Tri->indexes[1] );
            tri[2] = LittleLong( md3Tri->indexes[2] );
        }
        
        // swap all the XyzNormals
        surf->numVerts = md3Surf->numVerts;
        surf->verts = v = reinterpret_cast<mdvVertex_t*>( memorySystem->Alloc( sizeof( *v ) * ( md3Surf->numVerts * md3Surf->numFrames ), h_low ) );
        
        md3xyz = ( md3XyzNormal_t* )( ( U8* ) md3Surf + md3Surf->ofsXyzNormals );
        
        for ( j = 0; j < md3Surf->numVerts * md3Surf->numFrames; j++, md3xyz++, v++ )
        {
            U16 normal;
            U32 lat, lng;
            vec3_t fNormal;
            
            v->xyz[0] = LittleShort( md3xyz->xyz[0] ) * MD3_XYZ_SCALE;
            v->xyz[1] = LittleShort( md3xyz->xyz[1] ) * MD3_XYZ_SCALE;
            v->xyz[2] = LittleShort( md3xyz->xyz[2] ) * MD3_XYZ_SCALE;
            
            normal = LittleShort( md3xyz->normal );
            
            lat = ( normal >> 8 ) & 0xff;
            lng = ( normal & 0xff );
            lat *= ( FUNCTABLE_SIZE / 256 );
            lng *= ( FUNCTABLE_SIZE / 256 );
            
            // decode X as cos( lat ) * sin( long )
            // decode Y as sin( lat ) * sin( long )
            // decode Z as cos( long )
            
            fNormal[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) )&FUNCTABLE_MASK] * tr.sinTable[lng];
            fNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
            fNormal[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) )&FUNCTABLE_MASK];
            
            idRenderSystemVaoLocal::VaoPackNormal( v->normal, fNormal );
        }
        
        // swap all the ST
        surf->st = st = reinterpret_cast<mdvSt_t*>( memorySystem->Alloc( sizeof( *st ) * md3Surf->numVerts, h_low ) );
        
        md3st = ( md3St_t* )( ( U8* ) md3Surf + md3Surf->ofsSt );
        for ( j = 0; j < md3Surf->numVerts; j++, md3st++, st++ )
        {
            st->st[0] = LittleFloat( md3st->st[0] );
            st->st[1] = LittleFloat( md3st->st[1] );
        }
        
        // calc tangent spaces
        {
            vec3_t* sdirs = ( vec3_t* )memorySystem->Malloc( sizeof( *sdirs ) * surf->numVerts * mdvModel->numFrames );
            vec3_t* tdirs = ( vec3_t* )memorySystem->Malloc( sizeof( *tdirs ) * surf->numVerts * mdvModel->numFrames );
            
            for ( j = 0, v = surf->verts; j < ( surf->numVerts * mdvModel->numFrames ); j++, v++ )
            {
                VectorClear( sdirs[j] );
                VectorClear( tdirs[j] );
            }
            
            for ( f = 0; f < mdvModel->numFrames; f++ )
            {
                for ( j = 0, tri = surf->indexes; j < surf->numIndexes; j += 3, tri += 3 )
                {
                    vec3_t sdir, tdir;
                    const F32* v0, *v1, *v2, *t0, *t1, *t2;
                    U32 index0, index1, index2;
                    
                    index0 = surf->numVerts * f + tri[0];
                    index1 = surf->numVerts * f + tri[1];
                    index2 = surf->numVerts * f + tri[2];
                    
                    v0 = surf->verts[index0].xyz;
                    v1 = surf->verts[index1].xyz;
                    v2 = surf->verts[index2].xyz;
                    
                    t0 = surf->st[tri[0]].st;
                    t1 = surf->st[tri[1]].st;
                    t2 = surf->st[tri[2]].st;
                    
                    idRenderSystemMainLocal::CalcTexDirs( sdir, tdir, v0, v1, v2, t0, t1, t2 );
                    
                    VectorAdd( sdir, sdirs[index0], sdirs[index0] );
                    VectorAdd( sdir, sdirs[index1], sdirs[index1] );
                    VectorAdd( sdir, sdirs[index2], sdirs[index2] );
                    VectorAdd( tdir, tdirs[index0], tdirs[index0] );
                    VectorAdd( tdir, tdirs[index1], tdirs[index1] );
                    VectorAdd( tdir, tdirs[index2], tdirs[index2] );
                }
            }
            
            for ( j = 0, v = surf->verts; j < ( surf->numVerts * mdvModel->numFrames ); j++, v++ )
            {
                vec3_t normal;
                vec4_t tangent;
                
                VectorNormalize( sdirs[j] );
                VectorNormalize( tdirs[j] );
                
                idRenderSystemVaoLocal::VaoUnpackNormal( normal, v->normal );
                
                tangent[3] = idRenderSystemMainLocal::CalcTangentSpace( tangent, NULL, normal, sdirs[j], tdirs[j] );
                
                idRenderSystemVaoLocal::VaoPackTangent( v->tangent, tangent );
            }
            
            memorySystem->Free( sdirs );
            memorySystem->Free( tdirs );
        }
        
        // find the next surface
        md3Surf = ( md3Surface_t* )( ( U8* ) md3Surf + md3Surf->ofsEnd );
        surf++;
    }
    
    {
        srfVaoMdvMesh_t* vaoSurf;
        
        mdvModel->numVaoSurfaces = mdvModel->numSurfaces;
        mdvModel->vaoSurfaces = reinterpret_cast<srfVaoMdvMesh_t*>( memorySystem->Alloc( sizeof( *mdvModel->vaoSurfaces ) * mdvModel->numSurfaces, h_low ) );
        
        vaoSurf = mdvModel->vaoSurfaces;
        surf = mdvModel->surfaces;
        for ( i = 0; i < mdvModel->numSurfaces; i++, vaoSurf++, surf++ )
        {
            U32 offset_xyz, offset_st, offset_normal, offset_tangent;
            U32 stride_xyz, stride_st, stride_normal, stride_tangent;
            U32 dataSize, dataOfs;
            U8* data;
            
            if ( mdvModel->numFrames > 1 )
            {
                // vertex animation, store texcoords first, then position/normal/tangents
                offset_st = 0;
                offset_xyz = surf->numVerts * sizeof( vec2_t );
                offset_normal = offset_xyz + sizeof( vec3_t );
                offset_tangent = offset_normal + sizeof( S16 ) * 4;
                stride_st  = sizeof( vec2_t );
                stride_xyz = sizeof( vec3_t ) + sizeof( S16 ) * 4;
                stride_xyz += sizeof( S16 ) * 4;
                stride_normal = stride_tangent = stride_xyz;
                
                dataSize = offset_xyz + surf->numVerts * mdvModel->numFrames * stride_xyz;
            }
            else
            {
                // no animation, interleave everything
                offset_xyz = 0;
                offset_st = offset_xyz + sizeof( vec3_t );
                offset_normal = offset_st + sizeof( vec2_t );
                offset_tangent = offset_normal + sizeof( S16 ) * 4;
                stride_xyz = offset_tangent + sizeof( S16 ) * 4;
                stride_st = stride_normal = stride_tangent = stride_xyz;
                
                dataSize = surf->numVerts * stride_xyz;
            }
            
            data = ( U8* )memorySystem->Malloc( dataSize );
            dataOfs = 0;
            
            if ( mdvModel->numFrames > 1 )
            {
                st = surf->st;
                
                for ( j = 0 ; j < surf->numVerts ; j++, st++ )
                {
                    ::memcpy( data + dataOfs, &st->st, sizeof( vec2_t ) );
                    dataOfs += sizeof( st->st );
                }
                
                v = surf->verts;
                for ( j = 0; j < surf->numVerts * mdvModel->numFrames ; j++, v++ )
                {
                    // xyz
                    ::memcpy( data + dataOfs, &v->xyz, sizeof( vec3_t ) );
                    dataOfs += sizeof( vec3_t );
                    
                    // normal
                    ::memcpy( data + dataOfs, &v->normal, sizeof( S16 ) * 4 );
                    dataOfs += sizeof( S16 ) * 4;
                    
                    // tangent
                    ::memcpy( data + dataOfs, &v->tangent, sizeof( S16 ) * 4 );
                    dataOfs += sizeof( S16 ) * 4;
                }
            }
            else
            {
                v = surf->verts;
                st = surf->st;
                
                for ( j = 0; j < surf->numVerts; j++, v++, st++ )
                {
                    // xyz
                    ::memcpy( data + dataOfs, &v->xyz, sizeof( vec3_t ) );
                    dataOfs += sizeof( v->xyz );
                    
                    // st
                    ::memcpy( data + dataOfs, &st->st, sizeof( vec2_t ) );
                    dataOfs += sizeof( st->st );
                    
                    // normal
                    ::memcpy( data + dataOfs, &v->normal, sizeof( S16 ) * 4 );
                    dataOfs += sizeof( S16 ) * 4;
                    
                    // tangent
                    memcpy( data + dataOfs, &v->tangent, sizeof( S16 ) * 4 );
                    dataOfs += sizeof( S16 ) * 4;
                }
            }
            
            vaoSurf->surfaceType = SF_VAO_MDVMESH;
            vaoSurf->mdvModel = mdvModel;
            vaoSurf->mdvSurface = surf;
            vaoSurf->numIndexes = surf->numIndexes;
            vaoSurf->numVerts = surf->numVerts;
            
            vaoSurf->minIndex = 0;
            vaoSurf->maxIndex = surf->numVerts - 1;
            
            vaoSurf->vao = idRenderSystemVaoLocal::CreateVao( va( "staticMD3Mesh_VAO '%s'", surf->name ), data, dataSize,
                           ( U8* )surf->indexes, surf->numIndexes * sizeof( *surf->indexes ), VAO_USAGE_STATIC );
                           
            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].enabled = 1;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].enabled = 1;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].enabled = 1;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].enabled = 1;
            
            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].count = 3;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].count = 2;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].count = 4;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].count = 4;
            
            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].type = GL_FLOAT;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].type = GL_FLOAT;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].type = GL_SHORT;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].type = GL_SHORT;
            
            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].normalized = GL_FALSE;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].normalized = GL_FALSE;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].normalized = GL_TRUE;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].normalized = GL_TRUE;
            
            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].offset = offset_xyz;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].offset = offset_st;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].offset = offset_normal;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].offset = offset_tangent;
            
            vaoSurf->vao->attribs[ATTR_INDEX_POSITION].stride = stride_xyz;
            vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].stride = stride_st;
            vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ].stride = stride_normal;
            vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ].stride = stride_tangent;
            
            if ( mdvModel->numFrames > 1 )
            {
                vaoSurf->vao->attribs[ATTR_INDEX_POSITION2] = vaoSurf->vao->attribs[ATTR_INDEX_POSITION];
                vaoSurf->vao->attribs[ATTR_INDEX_NORMAL2  ] = vaoSurf->vao->attribs[ATTR_INDEX_NORMAL  ];
                vaoSurf->vao->attribs[ATTR_INDEX_TANGENT2 ] = vaoSurf->vao->attribs[ATTR_INDEX_TANGENT ];
                
                vaoSurf->vao->frameSize = stride_xyz    * surf->numVerts;
            }
            
            idRenderSystemVaoLocal::SetVertexPointers( vaoSurf->vao );
            
            memorySystem->Free( data );
        }
    }
    
    return true;
}

/*
=================
idRenderSystemModelLocal::LoadMDR
=================
*/
bool idRenderSystemModelLocal::LoadMDR( model_t* mod, void* buffer, S32 filesize, StringEntry mod_name )
{
    S32 i, j, k, l, size;
    mdrHeader_t* pinmodel, *mdr = nullptr;
    mdrFrame_t* frame;
    mdrLOD_t* lod, *curlod;
    mdrSurface_t* surf, *cursurf;
    mdrTriangle_t* tri, *curtri;
    mdrVertex_t* v, *curv;
    mdrWeight_t* weight, *curweight;
    mdrTag_t* tag, *curtag;
    shader_t* sh;
    
    pinmodel = ( mdrHeader_t* )buffer;
    
    pinmodel->version = LittleLong( pinmodel->version );
    if ( pinmodel->version != MDR_VERSION )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has wrong version (%i should be %i)\n", mod_name, pinmodel->version, MDR_VERSION );
        return false;
    }
    
    size = LittleLong( pinmodel->ofsEnd );
    
    if ( size > filesize )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name );
        return false;
    }
    
    mod->type = MOD_MDR;
    
    LL( pinmodel->numFrames );
    LL( pinmodel->numBones );
    LL( pinmodel->ofsFrames );
    
    // This is a model that uses some type of compressed Bones. We don't want to uncompress every bone for each rendered frame
    // over and over again, we'll uncompress it in this function already, so we must adjust the size of the target mdr.
    if ( pinmodel->ofsFrames < 0 )
    {
        // mdrFrame_t is larger than mdrCompFrame_t:
        size += pinmodel->numFrames * sizeof( frame->name );
        
        // now add enough space for the uncompressed bones.
        size += pinmodel->numFrames * pinmodel->numBones * ( ( sizeof( mdrBone_t ) - sizeof( mdrCompBone_t ) ) );
    }
    
    // simple bounds check
    if ( pinmodel->numBones < 0 ||
            sizeof( *mdr ) + pinmodel->numFrames * ( sizeof( *frame ) + ( pinmodel->numBones - 1 ) * sizeof( *frame->bones ) ) > size )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has broken structure.\n", mod_name );
        return false;
    }
    
    mod->dataSize += size;
    mod->modelData = mdr = reinterpret_cast<mdrHeader_t*>( memorySystem->Alloc( size, h_low ) );
    
    // Copy all the values over from the file and fix endian issues in the process, if necessary.
    
    mdr->ident = LittleLong( pinmodel->ident );
    mdr->version = pinmodel->version;	// Don't need to swap U8 order on this one, we already did above.
    Q_strncpyz( mdr->name, pinmodel->name, sizeof( mdr->name ) );
    mdr->numFrames = pinmodel->numFrames;
    mdr->numBones = pinmodel->numBones;
    mdr->numLODs = LittleLong( pinmodel->numLODs );
    mdr->numTags = LittleLong( pinmodel->numTags );
    // We don't care about the other offset values, we'll generate them ourselves while loading.
    
    mod->numLods = mdr->numLODs;
    
    if ( mdr->numFrames < 1 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has no frames\n", mod_name );
        return false;
    }
    
    /* The first frame will be put into the first free space after the header */
    frame = ( mdrFrame_t* )( mdr + 1 );
    mdr->ofsFrames = ( S32 )( ( U8* ) frame - ( U8* ) mdr );
    
    if ( pinmodel->ofsFrames < 0 )
    {
        mdrCompFrame_t* cframe;
        
        // compressed model...
        cframe = ( mdrCompFrame_t* )( ( U8* ) pinmodel - pinmodel->ofsFrames );
        
        for ( i = 0; i < mdr->numFrames; i++ )
        {
            for ( j = 0; j < 3; j++ )
            {
                frame->bounds[0][j] = LittleFloat( cframe->bounds[0][j] );
                frame->bounds[1][j] = LittleFloat( cframe->bounds[1][j] );
                frame->localOrigin[j] = LittleFloat( cframe->localOrigin[j] );
            }
            
            frame->radius = LittleFloat( cframe->radius );
            frame->name[0] = '\0';	// No name supplied in the compressed version.
            
            for ( j = 0; j < mdr->numBones; j++ )
            {
                for ( k = 0; k < ( sizeof( cframe->bones[j].Comp ) / 2 ); k++ )
                {
                    // Do swapping for the uncompressing functions. They seem to use shorts
                    // values only, so I assume this will work. Never tested it on other
                    // platforms, though.
                    
                    ( ( U16* )( cframe->bones[j].Comp ) )[k] =
                        LittleShort( ( ( U16* )( cframe->bones[j].Comp ) )[k] );
                }
                
                /* Now do the actual uncompressing */
                idRenderSystemAnimationLocal::UnCompress( frame->bones[j].matrix, cframe->bones[j].Comp );
            }
            
            // Next Frame...
            cframe = ( mdrCompFrame_t* ) &cframe->bones[j];
            frame = ( mdrFrame_t* ) &frame->bones[j];
        }
    }
    else
    {
        mdrFrame_t* curframe;
        
        // uncompressed model...
        curframe = ( mdrFrame_t* )( ( U8* ) pinmodel + pinmodel->ofsFrames );
        
        // swap all the frames
        for ( i = 0 ; i < mdr->numFrames ; i++ )
        {
            for ( j = 0; j < 3; j++ )
            {
                frame->bounds[0][j] = LittleFloat( curframe->bounds[0][j] );
                frame->bounds[1][j] = LittleFloat( curframe->bounds[1][j] );
                frame->localOrigin[j] = LittleFloat( curframe->localOrigin[j] );
            }
            
            frame->radius = LittleFloat( curframe->radius );
            Q_strncpyz( frame->name, curframe->name, sizeof( frame->name ) );
            
            for ( j = 0; j < ( S32 )( mdr->numBones * sizeof( mdrBone_t ) / 4 ); j++ )
            {
                ( ( F32* )frame->bones )[j] = LittleFloat( ( ( F32* )curframe->bones )[j] );
            }
            
            curframe = ( mdrFrame_t* ) &curframe->bones[mdr->numBones];
            frame = ( mdrFrame_t* ) &frame->bones[mdr->numBones];
        }
    }
    
    // frame should now point to the first free address after all frames.
    lod = ( mdrLOD_t* ) frame;
    mdr->ofsLODs = ( S32 )( ( U8* ) lod - ( U8* )mdr );
    
    curlod = ( mdrLOD_t* )( ( U8* ) pinmodel + LittleLong( pinmodel->ofsLODs ) );
    
    // swap all the LOD's
    for ( l = 0 ; l < mdr->numLODs ; l++ )
    {
        // simple bounds check
        if ( ( U8* )( lod + 1 ) > ( U8* ) mdr + size )
        {
            clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has broken structure.\n", mod_name );
            return false;
        }
        
        lod->numSurfaces = LittleLong( curlod->numSurfaces );
        
        // swap all the surfaces
        surf = ( mdrSurface_t* )( lod + 1 );
        lod->ofsSurfaces = ( S32 )( ( U8* ) surf - ( U8* ) lod );
        cursurf = ( mdrSurface_t* )( ( U8* )curlod + LittleLong( curlod->ofsSurfaces ) );
        
        for ( i = 0 ; i < lod->numSurfaces ; i++ )
        {
            // simple bounds check
            if ( ( U8* )( surf + 1 ) > ( U8* ) mdr + size )
            {
                clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has broken structure.\n", mod_name );
                return false;
            }
            
            // first do some copying stuff
            
            surf->ident = SF_MDR;
            Q_strncpyz( surf->name, cursurf->name, sizeof( surf->name ) );
            Q_strncpyz( surf->shader, cursurf->shader, sizeof( surf->shader ) );
            
            surf->ofsHeader = ( S32 )( ( U8* ) mdr - ( U8* ) surf );
            
            surf->numVerts = LittleLong( cursurf->numVerts );
            surf->numTriangles = LittleLong( cursurf->numTriangles );
            // numBoneReferences and BoneReferences generally seem to be unused
            
            // now do the checks that may fail.
            if ( surf->numVerts >= SHADER_MAX_VERTEXES )
            {
                clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has more than %i verts on %s (%i).\n",
                                             mod_name, SHADER_MAX_VERTEXES - 1, surf->name[0] ? surf->name : "a surface",
                                             surf->numVerts );
                return false;
            }
            if ( surf->numTriangles * 3 >= SHADER_MAX_INDEXES )
            {
                clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has more than %i triangles on %s (%i).\n",
                                             mod_name, ( SHADER_MAX_INDEXES / 3 ) - 1, surf->name[0] ? surf->name : "a surface",
                                             surf->numTriangles );
                return false;
            }
            
            // lowercase the surface name so skin compares are faster
            Q_strlwr( surf->name );
            
            // register the shaders
            sh = idRenderSystemShaderLocal::FindShader( surf->shader, LIGHTMAP_NONE, true );
            if ( sh->defaultShader )
            {
                surf->shaderIndex = 0;
            }
            else
            {
                surf->shaderIndex = sh->index;
            }
            
            // now copy the vertexes.
            v = ( mdrVertex_t* )( surf + 1 );
            surf->ofsVerts = ( S32 )( ( U8* ) v - ( U8* ) surf );
            curv = ( mdrVertex_t* )( ( U8* )cursurf + LittleLong( cursurf->ofsVerts ) );
            
            for ( j = 0; j < surf->numVerts; j++ )
            {
                LL( curv->numWeights );
                
                // simple bounds check
                if ( curv->numWeights < 0 || ( U8* )( v + 1 ) + ( curv->numWeights - 1 ) * sizeof( *weight ) > ( U8* ) mdr + size )
                {
                    clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has broken structure.\n", mod_name );
                    return false;
                }
                
                v->normal[0] = LittleFloat( curv->normal[0] );
                v->normal[1] = LittleFloat( curv->normal[1] );
                v->normal[2] = LittleFloat( curv->normal[2] );
                
                v->texCoords[0] = LittleFloat( curv->texCoords[0] );
                v->texCoords[1] = LittleFloat( curv->texCoords[1] );
                
                v->numWeights = curv->numWeights;
                weight = &v->weights[0];
                curweight = &curv->weights[0];
                
                // Now copy all the weights
                for ( k = 0; k < v->numWeights; k++ )
                {
                    weight->boneIndex = LittleLong( curweight->boneIndex );
                    weight->boneWeight = LittleFloat( curweight->boneWeight );
                    
                    weight->offset[0] = LittleFloat( curweight->offset[0] );
                    weight->offset[1] = LittleFloat( curweight->offset[1] );
                    weight->offset[2] = LittleFloat( curweight->offset[2] );
                    
                    weight++;
                    curweight++;
                }
                
                v = ( mdrVertex_t* ) weight;
                curv = ( mdrVertex_t* ) curweight;
            }
            
            // we know the offset to the triangles now:
            tri = ( mdrTriangle_t* ) v;
            surf->ofsTriangles = ( S32 )( ( U8* ) tri - ( U8* ) surf );
            curtri = ( mdrTriangle_t* )( ( U8* ) cursurf + LittleLong( cursurf->ofsTriangles ) );
            
            // simple bounds check
            if ( surf->numTriangles < 0 || ( U8* )( tri + surf->numTriangles ) > ( U8* ) mdr + size )
            {
                clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has broken structure.\n", mod_name );
                return false;
            }
            
            for ( j = 0; j < surf->numTriangles; j++ )
            {
                tri->indexes[0] = LittleLong( curtri->indexes[0] );
                tri->indexes[1] = LittleLong( curtri->indexes[1] );
                tri->indexes[2] = LittleLong( curtri->indexes[2] );
                
                tri++;
                curtri++;
            }
            
            // tri now points to the end of the surface.
            surf->ofsEnd = ( S32 )( ( U8* ) tri - ( U8* ) surf );
            surf = ( mdrSurface_t* ) tri;
            
            // find the next surface.
            cursurf = ( mdrSurface_t* )( ( U8* ) cursurf + LittleLong( cursurf->ofsEnd ) );
        }
        
        // surf points to the next lod now.
        lod->ofsEnd = ( S32 )( ( U8* ) surf - ( U8* ) lod );
        lod = ( mdrLOD_t* ) surf;
        
        // find the next LOD.
        curlod = ( mdrLOD_t* )( ( U8* ) curlod + LittleLong( curlod->ofsEnd ) );
    }
    
    // lod points to the first tag now, so update the offset too.
    tag = ( mdrTag_t* ) lod;
    mdr->ofsTags = ( S32 )( ( U8* ) tag - ( U8* ) mdr );
    curtag = ( mdrTag_t* )( ( U8* )pinmodel + LittleLong( pinmodel->ofsTags ) );
    
    // simple bounds check
    if ( mdr->numTags < 0 || ( U8* )( tag + mdr->numTags ) > ( U8* ) mdr + size )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemModelLocal::LoadMDR: %s has broken structure.\n", mod_name );
        return false;
    }
    
    for ( i = 0 ; i < mdr->numTags ; i++ )
    {
        tag->boneIndex = LittleLong( curtag->boneIndex );
        Q_strncpyz( tag->name, curtag->name, sizeof( tag->name ) );
        
        tag++;
        curtag++;
    }
    
    // And finally we know the real offset to the end.
    mdr->ofsEnd = ( S32 )( ( U8* ) tag - ( U8* ) mdr );
    
    // phew! we're done.
    
    return true;
}

/*
** idRenderSystemLocal::Init
*/
void idRenderSystemLocal::Init( vidconfig_t* glconfigOut )
{
    S32	i;
    
    renderSystemInitLocal.Init();
    
    *glconfigOut = glConfig;
    
    renderSystemCmdsLocal.IssuePendingRenderCommands();
    
    tr.visIndex = 0;
    // force markleafs to regenerate
    for ( i = 0; i < MAX_VISCOUNTS; i++ )
    {
        tr.visClusters[i] = -2;
    }
    
    renderSystemFlaresLocal.ClearFlares();
    renderSystemLocal.ClearScene();
    
    tr.registered = true;
}

/*
===============
idRenderSystemModelLocal::ModelInit
===============
*/
void idRenderSystemModelLocal::ModelInit( void )
{
    model_t*		mod;
    
    // leave a space for NULL model
    tr.numModels = 0;
    
    mod = AllocModel();
    mod->type = MOD_BAD;
}

/*
================
idRenderSystemModelLocal::Modellist_f
================
*/
void idRenderSystemModelLocal::Modellist_f( void )
{
    S32	i, j, total = 0, lods;
    model_t* mod;
    
    for ( i = 1 ; i < tr.numModels; i++ )
    {
        mod = tr.models[i];
        lods = 1;
        
        for ( j = 1 ; j < MD3_MAX_LODS ; j++ )
        {
            if ( mod->mdv[j] && mod->mdv[j] != mod->mdv[j - 1] )
            {
                lods++;
            }
        }
        
        clientMainSystem->RefPrintf( PRINT_ALL, "%8i : (%i) %s\n", mod->dataSize, lods, mod->name );
        
        total += mod->dataSize;
    }
    clientMainSystem->RefPrintf( PRINT_ALL, "%8i : Total models\n", total );
    
#if	0
    // not working right with new hunk
    if ( tr.world )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
    }
#endif
}

/*
================
idRenderSystemModelLocal::GetTag
================
*/
mdvTag_t* idRenderSystemModelLocal::GetTag( mdvModel_t* mod, S32 frame, StringEntry _tagName )
{
    S32 i;
    mdvTag_t* tag;
    mdvTagName_t* tagName;
    
    if ( frame >= mod->numFrames )
    {
        // it is possible to have a bad frame while changing models, so don't error
        frame = mod->numFrames - 1;
    }
    
    tag = mod->tags + frame * mod->numTags;
    tagName = mod->tagNames;
    
    for ( i = 0; i < mod->numTags; i++, tag++, tagName++ )
    {
        if ( !::strcmp( tagName->name, _tagName ) )
        {
            return tag;
        }
    }
    
    return nullptr;
}

mdvTag_t* idRenderSystemModelLocal::GetAnimTag( mdrHeader_t* mod, S32 framenum, StringEntry tagName, mdvTag_t* dest )
{
    S32 i, j, k;
    intptr_t frameSize;
    mdrFrame_t* frame;
    mdrTag_t* tag;
    
    if ( framenum >= mod->numFrames )
    {
        // it is possible to have a bad frame while changing models, so don't error
        framenum = mod->numFrames - 1;
    }
    
    tag = ( mdrTag_t* )( ( U8* )mod + mod->ofsTags );
    for ( i = 0 ; i < mod->numTags ; i++, tag++ )
    {
        if ( !::strcmp( tag->name, tagName ) )
        {
            // uncompressed model...
            frameSize = ( intptr_t )( &( ( mdrFrame_t* )0 )->bones[mod->numBones] );
            frame = ( mdrFrame_t* )( ( U8* )mod + mod->ofsFrames + framenum * frameSize );
            
            for ( j = 0; j < 3; j++ )
            {
                for ( k = 0; k < 3; k++ )
                {
                    dest->axis[j][k] = frame->bones[tag->boneIndex].matrix[k][j];
                }
            }
            
            dest->origin[0] = frame->bones[tag->boneIndex].matrix[0][3];
            dest->origin[1] = frame->bones[tag->boneIndex].matrix[1][3];
            dest->origin[2] = frame->bones[tag->boneIndex].matrix[2][3];
            
            return dest;
        }
    }
    
    return NULL;
}

/*
================
idRenderSystemLocal::LerpTag
================
*/
S32 idRenderSystemLocal::LerpTag( orientation_t* tag, qhandle_t handle, S32 startFrame, S32 endFrame, F32 frac, StringEntry tagName )
{
    mdvTag_t*	start, *end;
    mdvTag_t	start_space, end_space;
    S32		i;
    F32		frontLerp, backLerp;
    model_t*		model;
    
    model = renderSystemModelLocal.GetModelByHandle( handle );
    if ( !model->mdv[0] )
    {
        if ( model->type == MOD_MDR )
        {
            start = renderSystemModelLocal.GetAnimTag( ( mdrHeader_t* ) model->modelData, startFrame, tagName, &start_space );
            end = renderSystemModelLocal.GetAnimTag( ( mdrHeader_t* ) model->modelData, endFrame, tagName, &end_space );
        }
        else if ( model->type == MOD_IQM )
        {
            return idRenderSystelModelIQMLocal::IQMLerpTag( tag, ( iqmData_t* )model->modelData, startFrame, endFrame, frac, tagName );
        }
        else
        {
            start = end = NULL;
        }
    }
    else
    {
        start = idRenderSystemModelLocal::GetTag( model->mdv[0], startFrame, tagName );
        end = idRenderSystemModelLocal::GetTag( model->mdv[0], endFrame, tagName );
    }
    
    if ( !start || !end )
    {
        AxisClear( tag->axis );
        VectorClear( tag->origin );
        return false;
    }
    
    frontLerp = frac;
    backLerp = 1.0f - frac;
    
    for ( i = 0 ; i < 3 ; i++ )
    {
        tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
        tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
        tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
        tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
    }
    VectorNormalize( tag->axis[0] );
    VectorNormalize( tag->axis[1] );
    VectorNormalize( tag->axis[2] );
    return true;
}


/*
====================
idRenderSystemLocal::ModelBounds
====================
*/
void idRenderSystemLocal::ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs )
{
    model_t* model;
    
    model = idRenderSystemModelLocal::GetModelByHandle( handle );
    
    if ( model->type == MOD_BRUSH )
    {
        VectorCopy( model->bmodel->bounds[0], mins );
        VectorCopy( model->bmodel->bounds[1], maxs );
        
        return;
    }
    else if ( model->type == MOD_MESH )
    {
        mdvModel_t*	header;
        mdvFrame_t*	frame;
        
        header = model->mdv[0];
        frame = header->frames;
        
        VectorCopy( frame->bounds[0], mins );
        VectorCopy( frame->bounds[1], maxs );
        
        return;
    }
    else if ( model->type == MOD_MDR )
    {
        mdrHeader_t*	header;
        mdrFrame_t*	frame;
        
        header = ( mdrHeader_t* )model->modelData;
        frame = ( mdrFrame_t* )( ( U8* )header + header->ofsFrames );
        
        VectorCopy( frame->bounds[0], mins );
        VectorCopy( frame->bounds[1], maxs );
        
        return;
    }
    else if ( model->type == MOD_IQM )
    {
        iqmData_t* iqmData;
        
        iqmData = ( iqmData_t* )model->modelData;
        
        if ( iqmData->bounds )
        {
            VectorCopy( iqmData->bounds, mins );
            VectorCopy( iqmData->bounds + 3, maxs );
            return;
        }
    }
    
    VectorClear( mins );
    VectorClear( maxs );
}

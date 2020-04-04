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
// File name:   r_world.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemSkinsLocal renderSystemSkinsLocal;

/*
===============
idRenderSystemSkinsLocal::idRenderSystemSkinsLocal
===============
*/
idRenderSystemSkinsLocal::idRenderSystemSkinsLocal( void )
{
}

/*
===============
idRenderSystemSkinsLocal::~idRenderSystemSkinsLocal
===============
*/
idRenderSystemSkinsLocal::~idRenderSystemSkinsLocal( void )
{
}

/*
==================
idRenderSystemSkinsLocal::CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
StringEntry idRenderSystemSkinsLocal::CommaParse( UTF8** data_p )
{
    S32 c = 0, len;
    UTF8* data;
    static	UTF8 com_token[MAX_TOKEN_CHARS];
    
    data = *data_p;
    len = 0;
    com_token[0] = 0;
    
    // make sure incoming data is valid
    if ( !data )
    {
        *data_p = NULL;
        return com_token;
    }
    
    while ( 1 )
    {
        // skip whitespace
        while ( ( c = *data ) <= ' ' )
        {
            if ( !c )
            {
                break;
            }
            
            data++;
        }
        
        c = *data;
        
        // skip double slash comments
        if ( c == '/' && data[1] == '/' )
        {
            data += 2;
            
            while ( *data && *data != '\n' )
            {
                data++;
            }
        }
        // skip /* */ comments
        else if ( c == '/' && data[1] == '*' )
        {
            data += 2;
            
            while ( *data && ( *data != '*' || data[1] != '/' ) )
            {
                data++;
            }
            
            if ( *data )
            {
                data += 2;
            }
        }
        else
        {
            break;
        }
    }
    
    if ( c == 0 )
    {
        return "";
    }
    
    // handle quoted strings
    if ( c == '\"' )
    {
        data++;
        
        while ( 1 )
        {
            c = *data++;
            
            if ( c == '\"' || !c )
            {
                com_token[len] = 0;
                *data_p = const_cast< UTF8* >( data );
                return com_token;
            }
            
            if ( len < MAX_TOKEN_CHARS - 1 )
            {
                com_token[len] = c;
                len++;
            }
        }
    }
    
    // parse a regular word
    do
    {
        if ( len < MAX_TOKEN_CHARS - 1 )
        {
            com_token[len] = c;
            len++;
        }
        
        data++;
        c = *data;
    }
    while ( c > 32 && c != ',' );
    
    com_token[len] = 0;
    
    *data_p = const_cast< UTF8* >( data );
    return com_token;
}

/*
===============
idRenderSystemLocal::RegisterSkin
===============
*/
qhandle_t idRenderSystemLocal::RegisterSkin( StringEntry name )
{
    union
    {
        UTF8* c;
        void* v;
    } text;
    qhandle_t hSkin;
    skin_t*	 skin;
    skinSurface_t parseSurfaces[MAX_SKIN_SURFACES], *surf;
    UTF8* text_p, surfName[MAX_QPATH];
    StringEntry	token;
    
    if ( !name || !name[0] )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Empty name passed to idRenderSystemLocal::RegisterSkin\n" );
        return 0;
    }
    
    if ( ::strlen( name ) >= MAX_QPATH )
    {
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "Skin name exceeds MAX_QPATH\n" );
        return 0;
    }
    
    // see if the skin is already loaded
    for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ )
    {
        skin = tr.skins[hSkin];
        
        if ( !Q_stricmp( skin->name, name ) )
        {
            if ( skin->numSurfaces == 0 )
            {
                // default skin
                return 0;
            }
            
            return hSkin;
        }
    }
    
    // allocate a new skin
    if ( tr.numSkins == MAX_SKINS )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: idRenderSystemLocal::RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
        return 0;
    }
    
    tr.numSkins++;
    skin = reinterpret_cast<skin_t*>( memorySystem->Alloc( sizeof( skin_t ), h_low ) );
    tr.skins[hSkin] = skin;
    Q_strncpyz( skin->name, name, sizeof( skin->name ) );
    skin->numSurfaces = 0;
    
    renderSystemCmdsLocal.IssuePendingRenderCommands();
    
    // If not a .skin file, load as a single shader
    if ( strcmp( name + strlen( name ) - 5, ".skin" ) )
    {
        skin->numSurfaces = 1;
        skin->surfaces = reinterpret_cast<skinSurface_t*>( memorySystem->Alloc( sizeof( skinSurface_t ), h_low ) );
        skin->surfaces[0].shader = idRenderSystemShaderLocal::FindShader( name, LIGHTMAP_NONE, true );
        return hSkin;
    }
    
    // load and parse the skin file
    fileSystem->ReadFile( name, &text.v );
    if ( !text.c )
    {
        return 0;
    }
    
    S32 totalSurfaces = 0;
    text_p = text.c;
    
    while ( text_p && *text_p )
    {
        // get surface name
        token = renderSystemSkinsLocal.CommaParse( &text_p );
        Q_strncpyz( surfName, token, sizeof( surfName ) );
        
        if ( !token[0] )
        {
            break;
        }
        
        // lowercase the surface name so skin compares are faster
        Q_strlwr( surfName );
        
        if ( *text_p == ',' )
        {
            text_p++;
        }
        
        if ( ::strstr( token, "tag_" ) )
        {
            continue;
        }
        
        // parse the shader name
        token = renderSystemSkinsLocal.CommaParse( &text_p );
        
        if ( skin->numSurfaces < MAX_SKIN_SURFACES )
        {
            surf = &parseSurfaces[skin->numSurfaces];
            Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
            surf->shader = idRenderSystemShaderLocal::FindShader( token, LIGHTMAP_NONE, true );
            skin->numSurfaces++;
        }
        
        totalSurfaces++;
    }
    
    fileSystem->FreeFile( text.v );
    
    if ( totalSurfaces > MAX_SKIN_SURFACES )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "WARNING: Ignoring excess surfaces (found %d, max is %d) in skin '%s'!\n", totalSurfaces, MAX_SKIN_SURFACES, name );
    }
    
    // never let a skin have 0 shaders
    if ( skin->numSurfaces == 0 )
    {
        // use default skin
        return 0;
    }
    
    // copy surfaces to skin
    skin->surfaces = reinterpret_cast<skinSurface_t*>( memorySystem->Alloc( skin->numSurfaces * sizeof( skinSurface_t ), h_low ) );
    ::memcpy( skin->surfaces, parseSurfaces, skin->numSurfaces * sizeof( skinSurface_t ) );
    
    return hSkin;
}

/*
===============
idRenderSystemSkinsLocal::InitSkins
===============
*/
void idRenderSystemSkinsLocal::InitSkins( void )
{
    skin_t* skin = nullptr;
    
    tr.numSkins = 1;
    
    // make the default skin have all default shaders
    skin = tr.skins[0] = ( skin_t* )memorySystem->Alloc( sizeof( skin_t ), h_low );
    Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name ) );
    skin->numSurfaces = 1;
    skin->surfaces = ( skinSurface_t* )memorySystem->Alloc( sizeof( skinSurface_t ), h_low );
    skin->surfaces[0].shader = tr.defaultShader;
}

/*
===============
idRenderSystemSkinsLocal::GetSkinByHandle
===============
*/
skin_t* idRenderSystemSkinsLocal::GetSkinByHandle( qhandle_t hSkin )
{
    if ( hSkin < 1 || hSkin >= tr.numSkins )
    {
        return tr.skins[0];
    }
    
    return tr.skins[ hSkin ];
}

/*
===============
idRenderSystemSkinsLocal::SkinList_f
===============
*/
void idRenderSystemSkinsLocal::SkinList_f( void )
{
    S32 i, j;
    skin_t* skin;
    
    clientMainSystem->RefPrintf( PRINT_ALL, "------------------\n" );
    
    for ( i = 0 ; i < tr.numSkins ; i++ )
    {
        skin = tr.skins[i];
        
        clientMainSystem->RefPrintf( PRINT_ALL, "%3i:%s (%d surfaces)\n", i, skin->name, skin->numSurfaces );
        
        for ( j = 0 ; j < skin->numSurfaces ; j++ )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "       %s = %s\n", skin->surfaces[j].name, skin->surfaces[j].shader->name );
        }
    }
    
    clientMainSystem->RefPrintf( PRINT_ALL, "------------------\n" );
}

void* idRenderSystemSkinsLocal::LocalMalloc( size_t size )
{
    return clientMainSystem->RefMalloc( ( S32 )size );
}

void* idRenderSystemSkinsLocal::LocalReallocSized( void* ptr, size_t old_size, size_t new_size )
{
    void* mem = clientMainSystem->RefMalloc( ( S32 )new_size );
    
    if ( ptr )
    {
        ::memcpy( mem, ptr, old_size );
        memorySystem->Free( ptr );
    }
    return mem;
}

void idRenderSystemSkinsLocal::LocalFree( void* ptr )
{
    if ( ptr )
    {
        memorySystem->Free( ptr );
    }
}


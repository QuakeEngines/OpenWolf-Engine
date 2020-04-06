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
// File name:   r_sky.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemSkyLocal renderSystemSkyLocal;

/*
===============
idRenderSystemSkyLocal::idRenderSystemSkyLocal
===============
*/
idRenderSystemSkyLocal::idRenderSystemSkyLocal( void )
{
}

/*
===============
idRenderSystemSkyLocal::~idRenderSystemSkyLocal
===============
*/
idRenderSystemSkyLocal::~idRenderSystemSkyLocal( void )
{
}

/*
================
idRenderSystemSkyLocal::AddSkyPolygon
================
*/
void idRenderSystemSkyLocal::AddSkyPolygon( S32 nump, vec3_t vecs )
{
    S32 i, j, axis;
    vec3_t v, av;
    F32	s, t, dv, *vp;
    
    // s = [0]/[2], t = [1]/[2]
    static S32	vec_to_st[6][3] =
    {
        { -2, 3, 1},
        {2, 3, -1},
        
        {1, 3, 2},
        { -1, 3, -2},
        
        { -2, -1, 3},
        { -2, 1, -3}
        
        //	{-1,2,3},
        //	{1,2,-3}
    };
    
    // decide which face it maps to
    VectorCopy( vec3_origin, v );
    
    for ( i = 0, vp = vecs ; i < nump ; i++, vp += 3 )
    {
        VectorAdd( vp, v, v );
    }
    
    av[0] = Q_fabs( v[0] );
    av[1] = Q_fabs( v[1] );
    av[2] = Q_fabs( v[2] );
    
    if ( av[0] > av[1] && av[0] > av[2] )
    {
        if ( v[0] < 0 )
        {
            axis = 1;
        }
        else
        {
            axis = 0;
        }
    }
    else if ( av[1] > av[2] && av[1] > av[0] )
    {
        if ( v[1] < 0 )
        {
            axis = 3;
        }
        else
        {
            axis = 2;
        }
    }
    else
    {
        if ( v[2] < 0 )
        {
            axis = 5;
        }
        else
        {
            axis = 4;
        }
    }
    
    // project new texture coords
    for ( i = 0 ; i < nump ; i++, vecs += 3 )
    {
        j = vec_to_st[axis][2];
        if ( j > 0 )
        {
            dv = vecs[j - 1];
        }
        else
        {
            dv = -vecs[-j - 1];
        }
        
        if ( dv < 0.001f )
        {
            // don't divide by zero
            continue;
        }
        
        j = vec_to_st[axis][0];
        
        if ( j < 0 )
        {
            s = -vecs[-j - 1] / dv;
        }
        else
        {
            s = vecs[j - 1] / dv;
        }
        
        j = vec_to_st[axis][1];
        
        if ( j < 0 )
        {
            t = -vecs[-j - 1] / dv;
        }
        else
        {
            t = vecs[j - 1] / dv;
        }
        
        if ( s < sky_mins[0][axis] )
        {
            sky_mins[0][axis] = s;
        }
        
        if ( t < sky_mins[1][axis] )
        {
            sky_mins[1][axis] = t;
        }
        
        if ( s > sky_maxs[0][axis] )
        {
            sky_maxs[0][axis] = s;
        }
        
        if ( t > sky_maxs[1][axis] )
        {
            sky_maxs[1][axis] = t;
        }
    }
}

/*
================
idRenderSystemSkyLocal::ClipSkyPolygon
================
*/
void idRenderSystemSkyLocal::ClipSkyPolygon( S32 nump, vec3_t vecs, S32 stage )
{
    S32		sides[MAX_CLIP_VERTS], newc[2], i, j;
    F32*	norm, *v, d, e, dists[MAX_CLIP_VERTS];
    vec3_t	newv[2][MAX_CLIP_VERTS];
    bool	front, back;
    
    if ( nump > MAX_CLIP_VERTS - 2 )
    {
        Com_Error( ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS" );
    }
    
    if ( stage == 6 )
    {
        // fully clipped, so draw it
        AddSkyPolygon( nump, vecs );
        return;
    }
    
    front = back = false;
    norm = sky_clip[stage];
    
    for ( i = 0, v = vecs ; i < nump ; i++, v += 3 )
    {
        d = DotProduct( v, norm );
        if ( d > ON_EPSILON )
        {
            front = true;
            sides[i] = SIDE_FRONT;
        }
        else if ( d < -ON_EPSILON )
        {
            back = true;
            sides[i] = SIDE_BACK;
        }
        else
        {
            sides[i] = SIDE_ON;
        }
        
        dists[i] = d;
    }
    
    if ( !front || !back )
    {
        // not clipped
        ClipSkyPolygon( nump, vecs, stage + 1 );
        return;
    }
    
    // clip it
    sides[i] = sides[0];
    dists[i] = dists[0];
    VectorCopy( vecs, ( vecs + ( i * 3 ) ) );
    newc[0] = newc[1] = 0;
    
    for ( i = 0, v = vecs ; i < nump ; i++, v += 3 )
    {
        switch ( sides[i] )
        {
            case SIDE_FRONT:
                VectorCopy( v, newv[0][newc[0]] );
                newc[0]++;
                break;
            case SIDE_BACK:
                VectorCopy( v, newv[1][newc[1]] );
                newc[1]++;
                break;
            case SIDE_ON:
                VectorCopy( v, newv[0][newc[0]] );
                newc[0]++;
                VectorCopy( v, newv[1][newc[1]] );
                newc[1]++;
                break;
        }
        
        if ( sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] )
        {
            continue;
        }
        
        d = dists[i] / ( dists[i] - dists[i + 1] );
        for ( j = 0 ; j < 3 ; j++ )
        {
            e = v[j] + d * ( v[j + 3] - v[j] );
            newv[0][newc[0]][j] = e;
            newv[1][newc[1]][j] = e;
        }
        newc[0]++;
        newc[1]++;
    }
    
    // continue
    ClipSkyPolygon( newc[0], newv[0][0], stage + 1 );
    ClipSkyPolygon( newc[1], newv[1][0], stage + 1 );
}

/*
==============
idRenderSystemSkyLocal::ClearSkyBox
==============
*/
void idRenderSystemSkyLocal::ClearSkyBox( void )
{
    S32		i;
    
    for ( i = 0 ; i < 6 ; i++ )
    {
        sky_mins[0][i] = sky_mins[1][i] = 9999;
        sky_maxs[0][i] = sky_maxs[1][i] = -9999;
    }
}

/*
================
idRenderSystemSkyLocal::ClipSkyPolygons
================
*/
void idRenderSystemSkyLocal::ClipSkyPolygons( shaderCommands_t* input )
{
    vec3_t		p[5];	// need one extra point for clipping
    S32			i, j;
    
    ClearSkyBox();
    
    for ( i = 0; i < input->numIndexes; i += 3 )
    {
        for ( j = 0 ; j < 3 ; j++ )
        {
            VectorSubtract( input->xyz[input->indexes[i + j]],
                            backEnd.viewParms.orientation.origin,
                            p[j] );
        }
        ClipSkyPolygon( 3, p[0], 0 );
    }
}

/*
================
idRenderSystemSkyLocal::MakeSkyVec

Parms: s, t range from -1 to 1
================
*/
void idRenderSystemSkyLocal::MakeSkyVec( F32 s, F32 t, S32 axis, F32 outSt[2], vec3_t outXYZ )
{
    // 1 = s, 2 = t, 3 = 2048
    static S32	st_to_vec[6][3] =
    {
        {3, -1, 2},
        { -3, 1, 2},
        
        {1, 3, 2},
        { -1, -3, 2},
        
        { -2, -1, 3},		// 0 degrees yaw, look straight up
        {2, -1, -3}		// look straight down
    };
    
    S32	j, k;
    F32	boxSize;
    vec3_t b;
    
    boxSize = backEnd.viewParms.zFar / 1.75f; // div sqrt(3)
    b[0] = s * boxSize;
    b[1] = t * boxSize;
    b[2] = boxSize;
    
    for ( j = 0 ; j < 3 ; j++ )
    {
        k = st_to_vec[axis][j];
        
        if ( k < 0 )
        {
            outXYZ[j] = -b[-k - 1];
        }
        else
        {
            outXYZ[j] = b[k - 1];
        }
    }
    
    // avoid bilerp seam
    s = ( s + 1 ) * 0.5f;
    t = ( t + 1 ) * 0.5f;
    
    if ( s < sky_min )
    {
        s = sky_min;
    }
    else if ( s > sky_max )
    {
        s = sky_max;
    }
    
    if ( t < sky_min )
    {
        t = sky_min;
    }
    else if ( t > sky_max )
    {
        t = sky_max;
    }
    
    t = 1.0f - t;
    
    if ( outSt )
    {
        outSt[0] = s;
        outSt[1] = t;
    }
}

/*
================
idRenderSystemSkyLocal::DrawSkySide
================
*/
void idRenderSystemSkyLocal::DrawSkySide( struct image_s* image, const S32 mins[2], const S32 maxs[2] )
{
    S32 s, t;
    S32 firstVertex = tess.numVertexes;
    //S32 firstIndex = tess.numIndexes;
    vec4_t color;
    
    //tess.numVertexes = 0;
    //tess.numIndexes = 0;
    tess.firstIndex = tess.numIndexes;
    
    idRenderSystemBackendLocal::BindToTMU( image, TB_COLORMAP );
    idRenderSystemBackendLocal::Cull( CT_TWO_SIDED );
    
    for ( t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++ )
    {
        for ( s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++ )
        {
            tess.xyz[tess.numVertexes][0] = s_skyPoints[t][s][0];
            tess.xyz[tess.numVertexes][1] = s_skyPoints[t][s][1];
            tess.xyz[tess.numVertexes][2] = s_skyPoints[t][s][2];
            tess.xyz[tess.numVertexes][3] = 1.0;
            
            tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
            tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];
            
            tess.numVertexes++;
            
            if ( tess.numVertexes >= SHADER_MAX_VERTEXES )
            {
                Com_Error( ERR_DROP, "SHADER_MAX_VERTEXES hit in DrawSkySideVBO()" );
            }
        }
    }
    
    for ( t = 0; t < maxs[1] - mins[1]; t++ )
    {
        for ( s = 0; s < maxs[0] - mins[0]; s++ )
        {
            if ( tess.numIndexes + 6 >= SHADER_MAX_INDEXES )
            {
                Com_Error( ERR_DROP, "SHADER_MAX_INDEXES hit in DrawSkySideVBO()" );
            }
            
            tess.indexes[tess.numIndexes++] =  s +       t      * ( maxs[0] - mins[0] + 1 ) + firstVertex;
            tess.indexes[tess.numIndexes++] =  s + ( t + 1 ) * ( maxs[0] - mins[0] + 1 ) + firstVertex;
            tess.indexes[tess.numIndexes++] = ( s + 1 ) +  t      * ( maxs[0] - mins[0] + 1 ) + firstVertex;
            
            tess.indexes[tess.numIndexes++] = ( s + 1 ) +  t      * ( maxs[0] - mins[0] + 1 ) + firstVertex;
            tess.indexes[tess.numIndexes++] =  s + ( t + 1 ) * ( maxs[0] - mins[0] + 1 ) + firstVertex;
            tess.indexes[tess.numIndexes++] = ( s + 1 ) + ( t + 1 ) * ( maxs[0] - mins[0] + 1 ) + firstVertex;
        }
    }
    
    // FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
    idRenderSystemVaoLocal::UpdateTessVao( ATTR_POSITION | ATTR_TEXCOORD );
    /*
    	{
    		shaderProgram_t *sp = &tr.textureColorShader;
    
    		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD);
    		idRenderSystemGLSLLocal::BindProgram(sp);
    
    		idRenderSystemGLSLLocal::SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
    
    		color[0] =
    		color[1] =
    		color[2] = tr.identityLight;
    		color[3] = 1.0f;
    		idRenderSystemGLSLLocal::SetUniformVec4(sp, UNIFORM_COLOR, color);
    	}
    */
    {
        shaderProgram_t* sp = &tr.lightallShader[0];
        vec4_t vector;
        
        idRenderSystemGLSLLocal::BindProgram( sp );
        
        idRenderSystemGLSLLocal::SetUniformMat4( sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection );
        
        color[0] =
            color[1] =
                color[2] =
                    color[3] = 1.0f;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_BASECOLOR, color );
        
        color[0] =
            color[1] =
                color[2] =
                    color[3] = 0.0f;
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_VERTCOLOR, color );
        
        VectorSet4( vector, 1.0, 0.0, 0.0, 1.0 );
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXMATRIX, vector );
        
        VectorSet4( vector, 0.0, 0.0, 0.0, 0.0 );
        idRenderSystemGLSLLocal::SetUniformVec4( sp, UNIFORM_DIFFUSETEXOFFTURB, vector );
        
        idRenderSystemGLSLLocal::SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
    }
    
    idRenderSystemShadeLocal::DrawElements( tess.numIndexes - tess.firstIndex, tess.firstIndex );
    
    //qglDrawElements(GL_TRIANGLES, tess.numIndexes - tess.firstIndex, GL_INDEX_TYPE, BUFFER_OFFSET(tess.firstIndex * sizeof(glIndex_t)));
    
    //R_BindNullVBO();
    //R_BindNullIBO();
    
    tess.numIndexes = tess.firstIndex;
    tess.numVertexes = firstVertex;
    tess.firstIndex = 0;
}

/*
================
idRenderSystemSkyLocal::DrawSkyBox
================
*/
void idRenderSystemSkyLocal::DrawSkyBox( shader_t* shader )
{
    S32		i;
    
    sky_min = 0;
    sky_max = 1;
    
    ::memset( s_skyTexCoords, 0, sizeof( s_skyTexCoords ) );
    
    for ( i = 0 ; i < 6 ; i++ )
    {
        S32 sky_mins_subd[2], sky_maxs_subd[2];
        S32 s, t;
        
        sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        
        if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) || ( sky_mins[1][i] >= sky_maxs[1][i] ) )
        {
            continue;
        }
        
        sky_mins_subd[0] = ( S32 )( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS );
        sky_mins_subd[1] = ( S32 )( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS );
        sky_maxs_subd[0] = ( S32 )( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS );
        sky_maxs_subd[1] = ( S32 )( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS );
        
        if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
        }
        else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
        }
        
        if ( sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
        }
        else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;
        }
        
        if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
        }
        else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
        }
        
        if ( sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
        }
        else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;
        }
        
        // iterate through the subdivisions
        for ( t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++ )
        {
            for ( s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++ )
            {
                MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( F32 ) HALF_SKY_SUBDIVISIONS,
                            ( t - HALF_SKY_SUBDIVISIONS ) / ( F32 ) HALF_SKY_SUBDIVISIONS,
                            i, s_skyTexCoords[t][s], s_skyPoints[t][s] );
            }
        }
        
        DrawSkySide( shader->sky.outerbox[sky_texorder[i]], sky_mins_subd, sky_maxs_subd );
    }
    
}

/*
================
idRenderSystemSkyLocal::DrawSkyBox
================
*/
void idRenderSystemSkyLocal::FillCloudySkySide( const S32 mins[2], const S32 maxs[2], bool addIndexes )
{
    S32 s, t, tHeight, sWidth, vertexStart = tess.numVertexes;
    
    tHeight = maxs[1] - mins[1] + 1;
    sWidth = maxs[0] - mins[0] + 1;
    
    for ( t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++ )
    {
        for ( s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++ )
        {
            VectorAdd( s_skyPoints[t][s], backEnd.viewParms.orientation.origin, tess.xyz[tess.numVertexes] );
            tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
            tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];
            
            tess.numVertexes++;
            
            if ( tess.numVertexes >= SHADER_MAX_VERTEXES )
            {
                Com_Error( ERR_DROP, "SHADER_MAX_VERTEXES hit in FillCloudySkySide()" );
            }
        }
    }
    
    // only add indexes for one pass, otherwise it would draw multiple times for each pass
    if ( addIndexes )
    {
        for ( t = 0; t < tHeight - 1; t++ )
        {
            for ( s = 0; s < sWidth - 1; s++ )
            {
                tess.indexes[tess.numIndexes] = vertexStart + s + t * ( sWidth );
                tess.numIndexes++;
                tess.indexes[tess.numIndexes] = vertexStart + s + ( t + 1 ) * ( sWidth );
                tess.numIndexes++;
                tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * ( sWidth );
                tess.numIndexes++;
                
                tess.indexes[tess.numIndexes] = vertexStart + s + ( t + 1 ) * ( sWidth );
                tess.numIndexes++;
                tess.indexes[tess.numIndexes] = vertexStart + s + 1 + ( t + 1 ) * ( sWidth );
                tess.numIndexes++;
                tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * ( sWidth );
                tess.numIndexes++;
            }
        }
    }
}

/*
================
idRenderSystemSkyLocal::FillCloudBox
================
*/
void idRenderSystemSkyLocal::FillCloudBox( const shader_t* shader, S32 stage )
{
    S32 i;
    
    for ( i = 0; i < 6; i++ )
    {
        S32 sky_mins_subd[2], sky_maxs_subd[2];
        S32 s, t;
        F32 MIN_T;
        
        if ( 1 ) // FIXME? shader->sky.fullClouds )
        {
            MIN_T = -HALF_SKY_SUBDIVISIONS;
            
            // still don't want to draw the bottom, even if fullClouds
            if ( i == 5 )
            {
                continue;
            }
        }
        else
        {
            switch ( i )
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    MIN_T = -1;
                    break;
                case 5:
                    // don't draw clouds beneath you
                    continue;
                case 4:		// top
                default:
                    MIN_T = -HALF_SKY_SUBDIVISIONS;
                    break;
            }
        }
        
        sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
        
        if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) || ( sky_mins[1][i] >= sky_maxs[1][i] ) )
        {
            continue;
        }
        
        sky_mins_subd[0] = static_cast<S32>( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS );
        sky_mins_subd[1] = static_cast<S32>( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS );
        sky_maxs_subd[0] = static_cast<S32>( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS );
        sky_maxs_subd[1] = static_cast<S32>( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS );
        
        if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
        }
        else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
        }
        
        if ( sky_mins_subd[1] < MIN_T )
        {
            sky_mins_subd[1] = ( S32 )MIN_T;
        }
        else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS )
        {
            sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;
        }
        
        if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
        }
        else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
        }
        
        if ( sky_maxs_subd[1] < MIN_T )
        {
            sky_maxs_subd[1] = ( S32 )MIN_T;
        }
        else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS )
        {
            sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;
        }
        
        // iterate through the subdivisions
        for ( t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++ )
        {
            for ( s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++ )
            {
                MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( F32 ) HALF_SKY_SUBDIVISIONS,
                            ( t - HALF_SKY_SUBDIVISIONS ) / ( F32 ) HALF_SKY_SUBDIVISIONS,
                            i, NULL, s_skyPoints[t][s] );
                            
                s_skyTexCoords[t][s][0] = s_cloudTexCoords[i][t][s][0];
                s_skyTexCoords[t][s][1] = s_cloudTexCoords[i][t][s][1];
            }
        }
        
        // only add indexes for first stage
        FillCloudySkySide( sky_mins_subd, sky_maxs_subd, ( stage == 0 ) );
    }
}

/*
================
idRenderSystemSkyLocal::FillCloudBox
================
*/
void idRenderSystemSkyLocal::BuildCloudData( shaderCommands_t* input )
{
    S32			i;
    shader_t*	shader;
    
    shader = input->shader;
    
    assert( shader->isSky );
    
    sky_min = 1.0f / 256.0f; // FIXME: not correct?
    sky_max = 255.0f / 256.0f;
    
    // set up for drawing
    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.firstIndex = 0;
    
    if ( shader->sky.cloudHeight )
    {
        for ( i = 0; i < MAX_SHADER_STAGES; i++ )
        {
            if ( !tess.xstages[i] )
            {
                break;
            }
            
            FillCloudBox( shader, i );
        }
    }
}

/*
================
idRenderSystemSkyLocal::InitSkyTexCoords

Called when a sky shader is parsed
================
*/
void idRenderSystemSkyLocal::InitSkyTexCoords( F32 heightCloud )
{
    S32 i, s, t;
    F32 p, sRad, tRad, radiusWorld = 4096;
    vec3_t v, skyVec;
    
    // init zfar so MakeSkyVec works even though
    // a world hasn't been bounded
    backEnd.viewParms.zFar = 1024;
    
    for ( i = 0; i < 6; i++ )
    {
        for ( t = 0; t <= SKY_SUBDIVISIONS; t++ )
        {
            for ( s = 0; s <= SKY_SUBDIVISIONS; s++ )
            {
                // compute vector from view origin to sky side integral point
                MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( F32 ) HALF_SKY_SUBDIVISIONS,
                            ( t - HALF_SKY_SUBDIVISIONS ) / ( F32 ) HALF_SKY_SUBDIVISIONS,
                            i,
                            NULL,
                            skyVec );
                            
                // compute parametric value 'p' that intersects with cloud layer
                p = ( 1.0f / ( 2 * DotProduct( skyVec, skyVec ) ) ) *
                    ( -2 * skyVec[2] * radiusWorld +
                      2 * sqrt( SQR( skyVec[2] ) * SQR( radiusWorld ) +
                                2 * SQR( skyVec[0] ) * radiusWorld * heightCloud +
                                SQR( skyVec[0] ) * SQR( heightCloud ) +
                                2 * SQR( skyVec[1] ) * radiusWorld * heightCloud +
                                SQR( skyVec[1] ) * SQR( heightCloud ) +
                                2 * SQR( skyVec[2] ) * radiusWorld * heightCloud +
                                SQR( skyVec[2] ) * SQR( heightCloud ) ) );
                                
                s_cloudTexP[i][t][s] = p;
                
                // compute intersection point based on p
                VectorScale( skyVec, p, v );
                v[2] += radiusWorld;
                
                // compute vector from world origin to intersection point 'v'
                VectorNormalize( v );
                
                sRad = Q_acos( v[0] );
                tRad = Q_acos( v[1] );
                
                s_cloudTexCoords[i][t][s][0] = sRad;
                s_cloudTexCoords[i][t][s][1] = tRad;
            }
        }
    }
}

/*
================
idRenderSystemSkyLocal::DrawSun
================
*/
void idRenderSystemSkyLocal::DrawSun( F32 scale, shader_t* shader )
{
    F32 size, dist;
    vec3_t origin, vec1, vec2;
    U8 sunColor[4] = { 255, 255, 255, 255 };
    
    if ( !shader )
    {
        return;
    }
    
    if ( !backEnd.skyRenderedThisView )
    {
        return;
    }
    
    {
        // FIXME: this could be a lot cleaner
        mat4_t translation, modelview;
        
        idRenderSystemMathsLocal::Mat4Translation( backEnd.viewParms.orientation.origin, translation );
        idRenderSystemMathsLocal::Mat4Multiply( backEnd.viewParms.world.modelMatrix, translation, modelview );
        idRenderSystemBackendLocal::SetModelviewMatrix( modelview );
    }
    
    dist =	( F32 )( backEnd.viewParms.zFar / 1.75 );
    size = dist * scale;
    
    if ( r_proceduralSun->integer )
    {
        size *= r_proceduralSunScale->value;
    }
    
    VectorScale( tr.sunDirection, dist, origin );
    PerpendicularVector( vec1, tr.sunDirection );
    CrossProduct( tr.sunDirection, vec1, vec2 );
    
    VectorScale( vec1, size, vec1 );
    VectorScale( vec2, size, vec2 );
    
    // farthest depth range
    qglDepthRange( 1.0, 1.0 );
    
    idRenderSystemShadeLocal::BeginSurface( shader, 0, 0 );
    
    idRenderSystemSurfaceLocal::AddQuadStamp( origin, vec1, vec2, tr.refdef.sunAmbCol );
    
    idRenderSystemShadeLocal::EndSurface();
    
    // back to normal depth range
    qglDepthRange( 0.0, 1.0 );
}

/*
================
idRenderSystemSkyLocal::StageIteratorSky

All of the visible sky triangles are in tess

Other things could be stuck in here, like birds in the sky, etc
================
*/
void idRenderSystemSkyLocal::StageIteratorSky( void )
{
    if ( r_fastsky->integer )
    {
        return;
    }
    
    // go through all the polygons and project them onto
    // the sky box to see which blocks on each side need
    // to be drawn
    ClipSkyPolygons( &tess );
    
    // r_showsky will let all the sky blocks be drawn in
    // front of everything to allow developers to see how
    // much sky is getting sucked in
    if ( r_showsky->integer )
    {
        qglDepthRange( 0.0, 0.0 );
    }
    else
    {
        qglDepthRange( 1.0, 1.0 );
    }
    
    // draw the outer skybox
    if ( tess.shader->sky.outerbox[0] && tess.shader->sky.outerbox[0] != tr.defaultImage )
    {
        mat4_t oldmodelview;
        
        idRenderSystemBackendLocal::State( 0 );
        idRenderSystemBackendLocal::Cull( CT_FRONT_SIDED );
        //qglTranslatef (backEnd.viewParms.orientation.origin[0], backEnd.viewParms.orientation.origin[1], backEnd.viewParms.orientation.origin[2]);
        
        {
            // FIXME: this could be a lot cleaner
            mat4_t trans, product;
            
            idRenderSystemMathsLocal::Mat4Copy( glState.modelview, oldmodelview );
            idRenderSystemMathsLocal::Mat4Translation( backEnd.viewParms.orientation.origin, trans );
            idRenderSystemMathsLocal::Mat4Multiply( glState.modelview, trans, product );
            idRenderSystemBackendLocal::SetModelviewMatrix( product );
            
        }
        
        DrawSkyBox( tess.shader );
        
        idRenderSystemBackendLocal::SetModelviewMatrix( oldmodelview );
    }
    
    // generate the vertexes for all the clouds, which will be drawn
    // by the generic shader routine
    BuildCloudData( &tess );
    
    if ( tess.numVertexes > 0 )
    {
        idRenderSystemShadeLocal::StageIteratorGeneric();
    }
    
    // draw the inner skybox
    
    // back to normal depth range
    qglDepthRange( 0.0, 1.0 );
    
    // note that sky was drawn so we will draw a sun later
    backEnd.skyRenderedThisView = true;
}

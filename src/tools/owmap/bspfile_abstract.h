////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 JV Software
// Copyright(C) 2011 - 2018 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   bspfile_abstract.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2015
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IDBSP_ABSTRACT__
#define __IDBSP_ABSTRACT__

#include <framework/types.h>

/* -------------------------------------------------------------------------------

abstracted bsp file

------------------------------------------------------------------------------- */

#define EXTERNAL_LIGHTMAP       "lm_%04d.tga"
#define EXTERNAL_HDR_LIGHTMAP   "lm_%04d.hdr"

#define MAX_LIGHTMAPS			2	/* RBSP */
#define MAX_LIGHT_STYLES		64
#define	MAX_SWITCHED_LIGHTS		32
#define LS_NORMAL				0x00
#define LS_UNUSED				0xFE
#define	LS_NONE					0xFF

#define MAX_LIGHTMAP_SHADERS	256

/* ok to increase these at the expense of more memory */
#ifdef Q3MAP2
#define	MAX_MAP_ENTITIES		0x3000		// Tr3B: old 0x1000 //%	0x800	/* ydnar */
#define	MAX_MAP_ENTSTRING		0x100000    //Dushan: old 0x80000 //% 0x40000 /* ydnar */
#endif
#define	MAX_MAP_AREAS			0x100		/* MAX_MAP_AREA_BYTES in q_shared must match! */
#define	MAX_MAP_FOGS			0x100		//& 0x100	/* RBSP (32 - world fog - goggles) */
#define	MAX_MAP_LEAFS			0x20000
#define	MAX_MAP_PORTALS			0x20000
#define	MAX_MAP_LIGHTING		0x800000
#ifdef Q3MAP2
#define	MAX_MAP_LIGHTGRID		0x100000	//% 0x800000 /* ydnar: set to points, not bytes */
#define MAX_MAP_VISCLUSTERS     0x4000	// <= MAX_MAP_LEAFS
#define	MAX_MAP_VISIBILITY		(VIS_HEADER_SIZE + MAX_MAP_VISCLUSTERS * (((MAX_MAP_VISCLUSTERS + 63) & ~63) >> 3))
#define	MAX_MAP_DRAW_SURFS		0x20000
#define	MAX_MAP_DRAW_VERTS		0x200000	//% 0x80000 /* Tr3B */
#define	MAX_MAP_DRAW_INDEXES	0x200000	//% 0x80000 /* Tr3B */
#endif

#define MAX_MAP_ADVERTISEMENTS	30

/* key / value pair sizes in the entities lump */
#define	MAX_KEY					32
#define	MAX_VALUE				1024

/* the editor uses these predefined yaw angles to orient entities up or down */
#define	ANGLE_UP				-1
#define	ANGLE_DOWN				-2

#define	LIGHTMAP_WIDTH			128
#define	LIGHTMAP_HEIGHT			128

#ifdef Q3MAP2
#define MIN_WORLD_COORD			(-65536)
#define	MAX_WORLD_COORD			(65536)
#define WORLD_SIZE				(MAX_WORLD_COORD - MIN_WORLD_COORD)
#endif

typedef void( *bspFunc )( StringEntry );


typedef struct
{
    S32             offset, length;
}
bspLump_t;


typedef struct
{
    UTF8            ident[4];
    S32             version;
    
    bspLump_t       lumps[100];	/* theoretical maximum # of bsp lumps */
}
bspHeader_t;

#ifdef Q3MAP2
typedef struct
{
    F32           mins[3], maxs[3];
    S32             firstBSPSurface, numBSPSurfaces;
    S32             firstBSPBrush, numBSPBrushes;
}
bspModel_t;
#endif

typedef struct
{
    UTF8            shader[64];
    S32             surfaceFlags;
    S32             contentFlags;
}
bspShader_t;


/* planes x^1 is allways the opposite of plane x */

typedef struct
{
    F32           normal[3];
    F32           dist;
}
bspPlane_t;

#ifdef Q3MAP2
typedef struct
{
    S32             planeNum;
    S32             children[2];	/* negative numbers are -(leafs+1), not nodes */
    S32             mins[3];	/* for frustom culling */
    S32             maxs[3];
}
bspNode_t;
#endif

typedef struct
{
    S32             cluster;	/* -1 = opaque cluster (do I still store these?) */
    S32             area;
    
    S32             mins[3];	/* for frustum culling */
    S32             maxs[3];
    
    S32             firstBSPLeafSurface;
    S32             numBSPLeafSurfaces;
    
    S32             firstBSPLeafBrush;
    S32             numBSPLeafBrushes;
}
bspLeaf_t;


typedef struct
{
    S32             planeNum;	/* positive plane side faces out of the leaf */
    S32             shaderNum;
    S32             surfaceNum;	/* RBSP */
}
bspBrushSide_t;


typedef struct
{
    S32             firstSide;
    S32             numSides;
    S32             shaderNum;	/* the shader that determines the content flags */
}
bspBrush_t;


typedef struct
{
    UTF8            shader[64];
    S32             brushNum;
    S32             visibleSide;	/* the brush side that ray tests need to clip against (-1 == none) */
}
bspFog_t;


typedef struct
{
    vec3_t xyz;
    float st[ 2 ];
    float lightmap[ MAX_LIGHTMAPS ][ 2 ];
    vec3_t normal;
    byte color[MAX_LIGHTMAPS][4];
}
bspDrawVert_t;

#if defined (Q3MAP2)
typedef enum
{
    MST_BAD,
    MST_PLANAR,
    MST_PATCH,
    MST_TRIANGLE_SOUP,
    MST_FLARE,
    MST_FOLIAGE
}
bspSurfaceType_t;

typedef struct bspGridPoint_s
{
    U8 ambient[MAX_LIGHTMAPS][3];
    U8 directed[MAX_LIGHTMAPS][3];
    U8 styles[MAX_LIGHTMAPS];
    U8 latLong[2];
}
bspGridPoint_t;
#endif

typedef struct
{
    S32             shaderNum;
    S32             fogNum;
    S32             surfaceType;
    
    S32             firstVert;
    S32             numVerts;
    
    S32             firstIndex;
    S32             numIndexes;
    
    U8            lightmapStyles[MAX_LIGHTMAPS];	/* RBSP */
    U8            vertexStyles[MAX_LIGHTMAPS];	/* RBSP */
    S32             lightmapNum[MAX_LIGHTMAPS];	/* RBSP */
    S32             lightmapX[MAX_LIGHTMAPS], lightmapY[MAX_LIGHTMAPS];	/* RBSP */
    S32             lightmapWidth, lightmapHeight;
    
    vec3_t          lightmapOrigin;
    vec3_t          lightmapVecs[3];	/* on patches, [ 0 ] and [ 1 ] are lodbounds */
    
    S32             patchWidth;
    S32             patchHeight;
}
bspDrawSurface_t;


/* advertisements */
typedef struct
{
    S32             cellId;
    vec3_t          normal;
    vec3_t          rect[4];
    UTF8            model[MAX_QPATH];
} bspAdvertisement_t;

#endif //!__IDBSP_ABSTRACT__

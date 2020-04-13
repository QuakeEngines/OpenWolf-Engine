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
// File name:   r_local.h
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_LOCAL_H__
#define __R_LOCAL_H__

#pragma once

#if !defined ( DEDICATED ) && !defined ( UPDATE_SERVER )

typedef void ( *xcommand_t )( void );

extern F32 displayAspect;	// FIXME

#define GLE(ret, name, ...) extern name##proc * qgl##name;
QGL_1_1_PROCS;
QGL_1_1_FIXED_FUNCTION_PROCS;
QGL_DESKTOP_1_1_PROCS;
QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
QGL_ES_1_1_PROCS;
QGL_ES_1_1_FIXED_FUNCTION_PROCS;
QGL_1_3_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
QGL_3_0_PROCS;
QGL_4_0_PROCS;
QGL_ARB_occlusion_query_PROCS;
QGL_ARB_framebuffer_object_PROCS;
QGL_ARB_vertex_array_object_PROCS;
QGL_EXT_direct_state_access_PROCS;
#undef GLE

#define GL_INDEX_TYPE		GL_UNSIGNED_INT

#define BUFFER_OFFSET(i) ((UTF8 *)NULL + (i))

// 14 bits
// can't be increased without changing bit packing for drawsurfs
// see QSORT_SHADERNUM_SHIFT
#define SHADERNUM_BITS	14
#define MAX_SHADERS		(1<<SHADERNUM_BITS)

#define	MAX_FBOS      64
#define MAX_VISCOUNTS 5
#define MAX_VAOS      4096

#define MAX_CALC_PSHADOWS    64
#define MAX_DRAWN_PSHADOWS    32 // do not increase past 32, because bit flags are used on surfaces
#define PSHADOW_MAP_SIZE      512

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

extern vidconfig_t glConfig; // outside of TR since it shouldn't be cleared during ref re-init

typedef struct cubemap_s
{
    UTF8 name[MAX_QPATH];
    vec3_t origin;
    F32 parallaxRadius;
    image_t* image;
} cubemap_t;

typedef struct dlight_s
{
    vec3_t	origin;
    vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
    F32	radius;
    
    vec3_t	transformed;		// origin in local coordinate system
    S32		additive;			// texture detail is lost tho when the lightmap is dark
} dlight_t;

#define MAX_ENTITY_LIGHTS	4			// max lights that will effect an entity

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct
{
    refEntity_t	e;
    
    F32		axisLength;		// compensate for non-normalized axis
    
    bool	needDlights;	// true for bmodels that touch a dlight
    bool	lightingCalculated;
    bool	mirrored;		// mirrored matrix, needs reversed culling
    vec3_t		lightDir;		// normalized direction towards light, in world space
    vec3_t      modelLightDir;  // normalized direction towards light, in model space
    vec3_t		ambientLight;	// color normalized to 0-255
    S32			ambientLightInt;	// 32 bit rgba packed
    vec3_t		directedLight;
    dlight_s* lights[MAX_ENTITY_LIGHTS];		// lights that touch this entity.
    F32 lightdist[MAX_ENTITY_LIGHTS];
} trRefEntity_t;


typedef struct
{
    vec3_t		origin;			// in world coordinates
    vec3_t		axis[3];		// orientation in world
    vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
    F32		modelMatrix[16];
    F32		transformMatrix[16];
} orientationr_t;

// Ensure this is >= the ATTR_INDEX_COUNT enum below
#define VAO_MAX_ATTRIBS 16

typedef enum
{
    VAO_USAGE_STATIC,
    VAO_USAGE_DYNAMIC
} vaoUsage_t;

typedef struct vaoAttrib_s
{
    U32 enabled;
    U32 count;
    U32 type;
    U32 normalized;
    U32 stride;
    U32 offset;
}
vaoAttrib_t;

typedef struct vao_s
{
    UTF8            name[MAX_QPATH];
    
    U32        vao;
    
    U32        vertexesVBO;
    S32             vertexesSize;	// amount of memory data allocated for all vertices in bytes
    vaoAttrib_t     attribs[VAO_MAX_ATTRIBS];
    
    U32        frameSize;      // bytes to skip per frame when doing vertex animation
    
    U32        indexesIBO;
    S32             indexesSize;	// amount of memory data allocated for all triangles in bytes
} vao_t;

//===============================================================================

typedef enum
{
    SS_BAD,
    SS_PORTAL,			// mirrors, portals, viewscreens
    SS_ENVIRONMENT,		// sky box
    SS_OPAQUE,			// opaque
    
    SS_DECAL,			// scorch marks, etc.
    SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
    // in addition to alpha test
    SS_BANNER,
    
    SS_FOG,
    
    SS_UNDERWATER,		// for items that should be drawn in front of the water plane
    
    SS_BLEND0,			// regular transparency and filters
    SS_BLEND1,			// generally only used for additive type effects
    SS_BLEND2,
    SS_BLEND3,
    
    SS_BLEND6,
    SS_STENCIL_SHADOW,
    SS_ALMOST_NEAREST,	// gun smoke puffs
    
    SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum
{
    GF_NONE,
    
    GF_SIN,
    GF_SQUARE,
    GF_TRIANGLE,
    GF_SAWTOOTH,
    GF_INVERSE_SAWTOOTH,
    GF_NOISE,
    GF_RANDOM
} genFunc_t;


typedef enum
{
    DEFORM_NONE,
    DEFORM_WAVE,
    DEFORM_NORMALS,
    DEFORM_BULGE,
    DEFORM_MOVE,
    DEFORM_PROJECTION_SHADOW,
    DEFORM_AUTOSPRITE,
    DEFORM_AUTOSPRITE2,
    DEFORM_TEXT0,
    DEFORM_TEXT1,
    DEFORM_TEXT2,
    DEFORM_TEXT3,
    DEFORM_TEXT4,
    DEFORM_TEXT5,
    DEFORM_TEXT6,
    DEFORM_TEXT7
} deform_t;

// deformVertexes types that can be handled by the GPU
typedef enum
{
    // do not edit: same as genFunc_t
    
    DGEN_NONE,
    DGEN_WAVE_SIN,
    DGEN_WAVE_SQUARE,
    DGEN_WAVE_TRIANGLE,
    DGEN_WAVE_SAWTOOTH,
    DGEN_WAVE_INVERSE_SAWTOOTH,
    DGEN_WAVE_NOISE,
    
    // do not edit until this line
    
    DGEN_BULGE,
    DGEN_MOVE
} deformGen_t;

typedef enum
{
    AGEN_IDENTITY,
    AGEN_SKIP,
    AGEN_ENTITY,
    AGEN_ONE_MINUS_ENTITY,
    AGEN_VERTEX,
    AGEN_ONE_MINUS_VERTEX,
    AGEN_LIGHTING_SPECULAR,
    AGEN_WAVEFORM,
    AGEN_PORTAL,
    AGEN_CONST,
} alphaGen_t;

typedef enum
{
    CGEN_BAD,
    CGEN_IDENTITY_LIGHTING,	// tr.identityLight
    CGEN_IDENTITY,			// always (1,1,1,1)
    CGEN_ENTITY,			// grabbed from entity's modulate field
    CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
    CGEN_EXACT_VERTEX,		// tess.vertexColors
    CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
    CGEN_EXACT_VERTEX_LIT,	// like CGEN_EXACT_VERTEX but takes a light direction from the lightgrid
    CGEN_VERTEX_LIT,		// like CGEN_VERTEX but takes a light direction from the lightgrid
    CGEN_ONE_MINUS_VERTEX,
    CGEN_WAVEFORM,			// programmatically generated
    CGEN_LIGHTING_DIFFUSE,
    CGEN_FOG,				// standard fog
    CGEN_CONST				// fixed color
} colorGen_t;

typedef enum
{
    TCGEN_BAD,
    TCGEN_IDENTITY,			// clear to 0,0
    TCGEN_LIGHTMAP,
    TCGEN_TEXTURE,
    TCGEN_ENVIRONMENT_MAPPED,
    TCGEN_FOG,
    TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum
{
    ACFF_NONE,
    ACFF_MODULATE_RGB,
    ACFF_MODULATE_RGBA,
    ACFF_MODULATE_ALPHA
} acff_t;

typedef struct
{
    genFunc_t	func;
    
    F32 base;
    F32 amplitude;
    F32 phase;
    F32 frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum
{
    TMOD_NONE,
    TMOD_TRANSFORM,
    TMOD_TURBULENT,
    TMOD_SCROLL,
    TMOD_SCALE,
    TMOD_STRETCH,
    TMOD_ROTATE,
    TMOD_ENTITY_TRANSLATE
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct
{
    deform_t	deformation;			// vertex coordinate modification type
    
    vec3_t		moveVector;
    waveForm_t	deformationWave;
    F32		deformationSpread;
    
    F32		bulgeWidth;
    F32		bulgeHeight;
    F32		bulgeSpeed;
} deformStage_t;


typedef struct
{
    texMod_t		type;
    
    // used for TMOD_TURBULENT and TMOD_STRETCH
    waveForm_t		wave;
    
    // used for TMOD_TRANSFORM
    F32			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
    F32			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]
    
    // used for TMOD_SCALE
    F32			scale[2];			// s *= scale[0]
    // t *= scale[1]
    
    // used for TMOD_SCROLL
    F32			scroll[2];			// s' = s + scroll[0] * time
    // t' = t + scroll[1] * time
    
    // + = clockwise
    // - = counterclockwise
    F32			rotateSpeed;
    
} texModInfo_t;


#define	MAX_IMAGE_ANIMATIONS 128

typedef struct
{
    image_t*			image[MAX_IMAGE_ANIMATIONS];
    S32				numImageAnimations;
    F32			imageAnimationSpeed;
    
    texCoordGen_t	tcGen;
    vec3_t			tcGenVectors[2];
    
    S32				numTexMods;
    texModInfo_t*	texMods;
    
    S32				videoMapHandle;
    bool		isLightmap;
    bool		isVideoMap;
} textureBundle_t;

enum
{
    TB_COLORMAP    = 0,
    TB_DIFFUSEMAP  = 0,
    TB_LIGHTMAP    = 1,
    TB_LEVELSMAP   = 1,
    TB_SHADOWMAP3  = 1,
    TB_NORMALMAP   = 2,
    TB_DELUXEMAP   = 3,
    TB_SHADOWMAP2  = 3,
    TB_SPECULARMAP = 4,
    TB_SHADOWMAP   = 5,
    TB_CUBEMAP     = 6,
    TB_SHADOWMAP4  = 6,
    TB_GLOWMAP     = 7,
    TB_ENVBRDFMAP  = 8,
    NUM_TEXTURE_BUNDLES = 9
};

typedef enum
{
    // material shader stage types
    ST_COLORMAP = 0,			// vanilla Q3A style shader treatening
    ST_DIFFUSEMAP = 0,          // treat color and diffusemap the same
    ST_NORMALMAP,
    ST_NORMALPARALLAXMAP,
    ST_SPECULARMAP,
    ST_LIGHTMAP,
    ST_GLSL
} stageType_t;

typedef struct
{
    bool		active;
    bool		isDetail;
    bool        isWater;
    bool hasSpecular;
    textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];
    
    waveForm_t		rgbWave;
    colorGen_t		rgbGen;
    
    waveForm_t		alphaWave;
    alphaGen_t		alphaGen;
    
    U8			    constantColor[4];			// for CGEN_CONST and AGEN_CONST
    
    U32		        stateBits;					// GLS_xxxx mask
    
    acff_t			adjustColorsForFog;
    
    stageType_t     type;
    struct shaderProgram_s* glslShaderGroup;
    S32 glslShaderIndex;
    
    vec4_t normalScale;
    vec4_t specularScale;
    
} shaderStage_t;

struct shaderCommands_s;

typedef enum
{
    FP_NONE,		// surface is translucent and will just be adjusted properly
    FP_EQUAL,		// surface is opaque but possibly alpha tested
    FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct
{
    F32		cloudHeight;
    image_t*		outerbox[6], *innerbox[6];
} skyParms_t;

typedef struct
{
    vec3_t	color;
    F32	depthForOpaque;
} fogParms_t;


typedef struct shader_s
{
    UTF8		name[MAX_QPATH];		// game path, including extension
    S32			lightmapIndex;			// for a shader to match, both name and lightmapIndex must match
    
    S32			index;					// this shader == tr.shaders[index]
    S32			sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]
    
    F32		sort;					// lower numbered shaders draw before higher numbered
    
    bool	defaultShader;			// we want to return index 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls idRenderSystemLocal::RegisterShader again with
    // the same name, we don't try looking for it again
    
    bool	explicitlyDefined;		// found in a .shader file
    
    S32			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
    S32			contentFlags;
    
    bool	entityMergable;			// merge across entites optimizable (smoke, blood)
    
    bool	isSky;
    skyParms_t	sky;
    fogParms_t	fogParms;
    
    F32		portalRange;			// distance to fog out at
    bool	isPortal;
    
    cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
    bool	polygonOffset;			// set for decals and other items that must be offset
    bool	noMipMaps;				// for console fonts, 2D elements, etc.
    bool	noPicMip;				// for images that must always be full resolution
    
    fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals
    
    S32         vertexAttribs;          // not all shaders will need all data to be gathered
    
    S32			numDeforms;
    deformStage_t	deforms[MAX_SHADER_DEFORMS];
    
    S32			numUnfoggedPasses;
    shaderStage_t*	stages[MAX_SHADER_STAGES];
    
    S32 lightingStage;
    
    void	( *optimalStageIteratorFunc )( void );
    
    F64 clampTime;                                  // time this shader is clamped to
    F64 timeOffset;                                 // current time offset for this shader
    
    struct shader_s* remappedShader;                  // current shader this one is remapped too
    
    struct	shader_s*	next;
} shader_t;

enum
{
    ATTR_INDEX_POSITION       = 0,
    ATTR_INDEX_TEXCOORD       = 1,
    ATTR_INDEX_LIGHTCOORD     = 2,
    ATTR_INDEX_TANGENT        = 3,
    ATTR_INDEX_NORMAL         = 4,
    ATTR_INDEX_COLOR          = 5,
    ATTR_INDEX_PAINTCOLOR     = 6,
    ATTR_INDEX_LIGHTDIRECTION = 7,
    ATTR_INDEX_BONE_INDEXES   = 8,
    ATTR_INDEX_BONE_WEIGHTS   = 9,
    
    // GPU vertex animations
    ATTR_INDEX_POSITION2      = 10,
    ATTR_INDEX_TANGENT2       = 11,
    ATTR_INDEX_NORMAL2        = 12,
    
    ATTR_INDEX_COUNT          = 13
};

enum
{
    ATTR_POSITION =       1 << ATTR_INDEX_POSITION,
    ATTR_TEXCOORD =       1 << ATTR_INDEX_TEXCOORD,
    ATTR_LIGHTCOORD =     1 << ATTR_INDEX_LIGHTCOORD,
    ATTR_TANGENT =        1 << ATTR_INDEX_TANGENT,
    ATTR_NORMAL =         1 << ATTR_INDEX_NORMAL,
    ATTR_COLOR =          1 << ATTR_INDEX_COLOR,
    ATTR_PAINTCOLOR =     1 << ATTR_INDEX_PAINTCOLOR,
    ATTR_LIGHTDIRECTION = 1 << ATTR_INDEX_LIGHTDIRECTION,
    ATTR_BONE_INDEXES =   1 << ATTR_INDEX_BONE_INDEXES,
    ATTR_BONE_WEIGHTS =   1 << ATTR_INDEX_BONE_WEIGHTS,
    
    // for .md3 interpolation
    ATTR_POSITION2 =      1 << ATTR_INDEX_POSITION2,
    ATTR_TANGENT2 =       1 << ATTR_INDEX_TANGENT2,
    ATTR_NORMAL2 =        1 << ATTR_INDEX_NORMAL2,
    
    ATTR_DEFAULT = ATTR_POSITION,
    ATTR_BITS =	ATTR_POSITION |
                ATTR_TEXCOORD |
                ATTR_LIGHTCOORD |
                ATTR_TANGENT |
                ATTR_NORMAL |
                ATTR_COLOR |
                ATTR_PAINTCOLOR |
                ATTR_LIGHTDIRECTION |
                ATTR_BONE_INDEXES |
                ATTR_BONE_WEIGHTS |
                ATTR_POSITION2 |
                ATTR_TANGENT2 |
                ATTR_NORMAL2
};

enum
{
    GENERICDEF_USE_DEFORM_VERTEXES  = 0x0001,
    GENERICDEF_USE_TCGEN_AND_TCMOD  = 0x0002,
    GENERICDEF_USE_VERTEX_ANIMATION = 0x0004,
    GENERICDEF_USE_FOG              = 0x0008,
    GENERICDEF_USE_RGBAGEN          = 0x0010,
    GENERICDEF_USE_BONE_ANIMATION   = 0x0020,
    GENERICDEF_ALL                  = 0x003F,
    GENERICDEF_COUNT                = 0x0040,
};

enum
{
    FOGDEF_USE_DEFORM_VERTEXES  = 0x0001,
    FOGDEF_USE_VERTEX_ANIMATION = 0x0002,
    FOGDEF_USE_BONE_ANIMATION   = 0x0004,
    FOGDEF_ALL                  = 0x0007,
    FOGDEF_COUNT                = 0x0008,
};

enum
{
    DLIGHTDEF_USE_DEFORM_VERTEXES  = 0x0001,
    DLIGHTDEF_ALL                  = 0x0001,
    DLIGHTDEF_COUNT                = 0x0002,
};

enum
{
    LIGHTDEF_USE_LIGHTMAP        = 0x0001,
    LIGHTDEF_USE_LIGHT_VECTOR    = 0x0002,
    LIGHTDEF_USE_LIGHT_VERTEX    = 0x0003,
    LIGHTDEF_LIGHTTYPE_MASK      = 0x0003,
    LIGHTDEF_ENTITY_VERTEX_ANIMATION = 0x0004,
    LIGHTDEF_USE_TCGEN_AND_TCMOD = 0x0008,
    LIGHTDEF_USE_PARALLAXMAP     = 0x0010,
    LIGHTDEF_USE_SHADOWMAP       = 0x0020,
    LIGHTDEF_ENTITY_BONE_ANIMATION = 0x0040,
    LIGHTDEF_ALL                 = 0x007F,
    LIGHTDEF_COUNT               = 0x0080
};

enum
{
    SHADOWMAPDEF_USE_VERTEX_ANIMATION = 0x0001,
    SHADOWMAPDEF_USE_BONE_ANIMATION   = 0x0002,
    SHADOWMAPDEF_ALL                  = 0x0003,
    SHADOWMAPDEF_COUNT                = 0x0004
};

enum
{
    GLSL_INT,
    GLSL_FLOAT,
    GLSL_FLOAT5,
    GLSL_VEC2,
    GLSL_VEC3,
    GLSL_VEC4,
    GLSL_MAT16,
    GLSL_MAT16_BONEMATRIX
};

typedef enum
{
    UNIFORM_DIFFUSEMAP = 0,
    UNIFORM_LIGHTMAP,
    UNIFORM_NORMALMAP,
    UNIFORM_DELUXEMAP,
    UNIFORM_SPECULARMAP,
    UNIFORM_GLOWMAP,
    
    UNIFORM_TEXTUREMAP,
    UNIFORM_LEVELSMAP,
    UNIFORM_CUBEMAP,
    UNIFORM_ENVBRDFMAP,
    
    UNIFORM_SCREENIMAGEMAP,
    UNIFORM_SCREENDEPTHMAP,
    
    UNIFORM_SHADOWMAP,
    UNIFORM_SHADOWMAP2,
    UNIFORM_SHADOWMAP3,
    UNIFORM_SHADOWMAP4,
    
    UNIFORM_SHADOWMVP,
    UNIFORM_SHADOWMVP2,
    UNIFORM_SHADOWMVP3,
    UNIFORM_SHADOWMVP4,
    
    UNIFORM_ENABLETEXTURES,
    
    UNIFORM_DIFFUSETEXMATRIX,
    UNIFORM_DIFFUSETEXOFFTURB,
    
    UNIFORM_TCGEN0,
    UNIFORM_TCGEN0VECTOR0,
    UNIFORM_TCGEN0VECTOR1,
    
    UNIFORM_DEFORMGEN,
    UNIFORM_DEFORMPARAMS,
    
    UNIFORM_COLORGEN,
    UNIFORM_ALPHAGEN,
    UNIFORM_COLOR,
    UNIFORM_BASECOLOR,
    UNIFORM_VERTCOLOR,
    
    UNIFORM_DLIGHTINFO,
    UNIFORM_LIGHTFORWARD,
    UNIFORM_LIGHTUP,
    UNIFORM_LIGHTRIGHT,
    UNIFORM_LIGHTORIGIN,
    UNIFORM_LIGHTORIGIN1,
    UNIFORM_LIGHTCOLOR,
    UNIFORM_LIGHTCOLOR1,
    UNIFORM_MODELLIGHTDIR,
    UNIFORM_LIGHTRADIUS,
    UNIFORM_AMBIENTLIGHT,
    UNIFORM_DIRECTEDLIGHT,
    
    UNIFORM_PORTALRANGE,
    
    UNIFORM_FOGDISTANCE,
    UNIFORM_FOGDEPTH,
    UNIFORM_FOGEYET,
    UNIFORM_FOGCOLORMASK,
    
    UNIFORM_MODELMATRIX,
    UNIFORM_MODELVIEWPROJECTIONMATRIX,
    
    UNIFORM_INVPROJECTIONMATRIX,
    UNIFORM_INVEYEPROJECTIONMATRIX,
    
    UNIFORM_TIME,
    UNIFORM_VERTEXLERP,
    UNIFORM_NORMALSCALE,
    UNIFORM_SPECULARSCALE,
    
    UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2
    UNIFORM_VIEWORIGIN,
    UNIFORM_LOCALVIEWORIGIN,
    UNIFORM_VIEWFORWARD,
    UNIFORM_VIEWLEFT,
    UNIFORM_VIEWUP,
    
    UNIFORM_INVTEXRES,
    UNIFORM_AUTOEXPOSUREMINMAX,
    UNIFORM_TONEMINAVGMAXLINEAR,
    
    UNIFORM_PRIMARYLIGHTORIGIN,
    UNIFORM_PRIMARYLIGHTCOLOR,
    UNIFORM_PRIMARYLIGHTAMBIENT,
    UNIFORM_PRIMARYLIGHTRADIUS,
    
    UNIFORM_CUBEMAPINFO,
    
    UNIFORM_ALPHATEST,
    
    UNIFORM_BONEMATRIX,
    
    UNIFORM_BRIGHTNESS,
    UNIFORM_CONTRAST,
    UNIFORM_GAMMA,
    
    UNIFORM_DIMENSIONS,
    UNIFORM_HEIGHTMAP,
    UNIFORM_LOCAL0,
    UNIFORM_LOCAL1,
    UNIFORM_LOCAL2,
    UNIFORM_LOCAL3,
    UNIFORM_TEXTURE0,
    UNIFORM_TEXTURE1,
    UNIFORM_TEXTURE2,
    UNIFORM_TEXTURE3,
    
    UNIFORM_COUNT
} uniform_t;

// shaderProgram_t represents a pair of one
// GLSL vertex and one GLSL fragment shader
typedef struct shaderProgram_s
{
    UTF8 name[MAX_QPATH];
    
    U32 program;
    U32 vertexShader;
    U32 fragmentShader;
    U32 attribs;	// vertex array attributes
    
    // uniform parameters
    S32 uniforms[UNIFORM_COUNT];
    S16 uniformBufferOffsets[UNIFORM_COUNT]; // max 32767/64=511 uniforms
    UTF8*  uniformBuffer;
} shaderProgram_t;

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct
{
    S32			x, y, width, height;
    F32		fov_x, fov_y;
    vec3_t		vieworg;
    vec3_t		viewaxis[3];		// transformation matrix
    
    stereoFrame_t	stereoFrame;
    
    S32			time;				// time in milliseconds for shader effects and other time dependent rendering issues
    S32			rdflags;			// RDF_NOWORLDMODEL, etc
    
    // 1 bits will prevent the associated area from rendering at all
    U8		areamask[MAX_MAP_AREA_BYTES];
    bool	areamaskModified;	// true if areamask changed since last scene
    
    F64		floatTime;			// tr.refdef.time / 1000.0
    
    F32		blurFactor;
    
    // text messages for deform text shaders
    UTF8		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
    
    S32			num_entities;
    trRefEntity_t*	entities;
    
    S32			num_dlights;
    struct dlight_s*	dlights;
    
    S32			numPolys;
    struct srfPoly_s*	polys;
    
    S32			numDrawSurfs;
    struct drawSurf_s*	drawSurfs;
    
    U32 dlightMask;
    S32         num_pshadows;
    struct pshadow_s* pshadows;
    
    F32       sunShadowMvp[4][16];
    F32       sunDir[4];
    F32       sunCol[4];
    F32       sunAmbCol[4];
    
    F32       autoExposureMinMax[2];
    F32       toneMinAvgMaxLinear[3];
} trRefdef_t;


//=================================================================================

// max surfaces per-skin
// This is an arbitry limit. Vanilla Q3 only supported 32 surfaces in skins but failed to
// enforce the maximum limit when reading skin files. It was possile to use more than 32
// surfaces which accessed out of bounds memory past end of skin->surfaces hunk block.
#define MAX_SKIN_SURFACES	256

// skins allow models to be retextured without modifying the model file
typedef struct
{
    UTF8		name[MAX_QPATH];
    shader_t*	shader;
} skinSurface_t;

typedef struct skin_s
{
    UTF8		name[MAX_QPATH];		// game path, including extension
    S32			numSurfaces;
    skinSurface_t*	surfaces;			// dynamically allocated array of surfaces
} skin_t;


typedef struct
{
    S32			originalBrushNumber;
    vec3_t		bounds[2];
    
    U32	colorInt;				// in packed U8 format
    F32		tcScale;				// texture coordinate vector scales
    fogParms_t	parms;
    
    // for clipping distance in fog when outside
    bool	hasSurface;
    F32		surface[4];
} fog_t;

typedef enum
{
    VPF_NONE            = 0x00,
    VPF_NOVIEWMODEL     = 0x01,
    VPF_SHADOWMAP       = 0x02,
    VPF_DEPTHSHADOW     = 0x04,
    VPF_DEPTHCLAMP      = 0x08,
    VPF_ORTHOGRAPHIC    = 0x10,
    VPF_USESUNLIGHT     = 0x20,
    VPF_FARPLANEFRUSTUM = 0x40,
    VPF_NOCUBEMAPS      = 0x80
} viewParmFlags_t;

typedef struct
{
    orientationr_t	orientation;
    orientationr_t	world;
    vec3_t		pvsOrigin;			// may be different than or.origin for portals
    bool	isPortal;			// true if this view is through a portal
    bool	isMirror;			// the portal is a mirror, invert the face culling
    S32/*viewParmFlags_t*/ flags;
    S32			frameSceneNum;		// copied from tr.frameSceneNum
    S32			frameCount;			// copied from tr.frameCount
    cplane_t	portalPlane;		// clip anything behind this if mirroring
    S32			viewportX, viewportY, viewportWidth, viewportHeight;
    FBO_t*		targetFbo;
    cubemap_t* cubemapSelection;
    S32         targetFboLayer;
    S32 targetFboCubemapIndex;
    F32		fovX, fovY;
    F32		projectionMatrix[16];
    cplane_t	frustum[5];
    vec3_t		visBounds[2];
    F32		zFar;
    F32       zNear;
    stereoFrame_t	stereoFrame;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/
typedef U8 color4ub_t[4];

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum
{
    SF_BAD,
    SF_SKIP,				// ignore
    SF_FACE,
    SF_GRID,
    SF_TRIANGLES,
    SF_POLY,
    SF_MDV,
    SF_MDR,
    SF_IQM,
    SF_FLARE,
    SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
    SF_VAO_MDVMESH,
    SF_VAO_IQM,
    
    SF_NUM_SURFACE_TYPES,
    SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( S32 )
} surfaceType_t;

typedef struct drawSurf_s
{
    U32		sort;			// bit combination for fast compares
    S32                 cubemapIndex;
    surfaceType_t*		surface;		// any of surface*_t
} drawSurf_t;

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s
{
    surfaceType_t	surfaceType;
    qhandle_t		hShader;
    S32				fogIndex;
    S32				numVerts;
    polyVert_t*		verts;
} srfPoly_t;


typedef struct srfFlare_s
{
    surfaceType_t	surfaceType;
    vec3_t			origin;
    vec3_t			normal;
    vec3_t			color;
} srfFlare_t;

typedef struct
{
    vec3_t          xyz;
    vec2_t          st;
    vec2_t          lightmap;
    S16         normal[4];
    S16         tangent[4];
    S16         lightdir[4];
    U16        color[4];
    
#if DEBUG_OPTIMIZEVERTICES
    U32    id;
#endif
} srfVert_t;

#define srfVert_t_cleared(x) srfVert_t (x) = {{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}

// srfBspSurface_t covers SF_GRID, SF_TRIANGLES and SF_POLY
typedef struct srfBspSurface_s
{
    surfaceType_t   surfaceType;
    
    // dynamic lighting information
    S32				dlightBits;
    S32             pshadowBits;
    
    // culling information
    vec3_t			cullBounds[2];
    vec3_t			cullOrigin;
    F32			cullRadius;
    cplane_t        cullPlane;
    
    // indexes
    S32             numIndexes;
    U32*      indexes;
    
    // vertexes
    S32             numVerts;
    srfVert_t*      verts;
    
    // SF_GRID specific variables after here
    
    // lod information, which may be different
    // than the culling information to allow for
    // groups of curves that LOD as a unit
    vec3_t			lodOrigin;
    F32			lodRadius;
    S32				lodFixed;
    S32				lodStitched;
    
    // vertexes
    S32				width, height;
    F32*			widthLodError;
    F32*			heightLodError;
} srfBspSurface_t;

typedef struct
{
    vec3_t translate;
    quat_t rotate;
    vec3_t scale;
} iqmTransform_t;

// inter-quake-model
typedef struct
{
    S32		num_vertexes;
    S32		num_triangles;
    S32		num_frames;
    S32		num_surfaces;
    S32		num_joints;
    S32		num_poses;
    struct srfIQModel_s*	surfaces;
    
    S32* triangles;
    
    // vertex arrays
    F32* positions;
    F32* texcoords;
    F32* normals;
    F32* tangents;
    U8* colors;
    S32* influences; // [num_vertexes] indexes into influenceBlendVertexes
    
    // unique list of vertex blend indexes/weights for faster CPU vertex skinning
    U8* influenceBlendIndexes; // [num_influences]
    union
    {
        F32* f;
        U8* b;
    } influenceBlendWeights; // [num_influences]
    
    // depending upon the exporter, blend indices and weights might be int/float
    // as opposed to the recommended byte/byte, for example Noesis exports
    // int/float whereas the official IQM tool exports byte/byte
    S32		blendWeightsType; // IQM_UBYTE or IQM_FLOAT
    
    UTF8*       jointNames;
    S32*		jointParents;
    F32* bindJoints; // [num_joints * 12]
    F32* invBindJoints; // [num_joints * 12]
    iqmTransform_t* poses;  // [num_frames * num_poses]
    F32*		bounds;
    UTF8*		names;
    S32 		numVaoSurfaces;
    struct srfVaoIQModel_s*	vaoSurfaces;
} iqmData_t;

// inter-quake-model surface
typedef struct srfIQModel_s
{
    surfaceType_t	surfaceType;
    UTF8		name[MAX_QPATH];
    shader_t*	shader;
    iqmData_t*	data;
    S32		first_vertex;
    S32 num_vertexes;
    S32		first_triangle;
    S32 num_triangles;
    S32     first_influence;
    S32 num_influences;
} srfIQModel_t;

typedef struct srfVaoIQModel_s
{
    surfaceType_t   surfaceType;
    
    iqmData_t* iqmData;
    struct srfIQModel_s* iqmSurface;
    
    // backEnd stats
    S32             numIndexes;
    S32             numVerts;
    
    // static render data
    vao_t*          vao;
} srfVaoIQModel_t;

typedef struct srfVaoMdvMesh_s
{
    surfaceType_t   surfaceType;
    
    struct mdvModel_s* mdvModel;
    struct mdvSurface_s* mdvSurface;
    
    // backEnd stats
    S32             numIndexes;
    S32             numVerts;
    U32       minIndex;
    U32       maxIndex;
    
    // static render data
    vao_t*          vao;
} srfVaoMdvMesh_t;

extern	void ( *rb_surfaceTable[SF_NUM_SURFACE_TYPES] )( void* );

/*
==============================================================================

SHADOWS

==============================================================================
*/

typedef struct pshadow_s
{
    F32 sort;
    
    S32    numEntities;
    S32    entityNums[8];
    vec3_t entityOrigins[8];
    F32  entityRadiuses[8];
    
    F32 viewRadius;
    vec3_t viewOrigin;
    
    vec3_t lightViewAxis[3];
    vec3_t lightOrigin;
    F32  lightRadius;
    cplane_t cullPlane;
} pshadow_t;


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

#define CULLINFO_NONE   0
#define CULLINFO_BOX    1
#define CULLINFO_SPHERE 2
#define CULLINFO_PLANE  4

typedef struct cullinfo_s
{
    S32             type;
    vec3_t          bounds[2];
    vec3_t			localOrigin;
    F32			radius;
    cplane_t        plane;
} cullinfo_t;

typedef struct msurface_s
{
    //S32					viewCount;		// if == tr.viewCount, already added
    struct shader_s*		shader;
    S32					fogIndex;
    S32                 cubemapIndex;
    cullinfo_t          cullinfo;
    
    surfaceType_t*		data;			// any of srf*_t
} msurface_t;


#define	CONTENTS_NODE		-1
typedef struct mnode_s
{
    // common with leaf and node
    S32			contents;		// -1 for nodes, to differentiate from leafs
    S32             visCounts[MAX_VISCOUNTS];	// node needs to be traversed if current
    vec3_t		mins, maxs;		// for bounding box culling
    struct mnode_s*	parent;
    
    // node specific
    cplane_t*	plane;
    struct mnode_s*	children[2];
    
    // leaf specific
    S32			cluster;
    S32			area;
    
    S32         firstmarksurface;
    S32			nummarksurfaces;
} mnode_t;

typedef struct
{
    vec3_t		bounds[2];		// for culling
    S32	        firstSurface;
    S32			numSurfaces;
} bmodel_t;

#define MAX_STATIC_LIGHTS		500

typedef struct
{
    UTF8		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
    UTF8		baseName[MAX_QPATH];	// ie: tim_dm2
    
    S32			dataSize;
    
    S32			numShaders;
    dshader_t*	shaders;
    
    S32			numBModels;
    bmodel_t*	bmodels;
    
    S32			numplanes;
    cplane_t*	planes;
    
    S32			numnodes;		// includes leafs
    S32			numDecisionNodes;
    mnode_t*		nodes;
    
    S32         numWorldSurfaces;
    
    S32			numsurfaces;
    msurface_t*	surfaces;
    S32*         surfacesViewCount;
    S32*         surfacesDlightBits;
    S32*			surfacesPshadowBits;
    
    S32			nummarksurfaces;
    S32*         marksurfaces;
    
    S32			numfogs;
    fog_t*		fogs;
    
    vec3_t		lightGridOrigin;
    vec3_t		lightGridSize;
    vec3_t		lightGridInverseSize;
    S32			lightGridBounds[3];
    U8*		lightGridData;
    U16*	lightGrid16;
    
    dlight_t staticLights[MAX_STATIC_LIGHTS];
    S32 numStaticLights;
    
    S32			numClusters;
    S32			clusterBytes;
    const U8*	vis;			// may be passed in by CM_LoadMap to save space
    
    UTF8*		entityString;
    UTF8*		entityParsePoint;
} world_t;


/*
==============================================================================
MDV MODELS - meta format for vertex animation models like .md2, .md3, .mdc
==============================================================================
*/
typedef struct
{
    F32           bounds[2][3];
    F32           localOrigin[3];
    F32           radius;
} mdvFrame_t;

typedef struct
{
    F32           origin[3];
    F32           axis[3][3];
} mdvTag_t;

typedef struct
{
    UTF8            name[MAX_QPATH];	// tag name
} mdvTagName_t;

typedef struct
{
    vec3_t          xyz;
    S16         normal[4];
    S16         tangent[4];
} mdvVertex_t;

typedef struct
{
    F32           st[2];
} mdvSt_t;

typedef struct mdvSurface_s
{
    surfaceType_t   surfaceType;
    
    UTF8            name[MAX_QPATH];	// polyset name
    
    S32             numShaderIndexes;
    S32*				shaderIndexes;
    
    S32             numVerts;
    mdvVertex_t*    verts;
    mdvSt_t*        st;
    
    S32             numIndexes;
    U32*      indexes;
    
    struct mdvModel_s* model;
} mdvSurface_t;

typedef struct mdvModel_s
{
    S32             numFrames;
    mdvFrame_t*     frames;
    
    S32             numTags;
    mdvTag_t*       tags;
    mdvTagName_t*   tagNames;
    
    S32             numSurfaces;
    mdvSurface_t*   surfaces;
    
    S32             numVaoSurfaces;
    srfVaoMdvMesh_t*  vaoSurfaces;
    
    S32             numSkins;
} mdvModel_t;


//======================================================================

typedef enum
{
    MOD_BAD,
    MOD_BRUSH,
    MOD_MESH,
    MOD_MDR,
    MOD_IQM
} modtype_t;

typedef struct model_s
{
    UTF8		name[MAX_QPATH];
    modtype_t	type;
    S32			index;		// model = tr.models[model->index]
    S32			dataSize;	// just for listing purposes
    bmodel_t*	bmodel;		// only if type == MOD_BRUSH
    mdvModel_t*	mdv[MD3_MAX_LODS];	// only if type == MOD_MESH
    void*	    modelData;			// only if type == (MOD_MDR | MOD_IQM)
    S32			 numLods;
} model_t;

#define	MAX_MOD_KNOWN	1024

#define	MAX_DRAWIMAGES			2048
#define	MAX_SKINS				1024

#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

0 - 1	: dlightmap index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
2 - 6	: fog index
11 - 20	: entity index
21 - 31	: sorted shader index

	TTimo - 1.32
0-1   : dlightmap index
2-6   : fog index
7-16  : entity index
17-30 : sorted shader index

    SmileTheory - for pshadows
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
1     : pshadow flag
0     : dlight flag
*/
#define	QSORT_FOGNUM_SHIFT	2
#define	QSORT_REFENTITYNUM_SHIFT	7
#define	QSORT_SHADERNUM_SHIFT	(QSORT_REFENTITYNUM_SHIFT+REFENTITYNUM_BITS)
#if (QSORT_SHADERNUM_SHIFT+SHADERNUM_BITS) > 32
#error "Need to update sorting, too many bits."
#endif
#define QSORT_PSHADOW_SHIFT     1

extern	S32			gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct
{
    S32		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
    S32		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
    S32		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
    S32		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;
    
    S32		c_leafs;
    S32		c_dlightSurfaces;
    S32		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)


// the renderer front end should never modify glstate_t
typedef struct
{
    bool	finishCalled;
    S32			texEnv[2];
    S32			faceCulling;
    S32         faceCullFront;
    U32    glStateBits;
    U32    storedGlState;
    F32           vertexAttribsInterpolation;
    bool        vertexAnimation;
    S32             boneAnimation; // number of bones
    mat4_t          boneMatrix[IQM_MAX_JOINTS];
    U32        vertexAttribsEnabled;  // global if no VAOs, tess only otherwise
    FBO_t*          currentFBO;
    vao_t*          currentVao;
    mat4_t        modelview;
    mat4_t        projection;
    mat4_t		modelviewProjection;
    mat4_t		invProjection;
    mat4_t		invEyeProjection;
} glstate_t;

typedef enum
{
    MI_NONE,
    MI_NVX,
    MI_ATI
} memInfo_t;

typedef enum
{
    TCR_NONE = 0x0000,
    TCR_RGTC = 0x0001,
    TCR_BPTC = 0x0002,
} textureCompressionRef_t;

// We can't change vidConfig_t without breaking DLL/vms compatibility, so
// store extensions we have here.
typedef struct
{
    S32 openglMajorVersion;
    S32 openglMinorVersion;
    
    bool intelGraphics;
    bool standardDerivatives;
    bool occlusionQuery;
    
    S32 glslMajorVersion;
    S32 glslMinorVersion;
    S32 glslMaxAnimatedBones;
    
    memInfo_t   memInfo;
    
    bool framebufferObject;
    S32 maxRenderbufferSize;
    S32 maxColorAttachments;
    
    bool textureFloat;
    S32 /*textureCompressionRef_t*/ textureCompression;
    bool swizzleNormalmap;
    
    bool framebufferMultisample;
    bool framebufferBlit;
    
    bool depthClamp;
    bool seamlessCubeMap;
    
    bool vertexArrayObject;
    bool directStateAccess;
    
    S32 maxVertexAttribs;
    bool gpuVertexAnimation;
} glRefConfig_t;


typedef struct
{
    S32		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
    S32     c_surfBatches;
    F32	c_overDraw;
    
    S32		c_vaoBinds;
    S32		c_vaoVertexes;
    S32		c_vaoIndexes;
    
    S32     c_staticVaoDraws;
    S32     c_dynamicVaoDraws;
    
    S32		c_dlightVertexes;
    S32		c_dlightIndexes;
    
    S32		c_flareAdds;
    S32		c_flareTests;
    S32		c_flareRenders;
    
    S32     c_glslShaderBinds;
    S32     c_genericDraws;
    S32     c_lightallDraws;
    S32     c_fogDraws;
    S32     c_dlightDraws;
    
    S32		msec;			// total msec for backend run
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct
{
    trRefdef_t	refdef;
    viewParms_t	viewParms;
    orientationr_t	orientation;
    backEndCounters_t	pc;
    bool	isHyperspace;
    trRefEntity_t*	currentEntity;
    bool	skyRenderedThisView;	// flag for drawing sun
    
    bool	projection2D;	// if true, drawstretchpic doesn't need to change modes
    U8		color2D[4];
    bool	vertexes2D;		// shader needs to be finished
    trRefEntity_t	entity2D;	// currentEntity will point at this when doing 2D rendering
    
    FBO_t* last2DFBO;
    bool    colorMask[4];
    bool    framePostProcessed;
    bool    depthFill;
} backEndState_t;

/*
** trGlobals_t
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct
{
    bool registered; // cleared at shutdown, set at beginRegistration
    
    S32 visIndex;
    S32 visClusters[MAX_VISCOUNTS];
    S32 visCounts[MAX_VISCOUNTS]; // incremented every time a new vis cluster is entered
    S32 frameCount; // incremented every frame
    S32 sceneCount; // incremented every scene
    S32 viewCount; // incremented every view (twice a scene if portaled)
    // and every R_MarkFragments call
    
    S32 frameSceneNum; // zeroed at RE_BeginFrame
    
    bool worldMapLoaded;
    bool worldDeluxeMapping;
    vec2_t autoExposureMinMax;
    vec3_t toneMinAvgMaxLevel;
    world_t* world;
    
    const U8* externalVisData;	// from RE_SetWorldVisData, shared with CM_Load
    
    image_t* defaultImage;
    image_t* scratchImage[32];
    image_t* fogImage;
    image_t* dlightImage; // inverse-quare highlight for projective adding
    image_t* flareImage;
    image_t* whiteImage; // full of 0xff
    image_t* identityLightImage; // full of tr.identityLightByte
    cubemap_t shadowCubemaps[MAX_DLIGHTS];
    image_t* renderImage;
    image_t* glowImage;
    image_t* normalDetailedImage;
    image_t* sunRaysImage;
    image_t* renderDepthImage;
    image_t* pshadowMaps[MAX_DRAWN_PSHADOWS];
    image_t* screenScratchImage;
    image_t* textureScratchImage[2];
    image_t* quarterImage[2];
    image_t* calcLevelsImage;
    image_t* targetLevelsImage;
    image_t* fixedLevelsImage;
    image_t* sunShadowDepthImage[4];
    image_t* screenShadowImage;
    image_t* screenSsaoImage;
    image_t* hdrDepthImage;
    image_t* renderCubeImage;
    image_t* textureDepthImage;
    image_t* prefilterEnvMapImage;
    image_t* envBrdfImage;
    image_t* cubeDepthImage;
    
    FBO_t* renderFbo;
    FBO_t* sunRaysFbo;
    FBO_t* depthFbo;
    FBO_t* pshadowFbos[MAX_DRAWN_PSHADOWS];
    FBO_t* screenScratchFbo;
    FBO_t* textureScratchFbo[2];
    FBO_t* quarterFbo[2];
    FBO_t* calcLevelsFbo;
    FBO_t* targetLevelsFbo;
    FBO_t* sunShadowFbo[4];
    FBO_t* screenShadowFbo;
    FBO_t* screenSsaoFbo;
    FBO_t* hdrDepthFbo;
    FBO_t* renderCubeFbo;
    FBO_t* preFilterEnvMapFbo;
    FBO_t* shadowCubeFbo;
    
    shader_t* defaultShader;
    shader_t* shadowShader;
    shader_t* projectionShadowShader;
    
    shader_t* flareShader;
    shader_t* sunShader;
    shader_t* sunFlareShader;
    
    S32	numLightmaps;
    S32	lightmapSize;
    image_t** lightmaps;
    image_t** deluxemaps;
    
    S32	fatLightmapCols;
    S32	fatLightmapRows;
    
    S32 numCubemaps;
    cubemap_t* cubemaps;
    cubemap_t skyboxCubemap;
    bool skyboxCubemapped;
    
    trRefEntity_t* currentEntity;
    trRefEntity_t worldEntity; // point currentEntity at this when rendering world
    S32 currentEntityNum;
    S32	shiftedEntityNum; // currentEntityNum << QSORT_REFENTITYNUM_SHIFT
    model_t* currentModel;
    
    //
    // GPU shader programs
    //
    shaderProgram_t genericShader[GENERICDEF_COUNT];
    shaderProgram_t textureColorShader;
    shaderProgram_t fogShader[FOGDEF_COUNT];
    shaderProgram_t dlightShader[DLIGHTDEF_COUNT];
    shaderProgram_t lightallShader[LIGHTDEF_COUNT];
    shaderProgram_t shadowmapShader[SHADOWMAPDEF_COUNT];
    shaderProgram_t pshadowShader;
    shaderProgram_t down4xShader;
    shaderProgram_t bokehShader;
    shaderProgram_t tonemapShader;
    shaderProgram_t calclevels4xShader[2];
    shaderProgram_t shadowmaskShader;
    shaderProgram_t ssaoShader;
    shaderProgram_t depthBlurShader[4];
    shaderProgram_t testcubeShader;
    
    shaderProgram_t darkexpandShader;
    shaderProgram_t hdrShader;
    shaderProgram_t dofShader;
    shaderProgram_t anaglyphShader;
    shaderProgram_t esharpeningShader;
    shaderProgram_t esharpening2Shader;
    shaderProgram_t texturecleanShader;
    shaderProgram_t lensflareShader;
    shaderProgram_t multipostShader;
    shaderProgram_t anamorphicDarkenShader;
    shaderProgram_t anamorphicBlurShader;
    shaderProgram_t anamorphicCombineShader;
    shaderProgram_t vibrancyShader;
    shaderProgram_t texturedetailShader;
    shaderProgram_t rbmShader;
    shaderProgram_t contrastShader;
    shaderProgram_t fxaaShader;
    shaderProgram_t bloomDarkenShader;
    shaderProgram_t bloomBlurShader;
    shaderProgram_t bloomCombineShader;
    shaderProgram_t ssrShader;
    shaderProgram_t ssrCombineShader;
    shaderProgram_t ssgiShader;
    shaderProgram_t ssgiBlurShader;
    shaderProgram_t prefilterEnvMapShader;
    shaderProgram_t waterShader;
    shaderProgram_t underWaterShader;
    shaderProgram_t sunPassShader;
    
    image_t*        bloomRenderFBOImage[3];
    image_t*        anamorphicRenderFBOImage[3];
    image_t*        genericFBOImage;
    image_t*        genericFBO2Image;
    
    FBO_t*          bloomRenderFBO[3];
    FBO_t*          anamorphicRenderFBO[3];
    FBO_t*	   	    genericFbo;
    FBO_t*	   	    genericFbo2;
    
    // -----------------------------------------
    
    viewParms_t viewParms;
    
    F32 identityLight; // 1.0 / ( 1 << overbrightBits )
    S32 identityLightByte; // identityLight * 255
    S32 overbrightBits; // r_overbrightBits->integer, but set to 0 if no hw gamma
    
    orientationr_t orientation;	// for current entity
    
    trRefdef_t refdef;
    
    S32 viewCluster;
    
    F32 sunShadowScale;
    
    bool sunShadows;
    vec3_t sunLight; // from the sky shader for this level
    vec3_t sunDirection;
    vec3_t lastCascadeSunDirection;
    F32 lastCascadeSunMvp[16];
    
    frontEndCounters_t pc;
    S32 frontEndMsec; // not in pc due to clearing issue
    
    vec4_t clipRegion; // 2D clipping region
    
    //
    // put large tables at the end, so most elements will be
    // within the +/32K indexed range on risc processors
    //
    model_t* models[MAX_MOD_KNOWN];
    S32 numModels;
    
    S32 numImages;
    image_t* images[MAX_DRAWIMAGES];
    
    S32 numFBOs;
    FBO_t* fbos[MAX_FBOS];
    
    S32 numVaos;
    vao_t* vaos[MAX_VAOS];
    
    // shader indexes from other modules will be looked up in tr.shaders[]
    // shader indexes from drawsurfs will be looked up in sortedShaders[]
    // lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
    S32 numShaders;
    shader_t* shaders[MAX_SHADERS];
    shader_t* sortedShaders[MAX_SHADERS];
    
    S32 numSkins;
    skin_t* skins[MAX_SKINS];
    
    U32 sunFlareQuery[2];
    S32 sunFlareQueryIndex;
    bool sunFlareQueryActive[2];
    
    F32 sinTable[FUNCTABLE_SIZE];
    F32 squareTable[FUNCTABLE_SIZE];
    F32 triangleTable[FUNCTABLE_SIZE];
    F32 sawToothTable[FUNCTABLE_SIZE];
    F32 inverseSawToothTable[FUNCTABLE_SIZE];
    F32 noiseTable[FUNCTABLE_SIZE];
    F32 fogTable[FOG_TABLE_SIZE];
} trGlobals_t;

extern backEndState_t	backEnd;
extern trGlobals_t	tr;
extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init
extern glRefConfig_t glRefConfig;

//
// cvars
//
extern convar_t*  r_mode;
extern convar_t*	r_flareSize;
extern convar_t*	r_flareFade;
// coefficient for the flare intensity falloff function.
#define FLARE_STDCOEFF "150"
extern convar_t*	r_flareCoeff;

extern convar_t*	r_railWidth;
extern convar_t*	r_railCoreWidth;
extern convar_t*	r_railSegmentLength;

extern convar_t*	r_verbose;				// used for verbose debug spew

extern convar_t*	r_znear;				// near Z clip plane
extern convar_t*	r_zproj;				// z distance of projection plane
extern convar_t*	r_stereoSeparation;			// separation of cameras for stereo rendering

extern convar_t*	r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern convar_t*	r_lodbias;				// push/pull LOD transitions
extern convar_t*	r_lodscale;

extern convar_t*	r_inGameVideo;				// controls whether in game video should be draw
extern convar_t*	r_fastsky;				// controls whether sky should be cleared or drawn
extern convar_t*	r_drawSun;				// controls drawing of sun quad
extern convar_t*	r_dynamiclight;		// dynamic lights enabled/disabled

extern	convar_t*	r_norefresh;			// bypasses the ref rendering
extern	convar_t*	r_drawentities;		// disable/enable entity rendering
extern	convar_t*	r_drawworld;			// disable/enable world rendering
extern	convar_t*	r_speeds;				// various levels of information display
extern  convar_t*	r_detailTextures;		// enables/disables detail texturing stages
extern	convar_t*	r_novis;				// disable/enable usage of PVS
extern	convar_t*	r_nocull;
extern	convar_t*	r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	convar_t*	r_nocurves;
extern	convar_t*	r_showcluster;

extern convar_t*	r_gamma;

extern  convar_t*  r_ext_framebuffer_object;
extern  convar_t*  r_ext_texture_float;
extern  convar_t*  r_ext_framebuffer_multisample;
extern  convar_t*  r_arb_seamless_cube_map;
extern  convar_t*  r_arb_vertex_array_object;
extern  convar_t*  r_ext_direct_state_access;

extern	convar_t*	r_nobind;						// turns off binding to appropriate textures
extern	convar_t*	r_singleShader;				// make most world faces use default shader
extern	convar_t*	r_roundImagesDown;
extern	convar_t*	r_colorMipLevels;				// development aid to see texture mip usage
extern	convar_t*	r_picmip;						// controls picmip values
extern	convar_t*	r_finish;
extern	convar_t*	r_textureMode;
extern	convar_t*	r_offsetFactor;
extern	convar_t*	r_offsetUnits;

extern	convar_t*	r_fullbright;					// avoid lightmap pass
extern	convar_t*	r_lightmap;					// render lightmaps only
extern	convar_t*	r_vertexLight;					// vertex lighting mode for better performance
extern	convar_t*	r_uiFullScreen;				// ui is running fullscreen

extern	convar_t*	r_logFile;						// number of frames to emit GL logs
extern	convar_t*	r_showtris;					// enables wireframe rendering of the world
extern	convar_t*	r_showsky;						// forces sky in front of all surfaces
extern	convar_t*	r_shownormals;					// draws wireframe normals
extern	convar_t*	r_clear;						// force screen clear every frame

extern	convar_t*	r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern	convar_t*	r_flares;						// light flares

extern	convar_t*	r_intensity;

extern	convar_t*	r_lockpvs;
extern	convar_t*	r_noportals;
extern	convar_t*	r_portalOnly;

extern	convar_t*	r_subdivisions;
extern	convar_t*	r_lodCurveError;
extern	convar_t*	r_skipBackEnd;

extern	convar_t*	r_anaglyphMode;
extern  convar_t*  r_hdr;
extern convar_t* r_truehdr;
extern  convar_t*  r_floatLightmap;
extern  convar_t*  r_postProcess;

extern  convar_t*  r_toneMap;
extern  convar_t*  r_forceToneMap;
extern  convar_t*  r_forceToneMapMin;
extern  convar_t*  r_forceToneMapAvg;
extern  convar_t*  r_forceToneMapMax;

extern  convar_t*  r_autoExposure;
extern  convar_t*  r_forceAutoExposure;
extern  convar_t*  r_forceAutoExposureMin;
extern  convar_t*  r_forceAutoExposureMax;

extern  convar_t*  r_cameraExposure;

extern  convar_t*  r_depthPrepass;
extern  convar_t*  r_ssao;

extern  convar_t*  r_normalMapping;
extern  convar_t*  r_specularMapping;
extern  convar_t*  r_deluxeMapping;
extern  convar_t*  r_parallaxMapping;
extern  convar_t*  r_parallaxMapShadows;
extern  convar_t*  r_cubeMapping;
extern  convar_t*  r_horizonFade;
extern  convar_t*  r_cubemapSize;
extern  convar_t*  r_deluxeSpecular;
extern  convar_t*  r_pbr;
extern  convar_t*  r_baseNormalX;
extern  convar_t*  r_baseNormalY;
extern  convar_t*  r_baseParallax;
extern  convar_t*  r_baseSpecular;
extern  convar_t*  r_baseGloss;
extern  convar_t*  r_glossType;
extern  convar_t*  r_dlightMode;
extern  convar_t*  r_pshadowDist;
extern  convar_t*  r_mergeLightmaps;
extern  convar_t*  r_imageUpsample;
extern  convar_t*  r_imageUpsampleMaxSize;
extern  convar_t*  r_imageUpsampleType;
extern  convar_t*  r_genNormalMaps;
extern  convar_t*  r_forceSun;
extern  convar_t*  r_forceSunLightScale;
extern  convar_t*  r_forceSunAmbientScale;
extern  convar_t*  r_sunlightMode;
extern  convar_t*  r_drawSunRays;
extern  convar_t*  r_sunShadows;
extern  convar_t*  r_shadowFilter;
extern  convar_t*  r_shadowBlur;
extern  convar_t*  r_shadowMapSize;
extern  convar_t*  r_shadowCascadeZNear;
extern  convar_t*  r_shadowCascadeZFar;
extern  convar_t*  r_shadowCascadeZBias;
extern  convar_t*  r_ignoreDstAlpha;

extern	convar_t*	r_greyscale;

extern	convar_t*	r_ignoreGLErrors;

extern	convar_t*	r_overBrightBits;
extern	convar_t*	r_mapOverBrightBits;

extern	convar_t*	r_debugSurface;
extern	convar_t*	r_simpleMipMaps;

extern	convar_t*	r_showImages;
extern	convar_t*	r_debugSort;

extern	convar_t*	r_printShaders;

extern convar_t*	r_marksOnTriangleMeshes;

extern convar_t* r_stencilbits;			// number of desired stencil bits
extern convar_t* r_depthbits;			// number of desired depth bits
extern convar_t* r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern convar_t* r_texturebits;			// number of desired texture bits
extern convar_t* r_ext_multisample;
// 0 = use framebuffer depth
// 16 = use 16-bit textures
// 32 = use 32-bit textures
// all else = error

extern convar_t* r_customwidth;
extern convar_t* r_customheight;
extern convar_t* r_pixelAspect;
extern convar_t* r_noborder;
extern convar_t* r_fullscreen;
extern convar_t* r_ignorehwgamma;		// overrides hardware gamma capabilities
extern convar_t* r_drawBuffer;
extern convar_t* r_swapInterval;
extern convar_t* r_allowExtensions;				// global enable/disable of OpenGL extensions
extern convar_t* r_ext_compressed_textures;		// these control use of specific extensions
extern convar_t* r_ext_multitexture;
extern convar_t* r_ext_compiled_vertex_array;
extern convar_t* r_ext_texture_env_add;

extern convar_t* r_ext_texture_filter_anisotropic;
extern convar_t* r_ext_max_anisotropy;

extern convar_t* r_stereoEnabled;

extern	convar_t* r_saveFontData;

extern convar_t* r_lensflare;
extern convar_t* r_anamorphic;
extern convar_t* r_anamorphicDarkenPower;
extern convar_t* r_ssgi;
extern convar_t* r_ssgiWidth;
extern convar_t* r_ssgiSamples;
extern convar_t* r_darkexpand;
extern convar_t* r_dof;
extern convar_t* r_esharpening;
extern convar_t* r_esharpening2;
extern convar_t* r_multipost;
extern convar_t* r_textureClean;
extern convar_t* r_textureCleanSigma;
extern convar_t* r_textureCleanBSigma;
extern convar_t* r_textureCleanMSize;
extern convar_t* r_trueAnaglyph;
extern convar_t* r_trueAnaglyphSeparation;
extern convar_t* r_trueAnaglyphRed;
extern convar_t* r_trueAnaglyphGreen;
extern convar_t* r_trueAnaglyphBlue;
extern convar_t* r_vibrancy;
extern convar_t* r_textureDetail;
extern convar_t* r_textureDetailStrength;
extern convar_t* r_rbm;
extern convar_t* r_rbmStrength;
extern convar_t* r_screenblur;
extern convar_t* r_brightness;
extern convar_t* r_contrast;
extern convar_t* r_gamma;
extern convar_t* r_fxaa;
extern convar_t* r_bloom;
extern convar_t* r_bloomPasses;
extern convar_t* r_bloomDarkenPower;
extern convar_t* r_bloomScale;
extern convar_t* r_ssr;
extern convar_t* r_ssrStrength;
extern convar_t* r_sse;
extern convar_t* r_sseStrength;
extern convar_t* r_proceduralSun;
extern convar_t* r_proceduralSunScale;

//====================================================================

#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define	GLS_SRCBLEND_BITS						0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define	GLS_DSTBLEND_BITS						0x000000f0

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000
#define GLS_DEPTHFUNC_GREATER                   0x00040000
#define GLS_DEPTHFUNC_BITS                      0x00060000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define	GLS_ATEST_BITS						    0x70000000

#define GLS_DEFAULT			GLS_DEPTHMASK_TRUE

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

typedef struct stageVars
{
    color4ub_t	colors[SHADER_MAX_VERTEXES];
    vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

typedef struct shaderCommands_s
{
    U32	indexes[SHADER_MAX_INDEXES] QALIGN( 16 );
    vec4_t xyz[SHADER_MAX_VERTEXES] QALIGN( 16 );
    S16 normal[SHADER_MAX_VERTEXES][4] QALIGN( 16 );
    S16 tangent[SHADER_MAX_VERTEXES][4] QALIGN( 16 );
    vec2_t texCoords[SHADER_MAX_VERTEXES] QALIGN( 16 );
    vec2_t lightCoords[SHADER_MAX_VERTEXES] QALIGN( 16 );
    U16 color[SHADER_MAX_VERTEXES][4] QALIGN( 16 );
    S16 lightdir[SHADER_MAX_VERTEXES][4]QALIGN( 16 );
    //S32			vertexDlightBits[SHADER_MAX_VERTEXES];
    
    void* attribPointers[ATTR_INDEX_COUNT];
    vao_t*       vao;
    bool    useInternalVao;
    bool    useCacheVao;
    
    stageVars_t	svars QALIGN( 16 );
    
    //color4ub_t	constantColor255[SHADER_MAX_VERTEXES];
    
    shader_t*	shader;
    F64		shaderTime;
    S32			fogNum;
    S32         cubemapIndex;
    
    S32			dlightBits;	// or together of all vertexDlightBits
    S32         pshadowBits;
    
    S32			firstIndex;
    S32			numIndexes;
    S32			numVertexes;
    
    // info extracted from current shader
    S32			numPasses;
    void	( *currentStageIteratorFunc )( void );
    shaderStage_t**	xstages;
} shaderCommands_t;

extern	shaderCommands_t	tess;

/*
=============================================================

UNCOMPRESSING BONES

=============================================================
*/

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

typedef struct
{
    S32		commandId;
    F32	color[4];
} setColorCommand_t;

typedef struct
{
    S32		commandId;
    S32		buffer;
} drawBufferCommand_t;

typedef struct
{
    S32		commandId;
    image_t*	image;
    S32		width;
    S32		height;
    void*	data;
} subImageCommand_t;

typedef struct
{
    S32		commandId;
} swapBuffersCommand_t;

typedef struct
{
    S32		commandId;
    S32		buffer;
} endFrameCommand_t;

typedef struct
{
    S32		commandId;
    F32	x, y;
    F32	w, h;
    F32	s1, t1;
    F32	s2, t2;
    shader_t*	shader;
} stretchPicCommand_t;

typedef struct
{
    S32		commandId;
    S32		numDrawSurfs;
    trRefdef_t	refdef;
    viewParms_t	viewParms;
    drawSurf_t* drawSurfs;
} drawSurfsCommand_t;

typedef struct
{
    S32 commandId;
    S32 x;
    S32 y;
    S32 width;
    S32 height;
    UTF8* fileName;
    bool jpeg;
} screenshotCommand_t;

typedef struct
{
    S32						commandId;
    S32						width;
    S32						height;
    U8*					captureBuffer;
    U8*					encodeBuffer;
    bool			motionJpeg;
} videoFrameCommand_t;

typedef struct
{
    S32 commandId;
    
    U8 rgba[4];
} colorMaskCommand_t;

typedef struct
{
    S32 commandId;
} clearDepthCommand_t;

typedef struct
{
    S32 commandId;
    S32 map;
    S32 cubeSide;
} capShadowmapCommand_t;

typedef struct convolveCubemapCommand_s
{
    S32 commandId;
    S32 cubemap;
    S32 cubeSide;
    cubemap_t* cubemaps;
} convolveCubemapCommand_t;

typedef struct
{
    S32		commandId;
    trRefdef_t	refdef;
    viewParms_t	viewParms;
} postProcessCommand_t;

typedef struct
{
    S32 commandId;
} exportCubemapsCommand_t;

typedef struct
{
    S32 commandId;
} renderFinishCommand_t;

typedef enum
{
    RC_END_OF_LIST,
    RC_SET_COLOR,
    RC_STRETCH_PIC,
    RC_DRAW_SURFS,
    RC_DRAW_BUFFER,
    RC_SWAP_BUFFERS,
    RC_SCREENSHOT,
    RC_VIDEOFRAME,
    RC_COLORMASK,
    RC_CLEARDEPTH,
    RC_CONVOLVECUBEMAP,
    RC_POSTPROCESS,
    RC_EXPORT_CUBEMAPS,
    RC_FINISH
} renderCommand_t;

// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	MAX_POLYS		10000
#define	MAX_POLYVERTS	30000

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct
{
    U8 cmds[MAX_RENDER_COMMANDS];
    size_t used;
} renderCommandList_t;

// all of the information needed by the back end must be
// contained in a backEndData_t
typedef struct
{
    drawSurf_t drawSurfs[MAX_DRAWSURFS];
    dlight_t dlights[MAX_DLIGHTS];
    trRefEntity_t entities[MAX_REFENTITIES];
    srfPoly_t* polys;//[MAX_POLYS];
    polyVert_t*	polyVerts;//[MAX_POLYVERTS];
    pshadow_t pshadows[MAX_CALC_PSHADOWS];
    renderCommandList_t	commands;
} backEndData_t;

extern S32 max_polys;
extern S32 max_polyverts;

extern backEndData_t* backEndData;	// the second one may not be allocated

e_status trap_CIN_RunCinematic( S32 handle );
S32 trap_CIN_PlayCinematic( StringEntry arg0, S32 xpos, S32 ypos, S32 width, S32 height, S32 bits );
void trap_CIN_UploadCinematic( S32 handle );

//
// idRenderSystemLocal
//
class idRenderSystemLocal : public idRenderSystem
{
public:
    virtual void Shutdown( bool destroyWindow );
    virtual void Init( vidconfig_t* config );
    virtual qhandle_t RegisterModel( StringEntry name );
    virtual qhandle_t RegisterSkin( StringEntry name );
    virtual qhandle_t RegisterShader( StringEntry name );
    virtual qhandle_t RegisterShaderNoMip( StringEntry name );
    virtual void LoadWorld( StringEntry name );
    virtual void SetWorldVisData( const U8* vis );
    virtual void EndRegistration( void );
    virtual void ClearScene( void );
    virtual void AddRefEntityToScene( const refEntity_t* re );
    virtual void AddPolyToScene( qhandle_t hShader, S32 numVerts, const polyVert_t* verts, S32 num );
    virtual bool LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
    virtual void AddLightToScene( const vec3_t org, F32 intensity, F32 r, F32 g, F32 b );
    virtual void AddAdditiveLightToScene( const vec3_t org, F32 intensity, F32 r, F32 g, F32 b );
    virtual void RenderScene( const refdef_t* fd );
    virtual void SetColor( const F32* rgba );
    virtual void SetClipRegion( const F32* region );
    virtual void DrawStretchPic( F32 x, F32 y, F32 w, F32 h, F32 s1, F32 t1, F32 s2, F32 t2, qhandle_t hShader );
    virtual void DrawStretchRaw( S32 x, S32 y, S32 w, S32 h, S32 cols, S32 rows, const U8* data, S32 client, bool dirty );
    virtual void UploadCinematic( S32 w, S32 h, S32 cols, S32 rows, const U8* data, S32 client, bool dirty );
    virtual void BeginFrame( stereoFrame_t stereoFrame );
    virtual void EndFrame( S32* frontEndMsec, S32* backEndMsec );
    virtual S32 MarkFragments( S32 numPoints, const vec3_t* points, const vec3_t projection, S32 maxPoints, vec3_t pointBuffer, S32 maxFragments, markFragment_t* fragmentBuffer );
    virtual S32	LerpTag( orientation_t* tag, qhandle_t model, S32 startFrame, S32 endFrame, F32 frac, StringEntry tagName );
    virtual void ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs );
    virtual void RegisterFont( StringEntry fontName, S32 pointSize, fontInfo_t* font );
    virtual void RemapShader( StringEntry oldShader, StringEntry newShader, StringEntry offsetTime );
    virtual bool GetEntityToken( UTF8* buffer, S32 size );
    virtual bool inPVS( const vec3_t p1, const vec3_t p2 );
    virtual void TakeVideoFrame( S32 h, S32 w, U8* captureBuffer, U8* encodeBuffer, bool motionJpeg );
    virtual void* MainWindow( void );
};

extern idRenderSystemLocal renderSystemLocal;

#endif //!DEDICATED

#endif //!__R_LOCAL_H__

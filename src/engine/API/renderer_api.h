////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
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
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   renderer_api.cpp
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_PUBLIC_H__
#define __R_PUBLIC_H__

#ifndef __R_TYPES_H__
#include <renderSystem/r_types.h>
#endif

// AVI files have the start of pixel lines 4 byte-aligned
#define AVI_LINE_PADDING 4

// font support
#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT (GLYPH_END - GLYPH_START + 1)
#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file

typedef struct
{
    S32 height;       // number of scan lines
    S32 top;          // top of glyph in buffer
    S32 bottom;       // bottom of glyph in buffer
    S32 pitch;        // width for copying
    S32 xSkip;        // x adjustment
    S32 imageWidth;   // width of actual image
    S32 imageHeight;  // height of actual image
    F32 s;          // x offset in image where glyph starts
    F32 t;          // y offset in image where glyph starts
    F32 s2;
    F32 t2;
    qhandle_t glyph;  // handle to the shader with the glyph
    UTF8 shaderName[32];
} glyphInfo_t;

typedef struct
{
    glyphInfo_t glyphs [GLYPHS_PER_FONT];
    F32 glyphScale;
    UTF8 name[MAX_QPATH];
} fontInfo_t;

//typedef enum f_status e_status;

//
// system functions provided by the main engine
//
struct rendererImports_t
{
    void( *Printf )( StringEntry fmt, ... );
    void( *Error )( S32 level, StringEntry fmt, ... );
    S32( *PlayCinematic )( StringEntry arg0, S32 xpos, S32 ypos, S32 width, S32 height, S32 bits );
    e_status( *RunCinematic )( S32 handle );
    void ( *UploadCinematic )( S32 handle );
    
    idCollisionModelManager* collisionModelManager;
    idFileSystem* fileSystem;
    idCVarSystem* cvarSystem;
    idCmdBufferSystem* cmdBufferSystem;
    idCmdSystem* cmdSystem;
    idSystem* idsystem;
    idClientAVISystemAPI* clientAVISystem;
    idMemorySystem* memorySystem;
    idClientMainSystem* clientMainSystem;
};

//
// idRenderSystem
//
class idRenderSystem
{
public:
    virtual void Shutdown( bool destroyWindow ) = 0;
    virtual void Init( vidconfig_t* config ) = 0;
    virtual qhandle_t RegisterModel( StringEntry name ) = 0;
    virtual qhandle_t RegisterSkin( StringEntry name ) = 0;
    virtual qhandle_t RegisterShader( StringEntry name ) = 0;
    virtual qhandle_t RegisterShaderNoMip( StringEntry name ) = 0;
    virtual void LoadWorld( StringEntry name ) = 0;
    virtual void SetWorldVisData( const U8* vis ) = 0;
    virtual void EndRegistration( void ) = 0;
    virtual void ClearScene( void ) = 0;
    virtual void AddRefEntityToScene( const refEntity_t* re ) = 0;
    virtual void AddPolyToScene( qhandle_t hShader, S32 numVerts, const polyVert_t* verts, S32 num ) = 0;
    virtual bool LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) = 0;
    virtual void AddLightToScene( const vec3_t org, F32 intensity, F32 r, F32 g, F32 b ) = 0;
    virtual void AddAdditiveLightToScene( const vec3_t org, F32 intensity, F32 r, F32 g, F32 b ) = 0;
    virtual void RenderScene( const refdef_t* fd ) = 0;
    virtual void SetColor( const F32* rgba ) = 0;
    virtual void SetClipRegion( const F32* region ) = 0;
    virtual void DrawStretchPic( F32 x, F32 y, F32 w, F32 h, F32 s1, F32 t1, F32 s2, F32 t2, qhandle_t hShader ) = 0;
    virtual void DrawStretchRaw( S32 x, S32 y, S32 w, S32 h, S32 cols, S32 rows, const U8* data, S32 client, bool dirty ) = 0;
    virtual void UploadCinematic( S32 w, S32 h, S32 cols, S32 rows, const U8* data, S32 client, bool dirty ) = 0;
    virtual void BeginFrame( stereoFrame_t stereoFrame ) = 0;
    virtual void EndFrame( S32* frontEndMsec, S32* backEndMsec ) = 0;
    virtual S32 MarkFragments( S32 numPoints, const vec3_t* points, const vec3_t projection, S32 maxPoints, vec3_t pointBuffer, S32 maxFragments, markFragment_t* fragmentBuffer ) = 0;
    virtual S32	LerpTag( orientation_t* tag,  qhandle_t model, S32 startFrame, S32 endFrame, F32 frac, StringEntry tagName ) = 0;
    virtual void ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) = 0;
    virtual void RegisterFont( StringEntry fontName, S32 pointSize, fontInfo_t* font ) = 0;
    virtual void RemapShader( StringEntry oldShader, StringEntry newShader, StringEntry offsetTime ) = 0;
    virtual bool GetEntityToken( UTF8* buffer, S32 size ) = 0;
    virtual bool inPVS( const vec3_t p1, const vec3_t p2 ) = 0;
    virtual void TakeVideoFrame( S32 h, S32 w, U8* captureBuffer, U8* encodeBuffer, bool motionJpeg ) = 0;
};

extern idRenderSystem* renderSystem;

#endif	//!__R_PUBLIC_H__

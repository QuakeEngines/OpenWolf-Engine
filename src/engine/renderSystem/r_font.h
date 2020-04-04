﻿////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   r_font.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: The font system uses FreeType 2.x to render TrueType fonts for use within the game.
//              As of this writing ( Nov, 2000 ) Team Arena uses these fonts for all of the ui and
//              about 90% of the cgame presentation. A few areas of the CGAME were left uses the old
//              fonts since the code is shared with standard Q3A.
//
//              If you include this font rendering code in a commercial product you MUST include the
//              following somewhere with your product, see www.freetype.org for specifics or changes.
//              The Freetype code also uses some hinting techniques that MIGHT infringe on patents
//              held by apple so be aware of that also.
//
//              As of Q3A 1.25+ and Team Arena, we are shipping the game with the font rendering code
//              disabled. This removes any potential patent issues and it keeps us from having to
//              distribute an actual TrueTrype font which is 1. expensive to do and 2. seems to require
//              an act of god to accomplish.
//
//              What we did was pre-render the fonts using FreeType ( which is why we leave the FreeType
//              credit in the credits ) and then saved off the glyph data and then hand touched up the
//              font bitmaps so they scale a bit better in GL.
//
//              There are limitations in the way fonts are saved and reloaded in that it is based on
//              point size and not name. So if you pre-render Helvetica in 18 point and Impact in 18 point
//              you will end up with a single 18 point data file and image set. Typically you will want to
//              choose 3 sizes to best approximate the scaling you will be doing in the ui scripting system
//
//              In the UI Scripting code, a scale of 1.0 is equal to a 48 point font. In Team Arena, we
//              use three or four scales, most of them exactly equaling the specific rendered size. We
//              rendered three sizes in Team Arena, 12, 16, and 20.
//
//              To generate new font data you need to go through the following steps.
//              1. delete the fontImage_x_xx.tga files and fontImage_xx.dat files from the fonts path.
//              2. in a ui script, specificy a font, smallFont, and bigFont keyword with font name and
//                 point size. the original TrueType fonts must exist in fonts at this point.
//              3. run the game, you should see things normally.
//              4. Exit the game and there will be three dat files and at least three tga files. The
//                 tga's are in 256x256 pages so if it takes three images to render a 24 point font you
//                 will end up with fontImage_0_24.tga through fontImage_2_24.tga
//              5. In future runs of the game, the system looks for these images and data files when a s
//                 specific point sized font is rendered and loads them for use.
//              6. Because of the original beta nature of the FreeType code you will probably want to hand
//                 touch the font bitmaps.
//
//              Currently a define in the project turns on or off the FreeType code which is currently
//              defined out. To pre-render new fonts you need enable the define ( BUILD_FREETYPE ) and
//              uncheck the exclude from build check box in the FreeType2 area of the Renderer project.
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __R_FONT_H__
#define __R_FONT_H__

#pragma once

typedef union
{
    U8	fred[4];
    F32	ffred;
} poor;

#ifdef BUILD_FREETYPE
#include FT_FREETYPE_H
#include FT_ERRORS_H
#include FT_SYSTEM_H
#include FT_IMAGE_H
#include FT_OUTLINE_H

#define _FLOOR(x) ((x) & -64)
#define _CEIL(x) (((x)+63) & -64)
#define _TRUNC(x) ((x) >> 6)

static FT_Library ftLibrary = nullptr;
#endif

#define MAX_FONTS 6
static S32 registeredFontCount = 0;
static fontInfo_t registeredFont[MAX_FONTS];

static S32 fdOffset;
static U8* fdFile;

//
// idRenderSystemFontLocal
//
class idRenderSystemFontLocal
{
public:
    idRenderSystemFontLocal();
    ~idRenderSystemFontLocal();
    
    static void GetGlyphInfo( FT_GlyphSlot glyph, S32* left, S32* right, S32* width, S32* top, S32* bottom, S32* height, S32* pitch );
    static FT_Bitmap* RenderGlyph( FT_GlyphSlot glyph, glyphInfo_t* glyphOut );
    static void WriteTGA( UTF8* filename, U8* data, S32 width, S32 height );
    static glyphInfo_t* ConstructGlyphInfo( U8* imageOut, S32* xOut, S32* yOut, S32* maxHeight, FT_Face face, const U8 c, bool calcHeight );
    static S32 readInt( void );
    static F32 readFloat( void );
    static void InitFreeType( void );
    static void DoneFreeType( void );
};

extern idRenderSystemFontLocal renderSystemFontLocal;

#endif //!__R_FONT_H__
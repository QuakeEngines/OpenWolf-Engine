////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   q_shared.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description: included first by ALL program modules.
//              A user mod should never modify this file
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__

#ifndef PRE_RELEASE_DEMO
// Dushan for ET game, basegame folder was "ETMAIN"
//#define BASEGAME "etmain"
#define BASEGAME "main"
#else
#define BASEGAME "owtest"
#endif

#define LOCALIZATION_SUPPORT

#define NEW_ANIMS
#define MAX_TEAMNAME    32

#define DEMOEXT	"dm_"			// standard demo extension

#if defined( ppc ) || defined( __ppc ) || defined( __ppc__ ) || defined( __POWERPC__ )
#define idppc 1
#endif

#if defined(__cplusplus) && !defined(min)
template <typename T> __inline T min( T a, T b )
{
    return ( a < b ) ? a : b;
}
template <typename T> __inline T max( T a, T b )
{
    return ( a > b ) ? a : b;
}
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#if defined __GNUC__ || defined __clang__
#define _attribute( x ) __attribute__( x )
#else
#define _attribute( x )
#define __attribute__( x )
#endif

//bani
//======================= GNUC DEFINES ==================================
#if (defined _MSC_VER)
#define Q_EXPORT __declspec(dllexport)
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
#define Q_EXPORT __attribute__((visibility("default")))
#else
#define Q_EXPORT
#endif
//=============================================================

typedef union
{
    F32 f;
    S32 i;
    U32 ui;
} floatint_t;

typedef S32 qhandle_t;
typedef S32 sfxHandle_t;
typedef S32 fileHandle_t;
typedef S32 clipHandle_t;

//#define	SND_NORMAL			0x000	// (default) Allow sound to be cut off only by the same sound on this channel
#define     SND_OKTOCUT         0x001   // Allow sound to be cut off by any following sounds on this channel
#define     SND_REQUESTCUT      0x002   // Allow sound to be cut off by following sounds on this channel only for sounds who request cutoff
#define     SND_CUTOFF          0x004   // Cut off sounds on this channel that are marked 'SND_REQUESTCUT'
#define     SND_CUTOFF_ALL      0x008   // Cut off all sounds on this channel
#define     SND_NOCUT           0x010   // Don't cut off.  Always let finish (overridden by SND_CUTOFF_ALL)
#define     SND_NO_ATTENUATION  0x020   // don't attenuate (even though the sound is in voice channel, for example)

#if defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x));
#elif defined(__GNUC__)
#define ALIGN(x) __attribute__((aligned(x)))
#else
#define ALIGN(x)
#endif

#define lengthof( a ) (sizeof( (a) ) / sizeof( (a)[0] ))

#define PAD(x,y) (((x)+(y)-1) & ~((y)-1))
#define PADLEN(base, alignment)	(PAD((base), (alignment)) - (base))
#define PADP(base, alignment)	((void *) PAD((intptr_t) (base), (alignment)))

#define STRING(s)			#s
// expand constants before stringifying them
#define XSTRING(s)			STRING(s)

#define MAX_QINT            0x7fffffff
#define MIN_QINT            ( -MAX_QINT - 1 )

#ifndef BIT
#define BIT(x)				(1 << x)
#endif

// TTimo gcc: was missing, added from Q3 source
#define maxow( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define minow( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )


// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define MAX_SP_CLIENTS      64      // increasing this will increase memory usage significantly

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS    1024    // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   256     // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS     1024    // max length of an individual token

#define MAX_INFO_STRING     4096
#define MAX_INFO_KEY        1024
#define MAX_INFO_VALUE      1024

#define BIG_INFO_STRING     8192    // used for system info key only
#define BIG_INFO_KEY        8192
#define BIG_INFO_VALUE      8192

#define MAX_QPATH           64      // max length of a quake game pathname
#define MAX_OSPATH          256     // max length of a filesystem pathname
#define MAX_CMD             1024    // max length of a command line

// rain - increased to 36 to match MAX_NETNAME, fixes #13 - UI stuff breaks
// with very long names
#define MAX_NAME_LENGTH     36      // max length of a client name

#define MAX_SAY_TEXT        150

typedef enum messageStatus_e
{
    MESSAGE_EMPTY = 0,
    MESSAGE_WAITING,        // rate/packet limited
    MESSAGE_WAITING_OVERFLOW,   // packet too large with message
} messageStatus_t;

// paramters for command buffer stuffing
typedef enum cbufExec_e
{
    EXEC_NOW,           // don't return until completed, a VM should NEVER use this,
    // because some commands might cause the VM to be unloaded...
    EXEC_INSERT,        // insert at current position, but don't run yet
    EXEC_APPEND         // add to end of the command buffer (normal case)
} cbufExec_t;


//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES      64      // bit vector of area visibility


// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum printParm_e
{
    PRINT_ALL,
    PRINT_DEVELOPER,        // only print when "developer 1"
    PRINT_WARNING,
    PRINT_ERROR
} printParm_t;

#ifdef  ERR_FATAL
#undef  ERR_FATAL               // this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum
{
    ERR_FATAL,                  // exit the entire game with a popup window
    ERR_VID_FATAL,              // exit the entire game with a popup window and doesn't delete profile.pid
    ERR_DROP,                   // print to console and disconnect from game
    ERR_SERVERDISCONNECT,       // don't kill server
    ERR_DISCONNECT,             // client disconnected from the server
    ERR_NEED_CD,                // pop up the need-cd dialog
    ERR_AUTOUPDATE
} errorParm_t;


// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH          3
#define PROP_SPACE_WIDTH        8
#define PROP_HEIGHT             27
#define PROP_SMALL_SIZE_SCALE   0.75

#define BLINK_DIVISOR           200
#define PULSE_DIVISOR           75

#define UI_LEFT         0x00000000  // default
#define UI_CENTER       0x00000001
#define UI_RIGHT        0x00000002
#define UI_FORMATMASK   0x00000007
#define UI_SMALLFONT    0x00000010
#define UI_BIGFONT      0x00000020  // default
#define UI_GIANTFONT    0x00000040
#define UI_DROPSHADOW   0x00000800
#define UI_BLINK        0x00001000
#define UI_INVERSE      0x00002000
#define UI_PULSE        0x00004000
// JOSEPH 10-24-99
#define UI_MENULEFT     0x00008000
#define UI_MENURIGHT    0x00010000
#define UI_EXSMALLFONT  0x00020000
#define UI_MENUFULL     0x00080000
// END JOSEPH

#define UI_SMALLFONT75  0x00100000

#if !defined(NDEBUG)
#define HUNK_DEBUG
#endif

typedef enum
{
    h_high,
    h_low,
    h_dontcare
} ha_pref;

#define CIN_system  1
#define CIN_loop    2
#define CIN_hold    4
#define CIN_silent  8
#define CIN_shader  16

/*
==============================================================

MATHLIB

==============================================================
*/

#if defined(SSEVEC3_T)
typedef F32   vec3_t[4];		// ALIGN(16);
typedef vec3_t  vec4_t;
#else
typedef F32   vec2_t[2];
typedef F32   vec3_t[3];
typedef F32   vec4_t[4];
#endif

typedef F32   vec5_t[5];

typedef vec3_t  axis_t[3];
typedef F32   matrix3x3_t[9];
typedef F32   matrix_t[16];
typedef F32   quat_t[4];		// | x y z w |

typedef S32     fixed4_t;
typedef S32     fixed8_t;
typedef S32     fixed16_t;

#undef M_PI
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288f
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.414213562f
#endif

#ifndef M_ROOT3
#define M_ROOT3 1.732050808f
#endif

#define ARRAY_INDEX(arr, el)	((S32)( (el) - (arr) ))

#define ARRAY_LEN(x)			(sizeof(x) / sizeof(*(x)))

// angle indexes
#define	PITCH				0	// up / down
#define	YAW					1	// left / right
#define	ROLL				2	// fall over

#define NUMVERTEXNORMALS	162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define TINYCHAR_WIDTH      ( SMALLCHAR_WIDTH )
#define TINYCHAR_HEIGHT     ( SMALLCHAR_HEIGHT )

#define MINICHAR_WIDTH      8
#define MINICHAR_HEIGHT     12

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

#define GIANTCHAR_WIDTH     32
#define GIANTCHAR_HEIGHT    48

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorMdRed;
extern vec4_t colorMdGreen;

#define GAME_INIT_FRAMES    6
#define FRAMETIME           100                 // msec

#define NUMBER_OF_COLORS 62
#define Q_COLOR_ESCAPE  '^'

#define COLOR_DEFAULT	'-'
#define COLOR_BLACK     '0'
#define COLOR_RED       '1'
#define COLOR_GREEN     '2'
#define COLOR_YELLOW    '3'
#define COLOR_BLUE      '4'
#define COLOR_CYAN      '5'
#define COLOR_MAGENTA   '6'
#define COLOR_WHITE     '7'
#define COLOR_ORANGE    '8'
#define COLOR_MDGREY    '9'
#define COLOR_LTGREY    ':'
#define COLOR_MDGREEN   '<'
#define COLOR_MDYELLOW  '='
#define COLOR_MDBLUE    '>'
#define COLOR_MDRED     '?'
#define COLOR_LTORANGE  'A'
#define COLOR_MDCYAN    'B'
#define COLOR_MDPURPLE  'C'
#define COLOR_PURPLE	'9'
#define COLOR_NULL      '*'

#define COLOR_BITS  31
#define ColorIndex(c)   ( (((c)) >= 'A' && ((c)) <= 'Z') ? (((c))-'A'+36) : ((((c)) >= 'a' && ((c)) <= 'z')?(((c))-'a'+10):(((c))-'0')) )

#define S_COLOR_BLACK       "^0"
#define S_COLOR_RED         "^1"
#define S_COLOR_GREEN       "^2"
#define S_COLOR_YELLOW      "^3"
#define S_COLOR_BLUE        "^4"
#define S_COLOR_CYAN        "^5"
#define S_COLOR_MAGENTA     "^6"
#define S_COLOR_WHITE       "^7"
#define S_COLOR_ORANGE      "^8"
#define S_COLOR_MDGREY      "^9"
#define S_COLOR_LTGREY      "^:"
//#define S_COLOR_LTGREY		"^;"
#define S_COLOR_MDGREEN     "^<"
#define S_COLOR_MDYELLOW    "^="
#define S_COLOR_MDBLUE      "^>"
#define S_COLOR_MDRED       "^?"
#define S_COLOR_LTORANGE    "^A"
#define S_COLOR_MDCYAN      "^B"
#define S_COLOR_MDPURPLE    "^C"
#define S_COLOR_NULL        "^*"

#define S_COLOR_GREY90	"^:"
#define S_COLOR_GREY80	"^<"
#define S_COLOR_GREY70	"^="
#define S_COLOR_GREY60	"^>"
#define S_COLOR_GREY50	"^?"

#define S_COLOR_DIRTYWH	"^|"

#define S_COLOR_ALPHA_RED					"^A"
#define S_COLOR_ALPHA_ORANGERED				"^B"
#define S_COLOR_ALPHA_DARKORANGE			"^C"
#define S_COLOR_ALPHA_ORANGE				"^D"
#define S_COLOR_ALPHA_YELLOW1				"^E"
#define S_COLOR_ALPHA_YELLOW2				"^F"
#define S_COLOR_ALPHA_GREENYELLOW			"^G"
#define S_COLOR_ALPHA_CHARTREUSE			"^H"
#define S_COLOR_ALPHA_GREEN1				"^I"
#define S_COLOR_ALPHA_GREEN2				"^J"
#define S_COLOR_ALPHA_SPRINGGREEN			"^K"
#define S_COLOR_ALPHA_GREENCYAN				"^L"
#define S_COLOR_ALPHA_CYAN					"^M"
#define S_COLOR_ALPHA_SKYBLUE				"^N"
#define S_COLOR_ALPHA_AZURE					"^O"
#define S_COLOR_ALPHA_COBALT				"^P"
#define S_COLOR_ALPHA_BLUE					"^Q"
#define S_COLOR_ALPHA_ELECTRICULTRAMARINE	"^R"
#define S_COLOR_ALPHA_ELECTRICPURPLE		"^S"
#define S_COLOR_ALPHA_LILAC					"^T"
#define S_COLOR_ALPHA_MAGENTA1				"^U"
#define S_COLOR_ALPHA_MAGENTA2				"^V"
#define S_COLOR_ALPHA_BRIGHTPINK			"^W"
#define S_COLOR_ALPHA_FOLLY					"^X"
#define S_COLOR_ALPHA_WHITE					"^Y"
#define S_COLOR_ALPHA_GREY40				"^Z"

// Dushan - Tremulous
#define INDENT_MARKER       '\v'

#define MAX_CCODES	62

extern vec4_t	g_color_table[MAX_CCODES];

#define MAKERGB( v, r, g, b ) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA( v, r, g, b, a ) v[0] = r; v[1] = g; v[2] = b; v[3] = a

// Hex Color string support
#define gethex( ch ) ( ( ch ) > '9' ? ( ( ch ) >= 'a' ? ( ( ch ) - 'a' + 10 ) : ( ( ch ) - '7' ) ) : ( ( ch ) - '0' ) )
#define ishex( ch )  ( ( ch ) && ( ( ( ch ) >= '0' && ( ch ) <= '9' ) || ( ( ch ) >= 'A' && ( ch ) <= 'F' ) || ( ( ch ) >= 'a' && ( ch ) <= 'f' ) ) )
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString( p ) ( ishex( *( p ) ) && ishex( *( ( p ) + 1 ) ) && ishex( *( ( p ) + 2 ) ) && ishex( *( ( p ) + 3 ) ) && ishex( *( ( p ) + 4 ) ) && ishex( *( ( p ) + 5 ) ) )
#define Q_HexColorStringHasAlpha( p ) ( ishex( *( ( p ) + 6 ) ) && ishex( *( ( p ) + 7 ) ) )

#define DEG2RAD( a ) ( ( ( a ) * M_PI ) / 180.0f )
#define RAD2DEG( a ) ( ( ( a ) * 180.0f ) / M_PI )

#define Q_max(a, b)      ((a) > (b) ? (a) : (b))
#define Q_min(a, b)      ((a) < (b) ? (a) : (b))
#define Q_bound(a, b, c) (Q_max(a, Q_min(b, c)))
#define Q_lerp(from, to, frac) (from + ((to - from) * frac))

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];
extern matrix_t matrixIdentity;
extern quat_t   quatIdentity;

#define nanmask ( 255 << 23 )

#define IS_NAN( x ) ( ( ( *(S32 *)&x ) & nanmask ) == nanmask )

static ID_INLINE F32 Q_fabs( F32 x )
{
    floatint_t      tmp;
    
    tmp.f = x;
    tmp.i &= 0x7FFFFFFF;
    return tmp.f;
}

bool Q_IsColorString( StringEntry p );

U8 ClampByte( S32 i );
S8 ClampChar( S32 i );
S16 ClampShort( S32 i );

// this isn't a real cheap function to call!
S32 DirToByte( vec3_t dir );
void ByteToDir( S32 b, vec3_t dir );

F32 DotProduct( const vec3_t v1, const vec3_t v2 );
void VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out );
void VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out );

#define VectorCopy( a,b )         ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2] )
#define VectorScale( v, s, o )    ( ( o )[0] = ( v )[0] * ( s ),( o )[1] = ( v )[1] * ( s ),( o )[2] = ( v )[2] * ( s ) )
#define VectorMA( v, s, b, o )    ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ) )
#define VectorLerpTrem( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
  (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
  (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]))

#define VectorClear( a )              ( ( a )[0] = ( a )[1] = ( a )[2] = 0 )
#define VectorNegate( a,b )           ( ( b )[0] = -( a )[0],( b )[1] = -( a )[1],( b )[2] = -( a )[2] )
#define VectorSet( v, x, y, z )       ( ( v )[0] = ( x ), ( v )[1] = ( y ), ( v )[2] = ( z ) )

#define Vector2Set( v, x, y )         ( ( v )[0] = ( x ),( v )[1] = ( y ) )
#define Vector2Copy( a,b )            ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1] )
#define Vector2Subtract( a,b,c )      ( ( c )[0] = ( a )[0] - ( b )[0],( c )[1] = ( a )[1] - ( b )[1] )
#define QuatCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define Vector4Set( v, x, y, z, n )   ( ( v )[0] = ( x ),( v )[1] = ( y ),( v )[2] = ( z ),( v )[3] = ( n ) )
#define Vector4Copy( a,b )            ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2],( b )[3] = ( a )[3] )
#define Vector4MA( v, s, b, o )       ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ),( o )[3] = ( v )[3] + ( b )[3] * ( s ) )
#define Vector4Average( v, b, s, o )  ( ( o )[0] = ( ( v )[0] * ( 1 - ( s ) ) ) + ( ( b )[0] * ( s ) ),( o )[1] = ( ( v )[1] * ( 1 - ( s ) ) ) + ( ( b )[1] * ( s ) ),( o )[2] = ( ( v )[2] * ( 1 - ( s ) ) ) + ( ( b )[2] * ( s ) ),( o )[3] = ( ( v )[3] * ( 1 - ( s ) ) ) + ( ( b )[3] * ( s ) ) )
#define Vector4Lerp( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
  (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
  (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]),\
  (r)[3]=(s)[3]+(f)*((e)[3]-(s)[3]))

#define DotProduct4(x, y)                        ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2]+(x)[3]*(y)[3])

#define SnapVector( v ) {v[0] = ( (S32)( v[0] ) ); v[1] = ( (S32)( v[1] ) ); v[2] = ( (S32)( v[2] ) );}

U32 ColorBytes3( F32 r, F32 g, F32 b );
U32 ColorBytes4( F32 r, F32 g, F32 b, F32 a );

F32 NormalizeColor( const vec3_t in, vec3_t out );
void  ClampColor( vec4_t color );

F32 RadiusFromBounds( const vec3_t mins, const vec3_t maxs );
void ZeroBounds( vec3_t mins, vec3_t maxs );
void ClearBounds( vec3_t mins, vec3_t maxs );
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );

// RB: same as BoundsIntersectPoint but kept for compatibility
bool PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs );

void BoundsAdd( vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
bool BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
bool BoundsIntersectSphere( const vec3_t mins, const vec3_t maxs, const vec3_t origin, F32 radius );
bool BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t origin );

S32 VectorCompare( const vec3_t v1, const vec3_t v2 );

static ID_INLINE void VectorLerp( const vec3_t from, const vec3_t to, F32 frac, vec3_t out )
{
    out[0] = from[0] + ( ( to[0] - from[0] ) * frac );
    out[1] = from[1] + ( ( to[1] - from[1] ) * frac );
    out[2] = from[2] + ( ( to[2] - from[2] ) * frac );
}

//Dushan - Tremulous
#define VectorLerp4( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
  (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
  (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]))

static ID_INLINE S32 VectorCompareEpsilon(
    const vec3_t v1, const vec3_t v2, F32 epsilon )
{
    vec3_t d;
    
    VectorSubtract( v1, v2, d );
    d[ 0 ] = fabsf( d[ 0 ] );
    d[ 1 ] = fabsf( d[ 1 ] );
    d[ 2 ] = fabsf( d[ 2 ] );
    
    if ( d[ 0 ] > epsilon || d[ 1 ] > epsilon || d[ 2 ] > epsilon )
        return 0;
        
    return 1;
}

F32 VectorLength( const vec3_t v );
F32 VectorLengthSquared( const vec3_t v );
F32 Distance( const vec3_t p1, const vec3_t p2 );
F32 DistanceSquared( const vec3_t p1, const vec3_t p2 );
void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross );
F32 VectorNormalize( vec3_t v );       // returns vector length
void VectorNormalizeFast( vec3_t v );     // does NOT return vector length, uses rsqrt approximation
F32 VectorNormalize2( const vec3_t v, vec3_t out );
void VectorInverse( vec3_t v );
void Vector4Scale( const vec4_t in, F32 scale, vec4_t out );
void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out );

S32 NearestPowerOfTwo( S32 val );
S32 Q_log2( S32 val );
F32 Q_acos( F32 c );
S32 Q_isnan( F32 x );
S32     Q_rand( S32* seed );
F32   Q_random( S32* seed );
F32   Q_crandom( S32* seed );

#define random()    ( ( rand() & 0x7fff ) / ( (F32)0x7fff ) )
#define crandom()   ( 2.0f * ( random() - 0.5f ) )

void vectoangles( const vec3_t value1, vec3_t angles );

static ID_INLINE void VectorToAngles( const vec3_t value1, vec3_t angles )
{
    vectoangles( value1, angles );
}

F32 vectoyaw( const vec3_t vec );
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
// TTimo: const F32 ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c
void AxisToAngles( /*const*/ vec3_t axis[3], vec3_t angles );
//void AxisToAngles ( const vec3_t axis[3], vec3_t angles );
F32 VectorDistance( vec3_t v1, vec3_t v2 );
F32 VectorDistanceSquared( vec3_t v1, vec3_t v2 );

F32 VectorMinComponent( vec3_t v );
F32 VectorMaxComponent( vec3_t v );

void AxisClear( vec3_t axis[3] );
void AxisCopy( vec3_t in[3], vec3_t out[3] );

void SetPlaneSignbits( struct cplane_s* out );

F32   AngleMod( F32 a );
F32   LerpAngle( F32 from, F32 to, F32 frac );
void    LerpPosition( vec3_t start, vec3_t end, F32 frac, vec3_t out );
F32   AngleSubtract( F32 a1, F32 a2 );
void    AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 );

F32			AngleNormalize2Pi( F32 angle );
F32			AngleNormalize360( F32 angle );
F32			AngleNormalize180( F32 angle );
F32			AngleDelta( F32 angle1, F32 angle2 );
F32           AngleBetweenVectors( const vec3_t a, const vec3_t b );
void            AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

static ID_INLINE void AnglesToVector( const vec3_t angles, vec3_t out )
{
    AngleVectors( angles, out, NULL, NULL );
}

void            VectorToAngles( const vec3_t value1, vec3_t angles );

F32           PlaneNormalize( vec4_t plane );	// returns normal length
void			PlaneIntersectRay( const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res );

bool        PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw );
bool        PlaneFromPointsOrder( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw );
void			ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void			RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, F32 degrees );
void			RotatePointAroundVertex( vec3_t pnt, F32 rot_x, F32 rot_y, F32 rot_z, const vec3_t origin );
void			RotateAroundDirection( vec3_t axis[3], F32 yaw );
void			MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );
// perpendicular vector could be replaced by this

//S32				PlaneTypeForNormal( vec3_t normal );

void			VectorMatrixMultiply( const vec3_t p, vec3_t m[ 3 ], vec3_t out );

// RB: NOTE renamed MatrixMultiply to AxisMultiply because it conflicts with most new matrix functions
// It is important for mod developers to do this change as well or they risk a memory corruption by using
// the other MatrixMultiply function.
void            AxisMultiply( F32 in1[3][3], F32 in2[3][3], F32 out[3][3] );
void			AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );
void			PerpendicularVector( vec3_t dst, const vec3_t src );

// Ridah
void			GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up );
void			ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
void			ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
F32			DistanceFromLineSquared( vec3_t p, vec3_t lp1, vec3_t lp2 );
F32			DistanceFromVectorSquared( vec3_t p, vec3_t lp1, vec3_t lp2 );
// done.

F32           DistanceBetweenLineSegmentsSquared( const vec3_t sP0, const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, F32* s, F32* t );
F32           DistanceBetweenLineSegments( const vec3_t sP0, const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, F32* s, F32* t );

void            MatrixFromAngles( matrix_t m, F32 pitch, F32 yaw, F32 roll );
void            MatrixSetupTransformFromRotation( matrix_t m, const matrix_t rot, const vec3_t origin );
void            MatrixAffineInverse( const matrix_t in, matrix_t out );
void            MatrixTransformNormal( const matrix_t m, const vec3_t in, vec3_t out );
void            MatrixTransformNormal2( const matrix_t m, vec3_t inout );
void            MatrixTransformPoint( const matrix_t m, const vec3_t in, vec3_t out );

//=============================================

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

//=============================================

F32 Com_Clamp( F32 min, F32 max, F32 value );

UTF8*    COM_SkipPath( UTF8* pathname );
UTF8*	 Com_SkipTokens( UTF8* s, S32 numTokens, UTF8* sep );
UTF8*	 Com_SkipCharset( UTF8* s, UTF8* sep );
void    COM_FixPath( UTF8* pathname );
StringEntry COM_GetExtension( StringEntry name );
void    COM_StripExtension( StringEntry in, UTF8* out );
void    COM_StripExtension2( StringEntry in, UTF8* out, S32 destsize );
void    COM_StripExtension3( StringEntry src, UTF8* dest, S32 destsize );
void    COM_StripFilename( UTF8* in, UTF8* out );
void    COM_DefaultExtension( UTF8* path, S32 maxSize, StringEntry extension );

void    COM_BeginParseSession( StringEntry name );
void    COM_RestoreParseSession( UTF8** data_p );
void    COM_SetCurrentParseLine( S32 line );
S32     COM_GetCurrentParseLine( void );
UTF8*    COM_Parse( UTF8** data_p );

// RB: added COM_Parse2 for having a Doom 3 style tokenizer.
UTF8* COM_Parse2( UTF8** data_p );
UTF8* COM_ParseExt2( UTF8** data_p, bool allowLineBreak );

UTF8*    COM_ParseExt( UTF8** data_p, bool allowLineBreak );
S32     COM_Compress( UTF8* data_p );
void    COM_ParseError( UTF8* format, ... ) _attribute( ( format( printf, 1, 2 ) ) );
void    COM_ParseWarning( UTF8* format, ... ) _attribute( ( format( printf, 1, 2 ) ) );
S32 COM_Parse2Infos( UTF8* buf, S32 max, UTF8 infos[][MAX_INFO_STRING] );

bool COM_BitCheck( const S32 array[], S32 bitNum );
void COM_BitSet( S32 array[], S32 bitNum );
void COM_BitClear( S32 array[], S32 bitNum );

S32     Com_HashKey( UTF8* string, S32 maxlen );

#define MAX_TOKENLENGTH     1024

#ifndef TT_STRING
//token types
#define TT_STRING                   1           // string
#define TT_LITERAL                  2           // literal
#define TT_NUMBER                   3           // number
#define TT_NAME                     4           // name
#define TT_PUNCTUATION              5           // punctuation
#endif

typedef struct pc_token_s
{
    S32 type;
    S32 subtype;
    S32 intvalue;
    F64 floatvalue;
    UTF8 string[MAX_TOKENLENGTH];
    S32 line;
    S32 linescrossed;
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void COM_MatchToken( UTF8** buf_p, UTF8* match );

void COM_Parse21DMatrix( UTF8** buf_p, S32 x, F32* m, bool checkBrackets );
void COM_Parse22DMatrix( UTF8** buf_p, S32 y, S32 x, F32* m );
void COM_Parse23DMatrix( UTF8** buf_p, S32 z, S32 y, S32 x, F32* m );

UTF8* Com_StringContains( UTF8* str1, UTF8* str2, S32 casesensitive );

bool SkipBracedSection( UTF8** program );
bool SkipBracedSection_Depth( UTF8** program, S32 depth ); // start at given depth if already
void SkipRestOfLine( UTF8** data );

// mode parm for FS_FOpenFile
typedef enum
{
    FS_READ,
    FS_WRITE,
    FS_APPEND,
    FS_APPEND_SYNC,
    FS_READ_DIRECT,
    FS_UPDATE
} fsMode_t;

typedef enum
{
    FS_SEEK_CUR,
    FS_SEEK_END,
    FS_SEEK_SET
} fsOrigin_t;

S32 Com_HexStrToInt( StringEntry str );

StringEntry Com_QuoteStr( StringEntry str );
StringEntry Com_UnquoteStr( StringEntry str );

//=============================================

S32 Q_isprint( S32 c );
S32 Q_islower( S32 c );
S32 Q_isupper( S32 c );
S32 Q_isalpha( S32 c );
S32 Q_isnumeric( S32 c );
S32 Q_isalphanumeric( S32 c );
S32 Q_isforfilename( S32 c );
bool Q_isanumber( StringEntry s );
bool Q_strtol( StringEntry s, S64* out );
bool Q_strtoi( StringEntry s, S32* out );
bool Q_isintegral( F32 f );

// portable case insensitive compare
S32     Q_stricmp( StringEntry s1, StringEntry s2 );
S32     Q_strncmp( StringEntry s1, StringEntry s2, S32 n );
S32     Q_stricmpn( StringEntry s1, StringEntry s2, S32 n );
UTF8*    Q_strlwr( UTF8* s1 );
UTF8*    Q_strupr( UTF8* s1 );
StringEntry Q_stristr( StringEntry s, StringEntry find );
// Count the number of char tocount encountered in string
S32 Q_CountChar( StringEntry string, UTF8 tocount );

#ifdef _WIN32
#define Q_putenv _putenv
#else
#define Q_putenv putenv
#endif

#if defined (_WIN32)
// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
//S32 Q_vsnprintf( UTF8* str, size_t size, StringEntry format, va_list args );
template<typename T, typename Y, typename P>
size_t Q_vsnprintf( T* str, Y size, P format, va_list ap );
#else // not using MSVC
#define Q_vsnprintf vsnprintf
#endif

// buffer size safe library replacements
template<typename T, typename Y, typename P>
void Q_strncpyz( T* dest, Y src, P destsize );

template<typename T, typename Y, typename P>
void Q_strcat( T* dest, Y destsize, P format ... );

template<typename T, typename Y, typename P>
size_t Q_snprintf( T* dest, Y destsize, P format, ... );

S32 Q_strnicmp( StringEntry string1, StringEntry string2, S32 n );
bool Q_strreplace( UTF8* dest, S32 destsize, StringEntry find, StringEntry replace );

// strlen that discounts Quake color sequences
S32 Q_PrintStrlen( StringEntry string );
// removes color sequences from string
UTF8* Q_CleanStr( UTF8* string );
// Count the number of UTF8 tocount encountered in string
S32 Q_CountChar( StringEntry string, UTF8 tocount );
// removes whitespaces and other bad directory characters
UTF8* Q_CleanDirName( UTF8* dirname );

//=============================================

UTF8* va( StringEntry format, ... ) __attribute__( ( format( printf, 1, 2 ) ) );

//=============================================

//
// key / value info strings
//
UTF8* Info_ValueForKey( StringEntry s, StringEntry key );
void Info_RemoveKey( UTF8* s, StringEntry key );
void Info_RemoveKey_Big( UTF8* s, StringEntry key );
bool Info_SetValueForKey( UTF8* s, StringEntry key, StringEntry value );
void Info_SetValueForKey_Big( UTF8* s, StringEntry key, StringEntry value );
bool Info_Validate( StringEntry s );
void Info_NextPair( StringEntry* s, UTF8* key, UTF8* value );

// this is only here so the functions in q_shared.c and bg_*.c can link
void Com_Error( S32 level, StringEntry error, ... ) _attribute( ( format( printf, 2, 3 ), noreturn ) );
void Com_FatalError( StringEntry error, ... );
void Com_DropError( StringEntry error, ... );
void Com_Warning( StringEntry error, ... );
void Com_Printf( StringEntry msg, ... ) _attribute( ( format( printf, 1, 2 ) ) );
void Com_DPrintf( StringEntry msg, ... ) _attribute( ( format( printf, 1, 2 ) ) );

/*
==========================================================

  RELOAD STATES

==========================================================
*/

#define RELOAD_SAVEGAME         0x01
#define RELOAD_NEXTMAP          0x02
#define RELOAD_NEXTMAP_WAITING  0x04
#define RELOAD_FAILED           0x08
#define RELOAD_ENDGAME          0x10


//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE        0x0001
#define KEYCATCH_UI             0x0002
#define KEYCATCH_MESSAGE        0x0004
#define KEYCATCH_CGAME          0x0008
#define KEYCATCH_BUG            0x0010


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum
{
    CHAN_AUTO,
    CHAN_LOCAL,     // menu sounds, etc
    CHAN_WEAPON,
    CHAN_VOICE,
    CHAN_ITEM,
    CHAN_BODY,
    CHAN_LOCAL_SOUND,   // chat messages, etc
    CHAN_ANNOUNCER,     // announcer voices, etc
    CHAN_VOICE_BG,  // xkan - background sound for voice (radio static, etc.)
} soundChannel_t;


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
#define ANIM_BITS       10

#define ANGLE2SHORT( x )  ( (S32)( ( x ) * 65536 / 360 ) & 65535 )
#define SHORT2ANGLE( x )  ( ( x ) * ( 360.0 / 65536 ) )

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2   // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4   // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define CLIENTNUM_BITS		7
#define	MAX_CLIENTS			(1<<CLIENTNUM_BITS)		// absolute limit

#define GENTITYNUM_BITS     12  // JPW NERVE put q3ta default back for testing	// don't need to send any more

#define MAX_GENTITIES       ( 1 << GENTITYNUM_BITS )

// tjw: used for limiting weapons that may overflow gentities[]
#define MIN_SPARE_GENTITIES	64

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE      ( MAX_GENTITIES - 1 )
#define ENTITYNUM_WORLD     ( MAX_GENTITIES - 2 )
#define ENTITYNUM_MAX_NORMAL    ( MAX_GENTITIES - 2 )

#define MAX_MODELS          256     // these are sent over the net as 8 bits (Gordon: upped to 9 bits, erm actually it was already at 9 bits, wtf? NEVAR TRUST GAMECODE COMMENTS, comments are evil :E, lets hope it doesnt horribly break anything....)
#define MAX_SOUNDS          256     // so they cannot be blindly increased
#define MAX_CS_SKINS        64
#define MAX_CSSTRINGS       32
#define MAX_EFFECTS			256
#define MAX_FX	            64
#define MAX_CS_SHADERS      32
#define MAX_SERVER_TAGS     256
#define MAX_TAG_FILES       64

#define MAX_MULTI_SPAWNTARGETS  16 // JPW NERVE

#define MAX_CONFIGSTRINGS   1024

#define MAX_DLIGHT_CONFIGSTRINGS    16
#define MAX_SPLINE_CONFIGSTRINGS    8

#define PARTICLE_SNOW128    1
#define PARTICLE_SNOW64     2
#define PARTICLE_SNOW32     3
#define PARTICLE_SNOW256    0

#define PARTICLE_BUBBLE8    4
#define PARTICLE_BUBBLE16   5
#define PARTICLE_BUBBLE32   6
#define PARTICLE_BUBBLE64   7

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO       0       // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO       1       // an info string for server system to client system configuration (timescale, etc)

#define RESERVED_CONFIGSTRINGS  2   // game can't modify below this, only the system can

#define MAX_GAMESTATE_CHARS 16000
typedef struct
{
    S32 stringOffsets[MAX_CONFIGSTRINGS];
    UTF8 stringData[MAX_GAMESTATE_CHARS];
    S32 dataCount;
} gameState_t;

// xkan, 1/10/2003 - adapted from original SP
typedef enum
{
    AISTATE_RELAXED,
    AISTATE_QUERY,
    AISTATE_ALERT,
    AISTATE_COMBAT,
    
    MAX_AISTATES
} aistateEnum_t;

#define REF_FORCE_DLIGHT    ( 1 << 31 ) // RF, passed in through overdraw parameter, force this dlight under all conditions
#define REF_JUNIOR_DLIGHT   ( 1 << 30 ) // (SA) this dlight does not light surfaces.  it only affects dynamic light grid
#define REF_DIRECTED_DLIGHT ( 1 << 29 ) // ydnar: global directional light, origin should be interpreted as a normal vector

// bit field limits
#define MAX_STATS               16
#define MAX_PERSISTANT          16
#define	MAX_MISC    			16	// Dushan - Tremulous
#define MAX_POWERUPS            16
#define MAX_WEAPONS             64  // (SA) and yet more!

#define MAX_EVENTS              4   // max events per frame before we drop events

#define PS_PMOVEFRAMECOUNTBITS  6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c
// (Gordon: unless it doesnt need transmitted over the network, in which case it should prolly go in the new pmext struct anyway)

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
typedef struct playerState_s
{
    S32 commandTime;            // cmd->serverTime of last executed command
    S32 pm_type;
    S32 bobCycle;               // for view bobbing and footstep generation
    S32 pm_flags;               // ducked, jump_held, etc
    S32 pm_time;
    
    vec3_t origin;
    vec3_t velocity;
    S32 weaponTime;
    S32 weaponDelay;            // for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
    S32 grenadeTimeLeft;            // for delayed grenade throwing.  this is set to a #define for grenade
    // lifetime when the attack button goes down, then when attack is released
    // this is the amount of time left before the grenade goes off (or if it
    // gets to 0 while in players hand, it explodes)
    
    
    S32 gravity;
    F32 leanf;                // amount of 'lean' when player is looking around corner //----(SA)	added
    
    S32 speed;
    S32 delta_angles[3];            // add to command angles to get view direction
    // changed by spawns, rotating objects, and teleporters
    
    S32 groundEntityNum;        // ENTITYNUM_NONE = in air
    
    S32 legsTimer;              // don't change low priority animations until this runs out
    S32 legsAnim;               // mask off ANIM_TOGGLEBIT
    
    S32 torsoTimer;             // don't change low priority animations until this runs out
    S32 torsoAnim;              // mask off ANIM_TOGGLEBIT
    
    S32 movementDir;            // a number 0 to 7 that represents the reletive angle
    // of movement to the view angle (axial and diagonals)
    // when at rest, the value will remain unchanged
    // used to twist the legs during strafing
    
    
    
    S32 eFlags;                 // copied to entityState_t->eFlags
    
    S32 eventSequence;          // pmove generated events
    S32 events[MAX_EVENTS];
    S32 eventParms[MAX_EVENTS];
    S32 oldEventSequence;           // so we can see which events have been added since we last converted to entityState_t
    
    S32 externalEvent;          // events set on player from another source
    S32 externalEventParm;
    S32 externalEventTime;
    
    S32 clientNum;              // ranges from 0 to MAX_CLIENTS-1
    
    // weapon info
    S32 weapon;                 // copied to entityState_t->weapon
    S32 weaponstate;
    
    // item info
    S32 item;
    
    vec3_t viewangles;          // for fixed views
    S32 viewheight;
    
    // damage feedback
    S32 damageEvent;            // when it changes, latch the other parms
    S32 damageYaw;
    S32 damagePitch;
    S32 damageCount;
    
    S32 stats[MAX_STATS];
    S32 persistant[MAX_PERSISTANT];         // stats that aren't cleared on death
    S32 powerups[MAX_POWERUPS];         // level.time that the powerup runs out
    S32 ammo;              // total amount of ammo
    S32 ammoclip;          // ammo in clip
    S32 holdable[16];
    S32 holding;                        // the current item in holdable[] that is selected (held)
    S32 weapons[MAX_WEAPONS / ( sizeof( S32 ) * 8 )]; // 64 bits for weapons held
    
    // Ridah, allow for individual bounding boxes
    vec3_t mins, maxs;
    F32 crouchMaxZ;
    F32 crouchViewHeight, standViewHeight, deadViewHeight;
    // variable movement speed
    F32 runSpeedScale, sprintSpeedScale, crouchSpeedScale;
    // done.
    
    // Ridah, view locking for mg42
    S32 viewlocked;
    S32 viewlocked_entNum;
    
    F32 friction;
    
    S32 nextWeapon;
    S32 teamNum;                        // Arnout: doesn't seem to be communicated over the net
    
    // Rafael
    //S32			gunfx;
    
    // RF, burning effect is required for view blending effect
    S32 onFireStart;
    
    S32 serverCursorHint;               // what type of cursor hint the server is dictating
    S32 serverCursorHintVal;            // a value (0-255) associated with the above
    
    //trace_t serverCursorHintTrace;      // not communicated over net, but used to store the current server-side cursorhint trace
    
    // ----------------------------------------------------------------------
    // So to use persistent variables here, which don't need to come from the server,
    // we could use a marker variable, and use that to store everything after it
    // before we read in the new values for the predictedPlayerState, then restore them
    // after copying the structure recieved from the server.
    
    // Arnout: use the pmoveExt_t structure in bg_public.h to store this kind of data now (presistant on client, not network transmitted)
    
    S32 ping;                   // server to game info for scoreboard
    S32 pmove_framecount;
    S32 entityEventSequence;
    
    S32 sprintExertTime;
    
    // JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
    S32 classWeaponTime;                // Arnout : DOES get send over the network
    S32 jumpTime;                   // used in MP to prevent jump accel
    // jpw
    
    S32 weapAnim;                   // mask off ANIM_TOGGLEBIT										//----(SA)	added		// Arnout : DOES get send over the network
    
    bool releasedFire;
    
    F32 aimSpreadScaleFloat;          // (SA) the server-side aimspreadscale that lets it track finer changes but still only
    // transmit the 8bit S32 to the client
    S32 aimSpreadScale;                 // 0 - 255 increases with angular movement		// Arnout : DOES get send over the network
    S32 lastFireTime;                   // used by server to hold last firing frame briefly when randomly releasing trigger (AI)
    
    S32 quickGrenTime;
    
    S32 leanStopDebounceTime;
    
    //----(SA)	added
    
    // seems like heat and aimspread could be tied together somehow, however, they (appear to) change at different rates and
    // I can't currently see how to optimize this to one server->client transmission "weapstatus" value.
    S32 weapHeat[MAX_WEAPONS];          // some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
    S32 curWeapHeat;                    // value for the currently selected weapon (for transmission to client)		// Arnout : DOES get send over the network
    S32 identifyClient;                 // NERVE - SMF
    S32 identifyClientHealth;
    
    aistateEnum_t aiState;          // xkan, 1/10/2003
    
    // Dushan - Tremulous
    S32	generic1;
    S32	loopSound;
    S32	otherEntityNum;
    vec3_t grapplePoint;	// location of grapple to pull towards if PMF_GRAPPLE_PULL
    S32	weaponAnim;			// mask off ANIM_TOGGLEBIT
    S32	clips;				// clips held
    S32	tauntTimer;			// don't allow another taunt until this runs out
    S32	misc[MAX_MISC];		// misc data
    S32	jumppad_frame;
    S32	jumppad_ent;	// jumppad entity hit this frame
    S32 extraFlags;
} playerState_t;


//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define	BUTTON_ATTACK		1
#define	BUTTON_TALK			2			// displays talk balloon and disables actions
#define BUTTON_USE_HOLDABLE 4           // activate upgrade
#define	BUTTON_GESTURE		8
#define	BUTTON_WALKING		16			// walking can't just be infered from MOVE_RUN
// because a key pressed late in the frame will
// only generate a small move value for that frame
// walking will use different animations and
// won't generate footsteps
#define BUTTON_ATTACK2	32
#define BUTTON_DODGE        64          // start a dodge or sprint motion
#define BUTTON_USE_EVOLVE   128         // use target or open evolve menu
#define BUTTON_SPRINT	256

#define	BUTTON_ANY			2048			// any key whatsoever

#define	MOVE_RUN			120			// if forwardmove or rightmove are >= MOVE_RUN,
// then BUTTON_WALKING should be set

// Arnout: doubleTap buttons - DT_NUM can be max 8
typedef enum
{
    DT_NONE,
    DT_MOVELEFT,
    DT_MOVERIGHT,
    DT_FORWARD,
    DT_BACK,
    DT_LEANLEFT,
    DT_LEANRIGHT,
    DT_UP,
    DT_NUM
} dtType_t;

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
    S32 serverTime;
    U8 buttons;
    U8 wbuttons;
    U8 weapon;
    U8 flags;
    S32 angles[3];
    
    S8 forwardmove, rightmove, upmove;
    U8 doubleTap;             // Arnout: only 3 bits used
    
    // rain - in ET, this can be any entity, and it's used as an array
    // index, so make sure it's unsigned
    U8 identClient;           // NERVE - SMF
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL    0xffffff

typedef enum
{
    TR_STATIONARY,
    TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
    TR_LINEAR,
    TR_LINEAR_STOP,
    TR_NONLINEAR_STOP,
    TR_SINE,					// value = base + sin( time / duration ) * delta
    TR_GRAVITY,
    TR_BUOYANCY
} trType_t;

typedef struct
{
    trType_t trType;
    S32 trTime;
    S32 trDuration;             // if non 0, trTime + trDuration = stop time
    //----(SA)	removed
    vec3_t trBase;
    vec3_t trDelta;             // velocity, etc
    //----(SA)	removed
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)


//
// entityState_t->eType
//
typedef enum
{
    ET_GENERAL,
    ET_PLAYER,
    ET_ITEM,
    
    ET_BUILDABLE,       // buildable type
    
    ET_LOCATION,
    
    ET_MISSILE,
    ET_MOVER,
    ET_BEAM,
    ET_PORTAL,
    ET_SPEAKER,
    ET_PUSH_TRIGGER,
    ET_TELEPORT_TRIGGER,
    ET_INVISIBLE,
    ET_GRAPPLE,       // grapple hooked on wall
    
    ET_CORPSE,
    ET_PARTICLE_SYSTEM,
    ET_ANIMMAPOBJ,
    ET_MODELDOOR,
    ET_LIGHTFLARE,
    ET_LEV2_ZAP_CHAIN,
    
    ET_EVENTS       // any of the EV_* events can be added freestanding
    // by setting eType to ET_EVENTS + eventNum
    // this avoids having to set eFlags and eventNum
} entityType_t;

typedef struct entityState_s
{
    S32		number;         // entity index
    entityType_t eType;     // entityType_t
    S32		eFlags;
    
    trajectory_t pos;       // for calculating position
    trajectory_t apos;      // for calculating angles
    
    S32 time;
    S32 time2;
    
    vec3_t origin;
    vec3_t origin2;
    
    vec3_t angles;
    vec3_t angles2;
    
    S32 otherEntityNum;     // shotgun sources, etc
    S32 otherEntityNum2;
    
    S32 groundEntityNum;        // -1 = in air
    
    S32 constantLight;      // r + (g<<8) + (b<<16) + (intensity<<24)
    S32 dl_intensity;       // used for coronas
    S32 loopSound;          // constantly loop this sound
    
    S32 modelindex;
    S32 modelindex2;
    S32 clientNum;          // 0 to (MAX_CLIENTS - 1), for players and corpses
    S32 frame;
    
    S32 solid;              // for client side prediction, trap_linkentity sets this properly
    
    // old style events, in for compatibility only
    S32 _event;				// impulse events -- muzzle flashes, footsteps, etc
    S32 eventParm;
    
    S32 eventSequence;      // pmove generated events
    S32 events[MAX_EVENTS];
    S32 eventParms[MAX_EVENTS];
    
    // for players
    S32 powerups;           // bit flags	// Arnout: used to store entState_t for non-player entities (so we know to draw them translucent clientsided)
    S32 weapon;             // determines weapon and flash model, etc
    S32 legsAnim;           // mask off ANIM_TOGGLEBIT
    S32 torsoAnim;          // mask off ANIM_TOGGLEBIT
    //	S32		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	removed (weap anims will be client-side only)
    
    S32 density;            // for particle effects
    
    S32 dmgFlags;           // to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities
    
    // Ridah
    S32 onFireStart, onFireEnd;
    
    S32 nextWeapon;
    S32 teamNum;
    
    S32 effect1Time, effect2Time, effect3Time;
    
    aistateEnum_t aiState;      // xkan, 1/10/2003
    S32 animMovetype;       // clients can't derive movetype of other clients for anim scripting system
    
    // Dushan - Tremulous
    S32	misc;			// bit flags
    S32	generic1;
    S32	weaponAnim;		// mask off ANIM_TOGGLEBIT
    S32	extraFlags;
} entityState_t;

typedef enum
{
    CA_UNINITIALIZED,
    CA_DISCONNECTED,    // not talking to a server
    CA_AUTHORIZING,     // not used any more, was checking cd key
    CA_CONNECTING,      // sending request packets to the server
    CA_CHALLENGING,     // sending challenge packets to the server
    CA_CONNECTED,       // netchan_t established, getting gamestate
    CA_LOADING,         // only during cgame initialization, never during main loop
    CA_PRIMED,          // got gamestate, waiting for first frame
    CA_ACTIVE,          // game views should be displayed
    CA_CINEMATIC        // playing a cinematic or a static pic, not connected to a server
} connstate_t;

typedef struct lineInfo_t
{
    StringEntry text;	// text
    S32			count;	// number of characters
    F32		sa;		// offset per white space
    F32		ox;		// ofset from left bounds
    F32		width;	// width of line
    F32		height;	// height of line
    
    F32		startColor[4];
    F32		endColor[4];
    F32		defaultColor[4];
} lineInfo_t;

typedef enum
{
    TEXT_ALIGN_LEFT = 0,
    TEXT_ALIGN_CENTER = 1,
    TEXT_ALIGN_RIGHT = 2,
    TEXT_ALIGN_JUSTIFY = 3,
    
    TEXT_ALIGN_NOCLIP = 0x0080,
} textAlign_e;

typedef enum
{
    TEXT_STYLE_SHADOWED = 2,
    TEXT_STYLE_OUTLINED = 4,
    TEXT_STYLE_BLINK = 8,
    TEXT_STYLE_ITALIC = 16,
    
} textStyle_e;

#define Square( x ) ( ( x ) * ( x ) )

// real time
//=============================================


typedef struct qtime_s
{
    S32 tm_sec;     /* seconds after the minute - [0,59] */
    S32 tm_min;     /* minutes after the hour - [0,59] */
    S32 tm_hour;    /* hours since midnight - [0,23] */
    S32 tm_mday;    /* day of the month - [1,31] */
    S32 tm_mon;     /* months since January - [0,11] */
    S32 tm_year;    /* years since 1900 */
    S32 tm_wday;    /* days since Sunday - [0,6] */
    S32 tm_yday;    /* days since January 1 - [0,365] */
    S32 tm_isdst;   /* daylight savings time flag */
} qtime_t;


// server browser sources
#define AS_LOCAL        0
#define AS_GLOBAL       1           // NERVE - SMF - modified
#define AS_FAVORITES    2


// cinematic states
typedef enum f_status
{
    FMV_IDLE,
    FMV_PLAY,       // play
    FMV_EOF,        // all other conditions, i.e. stop/EOF/abort
    FMV_ID_BLT,
    FMV_ID_IDLE,
    FMV_LOOPED,
    FMV_ID_WAIT
} e_status;

typedef enum _flag_status
{
    FLAG_ATBASE = 0,
    FLAG_TAKEN,         // CTF
    FLAG_TAKEN_RED,     // One Flag CTF
    FLAG_TAKEN_BLUE,    // One Flag CTF
    FLAG_DROPPED
} flagStatus_t;

// Dushan - Tremulous
typedef enum
{
    DS_NONE,
    
    DS_PLAYBACK,
    DS_RECORDING,
    
    DS_NUM_DEMO_STATES
} demoState_t;


#define MAX_GLOBAL_SERVERS          4096
#define MAX_OTHER_SERVERS           128
#define MAX_PINGREQUESTS            16
#define MAX_SERVERSTATUSREQUESTS    16

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

// NERVE - SMF - wolf server/game states
typedef enum
{
    GS_INITIALIZE = -1,
    GS_PLAYING,
    GS_WARMUP_COUNTDOWN,
    GS_WARMUP,
    GS_INTERMISSION,
    GS_WAITING_FOR_PLAYERS,
    GS_RESET
} gamestate_t;

// Dushan - Tremulous
#define GENTITYNUM_MASK		(MAX_GENTITIES - 1)

#define MAX_EMOTICON_NAME_LEN		16
#define MAX_EMOTICONS				64

#define MAX_LOCATIONS				64
#define	MAX_MODELS					256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS					256		// so they cannot be blindly increased
#define	MAX_GAME_SHADERS			64
#define	MAX_GAME_PARTICLE_SYSTEMS	64
#define	MAX_HOSTNAME_LENGTH			80		// max length of a host name
#define	MAX_NEWS_STRING				10000

typedef struct
{
    UTF8      name[ MAX_EMOTICON_NAME_LEN ];
#ifndef GAMEDLL
    S32       width;
    qhandle_t shader;
#endif
} emoticon_t;

typedef struct
{
    U32 hi;
    U32 lo;
} clientList_t;

bool Com_ClientListContains( const clientList_t* list, S32 clientNum );
UTF8* Com_ClientListString( const clientList_t* list );
void Com_ClientListParse( clientList_t* list, StringEntry s );

#define SQR( a ) ( ( a ) * ( a ) )

enum
{
    AUTHORIZE_BAD,
    AUTHORIZE_OK,
    AUTHORIZE_NOTVERIFIED,
    AUTHORIZE_CREATECHARACTER,
    AUTHORIZE_DELETECHARACTER,
    AUTHORIZE_ACCOUNTINFO,
    AUTHORIZE_UNAVAILABLE,
};

typedef enum
{
    CT_FRONT_SIDED,
    CT_BACK_SIDED,
    CT_TWO_SIDED
} cullType_t;

#define LERP( a, b, w ) ( ( a ) * ( 1.0f - ( w ) ) + ( b ) * ( w ) )
#define LUMA( red, green, blue ) ( 0.2126f * ( red ) + 0.7152f * ( green ) + 0.0722f * ( blue ) )

#define SAY_ALL		0
#define SAY_TEAM	1
#define SAY_TELL	2
#define SAY_ACTION      3
#define SAY_ACTION_T    4
#define SAY_ADMINS    5

// demo commands
typedef enum
{
    DC_SERVER_COMMAND = -1,
    DC_CLIENT_SET = 0,
    DC_CLIENT_REMOVE,
    DC_SET_STAGE
} demoCommand_t;

bool StringContainsWord( StringEntry haystack, StringEntry needle );
#define VALIDSTRING( a ) ( ( a != nullptr ) && ( a[0] != '\0' ) )

#ifdef __GNUC__
#define QALIGN(x) __attribute__((aligned(x)))
#else
#define QALIGN(x)
#endif

#endif //!__Q_SHARED_H__

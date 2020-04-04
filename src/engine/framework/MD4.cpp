/*
    mdfour.c

    An implementation of MD4 designed for use in the samba SMB
    authentication protocol

    Copyright (C) 1997-1998  Andrew Tridgell

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to:

        Free Software Foundation, Inc.
        59 Temple Place - Suite 330
        Boston, MA  02111-1307, USA

    $Id: mdfour.c,v 1.1 2002/08/23 22:03:27 abster Exp $
*/

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

idMD4SystemLocal MD4SystemLocal;
idMD4System* MD4System = &MD4SystemLocal;

/*
===============
idMD4SystemLocal::idMD4SystemLocal
===============
*/
idMD4SystemLocal::idMD4SystemLocal( void )
{
}

/*
===============
idMD4SystemLocal::~idMD4SystemLocal
===============
*/
idMD4SystemLocal::~idMD4SystemLocal( void )
{
}

/*
===============
idMD4SystemLocal::mdfour64

this applies md4 to 64 U8 chunks
===============
*/
void idMD4SystemLocal::mdfour64( U32* M )
{
    S32 j;
    U32 AA, BB, CC, DD;
    U32 X[16];
    U32 A, B, C, D;
    
    for ( j = 0; j < 16; j++ )
    {
        X[j] = M[j];
    }
    
    A = m->A;
    B = m->B;
    C = m->C;
    D = m->D;
    AA = A;
    BB = B;
    CC = C;
    DD = D;
    
    ROUND1( A, B, C, D, 0, 3 );
    ROUND1( D, A, B, C, 1, 7 );
    ROUND1( C, D, A, B, 2, 11 );
    ROUND1( B, C, D, A, 3, 19 );
    ROUND1( A, B, C, D, 4, 3 );
    ROUND1( D, A, B, C, 5, 7 );
    ROUND1( C, D, A, B, 6, 11 );
    ROUND1( B, C, D, A, 7, 19 );
    ROUND1( A, B, C, D, 8, 3 );
    ROUND1( D, A, B, C, 9, 7 );
    ROUND1( C, D, A, B, 10, 11 );
    ROUND1( B, C, D, A, 11, 19 );
    ROUND1( A, B, C, D, 12, 3 );
    ROUND1( D, A, B, C, 13, 7 );
    ROUND1( C, D, A, B, 14, 11 );
    ROUND1( B, C, D, A, 15, 19 );
    
    ROUND2( A, B, C, D, 0, 3 );
    ROUND2( D, A, B, C, 4, 5 );
    ROUND2( C, D, A, B, 8, 9 );
    ROUND2( B, C, D, A, 12, 13 );
    ROUND2( A, B, C, D, 1, 3 );
    ROUND2( D, A, B, C, 5, 5 );
    ROUND2( C, D, A, B, 9, 9 );
    ROUND2( B, C, D, A, 13, 13 );
    ROUND2( A, B, C, D, 2, 3 );
    ROUND2( D, A, B, C, 6, 5 );
    ROUND2( C, D, A, B, 10, 9 );
    ROUND2( B, C, D, A, 14, 13 );
    ROUND2( A, B, C, D, 3, 3 );
    ROUND2( D, A, B, C, 7, 5 );
    ROUND2( C, D, A, B, 11, 9 );
    ROUND2( B, C, D, A, 15, 13 );
    
    ROUND3( A, B, C, D, 0, 3 );
    ROUND3( D, A, B, C, 8, 9 );
    ROUND3( C, D, A, B, 4, 11 );
    ROUND3( B, C, D, A, 12, 15 );
    ROUND3( A, B, C, D, 2, 3 );
    ROUND3( D, A, B, C, 10, 9 );
    ROUND3( C, D, A, B, 6, 11 );
    ROUND3( B, C, D, A, 14, 15 );
    ROUND3( A, B, C, D, 1, 3 );
    ROUND3( D, A, B, C, 9, 9 );
    ROUND3( C, D, A, B, 5, 11 );
    ROUND3( B, C, D, A, 13, 15 );
    ROUND3( A, B, C, D, 3, 3 );
    ROUND3( D, A, B, C, 11, 9 );
    ROUND3( C, D, A, B, 7, 11 );
    ROUND3( B, C, D, A, 15, 15 );
    
    A += AA;
    B += BB;
    C += CC;
    D += DD;
    
    for ( j = 0; j < 16; j++ )
    {
        X[j] = 0;
    }
    
    m->A = A;
    m->B = B;
    m->C = C;
    m->D = D;
}

/*
===============
idMD4SystemLocal::copy64
===============
*/
void idMD4SystemLocal::copy64( U32* M, U8* in )
{
    S32 i;
    
    for ( i = 0; i < 16; i++ )
    {
        M[i] = ( in[i * 4 + 3] << 24 ) | ( in[i * 4 + 2] << 16 ) |
               ( in[i * 4 + 1] << 8 ) | ( in[i * 4 + 0] << 0 );
    }
}

/*
===============
idMD4SystemLocal::copy4
===============
*/
void idMD4SystemLocal::copy4( U8* out, U32 x )
{
    out[0] = x & 0xFF;
    out[1] = ( x >> 8 ) & 0xFF;
    out[2] = ( x >> 16 ) & 0xFF;
    out[3] = ( x >> 24 ) & 0xFF;
}

/*
===============
idMD4SystemLocal::mdfour_begin
===============
*/
void idMD4SystemLocal::mdfour_begin( struct mdfour* md )
{
    md->A = 0x67452301;
    md->B = 0xefcdab89;
    md->C = 0x98badcfe;
    md->D = 0x10325476;
    md->totalN = 0;
}

/*
===============
idMD4SystemLocal::mdfour_tail
===============
*/
void idMD4SystemLocal::mdfour_tail( U8* in, S32 n )
{
    U8 buf[128];
    U32 M[16];
    U32 b;
    
    m->totalN += n;
    
    b = m->totalN * 8;
    
    ::memset( buf, 0, 128 );
    if ( n ) ::memcpy( buf, in, n );
    buf[n] = 0x80;
    
    if ( n <= 55 )
    {
        copy4( buf + 56, b );
        copy64( M, buf );
        mdfour64( M );
    }
    else
    {
        copy4( buf + 120, b );
        copy64( M, buf );
        mdfour64( M );
        copy64( M, buf + 64 );
        mdfour64( M );
    }
}

/*
===============
idMD4SystemLocal::mdfour_update
===============
*/
void idMD4SystemLocal::mdfour_update( struct mdfour* md, U8* in, S32 n )
{
    U32 M[16];
    
    m = md;
    
    if ( n == 0 ) mdfour_tail( in, n );
    
    while ( n >= 64 )
    {
        copy64( M, in );
        mdfour64( M );
        in += 64;
        n -= 64;
        m->totalN += 64;
    }
    
    mdfour_tail( in, n );
}

/*
===============
idMD4SystemLocal::mdfour_result
===============
*/
void idMD4SystemLocal::mdfour_result( struct mdfour* md, U8* out )
{
    m = md;
    
    copy4( out, m->A );
    copy4( out + 4, m->B );
    copy4( out + 8, m->C );
    copy4( out + 12, m->D );
}

/*
===============
idMD4SystemLocal::mdfour
===============
*/
void idMD4SystemLocal::mdfour( U8* out, U8* in, S32 n )
{
    struct mdfour md;
    mdfour_begin( &md );
    mdfour_update( &md, in, n );
    mdfour_result( &md, out );
}

/*
===============
idMD4SystemLocal::mdfour_hex
===============
*/
void idMD4SystemLocal::mdfour_hex( const U8 md4[16], S32 hex[32] )
{
    static const UTF8 digits[] = "0123456789abcdef";
    
    S32 i, j, t;
    for ( i = 0, j = 0; i < 16; i += 1, j += 2 )
    {
        // high nibble
        t = ( md4[i] & 0xf0 ) >> 4;
        hex[j] = digits[t];
        // low nibble
        t = md4[i] & 0x0f;
        hex[j + 1] = digits[t];
    }
}

/*
===============
idMD4SystemLocal::BlockChecksum
===============
*/
U32 idMD4SystemLocal::BlockChecksum( const void* buffer, S32 length )
{
    S32	digest[4];
    U32	val;
    
#ifdef USE_OPENSSL
    MD4( ( U8* )buffer, length, ( U8* )digest );
    return digest[0] ^ digest[1] ^ digest[2] ^ digest[3];
#else
    mdfour( ( U8* )digest, ( U8* )buffer, length );
    val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];
    
    return val;
#endif	/* USE_OPENSSL */
}

typedef struct MD5Context
{
    U32        buf[4];
    U32        bits[2];
    U8   in[64];
} MD5_CTX;

static void MD5Init( struct MD5Context* ctx )
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;
    
    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void MD5Transform( U32 buf[4], U32 const in[16] )
{
    register U32 a, b, c, d;
    
    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];
    
    MD5STEP( F1, a, b, c, d, in[0] + 0xd76aa478, 7 );
    MD5STEP( F1, d, a, b, c, in[1] + 0xe8c7b756, 12 );
    MD5STEP( F1, c, d, a, b, in[2] + 0x242070db, 17 );
    MD5STEP( F1, b, c, d, a, in[3] + 0xc1bdceee, 22 );
    MD5STEP( F1, a, b, c, d, in[4] + 0xf57c0faf, 7 );
    MD5STEP( F1, d, a, b, c, in[5] + 0x4787c62a, 12 );
    MD5STEP( F1, c, d, a, b, in[6] + 0xa8304613, 17 );
    MD5STEP( F1, b, c, d, a, in[7] + 0xfd469501, 22 );
    MD5STEP( F1, a, b, c, d, in[8] + 0x698098d8, 7 );
    MD5STEP( F1, d, a, b, c, in[9] + 0x8b44f7af, 12 );
    MD5STEP( F1, c, d, a, b, in[10] + 0xffff5bb1, 17 );
    MD5STEP( F1, b, c, d, a, in[11] + 0x895cd7be, 22 );
    MD5STEP( F1, a, b, c, d, in[12] + 0x6b901122, 7 );
    MD5STEP( F1, d, a, b, c, in[13] + 0xfd987193, 12 );
    MD5STEP( F1, c, d, a, b, in[14] + 0xa679438e, 17 );
    MD5STEP( F1, b, c, d, a, in[15] + 0x49b40821, 22 );
    
    MD5STEP( F2, a, b, c, d, in[1] + 0xf61e2562, 5 );
    MD5STEP( F2, d, a, b, c, in[6] + 0xc040b340, 9 );
    MD5STEP( F2, c, d, a, b, in[11] + 0x265e5a51, 14 );
    MD5STEP( F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20 );
    MD5STEP( F2, a, b, c, d, in[5] + 0xd62f105d, 5 );
    MD5STEP( F2, d, a, b, c, in[10] + 0x02441453, 9 );
    MD5STEP( F2, c, d, a, b, in[15] + 0xd8a1e681, 14 );
    MD5STEP( F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20 );
    MD5STEP( F2, a, b, c, d, in[9] + 0x21e1cde6, 5 );
    MD5STEP( F2, d, a, b, c, in[14] + 0xc33707d6, 9 );
    MD5STEP( F2, c, d, a, b, in[3] + 0xf4d50d87, 14 );
    MD5STEP( F2, b, c, d, a, in[8] + 0x455a14ed, 20 );
    MD5STEP( F2, a, b, c, d, in[13] + 0xa9e3e905, 5 );
    MD5STEP( F2, d, a, b, c, in[2] + 0xfcefa3f8, 9 );
    MD5STEP( F2, c, d, a, b, in[7] + 0x676f02d9, 14 );
    MD5STEP( F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20 );
    
    MD5STEP( F3, a, b, c, d, in[5] + 0xfffa3942, 4 );
    MD5STEP( F3, d, a, b, c, in[8] + 0x8771f681, 11 );
    MD5STEP( F3, c, d, a, b, in[11] + 0x6d9d6122, 16 );
    MD5STEP( F3, b, c, d, a, in[14] + 0xfde5380c, 23 );
    MD5STEP( F3, a, b, c, d, in[1] + 0xa4beea44, 4 );
    MD5STEP( F3, d, a, b, c, in[4] + 0x4bdecfa9, 11 );
    MD5STEP( F3, c, d, a, b, in[7] + 0xf6bb4b60, 16 );
    MD5STEP( F3, b, c, d, a, in[10] + 0xbebfbc70, 23 );
    MD5STEP( F3, a, b, c, d, in[13] + 0x289b7ec6, 4 );
    MD5STEP( F3, d, a, b, c, in[0] + 0xeaa127fa, 11 );
    MD5STEP( F3, c, d, a, b, in[3] + 0xd4ef3085, 16 );
    MD5STEP( F3, b, c, d, a, in[6] + 0x04881d05, 23 );
    MD5STEP( F3, a, b, c, d, in[9] + 0xd9d4d039, 4 );
    MD5STEP( F3, d, a, b, c, in[12] + 0xe6db99e5, 11 );
    MD5STEP( F3, c, d, a, b, in[15] + 0x1fa27cf8, 16 );
    MD5STEP( F3, b, c, d, a, in[2] + 0xc4ac5665, 23 );
    
    MD5STEP( F4, a, b, c, d, in[0] + 0xf4292244, 6 );
    MD5STEP( F4, d, a, b, c, in[7] + 0x432aff97, 10 );
    MD5STEP( F4, c, d, a, b, in[14] + 0xab9423a7, 15 );
    MD5STEP( F4, b, c, d, a, in[5] + 0xfc93a039, 21 );
    MD5STEP( F4, a, b, c, d, in[12] + 0x655b59c3, 6 );
    MD5STEP( F4, d, a, b, c, in[3] + 0x8f0ccc92, 10 );
    MD5STEP( F4, c, d, a, b, in[10] + 0xffeff47d, 15 );
    MD5STEP( F4, b, c, d, a, in[1] + 0x85845dd1, 21 );
    MD5STEP( F4, a, b, c, d, in[8] + 0x6fa87e4f, 6 );
    MD5STEP( F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10 );
    MD5STEP( F4, c, d, a, b, in[6] + 0xa3014314, 15 );
    MD5STEP( F4, b, c, d, a, in[13] + 0x4e0811a1, 21 );
    MD5STEP( F4, a, b, c, d, in[4] + 0xf7537e82, 6 );
    MD5STEP( F4, d, a, b, c, in[11] + 0xbd3af235, 10 );
    MD5STEP( F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15 );
    MD5STEP( F4, b, c, d, a, in[9] + 0xeb86d391, 21 );
    
    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}


#ifndef Q3_BIG_ENDIAN
#define byteReverse(buf, len)	/* Nothing */
#else
static void     byteReverse( U8* buf, U32 longs );

/*
 * Note: this code is harmless on little-endian machines.
 */
static void byteReverse( U8* buf, U32 longs )
{
    U32        t;

    do
    {
        t = ( U32 )( ( U32 )buf[3] << 8 | buf[2] ) << 16 | ( ( U32 )buf[1] << 8 | buf[0] );
        *( U32* )buf = t;
        buf += 4;
    }
    while ( --longs );
}
#endif							// Q3_BIG_ENDIAN

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
static void MD5Update( struct MD5Context* ctx, U8 const* buf, U32 len )
{
    U32        t;
    
    /* Update bitcount */
    
    t = ctx->bits[0];
    if ( ( ctx->bits[0] = t + ( ( U32 )len << 3 ) ) < t )
        ctx->bits[1]++;			/* Carry from low to high */
    ctx->bits[1] += len >> 29;
    
    t = ( t >> 3 ) & 0x3f;		/* Bytes already in shsInfo->data */
    
    /* Handle any leading odd-sized chunks */
    
    if ( t )
    {
        U8* p = ( U8* )ctx->in + t;
        
        t = 64 - t;
        if ( len < t )
        {
            memcpy( p, buf, len );
            return;
        }
        memcpy( p, buf, t );
        byteReverse( ctx->in, 16 );
        MD5Transform( ctx->buf, ( U32* )ctx->in );
        buf += t;
        len -= t;
    }
    /* Process data in 64-byte chunks */
    
    while ( len >= 64 )
    {
        memcpy( ctx->in, buf, 64 );
        byteReverse( ctx->in, 16 );
        MD5Transform( ctx->buf, ( U32* )ctx->in );
        buf += 64;
        len -= 64;
    }
    
    /* Handle any remaining bytes of data. */
    
    memcpy( ctx->in, buf, len );
}


/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
static void MD5Final( struct MD5Context* ctx, U8* digest )
{
    U32        count;
    U8* p;
    
    /* Compute number of bytes mod 64 */
    count = ( ctx->bits[0] >> 3 ) & 0x3F;
    
    /* Set the first UTF8 of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;
    
    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;
    
    /* Pad out to 56 mod 64 */
    if ( count < 8 )
    {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset( p, 0, count );
        byteReverse( ctx->in, 16 );
        MD5Transform( ctx->buf, ( U32* )ctx->in );
        
        /* Now fill the next block with 56 bytes */
        memset( ctx->in, 0, 56 );
    }
    else
    {
        /* Pad block to 56 bytes */
        memset( p, 0, count - 8 );
    }
    byteReverse( ctx->in, 14 );
    
    /* Append length in bits and transform */
    ( ( U32* )ctx->in )[14] = ctx->bits[0];
    ( ( U32* )ctx->in )[15] = ctx->bits[1];
    
    MD5Transform( ctx->buf, ( U32* )ctx->in );
    byteReverse( ( U8* )ctx->buf, 4 );
    
    if ( digest != NULL )
        memcpy( digest, ctx->buf, 16 );
    memset( ctx, 0, sizeof( ctx ) );	/* In case it's sensitive */
}


UTF8* Com_MD5File( StringEntry fn, S32 length, StringEntry prefix, S32 prefix_len )
{
    static UTF8     final[33] = { "" };
    U8   digest[16] = { "" };
    fileHandle_t    f;
    MD5_CTX         md5;
    U8              buffer[2048];
    S32             i;
    S32             filelen = 0;
    S32             r = 0;
    S32             total = 0;
    
    Q_strncpyz( final, "", sizeof( final ) );
    
    filelen = fileSystem->SV_FOpenFileRead( fn, &f );
    
    if ( !f )
    {
        return final;
    }
    if ( filelen < 1 )
    {
        fileSystem->FCloseFile( f );
        return final;
    }
    if ( filelen < length || !length )
    {
        length = filelen;
    }
    
    MD5Init( &md5 );
    
    if ( prefix_len && *prefix )
        MD5Update( &md5, ( U8* )prefix, prefix_len );
        
    for ( ;; )
    {
        r = fileSystem->Read( buffer, sizeof( buffer ), f );
        if ( r < 1 )
            break;
        if ( r + total > length )
            r = length - total;
        total += r;
        MD5Update( &md5, buffer, r );
        if ( r < sizeof( buffer ) || total >= length )
            break;
    }
    fileSystem->FCloseFile( f );
    MD5Final( &md5, digest );
    final[0] = '\0';
    for ( i = 0; i < 16; i++ )
    {
        Q_strcat( final, sizeof( final ), va( "%02X", digest[i] ) );
    }
    return final;
}

void MD5InitSeed( MD5_CTX* mdContext, U64 pseudoRandomNumber )
{
    mdContext->bits[0] = mdContext->bits[1] = ( U32 )0;
    mdContext->buf[0] = ( U32 )0x67452301 + pseudoRandomNumber * 11;
    mdContext->buf[1] = ( U32 )0xefcdab89 + pseudoRandomNumber * 71;
    mdContext->buf[2] = ( U32 )0x98badcfe + pseudoRandomNumber * 37;
    mdContext->buf[3] = ( U32 )0x10325476 + pseudoRandomNumber * 97;
}

static UTF8* CalculateMD5ForSeed( StringEntry key, S32 seed )
{
    MD5_CTX           ctx;
    S32               i;
    static UTF8       hash[33];
    static const UTF8 hex[17] = "0123456789abcdef";
    U8     digest[16];
    
    MD5InitSeed( &ctx, seed );
    MD5Update( &ctx, ( const U8* )key, ( S32 )::strlen( key ) );
    MD5Final( &ctx, digest );
    
    for ( i = 0; i < 16; i++ )
    {
        hash[i << 1] = hex[digest[i] >> 4];
        hash[( i << 1 ) + 1] = hex[digest[i] & 15];
    }
    hash[i << 1] = 0;
    return hash;
}

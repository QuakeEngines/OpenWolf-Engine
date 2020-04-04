////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   serverCrypto_api.h
// Created:     11/24/2018
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCRYPTO_API_H__
#define __SERVERCRYPTO_API_H__

// key buffers
#define CRYPTOPUBKEYBINSIZE	32U // size in bytes of the public key
#define CRYPTOPUBKEYHEXSIZE	( CRYPTOPUBKEYBINSIZE * 2 + 1 ) // public hex representation size, NULL included
#define CRYPTOSECKEYBINSIZE	32U // size in bytes of the secret key
#define CRYPTOSECKEYHEXSIZE	( CRYPTOSECKEYBINSIZE * 2 + 1 ) // secret hex representation size, NULL included

// public crypto key
typedef struct publicKeys
{
    U8 keyBin[CRYPTOPUBKEYBINSIZE];
    UTF8 keyHex[CRYPTOPUBKEYHEXSIZE];
} publicKey_t;

// secret crypto key
typedef struct secretKeys
{
    U8 keyBin[CRYPTOSECKEYBINSIZE];
    UTF8 keyHex[CRYPTOSECKEYHEXSIZE];
} secretKey_t;

// cipher buffers
#define CRYPTOCIPHERBINSIZE 127U // binary cipher size, calculated to produce exactly 255 chars hex output
#define CRYPTOCIPHERRAWSIZE ( CRYPTOCIPHERBINSIZE - ( CRYPTOPUBKEYBINSIZE + 16U ) + 1 ) // max raw (unencrypted) buffer size, NULL included
#define CRYPTOCIPHERHEXSIZE ( CRYPTOCIPHERBINSIZE * 2 + 1 ) // cipher hex representation size, NULL included

// hash buffers
#define CRYPTOHASHBINSIZE 20U // hash binary size, gives a reasonable 40 chars long hex output
#define CRYPTOHASHHEXSIZE ( CRYPTOHASHBINSIZE * 2 + 1 ) // hash hex representation size, NULL included

//
// idServerCryptoSystem
//
class idServerCryptoSystem
{
public:
    virtual bool InitCrypto( void ) = 0;
    virtual bool GenerateCryptoKeys( publicKey_t* pk, secretKey_t* sk ) = 0;
    virtual bool LoadCryptoKeysFromFS( publicKey_t* pk, StringEntry pkFilename, secretKey_t* sk, StringEntry skFilename ) = 0;
    virtual bool SaveCryptoKeysToFS( publicKey_t* pk, StringEntry pkFilename, secretKey_t* sk, StringEntry skFilename ) = 0;
    virtual bool EncryptString( publicKey_t* pk, StringEntry inRaw, UTF8* outHex, size_t outHexSize ) = 0;
    virtual bool DecryptString( publicKey_t* pk, secretKey_t* sk, StringEntry inHex, UTF8* outRaw, size_t outRawSize ) = 0;
    virtual bool CryptoHash( StringEntry inRaw, UTF8* outHex, size_t outHexSize ) = 0;
};

extern idServerCryptoSystem* serverCryptoSystem;

#endif //!__SERVERCRYPTO_API_H__

////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   systemSound.cpp
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

SDL_AudioDeviceID dev;
bool snd_inited = false;

convar_t* s_sdlBits;
convar_t* s_sdlChannels;
convar_t* s_sdlDevSamps;
convar_t* s_sdlMixSamps;

/* The audio callback. All the magic happens here. */
static S32 dmapos = 0;
static S32 dmasize = 0;

static bool use_custom_memset = false;

/*
===============
SNDDMA_AudioCallback
===============
*/
static void SNDDMA_AudioCallback( void* userdata, Uint8* stream, S32 len )
{
    S32             pos = ( dmapos * ( dma.samplebits / 8 ) );
    
    if ( pos >= dmasize )
        dmapos = pos = 0;
        
    if ( !snd_inited )				/* shouldn't happen, but just in case... */
    {
        memset( stream, '\0', len );
        return;
    }
    else
    {
        S32             tobufend = dmasize - pos;	/* bytes to buffer's end. */
        S32             len1 = len;
        S32             len2 = 0;
        
        if ( len1 > tobufend )
        {
            len1 = tobufend;
            len2 = len - len1;
        }
        memcpy( stream, dma.buffer + pos, len1 );
        if ( len2 <= 0 )
            dmapos += ( len1 / ( dma.samplebits / 8 ) );
        else					/* wraparound? */
        {
            memcpy( stream + len1, dma.buffer, len2 );
            dmapos = ( len2 / ( dma.samplebits / 8 ) );
        }
    }
    
    if ( dmapos >= dmasize )
        dmapos = 0;
}

static struct
{
    U16          enumFormat;
    StringEntry  stringFormat;
} formatToStringTable[] =
{
    {
        AUDIO_U8, "AUDIO_U8"
    },
    {
        AUDIO_S8, "AUDIO_S8"
    },
    {
        AUDIO_U16LSB, "AUDIO_U16LSB"
    },
    {
        AUDIO_S16LSB, "AUDIO_S16LSB"
    },
    {
        AUDIO_U16MSB, "AUDIO_U16MSB"
    },
    {
        AUDIO_S16MSB, "AUDIO_S16MSB"
    },
    {
        AUDIO_S32LSB, "AUDIO_S32LSB"
    },
    {
        AUDIO_S32MSB, "AUDIO_S32MSB"
    },
    {
        AUDIO_F32LSB, "AUDIO_F32LSB"
    },
    {
        AUDIO_F32MSB, "AUDIO_F32MSB"
    }
};

static const U64 formatToStringTableSize = sizeof( formatToStringTable ) / sizeof( formatToStringTable[0] );

/*
===============
SNDDMA_PrintAudiospec
===============
*/
static void SNDDMA_PrintAudiospec( StringEntry str, const SDL_AudioSpec* spec )
{
    StringEntry fmt = NULL;
    
    Com_Printf( "%s:\n", str );
    
    for ( U64 i = 0; i < formatToStringTableSize; i++ )
    {
        if ( spec->format == formatToStringTable[i].enumFormat )
        {
            fmt = formatToStringTable[i].stringFormat;
        }
    }
    
    if ( fmt )
    {
        Com_Printf( "  Format:   %s\n", fmt );
    }
    else
    {
        Com_Printf( "  Format:   " S_COLOR_RED "UNKNOWN (%d)\n", ( S32 )spec->format );
    }
    
    Com_Printf( "  Freq:     %d\n", ( S32 )spec->freq );
    Com_Printf( "  Samples:  %d\n", ( S32 )spec->samples );
    Com_Printf( "  Channels: %d\n", ( S32 )spec->channels );
}

/*
===============
SNDDMA_Init
===============
*/
static S32 SNDDMA_ExpandSampleFrequencyKHzToHz( S32 khz )
{
    switch ( khz )
    {
        default:
        case 44:
            return 44100;
        case 22:
            return 22050;
        case 11:
            return 11025;
    }
}

/*
===============
SNDDMA_Init
===============
*/
bool SNDDMA_Init( S32 sampleFrequencyInKHz )
{
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    S32 tmp;
    
    if ( snd_inited )
        return true;
        
    Com_Printf( "Initializing SDL Audio\n" );
    
    if ( !s_sdlBits )
    {
        s_sdlBits = cvarSystem->Get( "s_sdlBits", "16", CVAR_ARCHIVE, "description" );
        s_sdlChannels = cvarSystem->Get( "s_sdlChannels", "2", CVAR_ARCHIVE, "description" );
        s_sdlDevSamps = cvarSystem->Get( "s_sdlDevSamps", "0", CVAR_ARCHIVE, "description" );
        s_sdlMixSamps = cvarSystem->Get( "s_sdlMixSamps", "0", CVAR_ARCHIVE, "description" );
    }
    
    Com_Printf( "SDL_Init( SDL_INIT_AUDIO )... " );
    
    if ( !SDL_WasInit( SDL_INIT_AUDIO ) )
    {
        if ( SDL_Init( SDL_INIT_AUDIO ) == -1 )
        {
            Com_Printf( "FAILED (%s)\n", SDL_GetError() );
            return false;
        }
    }
    
    Com_Printf( "SDL audio driver is \"%s\".\n", SDL_GetCurrentAudioDriver() );
    
    ::memset( &desired, '\0', sizeof( desired ) );
    ::memset( &obtained, '\0', sizeof( obtained ) );
    
    tmp = ( ( S32 )s_sdlBits->value );
    if ( ( tmp != 16 ) && ( tmp != 8 ) )
        tmp = 16;
        
    desired.freq = SNDDMA_ExpandSampleFrequencyKHzToHz( sampleFrequencyInKHz );
    desired.format = ( ( tmp == 16 ) ? AUDIO_S16SYS : AUDIO_U8 );
    
    // I dunno if this is the best idea, but I'll give it a try...
    //  should probably check a cvar for this...
    if ( s_sdlDevSamps->value )
        desired.samples = ( U16 )s_sdlDevSamps->value;
    else
    {
        // just pick a sane default.
        if ( desired.freq <= 11025 )
            desired.samples = 256;
        else if ( desired.freq <= 22050 )
            desired.samples = 512;
        else if ( desired.freq <= 44100 )
            desired.samples = 1024;
        else
            desired.samples = 2048;	// (*shrug*)
    }
    
    desired.channels = ( S32 )s_sdlChannels->value;
    desired.callback = SNDDMA_AudioCallback;
    
    dev = SDL_OpenAudioDevice( NULL, 0, &desired, &obtained, 0 );
    if ( !dev )
    {
        Com_Printf( "SDL_OpenAudioDevice() failed: %s\n", SDL_GetError() );
        SDL_QuitSubSystem( SDL_INIT_AUDIO );
        return false;
    }
    
    SNDDMA_PrintAudiospec( "SDL_AudioSpec", &obtained );
    
    // dma.samples needs to be big, or id's mixer will just refuse to
    //  work at all; we need to keep it significantly bigger than the
    //  amount of SDL callback samples, and just copy a little each time
    //  the callback runs.
    // 32768 is what the OSS driver filled in here on my system. I don't
    //  know if it's a good value overall, but at least we know it's
    //  reasonable...this is why I let the user override.
    tmp = s_sdlMixSamps->integer;
    if ( !tmp )
        tmp = ( obtained.samples * obtained.channels ) * 10;
        
    if ( tmp & ( tmp - 1 ) ) // not a power of two? Seems to confuse something.
    {
        S32 val = 1;
        while ( val < tmp )
            val <<= 1;
            
        tmp = val;
    }
    
    dmapos = 0;
    dma.samplebits = obtained.format & 0xFF;  // first byte of format is bits.
    dma.channels = obtained.channels;
    dma.samples = tmp;
    dma.submission_chunk = 1;
    dma.speed = obtained.freq;
    dmasize = ( dma.samples * ( dma.samplebits / 8 ) );
    dma.buffer = ( U8* )calloc( 1, dmasize );
    
    if ( !dma.buffer )
    {
        Com_Printf( "Unable to allocate dma buffer\n" );
        SDL_QuitSubSystem( SDL_INIT_AUDIO );
        return false;
    }
    
    Com_Printf( "Starting SDL audio callback...\n" );
    SDL_PauseAudioDevice( dev, 0 ); // start callback.
    
    Com_Printf( "SDL audio initialized.\n" );
    snd_inited = true;
    return true;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
S32 SNDDMA_GetDMAPos( void )
{
    return dmapos;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown( void )
{
    Com_Printf( "Closing SDL audio device...\n" );
    SDL_PauseAudioDevice( dev, 1 );
    SDL_CloseAudioDevice( dev );
    SDL_QuitSubSystem( SDL_INIT_AUDIO );
    free( dma.buffer );
    dma.buffer = NULL;
    dmapos = dmasize = 0;
    snd_inited = false;
    Com_Printf( "SDL audio device shut down.\n" );
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit( void )
{
    SDL_UnlockAudioDevice( dev );
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting( void )
{
    SDL_LockAudioDevice( dev );
}

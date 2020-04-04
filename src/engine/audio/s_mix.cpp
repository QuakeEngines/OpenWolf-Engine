////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
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
// File name:   s_mix.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: portable code to mix sounds for s_dma.cpp
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.h>

static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static S32 snd_vol;

// bk001119 - these not static, required by unix/snd_mixa.s
S32*     snd_p;
S32      snd_linear_count;
S16*   snd_out;

void S_WriteLinearBlastStereo16( void )
{
    S32	i, val;
    
    for ( i = 0 ; i < snd_linear_count ; i += 2 )
    {
        val = snd_p[i] >> 8;
        
        if ( val > 0x7fff )
        {
            snd_out[i] = 0x7fff;
        }
        else if ( val < -32768 )
        {
            snd_out[i] = -32768;
        }
        else
        {
            snd_out[i] = val;
        }
        
        val = snd_p[i + 1] >> 8;
        
        if ( val > 0x7fff )
        {
            snd_out[i + 1] = 0x7fff;
        }
        else if ( val < -32768 )
        {
            snd_out[i + 1] = -32768;
        }
        else
        {
            snd_out[i + 1] = val;
        }
    }
}


void S_TransferStereo16( U64* pbuf, S32 endtime )
{
    S32 lpos;
    S32 ls_paintedtime;
    
    snd_p = ( S32* ) paintbuffer;
    ls_paintedtime = s_paintedtime;
    
    while ( ls_paintedtime < endtime )
    {
        // handle recirculating buffer issues
        lpos = ls_paintedtime & ( ( dma.samples >> 1 ) - 1 );
        
        snd_out = ( S16* ) pbuf + ( lpos << 1 );
        
        snd_linear_count = ( dma.samples >> 1 ) - lpos;
        if ( ls_paintedtime + snd_linear_count > endtime )
        {
            snd_linear_count = endtime - ls_paintedtime;
        }
        
        snd_linear_count <<= 1;
        
        // write a linear blast of samples
        S_WriteLinearBlastStereo16();
        
        snd_p += snd_linear_count;
        ls_paintedtime += ( snd_linear_count >> 1 );
        
        if ( clientAVISystem->VideoRecording() )
        {
            clientAVISystem->WriteAVIAudioFrame( ( U8* )snd_out, snd_linear_count << 1 );
        }
    }
}

/*
===================
S_TransferPaintBuffer

===================
*/
void S_TransferPaintBuffer( S32 endtime )
{
    S32 out_idx;
    S32 count;
    S32 out_mask;
    S32* p;
    S32 step;
    S32 val;
    U64* pbuf;
    
    pbuf = ( U64* )dma.buffer;
    
    if ( s_testsound->integer )
    {
        S32 i, count;
        
        // write a fixed sine wave
        count = ( endtime - s_paintedtime );
        for ( i = 0; i < count; i++ )
        {
            paintbuffer[i].left = paintbuffer[i].right = ( S32 )( sinf( ( s_paintedtime + i ) * 0.1f ) * 20000 * 256 );
        }
    }
    
    if ( dma.samplebits == 16 && dma.channels == 2 )
    {
        // optimized case
        S_TransferStereo16( pbuf, endtime );
    }
    else
    {
        // general case
        p = ( S32* ) paintbuffer;
        count = ( endtime - s_paintedtime ) * dma.channels;
        out_mask = dma.samples - 1;
        out_idx = s_paintedtime * dma.channels & out_mask;
        step = 3 - dma.channels;
        
        if ( dma.samplebits == 16 )
        {
            S16* out = ( S16* ) pbuf;
            while ( count-- )
            {
                val = *p >> 8;
                p += step;
                
                if ( val > 0x7fff )
                {
                    val = 0x7fff;
                }
                else if ( val < -32768 )
                {
                    val = -32768;
                }
                out[out_idx] = val;
                out_idx = ( out_idx + 1 ) & out_mask;
            }
        }
        else if ( dma.samplebits == 8 )
        {
            U8* out = ( U8* ) pbuf;
            while ( count-- )
            {
                val = *p >> 8;
                p += step;
                
                if ( val > 0x7fff )
                {
                    val = 0x7fff;
                }
                else if ( val < -32768 )
                {
                    val = -32768;
                }
                
                out[out_idx] = ( val >> 8 ) + 128;
                out_idx = ( out_idx + 1 ) & out_mask;
            }
        }
    }
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void S_PaintChannelFrom16_scalar( channel_t* ch, const sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S32 data, aoff, boff;
    S32 leftvol, rightvol;
    S32 i, j;
    portable_samplepair_t* samp;
    sndBuffer* chunk;
    S16* samples;
    F32 ooff, fdata[2], fdiv, fleftvol, frightvol;
    
    samp = &paintbuffer[ bufferOffset ];
    
    if ( ch->doppler )
    {
        sampleOffset = ( S32 )( sampleOffset * ch->oldDopplerScale );
    }
    
    if ( sc->soundChannels == 2 )
    {
        sampleOffset *= sc->soundChannels;
        
        if ( sampleOffset & 1 )
        {
            sampleOffset &= ~1;
        }
    }
    
    chunk = sc->soundData;
    while ( sampleOffset >= SND_CHUNK_SIZE )
    {
        chunk = chunk->next;
        sampleOffset -= SND_CHUNK_SIZE;
        if ( !chunk )
        {
            chunk = sc->soundData;
        }
    }
    
    if ( !ch->doppler || ch->dopplerScale == 1.0f )
    {
        leftvol = ch->leftvol * snd_vol;
        rightvol = ch->rightvol * snd_vol;
        samples = chunk->sndChunk;
        for ( i = 0 ; i < count ; i++ )
        {
            data  = samples[sampleOffset++];
            samp[i].left += ( data * leftvol ) >> 8;
            
            if ( sc->soundChannels == 2 )
            {
                data = samples[sampleOffset++];
            }
            samp[i].right += ( data * rightvol ) >> 8;
            
            if ( sampleOffset == SND_CHUNK_SIZE )
            {
                chunk = chunk->next;
                samples = chunk->sndChunk;
                sampleOffset = 0;
            }
        }
    }
    else
    {
        fleftvol = ( F32 )( ch->leftvol * snd_vol );
        frightvol = ( F32 )( ch->rightvol * snd_vol );
        
        ooff = ( F32 )sampleOffset;
        samples = chunk->sndChunk;
        
        for ( i = 0 ; i < count ; i++ )
        {
        
            aoff = ( S32 )ooff;
            ooff = ooff + ch->dopplerScale * sc->soundChannels;
            boff = ( S32 )ooff;
            fdata[0] = fdata[1] = 0;
            
            for ( j = aoff; j < boff; j += sc->soundChannels )
            {
                if ( j == SND_CHUNK_SIZE )
                {
                    chunk = chunk->next;
                    if ( !chunk )
                    {
                        chunk = sc->soundData;
                    }
                    samples = chunk->sndChunk;
                    ooff -= SND_CHUNK_SIZE;
                }
                if ( sc->soundChannels == 2 )
                {
                    fdata[0] += samples[j & ( SND_CHUNK_SIZE - 1 )];
                    fdata[1] += samples[( j + 1 ) & ( SND_CHUNK_SIZE - 1 )];
                }
                else
                {
                    fdata[0] += samples[j & ( SND_CHUNK_SIZE - 1 )];
                    fdata[1] += samples[j & ( SND_CHUNK_SIZE - 1 )];
                }
            }
            fdiv = ( F32 )( 256 * ( boff - aoff ) / sc->soundChannels );
            samp[i].left += ( S32 )( ( fdata[0] * fleftvol ) / fdiv );
            samp[i].right += ( S32 )( ( fdata[1] * frightvol ) / fdiv );
        }
    }
}

static void S_PaintChannelFrom16( channel_t* ch, const sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S_PaintChannelFrom16_scalar( ch, sc, count, sampleOffset, bufferOffset );
}

void S_PaintChannelFromWavelet( channel_t* ch, sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S32 data;
    S32 leftvol, rightvol;
    S32 i;
    portable_samplepair_t* samp;
    sndBuffer* chunk;
    S16* samples;
    
    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;
    
    i = 0;
    samp = &paintbuffer[ bufferOffset ];
    chunk = sc->soundData;
    while ( sampleOffset >= ( SND_CHUNK_SIZE_FLOAT * 4 ) )
    {
        chunk = chunk->next;
        sampleOffset -= ( SND_CHUNK_SIZE_FLOAT * 4 );
        i++;
    }
    
    if ( i != sfxScratchIndex || sfxScratchPointer != sc )
    {
        S_AdpcmGetSamples( chunk, sfxScratchBuffer );
        sfxScratchIndex = i;
        sfxScratchPointer = sc;
    }
    
    samples = sfxScratchBuffer;
    
    for ( i = 0 ; i < count ; i++ )
    {
        data  = samples[sampleOffset++];
        samp[i].left += ( data * leftvol ) >> 8;
        samp[i].right += ( data * rightvol ) >> 8;
        
        if ( sampleOffset == SND_CHUNK_SIZE * 2 )
        {
            chunk = chunk->next;
            decodeWavelet( chunk, sfxScratchBuffer );
            sfxScratchIndex++;
            sampleOffset = 0;
        }
    }
}

void S_PaintChannelFromADPCM( channel_t* ch, sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S32 data;
    S32 leftvol, rightvol;
    S32 i;
    portable_samplepair_t* samp;
    sndBuffer* chunk;
    S16* samples;
    
    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;
    
    i = 0;
    samp = &paintbuffer[ bufferOffset ];
    chunk = sc->soundData;
    
    if ( ch->doppler )
    {
        sampleOffset = ( S32 )( sampleOffset * ch->oldDopplerScale );
    }
    
    while ( sampleOffset >= ( SND_CHUNK_SIZE * 4 ) )
    {
        chunk = chunk->next;
        sampleOffset -= ( SND_CHUNK_SIZE * 4 );
        i++;
    }
    
    if ( i != sfxScratchIndex || sfxScratchPointer != sc )
    {
        S_AdpcmGetSamples( chunk, sfxScratchBuffer );
        sfxScratchIndex = i;
        sfxScratchPointer = sc;
    }
    
    samples = sfxScratchBuffer;
    
    for ( i = 0 ; i < count ; i++ )
    {
        data  = samples[sampleOffset++];
        samp[i].left += ( data * leftvol ) >> 8;
        samp[i].right += ( data * rightvol ) >> 8;
        
        if ( sampleOffset == SND_CHUNK_SIZE * 4 )
        {
            chunk = chunk->next;
            S_AdpcmGetSamples( chunk, sfxScratchBuffer );
            sampleOffset = 0;
            sfxScratchIndex++;
        }
    }
}

void S_PaintChannelFromMuLaw( channel_t* ch, sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S32	data;
    S32	leftvol, rightvol;
    S32 i;
    portable_samplepair_t* samp;
    sndBuffer* chunk;
    U8* samples;
    F32 ooff;
    
    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;
    
    samp = &paintbuffer[ bufferOffset ];
    chunk = sc->soundData;
    while ( sampleOffset >= ( SND_CHUNK_SIZE * 2 ) )
    {
        chunk = chunk->next;
        sampleOffset -= ( SND_CHUNK_SIZE * 2 );
        if ( !chunk )
        {
            chunk = sc->soundData;
        }
    }
    
    if ( !ch->doppler )
    {
        samples = ( U8* )chunk->sndChunk + sampleOffset;
        for ( i = 0 ; i < count ; i++ )
        {
            data  = mulawToShort[*samples];
            samp[i].left += ( data * leftvol ) >> 8;
            samp[i].right += ( data * rightvol ) >> 8;
            samples++;
            if ( samples == ( U8* )chunk->sndChunk + ( SND_CHUNK_SIZE * 2 ) )
            {
                chunk = chunk->next;
                samples = ( U8* )chunk->sndChunk;
            }
        }
    }
    else
    {
        ooff = ( F32 )sampleOffset;
        samples = ( U8* )chunk->sndChunk;
        for ( i = 0 ; i < count ; i++ )
        {
            data  = mulawToShort[samples[( S32 )( ooff )]];
            ooff = ooff + ch->dopplerScale;
            samp[i].left += ( data * leftvol ) >> 8;
            samp[i].right += ( data * rightvol ) >> 8;
            if ( ooff >= SND_CHUNK_SIZE * 2 )
            {
                chunk = chunk->next;
                if ( !chunk )
                {
                    chunk = sc->soundData;
                }
                samples = ( U8* )chunk->sndChunk;
                ooff = 0.0;
            }
        }
    }
}

/*
===================
S_PaintChannels
===================
*/
void S_PaintChannels( S32 endtime )
{
    S32 i;
    S32 end;
    channel_t* ch;
    sfx_t* sc;
    S32 ltime, count;
    S32 sampleOffset;
    
    snd_vol = ( S32 )( s_volume->value * 255 );
    
    //Com_Printf ("%i to %i\n", s_paintedtime, endtime);
    while ( s_paintedtime < endtime )
    {
        // if paintbuffer is smaller than DMA buffer
        // we may need to fill it multiple times
        end = endtime;
        
        if ( endtime - s_paintedtime > PAINTBUFFER_SIZE )
        {
            end = s_paintedtime + PAINTBUFFER_SIZE;
        }
        
        // clear the paint buffer to either music or zeros
        if ( s_rawend < s_paintedtime )
        {
            if ( s_rawend )
            {
                //Com_DPrintf ("background sound underrun\n");
            }
            ::memset( paintbuffer, 0, ( end - s_paintedtime ) * sizeof( portable_samplepair_t ) );
        }
        else
        {
            // copy from the streaming sound source
            S32	s, stop;
            
            stop = ( end < s_rawend ) ? end : s_rawend;
            
            for ( i = s_paintedtime ; i < stop ; i++ )
            {
                s = i & ( MAX_RAW_SAMPLES - 1 );
                paintbuffer[i - s_paintedtime] = s_rawsamples[s];
            }
            //		if (i != end)
            //			Com_Printf ("partial stream\n");
            //		else
            //			Com_Printf ("full stream\n");
            for ( ; i < end ; i++ )
            {
                paintbuffer[i - s_paintedtime].left =
                    paintbuffer[i - s_paintedtime].right = 0;
            }
        }
        
        // paint in the channels.
        ch = s_channels;
        for ( i = 0; i < MAX_CHANNELS ; i++, ch++ )
        {
            if ( !ch->thesfx || ( ch->leftvol < 0.25 && ch->rightvol < 0.25 ) )
            {
                continue;
            }
            
            ltime = s_paintedtime;
            sc = ch->thesfx;
            
            sampleOffset = ltime - ch->startSample;
            count = end - ltime;
            if ( sampleOffset + count > sc->soundLength )
            {
                count = sc->soundLength - sampleOffset;
            }
            
            if ( count > 0 )
            {
                if ( sc->soundCompressionMethod == 1 )
                {
                    S_PaintChannelFromADPCM( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                }
                else if ( sc->soundCompressionMethod == 2 )
                {
                    S_PaintChannelFromWavelet( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                }
                else if ( sc->soundCompressionMethod == 3 )
                {
                    S_PaintChannelFromMuLaw( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                }
                else
                {
                    S_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                }
            }
        }
        
        // paint in the looped channels.
        ch = loop_channels;
        for ( i = 0; i < numLoopChannels ; i++, ch++ )
        {
            if ( !ch->thesfx || ( !ch->leftvol && !ch->rightvol ) )
            {
                continue;
            }
            
            ltime = s_paintedtime;
            sc = ch->thesfx;
            
            if ( sc->soundData == nullptr || sc->soundLength == 0 )
            {
                continue;
            }
            // we might have to make two passes if it
            // is a looping sound effect and the end of
            // the sample is hit
            do
            {
                sampleOffset = ( ltime % sc->soundLength );
                
                count = end - ltime;
                if ( sampleOffset + count > sc->soundLength )
                {
                    count = sc->soundLength - sampleOffset;
                }
                
                if ( count > 0 )
                {
                    if ( sc->soundCompressionMethod == 1 )
                    {
                        S_PaintChannelFromADPCM( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                    }
                    else if ( sc->soundCompressionMethod == 2 )
                    {
                        S_PaintChannelFromWavelet( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                    }
                    else if ( sc->soundCompressionMethod == 3 )
                    {
                        S_PaintChannelFromMuLaw( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                    }
                    else
                    {
                        S_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                    }
                    ltime += count;
                }
            }
            while ( ltime < end );
        }
        
        // transfer out according to DMA format
        S_TransferPaintBuffer( end );
        s_paintedtime = end;
    }
}

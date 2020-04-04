/*
Copyright (c) 2010-2013  p5yc0runn3r at gmail.com

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <framework/precompiled.h>

void dmaHD_Update_Mix( void );
void S_UpdateBackgroundTrack( void );
void S_GetSoundtime( void );
bool S_ScanChannelStarts( void );


// used in dmaEX mixer.
#define SOUND_FULLVOLUME 80
#define SOUND_ATTENUATE 0.0007f

extern channel_t s_channels[];
extern channel_t loop_channels[];
extern S32 numLoopChannels;

extern S32 s_soundStarted;
extern bool s_soundMuted;

extern S32 listener_number;
vec3_t g_listener_origin;
vec3_t g_listener_axis[3];

extern S32 s_soundtime;
extern S32 s_paintedtime;
static S32 dmaHD_inwater;

// MAX_SFX may be larger than MAX_SOUNDS because of custom player sounds
#define MAX_SFX 4096 // This must be the same as the snd_dma.c
#define MAX_SOUNDBYTES (256*1024*1024) // 256MiB MAXIMUM...
extern sfx_t s_knownSfx[];
extern S32 s_numSfx;

extern convar_t* s_mixahead;
convar_t* dmaHD_Enable = nullptr;
convar_t* dmaHD_Interpolation;
convar_t* dmaHD_Mixer;
convar_t* dmaEX_StereoSeparation;

extern loopSound_t loopSounds[];

extern S32 s_rawend;
extern portable_samplepair_t s_rawsamples[];

#define DMAHD_PAINTBUFFER_SIZE 65536
static portable_samplepair_t dmaHD_paintbuffer[DMAHD_PAINTBUFFER_SIZE];
static S32 dmaHD_snd_vol;

bool g_tablesinit = false;
F32 g_voltable[256];

#define SMPCLAMP(a) (((a) < -32768) ? -32768 : ((a) > 32767) ? 32767 : (a))
#define VOLCLAMP(a) (((a) < 0) ? 0 : ((a) > 255) ? 255 : (a))

void dmaHD_InitTables( void )
{
    if ( !g_tablesinit )
    {
        S32 i;
        F32 x, y;
        
        // Volume table.
        for ( i = 0; i < 256; i++ )
        {
            x = ( i * ( 9.0f / 256.0f ) ) + 1.0f;
            y = 1.0f - log10f( x );
            g_voltable[i] = y;
        }
        
        g_tablesinit = true;
    }
}

/*
===============================================================================
PART#01: dmaHD: dma sound EXtension : MEMORY
===============================================================================
*/

S32 g_dmaHD_allocatedsoundmemory = 0;

/*
======================
dmaHD_FreeOldestSound
======================
*/

void dmaHD_FreeOldestSound( void )
{
    S32	i, oldest, used;
    sfx_t* sfx;
    S16* buffer;
    
    oldest = Com_Milliseconds();
    used = 0;
    
    for ( i = 1 ; i < s_numSfx ; i++ )
    {
        sfx = &s_knownSfx[i];
        if ( sfx->inMemory && sfx->lastTimeUsed < oldest )
        {
            used = i;
            oldest = sfx->lastTimeUsed;
        }
    }
    
    sfx = &s_knownSfx[used];
    
    Com_DPrintf( "dmaHD_FreeOldestSound: freeing sound %s\n", sfx->soundName );
    
    i = ( sfx->soundLength * 2 ) * sizeof( S16 );
    
    g_dmaHD_allocatedsoundmemory -= i;
    
    if ( g_dmaHD_allocatedsoundmemory < 0 )
    {
        g_dmaHD_allocatedsoundmemory = 0;
    }
    
    if ( ( buffer = ( S16* )sfx->soundData ) != nullptr )
    {
        free( buffer );
    }
    
    sfx->inMemory = false;
    sfx->soundData = nullptr;
}

/*
======================
dmaHD_AllocateSoundBuffer
======================
*/

S16* dmaHD_AllocateSoundBuffer( S32 samples )
{
    S32 bytes = samples * sizeof( S16 );
    S16* buffer;
    
    while ( g_dmaHD_allocatedsoundmemory > 0 && ( g_dmaHD_allocatedsoundmemory + bytes ) > MAX_SOUNDBYTES )
    {
        dmaHD_FreeOldestSound();
    }
    
    if ( s_numSfx >= ( MAX_SFX - 8 ) )
    {
        dmaHD_FreeOldestSound();
    }
    
    do
    {
        if ( ( buffer = ( S16* )malloc( bytes ) ) != nullptr )
        {
            break;
        }
        dmaHD_FreeOldestSound();
    }
    while ( g_dmaHD_allocatedsoundmemory > 0 );
    
    if ( buffer == nullptr )
    {
        Com_Error( ERR_FATAL, "Out of Memory" );
    }
    
    g_dmaHD_allocatedsoundmemory += bytes;
    
    return buffer;
}

// =======================================================
// DMAHD - Interpolation functions / No need to optimize a lot here since the sounds are interpolated
// once on load and not on playback. This also means that at least twice more memory is used.
// =======================================================
// x0-----x1--t--x2-----x3 / x0/2/3/4 are know samples / t = 0.0 - 1.0 between x1 and x2 / returns y value at point t
static F32 dmaHD_InterpolateCubic( F32 x0, F32 x1, F32 x2, F32 x3, F32 t )
{
    F32 a0, a1, a2, a3;
    
    a0 = x3 - x2 - x0 + x1;
    a1 = x0 - x1 - a0;
    a2 = x2 - x0;
    a3 = x1;
    
    return ( a0 * ( t * t * t ) ) + ( a1 * ( t * t ) ) + ( a2 * t ) + ( a3 );
}

static F32 dmaHD_InterpolateHermite4pt3oX( F32 x0, F32 x1, F32 x2, F32 x3, F32 t )
{
    F32 c0, c1, c2, c3;
    
    c0 = x1;
    c1 = 0.5f * ( x2 - x0 );
    c2 = x0 - ( 2.5f * x1 ) + ( 2 * x2 ) - ( 0.5f * x3 );
    c3 = ( 0.5f * ( x3 - x0 ) ) + ( 1.5f * ( x1 - x2 ) );
    
    return ( ( ( ( ( c3 * t ) + c2 ) * t ) + c1 ) * t ) + c0;
}

static F32 dmaHD_NormalizeSamplePosition( F32 t, S32 samples )
{
    if ( !samples )
    {
        return t;
    }
    
    while ( t < 0.0 )
    {
        t += ( F32 )samples;
    }
    
    while ( t >= ( F32 )samples )
    {
        t -= ( F32 )samples;
    }
    
    return t;
}

static S32 dmaHD_GetSampleRaw_8bitMono( S32 index, S32 samples, U8* data )
{
    if ( index < 0 )
    {
        index += samples;
    }
    else if ( index >= samples )
    {
        index -= samples;
    }
    
    return ( S32 )( ( ( U8 )( data[index] ) - 128 ) << 8 );
}

static S32 dmaHD_GetSampleRaw_16bitMono( S32 index, S32 samples, U8* data )
{
    if ( index < 0 )
    {
        index += samples;
    }
    else if ( index >= samples )
    {
        index -= samples;
    }
    
    return ( S32 )LittleShort( ( ( S16* )data )[index] );
}
static S32 dmaHD_GetSampleRaw_8bitStereo( S32 index, S32 samples, U8* data )
{
    S32 left, right;
    
    if ( index < 0 )
    {
        index += samples;
    }
    else if ( index >= samples )
    {
        index -= samples;
    }
    
    left = ( S32 )( ( ( U8 )( data[index * 2] ) - 128 ) << 8 );
    right = ( S32 )( ( ( U8 )( data[index * 2 + 1] ) - 128 ) << 8 );
    
    return ( left + right ) / 2;
}
static S32 dmaHD_GetSampleRaw_16bitStereo( S32 index, S32 samples, U8* data )
{
    S32 left, right;
    
    if ( index < 0 )
    {
        index += samples;
    }
    else if ( index >= samples )
    {
        index -= samples;
    }
    left = ( S32 )LittleShort( ( ( S16* )data )[index * 2] );
    right = ( S32 )LittleShort( ( ( S16* )data )[index * 2 + 1] );
    
    return ( left + right ) / 2;
}

// Get only decimal part (a - floor(a))
#define FLOAT_DECIMAL_PART(a) (a-(F32)((S32)a))

// t must be a F32 between 0 and samples
static S32 dmaHD_GetInterpolatedSampleHermite4pt3oX( F32 t, S32 samples, U8* data, S32( *dmaHD_GetSampleRaw )( S32, S32, U8* ) )
{
    S32 x, val;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = ( S32 )t;
    
    // Interpolate
    val = ( S32 )dmaHD_InterpolateHermite4pt3oX(
              ( F32 )dmaHD_GetSampleRaw( x - 1, samples, data ),
              ( F32 )dmaHD_GetSampleRaw( x, samples, data ),
              ( F32 )dmaHD_GetSampleRaw( x + 1, samples, data ),
              ( F32 )dmaHD_GetSampleRaw( x + 2, samples, data ), FLOAT_DECIMAL_PART( t ) );
              
    // Clamp
    return SMPCLAMP( val );
}

// t must be a F32 between 0 and samples
static S32 dmaHD_GetInterpolatedSampleCubic( F32 t, S32 samples, U8* data, S32( *dmaHD_GetSampleRaw )( S32, S32, U8* ) )
{
    S32 x, val;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = ( S32 )t;
    
    // Interpolate
    val = ( S32 )dmaHD_InterpolateCubic(
              ( F32 )dmaHD_GetSampleRaw( x - 1, samples, data ),
              ( F32 )dmaHD_GetSampleRaw( x, samples, data ),
              ( F32 )dmaHD_GetSampleRaw( x + 1, samples, data ),
              ( F32 )dmaHD_GetSampleRaw( x + 2, samples, data ), FLOAT_DECIMAL_PART( t ) );
              
    // Clamp
    return SMPCLAMP( val );
}

// t must be a F32 between 0 and samples
static S32 dmaHD_GetInterpolatedSampleLinear( F32 t, S32 samples, U8* data, S32( *dmaHD_GetSampleRaw )( S32, S32, U8* ) )
{
    S32 x, val;
    F32 c0, c1;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = ( S32 )t;
    
    c0 = ( F32 )dmaHD_GetSampleRaw( x, samples, data );
    c1 = ( F32 )dmaHD_GetSampleRaw( x + 1, samples, data );
    
    val = ( S32 )( ( ( c1 - c0 ) * FLOAT_DECIMAL_PART( t ) ) + c0 );
    
    // No need to clamp for linear
    return val;
}

// t must be a float between 0 and samples
static S32 dmaHD_GetNoInterpolationSample( F32 t, S32 samples, U8* data, S32( *dmaHD_GetSampleRaw )( S32, S32, U8* ) )
{
    S32 x;
    
    t = dmaHD_NormalizeSamplePosition( t, samples );
    
    // Get points
    x = ( S32 )t;
    
    if ( FLOAT_DECIMAL_PART( t ) > 0.5 )
    {
        x++;
    }
    
    return dmaHD_GetSampleRaw( x, samples, data );
}

S32( *dmaHD_GetInterpolatedSample )( F32 t, S32 samples, U8* data, S32( *dmaHD_GetSampleRaw )( S32, S32, U8* ) ) = dmaHD_GetInterpolatedSampleHermite4pt3oX;

/*
================
dmaHD_ResampleSfx

resample / decimate to the current source rate
================
*/
void dmaHD_ResampleSfx( sfx_t* sfx, S32 channels, S32 inrate, S32 inwidth, U8* data, bool compressed )
{
    S16* buffer;
    S32( *dmaHD_GetSampleRaw )( S32, S32, U8* );
    F32 stepscale, idx_smp, sample, bsample;
    F32 lp_inva, lp_a, hp_a, lp_data, lp_last, hp_data, hp_last, hp_lastsample;
    S32 outcount, idx_hp, idx_lp;
    
    stepscale = ( F32 )inrate / ( F32 )dma.speed;
    outcount = ( S32 )( ( F32 )sfx->soundLength / stepscale );
    
    // Create secondary buffer for bass sound while performing lowpass filter;
    buffer = dmaHD_AllocateSoundBuffer( outcount * 2 );
    
    // Check if this is a weapon sound.
    sfx->weaponsound = ( ::memcmp( sfx->soundName, "sound/weapons/", 14 ) == 0 ) ? true : false;
    
    if ( channels == 2 )
    {
        dmaHD_GetSampleRaw = ( inwidth == 2 ) ? dmaHD_GetSampleRaw_16bitStereo : dmaHD_GetSampleRaw_8bitStereo;
    }
    else
    {
        dmaHD_GetSampleRaw = ( inwidth == 2 ) ? dmaHD_GetSampleRaw_16bitMono : dmaHD_GetSampleRaw_8bitMono;
    }
    
    // Get last sample from sound effect.
    idx_smp = -( stepscale * 4.0f );
    sample = ( F32 )dmaHD_GetInterpolatedSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
    bsample = ( F32 )dmaHD_GetNoInterpolationSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
    idx_smp += stepscale;
    
    // Set up high pass filter.
    idx_hp = 0;
    hp_last = sample;
    hp_lastsample = sample;
    //buffer[idx_hp++] = sample;
    hp_a = 0.95f;
    
    // Set up Low pass filter.
    idx_lp = outcount;
    lp_last = bsample;
    lp_a = 0.03f;
    lp_inva = ( 1 - lp_a );
    
    // Now do actual high/low pass on actual data.
    for ( ; idx_hp < outcount; idx_hp++ )
    {
        sample = ( F32 )dmaHD_GetInterpolatedSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
        bsample = ( F32 )dmaHD_GetNoInterpolationSample( idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw );
        idx_smp += stepscale;
        
        // High pass.
        hp_data = hp_a * ( hp_last + sample - hp_lastsample );
        buffer[idx_hp] = ( S16 )SMPCLAMP( hp_data );
        hp_last = hp_data;
        hp_lastsample = sample;
        
        // Low pass.
        lp_data = lp_a * ( F32 )bsample + lp_inva * lp_last;
        buffer[idx_lp++] = ( S16 )SMPCLAMP( lp_data );
        lp_last = lp_data;
    }
    
    sfx->soundData = ( sndBuffer* )buffer;
    sfx->soundLength = outcount;
}

bool dmaHD_LoadSound( sfx_t* sfx )
{
    U8* data;
    snd_info_t info;
    UTF8 dmahd_soundName[MAX_QPATH];
    UTF8* lpext;
    
    // Player specific sounds are never directly loaded.
    if ( sfx->soundName[0] == '*' ) return false;
    
    strcpy( dmahd_soundName, sfx->soundName );
    if ( ( lpext = strrchr( sfx->soundName, '.' ) ) != nullptr )
    {
        strcpy( dmahd_soundName, sfx->soundName );
        *( strrchr( dmahd_soundName, '.' ) ) = '\0'; // for sure there is a '.'
    }
    strcat( dmahd_soundName, "_dmahd" );
    if ( lpext != nullptr ) strcat( dmahd_soundName, lpext );
    
    // Just check if file exists
    if ( fileSystem->FOpenFileRead( dmahd_soundName, nullptr, true ) )
    {
        // Load it in.
        if ( !( data = static_cast<U8*>( S_CodecLoad( dmahd_soundName, &info ) ) ) )
        {
            return false;
        }
    }
    else
    {
        // Load it in.
        if ( !( data = static_cast<U8*>( S_CodecLoad( sfx->soundName, &info ) ) ) )
        {
            return false;
        }
    }
    
    // Information
    Com_DPrintf( "Loading sound: %s", sfx->soundName );
    if ( info.width == 1 )
    {
        Com_DPrintf( " [8 bit -> 16 bit]" );
    }
    
    if ( info.rate != dma.speed )
    {
        Com_DPrintf( " [%d Hz -> %d Hz]", info.rate, dma.speed );
    }
    
    Com_DPrintf( "\n" );
    
    sfx->lastTimeUsed = Com_Milliseconds() + 1;
    
    // Do not compress.
    sfx->soundCompressionMethod = 0;
    sfx->soundLength = info.samples;
    sfx->soundData = nullptr;
    
    dmaHD_ResampleSfx( sfx, info.channels, info.rate, info.width, data + info.dataofs, false );
    
    // Free data allocated by Codec
    memorySystem->Free( data );
    
    return true;
}

/*
===============================================================================
PART#02: dmaHD: Mixing
===============================================================================
*/

static void dmaHD_PaintChannelFrom16_HHRTF( channel_t* ch, const sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset, S32 chan )
{
    S32 vol, i, so;
    portable_samplepair_t* samp = &dmaHD_paintbuffer[bufferOffset];
    S16* samples;
    S16* tsamples;
    S32* out;
    ch_side_t* chs = ( chan == 0 ) ? &ch->l : &ch->r;
    
    if ( dmaHD_snd_vol <= 0 ) return;
    
    so = sampleOffset - chs->offset;
    if ( so < 0 )
    {
        // [count -= (-so)] == [count += so]
        count += so;
        so = 0;
    }
    
    if ( ( so + count ) >= sc->soundLength )
    {
        count = sc->soundLength - so;
    }
    
    if ( count <= 0 )
    {
        return;
    }
    
    // Process low frequency
    if ( chs->bassvol > 0 )
    {
        // Select bass frequency offset (just after high frequency)
        samples = &( ( S16* )sc->soundData )[sc->soundLength];
        
        // Calculate volumes.
        vol = chs->bassvol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = ( S32* )samp;
        if ( chan == 1 )
        {
            out++;
        }
        
        for ( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * vol ) >> 8;
            ++tsamples;
            ++out;
            ++out;
        }
    }
    
    // Process high frequency
    if ( chs->vol > 0 )
    {
        // Select high frequency offset.
        samples = ( S16* )sc->soundData;
        
        // Calculate volumes.
        vol = chs->vol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = ( S32* )samp;
        
        if ( chan == 1 )
        {
            out++;
        }
        
        for ( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * vol ) >> 8;
            ++tsamples;
            ++out;
            ++out;
        }
    }
}

static void dmaHD_PaintChannelFrom16_dmaEX2( channel_t* ch, const sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S32 data, rvol, lvol, i, so;
    portable_samplepair_t* samp = &dmaHD_paintbuffer[bufferOffset];
    S16* samples;
    S16* tsamples;
    S32* out;
    
    if ( dmaHD_snd_vol <= 0 )
    {
        return;
    }
    
    so = sampleOffset - ch->l.offset;
    
    if ( so < 0 )
    {
        // [count -= (-so)] == [count += so]
        count += so;
        so = 0;
    }
    
    if ( ( so + count ) > sc->soundLength )
    {
        count = sc->soundLength - so;
    }
    if ( count <= 0 )
    {
        return;
    }
    
    // Process low frequency.
    if ( ch->l.bassvol > 0 )
    {
        samples = &( ( S16* )sc->soundData )[sc->soundLength]; // Select bass frequency offset (just after high frequency)
        
        // Calculate volumes.
        lvol = ch->l.bassvol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = ( S32* )samp;
        for ( i = 0; i < count; i++ )
        {
            data = ( *tsamples * lvol ) >> 8;
            ++tsamples;
            *out += data;
            ++out; // L
            *out += data;
            ++out; // R
        }
    }
    
    // Process high frequency.
    if ( ch->l.vol > 0 || ch->r.vol > 0 )
    {
        // Select high frequency offset.
        samples = ( S16* )sc->soundData;
        
        // Calculate volumes.
        lvol = ch->l.vol * dmaHD_snd_vol;
        rvol = ch->r.vol * dmaHD_snd_vol;
        
        // Behind viewer?
        if ( ch->fixed_origin && ch->sodrot[0] < 0 )
        {
            if ( ch->r.vol > ch->l.vol )
            {
                lvol = -lvol;
            }
            else
            {
                rvol = -rvol;
            }
        }
        
        tsamples = &samples[so];
        out = ( S32* )samp;
        for ( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * lvol ) >> 8;
            ++out; // L
            *out += ( *tsamples * rvol ) >> 8;
            ++out; // R
            ++tsamples;
        }
    }
    
    // Process high frequency reverb.
    if ( ch->l.reverbvol > 0 || ch->r.reverbvol > 0 )
    {
        // Select high frequency offset.
        samples = ( S16* )sc->soundData;
        so = sampleOffset - ch->l.reverboffset;
        if ( so < 0 )
        {
            // [count -= (-so)] == [count += so]
            count += so;
            so = 0;
        }
        
        if ( ( so + count ) > sc->soundLength )
        {
            count = sc->soundLength - so;
        }
        
        // Calculate volumes for reverb.
        lvol = ch->l.reverbvol * dmaHD_snd_vol;
        rvol = ch->r.reverbvol * dmaHD_snd_vol;
        tsamples = &samples[so];
        out = ( S32* )samp;
        
        for ( i = 0; i < count; i++ )
        {
            *out += ( *tsamples * lvol ) >> 8;
            ++out; // L
            *out += ( *tsamples * rvol ) >> 8;
            ++out; // R
            ++tsamples;
        }
    }
}

static void dmaHD_PaintChannelFrom16_dmaEX( channel_t* ch, const sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    S32 rvol, lvol, i, so;
    portable_samplepair_t* samp = &dmaHD_paintbuffer[bufferOffset];
    S16* samples, *bsamples;
    S32* out;
    
    if ( dmaHD_snd_vol <= 0 )
    {
        return;
    }
    
    so = sampleOffset - ch->l.offset;
    if ( so < 0 )
    {
        count += so;    // [count -= (-so)] == [count += so]
        so = 0;
    }
    if ( ( so + count ) > sc->soundLength )
    {
        count = sc->soundLength - so;
    }
    if ( count <= 0 )
    {
        return;
    }
    if ( ch->l.vol <= 0 && ch->r.vol <= 0 )
    {
        return;
    }
    
    samples = &( ( S16* )sc->soundData )[so]; // Select high frequency offset.
    bsamples = &( ( S16* )sc->soundData )[sc->soundLength + so]; // Select bass frequency offset (just after high frequency)
    
    // Calculate volumes.
    lvol = ch->l.vol * dmaHD_snd_vol;
    rvol = ch->r.vol * dmaHD_snd_vol;
    
    // Behind viewer?
    if ( ch->fixed_origin && ch->sodrot[0] < 0 )
    {
        if ( lvol < rvol )
        {
            lvol = -lvol;
        }
        else
        {
            rvol = -rvol;
        }
    }
    out = ( S32* )samp;
    for ( i = 0; i < count; i++ )
    {
        *out += ( ( *samples * lvol ) >> 8 ) + ( ( *bsamples * lvol ) >> 8 );
        ++out; // L
        *out += ( ( *samples * rvol ) >> 8 ) + ( ( *bsamples * rvol ) >> 8 );
        ++out; // R
        ++samples;
        ++bsamples;
    }
}

static void dmaHD_PaintChannelFrom16( channel_t* ch, const sfx_t* sc, S32 count, S32 sampleOffset, S32 bufferOffset )
{
    switch ( dmaHD_Mixer->integer )
    {
            // Hybrid-HRTF
        case 10:
        case 11:
            dmaHD_PaintChannelFrom16_HHRTF( ch, sc, count, sampleOffset, bufferOffset, 0 ); // LEFT
            dmaHD_PaintChannelFrom16_HHRTF( ch, sc, count, sampleOffset, bufferOffset, 1 ); // RIGHT
            break;
            // dmaEX2
        case 20:
            dmaHD_PaintChannelFrom16_dmaEX2( ch, sc, count, sampleOffset, bufferOffset );
            break;
        case 21:
            // No reverb.
            ch->l.reverbvol = ch->r.reverbvol = 0;
            dmaHD_PaintChannelFrom16_dmaEX2( ch, sc, count, sampleOffset, bufferOffset );
            break;
            // dmaEX
        case 30:
            dmaHD_PaintChannelFrom16_dmaEX( ch, sc, count, sampleOffset, bufferOffset );
            break;
    }
}

void dmaHD_TransferPaintBuffer( S32 endtime )
{
    S32 lpos;
    S32 ls_paintedtime;
    S32 i;
    S32 val;
    S32* snd_p;
    S32 snd_linear_count;
    S16* snd_out;
    S16* snd_outtmp;
    U64* pbuf = ( U64* )dma.buffer;
    
    snd_p = ( S32* )dmaHD_paintbuffer;
    ls_paintedtime = s_paintedtime;
    
    while ( ls_paintedtime < endtime )
    {
        // handle recirculating buffer issues
        lpos = ls_paintedtime & ( ( dma.samples >> 1 ) - 1 );
        
        snd_out = ( S16* )pbuf + ( lpos << 1 );
        
        snd_linear_count = ( dma.samples >> 1 ) - lpos;
        if ( ls_paintedtime + snd_linear_count > endtime )
        {
            snd_linear_count = endtime - ls_paintedtime;
        }
        
        snd_linear_count <<= 1;
        
        // write a linear blast of samples
        for ( snd_outtmp = snd_out, i = 0; i < snd_linear_count; ++i )
        {
            val = *snd_p++ >> 8;
            *snd_outtmp++ = SMPCLAMP( val );
        }
        
        ls_paintedtime += ( snd_linear_count >> 1 );
        
        if ( clientAVISystem->VideoRecording() )
        {
            clientAVISystem->WriteAVIAudioFrame( ( U8* )snd_out, snd_linear_count << 1 );
        }
    }
}

void dmaHD_PaintChannels( S32 endtime )
{
    S32 i;
    S32 end;
    channel_t* ch;
    sfx_t* sc;
    S32 ltime, count;
    S32 sampleOffset;
    
    dmaHD_snd_vol = ( S32 )s_volume->value * 256;
    
    while ( s_paintedtime < endtime )
    {
        // if paintbuffer is smaller than DMA buffer we may need to fill it multiple times
        end = endtime;
        
        if ( ( endtime - s_paintedtime ) >= DMAHD_PAINTBUFFER_SIZE )
        {
            end = s_paintedtime + DMAHD_PAINTBUFFER_SIZE;
        }
        
        // clear the paint buffer to either music or zeros
        if ( s_rawend < s_paintedtime )
        {
            ::memset( dmaHD_paintbuffer, 0, ( end - s_paintedtime ) * sizeof( portable_samplepair_t ) );
        }
        else
        {
            // copy from the streaming sound source
            S32 s;
            S32 stop;
            
            stop = ( end < s_rawend ) ? end : s_rawend;
            
            for ( i = s_paintedtime ; i < stop ; i++ )
            {
                s = i & ( MAX_RAW_SAMPLES - 1 );
                dmaHD_paintbuffer[i - s_paintedtime].left = s_rawsamples[s].left;
                dmaHD_paintbuffer[i - s_paintedtime].right = s_rawsamples[s].right;
            }
            for ( ; i < end ; i++ )
            {
                dmaHD_paintbuffer[i - s_paintedtime].left = 0;
                dmaHD_paintbuffer[i - s_paintedtime].right = 0;
            }
        }
        
        // paint in the channels.
        ch = s_channels;
        for ( i = 0; i < MAX_CHANNELS ; i++, ch++ )
        {
            if ( !ch->thesfx )
            {
                continue;
            }
            
            ltime = s_paintedtime;
            sc = ch->thesfx;
            sampleOffset = ltime - ch->startSample;
            count = end - ltime;
            
            if ( sampleOffset + count >= sc->soundLength )
            {
                count = sc->soundLength - sampleOffset;
            }
            
            if ( count > 0 )
            {
                dmaHD_PaintChannelFrom16( ch, sc, count, sampleOffset, 0 );
            }
        }
        
        // paint in the looped channels.
        ch = loop_channels;
        for ( i = 0; i < numLoopChannels ; i++, ch++ )
        {
            if ( !ch->thesfx )
            {
                continue;
            }
            
            ltime = s_paintedtime;
            sc = ch->thesfx;
            
            if ( sc->soundData == nullptr || sc->soundLength == 0 )
            {
                continue;
            }
            // we might have to make two passes if it is a looping sound effect and the end of the sample is hit
            do
            {
                sampleOffset = ( ltime % sc->soundLength );
                count = end - ltime;
                if ( sampleOffset + count >= sc->soundLength )
                {
                    count = sc->soundLength - sampleOffset;
                }
                if ( count > 0 )
                {
                    dmaHD_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
                    ltime += count;
                }
            }
            while ( ltime < end );
        }
        
        // transfer out according to DMA format
        dmaHD_TransferPaintBuffer( end );
        s_paintedtime = end;
    }
}

/*
===============================================================================
PART#03: dmaHD: main
===============================================================================
*/


/*
=================
dmaHD_SpatializeReset

Reset/Prepares channel before calling dmaHD_SpatializeOrigin
=================
*/
void dmaHD_SpatializeReset( channel_t* ch )
{
    VectorClear( ch->sodrot );
    ::memset( &ch->l, 0, sizeof( ch_side_t ) );
    ::memset( &ch->r, 0, sizeof( ch_side_t ) );
}

/*
=================
dmaHD_SpatializeOrigin

Used for spatializing s_channels
=================
*/

#define CALCVOL(dist) (((tmp = (S32)((F32)ch->master_vol * g_voltable[ \
(((idx = (dist / iattenuation)) > 255) ? 255 : idx)])) < 0) ? 0 : tmp)
#define CALCSMPOFF(dist) (dist * dma.speed) >> ismpshift

void dmaHD_SpatializeOrigin_HHRTF( vec3_t so, channel_t* ch )
{
    // so = sound origin/[d]irection/[n]ormalized/[rot]ated/[d]irection [l]eft/[d]irection [r]ight
    vec3_t sod, sodl, sodr;
    // lo = listener origin/[l]eft/[r]ight
    vec3_t lol, lor;
    // distance to ears/[l]eft/[r]ight
    S32 distl, distr; // using S32 since calculations are integer based.
    // temp, index
    S32 tmp, idx;
    F32 t;
    
    S32 iattenuation = ( dmaHD_inwater ) ? 2 : 6;
    S32 ismpshift = ( dmaHD_inwater ) ? 19 : 17;
    
    // Increase attenuation for weapon sounds since they would be very loud!
    if ( ch->thesfx && ch->thesfx->weaponsound )
    {
        iattenuation *= 2;
    }
    
    // Calculate sound direction.
    VectorSubtract( so, g_listener_origin, sod );
    // Rotate sound origin to listener axis
    VectorRotate( sod, g_listener_axis, ch->sodrot );
    
    // Origin for ears (~20cm apart)
    lol[0] = 0.0;
    lol[1] = 40;
    lol[2] = 0.0; // left
    lor[0] = 0.0;
    lor[1] = -40;
    lor[2] = 0.0; // right
    
    // Calculate sound direction.
    VectorSubtract( ch->sodrot, lol, sodl ); // left
    VectorSubtract( ch->sodrot, lor, sodr ); // right
    
    VectorNormalize( ch->sodrot );
    // Calculate length of sound origin direction vector.
    distl = ( S32 )VectorNormalize( sodl ); // left
    distr = ( S32 )VectorNormalize( sodr ); // right
    
    // Close enough to be at full volume?
    if ( distl < 80 )
    {
        distl = 0; // left
    }
    if ( distr < 80 )
    {
        distr = 0; // right
    }
    
    // Distance 384units = 1m
    // 340.29m/s (speed of sound at sea level)
    // Do surround effect with doppler.
    // 384.0 * 340.29 = 130671.36
    // Most similar is 2 ^ 17 = 131072; so shift right by 17 to divide by 131072
    
    // 1484m/s in water
    // 384.0 * 1484 = 569856
    // Most similar is 2 ^ 19 = 524288; so shift right by 19 to divide by 524288
    
    ch->l.offset = CALCSMPOFF( distl ); // left
    ch->r.offset = CALCSMPOFF( distr ); // right
    
    // Calculate volume at ears
    ch->l.bassvol = ch->l.vol = CALCVOL( distl ); // left
    ch->r.bassvol = ch->r.vol = CALCVOL( distr ); // right
    
    if ( distl != 0 || distr != 0 )
    {
        // Sound originating from inside head of left ear (i.e. from right)
        if ( ch->sodrot[1] < 0 )
        {
            ch->l.vol *= ( S32 )( 1.0f + ( ch->sodrot[1] * 0.7f ) );
        }
        
        // Sound originating from inside head of right ear (i.e. from left)
        if ( ch->sodrot[1] > 0 )
        {
            ch->r.vol *= ( S32 )( 1.0f - ( ch->sodrot[1] * 0.7f ) );
        }
        
        // Calculate HRTF function (lowpass filter) parameters
        //if (ch->fixed_origin)
        {
            // Sound originating from behind viewer
            if ( ch->sodrot[0] < 0 )
            {
                ch->l.vol *= ( S32 )( 1.0f + ( ch->sodrot[0] * 0.05f ) );
                ch->r.vol *= ( S32 )( 1.0f + ( ch->sodrot[0] * 0.05f ) );
                
                // 2ms max
                //t = -ch->sodrot[0] * 0.04f; if (t > 0.005f) t = 0.005f;
                t = ( dma.speed * 0.001f );
                ch->l.offset -= ( S32 )t;
                ch->r.offset += ( S32 )t;
            }
        }
        
        if ( dmaHD_Mixer->integer == 10 )
        {
            // Sound originating from above viewer (decrease bass)
            // Sound originating from below viewer (increase bass)
            ch->l.bassvol *= ( S32 )( ( 1 - ch->sodrot[2] ) * 0.5f );
            ch->r.bassvol *= ( S32 )( ( 1 - ch->sodrot[2] ) * 0.5f );
        }
    }
    
    // Normalize volume
    ch->l.vol *= ( S32 )0.5f;
    ch->r.vol *= ( S32 )0.5f;
    
    if ( dmaHD_inwater )
    {
        // Keep bass in water.
        ch->l.vol *= ( S32 )0.2f;
        ch->r.vol *= ( S32 )0.2f;
    }
}

void dmaHD_SpatializeOrigin_dmaEX2( vec3_t so, channel_t* ch )
{
    // so = sound origin/[d]irection/[n]ormalized/[rot]ated
    vec3_t sod;
    // distance to head
    S32 dist; // using S32 since calculations are integer based.
    // temp, index
    S32 tmp, idx, vol;
    F32 dot;
    
    S32 iattenuation = ( dmaHD_inwater ) ? 2 : 6;
    S32 ismpshift = ( dmaHD_inwater ) ? 19 : 17;
    
    // Increase attenuation for weapon sounds since they would be very loud!
    if ( ch->thesfx && ch->thesfx->weaponsound )
    {
        iattenuation *= 2;
    }
    
    // Calculate sound direction.
    VectorSubtract( so, g_listener_origin, sod );
    // Rotate sound origin to listener axis
    VectorRotate( sod, g_listener_axis, ch->sodrot );
    
    VectorNormalize( ch->sodrot );
    // Calculate length of sound origin direction vector.
    dist = ( S32 )VectorNormalize( sod ); // left
    
    // Close enough to be at full volume?
    if ( dist < 0 )
    {
        dist = 0; // left
    }
    
    // Distance 384units = 1m
    // 340.29m/s (speed of sound at sea level)
    // Do surround effect with doppler.
    // 384.0 * 340.29 = 130671.36
    // Most similar is 2 ^ 17 = 131072; so shift right by 17 to divide by 131072
    
    // 1484m/s in water
    // 384.0 * 1484 = 569856
    // Most similar is 2 ^ 19 = 524288; so shift right by 19 to divide by 524288
    
    ch->l.offset = CALCSMPOFF( dist );
    
    // Calculate volume at ears
    vol = CALCVOL( dist );
    ch->l.vol = vol;
    ch->r.vol = vol;
    ch->l.bassvol = vol;
    
    dot = -ch->sodrot[1];
    ch->l.vol *= ( S32 )( 0.5f * ( 1.0f - dot ) );
    ch->r.vol *= ( S32 )( 0.5f * ( 1.0f + dot ) );
    
    // Calculate HRTF function (lowpass filter) parameters
    if ( ch->fixed_origin )
    {
        // Reverberation
        dist += 768;
        ch->l.reverboffset = CALCSMPOFF( dist );
        vol = CALCVOL( dist );
        ch->l.reverbvol = vol;
        ch->r.reverbvol = vol;
        ch->l.reverbvol *= ( S32 )( 0.5f * ( 1.0f + dot ) );
        ch->r.reverbvol *= ( S32 )( 0.5f * ( 1.0f - dot ) );
        
        // Sound originating from behind viewer: decrease treble + reverb
        if ( ch->sodrot[0] < 0 )
        {
            ch->l.vol *= ( S32 )( 1.0 + ( ch->sodrot[0] * 0.5f ) );
            ch->r.vol *= ( S32 )( 1.0 + ( ch->sodrot[0] * 0.5f ) );
        }
        else // from front...
        {
            // adjust reverb for each ear.
            if ( ch->sodrot[1] < 0 )
            {
                ch->r.reverbvol = 0;
            }
            else if ( ch->sodrot[1] > 0 )
            {
                ch->l.reverbvol = 0;
            }
        }
        
        // Sound originating from above viewer (decrease bass)
        // Sound originating from below viewer (increase bass)
        ch->l.bassvol *= ( S32 )( ( 1 - ch->sodrot[2] ) * 0.5f );
    }
    else
    {
        // Reduce base volume by half to keep overall valume.
        ch->l.bassvol *= ( S32 )0.5f;
    }
    
    if ( dmaHD_inwater )
    {
        // Keep bass in water.
        ch->l.vol *= ( S32 )0.2f;
        ch->r.vol *= ( S32 )0.2f;
    }
}

void dmaHD_SpatializeOrigin_dmaEX( vec3_t origin, channel_t* ch )
{
    F32 dot;
    F32 dist;
    F32 lscale, rscale, scale;
    vec3_t source_vec;
    S32 tmp;
    
    const F32 dist_mult = SOUND_ATTENUATE;
    
    // calculate stereo seperation and distance attenuation
    VectorSubtract( origin, g_listener_origin, source_vec );
    
    // VectorNormalize returns original length of vector and normalizes vector.
    dist = VectorNormalize( source_vec );
    dist -= SOUND_FULLVOLUME;
    
    if ( dist < 0 )
    {
        // close enough to be at full volume
        dist = 0;
    }
    
    // different attenuation levels
    dist *= dist_mult;
    
    VectorRotate( source_vec, g_listener_axis, ch->sodrot );
    
    dot = -ch->sodrot[1];
    
    // DMAEX - Multiply by the stereo separation CVAR.
    dot *= dmaEX_StereoSeparation->value;
    
    rscale = ( F32 )( 0.5f * S32( 1.0f + dot ) );
    lscale = ( F32 )( 0.5f * S32( 1.0f - dot ) );
    if ( rscale < 0 )
    {
        rscale = 0;
    }
    
    if ( lscale < 0 )
    {
        lscale = 0;
    }
    
    // add in distance effect
    scale = ( ( F32 )( 1.0f - dist ) * rscale );
    tmp = ( S32 )( ch->master_vol * scale );
    if ( tmp < 0 )
    {
        tmp = 0;
    }
    ch->r.vol = tmp;
    
    scale = ( ( F32 )( 1.0f - dist ) * lscale );
    tmp = ( S32 )( ch->master_vol * scale );
    if ( tmp < 0 ) tmp = 0;
    ch->l.vol = tmp;
}

void dmaHD_SpatializeOrigin( vec3_t so, channel_t* ch )
{
    switch ( dmaHD_Mixer->integer )
    {
            // HHRTF
        case 10:
        case 11:
            dmaHD_SpatializeOrigin_HHRTF( so, ch );
            break;
            // dmaEX2
        case 20:
        case 21:
            dmaHD_SpatializeOrigin_dmaEX2( so, ch );
            break;
            // dmaEX
        case 30:
            dmaHD_SpatializeOrigin_dmaEX( so, ch );
            break;
    }
}

/*
==============================================================
continuous looping sounds are added each frame
==============================================================
*/

/*
==================
dmaHD_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void dmaHD_AddLoopSounds( void )
{
    S32 i, time;
    channel_t* ch;
    loopSound_t* loop;
    static S32 loopFrame;
    
    numLoopChannels = 0;
    
    time = Com_Milliseconds();
    
    loopFrame++;
    
    //#pragma omp parallel for private(loop, ch)
    for ( i = 0 ; i < MAX_GENTITIES; i++ )
    {
        if ( numLoopChannels >= MAX_CHANNELS )
        {
            continue;
        }
        
        loop = &loopSounds[i];
        // already merged into an earlier sound
        if ( !loop->active || loop->mergeFrame == loopFrame )
        {
            continue;
        }
        
        // allocate a channel
        ch = &loop_channels[numLoopChannels];
        
        dmaHD_SpatializeReset( ch );
        ch->fixed_origin = true;
        ch->master_vol = ( loop->kill ) ? 127 : 90; // 3D / Sphere
        dmaHD_SpatializeOrigin( loop->origin, ch );
        
        loop->sfx->lastTimeUsed = time;
        
        ch->master_vol = 127;
        
        // Clip volumes.
        ch->l.vol = VOLCLAMP( ch->l.vol );
        ch->r.vol = VOLCLAMP( ch->r.vol );
        ch->l.bassvol = VOLCLAMP( ch->l.bassvol );
        ch->r.bassvol = VOLCLAMP( ch->r.bassvol );
        ch->thesfx = loop->sfx;
        ch->doppler = false;
        
        //#pragma omp critical
        {
            numLoopChannels++;
        }
    }
}

//=============================================================================

/*
============
dmaHD_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void dmaHD_Respatialize( S32 entityNum, const vec3_t head, vec3_t axis[3], S32 inwater )
{
    S32 i;
    channel_t* ch;
    vec3_t origin;
    
    if ( !s_soundStarted || s_soundMuted )
    {
        return;
    }
    
    dmaHD_inwater = inwater;
    
    listener_number = entityNum;
    VectorCopy( head, g_listener_origin );
    VectorCopy( axis[0], g_listener_axis[0] );
    VectorCopy( axis[1], g_listener_axis[1] );
    VectorCopy( axis[2], g_listener_axis[2] );
    
    // update spatialization for dynamic sounds
    //#pragma omp parallel for private(ch)
    for ( i = 0 ; i < MAX_CHANNELS; i++ )
    {
        ch = &s_channels[i];
        if ( !ch->thesfx )
        {
            continue;
        }
        
        dmaHD_SpatializeReset( ch );
        // Anything coming from the view entity will always be full volume
        if ( ch->entnum == listener_number )
        {
            ch->l.vol = ch->master_vol;
            ch->r.vol = ch->master_vol;
            ch->l.bassvol = ch->master_vol;
            ch->r.bassvol = ch->master_vol;
            
            switch ( dmaHD_Mixer->integer )
            {
                case 10:
                case 11:
                case 20:
                case 21:
                    if ( dmaHD_inwater )
                    {
                        ch->l.vol *= ( S32 )0.2f;
                        ch->r.vol *= ( S32 )0.2f;
                    }
                    break;
            }
        }
        else
        {
            if ( ch->fixed_origin )
            {
                VectorCopy( ch->origin, origin );
            }
            else
            {
                VectorCopy( loopSounds[ ch->entnum ].origin, origin );
            }
            
            dmaHD_SpatializeOrigin( origin, ch );
        }
    }
    
    // add loopsounds
    dmaHD_AddLoopSounds();
}

/*
============
dmaHD_Update

Called once each time through the main loop
============
*/
void dmaHD_Update( void )
{
    if ( !s_soundStarted || s_soundMuted )
    {
        return;
    }
    
    // add raw data from streamed samples
    S_UpdateBackgroundTrack();
    
    // mix some sound
    dmaHD_Update_Mix();
}

void dmaHD_Update_Mix( void )
{
    S32 endtime;
    S32 samps;
    static F32 lastTime = 0.0f;
    S32 mixahead, op, thisTime, sane;
    static S32 lastsoundtime = -1;
    
    if ( !s_soundStarted || s_soundMuted ) return;
    
    thisTime = Com_Milliseconds();
    
    // Updates s_soundtime
    S_GetSoundtime();
    
    if ( s_soundtime <= lastsoundtime ) return;
    lastsoundtime = s_soundtime;
    
    // clear any sound effects that end before the current time,
    // and start any new sounds
    S_ScanChannelStarts();
    
    if ( ( sane = thisTime - ( S32 )lastTime ) < 8 )
    {
        // ms since last mix (cap to 8ms @ 125fps)
        sane = 8;
    }
    op = ( S32 )( ( F32 )( dma.speed * sane ) * 0.001 ); // samples to mix based on last mix time
    mixahead = ( S32 )( ( F32 )dma.speed * s_mixahead->value );
    
    if ( mixahead < op )
    {
        mixahead = op;
    }
    
    // mix ahead of current position
    endtime = s_soundtime + mixahead;
    
    // never mix more than the complete buffer
    samps = dma.samples >> ( dma.channels - 1 );
    if ( ( endtime - s_soundtime ) > samps )
    {
        endtime = ( s_soundtime + samps );
    }
    
    SNDDMA_BeginPainting();
    
    dmaHD_PaintChannels( endtime );
    
    SNDDMA_Submit();
    
    lastTime = ( F32 )thisTime;
}

/*
================
dmaHD_Enabled
================
*/
bool dmaHD_Enabled( void )
{
    if ( dmaHD_Enable == nullptr )
        dmaHD_Enable = cvarSystem->Get( "dmaHD_enable", "1", CVAR_ARCHIVE, "description" ); //@p5yc0runn3r- Turn on by default
        
    return ( dmaHD_Enable->integer );
}

// ====================================================================
// User-setable variables
// ====================================================================
void dmaHD_SoundInfo( void )
{
    Com_Printf( "\n" );
    Com_Printf( "dmaHD 3D software sound engine by p5yc0runn3r\n" );
    
    if ( !s_soundStarted )
    {
        Com_Printf( " Engine not started.\n" );
    }
    else
    {
        switch ( dmaHD_Mixer->integer )
        {
            case 10:
                Com_Printf( " dmaHD full 3D sound mixer [10]\n" );
                break;
            case 11:
                Com_Printf( " dmaHD planar 3D sound mixer [11]\n" );
                break;
            case 20:
                Com_Printf( " dmaEX2 sound mixer [20]\n" );
                break;
            case 21:
                Com_Printf( " dmaEX2 sound mixer with no reverb [21]\n" );
                break;
            case 30:
                Com_Printf( " dmaEX sound mixer [30]\n" );
                break;
        }
        Com_Printf( " %d ch / %d Hz / %d bps\n", dma.channels, dma.speed, dma.samplebits );
        if ( s_numSfx > 0 || g_dmaHD_allocatedsoundmemory > 0 )
        {
            Com_Printf( " %d sounds in %.2f MiB\n", s_numSfx, ( F32 )g_dmaHD_allocatedsoundmemory / 1048576.0f );
        }
        else
        {
            Com_Printf( " No sounds loaded yet.\n" );
        }
    }
    Com_Printf( "\n" );
}

void dmaHD_SoundList( void )
{
    S32 i;
    sfx_t* sfx;
    
    Com_Printf( "\n" );
    Com_Printf( "dmaHD HRTF sound engine by p5yc0runn3r\n" );
    
    if ( s_numSfx > 0 || g_dmaHD_allocatedsoundmemory > 0 )
    {
        for ( sfx = s_knownSfx, i = 0; i < s_numSfx; i++, sfx++ )
        {
            Com_Printf( " %s %.2f KiB %s\n", sfx->soundName, ( F32 )sfx->soundLength / 1024.0f, ( sfx->inMemory ? "" : "!" ) );
        }
        Com_Printf( " %d sounds in %.2f MiB\n", s_numSfx, ( F32 )g_dmaHD_allocatedsoundmemory / 1048576.0f );
    }
    else
    {
        Com_Printf( " No sounds loaded yet.\n" );
    }
    Com_Printf( "\n" );
}


/*
================
dmaHD_Init
================
*/
bool dmaHD_Init( soundInterface_t* si )
{
    if ( !si )
    {
        return false;
    }
    
    // Return if not enabled
    if ( !dmaHD_Enabled() )
    {
        return true;
    }
    
    dmaHD_Mixer = cvarSystem->Get( "dmaHD_mixer", "10", CVAR_ARCHIVE, "description" );
    if ( dmaHD_Mixer->integer != 10 &&
            dmaHD_Mixer->integer != 11 &&
            dmaHD_Mixer->integer != 20 &&
            dmaHD_Mixer->integer != 21 &&
            dmaHD_Mixer->integer != 30 )
    {
        cvarSystem->Set( "dmaHD_Mixer", "10" );
        dmaHD_Mixer = cvarSystem->Get( "dmaHD_mixer", "10", CVAR_ARCHIVE, "description" );
    }
    
    dmaEX_StereoSeparation = cvarSystem->Get( "dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE, "description" );
    if ( dmaEX_StereoSeparation->value < 0.1 )
    {
        cvarSystem->Set( "dmaEX_StereoSeparation", "0.1" );
        dmaEX_StereoSeparation = cvarSystem->Get( "dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE, "description" );
    }
    else if ( dmaEX_StereoSeparation->value > 2.0 )
    {
        cvarSystem->Set( "dmaEX_StereoSeparation", "2.0" );
        dmaEX_StereoSeparation = cvarSystem->Get( "dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE, "description" );
    }
    
    dmaHD_Interpolation = cvarSystem->Get( "dmaHD_interpolation", "3", CVAR_ARCHIVE, "description" );
    if ( dmaHD_Interpolation->integer == 0 )
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetNoInterpolationSample;
    }
    else if ( dmaHD_Interpolation->integer == 1 )
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleLinear;
    }
    else if ( dmaHD_Interpolation->integer == 2 )
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleCubic;
    }
    else //if (dmaHD_Interpolation->integer == 3) // DEFAULT
    {
        dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleHermite4pt3oX;
    }
    
    dmaHD_InitTables();
    
    // Override function pointers to dmaHD version, the rest keep base.
    si->SoundInfo = dmaHD_SoundInfo;
    si->Respatialize = dmaHD_Respatialize;
    si->Update = dmaHD_Update;
    si->SoundList = dmaHD_SoundList;
    
    return true;
}

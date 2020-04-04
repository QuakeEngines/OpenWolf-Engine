////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2007 - 2008 Joerg Dietrich
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
// File name:   r_image_png.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemImagePNGLocal renderSystemImagePNGLocal;

/*
===============
idRenderSystemImagePNGLocal::idRenderSystemImagePNGLocal
===============
*/
idRenderSystemImagePNGLocal::idRenderSystemImagePNGLocal( void )
{
}

/*
===============
idRenderSystemImagePNGLocal::~idRenderSystemImagePNGLocal
===============
*/
idRenderSystemImagePNGLocal::~idRenderSystemImagePNGLocal( void )
{
}

struct BufferedFile* idRenderSystemImagePNGLocal::ReadBufferedFile( StringEntry name )
{
    struct BufferedFile* BF;
    union
    {
        U8* b;
        void* v;
    } buffer;
    
    /*
     *  input verification
     */
    
    if ( !name )
    {
        return( NULL );
    }
    
    /*
     *  Allocate control struct.
     */
    
    BF = ( struct BufferedFile* )memorySystem->Malloc( sizeof( struct BufferedFile ) );
    if ( !BF )
    {
        return( NULL );
    }
    
    /*
     *  Initialize the structs components.
     */
    
    BF->Length    = 0;
    BF->Buffer    = NULL;
    BF->Ptr       = NULL;
    BF->BytesLeft = 0;
    
    /*
     *  Read the file.
     */
    
    BF->Length = fileSystem->ReadFile( const_cast< UTF8* >( name ), &buffer.v );
    BF->Buffer = buffer.b;
    
    /*
     *  Did we get it? Is it big enough?
     */
    
    if ( !( BF->Buffer && ( BF->Length > 0 ) ) )
    {
        memorySystem->Free( BF );
        
        return( NULL );
    }
    
    /*
     *  Set the pointers and counters.
     */
    
    BF->Ptr       = BF->Buffer;
    BF->BytesLeft = BF->Length;
    
    return( BF );
}

/*
 *  Close a buffered file.
 */

void idRenderSystemImagePNGLocal::CloseBufferedFile( struct BufferedFile* BF )
{
    if ( BF )
    {
        if ( BF->Buffer )
        {
            fileSystem->FreeFile( BF->Buffer );
        }
        
        memorySystem->Free( BF );
    }
}

/*
 *  Get a pointer to the requested bytes.
 */

void* idRenderSystemImagePNGLocal::BufferedFileRead( struct BufferedFile* BF, U32 Length )
{
    void* RetVal;
    
    /*
     *  input verification
     */
    
    if ( !( BF && Length ) )
    {
        return( NULL );
    }
    
    /*
     *  not enough bytes left
     */
    
    if ( ( S32 )Length > BF->BytesLeft )
    {
        return( NULL );
    }
    
    /*
     *  the pointer to the requested data
     */
    
    RetVal = BF->Ptr;
    
    /*
     *  Raise the pointer and counter.
     */
    
    BF->Ptr       += Length;
    BF->BytesLeft -= Length;
    
    return( RetVal );
}

/*
 *  Rewind the buffer.
 */

bool idRenderSystemImagePNGLocal::BufferedFileRewind( struct BufferedFile* BF, U32 Offset )
{
    U32 BytesRead;
    
    /*
     *  input verification
     */
    
    if ( !BF )
    {
        return( false );
    }
    
    /*
     *  special trick to rewind to the beginning of the buffer
     */
    
    if ( Offset == ( U32 ) - 1 )
    {
        BF->Ptr       = BF->Buffer;
        BF->BytesLeft = BF->Length;
        
        return( true );
    }
    
    /*
     *  How many bytes do we have already read?
     */
    
    BytesRead = ( U32 )( BF->Ptr - BF->Buffer );
    
    /*
     *  We can only rewind to the beginning of the BufferedFile.
     */
    
    if ( Offset > BytesRead )
    {
        return( false );
    }
    
    /*
     *  lower the pointer and counter.
     */
    
    BF->Ptr       -= Offset;
    BF->BytesLeft += Offset;
    
    return( true );
}

/*
 *  Skip some bytes.
 */

bool idRenderSystemImagePNGLocal::BufferedFileSkip( struct BufferedFile* BF, U32 Offset )
{
    /*
     *  input verification
     */
    
    if ( !BF )
    {
        return( false );
    }
    
    /*
     *  We can only skip to the end of the BufferedFile.
     */
    
    if ( ( S32 )Offset > BF->BytesLeft )
    {
        return( false );
    }
    
    /*
     *  lower the pointer and counter.
     */
    
    BF->Ptr       += Offset;
    BF->BytesLeft -= Offset;
    
    return( true );
}

/*
 *  Find a chunk
 */

bool idRenderSystemImagePNGLocal::FindChunk( struct BufferedFile* BF, U32 ChunkType )
{
    struct PNG_ChunkHeader* CH;
    
    U32 Length;
    U32 Type;
    
    /*
     *  input verification
     */
    
    if ( !BF )
    {
        return( false );
    }
    
    /*
     *  cycle trough the chunks
     */
    
    while ( true )
    {
        /*
         *  Read the chunk-header.
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( BF, PNG_ChunkHeader_Size );
        if ( !CH )
        {
            return( false );
        }
        
        /*
         *  Do not swap the original types
         *  they might be needed later.
         */
        
        Length = BigLong( CH->Length );
        Type   = BigLong( CH->Type );
        
        /*
         *  We found it!
         */
        
        if ( Type == ChunkType )
        {
            /*
             *  Rewind to the start of the chunk.
             */
            
            BufferedFileRewind( BF, PNG_ChunkHeader_Size );
            
            break;
        }
        else
        {
            /*
             *  Skip the rest of the chunk.
             */
            
            if ( Length )
            {
                if ( !BufferedFileSkip( BF, Length + PNG_ChunkCRC_Size ) )
                {
                    return( false );
                }
            }
        }
    }
    
    return( true );
}

/*
 *  Decompress all IDATs
 */

U32 idRenderSystemImagePNGLocal::DecompressIDATs( struct BufferedFile* BF, U8** Buffer )
{
    U8*  DecompressedData;
    U32  DecompressedDataLength;
    
    U8*  CompressedData;
    U8*  CompressedDataPtr;
    U32  CompressedDataLength;
    
    struct PNG_ChunkHeader* CH;
    
    U32 Length;
    U32 Type;
    
    S32 BytesToRewind;
    
    S32   puffResult;
    U8*  puffDest;
    U32  puffDestLen;
    U8*  puffSrc;
    U32  puffSrcLen;
    
    /*
     *  input verification
     */
    
    if ( !( BF && Buffer ) )
    {
        return( -1 );
    }
    
    /*
     *  some zeroing
     */
    
    DecompressedData = NULL;
    *Buffer = DecompressedData;
    
    CompressedData = NULL;
    CompressedDataLength = 0;
    
    BytesToRewind = 0;
    
    /*
     *  Find the first IDAT chunk.
     */
    
    if ( !FindChunk( BF, PNG_ChunkType_IDAT ) )
    {
        return( -1 );
    }
    
    /*
     *  Count the size of the uncompressed data
     */
    
    while ( true )
    {
        /*
         *  Read chunk header
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( BF, PNG_ChunkHeader_Size );
        if ( !CH )
        {
            /*
             *  Rewind to the start of this adventure
             *  and return unsuccessfull
             */
            
            BufferedFileRewind( BF, BytesToRewind );
            
            return( -1 );
        }
        
        /*
         *  Length and Type of chunk
         */
        
        Length = BigLong( CH->Length );
        Type   = BigLong( CH->Type );
        
        /*
         *  We have reached the end of the IDAT chunks
         */
        
        if ( !( Type == PNG_ChunkType_IDAT ) )
        {
            BufferedFileRewind( BF, PNG_ChunkHeader_Size );
            
            break;
        }
        
        /*
         *  Add chunk header to count.
         */
        
        BytesToRewind += PNG_ChunkHeader_Size;
        
        /*
         *  Skip to next chunk
         */
        
        if ( Length )
        {
            if ( !BufferedFileSkip( BF, Length + PNG_ChunkCRC_Size ) )
            {
                BufferedFileRewind( BF, BytesToRewind );
                
                return( -1 );
            }
            
            BytesToRewind += Length + PNG_ChunkCRC_Size;
            CompressedDataLength += Length;
        }
    }
    
    BufferedFileRewind( BF, BytesToRewind );
    
    CompressedData = ( U8* )memorySystem->Malloc( CompressedDataLength );
    if ( !CompressedData )
    {
        return( -1 );
    }
    
    CompressedDataPtr = CompressedData;
    
    /*
     *  Collect the compressed Data
     */
    
    while ( true )
    {
        /*
         *  Read chunk header
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( BF, PNG_ChunkHeader_Size );
        if ( !CH )
        {
            memorySystem->Free( CompressedData );
            
            return( -1 );
        }
        
        /*
         *  Length and Type of chunk
         */
        
        Length = BigLong( CH->Length );
        Type   = BigLong( CH->Type );
        
        /*
         *  We have reached the end of the IDAT chunks
         */
        
        if ( !( Type == PNG_ChunkType_IDAT ) )
        {
            BufferedFileRewind( BF, PNG_ChunkHeader_Size );
            
            break;
        }
        
        /*
         *  Copy the Data
         */
        
        if ( Length )
        {
            U8* OrigCompressedData;
            
            OrigCompressedData = ( U8* )BufferedFileRead( BF, Length );
            if ( !OrigCompressedData )
            {
                memorySystem->Free( CompressedData );
                
                return( -1 );
            }
            
            if ( !BufferedFileSkip( BF, PNG_ChunkCRC_Size ) )
            {
                memorySystem->Free( CompressedData );
                
                return( -1 );
            }
            
            memcpy( CompressedDataPtr, OrigCompressedData, Length );
            CompressedDataPtr += Length;
        }
    }
    
    /*
     *  Let puff() calculate the decompressed data length.
     */
    
    puffDest    = NULL;
    puffDestLen = 0;
    
    /*
     *  The zlib header and checkvalue don't belong to the compressed data.
     */
    
    puffSrc    = CompressedData + PNG_ZlibHeader_Size;
    puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;
    
    /*
     *  first puff() to calculate the size of the uncompressed data
     */
    
    puffResult = puff( puffDest, &puffDestLen, puffSrc, &puffSrcLen );
    if ( !( ( puffResult == 0 ) && ( puffDestLen > 0 ) ) )
    {
        memorySystem->Free( CompressedData );
        
        return( -1 );
    }
    
    /*
     *  Allocate the buffer for the uncompressed data.
     */
    
    DecompressedData = ( U8* )memorySystem->Malloc( puffDestLen );
    if ( !DecompressedData )
    {
        memorySystem->Free( CompressedData );
        
        return( -1 );
    }
    
    /*
     *  Set the input again in case something was changed by the last puff() .
     */
    
    puffDest   = DecompressedData;
    puffSrc    = CompressedData + PNG_ZlibHeader_Size;
    puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;
    
    /*
     *  decompression puff()
     */
    
    puffResult = puff( puffDest, &puffDestLen, puffSrc, &puffSrcLen );
    
    /*
     *  The compressed data is not needed anymore.
     */
    
    memorySystem->Free( CompressedData );
    
    /*
     *  Check if the last puff() was successfull.
     */
    
    if ( !( ( puffResult == 0 ) && ( puffDestLen > 0 ) ) )
    {
        memorySystem->Free( DecompressedData );
        
        return( -1 );
    }
    
    /*
     *  Set the output of this function.
     */
    
    DecompressedDataLength = puffDestLen;
    *Buffer = DecompressedData;
    
    return( DecompressedDataLength );
}

/*
 *  the Paeth predictor
 */

U8 idRenderSystemImagePNGLocal::PredictPaeth( U8 a, U8 b, U8 c )
{
    /*
     *  a == Left
     *  b == Up
     *  c == UpLeft
     */
    
    U8 Pr;
    S32 p;
    S32 pa, pb, pc;
    
    p  = ( ( S32 ) a ) + ( ( S32 ) b ) - ( ( S32 ) c );
    pa = abs( p - ( ( S32 ) a ) );
    pb = abs( p - ( ( S32 ) b ) );
    pc = abs( p - ( ( S32 ) c ) );
    
    if ( ( pa <= pb ) && ( pa <= pc ) )
    {
        Pr = a;
    }
    else if ( pb <= pc )
    {
        Pr = b;
    }
    else
    {
        Pr = c;
    }
    
    return( Pr );
    
}

/*
 *  Reverse the filters.
 */

bool idRenderSystemImagePNGLocal::UnfilterImage( U8*  DecompressedData, U32  ImageHeight, U32  BytesPerScanline, U32  BytesPerPixel )
{
    U8*   DecompPtr;
    U8   FilterType;
    U8*  PixelLeft, *PixelUp, *PixelUpLeft;
    U32  w, h, p;
    
    /*
     *  some zeros for the filters
     */
    
    U8 Zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    /*
     *  input verification
     */
    
    if ( !( DecompressedData && BytesPerPixel ) )
    {
        return( false );
    }
    
    /*
     *  ImageHeight and BytesPerScanline can be zero in small interlaced images.
     */
    
    if ( ( !ImageHeight ) || ( !BytesPerScanline ) )
    {
        return( true );
    }
    
    /*
     *  Set the pointer to the start of the decompressed Data.
     */
    
    DecompPtr = DecompressedData;
    
    /*
     *  Un-filtering is done in place.
     */
    
    /*
     *  Go trough all scanlines.
     */
    
    for ( h = 0; h < ImageHeight; h++ )
    {
        /*
         *  Every scanline starts with a FilterType byte.
         */
        
        FilterType = *DecompPtr;
        DecompPtr++;
        
        /*
         *  Left pixel of the first byte in a scanline is zero.
         */
        
        PixelLeft = Zeros;
        
        /*
         *  Set PixelUp to previous line only if we are on the second line or above.
         *
         *  Plus one byte for the FilterType
         */
        
        if ( h > 0 )
        {
            PixelUp = DecompPtr - ( BytesPerScanline + 1 );
        }
        else
        {
            PixelUp = Zeros;
        }
        
        /*
         * The pixel left to the first pixel of the previous scanline is zero too.
         */
        
        PixelUpLeft = Zeros;
        
        /*
         *  Cycle trough all pixels of the scanline.
         */
        
        for ( w = 0; w < ( BytesPerScanline / BytesPerPixel ); w++ )
        {
            /*
             *  Cycle trough the bytes of the pixel.
             */
            
            for ( p = 0; p < BytesPerPixel; p++ )
            {
                switch ( FilterType )
                {
                    case PNG_FilterType_None :
                    {
                        /*
                         *  The byte is unfiltered.
                         */
                        
                        break;
                    }
                    
                    case PNG_FilterType_Sub :
                    {
                        DecompPtr[p] += PixelLeft[p];
                        
                        break;
                    }
                    
                    case PNG_FilterType_Up :
                    {
                        DecompPtr[p] += PixelUp[p];
                        
                        break;
                    }
                    
                    case PNG_FilterType_Average :
                    {
                        DecompPtr[p] += ( ( U8 )( ( ( ( U16 ) PixelLeft[p] ) + ( ( U16 ) PixelUp[p] ) ) / 2 ) );
                        
                        break;
                    }
                    
                    case PNG_FilterType_Paeth :
                    {
                        DecompPtr[p] += PredictPaeth( PixelLeft[p], PixelUp[p], PixelUpLeft[p] );
                        
                        break;
                    }
                    
                    default :
                    {
                        return( false );
                    }
                }
            }
            
            PixelLeft = DecompPtr;
            
            /*
             *  We only have an upleft pixel if we are on the second line or above.
             */
            
            if ( h > 0 )
            {
                PixelUpLeft = DecompPtr - ( BytesPerScanline + 1 );
            }
            
            /*
             *  Skip to the next pixel.
             */
            
            DecompPtr += BytesPerPixel;
            
            /*
             *  We only have a previous line if we are on the second line and above.
             */
            
            if ( h > 0 )
            {
                PixelUp = DecompPtr - ( BytesPerScanline + 1 );
            }
        }
    }
    
    return( true );
}

/*
 *  Convert a raw input pixel to Quake 3 RGA format.
 */
bool idRenderSystemImagePNGLocal::ConvertPixel( struct PNG_Chunk_IHDR* IHDR, U8* OutPtr, U8* DecompPtr, bool HasTransparentColour, U8* TransparentColour, U8* OutPal )
{
    /*
     *  input verification
     */
    
    if ( !( IHDR && OutPtr && DecompPtr && TransparentColour && OutPal ) )
    {
        return( false );
    }
    
    switch ( IHDR->ColourType )
    {
        case PNG_ColourType_Grey :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    U8 Step;
                    U8 GreyValue;
                    
                    Step = 0xFF / ( ( 1 << IHDR->BitDepth ) - 1 );
                    
                    GreyValue = DecompPtr[0] * Step;
                    
                    OutPtr[0] = GreyValue;
                    OutPtr[1] = GreyValue;
                    OutPtr[2] = GreyValue;
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  Grey supports full transparency for one specified colour
                     */
                    
                    if ( HasTransparentColour )
                    {
                        if ( TransparentColour[1] == DecompPtr[0] )
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
                    
                    
                    break;
                }
                
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  Grey supports full transparency for one specified colour
                     */
                    
                    if ( HasTransparentColour )
                    {
                        if ( IHDR->BitDepth == PNG_BitDepth_8 )
                        {
                            if ( TransparentColour[1] == DecompPtr[0] )
                            {
                                OutPtr[3] = 0x00;
                            }
                        }
                        else
                        {
                            if ( ( TransparentColour[0] == DecompPtr[0] ) && ( TransparentColour[1] == DecompPtr[1] ) )
                            {
                                OutPtr[3] = 0x00;
                            }
                        }
                    }
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_True :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[1];
                    OutPtr[2] = DecompPtr[2];
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  True supports full transparency for one specified colour
                     */
                    
                    if ( HasTransparentColour )
                    {
                        if ( ( TransparentColour[1] == DecompPtr[0] ) &&
                                ( TransparentColour[3] == DecompPtr[1] ) &&
                                ( TransparentColour[5] == DecompPtr[2] ) )
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
                    
                    break;
                }
                
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */
                    
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[2];
                    OutPtr[2] = DecompPtr[4];
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  True supports full transparency for one specified colour
                     */
                    
                    if ( HasTransparentColour )
                    {
                        if ( ( TransparentColour[0] == DecompPtr[0] ) && ( TransparentColour[1] == DecompPtr[1] ) &&
                                ( TransparentColour[2] == DecompPtr[2] ) && ( TransparentColour[3] == DecompPtr[3] ) &&
                                ( TransparentColour[4] == DecompPtr[4] ) && ( TransparentColour[5] == DecompPtr[5] ) )
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_Indexed :
        {
            OutPtr[0] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 0];
            OutPtr[1] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 1];
            OutPtr[2] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 2];
            OutPtr[3] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 3];
            
            break;
        }
        
        case PNG_ColourType_GreyAlpha :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = DecompPtr[1];
                    
                    break;
                }
                
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */
                    
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = DecompPtr[2];
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_TrueAlpha :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[1];
                    OutPtr[2] = DecompPtr[2];
                    OutPtr[3] = DecompPtr[3];
                    
                    break;
                }
                
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */
                    
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[2];
                    OutPtr[2] = DecompPtr[4];
                    OutPtr[3] = DecompPtr[6];
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        default :
        {
            return( false );
        }
    }
    
    return( true );
}


/*
 *  Decode a non-interlaced image.
 */

bool idRenderSystemImagePNGLocal::DecodeImageNonInterlaced( struct PNG_Chunk_IHDR* IHDR, U8* OutBuffer, U8* DecompressedData,
        U32 DecompressedDataLength, bool HasTransparentColour, U8* TransparentColour, U8* OutPal )
{
    U32 IHDR_Width;
    U32 IHDR_Height;
    U32 BytesPerScanline, BytesPerPixel, PixelsPerByte;
    U32  w, h, p;
    U8* OutPtr;
    U8* DecompPtr;
    
    /*
     *  input verification
     */
    
    if ( !( IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal ) )
    {
        return( false );
    }
    
    /*
     *  byte swapping
     */
    
    IHDR_Width  = BigLong( IHDR->Width );
    IHDR_Height = BigLong( IHDR->Height );
    
    /*
     *  information for un-filtering
     */
    
    switch ( IHDR->ColourType )
    {
        case PNG_ColourType_Grey :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_Grey;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_True :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_True;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_Indexed :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8 :
                {
                    BytesPerPixel    = PNG_NumColourComponents_Indexed;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_GreyAlpha :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_GreyAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_TrueAlpha :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_TrueAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        default :
        {
            return( false );
        }
    }
    
    /*
     *  Calculate the size of one scanline
     */
    
    BytesPerScanline = ( IHDR_Width * BytesPerPixel + ( PixelsPerByte - 1 ) ) / PixelsPerByte;
    
    /*
     *  Check if we have enough data for the whole image.
     */
    
    if ( !( DecompressedDataLength == ( ( BytesPerScanline + 1 ) * IHDR_Height ) ) )
    {
        return( false );
    }
    
    /*
     *  Unfilter the image.
     */
    
    if ( !UnfilterImage( DecompressedData, IHDR_Height, BytesPerScanline, BytesPerPixel ) )
    {
        return( false );
    }
    
    /*
     *  Set the working pointers to the beginning of the buffers.
     */
    
    OutPtr = OutBuffer;
    DecompPtr = DecompressedData;
    
    /*
     *  Create the output image.
     */
    
    for ( h = 0; h < IHDR_Height; h++ )
    {
        /*
         *  Count the pixels on the scanline for those multipixel bytes
         */
        
        U32 CurrPixel;
        
        /*
         *  skip FilterType
         */
        
        DecompPtr++;
        
        /*
         *  Reset the pixel count.
         */
        
        CurrPixel = 0;
        
        for ( w = 0; w < ( BytesPerScanline / BytesPerPixel ); w++ )
        {
            if ( PixelsPerByte > 1 )
            {
                U8  Mask;
                U32 Shift;
                U8  SinglePixel;
                
                for ( p = 0; p < PixelsPerByte; p++ )
                {
                    if ( CurrPixel < IHDR_Width )
                    {
                        Mask  = ( 1 << IHDR->BitDepth ) - 1;
                        Shift = ( PixelsPerByte - 1 - p ) * IHDR->BitDepth;
                        
                        SinglePixel = ( ( DecompPtr[0] & ( Mask << Shift ) ) >> Shift );
                        
                        if ( !ConvertPixel( IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal ) )
                        {
                            return( false );
                        }
                        
                        OutPtr += Q3IMAGE_BYTESPERPIXEL;
                        CurrPixel++;
                    }
                }
                
            }
            else
            {
                if ( !ConvertPixel( IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal ) )
                {
                    return( false );
                }
                
                
                OutPtr += Q3IMAGE_BYTESPERPIXEL;
            }
            
            DecompPtr += BytesPerPixel;
        }
    }
    
    return( true );
}

/*
 *  Decode an interlaced image.
 */

bool idRenderSystemImagePNGLocal::DecodeImageInterlaced( struct PNG_Chunk_IHDR* IHDR, U8* OutBuffer, U8* DecompressedData, U32 DecompressedDataLength,
        bool HasTransparentColour, U8* TransparentColour, U8* OutPal )
{
    U32 IHDR_Width;
    U32 IHDR_Height;
    U32 BytesPerScanline[PNG_Adam7_NumPasses], BytesPerPixel, PixelsPerByte;
    U32 PassWidth[PNG_Adam7_NumPasses], PassHeight[PNG_Adam7_NumPasses];
    U32 WSkip[PNG_Adam7_NumPasses], WOffset[PNG_Adam7_NumPasses], HSkip[PNG_Adam7_NumPasses], HOffset[PNG_Adam7_NumPasses];
    U32 w, h, p, a;
    U8* OutPtr;
    U8* DecompPtr;
    U32 TargetLength;
    
    /*
     *  input verification
     */
    
    if ( !( IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal ) )
    {
        return( false );
    }
    
    /*
     *  byte swapping
     */
    
    IHDR_Width  = BigLong( IHDR->Width );
    IHDR_Height = BigLong( IHDR->Height );
    
    /*
     *  Skip and Offset for the passes.
     */
    
    WSkip[0]   = 8;
    WOffset[0] = 0;
    HSkip[0]   = 8;
    HOffset[0] = 0;
    
    WSkip[1]   = 8;
    WOffset[1] = 4;
    HSkip[1]   = 8;
    HOffset[1] = 0;
    
    WSkip[2]   = 4;
    WOffset[2] = 0;
    HSkip[2]   = 8;
    HOffset[2] = 4;
    
    WSkip[3]   = 4;
    WOffset[3] = 2;
    HSkip[3]   = 4;
    HOffset[3] = 0;
    
    WSkip[4]   = 2;
    WOffset[4] = 0;
    HSkip[4]   = 4;
    HOffset[4] = 2;
    
    WSkip[5]   = 2;
    WOffset[5] = 1;
    HSkip[5]   = 2;
    HOffset[5] = 0;
    
    WSkip[6]   = 1;
    WOffset[6] = 0;
    HSkip[6]   = 2;
    HOffset[6] = 1;
    
    /*
     *  Calculate the sizes of the passes.
     */
    
    PassWidth[0]  = ( IHDR_Width  + 7 ) / 8;
    PassHeight[0] = ( IHDR_Height + 7 ) / 8;
    
    PassWidth[1]  = ( IHDR_Width  + 3 ) / 8;
    PassHeight[1] = ( IHDR_Height + 7 ) / 8;
    
    PassWidth[2]  = ( IHDR_Width  + 3 ) / 4;
    PassHeight[2] = ( IHDR_Height + 3 ) / 8;
    
    PassWidth[3]  = ( IHDR_Width  + 1 ) / 4;
    PassHeight[3] = ( IHDR_Height + 3 ) / 4;
    
    PassWidth[4]  = ( IHDR_Width  + 1 ) / 2;
    PassHeight[4] = ( IHDR_Height + 1 ) / 4;
    
    PassWidth[5]  = ( IHDR_Width  + 0 ) / 2;
    PassHeight[5] = ( IHDR_Height + 1 ) / 2;
    
    PassWidth[6]  = ( IHDR_Width  + 0 ) / 1;
    PassHeight[6] = ( IHDR_Height + 0 ) / 2;
    
    /*
     *  information for un-filtering
     */
    
    switch ( IHDR->ColourType )
    {
        case PNG_ColourType_Grey :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_Grey;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_True :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_True;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_Indexed :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8 :
                {
                    BytesPerPixel    = PNG_NumColourComponents_Indexed;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_GreyAlpha :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_GreyAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_TrueAlpha :
        {
            switch ( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_TrueAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        default :
        {
            return( false );
        }
    }
    
    /*
     *  Calculate the size of the scanlines per pass
     */
    
    for ( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        BytesPerScanline[a] = ( PassWidth[a] * BytesPerPixel + ( PixelsPerByte - 1 ) ) / PixelsPerByte;
    }
    
    /*
     *  Calculate the size of all passes
     */
    
    TargetLength = 0;
    
    for ( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        TargetLength += ( ( BytesPerScanline[a] + ( BytesPerScanline[a] ? 1 : 0 ) ) * PassHeight[a] );
    }
    
    /*
     *  Check if we have enough data for the whole image.
     */
    
    if ( !( DecompressedDataLength == TargetLength ) )
    {
        return( false );
    }
    
    /*
     *  Unfilter the image.
     */
    
    DecompPtr = DecompressedData;
    
    for ( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        if ( !UnfilterImage( DecompPtr, PassHeight[a], BytesPerScanline[a], BytesPerPixel ) )
        {
            return( false );
        }
        
        DecompPtr += ( ( BytesPerScanline[a] + ( BytesPerScanline[a] ? 1 : 0 ) ) * PassHeight[a] );
    }
    
    /*
     *  Set the working pointers to the beginning of the buffers.
     */
    
    DecompPtr = DecompressedData;
    
    /*
     *  Create the output image.
     */
    
    for ( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        for ( h = 0; h < PassHeight[a]; h++ )
        {
            /*
             *  Count the pixels on the scanline for those multipixel bytes
             */
            
            U32 CurrPixel;
            
            /*
             *  skip FilterType
             *  but only when the pass has a width bigger than zero
             */
            
            if ( BytesPerScanline[a] )
            {
                DecompPtr++;
            }
            
            /*
             *  Reset the pixel count.
             */
            
            CurrPixel = 0;
            
            for ( w = 0; w < ( BytesPerScanline[a] / BytesPerPixel ); w++ )
            {
                if ( PixelsPerByte > 1 )
                {
                    U8  Mask;
                    U32 Shift;
                    U8  SinglePixel;
                    
                    for ( p = 0; p < PixelsPerByte; p++ )
                    {
                        if ( CurrPixel < PassWidth[a] )
                        {
                            Mask  = ( 1 << IHDR->BitDepth ) - 1;
                            Shift = ( PixelsPerByte - 1 - p ) * IHDR->BitDepth;
                            
                            SinglePixel = ( ( DecompPtr[0] & ( Mask << Shift ) ) >> Shift );
                            
                            OutPtr = OutBuffer + ( ( ( ( ( h * HSkip[a] ) + HOffset[a] ) * IHDR_Width ) + ( ( CurrPixel * WSkip[a] ) + WOffset[a] ) ) * Q3IMAGE_BYTESPERPIXEL );
                            
                            if ( !ConvertPixel( IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal ) )
                            {
                                return( false );
                            }
                            
                            CurrPixel++;
                        }
                    }
                    
                }
                else
                {
                    OutPtr = OutBuffer + ( ( ( ( ( h * HSkip[a] ) + HOffset[a] ) * IHDR_Width ) + ( ( w * WSkip[a] ) + WOffset[a] ) ) * Q3IMAGE_BYTESPERPIXEL );
                    
                    if ( !ConvertPixel( IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal ) )
                    {
                        return( false );
                    }
                }
                
                DecompPtr += BytesPerPixel;
            }
        }
    }
    
    return( true );
}

/*
 *  The PNG loader
 */

void idRenderSystemImagePNGLocal::LoadPNG( StringEntry name, U8** pic, S32* width, S32* height )
{
    struct BufferedFile* ThePNG;
    U8* OutBuffer;
    U8* Signature;
    struct PNG_ChunkHeader* CH;
    U32 ChunkHeaderLength;
    U32 ChunkHeaderType;
    struct PNG_Chunk_IHDR* IHDR;
    U32 IHDR_Width;
    U32 IHDR_Height;
    PNG_ChunkCRC* CRC;
    U8* InPal;
    U8* DecompressedData;
    U32 DecompressedDataLength;
    U32 i;
    
    /*
     *  palette with 256 RGBA entries
     */
    
    U8 OutPal[1024];
    
    /*
     *  transparent colour from the tRNS chunk
     */
    
    bool HasTransparentColour = false;
    U8 TransparentColour[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    /*
     *  input verification
     */
    
    if ( !( name && pic ) )
    {
        return;
    }
    
    /*
     *  Zero out return values.
     */
    
    *pic = NULL;
    
    if ( width )
    {
        *width = 0;
    }
    
    if ( height )
    {
        *height = 0;
    }
    
    /*
     *  Read the file.
     */
    
    ThePNG = ReadBufferedFile( name );
    if ( !ThePNG )
    {
        return;
    }
    
    /*
     *  Read the siganture of the file.
     */
    
    Signature = ( U8* )BufferedFileRead( ThePNG, PNG_Signature_Size );
    if ( !Signature )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Is it a PNG?
     */
    
    if ( memcmp( Signature, PNG_Signature, PNG_Signature_Size ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read the first chunk-header.
     */
    
    CH = ( struct PNG_ChunkHeader* )BufferedFileRead( ThePNG, PNG_ChunkHeader_Size );
    if ( !CH )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  PNG multi-byte types are in Big Endian
     */
    
    ChunkHeaderLength = BigLong( CH->Length );
    ChunkHeaderType   = BigLong( CH->Type );
    
    /*
     *  Check if the first chunk is an IHDR.
     */
    
    if ( !( ( ChunkHeaderType == PNG_ChunkType_IHDR ) && ( ChunkHeaderLength == PNG_Chunk_IHDR_Size ) ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read the IHDR.
     */
    
    IHDR = ( struct PNG_Chunk_IHDR* )BufferedFileRead( ThePNG, PNG_Chunk_IHDR_Size );
    if ( !IHDR )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read the CRC for IHDR
     */
    
    CRC = ( PNG_ChunkCRC* )BufferedFileRead( ThePNG, PNG_ChunkCRC_Size );
    if ( !CRC )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Here we could check the CRC if we wanted to.
     */
    
    /*
     *  multi-byte type swapping
     */
    
    IHDR_Width  = BigLong( IHDR->Width );
    IHDR_Height = BigLong( IHDR->Height );
    
    /*
     *  Check if Width and Height are valid.
     */
    
    if ( !( ( IHDR_Width > 0 ) && ( IHDR_Height > 0 ) )
            || IHDR_Width > INT_MAX / Q3IMAGE_BYTESPERPIXEL / IHDR_Height )
    {
        CloseBufferedFile( ThePNG );
        
        clientMainSystem->RefPrintf( PRINT_WARNING, "%s: invalid image size\n", name );
        
        return;
    }
    
    /*
     *  Do we need to check if the dimensions of the image are valid for Quake3?
     */
    
    /*
     *  Check if CompressionMethod and FilterMethod are valid.
     */
    
    if ( !( ( IHDR->CompressionMethod == PNG_CompressionMethod_0 ) && ( IHDR->FilterMethod == PNG_FilterMethod_0 ) ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Check if InterlaceMethod is valid.
     */
    
    if ( !( ( IHDR->InterlaceMethod == PNG_InterlaceMethod_NonInterlaced )  || ( IHDR->InterlaceMethod == PNG_InterlaceMethod_Interlaced ) ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read palette for an indexed image.
     */
    
    if ( IHDR->ColourType == PNG_ColourType_Indexed )
    {
        /*
         *  We need the palette first.
         */
        
        if ( !FindChunk( ThePNG, PNG_ChunkType_PLTE ) )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the chunk-header.
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( ThePNG, PNG_ChunkHeader_Size );
        if ( !CH )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  PNG multi-byte types are in Big Endian
         */
        
        ChunkHeaderLength = BigLong( CH->Length );
        ChunkHeaderType   = BigLong( CH->Type );
        
        /*
         *  Check if the chunk is a PLTE.
         */
        
        if ( !( ChunkHeaderType == PNG_ChunkType_PLTE ) )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Check if Length is divisible by 3
         */
        
        if ( ChunkHeaderLength % 3 )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the raw palette data
         */
        
        InPal = ( U8* )BufferedFileRead( ThePNG, ChunkHeaderLength );
        if ( !InPal )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the CRC for the palette
         */
        
        CRC = ( PNG_ChunkCRC* )BufferedFileRead( ThePNG, PNG_ChunkCRC_Size );
        if ( !CRC )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Set some default values.
         */
        
        for ( i = 0; i < 256; i++ )
        {
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;
        }
        
        /*
         *  Convert to the Quake3 RGBA-format.
         */
        
        for ( i = 0; i < ( ChunkHeaderLength / 3 ); i++ )
        {
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = InPal[i * 3 + 0];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = InPal[i * 3 + 1];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = InPal[i * 3 + 2];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;
        }
    }
    
    /*
     *  transparency information is sometimes stored in a tRNS chunk
     */
    
    /*
     *  Let's see if there is a tRNS chunk
     */
    
    if ( FindChunk( ThePNG, PNG_ChunkType_tRNS ) )
    {
        U8* Trans;
        
        /*
         *  Read the chunk-header.
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( ThePNG, PNG_ChunkHeader_Size );
        if ( !CH )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  PNG multi-byte types are in Big Endian
         */
        
        ChunkHeaderLength = BigLong( CH->Length );
        ChunkHeaderType   = BigLong( CH->Type );
        
        /*
         *  Check if the chunk is a tRNS.
         */
        
        if ( !( ChunkHeaderType == PNG_ChunkType_tRNS ) )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the transparency information.
         */
        
        Trans = ( U8* )BufferedFileRead( ThePNG, ChunkHeaderLength );
        if ( !Trans )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the CRC.
         */
        
        CRC = ( PNG_ChunkCRC* )BufferedFileRead( ThePNG, PNG_ChunkCRC_Size );
        if ( !CRC )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Only for Grey, True and Indexed ColourType should tRNS exist.
         */
        
        switch ( IHDR->ColourType )
        {
            case PNG_ColourType_Grey :
            {
                if ( ChunkHeaderLength != 2 )
                {
                    CloseBufferedFile( ThePNG );
                    
                    return;
                }
                
                HasTransparentColour = true;
                
                /*
                 *  Grey can have one colour which is completely transparent.
                 *  This colour is always stored in 16 bits.
                 */
                
                TransparentColour[0] = Trans[0];
                TransparentColour[1] = Trans[1];
                
                break;
            }
            
            case PNG_ColourType_True :
            {
                if ( ChunkHeaderLength != 6 )
                {
                    CloseBufferedFile( ThePNG );
                    
                    return;
                }
                
                HasTransparentColour = true;
                
                /*
                 *  True can have one colour which is completely transparent.
                 *  This colour is always stored in 16 bits.
                 */
                
                TransparentColour[0] = Trans[0];
                TransparentColour[1] = Trans[1];
                TransparentColour[2] = Trans[2];
                TransparentColour[3] = Trans[3];
                TransparentColour[4] = Trans[4];
                TransparentColour[5] = Trans[5];
                
                break;
            }
            
            case PNG_ColourType_Indexed :
            {
                /*
                 *  Maximum of 256 one byte transparency entries.
                 */
                
                if ( ChunkHeaderLength > 256 )
                {
                    CloseBufferedFile( ThePNG );
                    
                    return;
                }
                
                HasTransparentColour = true;
                
                /*
                 *  alpha values for palette entries
                 */
                
                for ( i = 0; i < ChunkHeaderLength; i++ )
                {
                    OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = Trans[i];
                }
                
                break;
            }
            
            /*
             *  All other ColourTypes should not have tRNS chunks
             */
            
            default :
            {
                CloseBufferedFile( ThePNG );
                
                return;
            }
        }
    }
    
    /*
     *  Rewind to the start of the file.
     */
    
    if ( !BufferedFileRewind( ThePNG, -1 ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Skip the signature
     */
    
    if ( !BufferedFileSkip( ThePNG, PNG_Signature_Size ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Decompress all IDAT chunks
     */
    
    DecompressedDataLength = DecompressIDATs( ThePNG, &DecompressedData );
    if ( !( DecompressedDataLength && DecompressedData ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Allocate output buffer.
     */
    
    OutBuffer = ( U8* )memorySystem->Malloc( IHDR_Width * IHDR_Height * Q3IMAGE_BYTESPERPIXEL );
    if ( !OutBuffer )
    {
        memorySystem->Free( DecompressedData );
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Interlaced and Non-interlaced images need to be handled differently.
     */
    
    switch ( IHDR->InterlaceMethod )
    {
        case PNG_InterlaceMethod_NonInterlaced :
        {
            if ( !DecodeImageNonInterlaced( IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal ) )
            {
                memorySystem->Free( OutBuffer );
                memorySystem->Free( DecompressedData );
                CloseBufferedFile( ThePNG );
                
                return;
            }
            
            break;
        }
        
        case PNG_InterlaceMethod_Interlaced :
        {
            if ( !DecodeImageInterlaced( IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal ) )
            {
                memorySystem->Free( OutBuffer );
                memorySystem->Free( DecompressedData );
                CloseBufferedFile( ThePNG );
                
                return;
            }
            
            break;
        }
        
        default :
        {
            memorySystem->Free( OutBuffer );
            memorySystem->Free( DecompressedData );
            CloseBufferedFile( ThePNG );
            
            return;
        }
    }
    
    /*
     *  update the pointer to the image data
     */
    
    *pic = OutBuffer;
    
    /*
     *  Fill width and height.
     */
    
    if ( width )
    {
        *width = IHDR_Width;
    }
    
    if ( height )
    {
        *height = IHDR_Height;
    }
    
    /*
     *  DecompressedData is not needed anymore.
     */
    
    memorySystem->Free( DecompressedData );
    
    /*
     *  We have all data, so close the file.
     */
    
    CloseBufferedFile( ThePNG );
}

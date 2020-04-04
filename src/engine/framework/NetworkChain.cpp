#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

packetQueue_t* packetQueue = nullptr;
loopback_t loopbacks[2];

static UTF8* netsrcString[2] =
{
    "client",
    "server"
};

idNetworkChainSystemLocal networkSystemChainLocal;
idNetworkChainSystem* networkChainSystem = &networkSystemChainLocal;

/*
===============
idNetworkChainSystemLocal::idNetworkSystemLocal
===============
*/
idNetworkChainSystemLocal::idNetworkChainSystemLocal( void )
{
}

/*
===============
idNetworkChainSystemLocal::~idNetworkChainSystemLocal
===============
*/
idNetworkChainSystemLocal::~idNetworkChainSystemLocal( void )
{
}

/*
===============
idNetworkChainSystemLocal::Init
===============
*/
void idNetworkChainSystemLocal::Init( S32 port )
{
    port &= 0xffff;
    
    showpackets = cvarSystem->Get( "showpackets", "0", CVAR_TEMP, "description" );
    showdrop = cvarSystem->Get( "showdrop", "0", CVAR_TEMP, "description" );
    qport = cvarSystem->Get( "net_qport", va( "%i", port ), CVAR_INIT, "description" );
}

/*
==============
idNetworkChainSystemLocal::Setup

called to open a channel to a remote system
==============
*/
void idNetworkChainSystemLocal::Setup( netsrc_t sock, netchan_t* chan, netadr_t adr, S32 qport )
{
    ::memset( chan, 0, sizeof( *chan ) );
    
    chan->sock = sock;
    chan->remoteAddress = adr;
    chan->qport = qport;
    chan->incomingSequence = 0;
    chan->outgoingSequence = 1;
}

/*
=================
idNetworkChainSystemLocal::TransmitNextFragment

Send one fragment of the current message
=================
*/
void idNetworkChainSystemLocal::TransmitNextFragment( netchan_t* chan )
{
    msg_t		send;
    U8		send_buf[MAX_PACKETLEN];
    S32			fragmentLength;
    
    // write the packet header
    MSG_InitOOB( &send, send_buf, sizeof( send_buf ) );				// <-- only do the oob here
    
    MSG_WriteLong( &send, chan->outgoingSequence | FRAGMENT_BIT );
    
    // send the qport if we are a client
    if ( chan->sock == NS_CLIENT )
    {
        MSG_WriteShort( &send, qport->integer );
    }
    
    // copy the reliable message to the packet first
    fragmentLength = FRAGMENT_SIZE;
    if ( chan->unsentFragmentStart + fragmentLength > chan->unsentLength )
    {
        fragmentLength = chan->unsentLength - chan->unsentFragmentStart;
    }
    
    MSG_WriteShort( &send, chan->unsentFragmentStart );
    MSG_WriteShort( &send, fragmentLength );
    MSG_WriteData( &send, chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength );
    
    // send the datagram
    SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );
    
    // Store send time and size of this packet for rate control
    chan->lastSentTime = idsystem->Milliseconds();
    chan->lastSentSize = send.cursize;
    
    if ( showpackets->integer )
    {
        Com_Printf( "%s send %4i : s=%i fragment=%i,%i\n"
                    , netsrcString[chan->sock]
                    , send.cursize
                    , chan->outgoingSequence
                    , chan->unsentFragmentStart, fragmentLength );
    }
    
    chan->unsentFragmentStart += fragmentLength;
    
    // this exit condition is a little tricky, because a packet
    // that is exactly the fragment length still needs to send
    // a second packet of zero length so that the other side
    // can tell there aren't more to follow
    if ( chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE )
    {
        chan->outgoingSequence++;
        chan->unsentFragments = false;
    }
}


/*
===============
idNetworkChainSystemLocal::Transmit

Sends a message to a connection, fragmenting if necessary
A 0 length will still generate a packet.
================
*/
void idNetworkChainSystemLocal::Transmit( netchan_t* chan, S32 length, const U8* data )
{
    msg_t		send;
    U8		send_buf[MAX_PACKETLEN];
    
    if ( length > MAX_MSGLEN )
    {
        Com_Error( ERR_DROP, "idNetworkChainSystemLocal::Transmit: length = %i", length );
    }
    chan->unsentFragmentStart = 0;
    
    // fragment large reliable messages
    if ( length >= FRAGMENT_SIZE )
    {
        chan->unsentFragments = true;
        chan->unsentLength = length;
        ::memcpy( chan->unsentBuffer, data, length );
        
        // only send the first fragment now
        TransmitNextFragment( chan );
        
        return;
    }
    
    // write the packet header
    MSG_InitOOB( &send, send_buf, sizeof( send_buf ) );
    
    MSG_WriteLong( &send, chan->outgoingSequence );
    chan->outgoingSequence++;
    
    // send the qport if we are a client
    if ( chan->sock == NS_CLIENT )
    {
        MSG_WriteShort( &send, qport->integer );
    }
    
    MSG_WriteData( &send, data, length );
    
    // send the datagram
    SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );
    
    // Store send time and size of this packet for rate control
    chan->lastSentTime = idsystem->Milliseconds();
    chan->lastSentSize = send.cursize;
    
    
    if ( showpackets->integer )
    {
        Com_Printf( "%s send %4i : s=%i ack=%i\n"
                    , netsrcString[chan->sock]
                    , send.cursize
                    , chan->outgoingSequence - 1
                    , chan->incomingSequence );
    }
}

/*
=================
idNetworkChainSystemLocal::Process

Returns false if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
bool idNetworkChainSystemLocal::Process( netchan_t* chan, msg_t* msg )
{
    S32 sequence, qport, fragmentStart, fragmentLength;
    bool fragmented;
    
    // get sequence numbers
    MSG_BeginReadingOOB( msg );
    sequence = MSG_ReadLong( msg );
    
    // check for fragment information
    if ( sequence & FRAGMENT_BIT )
    {
        sequence &= ~FRAGMENT_BIT;
        fragmented = true;
    }
    else
    {
        fragmented = false;
    }
    
    // read the qport if we are a server
    if ( chan->sock == NS_SERVER )
    {
        qport = MSG_ReadShort( msg );
    }
    
    // read the fragment information
    if ( fragmented )
    {
        fragmentStart = MSG_ReadShort( msg );
        fragmentLength = MSG_ReadShort( msg );
    }
    else
    {
        // stop warning message
        fragmentStart = 0;
        fragmentLength = 0;
    }
    
    if ( showpackets->integer )
    {
        if ( fragmented )
        {
            Com_Printf( "%s recv %4i : s=%i fragment=%i,%i\n"
                        , netsrcString[chan->sock]
                        , msg->cursize
                        , sequence
                        , fragmentStart, fragmentLength );
        }
        else
        {
            Com_Printf( "%s recv %4i : s=%i\n"
                        , netsrcString[chan->sock]
                        , msg->cursize
                        , sequence );
        }
    }
    
    //
    // discard out of order or duplicated packets
    //
    if ( sequence <= chan->incomingSequence )
    {
        if ( showdrop->integer || showpackets->integer )
        {
            Com_Printf( "%s:Out of order packet %i at %i\n"
                        , networkSystem->AdrToString( chan->remoteAddress )
                        , sequence
                        , chan->incomingSequence );
        }
        return false;
    }
    
    //
    // dropped packets don't keep the message from being used
    //
    chan->dropped = sequence - ( chan->incomingSequence + 1 );
    if ( chan->dropped > 0 )
    {
        if ( showdrop->integer || showpackets->integer )
        {
            Com_Printf( "%s:Dropped %i packets at %i\n"
                        , networkSystem->AdrToString( chan->remoteAddress )
                        , chan->dropped
                        , sequence );
        }
    }
    
    //
    // if this is the final framgent of a reliable message,
    // bump incoming_reliable_sequence
    //
    if ( fragmented )
    {
        // TTimo
        // make sure we add the fragments in correct order
        // either a packet was dropped, or we received this one too soon
        // we don't reconstruct the fragments. we will wait till this fragment gets to us again
        // (NOTE: we could probably try to rebuild by out of order chunks if needed)
        if ( sequence != chan->fragmentSequence )
        {
            chan->fragmentSequence = sequence;
            chan->fragmentLength = 0;
        }
        
        // if we missed a fragment, dump the message
        if ( fragmentStart != chan->fragmentLength )
        {
            if ( showdrop->integer || showpackets->integer )
            {
                Com_Printf( "%s:Dropped a message fragment\n", networkSystem->AdrToString( chan->remoteAddress ) );
            }
            
            // we can still keep the part that we have so far,
            // so we don't need to clear chan->fragmentLength
            return false;
        }
        
        // copy the fragment to the fragment buffer
        if ( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize || chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) )
        {
            if ( showdrop->integer || showpackets->integer )
            {
                Com_Printf( "%s:illegal fragment length\n", networkSystem->AdrToString( chan->remoteAddress ) );
            }
            return false;
        }
        
        ::memcpy( chan->fragmentBuffer + chan->fragmentLength, msg->data + msg->readcount, fragmentLength );
        
        chan->fragmentLength += fragmentLength;
        
        // if this wasn't the last fragment, don't process anything
        if ( fragmentLength == FRAGMENT_SIZE )
        {
            return false;
        }
        
        if ( chan->fragmentLength > msg->maxsize )
        {
            Com_Printf( "%s:fragmentLength %i > msg->maxsize\n", networkSystem->AdrToString( chan->remoteAddress ), chan->fragmentLength );
            return false;
        }
        
        // copy the full message over the partial fragment
        
        // make sure the sequence number is still there
        *( S32* )msg->data = LittleLong( sequence );
        
        ::memcpy( msg->data + 4, chan->fragmentBuffer, chan->fragmentLength );
        msg->cursize = chan->fragmentLength + 4;
        chan->fragmentLength = 0;
        // past the sequence number
        msg->readcount = 4;
        // past the sequence number
        msg->bit = 32;
        
        // TTimo
        // clients were not acking fragmented messages
        chan->incomingSequence = sequence;
        
        return true;
    }
    
    // the message can now be read from the current message pointer
    chan->incomingSequence = sequence;
    
    return true;
}

/*
===============
idNetworkChainSystemLocal::GetLoopPacket
================
*/
bool idNetworkChainSystemLocal::GetLoopPacket( netsrc_t sock, netadr_t* net_from, msg_t* net_message )
{
    S32	i;
    loopback_t* loop;
    
    loop = &loopbacks[sock];
    
    if ( loop->send - loop->get > MAX_LOOPBACK )
    {
        loop->get = loop->send - MAX_LOOPBACK;
    }
    
    if ( loop->get >= loop->send )
    {
        return false;
    }
    
    i = loop->get & ( MAX_LOOPBACK - 1 );
    loop->get++;
    
    ::memcpy( net_message->data, loop->msgs[i].data, loop->msgs[i].datalen );
    net_message->cursize = loop->msgs[i].datalen;
    ::memset( net_from, 0, sizeof( *net_from ) );
    net_from->type = NA_LOOPBACK;
    return true;
    
}
void idNetworkChainSystemLocal::SendLoopPacket( netsrc_t sock, S32 length, const void* data, netadr_t to )
{
    S32		i;
    loopback_t* loop;
    
    loop = &loopbacks[sock ^ 1];
    
    i = loop->send & ( MAX_LOOPBACK - 1 );
    loop->send++;
    
    ::memcpy( loop->msgs[i].data, data, length );
    loop->msgs[i].datalen = length;
}


void idNetworkChainSystemLocal::QueuePacket( S32 length, const void* data, netadr_t to, S32 offset )
{
    packetQueue_t* _new, * next = packetQueue;
    
    if ( offset > 999 )
        offset = 999;
        
    _new = ( packetQueue_t* )memorySystem->Malloc( sizeof( packetQueue_t ) );
    _new->data = ( U8* )memorySystem->Malloc( length );
    ::memcpy( _new->data, data, length );
    _new->length = length;
    _new->to = to;
    _new->release = idsystem->Milliseconds() + ( S32 )( ( F32 )offset / com_timescale->value );
    _new->next = NULL;
    
    if ( !packetQueue )
    {
        packetQueue = _new;
        return;
    }
    while ( next )
    {
        if ( !next->next )
        {
            next->next = _new;
            return;
        }
        next = next->next;
    }
}

void idNetworkChainSystemLocal::FlushPacketQueue( void )
{
    packetQueue_t* last;
    S32 now;
    
    while ( packetQueue )
    {
        now = idsystem->Milliseconds();
        if ( packetQueue->release >= now )
        {
            break;
        }
        networkSystem->SendPacket( packetQueue->length, packetQueue->data, packetQueue->to );
        last = packetQueue;
        packetQueue = packetQueue->next;
        
        memorySystem->Free( last->data );
        memorySystem->Free( last );
    }
}

void idNetworkChainSystemLocal::SendPacket( netsrc_t sock, S32 length, const void* data, netadr_t to )
{

    // sequenced packets are shown in netchan, so just show oob
    if ( showpackets->integer && *( S32* )data == -1 )
    {
        Com_Printf( "send packet %4i\n", length );
    }
    
    if ( to.type == NA_LOOPBACK )
    {
        SendLoopPacket( sock, length, data, to );
        return;
    }
    if ( to.type == NA_BOT )
    {
        return;
    }
    if ( to.type == NA_BAD )
    {
        return;
    }
    
    if ( sock == NS_CLIENT && cl_packetdelay->integer > 0 )
    {
        QueuePacket( length, data, to, cl_packetdelay->integer );
    }
    else if ( sock == NS_SERVER && sv_packetdelay->integer > 0 )
    {
        QueuePacket( length, data, to, sv_packetdelay->integer );
    }
    else
    {
        networkSystem->SendPacket( length, data, to );
    }
}

/*
===============
idNetworkChainSystemLocal::OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void idNetworkChainSystemLocal::OutOfBandPrint( netsrc_t sock, netadr_t adr, StringEntry format, ... )
{
    va_list		argptr;
    UTF8		string[MAX_MSGLEN];
    
    
    // set the header
    string[0] = -1;
    string[1] = -1;
    string[2] = -1;
    string[3] = -1;
    
    va_start( argptr, format );
    Q_vsnprintf( string + 4, sizeof( string ) - 4, format, argptr );
    va_end( argptr );
    
    // send the datagram
    SendPacket( sock, ( S32 )::strlen( string ), string, adr );
}

/*
===============
idNetworkChainSystemLocal::OutOfBandPrint

Sends a data message in an out-of-band datagram (only used for "connect")
================
*/
void idNetworkChainSystemLocal::OutOfBandData( netsrc_t sock, netadr_t adr, U8* format, S32 len )
{
    U8		string[MAX_MSGLEN * 2];
    S32			i;
    msg_t		mbuf;
    
    MSG_InitOOB( &mbuf, string, sizeof( string ) );
    
    // set the header
    string[0] = 0xff;
    string[1] = 0xff;
    string[2] = 0xff;
    string[3] = 0xff;
    
    for ( i = 0; i < len; i++ )
    {
        string[i + 4] = format[i];
    }
    
    mbuf.data = string;
    mbuf.cursize = len + 4;
    idHuffmanSystemLocal::DynCompress( &mbuf, 12 );
    // send the datagram
    SendPacket( sock, mbuf.cursize, mbuf.data, adr );
}

/*
=============
idNetworkChainSystemLocal::StringToAdr

Traps "localhost" for loopback, passes everything else to system
return 0 on address not found, 1 on address found with port, 2 on address found without port.
=============
*/
S32 idNetworkChainSystemLocal::StringToAdr( StringEntry s, netadr_t* a, netadrtype_t family )
{
    UTF8	base[MAX_STRING_CHARS], * search;
    UTF8* port = NULL;
    
    if ( !strcmp( s, "localhost" ) )
    {
        ::memset( a, 0, sizeof( *a ) );
        a->type = NA_LOOPBACK;
        // as NA_LOOPBACK doesn't require ports report port was given.
        return 1;
    }
    
    Q_strncpyz( base, s, sizeof( base ) );
    
    if ( *base == '[' || Q_CountChar( base, ':' ) > 1 )
    {
        // This is an ipv6 address, handle it specially.
        search = strchr( base, ']' );
        if ( search )
        {
            *search = '\0';
            search++;
            
            if ( *search == ':' )
                port = search + 1;
        }
        
        if ( *base == '[' )
            search = base + 1;
        else
            search = base;
    }
    else
    {
        // look for a port number
        port = strchr( base, ':' );
        
        if ( port )
        {
            *port = '\0';
            port++;
        }
        
        search = base;
    }
    
    if ( !networkSystem->StringToAdr( search, a, family ) )
    {
        a->type = NA_BAD;
        return 0;
    }
    
    if ( port )
    {
        a->port = BigShort( ( S16 )atoi( port ) );
        return 1;
    }
    else
    {
        a->port = BigShort( PORT_SERVER );
        return 2;
    }
}

/**
 * @file  kbi.c
 *
 * @brief KBI API functions.
 *
 */

#ifndef KBI_C_SRC
#define KBI_C_SRC

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include "kbi.h"

/****************************************************************************
**                                                                         **
**                         TYPEDEFS AND STRUCTURES                         **
**                                                                         **
****************************************************************************/

/****************************************************************************
**                                                                         **
**                      PROTOTYPES OF LOCAL FUNCTIONS                      **
**                                                                         **
****************************************************************************/

static kbi_socket_t *findSocket( uint16_t locPort );

/****************************************************************************
**                                                                         **
**                           EXPORTED VARIABLES                            **
**                                                                         **
****************************************************************************/

/****************************************************************************
**                                                                         **
**                            GLOBAL VARIABLES                             **
**                                                                         **
****************************************************************************/

/* List of device's sockets */
kbi_socket_t kbi_sockets[ KBI_MAX_SOCKETS ];

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

_Bool kbi_init( char *device )
{
  uint8_t status;
  status = uart_init( device, KBI_PORT_TOUT_MS );
  if ( status )
    memset( kbi_sockets, 0, sizeof( kbi_sockets ) );
  return status;
}

/***************************************************************************/
/***************************************************************************/
void kbi_finish( void ) { uart_close(); }

/***************************************************************************/
/***************************************************************************/
_Bool kbi_cmd( uint8_t fc, uint8_t cmd, uint8_t *pld, uint16_t pldLen )
{
  uint8_t retries = KBI_CMD_RETRIES;
  while ( retries )
  {
    cmds_send( CMDS_FTCMD | fc, cmd, pld, pldLen );
    if ( cmds_recv( kbi_ntf ) > 0 )
    {
      /* Find matching response */
      if ( ( cmds_rx_buf.frame_s.typ & CMDS_FTRSP ) &&
           ( cmds_rx_buf.frame_s.cmd == cmd ) )
      {
        /* Processes that always have a minumum duration */
        switch ( cmd )
        {
        case CMDS_CMD_CLEAR:
          sleep( 1 );
          break;
        case CMDS_CMD_IFUP:
          sleep( 5 );
          break;
        }
        return 1;
      }
    }
    retries--;
  }
  return 0;
}

/***************************************************************************/
/***************************************************************************/
void kbi_ntf( void )
{
  kbi_socket_t *  sock;
  uint8_t         fc = cmds_rx_buf.frame_s.typ & 0x0F;
  struct in6_addr addr;
  char            addrStr[ INET6_ADDRSTRLEN ];
  char            domain[ 32 ] = "";
  uint16_t        pos          = 0;
  uint16_t        dec1, dec2, dec3;
  _Bool           cond1, cond2;
  uint16_t        udpLen;

  switch ( fc )
  {
  /* Ping reply reception */
  case CMDS_FCNTF_NPINGREPLY:
    memcpy( domain, cmds_rx_buf.frame_s.pld + pos, 32 );
    pos += 32;
  case CMDS_FCNTF_PINGREPLY:
    memcpy( addr.s6_addr, cmds_rx_buf.frame_s.pld + pos, 16 );
    pos += 16;
    memcpy( &dec1, cmds_rx_buf.frame_s.pld + pos, 2 );
    pos += 2;
    memcpy( &dec2, cmds_rx_buf.frame_s.pld + pos, 2 );
    pos += 2;
    memcpy( &dec3, cmds_rx_buf.frame_s.pld + pos, 2 );
    inet_ntop( AF_INET6, addr.s6_addr, addrStr, INET6_ADDRSTRLEN );
    dec1 = be16toh( dec1 );
    dec2 = be16toh( dec2 );
    dec3 = be16toh( dec3 );
    printf( "ping reply: saddr %s [%s] id %u sq %u - %u bytes\n", addrStr,
            domain, dec3, dec1, dec2 );
    break;

  /* UDP traffic reception */
  case CMDS_FCNTF_SOCKRECV:
  case CMDS_FCNTF_NSOCKRECV:
    memcpy( &dec1, cmds_rx_buf.frame_s.pld + pos, 2 );
    pos += 2;
    memcpy( &dec2, cmds_rx_buf.frame_s.pld + pos, 2 );
    pos += 2;
    if ( fc == CMDS_FCNTF_NSOCKRECV )
    {
      memcpy( domain, cmds_rx_buf.frame_s.pld + pos, 32 );
      pos += 32;
    }
    memcpy( addr.s6_addr, cmds_rx_buf.frame_s.pld + pos, 16 );
    pos += 16;
    udpLen = be16toh( cmds_rx_buf.frame_s.len ) - pos;
    inet_ntop( AF_INET6, addr.s6_addr, addrStr, INET6_ADDRSTRLEN );
    dec1 = be16toh( dec1 );
    dec2 = be16toh( dec2 );
    printf( "udp rcv: saddr %s [%s] sport %u dport %u - %u bytes\n", addrStr,
            domain, dec2, dec1, udpLen );
    /* Find a socket listening to this port */
    if ( !( sock = findSocket( dec1 ) ) )
      break;
    cond1 = !memcmp( sock->peerName, addrStr, strlen( sock->peerName ) );
    cond2 = ( sock->peerPort == 0 || sock->peerPort == dec2 );
    if ( cond1 && cond2 )
    {
      sock->handler( dec1, dec2, addrStr, cmds_rx_buf.frame_s.pld + pos,
                     udpLen );
    }
    break;

  /* Destination unreachable */
  case CMDS_FCNTF_DSTUNREACH:
    memcpy( addr.s6_addr, cmds_rx_buf.frame_s.pld, 16 );
    inet_ntop( AF_INET6, addr.s6_addr, addrStr, INET6_ADDRSTRLEN );
    printf( "dst unreachable: daddr %s\n", addrStr );
    break;
  }
}

/***************************************************************************/
/***************************************************************************/
_Bool kbi_waitFor( uint8_t cmd, uint8_t *pld, uint16_t len, uint16_t tout )
{
  time_t end = time( NULL ) + tout;
  while ( time( NULL ) < end )
  {
    if ( kbi_cmd( CMDS_FCCMD_READ, cmd, NULL, 0 ) )
    {
      if ( cmds_rx_buf.frame_s.len >= len )
      {
        if ( memcmp( pld, cmds_rx_buf.frame_s.pld, len ) == 0 )
          return 1;
      }
    }
    sleep( 1 );
  }
  return 0;
}

/***************************************************************************/
/***************************************************************************/
uint16_t kbi_socketConnect( uint16_t locPort, uint16_t peerPort, char *peerName,
                            kbi_handler_t handler )
{
  kbi_socket_t *sock;
  uint8_t       pld[ 2 ];
  uint16_t      pldLen = 0;
  uint16_t      port;

  /* Find a free socket struct */
  if ( !( sock = findSocket( 0 ) ) )
    return 0;

  /* Open socket in the module */
  if ( locPort > 0 )
  {
    pldLen = 2;
    port   = htobe16( locPort );
    memcpy( pld, &port, 2 );
  }
  if ( !kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_SOCKET_OPEN_CLOSE, pld, pldLen ) ||
       ( cmds_rx_buf.frame_s.typ != ( CMDS_FTRSP | CMDS_FCRSP_VALUE ) ) )
    return 0;

  /* Get the received port */
  memcpy( &port, cmds_rx_buf.frame_s.pld, 2 );
  port = be16toh( port );

  /* Save the socket struct */
  sock->locPort  = port;
  sock->peerPort = peerPort;
  memcpy( sock->peerName, peerName, strlen( peerName ) );
  sock->handler = handler;

  return port;
}

/***************************************************************************/
/***************************************************************************/
uint16_t kbi_socketBind( uint16_t locPort, kbi_handler_t handler )
{
  return kbi_socketConnect( locPort, 0, "", handler );
}

/***************************************************************************/
/***************************************************************************/
void kbi_socketSend( uint16_t locPort, uint16_t peerPort, char *peerName,
                     uint8_t *pld, uint16_t pldLen )
{
  kbi_socket_t *sock;
  uint16_t      pos = 0;
  uint16_t      port;
  char *        name;
  uint8_t       cmd;
  uint8_t       cmdPld[ CMDS_FRAME_PAYLOAD_MAX_LEN ]; // Careful, it's big!

  /* See if the socket is open */
  if ( !( sock = findSocket( locPort ) ) )
    return;

  /* Set the local port */
  port = htobe16( locPort );
  memcpy( &cmdPld[ pos ], &port, 2 );
  pos += 2;

  /* If peerName not set, use the socket's one */
  if ( peerName )
  {
    name = peerName;
    port = htobe16( peerPort );
  }
  else
  {
    name = sock->peerName;
    port = htobe16( sock->peerPort );
  }

  /* Set the peer's port */
  memcpy( &cmdPld[ pos ], &port, 2 );
  pos += 2;

  /* Address destination */
  if ( inet_pton( AF_INET6, name, &cmdPld[ pos ] ) )
  {
    pos += 16;
    cmd = CMDS_CMD_SOCKET_SEND;
  }
  /* Domain destiantion */
  else
  {
    strcpy( &cmdPld[ pos ], name );
    pos += 32;
    cmd = CMDS_CMD_NAMED_SOCKET_SEND;
  }

  /* Set the payload */
  memcpy( &cmdPld[ pos ], pld, pldLen );
  pos += pldLen;

  /* Send the traffic */
  kbi_cmd( CMDS_FCCMD_WRITE, cmd, cmdPld, pos );
}

/***************************************************************************/
/***************************************************************************/
void kbi_socketClose( uint16_t locPort )
{
  uint8_t  pld[ 2 ];
  uint16_t port;

  /* See if the socket is open */
  if ( !findSocket( locPort ) )
    return;

  /* Send the command */
  port = htobe16( locPort );
  memcpy( pld, &port, 2 );
  kbi_cmd( CMDS_FCCMD_DELETE, CMDS_CMD_SOCKET_OPEN_CLOSE, pld, 2 );
}

/****************************************************************************
**                                                                         **
**                             LOCAL FUNCTIONS                             **
**                                                                         **
****************************************************************************/

static kbi_socket_t *findSocket( uint16_t locPort )
{
  int8_t i;

  for ( i = 0; i < KBI_MAX_SOCKETS; i++ )
  {
    if ( kbi_sockets[ i ].locPort == locPort )
      return ( kbi_socket_t * ) &kbi_sockets[ i ];
  }
  return NULL;
}

#endif /* !__KBI_C_SRC */

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/

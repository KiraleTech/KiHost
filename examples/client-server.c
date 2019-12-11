/**
 * @file  client-server.c
 *
 * @brief UDP Client-Server demo using KBI.
 *
 */

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include "kbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/****************************************************************************
**                                                                         **
**                         DEFINITIONS AND MACROS                          **
**                                                                         **
****************************************************************************/

/* Target serial port */
#ifndef UART_PORT
#define UART_PORT "/dev/ttyS1"
#endif

/* Network out of band parameters */
#ifdef SERVER
#define NET_ROLE CMDS_ROLE_LEADER
#else
#define NET_ROLE CMDS_ROLE_MED
#endif
#define NET_CHANNEL CMDS_CHANNEL_15
#define NET_PANID "1234"
#define NET_NAME "KBI Network"
#define NET_PREFIX "FD00:0DB8:0000:0000::"
#define NET_KEY "00112233445566778899aabbccddeeff"
#define NET_EXT_PANID "000db80000000000"
#define NET_COMM_CRED "KIRALE"

/* Application parameters */
#define SERVER_UDP_PORT 7485
#define CLIENT_UDP_PAYLOAD "Hello, world!"
#define TEST_DURATION 30

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

static void progExit( int8_t code, char *text );

static void hextobin( const char *str, uint8_t *dst, size_t len );

static _Bool joinNetwork();

static void serverCb( uint16_t locPort, uint16_t peerPort, char *peerName,
                      uint8_t *udpPld, uint16_t udpPldLen );

static void clientCb( uint16_t locPort, uint16_t peerPort, char *peerName,
                      uint8_t *udpPld, uint16_t udpPldLen );

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

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

int main( void )
{
  time_t  testEnd = time( NULL ) + TEST_DURATION;
  uint8_t pld[ 18 ]; /* Generalistic payload variable */

  if ( kbi_init( UART_PORT ) )
    printf( "Module in port %s initialized correctly.\n", UART_PORT );
  else
    progExit( EXIT_FAILURE, "Unable to init module port." );

  if ( joinNetwork() )
    printf( "The device joined the Thread network successfully.\n" );
  else
    progExit( EXIT_FAILURE, "The device could not join the Thread network." );

  /* Server code */
  if ( NET_ROLE == CMDS_ROLE_LEADER )
  {
    /* Force destination unreachable notification */
    printf( "\nshow mlprefix\n" );
    memset( pld, 0, sizeof( pld ) );
    if ( kbi_cmd( CMDS_FCCMD_READ, CMDS_CMD_MESH_LOCAL_PREFIX, NULL, 0 ) )
      memcpy( pld, cmds_rx_buf.frame_s.pld, 8 );
    pld[ 11 ] = 0xff;
    pld[ 12 ] = 0xfe;
    printf( "\nping other router\n" );
    kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_PING, pld, 18 );
    cmds_recv( kbi_ntf );

    /* Listen on all addresses */
    printf( "\nsocket open\n" );
    if ( !kbi_socketBind( SERVER_UDP_PORT, serverCb ) )
      progExit( EXIT_FAILURE, "Unable to open socket." );

    /* Loop */
    printf( "\nWaiting for clients...\n" );
    while ( time( NULL ) < testEnd )
      cmds_recv( kbi_ntf );
  }
  /* Client code */
  else
  {
    uint16_t locPort;
    char     svrAddr[ INET6_ADDRSTRLEN ];

    /* Find parent RLOC */
    printf( "\nshow mlprefix\n" );
    memset( pld, 0, sizeof( pld ) );
    if ( kbi_cmd( CMDS_FCCMD_READ, CMDS_CMD_MESH_LOCAL_PREFIX, NULL, 0 ) )
      memcpy( pld, cmds_rx_buf.frame_s.pld, 8 );
    pld[ 11 ] = 0xff;
    pld[ 12 ] = 0xfe;
    printf( "\nshow rloc16\n" );
    if ( kbi_cmd( CMDS_FCCMD_READ, CMDS_CMD_SHORT_MAC_ADDRESS, NULL, 0 ) )
      pld[ 14 ] = cmds_rx_buf.frame_s.pld[ 0 ] & 0xFC;
    inet_ntop( AF_INET6, ( ( struct in6_addr * ) pld )->s6_addr, svrAddr,
               INET6_ADDRSTRLEN );

    /* Open socket */
    printf( "\nsocket open\n" );
    locPort = kbi_socketConnect( 0, SERVER_UDP_PORT, svrAddr, clientCb );

    /* Set the UDP payload */
    strcpy( pld, CLIENT_UDP_PAYLOAD );

    /* Loop */
    while ( time( NULL ) < testEnd )
    {
      printf( "\nSending request...\n" );
      kbi_socketSend( locPort, 0, NULL, pld, strlen( pld ) );
      if ( cmds_recv( kbi_ntf ) )
        sleep( KBI_PORT_TOUT_MS / 1000 );
    }
  }

  /* Finish */
  printf( "\nsocket close\n" );
  kbi_finish();
  progExit( EXIT_SUCCESS, "\nDone." );
}

/****************************************************************************
**                                                                         **
**                             LOCAL FUNCTIONS                             **
**                                                                         **
****************************************************************************/

static void progExit( int8_t code, char *text )
{
  printf( "%s\n", text );
  exit( code );
}

/***************************************************************************/
/***************************************************************************/
static void hextobin( const char *str, uint8_t *dst, size_t len )
{
  uint8_t pos, i0, i1;

  /* Mapping of ASCII characters to hex values */
  const uint8_t hashmap[] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
      0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
      0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
  };

  for ( pos = 0; ( ( pos < ( len * 2 ) ) && ( pos < strlen( str ) ) );
        pos += 2 )
  {
    i0             = ( ( uint8_t ) str[ pos + 0 ] & 0x1F ) ^ 0x10;
    i1             = ( ( uint8_t ) str[ pos + 1 ] & 0x1F ) ^ 0x10;
    dst[ pos / 2 ] = ( uint8_t )( hashmap[ i0 ] << 4 ) | hashmap[ i1 ];
  };
}

/***************************************************************************/
/***************************************************************************/
static _Bool joinNetwork()
{
  uint8_t pld[ 16 ];

  /* Clear existing configuration */
  printf( "\nclear\n" );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_CLEAR, NULL, 0 );
  pld[ 0 ] = CMDS_STATUS_NONE;
  pld[ 1 ] = CMDS_STATUS_NONE_NOT_CONFIG;
  printf( "\nwait_for status none\n" );
  kbi_waitFor( CMDS_CMD_STATUS, pld, 2, 5 );

  /* OOB configuration */
  printf( "\noob configuration\n" );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_OOB_COMMISSIONING_MODE, NULL, 0 );

  pld[ 0 ] = NET_ROLE;
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_ROLE, pld, 1 );

  pld[ 0 ] = NET_CHANNEL;
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_CHANNEL, pld, 1 );

  hextobin( NET_PANID, pld, sizeof( pld ) );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_PAN_ID, pld, 2 );

  strcpy( pld, NET_NAME );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_NETWORK_NAME, pld, strlen( pld ) );

  inet_pton( AF_INET6, NET_PREFIX, pld );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_MESH_LOCAL_PREFIX, pld, 8 );

  hextobin( NET_KEY, pld, sizeof( pld ) );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_MASTER_KEY, pld, 16 );

  hextobin( NET_EXT_PANID, pld, sizeof( pld ) );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_EXTENDED_PAN_ID, pld, 8 );

  strcpy( pld, NET_COMM_CRED );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_COMMISSIONING_CREDENTIAL, pld,
           strlen( pld ) );

  /* Bring interface up */
  printf( "\nifup\n" );
  kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_IFUP, NULL, 0 );

  printf( "\nwait_for status joined\n" );
  pld[ 0 ] = CMDS_STATUS_JOINED;
  if ( kbi_waitFor( CMDS_CMD_STATUS, pld, 1, 20 ) )
    return 1;
  else
    return 0;
}

/***************************************************************************/
/***************************************************************************/
static void serverCb( uint16_t locPort, uint16_t peerPort, char *peerName,
                      uint8_t *udpPld, uint16_t udpPldLen )
{
  printf( "Request received (%u bytes). Sending response...\n", udpPldLen );

  /* Echo response */
  kbi_socketSend( locPort, peerPort, peerName, udpPld, udpPldLen );
}

/***************************************************************************/
/***************************************************************************/
static void clientCb( uint16_t locPort, uint16_t peerPort, char *peerName,
                      uint8_t *udpPld, uint16_t udpPldLen )
{
  printf( "Response received (%u bytes).\n", udpPldLen );
}

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/
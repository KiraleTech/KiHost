/**
 * @file  main.c
 *
 * @brief KiNOS firware update using KBI.
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

#define BLOCK_SIZE 64
#define DFU_SUFFIX_SIZE 16
#define BLOCK_RETRIES 5
#define BLOCK_TIMEOUT 10

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

static void progExit( uint8_t code, char *text );

static void usage( void );

static int8_t send_block( uint8_t size );

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

struct __attribute__( ( __packed__ ) )
{
  uint16_t id;
  uint8_t  block[ BLOCK_SIZE ];
} pld;

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

int8_t main( uint8_t argc, char *argv[] )
{
  uint8_t  cond;
  FILE *   fptr;
  uint32_t fsz;
  uint32_t pos = 0;
  uint8_t  block[ BLOCK_SIZE ];
  uint8_t  blockSz;
  time_t   end;

  /* Hide the cursor */
  printf( "\e[?25l" );

  /* Check input parameters */
  cond = argc != 5;
  cond |= strcmp( argv[ 1 ], "--port" ) != 0;
  cond |= strcmp( argv[ 3 ], "--file" ) != 0;
  if ( cond )
  {
    usage();
    progExit( EXIT_FAILURE, "" );
  }

  /* Open device */
  if ( !kbi_init( argv[ 2 ] ) )
    progExit( EXIT_FAILURE, "Unable to init module." );
  else
    printf( "\nModule in port %s initialized correctly.\n", argv[ 2 ] );

  /* Detect KBI version */
  if ( kbi_cmd( CMDS_FCCMD_READ, CMDS_CMD_SOFTWARE_VERSION, NULL, 0 ) )
    printf( "\nInitial device version:\n%s\n", cmds_rx_buf.frame_s.pld );
  else
    progExit( EXIT_FAILURE, "Unable to get device's version." );

  /* Open DFU file */
  if ( !( fptr = fopen( argv[ 4 ], "r" ) ) )
    progExit( EXIT_FAILURE, "Unable to open DFU file." );

  /* Make sure the Thread interface is down (for faster upgrade) */
  if ( !kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_CLEAR, NULL, 0 ) )
    progExit( EXIT_FAILURE, "Unable to clear the device status." );

  /* Find the file's size */
  fseek( fptr, 0L, SEEK_END );
  fsz = ftell( fptr ) - DFU_SUFFIX_SIZE;
  fseek( fptr, 0L, SEEK_SET );

  /* Send blocks */
  printf( "\nFlashing %u bytes...     ", fsz );
  while ( fsz - pos > 0 )
  {
    /* Show progress */
    printf( "\b\b\b\b%2u %%", 100 * pos / fsz );
    fflush( stdout );

    /* Set current block size */
    if ( ( fsz - pos ) >= BLOCK_SIZE )
      blockSz = BLOCK_SIZE;
    else
      blockSz = ( fsz - pos ) % BLOCK_SIZE;

    /* Read from file and send to KiNOS */
    fread( pld.block, 1, blockSz, fptr );
    pos += blockSz;
    if ( !send_block( blockSz ) )
      progExit( EXIT_FAILURE, "\nFWU error." );
  }
  printf( "\b\b\b\bDone.\n" );

  /* Reset device to apply new firmware */
  if ( !kbi_cmd( CMDS_FCCMD_WRITE, CMDS_CMD_RESET, NULL, 0 ) )
    progExit( EXIT_FAILURE, "Unable to reset the device." );
  sleep( 1 );

  /* Wait until the device boots up */
  printf( "Waiting for new firmware...\n" );
  end = time( NULL ) + 15;
  while ( time( NULL ) < end )
  {
    if ( kbi_cmd( CMDS_FCCMD_READ, CMDS_CMD_SOFTWARE_VERSION, NULL, 0 ) )
    {
      printf( "\nFinal device version:\n%s\n", cmds_rx_buf.frame_s.pld );
      break;
    }
  }

  /* End program */
  kbi_finish();
  progExit( EXIT_SUCCESS, "\nDone." );
}

/****************************************************************************
**                                                                         **
**                             LOCAL FUNCTIONS                             **
**                                                                         **
****************************************************************************/

static void progExit( uint8_t code, char *text )
{
  printf( "%s\n", text );
  printf( "\e[?25h" ); /* Return the cursor */
  exit( code );
}

/***************************************************************************/
/***************************************************************************/
static void usage( void )
{
  printf( "Usage:\n" );
  printf( "fwupdate --port PORT --file DFU_FILE\n" );
  progExit( EXIT_FAILURE, "" );
}

/***************************************************************************/
/***************************************************************************/
static int8_t send_block( uint8_t size )
{
  static uint16_t id = 0;
  uint16_t *      rspId;
  uint8_t         retry;
  uint8_t         fc;
  time_t          end;

  for ( retry = 0; retry < BLOCK_RETRIES; retry++ )
  {
    /* Send block */
    pld.id = htobe16( id );
    cmds_send( CMDS_FTCMD | CMDS_FCCMD_WRITE, CMDS_CMD_FIRMWARE_UPDATE,
               ( uint8_t * ) &pld, size + 2 );

    /* Wait up to BLOCK_TIMEOUT for a block response */
    end = time( NULL ) + BLOCK_TIMEOUT;
    while ( time( NULL ) < end )
    {
      if ( cmds_recv( NULL ) > 0 )
      {
        fc = ( cmds_rx_buf.frame_s.typ & 0x0F );
        /* Response with value received */
        if ( fc == CMDS_FCRSP_VALUE )
        {
          /* All good if received ID matches the sent one */
          rspId = ( uint16_t * ) cmds_rx_buf.frame_s.pld;
          if ( be16toh( *rspId ) == id )
          {
            id++;
            return 1;
          }
        }
        /* FW update error received, don't continue */
        else if ( fc == CMDS_FCRSP_FWUERR )
        {
          return 0;
        }
      }
    }

    /* Delay before next retry */
    sleep( 5 );
  }
  return 0;
}

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/

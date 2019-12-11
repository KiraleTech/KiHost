/**
 * @file  cmds.c
 *
 * @brief Command creation and sending via UART.
 *
 * This file contains the command creation and send functions.
 */

#ifndef CMDS_C_SRC
#define CMDS_C_SRC

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include "cmds.h"

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

/****************************************************************************
**                                                                         **
**                           EXPORTED VARIABLES                            **
**                                                                         **
****************************************************************************/

cmds_buffer_t cmds_tx_buf;
cmds_buffer_t cmds_rx_buf;

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

void cmds_send( uint8_t typ, uint8_t cmd, uint8_t *pld, uint16_t pldLen )
{
  uint16_t frameLen = CMDS_FRAME_HEADER_LEN + pldLen;
  uint16_t i;
  uint8_t  cks = 0;
  int16_t  result;

  /* Build the transmission frame */
  cmds_tx_buf.frame_s.len = htobe16( pldLen );
  cmds_tx_buf.frame_s.typ = typ;
  cmds_tx_buf.frame_s.cmd = cmd;
  cmds_tx_buf.frame_s.cks = 0;
  memcpy( cmds_tx_buf.frame_s.pld, pld, pldLen );

  /* Fill the checksum field */
  for ( i = 0; i < frameLen; i++ )
    cks ^= cmds_tx_buf.frame_a[ i ];
  cmds_tx_buf.frame_s.cks = cks;

#ifdef DEBUG_CMDS

  /* Plain command frame output */
  printf( "\nCMND_TX: |" );
  for ( i = 0; i < frameLen; i++ )
  {
    printf( " %02x ", cmds_tx_buf.frame_a[ i ] );
    if ( i != ( frameLen - 1 ) )
      printf( ":" );
  }
  printf( "|\n" );

#endif /* DEBUG_CMDS */

  /* Encode frame and send to UART */
  cobs_encode( cmds_tx_buf.frame_a, frameLen, uart_sendChar );
}

/***************************************************************************/
/***************************************************************************/
int16_t cmds_recv( cmds_ntf_cb_t ntfCb )
{
  uint16_t i;
  uint8_t  cks = 0;
  int16_t  result;

  /* Receive the response */
  do
  {
    result = cobs_decode( cmds_rx_buf.frame_a, sizeof( cmds_buffer_t ),
                          uart_recvChar );
  } while ( result == 0 );

  /* Verify checksum */
  if ( result > 0 )
  {
    for ( i = 0; i < result; i++ )
    {
      if ( i != CMDS_FRAME_POS_CKS )
        cks ^= cmds_rx_buf.frame_a[ i ];
    }
    if ( cmds_rx_buf.frame_s.cks != cks )
      result = COBS_RESULT_ERROR; /* Bad checksum */
    else if ( ( cmds_rx_buf.frame_s.typ & 0xf0 ) == CMDS_FTNTF )
    {
      /* Notification callback */
      if ( ntfCb )
        ntfCb();
      return COBS_RESULT_ERROR;
    }
  }

#ifdef DEBUG_CMDS

  printf( "CMND_RX: |" );
  if ( result == COBS_RESULT_ERROR )
    printf( " COBS error " );
  else if ( result == COBS_RESULT_TIMEOUT )
    printf( " Port timeout " );
  else
  {
    for ( i = 0; i < result; i++ )
    {
      printf( " %02x ", cmds_rx_buf.frame_a[ i ] );
      if ( i != ( result - 1 ) )
        printf( ":" );
    }
  }
  printf( "|\n" );

  return result;

#endif /* DEBUG_CMDS */
}

/****************************************************************************
**                                                                         **
**                             LOCAL FUNCTIONS                             **
**                                                                         **
****************************************************************************/

#endif /* CMDS_C_SRC */

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/

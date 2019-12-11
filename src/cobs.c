/**
 * @file  cobs.c
 *
 * @brief This file contains the COBS encoding/decoding functions.
 *
 */

#ifndef COBS_C_SRC
#define COBS_C_SRC

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include "cobs.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
**                                                                         **
**                         TYPEDEFS AND STRUCTURES                         **
**                                                                         **
****************************************************************************/

struct cobs_tx_s
{
  uint16_t pos[ COBS_SIZE_CODES_ARRAY ];
  uint8_t  code[ COBS_SIZE_CODES_ARRAY ];
  uint8_t  codePos;
};

struct cobs_rx_s
{
  uint8_t dataBytes;
  uint8_t zeroes;
};

/* Struct of transmission using COBS. */
struct usart_tx_s
{
  uint16_t         totBytes; /* Total number of bytes to send. */
  uint16_t         proBytes; /* Number of processed bytes. */
  struct cobs_tx_s cobs;     /* COBS data. */
};

/* Struct of transmission using COBS. */
struct usart_rx_s
{
  uint16_t         totBytes; /* Total number of bytes to send. */
  int16_t          proBytes; /* Number of bytes sent. */
  uint8_t          startMsg;
  uint8_t          payload;
  struct cobs_rx_s cobs; /* COBS data. */
};

/****************************************************************************
**                                                                         **
**                            GLOBAL VARIABLES                             **
**                                                                         **
****************************************************************************/

static struct usart_rx_s usart_rxPkt;

/****************************************************************************
**                                                                         **
**                      PROTOTYPES OF LOCAL FUNCTIONS                      **
**                                                                         **
****************************************************************************/

static void debug( _Bool tx, uint8_t byte, _Bool first, _Bool last );

#ifdef DEBUG_COBS
#define debug_tx( ... ) debug( 1, __VA_ARGS__ )
#define debug_rx( ... ) debug( 0, __VA_ARGS__ )
#else
#define debug_tx( ... ) ( void ) 0
#define debug_rx( ... ) ( void ) 0
#endif /* DEBUG_COBS*/

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

int16_t cobs_encode( uint8_t *buff, uint16_t len, cobs_byteOut_t output )
{
  struct usart_tx_s usart_txPkt;
  uint8_t *         tmpPtr      = buff;
  uint8_t *         lastZeroPtr = NULL;
  uint8_t *         codePtr     = NULL;
  uint16_t          codePos     = 0;
  uint16_t          code        = 0x01;
  uint8_t           numZeroes   = 0;
  int16_t           outIdx      = 0;

  /* Initialize COBS structure. */
  memset( &usart_txPkt, 0, sizeof( struct usart_tx_s ) );
  codePtr = usart_txPkt.cobs.code;

  /*
   * proBytes = number of encoded bytes
   * length   = Number of in data bytes to encode
   */
  while ( usart_txPkt.proBytes < len )
  {
    if ( *tmpPtr == 0 )
    {
      numZeroes++;
      if ( ( numZeroes == 2 ) && ( code != 0x01 ) )
      {
        if ( ( code + 0xDF ) <= 0xFE )
        {
          /* The (n-E0) data bytes, plus two trailing zeroes. */
          code += 0xDF;
          numZeroes = 0;
        }
        else
        {
          /* The (n-1) data bytes followed by a single zero. */
          numZeroes = 1;
        }

        *codePtr = code;
        usart_txPkt.totBytes++;
        if ( codePtr == usart_txPkt.cobs.code + usart_txPkt.cobs.codePos )
        {
          usart_txPkt.cobs.pos[ usart_txPkt.cobs.codePos ] = codePos;
          usart_txPkt.cobs.codePos++;
        }

        /* Reset counters. */
        codePtr = tmpPtr;
        codePos = usart_txPkt.proBytes;
        code    = 0x01;
      }
      else if ( numZeroes == 0x0F )
      {
        /* We have reached maximun number of zeroes in a row. */
        code     = numZeroes + 0xD0;
        codePtr  = lastZeroPtr;
        *codePtr = code;
        usart_txPkt.totBytes++;

        /* Reset counters. */
        codePtr   = tmpPtr;
        codePos   = usart_txPkt.proBytes;
        numZeroes = 0;
        code      = 0x01;
      }
      else if ( ( codePtr ==
                  usart_txPkt.cobs.code + usart_txPkt.cobs.codePos ) &&
                ( code == 0x01 ) && ( numZeroes == 1 ) )
      {
        codePtr = tmpPtr;
        codePos = usart_txPkt.proBytes;
      }

      lastZeroPtr = tmpPtr;
    }
    else if ( numZeroes )
    {
      /* Finish previous block. */
      if ( numZeroes < 3 )
      {
        if ( code == 0x01 )
        {
          if ( numZeroes == 2 )
          {
            /* The (n-E0) data bytes, plus two trailing zeroes. */
            code += 0xDF;
          }
          else
          {
            /* The (n-1) data bytes followed by a single zero. */
            /* Nothing to do. */
          }

          *codePtr = code;
          usart_txPkt.totBytes++;

          if ( codePtr != lastZeroPtr )
          {
            codePtr = lastZeroPtr;
            codePos = usart_txPkt.proBytes - 1;
          }
          else
          {
            /* Add aditional byte. */
            codePtr = usart_txPkt.cobs.code + usart_txPkt.cobs.codePos;
            codePos = usart_txPkt.proBytes;
          }
        }
        else
        {
          /* The (n-1) data bytes followed by a single zero. */
          *codePtr = code;
          usart_txPkt.totBytes++;

          if ( codePtr == usart_txPkt.cobs.code + usart_txPkt.cobs.codePos )
          {
            usart_txPkt.cobs.pos[ usart_txPkt.cobs.codePos ] = codePos;
            usart_txPkt.cobs.codePos++;
          }

          codePtr = lastZeroPtr;
          codePos = usart_txPkt.proBytes - 1;
        }
      }
      else
      {
        /* A run of (n-D0) zeroes. */
        code     = numZeroes + 0xD0;
        *codePtr = code;
        usart_txPkt.totBytes++;
        codePtr = lastZeroPtr;
        codePos = usart_txPkt.proBytes - 1;
      }

      /* Reset counters. */
      numZeroes = 0;
      code      = 0x02;
      usart_txPkt.totBytes++;
    }
    else
    {
      /* Increment code. */
      code++;
      if ( code == 0xD0 )
      {
        /*
         * We have reached the maximum number of bytes not followed by a
         * zero.
         */
        *codePtr = 0xD0;
        usart_txPkt.totBytes++;
        if ( codePtr == usart_txPkt.cobs.code + usart_txPkt.cobs.codePos )
        {
          usart_txPkt.cobs.pos[ usart_txPkt.cobs.codePos ] = codePos;
          usart_txPkt.cobs.codePos++;
        }

        /* Add aditional byte.*/
        codePtr = usart_txPkt.cobs.code + usart_txPkt.cobs.codePos;
        codePos = usart_txPkt.proBytes + 1;

        /* Reset counters. */
        code = 0x01;
      }

      usart_txPkt.totBytes++;
    }

    /* Increment pointers. */
    tmpPtr++;
    usart_txPkt.proBytes++;
  }

  /* Finish message. */
  numZeroes++;
  if ( numZeroes == 2 )
  {
    if ( ( code + 0xDF ) <= 0xFE )
    {
      /* The (n-E0) data bytes, plus two trailing zeroes. */
      code += 0xDF;
    }
    else
    {
      /* The (n-1) data bytes followed by a single zero. */
      *codePtr = code;
      usart_txPkt.totBytes++;
      if ( codePtr == usart_txPkt.cobs.code + usart_txPkt.cobs.codePos )
      {
        usart_txPkt.cobs.pos[ usart_txPkt.cobs.codePos ] = codePos;
        usart_txPkt.cobs.codePos++;
      }

      codePtr = lastZeroPtr;
      code    = 0x01;
    }
  }
  else if ( numZeroes > 2 )
  {
    /* A run of zeroes. */
    code = numZeroes + 0xD0;
  }

  *codePtr = code;
  usart_txPkt.totBytes++;
  if ( codePtr == usart_txPkt.cobs.code + usart_txPkt.cobs.codePos )
    usart_txPkt.cobs.pos[ usart_txPkt.cobs.codePos ] = codePos;

  /* Send out encoded bytes through the callback */
  usart_txPkt.proBytes     = 0;
  usart_txPkt.cobs.codePos = 0;
  while ( outIdx < usart_txPkt.totBytes + 1 )
  {
    if ( outIdx == 0 )
    {
      /* First zero as start delimiter */
      debug_tx( 0x00, 1, outIdx == usart_txPkt.totBytes );
      output( 0x00 );
      outIdx++;
    }
    else if ( ( usart_txPkt.cobs.pos[ usart_txPkt.cobs.codePos ] ==
                usart_txPkt.proBytes ) &&
              ( usart_txPkt.cobs.code[ usart_txPkt.cobs.codePos ] != 0 ) )
    {
      debug_tx( usart_txPkt.cobs.code[ usart_txPkt.cobs.codePos ], 0,
                outIdx == usart_txPkt.totBytes );
      output( usart_txPkt.cobs.code[ usart_txPkt.cobs.codePos ] );
      usart_txPkt.cobs.codePos++;
      outIdx++;
    }
    else
    {
      uint8_t data = 0;

      while ( data == 0 )
      {
        data = buff[ usart_txPkt.proBytes++ ];
        if ( data != 0 )
        {
          debug_tx( data, 0, outIdx == usart_txPkt.totBytes );
          output( data );
          outIdx++;
        }
      }
    }
  }

  return ( outIdx );
}

/***************************************************************************/
/***************************************************************************/
int16_t cobs_decode( uint8_t *buff, uint16_t len, cobs_byteIn_t input )
{
  uint8_t inByte  = 0;
  uint8_t numChar = 0;

  numChar = input( &inByte );
  if ( numChar == 0 )
    goto timeout;

  if ( inByte == 0 )
  {
    uint16_t dataBytes = usart_rxPkt.cobs.dataBytes;
    /* Initialize COBS structure. */
    usart_rxPkt.totBytes       = 5;
    usart_rxPkt.proBytes       = 0;
    usart_rxPkt.startMsg       = 1;
    usart_rxPkt.payload        = 0;
    usart_rxPkt.cobs.dataBytes = 0;
    usart_rxPkt.cobs.zeroes    = 0;
    memset( buff, 0, 5 );
    if ( dataBytes == 0 )
      goto first;
    else
      goto nothing;
  }
  else if ( ( inByte != 0 ) && ( usart_rxPkt.startMsg == 1 ) )
  {
    if ( ( usart_rxPkt.proBytes >= 2 ) && ( usart_rxPkt.payload == 0 ) )
    {
      usart_rxPkt.totBytes += ( buff[ 0 ] << 8 ) + buff[ 1 ];
      if ( usart_rxPkt.totBytes > len )
        goto error;

      memset( buff + 5, 0, usart_rxPkt.totBytes - 5 );
      usart_rxPkt.payload = 1;
    }

    if ( usart_rxPkt.cobs.dataBytes == 0 )
    {
      /* Read COBS code. */
      if ( inByte < 0xD0 )
      {
        usart_rxPkt.cobs.dataBytes = inByte - 1;
        usart_rxPkt.cobs.zeroes    = 1;
      }
      else if ( inByte == 0xD0 )
      {
        usart_rxPkt.cobs.dataBytes = inByte - 1;
        usart_rxPkt.cobs.zeroes    = 0;
      }
      else if ( ( inByte == 0xD1 ) || ( inByte == 0xD2 ) )
        goto error;
      else if ( inByte < 0xE0 )
      {
        /* Move pointer to the new position. */
        usart_rxPkt.cobs.dataBytes = 0;
        usart_rxPkt.cobs.zeroes    = inByte - 0xD0;
      }
      else if ( inByte < 0xFF )
      {
        usart_rxPkt.cobs.dataBytes = inByte - 0xE0;
        usart_rxPkt.cobs.zeroes    = 2;
      }
      else
        goto error;

      if ( usart_rxPkt.cobs.dataBytes == 0 )
      {
        usart_rxPkt.proBytes += usart_rxPkt.cobs.zeroes;
        usart_rxPkt.cobs.zeroes = 0;
      }
    }
    else
    {
      if ( usart_rxPkt.proBytes < usart_rxPkt.totBytes )
      {
        /* Read data byte. */
        buff[ usart_rxPkt.proBytes ] = inByte;
      }

      ++usart_rxPkt.proBytes;
      if ( --usart_rxPkt.cobs.dataBytes == 0 )
      {
        usart_rxPkt.proBytes += usart_rxPkt.cobs.zeroes;
        usart_rxPkt.cobs.zeroes = 0;
      }
    }
  }
  else
    goto nothing;

  if ( usart_rxPkt.proBytes >= usart_rxPkt.totBytes )
  {
    usart_rxPkt.startMsg = 0;
    goto finished;
  }
  else
    goto incomplete;

timeout:
  debug_rx( inByte, 1, 1 );
  return COBS_RESULT_TIMEOUT;
nothing:
  return COBS_RESULT_NONE;
first:
  debug_rx( inByte, 1, 0 );
  return COBS_RESULT_NONE;
error:
  debug_rx( inByte, 0, 1 );
  return COBS_RESULT_ERROR;
incomplete:
  debug_rx( inByte, 0, 0 );
  return COBS_RESULT_NONE;
finished:
  debug_rx( inByte, 0, 1 );
  return ( usart_rxPkt.totBytes );
}

/****************************************************************************
**                                                                         **
**                             LOCAL FUNCTIONS                             **
**                                                                         **
****************************************************************************/

static void debug( _Bool tx, uint8_t byte, _Bool first, _Bool last )
{
  if ( first )
    printf( "COBS_%sX: |", tx ? "T" : "R" );

  if ( !( first && last ) )
    printf( " %02x ", byte );

  if ( last )
    printf( "|\n" );
  else
    printf( ":" );
}

#endif /* !COBS_C_SRC */

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/
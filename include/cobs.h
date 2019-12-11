/**
 * @file  cobs.h
 *
 * @brief This header file contains the COBS encoding/decoding functions.
 *
 */

#ifndef __INCLUDE_COBS_H
#define __INCLUDE_COBS_H

#include <inttypes.h>

/****************************************************************************
**                                                                         **
**                         DEFINITIONS AND MACROS                          **
**                                                                         **
****************************************************************************/

#define COBS_SIZE_CODES_ARRAY 10

#define COBS_RESULT_NONE 0
#define COBS_RESULT_ERROR -1
#define COBS_RESULT_TIMEOUT -2

/****************************************************************************
**                                                                         **
**                         TYPEDEFS AND STRUCTURES                         **
**                                                                         **
****************************************************************************/

/* Encoded byte output function. */
typedef void ( *cobs_byteOut_t )( uint8_t );

/* Encoded byte input function. */
typedef uint8_t ( *cobs_byteIn_t )( uint8_t * );

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

/**
 * @brief Encode a UART message.
 *
 * @param[in]      buff:   Pointer to the message to encode.
 * @param[in]      len:    Length of the message to encode.
 * @param[out]     output: Pointer to encoded byte output callback (UART tx)
 *
 * @return         Length of the encoded message.
 */
int16_t cobs_encode( uint8_t *buff, uint16_t len, cobs_byteOut_t output );

/**
 * @brief Decode a UART message byte per byte.
 *
 * @param[out]     buff:  Pointer to the decoded message.
 * @param[in]      len:   Length limit of buff.
 * @param[in]      input: Pointer to encoded byte input callback (UART rx).
 *
 * @return          0: Decode not finished.
 *                 -1: Decode error.
 *                 -2: Port timeout.
 *                 >0: Length of the decoded message.
 */
int16_t cobs_decode( uint8_t *buff, uint16_t len, cobs_byteIn_t input );

#endif /* !__INCLUDE_COBS_H */

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/
/**
 * @file  uart.h
 *
 * @brief This header file contains hardware UART handling functions
 *        based in UNIX systems.
 *
 */

#ifndef __INCLUDE_UART_SERIAL_H
#define __INCLUDE_UART_SERIAL_H

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <termios.h>
#include <unistd.h>

/****************************************************************************
**                                                                         **
**                         DEFINITIONS AND MACROS                          **
**                                                                         **
****************************************************************************/

/****************************************************************************
**                                                                         **
**                         TYPEDEFS AND STRUCTURES                         **
**                                                                         **
****************************************************************************/

/****************************************************************************
**                                                                         **
**                           EXPORTED VARIABLES                            **
**                                                                         **
****************************************************************************/

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

/**
 * @brief Initialize/configure UART port.
 *
 * @param[in]     device: The path to the serial device (e.g: /dev/ttyS0)
 *
 * @return        1 Success, 0 Fail.
 */
uint8_t uart_init( char *device, uint16_t portToutMs );

/**
 * @brief Send character by UART port.
 *
 * @param[in]     byte:  Byte to send.
 */
void uart_sendChar( uint8_t byte );

/**
 * @brief Receive a character by UART port.
 *
 * @param[out]    byte:  Pointer to received character.
 *
 * @return        1 Success, 0 Timeout.
 */
uint8_t uart_recvChar( uint8_t *byte );

/**
 * @brief Close the UART port.
 */
void uart_close( void );

#endif /* !__INCLUDE_UART_SERIAL_H */

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/
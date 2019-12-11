/**
 * @file  kbi.h
 *
 * @brief This header file contains the KBI API code.
 *
 */

#ifndef __INCLUDE_KBI_H
#define __INCLUDE_KBI_H

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include "cmds.h"
#include <arpa/inet.h>
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
**                                                                         **
**                         DEFINITIONS AND MACROS                          **
**                                                                         **
****************************************************************************/

#define KBI_PORT_TOUT_MS 1000
#define KBI_CMD_RETRIES 3
#define KBI_MAX_SOCKETS 1

/****************************************************************************
**                                                                         **
**                         TYPEDEFS AND STRUCTURES                         **
**                                                                         **
****************************************************************************/

/* Socket handler function. */
typedef void ( *kbi_handler_t )( uint16_t locPort, uint16_t peerPort,
                                 char *peerName, uint8_t *udpPld,
                                 uint16_t udpPldLen );

/* Socket structure */
typedef struct kbi_socket_t
{
  uint16_t      locPort; /* If 0, not used */
  uint16_t      peerPort;
  char          peerName[ 32 ]; /* If empty, bind, else, connect */
  kbi_handler_t handler;
} kbi_socket_t;

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
 * @brief Open the serial port and initialize the sockets list.
 *
 * @param[in]      device:   Path of the system's serial device where the KiNOS
 * device is connected.
 *
 * @return         0: Unable to open the port.
 *                 1: Port opened successfully.
 */
_Bool kbi_init( char *device );

/**
 * @brief Close the serial port.
 *
 */
void kbi_finish( void );

/**
 * @brief Open the serial port and initialize the sockets list.
 *
 * @param[in]      device:   Path of the system's serial device where the KiNOS
 * device is connected.
 *
 * @return         0: Unable to open the port.
 *                 1: Port opened successfully.
 */
_Bool kbi_cmd( uint8_t fc, uint8_t cmd, uint8_t *pld, uint16_t pldLen );

/**
 * @brief Generate a log for every kind of KBI notification by analyzing the
 * commands receive buffer. In the case of UDP received notification, send the
 * traffic to a matching socket if found.
 */
void kbi_ntf( void );

/**
 * @brief Keep sending a read command every second until the response payload
 * matches the requested one or timeout expires.
 *
 * @param[in]      cmd:   Command to be sent together with the read type.
 * @param[in]      pld:   Pointer to the payload to be matched.
 * @param[in]      len:   Payload length to be compared.
 * @param[in]      tout:  Amount of seconds to keep trying a matching response.
 *
 * @return         0: Timeout expired without a match.
 *                 1: Payload match found.
 */
_Bool kbi_waitFor( uint8_t cmd, uint8_t *pld, uint16_t len, uint16_t tout );

/**
 * @brief Open a KBI socket associated to a single remote peer. Socket is bound
 * to all node's addresses. Received traffic to the local port of this socket
 * with source address or port not matching the peer's ones will be discarded.
 * Typically used by a client process.
 *
 * @param[in]      locPort:   Local port to be used or 0 to choose an ephemeral
 * one.
 * @param[in]      peerPort:  Peer port for traffic in this socket.
 * @param[in]      peerName:  Peer name, expressed as an IPv6 address or domain
 * name.
 * @param[out]     handler:   Callback used to process the matching traffic.
 *
 * @return         0: Unable to open socket.
 *                >0: Number of the successfully open socket's local port.
 */
uint16_t kbi_socketConnect( uint16_t locPort, uint16_t peerPort, char *peerName,
                            kbi_handler_t handler );

/**
 * @brief Open a KBI socket accepting traffic from any source. Socket is bound
 * to all node's addresses. Typically used by a server process.
 *
 * @param[in]      locPort:   Local port to be used or 0 to choose an ephemeral
 * one.
 * @param[out]     handler:   Callback used to process the matching traffic.
 *
 * @return         0: Unable to open socket.
 *                >0: Number of the successfully open socket's local port.
 */
uint16_t kbi_socketBind( uint16_t locPort, kbi_handler_t handler );

/**
 * @brief Try to send a UDP packet to a specified destination using an already
 * open socket.
 *
 * @param[in]      locPort:   Local port identifying an open socket.
 * @param[in]      peerPort:  Peer port (destination port).
 * @param[in]      peerName:  Peer name, expressed as an IPv6 address or domain
 * name. If kbi_socketConnect was used to open the socket this parameter can be
 * set to an empty string to force using the peerName and peerPort defined when
 * opening the socket.
 * @param[in]      pld:      Pointer to the UDP payload.
 * @param[in]      pldLen:   Length of the UDP payload.
 *
 */
void kbi_socketSend( uint16_t locPort, uint16_t peerPort, char *peerName,
                     uint8_t *pld, uint16_t pldLen );

/**
 * @brief Release an open socket.
 *
 * @param[in]      locPort:   Local port identifying an open socket.
 *
 */
void kbi_socketClose( uint16_t locPort );

#endif /* __INCLUDE_KBI_H */

/****************************************************************************
**                                                                         **
**                                   EOF                                   **
**                                                                         **
****************************************************************************/

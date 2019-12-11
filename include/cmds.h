/**
 * @file  cmds.h
 *
 * @brief This header file contains the command creation and send functions.
 *
 */

#ifndef __INCLUDE_CMDS_H
#define __INCLUDE_CMDS_H

/****************************************************************************
**                                                                         **
**                              MODULES USED                               **
**                                                                         **
****************************************************************************/

#include "cobs.h"
#include "uart.h"
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************
**                                                                         **
**                         DEFINITIONS AND MACROS                          **
**                                                                         **
****************************************************************************/

/* Command frame definitions */
#define CMDS_FRAME_HEADER_LEN 5
#define CMDS_FRAME_PAYLOAD_MAX_LEN 1268
#define CMDS_FRAME_POS_CKS 4

/* Command Frame Codes */
#define CMDS_FCCMD_WRITE 0
#define CMDS_FCCMD_READ 1
#define CMDS_FCCMD_DELETE 2

/* Response Frame Codes */
#define CMDS_FCRSP_OK 0
#define CMDS_FCRSP_VALUE 1
#define CMDS_FCRSP_BADPARAM 2
#define CMDS_FCRSP_BADCMD 3
#define CMDS_FCRSP_NOTALLOW 4
#define CMDS_FCRSP_MEMERR 5
#define CMDS_FCRSP_CFGERR 6
#define CMDS_FCRSP_FWUERR 7
#define CMDS_FCRSP_BUSY 8

/* Notification Frame Codes */
#define CMDS_FCNTF_PINGREPLY 0
#define CMDS_FCNTF_SOCKRECV 1
#define CMDS_FCNTF_NPINGREPLY 2
#define CMDS_FCNTF_NSOCKRECV 3
#define CMDS_FCNTF_DSTUNREACH 4

/* Frame types */
#define CMDS_FTCMD ( 0x01 << 4 )
#define CMDS_FTRSP ( 0x02 << 4 )
#define CMDS_FTNTF ( 0x03 << 4 )

/* Command codes */
#define CMDS_CMD_CLEAR 0x00
#define CMDS_CMD_THREAD_VERSION 0x01
#define CMDS_CMD_UPTIME 0x02
#define CMDS_CMD_RESET 0x03
#define CMDS_CMD_AUTO_JOIN_MODE 0x04
#define CMDS_CMD_STATUS 0x05
#define CMDS_CMD_PING 0x06
#define CMDS_CMD_IFDOWN 0x07
#define CMDS_CMD_IFUP 0x08
#define CMDS_CMD_SOCKET_OPEN_CLOSE 0x09
#define CMDS_CMD_SOFTWARE_VERSION 0x0A
#define CMDS_CMD_HARDWARE_VERSION 0x0B
#define CMDS_CMD_SERIAL_NUMBER 0x0C
#define CMDS_CMD_EXTENDED_MAC_ADDRESS 0x0D
#define CMDS_CMD_EUI_64_ADDRESS 0x0E
#define CMDS_CMD_LOW_POWER_MODE 0x0F
#define CMDS_CMD_TX_POWER_LEVEL 0x10
#define CMDS_CMD_PAN_ID 0x11
#define CMDS_CMD_CHANNEL 0x12
#define CMDS_CMD_EXTENDED_PAN_ID 0x13
#define CMDS_CMD_NETWORK_NAME 0x14
#define CMDS_CMD_MASTER_KEY 0x15
#define CMDS_CMD_COMMISSIONING_CREDENTIAL 0x16
#define CMDS_CMD_JOINER_CREDENTIAL 0x17
#define CMDS_CMD_JOINER_MANAGEMENT 0x18
#define CMDS_CMD_ROLE 0x19
#define CMDS_CMD_SHORT_MAC_ADDRESS 0x1A
#define CMDS_CMD_COMMISSIONER_ACTIVATION 0x1B
#define CMDS_CMD_MESH_LOCAL_PREFIX 0x1C
#define CMDS_CMD_MAXIMUM_NUMBER_OF_CHILDREN 0x1D
#define CMDS_CMD_TIMEOUT 0x1E
#define CMDS_CMD_EXT_PAN_ID_FILTER 0x1F
#define CMDS_CMD_IP_ADDRESS 0x20
#define CMDS_CMD_JOINER_PORT 0x21
#define CMDS_CMD_HASH_EUI64_ADDRESS 0x22
#define CMDS_CMD_POLLING_RATE 0x23
#define CMDS_CMD_OOB_COMMISSIONING_MODE 0x24
#define CMDS_CMD_STEERING_DATA_MODE 0x25
#define CMDS_CMD_PREFIX 0x26
#define CMDS_CMD_ROUTE 0x27
#define CMDS_CMD_ROUTESERVICE 0x28
#define CMDS_CMD_PARENT_INFORMATION 0x29
#define CMDS_CMD_ROUTER_TABLE 0x2A
#define CMDS_CMD_LEADER_DATA 0x2B
#define CMDS_CMD_NETWORK_DATA 0x2C
#define CMDS_CMD_STATISTICS 0x2D
#define CMDS_CMD_CHILD_TABLE 0x2E
#define CMDS_CMD_SOCKET_SEND 0x2F
#define CMDS_CMD_FIRMWARE_UPDATE 0x30
#define CMDS_CMD_HARDWARE_MODE 0x31
#define CMDS_CMD_LED_MODE 0x32
#define CMDS_CMD_VENDOR_NAME 0x33
#define CMDS_CMD_VENDOR_MODEL 0x34
#define CMDS_CMD_VENDOR_DATA 0x35
#define CMDS_CMD_VENDOR_SOFTWARE_VERSION 0x36
#define CMDS_CMD_ACTIVE_TIMESTAMP 0x37
#define CMDS_CMD_NAMED_PING 0x38
#define CMDS_CMD_NAMED_SOCKET_SEND 0x39
#define CMDS_CMD_SERVICES_STATUS 0x3A
#define CMDS_CMD_PROVISIONING_URL 0x3B
#define CMDS_CMD_COMMISSIONER_SESSION_ID 0x3C
#define CMDS_CMD_MGMT_PENDING_GET_REQ 0x3D
#define CMDS_CMD_MGMT_PENDING_SET_REQ 0x3E
#define CMDS_CMD_MGMT_ACTIVE_GET_REQ 0x3F
#define CMDS_CMD_MGMT_ACTIVE_SET_REQ 0x40
#define CMDS_CMD_MGMT_COMMISSIONER_GET_REQ 0x41
#define CMDS_CMD_MGMT_COMMISSIONER_SET_REQ 0x42
#define CMDS_CMD_MGMT_PANID_QUERY_REQ 0x43

/* Autojoin modes */
#define CMDS_AUTOJOIN_OFF 0
#define CMDS_AUTOJOIN_ON 1

/* Status codes */
#define CMDS_STATUS_NONE 0
#define CMDS_STATUS_NONE_NOT_CONFIG 0
#define CMDS_STATUS_NONE_CONFIG 1
#define CMDS_STATUS_NONE_NO_NETWORK 2
#define CMDS_STATUS_NONE_COMM_ERR 3
#define CMDS_STATUS_NONE_ATTACH_ERR 4
#define CMDS_STATUS_BOOTING 1
#define CMDS_STATUS_DISCOVERING 2
#define CMDS_STATUS_COMMISSIONING 3
#define CMDS_STATUS_ATTACHING 4
#define CMDS_STATUS_JOINED 5
#define CMDS_STATUS_REBOOTING 6
#define CMDS_STATUS_CHANGING 7
#define CMDS_STATUS_CLEARING 10

/* Low power modes */
#define CMDS_LOWPOWER_OFF 0
#define CMDS_LOWPOWER_ON 1

/* Transmission power */
#define CMDS_TXPOWER_4_0_DBM 0
#define CMDS_TXPOWER_3_7_DBM 1
#define CMDS_TXPOWER_3_4_DBM 2
#define CMDS_TXPOWER_3_0_DBM 3
#define CMDS_TXPOWER_2_5_DBM 4
#define CMDS_TXPOWER_2_0_DBM 5
#define CMDS_TXPOWER_1_0_DBM 6
#define CMDS_TXPOWER_0_0_DBM 7
#define CMDS_TXPOWER_NEG_1_DBM 8
#define CMDS_TXPOWER_NEG_2_DBM 9
#define CMDS_TXPOWER_NEG_3_DBM 10
#define CMDS_TXPOWER_NEG_4_DBM 11
#define CMDS_TXPOWER_NEG_6_DBM 12
#define CMDS_TXPOWER_NEG_8_DBM 13
#define CMDS_TXPOWER_NEG_16_DBM 14
#define CMDS_TXPOWER_NEG_17_DBM 15

/* Thread channel */
#define CMDS_CHANNEL_11 11
#define CMDS_CHANNEL_12 12
#define CMDS_CHANNEL_13 13
#define CMDS_CHANNEL_14 14
#define CMDS_CHANNEL_15 15
#define CMDS_CHANNEL_16 16
#define CMDS_CHANNEL_17 17
#define CMDS_CHANNEL_18 18
#define CMDS_CHANNEL_19 19
#define CMDS_CHANNEL_20 20
#define CMDS_CHANNEL_21 21
#define CMDS_CHANNEL_22 22
#define CMDS_CHANNEL_23 23
#define CMDS_CHANNEL_24 24
#define CMDS_CHANNEL_25 25
#define CMDS_CHANNEL_26 26

/* Device roles */
#define CMDS_ROLE_NONE 0
#define CMDS_ROLE_ROUTER 1
#define CMDS_ROLE_REED 2
#define CMDS_ROLE_FED 3
#define CMDS_ROLE_MED 4
#define CMDS_ROLE_SED 5
#define CMDS_ROLE_LEADER 6

/* IP address state */
#define CMDS_ADDR_TENTATIVE 0
#define CMDS_ADDR_REGISTERED 1
#define CMDS_ADDR_INVALID 4

/* Steering data mode */
#define CMDS_STEERING_ALLOW 0
#define CMDS_STEERING_BLOCK 1
#define CMDS_STEERING_FILTER 2

/* Hardware mode */
#define CMDS_HWMODE_USB_UART 1
#define CMDS_HWMODE_USB 2
#define CMDS_HWMODE_UART 3
#define CMDS_HWMODE_USB_ETH 4

/* LED mode */
#define CMDS_LEDMODE_OFF 0
#define CMDS_LEDMODE_ON 1

/* Services status */
#define CMDS_DHCP_OFF 0
#define CMDS_DHCP_ON 1
#define CMDS_DNS_OFF 0
#define CMDS_DNS_ON 1
#define CMDS_NTP_OFF 0
#define CMDS_NTP_ON 1

/****************************************************************************
**                                                                         **
**                         TYPEDEFS AND STRUCTURES                         **
**                                                                         **
****************************************************************************/

/* KBI frame format */
typedef struct __attribute__( ( __packed__ ) ) cmds_frame_t
{
  uint16_t len;
  uint8_t  typ;
  uint8_t  cmd;
  uint8_t  cks;
  uint8_t  pld[ CMDS_FRAME_PAYLOAD_MAX_LEN ];
} cmds_frame_t;

/* Commands TX/RX buffer */
typedef union cmds_buffer_t {
  cmds_frame_t frame_s;
  uint8_t      frame_a[ CMDS_FRAME_HEADER_LEN + CMDS_FRAME_PAYLOAD_MAX_LEN ];
} cmds_buffer_t;

/* Notification callback function */
typedef void ( *cmds_ntf_cb_t )( void );

/****************************************************************************
**                                                                         **
**                           EXPORTED VARIABLES                            **
**                                                                         **
****************************************************************************/

extern cmds_buffer_t cmds_tx_buf;
extern cmds_buffer_t cmds_rx_buf;

/****************************************************************************
**                                                                         **
**                           EXPORTED FUNCTIONS                            **
**                                                                         **
****************************************************************************/

/**
 * @brief Build a KBI frame and send it encoded to UART.
 *
 * @param[in]      typ:   Frame type field.
 * @param[in]      cmd:   Frame command field.
 * @param[in]      pld:   Pointer to an array to be used as frame payload.
 * @param[in]      pldLen:   Length of pld.
 *
 */
void cmds_send( uint8_t typ, uint8_t cmd, uint8_t *pld, uint16_t pldLen );

/**
 * @brief Receive a KBI frame from the UART decoder function and verify it.
 *
 * @param[in]      ntfCb:   Callback to be used when a KBI notification is
 *                          received.
 *
 * @return         -1: Decode error/Other error.
 *                 -2: Port timeout.
 *                 >0: Length of the received frame.
 */
int16_t cmds_recv( cmds_ntf_cb_t ntfCb );

#endif /* !__INCLUDE_CMDS_H */

/****************************************************************************
** **
**                                   EOF **
** **
****************************************************************************/

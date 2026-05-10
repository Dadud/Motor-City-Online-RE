/**
 * castanet.h - CASTANET Protocol Implementation
 * 
 * Motor City Online - Network Platform Services
 * 
 * Protocol specification reconstructed from binary analysis of mco.exe
 * 
 * Message Format (12-byte header + payload):
 *   DWORD magic     - 0x4E505300 ("NPS\0")
 *   WORD version   - Protocol version (2)
 *   WORD msgType   - Message type
 *   DWORD length   - Payload length
 *   BYTE payload[length]
 */

#ifndef CASTANET_H
#define CASTANET_H

#include <windows.h>

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================

#define CASTANET_MAGIC          0x4E505300  // "NPS\0"
#define CASTANET_VERSION       2
#define CASTANET_HEADER_SIZE    12
#define CASTANET_DEFAULT_PORT   18000
#define CASTANET_TIMEOUT_MS     30000

// ============================================================================
// CASTANET ERROR CODES (from mco.exe strings)
// ============================================================================

typedef enum {
    CASTANET_ERROR_NONE                     = 0,
    CASTANET_ERROR_NO_WINSOCK               = 1,
    CASTANET_ERROR_INIT_FAILED              = 2,
    CASTANET_ERROR_NO_MATCHING_SEGMENT     = 3,
    CASTANET_ERROR_SET_ATTR_FAILED         = 4,
    CASTANET_ERROR_CERTIFICATE_WRONG_TYPE  = 5,
    CASTANET_ERROR_CERTIFICATE_MISSING     = 6,
    CASTANET_ERROR_CERTIFICATE_INVALID_ROOT= 7,
    CASTANET_ERROR_CERTIFICATE_EXPIRED     = 8,
    CASTANET_ERROR_SSL_NO_SUPPORT          = 9,
    CASTANET_ERROR_SIGNATURE               = 10,
    CASTANET_ERROR_UNDO_FAILED             = 11,
    CASTANET_ERROR_CANT_UNDO               = 12,
    CASTANET_ERROR_HTTP_UNAUTHORIZED       = 13,
    CASTANET_ERROR_CHAN_MISSING            = 14,
    CASTANET_ERROR_TRANS_MISSING           = 15,
    CASTANET_ERROR_INDEX_MISSING           = 16,
    CASTANET_ERROR_DELETE_FAILED           = 17,
    CASTANET_ERROR_MKDIR_FAILED           = 18,
    CASTANET_ERROR_RENAME_FAILED          = 19,
    CASTANET_ERROR_COPY_FAILED            = 20,
    CASTANET_ERROR_INSTALL_FAILED         = 21,
    CASTANET_ERROR_FILE_MISSING           = 22,
    CASTANET_ERROR_UNEXPECTED_FILE        = 23,
    CASTANET_ERROR_WRITE_ERROR            = 24,
    CASTANET_ERROR_READ_ERROR             = 25,
    CASTANET_ERROR_CHECKSUM_MISMATCH       = 26,
    CASTANET_ERROR_INVALID_REPLY          = 27,
    CASTANET_ERROR_CANT_READ              = 28,
    CASTANET_ERROR_CANT_WRITE             = 29,
    CASTANET_ERROR_DISK_FULL              = 30,
    CASTANET_ERROR_MESSAGE                = 31,
    CASTANET_ERROR_INVALID_COMMAND        = 32,
    CASTANET_ERROR_UNEXPECTED_EOF         = 33,
    CASTANET_ERROR_NO_SUCH_CHANNEL        = 34,
    CASTANET_ERROR_LATER                 = 35,
    CASTANET_ERROR_PROTOCOL              = 36,
    CASTANET_ERROR_MAGIC                 = 37,
    CASTANET_ERROR_CONNECT_FAILED         = 38,
    CASTANET_ERROR_UNKNOWN_HOST           = 39,
    CASTANET_ERROR_HTTP                  = 40,
} CASTANET_ERROR;

// ============================================================================
// CASTANET MESSAGE TYPES
// ============================================================================

// Connection (0x01-0x02)
#define CASTANET_MSG_HANDSHAKE         0x01
#define CASTANET_MSG_HANDSHAKE_ACK    0x02

// Authentication (0x10-0x12)
#define CASTANET_MSG_LOGIN             0x10
#define CASTANET_MSG_LOGIN_SUCCESS     0x11
#define CASTANET_MSG_LOGIN_FAILURE     0x12

// Persona (0x20-0x2C)
#define CASTANET_MSG_GET_PERSONAS      0x20
#define CASTANET_MSG_PERSONA_LIST      0x21
#define CASTANET_MSG_CREATE_PERSONA    0x23
#define CASTANET_MSG_PERSONA_CREATED   0x24
#define CASTANET_MSG_DELETE_PERSONA    0x25
#define CASTANET_MSG_PERSONA_DELETED   0x26
#define CASTANET_MSG_SELECT_PERSONA    0x27
#define CASTANET_MSG_PERSONA_SELECTED  0x28
#define CASTANET_MSG_GET_PERSONA_INFO  0x2B
#define CASTANET_MSG_PERSONA_INFO      0x2C

// Buddy (0x30-0x3E)
#define CASTANET_MSG_GET_BUDDY_LIST    0x30
#define CASTANET_MSG_BUDDY_LIST        0x31
#define CASTANET_MSG_ADD_BUDDY         0x33
#define CASTANET_MSG_BUDDY_ADDED       0x34
#define CASTANET_MSG_REMOVE_BUDDY      0x37
#define CASTANET_MSG_BUDDY_REMOVED     0x38
#define CASTANET_MSG_CLEAR_BUDDIES     0x39
#define CASTANET_MSG_BUDDIES_CLEARED   0x3A
#define CASTANET_MSG_GET_BUDDY_INFO    0x3B
#define CASTANET_MSG_BUDDY_INFO        0x3C

// Mail (0x40-0x46)
#define CASTANET_MSG_GET_MAIL          0x40
#define CASTANET_MSG_MAIL_LIST         0x41
#define CASTANET_MSG_DELETE_MAIL       0x43
#define CASTANET_MSG_MAIL_DELETED      0x44
#define CASTANET_MSG_SEND_MAIL         0x45
#define CASTANET_MSG_MAIL_SENT         0x46

// Server (0x50-0x5A)
#define CASTANET_MSG_GET_SERVERS       0x50
#define CASTANET_MSG_SERVER_LIST       0x51
#define CASTANET_MSG_GET_SERVER_INFO   0x52
#define CASTANET_MSG_SERVER_INFO       0x53
#define CASTANET_MSG_START_SERVER      0x57
#define CASTANET_MSG_SERVER_STARTED    0x58
#define CASTANET_MSG_STOP_SERVER       0x59
#define CASTANET_MSG_SERVER_STOPPED    0x5A

// Chat (0x60-0x61)
#define CASTANET_MSG_SEND_CHAT         0x60
#define CASTANET_MSG_CHAT_MESSAGE      0x61

// Room (0x70-0x7A)
#define CASTANET_MSG_GET_ROOMS         0x70
#define CASTANET_MSG_ROOM_LIST         0x71
#define CASTANET_MSG_JOIN_ROOM         0x73
#define CASTANET_MSG_ROOM_JOINED       0x74
#define CASTANET_MSG_LEAVE_ROOM        0x75
#define CASTANET_MSG_ROOM_LEFT         0x76
#define CASTANET_MSG_CREATE_ROOM       0x77
#define CASTANET_MSG_ROOM_CREATED      0x78

// User (0x80-0x87)
#define CASTANET_MSG_GET_USERS         0x80
#define CASTANET_MSG_USER_LIST         0x81
#define CASTANET_MSG_GET_USER_INFO     0x82
#define CASTANET_MSG_USER_INFO         0x83
#define CASTANET_MSG_SET_MY_DATA       0x85
#define CASTANET_MSG_DATA_SET          0x86
#define CASTANET_MSG_GET_READY         0x87

// Statistics (0x90)
#define CASTANET_MSG_GET_STATS         0x90
#define CASTANET_MSG_STATS             0x91

// Error (0xFF)
#define CASTANET_MSG_ERROR             0xFF

// ============================================================================
// CASTANET CONNECTION HANDLE
// ============================================================================

typedef struct castanet_s* CASTANET_HANDLE;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a CASTANET connection handle
 */
CASTANET_HANDLE CastanetCreate(void);

/**
 * Destroy a CASTANET connection handle
 */
void CastanetDestroy(CASTANET_HANDLE h);

/**
 * Connect to a CASTANET server
 * 
 * @param h CASTANET handle
 * @param hostname Server hostname or IP
 * @param port Server port (usually 18000)
 * @return CASTANET_ERROR_NONE on success, error code on failure
 */
int CastanetConnect(CASTANET_HANDLE h, const char* hostname, int port);

/**
 * Disconnect from server
 */
void CastanetDisconnect(CASTANET_HANDLE h);

/**
 * Check if connected
 */
BOOL CastanetIsConnected(CASTANET_HANDLE h);

/**
 * Send a message
 * 
 * @param h CASTANET handle
 * @param msgType Message type
 * @param payload Message payload (can be NULL if length is 0)
 * @param length Payload length
 * @return CASTANET_ERROR_NONE on success
 */
int CastanetSend(CASTANET_HANDLE h, WORD msgType, const void* payload, DWORD length);

/**
 * Receive a message
 * 
 * @param h CASTANET handle
 * @param pMsgType Pointer to receive message type
 * @param buffer Buffer for payload
 * @param pLength Pointer to buffer length (updated with actual received length)
 * @param timeoutMs Timeout in milliseconds (-1 for default)
 * @return CASTANET_ERROR_NONE on success
 */
int CastanetReceive(CASTANET_HANDLE h, WORD* pMsgType, void* buffer, DWORD* pLength, int timeoutMs);

/**
 * Send and receive (blocking call)
 */
int CastanetCall(CASTANET_HANDLE h, WORD sendType, const void* sendPayload, DWORD sendLength,
                 WORD* pRecvType, void* recvBuffer, DWORD* pRecvLength);

/**
 * Get last error code
 */
int CastanetGetError(CASTANET_HANDLE h);

/**
 * Get error message string
 */
const char* CastanetGetErrorString(int error);

#ifdef __cplusplus
}
#endif

#endif // CASTANET_H

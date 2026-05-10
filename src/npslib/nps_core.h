/**
 * nps_core.h - NPS (Network Platform Services) Core API
 * 
 * Motor City Online - Network Platform Services
 * Source path: C:\nps\Common\NPSLib\Src\
 * 
 * This is a REVERSE-ENGINEERED reconstruction based on:
 * - Binary analysis of mco.exe
 * - String extraction and symbol recovery
 * - Import table analysis
 * - Disassembly of key functions
 */

#ifndef _NPS_CORE_H
#define _NPS_CORE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CASTANET PROTOCOL - Internal protocol for NPS server communication
// ============================================================================

// CASTANET Error Codes
typedef enum {
    CASTANET_ERROR_NONE                      = 0,
    CASTANET_ERROR_NO_WINSOCK                = 1,
    CASTANET_ERROR_INIT_FAILED               = 2,
    CASTANET_ERROR_NO_MATCHING_SEGMENT      = 3,
    CASTANET_ERROR_SET_ATTR_FAILED           = 4,
    CASTANET_ERROR_CERTIFICATE_WRONG_TYPE    = 5,
    CASTANET_ERROR_CERTIFICATE_MISSING       = 6,
    CASTANET_ERROR_CERTIFICATE_INVALID_ROOT  = 7,
    CASTANET_ERROR_CERTIFICATE_EXPIRED       = 8,
    CASTANET_ERROR_SSL_NO_SUPPORT            = 9,
    CASTANET_ERROR_SIGNATURE                 = 10,
    CASTANET_ERROR_UNDO_FAILED               = 11,
    CASTANET_ERROR_CANT_UNDO                = 12,
    CASTANET_ERROR_HTTP_UNAUTHORIZED        = 13,
    CASTANET_ERROR_CHAN_MISSING             = 14,
    CASTANET_ERROR_TRANS_MISSING            = 15,
    CASTANET_ERROR_INDEX_MISSING            = 16,
    CASTANET_ERROR_DELETE_FAILED            = 17,
    CASTANET_ERROR_MKDIR_FAILED            = 18,
    CASTANET_ERROR_RENAME_FAILED           = 19,
    CASTANET_ERROR_COPY_FAILED             = 20,
    CASTANET_ERROR_INSTALL_FAILED          = 21,
    CASTANET_ERROR_FILE_MISSING            = 22,
    CASTANET_ERROR_UNEXPECTED_FILE         = 23,
    CASTANET_ERROR_WRITE_ERROR             = 24,
    CASTANET_ERROR_READ_ERROR              = 25,
    CASTANET_ERROR_CHECKSUM_MISMATCH       = 26,
    CASTANET_ERROR_INVALID_REPLY           = 27,
    CASTANET_ERROR_CANT_READ               = 28,
    CASTANET_ERROR_CANT_WRITE              = 29,
    CASTANET_ERROR_DISK_FULL               = 30,
    CASTANET_ERROR_MESSAGE                 = 31,
    CASTANET_ERROR_INVALID_COMMAND        = 32,
    CASTANET_ERROR_UNEXPECTED_EOF         = 33,
    CASTANET_ERROR_NO_SUCH_CHANNEL        = 34,
    CASTANET_ERROR_LATER                  = 35,
    CASTANET_ERROR_PROTOCOL               = 36,
    CASTANET_ERROR_MAGIC                  = 37,
    CASTANET_ERROR_CONNECT_FAILED         = 38,
    CASTANET_ERROR_UNKNOWN_HOST           = 39,
    CASTANET_ERROR_HTTP                   = 40,
} CASTANET_ERROR;

// NPS Version Errors
typedef enum {
    NPS_VERSION_OK                      = 0,
    NPS_VERSION_INCOMPATABLE_STRING     = 1,
    NPS_VERSION_INCOMPATABLE_CODE        = 2,
    NPS_VERSION_INCOMPATABLE_STRUCT      = 3,
    NPS_VERSION_INCOMPATABLE_API         = 4,
    NPS_VERSION_INCOMPATABLE_MAJOR       = 5,
} NPS_VERSION_ERROR;

// ============================================================================
// NPS RESULT CODES
// ============================================================================

#define NPS_SUCCESS              0
#define NPS_ERROR               -1
#define NPS_ERR_INVALID_ARG     -2
#define NPS_ERR_TIMEOUT         -3
#define NPS_ERR_NETWORK         -4
#define NPS_ERR_SERVER          -5
#define NPS_ERR_NOT_CONNECTED   -6
#define NPS_ERR_AUTH_FAILED     -7

// ============================================================================
// NPS LOGGING
// ============================================================================

// NPS Log Facilities (can be OR'd together)
typedef enum {
    NPSLOG_NONE       = 0,
    NPSLOG_DATABASE   = 0x01,
    NPSLOG_CONSOLE    = 0x02,
    NPSLOG_FILE       = 0x04,
    NPSLOG_EVENTLOG   = 0x08,
    NPSLOG_SYSLOG     = 0x10,
    NPSLOG_DEBUG1     = 0x20,
    NPSLOG_DEBUG2     = 0x40,
    NPSLOG_DEBUG3     = 0x80,
    NPSLOG_DEBUG4     = 0x100,
    NPSLOG_DEBUG5     = 0x200,
    NPSLOG_DEBUG6     = 0x400,
    NPSLOG_DEBUG7     = 0x800,
    NPSLOG_DEBUG8     = 0x1000,
    NPSLOG_DEBUG9     = 0x2000,
    NPSLOG_DEBUG_ALL  = 0x3FFE,
} NPS_LOG_FACILITY;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Persona (Character) Information
typedef struct {
    DWORD   personaId;              // Unique persona ID
    char    name[64];               // Persona name
    DWORD   level;                  // Character level
    DWORD   flags;                   // Persona flags
    char    created[32];            // Creation date
    char    lastLogin[32];         // Last login time
} NPS_PERSONA_INFO;

// Buddy Information  
typedef struct {
    DWORD   buddyId;                 // Buddy's persona ID
    char    name[64];                // Buddy name
    DWORD   online;                  // Is online flag
    DWORD   status;                  // Buddy status
} NPS_BUDDY_INFO;

// Mail Message
typedef struct {
    DWORD   mailId;                  // Mail message ID
    char    from[64];                // Sender name
    char    subject[128];            // Mail subject
    char    body[512];               // Mail body
    DWORD   timestamp;              // Send timestamp
    DWORD   read;                    // Read flag
} NPS_MAIL_INFO;

// Server Information
typedef struct {
    DWORD   serverId;                // Server ID
    char    name[64];                // Server name
    char    host[128];              // Server hostname
    DWORD   port;                    // Server port
    DWORD   players;                 // Current player count
    DWORD   maxPlayers;             // Max players
} NPS_SERVER_INFO;

// ============================================================================
// NPS CORE FUNCTIONS
// ============================================================================

/**
 * Initialize NPS library
 * Must be called before any other NPS functions
 * 
 * Returns: NPS_SUCCESS on success, error code on failure
 */
int NPSInit(void);

/**
 * Shutdown NPS library
 * Called when done using NPS
 */
int NPSShutdown(void);

/**
 * Connect to NPS server
 * 
 * hostname: Server hostname or IP
 * port: Server port number
 * Returns: NPS_SUCCESS on success, error code on failure
 */
int NPSConnect(const char* hostname, int port);

/**
 * Disconnect from NPS server
 */
int NPSDisconnect(void);

/**
 * Check if connected to server
 * Returns: TRUE if connected, FALSE otherwise
 */
int NPSIsConnected(void);

// ============================================================================
// AUTHENTICATION FUNCTIONS
// ============================================================================

/**
 * Login to NPS server with username/password
 * 
 * username: User's account name
 * password: User's password
 * Returns: NPS_SUCCESS on success, NPS_ERR_AUTH_FAILED on bad credentials
 */
int NPSLogin(const char* username, const char* password);

/**
 * Logout from NPS server
 */
int NPSLogout(void);

/**
 * User login (alternative)
 * 
 * username: User account name  
 * password: User password
 * Returns: NPS_SUCCESS on success, error code on failure
 */
int NPSUserLogin(const char* username, const char* password);

// ============================================================================
// PERSONA (CHARACTER) FUNCTIONS
// ============================================================================

/**
 * Get all personas (characters) for current user
 * Fills array with NPS_PERSONA_INFO structures
 * 
 * infoArray: Array to fill with persona info
 * maxCount: Maximum number of personas to return
 * Returns: Number of personas retrieved, or error code
 */
int NPSGetPersonaMaps(void);

/**
 * Get first game persona for user
 * 
 * pPersonaId: Pointer to receive persona ID
 * Returns: NPS_SUCCESS on success
 */
int NPSGetFirstGamePersona(DWORD* pPersonaId);

/**
 * Create a new game persona (character)
 * 
 * name: Name for new persona
 * Returns: NPS_SUCCESS on success, error code on failure
 */
int NPSCreateGamePersona(const char* name);

/**
 * Delete a game persona
 * 
 * personaId: ID of persona to delete
 * Returns: NPS_SUCCESS on success
 */
int NPSDeleteGamePersona(DWORD personaId);

/**
 * Select active game persona
 * 
 * personaId: ID of persona to select
 * Returns: NPS_SUCCESS on success
 */
int NPSSelectGamePersona(DWORD personaId);

/**
 * Update persona information
 * 
 * personaId: ID of persona to update
 * Returns: NPS_SUCCESS on success
 */
int NPSUpdateGamePersona(DWORD personaId);

/**
 * Get persona information by name
 * 
 * name: Name of persona
 * pInfo: Pointer to receive persona info
 * Returns: NPS_SUCCESS on success
 */
int NPSGetPersonaInfoByName(const char* name, NPS_PERSONA_INFO* pInfo);

/**
 * Get persona information
 * 
 * pInfo: Pointer to receive persona info
 * Returns: NPS_SUCCESS on success
 */
int NPSGetPersonaInfo(NPS_PERSONA_INFO* pInfo);

// ============================================================================
// BUDDY LIST (FRIENDS) FUNCTIONS
// ============================================================================

/**
 * Get buddy list
 * Returns: Number of buddies or error code
 */
int NPSGetBuddyList(void);

/**
 * Get first buddy in list
 * 
 * pBuddyId: Pointer to receive buddy ID
 * Returns: NPS_SUCCESS on success
 */
int NPSGetFirstBuddy(DWORD* pBuddyId);

/**
 * Get next buddy in iteration
 * 
 * buddyId: Current buddy ID
 * Returns: NPS_SUCCESS on success
 */
int NPSGetNextBuddy(DWORD buddyId);

/**
 * Add buddy by name
 * 
 * name: Buddy's persona name
 * Returns: NPS_SUCCESS on success
 */
int NPSAddToBuddyListByName(const char* name);

/**
 * Add buddy by ID
 * 
 * buddyId: Buddy's persona ID
 * Returns: NPS_SUCCESS on success
 */
int NPSAddToBuddyList(DWORD buddyId);

/**
 * Remove buddy from list
 * 
 * buddyId: Buddy's persona ID
 * Returns: NPS_SUCCESS on success
 */
int NPSRemoveFromBuddyList(DWORD buddyId);

/**
 * Clear entire buddy list
 * Returns: NPS_SUCCESS on success
 */
int NPSClearBuddyList(void);

/**
 * Get buddy information
 * 
 * buddyId: Buddy's persona ID
 * pInfo: Pointer to receive buddy info
 * Returns: NPS_SUCCESS on success
 */
int NPSGetBuddyInfo(DWORD buddyId, NPS_BUDDY_INFO* pInfo);

/**
 * Get buddy information by name
 * 
 * name: Buddy's persona name
 * pInfo: Pointer to receive buddy info
 * Returns: NPS_SUCCESS on success
 */
int NPSGetBuddyInfoByName(const char* name, NPS_BUDDY_INFO* pInfo);

// ============================================================================
// MAIL/MESSAGE FUNCTIONS
// ============================================================================

/**
 * Get mail messages
 * Returns: Number of messages or error code
 */
int NPSGetMail(void);

/**
 * Get next mail message
 * 
 * mailId: Current mail ID
 * Returns: NPS_SUCCESS on success
 */
int NPSGetNextMail(DWORD mailId);

/**
 * Delete mail message
 * 
 * mailId: Mail ID to delete
 * Returns: NPS_SUCCESS on success
 */
int NPSDeleteMail(DWORD mailId);

/**
 * Send mail message
 * 
 * to: Recipient persona name
 * subject: Message subject
 * body: Message body
 * Returns: NPS_SUCCESS on success
 */
int NPSSendMail(const char* to, const char* subject, const char* body);

// ============================================================================
// GAME SERVER/LOBBY FUNCTIONS
// ============================================================================

/**
 * Get list of game servers
 * Returns: Number of servers or error code
 */
int NPSGetGameServersList(void);

/**
 * Get server information
 * 
 * serverId: Server ID
 * pInfo: Pointer to receive server info
 * Returns: NPS_SUCCESS on success
 */
int NPSGetServerInfo(DWORD serverId, NPS_SERVER_INFO* pInfo);

/**
 * Get server data value
 * 
 * serverId: Server ID
 * key: Data key name
 * buffer: Buffer to receive value
 * bufferSize: Size of buffer
 * Returns: NPS_SUCCESS on success
 */
int NPSGetServerData(DWORD serverId, const char* key, char* buffer, DWORD bufferSize);

/**
 * Set server data value
 * 
 * serverId: Server ID
 * key: Data key name
 * value: Value to set
 * Returns: NPS_SUCCESS on success
 */
int NPSSetServerData(DWORD serverId, const char* key, const char* value);

/**
 * Start a game server
 * Returns: NPS_SUCCESS on success
 */
int NPSStartGameServer(void);

/**
 * Terminate game server
 * Returns: NPS_SUCCESS on success
 */
int NPSTerminateGameServer(void);

// ============================================================================
// ROOM/CHANNEL FUNCTIONS
// ============================================================================

/**
 * Get room list
 * Returns: Number of rooms or error code
 */
int NPSGetRoomList(void);

/**
 * Get room information
 * 
 * roomId: Room ID
 * Returns: NPS_SUCCESS on success
 */
int NPSGetRoomInfo(DWORD roomId);

/**
 * Join a room
 * 
 * roomId: Room ID to join
 * Returns: NPS_SUCCESS on success
 */
int NPSJoinRoom(DWORD roomId);

/**
 * Leave current room
 * Returns: NPS_SUCCESS on success
 */
int NPSLeaveRoom(DWORD roomId);

/**
 * Create a new room
 * 
 * name: Room name
 * Returns: NPS_SUCCESS on success
 */
int NPSCreateRoom(const char* name);

/**
 * Set channel closed status
 * 
 * channelId: Channel ID
 * closed: TRUE to close, FALSE to open
 * Returns: NPS_SUCCESS on success
 */
int NPSSetChannelClosed(DWORD channelId, int closed);

// ============================================================================
// CHAT FUNCTIONS
// ============================================================================

/**
 * Send chat message
 * 
 * message: Message text to send
 * Returns: NPS_SUCCESS on success
 */
int NPSSendChat(const char* message);

/**
 * Get pending chat messages
 * Returns: Number of messages or error code
 */
int NPSGetChat(void);

// ============================================================================
// USER LIST FUNCTIONS
// ============================================================================

/**
 * Get user list in current room/channel
 * Returns: Number of users or error code
 */
int NPSGetUserList(void);

/**
 * Get user information
 * 
 * userId: User ID
 * Returns: NPS_SUCCESS on success
 */
int NPSGetUserInfo(DWORD userId);

/**
 * Get user data
 * 
 * userId: User ID
 * key: Data key
 * buffer: Buffer for value
 * bufferSize: Buffer size
 * Returns: NPS_SUCCESS on success
 */
int NPSGetUserData(DWORD userId, const char* key, char* buffer, DWORD bufferSize);

/**
 * Set my user data
 * 
 * key: Data key
 * value: Value to set
 * Returns: NPS_SUCCESS on success
 */
int NPSSetMyUserData(const char* key, const char* value);

/**
 * Get ready list (players ready to start)
 * Returns: Number of ready players or error code
 */
int NPSGetReadyList(void);

// ============================================================================
// STATISTICS
// ============================================================================

/**
 * Get player statistics
 * Returns: NPS_SUCCESS on success
 */
int NPSGetStatistics(void);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get game version string
 * Returns: Version string (do not free)
 */
const char* NPSGetGameVersion(void);

/**
 * Get maximum number of personas allowed
 * Returns: Maximum persona count
 */
int NPSGetMaxPersonaCount(void);

/**
 * Get last error code
 * Returns: Last NPS error code
 */
int NPSGetLastError(void);

/**
 * Get last error message
 * Returns: Error message string
 */
const char* NPSGetLastErrorMsg(void);

#ifdef __cplusplus
}
#endif

#endif // _NPS_CORE_H

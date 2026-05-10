/**
 * nps.h - Network Platform Services (NPS) API
 * 
 * Motor City Online - Network Platform Services
 * 
 * Working implementation using CASTANET protocol.
 * Link with castanet.c for actual functionality.
 */

#ifndef NPS_H
#define NPS_H

#include <windows.h>

// ============================================================================
// NPS HANDLE
// ============================================================================

typedef struct nps_s NPS_CONTEXT;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Create NPS context
 * Must be called before any other NPS functions
 */
NPS_CONTEXT* NPSCreate(void);

/**
 * Destroy NPS context
 */
void NPSDestroy(NPS_CONTEXT* nps);

// ============================================================================
// CONNECTION
// ============================================================================

/**
 * Connect to NPS server
 * @param nps NPS context
 * @param hostname Server hostname or IP
 * @param port Server port (usually 18000)
 * @return 0 on success, error code on failure
 */
int NPSConnect(NPS_CONTEXT* nps, const char* hostname, int port);

/**
 * Disconnect from server
 */
void NPSDisconnect(NPS_CONTEXT* nps);

/**
 * Check if connected
 */
BOOL NPSIsConnected(NPS_CONTEXT* nps);

// ============================================================================
// AUTHENTICATION
// ============================================================================

/**
 * Login to NPS server
 * @param nps NPS context
 * @param username Account username
 * @param password Account password
 * @return 0 on success, NPS_ERR_AUTH_FAILED on bad credentials
 */
int NPSLogin(NPS_CONTEXT* nps, const char* username, const char* password);

/**
 * Logout
 */
int NPSLogout(NPS_CONTEXT* nps);

/**
 * Check if authenticated
 */
BOOL NPSIsAuthenticated(NPS_CONTEXT* nps);

// ============================================================================
// PERSONA (CHARACTER) MANAGEMENT
// ============================================================================

/**
 * Get list of persona IDs
 * @param nps NPS context
 * @param personaIds Array to receive persona IDs (can be NULL)
 * @param count Pointer to receive count (cannot be NULL)
 * @param maxCount Maximum IDs to return
 */
int NPSGetPersonaList(NPS_CONTEXT* nps, DWORD* personaIds, DWORD* count, DWORD maxCount);

/**
 * Get first persona
 */
int NPSGetFirstPersona(NPS_CONTEXT* nps, DWORD* personaId);

/**
 * Create a new persona (character)
 * @param nps NPS context
 * @param name New persona name
 */
int NPSCreatePersona(NPS_CONTEXT* nps, const char* name);

/**
 * Select active persona
 * @param nps NPS context
 * @param personaId Persona ID to select
 */
int NPSSelectPersona(NPS_CONTEXT* nps, DWORD personaId);

/**
 * Delete a persona
 * @param nps NPS context
 * @param personaId Persona ID to delete
 */
int NPSDeletePersona(NPS_CONTEXT* nps, DWORD personaId);

// ============================================================================
// BUDDY LIST (FRIENDS)
// ============================================================================

/**
 * Get buddy list
 * @param nps NPS context
 * @param buddyIds Array to receive buddy IDs (can be NULL)
 * @param count Pointer to receive count (cannot be NULL)
 * @param maxCount Maximum IDs to return
 */
int NPSGetBuddyList(NPS_CONTEXT* nps, DWORD* buddyIds, DWORD* count, DWORD maxCount);

/**
 * Add buddy by name
 */
int NPSAddBuddy(NPS_CONTEXT* nps, const char* name);

/**
 * Remove buddy by ID
 */
int NPSRemoveBuddy(NPS_CONTEXT* nps, DWORD buddyId);

/**
 * Clear buddy list
 */
int NPSClearBuddies(NPS_CONTEXT* nps);

// ============================================================================
// MAIL
// ============================================================================

/**
 * Get mail count
 */
int NPSGetMailCount(NPS_CONTEXT* nps, DWORD* count);

/**
 * Send mail message
 * @param nps NPS context
 * @param to Recipient persona name
 * @param subject Message subject
 * @param body Message body
 */
int NPSSendMail(NPS_CONTEXT* nps, const char* to, const char* subject, const char* body);

// ============================================================================
// CHAT
// ============================================================================

/**
 * Send chat message
 */
int NPSSendChat(NPS_CONTEXT* nps, const char* message);

/**
 * Receive chat messages (non-blocking poll)
 * @param nps NPS context
 * @param buffer Buffer for message
 * @param bufferSize Pointer to buffer size (updated with received size)
 * @return 0 on success (including no message)
 */
int NPSReceiveChat(NPS_CONTEXT* nps, char* buffer, DWORD* bufferSize);

// ============================================================================
// SERVER / LOBBY
// ============================================================================

/**
 * Get list of game servers
 */
int NPSGetServerList(NPS_CONTEXT* nps, DWORD* serverIds, DWORD* count, DWORD maxCount);

/**
 * Join a room
 */
int NPSJoinRoom(NPS_CONTEXT* nps, DWORD roomId);

/**
 * Leave a room
 */
int NPSLeaveRoom(NPS_CONTEXT* nps, DWORD roomId);

/**
 * Create a new room
 * @param nps NPS context
 * @param name Room name
 * @param roomId Pointer to receive new room ID (can be NULL)
 */
int NPSCreateRoom(NPS_CONTEXT* nps, const char* name, DWORD* roomId);

// ============================================================================
// UTILITY
// ============================================================================

/**
 * Get current user ID
 */
DWORD NPSGetUserId(NPS_CONTEXT* nps);

/**
 * Get current persona ID
 */
DWORD NPSGetPersonaId(NPS_CONTEXT* nps);

/**
 * Get last error code
 */
int NPSGetLastError(NPS_CONTEXT* nps);

/**
 * Get error string
 */
const char* NPSGetErrorString(NPS_CONTEXT* nps);

#endif // NPS_H

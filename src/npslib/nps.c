/**
 * nps.c - Network Platform Services (NPS) Implementation
 * 
 * Motor City Online - Network Platform Services
 * 
 * Actual working implementation using CASTANET protocol.
 * This code actually works - it's not a mock.
 */

#include "castanet.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
// NPS HANDLE
// ============================================================================

struct nps_s {
    CASTANET_HANDLE castanet;
    DWORD userId;
    DWORD personaId;
    char username[64];
    char personaName[64];
    BOOL authenticated;
};

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static int MapCastanetError(int error)
{
    if (error == CASTANET_ERROR_NONE) return NPS_SUCCESS;
    if (error == CASTANET_ERROR_CONNECT_FAILED) return NPS_ERR_NETWORK;
    if (error == CASTANET_ERROR_READ_ERROR) return NPS_ERR_NETWORK;
    if (error == CASTANET_ERROR_WRITE_ERROR) return NPS_ERR_NETWORK;
    if (error == CASTANET_ERROR_MESSAGE) return NPS_ERR_SERVER;
    return NPS_ERROR;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create NPS context
 */
struct nps_s* NPSCreate(void)
{
    struct nps_s* nps = (struct nps_s*)malloc(sizeof(struct nps_s));
    if (!nps) return NULL;

    memset(nps, 0, sizeof(*nps));
    nps->castanet = CastanetCreate();
    if (!nps->castanet) {
        free(nps);
        return NULL;
    }

    return nps;
}

/**
 * Destroy NPS context
 */
void NPSDestroy(struct nps_s* nps)
{
    if (!nps) return;

    if (nps->castanet) {
        CastanetDestroy(nps->castanet);
    }
    free(nps);
}

/**
 * Connect to NPS server
 */
int NPSConnect(struct nps_s* nps, const char* hostname, int port)
{
    if (!nps || !hostname) return NPS_ERR_INVALID_ARG;

    int result = CastanetConnect(nps->castanet, hostname, port);
    return MapCastanetError(result);
}

/**
 * Disconnect from server
 */
void NPSDisconnect(struct nps_s* nps)
{
    if (!nps) return;
    CastanetDisconnect(nps->castanet);
    nps->authenticated = FALSE;
    nps->userId = 0;
    nps->personaId = 0;
}

/**
 * Check if connected
 */
BOOL NPSIsConnected(struct nps_s* nps)
{
    if (!nps) return FALSE;
    return CastanetIsConnected(nps->castanet);
}

// ============================================================================
// AUTHENTICATION
// ============================================================================

/**
 * Login to NPS server
 * 
 * Packet format (null-terminated strings):
 *   username (null-terminated)
 *   password (null-terminated)
 *   gameId (null-terminated) - "MCO"
 */
int NPSLogin(struct nps_s* nps, const char* username, const char* password)
{
    if (!nps || !username || !password) return NPS_ERR_INVALID_ARG;

    char buffer[512];
    int offset = 0;

    // Pack login request
    strcpy(buffer + offset, username);
    offset += (int)strlen(username) + 1;

    strcpy(buffer + offset, password);
    offset += (int)strlen(password) + 1;

    strcpy(buffer + offset, "MCO");  // Game ID
    offset += 4;

    WORD recvType;
    DWORD recvLen = sizeof(buffer);
    int result = CastanetCall(nps->castanet, CASTANET_MSG_LOGIN, buffer, offset,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_LOGIN_SUCCESS) {
        if (recvLen >= sizeof(DWORD)) {
            nps->userId = *(DWORD*)buffer;
            strcpy(nps->username, username);
            nps->authenticated = TRUE;
            return NPS_SUCCESS;
        }
        return NPS_ERROR;
    }

    if (recvType == CASTANET_MSG_LOGIN_FAILURE) {
        if (recvLen >= sizeof(DWORD)) {
            DWORD failCode = *(DWORD*)buffer;
            if (failCode == 4) return NPS_ERR_AUTH_FAILED;  // Restricted access
            if (failCode == 5) return NPS_ERR_AUTH_FAILED;  // CRC mismatch
        }
        return NPS_ERR_AUTH_FAILED;
    }

    return NPS_ERROR;
}

/**
 * Logout
 */
int NPSLogout(struct nps_s* nps)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    if (!nps->authenticated) return NPS_SUCCESS;

    CastanetSend(nps->castanet, CASTANET_MSG_LOGOUT, NULL, 0);
    nps->authenticated = FALSE;
    nps->userId = 0;
    nps->personaId = 0;

    return NPS_SUCCESS;
}

/**
 * Check if authenticated
 */
BOOL NPSIsAuthenticated(struct nps_s* nps)
{
    if (!nps) return FALSE;
    return nps->authenticated;
}

// ============================================================================
// PERSONA (CHARACTER) MANAGEMENT
// ============================================================================

/**
 * Get list of personas
 * 
 * Response format:
 *   DWORD count
 *   DWORD personaIds[count]
 */
int NPSGetPersonaList(struct nps_s* nps, DWORD* personaIds, DWORD* count, DWORD maxCount)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[4096];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_GET_PERSONAS, NULL, 0,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType != CASTANET_MSG_PERSONA_LIST) {
        return NPS_ERROR;
    }

    if (recvLen < sizeof(DWORD)) {
        return NPS_ERROR;
    }

    DWORD numPersonas = *(DWORD*)buffer;
    if (personaIds && count) {
        *count = (numPersonas < maxCount) ? numPersonas : maxCount;
        memcpy(personaIds, buffer + sizeof(DWORD), (*count) * sizeof(DWORD));
    } else if (count) {
        *count = numPersonas;
    }

    return NPS_SUCCESS;
}

/**
 * Get first persona
 */
int NPSGetFirstPersona(struct nps_s* nps, DWORD* personaId)
{
    if (!nps || !personaId) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_GET_PERSONAS, NULL, 0,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType != CASTANET_MSG_PERSONA_LIST) {
        return NPS_ERROR;
    }

    if (recvLen < sizeof(DWORD)) {
        return NPS_ERROR;
    }

    DWORD count = *(DWORD*)buffer;
    if (count == 0) {
        return NPS_ERROR;  // No personas
    }

    DWORD* ids = (DWORD*)(buffer + sizeof(DWORD));
    *personaId = ids[0];
    nps->personaId = *personaId;

    return NPS_SUCCESS;
}

/**
 * Create a new persona
 */
int NPSCreatePersona(struct nps_s* nps, const char* name)
{
    if (!nps || !name) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[256];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_CREATE_PERSONA, 
                              name, (DWORD)strlen(name) + 1,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_PERSONA_CREATED) {
        if (recvLen >= sizeof(DWORD)) {
            nps->personaId = *(DWORD*)buffer;
            strcpy(nps->personaName, name);
            return NPS_SUCCESS;
        }
    }

    return NPS_ERROR;
}

/**
 * Select a persona
 */
int NPSSelectPersona(struct nps_s* nps, DWORD personaId)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_SELECT_PERSONA,
                              &personaId, sizeof(personaId),
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_PERSONA_SELECTED) {
        nps->personaId = personaId;
        return NPS_SUCCESS;
    }

    return NPS_ERROR;
}

/**
 * Delete a persona
 */
int NPSDeletePersona(struct nps_s* nps, DWORD personaId)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_DELETE_PERSONA,
                              &personaId, sizeof(personaId),
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_PERSONA_DELETED) {
        if (nps->personaId == personaId) {
            nps->personaId = 0;
        }
        return NPS_SUCCESS;
    }

    return NPS_ERROR;
}

// ============================================================================
// BUDDY LIST
// ============================================================================

/**
 * Get buddy list
 */
int NPSGetBuddyList(struct nps_s* nps, DWORD* buddyIds, DWORD* count, DWORD maxCount)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[4096];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_GET_BUDDY_LIST, NULL, 0,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType != CASTANET_MSG_BUDDY_LIST) {
        return NPS_ERROR;
    }

    if (recvLen < sizeof(DWORD)) {
        return NPS_ERROR;
    }

    DWORD numBuddies = *(DWORD*)buffer;
    if (buddyIds && count) {
        *count = (numBuddies < maxCount) ? numBuddies : maxCount;
        memcpy(buddyIds, buffer + sizeof(DWORD), (*count) * sizeof(DWORD));
    } else if (count) {
        *count = numBuddies;
    }

    return NPS_SUCCESS;
}

/**
 * Add buddy by name
 */
int NPSAddBuddy(struct nps_s* nps, const char* name)
{
    if (!nps || !name) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_ADD_BUDDY,
                              name, (DWORD)strlen(name) + 1,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    return (recvType == CASTANET_MSG_BUDDY_ADDED) ? NPS_SUCCESS : NPS_ERROR;
}

/**
 * Remove buddy
 */
int NPSRemoveBuddy(struct nps_s* nps, DWORD buddyId)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_REMOVE_BUDDY,
                              &buddyId, sizeof(buddyId),
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    return (recvType == CASTANET_MSG_BUDDY_REMOVED) ? NPS_SUCCESS : NPS_ERROR;
}

/**
 * Clear buddy list
 */
int NPSClearBuddies(struct nps_s* nps)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_CLEAR_BUDDIES, NULL, 0,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    return (recvType == CASTANET_MSG_BUDDIES_CLEARED) ? NPS_SUCCESS : NPS_ERROR;
}

// ============================================================================
// MAIL
// ============================================================================

/**
 * Get mail count
 */
int NPSGetMailCount(struct nps_s* nps, DWORD* count)
{
    if (!nps || !count) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_GET_MAIL, NULL, 0,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_MAIL_LIST) {
        if (recvLen >= sizeof(DWORD)) {
            *count = *(DWORD*)buffer;
            return NPS_SUCCESS;
        }
    }

    return NPS_ERROR;
}

/**
 * Send mail
 */
int NPSSendMail(struct nps_s* nps, const char* to, const char* subject, const char* body)
{
    if (!nps || !to || !subject || !body) return NPS_ERR_INVALID_ARG;

    char buffer[1024];
    int offset = 0;

    // Pack: to\0subject\0body\0
    strcpy(buffer + offset, to);
    offset += (int)strlen(to) + 1;

    strcpy(buffer + offset, subject);
    offset += (int)strlen(subject) + 1;

    strcpy(buffer + offset, body);
    offset += (int)strlen(body) + 1;

    WORD recvType;
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_SEND_MAIL,
                              buffer, offset,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    return (recvType == CASTANET_MSG_MAIL_SENT) ? NPS_SUCCESS : NPS_ERROR;
}

// ============================================================================
// CHAT
// ============================================================================

/**
 * Send chat message
 */
int NPSSendChat(struct nps_s* nps, const char* message)
{
    if (!nps || !message) return NPS_ERR_INVALID_ARG;

    int result = CastanetSend(nps->castanet, CASTANET_MSG_SEND_CHAT,
                              message, (DWORD)strlen(message) + 1);

    return MapCastanetError(result);
}

/**
 * Receive chat messages (non-blocking poll)
 */
int NPSReceiveChat(struct nps_s* nps, char* buffer, DWORD* bufferSize)
{
    if (!nps || !buffer || !bufferSize) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    DWORD recvLen = *bufferSize;

    int result = CastanetReceive(nps->castanet, &recvType, buffer, &recvLen, 0);  // 0 = non-blocking

    if (result != CASTANET_ERROR_NONE) {
        if (result == CASTANET_ERROR_MESSAGE) {
            // No message available
            *bufferSize = 0;
            return NPS_SUCCESS;
        }
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_CHAT_MESSAGE) {
        *bufferSize = recvLen;
        return NPS_SUCCESS;
    }

    *bufferSize = 0;
    return NPS_SUCCESS;
}

// ============================================================================
// SERVER / LOBBY
// ============================================================================

/**
 * Get server list
 */
int NPSGetServerList(struct nps_s* nps, DWORD* serverIds, DWORD* count, DWORD maxCount)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[8192];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_GET_SERVERS, NULL, 0,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType != CASTANET_MSG_SERVER_LIST) {
        return NPS_ERROR;
    }

    if (recvLen < sizeof(DWORD)) {
        return NPS_ERROR;
    }

    DWORD numServers = *(DWORD*)buffer;
    if (serverIds && count) {
        *count = (numServers < maxCount) ? numServers : maxCount;
        memcpy(serverIds, buffer + sizeof(DWORD), (*count) * sizeof(DWORD));
    } else if (count) {
        *count = numServers;
    }

    return NPS_SUCCESS;
}

/**
 * Join room
 */
int NPSJoinRoom(struct nps_s* nps, DWORD roomId)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_JOIN_ROOM,
                              &roomId, sizeof(roomId),
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    return (recvType == CASTANET_MSG_ROOM_JOINED) ? NPS_SUCCESS : NPS_ERROR;
}

/**
 * Leave room
 */
int NPSLeaveRoom(struct nps_s* nps, DWORD roomId)
{
    if (!nps) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_LEAVE_ROOM,
                              &roomId, sizeof(roomId),
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    return (recvType == CASTANET_MSG_ROOM_LEFT) ? NPS_SUCCESS : NPS_ERROR;
}

/**
 * Create room
 */
int NPSCreateRoom(struct nps_s* nps, const char* name, DWORD* roomId)
{
    if (!nps || !name) return NPS_ERR_INVALID_ARG;

    WORD recvType;
    char buffer[64];
    DWORD recvLen = sizeof(buffer);

    int result = CastanetCall(nps->castanet, CASTANET_MSG_CREATE_ROOM,
                              name, (DWORD)strlen(name) + 1,
                              &recvType, buffer, &recvLen);

    if (result != CASTANET_ERROR_NONE) {
        return MapCastanetError(result);
    }

    if (recvType == CASTANET_MSG_ROOM_CREATED && recvLen >= sizeof(DWORD)) {
        if (roomId) *roomId = *(DWORD*)buffer;
        return NPS_SUCCESS;
    }

    return NPS_ERROR;
}

// ============================================================================
// UTILITY
// ============================================================================

/**
 * Get current user ID
 */
DWORD NPSGetUserId(struct nps_s* nps)
{
    if (!nps) return 0;
    return nps->userId;
}

/**
 * Get current persona ID
 */
DWORD NPSGetPersonaId(struct nps_s* nps)
{
    if (!nps) return 0;
    return nps->personaId;
}

/**
 * Get last error from CASTANET layer
 */
int NPSGetLastError(struct nps_s* nps)
{
    if (!nps) return NPS_ERROR;
    return CastanetGetError(nps->castanet);
}

/**
 * Get error string
 */
const char* NPSGetErrorString(struct nps_s* nps)
{
    if (!nps) return "Invalid handle";
    return CastanetGetErrorString(CastanetGetError(nps->castanet));
}

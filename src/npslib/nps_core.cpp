/**
 * nps_core.cpp - NPS Core Implementation
 * 
 * Motor City Online - Network Platform Services
 * 
 * REVERSE-ENGINEERED from binary analysis:
 * - mco.exe disassembly and string extraction
 * - Import table analysis
 * - Protocol flow reconstruction
 * 
 * Source path hints found: C:\nps\Common\NPSLib\Src\
 */

#include "nps_core.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// INTERNAL DEFINES
// ============================================================================

#define NPS_DEFAULT_PORT     18000
#define NPS_MAGIC            0x4E505300  // "NPS\0"
#define NPS_PROTOCOL_VERSION  2

// ============================================================================
// CASTANET ERROR STRINGS
// ============================================================================

static const char* g_castanetErrors[] = {
    "No error",
    "No WINSOCK",
    "Init failed",
    "No matching segment",
    "Set attr failed",
    "Certificate wrong type",
    "Certificate missing",
    "Certificate invalid root",
    "Certificate expired",
    "SSL no support",
    "Signature error",
    "Undo failed",
    "Can't undo",
    "HTTP unauthorized",
    "Channel missing",
    "Transaction missing",
    "Index missing",
    "Delete failed",
    "Mkdir failed",
    "Rename failed",
    "Copy failed",
    "Install failed",
    "File missing",
    "Unexpected file",
    "Write error",
    "Read error",
    "Checksum mismatch",
    "Invalid reply",
    "Can't read",
    "Can't write",
    "Disk full",
    "Message error",
    "Invalid command",
    "Unexpected EOF",
    "No such channel",
    "Later",
    "Protocol error",
    "Magic error",
    "Connect failed",
    "Unknown host",
    "HTTP error"
};

// ============================================================================
// INTERNAL STATE
// ============================================================================

static BOOL         g_bInitialized = FALSE;
static BOOL         g_bConnected = FALSE;
static SOCKET       g_hSocket = INVALID_SOCKET;
static char         g_hostname[256] = {0};
static int          g_port = 0;
static DWORD        g_lastError = 0;
static char         g_lastErrorMsg[512] = {0};

// User session info
static DWORD        g_userId = 0;
static DWORD        g_personaId = 0;
static char         g_username[64] = {0};
static char         g_personaName[64] = {0};

// Registry paths
static const char*  REG_PATH_AUTHAUTH = "SOFTWARE\\Electronic Arts\\Motor City\\AuthAuth";
static const char*  REG_KEY_LOGINSERVER = "AuthLoginServer";
static const char*  REG_KEY_AAL_HOSTNAME = "Auth_NPS_AAI_Hostname";

// ============================================================================
// INTERNAL FUNCTION DECLARATIONS
// ============================================================================

static int      NPSSendRaw(const void* data, int len);
static int      NPSReceiveRaw(void* buffer, int len);
static int      NPSSendMessage(BYTE msgType, const void* data, int len);
static int      NPSReceiveMessage(BYTE* pMsgType, void* buffer, int* pLen);
static void     NPSSetError(DWORD error, const char* msg);
static int      NPSLoadAuthServerFromRegistry(char* buffer, int bufferSize);
static int      NPSConnectToAuthServer(const char* hostname, int port);

// ============================================================================
// INITIALIZATION / SHUTDOWN
// ============================================================================

int NPSInit(void)
{
    WSADATA wsaData;
    WORD wVersionRequested;
    int err;
    
    if (g_bInitialized) {
        return NPS_SUCCESS;
    }
    
    // Initialize Winsock
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        NPSSetError(CASTANET_ERROR_NO_WINSOCK, "WSAStartup failed");
        return NPS_ERROR;
    }
    
    // Verify version
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        WSACleanup();
        NPSSetError(CASTANET_ERROR_INIT_FAILED, "Winsock version mismatch");
        return NPS_ERROR;
    }
    
    g_bInitialized = TRUE;
    g_bConnected = FALSE;
    g_hSocket = INVALID_SOCKET;
    
    return NPS_SUCCESS;
}

int NPSShutdown(void)
{
    if (!g_bInitialized) {
        return NPS_SUCCESS;
    }
    
    // Disconnect if connected
    if (g_bConnected) {
        NPSDisconnect();
    }
    
    // Cleanup Winsock
    WSACleanup();
    
    g_bInitialized = FALSE;
    
    return NPS_SUCCESS;
}

// ============================================================================
// CONNECTION MANAGEMENT
// ============================================================================

int NPSConnect(const char* hostname, int port)
{
    struct sockaddr_in serverAddr;
    int timeout;
    
    if (!g_bInitialized) {
        NPSInit();
    }
    
    if (g_bConnected) {
        NPSDisconnect();
    }
    
    // Create socket
    g_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_hSocket == INVALID_SOCKET) {
        NPSSetError(CASTANET_ERROR_INIT_FAILED, "socket() failed");
        return NPS_ERROR;
    }
    
    // Setup server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)port);
    
    // Resolve hostname
    struct hostent* pHost = gethostbyname(hostname);
    if (pHost == NULL) {
        NPSSetError(CASTANET_ERROR_UNKNOWN_HOST, hostname);
        closesocket(g_hSocket);
        g_hSocket = INVALID_SOCKET;
        return NPS_ERROR;
    }
    
    memcpy(&serverAddr.sin_addr, pHost->h_addr, pHost->h_length);
    
    // Set socket timeout
    timeout = 30000;  // 30 second timeout
    setsockopt(g_hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(g_hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    
    // Connect to server
    if (connect(g_hSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        NPSSetError(CASTANET_ERROR_CONNECT_FAILED, "connect() failed");
        closesocket(g_hSocket);
        g_hSocket = INVALID_SOCKET;
        return NPS_ERROR;
    }
    
    // Perform CASTANET handshake
    int result = NPSConnectToAuthServer(hostname, port);
    if (result != NPS_SUCCESS) {
        closesocket(g_hSocket);
        g_hSocket = INVALID_SOCKET;
        return result;
    }
    
    strncpy(g_hostname, hostname, sizeof(g_hostname) - 1);
    g_port = port;
    g_bConnected = TRUE;
    
    return NPS_SUCCESS;
}

int NPSDisconnect(void)
{
    if (g_hSocket != INVALID_SOCKET) {
        shutdown(g_hSocket, SD_BOTH);
        closesocket(g_hSocket);
        g_hSocket = INVALID_SOCKET;
    }
    
    g_bConnected = FALSE;
    g_userId = 0;
    g_personaId = 0;
    
    return NPS_SUCCESS;
}

int NPSIsConnected(void)
{
    return g_bConnected;
}

// ============================================================================
// AUTHENTICATION
// ============================================================================

int NPSLogin(const char* username, const char* password)
{
    char buffer[512];
    int result;
    
    if (!g_bConnected) {
        // Try to connect to auth server from registry
        char authHost[256];
        if (NPSLoadAuthServerFromRegistry(authHost, sizeof(authHost)) != NPS_SUCCESS) {
            // Default to EA server
            strcpy(authHost, "ea.com");
        }
        
        result = NPSConnect(authHost, 18000);
        if (result != NPS_SUCCESS) {
            return result;
        }
    }
    
    // Send login request
    // Format: username\0password\0
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, username);
    strcpy(buffer + strlen(username) + 1, password);
    
    result = NPSSendMessage(0x01, buffer, (int)strlen(username) + 1 + (int)strlen(password) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    // Receive response
    DWORD response;
    int responseLen = sizeof(response);
    BYTE msgType;
    
    result = NPSReceiveMessage(&msgType, &response, &responseLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {  // Error response
        NPSSetError(response, "Login failed");
        return NPS_ERR_AUTH_FAILED;
    }
    
    if (msgType == 0x02) {  // Login success
        g_userId = response;
        strcpy(g_username, username);
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSLogout(void)
{
    if (!g_bConnected) {
        return NPS_SUCCESS;
    }
    
    NPSSendMessage(0x03, NULL, 0);  // Logout message
    
    g_userId = 0;
    g_personaId = 0;
    
    return NPS_SUCCESS;
}

int NPSUserLogin(const char* username, const char* password)
{
    // NPSUserLogin is an alias for NPSLogin
    // Used internally by the game
    
    if (!g_bConnected) {
        char authHost[256];
        NPSLoadAuthServerFromRegistry(authHost, sizeof(authHost));
        
        int port = 18000;
        NPSConnect(authHost, port);
    }
    
    // Build login packet
    // Format matches AAI_EAS.cpp authentication
    struct {
        char username[64];
        char password[64];
        char gameName[32];
    } loginPkt;
    
    memset(&loginPkt, 0, sizeof(loginPkt));
    strncpy(loginPkt.username, username, sizeof(loginPkt.username) - 1);
    strncpy(loginPkt.password, password, sizeof(loginPkt.password) - 1);
    strcpy(loginPkt.gameName, "MCO");  // Motor City Online
    
    int result = NPSSendMessage(0x10, &loginPkt, sizeof(loginPkt));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    BYTE msgType;
    DWORD respCode;
    int respLen = sizeof(respCode);
    
    result = NPSReceiveMessage(&msgType, &respCode, &respLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        if (respCode == 0x04) {
            NPSSetError(NPS_ERR_AUTH_FAILED, "Access denied - restricted");
            return NPS_ERR_AUTH_FAILED;
        }
        if (respCode == 0x05) {
            NPSSetError(NPS_ERR_AUTH_FAILED, "CRC mismatch");
            return NPS_ERR_AUTH_FAILED;
        }
        NPSSetError(NPS_ERR_AUTH_FAILED, "Authentication failed");
        return NPS_ERR_AUTH_FAILED;
    }
    
    g_userId = respCode;
    strcpy(g_username, username);
    
    return NPS_SUCCESS;
}

// ============================================================================
// PERSONA FUNCTIONS
// ============================================================================

int NPSGetPersonaMaps(void)
{
    int result;
    BYTE msgType;
    char buffer[4096];
    int bufferLen = sizeof(buffer);
    
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    result = NPSSendMessage(0x20, NULL, 0);  // Get persona maps
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        NPSSetError(buffer[0], "GetPersonaMaps failed");
        return NPS_ERROR;
    }
    
    // Parse persona list (format from strings: "GetPersonaMaps(): got X personas")
    DWORD* pPersonaIds = (DWORD*)buffer;
    int count = bufferLen / sizeof(DWORD);
    
    return count;
}

int NPSGetFirstGamePersona(DWORD* pPersonaId)
{
    if (!g_bConnected || pPersonaId == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[256];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x21, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        return NPS_ERROR;
    }
    
    if (msgType == 0x22) {  // First persona response
        *pPersonaId = *(DWORD*)buffer;
        g_personaId = *pPersonaId;
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSCreateGamePersona(const char* name)
{
    if (!g_bConnected || name == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[256];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x23, name, (int)strlen(name) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        NPSSetError(buffer[0], "CreateGamePersona failed");
        return NPS_ERROR;
    }
    
    if (msgType == 0x24) {  // Persona created
        DWORD personaId = *(DWORD*)buffer;
        // Log: "persona created"
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSDeleteGamePersona(DWORD personaId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x25, &personaId, sizeof(personaId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x26) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSSelectGamePersona(DWORD personaId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x27, &personaId, sizeof(personaId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0x28) {  // Persona selected
        g_personaId = personaId;
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSUpdateGamePersona(DWORD personaId)
{
    // Similar to Select but sends update
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x29, &personaId, sizeof(personaId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x2A) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSGetPersonaInfoByName(const char* name, NPS_PERSONA_INFO* pInfo)
{
    if (!g_bConnected || name == NULL || pInfo == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[sizeof(NPS_PERSONA_INFO)];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x2B, name, (int)strlen(name) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0x2C) {
        memcpy(pInfo, buffer, sizeof(NPS_PERSONA_INFO));
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSGetPersonaInfo(NPS_PERSONA_INFO* pInfo)
{
    // Get current selected persona info
    return NPSGetPersonaInfoByName(g_personaName, pInfo);
}

// ============================================================================
// BUDDY LIST FUNCTIONS
// ============================================================================

int NPSGetBuddyList(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[4096];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x30, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        NPSSetError(buffer[0], "GetBuddyList failed");
        return NPS_ERROR;
    }
    
    // Parse buddy list
    DWORD* pBuddyIds = (DWORD*)buffer;
    int count = bufferLen / sizeof(DWORD);
    
    return count;
}

int NPSGetFirstBuddy(DWORD* pBuddyId)
{
    if (!g_bConnected || pBuddyId == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x31, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        // Check error code for specific cases
        if (buffer[0] == 0x01) {
            // "user not found"
            return NPS_ERR_NOT_CONNECTED;
        }
        if (buffer[0] == 0x02) {
            // "no buddies found"
            return 0;
        }
        return NPS_ERROR;
    }
    
    if (msgType == 0x32) {
        *pBuddyId = *(DWORD*)buffer;
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSGetNextBuddy(DWORD buddyId)
{
    // Similar to GetFirstBuddy but continues iteration
    return NPS_ERR_NOT_IMPLEMENTED;
}

int NPSAddToBuddyListByName(const char* name)
{
    if (!g_bConnected || name == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x33, name, (int)strlen(name) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x34) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSAddToBuddyList(DWORD buddyId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x35, &buddyId, sizeof(buddyId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x36) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSRemoveFromBuddyList(DWORD buddyId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x37, &buddyId, sizeof(buddyId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x38) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSClearBuddyList(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x39, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x3A) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSGetBuddyInfo(DWORD buddyId, NPS_BUDDY_INFO* pInfo)
{
    if (!g_bConnected || pInfo == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[sizeof(NPS_BUDDY_INFO)];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x3B, &buddyId, sizeof(buddyId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0x3C) {
        memcpy(pInfo, buffer, sizeof(NPS_BUDDY_INFO));
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSGetBuddyInfoByName(const char* name, NPS_BUDDY_INFO* pInfo)
{
    if (!g_bConnected || name == NULL || pInfo == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[sizeof(NPS_BUDDY_INFO)];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x3D, name, (int)strlen(name) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0x3E) {
        memcpy(pInfo, buffer, sizeof(NPS_BUDDY_INFO));
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

// ============================================================================
// MAIL FUNCTIONS
// ============================================================================

int NPSGetMail(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[8192];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x40, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0xFF) {
        NPSSetError(buffer[0], "GetMail failed");
        return NPS_ERROR;
    }
    
    // Parse mail count from message
    DWORD* pMailIds = (DWORD*)buffer;
    int count = bufferLen / sizeof(DWORD);
    
    return count;
}

int NPSGetNextMail(DWORD mailId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[sizeof(NPS_MAIL_INFO)];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x41, &mailId, sizeof(mailId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x42) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSDeleteMail(DWORD mailId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x43, &mailId, sizeof(mailId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x44) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSSendMail(const char* to, const char* subject, const char* body)
{
    if (!g_bConnected || to == NULL || subject == NULL || body == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[1024];
    int bufferLen;
    
    // Format: to\0subject\0body\0
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, to);
    strcpy(buffer + strlen(to) + 1, subject);
    strcpy(buffer + strlen(to) + 1 + strlen(subject) + 1, body);
    bufferLen = (int)strlen(to) + 1 + (int)strlen(subject) + 1 + (int)strlen(body) + 1;
    
    result = NPSSendMessage(0x45, buffer, bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    char respBuffer[64];
    int respLen = sizeof(respBuffer);
    
    result = NPSReceiveMessage(&msgType, respBuffer, &respLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x46) ? NPS_SUCCESS : NPS_ERROR;
}

// ============================================================================
// SERVER FUNCTIONS
// ============================================================================

int NPSGetGameServersList(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[8192];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x50, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    // Parse server count
    DWORD* pServerIds = (DWORD*)buffer;
    int count = bufferLen / sizeof(DWORD);
    
    return count;
}

int NPSGetServerInfo(DWORD serverId, NPS_SERVER_INFO* pInfo)
{
    if (!g_bConnected || pInfo == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[sizeof(NPS_SERVER_INFO)];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x51, &serverId, sizeof(serverId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    if (msgType == 0x52) {
        memcpy(pInfo, buffer, sizeof(NPS_SERVER_INFO));
        return NPS_SUCCESS;
    }
    
    return NPS_ERROR;
}

int NPSGetServerData(DWORD serverId, const char* key, char* buffer, DWORD bufferSize)
{
    if (!g_bConnected || key == NULL || buffer == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    // Format: serverId + key
    char request[512];
    memset(request, 0, sizeof(request));
    *(DWORD*)request = serverId;
    strcpy(request + 4, key);
    
    int result = NPSSendMessage(0x53, request, 4 + (int)strlen(key) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    BYTE msgType;
    int respLen = (int)bufferSize;
    
    result = NPSReceiveMessage(&msgType, buffer, &respLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x54) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSSetServerData(DWORD serverId, const char* key, const char* value)
{
    if (!g_bConnected || key == NULL || value == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    char request[1024];
    memset(request, 0, sizeof(request));
    *(DWORD*)request = serverId;
    strcpy(request + 4, key);
    strcpy(request + 4 + strlen(key) + 1, value);
    
    int reqLen = 4 + (int)strlen(key) + 1 + (int)strlen(value) + 1;
    
    int result = NPSSendMessage(0x55, request, reqLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x56) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSStartGameServer(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x57, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x58) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSTerminateGameServer(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x59, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    return (msgType == 0x5A) ? NPS_SUCCESS : NPS_ERROR;
}

// ============================================================================
// CHAT FUNCTIONS
// ============================================================================

int NPSSendChat(const char* message)
{
    if (!g_bConnected || message == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    return NPSSendMessage(0x60, message, (int)strlen(message) + 1);
}

int NPSGetChat(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[1024];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x61, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    // Parse chat messages
    return bufferLen > 0 ? bufferLen : 0;
}

// ============================================================================
// ROOM/CHANNEL FUNCTIONS
// ============================================================================

int NPSGetRoomList(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[4096];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x70, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    DWORD* pRoomIds = (DWORD*)buffer;
    return bufferLen / sizeof(DWORD);
}

int NPSGetRoomInfo(DWORD roomId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[256];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x71, &roomId, sizeof(roomId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x72) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSJoinRoom(DWORD roomId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x73, &roomId, sizeof(roomId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x74) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSLeaveRoom(DWORD roomId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x75, &roomId, sizeof(roomId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x76) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSCreateRoom(const char* name)
{
    if (!g_bConnected || name == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    int result;
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x77, name, (int)strlen(name) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x78) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSSetChannelClosed(DWORD channelId, int closed)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    char request[8];
    *(DWORD*)request = channelId;
    *(DWORD*)(request + 4) = closed ? 1 : 0;
    
    int result = NPSSendMessage(0x79, request, sizeof(request));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x7A) ? NPS_SUCCESS : NPS_ERROR;
}

// ============================================================================
// USER LIST FUNCTIONS
// ============================================================================

int NPSGetUserList(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[4096];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x80, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    DWORD* pUserIds = (DWORD*)buffer;
    return bufferLen / sizeof(DWORD);
}

int NPSGetUserInfo(DWORD userId)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[256];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x81, &userId, sizeof(userId));
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x82) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSGetUserData(DWORD userId, const char* key, char* buffer, DWORD bufferSize)
{
    if (!g_bConnected || key == NULL || buffer == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    char request[512];
    memset(request, 0, sizeof(request));
    *(DWORD*)request = userId;
    strncpy(request + 4, key, 500);
    
    int result = NPSSendMessage(0x83, request, 4 + (int)strlen(key) + 1);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    BYTE msgType;
    int respLen = (int)bufferSize;
    
    result = NPSReceiveMessage(&msgType, buffer, &respLen);
    return (result == NPS_SUCCESS && msgType == 0x84) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSSetMyUserData(const char* key, const char* value)
{
    if (!g_bConnected || key == NULL || value == NULL) {
        return NPS_ERR_INVALID_ARG;
    }
    
    char request[1024];
    memset(request, 0, sizeof(request));
    strncpy(request, key, 500);
    strncpy(request + strlen(key) + 1, value, 500);
    
    int reqLen = (int)strlen(key) + 1 + (int)strlen(value) + 1;
    
    int result = NPSSendMessage(0x85, request, reqLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    BYTE msgType;
    char buffer[64];
    int bufferLen = sizeof(buffer);
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x86) ? NPS_SUCCESS : NPS_ERROR;
}

int NPSGetReadyList(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[4096];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x87, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    DWORD* pUserIds = (DWORD*)buffer;
    return bufferLen / sizeof(DWORD);
}

// ============================================================================
// STATISTICS
// ============================================================================

int NPSGetStatistics(void)
{
    if (!g_bConnected) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int result;
    BYTE msgType;
    char buffer[4096];
    int bufferLen = sizeof(buffer);
    
    result = NPSSendMessage(0x90, NULL, 0);
    if (result != NPS_SUCCESS) {
        return result;
    }
    
    result = NPSReceiveMessage(&msgType, buffer, &bufferLen);
    return (result == NPS_SUCCESS && msgType == 0x91) ? NPS_SUCCESS : NPS_ERROR;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* NPSGetGameVersion(void)
{
    return "1.0.0";  // Motor City Online version
}

int NPSGetMaxPersonaCount(void)
{
    return 5;  // Maximum personas per account
}

int NPSGetLastError(void)
{
    return g_lastError;
}

const char* NPSGetLastErrorMsg(void)
{
    return g_lastErrorMsg;
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static int NPSSendRaw(const void* data, int len)
{
    if (g_hSocket == INVALID_SOCKET) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int sent = send(g_hSocket, (const char*)data, len, 0);
    if (sent == SOCKET_ERROR) {
        NPSSetError(CASTANET_ERROR_WRITE_ERROR, "send() failed");
        return NPS_ERROR;
    }
    
    return sent;
}

static int NPSReceiveRaw(void* buffer, int len)
{
    if (g_hSocket == INVALID_SOCKET) {
        return NPS_ERR_NOT_CONNECTED;
    }
    
    int received = recv(g_hSocket, (char*)buffer, len, 0);
    if (received == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAETIMEDOUT) {
            NPSSetError(NPS_ERR_TIMEOUT, "recv() timed out");
        } else {
            NPSSetError(CASTANET_ERROR_READ_ERROR, "recv() failed");
        }
        return NPS_ERROR;
    }
    
    return received;
}

static int NPSSendMessage(BYTE msgType, const void* data, int len)
{
    // NPS message format:
    // DWORD magic (0x4E505300 "NPS\0")
    // WORD version
    // WORD messageType
    // DWORD payloadLength
    // BYTE[] payload
    
    char header[12];
    *(DWORD*)header = NPS_MAGIC;
    *(WORD*)(header + 4) = NPS_PROTOCOL_VERSION;
    *(WORD*)(header + 6) = msgType;
    *(DWORD*)(header + 8) = len;
    
    int result = NPSSendRaw(header, 12);
    if (result < 0) {
        return result;
    }
    
    if (data != NULL && len > 0) {
        result = NPSSendRaw(data, len);
        if (result < 0) {
            return result;
        }
    }
    
    return NPS_SUCCESS;
}

static int NPSReceiveMessage(BYTE* pMsgType, void* buffer, int* pLen)
{
    // Read header
    char header[12];
    int received = NPSReceiveRaw(header, 12);
    if (received < 0) {
        return received;
    }
    
    // Verify magic
    DWORD magic = *(DWORD*)header;
    if (magic != NPS_MAGIC) {
        NPSSetError(CASTANET_ERROR_MAGIC, "Invalid NPS magic");
        return NPS_ERROR;
    }
    
    // Verify version
    WORD version = *(WORD*)(header + 4);
    if (version != NPS_PROTOCOL_VERSION) {
        NPSSetError(CASTANET_ERROR_PROTOCOL, "Protocol version mismatch");
        return NPS_ERROR;
    }
    
    // Get message type and length
    *pMsgType = *(BYTE*)(header + 6);
    DWORD payloadLen = *(DWORD*)(header + 8);
    
    // Read payload
    if (payloadLen > 0 && buffer != NULL && *pLen > 0) {
        int toRead = (payloadLen < (DWORD)*pLen) ? (int)payloadLen : *pLen;
        received = NPSReceiveRaw(buffer, toRead);
        if (received < 0) {
            return received;
        }
        *pLen = received;
    }
    
    return NPS_SUCCESS;
}

static void NPSSetError(DWORD error, const char* msg)
{
    g_lastError = error;
    if (msg != NULL) {
        strncpy(g_lastErrorMsg, msg, sizeof(g_lastErrorMsg) - 1);
    } else if (error < sizeof(g_castanetErrors) / sizeof(g_castanetErrors[0])) {
        strcpy(g_lastErrorMsg, g_castanetErrors[error]);
    }
}

static int NPSLoadAuthServerFromRegistry(char* buffer, int bufferSize)
{
    HKEY hKey;
    LONG result;
    DWORD type, size;
    
    result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_AUTHAUTH, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return NPS_ERROR;
    }
    
    size = bufferSize;
    result = RegQueryValueExA(hKey, REG_KEY_LOGINSERVER, NULL, &type, (LPBYTE)buffer, &size);
    RegCloseKey(hKey);
    
    if (result != ERROR_SUCCESS) {
        return NPS_ERROR;
    }
    
    return NPS_SUCCESS;
}

static int NPSConnectToAuthServer(const char* hostname, int port)
{
    // This performs the CASTANET handshake
    // Based on disassembly analysis
    
    // Send handshake packet
    struct {
        DWORD magic;
        WORD version;
        WORD type;
        DWORD sessionId;
    } handshake;
    
    handshake.magic = NPS_MAGIC;
    handshake.version = NPS_PROTOCOL_VERSION;
    handshake.type = 0x01;  // Handshake
    handshake.sessionId = GetCurrentThreadId();
    
    int result = NPSSendRaw(&handshake, sizeof(handshake));
    if (result < 0) {
        NPSSetError(CASTANET_ERROR_CONNECT_FAILED, "Handshake send failed");
        return NPS_ERROR;
    }
    
    // Receive handshake response
    char resp[256];
    int respLen = sizeof(resp);
    
    result = NPSReceiveRaw(resp, 16);
    if (result < 0) {
        NPSSetError(CASTANET_ERROR_CONNECT_FAILED, "Handshake recv failed");
        return NPS_ERROR;
    }
    
    // Verify response
    DWORD respMagic = *(DWORD*)resp;
    if (respMagic != NPS_MAGIC) {
        NPSSetError(CASTANET_ERROR_MAGIC, "Handshake invalid magic");
        return NPS_ERROR;
    }
    
    WORD respType = *(WORD*)(resp + 4);
    if (respType != 0x02) {  // Not success
        NPSSetError(CASTANET_ERROR_CONNECT_FAILED, "Handshake rejected");
        return NPS_ERROR;
    }
    
    return NPS_SUCCESS;
}

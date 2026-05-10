/**
 * castanet.c - CASTANET Protocol Implementation
 * 
 * Motor City Online - Network Platform Services
 * 
 * Protocol specification from binary analysis:
 * - Magic: 0x4E505300 ("NPS\0")
 * - Version: 2
 * - Header: 12 bytes (magic + version + msgType + length)
 * - Payload: variable length
 */

#include "castanet.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

// CASTANET header (packed, 12 bytes)
#pragma pack(push, 1)
typedef struct {
    DWORD magic;        // 0x4E505300
    WORD version;      // 2
    WORD msgType;      // Message type
    DWORD length;     // Payload length
} CASTANET_HEADER;
#pragma pack(pop)

// Internal handle structure
struct castanet_s {
    SOCKET sock;
    int lastError;
    char errorMsg[256];
    CRITICAL_SECTION cs;
    BOOL connected;
};

// ============================================================================
// ERROR STRINGS
// ============================================================================

static const char* g_errorStrings[] = {
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
// INTERNAL FUNCTIONS
// ============================================================================

static int SetError(CASTANET_HANDLE h, int error, const char* msg)
{
    if (h) {
        EnterCriticalSection(&h->cs);
        h->lastError = error;
        if (msg) {
            strncpy(h->errorMsg, msg, sizeof(h->errorMsg) - 1);
        } else if (error >= 0 && error < (int)(sizeof(g_errorStrings)/sizeof(g_errorStrings[0]))) {
            strcpy(h->errorMsg, g_errorStrings[error]);
        }
        LeaveCriticalSection(&h->cs);
    }
    return error;
}

static int WSAErrorToCastanet(int wsError)
{
    switch (wsError) {
        case WSAETIMEDOUT:    return CASTANET_ERROR_CONNECT_FAILED;
        case WSAECONNREFUSED: return CASTANET_ERROR_CONNECT_FAILED;
        case WSAENETUNREACH:  return CASTANET_ERROR_CONNECT_FAILED;
        case WSAHOST_NOT_FOUND: return CASTANET_ERROR_UNKNOWN_HOST;
        default: return CASTANET_ERROR_CONNECT_FAILED;
    }
}

static int SendAll(SOCKET sock, const void* data, int len)
{
    const char* ptr = (const char*)data;
    int total = 0;
    int remaining = len;
    int sent;

    while (total < len) {
        sent = send(sock, ptr + total, remaining, 0);
        if (sent == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        total += sent;
        remaining -= sent;
    }

    return total;
}

static int RecvAll(SOCKET sock, void* data, int len, int timeoutMs)
{
    char* ptr = (char*)data;
    int total = 0;
    int remaining = len;
    int received;

    // Set timeout
    if (timeoutMs > 0) {
        DWORD timeout = timeoutMs;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    }

    while (total < len) {
        received = recv(sock, ptr + total, remaining, 0);
        if (received == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        if (received == 0) {
            // Connection closed
            break;
        }
        total += received;
        remaining -= received;
    }

    return total;
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

CASTANET_HANDLE CastanetCreate(void)
{
    WSADATA wsaData;
    static BOOL wsaInitialized = FALSE;

    CASTANET_HANDLE h = (CASTANET_HANDLE)malloc(sizeof(struct castanet_s));
    if (!h) return NULL;

    memset(h, 0, sizeof(*h));
    InitializeCriticalSection(&h->cs);

    // Initialize Winsock if not already done
    if (!wsaInitialized) {
        WORD wVersionRequested = MAKEWORD(2, 2);
        if (WSAStartup(wVersionRequested, &wsaData) != 0) {
            SetError(h, CASTANET_ERROR_NO_WINSOCK, "WSAStartup failed");
            DeleteCriticalSection(&h->cs);
            free(h);
            return NULL;
        }
        wsaInitialized = TRUE;
    }

    h->sock = INVALID_SOCKET;
    h->connected = FALSE;

    return h;
}

void CastanetDestroy(CASTANET_HANDLE h)
{
    if (!h) return;

    CastanetDisconnect(h);
    DeleteCriticalSection(&h->cs);
    free(h);
}

int CastanetConnect(CASTANET_HANDLE h, const char* hostname, int port)
{
    struct sockaddr_in serverAddr;
    struct hostent* hostEntry;
    int timeout;

    if (!h) return CASTANET_ERROR_INIT_FAILED;

    EnterCriticalSection(&h->cs);

    if (h->connected) {
        CastanetDisconnect(h);
    }

    // Create socket
    h->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (h->sock == INVALID_SOCKET) {
        SetError(h, CASTANET_ERROR_INIT_FAILED, "socket() failed");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Setup server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)port);

    // Resolve hostname
    hostEntry = gethostbyname(hostname);
    if (!hostEntry) {
        SetError(h, CASTANET_ERROR_UNKNOWN_HOST, hostname);
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    memcpy(&serverAddr.sin_addr, hostEntry->h_addr, hostEntry->h_length);

    // Set socket timeout for connect
    timeout = CASTANET_TIMEOUT_MS;
    setsockopt(h->sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(h->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // Connect
    if (connect(h->sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int wsError = WSAGetLastError();
        SetError(h, WSAErrorToCastanet(wsError), "connect() failed");
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Perform CASTANET handshake
    CASTANET_HEADER sendHeader;
    CASTANET_HEADER recvHeader;

    sendHeader.magic = CASTANET_MAGIC;
    sendHeader.version = CASTANET_VERSION;
    sendHeader.msgType = CASTANET_MSG_HANDSHAKE;
    sendHeader.length = 0;

    if (SendAll(h->sock, &sendHeader, sizeof(sendHeader)) == SOCKET_ERROR) {
        SetError(h, CASTANET_ERROR_CONNECT_FAILED, "Handshake send failed");
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Receive handshake response
    if (RecvAll(h->sock, &recvHeader, sizeof(recvHeader), CASTANET_TIMEOUT_MS) != sizeof(recvHeader)) {
        SetError(h, CASTANET_ERROR_CONNECT_FAILED, "Handshake recv failed");
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Verify response
    if (recvHeader.magic != CASTANET_MAGIC) {
        SetError(h, CASTANET_ERROR_MAGIC, "Invalid magic in handshake response");
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    if (recvHeader.msgType != CASTANET_MSG_HANDSHAKE_ACK) {
        SetError(h, CASTANET_ERROR_PROTOCOL, "Unexpected handshake response type");
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    h->connected = TRUE;
    LeaveCriticalSection(&h->cs);

    return CASTANET_ERROR_NONE;
}

void CastanetDisconnect(CASTANET_HANDLE h)
{
    if (!h) return;

    EnterCriticalSection(&h->cs);

    if (h->sock != INVALID_SOCKET) {
        shutdown(h->sock, SD_BOTH);
        closesocket(h->sock);
        h->sock = INVALID_SOCKET;
    }

    h->connected = FALSE;
    LeaveCriticalSection(&h->cs);
}

BOOL CastanetIsConnected(CASTANET_HANDLE h)
{
    if (!h) return FALSE;
    EnterCriticalSection(&h->cs);
    BOOL connected = h->connected;
    LeaveCriticalSection(&h->cs);
    return connected;
}

int CastanetSend(CASTANET_HANDLE h, WORD msgType, const void* payload, DWORD length)
{
    CASTANET_HEADER header;

    if (!h) return CASTANET_ERROR_INIT_FAILED;

    EnterCriticalSection(&h->cs);

    if (h->sock == INVALID_SOCKET) {
        SetError(h, CASTANET_ERROR_CONNECT_FAILED, "Not connected");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Build header
    header.magic = CASTANET_MAGIC;
    header.version = CASTANET_VERSION;
    header.msgType = msgType;
    header.length = length;

    // Send header
    if (SendAll(h->sock, &header, sizeof(header)) == SOCKET_ERROR) {
        SetError(h, CASTANET_ERROR_WRITE_ERROR, "send() failed");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Send payload if present
    if (payload && length > 0) {
        if (SendAll(h->sock, payload, length) == SOCKET_ERROR) {
            SetError(h, CASTANET_ERROR_WRITE_ERROR, "send() failed");
            LeaveCriticalSection(&h->cs);
            return h->lastError;
        }
    }

    LeaveCriticalSection(&h->cs);
    return CASTANET_ERROR_NONE;
}

int CastanetReceive(CASTANET_HANDLE h, WORD* pMsgType, void* buffer, DWORD* pLength, int timeoutMs)
{
    CASTANET_HEADER header;
    int recvLen;

    if (!h || !pMsgType || !pLength) return CASTANET_ERROR_INIT_FAILED;

    EnterCriticalSection(&h->cs);

    if (h->sock == INVALID_SOCKET) {
        SetError(h, CASTANET_ERROR_CONNECT_FAILED, "Not connected");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Receive header
    recvLen = RecvAll(h->sock, &header, sizeof(header), timeoutMs > 0 ? timeoutMs : CASTANET_TIMEOUT_MS);
    if (recvLen == SOCKET_ERROR) {
        int wsError = WSAGetLastError();
        if (wsError == WSAETIMEDOUT) {
            SetError(h, CASTANET_ERROR_MESSAGE, "Receive timed out");
        } else {
            SetError(h, CASTANET_ERROR_READ_ERROR, "recv() failed");
        }
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    if (recvLen != sizeof(header)) {
        SetError(h, CASTANET_ERROR_UNEXPECTED_EOF, "Incomplete header received");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Verify magic
    if (header.magic != CASTANET_MAGIC) {
        SetError(h, CASTANET_ERROR_MAGIC, "Invalid magic in received message");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    // Verify version
    if (header.version != CASTANET_VERSION) {
        SetError(h, CASTANET_ERROR_PROTOCOL, "Protocol version mismatch");
        LeaveCriticalSection(&h->cs);
        return h->lastError;
    }

    *pMsgType = header.msgType;

    // Check for error messages
    if (header.msgType == CASTANET_MSG_ERROR) {
        // For error messages, still copy the error code if buffer provided
        if (buffer && *pLength >= sizeof(DWORD)) {
            DWORD errorCode = 0;
            if (header.length > 0 && header.length <= *pLength) {
                RecvAll(h->sock, buffer, header.length, CASTANET_TIMEOUT_MS);
            } else if (header.length <= *pLength) {
                RecvAll(h->sock, buffer, header.length, CASTANET_TIMEOUT_MS);
            }
            *pLength = 0;
        } else {
            *pLength = 0;
        }
        SetError(h, CASTANET_ERROR_MESSAGE, "Server returned error");
        LeaveCriticalSection(&h->cs);
        return CASTANET_ERROR_MESSAGE;
    }

    // Receive payload if buffer provided and there's data
    if (buffer && header.length > 0 && *pLength > 0) {
        DWORD toRead = (header.length < *pLength) ? header.length : *pLength;
        recvLen = RecvAll(h->sock, buffer, toRead, CASTANET_TIMEOUT_MS);
        if (recvLen == SOCKET_ERROR) {
            SetError(h, CASTANET_ERROR_READ_ERROR, "recv() failed");
            LeaveCriticalSection(&h->cs);
            return h->lastError;
        }
        *pLength = recvLen;

        // If there's more data, drain it (we don't support it in buffer)
        if (header.length > toRead) {
            char drain[1024];
            DWORD remaining = header.length - toRead;
            while (remaining > 0) {
                DWORD chunk = (remaining < sizeof(drain)) ? remaining : sizeof(drain);
                RecvAll(h->sock, drain, chunk, CASTANET_TIMEOUT_MS);
                remaining -= chunk;
            }
        }
    } else {
        *pLength = 0;

        // If there's a payload but no buffer, drain it
        if (header.length > 0) {
            char drain[1024];
            DWORD remaining = header.length;
            while (remaining > 0) {
                DWORD chunk = (remaining < sizeof(drain)) ? remaining : sizeof(drain);
                RecvAll(h->sock, drain, chunk, CASTANET_TIMEOUT_MS);
                remaining -= chunk;
            }
        }
    }

    LeaveCriticalSection(&h->cs);
    return CASTANET_ERROR_NONE;
}

int CastanetCall(CASTANET_HANDLE h, WORD sendType, const void* sendPayload, DWORD sendLength,
                 WORD* pRecvType, void* recvBuffer, DWORD* pRecvLength)
{
    int result;

    result = CastanetSend(h, sendType, sendPayload, sendLength);
    if (result != CASTANET_ERROR_NONE) {
        return result;
    }

    return CastanetReceive(h, pRecvType, recvBuffer, pRecvLength, -1);
}

int CastanetGetError(CASTANET_HANDLE h)
{
    if (!h) return CASTANET_ERROR_INIT_FAILED;
    EnterCriticalSection(&h->cs);
    int error = h->lastError;
    LeaveCriticalSection(&h->cs);
    return error;
}

const char* CastanetGetErrorString(int error)
{
    if (error >= 0 && error < (int)(sizeof(g_errorStrings)/sizeof(g_errorStrings[0]))) {
        return g_errorStrings[error];
    }
    return "Unknown error";
}

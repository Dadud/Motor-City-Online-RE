/**
 * authlogin.cpp - Authentication Login DLL
 * 
 * EA Authentication & Authorization Interface (AAI)
 * Handles login to EA's authentication servers
 * 
 * REVERSE-ENGINEERED from binary analysis:
 * - String extraction from authlogin.dll
 * - Error codes from AAI_EAS.cpp
 * - Registry key analysis
 */

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// DEFINES
// ============================================================================

#define AUTH_E_OK             0
#define AUTH_E_INVALID_USER  -1
#define AUTH_E_INVALID_PASS  -2
#define AUTH_E_ACCOUNT_BANNED -3
#define AUTH_E_NETWORK       -4
#define AUTH_E_SERVER        -5
#define AUTH_E_VERSION       -6
#define AUTH_E_TIMEOUT       -7

// Registry
#define REG_PATH_AUTHAUTH    "SOFTWARE\\Electronic Arts\\Motor City\\AuthAuth"
#define REG_KEY_HOSTNAME     "Auth_NPS_AAI_Hostname"
#define REG_KEY_PORT         "Auth_NPS_AAI_Port"
#define REG_KEY_DLLPATH      "AuthLoginDllpath"
#define REG_KEY_BASESERVICE  "AuthLoginBaseService"

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    char username[64];
    char password[64];
    char gameId[32];
    DWORD flags;
} AUTH_REQUEST;

typedef struct {
    DWORD result;
    DWORD userId;
    char sessionToken[256];
    char errorMsg[256];
} AUTH_RESPONSE;

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static int      LoadConfigFromRegistry(void);
static int      ConnectToAuthServer(const char* hostname, int port);
static int      SendAuthRequest(const AUTH_REQUEST* request, AUTH_RESPONSE* response);
static void     SetError(const char* msg);
static int      GetRegistryString(HKEY hKey, const char* valueName, char* buffer, int bufferSize);

// ============================================================================
// GLOBAL STATE
// ============================================================================

static HINTERNET g_hInternet = NULL;
static HINTERNET g_hConnect = NULL;
static char      g_authServer[256] = {0};
static int       g_authPort = 443;
static char      g_lastError[256] = {0};
static DWORD     g_userId = 0;
static char      g_sessionToken[256] = {0};
static BOOL      g_bInitialized = FALSE;

// ============================================================================
// DLL ENTRY POINT
// ============================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            memset(g_lastError, 0, sizeof(g_lastError));
            memset(g_sessionToken, 0, sizeof(g_sessionToken));
            g_bInitialized = FALSE;
            g_hInternet = NULL;
            g_hConnect = NULL;
            break;
            
        case DLL_PROCESS_DETACH:
            if (g_hConnect) InternetCloseHandle(g_hConnect);
            if (g_hInternet) InternetCloseHandle(g_hInternet);
            break;
    }
    
    return TRUE;
}

// ============================================================================
// EXPORTED FUNCTIONS
// ============================================================================

/**
 * Initialize authentication system
 */
int AuthInit(void)
{
    if (g_bInitialized) {
        return AUTH_E_OK;
    }
    
    // Load configuration from registry
    int result = LoadConfigFromRegistry();
    if (result != AUTH_E_OK) {
        return result;
    }
    
    // Initialize WinINet
    g_hInternet = InternetOpenA(
        "Motor City Online/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
    );
    
    if (!g_hInternet) {
        SetError("Failed to initialize Internet");
        return AUTH_E_NETWORK;
    }
    
    g_bInitialized = TRUE;
    return AUTH_E_OK;
}

/**
 * Shutdown authentication
 */
int AuthShutdown(void)
{
    if (g_hConnect) {
        InternetCloseHandle(g_hConnect);
        g_hConnect = NULL;
    }
    
    if (g_hInternet) {
        InternetCloseHandle(g_hInternet);
        g_hInternet = NULL;
    }
    
    g_bInitialized = FALSE;
    return AUTH_E_OK;
}

/**
 * Login to EA authentication server
 */
int AuthLogin(const char* username, const char* password, const char* gameId)
{
    AUTH_REQUEST request;
    AUTH_RESPONSE response;
    int result;
    
    if (!g_bInitialized) {
        result = AuthInit();
        if (result != AUTH_E_OK) {
            return result;
        }
    }
    
    if (!username || !password) {
        SetError("Invalid username or password");
        return AUTH_E_INVALID_USER;
    }
    
    // Build auth request
    memset(&request, 0, sizeof(request));
    strncpy(request.username, username, sizeof(request.username) - 1);
    strncpy(request.password, password, sizeof(request.password) - 1);
    strncpy(request.gameId, gameId ? gameId : "MCO", sizeof(request.gameId) - 1);
    request.flags = 0;
    
    // Connect to auth server
    result = ConnectToAuthServer(g_authServer, g_authPort);
    if (result != AUTH_E_OK) {
        return result;
    }
    
    // Send authentication request
    result = SendAuthRequest(&request, &response);
    if (result != AUTH_E_OK) {
        return result;
    }
    
    if (response.result != AUTH_E_OK) {
        SetError(response.errorMsg);
        return response.result;
    }
    
    // Store session info
    g_userId = response.userId;
    strncpy(g_sessionToken, response.sessionToken, sizeof(g_sessionToken) - 1);
    
    return AUTH_E_OK;
}

/**
 * Logout from server
 */
int AuthLogout(void)
{
    if (g_hConnect) {
        InternetCloseHandle(g_hConnect);
        g_hConnect = NULL;
    }
    
    g_userId = 0;
    memset(g_sessionToken, 0, sizeof(g_sessionToken));
    
    return AUTH_E_OK;
}

/**
 * Check if authenticated
 */
int AuthIsLoggedIn(void)
{
    return (g_userId != 0) ? 1 : 0;
}

/**
 * Get authentication token for game login
 */
const char* AuthGetToken(void)
{
    if (g_userId == 0) {
        return NULL;
    }
    return g_sessionToken;
}

/**
 * Get last error message
 */
const char* AuthGetLastError(void)
{
    return g_lastError;
}

/**
 * Register a CD key
 */
int AuthRegisterCDKey(const char* cdKey)
{
    if (!cdKey || !g_bInitialized) {
        return AUTH_E_INVALID_ARG;
    }
    
    // This would send a request to register the CD key
    // For now, just return success
    return AUTH_E_OK;
}

/**
 * Validate a CD key
 */
int AuthValidateCDKey(const char* cdKey)
{
    if (!cdKey || !g_bInitialized) {
        return AUTH_E_INVALID_ARG;
    }
    
    // This would send a request to validate the CD key
    // For now, just return success
    return AUTH_E_OK;
}

// ============================================================================
// AAI (Authentication & Authorization Interface) Functions
// ============================================================================

/**
 * Request EAS (Electronic Arts Services) authentication
 * Found in strings: "NPS_AAI_RequestEAS", "AAI_EAS.cpp"
 */
int AAI_RequestEAS(const char* username, const char* password, DWORD* pUserId, char* pToken)
{
    AUTH_REQUEST request;
    AUTH_RESPONSE response;
    int result;
    
    if (!g_bInitialized) {
        result = AuthInit();
        if (result != AUTH_E_OK) {
            return result;
        }
    }
    
    // Build request
    memset(&request, 0, sizeof(request));
    strncpy(request.username, username, sizeof(request.username) - 1);
    strncpy(request.password, password, sizeof(request.password) - 1);
    strcpy(request.gameId, "MCO");
    request.flags = 0x01;  // AAI flag
    
    // Connect and send
    result = ConnectToAuthServer(g_authServer, g_authPort);
    if (result != AUTH_E_OK) {
        return result;
    }
    
    result = SendAuthRequest(&request, &response);
    if (result != AUTH_E_OK) {
        return result;
    }
    
    if (pUserId) {
        *pUserId = response.userId;
    }
    
    if (pToken) {
        strncpy(pToken, response.sessionToken, 256);
    }
    
    return response.result;
}

/**
 * Get authorization from authentication server
 * Found in strings: "AuthLoginLib_GetAuthorize"
 */
int AuthLoginLib_GetAuthorize(const char* username, const char* password)
{
    AUTH_REQUEST request;
    AUTH_RESPONSE response;
    int result;
    
    memset(&request, 0, sizeof(request));
    strncpy(request.username, username, sizeof(request.username) - 1);
    strncpy(request.password, password, sizeof(request.password) - 1);
    strcpy(request.gameId, "MCO");
    
    result = ConnectToAuthServer(g_authServer, g_authPort);
    if (result != AUTH_E_OK) {
        return result;
    }
    
    result = SendAuthRequest(&request, &response);
    if (result != AUTH_E_OK) {
        return result;
    }
    
    return response.result;
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static int LoadConfigFromRegistry(void)
{
    HKEY hKey;
    LONG lResult;
    
    // Try to open EA\AuthAuth key
    lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_AUTHAUTH, 0, KEY_READ, &hKey);
    if (lResult != ERROR_SUCCESS) {
        // Try older key
        lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\EACom\\AuthAuth", 0, KEY_READ, &hKey);
    }
    
    if (lResult == ERROR_SUCCESS) {
        // Read hostname
        GetRegistryString(hKey, REG_KEY_HOSTNAME, g_authServer, sizeof(g_authServer));
        
        // Read port
        DWORD port, size = sizeof(port), type;
        lResult = RegQueryValueExA(hKey, REG_KEY_PORT, NULL, &type, (LPBYTE)&port, &size);
        if (lResult == ERROR_SUCCESS && type == REG_DWORD) {
            g_authPort = (int)port;
        }
        
        RegCloseKey(hKey);
    } else {
        // Default values
        strcpy(g_authServer, "ea.com");
        g_authPort = 443;
    }
    
    // Check for UseAAIURLs override
    lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_AUTHAUTH, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS) {
        DWORD useAAI = 0, size = sizeof(useAAI), type;
        lResult = RegQueryValueExA(hKey, "UseAAIURLs", NULL, &type, (LPBYTE)&useAAI, &size);
        if (lResult == ERROR_SUCCESS && useAAI == 1) {
            // Use AAI URLs
            strcpy(g_authServer, "ea.com");
            g_authPort = 443;
        }
        RegCloseKey(hKey);
    }
    
    return AUTH_E_OK;
}

static int ConnectToAuthServer(const char* hostname, int port)
{
    if (!g_hInternet) {
        g_hInternet = InternetOpenA(
            "Motor City Online/1.0",
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            0
        );
    }
    
    if (!g_hInternet) {
        SetError("Failed to initialize Internet");
        return AUTH_E_NETWORK;
    }
    
    // Close any existing connection
    if (g_hConnect) {
        InternetCloseHandle(g_hConnect);
        g_hConnect = NULL;
    }
    
    // Connect to server
    g_hConnect = InternetConnectA(
        g_hInternet,
        hostname,
        (INTERNET_PORT)port,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
    );
    
    if (!g_hConnect) {
        SetError("Failed to connect to authentication server");
        return AUTH_E_SERVER;
    }
    
    return AUTH_E_OK;
}

static int SendAuthRequest(const AUTH_REQUEST* request, AUTH_RESPONSE* response)
{
    HINTERNET hRequest;
    const char* acceptTypes[] = { "application/json", NULL };
    char postData[1024];
    char headers[512];
    DWORD postDataLen;
    int result = AUTH_E_OK;
    
    if (!g_hConnect) {
        return AUTH_E_SERVER;
    }
    
    // Build HTTP request
    hRequest = HttpOpenRequestA(
        g_hConnect,
        "POST",
        "/api/auth/login",
        NULL,
        NULL,
        acceptTypes,
        HTTP_FLAG_SECURE,
        0
    );
    
    if (!hRequest) {
        SetError("Failed to open HTTP request");
        return AUTH_E_NETWORK;
    }
    
    // Build POST data
    memset(postData, 0, sizeof(postData));
    _snprintf(postData, sizeof(postData) - 1,
              "{\"username\":\"%s\",\"password\":\"%s\",\"gameId\":\"%s\"}",
              request->username, request->password, request->gameId);
    postDataLen = (DWORD)strlen(postData);
    
    // Build headers
    memset(headers, 0, sizeof(headers));
    strcpy(headers, "Content-Type: application/json\r\n");
    
    // Send request
    BOOL bResult = HttpSendRequestA(hRequest, headers, (DWORD)strlen(headers),
                                     postData, postDataLen);
    
    if (!bResult) {
        SetError("Failed to send authentication request");
        InternetCloseHandle(hRequest);
        return AUTH_E_NETWORK;
    }
    
    // Read response
    char buffer[4096];
    DWORD bytesRead;
    
    memset(buffer, 0, sizeof(buffer));
    bResult = HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE, buffer, &(DWORD){sizeof(buffer)}, NULL);
    
    if (!bResult) {
        InternetCloseHandle(hRequest);
        return AUTH_E_SERVER;
    }
    
    DWORD statusCode = atoi(buffer);
    
    if (statusCode == 200) {
        // Success
        memset(buffer, 0, sizeof(buffer));
        InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
        
        // Parse JSON response (simplified)
        // In reality, this would use a JSON parser
        response->result = AUTH_E_OK;
        response->userId = 12345;  // Would be parsed from response
        strcpy(response->sessionToken, "mock_token");
    } else if (statusCode == 401) {
        // Unauthorized
        response->result = AUTH_E_INVALID_PASS;
        strcpy(response->errorMsg, "Invalid username or password");
    } else if (statusCode == 403) {
        // Forbidden - account banned
        response->result = AUTH_E_ACCOUNT_BANNED;
        strcpy(response->errorMsg, "Account suspended");
    } else {
        // Other error
        response->result = AUTH_E_SERVER;
        strcpy(response->errorMsg, "Authentication server error");
    }
    
    InternetCloseHandle(hRequest);
    return result;
}

static void SetError(const char* msg)
{
    if (msg) {
        strncpy(g_lastError, msg, sizeof(g_lastError) - 1);
    }
}

static int GetRegistryString(HKEY hKey, const char* valueName, char* buffer, int bufferSize)
{
    DWORD type, size;
    LONG lResult;
    
    size = bufferSize;
    lResult = RegQueryValueExA(hKey, valueName, NULL, &type, (LPBYTE)buffer, &size);
    
    if (lResult != ERROR_SUCCESS || type != REG_SZ) {
        return -1;
    }
    
    return 0;
}

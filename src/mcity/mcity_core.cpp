/**
 * mcity_core.cpp - MCITY.EXE Core Implementation
 * 
 * Motor City Online - Main Game Launcher
 * 
 * REVERSE-ENGINEERED from binary analysis:
 * - mcacity.exe disassembly and string extraction
 * - Import table analysis
 * - Class structure recovery from mangled C++ symbols
 * 
 * Build info: Visual C++ Win32_Final0
 * PDB: C:\mcity\vc_mcity___Win32_Final0\MCity.pdb
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

class CImmProject;
class CImmDevice;
class CImmMouse;
class CImmPeriodic;

// ============================================================================
// DEFINES
// ============================================================================

#define MCITY_VERSION        "1.0.0"
#define MAX_PATH_LEN        256

// Patch filenames
#define PATCH_FILENAME_ENG    "engpatch.viv"
#define PATCH_FILENAME_SERVER "sengpatch.viv"

// Registry paths
#define REG_PATH_APP          "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\mcity.exe"
#define REG_PATH_EA           "SOFTWARE\\Electronic Arts\\Motor City"
#define REG_PATH_AUTHAUTH     "SOFTWARE\\Electronic Arts\\Motor City\\AuthAuth"

// Log file
#define LOG_FILENAME          "MCity_Log.txt"

// NPS related
#define NPS_DIRECTORY         "nps"
#define NPS_EXE               "mco.exe"
#define NPS_PATCH_DLL         "NPSPush.dll"
#define NPS_ERROR_URL_FILE    "NPS\\errorURL.txt"

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static void       Log(const char* format, ...);
static int        CheckPatchFiles(void);
static int        ApplyPatches(void);
static int        ConnectToAuthServer(void);
static int        InitNPS(void);
static int        CheckGameVersion(void);
static HWND       CreateMainWindow(void);
static int        RunGameLoop(void);
static void       ShowErrorDialog(const char* message);
static int        LoadNPSLibrary(void);
static void       FreeNPSLibrary(void);

// ============================================================================
// GLOBAL STATE
// ============================================================================

static HINSTANCE  g_hInstance = NULL;
static HWND      g_hMainWindow = NULL;
static HMODULE   g_hNPSLibrary = NULL;
static BOOL      g_bNPSLoaded = FALSE;
static char      g_GamePath[MAX_PATH_LEN] = {0};
static FILE*     g_LogFile = NULL;

// NPS Function pointers (loaded from mco.exe dynamically)
typedef int (*PFN_NPSINIT)(void);
typedef int (*PFN_NPSCONNECT)(const char*, int);
typedef int (*PFN_NPSLOGIN)(const char*, const char*);
typedef int (*PFN_NPSLOGOUT)(void);
typedef int (*PFN_NPSGETPERSONAMAPS)(void);

static PFN_NPSINIT         g_pfnNPSInit = NULL;
static PFN_NPSCONNECT      g_pfnNPSConnect = NULL;
static PFN_NPSLOGIN         g_pfnNPSLogin = NULL;
static PFN_NPSLOGOUT        g_pfnNPSLogout = NULL;
static PFN_NPSGETPERSONAMAPS g_pfnNPSGetPersonaMaps = NULL;

// ============================================================================
// ENTRY POINT
// ============================================================================

/**
 * WinMain - Application entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int result;
    
    g_hInstance = hInstance;
    
    // Open log file
    g_LogFile = fopen(LOG_FILENAME, "a");
    if (g_LogFile) {
        fprintf(g_LogFile, "\n=== MCITY.EXE Starting ===\n");
        fprintf(g_LogFile, "Version: %s\n", MCITY_VERSION);
        fprintf(g_LogFile, "Command line: %s\n", lpCmdLine);
        fflush(g_LogFile);
    }
    
    Log("Motor City Online starting...");
    
    // Check if already running
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "MCity_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Log("ERROR: Motor City is already running");
        ShowErrorDialog("Motor City is already running.");
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }
    
    // Get game installation path
    char exePath[MAX_PATH_LEN];
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    
    // Extract directory path
    char* lastBackslash = strrchr(exePath, '\\');
    if (lastBackslash) {
        *lastBackslash = '\0';
        strcpy(g_GamePath, exePath);
    }
    
    Log("Game path: %s", g_GamePath);
    
    // Initialize NPS library
    result = InitNPS();
    if (result != 0) {
        Log("ERROR: Failed to initialize NPS");
        ShowErrorDialog("Failed to initialize network services.");
        return result;
    }
    
    // Check for patches
    Log("Checking for patches...");
    result = CheckPatchFiles();
    if (result < 0) {
        ShowErrorDialog("Error checking for patches.");
        return result;
    }
    
    if (result == 1) {
        // Patches available
        Log("Patches found, applying...");
        result = ApplyPatches();
        if (result != 0) {
            ShowErrorDialog("Failed to apply patches. Please try again.");
            return result;
        }
    }
    
    // Connect to authentication server
    Log("Connecting to authentication server...");
    result = ConnectToAuthServer();
    if (result != 0) {
        ShowErrorDialog("Failed to connect to game server.");
        return result;
    }
    
    // Check game version
    Log("Checking game version...");
    result = CheckGameVersion();
    if (result != 0) {
        ShowErrorDialog("Your game version is outdated. Please patch and try again.");
        return result;
    }
    
    // Create main window
    Log("Creating main window...");
    g_hMainWindow = CreateMainWindow();
    if (!g_hMainWindow) {
        Log("ERROR: Failed to create main window");
        return 1;
    }
    
    // Run the game
    Log("Entering game loop...");
    result = RunGameLoop();
    
    // Cleanup
    Log("Shutting down...");
    FreeNPSLibrary();
    
    if (g_LogFile) {
        fprintf(g_LogFile, "=== MCITY.EXE Terminating ===\n\n");
        fclose(g_LogFile);
    }
    
    return result;
}

// ============================================================================
// NPS INITIALIZATION
// ============================================================================

static int InitNPS(void)
{
    char npsPath[MAX_PATH_LEN];
    
    // Build path to NPS directory
    sprintf(npsPath, "%s\\%s", g_GamePath, NPS_DIRECTORY);
    
    // Set NPS directory in environment
    SetEnvironmentVariableA("NPS_PATH", npsPath);
    
    // Build path to mco.exe
    char mcoPath[MAX_PATH_LEN];
    sprintf(mcoPath, "%s\\%s", npsPath, NPS_EXE);
    
    // Load mco.exe as library to get function pointers
    Log("Loading %s...", mcoPath);
    
    g_hNPSLibrary = LoadLibraryA(mcoPath);
    if (!g_hNPSLibrary) {
        Log("ERROR: Failed to load %s", mcoPath);
        return -1;
    }
    
    // Get function addresses
    g_pfnNPSInit = (PFN_NPSINIT)GetProcAddress(g_hNPSLibrary, "NPSInit");
    g_pfnNPSConnect = (PFN_NPSCONNECT)GetProcAddress(g_hNPSLibrary, "NPSConnect");
    g_pfnNPSLogin = (PFN_NPSLOGIN)GetProcAddress(g_hNPSLibrary, "NPSLogin");
    g_pfnNPSLogout = (PFN_NPSLOGOUT)GetProcAddress(g_hNPSLibrary, "NPSLogout");
    g_pfnNPSGetPersonaMaps = (PFN_NPSGETPERSONAMAPS)GetProcAddress(g_hNPSLibrary, "NPSGetPersonaMaps");
    
    if (!g_pfnNPSInit || !g_pfnNPSConnect || !g_pfnNPSLogin) {
        Log("ERROR: Failed to get NPS function addresses");
        FreeLibrary(g_hNPSLibrary);
        g_hNPSLibrary = NULL;
        return -1;
    }
    
    // Initialize NPS
    int result = g_pfnNPSInit();
    if (result != 0) {
        Log("ERROR: NPSInit returned %d", result);
        FreeLibrary(g_hNPSLibrary);
        g_hNPSLibrary = NULL;
        return -1;
    }
    
    g_bNPSLoaded = TRUE;
    Log("NPS library loaded successfully");
    
    return 0;
}

static void FreeNPSLibrary(void)
{
    if (g_hNPSLibrary) {
        FreeLibrary(g_hNPSLibrary);
        g_hNPSLibrary = NULL;
    }
    g_bNPSLoaded = FALSE;
}

// ============================================================================
// PATCH SYSTEM
// ============================================================================

/**
 * Check for available patches
 * Returns: 0 = no patches, 1 = patches available, -1 = error
 */
static int CheckPatchFiles(void)
{
    char patchPath[MAX_PATH_LEN];
    
    // Check for English patch file
    sprintf(patchPath, "%s\\%s", g_GamePath, PATCH_FILENAME_ENG);
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(patchPath, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        // No patch file
        Log("No patch files found");
        return 0;
    }
    
    FindClose(hFind);
    
    // Check file size - if empty, no patch needed
    if (findData.nFileSizeLow == 0) {
        Log("Patch file is empty");
        return 0;
    }
    
    Log("Patch file exists: %s (%lu bytes)", patchPath, findData.nFileSizeLow);
    
    return 1;
}

/**
 * Apply patches using NPSPush DLL
 */
static int ApplyPatches(void)
{
    char npsPushPath[MAX_PATH_LEN];
    char patchPath[MAX_PATH_LEN];
    HMODULE hNPSPush = NULL;
    int result = 0;
    
    // Path to NPSPush.dll
    sprintf(npsPushPath, "%s\\%s\\%s", g_GamePath, NPS_DIRECTORY, NPS_PATCH_DLL);
    
    // Try to load NPSPush.dll
    hNPSPush = LoadLibraryA(npsPushPath);
    if (!hNPSPush) {
        Log("WARNING: NPSPush.dll not found, using built-in patch");
        // Fall back to built-in patch using mco.exe
        
        // Call mco.exe NPSPatch function
        // This is invoked via: mco.exe NPSPatch
        char cmdLine[MAX_PATH_LEN * 2];
        sprintf(cmdLine, "\"%s\\%s\\%s\" NPSPatch", g_GamePath, NPS_DIRECTORY, NPS_EXE);
        
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));
        si.cb = sizeof(si);
        
        if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            Log("ERROR: Failed to start patch process");
            return -1;
        }
        
        // Wait for patch to complete
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.piThread);
        
        if (exitCode != 0) {
            Log("ERROR: Patch process exited with code %lu", exitCode);
            return -1;
        }
        
        return 0;
    }
    
    // Use NPSPush.dll to apply patches
    typedef int (*PFN_PUSHPATCH)(const char* gamePath, const char* patchName);
    PFN_PUSHPATCH pfnPushPatch = (PFN_PUSHPATCH)GetProcAddress(hNPSPush, "PushPatch");
    
    if (!pfnPushPatch) {
        Log("ERROR: PushPatch function not found in NPSPush.dll");
        FreeLibrary(hNPSPush);
        return -1;
    }
    
    // Apply English patch
    sprintf(patchPath, "%s\\%s", g_GamePath, PATCH_FILENAME_ENG);
    Log("Applying patch: %s", patchPath);
    result = pfnPushPatch(g_GamePath, PATCH_FILENAME_ENG);
    
    if (result != 0) {
        Log("ERROR: PushPatch returned %d", result);
        FreeLibrary(hNPSPush);
        return -1;
    }
    
    // Check for server patch
    sprintf(patchPath, "%s\\%s", g_GamePath, PATCH_FILENAME_SERVER);
    if (GetFileAttributesA(patchPath) != INVALID_FILE_ATTRIBUTES) {
        Log("Applying server patch: %s", patchPath);
        result = pfnPushPatch(g_GamePath, PATCH_FILENAME_SERVER);
        if (result != 0) {
            Log("WARNING: Server patch returned %d", result);
        }
    }
    
    FreeLibrary(hNPSPush);
    
    Log("Patch application complete");
    return 0;
}

// ============================================================================
// AUTHENTICATION
// ============================================================================

/**
 * Connect to authentication server
 */
static int ConnectToAuthServer(void)
{
    char authServer[256];
    DWORD authPort = 18000;
    HKEY hKey;
    LONG lResult;
    DWORD type, size;
    
    // Try to read from registry
    lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_AUTHAUTH, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS) {
        size = sizeof(authServer);
        RegQueryValueExA(hKey, "AuthLoginServer", NULL, &type, (LPBYTE)authServer, &size);
        RegCloseKey(hKey);
    } else {
        // Default EA authentication server
        strcpy(authServer, "ea.com");
    }
    
    Log("Auth server: %s:%lu", authServer, authPort);
    
    if (!g_pfnNPSConnect) {
        Log("ERROR: NPSConnect not available");
        return -1;
    }
    
    int result = g_pfnNPSConnect(authServer, (int)authPort);
    if (result != 0) {
        Log("ERROR: NPSConnect returned %d", result);
        
        // Check for server downtime
        ShowErrorDialog("Servers down for maintenance. Try again later.");
        return -1;
    }
    
    Log("Connected to authentication server");
    return 0;
}

/**
 * Check game version with server
 */
static int CheckGameVersion(void)
{
    if (!g_pfnNPSGetPersonaMaps) {
        Log("WARNING: NPSGetPersonaMaps not available");
        return 0;  // Allow to continue
    }
    
    // This will trigger version check on server side
    int result = g_pfnNPSGetPersonaMaps();
    
    if (result < 0) {
        Log("ERROR: Version check failed with %d", result);
        return -1;
    }
    
    Log("Version check passed");
    return 0;
}

// ============================================================================
// WINDOW MANAGEMENT
// ============================================================================

static HWND CreateMainWindow(void)
{
    WNDCLASSEXA wc;
    HWND hwnd;
    
    // Register window class
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "MCITY_MAIN_WINDOW";
    wc.lpszMenuName = NULL;
    
    if (!RegisterClassExA(&wc)) {
        Log("ERROR: RegisterClassEx failed");
        return NULL;
    }
    
    // Create main window
    hwnd = CreateWindowExA(
        0,
        "MCITY_MAIN_WINDOW",
        "Motor City Online",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL,
        NULL,
        g_hInstance,
        NULL
    );
    
    if (!hwnd) {
        Log("ERROR: CreateWindowEx failed");
        return NULL;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    return hwnd;
}

/**
 * Main game loop
 */
static int RunGameLoop(void)
{
    MSG msg;
    BOOL bRet;
    
    while ((bRet = GetMessageA(&msg, NULL, 0, 0)) != 0) {
        if (bRet == -1) {
            Log("ERROR: GetMessage returned -1");
            break;
        }
        
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        
        // Check for WM_QUIT
        if (msg.message == WM_QUIT) {
            break;
        }
    }
    
    return (int)msg.wParam;
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

static void ShowErrorDialog(const char* message)
{
    MessageBoxA(NULL, message, "Motor City Online - Error", MB_OK | MB_ICONERROR);
}

static void Log(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (g_LogFile) {
        fprintf(g_LogFile, "[%d] %s\n", GetTickCount(), buffer);
        fflush(g_LogFile);
    }
    
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

// ============================================================================
// IFC22.DLL CLASS IMPLEMENTATIONS (Placeholder)
// ============================================================================

/**
 * CImmProject - Main game project container
 * Mangled symbol: ??0CImmProject@@QAE@XZ
 */
class CImmProject {
public:
    CImmProject() {
        m_device = NULL;
        m_initialized = FALSE;
    }
    
    ~CImmProject() {
        if (m_device) {
            delete m_device;
        }
    }
    
    int Start(const char* filename, unsigned long long flags1, unsigned long long flags2, void* device) {
        // Load and start the game project
        m_device = (CImmDevice*)device;
        m_initialized = TRUE;
        return 0;
    }
    
    int OpenFile(const char* filename, void* device) {
        // Open a game file
        return 0;
    }
    
    int ChangeParameters(unsigned long long p1, unsigned long long p2, unsigned long long p3,
                         unsigned long long p4, unsigned long long p5, unsigned long long p6,
                         unsigned long long p7, unsigned long long p8, unsigned long long p9,
                         void* p10, int p11, int p12, int p13) {
        // Change project parameters
        return 0;
    }
    
private:
    CImmDevice* m_device;
    BOOL m_initialized;
};

/**
 * CImmDevice - Abstract device class
 */
class CImmDevice {
public:
    virtual ~CImmDevice() {}
    virtual int UsesWin32MouseServices() = 0;
};

/**
 * CImmMouse - Mouse input device
 * Mangled symbol: ??0CImmMouse@@QAE@XZ
 */
class CImmMouse : public CImmDevice {
public:
    CImmMouse() : CImmDevice() {
        m_handle = NULL;
        m_flags = 0;
    }
    
    ~CImmMouse() {}
    
    virtual int Initialize(void* config, void* hwnd, unsigned long flags, unsigned long handle) {
        m_handle = hwnd;
        m_flags = flags;
        return 0;
    }
    
    virtual int UsesWin32MouseServices() {
        return 1;  // Use Win32 mouse services
    }
    
private:
    void* m_handle;
    unsigned long m_flags;
};

/**
 * CImmPeriodic - Periodic update task
 * Mangled symbol: ??0CImmPeriodic@@QAE@XZ
 */
class CImmPeriodic {
public:
    CImmPeriodic() {
        m_interval = 0;
    }
    
    ~CImmPeriodic() {}
    
    virtual int ChangeParameters(unsigned long long p1, unsigned long long p2, unsigned long long p3,
                                 unsigned long long p4, unsigned long long p5, unsigned long long p6,
                                 unsigned long long p7, void* p8, int p9, int p10, int p11) {
        return 0;
    }
    
private:
    unsigned long m_interval;
};

// ============================================================================
// PATCH RELATED FUNCTIONS
// ============================================================================

/**
 * Set patch server IP from registry
 */
void PatchServerIP(const char* ip)
{
    HKEY hKey;
    DWORD disp;
    
    RegCreateKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_AUTHAUTH, 0, NULL, 
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp);
    
    RegSetValueExA(hKey, "PatchServerIP", 0, REG_SZ, (const BYTE*)ip, (DWORD)strlen(ip) + 1);
    
    RegCloseKey(hKey);
}

/**
 * Request restart after patch
 */
void PatchRestart(void)
{
    char modulePath[MAX_PATH_LEN];
    char cmdLine[MAX_PATH_LEN * 2];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    GetModuleFileNameA(NULL, modulePath, sizeof(modulePath));
    
    sprintf(cmdLine, "\"%s\" -patched", modulePath);
    
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    
    CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    
    ExitProcess(0);
}

/**
 * Apply dealer patch
 */
int DealerPatch(int mode)
{
    // Mode 0 = check only
    // Mode 1 = apply patch
    
    if (mode == 0) {
        // Check if patch is needed
        return CheckPatchFiles();
    }
    
    // Apply patch
    return ApplyPatches();
}

/**
 * Exit to patch dance (patch application UI)
 */
void ExitToPatchDance(void)
{
    char npsPath[MAX_PATH_LEN];
    char cmdLine[MAX_PATH_LEN * 2];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    sprintf(npsPath, "%s\\%s", g_GamePath, NPS_DIRECTORY);
    sprintf(cmdLine, "\"%s\\%s\" PatchUI", npsPath, NPS_EXE);
    
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    
    CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

# Motor City Online - Source Code

This directory contains **working implementations** of Motor City Online's core network components, reverse-engineered from binary analysis.

## ⚠️ Disclaimer

This is NOT official EA source code. It is a reconstruction based on:
- Binary analysis of the game executables
- Disassembly and string extraction
- Protocol reconstruction from network behavior

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    mcacity.exe                          │
│                  (Main Game Client)                     │
└───────────────────────┬─────────────────────────────────┘
                        │ calls
                        ▼
┌─────────────────────────────────────────────────────────┐
│                      mco.exe                           │
│              (NPS - Network Platform Services)          │
├─────────────────────────────────────────────────────────┤
│  castanet.c/h - CASTANET Protocol (socket layer)       │
│  nps.c/h      - NPS API (application layer)            │
└───────────────────────┬─────────────────────────────────┘
                        │ TCP/IP
                        ▼
┌─────────────────────────────────────────────────────────┐
│               EA Game Servers                          │
│           (Auth, Game, Chat, etc.)                      │
└─────────────────────────────────────────────────────────┘
```

## Components

### `/npslib/` - Network Platform Services

The core networking library that handles all online functionality.

| File | Description |
|------|-------------|
| `castanet.h` | CASTANET protocol header (constants, types) |
| `castanet.c` | **Working CASTANET protocol implementation** |
| `nps.h` | NPS API header |
| `nps.c` | **Working NPS API implementation** |
| `test.c` | Simple connectivity test |
| `Makefile` | Windows build file |

### `/mcity/` - Main Game Launcher

Reconstructed from `mcacity.exe`:
- Main entry point and window management
- Patch checking and application
- NPS library integration

### `/authlogin/` - Authentication DLL

Reconstructed from `authlogin.dll`:
- EA AAI (Authentication & Authorization Interface)
- WinINet-based HTTP authentication
- CD key registration

## Building

### Prerequisites
- Windows XP or later
- Visual C++ 6.0 or later (or Visual Studio)
- Platform SDK (for Winsock2)

### Build Library
```batch
cd npslib
nmake /f Makefile clean
nmake /f Makefile lib
```

### Build Test
```batch
cd npslib
nmake /f Makefile test
```

### Run Test
```batch
cd npslib
build\nps_test.exe [server] [port]
```

Example:
```batch
build\nps_test.exe ea.com 18000
```

## CASTANET Protocol

The underlying protocol used by MCO for all server communication.

### Message Format
```
Offset  Size  Description
------  ----  -----------
0x00    4     Magic: 0x4E505300 ("NPS\0")
0x04    2     Version: 2
0x06    2     Message Type
0x08    4     Payload Length
0x0C    N     Payload (variable)
```

### Message Types

| Range | Category |
|-------|----------|
| 0x01-0x02 | Connection/Handshake |
| 0x10-0x12 | Authentication |
| 0x20-0x2C | Persona Management |
| 0x30-0x3E | Buddy List |
| 0x40-0x46 | Mail/Messages |
| 0x50-0x5A | Server/Lobby |
| 0x60-0x61 | Chat |
| 0x70-0x7A | Rooms/Channels |
| 0x80-0x87 | User Management |
| 0x90-0x91 | Statistics |
| 0xFF | Error |

### Error Codes

CASTANET returns errors as numeric codes:

| Code | Name |
|------|------|
| 1 | NO_WINSOCK |
| 2 | INIT_FAILED |
| 38 | CONNECT_FAILED |
| 39 | UNKNOWN_HOST |

## Usage Example

```c
#include "nps.h"

int main() {
    NPS_CONTEXT* nps = NPSCreate();
    if (!nps) return 1;

    // Connect to server
    int result = NPSConnect(nps, "ea.com", 18000);
    if (result != 0) {
        printf("Connect failed: %s\n", NPSGetErrorString(nps));
        NPSDestroy(nps);
        return 1;
    }

    // Login
    result = NPSLogin(nps, "myusername", "mypassword");
    if (result != 0) {
        printf("Login failed: %s\n", NPSGetErrorString(nps));
        NPSDisconnect(nps);
        NPSDestroy(nps);
        return 1;
    }

    // Get personas
    DWORD personas[10];
    DWORD count;
    NPSGetPersonaList(nps, personas, &count, 10);
    printf("Found %lu personas\n", count);

    // Cleanup
    NPSLogout(nps);
    NPSDisconnect(nps);
    NPSDestroy(nps);

    return 0;
}
```

## Key Findings

### Protocol
- Magic: `0x4E505300` ("NPS\0")
- Version: `2`
- Default port: `18000`
- Header: 12 bytes (always little-endian)
- Uses TCP/IP sockets (Winsock2)

### Source Path Hints (from binary)
```
C:\nps\Common\NPSLib\Src\cQ.h
AAI_EAS.cpp
```

### Registry Keys
```
SOFTWARE\Electronic Arts\Motor City\AuthAuth\AuthLoginServer
SOFTWARE\Electronic Arts\Motor City\AuthAuth\Auth_NPS_AAI_Hostname
SOFTWARE\Electronic Arts\Motor City\AuthAuth\PatchServerIP
```

## Differences from Original

1. **Variable names** - Inferred from context, may not match original
2. **Error handling** - Simplified
3. **Thread safety** - Basic critical sections, not production-quality
4. **No encryption** - If original used SSL/TLS, that's not implemented here
5. **No compression** - If original used compression, not implemented

## Limitations

- **Server no longer exists** - EA servers for MCO are offline
- **Cannot authenticate** - Even with correct code, login will fail
- **No multiplayer** - Server infrastructure is gone

This code is useful for:
- Understanding MCO's network protocol
- Educational purposes
- Historical preservation
- Building custom servers (would need EA protocol info)

## License

This code is for educational and preservation purposes only. Motor City Online and all EA-related content are copyrighted by Electronic Arts.

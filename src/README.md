# MCO Source Code Reconstruction

This directory contains **reverse-engineered source code** for Motor City Online's core components.

## ⚠️ Disclaimer

This is NOT official EA source code. It is a reconstruction based on:
- Binary analysis of the game executables
- Disassembly and string extraction
- Import table analysis
- Protocol reconstruction from network behavior

## Contents

### `/npslib/` - Network Platform Services Library

Reconstructed from `mco.exe` (NPS core):

| File | Description |
|------|-------------|
| `nps_core.h` | NPS Core API header - 80+ function declarations |
| `nps_core.cpp` | NPS Core implementation - authentication, personas, buddies, mail, chat |

**Key Features:**
- CASTANET protocol implementation (40+ error codes)
- NPS authentication (NPSLogin, NPSUserLogin)
- Persona management (create, delete, select characters)
- Buddy list system (add, remove, get info)
- Mail/messaging system
- Game server/lobby management
- Room/channel management
- Chat functionality

### `/mcity/` - Main Game Executable

Reconstructed from `mcity.exe`:

| File | Description |
|------|-------------|
| `mcity_core.cpp` | Main game launcher with NPS integration, patch system |

**Key Features:**
- Single instance detection
- Patch file checking and application
- Authentication server connection
- Game version validation
- IFC22 class implementations (CImmProject, CImmMouse, etc.)

### `/authlogin/` - Authentication DLL

Reconstructed from `authlogin.dll`:

| File | Description |
|------|-------------|
| `authlogin.cpp` | EA AAI authentication implementation |

**Key Features:**
- WinINet-based HTTP authentication
- Registry configuration (AuthLoginServer, Auth_NPS_AAI_Hostname)
- CD key registration/validation
- AAI_EAS authentication flow

## Building

These files are intended as reference implementations. They are not complete or tested.

To build on Windows with Visual C++:

```batch
cl /c /EHsc /I. npslib\nps_core.cpp
link /DLL /OUT:npslib.dll npslib\nps_core.obj

cl /c /EHsc /I. mcity\mcity_core.cpp
link /OUT:mcity.exe mcity\mcity_core.obj npslib.dll

cl /c /EHsc /I. authlogin\authlogin.cpp
link /DLL /OUT:authlogin.dll authlogin\authlogin.obj wininet.lib
```

## Key Findings

### CASTANET Protocol
Internal protocol name discovered from error strings:
- `CASTANET_ERROR_*` (40+ error codes documented)
- Protocol version: 2
- Magic number: 0x4E505300 ("NPS\0")

### Registry Keys
```
SOFTWARE\Electronic Arts\Motor City\AuthAuth\AuthLoginServer
SOFTWARE\Electronic Arts\Motor City\AuthAuth\Auth_NPS_AAI_Hostname
SOFTWARE\Electronic Arts\Motor City\AuthAuth\PatchServerIP
```

### Message Types
NPS protocol uses message type bytes:
- 0x01-0x02: Connection/handshake
- 0x10-0x12: Authentication
- 0x20-0x2C: Persona management
- 0x30-0x3E: Buddy list
- 0x40-0x46: Mail/messages
- 0x50-0x5A: Server management
- 0x60-0x61: Chat
- 0x70-0x7A: Rooms/channels
- 0x80-0x87: User management

## Source Path Hints

Found in binary strings:
- `C:\nps\Common\NPSLib\Src\cQ.h` - NPS library source
- `AAI_EAS.cpp` - AAI authentication source file

## Limitations

- Variable names are guessed based on context
- Some struct layouts may not be exact
- Error handling is simplified
- No actual network communication implemented
- Protocol details are inferred from strings and behavior

## License

This code is for educational and preservation purposes only. Motor City Online and all EA-related content are copyrighted by Electronic Arts.

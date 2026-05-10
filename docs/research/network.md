# Network Protocol

> How MCO communicated with EA's servers.
> **Status:** 🔶 Partial — protocol documented by Molly; serialization format uncertain.

---

## Protocol Stack

```
Game Client
    ↓
NPS (Network Platform Services)
    ↓
CASTANET (EA RPC protocol)
    ↓
HTTP (TCP)
    ↓
EA Server Infrastructure
```

CASTANET wraps NPS RPC calls in HTTP. The game connects to two types of servers:
- **Auth server** — `ea.com` (login, account, persona management)
- **Patch server** — `downloads.mco.ea.com` (game updates)
- **Game server** — EA game server infrastructure (race events, lobbies)

---

## CASTANET Protocol

CASTANET is EA's proprietary RPC protocol. Discovered from EXE strings in `mco.exe`.

### Error Codes (40+ documented)

```
CASTANET_ERROR_CONNECT_FAILED
CASTANET_ERROR_INIT_FAILED
CASTANET_ERROR_NO_MATCHING_SEGMENT
CASTANET_ERROR_SET_ATTR_FAILED
CASTANET_ERROR_CERTIFICATE_WRONG_TYPE
CASTANET_ERROR_CERTIFICATE_MISSING
CASTANET_ERROR_CERTIFICATE_INVALID_ROOT
CASTANET_ERROR_CERTIFICATE_EXPIRED
CASTANET_ERROR_SSL_NO_SUPPORT
CASTANET_ERROR_SIGNATURE
CASTANET_ERROR_HTTP_UNAUTHORIZED
CASTANET_ERROR_INVALID_REPLY
CASTANET_ERROR_CHECKSUM_MISMATCH
CASTANET_ERROR_NO_SUCH_CHANNEL
CASTANET_ERROR_PROTOCOL
CASTANET_ERROR_CONNECT_FAILED
CASTANET_ERROR_UNKNOWN_HOST
CASTANET_ERROR_HTTP
```

All 40+ CASTANET error codes documented in EXE strings. [E2]

---

## Authentication Flow

```
1. Connect to: http://www.ea.com/SubscribeEntry.jsp?prodID=REG-MCO
2. NPSLogin → NPSUserLogin
3. Auth_NPS_AAI_Hostname (ea.com)
4. Auth_NPS_AAI_Port (runtime-configured)
5. AuthLoginLib_GetAuthorize
6. NPSGetPersonaMaps → NPSGetFirstGamePersona → NPSSelectGamePersona
7. NPSGetGameServersList
8. NPSStartGameServer (join race lobby)
```

Confirmed from EXE strings. [E2]

### Auth Server URLs

```
http://www.ea.com/SubscribeEntry.jsp?prodID=REG-MCO
http://www.ea.com/TransferToCamHelpServlet?sDestinationURL=PASSWORD
SOFTWARE\Electronic Arts\Motor City\AuthAuth\AuthLoginServer
```

---

## NPS Message Format

Messages use a header with `msgId` (message ID) and `msg_len` (length). Requests and responses are tracked separately.

### Message Types (sample)

| ID | Name | Direction |
|----|------|-----------|
| — | NPSLogin | C→S |
| — | NPSUserLogin | C→S |
| — | NPSGetMail | C→S |
| — | NPSSendMail | C→S |
| — | NPSGetBuddyList | C→S |
| — | NPSAddToBuddyList | C→S |
| — | NPSGetPersonaMaps | C→S |
| — | NPSGetGameServersList | C→S |
| — | NPSStartGameServer | C→S |
| — | NPSUpdateGamePersona | C→S |

Full message table donated by Molly. [E2]

---

## 🔴 Unknown

- **CASTANET serialization format** — message binary structure not analyzed
- **Packet encryption** — RSA/DES-CBC mentioned in donated doc; not verified
- **Live server behavior** — impossible to test (servers shut down March 2003)

---

## See Also

- `research/NETWORK_PROTOCOL.md` — full donated protocol specification (community doc)

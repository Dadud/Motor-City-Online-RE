/**
 * nps_test.c - Test program for NPS library
 * 
 * Simple test to verify NPS/Castanet connectivity.
 * 
 * Usage: nps_test.exe <server> <port>
 *   e.g.: nps_test.exe ea.com 18000
 */

#include <stdio.h>
#include <string.h>
#include "nps.h"

int main(int argc, char* argv[])
{
    const char* server = "ea.com";
    int port = 18000;

    if (argc >= 2) {
        server = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    printf("NPS Test Program\n");
    printf("================\n\n");

    printf("Connecting to %s:%d...\n", server, port);

    NPS_CONTEXT* nps = NPSCreate();
    if (!nps) {
        printf("ERROR: Failed to create NPS context\n");
        return 1;
    }

    int result = NPSConnect(nps, server, port);
    if (result != 0) {
        printf("ERROR: Connect failed: %s\n", NPSGetErrorString(nps));
        NPSDestroy(nps);
        return 1;
    }

    printf("Connected successfully!\n");
    printf("CASTANET protocol handshake complete.\n");

    printf("\nDisconnecting...\n");
    NPSDisconnect(nps);
    NPSDestroy(nps);

    printf("Done.\n");
    return 0;
}

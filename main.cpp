/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "main.h"

XMServerUpdater updater;
XMServer server;
xmLevel level;

int main(int argc, char *argv[]) {
    if (argc > 1 && strstr(argv[1], "-hide")) FreeConsole();

    if (!server.Start())
        printf("error: cannot start XMServer!\n");

    if (!server.Commands())
        printf("error: command fifo problem!\n");

    return 0;
}

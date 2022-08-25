/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../main.h"

extern XMServerUpdater updater;
extern XMServer server;
extern xmLevel level;
MD5 md5;

void PrepareMainThread(void *data);

void PrepareTimerThread(void *data);

void PrepareUploadThread(void *data);
//void PrepareChatThread(void * data);


bool XMServer::Start(void) {

    printf("===============================================================================\n"
           "|    db    db .88b  d88.   .d8888. d88888b d8888b. db    db d88888b d8888b.   |\n"
           "|    `8b  d8' 88'YbdP`88   88'  YP 88'     88  `8D 88    88 88'     88  `8D   |\n"
           "|     `8bd8'  88  88  88   `8bo.   88ooooo 88oobY' Y8    8P 88ooooo 88oobY'   |\n"
           "|     .dPYb.  88  88  88     `Y8b. 88~~~~~ 88`8b   `8b  d8' 88~~~~~ 88`8b     |\n"
           "|    .8P  Y8. 88  88  88   db   8D 88.     88 `88.  `8bd8'  88.     88 `88.   |\n"
           "|    YP    YP YP  YP  YP   `8888Y' Y88888P 88   YD    YP    Y88888P 88   YD   |\n\n"
           "===============================================================================\n");

    if (!InitWinsock() || !IsServerRunning() || !LoadConfig("config.ini")) return false;
    srand(time(NULL));

    player = new PlayerInfo[iNumSlots];
    if (!player) return false;

    HANDLE handle[4];
    handle[0] = (HANDLE) _beginthread(PrepareMainThread, 0, NULL);
    handle[1] = (HANDLE) _beginthread(PrepareTimerThread, 0, NULL);
    handle[2] = (HANDLE) _beginthread(PrepareUploadThread, 0, NULL);
    //handle[3] = (HANDLE)_beginthread(UploadChatThread,   0, NULL);
    WaitForSingleObject(handle[0], 100);
    WaitForSingleObject(handle[1], 100);
    WaitForSingleObject(handle[2], 100);

    return true;
}

#define printf print

void PrepareUploadThread(void *data) {
    {
        updater.Connect();
        while (1) {
            if (!updater.Listen()) {
                printf("error: file updater thread!\n");
                //return false;
            }
            Sleep(25);
        }
    }
    _endthread();
}

void PrepareTimerThread(void *data) {
    {
        while (1) {
            if (!server.TimerThread()) {
                printf("error: timer thread!\n");
                //return false;
            }
            Sleep(25);
        }
    }
    _endthread();
}

void PrepareMainThread(void *data) {
    {
        if (!server.Connect()) {
            printf("error: cannot start server socket!\n");
            //return false;
        }

        srand(timeGetTime());

        while (1) {
            server.Listen();
        }

        if (!server.Close()) {
            printf("error: invalid socket, cannot close server!\n");
            //return false;
        }
    }
    _endthread();
}

bool XMServer::Listen(void) {
    DisconnectAFK();

    if (recvfrom(sock, inBuffer, sizeof(inBuffer), 0, (struct sockaddr *) &from, &fromlen) != SOCKET_ERROR) {
        if (!inBuffer || !*inBuffer) return true;
        switch (ClassifyResponse(inBuffer)) {
            case FIRST_CONNECT: {
                if (iNumUsedSlots >= iNumSlots) {
                    printf("error: number of clients reached limit %d players at server!", iNumSlots);
                    sprintf(outBuffer, "full_server");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (struct sockaddr *) &from, fromlen);
                } else {
                    printf("ok: Client ip %s's connecting!", inet_ntoa(from.sin_addr));
                    for (int i = 0; i < iNumSlots; i++) {
                        if (!player[i].slotUsed) {
                            char nick[1024];
                            char hash[256];
                            if (sscanf(inBuffer, "first_connect %s %s", &nick, &hash) == 2) {
                                if (CheckActiveUser(hash)) {
                                    player[i].ip = inet_ntoa(from.sin_addr);
                                    player[i].imprint = GenerateImprint();
                                    player[i].slotUsed = true;
                                    player[i].slotLoaded = false;
                                    player[i].score = ScoreInfo();
                                    player[i].bonuses = BonusInfo();
                                    player[i].sessionid = hash;
                                    player[i].id = i;
                                    player[i].ping = -1;
                                    player[i].health = 100;
                                    player[i].startTime = timeGetTime();
                                    strcpy(player[i].nick, nick);
                                    player[i].Spawn();
                                    player[i].dead = true;
                                    player[i].skinID = rand() % iNumSkins;
                                    player[i].waiting = true;

                                    //if(strstr(nick, "JatruvBot"))
                                    //    player[i].dead = false;

                                    printf("ok: Client id: %d, nick %s, ip %s is connecting to server!", player[i].id,
                                           player[i].nick, player[i].ip);
                                    sprintf(outBuffer, "cid %d %d %f %f %d %s %s", player[i].id,
                                            iNumSlots,
                                            fMaxGrenadeLife,
                                            fMaxMineLife,
                                            iMaxGrenades,
                                            map,
                                            player[i].imprint);

                                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                                    strcpy(outBuffer, "");
                                    iNumUsedSlots++;
                                } else {
                                    printf("ok: Client %s, ip %s has invalid SESSIONID - %s!", nick,
                                           inet_ntoa(from.sin_addr), hash);
                                    sprintf(outBuffer, "unlogged_hacker");
                                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                                }
                            }
                        }
                    }
                }
            }
                break;

            case POS: {
                char hash[16];
                float angle[2];
                vec3 position;
                int id, isMoving, throws, wid, aid, cframe, animFrame[2], ping, skinID, grenType;

                if (sscanf(inBuffer, "pos %f %f %f %f %f %d %d %d %d %s", &position.x, &position.y, &position.z,
                           &angle[0], &angle[1], &id, &throws, &grenType, &ping, &hash) == 10) {
                    if (!player[id].slotLoaded) {
                        strcpy(outBuffer, "timeout");
                        sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                    }

                    if (!IsValidImprint(hash)) {
                        strcpy(outBuffer, "err");
                        sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                    }

                    if (player[id].slotUsed && !player[id].dead && !bRoundRestart) {
                        player[id].pos = position;

                        player[id].angle[0] = angle[0];
                        player[id].angle[1] = angle[1];

                        player[id].isMoving = (bool) isMoving;
                        player[id].throwsGrenade = (bool) throws;
                        player[id].grenType = grenType;
                    }

                    player[id].ping = ping;

                    strcpy(outBuffer, "pos\n");
                    if (bRoundRestart) bWasAllPlayersRespawned = false;
                    for (int i = 0; i < iNumSlots; i++) {
                        if (player[i].slotUsed && player[i].slotLoaded) {
                            char tmpBuff[1024];
                            sprintf(tmpBuff, "%.1f %.1f %.1f %.1f %.1f %d %d %d %d %d %d %d %d %d\n", //14
                                    player[i].pos.x, player[i].pos.y, player[i].pos.z,
                                    player[i].angle[0], player[i].angle[1],
                                    player[i].id,
                                    (int) player[i].dead,
                                    (int) player[i].throwsGrenade,
                                    player[i].grenType,
                                    player[i].health,
                                    (int) bRoundRestart,
                                    (int) fRoundTimeCountdown,
                                    player[i].ping,
                                    player[i].skinID);
                            strcat(outBuffer, tmpBuff);
                        }
                    }

                    if (bRoundRestart) bWasAllPlayersRespawned = true;
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (struct sockaddr *) &from, fromlen);
                } else {

                    strcpy(outBuffer, "err");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (struct sockaddr *) &from, fromlen);
                }
            }
                break;

            case DISCONNECT: {
                int id = 0;
                char hash[16];
                if (sscanf(inBuffer, "disconnect %d %s", &id, &hash) == 2) {
                    if (player[id].slotUsed && player[id].slotLoaded && IsValidImprint(hash)) {
                        printf("ok: Client cid: %d nick: %s disconnected from server!\n", id, player[id].nick);

                        Disconnect(id);
                        sprintf(outBuffer, "disconnected %d", id);
                        sendto(sock, outBuffer, strlen(outBuffer), 0, (struct sockaddr *) &from, fromlen);
                    } else {
                        sprintf(outBuffer, "failed");
                        sendto(sock, outBuffer, strlen(outBuffer), 0, (struct sockaddr *) &from, fromlen);
                    }
                } else {
                    sprintf(outBuffer, "failed");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (struct sockaddr *) &from, fromlen);
                }
            }
                break;

            case INFO: {
                sprintf(outBuffer, "%s|%d|%d", GetServerName(), iNumUsedSlots, iCurrRound);
                sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
            }
                break;

            case CURRMAP: {
                sprintf(outBuffer, "currMap %s", map);
                sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
            }
                break;

            case LOADED: {
                char hash[256];
                int id;
                if (sscanf(inBuffer, "loaded %d %s", &id, &hash) == 2) {
                    //if(!IsValidImprint(hash))
                    //{
                    //    strcpy(outBuffer, " ");
                    //    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr*)&from, fromlen);
                    //}

                    //if(IsValidID(id))
                    {
                        float time = (timeGetTime() - player[id].startTime) / 1000.0;
                        printf("Player %d loaded map for time %.1fs", id, time);
                        if (time <= 90.0) {
                            player[id].slotLoaded = true;
                            sprintf(outBuffer, "loaded %f", time);
                            sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                        } else {
                            //disconnectnu harciQa, alespon se o to pokusim,
                            //pokud se to nepovede tady, hlavni smycka to udela
                            Disconnect(id);
                        }
                    }

                    strcpy(outBuffer, " ");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                }
            }
                break;

            case GHASH: {
                char filepath[1024];
                if (sscanf(inBuffer, "ghash %s", &filepath) == 1) {
                    char finalfilepath[1024];
                    sprintf(finalfilepath, "files/%s", filepath);
                    printf("ok: ip: %s is trying to get hash of \"%s\"!", inet_ntoa(from.sin_addr), finalfilepath);

                    char *hash = md5.digestFile(finalfilepath);
                    sendto(sock, hash, strlen(hash) + 1, 0, (sockaddr *) &from, fromlen);
                } else {
                    strcpy(outBuffer, " ");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                }

            }
                break;

            case DBUP: //destroyable box update
            {
                char imprint[16];
                int pid;
                if (sscanf(inBuffer, "dbup %d %s", &pid, &imprint) == 2) {
                    char tmpbuff[256];
                    strcpy(outBuffer, "dbup\n");
                    for (int i = 0; i < level.GetNumEntities(); i++) {
                        sprintf(tmpbuff, "%d %d %d %d\n", entity[i].id, entity[i].destroy, entity[i].destroyed,
                                entity[i].bonusID);
                        strcat(outBuffer, tmpbuff);
                    }

                    sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                } else {
                    strcpy(outBuffer, " ");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                }
            }
                break;

            case GRNPS: //grenades position update
            {
                if (strstr(inBuffer, "grnps")) {
                    char *line = strtok(inBuffer, "\n");
                    while (line) {
                        char imprint[16];
                        int id, pid, type;
                        vec3 pos, rot;
                        float life, dir;
                        if (sscanf(line, "%d %f %f %f %f %f %f %f %f %d %d %d %s\n", &id, &pos.x, &pos.y, &pos.z,
                                   &rot.x, &rot.y, &rot.z, &life, &dir, &type, &pid, &imprint) == 11) {
                            if (pid < 0 || pid > iNumSlots || id < 0 || id > iNumGrenades) {
                                strcpy(outBuffer, " ");
                                sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                                break;
                            }

                            player[pid].grenade[id].pos = pos;
                            player[pid].grenade[id].rot = rot;
                            player[pid].grenade[id].type = type;
                            player[pid].grenade[id].speed = dir;
                            player[pid].grenade[id].life = life;
                            player[pid].grenade[id].pid = pid;

                            switch (type) {
                                //zmenit rozmezi casove znamky, dela problemy pri ztrate paketu/desynchronizaci
                                //zmenit startLife podle zmeny znamky // 0.1 == +100ms
                                case STICKY_GRENADE: {
                                    if (life >= fMaxGrenadeLife - 0.1) {
                                        player[pid].grenade[id].startLife = timeGetTime();
                                        player[pid].grenade[id].threwn = true;
                                    }
                                }
                                    break;

                                case REFLECTIVE_GRENADE: {

                                }
                                    break;

                                case MINE_GRENADE: {
                                    if (life >= fMaxMineLife - 0.1) {
                                        player[pid].grenade[id].startLife = timeGetTime();
                                        player[pid].grenade[id].threwn = true;
                                    }
                                }
                                    break;
                            }
                        }
                        line = strtok(NULL, "\n");
                    }

                    strcpy(outBuffer, "grnps\n");
                    for (int i = 0; i < iNumUsedSlots; i++) {
                        if (player[i].slotUsed && player[i].slotLoaded) {
                            for (int j = 0; j < iNumGrenades; j++) {
                                if (player[i].grenade[j].threwn) {
                                    char tmpbuff[256];
                                    vec3 pos = player[i].grenade[j].pos;
                                    vec3 rot = player[i].grenade[j].rot;
                                    int type = player[i].grenade[j].type;
                                    float life = player[i].grenade[j].life;
                                    sprintf(tmpbuff, "%d %d %f %f %f %f %f %f %f %d\n", j, type, pos.x, pos.y, pos.z,
                                            rot.x, rot.y, rot.z, life, i);
                                    strcat(outBuffer, tmpbuff);
                                }
                            }
                        }
                    }

                    sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                }

            }
                break;

            case GPS: //GPS - Get Player Scores
            {
                char imprint[16];
                if (sscanf(inBuffer, "gps %s", &imprint) == 1) {
                    /*if(!IsValidImprint(imprint))
                    {
                        strcpy(outBuffer, "err");
                        sendto(sock, outBuffer, strlen(outBuffer)+1, 0, (sockaddr*)&from, fromlen);
                        break;
                    }*/
                    strcpy(outBuffer, "gps\n");
                    for (int i = 0; i < iNumSlots; i++) {
                        if (player[i].slotUsed && player[i].slotLoaded) {
                            char tmpBuff[256];
                            sprintf(tmpBuff, "%s %d %d %d %d\n", player[i].nick, i, player[i].score.total,
                                    player[i].score.frags, player[i].score.numDestroyedBoxes);
                            strcat(outBuffer, tmpBuff);
                        }
                    }

                    sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                } else {
                    strcpy(outBuffer, " ");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                }
            }
                break;

            case UPB: {
                char imprint[16];
                int pid, bid;
                if (sscanf(inBuffer, "upb %d %d %s", &pid, &bid, &imprint) == 3) {
                    if (pid < 0 || pid > GetNumSlots() || IsSlotActive(pid)) break;
                    /*if(!IsValidImprint(imprint))
                    {
                        strcpy(outBuffer, "err");
                        sendto(sock, outBuffer, strlen(outBuffer)+1, 0, (sockaddr*)&from, fromlen);
                        break;
                    }*/

                    player[pid].UseBonus(bid);
                    strcpy(outBuffer, "upb\n");
                    sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                } else {
                    strcpy(outBuffer, " ");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                }
            }
                break;

            case GHB: {
                char imprint[16];
                int pid;
                if (sscanf(inBuffer, "ghb %d %s", &pid, &imprint) == 2) {
                    if (!IsSlotActive(pid)/* || IsValidImprint(imprint)*/) {
                        strcpy(outBuffer, "err");
                        sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                        break;
                    }

                    sprintf(outBuffer, "ghb\n%d %d %d", player[pid].bonuses.GetBonusID(0),
                            player[pid].bonuses.GetBonusID(1),
                            player[pid].bonuses.GetBonusID(2));

                    sendto(sock, outBuffer, strlen(outBuffer) + 1, 0, (sockaddr *) &from, fromlen);
                } else {
                    strcpy(outBuffer, " ");
                    sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr *) &from, fromlen);
                }
            }
                break;

            default:
                printf("unidentified response from %s!\"%s\"", inet_ntoa(from.sin_addr), inBuffer);
                break;
        }
        //strcpy(outBuffer, " ");
        //sendto(sock, outBuffer, strlen(outBuffer), 0, (sockaddr*)&from, fromlen);
    }

    return true;
}

bool XMServer::TimerThread(void) {
    if (FixedTimeStep(1)) {
        if (fGameTime >= fMaxGameTime) {
            //zmena mapy za jinou
            fGameTime = 0.0;
            level.~xmLevel();
            if (level.Load(GetMapName())) {
                if (entity) delete entity;
                entity = new EntityInfo[level.GetNumEntities()];
                if (!entity) return false;

                for (int i = 0; i < level.GetNumEntities(); i++) {
                    entity[i].Set(level.entity[i].GetPosition(),
                                  level.entity[i].GetRotation(),
                                  i,
                                  i % 5 == 0 ? rand() % iNumBonuses : NO_BONUS,
                                  level.entity[i].GetRadius());
                }
            } else {
                printf("error: cannot load XMLevel %s!::FATAL!", GetMapName());
                return false;
            }

            strcpy(map, cMaps[iCurrRound]);
            printf("ok: map changed to %s\n", GetMapName());

            //vyslat signal do enginu o reloadu mapy, aby zacal timeout.

            fRoundTimeCountdown = 0.0;
            if (iCurrRound >= MAX_MAPS) iCurrRound = 0; else iCurrRound++;
        }

        if (fRoundTimeCountdown <= 1 || GetNumWaitingPlayers() >= iWaitForNumPlayers) {
            level.ResetSpawns();
            srand(timeGetTime());

            for (int i = 0; i < iNumSlots; i++) {
                if (IsSlotActive(i))
                    player[i].Spawn();
            }

            bRoundRestart = true;
            fRoundTimeCountdown = fMaxRoundTime;
            SetPlayersWaiting(false);
            printf("ok: respawned!\n");
        } else {
            if (bWasAllPlayersRespawned) bRoundRestart = false;
        }


        for (int i = 0; i < iNumSlots; i++) {
            if (IsSlotActive(i)) {
                if (player[i].hasGrenadeMasking && player[i].gmTimer > 0)
                    player[i].gmTimer--;

                if (player[i].freezedMovement && player[i].fmTimer > 0)
                    player[i].fmTimer--;
            }
        }


        fGameTime++;
        fRoundTimeCountdown--;

        SetTitle();
    }

    for (int i = 0; i < level.GetNumEntities(); i++) {
        if (entity[i].destroy) {
            if (timeGetTime() - entity[i].destroyTime >= 1000 && !entity[i].destroyed)
                entity[i].destroyed = true;
        }
    }

    for (int i = 0; i < iNumUsedSlots; i++) {
        if (!player[i].SlotActive()) continue;

        for (int j = 0; j < iNumGrenades; j++) {
            vec3 pos = player[i].grenade[j].pos;

            if (player[i].grenade[j].threwn && pos != vec3(0)) {
                int type = player[i].grenade[j].type;
                float life = (timeGetTime() - player[i].grenade[j].startLife) * 0.001;

                switch (type) {
                    case STICKY_GRENADE: {
                        if (life >= fMaxGrenadeLife - grenDelay) {
                            CollideGrenadeWithDestroyableBox(i, pos);
                            CollideGrenadeWithPlayer(i, pos);
                            player[i].grenade[j].reset();
                        }
                    }
                        break;

                    case REFLECTIVE_GRENADE: {
                        player[i].grenade[j].reset();
                    }
                        break;

                    case MINE_GRENADE: {
                        float distance = Distance(pos, player[i].pos);
                        if (life >= fMaxMineLife - grenDelay ||
                            (distance <= charHeight + grenRadius && i != player[i].grenade[j].pid)) {
                            CollideGrenadeWithDestroyableBox(i, pos);
                            CollideGrenadeWithPlayer(i, pos);
                            player[i].grenade[j].reset();
                        }
                    }
                        break;

                    default:
                        printf("invalid grenade type %d\n", type);
                }
            }
        }

        //check kolize hrace s bonusem
        for (int e = 0; e < level.GetNumEntities() && !player[i].dead; e++) {
            if (entity[e].bonusID == NO_BONUS || !entity[e].destroyed) continue;
            if (SphereInSphere(entity[e].position, entity[e].radius * 0.5, player[i].pos, charHeight))
                player[i].ClassifyBonus(entity[e]);
        }


    }

    //neomezena smycka

    return true;
}

bool XMServer::Commands(void) {
#undef printf
    char command[1024];
    printf("\n\n");
    while (1) {
        printf("XMServer> ");
        gets(command);

        switch (ClassifyCommand(command)) {
            case RESPAWN: {
                fRoundTimeCountdown = 0.0;
            }
                break;

            case DISCONNECTCMD: {
                if (!strcmp(command, "disconnect")) {
                    for (int i = 0; i < iNumSlots; i++)
                        Disconnect(i);

                    printf("ok: all disconnected!");
                } else {
                    int id;
                    if (sscanf(command, "disconnect %d", &id) == 1) {
                        if (id < 0 || id > iNumSlots) break;
                        Disconnect(id);
                        printf("ok: %d disconnected from server!\n", id);
                    }
                }
            }
                break;

            case PLAYERLIST: {
                printf("id - ip - nick\n");
                for (int i = 0; i < GetNumUsedSlots(); i++) {
                    if (IsSlotActive(i))
                        printf("%d. %s - %s\n", i, player[i].ip, player[i].nick);
                }
            }
                break;

            case DESTROY_BOXES: {
                for (int k = 0; k < level.GetNumEntities(); k++) {
                    entity[k].destroy = true;
                    entity[k].destroyed = true;
                }

                printf("ok: all boxes destroyed!\n");
            }
                break;

            default:
                printf("invalid command!\n");
        }
    }
}

bool XMServer::Close(void) {
    if (!sock) return false;
    closesocket(sock);

    return true;
}

bool XMServerUpdater::Listen(void) {
#define printf print
    int client = accept(sock, (sockaddr *) &from, &fromlen);
    if (client == -1) {
        printf("error: cannot accept connection to ip: %s!", inet_ntoa(from.sin_addr));
        return true;
    }

    if (recv(client, inBuffer, sizeof(inBuffer), 0) != SOCKET_ERROR) {
        switch (ClassifyResponse(inBuffer)) {
            case GETFILE: {
                char imprint[16], filepath[512];
                if (sscanf(inBuffer, "file %s %s", &filepath, &imprint) == 2) {
                    printf("ok: client %s downloads file %s!", inet_ntoa(from.sin_addr), filepath);
                    if (filepath/* && IsValidImprint(imprint)*/ && fopen(filepath, "rb") && strstr(filepath, "files/")
                        && (!strstr(filepath, "../") || !strstr(filepath, ".../") || !strstr(filepath, "./"))) {
                        FILE *fp = fopen(filepath, "rb");

                        fseek(fp, 0, SEEK_END);
                        int fileSize = ftell(fp);
                        rewind(fp);

                        char *checksum = md5.digestFile(filepath);

                        int j = 0;
                        char header[1024];

                        sprintf(header, "file %s %d %s", filepath + 6, fileSize, checksum);
                        send(client, header, strlen(header) + 1, 0);

                        while (!feof(fp)) {
                            fread(outBuffer, sizeof(outBuffer), 1, fp);
                            send(client, outBuffer, 65536, 0);
                        }

                        fclose(fp);
                        printf("ok: file %s sent sucessfully - %dMB!", filepath, (fileSize / 1024) / 1024);
                    } else {
                        sprintf(outBuffer, "invalid_file");
                        send(client, outBuffer, strlen(outBuffer) + 1, 0);
                        printf("error: an error occured in sending file %s!", filepath);
                    }
                }
            }
                break;

            default:
                printf("error: unidentified response %s in file updater from %s", inBuffer, inet_ntoa(from.sin_addr));
                break;
        }

        strcpy(outBuffer, " ");
        send(client, outBuffer, strlen(outBuffer), 0);
    }
#undef printf
    return true;
}


bool PlayerInfo::Spawn(void) {
    int slotID;
    xmlPlayerSpawn spawn = level.GetRandomSpawn(slotID);
    if (slotID != -1) {
        Spawn(spawn.GetCenter(), server.GetMaxHealth());
        return true;
    }

    return false;
}

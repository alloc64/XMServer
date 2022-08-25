/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include "../math/vector.h"
#include "../core/xmlevel.h"
#include "./server.h"

#define iNumGrenades 5
#define grenRadius 4.0
#define grenDelay 0.4
#define grenDefaultExpRadius 120.0

#define iNumBonuses 11
#define iMaxBonuses 3

#define MAGNET_TIME 5
#define GRENADE_MASKING_TIME 20
#define MORE_GRENADE_DAMAGE_VALUE 25
#define HP_PACK_VALUE 25
#define POINTS_PACK 20

#define iNumSkins 4

//this server
#define SERVER_PORT 3515
#define LOG_PATH "output.log"
#define MAX_MAPS 5

//master
#define MASTER_SERVER_IP "122.74.broadband2.iol.cz"
#define MASTER_SERVER_PORT 3518

//updater
#define MAX_QUEUES 1024
#define RECV_TIMEOUT 100

extern xmLevel level;

inline char *GetDateTime(void) {
    time_t rawtime;
    time(&rawtime);
    char *time = asctime(localtime(&rawtime));
    return time ? strtok(time, "\n") : (char *) "";
}

inline void print(const char *fmt, ...) {
    char str[2048];
    va_list args;
    va_start(args, fmt);
    vsprintf(str, fmt, args);
    va_end(args);

    printf("[%s] ", GetDateTime());
    printf("%s\n", str);

    FILE *fp = fopen(LOG_PATH, "a");
    if (!fp) return;

    fprintf(fp, "[%s] %s\n", GetDateTime(), str);
    fclose(fp);
}

class BonusInfo {
public:
    BonusInfo() {
        bonusPointer = 0;
        for (int i = 0; i < iMaxBonuses; i++)
            bonusStack[i] = -1;
    }

    bool PushBonus(int id) {
        if (id < 0 || id > iNumBonuses || bonusPointer < 0 || bonusPointer >= iMaxBonuses) return false;
        for (int i = 0; i < iMaxBonuses; i++) {
            if (bonusStack[i] == -1) {
                bonusStack[i] = id;
                bonusPointer++;
                break;
            }
        }

        return true;
    }

    bool HasBonus(int id) {
        if (id < 0 || id > iNumBonuses) return false;

        for (int i = 0; i < 3; i++)
            if (bonusStack[i] == id) return true;
        return false;
    }

    bool PopBonus(int id) {
        if (id < 0 || id > iNumBonuses) return false;
        if (bonusPointer > 0 && bonusStack[id] != -1) {
            bonusStack[id] = -1;
            bonusPointer--;
        }
        return true;
    }

    int GetBonusID(int id) {
        if (id < 0 || id > iMaxBonuses)
            return -1;

        return bonusStack[id];
    }

    int GetNumBonuses(void) {
        return bonusPointer;
    }

private:
    int bonusStack[iMaxBonuses];
    int bonusPointer;

};


enum GrenType {
    STICKY_GRENADE = 0,
    REFLECTIVE_GRENADE,
    MINE_GRENADE
};

class GrenadeInfo {
public:
    vec3 pos;
    vec3 rot;
    float life, startLife, speed;
    bool exploded;
    bool threwn;
    int type, pid;
#define grenRadius 4.0


    void reset() {
        pos = vec3(0.0);
        rot = vec3(0.0);
        life = 1.0;
        startLife = 0.0;
        exploded = false;
        threwn = false;
        type = 0;
    }
};

class ScoreInfo {
public:
    ScoreInfo() {
        total = 0;
        frags = 0;
        numDestroyedBoxes = 0;
    }

    int total, frags, numDestroyedBoxes;
};

class EntityInfo {
public:
    EntityInfo() {
        id = 0;
        destroyed = false;
    }

    bool Set(vec3 position, vec3 rotation, int id, int bonusID, int radius) {
        if (id < 0 || bonusID < 0 || radius < 0) return false;

        this->position = position;
        this->rotation = rotation;
        this->id = id;
        this->bonusID = bonusID;
        this->radius = radius;
        this->destroyTime = 0;
        this->destroy = false;
        this->destroyed = false;

        return true;
    }

    vec3 position, rotation;
    int id, bonusID, radius, destroyTime;
    bool destroy, destroyed;
};


class PlayerInfo {
public:
    char *ip, *imprint, *sessionid, nick[1024];
    bool slotUsed, slotLoaded, isMoving, throwsGrenade, dead, waiting;
    int id, ping, health, startTime, activeGrenades, skinID, grenType;
    vec3 pos;
    float angle[2], grenExpRadius;
    GrenadeInfo grenade[iNumGrenades];
    ScoreInfo score;

    BonusInfo bonuses;
    bool freezedMovement, hasGrenadeMasking;
    float fmTimer, gmTimer;

#define charHeight 35.0

    PlayerInfo() {
        id = -1;
        ip = "";;
        slotUsed = false;
        slotLoaded = false;
        health = 100;
        dead = false;
        startTime = 0;
        activeGrenades = 2;
        waiting = true;
        freezedMovement = false;
        grenExpRadius = grenDefaultExpRadius;
        fmTimer = gmTimer = 0;
    }

    bool Spawn(void);

    void Spawn(vec3 pos, int health) {
        this->dead = false;
        this->pos = pos;
        this->health = health;
        this->activeGrenades = 2;
    }

    bool SlotActive() {
        return slotUsed && slotLoaded;
    }

    bool IsWaiting() {
        return waiting;
    }

    void SetWaiting(bool waiting) {
        this->waiting = waiting;
    }

    void ClassifyBonus(EntityInfo &entity) {
        int bid = entity.bonusID;
        if (bid < 0 || bid > iNumBonuses || bid == NO_BONUS || bonuses.GetNumBonuses() >= iMaxBonuses) return;

        //pushnuti bonusu bude stejne i pro pasivni, jenom musim w8tkonout bez petr predela hud
        if (bid == GRENADE_PLUS || bid == MAGNET || bid == MORE_GRENADE_DAMAGE || bid == GRENADE_MASKING) {
            UseBonus(bid);
        } else {
            bonuses.PushBonus(bid);
        }

        entity.bonusID = NO_BONUS;
    }

    void UseBonus(int hudBID) // hudBID <0,2>
    {
        int bid = bonuses.GetBonusID(hudBID);
        if (bid < 0) return;
        bonuses.PopBonus(hudBID);

        switch (bid) {
            case GRENADE_PLUS:
                //pridani granatu
                if (activeGrenades <= iNumGrenades)
                    activeGrenades++;
                break;

            case MAGNET:
                //zmrazeni hrace na miste
                freezedMovement = true;
                fmTimer = MAGNET_TIME;
                break;

            case MORE_GRENADE_DAMAGE:
                //zvisi rozsah vybuchu granatu
                //         if(grenExpRadius < 200)
                //             grenExpRadius += MORE_GRENADE_DAMAGE_VALUE;
                break;

            case GRENADE_MASKING:
                //nastavim flag a rozeslu vsem klientum
                hasGrenadeMasking = true;
                gmTimer = GRENADE_MASKING_TIME;
                break;

            case HP_PACK:
                health += HP_PACK_VALUE;
                break;

            case POINTS_PACK:
                score.total += POINTS_PACK;
                break;

                //aktivní bonusy
                //case EARTHQUAKE:

                //break;

            case SPEED:

                break;

            case GLOBAL_SLOWDOWN:

                break;

                //case BOX_BLOCKING:

                //break;

            case TIME_SPEED_UP:

                break;

            case CHAOS:

                break;

            case TELEPORTIKUM:

                break;

            case MINE:

                break;

            case DETONATION:

                break;
        }
    }
};

class XMServerUpdater {
public:
    XMServerUpdater() {

    }

    bool Connect(void) {
#define printf print
        if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
            printf("error: couldn't create socket ::FUP!\n");
            return false;
        }

        fromlen = sizeof(from);
        local.sin_family = AF_INET;
        local.sin_port = htons(SERVER_PORT + 2);
        local.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (sockaddr *) &local, sizeof(local)) == SOCKET_ERROR) {
            printf("error: bind() failed!\n");
            return false;
        }

        if (listen(sock, MAX_QUEUES) == -1) {
            printf("error: cannot create request queue!\n");
            return false;
        }

        timeval tv;
        tv.tv_sec = RECV_TIMEOUT;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(timeval)) == SOCKET_ERROR) {
            printf("error: setsockopt() - error: %d\n", WSAGetLastError());
            return false;
        }

        printf("ok: updater listening...");
#undef printf
        return true;
    }

    bool Listen(void);

    inline int ClassifyResponse(char *inBuffer) {
        if (strstr(inBuffer, "file")) return GETFILE;

        return -1;
    }

private:
    sockaddr_in local, from;
    SOCKET sock;
    char inBuffer[65536], outBuffer[65536];
    int fromlen, size;

    enum {
        GETFILE = 0
    };

};

class XMServer {
public:
    XMServer() {
        iNumUsedSlots = 0;
        iCurrRound = 0;
        bRoundRestart = true;
        fGameTime = fRoundTimeCountdown = 0.0;
        bWasAllPlayersRespawned = false;
        bSpawned = false;
    }

    bool Start(void);

    bool Listen(void);

    bool TimerThread(void);

    bool Commands(void);

    bool Close(void);

    int GetNumSlots(void) {
        return iNumSlots;
    }

    int GetNumUsedSlots(void) {
        return iNumUsedSlots;
    }

    int GetMaxHealth(void) {
        return iMaxHealth;
    }

    int GetNumWaitingPlayers(void) {
        int iNumWaitingPlayers = 0;
        for (int i = 0; i < GetNumSlots(); i++) {
            if (player[i].IsWaiting() && player[i].SlotActive())
                iNumWaitingPlayers++;
        }

        return iNumWaitingPlayers;
    }

    void SetPlayersWaiting(bool waiting) {
        for (int i = 0; i < GetNumSlots(); i++)
            player[i].SetWaiting(waiting);
    }

    bool IsSlotActive(int id) {
        if (id < 0 || id > GetNumUsedSlots()) return false;
        return (player[id].slotUsed && player[id].slotLoaded);
    }

    bool Connect(void) {
#define printf print
        fromlen = sizeof(from);
        local.sin_family = AF_INET;
        local.sin_port = htons(SERVER_PORT);
        local.sin_addr.s_addr = INADDR_ANY;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (!sock) {
            printf("error: couldn't create UDP socket ::XMServer::MainThread!");
            return false;
        }

        bind(sock, (sockaddr *) &local, sizeof(local));

        timeval tv;
        tv.tv_sec = 15;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(timeval)) == SOCKET_ERROR) {
            printf("error: setsockopt() - error: %d\n", WSAGetLastError());
            return false;
        }

        printf("ok: server started...");
        printf("ok: waiting for clients...");
#undef printf
        return true;
    }

    bool LoadConfig(char *path) {
        if (!*path) return false;
        FILE *fp = fopen(LOG_PATH, "w");
        fclose(fp);

        fp = fopen(path, "r");

        char buffer[1024], name[256], maps[1024];
        float time;
        int data;
        while (fgets(buffer, 1024, fp)) {
            if (strstr(buffer, "//")) continue;
            if (strstr(buffer, "ServerName: ")) {
                char *name = buffer + 12;
                name[strlen(name) - 1] = NULL;
                strcpy(this->name, name);
            }
            if (sscanf(buffer, "GameTime: %f\n", &time) == 1) this->fMaxGameTime = time;
            if (sscanf(buffer, "RoundTime: %f\n", &time) == 1) this->fMaxRoundTime = time;
            if (sscanf(buffer, "Maps: %s\n", &maps) == 1) {
                int i = 0;
                char *pch = strtok(maps, ",");
                while (pch) {
                    strcpy(cMaps[i++], pch);
                    pch = strtok(NULL, ",");
                }

                if (i != MAX_MAPS) return false;
                strcpy(map, cMaps[0]);
            }

            if (sscanf(buffer, "MaxPlayers: %d\n", &data) == 1) this->iNumSlots = data;
            if (sscanf(buffer, "Health: %d\n", &data) == 1) this->iMaxHealth = data;
            if (sscanf(buffer, "GrenadeLife: %f\n", &time) == 1) this->fMaxGrenadeLife = time;
            if (sscanf(buffer, "MineLife: %f\n", &time) == 1) this->fMaxMineLife = time;
            if (sscanf(buffer, "MaxGrenades: %d\n", &data) == 1) this->iMaxGrenades = data;
            if (sscanf(buffer, "WaitForNumPlayers: %d\n", &data) == 1) this->iWaitForNumPlayers = data;
        }
        fclose(fp);

        if (iNumSlots <= 0 || iWaitForNumPlayers <= 0) return false;
        fGameTime = fMaxGameTime + 1;

        return true;
    }

    inline int ClassifyResponse(char *inBuffer) {
        if (strstr(inBuffer, "first_connect")) return FIRST_CONNECT;
        if (strstr(inBuffer, "pos")) return POS;
        if (strstr(inBuffer, "disconnect")) return DISCONNECT;
        if (strstr(inBuffer, "info")) return INFO;
        if (strstr(inBuffer, "currmap")) return CURRMAP;
        if (strstr(inBuffer, "loaded")) return LOADED;
        if (strstr(inBuffer, "ghash")) return GHASH;
        if (strstr(inBuffer, "dbup")) return DBUP;
        if (strstr(inBuffer, "grnps")) return GRNPS;
        if (strstr(inBuffer, "gps")) return GPS;
        if (strstr(inBuffer, "ghb")) return GHB;

        return -1;
    }

    inline int ClassifyCommand(char *cmd) {
        if (strstr(cmd, "respawn")) return RESPAWN;
        if (strstr(cmd, "disconnect")) return DISCONNECTCMD;
        if (strstr(cmd, "playerlist")) return PLAYERLIST;
        if (strstr(cmd, "destroyboxes")) return DESTROY_BOXES;

        return -1;
    }

    void Disconnect(int id) {
        if (player[id].slotUsed) {
            strcpy(player[id].ip, "");
            strcpy(player[id].nick, "");

            player[id].id = -1;
            player[id].slotUsed = false;
            player[id].slotLoaded = false;
            player[id].startTime = 0;
            player[id].score = ScoreInfo();
            iNumUsedSlots--;
        }
    }

    void DisconnectAFK(void) {
        for (int i = 0; i < iNumSlots; i++)
            if (!player[i].slotLoaded && (timeGetTime() - player[i].startTime) / 1000.0 > 90.0)
                Disconnect(i);
    }

    bool CheckActiveUser(char *SESSIONID) {
#define activeUserHeader "GET /index.php?isUserActive= HTTP/1.0\nhost: 85.207.14.161\nUser-Agent: XMServer UserChecker\nCookie: PHPSESSID=%s\n\n"
        hostent *host;
        sockaddr_in server;
        SOCKET sock;
        if ((host = gethostbyname(MASTER_SERVER_IP)) == NULL) return false;
        if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) return false;
        server.sin_family = AF_INET;
        server.sin_port = htons(MASTER_SERVER_PORT);
        memcpy(&(server.sin_addr), host->h_addr, host->h_length);
        if (connect(sock, (sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) return false;
        timeval tv;
        tv.tv_sec = 150;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(timeval)) == SOCKET_ERROR) {
            printf("error: setsockopt() - error: %d\n", WSAGetLastError());
            return false;
        }
        sprintf(outBuffer, activeUserHeader, SESSIONID);
        if (send(sock, outBuffer, strlen(outBuffer) + 1, 0) == -1) return false;
        if (recv(sock, inBuffer, sizeof(inBuffer), 0) == -1) return false;
        closesocket(sock);
        return strstr(inBuffer, "TRUE");
    }

    char *GenerateImprint(void) {
        const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        char imp[16] = "";
        int i, j = 0;
        int length = strlen(base);
        while (j <= 6) {
            i = rand() % 40;
            if (i < length) {
                imp[j] = (char) base[i];
                j++;
            }
        }
        imp[j + 1] = '\0';

        return imp;
    }

    bool IsValidImprint(char *imprint) {
        return true;
        for (int i = 0; i < iNumSlots; i++) {
            if (!player[i].imprint) continue;
            if (!strcmp(player[i].imprint, imprint)) return true;
        }

        return false;
    }

    bool Slap(int aid, int pid, float distance) {
        if (pid < 0 || pid > GetNumSlots() || aid < 0 || aid > GetNumSlots()) return false;

        float maxRadius = charHeight + player[aid].grenExpRadius;

        if (distance <= maxRadius && distance > 0.0) {
            if (distance < maxRadius) {
                int damage = int((1 - (distance / maxRadius)) * GetMaxHealth());
                if (player[pid].health - damage <= 0) {
                    player[aid].score.frags++;
                    player[aid].score.total += 20;

                    player[pid].score.frags--;
                    player[pid].score.total -= 20;

                    player[pid].health = 0;
                    player[pid].dead = true;
                } else {
                    player[pid].health -= damage;
                    player[pid].dead = false;
                }
            }

            return true;
        }

        return false;
    }

    void Spawn(void) {
        level.ResetSpawns();
        srand(timeGetTime());

        for (int i = 0; i < iNumSlots; i++) {
            if (IsSlotActive(i))
                player[i].Spawn();
        }
        bRoundRestart = true;
        fRoundTimeCountdown = fMaxRoundTime;
    }

    char *GetServerName(void) {
        return name;
    }

    char *GetMapName(void) {
        return map;
    }

    SOCKET GetSocket(void) {
        return sock;
    }

    void CollideGrenadeWithDestroyableBox(int pid, vec3 pos) {
        //otestovani kolize granatu s krabici
        for (int k = 0; k < level.GetNumEntities(); k++) {
            if (SphereInSphere(entity[k].position, entity[k].radius, pos, grenRadius) && !entity[k].destroyed) {
                entity[k].destroy = true;
                entity[k].destroyTime = timeGetTime();

                player[pid].score.numDestroyedBoxes++;
                player[pid].score.total += 5;
            }
        }
    }

    void CollideGrenadeWithPlayer(int pid, vec3 pos) {
        for (int k = 0; k < iNumUsedSlots; k++) {
            if (player[k].SlotActive())
                Slap(pid, k, fabs(((player[k].pos - vec3(0, charHeight, 0)) - pos).length()));
        }
    }

private:
    PlayerInfo *player;
    EntityInfo *entity;
    int iNumSlots, iNumUsedSlots, iMaxHealth, iCurrRound, iWaitForNumPlayers, iMaxGrenades, fromlen;
    float fGameTime, fMaxGameTime, fMaxRoundTime, fRoundTimeCountdown, fMaxGrenadeLife, fMaxMineLife;
    SOCKET sock;
    sockaddr_in local, from;
    char inBuffer[4096], outBuffer[4096], name[256], map[256], cMaps[MAX_MAPS][256];
    bool bRoundRestart, bWasAllPlayersRespawned, bSpawned;

    enum {
        FIRST_CONNECT = 0,
        POS,
        DISCONNECT,
        INFO,
        CURRMAP,
        LOADED,
        GHASH,
        DBUP,
        GRNPS,
        GPS,
        UPB,
        GHB
    };

    enum {
        RESPAWN = 0,
        DISCONNECTCMD,
        PLAYERLIST,
        DESTROY_BOXES
    };

    void SetTitle(void) {
        char t[32];
        sprintf(t, "XMServer - %1.fs from server restart", fGameTime);
        SetConsoleTitle(t);
    }
};

inline bool FixedTimeStep(int value) {
    static double lastTime = 0.0f;
    double elapsedTime = 0.0;
    double currentTime = timeGetTime() * 0.001;
    elapsedTime = currentTime - lastTime;

    if (elapsedTime > (1.0 / value)) {
        lastTime = currentTime;
        return true;
    }

    return false;
}

inline bool IsServerRunning(void) {
    HANDLE event = CreateEvent(NULL, TRUE, FALSE, "XMSERVER");

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (MessageBox(NULL, "XMServer is already running!", "Server error!", MB_ICONWARNING))
            return false;
    }

    return true;
}


inline bool InitWinsock() {
    WSADATA WsaData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0) {
        return false;
    } else {
        if (LOBYTE(WsaData.wVersion) != 2 || HIBYTE(WsaData.wHighVersion) != 2) {
            WSACleanup();
            return false;
        }
    }
    return true;
}

#endif

/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef _XMLEVEL_H
#define _XMLEVEL_H

#include "..\math\vector.h"

enum BonusTypes {
    //pasivni bonusy
    GRENADE_PLUS = 0,     //Zvýšení poètu granátù (cooldown použití,*3)    pasivní
    MAGNET,               //Magnet (Hráè se na chvíli zasekne na místì
    MORE_GRENADE_DAMAGE,  //Vìtší Dmg (Dodá granátu vìtší sílu, *3)
    GRENADE_MASKING,      //Maskování granátu (gránát bude neviditelnej, platí na 3 granáty*2)
    HP_PACK,              //Balíky s HP - přidá 25HP
    POINTS_PACK,          //přidá 20 bodů při sebrání bonusu, 4 balíky v jednom levelu

    //aktivní bonusy
    //EARTHQUAKE,                 //(Aréna se začne třást, náhodně budu vybuchovat některý krabice a miny, tak cca 10% )
    SPEED,                      //(zvýšení rychlosti hráče na 20s)
    GLOBAL_SLOWDOWN,            //dočasné(všichni hráči budou na určitou dobu zpomaleni kromě toho, který abilitu aktivoval)
    //BOX_BLOCKING,               //dostaneš možnost zablokovat 1 krabicu třeba na 30s proti zničení aby ses třeba z 1 strany kryl
    TIME_SPEED_UP,              //Celá hra se na chvíli zrychlí
    CHAOS,                      //(V teamové hře se všichni přebarví na jednu barvu a nejde poznat kdo ke komu patří - na určitou dobu)
    TELEPORTIKUM,               //(Hráči jsou náhodně přehozeni, tam kde byl H1 je nyní H3 kde byl H3 je H2, tam kde byl H2 je H1)
    MINE,                       //Umožní hráči položit 1 minu na kterou když stoupnou ostatni tak bouchne
    DETONATION,                 //Všechny položený miny v aréně bouchnou

    //
    NO_BONUS
};

class xmlPlayerSpawn {
public:
    char name[256];
    vec3 center;
    int id;
    bool used;

    vec3 GetCenter(void) {
        return center;
    }

    bool IsUsed(void) {
        return used;
    }

};

class xmlEntity {
public:
    vec3 GetPosition() {
        return position;
    }

    vec3 GetRotation() {
        return rotation;
    }

    float GetRadius() {
        return radius;
    }

    char name[256];
    vec3 position, rotation, min, max, BoxVertices[36];
    float radius;
};


class xmLevel {
public:
    xmLevel() {

    };

    ~xmLevel() {
        if (spawn) delete[] spawn;
        if (entity) delete[] entity;
    }

    bool Load(char *path);

    void ResetSpawns(void) {
        for (int k = 0; k < GetNumSpawns(); k++)
            spawn[k].used = false;
    }

    xmlPlayerSpawn GetRandomSpawn(int &id) {
        if (iNumSpawns > 0) //dycky bude, protoze se vygeneruje defaultni spawn
        {
            if (iNumSpawns == 1) {
                id = 0;
                return spawn[id];
            }

            id = -1;
            int j = 0;
            while (id == -1 && j < GetNumSpawns() * 2) {
                id = rand() % iNumSpawns;
                if (!spawn[id].used) {
                    //printf("%d\n", id);
                    spawn[id].used = true;
                    return spawn[id];
                }
                id = -1;
                j++;
            }

            if (j >= GetNumSpawns() * 2) {
                printf("error: all spawns are occupied!");
                id = -1;
            }
        }
    }

    xmlPlayerSpawn *spawn;
    xmlEntity *entity;
    short int iNumSpawns;
    short int iNumLights;
    short int iNumEntities;

    short int GetNumEntities() {
        return iNumEntities;
    }

    short int GetNumSpawns() {
        return iNumSpawns;
    }

private:
    char headerTitle[255];
    char header[8];

};

#endif

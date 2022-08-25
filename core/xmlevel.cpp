/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../main.h"

bool xmLevel::Load(char *path) {
#define printf print
    char mapPath[1024];
    strcpy(mapPath, "files/maps/");
    strcat(mapPath, path);

    FILE *pFile = fopen(mapPath, "rb");

    if (!pFile) {
        printf("error: cannot load XMLevel %s!", mapPath);
        return false;
    }

    fread(headerTitle, 1, 74, pFile);
    fread(header, 1, 7, pFile);

    if (strcmp(header, "LXML32")) return false;

    short int size;
    fread(&size, 1, sizeof(short int), pFile);
    fseek(pFile, size, SEEK_CUR);

    fread(&iNumLights, 1, sizeof(short int), pFile);
    fseek(pFile, 52, SEEK_CUR);
    fseek(pFile, 52 * iNumLights, SEEK_CUR);

    fread(&iNumSpawns, 1, sizeof(short int), pFile);
    printf("ok: loaded %d spawns!", iNumSpawns);

    spawn = new xmlPlayerSpawn[iNumSpawns];
    if (!spawn) return false;

    if (iNumSpawns > 0) {
        for (int s = 0; s < iNumSpawns; s++) {
            short int size = strlen(spawn[s].name);
            fread(&size, 1, sizeof(short int), pFile);
            fread(spawn[s].name, 1, size, pFile);
            fread(&spawn[s].center, 1, sizeof(vec3), pFile);
            spawn[s].id = s;
        }
    } else {
        iNumSpawns = 1;
        printf("warning: creating default spawn!", GetDateTime());
        spawn = new xmlPlayerSpawn[1];
        if (!spawn) return false;
        strcpy(spawn[0].name, "default_Spawn");
        spawn[0].center = vec3(-240, 400, -400);
    }

    fread(&iNumEntities, 1, sizeof(short int), pFile);
    entity = new xmlEntity[iNumEntities];
    if (!entity) return false;

    for (int e = 0; e < iNumEntities; e++) {
        short int size = 0;
        fread(&size, 1, sizeof(short int), pFile);
        fread(entity[e].name, 1, size, pFile);
        fread(&entity[e].rotation, 1, sizeof(vec3), pFile);
        fread(&entity[e].position, 1, sizeof(vec3), pFile);
        fread(&entity[e].min, 1, sizeof(vec3), pFile);
        fread(&entity[e].max, 1, sizeof(vec3), pFile);
        fread(&entity[e].radius, 1, sizeof(float), pFile);
        fread(&entity[e].BoxVertices, 36, sizeof(vec3), pFile);
    }

    printf("ok: loaded XMLevel %s", mapPath);

    fclose(pFile);
#undef printf
    return true;
}

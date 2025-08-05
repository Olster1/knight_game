enum PlacementType {
    UNIT_PLACE_SAFE,
    UNIT_PLACE_EXACT,
};

float3 getSafeSpawnLocation(GameState *gameState, float3 offset, int pWidth, int pHeight) {
    int maxChunkSearch = 10;
    bool searching = true;
    int maxSafeFound = 0;
    float safeAreaSize = pHeight*pWidth;
    float2 chunkOffset = getChunkPosForWorldP(offset.xy);
    for(int chunkY = 0; chunkY < maxChunkSearch && searching; chunkY++) {
        for(int chunkX = 0; chunkX < maxChunkSearch && searching; chunkX++) {
            for(int currentMapHeight = 1; currentMapHeight <= 2; currentMapHeight++) {
                float2 chunkP = make_float2(chunkX + chunkOffset.x, chunkY + chunkOffset.y);
                Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState,  &gameState->textureAtlas,chunkP.x, chunkP.y, 0, true, true, &globalPerFrameArena);
                if(c) { 
                    float3 chunkWorldP = getChunkWorldP(c);
                    for(int localY = 0; localY < (CHUNK_DIM - pHeight)  && searching; localY++) {
                        for(int localX = 0; localX < (CHUNK_DIM - pWidth) && searching; localX++) {
                            bool looking = true;
                            int safeFound = 0;
                            for(int y = 0; y < pHeight && looking; y++) {
                                for(int x = 0; x < pWidth && looking; x++) {
                                    float2 tileP = make_float2(chunkWorldP.x + localX + x, chunkWorldP.y + localY + y);
                                    float mapHeight = getMapHeight(tileP.x, tileP.y);

                                    float2 localP = make_float2(localX + x, localY + y);
                                    assert(c->visited);
                                    assert(localP.x < CHUNK_DIM && localP.y < CHUNK_DIM);
                                    if(mapHeight == currentMapHeight && !c->visited[(int)localP.y*CHUNK_DIM + (int)localP.x]) {
                                        Tile *tile = c->getTile(localX + x, localY + y, mapHeight - chunkWorldP.z);
                                        if(tile && (tile->flags & TILE_FLAG_WALKABLE)) {
                                            safeFound++;
                                        } else {
                                            looking = false;
                                        }                     
                                    } else {
                                        looking = false;
                                    }
                                }
                            }

                            if(safeFound > maxSafeFound && searching) {
                                //NOTE: Keep track of the best one we've found so far
                                maxSafeFound = safeFound;
                                offset.xy = make_float2(chunkWorldP.x + localX, chunkWorldP.y + localY);
                            }

                            if(safeFound == safeAreaSize) {
                                searching = false;
                                offset.xy = make_float2(chunkWorldP.x + localX, chunkWorldP.y + localY);
                            }
                        }
                    }
                }
            }
        }
    }

    assert(!searching);

    return offset;
}
void addBoardPosToAverage(GameState *gameState, float3 pos) { 
    gameState->averageStartBoardCount++;
    gameState->averageStartBoardPos = plus_float3(gameState->averageStartBoardPos, pos);
}

void markTileAsUsed(GameState *gameState, float3 boardP, bool markUnWalkable = false) {
    addBoardPosToAverage(gameState, boardP);
     //NOTE: Now say we've used this cell up
     float2 chunkP = getChunkPosForWorldP(boardP.xy);
     Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState,  &gameState->textureAtlas,chunkP.x, chunkP.y, 0, true, true);

     float3 localP = getChunkLocalPos(boardP.x, boardP.y, boardP.z);

     assert(c->visited);
     assert(localP.x < CHUNK_DIM && localP.y < CHUNK_DIM);

     assert(!c->visited[(int)localP.y*CHUNK_DIM + (int)localP.x]);
     c->visited[(int)localP.y*CHUNK_DIM + (int)localP.x] = true;

     if(markUnWalkable) {
        Tile *tile = c->getTile(localP.x, localP.y, localP.z);
        if(tile) {
            assert((tile->flags & TILE_FLAG_WALKABLE));
            //NOTE: Mark unwalkable
            tile->flags &= (~TILE_FLAG_WALKABLE);
            assert(!(tile->flags & TILE_FLAG_WALKABLE));
        }

     }
}


void placeSingleKnight(GameState *gameState, float3 offset, PlacementType type) {
    DEBUG_TIME_BLOCK();
    if(type == UNIT_PLACE_SAFE) {
        int pWidth = 1;
        int pHeight = 1;
        //NOTE: Find a safe location to spawn the knight unit
        offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);
    }

    {
        float3 p = offset;
        p.z = getMapHeight(p.x, p.y);
        float3 boardP = convertRealWorldToBlockCoords(p);

        addKnightEntity(gameState, boardP);

        if(type == UNIT_PLACE_SAFE) {
            markTileAsUsed(gameState, boardP);
        }
    }
        
}


void placeGoblinSingleUnit(GameState *gameState, float3 offset, PlacementType type) {
    DEBUG_TIME_BLOCK();
    if(type == UNIT_PLACE_SAFE) {
        int pWidth = 1;
        int pHeight = 1;
        //NOTE: Find a safe location to spawn the knight unit
        offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);
    }

    {
        float3 p = offset;
        p.z = getMapHeight(p.x, p.y);
        float3 boardP = convertRealWorldToBlockCoords(p);

        addGoblinEntity(gameState, boardP);

        if(type == UNIT_PLACE_SAFE) {
            markTileAsUsed(gameState, boardP);
        }
    }
        
}

void placeKnightUnit(GameState *gameState, float3 offset, PlacementType type) {
    DEBUG_TIME_BLOCK();
    int unitCount = 6;
    int rowCount = 2;
    int halfUnitCount = unitCount / rowCount;
    int widthScale = 1;

    if(type == UNIT_PLACE_SAFE) {
        int pWidth = halfUnitCount*widthScale;
        int pHeight = rowCount;
        //NOTE: Find a safe location to spawn the knight unit
        offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);
    }

    for(int i = 0; i < unitCount; i++) {
        float3 p = offset;

        p.x += (widthScale*(i % halfUnitCount));
        p.y += i / halfUnitCount;
        p.z = getMapHeight(p.x, p.y);
        float3 boardP = convertRealWorldToBlockCoords(p);
        addKnightEntity(gameState, boardP);

       markTileAsUsed(gameState, boardP);
    }
}

void placePeasantUnit(GameState *gameState, float3 offset, PlacementType type) {
    DEBUG_TIME_BLOCK();
    if(type == UNIT_PLACE_SAFE) {
        int pWidth = 1;
        int pHeight = 1;
        //NOTE: Find a safe location to spawn the knight unit
        offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);
    }

    {
        float3 p = offset;
        p.z = getMapHeight(p.x, p.y);
        float3 boardP = convertRealWorldToBlockCoords(p);

        addPeasantEntity(gameState, boardP);

        if(type == UNIT_PLACE_SAFE) {
            markTileAsUsed(gameState, boardP);
        }
    }
        
}


void placeGoblinTower(GameState *gameState, float3 offset) {
    DEBUG_TIME_BLOCK();
    int pWidth = 2;
    int pHeight = 2 + 2; //NOTE: Plus two for safe location out the front
    
    //NOTE: Find a safe location to spawn the tower unit
    offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);

    for(int y = 0; y < pHeight; y++) {
        for(int x = 0; x < pWidth; x++) {
            float3 p = offset;
            p.x += x;
            p.y += y;
            p.z = getMapHeight(p.x, p.y);
            float3 boardP = convertRealWorldToBlockCoords(p);
            markTileAsUsed(gameState, boardP, y > 1); //NOTE: Mark chunk as not WALKABLE aswell
        }
    }

    float3 p = offset;
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    p.x += 0.5f;
    p.y += 2.5f; //NOTE: We move it to the center of the two tiles
    addGoblinTowerEntity(gameState, p);

    // p = offset;
    // p.y += 1;
    // p.x += random_between_int(0, 2);
    // p.z = getMapHeight(p.x, p.y);
    // p = convertRealWorldToBlockCoords(p);
    // placePeasantUnit(gameState, p, UNIT_PLACE_EXACT);
}

void placeTower(GameState *gameState, float3 offset) {
    DEBUG_TIME_BLOCK();
    int pWidth = 2;
    int pHeight = 2 + 2; //NOTE: Plus two for safe location out the front
    
    //NOTE: Find a safe location to spawn the house unit
    offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);

    for(int y = 0; y < pHeight; y++) {
        for(int x = 0; x < pWidth; x++) {
            float3 p = offset;
            p.x += x;
            p.y += y;
            p.z = getMapHeight(p.x, p.y);
            float3 boardP = convertRealWorldToBlockCoords(p);
            markTileAsUsed(gameState, boardP, y > 1); //NOTE: Mark chunk as not WALKABLE aswell
        }
    }

    float3 p = offset;
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    p.x += 0.5f;
    p.y += 2.5f; //NOTE: We move it to the center of the two tiles
    addTowerEntity(gameState, p);

    // p = offset;
    // p.y += 1;
    // p.x += random_between_int(0, 2);
    // p.z = getMapHeight(p.x, p.y);
    // p = convertRealWorldToBlockCoords(p);
    // placePeasantUnit(gameState, p, UNIT_PLACE_EXACT);
}

void placeHouse(GameState *gameState, float3 offset) {
    DEBUG_TIME_BLOCK();
    int pWidth = 2;
    int pHeight = 2 + 2; //NOTE: Plus two for safe location out the front
    
    //NOTE: Find a safe location to spawn the house unit
    offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);

    for(int y = 0; y < pHeight; y++) {
        for(int x = 0; x < pWidth; x++) {
            float3 p = offset;
            p.x += x;
            p.y += y;
            p.z = getMapHeight(p.x, p.y);
            float3 boardP = convertRealWorldToBlockCoords(p);
            markTileAsUsed(gameState, boardP, y > 1); //NOTE: Mark chunk as not WALKABLE aswell
        }
    }

    float3 p = offset;
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    p.x += 0.5f;
    p.y += 2.5f; //NOTE: We move it to the center of the two tiles
    addHouseEntity(gameState, p);

    p = offset;
    p.y += 1;
    p.x += random_between_int(0, 2);
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    placePeasantUnit(gameState, p, UNIT_PLACE_EXACT);
}

void placeGoblinHouse(GameState *gameState, float3 offset) {
    DEBUG_TIME_BLOCK();
    int pWidth = 2;
    int pHeight = 2 + 2; //NOTE: Plus two for safe location out the front
    
    //NOTE: Find a safe location to spawn the house unit
    offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);

    for(int y = 0; y < pHeight; y++) {
        for(int x = 0; x < pWidth; x++) {
            float3 p = offset;
            p.x += x;
            p.y += y;
            p.z = getMapHeight(p.x, p.y);
            float3 boardP = convertRealWorldToBlockCoords(p);
            markTileAsUsed(gameState, boardP, y > 1); //NOTE: Mark chunk as not WALKABLE aswell
        }
    }

    float3 p = offset;
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    p.x += 0.5f;
    p.y += 2.5f; //NOTE: We move it to the center of the two tiles
    addGoblinHouseEntity(gameState, p);

    p = offset;
    p.y += 1;
    p.x += random_between_int(0, 2);
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    placeGoblinSingleUnit(gameState, p, UNIT_PLACE_EXACT);
}

void placeCastle(GameState *gameState, float3 offset) {
    DEBUG_TIME_BLOCK();
    int pWidth = 6;
    int pHeight = 3 + 2; //NOTE: + 2 for safe place out the front
        //NOTE: Find a safe location to spawn the hosue unit
    offset = getSafeSpawnLocation(gameState, offset, pWidth, pHeight);

    for(int y = 0; y < pHeight; y++) {
        for(int x = 0; x < pWidth; x++) {
            float3 p = offset;
            p.x += x;
            p.y += y;
            p.z = getMapHeight(p.x, p.y);
            float3 boardP = convertRealWorldToBlockCoords(p);
            markTileAsUsed(gameState, boardP, y > 1); //NOTE: Mark chunk as not WALKABLE aswell only if it's the ones not one out front   
        }
    }

    float3 p = offset;
    p.z = getMapHeight(p.x, p.y);
    p = convertRealWorldToBlockCoords(p);
    p.x += 2.5f; //NOTE: We move it to the center of the two tiles
    p.y += 3.5f; //NOTE: We move it to the center of the two tiles
    addCastleEntity(gameState, p);

    {
        //NOTE: Put two peasants out the front of the castle
        p = offset;
        p.y += 1;
        p.x += 2;
        p.z = getMapHeight(p.x, p.y);
        p = convertRealWorldToBlockCoords(p);
        placeSingleKnight(gameState, p, UNIT_PLACE_EXACT);

        p.x += 1;
        p.z = getMapHeight(p.x, p.y);
        p = convertRealWorldToBlockCoords(p);
        placeSingleKnight(gameState, p, UNIT_PLACE_EXACT);
    }
}

void initPlayerBoard(GameState *gameState) {
    DEBUG_TIME_BLOCK();
    gameState->averageStartBoardPos = make_float3(0, 0, 0);
    gameState->averageStartBoardCount = 1;
    
    MemoryArenaMark mark = takeMemoryMark(&globalPerFrameArena);

    float3 p = make_float3(-CHUNK_DIM, -CHUNK_DIM, 0);

    addManEntity(gameState, p);
    addTemplerKnightEntity(gameState, plus_float3(p, make_float3(10, 0, 0)));


    int startBoardSize = 10;
    for(int x = -startBoardSize; x < startBoardSize; ++x) {
        for(int y = -startBoardSize; y < startBoardSize; ++y) {
            getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, x, y, 0, true, true);
        }
    }

    gameState->cameraPos.x = p.x;
    gameState->cameraPos.y = p.y;

    releaseMemoryMark(&mark);
}
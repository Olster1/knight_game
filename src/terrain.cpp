struct CubeVertex {
    float3 pos;
    float3 normal;
};

CubeVertex make_cube_vertex(float3 pos, float3 normal) {
    CubeVertex result = {};

    result.pos = pos;
    result.normal = normal;

    return result;
}

static CubeVertex global_cubeData[] = {
    // Top face (z = 1.0f)
    make_cube_vertex(make_float3(0.5f, -0.5f, 0.5f), make_float3(0, 0, 1)),
    make_cube_vertex(make_float3(-0.5f, -0.5f, 0.5f), make_float3(0, 0, 1)),
    make_cube_vertex(make_float3(-0.5f, 0.5f,  0.5f), make_float3(0, 0, 1)),
    make_cube_vertex(make_float3(0.5f, 0.5f,  0.5f), make_float3(0, 0, 1)),
    // Back face (y = -1.0f)
    make_cube_vertex(make_float3(0.5f, -0.5f, -0.5f), make_float3(0, -1, 0)),
    make_cube_vertex(make_float3(-0.5f, -0.5f, -0.5f), make_float3(0, -1, 0)),
    make_cube_vertex(make_float3(-0.5f, -0.5f, 0.5f), make_float3(0, -1, 0)),
    make_cube_vertex(make_float3(0.5f, -0.5f, 0.5f), make_float3(0, -1, 0)),
};

int getMapHeight(float worldX, float worldY) {
    // float maxHeight = 3.0f;
    // int height = round(maxHeight*mapSimplexNoiseTo11(SimplexNoise_fractal_2d(8, 0.4*round(worldX), 0.4*round(worldY), 0.03f)));
    return 0;
}

TileType getLandscapeValue(int worldX, int worldY, int worldZ) {
    DEBUG_TIME_BLOCK()
    
    int height = getMapHeight(worldX, worldY);

    TileType type = TILE_TYPE_NONE;
    if(height >= 0 && worldZ <= height) {
        type = TILE_TYPE_SOLID;
    }

    return type;
}

u32 calculateLightingMask(int worldX, int worldY, int worldZ, LightingOffsets *offsets) {
    DEBUG_TIME_BLOCK()
    u32 result = 0;

    float3 worldP = make_float3(worldX, worldY, worldZ);

    for(int i = 0; i < arrayCount(global_cubeData); ++i) {
        CubeVertex v = global_cubeData[i];

        bool blockValues[3] = {false, false, false};
        
        for(int j = 0; j < arrayCount(blockValues); j++) {
            float3 p = plus_float3(worldP, offsets->aoOffsets[i].offsets[j]);
            if(getLandscapeValue(p.x, p.y, p.z) == TILE_TYPE_SOLID) {
                blockValues[j] = true; 
            }
        }

        //NOTE: Get the ambient occulusion level
        uint64_t value = 0;
        //SPEED: Somehow make this not into if statments
        if(blockValues[0] && blockValues[2])  {
            value = 3;
        } else if((blockValues[0] && blockValues[1])) {
            assert(!blockValues[2]);
            value = 2;
        } else if((blockValues[1] && blockValues[2])) {
            assert(!blockValues[0]);
            value = 2;
        } else if(blockValues[0]) {
            assert(!blockValues[1]);
            assert(!blockValues[2]);
            value = 1;
        } else if(blockValues[1]) {
            assert(!blockValues[0]);
            assert(!blockValues[2]);
            value = 1;
        } else if(blockValues[2]) {
            assert(!blockValues[0]);
            assert(!blockValues[1]);
            value = 1;
        } 
        
        //NOTE: Times 2 because each value need 2 bits to write 0 - 3. 
        result |= (value << (uint64_t)(i*2)); //NOTE: Add the mask value
        // assert(((i + 1)*2) < AO_BIT_NOT_VISIBLE); //NOTE: +1 to account for the top bit
    }


    result = (uint64_t)result;


    return result;
}


bool hasGrassyTop(int worldX, int worldY, int worldZ) {
    DEBUG_TIME_BLOCK()
    float perlin = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(8, worldX, worldY, 0.1f));
    bool result = perlin > 0.5f;

    float mapHeight = getMapHeight(worldX, worldY);
    
    //NOTE: Only on the top
    if(worldZ != mapHeight) {
        result = false;
    }
    
    return result;
}

bool hasDecorBush(int worldX, int worldY, int worldZ) {
    DEBUG_TIME_BLOCK()
    float scale = 10;
    float perlin = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(8, scale*worldX, scale*worldY, 1.01f));
    bool result = perlin > 0.65f;

    float mapHeight = getMapHeight(worldX, worldY);

    //NOTE: Only on the top
    if(worldZ != mapHeight) {
        result = false;
    }

    return result;
}

// Stateless hash from two ints
uint32_t coord_hash_2d(int x, int y) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263); // big primes
    h = (h ^ (h >> 13)) * 1274126177;
    h ^= h >> 16;
    return h;
}

// Convert to float in [0,1)
float coord_rand01_2d(int x, int y) {
    return (coord_hash_2d(x, y) & 0xFFFFFF) / (float)0x1000000;
}

uint32_t hash_coord_1d(uint32_t x) {
    x = (x ^ 0x27d4eb2d) * 0x165667b1;  // mix with big odd constants
    x ^= x >> 15;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    return x;
}

float coord_rand01_1d(uint32_t x) {
    return (hash_coord_1d(x) & 0xFFFFFF) / (float)0x1000000;
}

static inline uint32_t hash_u32(uint32_t v) {
    v ^= v >> 16;
    v *= 0x7feb352d;
    v ^= v >> 15;
    v *= 0x846ca68b;
    v ^= v >> 16;
    return v;
}

void rand2_from_coords(int x, int y, float *out1, float *out2) {
    uint32_t n = (uint32_t)(x * 374761393u + y * 668265263u);

    uint32_t h1 = hash_u32(n);         // first hash
    uint32_t h2 = hash_u32(n ^ 0x9e3779b9u); // second hash with different salt

    *out1 = (h1 & 0xFFFFFF) / (float)0x1000000;
    *out2 = (h2 & 0xFFFFFF) / (float)0x1000000;
}

struct WorldGenerationPositionInfo {
    bool valid;
    float randomValue;
};

WorldGenerationPositionInfo isCellPosition(int worldX, int worldY, int cellSizeInVoxels, int marginInVoxels) {
     WorldGenerationPositionInfo result = {};

    int totalMarginForBothSides = cellSizeInVoxels;
    
    float3 cellSize = make_float3(totalMarginForBothSides, totalMarginForBothSides, totalMarginForBothSides);
    float3 worldVoxelP = make_float3((int)(worldX), (int)(worldY), 0);

    int cellX = roundChunkCoord((float)worldVoxelP.x / (float)cellSize.x);
    int cellY = roundChunkCoord((float)worldVoxelP.y / (float)cellSize.y);

    float xNoise;
    float yNoise;

    result.randomValue = coord_rand01_2d(cellX, cellY);

    rand2_from_coords(cellX, cellY, &xNoise, &yNoise);

    int xOffset = (int)lerp(0, marginInVoxels, make_lerpTValue(xNoise));
    int yOffset = (int)lerp(0, marginInVoxels, make_lerpTValue(yNoise));
    
    float3 cellTargetP = make_float3(xOffset, yOffset, 0);
    int remainderX_centerBased = (worldVoxelP.x - (cellX * cellSize.x));
    int remainderY_centerBased = (worldVoxelP.y - (cellY * cellSize.y));
    assert(remainderX_centerBased >= 0);
    assert(remainderY_centerBased >= 0);

    if(remainderX_centerBased == (int)cellTargetP.x && remainderY_centerBased == (int)cellTargetP.y) {
        result.valid = true;
    }
    return result;
}

Entity *addAshTreeEntity(GameState *state, float3 worldP);
Entity *addAlderTreeEntity(GameState *state, float3 worldP);
Entity *addBearEntity(GameState *state, float3 worldP);
Entity *addGhostEntity(GameState *state, float3 worldP);

u32 addTreeIfHasTree(GameState *state, int worldX, int worldY) {
    u32 flags = TILE_FLAG_WALKABLE;
    DEBUG_TIME_BLOCK()
    WorldGenerationPositionInfo info = isCellPosition(worldX, worldY, 8, 7);

    if(info.valid) {
        if(info.randomValue < 0.3f) {
            addAshTreeEntity(state, make_float3(worldX, worldY, 0));
            flags = 0;
        } else if(info.randomValue < 1) {
            addAlderTreeEntity(state, make_float3(worldX, worldY, 0));
            flags = 0;
        }
    }
    return flags;
}

bool hasBearEntity(int worldX, int worldY) {
    DEBUG_TIME_BLOCK()
    WorldGenerationPositionInfo info = isCellPosition(worldX, worldY, 80, 40);
    bool result = info.valid;

    if(info.randomValue < 0.4f) {
        result = false;
    }
    return result;
}


bool hasGhostEntity(int worldX, int worldY) {
    DEBUG_TIME_BLOCK()
    WorldGenerationPositionInfo info = isCellPosition(worldX, worldY, 40, 20);
    bool result = info.valid;

    if(info.randomValue < 0.2f) {
        result = false;
    }
    return result;
}


bool isWaterRock(int worldX, int worldY, int worldZ) {
    DEBUG_TIME_BLOCK()
    bool result = false;
    int height = getMapHeight(worldX, worldY);
    if(height == -1 && worldZ == 0) {
        float perlin = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(8, worldX, worldY, 100.01f));
        result = perlin > 0.75f;
    }
    return result;
}

Chunk *Terrain::generateChunk(int x, int y, int z, uint32_t hash, Memory_Arena *tempArena) {
    DEBUG_TIME_BLOCK()
    Chunk *chunk = (Chunk *)pushStruct(&global_long_term_arena, Chunk);
    *chunk = Chunk(tempArena);
    assert(chunk);
    assert(!chunk->generatedMipMaps);

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;
    chunk->generateState = CHUNK_NOT_GENERATED;

    chunk->next = chunks[hash];
    chunks[hash] = chunk;

    return chunk;
}


bool tileIsOccupied(GameState *gameState, float3 worldP);

void fillChunk(GameState *gameState, LightingOffsets *lightingOffsets, AnimationState *animationState, TextureAtlas *textureAtlas, Chunk *chunk) {
    DEBUG_TIME_BLOCK()
    assert(chunk);
    assert(chunk->generateState & CHUNK_NOT_GENERATED);
    chunk->generateState = CHUNK_GENERATING;

    chunk->cloudFadeTimer = 0; //NOTE: for fading out the fog of war

    float3 chunkP = getChunkWorldP(chunk);
    int chunkWorldX = roundChunkCoord(chunkP.x);
    int chunkWorldY = roundChunkCoord(chunkP.y);
    int chunkWorldZ = roundChunkCoord(chunkP.z);

    for(int y = 0; y < CHUNK_DIM; ++y) {
        for(int x = 0; x < CHUNK_DIM; ++x) {
            int worldX = chunkWorldX + x;
            int worldY = chunkWorldY + y;

            Tile *tile = &chunk->tiles[y*CHUNK_DIM + x];
            u32 flags = TILE_FLAG_WALKABLE;

            if(hasBearEntity(worldX, worldY)) {
                addBearEntity(gameState, make_float3(worldX, worldY, 0));
            }
            if(hasGhostEntity(worldX, worldY)) {
                assert(!tileIsOccupied(gameState, make_float3(worldX, worldY, 0)));
                addGhostEntity(gameState, make_float3(worldX, worldY, 0));
                assert(tileIsOccupied(gameState, make_float3(worldX, worldY, 0)));
            }

            if(!tileIsOccupied(gameState, make_float3(worldX, worldY, 0))){
                flags = addTreeIfHasTree(gameState, worldX, worldY);
            }

            tile->flags |= flags;
            
        }
    }

    chunk->generateState = CHUNK_GENERATED;

}

Chunk *getChunk(GameState *gameState, LightingOffsets *lightingOffsets, AnimationState *animationState, TextureAtlas *textureAtlas, int x, int y, int z, bool shouldGenerateChunk, bool shouldGenerateFully, Memory_Arena *tempArena = 0) {
    DEBUG_TIME_BLOCK()
    uint32_t hash = getHashForChunk(x, y, z);

    Chunk *chunk = gameState->terrain.chunks[hash];

    bool found = false;

    while(chunk && !found) {
        if(chunk->x == x && chunk->y == y && chunk->z == z) {
            found = true;
            break;
        }
        chunk = chunk->next;
    }

    if((!chunk && shouldGenerateChunk)) {
        chunk = gameState->terrain.generateChunk(x, y, z, hash, tempArena);
    }

    if(chunk && shouldGenerateFully && (chunk->generateState & CHUNK_NOT_GENERATED)) {
        //NOTE: Launches multi-thread work
        fillChunk(gameState, lightingOffsets, animationState, textureAtlas, chunk);
    } 

    return chunk;
}
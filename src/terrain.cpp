Chunk *Terrain::generateChunk(int x, int y, int z, uint32_t hash, Memory_Arena *tempArena) {
    DEBUG_TIME_BLOCK()
    Chunk *chunk = (Chunk *)pushStruct(&global_long_term_arena, Chunk);
    *chunk = Chunk(tempArena);
    assert(chunk);

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;
    chunk->generateState = CHUNK_NOT_GENERATED;

    chunk->next = chunks[hash];
    chunks[hash] = chunk;

    return chunk;
}

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


// static Vertex global_quadData[] = {
//     makeVertex(make_float3(0.5f, -0.5f, 0), make_float2(1, 1)),
//     makeVertex(make_float3(-0.5f, -0.5f, 0), make_float2(0, 1)),
//     makeVertex(make_float3(-0.5f,  0.5f, 0), make_float2(0, 0)),
//     makeVertex(make_float3(0.5f, 0.5f, 0), make_float2(1, 0)),
// };

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
    float maxHeight = 3.0f;
    int height = round(maxHeight*mapSimplexNoiseTo11(SimplexNoise_fractal_2d(8, 0.4*round(worldX), 0.4*round(worldY), 0.03f)));
    return height;
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

bool hasTree(int worldX, int worldY, int worldZ) {
    DEBUG_TIME_BLOCK()
    float scaleFactor = 1.0f;
    float perlin = mapSimplexNoiseTo01(SimplexNoise_fractal_2d(8, scaleFactor*worldX, scaleFactor*worldY, 10.f));
    bool result = perlin > 0.7f;

    float mapHeight = getMapHeight(worldX, worldY);

    //NOTE: Only on the top
    if(worldZ != mapHeight) {
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

static TileMapCoords global_tileLookup[16] = {
    {3, 3}, // 0000 - No beach tiles
    {3, 2}, // 0001 - Top
    {3, 0}, // 0010 - Bottom
    {3, 1}, // 0011 - Top & Bottom
    {0, 3}, // 0100 - Right
    {0, 2}, // 0101 - Top & Right
    {0, 0}, // 0110 - Bottom & Right
    {0, 1}, // 0111 - Top, Bottom, Right
    {2, 3}, // 1000 - Left
    {2, 2}, // 1001 - Top & Left
    {2, 0}, // 1010 - Bottom & Left
    {2, 1}, // 1011 - Top, Bottom, Left
    {1, 3}, // 1100 - Left & Right
    {1, 2}, // 1101 - Top, Right, Left
    {1, 0}, // 1110 - Bottom, Right, Left
    {1, 1}  // 1111 - Surrounded by beach tiles
};

static TileMapCoords global_tileLookup_elevated[16] = {
    {3, 4}, // 0000 - No beach tiles
    {3, 2}, // 0001 - Top
    {3, 0}, // 0010 - Bottom
    {3, 1}, // 0011 - Top & Bottom
    {0, 4}, // 0100 - Right
    {0, 2}, // 0101 - Top & Right
    {0, 0}, // 0110 - Bottom & Right
    {0, 1}, // 0111 - Top, Bottom, Right
    {2, 4}, // 1000 - Left
    {2, 2}, // 1001 - Top & Left
    {2, 0}, // 1010 - Bottom & Left
    {2, 1}, // 1011 - Top, Bottom, Left
    {1, 4}, // 1100 - Left & Right
    {1, 2}, // 1101 - Top, Right, Left
    {1, 0}, // 1110 - Bottom, Right, Left
    {1, 1}  // 1111 - Surrounded by beach tiles
};

void Terrain::fillChunk(LightingOffsets *lightingOffsets, AnimationState *animationState, TextureAtlas *textureAtlas, Chunk *chunk) {
    DEBUG_TIME_BLOCK()
    assert(chunk);
    assert(chunk->generateState & CHUNK_NOT_GENERATED);
    chunk->generateState = CHUNK_GENERATING;

    chunk->cloudFadeTimer = 0; //NOTE: for fading out the fog of war

    // float3 chunkP = getChunkWorldP(chunk);
    // int chunkWorldX = roundChunkCoord(chunkP.x);
    // int chunkWorldY = roundChunkCoord(chunkP.y);
    // int chunkWorldZ = roundChunkCoord(chunkP.z);

    // char *decorNames[] = {"bush1.png", "bush2.png", "bush4.png", "bush5.png",};
      
    // for(int z = 0; z < CHUNK_DIM; ++z) {
    //     for(int y = 0; y < CHUNK_DIM; ++y) {
    //         for(int x = 0; x < CHUNK_DIM; ++x) {
    //             int worldX = chunkWorldX + x;
    //             int worldY = chunkWorldY + y;
    //             int worldZ = chunkWorldZ + z;

    //             TileType type = getLandscapeValue(worldX, worldY, worldZ);

    //             if(type == TILE_TYPE_SOLID) {
    //                 Animation *animation = 0;
    //                 TileMapCoords tileCoords = {};
    //                 TileMapCoords tileCoordsSecondary = {};
    //                 u32 flags = 0;
    //                 u32 lightingMask = calculateLightingMask(worldX, worldY, worldZ, lightingOffsets);

    //                 u8 bits = 0;
    //                 if (getLandscapeValue(worldX, worldY + 1, worldZ) == TILE_TYPE_SOLID) bits |= 1 << 0;
    //                 if (getLandscapeValue(worldX, worldY - 1, worldZ) == TILE_TYPE_SOLID) bits |= 1 << 1;
    //                 if (getLandscapeValue(worldX + 1, worldY, worldZ) == TILE_TYPE_SOLID) bits |= 1 << 2;
    //                 if (getLandscapeValue(worldX - 1, worldY, worldZ) == TILE_TYPE_SOLID) bits |= 1 << 3;

    //                 if(worldZ == 0) {
    //                     animation = &animationState->waterAnimation;
    //                     tileCoords = global_tileLookup[bits];
    //                     tileCoords.x += 5;
    //                     type = TILE_TYPE_BEACH;

                     

    //                 } else if(worldZ > 0) {
    //                     // type = TILE_TYPE_NONE;
    //                     tileCoords = global_tileLookup_elevated[bits];
    //                     tileCoords.x += 10;
    //                     tileCoordsSecondary = global_tileLookup[bits];
    //                     flags |= TILE_FLAG_SHADOW;
    //                     type = TILE_TYPE_ROCK;
    //                     if(bits == 0b0101 || bits == 0b1101 || bits == 0b1001 || bits == 0b0001 || 
    //                         bits == 0b0000 || bits == 0b0100 || bits == 0b1000 || bits == 0b1100) {
    //                         flags |= TILE_FLAG_FRONT_FACE;

    //                         //NOTE: Now check whether it should have dirt at the front 
    //                         if(getLandscapeValue(worldX, worldY - 1, worldZ - 1) == TILE_TYPE_SOLID) {
    //                             if((worldZ - 1) == 0) {
    //                                 flags |= TILE_FLAG_FRONT_BEACH;
    //                             } else if(hasGrassyTop(worldX, worldY - 1, worldZ - 1)) {
    //                                 flags |= TILE_FLAG_FRONT_GRASS;
    //                             }
    //                         }
                            
    //                     }

    //                     if(hasGrassyTop(worldX, worldY, worldZ)) {
    //                         flags |= TILE_FLAG_GRASSY_TOP;
                            

    //                         //NOTE: Get new bits based on whether grass is next to it
    //                         u8 bits2 = 0;
    //                         if (hasGrassyTop(worldX, worldY + 1, worldZ) && (bits & (1 << 0))) bits2 |= 1 << 0;
    //                         if (hasGrassyTop(worldX, worldY - 1, worldZ) && (bits & (1 << 1))) bits2 |= 1 << 1;
    //                         if (hasGrassyTop(worldX + 1, worldY, worldZ) && (bits & (1 << 2))) bits2 |= 1 << 2;
    //                         if (hasGrassyTop(worldX - 1, worldY, worldZ) && (bits & (1 << 3))) bits2 |= 1 << 3;
        
    //                         tileCoordsSecondary = global_tileLookup[bits2];

    //                         // if(hasTree(worldX, worldY, worldZ)) {
    //                         //     assert(worldZ > 0);
    //                         //     assert(chunk->treeSpriteCount < arrayCount(chunk->treeSpritesWorldP));
    //                         //     if(chunk->treeSpriteCount < arrayCount(chunk->treeSpritesWorldP)) {
    //                         //         chunk->treeSpritesWorldP[chunk->treeSpriteCount++] = make_float3(worldX, worldY, worldZ);
    //                         //     }
                        
    //                         //     flags |= TILE_FLAG_TREE;
    //                         // } 
    //                     }
    //                 }

    //                 if(type != TILE_TYPE_NONE) {
    //                     if(!(flags & TILE_FLAG_TREE)) {
    //                         //NOTE: Can walk on this tile
    //                         flags |= TILE_FLAG_WALKABLE;
    //                     }
    //                     chunk->tiles[z*CHUNK_DIM*CHUNK_DIM + y*CHUNK_DIM + x] = Tile(type, &animationState->animationItemFreeListPtr, animation, tileCoords, tileCoordsSecondary, flags, lightingMask);
    //                 }

    //                 //NOTE: Add decor item
    //                 assert(chunk->decorSpriteCount < arrayCount(chunk->decorSprites));
    //                 if(chunk->decorSpriteCount < arrayCount(chunk->decorSprites)) {
                        
    //                     int index = random_between_int(0, arrayCount(decorNames));
    //                     assert(index < arrayCount(decorNames));

    //                     float mapHeight = getMapHeight(worldX, worldY);

    //                     if(mapHeight > 0 && mapHeight == worldZ && hasDecorBush(worldX, worldY, worldZ)) {
            
    //                         float3 worldP = {};
                            
    //                         worldP.x = random_between_float(-0.2f, 0.2f) + worldX;
    //                         worldP.y = random_between_float(-0.2f, 0.2f) + worldY;
    //                         worldP.z = mapHeight;
                
    //                         AtlasAsset *asset = 0;
    //                         if(hasGrassyTop(round(worldP.x), round(worldP.y), worldP.z)) {
    //                             asset = textureAtlas_getItem(textureAtlas, decorNames[index]);
    //                         } else {
    //                             asset = textureAtlas_getItem(textureAtlas, "rock.png");
    //                         }
    //                         assert(asset);
                
    //                         // if(worldP.z > 0) {
    //                         //     DecorSprite *obj = chunk->decorSprites + chunk->decorSpriteCount++;
    //                         //     obj->worldP = worldP;
    //                         //     obj->uvs = asset->uv;
    //                         //     obj->textureHandle = textureAtlas->texture.handle;
    //                         //     float s = random_between_float(0.4f, 0.8f);
    //                         //     obj->scale = make_float2(s, s);
    //                         // }
    //                     }
    //                 }
            
                    
    //             } else {
    //                 bool waterRock = false;//isWaterRock(worldX, worldY, worldZ);

    //                 if(waterRock) {
    //                     TileMapCoords tileCoords = {};
    //                     TileMapCoords tileCoordsSecondary = {};
    //                     chunk->tiles[z*CHUNK_DIM*CHUNK_DIM + y*CHUNK_DIM + x] = Tile(TILE_TYPE_WATER_ROCK, &animationState->animationItemFreeListPtr, &animationState->waterRocks[0], tileCoords, tileCoordsSecondary, 0, 0);
    //                 }
    //             }
    //         }
    //     }
    // }

    chunk->generateState = CHUNK_GENERATED;

}

Chunk *Terrain::getChunk(LightingOffsets *lightingOffsets, AnimationState *animationState, TextureAtlas *textureAtlas, int x, int y, int z, bool shouldGenerateChunk, bool shouldGenerateFully, Memory_Arena *tempArena) {
    DEBUG_TIME_BLOCK()
    uint32_t hash = getHashForChunk(x, y, z);

    Chunk *chunk = chunks[hash];

    bool found = false;

    while(chunk && !found) {
        if(chunk->x == x && chunk->y == y && chunk->z == z) {
            found = true;
            break;
        }
        chunk = chunk->next;
    }

    if((!chunk && shouldGenerateChunk)) {
        chunk = generateChunk(x, y, z, hash, tempArena);
    }

    if(chunk && shouldGenerateFully && (chunk->generateState & CHUNK_NOT_GENERATED)) {
        //NOTE: Launches multi-thread work
        fillChunk(lightingOffsets, animationState, textureAtlas, chunk);
    } 

    return chunk;
}
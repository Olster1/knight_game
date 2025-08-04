//NOTE: Helper funciton since the simplex lib I'm using for 3d noise maps between -1 -> 1, and we want 0 -> 1
float mapSimplexNoiseTo01(float value) {
    value += 1;
    value *= 0.5f;

    assert(value >= 0 && value <= 1);

    return value;
}
float mapSimplexNoiseTo11(float value) {
    return value;
}

struct AOOffset {
    float3 offsets[3];
};

struct LightingOffsets {
    AOOffset aoOffsets[8];
};

struct DecorSprite {
    float2 scale;
    float3 worldP;
    float4 uvs;
    TextureHandle *textureHandle;
};

enum TileType {
    TILE_TYPE_NONE,
    TILE_TYPE_WATER,
    TILE_TYPE_BEACH,
    TILE_TYPE_ROCK,
    TILE_TYPE_SOLID,
    TILE_TYPE_WATER_ROCK,

};

struct TileMapCoords {
    int x;
    int y;
};

enum TileFlags {
    TILE_FLAG_FRONT_FACE = 1 << 0, //NOTE: This is whether we should render the front face for 3d
    TILE_FLAG_GRASSY_TOP = 1 << 1, //NOTE: Wether a block should have an additional grassy top - controlled by perlin noise if it is a rock type
    TILE_FLAG_SHADOW = 1 << 2, //NOTE: Wether a block should have an additional grassy top - controlled by perlin noise if it is a rock type
    TILE_FLAG_FRONT_GRASS = 1 << 3, 
    TILE_FLAG_FRONT_BEACH = 1 << 4, 
    TILE_FLAG_TREE = 1 << 5,  //NOTE: Wether this tile has a tree
    TILE_FLAG_WALKABLE = 1 << 6,  //NOTE: Wether we can walk on this tile
};

struct Tile {
    TileType type = TILE_TYPE_NONE;
    EasyAnimation_Controller *animationController = 0;
    TileMapCoords coords;
    TileMapCoords coordsSecondary;
    u32 flags = 0;
    u32 lightingMask = 0; //NOTE: Minecraft like lighting data bottom 8 bits are top surface, next 8 bits are the front facing surface 
    int entityOccupation = 0;

    Tile() {
        this->entityOccupation = 0;
    }

    Tile(TileType type, EasyAnimation_ListItem **freeList, Animation *animation, TileMapCoords coords, TileMapCoords coordsSecondary, u32 flags, u32 lightingMask) {
        this->type = type;
        this->coords = coords;
        this->coordsSecondary = coordsSecondary;
        this->flags = flags;
        this->entityOccupation = 0;
        this->lightingMask = lightingMask;
        if(type == TILE_TYPE_BEACH || type == TILE_TYPE_WATER_ROCK) {
            assert(animation);
            animationController = pushStruct(&global_long_term_arena, EasyAnimation_Controller);
            easyAnimation_initController(animationController);
            EasyAnimation_ListItem *anim = easyAnimation_addAnimationToController(animationController, freeList, animation, 0.08f);
        }
    }
};


enum ChunkGenerationState {
    CHUNK_NOT_GENERATED = 1 << 0, 
    CHUNK_GENERATING = 1 << 1, 
    CHUNK_GENERATED = 1 << 2, 
    CHUNK_MESH_DIRTY = 1 << 3, 
    CHUNK_MESH_BUILDING = 1 << 4, 
};

struct Chunk {
    int x = 0;
    int y = 0;
    int z = 0;

    FramebufferHandle texture;

    volatile int64_t generateState = 0; //NOTE: Chunk might not be generated, so check first when you get one

    //NOTE: 16 x 16 x 16
    //NOTE: Z Y
    Tile tiles[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];

    float cloudFadeTimer = -1; //NOTE: Timer for fading out the clouds
    int cloudCount = 0;
	CloudData clouds[MAX_CLOUD_DIM*MAX_CLOUD_DIM];
    FramebufferHandle cloudTexture;

    bool generatedMipMaps = false;
    bool generatedCloudMipMaps = false;

    int decorSpriteCount = 0;
	DecorSprite decorSprites[CHUNK_DIM*CHUNK_DIM];
    int treeSpriteCount = 0;
	float3 treeSpritesWorldP[CHUNK_DIM*CHUNK_DIM];

    u8 *visited; //NOTE: For when we create the board we keep track of where we've put positions. we allocate this on a temp arena when we do the board init

    // Entity *entities;
    Chunk *next = 0;

    Tile *getTile(int x, int y, int z) {
        Tile *t = 0;
        if(x >= 0 && y >= 0 && z >= 0 && x < CHUNK_DIM && y < CHUNK_DIM && z < CHUNK_DIM) {
            t = &tiles[z*CHUNK_DIM*CHUNK_DIM + y*CHUNK_DIM + x];
        }
        return t;
    }

    Chunk(Memory_Arena *tempArena) {
        generateState = CHUNK_NOT_GENERATED;
        x = 0;
        y = 0;
        z = 0;
        next = 0;
        cloudCount = 0;
        cloudFadeTimer = -1;
        decorSpriteCount = 0;
        treeSpriteCount = 0;
        texture.textureHandle = 0;
        generatedMipMaps = false;
        generatedCloudMipMaps = false;

        //NOTE: This is for when we create the board we only allocate this array
        if(tempArena) {
            visited = pushArray(tempArena, CHUNK_DIM*CHUNK_DIM, u8);
        }
    }

};

#define CHUNK_LIST_SIZE 4096

float3 getChunkWorldP(Chunk *c) {
    float3 worldP = make_float3(c->x*CHUNK_DIM, c->y*CHUNK_DIM, c->z*CHUNK_DIM);
    return worldP;
}

float3 getTileWorldP(Chunk *c, int x, int y, int z) {
    float3 worldP = make_float3(c->x*CHUNK_DIM + x, c->y*CHUNK_DIM + y, c->z*CHUNK_DIM + z);
    return worldP;
}


float3 convertRealWorldToBlockCoords(float3 p) {
    //NOTE: The origin is at the center of a block
    //NOTE: So 0.5 & 1.4 should both map to 1 - I think the +0.5
    //NOTE: So -0.5 & -1.4 should both map to -1 - I think the -0.5
    //NOTE: So -0.4 & 0.4 should both map to 0 - I think the -0.5
    p.x = round(p.x);
    p.y = round(p.y);
    p.z = round(p.z);

    return p;
}

int roundChunkCoord(float value) {
    return floor(value);
}

float2 getChunkPosForWorldP(float2 p) {
    p.x = round(p.x);
    p.y = round(p.y);

    p.x = roundChunkCoord((float)p.x / (float)CHUNK_DIM);
    p.y = roundChunkCoord((float)p.y / (float)CHUNK_DIM);

    return p;
}

float3 getChunkLocalPos(float x_, float y_, float z_)  {
    int x = (int)round(x_);
    int y = (int)round(y_);
    int z = (int)round(z_);

    //NOTE: CHUNK DIM Must be power of 2
    assert((CHUNK_DIM & (CHUNK_DIM - 1)) == 0);

    x = ((x % CHUNK_DIM) + CHUNK_DIM) % CHUNK_DIM;
    y = ((y % CHUNK_DIM) + CHUNK_DIM) % CHUNK_DIM;
    z = ((z % CHUNK_DIM) + CHUNK_DIM) % CHUNK_DIM;

    return make_float3(x, y, z);
}


uint32_t getHashForChunk(int x, int y, int z) {
    DEBUG_TIME_BLOCK()
    int values[3] = {x, y, z};
    uint32_t hash = get_crc32((char *)values, arrayCount(values)*sizeof(int));
    hash = hash & (CHUNK_LIST_SIZE - 1);
    assert(hash < CHUNK_LIST_SIZE);
    assert(hash >= 0);
    return hash;
}

struct Terrain {
    Chunk *chunks[CHUNK_LIST_SIZE];

    Terrain() {
        memset(chunks, 0, arrayCount(chunks)*sizeof(Chunk *));
    }

    Chunk *generateChunk(int x, int y, int z, uint32_t hash, Memory_Arena *tempArena);
    void fillChunk(LightingOffsets *lightingOffsets, AnimationState *animationState, TextureAtlas *textureAtlas, Chunk *chunk);
    Chunk *getChunk(LightingOffsets *lightingOffsets, AnimationState *animationState, TextureAtlas *textureAtlas, int x, int y, int z, bool shouldGenerateChunk = true, bool shouldGenerateFully = true, Memory_Arena *tempArena = 0);
       
};



#pragma pack(push, 1)
struct SaveTileV1 {
	float x;
	float y;
	float z;

	int xId;
	int yId;

	int type;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SaveEntityV1 {
	float x;
	float y;
	float z;

    char id[512];

	int type;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SaveLevelHeaderVersion {
	u32 version;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SaveLevelHeaderV1 {
	u32 tileCount;
	size_t bytesPerTile;

    u32 entityCount;
    size_t bytesPerEntity;
};
#pragma pack(pop)

void saveLevel_version1_json(GameState *gameState, char *utf8_full_file_name) {

    Platform_File_Handle handle = platform_begin_file_write_utf8_file_path(utf8_full_file_name);
    assert(!handle.has_errors);

    size_t offset = 0;

#define writeVarString(format, ...) {char *s = easy_createString_printf(&globalPerFrameArena, format, __VA_ARGS__); size_t inBytes = easyString_getSizeInBytes_utf8(s); platform_write_file_data(handle, s, inBytes, offset); offset += inBytes; }
	for(int i = 0; i < gameState->entityCount; ++i) {
		Entity *e = &gameState->entities[i];

        MemoryArenaMark memMark = takeMemoryMark(&globalPerFrameArena);

        writeVarString("{\n", "");
        writeVarString("id: \"%d\"\n", e->id);
        writeVarString("type: \"%s\"\n", MyEntity_TypeStrings[(int)e->type]);
        writeVarString("pos: %f %f %f\n", e->pos.x, e->pos.y, e->pos.z);
        writeVarString("}\n", "");

        releaseMemoryMark(&memMark);
	}

    for(int i = 0; i < gameState->tileCount; ++i) {
		MapTile t = gameState->tiles[i];

        MemoryArenaMark memMark = takeMemoryMark(&globalPerFrameArena);

        writeVarString("{\n", "");
        writeVarString("type: \"%s\"\n", "ENTITY_TILE_MAP");
        writeVarString("tileType: \"%s\"\n", MyTileMap_TypeStrings[(int)t.type]);
        writeVarString("pos: %f %f %f\n", (float)t.x, (float)t.y, 0);
        writeVarString("tileMapId: %d %d\n", t.xId, t.yId);
        writeVarString("}\n", "");

        releaseMemoryMark(&memMark);
    }

#undef writeVarString

	platform_close_file(handle);
}

char *getStringFromTokenizer(EasyTokenizer *tokenizer, Memory_Arena *arena) {
    EasyToken t = lexGetNextToken(tokenizer);
    assert(t.type == TOKEN_STRING);
    char *s = easy_createString_printf(arena, "%.*s", t.size, t.at);

    return s;
}

float3 getFloat3FromTokenizer(EasyTokenizer *tokenizer) {
    EasyToken t = lexGetNextToken(tokenizer);
    assert(t.type == TOKEN_FLOAT);
    
    float x = t.floatVal;

    t = lexGetNextToken(tokenizer);
    assert(t.type == TOKEN_FLOAT);
    
    float y = t.floatVal;

    t = lexGetNextToken(tokenizer);
    assert(t.type == TOKEN_FLOAT);
    
    float z = t.floatVal;

    return make_float3(x, y, z);
}

int findEnumValue(char *name, int nameLength, char **names, int nameCount) {
    int result = -1; //not found
    for(int i = 0; i < nameCount; i++) {
        if(easyString_stringsMatch_null_and_count(names[i], name, nameLength)) {
            result = i;
            break;
        }
    }
    assert(result >= 0);
    return result;
}

void clearEntities(GameState *state) {
    // releaseMemoryMark(&global_perFrameArenaMark);
	// global_perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);

    state->animationState.animationItemFreeListPtr = 0;

    // for(int i = 0; i < state->entityCount; ++i) {
    //     Entity *e = &state->entities[i];
    // }

    state->entityCount = 0;
    state->tileCount = 0;
}

void loadSaveLevel_json(GameState *state, char *fileName_utf8) {
    u8 *data = 0;
	size_t data_size = 0;

    bool worked = Platform_LoadEntireFile_utf8(fileName_utf8, (void **)&data, &data_size);

	if(worked && data) {
        //NOTE: Parse the json format
        EasyTokenizer tokenizer = lexBeginParsing(data, (EasyLexOptions)(EASY_LEX_EAT_SLASH_COMMENTS | EASY_LEX_OPTION_EAT_WHITE_SPACE));

        Entity entity = {};

        float2 tileMapId = make_float2(0, 0);
        TileSetType tileType = TILE_SET_SAND;

        //NOTE: Delete all the entities
        clearEntities(state);

        bool parsing = true;
        while(parsing) {
            EasyToken token = lexGetNextToken(&tokenizer);
            switch(token.type) {
                case TOKEN_NULL_TERMINATOR: {
                    parsing = false;
                } break;
                case TOKEN_OPEN_BRACKET: {
                    memset(&entity, 0, sizeof(Entity));
                } break;
                case TOKEN_WORD: {
                    if(easyString_stringsMatch_null_and_count("pos", token.at, token.size)) {
                        EasyToken t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_COLON);
                        entity.pos = getFloat3FromTokenizer(&tokenizer);
                    }
                    if(easyString_stringsMatch_null_and_count("id", token.at, token.size)) {
                        EasyToken t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_COLON);
                        t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_INTEGER);
                        entity.id = t.intVal;
                    }
                    if(easyString_stringsMatch_null_and_count("type", token.at, token.size)) {
                        EasyToken t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_COLON);

                        t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_STRING);

                        entity.type = (EntityType)findEnumValue(t.at, t.size, MyEntity_TypeStrings, arrayCount(MyEntity_TypeStrings));
                    }
                    if(easyString_stringsMatch_null_and_count("tileType", token.at, token.size)) {
                        EasyToken t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_COLON);

                        t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_STRING);

                        tileType = (TileSetType)findEnumValue(t.at, t.size, MyTileMap_TypeStrings, arrayCount(MyTileMap_TypeStrings));
                    }
                    if(easyString_stringsMatch_null_and_count("tileMapId", token.at, token.size)) {
                        EasyToken t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_COLON);

                        t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_INTEGER);
                        tileMapId.x = t.intVal;

                        t = lexGetNextToken(&tokenizer);
                        assert(t.type == TOKEN_INTEGER);
                        tileMapId.y = t.intVal;
                    }

                    
                } break;
                case TOKEN_CLOSE_BRACKET: {
                    // if(entity.type == ENTITY_PLAYER) {
                    //     Entity *e = addPlayerEntity(state);

                    //     e->id = entity.id;
                    //     e->idHash = entity.idHash;
                    //     e->pos = entity.pos;

                    //     state->player = e;
                    //     assert(state->entityCount == 1);
                    // } else if(entity.type == ENTITY_TILE_MAP) {
                    //     assert(state->tileCount < arrayCount(state->tiles));
	                //     MapTile *tile = state->tiles + state->tileCount++;
                    //     *tile = getDefaultMapTile(state, (TileSetType)tileType, entity.pos.x, entity.pos.y, tileMapId.x, tileMapId.y, true);
                    // }
                    
                } break;
                case TOKEN_NEWLINE: {

                } break;
                default: {

                } break;
            }
        }

        printf("entities loaded: %d\n", state->entityCount);
        printf("tiles loaded: %d\n", state->tileCount);

        if(data) {
		    platform_free_memory(data);	
    	}
    }
}

void saveLevel_version1_binary(GameState *gameState, char *utf8_full_file_name) {

	size_t toAllocate = sizeof(SaveLevelHeaderVersion) + sizeof(SaveLevelHeaderV1) + gameState->tileCount*sizeof(SaveTileV1);

	u8 *data = (u8 *)pushSize(&globalPerFrameArena, toAllocate);

	SaveLevelHeaderVersion *v = (SaveLevelHeaderVersion *)data;
	v->version = 1;
	SaveLevelHeaderV1 *header = (SaveLevelHeaderV1 *)(data + sizeof(SaveLevelHeaderVersion));
	header->tileCount = gameState->tileCount;
	header->bytesPerTile = sizeof(SaveTileV1);

	SaveTileV1 *tiles = (SaveTileV1 *)(data + sizeof(SaveLevelHeaderV1) + sizeof(SaveLevelHeaderVersion));

	for(int i = 0; i < gameState->tileCount; ++i) {
		MapTile t = gameState->tiles[i];
		SaveTileV1 tile = {};
		tile.x = t.x;
		tile.y = t.y;
		tile.xId = t.xId;
		tile.yId = t.yId;
		tile.type = t.type;
		tiles[i] = tile;
	}

	Platform_File_Handle handle = platform_begin_file_write_utf8_file_path(utf8_full_file_name);

	assert(!handle.has_errors);

	platform_write_file_data(handle, data, toAllocate, 0);

	platform_close_file(handle);
}

void loadSaveLevel_binary(GameState *state, char *fileName_utf8) {
	u8 *data = 0;
	size_t data_size = 0;

	u32 currentVersion = 1;

	bool worked = Platform_LoadEntireFile_utf8(fileName_utf8, (void **)&data, &data_size);

	if(worked && data) {
		SaveLevelHeaderVersion *v = (SaveLevelHeaderVersion *)data;

		if(v->version == 1) {
			SaveLevelHeaderV1 *h = (SaveLevelHeaderV1 *)(data + sizeof(SaveLevelHeaderVersion));

			SaveTileV1 *tiles = (SaveTileV1 *)(data + sizeof(SaveLevelHeaderV1) + sizeof(SaveLevelHeaderVersion));

			assert(h->bytesPerTile == sizeof(SaveTileV1));

			for(int i = 0; i < h->tileCount; ++i) {
				SaveTileV1 t = tiles[i];

				assert(state->tileCount < arrayCount(state->tiles));
                MapTile *tile = state->tiles + state->tileCount++;

				tile->x = t.x;
				tile->y = t.y;
				tile->xId = t.xId;
				tile->yId = t.yId;
				//TODO: More robust way of mapping save type to in-game type
				tile->type = (TileSetType)t.type;
			}
		}
	}

	if(data) {
		platform_free_memory(data);	
	}
}
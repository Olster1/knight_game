

float3 getRenderWorldPWithOffset(Entity *e) {
    float3 p = e->pos;
    p.y += p.z;
    p.x += e->offsetP.x * e->scale.x;
    p.y += e->offsetP.y * e->scale.y;
    return p;
}

void pushGameLight(GameState *state, float3 worldPos, float4 color, float perlinNoiseValue) {
	if(state->lightCount < arrayCount(state->lights)) {
		GameLight *l = &state->lights[state->lightCount++];

		l->worldPos = worldPos;
		l->color = scale_float4(perlinNoiseValue, color);
	}
}

char *makeEntityId(GameState *gameState) {
    u64 timeSinceEpoch = platform_getTimeSinceEpoch();
    char *result = easy_createString_printf(&globalPerEntityLoadArena, "%ld-%d-%d", timeSinceEpoch, gameState->randomIdStartApp, gameState->randomIdStart);

    //NOTE: This would have to be locked in threaded application
    gameState->randomIdStart++;

    return result;
}  

float16 getModelToWorldTransform(Entity *e_) {
    float16 result = float16_identity();

    Entity *e = e_;

    while(e) {
        //NOTE: ROTATION
        float16 local = float16_angle_aroundZ(e->rotation);

        if(e == e_) { //NOTE: Only scale by the orignal entity
            //NOTE: SCALE
            local = float16_scale(local, e->scale);
        }

        //NOTE: POS
        local = float16_set_pos(local, e->pos);

        //NOTE: Cocat to end matrix
        result = float16_multiply(local, result);

        e = e->parent;
    }

    return result;
}

float3 getWorldPosition(Entity *e_) {
    float3 result = e_->pos;
    result.x += e_->offsetP.x * e_->scale.x;
    result.y += e_->offsetP.y * e_->scale.y;

    Entity *e = e_->parent;

    while(e) {
        float3 localP = e->pos;
        localP.x += e->offsetP.x * e->scale.x;
        localP.y += e->offsetP.y * e->scale.y;
        
        result = plus_float3(result, localP);

        e = e->parent;
    }

    return result;
}

float16 getModelToViewTransform(Entity *e_, float3 cameraPos) {

    float16 result = getModelToWorldTransform(e_);

    result = float16_multiply(float16_set_pos(float16_identity(), float3_negate(cameraPos)), result);

    return result;

}

void markBoardAsEntityOccupied(GameState *gameState, float3 worldP) {
    float2 chunkP = getChunkPosForWorldP(worldP.xy);
    Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, true);
    if(c) {
        float3 localP = getChunkLocalPos(worldP.x, worldP.y, worldP.z);
        Tile *tile = c->getTile(localP.x, localP.y, localP.z);
        if(tile) {
            tile->entityOccupation++;
        }
    }
}

void markBoardAsEntityUnOccupied(GameState *gameState, float3 worldP) {
    float2 chunkP = getChunkPosForWorldP(worldP.xy);
    Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, true);
    if(c) {
        float3 localP = getChunkLocalPos(worldP.x, worldP.y, worldP.z);
        Tile *tile = c->getTile(localP.x, localP.y, localP.z);
        if(tile) {
            tile->entityOccupation--;
            assert(tile->entityOccupation >= 0);
        }
    }
}


Entity *makeNewEntity(GameState *state, float3 worldP) {
    Entity *e = 0;
    if(state->entityCount < arrayCount(state->entities)) {
        e = &state->entities[state->entityCount++];

        memset(e, 0, sizeof(Entity));

        e->id = global_entityId++;

        e->fireTimer = -1;

        e->velocity = make_float3(0, 0, 0);
        e->offsetP = make_float3(0, 0, 0);
        e->maxMoveDistance = 0.5f*DOUBLE_MAX_MOVE_DISTANCE; //NOTE: Default move distance per turn
        e->pos = make_float3(worldP.x, worldP.y, worldP.z);
        e->flags |= ENTITY_ACTIVE;
        e->scale = make_float3(4, 4, 1);
        e->speed = 5.0f;
        e->sortYOffset = 0;

        assert(e->maxMoveDistance <= 0.5f*DOUBLE_MAX_MOVE_DISTANCE);

        markBoardAsEntityOccupied(state, worldP);


    }
    return e;
}

int getNewOrReuseParticler(Entity *e, EntityFlag type, ParticlerParent *parent) {
    int index = -1;
    
    assert(parent->particlerCount < arrayCount(parent->particlers));
    if(parent->particlerCount < arrayCount(parent->particlers)) { //NOTE: IF there's no particlers left don;t bother
        
        for(int i = 0; i < e->particlerCount && index < 0; i++) {
            Particler *pTemp = e->particlers[i];
            if(pTemp->flags & type) {
                index = i;
                break;
            }
        }

        if(index < 0) {
            
            //NOTE: Get a new one from the storeage
            assert(e->particlerCount < arrayCount(e->particlers));
            if(e->particlerCount < arrayCount(e->particlers)) {
                index = e->particlerCount++;
            }
        }
    }

    return index;
}

void entityCatchFire(GameState *state, Entity *e, float3 spawnArea) {
    if(e->fireTimer) {
        int pEntIndex = getNewOrReuseParticler(e, ENTITY_ON_FIRE, &state->particlers);
        if(pEntIndex >= 0) {
            float3 particleP = e->pos;
            particleP.y += 1.5f;

            Particler *p = getNewParticleSystem(&state->particlers, particleP, (TextureHandle *)global_white_texture, spawnArea, make_float4(0, 0, 1, 1), 100);
            if(p) {
                addColorToParticler(p, make_float4(1.0, 0.9, 0.6, 0.0));
                addColorToParticler(p, make_float4(1.0, 0.5, 0.1, 0.8));
                addColorToParticler(p, make_float4(0.6, 0.1, 0.05, 0.5));
                addColorToParticler(p, make_float4(0.2, 0.2, 0.2, 0.0));

                p->pattern.randomSize = make_float2(0.3f, 1.0f);
                p->pattern.dpMargin = 0.8f;
                p->pattern.speed = 2.0f;

                p->flags |= ENTITY_ON_FIRE; //NOTE: Tag it as a 'fire' particle system to check in the entity update code

                e->particlers[pEntIndex] = p;
                e->particlerIds[pEntIndex] = p->id;
                e->flags |= ENTITY_ON_FIRE;
                e->fireTimer = 0;
            }
        }
    }
}


void entityRenderSelected(GameState *state, Entity *e) {
    if(!(e->flags & ENTITY_SELECTED)) {
        int pEntIndex = getNewOrReuseParticler(e, ENTITY_SELECTED, &state->particlers);
        if(pEntIndex >= 0) {
            float3 particleP = e->pos;

            float3 spawnMargin = make_float3(0.6f, 0.6f, 1);
            Particler *p = getNewParticleSystem(&state->particlers, particleP, (TextureHandle *)global_white_texture, spawnMargin, make_float4(0, 0, 1, 1), 3);
            if(p) {
                addColorToParticler(p, make_float4(1.0, 0.843, 0.0, 1.0));
                addColorToParticler(p, make_float4(1.0, 0.843, 0.0, 0.0));

                p->pattern.randomSize = make_float2(0.2f, 0.2f);
                p->pattern.dpMargin = 0.0f;
                p->pattern.speed = 1.3f;

                p->flags |= ENTITY_SELECTED; //NOTE: Tag it as a 'fire' particle system to check in the entity update code

                e->particlers[pEntIndex] = p;
                e->particlerIds[pEntIndex] = p->id;
                e->flags |= ENTITY_SELECTED;
            }
        }
    }
}

Entity *addKnightEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_KNIGHT;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        e->scale = make_float3(2.5f, 2.5f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->knightAnimations.idle, 0.08f);
        
    }
    return e;
} 

Entity *addManEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_MAN;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        e->scale = make_float3(1.11f, 2.5f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->manAnimations.idle, 0.08f);
    }
    return e;
}

Entity *addTreeEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_TREE;
        e->offsetP.y = 0.3; //NOTE: Fraction of the scale
        e->scale = make_float3(6.419f, 10, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->treeAnimations.idle, 0.08f);
        
    }
    return e;
}

Entity *addTemplerKnightEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_TEMPLER_KNIGHT;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        e->scale = make_float3(1.32f, 3.0f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->templerKnightAnimations.idle, 0.08f);
        
    }
    return e;
}

Entity *addHouseEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_HOUSE;
        e->offsetP.y = 0.08f;
        e->sortYOffset = 0.9f;
        e->scale = make_float3(2, 3, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->houseAnimation, 0.08f);

        // entityCatchFire(state, e, make_float2(0.8f, 0.8f));
    }
    return e;
} 

Entity *addTowerEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_HOUSE;
        e->offsetP.y = 0.2f;
        e->sortYOffset = 0.7f;
        e->scale = make_float3(2, 4, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->towerAnimation, 0.08f);

        // entityCatchFire(state, e, make_float2(0.8f, 0.8f));
    }
    return e;
} 

Entity *addGoblinHouseEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_GOBLIN_HOUSE;
        e->offsetP.y = 0.08f;
        e->sortYOffset = 0.6f;
        e->scale = make_float3(1.8, 2.7, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->goblinHutAnimation, 0.08f);

        // entityCatchFire(state, e, make_float2(0.8f, 0.8f));
    }
    return e;
} 


Entity *addGoblinTowerEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_GOBLIN_TOWER;
        e->offsetP.y = 0.08f;
        e->sortYOffset = 0.5f;
        e->scale = make_float3(2.66666, 2, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->goblinTowerAnimation, 0.08f);

        // entityCatchFire(state, e, make_float2(0.8f, 0.8f));
    }
    return e;
} 

Entity *addCastleEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_CASTLE;
        e->offsetP.y = 0.07;
        e->sortYOffset = 1.7f;
        e->scale = make_float3(6, 5, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->castleAnimation, 0.08f);

        // entityCatchFire(state, e, make_float2(3, 1));
    }
    return e;
} 

Entity *addPeasantEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_PEASANT;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.12; //NOTE: Fraction of the scale
        e->scale = make_float3(2.5f, 2.5f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->peasantAnimations.idle, 0.08f);
    }
    return e;
} 

Entity *addArcherEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_ARCHER;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->archerAnimations.idle, 0.08f);
    }
    return e;
} 

Entity *addGoblinEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_GOBLIN;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.12; //NOTE: Fraction of the scale
        e->scale = make_float3(2.5f, 2.5f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->goblinAnimations.idle, 0.08f);
    }
    return e;
} 

Entity *addGoblinBarrelEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_GOBLIN_BARREL;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->barrellAnimations.idle, 0.08f);
    }
    return e;
} 

Entity *addGoblinTntEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_GOBLIN_TNT;
        e->flags |= ENTITY_CAN_WALK;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->tntAnimations.idle, 0.08f);
    }
    return e;
} 




void drawClouds(GameState *gameState, Renderer *renderer, float dt) {
    DEBUG_TIME_BLOCK();

    TextureHandle *atlasHandle = gameState->textureAtlas.texture.handle;
    int cloudDistance = 3;
    for(int y = cloudDistance; y >= -cloudDistance; --y) {
        for(int x = -cloudDistance; x <= cloudDistance; ++x) {
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, x, y, 0, true, false);
            if(c && (c->generateState == CHUNK_NOT_GENERATED || c->cloudFadeTimer >= 0)) {
                float maxTime = 1.5f;
                
                //NOTE: Generate the clouds
                if(c->cloudCount == 0) {
                    DEBUG_TIME_BLOCK_NAMED("CREATE CLOUDS");
                    int margin = 1;
                    for(int i = 0; i < MAX_CLOUD_DIM; i++) {
                        for(int j = 0; j < MAX_CLOUD_DIM; j++) {
                            float2 p = {};
                            p.x += random_between_float(margin, CHUNK_DIM - margin);
                            p.y += random_between_float(margin, CHUNK_DIM - margin);
                            assert(c->cloudCount < arrayCount(c->clouds));
                            CloudData *d = &c->clouds[c->cloudCount++];
                            d->pos = p;
                            d->cloudIndex = random_between_int(0, 3);
                            assert(d->cloudIndex < 3);
                            d->fadePeriod = random_between_float(0.4f, maxTime);
                            d->scale = random_between_float(4, 10);
                            d->darkness = random_between_float(0.95f, 1.0f);
                            assert(d->cloudIndex < 3);

                        }
                    }
                }   
                
                for(int i = 0; i < c->cloudCount; ++i) {
                    DEBUG_TIME_BLOCK_NAMED("PUSH CLOUDS AS TEXTUREs");
                    CloudData *cloud = &c->clouds[i];
                    float3 worldP = make_float3(x*CHUNK_DIM, y*CHUNK_DIM, CLOUDS_RENDER_Z);
                    worldP.x += cloud->pos.x;
                    worldP.y += cloud->pos.y;
                    worldP.x -= gameState->cameraPos.x;
                    worldP.y -= gameState->cameraPos.y;
                    
                    float s = cloud->scale;

                    float tVal = c->cloudFadeTimer;
                    if(tVal < 0) {
                        tVal = 0;
                    }
                    float alpha = lerp(0.4f, 0, make_lerpTValue(tVal / cloud->fadePeriod));
                    
                    AtlasAsset *t = gameState->cloudText[cloud->cloudIndex];
                    
                    float2 scale = make_float2(s, s*t->aspectRatio_h_over_w);
                    float darkness = cloud->darkness;
                    pushTexture(renderer, atlasHandle, worldP, scale, make_float4(cloud->darkness, cloud->darkness, cloud->darkness, alpha), t->uv);
                }

                if(c->cloudFadeTimer >= 0) {
                    c->cloudFadeTimer += dt;

                    if(c->cloudFadeTimer >= maxTime) {
                        //NOTE: Turn fade timer off
                        c->cloudFadeTimer = -1;
                    }
                }
                
            }
        }
    }
}

void renderChunkDecor(GameState *gameState, Renderer *renderer, Chunk *c) {
    //NOTE: Render trees
    for(int i = 0; i < c->treeSpriteCount; ++i) {
        float3 worldP = c->treeSpritesWorldP[i];

        float3 renderP = getRenderWorldP(worldP);
        renderP.y += 1; //NOTE: To get it in the middle
        renderP.x -= gameState->cameraPos.x;
        renderP.y -= gameState->cameraPos.y;
        renderP.z = RENDER_Z;

        float3 sortP = worldP;

        float alpha = 1.0f;

        float3 posToCheck = worldP;
        posToCheck.y += 1; //NOTE For behind the tree
        float3 tileP = getChunkLocalPos(posToCheck.x, posToCheck.y, posToCheck.z);

        Tile *tile = 0;

        if((posToCheck.y / CHUNK_DIM) != c->y) {
            //NOTE: See if it is on a different chunk
            float2 chunkP = getChunkPosForWorldP(posToCheck.xy);
            Chunk *chunkToCheck = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, false, false);
            if(chunkToCheck) {
                tile = chunkToCheck->getTile(tileP.x, tileP.y, tileP.z);
            }
            
        } else {
            tile = c->getTile(tileP.x, tileP.y, tileP.z);
        }
        
        if(tile && (tile->entityOccupation > 0)) {
            alpha = 0.5f;
        }

        pushEntityTexture(renderer, gameState->treeTexture.handle, renderP, make_float2(3, 3), make_float4(1, 1, 1, alpha), gameState->treeTexture.uvCoords, getSortIndex(sortP, RENDER_LAYER_4));
    }

     //NOTE: Render stones and bushes
    for(int i = 0; i < c->decorSpriteCount; ++i) {
        DecorSprite b = c->decorSprites[i];
        float3 renderP = getRenderWorldP(b.worldP);
        
        renderP.x -= gameState->cameraPos.x;
        renderP.y -= gameState->cameraPos.y;
        renderP.z = RENDER_Z;

        float3 sortP = convertRealWorldToBlockCoords(b.worldP);

        pushEntityTexture(renderer, b.textureHandle, renderP, b.scale, make_float4(1, 1, 1, 1), b.uvs, getSortIndex(sortP, RENDER_LAYER_3));
    }

   
}

//NOTE: If this function returns a positive number it means it should swap a & b, making a come after b
int compare_by_height(InstanceEntityData *pa, InstanceEntityData *pb) {
    float a_yz = (float)pa->sortIndex.worldP.y;// + (float)pa->sortIndex.worldP.z;
    float b_yz = (float)pb->sortIndex.worldP.y;// + (float)pb->sortIndex.worldP.z;

	if (a_yz != b_yz) {
		return (b_yz - a_yz) > 0 ? 1 : -1; 
	}

	if(pa->sortIndex.worldP.z != pb->sortIndex.worldP.z) {
		return (pa->sortIndex.worldP.z - pb->sortIndex.worldP.z)  > 0 ? 1 : -1;
	}

	if (pa->sortIndex.layer != pb->sortIndex.layer) {
		return (pa->sortIndex.layer - pb->sortIndex.layer);
	}

    if (pa->sortIndex.worldP.x != pb->sortIndex.worldP.x) {
		return pa->sortIndex.worldP.x - pb->sortIndex.worldP.x;
	}
    return 0;
}

void sortAndRenderTileQueue(Renderer *renderer) {
	DEBUG_TIME_BLOCK();
	
	{
		//TODO: Make this faster
		DEBUG_TIME_BLOCK_NAMED("SORT RENDER TILES");
		//NOTE: First sort the list. This is an insert sort
		for (int i = 1; i < renderer->tileEntityRenderCount; i++) {
			InstanceEntityData temp = renderer->tileRenderData[i];
			int j = i - 1;
			while (j >= 0 && compare_by_height(&renderer->tileRenderData[j], &temp) > 0) {
				renderer->tileRenderData[j + 1] = renderer->tileRenderData[j];
				j--;
			}
			renderer->tileRenderData[j + 1] = temp;
		}
	}

	pushShader(renderer, &terrainLightingShader);
	//NOTE: Draw the list
	for(int i = 0; i < renderer->tileEntityRenderCount; ++i) {
		InstanceEntityData *d = renderer->tileRenderData + i; 
		pushTexture(renderer, d->textureHandle, d->pos, d->scale, d->color, d->uv, d->aoMask);
	}

	renderer->tileEntityRenderCount = 0;
}

void sortAndRenderEntityQueue(Renderer *renderer) {
	DEBUG_TIME_BLOCK();
	
	{
		//TODO: Make this faster
		DEBUG_TIME_BLOCK_NAMED("SORT RENDER ENTITIES");
		//NOTE: First sort the list. This is an insert sort
		for (int i = 1; i < renderer->entityRenderCount; i++) {
			InstanceEntityData temp = renderer->entityRenderData[i];
			int j = i - 1;
			while (j >= 0 && compare_by_height(&renderer->entityRenderData[j], &temp) > 0) {
				renderer->entityRenderData[j + 1] = renderer->entityRenderData[j];
				j--;
			}
			renderer->entityRenderData[j + 1] = temp;
		}
	}

	pushShader(renderer, &terrainLightingShader);
	//NOTE: Draw the list
	for(int i = 0; i < renderer->entityRenderCount; ++i) {
		InstanceEntityData *d = renderer->entityRenderData + i; 
		pushTexture(renderer, d->textureHandle, d->pos, d->scale, d->color, d->uv, d->aoMask);
	}

	renderer->entityRenderCount = 0;
}


void renderTileMap(GameState *gameState, Renderer *renderer, float16 fovMatrix, float2 windowScale, float dt) {
    DEBUG_TIME_BLOCK();

    //NOTE: Draw the tile map
    int renderDistance = 3;
    float2 cameraBlockP = getChunkPosForWorldP(gameState->cameraPos.xy);
    float2 offset = make_float2(0, 0);

    pushShader(renderer, &terrainLightingShader);
    for(int y_ = renderDistance; y_ >= -renderDistance; --y_) {
        for(int x_ = -renderDistance; x_ <= renderDistance; ++x_) {
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, x_ + offset.x, y_ + offset.y, 0, true, false);
            if(c) {
                float2 chunkScale = make_float2(CHUNK_DIM, CHUNK_DIM);
                if(c->texture.textureHandle) {
                    if(!c->generatedMipMaps) {
                        generateMipMapsForTexture(c->texture.textureHandle);
                        c->generatedMipMaps = true;
                    }
                    //NOTE: Render this tile
                    float3 worldP = getChunkWorldP(c);
                    float3 renderP = worldP;
                    renderP.x -= gameState->cameraPos.x;
                    renderP.y -= gameState->cameraPos.y;
                    renderP.x += 0.5f*chunkScale.x - 0.5f;
                    renderP.y += 0.5f*chunkScale.y - 0.5f;
                    renderP.z = RENDER_Z;
                    // pushEntityTexture(renderer, c->texture.textureHandle, renderP, chunkScale, make_float4(1, 1, 1, 1), make_float4(0, 1, 1, 0), getSortIndex(worldP, RENDER_LAYER_1), 0);
                    float4 color = make_float4(1, 1, 1, 1); //make_float4(c->y % 2 ? 1 : 0, c->x % 2 ? 1 : 0, 1, 1)
                    pushTexture(renderer, c->texture.textureHandle, renderP, chunkScale, color, make_float4(0, 1, 1, 0));
                    
                    renderChunkDecor(gameState, renderer, c);
                } else if(c->generateState & CHUNK_GENERATED) {
                    float2 chunkInPixels = make_float2(chunkScale.x*TILE_WIDTH_PIXELS, chunkScale.y*TILE_WIDTH_PIXELS);
                    c->texture = platform_createFramebuffer(chunkInPixels.x, chunkInPixels.y);
                    assert(c->texture.framebuffer > 0);
                    pushRenderFrameBuffer(renderer, c->texture.framebuffer);
                    pushClearColor(renderer, make_float4(1, 1, 1, 0));

                    pushMatrix(renderer, make_ortho_matrix_bottom_left_corner(chunkScale.x, chunkScale.y, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE));
                    pushViewport(renderer, make_float4(0, 0, chunkInPixels.x, chunkInPixels.y));
                    renderer_defaultScissors(renderer, chunkInPixels.x, chunkInPixels.y);

                    pushTileEntityTexture(renderer, gameState->backgroundTexture.handle, make_float3(0.5f*chunkScale.x, 0.5f*chunkScale.y, 0), chunkScale, make_float4(1, 1, 1, 1), make_float4(0, 0, 1, 1), getSortIndex(make_float3(0, 0, 0), RENDER_LAYER_1), 0);

                    // for(int tilez = 0; tilez <= CHUNK_DIM; ++tilez) {
                    //     for(int tiley = 0; tiley <= CHUNK_DIM; ++tiley) {
                    //         for(int tilex = 0; tilex <= CHUNK_DIM; ++tilex) {
                    //             Tile *tile = c->getTile(tilex, tiley, tilez);

                    //             if(tile && tile->type != TILE_TYPE_NONE) {
                    //                 float waterScale = 3;
                    //                 float shadowScale = 3.1f;
                    //                 float3 worldP = getTileWorldP(c, tilex, tiley, tilez);
                    //                 {

                    //                     float2 localP = make_float2(tiley, tilex);
                    //                     float3 defaultP = make_float3(tilex + 0.5f, tiley + tilez + 0.5f, RENDER_Z);
                                       
                    //                     assert(defaultP.y < chunkScale.y);

                    //                     u32 lightingMask = tile->lightingMask;

                    //                     {
                    //                         if(tile->type == TILE_TYPE_BEACH) {
                    //                             Texture *t = getTileTexture(&gameState->sandTileSet, tile->coords);
                    //                             pushTileEntityTexture(renderer, t->handle, defaultP, make_float2(1, 1), make_float4(1, 1, 1, 1), t->uvCoords, getSortIndex(worldP, RENDER_LAYER_1), lightingMask);
                    //                         } else if(tile->type == TILE_TYPE_WATER_ROCK) {
                    //                             waterScale = 2;
                    //                             if(tile->animationController) {
                    //                                 Texture *animationSprite = easyAnimation_updateAnimation_getTexture(tile->animationController, &gameState->animationState.animationItemFreeListPtr, dt);
                    //                                 pushTileEntityTexture(renderer, animationSprite->handle, defaultP, make_float2(waterScale, waterScale), make_float4(1, 1, 1, 1), animationSprite->uvCoords, getSortIndex(worldP, RENDER_LAYER_1), lightingMask);
                    //                             }
                    //                         } else if(tile->type == TILE_TYPE_ROCK) {
                    //                             Texture *t = getTileTexture(&gameState->sandTileSet, tile->coords);
                    //                             pushTileEntityTexture(renderer, t->handle, defaultP, make_float2(1, 1), make_float4(1, 1, 1, 1), t->uvCoords, getSortIndex(worldP, RENDER_LAYER_1), lightingMask);

                    //                             if(tile->flags & TILE_FLAG_FRONT_FACE) {
                    //                                 TileMapCoords coord = tile->coords;
                    //                                 coord.y += 1;
                    //                                 u32 lightingMask = tile->lightingMask >> 8; //NOTE: We want the top 8 bits so move them down

                    //                                 float3 p1 = worldP;
                    //                                 p1.y -= 1;
                    //                                 RenderSortIndex sort = getSortIndex(p1, RENDER_LAYER_2);
                                                    
                    //                                 Texture *t = getTileTexture(&gameState->sandTileSet, coord);
                    //                                 pushTileEntityTexture(renderer, t->handle, make_float3(defaultP.x, defaultP.y - 1, RENDER_Z), make_float2(1, 1), make_float4(1, 1, 1, 1), t->uvCoords, sort, lightingMask);
                    //                             }
                    //                             // if(tile->flags & TILE_FLAG_TREE) {
                    //                             //NOTE: Trees are now seperate entities
                    //                             //     RenderSortIndex sort = getSortIndex(worldP, RENDER_LAYER_3);
                    //                             //     float renderOffset = 1;
                    //                             //     pushTileEntityTexture(renderer, gameState->treeTexture.handle, make_float3(defaultP.x, defaultP.y + renderOffset, RENDER_Z), make_float2(3, 3), make_float4(1, 1, 1, 1), gameState->treeTexture.uvCoords, sort, 0);
                    //                             // }
                    //                         }

                    //                         if(tile->flags & TILE_FLAG_GRASSY_TOP) {
                    //                             Texture *t = getTileTexture(&gameState->sandTileSet, tile->coordsSecondary);
                    //                             pushTileEntityTexture(renderer, t->handle, defaultP, make_float2(1, 1), make_float4(1, 1, 1, 1), t->uvCoords, getSortIndex(worldP, RENDER_LAYER_2), lightingMask);
                    //                         }

                    //                         if((tile->flags & TILE_FLAG_FRONT_GRASS) || (tile->flags & TILE_FLAG_FRONT_BEACH)) {
                    //                             TileMapCoords coord = {};
                    //                             coord.x = 4;
                    //                             coord.y = 0;

                    //                             if(tile->flags & TILE_FLAG_FRONT_BEACH) {
                    //                                 coord.x += 5;
                    //                             }

                    //                             Texture *t = getTileTexture(&gameState->sandTileSet, coord);
                    //                             pushTileEntityTexture(renderer, t->handle, make_float3(defaultP.x, defaultP.y - 1, RENDER_Z), make_float2(1, 1), make_float4(1, 1, 1, 1), t->uvCoords, getSortIndex(worldP, RENDER_LAYER_3), lightingMask);
                    //                         }
                    //                     } 
                                        
                    //                     if(tile->type != TILE_TYPE_WATER_ROCK) {
                    //                         if(tile->animationController) {
                    //                             Texture *animationSprite = easyAnimation_updateAnimation_getTexture(tile->animationController, &gameState->animationState.animationItemFreeListPtr, dt);
                    //                             //TODO: Add the water animation back in
                    //                             float3 sortP = worldP;
                    //                             // sortP.z += 2;
                    //                             // pushTileEntityTexture(renderer, animationSprite->handle, defaultP, make_float2(waterScale, waterScale), make_float4(1, 1, 1, 1), animationSprite->uvCoords, getSortIndex(sortP, RENDER_LAYER_0), lightingMask);
                    //                         }
                    //                     }
                    //                 }
                    //             }
                    //         }
                    //     }
                    // }
                    sortAndRenderTileQueue(renderer);

                    //NOTE: Restore the regular render state
                    pushRenderFrameBuffer(renderer, 0);
                    pushMatrix(renderer, fovMatrix);
                    pushViewport(renderer, make_float4(0, 0, windowScale.x, windowScale.y));
                    renderer_defaultScissors(renderer, windowScale.x, windowScale.y);
                }
            }
        }
    }
}

bool tileIsOccupied(GameState *gameState, float3 worldP) {
    bool result = false;
    float2 chunkP = getChunkPosForWorldP(worldP.xy);
    Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, true);
    if(c) {
        float3 localP = getChunkLocalPos(worldP.x, worldP.y, worldP.z);
        Tile *tile = c->getTile(localP.x, localP.y, localP.z);
        if(tile) {
            result = (tile->entityOccupation > 0);
        }
    }
    return result;
}


static float3 roundToGridBoard(float3 in, float tileSize) {
    float xMod = (in.x < 0) ? -tileSize : tileSize;
    float yMod = (in.y < 0) ? -tileSize : tileSize;
    
    float3 result = {};
    if(tileSize == 1) {
        result = make_float3((int)(in.x + xMod*0.5f), (int)(in.y + yMod*0.5f), (int)(in.z));
    } else {
        result = make_float3((int)(in.x + xMod*0.5f), (int)(in.y + yMod*0.5f), (int)(in.z));

        result.x -= ((int)result.x) % (int)tileSize;
        result.y -= ((int)result.y) % (int)tileSize;
    }
    
    return result;
}


Animation *getBestWalkAnimation(Entity *e) {
    Animation *animation = &e->animations->idle;
    float margin = 0.3f;
    removeEntityFlag(e, ENTITY_SPRITE_FLIPPED);

    float2 impluse = e->velocity.xy;

    if((impluse.x > margin  && impluse.y < -margin) || (impluse.x < -margin && impluse.y > margin) || ((impluse.x > margin && impluse.y < margin && impluse.y > -margin))) { //NOTE: The extra check is becuase the front & back sideways animation aren't matching - should flip one in the aesprite
        addEntityFlag(e, ENTITY_SPRITE_FLIPPED);
    }

    if(impluse.y > margin) {
        if(impluse.x < -margin || impluse.x > margin) {
            animation = &e->animations->run;	
        } else {
            animation = &e->animations->run;
        }
    } else if(impluse.y < -margin) {
        if(impluse.x < -margin || impluse.x > margin) {
            animation = &e->animations->run;	

            //TODO: Remove this when the animations are coorect
            if((impluse.x > margin  && impluse.y < -margin) || (impluse.x < -margin && impluse.y > margin) || ((impluse.x > margin && impluse.y < margin && impluse.y > -margin))) { //NOTE: The extra check is becuase the front & back sideways animation aren't matching - should flip one in the aesprite
                removeEntityFlag(e, ENTITY_SPRITE_FLIPPED);
            } else {
                addEntityFlag(e, ENTITY_SPRITE_FLIPPED);
            }
        } else {
            animation = &e->animations->run;
        }
    } else if(impluse.x < -margin || impluse.x > margin) {
        animation = &e->animations->run;
    }

    return animation;
}

int isEntitySelected(GameState *gameState, Entity *e) {
    int result = -1;
    for(int i = 0; i < gameState->selectedEntityCount; ++i) {
        if(gameState->selectedEntityIds[i].id == e->id) {
            result = i;
            break;
        }
    }
    return result;
}

void refreshParticlers(GameState *gameState, Entity *e) {
    ParticlerParent *parent = &gameState->particlers;
    bool wentIn0 = false;
    bool wentIn1 = false;
    bool wentIn2 = false;
    for(int i = 0; i < e->particlerCount; ) {
        Particler *p = e->particlers[i];
        ParticlerId id = e->particlerIds[i];
        int addend = 1;

        if(p->id.id == id.id && !p->id.invalid) {
            //NOTE: Wasn't moved in the master array, so don't do anything
            wentIn1 = true;
        } else {
            //NOTE: Try find the right particle system
            bool found = false;
            for(int j = 0; j < parent->particlerCount && !found; j++) {
        		Particler *pCheck = &parent->particlers[j];

                if(pCheck->id.id == id.id) {
                    e->particlers[i] = pCheck;
                    assert(pCheck->id.id == e->particlerIds[i].id);
                    found = true;
                    wentIn0 = true;
                }
            }

            if(!found) {
                //NOTE: For some reason this doesn't exist so invalidate it
                // assert(false);
                assert(e->particlerCount > 0);
                e->particlers[i] = e->particlers[--e->particlerCount];
                e->particlerIds[i] = e->particlerIds[e->particlerCount];
                addend = 0;
                wentIn2 = true;
            }
        }

        i += addend;
    }

    for(int i = 0; i < e->particlerCount; i++) {
        Particler *p = e->particlers[i];
        ParticlerId id = e->particlerIds[i];

        assert(p->id.id == id.id);
    }
}

void addMovePositionsFromBoardAstar(GameState *gameState, FloodFillResult searchResult, Entity *e, bool endMove) {
    //NOTE: Path is a sentinel doubley linked list
    NodeDirection *d = searchResult.cameFrom;
    assert(d);
    while(d) {
        float3 worldP = d->p;

        /* NOTE: Rendering code for the arrows 
        float3 renderP = getRenderWorldP(worldP);
        renderP.z = RENDER_Z;

        int arrowIndex = 0;

        {
            //NOTE: Get the right texture to show the direction
            if(d->next) {
                
                float3 p = d->next->p;
                float3 diff = minus_float3(p, worldP);

                if(diff.x < 0) {
                    arrowIndex = 2;
                } else if(diff.x > 0) {
                    arrowIndex = 0;
                } else if(diff.y > 0) {
                    arrowIndex = 1;
                } else if(diff.y < 0) {
                    arrowIndex = 3;
                }
            } else {
                //NOTE: No direction, so show the circle 
                arrowIndex = 4;
            }
        }
        */

        if(endMove) {
            //NOTE: Move the entities
            EntityMove *move = 0;
            if(gameState->freeEntityMoves) {
                move = gameState->freeEntityMoves;
                gameState->freeEntityMoves = move->next;
                move->next = 0;
            } else {
                move = pushStruct(&global_long_term_arena, EntityMove);
            }
            move->move = worldP;
            move->next = 0;

            //NOTE: Add to the end of the list
            {
                EntityMove **lastMove = &e->moves;
                while(*lastMove) {
                    lastMove = &(*lastMove)->next;
                }

                *lastMove = move;
            }
        }

        /*
        //NOTE: This draw the direction arrow images
        Texture *t = &gameState->arrows[arrowIndex];
        renderP.x -= gameState->cameraPos.x;
        renderP.y -= gameState->cameraPos.y;
        pushEntityTexture(renderer, t->handle, renderP, make_float2(1, 1), make_float4(1, 1, 1, 1), t->uvCoords, getSortIndex(worldP, RENDER_LAYER_2));
        */

        d = d->next;
    }
}

float3 getOriginSelection(GameState *gameState) {
    float3 p = {};
    if(gameState->selectedEntityCount > 0) {
        p = gameState->selectedEntityIds[gameState->selectedEntityCount - 1].worldPos;    
    }
    return p;
}

void updateEntity(GameState *gameState, Renderer *renderer, Entity *e, float dt, float3 mouseWorldP) {
    DEBUG_TIME_BLOCK();
    bool clicked = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0;

    float3 p = getWorldPosition(e);
    
    {
        float2 chunkP =  getChunkPosForWorldP(p.xy);
        float3 localP = getChunkLocalPos(p.x, p.y, p.z);

        int margin = CHUNK_REVEAL_MARGIN;

        if(localP.x < margin) {
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x - 1, chunkP.y, 0, true, true);
            assert(c);
        }
        if(localP.x >= (CHUNK_DIM - margin)) {
            // float2 chunkP =  getChunkPosForWorldP(p.xy);
            // float3 localP = getChunkLocalPos(p.x, p.y, p.z);
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x + 1, chunkP.y, 0, true, true);
            assert(c);
        }
        if(localP.y < margin) {
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y - 1, 0, true, true);
            assert(c);
        }
        if(localP.y >= (CHUNK_DIM - margin)) {
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y + 1, 0, true, true);
            assert(c);
        }
    }

    refreshParticlers(gameState, e);

    if(e->type & ENTITY_MAN) {
        for(int i = 0; i < gameState->selectedEntityCount; ++i) {
            Entity *treeEntity = gameState->selectedEntityIds[i].e;
            if(treeEntity->type == ENTITY_TREE) {
                gameState->selectedEntityIds[i] = gameState->selectedEntityIds[--gameState->selectedEntityCount];
                easyAnimation_addAnimationToController(&treeEntity->animationController, &gameState->animationState.animationItemFreeListPtr, &gameState->treeAnimations.dead, 0.08f);

                Entity *newTree = addTreeEntity(gameState, plus_float3(make_float3(4, -1, 0), treeEntity->pos));
                easyAnimation_emptyAnimationContoller(&newTree->animationController, &gameState->animationState.animationItemFreeListPtr);
                easyAnimation_addAnimationToController(&newTree->animationController, &gameState->animationState.animationItemFreeListPtr, &gameState->treeAnimations.fallen, 0.08f);
                float temp = newTree->scale.x;
                float bigger = 1.2f;
                newTree->scale.x = bigger*newTree->scale.y;
                newTree->scale.y = bigger*temp;
                newTree->sortYOffset = 0.5f;
                newTree->offsetP.y = 0;

                int pEntIndex = getNewOrReuseParticler(newTree, ENTITY_SELECTED, &gameState->particlers);
                if(pEntIndex >= 0) {
                    float3 particleP = newTree->pos;

                    float3 spawnMargin = make_float3(0.8f*newTree->scale.x, 0.4f, 1);
                    Particler *p = getNewParticleSystem(&gameState->particlers, particleP, gameState->smokeTexture.handle, spawnMargin, gameState->smokeTexture.uvCoords, 20);
                    
                    if(p) {
                        p->lifespan = 0.3f;
                        addColorToParticler(p, make_float4(1, 1, 1, 1.0));
                        addColorToParticler(p, make_float4(1, 1, 1, 0.0));

                        p->pattern.randomSize = make_float2(1.0f, 2.5f);
                        p->pattern.dpMargin = 0.8f;
                        p->pattern.speed = 4.3f;

                        p->flags |= ENTITY_SELECTED; //NOTE: Tag it as a 'fire' particle system to check in the entity update code

                        newTree->particlers[pEntIndex] = p;
                        newTree->particlerIds[pEntIndex] = p->id;
                    }
                }

                break;
            }
        }
    }

    

}


void renderEntity(GameState *gameState, Renderer *renderer, Entity *e, float16 fovMatrix, float dt) {

    Texture *t = 0;

    if(easyAnimation_isControllerValid(&e->animationController)) {

        t = easyAnimation_updateAnimation_getTexture(&e->animationController, &gameState->animationState.animationItemFreeListPtr, dt);
        if(e->animationController.finishedAnimationLastUpdate) {
            //NOTE: Make not active anymore. Should Probably remove it from the list. 
            // e->flags &= ~ENTITY_ACTIVE;

            // if(e->animationController.lastAnimationOn == &gameState->playerAttackAnimation) {
            //     //NOTE: Turn off attack collider when attack finishes
            //     // e->colliders[ATTACK_COLLIDER_INDEX].flags &= ~COLLIDER_ACTIVE; 
            // }
        }
    } 

    float3 renderWorldP = getRenderWorldPWithOffset(e);
    
    
    renderWorldP.x -= gameState->cameraPos.x;
    renderWorldP.y -= gameState->cameraPos.y;

    float4 color = make_float4(1, 1, 1, 1);

    // NOTE: color the entity that you have selected
    if(isEntitySelected(gameState, e) >= 0) {
        color = make_float4(1, 0.8f, 0, 1);

        entityRenderSelected(gameState, e);
    } else {
        e->flags &= ~(ENTITY_SELECTED);
    }
    

    //NOTE: Draw position above player
    // float3 tileP = convertRealWorldToBlockCoords(e->pos);
    // char *str = easy_createString_printf(&globalPerFrameArena, "(%d %d %d)", (int)tileP.x, (int)tileP.y, (int)tileP.z);
    // pushShader(renderer, &sdfFontShader);
	// draw_text(renderer, &gameState->font, str, renderWorldP.x, renderWorldP.y, 0.02, make_float4(0, 0, 0, 1)); 

    if(t) {
        float3 sortPos = e->pos;
        sortPos.y -= e->sortYOffset;
        renderWorldP.z = RENDER_Z;

        {
            //NOTE: This draws the position of the sort index position
            /*
            float3 a = sortPos;
            a.y += a.z;
            a.z = 2;
            a.x -= gameState->cameraPos.x;
            a.y -= gameState->cameraPos.y;
            pushRect(renderer, a, make_float2(0.3f, 0.3f), make_float4(1, 0, 0, 1));
            */
        }

        pushEntityTexture(renderer, t->handle, renderWorldP, e->scale.xy, color, t->uvCoords, getSortIndex(sortPos, RENDER_LAYER_3));
    }
}



void pushAllEntityLights(GameState *gameState, float dt) {
    //NOTE: Push all lights for the renderer to use
	for(int i = 0; i < gameState->entityCount; ++i) {
		Entity *e = &gameState->entities[i];

		if(hasEntityFlag(e, ENTITY_ACTIVE) && hasEntityFlag(e, LIGHT_COMPONENT)) {
            float3 worldPos = getWorldPosition(e);
            //NOTE: Update the flicker
            e->perlinNoiseLight += dt;

            if(e->perlinNoiseLight > 1.0f) {
                e->perlinNoiseLight = 0.0f;
            }

            float value = SimplexNoise_fractal_1d(40, e->perlinNoiseLight, 3);

            //NOTE: Push light
            pushGameLight(gameState, worldPos, make_float4(1, 0.5f, 0, 1), value);
		}
	}
}


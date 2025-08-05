

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
    Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, false);
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
    Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, true);
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
        e->health = 10;

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

// void entityCatchFire(GameState *state, Entity *e, float3 spawnArea) {
//     if(e->fireTimer) {
//         int pEntIndex = getNewOrReuseParticler(e, ENTITY_ON_FIRE, &state->particlers);
//         if(pEntIndex >= 0) {
//             float3 particleP = e->pos;
//             particleP.y += 1.5f;

//             Particler *p = getNewParticleSystem(&state->particlers, particleP, (TextureHandle *)global_white_texture, spawnArea, make_float4(0, 0, 1, 1), 100);
//             if(p) {
//                 addColorToParticler(p, make_float4(1.0, 0.9, 0.6, 0.0));
//                 addColorToParticler(p, make_float4(1.0, 0.5, 0.1, 0.8));
//                 addColorToParticler(p, make_float4(0.6, 0.1, 0.05, 0.5));
//                 addColorToParticler(p, make_float4(0.2, 0.2, 0.2, 0.0));

//                 p->pattern.randomSize = make_float2(0.3f, 1.0f);
//                 p->pattern.dpMargin = 0.8f;
//                 p->pattern.speed = 2.0f;

//                 p->flags |= ENTITY_ON_FIRE; //NOTE: Tag it as a 'fire' particle system to check in the entity update code

//                 e->particlers[pEntIndex] = p;
//                 e->particlerIds[pEntIndex] = p->id;
//                 e->flags |= ENTITY_ON_FIRE;
//                 e->fireTimer = 0;
//             }
//         }
//     }
// }


void entityRenderSelected(GameState *state, Entity *e) {
    if(!(e->flags & ENTITY_SELECTED)) {
        // int pEntIndex = getNewOrReuseParticler(e, ENTITY_SELECTED, &state->particlers);
        // if(pEntIndex >= 0) {
        //     float3 particleP = e->pos;

        //     float3 spawnMargin = make_float3(0.6f, 0.6f, 1);
        //     Particler *p = getNewParticleSystem(&state->particlers, particleP, (TextureHandle *)global_white_texture, spawnMargin, make_float4(0, 0, 1, 1), 3);
        //     if(p) {
        //         addColorToParticler(p, make_float4(1.0, 0.843, 0.0, 1.0));
        //         addColorToParticler(p, make_float4(1.0, 0.843, 0.0, 0.0));

        //         p->pattern.randomSize = make_float2(0.2f, 0.2f);
        //         p->pattern.dpMargin = 0.0f;
        //         p->pattern.speed = 1.3f;

        //         p->flags |= ENTITY_SELECTED; //NOTE: Tag it as a 'fire' particle system to check in the entity update code

        //         e->particlers[pEntIndex] = p;
        //         e->particlerIds[pEntIndex] = p->id;
        //         e->flags |= ENTITY_SELECTED;
        //     }
        // }
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
        float scaleFloat = 1.0f;
        e->scale = make_float3(scaleFloat*1.11f, scaleFloat*2.5f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->manAnimations.idle, 0.08f);
    }
    return e;
}

Entity *addAlderTreeEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_TREE;
        e->offsetP.y = 0.3; //NOTE: Fraction of the scale
        e->scale = make_float3(6.419f, 10, 1);

        e->animations = &state->alderTreeAnimations[random_between_int(0, arrayCount(state->alderTreeAnimations))];
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &e->animations->idle, 0.08f);
        
    }
    return e;
}

Entity *addAshTreeEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_TREE;
        e->offsetP.y = 0.3; //NOTE: Fraction of the scale
        e->scale = make_float3(6.419f, 10, 1);

        e->animations = &state->ashTreeAnimations[random_between_int(0, arrayCount(state->ashTreeAnimations))];
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &e->animations->idle, 0.08f);
        
        
    }
    return e;
}

Entity *addTemplerKnightEntity(GameState *state, float3 worldP) {
    Entity *e = makeNewEntity(state, worldP);
    if(e) {
        e->type = ENTITY_TEMPLER_KNIGHT;
        e->flags |= ENTITY_CAN_WALK | ENTITY_SHOW_DAMAGE_SPLAT;
        e->offsetP.y = 0.16; //NOTE: Fraction of the scale
        e->scale = make_float3(1.32f, 3.0f, 1);
        easyAnimation_initController(&e->animationController);
		easyAnimation_addAnimationToController(&e->animationController, &state->animationState.animationItemFreeListPtr, &state->templerKnightAnimations.idle, 0.08f);
        e->animations = &state->templerKnightAnimations;
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

DamageSplat *getDamageSplat(GameState *gameState) {
    DamageSplat *result = 0;

    if(gameState->freeListDamageSplats) {
        result = gameState->freeListDamageSplats;
        gameState->freeListDamageSplats = gameState->freeListDamageSplats->next;
    } else {
        result = pushStruct(&global_long_term_arena, DamageSplat);
    }
    
    if(result) {
        result->timeAt = 1;
        result->next = gameState->damageSplats;
        gameState->damageSplats = result;
    }

    return result;
} 


void renderDamageSplats(GameState *gameState, float dt) {
    DamageSplat **d = &gameState->damageSplats;
    while(*d) {
        ((*d)->timeAt) -= dt;

        {
            float3 p = getRenderWorldP((*d)->worldP);
            p.z = 1;

            p.x -= gameState->cameraPos.x; //NOTE: Offset for middle of the tile
            p.y -= gameState->cameraPos.y; //NOTE: Offset for middle of the tile

            float scale = 1.3f;//lerp(0.9f, 1.1f, make_lerpTValue(sin01(gameState->selectHoverTimer)));
            pushTexture(&gameState->renderer, gameState->splatTexture.handle, plus_float3(make_float3(0.4, -0.4, 0), p), make_float2(scale, scale), make_float4(0.4, 0, 0, 1), gameState->splatTexture.uvCoords);

            char *str = easy_createString_printf(&globalPerFrameArena, "%d", (*d)->damage);
            pushShader(&gameState->renderer, &sdfFontShader);
            draw_text(&gameState->renderer, &gameState->font, str, p.x, p.y, 0.02, make_float4(1, 1, 1, 1)); 
        }

        if((*d)->timeAt <= 0) {
            DamageSplat *temp = *d;
            *d = (*d)->next;

            temp->next = gameState->freeListDamageSplats;
            gameState->freeListDamageSplats = temp;
        } else {
            d = &((*d)->next);
        }
        
    }
}

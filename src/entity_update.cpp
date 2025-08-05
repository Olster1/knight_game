

void drawClouds(GameState *gameState, Renderer *renderer, float dt) {
    DEBUG_TIME_BLOCK();

    TextureHandle *atlasHandle = gameState->textureAtlas.texture.handle;
    int cloudDistance = 3;
    for(int y = cloudDistance; y >= -cloudDistance; --y) {
        for(int x = -cloudDistance; x <= cloudDistance; ++x) {
            Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, x, y, 0, true, false);
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
            Chunk *chunkToCheck = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, false, false);
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
    float2 offset = gameState->cameraPos.xy;
	offset.x /= CHUNK_DIM;
	offset.y /= CHUNK_DIM;

    pushShader(renderer, &pixelArtShader);
    for(int y_ = renderDistance; y_ >= -renderDistance; --y_) {
        for(int x_ = -renderDistance; x_ <= renderDistance; ++x_) {
            Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, x_ + offset.x, y_ + offset.y, 0, true, false);
            if(c) {
                float2 chunkScale = make_float2(CHUNK_DIM, CHUNK_DIM);
                // if(c->texture.textureHandle && c->texture.textureHandle->handle > 0) 
                {
                    if(!c->generatedMipMaps) {
                        // generateMipMapsForTexture(c->texture.textureHandle);
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
                    float4 color = make_float4(1, 1, 1, 1); //make_float4(c->y % 2 ? 1 : 0, c->x % 2 ? 1 : 0, 1, 1)
                    pushTexture(renderer, gameState->backgroundTexture.handle, renderP, chunkScale, color, make_float4(0, 0, 1, 1));
                    
                    renderChunkDecor(gameState, renderer, c);
                } 
                // else if(c->generateState & CHUNK_GENERATED) {
                //     float2 chunkInPixels = make_float2(chunkScale.x*TILE_WIDTH_PIXELS, chunkScale.y*TILE_WIDTH_PIXELS);
                //     c->texture = platform_createFramebuffer(chunkInPixels.x, chunkInPixels.y);
                //     assert(c->texture.framebuffer > 0);
                //     pushRenderFrameBuffer(renderer, c->texture.framebuffer);
                //     pushClearColor(renderer, make_float4(1, 1, 1, 0));

                //     pushMatrix(renderer, make_ortho_matrix_bottom_left_corner(chunkScale.x, chunkScale.y, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE));
                //     pushViewport(renderer, make_float4(0, 0, chunkInPixels.x, chunkInPixels.y));
                //     renderer_defaultScissors(renderer, chunkInPixels.x, chunkInPixels.y);

                //     pushTileEntityTexture(renderer, gameState->backgroundTexture.handle, make_float3(0.5f*chunkScale.x, 0.5f*chunkScale.y, 0), chunkScale, make_float4(1, 1, 1, 1), make_float4(0, 0, 1, 1), getSortIndex(make_float3(0, 0, 0), RENDER_LAYER_1), 0);
                  
                //     sortAndRenderTileQueue(renderer);

                //     //NOTE: Restore the regular render state
                //     pushRenderFrameBuffer(renderer, 0);
                //     pushMatrix(renderer, fovMatrix);
                //     pushViewport(renderer, make_float4(0, 0, windowScale.x, windowScale.y));
                //     renderer_defaultScissors(renderer, windowScale.x, windowScale.y);
                // }
            }
        }
    }
}


bool tileIsOccupied(GameState *gameState, float3 worldP) {
    bool result = false;
    float2 chunkP = getChunkPosForWorldP(worldP.xy);
    Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, true);
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

void updateEntity(GameState *gameState, Renderer *renderer, Entity *e, float dt, float3 mouseWorldP) {
    DEBUG_TIME_BLOCK();
    bool clicked = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0;

    float3 p = getWorldPosition(e);
    
    {
        float2 chunkP =  getChunkPosForWorldP(p.xy);
        float3 localP = getChunkLocalPos(p.x, p.y, p.z);

        int margin = CHUNK_REVEAL_MARGIN;

        if(localP.x < margin) {
            Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x - 1, chunkP.y, 0, true, true);
            assert(c);
        }
        if(localP.x >= (CHUNK_DIM - margin)) {
            // float2 chunkP =  getChunkPosForWorldP(p.xy);
            // float3 localP = getChunkLocalPos(p.x, p.y, p.z);
            Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x + 1, chunkP.y, 0, true, true);
            assert(c);
        }
        if(localP.y < margin) {
            Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y - 1, 0, true, true);
            assert(c);
        }
        if(localP.y >= (CHUNK_DIM - margin)) {
            Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y + 1, 0, true, true);
            assert(c);
        }
    }

    refreshParticlers(gameState, e);

    if(e->type & ENTITY_MAN) {
        for(int i = 0; i < gameState->selectedEntityCount; ++i) {
            Entity *attackEntity = gameState->selectedEntityIds[i].e;
            if(easyAnimation_getCurrentAnimation(&attackEntity->animationController, &attackEntity->animations->idle)) 
            {
                gameState->selectedEntityIds[i] = gameState->selectedEntityIds[--gameState->selectedEntityCount];

				float damage = 3;
				attackEntity->health -= damage;

				DamageSplat *d = getDamageSplat(gameState);

				if(d) {
					d->damage = damage;
					d->worldP = attackEntity->pos;
					
					d->worldP.y += random_between_float(1, 2);
					d->worldP.x += random_between_float(-0.8, 0.8);
				}

				if(attackEntity->health <= 0) {

                    if(attackEntity->type == ENTITY_TEMPLER_KNIGHT) {
                        templerKnightDie(gameState, attackEntity);
                        
                    } else if(attackEntity->type == ENTITY_TREE) {
                        treeDie(gameState, attackEntity);
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
float2 getMouseWorldPLvl0(GameState *state, float windowWidth, float windowHeight) {
	float2 mouseP = make_float2(global_platformInput.mouseX, windowHeight - global_platformInput.mouseY);
    float2 mouseP_01 = make_float2(mouseP.x / windowWidth, mouseP.y / windowHeight);

	float worldX = lerp(-0.5f*state->planeSizeX*state->zoomLevel, 0.5f*state->planeSizeX*state->zoomLevel, make_lerpTValue(mouseP_01.x));
    float worldY = lerp(-0.5f*state->planeSizeY*state->zoomLevel, 0.5f*state->planeSizeY*state->zoomLevel, make_lerpTValue(mouseP_01.y));

    worldX += state->cameraPos.x;
    worldY += state->cameraPos.y;

	return make_float2(worldX, worldY);
} 


float3 getMouseWorldP(GameState *state, float windowWidth, float windowHeight) {
	float2 mouseP = make_float2(global_platformInput.mouseX, windowHeight - global_platformInput.mouseY);
    float2 mouseP_01 = make_float2(mouseP.x / windowWidth, mouseP.y / windowHeight);

	float worldX = lerp(-0.5f*state->planeSizeX*state->zoomLevel, 0.5f*state->planeSizeX*state->zoomLevel, make_lerpTValue(mouseP_01.x));
    float worldY = lerp(-0.5f*state->planeSizeY*state->zoomLevel, 0.5f*state->planeSizeY*state->zoomLevel, make_lerpTValue(mouseP_01.y));

    worldX += state->cameraPos.x;
    worldY += state->cameraPos.y;

	float3 tileP = convertRealWorldToBlockCoords(make_float3(worldX, worldY, 0));
	float worldZ = getMapHeight(tileP.x, tileP.y);

	bool wasStandAlone = true;

	float3 oneIn = convertRealWorldToBlockCoords(make_float3(worldX, worldY - 1, 0));
	float oneInHeight = getMapHeight(oneIn.x, oneIn.y);

	//NOTE: Check now if the worldZ is what it says it is
	{
		float3 twoIn = convertRealWorldToBlockCoords(make_float3(worldX, worldY - 2, 0));
		float twoInHeight = getMapHeight(twoIn.x, twoIn.y);

		if(twoInHeight == 2) {
			//NOTE: Height is actually 2 because of our werid rendering perspective
			worldZ = 2;
			worldY -= 2;
			wasStandAlone = false;
		} else {
			

			if(oneInHeight == 1) {
				//NOTE: Height is actually 1 because of our werid rendering perspective
				worldZ = 1;
				worldY -= 1;
				wasStandAlone = false;
			}
		}
	}

	if(wasStandAlone && worldZ > 0) {
		if(worldZ == 1) {
			if(oneInHeight > 0) {
				tileP = convertRealWorldToBlockCoords(make_float3(worldX, worldY - 1, 0));
				worldZ = getMapHeight(tileP.x, tileP.y);
			}
		}
		worldY -= (worldZ - 1);
	}

	if(worldZ < 0) {
		//NOTE: Nothign is below 0 and we don't want to render the selectable offset
		worldZ = 0;
	}

	return make_float3(worldX, worldY, worldZ);

}

void drawSelectionHover(GameState *gameState, Renderer *renderer, float dt, float3 worldMouseP, SelectedEntityData *selectedData) {
	{
		
		float3 worldP = convertRealWorldToBlockCoords(worldMouseP);
		float3 p = worldP;

		// {
		// 	//NOTE: See if the selection is on a tree tile
		// 	float2 chunkP = getChunkPosForWorldP(worldMouseP.xy);
		// 	Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, true, true);
		// 	float3 tileP = getChunkLocalPos(p.x, p.y, p.z);
		// 	Tile *tile = c->getTile(tileP.x, tileP.y, tileP.z);
		// 	if(tile && !(tile->flags & TILE_FLAG_WALKABLE)) {
		// 		gameState->selectedColor = make_float4(1, 0, 0, 1);
		// 	}
		// }

		float4 color = make_float4(1, 1, 1, 1);

		if(selectedData && !selectedData->isValidPos) {
			color = make_float4(1, 0, 0, 1);
		}

		p = getRenderWorldP(worldP);
		p.z = RENDER_Z;

		p.x -= gameState->cameraPos.x; //NOTE: Offset for middle of the tile
		p.y -= gameState->cameraPos.y; //NOTE: Offset for middle of the tile

		//NOTE: P is now in camera space
		
		float scale = 1.0f;//lerp(0.9f, 1.1f, make_lerpTValue(sin01(gameState->selectHoverTimer)));
		float3 sortP = worldP;
		pushEntityTexture(renderer, gameState->selectImage.handle, p, make_float2(scale, scale), color, gameState->selectImage.uvCoords, getSortIndex(sortP, RENDER_LAYER_2));

		float3 tileP = convertRealWorldToBlockCoords(make_float3(worldMouseP.x, worldMouseP.y, worldMouseP.z));
		//NOTE: Draw position above player
		char *str = easy_createString_printf(&globalPerFrameArena, "(%d %d %d)", (int)tileP.x, (int)tileP.y, (int)tileP.z);
		pushShader(renderer, &sdfFontShader);
		draw_text(renderer, &gameState->font, str, p.x, p.y, 0.02, make_float4(0, 0, 0, 1)); 
	 
    }
}

void drawAllSectionHovers(GameState *gameState, Renderer *renderer, float dt, float3 worldMouseP) {
	gameState->selectHoverTimer += dt;
	if(gameState->selectedEntityCount == 0) {
		drawSelectionHover(gameState, renderer, dt, worldMouseP, 0);
	} else {
		//NOTE: The position all the other positions are relative to
		float3 startP = getOriginSelection(gameState); 
		for(int i = 0; i < gameState->selectedEntityCount; ++i) {
			SelectedEntityData *data = gameState->selectedEntityIds + i; 
			float3 offset = minus_float3(data->worldPos, startP);

			drawSelectionHover(gameState, renderer, dt, plus_float3(offset, worldMouseP), data);
		}
	}
}

void updateParticlers(Renderer *renderer, GameState *gameState, ParticlerParent *parent, float dt) {
	for(int i = 0; i < parent->particlerCount; ) {
		int addend = 1;
		Particler *p = &parent->particlers[i];

		bool shouldRemove = updateParticler(renderer, p, gameState->cameraPos, dt, p->pattern);

		if(shouldRemove) {
			//NOTE: Move from the end
			parent->particlers[i] = parent->particlers[--parent->particlerCount];
			
			//NOTE: Invalidate the end particle system so any entity still pointing to this knows to reset their pointer
			parent->particlers[parent->particlerCount].id = getInvalidParticleId();
			addend = 0;
		} 

		i += addend;
	}
}

void updateAndDrawEntitySelection(GameState *gameState, Renderer *renderer, bool clicked, float2 worldMousePLvl0, float3 mouseWorldP) {
	bool released = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].releasedCount > 0;
	pushShader(renderer, &lineShader);
	
	{
		gameState->selectedEntityCount = 0;

		//NOTE: See if entities are selected 
		for(int i = 0; i < gameState->entityCount; ++i) {
			Entity *e = gameState->entities + i;

			if((e->flags & ENTITY_ACTIVE) && e->type != ENTITY_MAN) {
				float3 renderWorldP = getRenderWorldPWithOffset(e);
				if(in_rect2f_bounds(make_rect2f_center_dim(renderWorldP.xy, e->scale.xy), worldMousePLvl0)) {
					if(e->type == ENTITY_TEMPLER_KNIGHT) {
						gameState->actionString = "BATTLE KNIGHT";
					} else if(e->type == ENTITY_TREE && easyAnimation_getCurrentAnimation(&e->animationController, &e->animations->idle)) {
						gameState->actionString = "CUT TREE";
					}

				
					
					assert(gameState->selectedEntityCount < arrayCount(gameState->selectedEntityIds));
					if(clicked && gameState->selectedEntityCount < arrayCount(gameState->selectedEntityIds)) {
						SelectedEntityData *data = &gameState->selectedEntityIds[gameState->selectedEntityCount++];
						data->id = e->id;
						data->e = e;
						data->worldPos = e->pos;
					
					}
				}
			}
		}
	} 
}

void updateAndRenderEntities(GameState *gameState, Renderer *renderer, float dt, float16 fovMatrix, float windowWidth, float windowHeight){
	DEBUG_TIME_BLOCK();

	gameState->selectedMoveCount = 0; //NOTE: The count of how many entities are able to move

    //NOTE: Push all lights for the renderer to use
	pushAllEntityLights(gameState, dt);
	updateEntityPhysics(gameState, dt);

	pushMatrix(renderer, fovMatrix);

	float2 windowSize = make_float2(windowWidth, windowHeight);

    pushShader(renderer, &pixelArtShader);
	renderTileMap(gameState, renderer, fovMatrix, windowSize, dt);
	

	float3 worldMouseP = getMouseWorldP(gameState, windowWidth, windowHeight);
	float2 worldMousePLvl0 = getMouseWorldPLvl0(gameState, windowWidth, windowHeight);

	//NOTE: Gameplay code
	for(int i = 0; i < gameState->entityCount; ++i) {
		Entity *e = &gameState->entities[i];

		if(e->flags & ENTITY_ACTIVE) {
			updateEntity(gameState, renderer, e, dt, worldMouseP);
			renderEntity(gameState, renderer, e, fovMatrix, dt);
		}
	}

	bool clicked = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0;
	bool endMove = gameState->selectedEntityCount > 0 && clicked;

	if(gameState->selectedMoveCount == gameState->selectedEntityCount) {
		for(int i = 0; i < gameState->selectedEntityCount; ++i) {
			SelectedEntityData *data = gameState->selectedEntityIds + i;
			assert(data->isValidPos);
			addMovePositionsFromBoardAstar(gameState, data->floodFillResult, data->e, endMove);
		}
	}

	updateParticlers(renderer, gameState, &gameState->particlers, dt);

	renderDamageSplats(gameState, dt);

	updateAndDrawEntitySelection(gameState, renderer, clicked, worldMousePLvl0, worldMouseP);

	// drawAllSectionHovers(gameState, renderer, dt, worldMouseP);

	sortAndRenderEntityQueue(renderer);

	// drawClouds(gameState, renderer, dt);
	// drawCloudsAsTexture(gameState, renderer, dt, fovMatrix, windowSize);
	
}
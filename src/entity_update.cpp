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
					} else if(e->type == ENTITY_TREE && easyAnimation_getCurrentAnimation(&e->animationController, &gameState->treeAnimations.idle)) {
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

	renderTileMap(gameState, renderer, fovMatrix, windowSize, dt);

	pushShader(renderer, &pixelArtShader);

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

	pushBlendMode(renderer, RENDER_BLEND_MODE_ADD);

	updateParticlers(renderer, gameState, &gameState->particlers, dt);

	pushBlendMode(renderer, RENDER_BLEND_MODE_DEFAULT);

	updateAndDrawEntitySelection(gameState, renderer, clicked, worldMousePLvl0, worldMouseP);

	// drawAllSectionHovers(gameState, renderer, dt, worldMouseP);

	sortAndRenderEntityQueue(renderer);

	// drawClouds(gameState, renderer, dt);
	// drawCloudsAsTexture(gameState, renderer, dt, fovMatrix, windowSize);
	
}
void clearGameStatePerFrameValues(GameState *state) {
	state->renderer.lightCount = 0;
}


void createAOOffsets(GameState *gameState) {
    for(int i = 0; i < arrayCount(global_cubeData); ++i) {
        assert(i < arrayCount(gameState->lightingOffsets.aoOffsets));

        CubeVertex v = global_cubeData[i];
        float3 normal = v.normal;
        float3 sizedOffset = scale_float3(2, v.pos);

        float3 masks[2] = {};
        int maskCount = 0;

        for(int k = 0; k < 3; k++) {
            if(normal.E[k] == 0) {
                assert(maskCount < arrayCount(masks));
                float3 m = make_float3(0, 0, 0);
                m.E[k] = -sizedOffset.E[k];

                masks[maskCount++] = m;
            }
        }

        gameState->lightingOffsets.aoOffsets[i].offsets[0] = plus_float3(sizedOffset, masks[0]);
        gameState->lightingOffsets.aoOffsets[i].offsets[1] = sizedOffset; 
        gameState->lightingOffsets.aoOffsets[i].offsets[2] = plus_float3(sizedOffset, masks[1]);
    }
}

void clearMemoryForEntityScene(GameState *gameState) {
	gameState->freeEntityMoves = 0;
	gameState->freeListDamageSplats = 0;
	gameState->entityCount = 0;
	gameState->particlers.particlerCount = 0;
	gameState->terrain = Terrain();
	gameState->gamePlayBoardInited = false;

	releaseMemoryMark(&global_perEntityLoadArenaMark);
	global_perEntityLoadArenaMark = takeMemoryMark(&globalPerEntityLoadArena);
}	

void initBuildingTextures(GameState *gameState) {
	gameState->houseTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "house.png");
	easyAnimation_pushFrame(&gameState->houseAnimation, &gameState->houseTexture);

	gameState->castleTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "castle.png");
	easyAnimation_pushFrame(&gameState->castleAnimation, &gameState->castleTexture);

	gameState->castleBurntTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "castle_burnt.png");
	easyAnimation_pushFrame(&gameState->castleBurntAnimation, &gameState->castleBurntTexture);

	gameState->houseBurntTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "house_burnt.png");
	easyAnimation_pushFrame(&gameState->houseBurntAnimation, &gameState->houseBurntTexture);

	gameState->towerTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "tower.png");
	easyAnimation_pushFrame(&gameState->towerAnimation, &gameState->towerTexture);

	gameState->towerBurntTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "tower_burnt.png");
	easyAnimation_pushFrame(&gameState->towerBurntAnimation, &gameState->towerBurntTexture);

	gameState->goblinHutTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "goblinHut.png");
	easyAnimation_pushFrame(&gameState->goblinHutAnimation, &gameState->goblinHutTexture);

	gameState->goblinHutBurntTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "goblinHut_burnt.png");
	easyAnimation_pushFrame(&gameState->goblinHutBurntAnimation, &gameState->goblinHutBurntTexture);

	gameState->goblinTowerBurntTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "goblinTowerBurnt.png");
	easyAnimation_pushFrame(&gameState->goblinTowerBurntAnimation, &gameState->goblinTowerBurntTexture);

}


void initGameState(GameState *gameState, BackendRenderer *backendRenderer) {
		gameState->initialized = true;

		initRenderer(&gameState->renderer);
		gameState->animationState.animationItemFreeListPtr = 0;
		{
			DEBUG_TIME_BLOCK_NAMED("INIT FONTS");

			gameState->font = initFont("../fonts/liberation-mono.ttf");
			gameState->pixelFont = initFont("../fonts/Medieval.ttf");
		}
		
		gameState->fontScale = 0.6f;
		gameState->scrollDp = 0;

		gameState->terrain = Terrain();

		gameState->draw_debug_memory_stats = false;

		{
			DEBUG_TIME_BLOCK_NAMED("INIT DIALOG TREES");

			initDialogTrees(&gameState->dialogs);
		}

		gameState->shakeTimer = -1;//NOTE: Turn the timer off

		//Non-determnistic
		// srand(time(NULL));   // Initialization, should only be called once.
		//determnistic
		srand(1);   // Initialization, should only be called once.

		//NOTE: Used to build the entity ids 
		gameState->randomIdStartApp = rand();
		gameState->randomIdStart = rand();

		gameState->planeSizeY = 20;
		gameState->planeSizeX = 20;

		{
			DEBUG_TIME_BLOCK_NAMED("INIT PROFILER STATE");

			gameState->drawState = EasyProfiler_initProfilerDrawState();
		}

		
		{
			DEBUG_TIME_BLOCK_NAMED("LOAD TEXTURES");
			// gameState->textureAtlas = readTextureAtlas("../images/texture_atlas.json", "../images/texture_atlas.png");
			gameState->bannerTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/banner.png");
			gameState->selectTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/select.png");
			gameState->shadowUiTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/shadow.png");
			gameState->kLogoText = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/kLogo.png");
			gameState->gLogoText = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/gLogo.png");
			gameState->blueText = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/b.png");
			gameState->redText = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/r.png");
			gameState->selectImage = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/select.png");
			// gameState->cloudText[0] = textureAtlas_getItem(&gameState->textureAtlas, "cloud.png");
			// gameState->cloudText[1] = textureAtlas_getItem(&gameState->textureAtlas, "cloud2.png");
			// gameState->cloudText[2] = textureAtlas_getItem(&gameState->textureAtlas, "cloud3.png");
			// gameState->treeTexture = textureAtlas_getItemAsTexture(&gameState->textureAtlas, "tree.png");
			// gameState->smokeTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "flame.png");
			// gameState->smokeTexture =  textureAtlas_getItemAsTexture(&gameState->textureAtlas, "flame.png");
			gameState->smokeTextures[0] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/puffs/puff1.png");
			gameState->smokeTextures[1] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/puffs/puff2.png");
			gameState->smokeTextures[2] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/puffs/puff3.png");
			gameState->smokeTextures[3] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/puffs/puff4.png");
			gameState->smokeTextures[4] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/puffs/puff5.png");
			gameState->fireTextures[0] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/puffs/fire.png");
			
			gameState->pressStartTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/press_start.png");
			gameState->titleScreenWordsTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/title.png");
			gameState->backgroundTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/background.png");
			gameState->titleScreenTexture[0] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/titleScreen.png");
			gameState->titleScreenTexture[1] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/titleScreen1.png");
			gameState->titleScreenTexture[2] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/titleScreen2.png");
			gameState->titleScreenTexture[3] = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/ui/game_over_screen.png");
			gameState->splatTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/splat.png");
			gameState->inventoryTexture = backendRenderer_loadFromFileToGPU(backendRenderer, "../images/entities/inventory.png");

			gameState->gameModeFadeTimer = -1;
			gameState->gameModeState = GAME_START_SCREEN_MODE;
			
		
			loadImageStripXY(&gameState->manAnimations.idle, backendRenderer, "../images/entities/man.png", 32, 72, 1, 0, 0);

			loadImageStripXY(&gameState->alderTreeAnimations[0].idle, backendRenderer, "../images/entities/alder/tree.png", 108, 144, 1, 0, 0);
			loadImageStripXY(&gameState->alderTreeAnimations[0].dead, backendRenderer, "../images/entities/tree stump.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->alderTreeAnimations[0].fallen, backendRenderer, "../images/entities/fallentree.png", 162, 104, 1, 0, 0);

			loadImageStripXY(&gameState->alderTreeAnimations[1].idle, backendRenderer, "../images/entities/alder/alder2.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->alderTreeAnimations[1].dead, backendRenderer, "../images/entities/tree stump.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->alderTreeAnimations[1].fallen, backendRenderer, "../images/entities/fallentree.png", 162, 104, 1, 0, 0);

			loadImageStripXY(&gameState->ashTreeAnimations[0].idle, backendRenderer, "../images/entities/ash/ash1.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[0].dead, backendRenderer, "../images/entities/tree stump.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[0].fallen, backendRenderer, "../images/entities/fallentree.png", 162, 104, 1, 0, 0);

			loadImageStripXY(&gameState->ashTreeAnimations[1].idle, backendRenderer, "../images/entities/ash/ash2.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[1].dead, backendRenderer, "../images/entities/tree stump.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[1].fallen, backendRenderer, "../images/entities/fallentree.png", 162, 104, 1, 0, 0);

			loadImageStripXY(&gameState->ashTreeAnimations[2].idle, backendRenderer, "../images/entities/ash/ash3.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[2].dead, backendRenderer, "../images/entities/tree stump.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[2].fallen, backendRenderer, "../images/entities/fallentree.png", 162, 104, 1, 0, 0);

			loadImageStripXY(&gameState->ashTreeAnimations[3].idle, backendRenderer, "../images/entities/ash/ash4.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[3].dead, backendRenderer, "../images/entities/tree stump.png", 104, 162, 1, 0, 0);
			loadImageStripXY(&gameState->ashTreeAnimations[3].fallen, backendRenderer, "../images/entities/fallentree.png", 162, 104, 1, 0, 0);

			loadImageStripXY(&gameState->templerKnightAnimations.idle, backendRenderer, "../images/entities/knight.png", 32, 72, 1, 0, 0);

			loadImageStripXY(&gameState->bearAnimations.idle, backendRenderer, "../images/entities/bear/bear.png", 73, 73, 1, 0, 0);
			loadImageStripXY(&gameState->bearAnimations.dead, backendRenderer, "../images/entities/bear/beardown.png", 128, 128, 1, 0, 0);
			loadImageStripXY(&gameState->bearAnimations.skinned, backendRenderer, "../images/entities/bear/bearskinned.png", 128, 128, 1, 0, 0);
			loadImageStripXY(&gameState->bearAnimations.skeleton, backendRenderer, "../images/entities/bear/bearskeleton.png", 128, 128, 1, 0, 0);

			loadImageStripXY(&gameState->bearPelt.idle, backendRenderer, "../images/entities/bear/bearpelt.png", 16, 16, 1, 0, 0);
			loadImageStripXY(&gameState->skinningKnife.idle, backendRenderer, "../images/entities/weapons/knife.png", 32, 32, 1, 0, 0);
			loadImageStripXY(&gameState->bearTent.idle, backendRenderer, "../images/entities/bear/bearTent.png", 128, 128, 1, 0, 0);

			loadImageStripXY(&gameState->ghostAnimations.idle, backendRenderer, "../images/entities/nightime/ghost.png", 64, 64, 1, 0, 0);
		}

		gameState->selectedColor = make_float4(1, 1, 1, 1);
		createAOOffsets(gameState);
		gameState->gameMode = PLAY_MODE;
		gameState->cameraFollowPlayer = true;
		//TODO: Probably save this each time we leave the app
		gameState->zoomLevel = 1.8f;

		initAudioSpec(&gameState->audioSpec, 44100);
		initAudio(&gameState->audioSpec);

		// loadWavFile(&gameState->titleScreenSound, , &gameState->audioSpec);
		loadOggVorbisFile(&gameState->titleScreenSound, "../sounds/title_sound.ogg", &gameState->audioSpec);
		gameState->titlePagePlayingSound = playSound(&gameState->titleScreenSound);

		
	// DefaultEntityAnimations barrellAnimations;

		/////////////
		// {
		// 	int tileCount = 0;
		// 	int countX = 0;
		// 	int countY = 0;
		// 	Texture ** tiles = loadTileSet(backendRenderer, "../images/tilemap.png", 64, 64, &global_long_term_arena, &tileCount, &countX, &countY);
		// 	gameState->sandTileSet = buildTileSet(tiles, tileCount, TILE_SET_SAND, countX, countY, 64, 64);
		// }

	#if DEBUG_BUILD
		DEBUG_runUnitTests(gameState);
	#endif
}
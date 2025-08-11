
void updateFadeScreen(GameState *gameState, float dt, float w, float h) {
	if(gameState->gameModeFadeTimer >= 0) {
		gameState->gameModeFadeTimer -= dt;

		float useValue = gameState->gameModeFadeTimer;
		int direction = gameState->gameModeFadeDirection;

		if(gameState->gameModeFadeTimer <= 0) {
			if(gameState->targetGameMode != GAME_MODE_STATE_NONE) {
				gameState->gameModeState = gameState->targetGameMode;
				gameState->gameModeFadeTimer = MAX_FADE_TIME;
				gameState->gameModeFadeDirection *= -1;
				gameState->targetGameMode = GAME_MODE_STATE_NONE;

                if(gameState->gameModeState == GAME_GAMEOVER_MODE) {
                    clearMemoryForEntityScene(gameState);
                }
			} else {
				gameState->gameModeFadeTimer = -1;
			}
		} 
		
		float transparent = clamp(0, 1, (useValue / MAX_FADE_TIME));
		
		if(direction < 0) {
			transparent = 1.0f - transparent;
		}

		
		pushTexture(&gameState->renderer, global_white_texture, make_float3(0, 0, 0), make_float2(w, h), make_float4(0, 0, 0, transparent), make_float4(0, 0, 1, 1));
	}
}


void updateAndRenderGameOverScreen(GameState *gameState, float dt) {
    Renderer *renderer = &gameState->renderer;
    pushMatrix(renderer, make_ortho_matrix_origin_center(gameState->planeSizeX, gameState->planeSizeY, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE));
    pushShader(&gameState->renderer, &pixelArtShader);
    float scale0 = 0.35f;
    float scale1 = 0.15f;
    float w = gameState->planeSizeX;
    float h = gameState->planeSizeY;
    pushTexture(renderer, gameState->titleScreenTexture.handle, make_float3(0, 0, 0), make_float2(w, gameState->titleScreenTexture.aspectRatio_h_over_w*w), make_float4(1, 1, 1, 1), gameState->titleScreenTexture.uvCoords);
    // pushTexture(renderer, gameState->titleScreenWordsTexture.handle, make_float3(0, 0, 0), make_float2(scale0*w, scale0*gameState->titleScreenWordsTexture.aspectRatio_h_over_w*w), make_float4(1, 1, 1, 1), gameState->titleScreenWordsTexture.uvCoords);
    pushTexture(renderer, gameState->pressStartTexture.handle, make_float3(0, -2, 0), make_float2(scale1*w, scale1*gameState->pressStartTexture.aspectRatio_h_over_w*w), make_float4(1, 1, 1, 1), gameState->pressStartTexture.uvCoords);

    updateFadeScreen(gameState, dt, w, h);

    if(global_platformInput.keyStates[PLATFORM_KEY_ENTER].pressedCount > 0) {
        gameState->gameModeFadeTimer = MAX_FADE_TIME;
        gameState->gameModeFadeDirection = -1; //NOTE: Fade Out
        gameState->targetGameMode = GAME_START_SCREEN_MODE;
    }
}

void updateAndRenderTitleScreen(GameState *gameState, float dt) {
    Renderer *renderer = &gameState->renderer;
    pushMatrix(renderer, make_ortho_matrix_origin_center(gameState->planeSizeX, gameState->planeSizeY, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE));
    pushShader(&gameState->renderer, &pixelArtShader);
    float scale0 = 0.35f;
    float scale1 = 0.15f;
    float w = gameState->planeSizeX;
    float h = gameState->planeSizeY;
    pushTexture(renderer, gameState->titleScreenTexture.handle, make_float3(0, 0, 0), make_float2(w, gameState->titleScreenTexture.aspectRatio_h_over_w*w), make_float4(1, 1, 1, 1), gameState->titleScreenTexture.uvCoords);
    pushTexture(renderer, gameState->titleScreenWordsTexture.handle, make_float3(0, 0, 0), make_float2(scale0*w, scale0*gameState->titleScreenWordsTexture.aspectRatio_h_over_w*w), make_float4(1, 1, 1, 1), gameState->titleScreenWordsTexture.uvCoords);
    pushTexture(renderer, gameState->pressStartTexture.handle, make_float3(0, -2, 0), make_float2(scale1*w, scale1*gameState->pressStartTexture.aspectRatio_h_over_w*w), make_float4(1, 1, 1, 1), gameState->pressStartTexture.uvCoords);

    updateFadeScreen(gameState, dt, w, h);

    if(global_platformInput.keyStates[PLATFORM_KEY_ENTER].pressedCount > 0) {
        gameState->gameModeFadeTimer = MAX_FADE_TIME;
        gameState->gameModeFadeDirection = -1; //NOTE: Fade Out
        gameState->targetGameMode = GAME_PLAY_MODE;
    }
}
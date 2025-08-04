void updateZoomAndPan(GameState *gameState, float dt, float2 mouseP_01) {
	{
		gameState->scrollDp += global_platformInput.mouseScrollY*dt;

		//NOTE: Drag
		gameState->scrollDp *= 0.81f;

		//NOTE: Zoom in & out
		gameState->zoomLevel *= 1 + gameState->scrollDp;
		
		float min = 0.01f;
		if(gameState->zoomLevel < min) {
			gameState->zoomLevel = min;
		}
	} 


	if(global_platformInput.keyStates[PLATFORM_KEY_SHIFT].isDown) {
		//NOTE: Update Pan
		if(global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0) {
			//NOTE: Move the canvas
			gameState->draggingCanvas = true;
			gameState->startDragP = mouseP_01;
			gameState->canvasMoveDp = make_float2(0, 0);
		} else if(global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].isDown && gameState->draggingCanvas) {
			float panPower = 500*gameState->zoomLevel; //NOTE: Scale with how far out we're looking
			float2 diff = scale_float2(panPower*dt, minus_float2(gameState->startDragP, mouseP_01));
			gameState->canvasMoveDp = diff;
		} else {
			gameState->draggingCanvas = false;
		}
		
		if(!gameState->draggingCanvas) {
			gameState->canvasMoveDp.x *= 0.9f;
			gameState->canvasMoveDp.y *= 0.9f;
		}
	
		gameState->cameraPos.xy = plus_float2(gameState->cameraPos.xy, gameState->canvasMoveDp);
		gameState->startDragP = mouseP_01;
	}
} 

void updateCamera(GameState *gameState, float dt) {
	float2 cameraOffset = make_float2(0, 0);

	if(gameState->shakeTimer >= 0) {

		float offset = SimplexNoise_fractal_1d(40, gameState->shakeTimer, 3)*(1.0f - gameState->shakeTimer);
		//NOTE: Update the camera position offset
		cameraOffset.x = offset;
		cameraOffset.y = offset;

		gameState->shakeTimer += dt;

		if(gameState->shakeTimer >= 1.0f) {
			gameState->shakeTimer = -1.0f;
		}
	}

	if(gameState->cameraFollowPlayer) {
		gameState->cameraPos.x = lerp(gameState->cameraPos.x, gameState->player->pos.x + cameraOffset.x, make_lerpTValue(0.4f));
		gameState->cameraPos.y = lerp(gameState->cameraPos.y, gameState->player->pos.y + cameraOffset.y, make_lerpTValue(0.4f));
	} else {
		float speed = 10*dt;
		if(global_platformInput.keyStates[PLATFORM_KEY_UP].isDown) {
			gameState->cameraPos.y += speed; 
		}
		if(global_platformInput.keyStates[PLATFORM_KEY_DOWN].isDown) {
			gameState->cameraPos.y -= speed; 
		}
		if(global_platformInput.keyStates[PLATFORM_KEY_LEFT].isDown) {
			gameState->cameraPos.x -= speed; 
		}
		if(global_platformInput.keyStates[PLATFORM_KEY_RIGHT].isDown) {
			gameState->cameraPos.x += speed; 
		}
		
	}

}
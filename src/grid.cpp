static void drawGrid(GameState *gameState) {
	float zPos = 10;
	float4 gridColor = make_float4(0.5f, 0.5f, 0.5f, 1.0f);

	Renderer *renderer = &gameState->renderer;

	float3 snappedCamera = gameState->cameraPos;
	snappedCamera.x = (int)snappedCamera.x;
	snappedCamera.y = (int)snappedCamera.y;

	//NOTE: Draw grid
	pushShader(renderer, &lineShader);

	int gridSize = 30;
	int halfGridSize = (int)(0.5f*gridSize);
		
	for(int x = -halfGridSize; x < halfGridSize; ++x) {
		float defaultY = halfGridSize;
		float3 posA = make_float3(x, -defaultY, zPos);
		float3 posB = make_float3(x, defaultY, zPos);

		posA = minus_float3(plus_float3(posA, snappedCamera), gameState->cameraPos);
		posB = minus_float3(plus_float3(posB, snappedCamera), gameState->cameraPos);
		
		pushLine(renderer, posA, posB, gridColor);
	}

	for(int y = -halfGridSize; y < halfGridSize; ++y) {
		float defaultX = halfGridSize;
		float3 posA = make_float3(-defaultX, y, zPos);
		float3 posB = make_float3(defaultX, y, zPos);

		posA = minus_float3(plus_float3(posA, snappedCamera), gameState->cameraPos);
		posB = minus_float3(plus_float3(posB, snappedCamera), gameState->cameraPos);
		
		pushLine(renderer, posA, posB, gridColor);
	}
}
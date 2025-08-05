
int getLocalBoardIndex(int x, int y, int z, float3 origin) {
	return DOUBLE_MAX_MOVE_DISTANCE*DOUBLE_MAX_MOVE_DISTANCE*z + DOUBLE_MAX_MOVE_DISTANCE*(y - origin.y) + (x - origin.x);
}

void pushOnFloodFillQueue(GameState *gameState, FloodFillEvent *queue, bool *visited, float3 *cameFrom, float3 cameFromThisTile, int x, int y, int z, float3 origin, float3 startP) {
	int index = getLocalBoardIndex(x, y, z, origin);
		
	if(index >= 0 && index < MAX_ASTAR_ARRAY_LENGTH && !visited[index]) { 
		//NOTE: Now check if it's a valid square to stand on i.e. not water or tree or house etc.
		float2 chunkP = getChunkPosForWorldP(make_float2(x, y));
        Chunk *c = getChunk(gameState, &gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, chunkP.x, chunkP.y, 0, false, false);
		if(c) {
			float3 tileP = getChunkLocalPos(x, y, z);
			Tile *tile = c->getTile(tileP.x, tileP.y, tileP.z);
			if(tile && (tile->flags & TILE_FLAG_WALKABLE)) { //(tile->entityOccupation == 0 || sameFloat3(make_float3(x, y, z), startP))
				
				FloodFillEvent *node = pushStruct(&globalPerFrameArena, FloodFillEvent);
				node->x = x;
				node->y = y;
				node->z = z;

				queue->next->prev = node;
				node->next = queue->next;

				queue->next = node;
				node->prev = queue;

				cameFrom[index] = cameFromThisTile;
			}
		}

		//say you visited it
		visited[index] = true;
	}
}


FloodFillEvent *popOffFloodFillQueue(FloodFillEvent *queue) {
	FloodFillEvent *result = 0;

	if(queue->prev != queue) { //something on the queue
		result = queue->prev;

		queue->prev = result->prev;
		queue->prev->next = queue;
	} 

	return result;
}

FloodFillResult floodFillSearch(GameState *gameState, float3 startP, float3 goalP, int maxMoveDistance) {
    bool *visited = pushArray(&globalPerFrameArena, MAX_ASTAR_ARRAY_LENGTH, bool);
	float3 *cameFrom = pushArray(&globalPerFrameArena, MAX_ASTAR_ARRAY_LENGTH, float3);

	//NOTE: Need this because the visited array is local coordinates
	float3 origin = startP;	
	origin.x -= 0.5f*DOUBLE_MAX_MOVE_DISTANCE;
	origin.y -= 0.5f*DOUBLE_MAX_MOVE_DISTANCE;

    FloodFillEvent *queue = pushStruct(&globalPerFrameArena, FloodFillEvent);
    queue->next = queue->prev = queue; 
	pushOnFloodFillQueue(gameState, queue, visited, cameFrom, startP, startP.x, startP.y, startP.z, origin, startP);

	bool searching = true;
	FloodFillEvent *foundNode = 0;
	int maxSearchTime = 1000;
	int searchCount = 0;
	while(searching && searchCount < maxSearchTime) {	
		FloodFillEvent *node = popOffFloodFillQueue(queue);
		if(node) {
			int x = node->x;
			int y = node->y;
			int z = node->z;

			if(x == goalP.x && y == goalP.y && z == goalP.z) {
              //NOTE: Found goal
			  searching = false;
			  foundNode = node;
			} else {
				float3 cameFromThisTile = make_float3(x, y, z);
				//push on more directions   
				pushOnFloodFillQueue(gameState, queue, visited, cameFrom, cameFromThisTile, x + 1, y, z, origin, startP);
				pushOnFloodFillQueue(gameState, queue, visited, cameFrom, cameFromThisTile, x, y + 1, z, origin, startP);
				pushOnFloodFillQueue(gameState, queue, visited, cameFrom, cameFromThisTile, x - 1, y, z, origin, startP);
				pushOnFloodFillQueue(gameState, queue, visited, cameFrom, cameFromThisTile, x, y - 1, z, origin, startP);
				
			}
		} else {
			searching = false;
			break;
		}
		searchCount++;
	}

	FloodFillResult result = {};
	result.foundNode = foundNode;

	if(foundNode) {
		bool building = true;
		result.cameFrom = pushStruct(&globalPerFrameArena, NodeDirection);
		result.cameFrom->p = goalP;
		int pathCount = 0;
		while(building && pathCount <= maxMoveDistance) {
			if(result.cameFrom->p.x == startP.x && result.cameFrom->p.y == startP.y && result.cameFrom->p.z == startP.z) {
				building = false;
			} else {
				NodeDirection *f = pushStruct(&globalPerFrameArena, NodeDirection);
				int index = getLocalBoardIndex(result.cameFrom->p.x, result.cameFrom->p.y, result.cameFrom->p.z, origin);
				assert(index >= 0 && index < MAX_ASTAR_ARRAY_LENGTH);
				float3 posAt = cameFrom[index];

				f->p = posAt;
				f->next = result.cameFrom;
				result.cameFrom = f;
				pathCount++;
			}

			if(pathCount > maxMoveDistance && building) {
				//NOTE: Too far to move, so make it an invalid move
				result.cameFrom = 0;
				result.foundNode = 0;
				building = false;
			}
		}
	}

    return result;
}


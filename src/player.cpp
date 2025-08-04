#define DEFAULT_PLAYER_ANIMATION_SPEED 0.1

void updatePlayerMoveKey(GameState *gameState, PlatformKeyType type, float2 moveVector, float2 *impluse, bool *playerMoved) {
	//&& gameState->player->animationController.lastAnimationOn != &gameState->playerAttackAnimation
	if(global_platformInput.keyStates[type].isDown ) {
		*impluse = plus_float2(moveVector, *impluse);
		*playerMoved = true;
	}
}

void updatePlayerInput(GameState *gameState) {
	if(!gameState->cameraFollowPlayer) {
		return;
	}

	assert(gameState->player->type == ENTITY_MAN);
    
	bool playerMoved = false;

	float2 impluse = make_float2(0, 0);

	//NOTE: Move up
	updatePlayerMoveKey(gameState, PLATFORM_KEY_UP, make_float2(0, 1), &impluse, &playerMoved);
	updatePlayerMoveKey(gameState, PLATFORM_KEY_DOWN, make_float2(0, -1), &impluse, &playerMoved);
	updatePlayerMoveKey(gameState, PLATFORM_KEY_LEFT, make_float2(-1, 0), &impluse, &playerMoved);
	updatePlayerMoveKey(gameState, PLATFORM_KEY_RIGHT, make_float2(1, 0), &impluse, &playerMoved);

	//NOTE: Update the walking animation 
	if(playerMoved) {
		Animation *animation = 0;
		float margin = 0.1f;
		removeEntityFlag(gameState->player, ENTITY_SPRITE_FLIPPED);

		if((impluse.x > margin  && impluse.y < -margin) || (impluse.x < -margin && impluse.y > margin) || ((impluse.x > margin && impluse.y < margin && impluse.y > -margin))) { //NOTE: The extra check is becuase the front & back sideways animation aren't matching - should flip one in the aesprite
			addEntityFlag(gameState->player, ENTITY_SPRITE_FLIPPED);
		}

		// if(impluse.y > margin) {
		// 	if(impluse.x < -margin || impluse.x > margin) {
		// 		animation = &gameState->playerbackwardSidewardRun;	
		// 	} else {
		// 		animation = &gameState->playerRunbackwardAnimation;
		// 	}
		// } else if(impluse.y < -margin) {
		// 	if(impluse.x < -margin || impluse.x > margin) {
		// 		animation = &gameState->playerforwardSidewardRun;	
		// 	} else {
		// 		animation = &gameState->playerRunForwardAnimation;
		// 	}
		// } else {
		// 	animation = &gameState->playerRunsidewardAnimation;
		// }

		// //NOTE: Push the Run Animation
		// if(gameState->player->animationController.lastAnimationOn != &gameState->playerJumpAnimation && gameState->player->animationController.lastAnimationOn != animation)  {
		// 	easyAnimation_emptyAnimationContoller(&gameState->player->animationController, &gameState->animationState.animationItemFreeListPtr);
		// 	easyAnimation_addAnimationToController(&gameState->player->animationController, &gameState->animationState.animationItemFreeListPtr, animation, DEFAULT_PLAYER_ANIMATION_SPEED);	
		// }
	}

	impluse = normalize_float2(impluse);
	impluse = scale_float2(gameState->player->speed, impluse); 

	gameState->player->velocity.xy = plus_float2(gameState->player->velocity.xy, impluse);

	//NOTE: IDLE ANIMATION
	// if(!playerMoved && gameState->player->animationController.lastAnimationOn != &gameState->playerJumpAnimation && gameState->player->animationController.lastAnimationOn != &gameState->playerIdleAnimation && gameState->player->animationController.lastAnimationOn != &gameState->playerAttackAnimation)  {
	// 	easyAnimation_emptyAnimationContoller(&gameState->player->animationController, &gameState->animationState.animationItemFreeListPtr);
	// 	easyAnimation_addAnimationToController(&gameState->player->animationController, &gameState->animationState.animationItemFreeListPtr, &gameState->playerIdleAnimation, 0.08f);	
	// }
	
}
void playerDie(GameState *gameState, Entity *entity) {
    if(gameState->targetGameMode != GAME_GAMEOVER_MODE) {
        gameState->gameModeFadeTimer = MAX_FADE_TIME;
        gameState->gameModeFadeDirection = -1; //NOTE: Fade Out
        gameState->targetGameMode = GAME_GAMEOVER_MODE;
    }
}

void bearDie(GameState *gameState, Entity *entity) {
    easyAnimation_emptyAnimationContoller(&entity->animationController, &gameState->animationState.animationItemFreeListPtr);
    easyAnimation_addAnimationToController(&entity->animationController, &gameState->animationState.animationItemFreeListPtr, &entity->animations->dead, 0.08f);
    entity->skeletonCountdown = 10;
    entity->flags &= ~ENTITY_ATTACK_PLAYER;
    // Entity *newPickupItem = addPickupItem(gameState, plus_float3(make_float3(0, 0, 0), entity->pos), PICKUP_ITEM_BEAR_PELT);
    // if(newPickupItem) {
    // }
}

void templerKnightDie(GameState *gameState, Entity *entity) {
    entity->flags &= ~ENTITY_ACTIVE;
}
void ghostDie(GameState *gameState, Entity *entity) {
    entity->flags &= ~ENTITY_ACTIVE;
}

void treeDie(GameState *gameState, Entity *attackEntity) {
    easyAnimation_addAnimationToController(&attackEntity->animationController, &gameState->animationState.animationItemFreeListPtr, &attackEntity->animations->dead, 0.08f);

    Entity *newTree = addAshTreeEntity(gameState, plus_float3(make_float3(4, -1, 0), attackEntity->pos));
    if(newTree) {
        easyAnimation_emptyAnimationContoller(&newTree->animationController, &gameState->animationState.animationItemFreeListPtr);
        easyAnimation_addAnimationToController(&newTree->animationController, &gameState->animationState.animationItemFreeListPtr, &attackEntity->animations->fallen, 0.08f);
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
            Particler *p = getNewParticleSystem(&gameState->particlers, particleP, gameState->smokeTextures, arrayCount(gameState->smokeTextures), spawnMargin, 20);
            
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
    } else {
        assert(false);
    }
}
                        
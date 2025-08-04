
void addCollisionEvent(Collider *a1, Entity *b, float2 hitDir) {
    CollideEventType type = COLLIDE_ENTER;

    CollideEvent *oldEvent = 0;

    //NOTE: See if there is already an event for this entity
    for(int i = 0; i < a1->collideEventsCount; ++i) {
        oldEvent = &a1->events[i];

        if(oldEvent->entityId == b->id) {
            if(oldEvent->type == COLLIDE_EXIT) {
                type = COLLIDE_ENTER;
            } else if(!oldEvent->hitThisFrame) {
                type = COLLIDE_STAY;
            }
            
            break;
        } 
    }

    CollideEvent *e = oldEvent;
    //NOTE: Add a new event
    if(!oldEvent && a1->collideEventsCount < arrayCount(a1->events)) {
        e = a1->events + a1->collideEventsCount++;
        
        e->type = type;
        e->entityId = b->id;
        e->entityType = b->type;
        e->hitThisFrame = true;
    } else if(oldEvent) {
        assert(oldEvent);

        if(!oldEvent->hitThisFrame) {
            //NOTE: Don't override info that has been added this frame
            oldEvent->type = type;
            oldEvent->hitThisFrame = true;
        }
    }

    //NOTE: Update the damage value based on entity state
    if(e) {
        e->hitDir = hitDir;
    }
}

void updateExitCollision(Collider *a1, Entity *b) {
    //NOTE: Loop though all the collision events on this collider and see if there was one with this colliding entity. 
    for(int i = 0; i < a1->collideEventsCount; ++i) {
        CollideEvent *oldEvent = &a1->events[i];
        if(oldEvent->entityId == b->id) {
            if(oldEvent->type == COLLIDE_EXIT) {
                //NOTE: Marked for removal by not setting hitThisFrame
                int b = 0;
            } else {
                //NOTE: Was ENTER or STAY collision type, so make it now exit
                oldEvent->type = COLLIDE_EXIT;
                oldEvent->hitThisFrame = true;
            }
            break;
        } 

    }
}

void prepareCollisions(Collider *a1) {
    for(int i = 0; i < a1->collideEventsCount; ++i) {
        CollideEvent *e = &a1->events[i];
        e->hitThisFrame = false;
    }
}

void clearStaleCollisions(Collider *a1) {
    for(int i = 0; i < a1->collideEventsCount; ) {
        int addend = 1;
        CollideEvent *e = &a1->events[i];

        if(!e->hitThisFrame) {
            //NOTE: Remove event
            a1->events[i] = a1->events[--a1->collideEventsCount];
            addend = 0;
        }
        i += addend;
    }
}

void updateTriggerCollision(Entity *a, Entity *b, Collider *a1) {
    float3 aPos = getWorldPosition(a);
    float3 bPos = getWorldPosition(b);

    Rect2f aRect = make_rect2f_center_dim(plus_float3(aPos, a1->offset).xy, float3_hadamard(a1->scale, a->scale).xy);
    Rect2f bRect = make_rect2f_center_dim(bPos.xy, b->scale.xy);

    float2 dir = minus_float2(rect2f_getCenter(aRect), rect2f_getCenter(bRect));

    //NOTE: See if it hit anything
    Rect2f minowskiPlus = rect2f_minowski_plus(aRect, bRect, b->pos.xy);
    if(in_rect2f_bounds(minowskiPlus, a->pos.xy)) {
        addCollisionEvent(a1, b, dir);
    } else {
        updateExitCollision(a1, b);
    }
}


struct RayCastResult {
    float2 collisionNormal; //NOTE: Pointing outwards
    float distance; //NOTE: Between 0 - 1 of the incoming deltaP
    float distanceToTest;
	bool hit;

    Entity *a;
    Collider *colA;
    Entity *b;
    Collider *colB;
};

RayCastResult castAgainstSide(float2 startP, float2 normal, float2 startSide, float3 deltaP_, float sideLength) {

    float2 tangent = float2_perp(normal);

    RayCastResult result = {};
    float2 relA_ = minus_float2(startP, startSide);
    float2 relB_ = minus_float2(plus_float2(startP, deltaP_.xy), startSide);

    float2 relA = float2_transform(relA_, normal, tangent);
    float2 relB = float2_transform(relB_, normal, tangent);

    float2 deltaP = minus_float2(relB, relA);

    // C = t*(B - A) + A
    float tValue = (-relA.x) / deltaP.x;

    if(tValue >= 0.0f && tValue <= 1.0f) {
        float yValue = lerp(relA.y, relB.y, make_lerpTValue(tValue)); 

        if(yValue >= 0.0f && yValue <= sideLength) {

            float epsilon = 0.01;

            result.hit = true;
            result.distanceToTest = tValue;
            result.distance = clamp(0, 1, tValue - epsilon);
            //NOTE: Make the normal facing outwards
            result.collisionNormal = float2_negate(normal);
        }
    }
    return result;
}

RayCastResult rayCast_rectangle(float3 startP, float3 deltaP, Rect2f rect) {
	RayCastResult result = {};
    result.distanceToTest = 1.0f;

    //NOTE: LEFT 
    {
        float2 startSide = make_float2(rect.minX, rect.minY);
        float2 normal = make_float2(1, 0);
        float sideLength =(rect.maxY - rect.minY);

        RayCastResult result1 = castAgainstSide(startP.xy, normal, startSide, deltaP, sideLength);

        if(result1.hit) {
            result = result1;
        }
    }

    //NOTE: TOP
    {
        float2 startSide = make_float2(rect.minX, rect.maxY);
        float2 normal = make_float2(0, -1);
        float sideLength =(rect.maxX - rect.minX);

        RayCastResult result1 = castAgainstSide(startP.xy, normal, startSide, deltaP, sideLength);

        if(result1.hit) {
            if(result1.distanceToTest < result.distanceToTest) {
                result = result1;
            }
        }
    }

    //NOTE: RIGHT
    {
        float2 startSide = make_float2(rect.maxX, rect.maxY);
        float2 normal = make_float2(-1, 0);
        float sideLength =(rect.maxY - rect.minY);

        RayCastResult result1 = castAgainstSide(startP.xy, normal, startSide, deltaP, sideLength);

        if(result1.hit) {
            if(result1.distanceToTest < result.distanceToTest) {
                result = result1;
            }
        }
    }

    //NOTE: BOTTOM
    {
        float2 startSide = make_float2(rect.maxX, rect.minY);
        float2 normal = make_float2(0, 1);
        float sideLength =(rect.maxX - rect.minX);

        RayCastResult result1 = castAgainstSide(startP.xy, normal, startSide, deltaP, sideLength);

        if(result1.hit) {
            if(result1.distanceToTest < result.distanceToTest) {
                result = result1;
            }
        }
    }
    

	return result;
}


void updateCollisions(Entity *a, Entity *b, Collider *a1, Collider *b1, float *shortestDistance, RayCastResult *shortestRayCastResult) {
    float3 aPos = getWorldPosition(a);
    float3 bPos = getWorldPosition(b);

    Rect2f aRect = make_rect2f_center_dim(plus_float3(aPos, a1->offset).xy, float3_hadamard(a1->scale, a->scale).xy);
    Rect2f bRect = make_rect2f_center_dim(plus_float3(bPos, b1->offset).xy, float3_hadamard(b1->scale, b->scale).xy);

    //NOTE: See if it hit anything
    Rect2f minowskiPlus = rect2f_minowski_plus(aRect, bRect, plus_float3(b->pos, b1->offset).xy);

    //NOTE: Ray cast agains minkowski rectangle
    RayCastResult r = rayCast_rectangle(aPos, a->deltaPos, minowskiPlus);

    if(r.hit && r.distanceToTest < *shortestDistance) {
        //NOTE: See if shortest distance
        *shortestRayCastResult = r;
        shortestRayCastResult->a = a;
        shortestRayCastResult->b = b;
        shortestRayCastResult->colA = a1;
        shortestRayCastResult->colB = b1;

        *shortestDistance = r.distanceToTest;
    }
}

void updateEntityPhysics(GameState *gameState, float dt) {
    for(int i = 0; i < gameState->entityCount; ++i) {
		Entity *a = &gameState->entities[i];

		// for(int colliderIndex = 0; colliderIndex < arrayCount(a->colliders); ++colliderIndex) {
		// 	Collider *colA = &a->colliders[colliderIndex];

        //     if(colA->flags & COLLIDER_ACTIVE) {
		// 	    prepareCollisions(colA);
        //     }
		// }

        //NOTE: Apply drag 
        //TODO: This isn't frame independent
        a->velocity.xy = scale_float2(0.6f, a->velocity.xy); 

        a->deltaPos.xy = scale_float2(dt, a->velocity.xy);
        a->deltaTLeft = 1.0f;
	}

// 	//NOTE: Collision detection
	for(int iterationIndex = 0; iterationIndex < 1; ++iterationIndex) {
		for(int i = 0; i < gameState->entityCount; ++i) {

			Entity *a = &gameState->entities[i];

            float shortestDistance = 1.0f; //NOTE: Percentage
            RayCastResult shortestRayCastResult = {};

            //NOTE: Update TILE collisions first
            if(float3_magnitude(a->deltaPos) > 0.0f) { 
                float3 entWorldP = getWorldPosition(a);

                for(int i = 0; i < gameState->tileCount; ++i) {
                    MapTile t = gameState->tiles[i];

                    if(t.collidable) {
                        float2 tileP = make_float2(t.x + 0.5f, t.y + 0.5f);

                        Rect2f tileRect = make_rect2f_center_dim(tileP, make_float2(1, 1));
                        Rect2f entRect = make_rect2f_center_dim(entWorldP.xy, a->scale.xy);

                        Rect2f minowskiPlus = rect2f_minowski_plus(tileRect, entRect, tileP);

                        //NOTE: Ray cast agains minkowski rectangle
                        RayCastResult r = rayCast_rectangle(entWorldP, a->deltaPos, minowskiPlus);

                        if(r.hit && r.distanceToTest < shortestDistance) {
                            //NOTE: See if shortest distance
                            shortestRayCastResult = r;
                            shortestDistance = r.distanceToTest;
                        }
                    }
                }
            }

                    
            //NOTE: Process other entity collisions
            // for(int j = 0; j < gameState->entityCount; ++j) {
            //     if(i == j) {
            //         continue;
            //     }

            //     Entity *b = &gameState->entities[j];

            //     assert(a->idHash != b->idHash);

                
            //     if(b->flags & ENTITY_ACTIVE)
            //     {
            //         //NOTE: See if trigger
            //         if((colA->flags & COLLIDER_TRIGGER)) {
            //             updateTriggerCollision(a, b, colA);
                        
            //         } else { 
            //             //TODO: continous collision
            //         //    updateCollisions(a, b, colA, colB, &shortestDistance, &shortestRayCastResult);
            //         }
            //     }
            // }

            //NOTE: Update position and velocity
            if(shortestRayCastResult.hit) {
                if(shortestRayCastResult.colA) {
                    addCollisionEvent(shortestRayCastResult.colA, shortestRayCastResult.b, make_float2(0, 0));
                    // addCollisionEvent(shortestRayCastResult.colB, shortestRayCastResult.a);
                }

                float2 moveVector = scale_float2(shortestRayCastResult.distance, a->deltaPos.xy);
                a->pos.xy = plus_float2(moveVector,  a->pos.xy);

                //NOTE: Take the component out of the velocity
                float2 d = scale_float2(float2_dot(a->velocity.xy, shortestRayCastResult.collisionNormal), shortestRayCastResult.collisionNormal);
                a->velocity.xy = minus_float2(a->velocity.xy, d);

                //NOTE: Remove distance left to travel
                a->deltaTLeft -= shortestRayCastResult.distance;
                if(a->deltaTLeft > 0) {
                    a->deltaPos.xy = scale_float2(dt*a->deltaTLeft, a->velocity.xy);
                } else {
                    a->deltaPos = make_float3(0, 0, 0);
                    a->deltaTLeft = 0;
                }

                //NOTE: See if the entity is standing on anything
                float slopeFactor = 0.3f;
                if(float2_dot(shortestRayCastResult.collisionNormal, make_float2(0, 1)) > slopeFactor) {
                    //  a->grounded = true;
                }
            } else {
                 a->pos.xy = plus_float2(a->deltaPos.xy,  a->pos.xy);
                 a->deltaTLeft = 0;
                 a->deltaPos = make_float3(0, 0, 0);
            }
        }
    }

	// for(int i = 0; i < gameState->entityCount; ++i) {
	// 	Entity *a = &gameState->entities[i];

	// 	for(int colliderIndex = 0; colliderIndex < arrayCount(a->colliders); ++colliderIndex) {
	// 		Collider *colA = &a->colliders[colliderIndex];

    //         if(colA->flags & COLLIDER_ACTIVE) {
	// 		    clearStaleCollisions(colA);
    //         }
	// 	}
	// }
}



struct Particle {
    TransformX T;
    float3 dP;
    float lifeTime;
    bool alive;
    Texture *imageHandle;
};

struct PariclePattern {
    float2 randomSize;
    float dpMargin;
    float speed;
};

struct ParticlerId {
    int id; 
    bool invalid;
};

ParticlerId makeParticlerId(int id) {
    ParticlerId result = {};
    result.id = id;
    return result;
}

ParticlerId getInvalidParticleId() {
    ParticlerId result = {};
    result.id = -1;
    result.invalid = true;
    return result;
}

bool particlerIdsMatch(ParticlerId id, ParticlerId id1) {
    return id.id == id1.id;
}

struct Particler {
    ParticlerId id;
    Particle particles[255];
    int count;
    int indexAt;
    PariclePattern pattern;
    bool warmStart;

    int maxTextureArrayCount;

    float3 spawnBox;
    float3 worldP;
    Texture *imageHandles;

    u32 flags; //NOTE: These are entity flags 

    int colorCount;
    float4 colors[4]; //NOTE: Moves through colors as lifespan increases

    float lastTimeCreation;
    float lifespan;
    float lifeAt;
    float tAt;
    float spawnRate; //NOTE: seconds per particle
};

void resetParticlerLife(Particler *p) {
    p->lifeAt = 0;
}

void updateParticlerWorldPosition(Particler *p, float3 worldP) {
    p->worldP = worldP;
}

Particler initParticler(float lifespan, float spawnRate, float3 pos, float3 spawnBox, Texture *imageHandles, int maxTextureArrayCount, ParticlerId id) {
    Particler p = {};

    p.count = 0;
    p.flags = 0;
    p.lifespan = lifespan;
    p.spawnRate = 1.0f / spawnRate;
    p.lastTimeCreation = 0;
    p.tAt = 0;
    p.maxTextureArrayCount = maxTextureArrayCount;
    p.lifeAt = 0;
    p.indexAt = 0;
    p.worldP = pos;
    p.spawnBox = spawnBox;
    p.imageHandles = imageHandles;
    p.id = id;
    p.warmStart = true;

    return p;   
}

bool addColorToParticler(Particler *p, float4 color) {
    bool result = false;
    if(p->colorCount < arrayCount(p->colors)) {
        p->colorCount++;
        //NOTE: We push them on in reverse order for the gradient lerping. Because it makes it less confusing when thinking about that code
        for(int i = (p->colorCount - 1); i > 0; --i) {
            p->colors[i] = p->colors[i - 1];    
        }
        p->colors[0] = color;
        result = true;
    }
    return result;
}



bool updateParticler(Renderer *renderer, Particler *particler, float3 cameraPos, float dt, PariclePattern pattern) {
    particler->tAt += dt;
    particler->lifeAt += dt;

    bool isDead = (particler->lifeAt >= particler->lifespan);

    float diff = (particler->tAt - particler->lastTimeCreation);
    if(!isDead && (diff >= particler->spawnRate) || particler->warmStart) {
        
        int numberOfParticles = diff / particler->spawnRate;

        if(particler->warmStart) {
            numberOfParticles = 1.0f / particler->spawnRate;
            particler->warmStart = false;
        }
        assert(numberOfParticles > 0);

        for(int i = 0; i < numberOfParticles; ++i) {
            Particle *p = 0;

            if(particler->count < arrayCount(particler->particles)) {
                p = &particler->particles[particler->count++];
                
            } else {
                //NOTE: Already full so use the ring buffer index
                assert(particler->indexAt < arrayCount(particler->particles));
                p = &particler->particles[particler->indexAt++];

                if(particler->indexAt >= arrayCount(particler->particles)) {
                    particler->indexAt = 0;
                }
            }

            assert(p);

            Rect3f spawnBox = make_rect3f_center_dim(particler->worldP, particler->spawnBox);
            float x = lerp(spawnBox.minX, spawnBox.maxX, make_lerpTValue((float)rand() / RAND_MAX));
            float y = lerp(spawnBox.minY, spawnBox.maxY, make_lerpTValue((float)rand() / RAND_MAX));
            float z = lerp(spawnBox.minZ, spawnBox.maxZ, make_lerpTValue((float)rand() / RAND_MAX));
            p->T.pos = make_float3(x, y, z);
            p->T.scale = make_float3(random_between_float(pattern.randomSize.x, pattern.randomSize.y), random_between_float(pattern.randomSize.x, pattern.randomSize.y), 0.1f);

            int imageIndex = random_between_int(0, particler->maxTextureArrayCount);
            p->imageHandle = &particler->imageHandles[imageIndex];

            p->dP = make_float3(random_between_float(-pattern.dpMargin, pattern.dpMargin), random_between_float(-pattern.dpMargin, pattern.dpMargin), pattern.speed); //NOTE: Straight up
            p->lifeTime = MAX_PARTICLE_LIFETIME; //Seconds
            p->alive = true;
        }

        particler->lastTimeCreation = particler->tAt;
    }
    
    int deadCount = 0;
    for(int i = 0; i < particler->count; ++i) {
        Particle *p = &particler->particles[i];

        if(p->alive) {
            float3 accelForFrame = scale_float3(dt, make_float3(0, 0, 0));

            //NOTE: Integrate velocity
            p->dP = plus_float3(p->dP, accelForFrame); //NOTE: Already * by dt 

            //NOTE: Apply drag
            // p->dP = scale_float3(0.95f, p->dP);

            //NOTE: Get the movement vector for this frame
            p->T.pos = plus_float3(p->T.pos, scale_float3(dt, p->dP));

            float3 drawP = getRenderWorldP(p->T.pos);

            drawP.x -= cameraPos.x;
            drawP.y -= cameraPos.y;
            drawP.z = RENDER_Z;

            float4 color = (particler->colorCount == 1) ? particler->colors[0] : make_float4(1, 1, 1, 1);

            //NOTE Update whether the particle should still be alive
            p->lifeTime -= dt;

            if(p->lifeTime <= 0) {
                p->alive = false;
            }

            if(particler->colorCount > 1) {
                //NOTE: This is lerping between the color gradients if there is more than one color
                float v = p->lifeTime / MAX_PARTICLE_LIFETIME;
                assert(v >= -dt && v < 1.0f);
                if(v < 0) {
                    v = 0;
                }

                float pos = lerp(0, particler->colorCount - 1, make_lerpTValue(v));
              
                int index = floor(pos);
                int index1 = (index + 1 < particler->colorCount) ? index + 1 : index;
               
                assert(index >= 0 && index < particler->colorCount);
                assert(index1 >= 0 && index1 < particler->colorCount);

                float t = pos - index;
                t = clamp(0.0f, 1.0f, t);
                color = lerp_float4(particler->colors[index], particler->colors[index1],  t);
            }

            //NOTE: Draw the particle
            
            pushEntityTexture(renderer, p->imageHandle->handle, drawP, p->T.scale.xy, color, p->imageHandle->uvCoords, getSortIndex(p->T.pos));
          
        } else {
            deadCount++;
        }
    }

    bool shouldRemove = false;

    if(isDead && deadCount == particler->count){
        //NOTE: Particle system should be removed
        shouldRemove = true;
    }

    return shouldRemove;

}

static int global_particleId = 0;

struct ParticlerParent {
    int particlerCount;
    Particler particlers[1024];
};

Particler *getNewParticleSystem(ParticlerParent *parent, float3 startP, Texture *imageHandles, int maxTextureArrayCount, float3 spawnArea, float spawnRatePerSecond) {
    Particler *p = 0;
    assert(parent->particlerCount < arrayCount(parent->particlers));
    if(parent->particlerCount < arrayCount(parent->particlers)) {
        float lifespan = 0.1f; //NOTE: Seconds for the _particler_ not particles. Particle lifespan is set by MAX_PARTICLE_LIFESPAN
        parent->particlers[parent->particlerCount++] = initParticler(lifespan, spawnRatePerSecond, startP, spawnArea, imageHandles, maxTextureArrayCount, makeParticlerId(global_particleId++));
        p = &parent->particlers[parent->particlerCount - 1];
        
    }

    return p;
}
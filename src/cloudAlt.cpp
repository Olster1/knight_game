
void drawCloudsAsTexture(GameState *gameState, Renderer *renderer, float dt, float16 fovMatrix, float2 windowScale) {
    DEBUG_TIME_BLOCK();

    TextureHandle *atlasHandle = gameState->textureAtlas.texture.handle;
    int cloudDistance = 3;
    for(int y = cloudDistance; y >= -cloudDistance; --y) {
        for(int x = -cloudDistance; x <= cloudDistance; ++x) {
            Chunk *c = gameState->terrain.getChunk(&gameState->lightingOffsets, &gameState->animationState, &gameState->textureAtlas, x, y, 0, true, false);
            if(c && (c->generateState == CHUNK_NOT_GENERATED || c->cloudFadeTimer >= 0)) {
                float maxTime = 1.5f;
                float chunkScale = CHUNK_DIM + CHUNK_DIM;

                bool notEntered = true;
                //NOTE: Generate the clouds
                if(!c->cloudTexture.textureHandle) {
                    notEntered = false;
                    float2 chunkInPixels = make_float2(chunkScale*TILE_WIDTH_PIXELS, chunkScale*TILE_WIDTH_PIXELS);
                    c->cloudTexture = platform_createFramebuffer(chunkInPixels.x, chunkInPixels.y);
                    assert(c->cloudTexture.framebuffer > 0);
                    pushRenderFrameBuffer(renderer, c->cloudTexture.framebuffer);
                    pushClearColor(renderer, make_float4(1, 1, 1, 0));

                    pushMatrix(renderer, make_ortho_matrix_bottom_left_corner(chunkScale, chunkScale, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE));
                    pushViewport(renderer, make_float4(0, 0, chunkInPixels.x, chunkInPixels.y));
                    renderer_defaultScissors(renderer, chunkInPixels.x, chunkInPixels.y);
                    
                    for(int i = 0; i < MAX_CLOUD_DIM; i++) {
                        for(int j = 0; j < MAX_CLOUD_DIM; j++) {
                            float3 p = {};
                            p.x += random_between_float(0, CHUNK_DIM) + 0.5f*CHUNK_DIM;
                            p.y += random_between_float(0, CHUNK_DIM) + 0.5f*CHUNK_DIM;
                            assert(c->cloudCount < arrayCount(c->clouds));
                            p.z = RENDER_Z;

                            int cloudIndex = random_between_int(0, 3);
                            assert(cloudIndex < 3);
                            // d->fadePeriod = random_between_float(0.4f, maxTime);
                            float scale = random_between_float(1, 5);
                            float darkness = random_between_float(0.95f, 1.0f);

                            AtlasAsset *t = gameState->cloudText[cloudIndex];
                            pushTexture(renderer, atlasHandle, p, make_float2(scale, scale), make_float4(darkness, darkness, darkness, 1), t->uv);
                        }
                    }

                    pushRenderFrameBuffer(renderer, 0);
                    pushMatrix(renderer, fovMatrix);
                    pushViewport(renderer, make_float4(0, 0, windowScale.x, windowScale.y));
                    renderer_defaultScissors(renderer, windowScale.x, windowScale.y);
                }   
                
                // for(int i = 0; i < c->cloudCount; ++i) {
                //     CloudData *cloud = &c->clouds[i];
                //     float3 worldP = make_float3(x*CHUNK_DIM, y*CHUNK_DIM, CLOUDS_RENDER_Z);
                //     worldP.x += cloud->pos.x;
                //     worldP.y += cloud->pos.y;
                //     worldP.x -= gameState->cameraPos.x;
                //     worldP.y -= gameState->cameraPos.y;
                    
                //     float s = cloud->scale;

                //     float tVal = c->cloudFadeTimer;
                //     if(tVal < 0) {
                //         tVal = 0;
                //     }
                //     float alpha = lerp(0.4f, 0, make_lerpTValue(tVal / cloud->fadePeriod));
                    
                //     AtlasAsset *t = gameState->cloudText[cloud->cloudIndex];
                    
                //     float2 scale = make_float2(s, s*t->aspectRatio_h_over_w);
                //     float darkness = cloud->darkness;
                //     pushTexture(renderer, atlasHandle, worldP, scale, make_float4(cloud->darkness, cloud->darkness, cloud->darkness, alpha), t->uv);
                // }

                {
                     if(!c->generatedCloudMipMaps && notEntered) {
                        generateMipMapsForTexture(c->cloudTexture.textureHandle);
                        c->generatedCloudMipMaps = true;
                    }

                    //NOTE: Draw the texture
                    float tVal = c->cloudFadeTimer;
                    if(tVal < 0) {
                        tVal = 0;
                    }
                    float alpha = lerp(1.0f, 0, make_lerpTValue(tVal / maxTime));

                    float3 renderP = getChunkWorldP(c);
                    renderP.x -= gameState->cameraPos.x;
                    renderP.y -= gameState->cameraPos.y;
                    renderP.x += 0.5f*CHUNK_DIM;
                    renderP.y += 0.5f*CHUNK_DIM;
                    renderP.z = RENDER_Z;

                    pushTexture(renderer, c->cloudTexture.textureHandle, renderP, make_float2(chunkScale, chunkScale), make_float4(1, 1, 1, alpha), make_float4(0, 1, 1, 0));
                }

                if(c->cloudFadeTimer >= 0) {
                    c->cloudFadeTimer += dt;

                    if(c->cloudFadeTimer >= maxTime) {
                        //NOTE: Turn fade timer off
                        c->cloudFadeTimer = -1;
                    }
                }
                
            }
        }
    }
}

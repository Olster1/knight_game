enum UiAnchorPoint {
    UI_ANCHOR_BOTTOM_LEFT,
    UI_ANCHOR_BOTTOM_RIGHT,
    UI_ANCHOR_TOP_LEFT,
    UI_ANCHOR_TOP_RIGHT,
    UI_ANCHOR_CENTER,

    UI_ANCHOR_CENTER_LEFT,
    UI_ANCHOR_CENTER_RIGHT,
    UI_ANCHOR_CENTER_TOP,
    UI_ANCHOR_CENTER_BOTTOM,
    
};

float2 getUiPosition(float2 percentOffset, UiAnchorPoint anchorPoint, float2 pos, float2 resolution) {
    float inverse100 = 1.0f / 100.0f;
    float2 t = make_float2(resolution.x*percentOffset.x*inverse100, resolution.y*percentOffset.y*inverse100);

    switch(anchorPoint) {
        case UI_ANCHOR_BOTTOM_LEFT: {
            pos.x += t.x;
            pos.y += t.y;
        } break;
        case UI_ANCHOR_BOTTOM_RIGHT: {
            pos.x = (resolution.x - pos.x) - t.x;
            pos.y += t.y;
        } break;
        case UI_ANCHOR_TOP_LEFT: {
            pos.x += t.x;
            pos.y = (resolution.y - pos.y) - t.y;
        } break;
        case UI_ANCHOR_TOP_RIGHT: {
            pos.x = (resolution.x - pos.x) - t.x;
            pos.y = (resolution.y - pos.y) - t.y;
        } break;
        case UI_ANCHOR_CENTER: {
            pos.x = (0.5f*resolution.x) + t.x;
            pos.y = (0.5f*resolution.y) + t.y;
        } break;
        case UI_ANCHOR_CENTER_LEFT: {
            pos.x += t.x;
            pos.y = 0.5f*resolution.y + t.y;
        } break;
        case UI_ANCHOR_CENTER_RIGHT: {
            pos.x = (resolution.x - pos.x) - t.x;
            pos.y = 0.5f*resolution.y + t.y;
        } break;
        case UI_ANCHOR_CENTER_TOP: {
            pos.x = (0.5f*resolution.x) + t.x;
            pos.y = (resolution.y - pos.y) - t.y;
        } break;
        case UI_ANCHOR_CENTER_BOTTOM: {
            pos.x = (0.5f*resolution.x) + t.x;
            pos.y += t.y;
        } break;
        default: {

        };
    }
    return pos;
}

void drawScrollText(char *text, GameState *gameState, Renderer *renderer, float2 percentOffset, UiAnchorPoint anchorPoint, float2 resolution) {
    DEBUG_TIME_BLOCK();
	pushShader(renderer, &pixelArtShader);
    float ar = gameState->bannerTexture.aspectRatio_h_over_w;
    float scalef = 25;
    float2 scale = make_float2(scalef, ar*scalef);


    Rect2f bounds = getTextBounds(renderer, &gameState->pixelFont, text, 0, 0, 0.1); 
    float2 bScale = get_scale_rect2f(bounds);

    scale.x = math3d_maxfloat(1.1f*bScale.x, scale.x);
    scale.y = math3d_maxfloat(bScale.y, scale.y);

    float2 pos = make_float2(0.5f*scale.x, 0.5f*scale.y);

    pos = getUiPosition(percentOffset, anchorPoint, pos, resolution);
    
    float sOffset = 0;
    pushTexture(renderer, gameState->shadowUiTexture.handle, make_float3(pos.x + sOffset, pos.y - sOffset, UI_Z_POS), scale, make_float4(1, 1, 1, 0.3), gameState->shadowUiTexture.uvCoords);
    pushTexture(renderer, gameState->bannerTexture.handle, make_float3(pos.x, pos.y, UI_Z_POS), scale, make_float4(1, 1, 1, 1), gameState->bannerTexture.uvCoords);

    pushShader(renderer, &sdfFontShader);
    draw_text(renderer, &gameState->pixelFont, text, pos.x - bScale.x*0.5f, pos.y + bScale.y*0.5f, 0.1, make_float4(0, 0, 0, 1)); 
}

void drawGameUi(GameState *gameState, Renderer *renderer, float dt, float windowWidth, float windowHeight, float2 mouseP_01){
	DEBUG_TIME_BLOCK();

    bool clicked = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0;
    
    float aspectRatio_yOverX = windowHeight / windowWidth;
    float fuaxWidth = 100;
	float2 resolution = make_float2(fuaxWidth, fuaxWidth*aspectRatio_yOverX);
	float16 fovMatrix = make_ortho_matrix_bottom_left_corner(resolution.x, resolution.y, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
    pushMatrix(renderer, fovMatrix);

	float mousex = lerp(0, resolution.x, make_lerpTValue(mouseP_01.x));
    float mousey = lerp(0, resolution.y, make_lerpTValue(mouseP_01.y));
	
    // drawScrollText("YOUR TURN", gameState, renderer, make_float2(1, 1));

    if(gameState->actionString) {
        char *str = easy_createString_printf(&globalPerFrameArena, gameState->actionString);
        // drawScrollText(str, gameState, renderer, make_float2(1, 1), UI_ANCHOR_CENTER_BOTTOM, resolution);
    }
    gameState->actionString = 0;
    
    // {
    //NOTE: Drawing the time left string
    //     int sec = (int)(play->maxTurnTime - play->turnTime);
    //     int min = sec / 60;
    //     sec -= min*60;
    
    //     str = easy_createString_printf(&globalPerFrameArena, "Time Left: %d:%d", min, sec);
    //     drawScrollText(str, gameState, renderer, make_float2(1, 1), UI_ANCHOR_TOP_LEFT, resolution);
    // }



    if(gameState->currentItemInfo.title) 
    {
        float3 pos = make_float3(mousex, mousey, UI_Z_POS);
        float ar = gameState->bannerTexture.aspectRatio_h_over_w;

        float fontSize = 0.06f;
        float fontSizeDesc = 0.05f;
        float defaultWidth = 30;
        Rect2f boundsTitle = getTextBounds(renderer, &gameState->pixelFont, gameState->currentItemInfo.title, 0, 0, fontSize); 

        float marginX = math3d_maxfloat(get_scale_rect2f(boundsTitle).x, defaultWidth);

        Rect2f boundsDescription = getTextBounds(renderer, &gameState->font, gameState->currentItemInfo.description, 0, 0, fontSizeDesc, marginX); 

        float2 scale = get_scale_rect2f(rect2f_union(boundsTitle, boundsDescription));

        scale.y = get_scale_rect2f(boundsTitle).y + get_scale_rect2f(boundsDescription).y;

        scale.x = math3d_maxfloat(scale.x, defaultWidth);
        scale.y = math3d_maxfloat(scale.y, ar*defaultWidth);

        float textureScale = 1.1f;
        float2 offset = {};
        offset.x = textureScale*scale.x - scale.x;
        offset.y = textureScale*scale.y - scale.y;
        
        // pushTexture(renderer, gameState->shadowUiTexture.handle, make_float3(pos.x + textureScale*0.5f*scale.x, pos.y - textureScale*0.5f*scale.y, UI_Z_POS), scale_float2(1.1*textureScale, scale), make_float4(1, 1, 1, 0.3), gameState->shadowUiTexture.uvCoords);
        pushTexture(renderer, global_white_texture, make_float3(pos.x + textureScale*0.5f*scale.x, pos.y - textureScale*0.5f*scale.y, UI_Z_POS), scale_float2(textureScale, scale), make_float4(0.769, 0.643, 0.518, 1), make_float4(0, 0, 1, 1));

        pushShader(renderer, &sdfFontShader);
        draw_text(renderer, &gameState->pixelFont, gameState->currentItemInfo.title, pos.x + 0.5f*offset.x, pos.y - 0.5f*offset.y, fontSize, make_float4(0, 0, 0, 1)); 
        draw_text(renderer, &gameState->font, gameState->currentItemInfo.description, pos.x + 0.5f*offset.x, (pos.y - get_scale_rect2f(boundsTitle).y) - 0.5f*offset.y, fontSizeDesc, make_float4(0, 0, 0, 1), marginX); 
    }

    {
        float3 mouseP = make_float3(mousex, mousey, UI_Z_POS);
        float2 s = scale_float2(3, make_float2(5.9f, 10));
        float2 pos = getUiPosition(make_float2(0, 0), UI_ANCHOR_BOTTOM_LEFT, make_float2(0.5f*s.x, 0.5f*s.y), resolution);
        float3 p = make_float3(pos.x, pos.y, UI_Z_POS);
        // pushTexture(renderer, gameState->inventoryTexture.handle, p, s, make_float4(1, 1, 1, 1), gameState->inventoryTexture.uvCoords);

        for(int i = 0; i < arrayCount(gameState->inventory.items); ++i) {
            if(gameState->inventory.items[i].type != PICKUP_ITEM_NONE) {
                float2 s = scale_float2(3, make_float2(1, 1));
                int x = (i % 3) + 1;
                int y = (i / 3) + 1;
                float2 pos = getUiPosition(make_float2(0.9f * x, 1.1f * y), UI_ANCHOR_BOTTOM_LEFT, make_float2(0.5f*s.x, 0.5f*s.y), resolution);
                float3 p = make_float3(pos.x, pos.y, UI_Z_POS);

                DefaultEntityAnimations *defaultAnimation = getAnimationForPickupItem(gameState, gameState->inventory.items[i].type);

                if(defaultAnimation) {
                    if(in_rect2f_bounds(make_rect2f_center_dim(p.xy, s), mouseP.xy) || gameState->placeItem == gameState->inventory.items[i].type) {
                        pushTexture(renderer, global_white_texture, p, s, make_float4(0.3, 1, 0, 1), make_float4(0, 0, 1, 1));    
                        gameState->placeItem = gameState->inventory.items[i].type;
                    }
                    pushTexture(renderer, defaultAnimation->idle.frameSprites[0]->handle, p, s, make_float4(1, 1, 1, 1), defaultAnimation->idle.frameSprites[0]->uvCoords);

                }
            }
        }
    }
    
    {
        // drawScrollText(text, gameState, renderer, make_float2(0, 1), UI_ANCHOR_CENTER_BOTTOM, resolution);
    }
    

    
    
}
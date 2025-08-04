static void DEBUG_draw_stats(GameState *gameState, Renderer *renderer, Font *font, float windowWidth, float windowHeight, float dt) {

	float16 orthoMatrix = make_ortho_matrix_top_left_corner(windowWidth, windowHeight, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
	pushMatrix(renderer, orthoMatrix);

	//NOTE: Draw the backing
	pushShader(renderer, &textureShader);
	float2 scale = make_float2(200, 400);
	// pushTexture(renderer, global_white_texture, make_float3(100, -200, 1.0f), scale, make_float4(0.3f, 0.3f, 0.3f, 1), make_float4(0, 0, 1, 1));
	///////////////////////////


	//NOTE: Draw the name of the file
	pushShader(renderer, &sdfFontShader);
		
	float fontScale = 0.6f;
	float4 color = make_float4(1, 1, 1, 1);

	float xAt = 0;
	float yAt = -1.5f*font->fontHeight*fontScale;

	float spacing = font->fontHeight*fontScale;

#define DEBUG_draw_stats_MACRO(title, size, draw_kilobytes) { char *name_str = 0; if(draw_kilobytes) { name_str = easy_createString_printf(&globalPerFrameArena, "%s  %d %dkilobytes", title, size, size/1000); } else { name_str = easy_createString_printf(&globalPerFrameArena, "%s  %d", title, size); } draw_text(renderer, font, name_str, xAt, yAt, fontScale, color); yAt -= spacing; }
#define DEBUG_draw_stats_FLOAT_MACRO(title, f0, f1) { char *name_str = 0; name_str = easy_createString_printf(&globalPerFrameArena, "%s  %f  %f", title, f0, f1); draw_text(renderer, font, name_str, xAt, yAt, fontScale, color); yAt -= spacing; }
	
	DEBUG_draw_stats_MACRO("Total Heap Allocated", global_debug_stats.total_heap_allocated, true);
	DEBUG_draw_stats_MACRO("Total Virtual Allocated", global_debug_stats.total_virtual_alloc, true);
	DEBUG_draw_stats_MACRO("Render Command Count", global_debug_stats.render_command_count, false);
	DEBUG_draw_stats_MACRO("Draw Count", global_debug_stats.draw_call_count, false);
	DEBUG_draw_stats_MACRO("Heap Block Count ", global_debug_stats.memory_block_count, false);
	DEBUG_draw_stats_MACRO("Per Frame Arena Total Size", DEBUG_get_total_arena_size(&globalPerFrameArena), true);
	DEBUG_draw_stats_MACRO("Per Entity Load Arena Total Size", DEBUG_get_total_arena_size(&globalPerEntityLoadArena), true);

	// WL_Window *w = &gameState->windows[gameState->active_window_index];
	// DEBUG_draw_stats_FLOAT_MACRO("Start at: ", gameState->selectable_state.start_pos.x, gameState->selectable_state.start_pos.y);
	// DEBUG_draw_stats_FLOAT_MACRO("Target Scroll: ", w->scroll_target_pos.x, w->scroll_target_pos.y);

	DEBUG_draw_stats_FLOAT_MACRO("mouse position 01 ", global_platformInput.mouseX / windowWidth, global_platformInput.mouseY / windowHeight);
	DEBUG_draw_stats_FLOAT_MACRO("dt for frame ", dt, dt);

}

void drawDebugAndEditorText(GameState *gameState, Renderer *renderer, float fauxDimensionX, float fauxDimensionY, float windowWidth, float windowHeight, float dt, float16 fovMatrix) {

	 if(global_platformInput.keyStates[PLATFORM_KEY_O].pressedCount > 0 && global_platformInput.keyStates[PLATFORM_KEY_CTRL].isDown) {
        char *result = platform_openFileDialog();

        loadSaveLevel_json(gameState, result);

        //TODO: Not sure how to free this string
    }

    if(global_platformInput.keyStates[PLATFORM_KEY_S].pressedCount > 0 && global_platformInput.keyStates[PLATFORM_KEY_CTRL].isDown) {
        char *result = platform_saveFileDialog();
        saveLevel_version1_json(gameState, result);

        //TODO: Not sure how to free this string
    }

	if(global_platformInput.keyStates[PLATFORM_KEY_1].pressedCount > 0) {
		gameState->gameMode = PLAY_MODE;
	} else if(global_platformInput.keyStates[PLATFORM_KEY_2].pressedCount > 0) {
		if(gameState->gameMode == TILE_MODE) {
			gameState->gameMode = PLAY_MODE;
			gameState->cameraFollowPlayer = true;
		} else {
			gameState->cameraFollowPlayer = false;
			gameState->gameMode = TILE_MODE;
		}
		
	} else if(global_platformInput.keyStates[PLATFORM_KEY_3].pressedCount > 0) {
		if(gameState->gameMode == SELECT_ENTITY_MODE) {
			gameState->gameMode = PLAY_MODE;
		} else {
			gameState->gameMode = SELECT_ENTITY_MODE;
		}
	} else if(global_platformInput.keyStates[PLATFORM_KEY_4].pressedCount > 0) {
		if(gameState->gameMode == A_STAR_MODE) {
			gameState->gameMode = PLAY_MODE;
		} else {
			gameState->gameMode = A_STAR_MODE;
		}
	} 

	if(global_platformInput.keyStates[PLATFORM_KEY_MINUS].pressedCount > 0) {
		gameState->zoomLevel += 0.1f;
	}

	if(global_platformInput.keyStates[PLATFORM_KEY_PLUS].pressedCount > 0) {
		gameState->zoomLevel -= 0.1f;
		if(gameState->zoomLevel < 0.01f) {
			gameState->zoomLevel = 0.01f;
		}
	}

	

	if(global_platformInput.keyStates[PLATFORM_KEY_5].pressedCount > 0) {
		gameState->draw_debug_memory_stats = !gameState->draw_debug_memory_stats;
	}
	
	if(gameState->gameMode == TILE_MODE) {
		// drawAndUpdateEditorGui(gameState, renderer, 0, 0, windowWidth, windowHeight);
	} else if(gameState->gameMode == A_STAR_MODE) {
		pushShader(renderer, &textureShader);
		pushMatrix(renderer, fovMatrix);

		// updateAStartEditor(gameState, renderer, windowWidth, windowHeight);
	}

    float16 orthoMatrix1 = make_ortho_matrix_bottom_left_corner(fauxDimensionX, fauxDimensionY, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);
	pushMatrix(renderer, orthoMatrix1);
	pushShader(renderer, &sdfFontShader);
	{
		char *name_str = "PLAY MODE";
		if(gameState->gameMode == TILE_MODE) {
			name_str = "TILE MODE";
		} else if(gameState->gameMode == SELECT_ENTITY_MODE) {
			name_str = "SELECT ENTITY MODE";
		} else if(gameState->gameMode == A_STAR_MODE) {
			name_str = "A* MODE";
		}

		if(gameState->drawState->openState == EASY_PROFILER_DRAW_CLOSED) {
			draw_text(renderer, &gameState->font, name_str, 50, fauxDimensionY - 50, 1, make_float4(0, 0, 0, 1)); 
		}
	}

	if(gameState->draw_debug_memory_stats) {
		renderer_defaultScissors(renderer, windowWidth, windowHeight);
		DEBUG_draw_stats(gameState, renderer, &gameState->font, windowWidth, windowHeight, dt);
	}
}
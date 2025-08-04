bool editorGui_isSameId(EditorGuiId a, EditorGuiId b) {
    return (a.a == b.a && a.b == b.b && a.type == b.type);
}

static void editorGui_clearInteraction(EditorGui *gui) {
    gui->currentInteraction.active = false;
}

static int wrapUndoRedoIndex(EditorGui *gui, int index) {

    int result = index + arrayCount(gui->undoRedoBlocks);

    result = result & (arrayCount(gui->undoRedoBlocks) - 1);

    return result; 
    
}

static void addUndoRedoBlock(EditorGui *gui, UndoRedoBlock block) {
    //NOTE: If overflowed buffer go to zero
    if(gui->undoRedoCursorAt >= arrayCount(gui->undoRedoBlocks)) {
        gui->undoRedoStartOfRingBuffer = wrapUndoRedoIndex(gui, gui->undoRedoStartOfRingBuffer + 1);
        gui->undoRedoCursorAt--;
    }

    int index = wrapUndoRedoIndex(gui, gui->undoRedoCursorAt + gui->undoRedoStartOfRingBuffer);

    assert(index >= 0 && index < arrayCount(gui->undoRedoBlocks));
    gui->undoRedoBlocks[index] = block;

    gui->undoRedoCursorAt++;

    if(gui->undoRedoCursorAt > arrayCount(gui->undoRedoBlocks)) {
        gui->undoRedoCursorAt = arrayCount(gui->undoRedoBlocks);
    }

    gui->undoRedoTotalCount = gui->undoRedoCursorAt;
}   

bool sameEntityId(EditorGuiId a, EditorGuiId b) {
    bool result = (a.type == b.type && a.a == b.a && a.c == b.c);
    return result;
}

float2 getClickedWorldPos(GameState *state, float2 mouseP_01) {
    float worldX = lerp(-0.5f*state->planeSizeX, 0.5f*state->planeSizeX, make_lerpTValue(mouseP_01.x));
    float worldY = lerp(-0.5f*state->planeSizeY, 0.5f*state->planeSizeY, make_lerpTValue(mouseP_01.y));

    worldX += state->cameraPos.x;
    worldY += state->cameraPos.y;


    //NOTE: Make sure the tile goes in the cell you click
    if(worldX < 0) {
        worldX = floor(worldX);
    }

    if(worldY < 0) {
        worldY = floor(worldY);
    }

    return make_float2(worldX, worldY);
}

Entity *findEntityById(GameState *state, u32 id) {
    Entity *result = 0;
    for(int i = 0; i < state->entityCount;++i) {
        Entity *e = &state->entities[i];

        if(e->id == id) {
            result = e;
            break;
        }
    }
    return result;
}

// void updateAStartEditor(GameState *state, Renderer *renderer, float windowWidth, float windowHeight) {
//     if(state->selectedEntityCount > 0) {
//         float2 mouseP = make_float2(global_platformInput.mouseX, windowHeight - global_platformInput.mouseY);

//         float2 mouseP_01 = make_float2(mouseP.x / windowWidth, mouseP.y / windowHeight);

//         bool clicked = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0;

//         Entity *e = findEntityById(state, state->selectedEntityId);

//         if(!e->aStarController) {
//             e->aStarController = easyAi_initController(&globalPerEntityLoadArena);
//         }
            
//         if(e) {
//             if(clicked) {
//                 float2 worldP = getClickedWorldPos(state, mouseP_01);
//                 worldP.x = (int)worldP.x;
//                 worldP.y = (int)worldP.y;   

//                 if(global_platformInput.keyStates[PLATFORM_KEY_CTRL].isDown) {
//                     //NOTE: Add Search bouys instead

//                     int index = easyAi_hasSearchBouy(e->aStarController, make_float3(worldP.x, worldP.y, 0));

//                     if(index >= 0) {
//                         //NOTE: Remove the search bouy
//                         easyAi_removeSearchBouy(e->aStarController, index);
//                     } else {
//                         easyAi_pushSearchBouy(e->aStarController, make_float3(worldP.x, worldP.y, 0));
//                     }

//                 } else {
//                     EasyAi_Node *node = easyAi_hasNode(e->aStarController, make_float3(worldP.x, worldP.y, 0), e->aStarController->boardHash, true);
                
//                     if(node) {
//                         //NOTE: Remove an ai block
//                         easyAi_removeNode(e->aStarController, make_float3(worldP.x, worldP.y, 0), e->aStarController->boardHash);
//                     } else {
//                         easyAi_pushNode(e->aStarController, make_float3(worldP.x, worldP.y, 0), e->aStarController->boardHash, true);
//                     }
//                 }
//             }

//             //NOTE: Draw the a * board
//             EasyAiController *aiController = e->aStarController;
//             for(int i = 0; i < arrayCount(aiController->boardHash); ++i) {
//                 EasyAi_Node *n = aiController->boardHash[i];

//                 while(n) {

//                     float pX = (n->pos.x + 0.5f) - state->cameraPos.x;
// 		            float pY = (n->pos.y + 0.5f)  - state->cameraPos.y;

//                     pushRect(renderer, make_float3(pX, pY, 10), make_float2(1, 1), n->canSeePlayerFrom ? make_float4(0.5f, 0.5f, 0, 1) : make_float4(0.5f, 0, 1, 1));

//                     n = n->next;
//                 }
//             }   

//             //NOTE: Draw the search bouys
//             for(int i = 0; i < aiController->searchBouysCount; ++i) {
//                 float3 *value = &aiController->searchBouys[i];

//                 float pX = (value->x + 0.5f) - state->cameraPos.x;
//                 float pY = (value->y + 0.5f)  - state->cameraPos.y;

//                 pushRect(renderer, make_float3(pX, pY, 10), make_float2(0.5f, 0.5f), make_float4(0, 0, 1, 1));

//             }
//         }
//     }
// }

void drawAndUpdateEditorGui(GameState *state, Renderer *renderer, float x, float y, float windowWidth, float windowHeight) {

    float2 mouseP = make_float2(global_platformInput.mouseX, windowHeight - global_platformInput.mouseY);

    float2 mouseP_01 = make_float2(mouseP.x / windowWidth, mouseP.y / windowHeight);

    bool clicked = global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].pressedCount > 0;

    TileSet *swampSet = &state->sandTileSet; 

    EditorGui *gui = &state->editorGuiState;

    if(global_platformInput.keyStates[PLATFORM_KEY_Z].pressedCount > 0 && global_platformInput.keyStates[PLATFORM_KEY_CTRL].isDown) {
        if(global_platformInput.keyStates[PLATFORM_KEY_SHIFT].isDown) {
           //NOTE: REDO
            if(gui->undoRedoCursorAt < gui->undoRedoTotalCount) {
                int nextIndex = wrapUndoRedoIndex(gui, gui->undoRedoCursorAt + gui->undoRedoStartOfRingBuffer);
                 //NOTE: Get the block
                UndoRedoBlock block =  gui->undoRedoBlocks[nextIndex];

                //NOTE: Execute the block
                //NOTE: Remove last tile block if have one from map
                if(block.hasLastTile) {
                    MapTileFindResult result = findMapTile(state, block.lastMapTile);
                    assert(result.found);

                    removeMapTile(state, result.indexAt);
                }

                //NOTE: Put in the current one back into the map
                assert(state->tileCount < arrayCount(state->tiles));
                MapTile *t = state->tiles + state->tileCount++;

                *t = block.mapTile;

                gui->undoRedoCursorAt++;
            }
        } else {
            //NOTE: UNDO
            if(gui->undoRedoCursorAt > 0) {
                int lastIndex = wrapUndoRedoIndex(gui, (gui->undoRedoCursorAt - 1) + gui->undoRedoStartOfRingBuffer);
                //NOTE: Get the block
                UndoRedoBlock block =  gui->undoRedoBlocks[lastIndex];

                //NOTE: Execute the block
                //NOTE: Remove current tile from map
                {
                    MapTileFindResult result = findMapTile(state, block.mapTile);
                    assert(result.found);
                    removeMapTile(state, result.indexAt);

                }

                //NOTE: Put in the last tile into map if valid
                if(block.hasLastTile) {
                    assert(state->tileCount < arrayCount(state->tiles));
                    MapTile *t = state->tiles + state->tileCount++;

                    *t = block.lastMapTile;
                }


                gui->undoRedoCursorAt--;

                assert(gui->undoRedoCursorAt >= 0);
            }
        }
	}

    
	if(global_platformInput.keyStates[PLATFORM_KEY_ESCAPE].pressedCount > 0) {
        editorGui_clearInteraction(&state->editorGuiState);
	}

    if(swampSet->count > 0) {
        float16 orthoMatrix = make_ortho_matrix_bottom_left_corner(windowWidth, windowHeight, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);

        pushMatrix(renderer, orthoMatrix);

        float aspectRatio = swampSet->tiles[0]->height / swampSet->tiles[0]->width;

        float size = 600;

        float scaleFactor = size / swampSet->tiles[0]->width;

        float tileSizeX = swampSet->tileSizeX * scaleFactor;
        float tileSizeY = swampSet->tileSizeY * scaleFactor;

        int xIndex = (mouseP.x - x) / tileSizeX; 
        int yIndex = (mouseP.y - y) / tileSizeY; 

        EditorGuiId thisId = {};

        thisId.a = xIndex;
        thisId.b = yIndex;
        thisId.type = TILE_SELECTION_SWAMP;

        //NOTE: Draw the backing
        pushShader(renderer, &textureShader);
        float2 scale = make_float2(size, size*aspectRatio);
        pushTexture(renderer, swampSet->tiles[0]->handle, make_float3(x + 0.5f*scale.x, y + 0.5f*scale.y, 2), scale, make_float4(1, 1, 1, 1), make_float4(0, 0, 1, 1));

        bool isActive = (state->editorGuiState.currentInteraction.active && state->editorGuiState.currentInteraction.id.type == TILE_SELECTION_SWAMP);

        bool inSelectionBounds = in_rect2f_bounds(make_rect2f_min_dim(x, y, scale.x, scale.y), mouseP);

        if(inSelectionBounds && clicked) {
            editorGui_clearInteraction(&state->editorGuiState);
	    }

        if((!state->editorGuiState.currentInteraction.active && inSelectionBounds) || isActive) {
            //NOTE: Get the tile mouse is hovering over
           
            pushShader(renderer, &rectOutlineShader);

            float4 selectColor = make_float4(0.3f, 0, 0.3f, 1);

            if(isActive) {
                selectColor = make_float4(0, 0.3f, 0.3f, 1);

                xIndex = state->editorGuiState.currentInteraction.id.a;
                yIndex = state->editorGuiState.currentInteraction.id.b;
            }

            Rect2f r = make_rect2f_min_dim(xIndex*tileSizeX + x, yIndex*tileSizeY + y, tileSizeX, tileSizeY);

            float2 tileP = get_centre_rect2f(r);
            float2 tileScaleP = get_scale_rect2f(r);

           
           if(!isActive) {
                if(global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].isDown) {
                    selectColor = make_float4(0.3f, 0.3f, 0, 1);
                } else if(global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].releasedCount > 0) {
                    state->editorGuiState.currentInteraction.id = thisId;
                    state->editorGuiState.currentInteraction.active = true;

                    selectColor = make_float4(0.3f, 0.3f, 0, 1);
                }
            }
        
            pushTexture(renderer, global_white_texture, make_float3(tileP.x, tileP.y, 1.0f), tileScaleP, selectColor, make_float4(0, 0, 1, 1));
        }


        //NOTE: See if user added tile to map
        if(!inSelectionBounds && global_platformInput.keyStates[PLATFORM_MOUSE_LEFT_BUTTON].isDown && isActive) {
            //NOTE: Add tile to map
            float worldX = lerp(-0.5f*state->planeSizeX*state->zoomLevel, 0.5f*state->planeSizeX*state->zoomLevel, make_lerpTValue(mouseP_01.x));
            float worldY = lerp(-0.5f*state->planeSizeY*state->zoomLevel, 0.5f*state->planeSizeY*state->zoomLevel, make_lerpTValue(mouseP_01.y));

            worldX += state->cameraPos.x;
            worldY += state->cameraPos.y;

            //NOTE: Make sure the tile goes in the cell you click
            if(worldX < 0) {
                worldX = floor(worldX);
            }

            if(worldY < 0) {
                worldY = floor(worldY);
            }
            
            MapTile t = getDefaultMapTile(state, TILE_SET_SAND, worldX, worldY, xIndex, (swampSet->countY - 1) - yIndex, true);

            UndoRedoBlock block = {};
            // block->lastMapTile;
            block.mapTile = t;

            MapTileFindResult mapTileQueryResult = findMapTile(state, t);

            bool addTile = true;

            //NOTE: See if there is a tile here already
            if(mapTileQueryResult.found) {
                if(isSameMapTile(mapTileQueryResult.tile, t)) {
                    //NOTE: Don't add if their exactly the same tile
                    addTile = false;
                } else {
                    block.hasLastTile = true;
                    block.lastMapTile = state->tiles[mapTileQueryResult.indexAt];

                    removeMapTile(state, mapTileQueryResult.indexAt);
                }
            } else {
                block.hasLastTile = false;
            }

            if(addTile) {
                assert(state->tileCount < arrayCount(state->tiles));
                state->tiles[state->tileCount++] = t;

                addUndoRedoBlock(&state->editorGuiState, block);
            }
        }
    }

}
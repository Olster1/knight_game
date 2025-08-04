void DEBUG_runUnitTests(GameState *state) {
    EditorGui *gui = &state->editorGuiState;

    {
        float3 p = getChunkLocalPos(18, 19, 4);
        assert(p.x == 2 && p.y == 3 && p.z == 4);
    }
    {
        float3 p = getChunkLocalPos(-18, -19, -4);
        assert(p.x == 14 && p.y == 13 && p.z == 12);
    }

    assert(0b1 == 1);
    assert(0b11 == 3);

    //NOTE: Make sure array is power of two
    assert((arrayCount(gui->undoRedoBlocks) & (arrayCount(gui->undoRedoBlocks) - 1)) == 0);

    assert(wrapUndoRedoIndex(gui, 6) == 6);
    assert(wrapUndoRedoIndex(gui, 1024) == 0);
    assert(wrapUndoRedoIndex(gui, 1026) == 2);
    assert(wrapUndoRedoIndex(gui, 1024*2) == 0);
    assert(wrapUndoRedoIndex(gui, 1026*2) == 4);

    {
        Rect2f r = make_rect2f_center_dim(make_float2(0, 0), make_float2(1, 1));
        {
            //NOTE: LEFT SIDE
            RayCastResult result = rayCast_rectangle(make_float3(-1, -1, 0), make_float3(2, 2, 0), r);
            assert(result.hit);
        }
        {
            //NOTE: LEFT SIDE
            RayCastResult result = rayCast_rectangle(make_float3(-1, -1, 0), make_float3(-1, -1, 0), r);
            assert(!result.hit);
        }    
        {
            //NOTE: RIGHT SIDE
            RayCastResult result = rayCast_rectangle(make_float3(1, -1, 0), make_float3(-1, 1, 0), r);
            assert(result.hit);
        }   
        {
            //NOTE: RIGHT SIDE
            RayCastResult result = rayCast_rectangle(make_float3(1, -1, 0), make_float3(1, 1, 0), r);
            assert(!result.hit);
        }      
        {
            //NOTE: TOP SIDE
            RayCastResult result = rayCast_rectangle(make_float3(1, 1, 0), make_float3(-1, -1, 0), r);
            assert(result.hit);
        }   
        {
            //NOTE: TOP SIDE
            RayCastResult result = rayCast_rectangle(make_float3(1, 1, 0), make_float3(1, 1, 0), r);
            assert(!result.hit);
        }
        {
            //NOTE: BOTTOM SIDE
            RayCastResult result = rayCast_rectangle(make_float3(0, -1, 0), make_float3(0.1f, 1, 0), r);
            assert(result.hit);
        }   
        {
            //NOTE: BOTTOM SIDE
            RayCastResult result = rayCast_rectangle(make_float3(0, -1, 0), make_float3(0.1f, -1, 0), r);
            assert(!result.hit);
        }      

        
    }
    
}
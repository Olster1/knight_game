enum InteractionType {
    TILE_SELECTION_SWAMP,
    EDITOR_PLAYER_SELECT
};

struct EditorGuiId {
    InteractionType type;
    int a;
    int b;
    u32 c;
};

struct GuiInteraction {
    EditorGuiId id;

    bool active;

};


struct UndoRedoBlock {
    union {
        struct {
            bool hasLastTile;
            MapTile lastMapTile;
            MapTile mapTile;
        };
    };
};

struct EditorGui {
    GuiInteraction currentInteraction; 

    //NOTE: Ring buffer
    int undoRedoTotalCount; //NOTE:  this is relative
    int undoRedoStartOfRingBuffer; //NOTE: Where the next most out of date spot is 
    int undoRedoCursorAt; //NOTE: Where the next most out of date spot is - this is relative 
    UndoRedoBlock undoRedoBlocks[1024]; 
    
};

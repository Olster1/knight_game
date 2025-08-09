typedef enum {
	EASY_PROFILER_DRAW_OVERVIEW,
	EASY_PROFILER_DRAW_ACCUMALTED,
} EasyProfile_DrawType;

typedef enum {
	EASY_PROFILER_DRAW_OPEN,
	EASY_PROFILER_DRAW_CLOSED,
} EasyDrawProfile_OpenState;

typedef struct {
	int hotIndex;

	EasyProfile_DrawType drawType;

	//For navigating the samples in a frame
	float zoomLevel;
	float xOffset; //for the scroll bar
	bool holdingScrollBar;
	float scrollPercent;
	float grabOffset;

	float2 lastMouseP;
	bool firstFrame;

	EasyDrawProfile_OpenState openState;
	float openTimer;

	int level;
} EasyProfile_ProfilerDrawState;

static EasyProfile_ProfilerDrawState *EasyProfiler_initProfilerDrawState() {
	EasyProfile_ProfilerDrawState *result = pushStruct(&global_long_term_arena, EasyProfile_ProfilerDrawState);
		
	result->hotIndex = -1;
	result->level = 0;
	result->drawType = EASY_PROFILER_DRAW_OVERVIEW;

	result->zoomLevel = 1;
	result->holdingScrollBar = false;
	result->xOffset = 0;
	result->lastMouseP =  make_float2(0, 0);
	result->firstFrame = true;
	result->grabOffset = 0;

	result->openTimer = -1;
	result->openState = EASY_PROFILER_DRAW_CLOSED;

	return result;

}

enum GameMode {
	PLAY_MODE,
	TILE_MODE,
	SELECT_ENTITY_MODE,
	A_STAR_MODE
};

struct SelectedEntityData {
	u32 id;
	float3 worldPos;
	bool isValidPos; //NOTE: If this selected entity is allowed to move
	FloodFillResult floodFillResult; //NOTE: Allocated on the perframe arena, so is rubbish across frame boundaries
	Entity *e;
};

struct RenderDamageSplatItem {
    char *string;
    float3 p;
};

typedef struct {
	bool initialized;

	GameMode gameMode;
	char *actionString;

	float2 startDragPForSelect;
	bool holdingSelect;
	int selectedMoveCount;
	int selectedEntityCount;
	SelectedEntityData selectedEntityIds[32];

	//NOTE: For creating unique entity ids like MongoDb ObjectIds
	int randomIdStartApp;
	int randomIdStart;

	Renderer renderer;

	Font font;
	Font pixelFont;

	float shakeTimer;

	float fontScale;

	bool draw_debug_memory_stats;

	EasyProfile_ProfilerDrawState *drawState;

	LightingOffsets lightingOffsets;

	Entity *player;	

	float scrollDp;

	int entityCount;
	Entity entities[2056];

	float3 cameraPos;
	float cameraFOV;

	float planeSizeX;
	float planeSizeY;

	GamePlay gamePlay;

	Texture bannerTexture;
	Texture selectTexture;
	Texture shadowUiTexture;
	Texture kLogoText;
	Texture gLogoText;
	Texture blueText;
	Texture redText;
	Texture selectImage;
	AtlasAsset *cloudText[3];
	Texture treeTexture;
	TextureAtlas textureAtlas;

	EntityMove *freeEntityMoves;

	Texture arrows[5]; //NOTE: goes anti-clockwise, starting at right. Last one is a circle when there is no direction

	float4 selectedColor;

	float selectHoverTimer;

	bool draggingCanvas;
	float2 startDragP;
	float2 canvasMoveDp;
	
	int tileCount;
	MapTile tiles[10000];

	DefaultEntityAnimations knightAnimations;
	DefaultEntityAnimations peasantAnimations;
	DefaultEntityAnimations archerAnimations;

	DefaultEntityAnimations goblinAnimations;
	DefaultEntityAnimations tntAnimations;
	DefaultEntityAnimations barrellAnimations;

	DefaultEntityAnimations manAnimations;
	DefaultEntityAnimations ashTreeAnimations[4];
	DefaultEntityAnimations alderTreeAnimations[2];
	DefaultEntityAnimations templerKnightAnimations;
	DefaultEntityAnimations bearAnimations;
	DefaultEntityAnimations bearPelt;
	DefaultEntityAnimations skinningKnife;
	DefaultEntityAnimations bearTent;
	DefaultEntityAnimations ghostAnimations;

	PickupItemType placeItem;
	
	Inventory inventory;

	Texture smokeTextures[5];
	Texture splatTexture;
	Texture inventoryTexture;

	float3 itemInfoPos;
	ItemInfo currentItemInfo;

	DamageSplat *freeListDamageSplats;
	RenderDamageSplatItem *perFrameDamageSplatArray;

	Texture goblinHutTexture;
	Animation goblinHutAnimation;
	Texture goblinHutBurntTexture;
	Animation goblinHutBurntAnimation;

	Animation goblinTowerAnimation; 
	Texture goblinTowerBurntTexture;
	Animation goblinTowerBurntAnimation;

	float3 averageStartBoardPos;
	int averageStartBoardCount;
	
	Texture towerTexture;
	Animation towerAnimation;
	Texture towerBurntTexture;
	Animation towerBurntAnimation;
	Texture houseTexture;
	Animation houseAnimation;
	Texture castleTexture;
	Animation castleAnimation;
	Texture houseBurntTexture;
	Animation houseBurntAnimation;
	Texture castleBurntTexture;
	Animation castleBurntAnimation;

	Texture backgroundTexture;

	AnimationState animationState;

	ParticlerParent particlers;

	GameDialogs dialogs;
	TileSet sandTileSet;

	Terrain terrain;

	EditorGui editorGuiState;

	bool cameraFollowPlayer;

	float zoomLevel;
} GameState;
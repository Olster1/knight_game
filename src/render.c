struct TextureHandle {
#ifdef __APPLE__
	u32 handle;
#else 
	void *handle;
#endif 
};
static TextureHandle* global_white_texture;

enum RenderBlendMode {
	RENDER_BLEND_MODE_DEFAULT,
	RENDER_BLEND_MODE_ADD, //NOTE: Used for particle effects
};

enum RenderLayer {
	RENDER_LAYER_0,
	RENDER_LAYER_1,
	RENDER_LAYER_2,
	RENDER_LAYER_3,
	RENDER_LAYER_4,
	RENDER_LAYER_5,
	RENDER_LAYER_6,
};

struct RenderSortIndex {
	float3 worldP;
	u32 layer;
};

struct InstanceEntityData {
    float3 pos;
    float2 scale;
    float4 color;
    float4 uv;
    TextureHandle *textureHandle;
    u32 aoMask;
	RenderSortIndex sortIndex;
};

RenderSortIndex getSortIndex(float3 worldP, RenderLayer renderLayer = RENDER_LAYER_0) {
    RenderSortIndex result = {};

    result.worldP = worldP;
	result.layer = renderLayer;

    return result;
}


struct InstanceData {
    float3 pos;
    float2 scale;
    float4 color;
    float4 uv;
    float textureIndex;
    u32 aoMask;
};

struct InstanceDataLine {
    float3 pos1;
    float3 pos2;
    float4 color;
};


struct Texture {
	TextureHandle *handle;

	float width;
	float height;
	float aspectRatio_h_over_w;

	float4 uvCoords;
};

enum RenderCommandType { 
	RENDER_NULL,
	RENDER_GLYPH,
	RENDER_TEXTURE,
	RENDER_LINE,
	RENDER_MATRIX,
	RENDER_CLEAR_COLOR_BUFFER,
	RENDER_SET_FRAME_BUFFER,
	RENDER_SET_VIEWPORT,
	RENDER_SET_SHADER,
	RENDER_SET_SCISSORS,
	RENDER_SET_BLEND_MODE,
};

struct RenderObject {
	Texture *sprite;
	float3 pos;
	float2 scale;
	float4 uvs;
	u32 lightingMask;
	int sortIndex;

	RenderObject() {}

	RenderObject(Texture *sprite,
		float3 pos,
		float2 scale, u32 lightingMask, int sortIndex = 0) {
			this->sprite = sprite;
			this->pos = pos;
			this->scale = scale;
			this->lightingMask = lightingMask;
			this->sortIndex = sortIndex;
	}
};


#define MAX_TEXTURE_COUNT_PER_DRAW_BATCH 1

//NOTE: Eventually we'll want to change this render command to be tight, that is just the command type, and then a data block after it based on what the command is
//		so we're not using up uneccessary space.


typedef struct {
	RenderCommandType type;

	u32 frameId; //NOTE: For setting a frambuffer object
	float16 matrix; //NOTE: For the Render_Matrix type
	float4 color; //NOTE: For the RENDER_CLEAR_COLOR_BUFFER
	float4 viewport; //NOTE: For the RENDER_SET_VIEWPORT
	void *shader; //NOTE: For the RENDER_SET_SHADER
	RenderBlendMode blendMode; //NOTE: For the RENDER_SET_BLEND_MODE command

	Rect2f scissors_bounds; //NOTE: For RENDER_SET_SCISSORS 

	int textureHandle_count;
	void *texture_handles[MAX_TEXTURE_COUNT_PER_DRAW_BATCH]; //NOTE: Used to send to the shader

	int instanceCount;
	int offset_in_bytes;
	int size_in_bytes;
} RenderCommand;

#define MAX_TEXTURE_COUNT 16384*10
#define MAX_GLYPH_COUNT 16384
#define MAX_LINE_COUNT 16384
#define MAX_RENDER_COMMAND_COUNT 16384

#define SIZE_OF_GLYPH_INSTANCE_IN_BYTES (sizeof(InstanceData))
#define GLYPH_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES MAX_GLYPH_COUNT*SIZE_OF_GLYPH_INSTANCE_IN_BYTES

#define SIZE_OF_TEXTURE_INSTANCE_IN_BYTES (sizeof(InstanceData))
#define TEXTURE_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES MAX_TEXTURE_COUNT*SIZE_OF_TEXTURE_INSTANCE_IN_BYTES

#define SIZE_OF_LINE_INSTANCE_IN_BYTES (sizeof(InstanceDataLine))
#define LINE_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES MAX_LINE_COUNT*SIZE_OF_LINE_INSTANCE_IN_BYTES

struct GameLight {
    float3 viewPos;
    float3 color;

};
float getTimeOfDayValueMapped(float t) {
    // t in [0,1], where 0 = midnight, 0.5 = noon, 1 = midnight
    return 0.5f * (1.0f - cos(t * TAU32));
}

typedef struct {
	RenderCommandType currentType;

	int commandCount;
	RenderCommand commands[MAX_RENDER_COMMAND_COUNT];

	//NOTE: Instance data
	int glyphCount;
	u8  glyphInstanceData[GLYPH_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES]; //NOTE: This would be x, y, z, r, g, b, a, u, v, s, t, index

	int textureCount;
	u8  textureInstanceData[TEXTURE_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES]; //NOTE: This would be x, y, z, r, g, b, a, u, v, s, t, index

	//NOTE: We store all the entities in a array, sort them,  then we push them all at once to the renderer
	int entityRenderCount;
	InstanceEntityData entityRenderData[MAX_TEXTURE_COUNT];

	//NOTE: We store all the tiles in a array, sort them,  then we push them all at once to the renderer
	int tileEntityRenderCount;
	InstanceEntityData tileRenderData[MAX_TEXTURE_COUNT];

	int lineCount;
	u8  lineInstanceData[LINE_INSTANCE_DATA_TOTAL_SIZE_IN_BYTES]; //NOTE: This would be x, y, z, x, y, z, r, g, b, a

	float totalTime;

	int lightCount;
    GameLight lights[64];
	float dayNightValue;

} Renderer;

static void initRenderer(Renderer *r) {
	r->commandCount = 0;
	r->currentType = RENDER_NULL;
	r->glyphCount = 0;
	r->textureCount = 0;
	r->lineCount = 0;
	r->totalTime = 0;
	r->entityRenderCount = 0;
	r->tileEntityRenderCount = 0;
}

static void clearRenderer(Renderer *r) {
	r->commandCount = 0;
	r->currentType = RENDER_NULL;
	r->glyphCount = 0;	
	r->textureCount = 0;
	r->entityRenderCount = 0;
	r->tileEntityRenderCount = 0;
}

static void render_endCommand(Renderer *r) {
	r->currentType = RENDER_NULL;
}

//NOTE: This assumes that we want to always override commands. We need to buffer them if we do.
static RenderCommand *getRenderCommand(Renderer *r, RenderCommandType type) {
	RenderCommand *command = 0;

	if(r->currentType != type) {
		if(r->commandCount < MAX_RENDER_COMMAND_COUNT) {
			command = &r->commands[r->commandCount++];
			command->type = type;
			command->instanceCount = 0;
			command->size_in_bytes = 0;
			command->textureHandle_count = 0;
			r->currentType = type;
			
			if(type == RENDER_GLYPH) {		
				//TODO: Block from adding any more glpyhs
				if(r->glyphCount < MAX_GLYPH_COUNT) {
					command->offset_in_bytes = r->glyphCount*SIZE_OF_GLYPH_INSTANCE_IN_BYTES;
					
				} else {
					assert(!"glyph data buffer full");
				}
			} else if(type == RENDER_TEXTURE) {
				if(r->textureCount < MAX_TEXTURE_COUNT) {
					command->offset_in_bytes = r->textureCount*SIZE_OF_TEXTURE_INSTANCE_IN_BYTES;
				} else {
					assert(!"texture data buffer full");
				}
			} else if(type == RENDER_LINE) {		
				//TODO: Block from adding any more glpyhs
				if(r->lineCount < MAX_LINE_COUNT) {
					command->offset_in_bytes = r->lineCount*SIZE_OF_LINE_INSTANCE_IN_BYTES;
					
				} else {
					assert(!"glyph data buffer full");
				}
			} 

			
		} else {
			assert(!"Command buffer full");
		}

	} else {
		assert(r->commandCount > 0);
		command = &r->commands[r->commandCount - 1];
	}

	assert(r->commandCount > 0);

	return command;
}

static void pushMatrix(Renderer *r, float16 m) {
	RenderCommand *c = getRenderCommand(r, RENDER_MATRIX);

	assert(c->type == RENDER_MATRIX);
	c->matrix = m;
}

static void pushShader(Renderer *r, void *shader) {
	RenderCommand *c = getRenderCommand(r, RENDER_SET_SHADER);

	assert(c->type == RENDER_SET_SHADER);
	c->shader = shader;
}

static void pushViewport(Renderer *r, float4 v) {
	RenderCommand *c = getRenderCommand(r, RENDER_SET_VIEWPORT);

	assert(c->type == RENDER_SET_VIEWPORT);
	c->viewport = v;
}


static void pushClearColor(Renderer *r, float4 color) {
	render_endCommand(r); //NOTE: So you can put more than one clear color on in a row
	RenderCommand *c = getRenderCommand(r, RENDER_CLEAR_COLOR_BUFFER);

	assert(c->type == RENDER_CLEAR_COLOR_BUFFER);
	c->color = color;
}

static void pushRenderFrameBuffer(Renderer *r, u32 frameId) {
	render_endCommand(r); //NOTE: So you can put more than one clear color on in a row
	RenderCommand *c = getRenderCommand(r, RENDER_SET_FRAME_BUFFER);

	assert(c->type == RENDER_SET_FRAME_BUFFER);
	c->frameId = frameId;
}

static int render_getTextureIndex(RenderCommand *c, TextureHandle *textureHandle) {

	int index = -1;//NOTE: -1 for unitialized

	//NOTE: Check if it's in the array
	for(int i = 0; i < c->textureHandle_count && index < 0; ++i) {
		void *handle = c->texture_handles[i];

		if(handle == textureHandle) {
			index = i;
			break;
		}
	}

	//NOTE: Add to the array now
	if(index < 0 && c->textureHandle_count < MAX_TEXTURE_COUNT_PER_DRAW_BATCH) {
		index = c->textureHandle_count++;
		c->texture_handles[index] =  textureHandle;

	}

	return index;
}

static void pushGlyph(Renderer *r, TextureHandle *textureHandle, float3 pos, float2 size, float4 color, float4 uv) {

	RenderCommand *c = getRenderCommand(r, RENDER_GLYPH);
	int textureIndex = render_getTextureIndex(c, textureHandle);

	if(textureIndex < 0) {
		render_endCommand(r);

		c = getRenderCommand(r, RENDER_GLYPH);
		textureIndex = render_getTextureIndex(c, textureHandle);
		assert(textureIndex >= 0);
	}

	assert(c->type == RENDER_GLYPH);
	
	if(r->glyphCount < MAX_GLYPH_COUNT) {
		float *data = (float *)(r->glyphInstanceData + r->glyphCount*SIZE_OF_GLYPH_INSTANCE_IN_BYTES);

		data[0] = pos.x;
		data[1] = pos.y;
		data[2] = pos.z;

		data[3] = size.x;
		data[4] = size.y;
		
		data[5] = color.x;
		data[6] = color.y;
		data[7] = color.z;
		data[8] = color.w;

		data[9] = uv.x;
		data[10] = uv.y;
		data[11] = uv.z;
		data[12] = uv.w;

		data[13] = textureIndex;

		r->glyphCount++;
		c->instanceCount++;
		c->size_in_bytes += SIZE_OF_GLYPH_INSTANCE_IN_BYTES;
	}
}

static void pushTileEntityTexture(Renderer *r, TextureHandle *textureHandle, float3 pos, float2 size, float4 color, float4 uv, RenderSortIndex sortIndex, u32 lightingMask = 0) {
	if(r->tileEntityRenderCount < MAX_TEXTURE_COUNT) {
		InstanceEntityData *data = r->tileRenderData + r->tileEntityRenderCount++;

		data->pos = pos;
		data->scale = size;
		data->color = color;
		data->uv = uv;
		data->sortIndex = sortIndex;
		data->textureHandle = textureHandle;
		data->aoMask = lightingMask;
	}
}

static void pushEntityTexture(Renderer *r, TextureHandle *textureHandle, float3 pos, float2 size, float4 color, float4 uv, RenderSortIndex sortIndex, u32 lightingMask = 0) {
	if(r->entityRenderCount < MAX_TEXTURE_COUNT) {
		InstanceEntityData *data = r->entityRenderData + r->entityRenderCount++;

		data->pos = pos;
		data->scale = size;
		data->color = color;
		data->uv = uv;
		data->sortIndex = sortIndex;
		data->textureHandle = textureHandle;
		data->aoMask = lightingMask;
	}
}


static void pushTexture(Renderer *r, TextureHandle *textureHandle, float3 pos, float2 size, float4 color, float4 uv, u32 lightingMask = 0) {
	RenderCommand *c = getRenderCommand(r, RENDER_TEXTURE);
	// assert(((TextureHandle *)textureHandle)->handle < 100);
	int textureIndex = render_getTextureIndex(c, textureHandle);

	// lightingMask = 0;

	if(textureIndex < 0) {
		render_endCommand(r);

		c = getRenderCommand(r, RENDER_TEXTURE);
		textureIndex = render_getTextureIndex(c, textureHandle);
		assert(textureIndex >= 0);
	}

	assert(c->type == RENDER_TEXTURE);

	if(r->textureCount < MAX_TEXTURE_COUNT) {
		InstanceData *data = (InstanceData *)(r->textureInstanceData + r->textureCount*SIZE_OF_TEXTURE_INSTANCE_IN_BYTES);

		data->pos = pos;
		data->scale = size;
		data->color = color;
		data->uv = uv;
		data->textureIndex = textureIndex;
		data->aoMask = lightingMask;

		r->textureCount++;
		c->instanceCount++;
		c->size_in_bytes += SIZE_OF_TEXTURE_INSTANCE_IN_BYTES;
	}
}

static void pushLine(Renderer *r, float3 posA, float3 posB, float4 color) {
	RenderCommand *c = getRenderCommand(r, RENDER_LINE);

	assert(c->type == RENDER_LINE);

	if(r->lineCount < MAX_LINE_COUNT) {
		float *data = (float *)(r->lineInstanceData + r->lineCount*SIZE_OF_LINE_INSTANCE_IN_BYTES);

		data[0] = posA.x;
		data[1] = posA.y;
		data[2] = posA.z;

		data[3] = posB.x;
		data[4] = posB.y;
		data[5] = posB.z;

		data[6] = color.x;
		data[7] = color.y;
		data[8] = color.z;
		data[9] = color.w;

		r->lineCount++;
		c->instanceCount++;
		c->size_in_bytes += SIZE_OF_LINE_INSTANCE_IN_BYTES;
	}
}

static void pushScissorsRect(Renderer *r, Rect2f scissors_bounds) {
	RenderCommand *c = getRenderCommand(r, RENDER_SET_SCISSORS);

	assert(c->type == RENDER_SET_SCISSORS);
	c->scissors_bounds = scissors_bounds;
}

static void pushBlendMode(Renderer *r, RenderBlendMode blendMode) {
	RenderCommand *c = getRenderCommand(r, RENDER_SET_BLEND_MODE);
	c->blendMode = blendMode;
	assert(c->type == RENDER_SET_BLEND_MODE);
}


static void renderer_defaultScissors(Renderer *r, float windowWidth, float windowHeight) {
	pushScissorsRect(r, make_rect2f(0, 0, windowWidth, windowHeight));
}

static void pushRect(Renderer *r, float3 pos, float2 size, float4 color) {
	pushTexture(r, global_white_texture, pos, size, color, make_float4(0, 0, 1, 1), 0);
}

static void pushRectOutlineWorldSpace(Renderer *r, float4 color) {
	float worldSpaceWidth = 0.05f;
	pushRect(r, make_float3(-0.5f, 0, 0), make_float2(worldSpaceWidth, 1), color);
	pushRect(r, make_float3(0.5f, 0, 0), make_float2(worldSpaceWidth, 1), color);
	pushRect(r, make_float3(0, -0.5f, 0), make_float2(1, worldSpaceWidth), color);
	pushRect(r, make_float3(0, 0.5f, 0), make_float2(1, worldSpaceWidth), color);
}


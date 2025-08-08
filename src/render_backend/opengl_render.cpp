#include <OpenGL/gl3.h>
#include "./opengl_shaders.cpp"

// NOTE: Each location index in a vertex attribute index - i.e. 4 floats. that's why for matrix we skip 4 values
#define VERTEX_ATTRIB_LOCATION 0
#define UV_ATTRIB_LOCATION 2
//INSTANCING LOCATIONS
#define POS_ATTRIB_LOCATION 3
#define UVATLAS_ATTRIB_LOCATION 4
#define COLOR_ATTRIB_LOCATION 5
#define SCALE_ATTRIB_LOCATION 6
#define SAMPLER_INDEX_ATTRIB_LOCATION 8
#define LIGHTING_MASK_ATTRIB_LOCATION 9

//INSTANCING LOCATIONS for line
#define POS1_ATTRIB_LOCATION 3
#define POS2_ATTRIB_LOCATION 4
#define COLOR1_ATTRIB_LOCATION 5

enum AttribInstancingType {
    ATTRIB_INSTANCE_TYPE_DEFAULT,
    ATTRIB_INSTANCE_TYPE_LINE,
};

struct Shader {
    bool valid;
    uint32_t handle;
};

struct Vertex {
    float3 pos;
    float2 texUV;
};

Vertex makeVertex(float3 pos, float2 texUV) {
    Vertex v = {};
    
    v.pos = pos;
    v.texUV = texUV;

    return v;
}

static Vertex global_quadData[] = {
    makeVertex(make_float3(0.5f, -0.5f, 0), make_float2(1, 1)),
    makeVertex(make_float3(-0.5f, -0.5f, 0), make_float2(0, 1)),
    makeVertex(make_float3(-0.5f,  0.5f, 0), make_float2(0, 0)),
    makeVertex(make_float3(0.5f, 0.5f, 0), make_float2(1, 0)),
};

static unsigned int global_quadIndices[] = {
    0, 1, 2, 0, 2, 3,
};

static Vertex global_lineData[] = {
    makeVertex(make_float3(1, 0, 0), make_float2(0, 0)),
    makeVertex(make_float3(0, 1, 0), make_float2(1, 1)),
};

static unsigned int global_lineIndices[] = {
    0, 1
};

#define renderCheckError() renderCheckError_(__LINE__, (char *)__FILE__)
void renderCheckError_(int lineNumber, char *fileName) {
    #define RENDER_CHECK_ERRORS 1
    #if RENDER_CHECK_ERRORS
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        assert(!err);
    }
    #endif
}

struct ModelBuffer {
    uint32_t handle;
    uint32_t instanceBufferhandle;
    int indexCount;
};

Shader sdfFontShader;
Shader textureShader;
Shader rectOutlineShader;
Shader lineShader;
Shader pixelArtShader;
Shader terrainLightingShader;
Shader cloudShader;


struct BackendRenderer {
    SDL_GLContext renderContext;
    SDL_Window *window_hwnd;

    ModelBuffer quadModel;
    ModelBuffer lineModel;

    float16 orthoMatrix;
    Shader *currentShader;
};

static TextureHandle *Platform_loadTextureToGPU(void *data, u32 texWidth, u32 texHeight, u32 bytesPerPixel) {
    assert(bytesPerPixel == 4);
    GLuint resultId;
    glGenTextures(1, &resultId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, resultId);
    renderCheckError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    renderCheckError();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    renderCheckError();

    glGenerateMipmap(GL_TEXTURE_2D);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();

    TextureHandle *handle = (TextureHandle *)platform_alloc_memory(sizeof(TextureHandle), true);
    handle->handle = resultId;

    return handle;
}

void generateMipMapsForTexture(TextureHandle *handle) {
    glBindTexture(GL_TEXTURE_2D, handle->handle);
    renderCheckError();

    glGenerateMipmap(GL_TEXTURE_2D);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();
}

struct FramebufferHandle {
    u32 width;
    u32 height;
    GLuint framebuffer;
    TextureHandle *textureHandle;
};

static FramebufferHandle platform_createFramebuffer(u32 width, u32 height) {
    GLuint framebufferId, textureId;
    
    // Generate framebuffer
    glGenFramebuffers(1, &framebufferId);
    renderCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    renderCheckError();

    // Create texture to attach to framebuffer
    glGenTextures(1, &textureId);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, textureId);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    renderCheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    renderCheckError();

    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    renderCheckError();

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, 0);

    // Allocate and return framebuffer handle
    FramebufferHandle fboHandle = {};
    TextureHandle *texHandle = (TextureHandle *)platform_alloc_memory(sizeof(TextureHandle), true);
    texHandle->handle = textureId;
    
    fboHandle.framebuffer = framebufferId;
    fboHandle.textureHandle = texHandle;
    fboHandle.width = width;
    fboHandle.height = height;

    return fboHandle;
}

static Texture platform_loadFromFileToGPU(char *image_to_load_utf8) {
	Texture result = {};

	// Load Image
	int texWidth;
	int texHeight;
	unsigned char *testTextureBytes = (unsigned char *)stbi_load(image_to_load_utf8, &texWidth, &texHeight, 0, STBI_rgb_alpha);
	int texBytesPerRow = 4 * texWidth;


	if(!testTextureBytes) {
        printf("Couldn't load %s\n", image_to_load_utf8);
        assert(testTextureBytes);
    }

	result.handle = Platform_loadTextureToGPU(testTextureBytes, texWidth, texHeight, 4);

	free(testTextureBytes);

	result.width = texWidth;
	result.height = texHeight;

	result.aspectRatio_h_over_w = (float)texHeight / (float)texWidth;

	result.uvCoords = make_float4(0, 0, 1, 1);

	return result;
}

Texture backendRenderer_loadFromFileToGPU(BackendRenderer *backendRenderer, char *image_to_load_utf8) {
	Texture result = platform_loadFromFileToGPU(image_to_load_utf8);
	return result;
}


Shader loadShader(char *vertexShader, char *fragShader, AttribInstancingType attribType) {
    Shader result = {};
    
    result.valid = true;
    
    GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    renderCheckError();
    GLuint fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    renderCheckError();
    
    glShaderSource(vertexShaderHandle, 1, (const GLchar **)(&vertexShader), 0);
    renderCheckError();
    glShaderSource(fragShaderHandle, 1, (const GLchar **)(&fragShader), 0);
    renderCheckError();
    
    glCompileShader(vertexShaderHandle);
    renderCheckError();
    glCompileShader(fragShaderHandle);
    renderCheckError();
    result.handle = glCreateProgram();
    renderCheckError();
    glAttachShader(result.handle, vertexShaderHandle);
    renderCheckError();
    glAttachShader(result.handle, fragShaderHandle);
    renderCheckError();

    int max_attribs;
    glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &max_attribs);

    glBindAttribLocation(result.handle, VERTEX_ATTRIB_LOCATION, "vertex");
    renderCheckError();
    // glBindAttribLocation(result.handle, NORMAL_ATTRIB_LOCATION, "normal");
    // renderCheckError();
    glBindAttribLocation(result.handle, UV_ATTRIB_LOCATION, "texUV");
    renderCheckError();

    if(attribType == ATTRIB_INSTANCE_TYPE_DEFAULT) {
        glBindAttribLocation(result.handle, POS_ATTRIB_LOCATION, "pos");
        renderCheckError();
        glBindAttribLocation(result.handle, UVATLAS_ATTRIB_LOCATION, "uvAtlas");
        renderCheckError();
        glBindAttribLocation(result.handle, COLOR_ATTRIB_LOCATION, "color");
        renderCheckError();
        glBindAttribLocation(result.handle, SCALE_ATTRIB_LOCATION, "scale");
        renderCheckError();
        glBindAttribLocation(result.handle, SAMPLER_INDEX_ATTRIB_LOCATION, "samplerIndex");
        renderCheckError();
        glBindAttribLocation(result.handle, LIGHTING_MASK_ATTRIB_LOCATION, "aoMask");
        renderCheckError();
    } else if(attribType == ATTRIB_INSTANCE_TYPE_LINE) {
        glBindAttribLocation(result.handle, POS1_ATTRIB_LOCATION, "pos1");
        renderCheckError();
        glBindAttribLocation(result.handle, POS2_ATTRIB_LOCATION, "pos2");
        renderCheckError();
        glBindAttribLocation(result.handle, COLOR1_ATTRIB_LOCATION, "color1");
        renderCheckError();
    } else {
        assert(false);
    }

    glLinkProgram(result.handle);
    renderCheckError();
    glUseProgram(result.handle);
    
    GLint success = 0;
    glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);
    
    GLint success1 = 0;
    glGetShaderiv(fragShaderHandle, GL_COMPILE_STATUS, &success1); 
    
    if(success == GL_FALSE || success1 == GL_FALSE) {
        result.valid = false;
        int  vlength,    flength,    plength;
        char vlog[2048];
        char flog[2048];
        char plog[2048];
        glGetShaderInfoLog(vertexShaderHandle, 2048, &vlength, vlog);
        glGetShaderInfoLog(fragShaderHandle, 2048, &flength, flog);
        glGetProgramInfoLog(result.handle, 2048, &plength, plog);
        
        if(vlength || flength || plength) {
            printf("%s\n", vertexShader);
            printf("%s\n", fragShader);
            printf("%s\n", vlog);
            printf("%s\n", flog);
            printf("%s\n", plog);
            
        }
    }
    
    assert(result.valid);
    assert(result.handle);

    return result;
}

static void backendRender_release_and_resize_default_frame_buffer(BackendRenderer *r) {
    //NOTE: The default framebuffer contains a number of images, based on how it was created. All default framebuffer images are automatically resized to the size of the output window, as it is resized.
}

static inline void addInstanceAttribForMatrix(int index, GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    glEnableVertexAttribArray(attribLoc + index);  
    renderCheckError();
    
    glVertexAttribPointer(attribLoc + index, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct + (4*sizeof(float)*index));
    renderCheckError();
    glVertexAttribDivisor(attribLoc + index, 1);
    renderCheckError();
}

static inline void addInstancingAttrib (GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    assert(offsetForStruct > 0);
    if(numOfFloats == 16) {
        addInstanceAttribForMatrix(0, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(1, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(2, attribLoc, 4, offsetForStruct, offsetInStruct);
        addInstanceAttribForMatrix(3, attribLoc, 4, offsetForStruct, offsetInStruct);
    } else {
        glEnableVertexAttribArray(attribLoc);  
        renderCheckError();
        
        assert(numOfFloats <= 4);
        glVertexAttribPointer(attribLoc, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct);
        renderCheckError();
        
        glVertexAttribDivisor(attribLoc, 1);
        renderCheckError();
    }
}

void addInstancingAttrib_int32(GLuint attribLoc, int numOfInt32s, size_t offsetForStruct, size_t offsetInStruct) {
    glEnableVertexAttribArray(attribLoc);  
    renderCheckError();
    
    glVertexAttribIPointer(attribLoc, numOfInt32s, GL_UNSIGNED_INT, offsetForStruct, ((char *)0) + offsetInStruct);
    renderCheckError();

    glVertexAttribDivisor(attribLoc, 1);
    renderCheckError();
}

void addInstancingAttribsForShader(AttribInstancingType type) {
    if(type == ATTRIB_INSTANCE_TYPE_DEFAULT) {
        
        size_t offsetForStruct = sizeof(InstanceData); 
        
        unsigned int posOffset = (intptr_t)(&(((InstanceData *)0)->pos));
        addInstancingAttrib (POS_ATTRIB_LOCATION, 3, offsetForStruct, posOffset);
        renderCheckError();

        unsigned int uvOffset = (intptr_t)(&(((InstanceData *)0)->uv));
        addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 4, offsetForStruct, uvOffset);
        renderCheckError();

        unsigned int colorOffset = (intptr_t)(&(((InstanceData *)0)->color));
        addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
        renderCheckError();

        unsigned int scaleOffset = (intptr_t)(&(((InstanceData *)0)->scale));
        addInstancingAttrib (SCALE_ATTRIB_LOCATION, 2, offsetForStruct, scaleOffset);
        renderCheckError();

        unsigned int samplerIndexOffset = (intptr_t)(&(((InstanceData *)0)->textureIndex));
        addInstancingAttrib(SAMPLER_INDEX_ATTRIB_LOCATION, 1, offsetForStruct, samplerIndexOffset);
        renderCheckError();
        
        unsigned int aoMaskOffset = (intptr_t)(&(((InstanceData *)0)->aoMask));
        addInstancingAttrib_int32(LIGHTING_MASK_ATTRIB_LOCATION, 1, offsetForStruct, aoMaskOffset);
        renderCheckError();
    } else if(type == ATTRIB_INSTANCE_TYPE_LINE) {
        size_t offsetForStruct = sizeof(InstanceDataLine); 

        unsigned int pos1Offset = (intptr_t)(&(((InstanceDataLine *)0)->pos1));
        addInstancingAttrib (POS1_ATTRIB_LOCATION, 3, offsetForStruct, pos1Offset);
        renderCheckError();
        unsigned int pos2Offset = (intptr_t)(&(((InstanceDataLine *)0)->pos2));
        addInstancingAttrib (POS2_ATTRIB_LOCATION, 3, offsetForStruct, pos2Offset);
        renderCheckError();
        unsigned int colorOffset = (intptr_t)(&(((InstanceDataLine *)0)->color));
        addInstancingAttrib (COLOR1_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
        renderCheckError();
    } else {
        assert(false);
    }
}

ModelBuffer generateVertexBuffer(Vertex *triangleData, int vertexCount, unsigned int *indicesData, int indexCount, AttribInstancingType attribInstancingType) {
    ModelBuffer result = {};
    glGenVertexArrays(1, &result.handle);
    renderCheckError();
    glBindVertexArray(result.handle);
    renderCheckError();
    
    GLuint vertices;
    GLuint indices;
    
    glGenBuffers(1, &vertices);
    renderCheckError();
    
    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    renderCheckError();
    
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeof(Vertex), triangleData, GL_STATIC_DRAW);
    renderCheckError();
    
    glGenBuffers(1, &indices);
    renderCheckError();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    renderCheckError();
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
    renderCheckError();
    
    result.indexCount = indexCount;
    
    //NOTE: Assign the attribute locations with the data offsets & types
    GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
    renderCheckError();
    glEnableVertexAttribArray(vertexAttrib);  
    renderCheckError();
    glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    renderCheckError();
    
    GLint texUVAttrib = UV_ATTRIB_LOCATION;
    glEnableVertexAttribArray(texUVAttrib);  
    renderCheckError();
    unsigned int uvByteOffset = (intptr_t)(&(((Vertex *)0)->texUV));
    glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + uvByteOffset);
    renderCheckError();

    // GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
    // glEnableVertexAttribArray(normalsAttrib);  
    // renderCheckError();
    // unsigned int normalOffset = (intptr_t)(&(((Vertex *)0)->normal));
    // glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + normalOffset);
    // renderCheckError();

    // vbo instance buffer
    {
        glGenBuffers(1, &result.instanceBufferhandle);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, result.instanceBufferhandle);
        renderCheckError();

        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_DYNAMIC_DRAW);
        
        addInstancingAttribsForShader(attribInstancingType);
    }
    
    glBindVertexArray(0);
        
    //we can delete these buffers since they are still referenced by the VAO 
    glDeleteBuffers(1, &vertices);
    glDeleteBuffers(1, &indices);

    return result;
}

static uint backendRender_init(BackendRenderer *r, SDL_Window *hwnd) {
    r->window_hwnd = hwnd;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    r->renderContext = SDL_GL_CreateContext(hwnd);

    if(r->renderContext) {
        if(SDL_GL_MakeCurrent(hwnd, r->renderContext) == 0) {
            
            if(SDL_GL_SetSwapInterval(1) == 0) {
                // Success
            } else {
                printf("Couldn't set swap interval\n");
            }
        } else {
            printf("Couldn't make context current\n");
        }
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    sdfFontShader = loadShader(quadVertexShader, sdfFragShader, ATTRIB_INSTANCE_TYPE_DEFAULT);
    textureShader = loadShader(quadVertexShader, quadTextureFragShader, ATTRIB_INSTANCE_TYPE_DEFAULT);
    rectOutlineShader = loadShader(rectOutlineVertexShader, rectOutlineFragShader, ATTRIB_INSTANCE_TYPE_DEFAULT);
    pixelArtShader = loadShader(quadVertexShader, pixelArtFragShader, ATTRIB_INSTANCE_TYPE_DEFAULT);
    lineShader = loadShader(lineVertexShader, lineFragShader, ATTRIB_INSTANCE_TYPE_LINE);
    terrainLightingShader = loadShader(quadAoMaskVertexShader, pixelArtAoMaskFragShader, ATTRIB_INSTANCE_TYPE_DEFAULT);
    cloudShader = loadShader(quadVertexShader, fragCloudShader, ATTRIB_INSTANCE_TYPE_DEFAULT);
    
    r->quadModel = generateVertexBuffer(global_quadData, 4, global_quadIndices, 6, ATTRIB_INSTANCE_TYPE_DEFAULT);
    r->lineModel = generateVertexBuffer(global_lineData, 2, global_lineIndices, 2, ATTRIB_INSTANCE_TYPE_LINE);

#if DEBUG_BUILD
	if(!global_white_texture) {
		global_white_texture = platform_loadFromFileToGPU("../images/white_texture.png").handle;
	}
#else 
	if(!global_white_texture) {
		global_white_texture = platform_loadFromFileToGPU("../white_texture.png").handle;
	}
#endif

    return 0;
}

GLuint OpenglGetTextureHandle(Texture *texture) {
    assert(texture);
    GLuint result = (GLuint)((uint64_t)(texture->handle));
    return result;
}

Texture loadCubeMapTextureToGPU(char *folderName) {
    Texture result = {};

    result.handle = (TextureHandle *)platform_alloc_memory(sizeof(TextureHandle), true);

    glGenTextures(1, &result.handle->handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, result.handle->handle);

    char *faces[] = {
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "front.jpg",
    "back.jpg"};
    
    int width, height, nrChannels;
    for (unsigned int i = 0; i < arrayCount(faces); i++)
    {
        size_t bytesToAlloc = snprintf(NULL, 0, "%s%s", folderName, faces[i]);
        char *concatFileName = (char *)malloc(bytesToAlloc + 1);
        snprintf(concatFileName, bytesToAlloc + 1, "%s%s", folderName, faces[i]);
        unsigned char *data = stbi_load(concatFileName, &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        } else {
            printf("Cubemap tex failed to load\n");
            stbi_image_free(data);
        }
        free(concatFileName);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return result;
}

void updateInstanceData(uint32_t bufferHandle, void *data, size_t sizeInBytes) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandle);
    renderCheckError();
    
    //send the data to GPU. glBufferData deletes the old one
    //NOTE(ollie): We were using glBufferData which deletes the old buffer and resends the create a new buffer, but 
    //NOTE(ollie): I saw on Dungeoneer code using glsubbufferdata is faster because it doesn't have to delete it.
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeInBytes, data);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_DYNAMIC_DRAW); 
    renderCheckError();

    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    renderCheckError();
}

enum ShaderFlags {
    SHADER_CUBE_MAP = 1 << 0,
};

void bindTexture(char *uniformName, int slotId, GLint textureId, Shader *shader) {
    GLint texUniform = glGetUniformLocation(shader->handle, uniformName); 
    renderCheckError();
    
    glUniform1i(texUniform, slotId);
    renderCheckError();
    
    glActiveTexture(GL_TEXTURE0 + slotId);
    renderCheckError();
  
    glBindTexture(GL_TEXTURE_2D, textureId); 
    renderCheckError();
}


void bindTextureArray(char *uniformName, GLint textureId, Shader *shader, uint32_t flags) {

}

void drawModels(Renderer *r_, BackendRenderer *r, ModelBuffer *model, uint32_t textureId, int instanceCount, GLenum toplogyType) {
    // printf("%d\n", model->indexCount);
    Shader *shader = r->currentShader;
    assert(shader);
    assert(shader->handle);
    
    glBindVertexArray(model->handle);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "projection"), 1, GL_FALSE, r->orthoMatrix.E);
    renderCheckError();

    glUniform1f(glGetUniformLocation(shader->handle, "totalTime"), r_->totalTime);
    renderCheckError();

    bindTexture("diffuse", 1, textureId, shader);
    renderCheckError();

    glDrawElementsInstanced(toplogyType, model->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
    renderCheckError();

    glBindVertexArray(0);
    renderCheckError();    
}


static void backendRender_processCommandBuffer(Renderer *r, BackendRenderer *backend_r, float dt) {
    r->totalTime += dt;
    DEBUG_TIME_BLOCK();
#if DEBUG_BUILD
	global_debug_stats.draw_call_count = 0;
    global_debug_stats.render_command_count = r->commandCount;
#endif

	for(int i = 0; i < r->commandCount; ++i) {
		RenderCommand *c = r->commands + i;

		switch(c->type) {
			case RENDER_NULL: {
				assert(false);
			} break;
            case RENDER_SET_BLEND_MODE: {
                if(c->blendMode == RENDER_BLEND_MODE_ADD) {
                    glBlendFunc(GL_ONE, GL_ONE);  
                } else if(c->blendMode == RENDER_BLEND_MODE_DEFAULT) {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
                }
                
            } break;
			case RENDER_SET_VIEWPORT: {
				glViewport(c->viewport.x, c->viewport.y, c->viewport.z, c->viewport.w);
			} break;
			case RENDER_CLEAR_COLOR_BUFFER: {
                glClearColor(c->color.x, c->color.y, c->color.z, c->color.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
			} break;
            case RENDER_SET_FRAME_BUFFER: {
                glBindFramebuffer(GL_FRAMEBUFFER, c->frameId);
			} break;
			case RENDER_MATRIX: {
				backend_r->orthoMatrix = c->matrix;
			} break;
			case RENDER_SET_SHADER: {
				Shader *program = (Shader *)c->shader;

                glUseProgram(program->handle);
                renderCheckError();

                backend_r->currentShader = program;
                assert(program->handle);
			} break;
			case RENDER_SET_SCISSORS: {
                float w = c->scissors_bounds.maxX - c->scissors_bounds.minX;
                float h = c->scissors_bounds.maxY - c->scissors_bounds.minY;

                glScissor((GLint)c->scissors_bounds.minX, (GLint)c->scissors_bounds.minY, (GLint)w, (GLint)h);
			} break;

			case RENDER_LINE: {
				u8 *data = r->lineInstanceData + c->offset_in_bytes;
				int sizeInBytes = c->size_in_bytes;

                assert(sizeInBytes > 0);
                assert(data);

                updateInstanceData(backend_r->lineModel.instanceBufferhandle, data, sizeInBytes);
				drawModels(r, backend_r, &backend_r->lineModel, 0, c->instanceCount, GL_LINES);

				#if DEBUG_BUILD
				    global_debug_stats.draw_call_count++;
				#endif

			} break;
            case RENDER_GLYPH: 
			case RENDER_TEXTURE: {
                //TODO: I think the glyph and texture case are basically the same. So look into removing glyph render command
                u8 *data = 0;
                int sizeInBytes = 0;

                if(c->type == RENDER_GLYPH) {
                    data = r->glyphInstanceData + c->offset_in_bytes;
				    sizeInBytes = c->size_in_bytes;
                } else {
                    data = r->textureInstanceData + c->offset_in_bytes;
				    sizeInBytes = c->size_in_bytes;
                }
				
				assert(c->textureHandle_count > 0);

                
				//NOTE: We just use the first texture handle
				TextureHandle *textureHandle = (TextureHandle *)c->texture_handles[0];
                assert(textureHandle);
                assert(sizeInBytes > 0);
                assert(data);

                updateInstanceData(backend_r->quadModel.instanceBufferhandle, data, sizeInBytes);

				drawModels(r, backend_r, &backend_r->quadModel, textureHandle->handle, c->instanceCount, GL_TRIANGLES);

				#if DEBUG_BUILD
				    global_debug_stats.draw_call_count++;
				#endif
			} break;
			default: {

			}
		}
	}
}


static void backendRender_presentFrame(BackendRenderer *r) {
	SDL_GL_SwapWindow(r->window_hwnd);
}
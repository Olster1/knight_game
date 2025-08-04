
static void loadImageStrip(Animation *animation, BackendRenderer *backendRenderer, char *filename_full_utf8, int widthPerImage) {
    if(doesFileExists(filename_full_utf8)) {
        Texture texOnStack = backendRenderer_loadFromFileToGPU(backendRenderer, filename_full_utf8);
        int count = 0;

        float xAt = 0;

        float widthTruncated = ((int)(texOnStack.width / widthPerImage))*widthPerImage;
        while(xAt < widthTruncated) {
            Texture *tex = pushStruct(&global_long_term_arena, Texture);
            easyPlatform_copyMemory(tex, &texOnStack, sizeof(Texture));

            tex->uvCoords.x = xAt / texOnStack.width;

            xAt += widthPerImage;

            tex->uvCoords.z = xAt / texOnStack.width;

            tex->aspectRatio_h_over_w = ((float)texOnStack.height) / ((float)(tex->uvCoords.z - tex->uvCoords.x)*(float)texOnStack.width);

            easyAnimation_pushFrame(animation, tex);

            count++;
        }
    }
}
static void loadImageStripFromAtlas(Animation *animation, BackendRenderer *backendRenderer, TextureAtlas *atlas, AtlasAsset *asset, int widthPerImage) {
    assert(atlas);
    assert(asset);

    Texture texOnStack = atlas->texture;
    texOnStack.uvCoords = asset->uv;
    texOnStack.aspectRatio_h_over_w = asset->aspectRatio_h_over_w;
    texOnStack.width = atlas->texture.width * (asset->uv.z - asset->uv.x);
    texOnStack.height = atlas->texture.height * (asset->uv.w - asset->uv.y);

    int count = 0;
    float xAt = 0;
    float startX = atlas->texture.width * asset->uv.x;
    float startY = atlas->texture.height * asset->uv.y;

    float widthTruncated = ((int)(texOnStack.width / widthPerImage))*widthPerImage;
    while(xAt < widthTruncated) {
        Texture *tex = pushStruct(&global_long_term_arena, Texture);
        easyPlatform_copyMemory(tex, &texOnStack, sizeof(Texture));

        tex->uvCoords.x = (xAt + startX) / atlas->texture.width;
        tex->uvCoords.z = ((xAt + startX)  + widthPerImage) / atlas->texture.width;
        tex->aspectRatio_h_over_w = ((float)(tex->uvCoords.w - tex->uvCoords.y)*(float)texOnStack.height) / ((float)(tex->uvCoords.z - tex->uvCoords.x)*(float)texOnStack.width);

        easyAnimation_pushFrame(animation, tex);

        xAt += widthPerImage;
        count++;
    }
}


static void loadImageStripXY(Animation *animation, BackendRenderer *backendRenderer, char *filename_full_utf8, int widthPerImage, int heightPerImage, int numberOfSprites, int offsetX, int offsetY) {
	Texture texOnStack = backendRenderer_loadFromFileToGPU(backendRenderer, filename_full_utf8);
	int count = 0;
	float xAt = offsetX*widthPerImage;
    float yAt = heightPerImage*offsetY;

	float widthTruncated = ((int)(texOnStack.width / widthPerImage))*widthPerImage;

    for(int i = 0; i < numberOfSprites; i++) {
        Texture *tex = pushStruct(&global_long_term_arena, Texture);
        easyPlatform_copyMemory(tex, &texOnStack, sizeof(Texture));

        tex->uvCoords.x = xAt / texOnStack.width;
        tex->uvCoords.y = yAt / texOnStack.height;

        xAt += widthPerImage;

        tex->uvCoords.z = xAt / texOnStack.width;
        tex->uvCoords.w = (yAt + heightPerImage) / texOnStack.height;

        tex->aspectRatio_h_over_w = ((float)(tex->uvCoords.w - tex->uvCoords.y)*(float)texOnStack.height) / ((float)(tex->uvCoords.z - tex->uvCoords.x)*(float)texOnStack.width);

        easyAnimation_pushFrame(animation, tex);

        if(xAt >= widthTruncated) {
            xAt = 0;
            yAt += heightPerImage;
        }
    }
}



Texture ** loadTileSet(BackendRenderer *backendRenderer, char *filename_full_utf8, int widthPerImage, int heightPerImage, Memory_Arena *arena, int *finalCount, int *countX, int *countY) {
	Texture texOnStack = backendRenderer_loadFromFileToGPU(backendRenderer, filename_full_utf8);
    
	int count = 0;

	float xAt = 0;
    float yAt = 0;

    int bestCountX = 0;

    int maxTileCount = 128;
    Texture **tileSet = pushArray(arena, maxTileCount, Texture *);

	float widthTruncated = ((int)(texOnStack.width / widthPerImage))*widthPerImage;
    float heightTruncated = ((int)(texOnStack.height / heightPerImage))*heightPerImage;
    while(yAt < heightTruncated) {
        while(xAt < widthTruncated) {
            Texture *tex = pushStruct(&global_long_term_arena, Texture);
            easyPlatform_copyMemory(tex, &texOnStack, sizeof(Texture));

            tex->uvCoords.x = xAt / texOnStack.width;
            tex->uvCoords.y = yAt / texOnStack.height;

            xAt += widthPerImage;

            tex->uvCoords.z = xAt / texOnStack.width;
            tex->uvCoords.w = (yAt + heightPerImage) / texOnStack.height;

            tex->aspectRatio_h_over_w = ((float)(tex->uvCoords.w - tex->uvCoords.y)*(float)texOnStack.height) / ((float)(tex->uvCoords.z - tex->uvCoords.x)*(float)texOnStack.width);
            
            assert(count < maxTileCount);

            tileSet[count] = tex;

            count++;

            (*countX)++;
        }
        yAt += heightPerImage;
        xAt = 0;
        if(*countX > bestCountX) {
            bestCountX = *countX;
        }
        *countX = 0;

        (*countY)++;
    }

    *countX = bestCountX;

    *finalCount = count; 
    
    return tileSet;
}
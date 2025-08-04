#include <dirent.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"

struct AtlasAsset {
    char *name;
    float4 uv;
    float aspectRatio_h_over_w;

    AtlasAsset *next;
};

struct TextureAtlas {
    Texture texture;
    
    //NOTE: Hash table of assets
    AtlasAsset *items[4096];
};


    AtlasAsset *textureAtlas_addItem(TextureAtlas *atlas, char *name, float4 uv) {
        uint32_t hash = get_crc32_for_string(name);

        hash %= arrayCount(atlas->items);
        assert(hash < arrayCount(atlas->items));

        AtlasAsset **aPtr = &atlas->items[hash];

        while(*aPtr) {
            aPtr = &((*aPtr)->next);
        }

        assert((*aPtr) == 0);

        AtlasAsset *a = pushStruct(&global_long_term_arena, AtlasAsset);

        a->name = name;
        a->uv = uv;
        a->aspectRatio_h_over_w = safeDivide(uv.w - uv.y, uv.z - uv.x);

        a->next = 0;

        *aPtr = a;

        return a;
    }

    AtlasAsset *textureAtlas_getItem(TextureAtlas *atlas, char *name) {
        AtlasAsset *result = 0;

        uint32_t hash = get_crc32_for_string(name);

        uint32_t hashIndex = hash % arrayCount(atlas->items);
        assert(hashIndex < arrayCount(atlas->items));

        AtlasAsset *a = atlas->items[hashIndex];

        while(a && !result) {
            uint32_t hashText = get_crc32_for_string(a->name);
            if(hashText == hash && easyString_stringsMatch_nullTerminated(a->name, name)) {
                result = a;
            }

            a = a->next;
        }

        return result;
    }


    Texture textureAtlas_getItemAsTexture(TextureAtlas *atlas, char *name) {
        Texture t = {};
        AtlasAsset *i = textureAtlas_getItem(atlas, name);
        assert(i);
        if(i) {
            //NOTE: Fill out the texture details
            t.handle = atlas->texture.handle;
            float wPercent = (i->uv.z - i->uv.x);
            float hPercent = (i->uv.w - i->uv.y);
            t.width = atlas->texture.width*wPercent;
            t.height = atlas->texture.height*hPercent;
            t.aspectRatio_h_over_w = i->aspectRatio_h_over_w;
            t.uvCoords = i->uv;
        }
        return t;
    }

    TextureAtlas readTextureAtlas(char *jsonFileName, char *textureFileName) {
        TextureAtlas result = {};

        void *memory = 0;
        size_t data_size = 0;

        bool valid  = Platform_LoadEntireFile_utf8(jsonFileName, &memory, &data_size);
        assert(valid);
        assert(data_size > 0);
        assert(memory);

        EasyTokenizer tokenizer = lexBeginParsing(memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

        bool parsing = true;
        while(parsing) {
            EasyToken t = lexGetNextToken(&tokenizer);

            if(t.type == TOKEN_NULL_TERMINATOR) {
                parsing = false;
            } else if(t.type == TOKEN_OPEN_BRACKET) {
                //NOTE: Get the item out
                float4 uv = make_float4(0, 0, 0, 0);

                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                char *assetName = nullTerminateArena(t.at, t.size, &global_long_term_arena);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COMMA);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.x = t.floatVal;
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.y = t.floatVal;
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.z = t.floatVal;
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_FLOAT);
                uv.w = t.floatVal;

                textureAtlas_addItem(&result, assetName, uv);
            }
        }

        result.texture = platform_loadFromFileToGPU(textureFileName);

        return result;
    }

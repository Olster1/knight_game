
typedef struct {
    TextureHandle *handle;

    float4 uvCoords;
    u32 unicodePoint;

    int xoffset;
    int yoffset;

    int xadvance;  

    int width;
    int height;

    bool hasTexture;

    u8 *sdfBitmap;

} GlyphInfo;

#define MY_MAX_GLYPH_COUNT 256

typedef struct FontSheet FontSheet;
typedef struct FontSheet {
    TextureHandle *handle;

    u32 glyphCount;
    GlyphInfo glyphs[MY_MAX_GLYPH_COUNT];

    u32 minText;
    u32 maxText;

    FontSheet *next;

} FontSheet;

typedef struct {
    float originalScaleFactor;
    stbtt_fontinfo *fontInfo;

    char *fileName;
    int fontHeight;
    FontSheet *sheets;
} Font;


FontSheet *createFontSheet(Font *font, u32 firstChar, u32 endChar) {

    //NOTE: This could go in permanent memory arena
    FontSheet *sheet = (FontSheet *)platform_alloc_memory(sizeof(FontSheet), true);
    sheet->minText = firstChar;
    sheet->maxText = endChar;
    sheet->next = 0;
    sheet->glyphCount = 0;
    
    
    //NOTE(ollie): Get the 'scale' for the max pixel height 
    float maxHeightForFontInPixels = 32;//pixels
    float scale = stbtt_ScaleForPixelHeight(font->fontInfo, maxHeightForFontInPixels);

    font->originalScaleFactor = scale;
    
    //NOTE(ollie): Scale the padding around the glyph proportional to the size of the glyph
    s32 padding = (s32)(maxHeightForFontInPixels / 3);
    //NOTE(ollie): The distance from the glyph center that determines the edge (i.e. all the 'in' pixels)
    u8 onedge_value = (u8)(0.8f*255); 
    //NOTE(ollie): The rate at which the distance from the center should increase
    float pixel_dist_scale = (float)onedge_value/(float)padding;

    font->fontHeight = maxHeightForFontInPixels;

    u32 maxWidth = 0;
    u32 maxHeight = 0;
    u32 totalWidth = 0;
    u32 totalHeight = 0;

    u32 counAt = 0;
    u32 rowCount = 1;
   
    for(s32 codeIndex = firstChar; codeIndex < endChar; ++codeIndex) {
        
        int width;
        int height;
        int xoffset; 
        int yoffset;

        assert((endChar - firstChar) <= 256);
        
        u8 *data = stbtt_GetCodepointSDF(font->fontInfo, scale, codeIndex, padding, onedge_value, pixel_dist_scale, &width, &height, &xoffset, &yoffset);    
            
        assert(sheet->glyphCount < MY_MAX_GLYPH_COUNT);
        GlyphInfo *info = &sheet->glyphs[sheet->glyphCount++];

        memset(info, 0, sizeof(GlyphInfo));

        info->unicodePoint = codeIndex;

        info->sdfBitmap = data;

        if(codeIndex == 97) {
            int h = 0;
        }

        int advance, lsb;
        stbtt_GetCodepointHMetrics(font->fontInfo, codeIndex, &advance, &lsb);
        info->xadvance = advance*scale;
        info->xoffset = xoffset;
        info->yoffset = yoffset;
        info->hasTexture = data;

        info->width = width;
        info->height = height;
        if(data) {
            { 
                totalWidth += width;
                counAt++;

                if(height > maxHeight) {
                    maxHeight = height;
                }
            } 

            if((counAt % 16) == 0) {
                rowCount++;
                counAt = 0;
                if(totalWidth > maxWidth) { maxWidth = totalWidth; }
                totalWidth = 0;
            }
        }
    }

        
    totalWidth = maxWidth;
    totalHeight = maxHeight*rowCount;
    u32 *sdfBitmap_32 = (u32 *)platform_alloc_memory(totalWidth*totalHeight*sizeof(u32), true);

    u32 xAt = 0;
    u32 yAt = 0;

    u32 countAt = 0;

    for(int i = 0; i < sheet->glyphCount; ++i) {
        GlyphInfo *info = &sheet->glyphs[i];

        if(info->sdfBitmap) {

            //NOTE: Calculate uv coords
            info->uvCoords = make_float4((float)xAt / (float)totalWidth, (float)yAt / (float)totalHeight, (float)(xAt + info->width) / (float)totalWidth, (float)(yAt + info->height) / (float)totalHeight);

            //NOTE(ollie): Blow out bitmap to 32bits per pixel instead of 8 so it's easier to load to the GPU
            for(int y = 0; y < info->height; ++y) {
                for(int x = 0; x < info->width; ++x) {
                    u32 stride = info->width*1;
                    u32 alpha = (u32)info->sdfBitmap[y*stride + x];
                    sdfBitmap_32[(y + yAt)*totalWidth + (x + xAt)] = 0x00000000 | (u32)((alpha) << 24);
                }
            }

            xAt += info->width;
            countAt++;

            if((countAt % 16) == 0) {
                yAt += maxHeight;
                xAt = 0;
            }

            assert(xAt < totalWidth);
            assert(yAt < totalHeight);


            //NOTE(ollie): Free the bitmap data
            stbtt_FreeSDF(info->sdfBitmap, 0);
        }
    }

    //NOTE Test purposes
    // for(int y = 0; y < totalHeight; ++y) {
    //     for(int x = 0; x < totalWidth; ++x) {
    //         sdfBitmap_32[y*totalWidth + x] = 0xFFF0FFF0;// | (u32)(((u32)alpha) << 24);
    //     }
    // }
    
    //NOTE(ollie): Load the texture to the GPU
    sheet->handle = Platform_loadTextureToGPU(sdfBitmap_32, totalWidth, totalHeight, 4);

    //NOTE(ollie): Release memory from 32bit bitmap
    platform_free_memory(sdfBitmap_32);

    return sheet;
}

FontSheet *findFontSheet(Font *font, unsigned int character) {

    FontSheet *sheet = font->sheets;
    FontSheet *result = 0;
    while(sheet) {
        if(character >= sheet->minText && character < sheet->maxText) {
            result = sheet;
            break;
        }
        sheet = sheet->next;
    }

    if(!result) {
        unsigned int interval = 256;
        unsigned int firstUnicode = ((int)(character / interval)) * interval;
        result = sheet = createFontSheet(font, firstUnicode, firstUnicode + interval);

        //put at the start of the list
        sheet->next = font->sheets;
        font->sheets = sheet;
    }
    assert(result);

    return result;
}

Font initFont_(char *fileName, int firstChar, int endChar) {
    Font result = {}; 
    result.fileName = fileName;

    void *contents = 0;
    size_t contentsSize = 0; 
    Platform_LoadEntireFile_utf8(fileName, &contents, &contentsSize);

    //NOTE(ollie): This stores all the info about the font
    result.fontInfo = (stbtt_fontinfo *)platform_alloc_memory(sizeof(stbtt_fontinfo), true);
    
    //NOTE(ollie): Fill out the font info
    stbtt_InitFont(result.fontInfo, (const unsigned char *)contents, 0);

    result.sheets = createFontSheet(&result, firstChar, endChar);

    platform_free_memory(contents);
    return result;
}

Font initFont(char *fileName) {
    Font result = initFont_(fileName, 0, 256); // ASCII 0..256 not inclusive
    return result;
}

static inline GlyphInfo easyFont_getGlyph(Font *font, u32 unicodePoint) {
    GlyphInfo glyph = {};

    if(unicodePoint != '\n') {
        FontSheet *sheet = findFontSheet(font, unicodePoint);
        assert(sheet);

        int indexInArray = unicodePoint - sheet->minText;

        glyph = sheet->glyphs[indexInArray];

        glyph.handle = sheet->handle;

    }
    return glyph;
}


static Rect2f draw_text_(Renderer *renderer, Font *font, char *str, float startX, float yAt_, float fontScale, float4 font_color, bool renderGlyph, float maxWidth = FLT_MAX, bool recursiveChild = false, float *lastAlphaCharacter = 0, int lastAlphaCount = 0) {
    DEBUG_TIME_BLOCK();
    Rect2f result = make_rect2f(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    maxWidth += startX;

    float yAt = -font->fontHeight*fontScale + yAt_;

    bool newLine = true;

    float xAt = startX;

    bool parsing = true;
    EasyTokenizer tokenizer = lexBeginParsing(str, EASY_LEX_OPTION_NONE);

    int runeAt = 0;
    
    int roundCount = 0;
    while(parsing) {
         EasyToken token = lexGetNextToken(&tokenizer);
        if(token.type == TOKEN_NULL_TERMINATOR) {
            parsing = false;
        } else {
            if(!recursiveChild && token.type != TOKEN_SPACE) {
                char *word = easy_createString_printf(&globalPerFrameArena, "%.*s", token.size, token.at);
                float2 bounds = get_scale_rect2f(draw_text_(renderer, font, word, startX, yAt_, fontScale, font_color, false, FLT_MAX, true));
                if((xAt + bounds.x) >= maxWidth && bounds.x < (maxWidth - startX)) {
                    xAt = maxWidth;
                }
            }

            char *at = token.at;

            u32 lastRune = '\0';

            while(at < (token.at + token.size)) {

                if((*at == '\n' || xAt >= maxWidth) && token.type != TOKEN_SPACE && roundCount < 1) {
                    //NOTE: New line
                    yAt -= font->fontHeight*fontScale;
                    if(*at == '\n') {
                        at++;
                        lastRune = '\n';
                        runeAt++;
                    } else {
                        roundCount++;
                    }
                    newLine = true;
                } else {
                    roundCount = 0;
                    
                    u32 rune = easyUnicode_utf8_codepoint_To_Utf32_codepoint(&at, true);

                    GlyphInfo g = easyFont_getGlyph(font, rune);

                    assert(g.unicodePoint == rune);

                    if(g.hasTexture) {

                        float4 color = font_color;//make_float4(0, 0, 0, 1);
                        float2 scale = make_float2(g.width*fontScale, g.height*fontScale);
                        

                        if(newLine) {
                            xAt = startX + fontScale*g.xoffset + 0.5f*scale.x;
                        }

                        float3 pos = {};
                        //NOTE: Because STB font has the sprite at the bottom corner but we render the glyphs in the center, 
                        //      we offset the position by the width & height;
                        pos.x = xAt + fontScale*g.xoffset + 0.5f*fontScale*g.width;
                        assert(pos.x >= startX);
                        pos.y = yAt -fontScale*g.yoffset - 0.5f*g.height*fontScale;
                        pos.z = 0;

                        if(renderGlyph) {
                            float4 color = font_color;

                            if(runeAt < lastAlphaCount) {
                                color.w = lastAlphaCharacter[runeAt];
                            }
                            pushGlyph(renderer, g.handle, pos, scale, color, g.uvCoords);
                        }
                        

                        result = rect2f_union(make_rect2f_center_dim(make_float2(pos.x, pos.y), scale), result);
                    }

                    if(lastRune != '\0') {
                        xAt += fontScale*font->originalScaleFactor*stbtt_GetCodepointKernAdvance(font->fontInfo, lastRune, rune);
                    }

                    xAt += g.xadvance*fontScale;
                    newLine = false;

                    lastRune = rune;
                    runeAt++;
                }
            }
        }
    }
    return result;
}

static void draw_text(Renderer *renderer, Font *font, char *str, float startX, float yAt_, float fontScale, float4 font_color, float maxWidth = FLT_MAX) {
    draw_text_(renderer, font, str, startX, yAt_, fontScale, font_color, true, maxWidth);
}

static Rect2f getTextBounds(Renderer *renderer, Font *font, char *str, float startX, float yAt_, float fontScale, float maxWidth = FLT_MAX) {
    return draw_text_(renderer, font, str, startX, yAt_, fontScale, make_float4(1, 1, 1, 0), false, maxWidth);
}

struct FontWriter {
    bool active;
    char *string;
    int runeAt;
    int runeCount;
    float runeAlphaValues[1024];
};

void startFontWriter(FontWriter *fontWriter, char *string) {
    easyPlatform_zeroMemory(fontWriter, sizeof(FontWriter));

    assert(string);
    fontWriter->active = true;
    fontWriter->string = string;
    fontWriter->runeCount = easyString_getStringLength_utf8(string);
    assert(fontWriter->runeCount <= arrayCount(fontWriter->runeAlphaValues));
    
}

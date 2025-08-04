typedef enum {
    VAR_NULL,
    VAR_CHAR_STAR,
    VAR_LONG_UNSIGNED_INT,
    VAR_LONG_INT,
    VAR_INT,
    VAR_FLOAT,
    VAR_V2,
    VAR_V3,
    VAR_float4,
    VAR_BOOL,
    VAR_STRING,
} VarType;

//TODO(ollie): Get rid of using infinte allocs from the code base, i think this would be a good idea & less error prone

void addVar_(InfiniteAlloc *mem, void *val_, int count, char *varName, VarType type) {
    char *data;
    data = easy_createString_printf(&globalPerFrameArena, "\t%s: ", varName);
    addElementInifinteAllocWithCount_(mem, data, strlen(data));
    data = 0;
    
    if(count > 0) {
        if(count > 1 && !(type == VAR_CHAR_STAR || type == VAR_INT || type == VAR_FLOAT || type == VAR_V3)) {
            assert(!"array not handled yet");
        }
        switch(type) {
            case VAR_CHAR_STAR: {
                if(count == 1) {
                    char *val = (char *)val_;
                    data = easy_createString_printf(&globalPerFrameArena, "\"%s\"", val);
                } else {
                    assert(count > 1);
                    printf("isArray\n");
                    
                    char **val = (char **)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        printf("%s\n", val[i]);
                        data = easy_createString_printf(&globalPerFrameArena, "\"%s\"", val[i]);    
                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data = 0; //clear data
                    
                }
            } break;
            case VAR_LONG_UNSIGNED_INT: {
                unsigned long *val = (unsigned long *)val_;
                data = easy_createString_printf(&globalPerFrameArena, "%lu", val[0]);
            } break;
            case VAR_LONG_INT: {
                long *val = (long *)val_;
                data = easy_createString_printf(&globalPerFrameArena, "%ld", val[0]);
            } break;
            case VAR_INT: {
                if(count == 1) {
                    int *val = (int *)val_;
                    data = easy_createString_printf(&globalPerFrameArena, "%d", val[0]);
                } else {
                    assert(count > 1);
                    
                    int *val = (int *)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        data = easy_createString_printf(&globalPerFrameArena, "%d", val[i]);    
                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data = 0; //clear data
                }
            } break;
            case VAR_FLOAT: {
                if(count == 1) {
                    float *val = (float *)val_;
                    data = easy_createString_printf(&globalPerFrameArena, "%f", val[0]);
                } else {
                    assert(count > 1);
                    
                    float *val = (float *)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        data = easy_createString_printf(&globalPerFrameArena, "%f", val[i]);    
                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data = 0; //clear data
                }
            } break;
            case VAR_V2: {
                float *val = (float *)val_;
                data = easy_createString_printf(&globalPerFrameArena, "%f %f", val[0], val[1]);
            } break;
            case VAR_V3: {
                if(count == 1) {
                    float *val = (float *)val_;
                    data = easy_createString_printf(&globalPerFrameArena, "%f %f %f", val[0], val[1], val[2]);
                } else {
                    assert(count > 1);
                    
                    V3 *val = (V3 *)val_;
                    char *bracket = "[";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    for(int i = 0; i < count; ++i) {
                        V3 val1 = val[i];
                        data = easy_createString_printf(&globalPerFrameArena, "%f %f %f", val1.E[0], val1.E[1], val1.E[2]);

                        addElementInifinteAllocWithCount_(mem, data, strlen(data));
                        if(i != count - 1) {
                            char *commaString = ", ";
                            addElementInifinteAllocWithCount_(mem, commaString, 2);
                        }
                    }
                    bracket = "]";
                    addElementInifinteAllocWithCount_(mem, bracket, 1);
                    data = 0; //clear data
                }
               
            } break;
            case VAR_float4: {
                float *val = (float *)val_;
                data = easy_createString_printf(&globalPerFrameArena, "%f %f %f %f", val[0], val[1], val[2], val[3]);
            } break;
            case VAR_BOOL: {
                bool *val = (bool *)val_;
                const char *boolVal = val[0] ? "true" : "false";
                data = easy_createString_printf(&globalPerFrameArena, "%s", boolVal);
            } break;
            default: {
                printf("%s\n", "Error: case not handled in saving");
            }
        }
    }

    if(data) {
        addElementInifinteAllocWithCount_(mem, data, strlen(data));
    }

    data = easy_createString_printf(&globalPerFrameArena, ";\n");
    addElementInifinteAllocWithCount_(mem, data, strlen(data));
}

#define addVarArray(mem, val_, count, varName, type) addVar_(mem, val_, count, varName, type)
#define addVar(mem, val_, varName, type) addVar_(mem, val_, 1, varName, type)

typedef struct {
    VarType type;
    
    union {
        struct {
            float floatVal;
        };
        struct {
            char *stringVal; //copied onto the perframe arena so don't have to worry about cleaning up
        };
        struct {
            unsigned long intVal;
        };
        struct {
            bool boolVal;
        };
    };
} DataObject;

InfiniteAlloc *getDataObjects(EasyTokenizer *tokenizer) {
    bool parsing = true;
    if(tokenizer->typesArray.sizeOfMember == 0) { //check if it's been initied yets
       tokenizer->typesArray = initInfinteAlloc(DataObject);    
    }
    
    bool isArray = false;
    while(parsing) {
        char *at = tokenizer->src;
        EasyToken token = lexGetNextToken(tokenizer);
        // lexPrintToken(&token);
        assert(at != tokenizer->src);
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                DataObject data = {};
                data.type = VAR_BOOL;
                if(stringsMatchNullN("true", token.at, token.size)) {
                    data.boolVal = true;
                    addElementInifinteAlloc_(&tokenizer->typesArray, &data);
                } else if(stringsMatchNullN("false", token.at, token.size)) {
                    data.boolVal = false;
                    addElementInifinteAlloc_(&tokenizer->typesArray, &data);
                }
               
            } break;
            case TOKEN_STRING: {
                DataObject data = {};
                data.type = VAR_CHAR_STAR;
                data.stringVal = easyString_copyToArena_(token.at, &globalPerFrameArena, token.size);
                assert(data.stringVal[token.size] == '\0'); //null terminate
                
                addElementInifinteAlloc_(&tokenizer->typesArray, &data);
            } break;
            case TOKEN_INTEGER: {
                DataObject data = {};
                data.type = VAR_INT;
               
                char *endptr;
                // int negative = 1;
                // chat *at = token.at;
                // if(*at == '-') {  
                //     negative = -1;
                //     at++;
                // }
                unsigned long value = strtoul(easyString_copyToArena_(token.at, &globalPerFrameArena, token.size), &endptr, 10);
                data.intVal = value;
                addElementInifinteAlloc_(&tokenizer->typesArray, &data);
            } break;
            case TOKEN_FLOAT: {
                float value = atof(easyString_copyToArena_(token.at, &globalPerFrameArena, token.size));
                
                DataObject data = {};
                data.type = VAR_FLOAT;
                
                data.floatVal = value;
                addElementInifinteAlloc_(&tokenizer->typesArray, &data);
            } break;
            case TOKEN_BOOL: {
                
            } break;
            case TOKEN_COLON: {
                
            } break;
            case TOKEN_OPEN_SQUARE_BRACKET: {
                isArray = true;
                //TODO: Do we want to check that this is before any other data??
            } break;
            case TOKEN_SEMI_COLON: {
                parsing = false;
            } break;
            default : {
                
            }
        }
    }
    
    return &tokenizer->typesArray;
}

static inline float easyText_getIntOrFloat(DataObject obj) {
    float a = 0;
    if(obj.type == VAR_FLOAT) {
        a = obj.floatVal;
    } else {
        assert(obj.type == VAR_INT);
        a = (int)obj.intVal;
    }
    return a;
}

float2 buildV2FromDataObjects(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);

    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_FLOAT || objs[0].type == VAR_INT);
    assert(objs[1].type == VAR_FLOAT || objs[1].type == VAR_INT);
    
    float a = easyText_getIntOrFloat(objs[0]);
    float b = easyText_getIntOrFloat(objs[1]);
    
    float2 result = v2(a, b);

    data->count = 0; //release the memoy

    return result;
}

V3 buildV3FromDataObjects(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_FLOAT || objs[0].type == VAR_INT);
    assert(objs[1].type == VAR_FLOAT || objs[1].type == VAR_INT);
    assert(objs[2].type == VAR_FLOAT || objs[2].type == VAR_INT);

    
    V3 result = v3(easyText_getIntOrFloat(objs[0]), easyText_getIntOrFloat(objs[1]), easyText_getIntOrFloat(objs[2]));

    data->count = 0; //release the memoy
    return result;
}

V3 makeV3FromDataObjects_notClearObjects(EasyTokenizer *tokenizer, int offset) {
    DataObject *objs = (DataObject *)tokenizer->typesArray.memory;
    assert(objs[0 + offset].type == VAR_FLOAT || objs[0 + offset].type == VAR_INT);
    assert(objs[1 + offset].type == VAR_FLOAT || objs[1 + offset].type == VAR_INT);
    assert(objs[2 + offset].type == VAR_FLOAT || objs[2 + offset].type == VAR_INT);

    
    V3 result = v3(easyText_getIntOrFloat(objs[0 + offset]), easyText_getIntOrFloat(objs[1 + offset]), easyText_getIntOrFloat(objs[2 + offset]));

    return result;
}



float4 buildfloat4FromDataObjects(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_FLOAT);
    assert(objs[1].type == VAR_FLOAT);
    assert(objs[2].type == VAR_FLOAT);
    assert(objs[3].type == VAR_FLOAT);
    
    float4 result = v4(objs[0].floatVal, objs[1].floatVal, objs[2].floatVal, objs[3].floatVal);

    data->count = 0; //release the memoy
    return result;
}

//NOTE(ollie): Have to release the memory yourself
char *getStringFromDataObjects_lifeSpanOfFrame(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_CHAR_STAR);
    
    char *result = objs[0].stringVal;

    data->count = 0; //release the memoy
    
    return result;
}

#define getIntFromDataObjects(tokenizer) (int)getIntFromDataObjects_(tokenizer)
#define getULongFromDataObjects(tokenizer) getIntFromDataObjects_(tokenizer)
#define getLongFromDataObjects(tokenizer) (long)getIntFromDataObjects_(tokenizer)

unsigned long getIntFromDataObjects_(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_INT);
    
    unsigned long result = objs[0].intVal;

    data->count = 0; //release the memoy
    
    return result;
}

int getIntFromDataObjects_notClearObjects(EasyTokenizer *tokenizer, int offset) {
    DataObject *objs = (DataObject *)tokenizer->typesArray.memory;
    assert(objs[offset].type == VAR_INT || objs[offset].type == VAR_FLOAT);
    
    int result = (int)easyText_getIntOrFloat(objs[offset]);
    
    return result;
}

bool getBoolFromDataObjects(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_BOOL);
    
    bool result = objs[0].boolVal;

    data->count = 0; //release the memoy
    return result;
}


float getFloatFromDataObjects(EasyTokenizer *tokenizer) {
    InfiniteAlloc *data = getDataObjects(tokenizer);
    DataObject *objs = (DataObject *)data->memory;
    assert(objs[0].type == VAR_FLOAT);
    
    float result = objs[0].floatVal;

    data->count = 0; //release the memoy
    return result;
}
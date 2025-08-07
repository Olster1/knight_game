struct ResizeArrayHeader {
    size_t sizeOfElement;
    int elementsCount;
    int maxCount;
    Memory_Arena *arena;
};

#define initResizeArray(type) (type *)initResizeArray_(sizeof(type))
#define initResizeArrayArena(type, arena) (type *)initResizeArray_(sizeof(type), arena)

u8 *initResizeArray_(size_t sizeOfElement, Memory_Arena *arena = 0) {
    ResizeArrayHeader *header = 0;

    size_t startSize = sizeOfElement + sizeof(ResizeArrayHeader);

    if(arena) {
        header = (ResizeArrayHeader *)pushSize(arena, startSize);
    } else {
        header =(ResizeArrayHeader *)easyPlatform_allocateMemory(startSize, EASY_PLATFORM_MEMORY_ZERO);
    }
     
    u8 *array = ((u8 *)header) + sizeof(ResizeArrayHeader);

    header->sizeOfElement = sizeOfElement;
    header->elementsCount = 0;
    header->maxCount = 1;
    header->arena = arena;

    return array;
}

ResizeArrayHeader *getResizeArrayHeader(u8 *array) {
    ResizeArrayHeader *header = (ResizeArrayHeader *)(((u8 *)array) - sizeof(ResizeArrayHeader));
    return header;
}

void freeResizeArray(void *array_) {
    u8 *array = (u8 *)array_;
    ResizeArrayHeader *header = getResizeArrayHeader(array);
    if(header->arena) {
        assert(false);
    } else {
        easyPlatform_freeMemory(header);
    }
}

void clearResizeArray(void *array) {
     if(!array) {
        return;
    }
    ResizeArrayHeader *header = getResizeArrayHeader((u8 *)array);
    header->elementsCount = 0;
    
}

u8 *getResizeArrayContents(ResizeArrayHeader *header) {
    u8 *array = ((u8 *)header) + sizeof(ResizeArrayHeader);
    return array;
}

int getArrayLength(void *array) {
    if(!array) {
        return 0;
    }
    ResizeArrayHeader *header = getResizeArrayHeader((u8 *)array);
    assert(header->elementsCount <= header->maxCount);
    int result = header->elementsCount;
    return result;
}

// bool removeArrayIndex(void *array, int index) {
//     ResizeArrayHeader *header = getResizeArrayHeader((u8 *)array);
//     assert(header->elementsCount <= header->maxCount);
//     assert(index < header->elementsCount);
//     return result;
// }

#define pushArrayItem(array_, data, type)  (type *)pushArrayItem_((void **)array_, &data)
void *pushArrayItem_(void **array_, void *data) {
    u8 *array = *((u8 **)array_);
    u8 *newPos = 0;
    if(array) {
        ResizeArrayHeader *header = getResizeArrayHeader(array);

        if(header->elementsCount == header->maxCount) {
            //NOTE: Resize array
            size_t oldSize = header->maxCount*header->sizeOfElement + sizeof(ResizeArrayHeader);
            float resizeFactor = 1.5; //NOTE: Same as MSVC C++ Vector. x2 on GCC c++ Vector
            header->maxCount = round(header->maxCount*resizeFactor); 
            size_t newSize = header->maxCount*header->sizeOfElement + sizeof(ResizeArrayHeader);
            if(header->arena) {
                MemoryPiece *piece = getCurrentMemoryPiece(header->arena);
                
                u8 *endOfArray = (u8 *)header + oldSize;
                u8 *endOfArena = (u8 *)piece->memory + piece->currentSize;
                bool atEndOfArena = (endOfArray == endOfArena);

                size_t deltaBytes = newSize - oldSize;
                if(atEndOfArena && ((piece->currentSize + deltaBytes) <= piece->totalSize)) {
                    //NOTE: Still at the end of the arena and has room for more
                    piece->currentSize += deltaBytes;
                    assert(piece->currentSize <= piece->totalSize);
                } else {
                    u8 *newAddress = (u8 *)pushSize(header->arena, newSize);
                    easyPlatform_copyMemory(newAddress, header, oldSize);
                    header = (ResizeArrayHeader *)newAddress;
                }
                
            } else {
                header = (ResizeArrayHeader *)easyPlatform_reallocMemory(header, oldSize, newSize);
            }
            array = getResizeArrayContents(header);
        } 

        newPos = array + (header->elementsCount * header->sizeOfElement);
        header->elementsCount++;

        easyPlatform_copyMemory(newPos, data, header->sizeOfElement);
    }

    *array_ = array;

    return newPos;
}

bool doesArrayContain(void *array, void *itemToCheck) {
      if(!array) {
        return 0;
    }
    ResizeArrayHeader *header = getResizeArrayHeader((u8 *)array);
    assert(header->elementsCount <= header->maxCount);
    int length = header->elementsCount;

    bool result = false;

    for(int i = 0; i < length; ++i) {
        u8 *a = ((u8 *)array) + i*header->sizeOfElement;
        u8 *b = (u8 *)itemToCheck;
        if(memcmp(a, b, header->sizeOfElement) == 0) {
            result = true;
            break;
        }
    }
    
    return result;
    
}

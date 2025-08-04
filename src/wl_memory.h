////NOTE: MEMORY FUNCTIONS
#ifndef EASY_PLATFORM_H
/*
Code for platform depedent functions 
*/

typedef enum {
    EASY_PLATFORM_MEMORY_NONE,
    EASY_PLATFORM_MEMORY_ZERO,
} EasyPlatform_MemoryFlag;

static void easyPlatform_zeroMemory(void *ptr, size_t sizeInBytes) {
    memset(ptr, 0, sizeInBytes);
}

static void *easyPlatform_allocateMemory(size_t sizeInBytes, EasyPlatform_MemoryFlag flags) {
    
    void *result = 0;
    
#if __WIN32__
    result = HeapAlloc(GetProcessHeap(), 0, sizeInBytes);
#else 
    result = malloc(sizeInBytes);
#endif
    
    if(!result) {
        // easyLogger_addLog("Platform out of memory on heap allocate!");
    }
    
    if(flags & EASY_PLATFORM_MEMORY_ZERO) {
        memset(result, 0, sizeInBytes);
    }

    #if DEBUG_BUILD
        DEBUG_add_memory_block_size(&global_debug_stats, result, sizeInBytes);
    #endif
    
    return result;
}

static void easyPlatform_freeMemory(void *memory) {
#if __WIN32__
    HeapFree(GetProcessHeap(), 0, memory);
#else 
    free(memory);
#endif

#if DEBUG_BUILD
    DEUBG_remove_memory_block_size(&global_debug_stats, memory);
#endif
    
}


static inline void easyPlatform_copyMemory(void *to, void *from, size_t sizeInBytes) {
    memcpy(to, from, sizeInBytes);
}

static inline u8 * easyPlatform_reallocMemory(void *from, size_t oldSize, size_t newSize) {
    u8 *result = (u8 *)easyPlatform_allocateMemory(newSize, EASY_PLATFORM_MEMORY_ZERO);

    easyPlatform_copyMemory(result, from, oldSize);

    easyPlatform_freeMemory(from);

    return result;
}

#define EASY_PLATFORM_H 1
#endif


/////////////////////////


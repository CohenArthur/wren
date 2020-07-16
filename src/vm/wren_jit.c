#include "wren_jit.h"

#define WREN_JIT_DEFAULT_CAPACITY 64

static JitMap *jitInstance = NULL;

static JitMap *wrenJitMapInstance()
{
    if (!jitInstance)
    {
        jitInstance = malloc(sizeof(JitMap));

        wrenJitMapInit(jitInstance);
    }

    return jitInstance;
}

// Initializes a new JitMap
void wrenJitMapInit(JitMap *jit)
{
    jit->size = 0;
    jit->capacity = WREN_JIT_DEFAULT_CAPACITY;
    jit->data = calloc(WREN_JIT_DEFAULT_CAPACITY, sizeof(JitFunction*));
}

// Fetch a function from the JitMap
JitFunction *wrenJitMapGet(JitMap *jit); // FIXME: Send back proper type

// Add a new function to the JitMap
int wrenJitMapInsert(JitMap *jit);

// Frees the memory used by the JitMap
void wrenJitMapClear(JitMap *jit);

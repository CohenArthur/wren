#ifndef wren_jit_h
#define wren_jit_h

#include <stdlib.h>

#include "wren_value.h"

// A JitFunction is a structure contained in the instance of the Jit compiler.
// It contains the stored function // FIXME: and what else ?
typedef struct JitFunction {
    // The contained function
    ObjFn *fn;

    // Pointer to the next function in case of hash collision
    struct JitFunction *next;
} JitFunction;

// The JitMap contains a map of the functions and a way to search for them
// quickly. You can add functions to that map and search for them
typedef struct {
    size_t size;
    size_t capacity;

    // FIXME: Use contiguous data for cache optimization, so
    // JitFunction *data;
    JitFunction **data;
} JitMap;

// Fetch the global JIT instance
JitMap *wrenJitMapInstance(WrenVM *vm);

// Initializes a new JitMap
void wrenJitMapInit(WrenVM *vm, JitMap *jit);

// Fetch a function from the JitMap
ObjFn *wrenJitMapGet(JitMap *jit, char *functionName);

// Add a new function to the JitMap
int wrenJitMapInsert(WrenVM *vm, JitMap *jit, ObjFn *fn);

// Frees the memory used by the JitMap
void wrenJitMapClear(WrenVM *vm, JitMap *jit);

#endif // wren_jit_h

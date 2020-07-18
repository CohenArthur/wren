#ifndef wren_jit_h
#define wren_jit_h

#include <stdlib.h>

#include "wren_value.h"

// Default amount of Closures that the JIT can store
#define WREN_JIT_DEFAULT_CAPACITY 64

// A JitFunction is a structure contained in the instance of the Jit compiler.
// It contains the stored function // FIXME: and what else ?
typedef struct JitFunction {
    // The contained function
    ObjClosure *closure;

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
    // But that would make insertion slower. Weigh both sides
    JitFunction **data;
} JitMap;

// Initializes a new JitMap
void wrenJitMapInit(WrenVM *vm, JitMap *jit);

// Fetch a function from the JitMap
ObjClosure *wrenJitMapGet(JitMap *jit, ObjFn *fn);

// Add a new function to the JitMap
int wrenJitMapInsert(WrenVM *vm, JitMap *jit, ObjClosure *closure);

// Frees the memory used by the JitMap
void wrenJitMapClear(WrenVM *vm, JitMap *jit);

#endif // wren_jit_h

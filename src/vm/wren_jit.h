#ifndef wren_jit_h
#define wren_jit_h

#include <stdlib.h>

// A JitModule is a structure contained in the instance of the Jit compiler.
// It contains the stored module // FIXME: and what else ?
typedef struct JitModule {

    // Pointer to the next module in case of hash collision
    struct JitModule *next;
} JitModule;

// The JitMap contains a map of the modules and a way to search for them
// quickly. You can add modules to that map and search for them
typedef struct {
    size_t size;
    size_t capacity;

    JitModule **data;
} JitMap;

// Global Jit compiler instance // FIXME: Do a proper singleton
static JitMap *jitInstance;

// Initializes a new JitMap
void wrenJitMapInit(JitMap *jit);

// Fetch a module from the JitMap
JitModule *wrenJitMapGet(JitMap *jit); // FIXME: Send back proper type

// Add a new module to the JitMap
int wrenJitMapInsert(JitMap *jit);

// Frees the memory used by the JitMap
void wrenJitMapClear(JitMap *jit);

#endif // wren_jit_h

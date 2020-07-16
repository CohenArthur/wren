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

void wrenJitMapInit(JitMap *jit)
{
    jit->size = 0;
    jit->capacity = WREN_JIT_DEFAULT_CAPACITY;
    jit->data = calloc(WREN_JIT_DEFAULT_CAPACITY, sizeof(JitFunction*));
}

static void objFnToString(char* buffer, ObjFn* fn)
{
    // Byte-copy of the ObjFn to a char buffer
    memcpy(buffer, fn, sizeof(ObjFn));
}

static int objFnCmp(ObjFn *lhs, ObjFn *rhs)
{
    return memcmp(lhs, rhs, sizeof(ObjFn));
}

static unsigned long wrenHashDjb2(const char *key, size_t length)
{
    unsigned long hash = 5381; // Magic starting point

    for (size_t i = 0; i < length; i++)
        hash = ((hash << 5) + hash) + key[i];

    return hash;
}

ObjFn *wrenJitMapGet(JitMap *jit);

static JitFunction *wrenJitFunctionInit(ObjFn *fn, JitFunction *next)
{
    JitFunction *newFn = malloc(sizeof(JitFunction));

    newFn->fn = fn;
    newFn->next = next;

    return newFn;
}

int wrenJitMapInsert(JitMap *jit, ObjFn *fn)
{
    // FIXME: Only insert when necessary

    char fnRepr[sizeof(ObjFn)];
    objFnToString(fnRepr, fn);

    size_t idx = wrenHashDjb2(fnRepr, sizeof(ObjFn)) % jit->capacity;

    jit->data[idx] = wrenJitFunctionInit(fn, jit->data[idx]);

    return 0;
}

void wrenJitMapClear(JitMap *jit)
{
    jit->size = 0;
}

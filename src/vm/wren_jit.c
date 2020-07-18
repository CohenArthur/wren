#include "wren_jit.h"
#include "wren_vm.h"

#define WREN_JIT_DEFAULT_CAPACITY 64

static JitMap *jitInstance = NULL;

JitMap *wrenJitMapInstance(WrenVM *vm)
{
    if (!jitInstance)
    {
        jitInstance = malloc(sizeof(JitMap));

        wrenJitMapInit(vm, jitInstance);
    }

    return jitInstance;
}

void wrenJitMapInit(WrenVM *vm, JitMap *jit)
{
    jit->size = 0;
    jit->capacity = WREN_JIT_DEFAULT_CAPACITY;
    jit->data = calloc(WREN_JIT_DEFAULT_CAPACITY, sizeof(JitFunction*));
}

static unsigned long wrenHashDjb2(const char *key)
{
    unsigned long hash = 5381; // Magic starting point
    char c = '\0';

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

ObjFn *wrenJitMapGet(JitMap *jit, char *functionName)
{
    size_t hashIdx = wrenHashDjb2(functionName) % jit->capacity;

    for (JitFunction *curr = jit->data[hashIdx]; curr; curr = curr->next)
        if (!strcmp(functionName, curr->fn->debug->name))
            return curr->fn;

    return NULL;
}

static JitFunction *wrenJitFunctionInit(WrenVM *vm, ObjFn *fn, JitFunction *next)
{
    JitFunction *newFn = wrenReallocate(vm, NULL, 0, sizeof(JitFunction));

    newFn->fn = fn;
    newFn->next = next;

    return newFn;
}

int wrenJitMapInsert(WrenVM *vm, JitMap *jit, ObjFn *fn)
{
    // Check if the value has already been inserted
    if (wrenJitMapGet(jit, fn->debug->name))
        return -1;

    // FIXME: Only insert when necessary

    size_t idx = wrenHashDjb2(fn->debug->name) % jit->capacity;

    jit->data[idx] = wrenJitFunctionInit(vm, fn, jit->data[idx]);

    return 0;
}

void wrenJitMapClear(WrenVM *vm, JitMap *jit)
{
    jit->size = 0;
}

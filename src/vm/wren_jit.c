#include "wren_jit.h"
#include "wren_vm.h"
#include "wren_common.h"

void wrenJitMapInit(WrenVM *vm, JitMap *jit)
{
    jit->size = 0;
    jit->capacity = WREN_JIT_DEFAULT_CAPACITY;
    jit->data = calloc(WREN_JIT_DEFAULT_CAPACITY, sizeof(JitFunction*));
}

static unsigned long wrenHashDjb2(const char *key, size_t length)
{
    unsigned long hash = 5381; // Magic starting point

    for (size_t i = 0; i < length; i++)
        hash = ((hash << 5) + hash) + key[i];

    return hash;
}

// Populate a char buffer with a unique representation of a function
static void objFnToString(char strBuffer[], ObjFn *fn)
{
    memcpy(strBuffer, fn, sizeof(ObjFn));
}

static int objFnCmp(const ObjFn *lhs, const ObjFn *rhs)
{
    return memcmp(lhs, rhs, sizeof(ObjFn));
}

ObjClosure *wrenJitMapGet(JitMap *jit, ObjFn *fn)
{
    char fnName[sizeof(ObjFn)];
    objFnToString(fnName, fn);

    size_t hashIdx = wrenHashDjb2(fnName, sizeof(ObjFn)) % jit->capacity;

    for (JitFunction *curr = jit->data[hashIdx]; curr; curr = curr->next)
        if (!objFnCmp(curr->closure->fn, fn))
            return curr->closure;

    return NULL;
}

static JitFunction *wrenJitFunctionInit(WrenVM *vm, ObjClosure *closure, JitFunction *next)
{
    JitFunction *newFn = ALLOCATE(vm, JitFunction);

    newFn->closure = closure;
    newFn->next = next;

    return newFn;
}

int wrenJitMapInsert(WrenVM *vm, JitMap *jit, ObjClosure *closure)
{
    // Check if the value has already been inserted
    if (wrenJitMapGet(jit, closure->fn))
        return -1;

    char fnName[sizeof(ObjFn)];
    objFnToString(fnName, closure->fn);

    size_t idx = wrenHashDjb2(fnName, sizeof(ObjFn)) % jit->capacity;

    jit->data[idx] = wrenJitFunctionInit(vm, closure, jit->data[idx]);

    return 0;
}

void wrenJitMapClear(WrenVM *vm, JitMap *jit)
{
    DEALLOCATE(vm, jit->data);
    jit->size = 0;
}

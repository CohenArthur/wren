#include "wren_jit.h"
#include "wren_vm.h"
#include "wren_common.h"

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

ObjClosure *wrenJitMapGet(JitMap *jit, char *functionName)
{
    size_t hashIdx = wrenHashDjb2(functionName) % jit->capacity;

    for (JitFunction *curr = jit->data[hashIdx]; curr; curr = curr->next)
        if (!strcmp(functionName, curr->closure->fn->debug->name))
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
    if (wrenJitMapGet(jit, closure->fn->debug->name))
        return -1;

    size_t idx = wrenHashDjb2(closure->fn->debug->name) % jit->capacity;

    jit->data[idx] = wrenJitFunctionInit(vm, closure, jit->data[idx]);

    return 0;
}

void wrenJitMapClear(WrenVM *vm, JitMap *jit)
{
    DEALLOCATE(vm, jit->data);
    jit->size = 0;
}

#include <string.h>

#include "wren_utils.h"
#include "wren_vm.h"

DEFINE_BUFFER(Byte, uint8_t);
DEFINE_BUFFER(Int, int);
DEFINE_BUFFER(String, ObjString*);

void wrenSymbolTableInit(SymbolTable* symbols)
{
    symbols->count = 0;
    symbols->capacity = WREN_ST_DEFAULT_CAPACITY;
    symbols->data = calloc(WREN_ST_DEFAULT_CAPACITY, sizeof(Symbol *));
}

void wrenSymbolTableClear(WrenVM* vm, SymbolTable* symbols)
{
    for (size_t i = 0; i < symbols->capacity; i++)
    {
        free(symbols->data[i]);
        symbols->data[i] = NULL;
    }
}

static unsigned long wrenHashDjb2(const char* str)
{
    // Magic value to use as starting point to the hash function
    unsigned long hash = 5381;
    char c = '\0';

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

static Symbol* wrenSymbolInit(char* key,
                              unsigned long hash,
                              size_t idx,
                              ObjString* value)
{
    Symbol *newSymbol = calloc(1, sizeof(Symbol));

    newSymbol->key = key;
    newSymbol->value = value;
    newSymbol->hash = hash;
    newSymbol->idx = idx;

    return newSymbol;
}

static void wrenSymbolTableGrow(WrenVM *vm, SymbolTable* symbols)
{
    symbols->capacity *= 2;
    Symbol **oldData = symbols->data;

    symbols->data = calloc(symbols->capacity, sizeof(Symbol *));

    // Replace the old data with the new one, then free the old one
    for (size_t i = 0; i < symbols->capacity / 2; i++)
        if (oldData[i])
            wrenSymbolTableAdd(vm, symbols, oldData[i]->value->value,
                               oldData[i]->value->length);

    free(oldData);
}

int wrenSymbolTableAdd(WrenVM* vm, SymbolTable* symbols,
                       const char* name, size_t length)
{
  ObjString* symbol = AS_STRING(wrenNewStringLength(vm, name, length));
  unsigned long hash = wrenHashDjb2(symbol->value);

  wrenPushRoot(vm, &symbol->obj);

  // The relative index is used to associate variables in the SymbolTable
  size_t relativeIndex = symbols->count;

  symbols->count++;
  if (symbols->count == symbols->capacity)
      wrenSymbolTableGrow(vm, symbols);

  size_t idx = hash % symbols->capacity;
  while (symbols->data[idx]) // Find the nearest free spot
      idx++;

  symbols->data[idx] = wrenSymbolInit(symbol->value, hash, relativeIndex, symbol);

  wrenPopRoot(vm);

  return relativeIndex;
}

int wrenSymbolTableEnsure(WrenVM* vm,
                          SymbolTable* symbols,
                          const char* name,
                          size_t length)
{
    // See if the symbol is already defined.
    int existing = wrenSymbolTableFind(symbols, name, length);
    if (existing != -1) return existing;

    // New symbol, so add it.
    return wrenSymbolTableAdd(vm, symbols, name, length);
}

int wrenSymbolTableFind(const SymbolTable* symbols,
                        const char* name, size_t length)
{
  // FIXME: Check to see if name is the computed name or not

  unsigned long hash = wrenHashDjb2(name);
  size_t idx = hash % symbols->capacity;
  for (size_t idx = hash % symbols->capacity; idx < symbols->capacity; idx++)
    if (symbols->data[idx] && !strcmp(name, symbols->data[idx]->value->value))
      return symbols->data[idx]->idx;
  // Return the index associated with that value, but not its "true" index
  // in the hashtable since it doesn't matter

  return -1;
}

void wrenBlackenSymbolTable(WrenVM* vm, SymbolTable* symbolTable)
{
  for (int i = 0; i < symbolTable->count; i++)
    if (symbolTable->data[i])
        wrenGrayObj(vm, &symbolTable->data[i]->value->obj);

  // Keep track of how much memory is still in use.
  vm->bytesAllocated += symbolTable->capacity * sizeof(*symbolTable->data);
}

int wrenUtf8EncodeNumBytes(int value)
{
  ASSERT(value >= 0, "Cannot encode a negative value.");
  
  if (value <= 0x7f) return 1;
  if (value <= 0x7ff) return 2;
  if (value <= 0xffff) return 3;
  if (value <= 0x10ffff) return 4;
  return 0;
}

int wrenUtf8Encode(int value, uint8_t* bytes)
{
  if (value <= 0x7f)
  {
    // Single byte (i.e. fits in ASCII).
    *bytes = value & 0x7f;
    return 1;
  }
  else if (value <= 0x7ff)
  {
    // Two byte sequence: 110xxxxx 10xxxxxx.
    *bytes = 0xc0 | ((value & 0x7c0) >> 6);
    bytes++;
    *bytes = 0x80 | (value & 0x3f);
    return 2;
  }
  else if (value <= 0xffff)
  {
    // Three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
    *bytes = 0xe0 | ((value & 0xf000) >> 12);
    bytes++;
    *bytes = 0x80 | ((value & 0xfc0) >> 6);
    bytes++;
    *bytes = 0x80 | (value & 0x3f);
    return 3;
  }
  else if (value <= 0x10ffff)
  {
    // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
    *bytes = 0xf0 | ((value & 0x1c0000) >> 18);
    bytes++;
    *bytes = 0x80 | ((value & 0x3f000) >> 12);
    bytes++;
    *bytes = 0x80 | ((value & 0xfc0) >> 6);
    bytes++;
    *bytes = 0x80 | (value & 0x3f);
    return 4;
  }

  // Invalid Unicode value. See: http://tools.ietf.org/html/rfc3629
  UNREACHABLE();
  return 0;
}

int wrenUtf8Decode(const uint8_t* bytes, uint32_t length)
{
  // Single byte (i.e. fits in ASCII).
  if (*bytes <= 0x7f) return *bytes;

  int value;
  uint32_t remainingBytes;
  if ((*bytes & 0xe0) == 0xc0)
  {
    // Two byte sequence: 110xxxxx 10xxxxxx.
    value = *bytes & 0x1f;
    remainingBytes = 1;
  }
  else if ((*bytes & 0xf0) == 0xe0)
  {
    // Three byte sequence: 1110xxxx	 10xxxxxx 10xxxxxx.
    value = *bytes & 0x0f;
    remainingBytes = 2;
  }
  else if ((*bytes & 0xf8) == 0xf0)
  {
    // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
    value = *bytes & 0x07;
    remainingBytes = 3;
  }
  else
  {
    // Invalid UTF-8 sequence.
    return -1;
  }

  // Don't read past the end of the buffer on truncated UTF-8.
  if (remainingBytes > length - 1) return -1;

  while (remainingBytes > 0)
  {
    bytes++;
    remainingBytes--;

    // Remaining bytes must be of form 10xxxxxx.
    if ((*bytes & 0xc0) != 0x80) return -1;

    value = value << 6 | (*bytes & 0x3f);
  }

  return value;
}

int wrenUtf8DecodeNumBytes(uint8_t byte)
{
  // If the byte starts with 10xxxxx, it's the middle of a UTF-8 sequence, so
  // don't count it at all.
  if ((byte & 0xc0) == 0x80) return 0;
  
  // The first byte's high bits tell us how many bytes are in the UTF-8
  // sequence.
  if ((byte & 0xf8) == 0xf0) return 4;
  if ((byte & 0xf0) == 0xe0) return 3;
  if ((byte & 0xe0) == 0xc0) return 2;
  return 1;
}

// From: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int wrenPowerOf2Ceil(int n)
{
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  
  return n;
}

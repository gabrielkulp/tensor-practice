#pragma once
#include "tensor.h"
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned long long htKey_t;
#define KEYGEN_FIELD_SIZE 16 // up to order-4 without conflict
//#define KEYGEN_FIELD_SIZE 8 // up to order-8, but each mode has max len 256
#define TENSOR_READ_OVERPROVISION_FACTOR 1.5 // = capacity / nnz on file load

typedef struct htEntry {
	bool valid;
	htKey_t key;
	float value;
} htEntry;

typedef struct Hashtable {
	size_t capacity;
	size_t count;
	htEntry * table;
} Hashtable;

Hashtable * htNew(size_t capacity);
void htFree(Tensor * T);

bool htSet(Tensor * T, tCoord_t * key, float value);
float htGet(Tensor * T, tCoord_t * key);

void htPrintAll(Hashtable * ht);

void * htIteratorInit(Tensor * T);
void htIteratorCleanup(void * context);
tensorEntry htIteratorNext(Tensor * T, void * context);

const static tensorIterator htIterator = {.init = htIteratorInit,
                                          .next = htIteratorNext,
                                          .cleanup = htIteratorCleanup};

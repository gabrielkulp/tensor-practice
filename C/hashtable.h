#pragma once
#include "tensor.h"
#include <stddef.h>

#ifndef HT_OVERPROVISION
#define HT_OVERPROVISION 1.5 // = capacity / nnz on file load
#endif

#define HT_KEYGEN_FIELD_SIZE 16 // up to order-4 without conflict
//#define HT_KEYGEN_FIELD_SIZE 8 // up to order-8, but each mode has max len 256

extern size_t ht_capacity;
// ht_capacity is only accessed in htNew
void * htNew();
void htFree(Tensor * T);

bool htSet(Tensor * T, tCoord_t * key, float value);
float htGet(Tensor * T, tCoord_t * key);

void htPrintAll(void * ht); // only for debug

size_t htSize(Tensor * T);

void * htIteratorInit(Tensor * T);
void htIteratorCleanup(void * context);
tensorEntry htIteratorNext(Tensor * T, void * context);

const static tensorIterator htIterator = {.init = htIteratorInit,
                                          .next = htIteratorNext,
                                          .cleanup = htIteratorCleanup};

#pragma once
#include "tensor.h"
#include <stddef.h>

#ifndef BPT_ORDER
// must be even number
#define BPT_ORDER 8
#endif

#define BPT_KEYGEN_FIELD_SIZE 16 // up to order-4 without conflict
//#define BPT_KEYGEN_FIELD_SIZE 8 // up to order-8, but modes have max len 256

void * bptNew();
void bptFree(Tensor * T);

bool bptSet(Tensor * T, tCoord_t * key, float value);
float bptGet(Tensor * T, tCoord_t * key);

void bptPrintAll(Tensor * T); // only for debug

size_t bptSize(Tensor * T);

void * bptIteratorInit(Tensor * T);
void bptIteratorCleanup(void * context);
tensorEntry bptIteratorNext(Tensor * T, void * context);

const static tensorIterator bptIterator = {.init = bptIteratorInit,
                                           .next = bptIteratorNext,
                                           .cleanup = bptIteratorCleanup};

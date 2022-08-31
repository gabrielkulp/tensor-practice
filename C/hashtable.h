#pragma once
#include "tensor.h"

#define HT_KEYGEN_FIELD_SIZE 16 // up to order-4 without conflict
//#define HT_KEYGEN_FIELD_SIZE 8 // up to order-8, but each mode has max len 256
#define HT_TENSOR_READ_OVERPROVISION_FACTOR 1.2 // = capacity / nnz on file load

void * htNew(size_t capacity);
void htFree(Tensor * T);

bool htSet(Tensor * T, tCoord_t * key, float value);
float htGet(Tensor * T, tCoord_t * key);

void htPrintAll(void * ht); // only for debug

void * htIteratorInit(Tensor * T);
void htIteratorCleanup(void * context);
tensorEntry htIteratorNext(Tensor * T, void * context);

const static tensorIterator htIterator = {.init = htIteratorInit,
                                          .next = htIteratorNext,
                                          .cleanup = htIteratorCleanup};

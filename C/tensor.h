#pragma once
#include <stdbool.h>
#include <stdlib.h>

enum storageType {
	probingHashtable,
};

typedef unsigned short tMode_t;
typedef unsigned int tCoord_t;

typedef struct tensorEntry {
	tCoord_t * coords;
	float value;
} tensorEntry;

typedef struct Tensor {
	tMode_t order;
	tCoord_t * shape;
	enum storageType type;
	void * values;
} Tensor;

typedef struct tensorIterator {
	void * (*init)(Tensor *);
	tensorEntry (*next)(Tensor *, void *);
	void (*cleanup)(void *);
} tensorIterator;

Tensor * tensorNew(enum storageType type, tMode_t order, tCoord_t * shape,
                   size_t capacity);
void tensorFree(Tensor * T);
bool tensorSet(Tensor * T, tCoord_t * coords, float value);
float tensorGet(Tensor * T, tCoord_t * coords);
void tensorPrint(Tensor * T);

bool tensorWrite(Tensor * T, const char * filename);
Tensor * tensorRead(enum storageType type, const char * filename);

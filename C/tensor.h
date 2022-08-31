#pragma once
#include <stdbool.h>
#include <stdlib.h>

enum storageType {
	probingHashtable,
	BPlusTree,
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
	size_t entryCount;
	void * values;
} Tensor;

typedef struct tensorIterator {
	void * (*init)(Tensor *);
	tensorEntry (*next)(Tensor *, void *);
	void (*cleanup)(void *);
} tensorIterator;

// remember to set ht_capacity for probing hashtable
Tensor * tensorNew(enum storageType type, tMode_t order, tCoord_t * shape);
void tensorFree(Tensor * T);
bool tensorSet(Tensor * T, tCoord_t * coords, float value);
float tensorGet(Tensor * T, tCoord_t * coords);
void coordsPrint(Tensor * T, tCoord_t * coords);
bool tensorPrintMetadata(Tensor * T);
void tensorPrint(Tensor * T);
size_t tensorSize(Tensor * T);

// tensorRead automatically sets ht_capacity based on file length
bool tensorWrite(Tensor * T, const char * filename);
Tensor * tensorRead(enum storageType type, const char * filename);

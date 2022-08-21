#include <stdbool.h>
#include <stdlib.h>

enum storage_type {
	probing_hashtable,
};

typedef unsigned short tMode_t;
typedef unsigned int tCoord_t;

typedef struct Tensor {
	tMode_t order;
	tCoord_t * shape;
	enum storage_type type;
	void * values;
} Tensor;

Tensor * tensorNew(enum storage_type type, tMode_t order, tCoord_t * shape,
                   size_t capacity);
void tensorFree(Tensor * T);
bool tensorSet(Tensor * T, tCoord_t * coords, float value);
float tensorGet(Tensor * T, tCoord_t * coords);
void tensorPrint(Tensor * T);

bool tensorWrite(Tensor * T, const char * filename);
Tensor * tensorRead(enum storage_type type, const char * filename);

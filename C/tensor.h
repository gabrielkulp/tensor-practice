#include <stdlib.h>
#include <stdbool.h>

typedef unsigned short tMode_t;
typedef unsigned int tCoord_t;

typedef struct Tensor {
	bool valid;
	tMode_t order;
	tCoord_t * shape;
	void * values;
} Tensor;

Tensor * tensorNew(tMode_t order, tCoord_t * shape, size_t capacity);
void tensorFree(Tensor * T);
bool tensorSet(Tensor * T, tCoord_t * coords, float value);
float tensorGet(Tensor * T, tCoord_t * coords);
void tensorPrint(Tensor * T);

bool tensorWrite(Tensor * T, const char * filename);
Tensor * tensorRead(const char * filename);

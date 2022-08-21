#include "tensor.h"
#include "hashtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
enum storage_type {
  probing_hashtable,
};

typedef unsigned short tMode_t;
typedef unsigned int tCoord_t;

typedef struct Tensor {
  tMode_t order;
  tCoord_t * shape; // always non-null
  void * values; // null means whole structure invalid
} Tensor;
*/

#define KEYGEN_FIELD_SIZE 16 // up to order-4 without conflict
//#define KEYGEN_FIELD_SIZE 8 // up to order-8, but each mode has max len 256
#define TENSOR_READ_OVERPROVISION_FACTOR 1.5 // = capacity / nnz on file load

Tensor * tensorNew(enum storage_type type, tMode_t order, tCoord_t * shape,
                   size_t capacity) {
	Tensor * T = calloc(1, sizeof(Tensor));
	if (!T)
		return 0;

	T->order = order;
	T->shape = malloc(order * sizeof(tCoord_t));
	if (!T->shape) {
		free(T);
		return 0;
	}
	// memcpy(T->shape, shape, order * sizeof(unsigned int));
	for (tMode_t mode = 0; mode < order; mode++)
		T->shape[mode] = shape[mode];

	switch (type) {
		case probing_hashtable:
			T->values = htNew(capacity);
			break;
	}

	if (!T->values) {
		free(T->shape);
		free(T);
		return 0;
	}
	return T;
}

void tensorFree(Tensor * T) {
	if (!T || !T->values)
		return;

	htFree(T->values);
	T->order = 0;
	free(T->shape);
	T->shape = 0;
	free(T);
}

htKey_t tensorCoords2Key(Tensor * T, tCoord_t * coords) {
	htKey_t key = 0;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		key += (htKey_t)coords[mode] << (mode * KEYGEN_FIELD_SIZE);
	}
	return key;
}

void tensorKey2Coords(Tensor * T, tCoord_t * coords, htKey_t key) {
	tCoord_t mask = ~0;
	mask <<= (sizeof(mask) * 8) - KEYGEN_FIELD_SIZE;
	mask >>= (sizeof(mask) * 8) - KEYGEN_FIELD_SIZE;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		coords[mode] = (key >> (mode * KEYGEN_FIELD_SIZE)) & mask;
	}
}

bool tensorBoundsCheck(Tensor * T, tCoord_t * coords) {
	if (!T || !T->values)
		return false;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		if (coords[mode] >= T->shape[mode])
			return false;
	}
	return true;
}

bool tensorSet(Tensor * T, tCoord_t * coords, float value) {
	if (!tensorBoundsCheck(T, coords))
		return false;
	htKey_t key = tensorCoords2Key(T, coords);
	return htSet(T->values, key, value);
}

float tensorGet(Tensor * T, tCoord_t * coords) {
	if (!tensorBoundsCheck(T, coords))
		return false;
	htKey_t key = tensorCoords2Key(T, coords);
	return htGet(T->values, key);
}

bool tensorPrintMetadata(Tensor * T) {
	printf("Tensor:\n");
	if (!T || !T->values) {
		printf("  <invalid>\n");
		return false;
	}
	printf("  order: %i\n", T->order);
	printf("  shape: ");
	for (tMode_t mode = 0; mode < T->order; mode++)
		printf("%i ", T->shape[mode]);
	printf("\n");
	Hashtable * ht = T->values;
	printf("  capacity: %lu\n", ht->capacity);
	printf("  entries: %lu\n", ht->count);
	float volume = 1;
	for (tMode_t mode = 0; mode < T->order; mode++)
		volume *= (float)T->shape[mode];
	printf("  density: %f%%\n", 100 * (float)ht->count / volume);
	return true;
}

void tensorPrintRaw(Tensor * T) {
	if (tensorPrintMetadata(T))
		htPrintAll(T->values);
}

void tensorPrint(Tensor * T) {
	if (!tensorPrintMetadata(T))
		return;

	if (!T->values) {
		printf("    <invalid>\n");
		return;
	}

	tCoord_t * coords = malloc(T->order * sizeof(tCoord_t));
	if (!coords) {
		free(coords);
		printf("failed to allocate\n");
		return;
	}
	htEntry * entry;
	Hashtable * ht = T->values;
	void * context = htIteratorInit(ht);

	printf("  values:\n");
	while ((entry = htIteratorNext(ht, context))) {
		tensorKey2Coords(T, coords, entry->key);
		printf("    [");
		for (tMode_t mode = 0; mode < T->order; mode++) {
			printf("%u", coords[mode]);
			if (mode != T->order - 1)
				printf(" ");
		}
		printf("] = %f\n", entry->value);
	}
	free(coords);
	htIteratorCleanup(context);
}

bool tensorWrite(Tensor * T, const char * filename) {
	FILE * fp = fopen(filename, "w");

	if (!T->values) {
		fputs("<invalid>\n", fp);
		fclose(fp);
		return false;
	}

	fprintf(fp, "order: %u\n", T->order);
	fputs("shape: ", fp);
	for (tMode_t m = 0; m < T->order; m++) {
		fprintf(fp, "%u", T->shape[m]);
		if (m != T->order - 1)
			fputs(", ", fp);
	}
	fputs("\nvalues:\n", fp);

	tCoord_t * coords = calloc(T->order, sizeof(tCoord_t));
	if (!coords) {
		free(coords);
		fputs("<failed>\n", fp);
		fclose(fp);
		return false;
	}
	htEntry entry;
	Hashtable * ht = T->values;

	if (!ht->table) {
		printf("<invalid>\n");
		free(coords);
		fclose(fp);
		return true;
	}
	for (size_t i = 0; i < ht->capacity; i++) {
		entry = ht->table[i];
		if (!entry.valid || entry.value == 0)
			continue;
		tensorKey2Coords(T, coords, entry.key);
		for (tMode_t mode = 0; mode < T->order; mode++) {
			fprintf(fp, "%u, ", coords[mode]);
		}
		fprintf(fp, "%f\n", entry.value);
	}
	free(coords);
	fclose(fp);
	return true;
}

Tensor * tensorRead(enum storage_type type, const char * filename) {
	FILE * fp = fopen(filename, "r");
	if (!fp) {
		printf("failed to read file \"%s\"\n", filename);
		return 0;
	}
	size_t linecount = 0;
	char c;
	while ((c = fgetc(fp)) != EOF)
		if (c == '\n')
			linecount++;
	rewind(fp);

	Tensor * T;
	tMode_t order;

	fscanf(fp, "order: %hu\n", &order);

	tCoord_t * shape = malloc(order * sizeof(tCoord_t));
	if (!shape) {
		printf("allocation error\n");
		free(shape);
		fclose(fp);
		return 0;
	}

	fscanf(fp, "shape: ");

	for (tMode_t m = 0; m < order; m++) {
		fscanf(fp, "%u", &shape[m]);
		if (m != order - 1)
			fscanf(fp, ", ");
	}

	fscanf(fp, "\nvalues:\n");

	size_t capacity = TENSOR_READ_OVERPROVISION_FACTOR * (linecount - 3);
	T = tensorNew(type, order, shape, capacity);
	if (!T || !T->values) {
		printf("failed to create empty tensor\n");
		free(shape);
		fclose(fp);
		return 0;
	}

	tCoord_t * coords = shape;
	int scanned;
	float value;
	while (true) {
		scanned = 0;
		for (tMode_t m = 0; m < order; m++) {
			scanned += fscanf(fp, "%u, ", &coords[m]);
		}
		scanned += fscanf(fp, "%f\n", &value);

		if (scanned == order + 1)
			tensorSet(T, coords, value);
		else
			break;
	}

	free(coords);
	fclose(fp);
	return T;
}

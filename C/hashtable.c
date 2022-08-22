#include "hashtable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
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
*/

htKey_t _Coords2Key(Tensor * T, tCoord_t * coords) {
	htKey_t key = 0;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		key += (htKey_t)coords[mode] << (mode * KEYGEN_FIELD_SIZE);
	}
	return key;
}

void _Key2Coords(Tensor * T, tCoord_t * coords, htKey_t key) {
	tCoord_t mask = ~0;
	mask <<= (sizeof(mask) * 8) - KEYGEN_FIELD_SIZE;
	mask >>= (sizeof(mask) * 8) - KEYGEN_FIELD_SIZE;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		coords[mode] = (key >> (mode * KEYGEN_FIELD_SIZE)) & mask;
	}
}

Hashtable * htNew(size_t capacity) {
	Hashtable * ht = calloc(1, sizeof(Hashtable));
	if (!ht)
		return 0;

	ht->count = 0;
	ht->table = calloc(capacity, sizeof(htEntry));
	ht->capacity = ht->table ? capacity : 0;
	return ht;
}

void htFree(Tensor * T) {
	if (!T || !T->values)
		return;
	Hashtable * ht = T->values;
	free(ht->table);
	ht->table = 0;
	ht->capacity = 0;
	ht->count = 0;
	T->values = 0;
	free(ht);
}

// makes a new entry or modifies existing one
bool htSet(Tensor * T, tCoord_t * coords, float value) {
	if (!T)
		return false;
	if (!T->values)
		return false;
	if (!coords)
		return false;
	Hashtable * ht = T->values;
	if (!ht->table || !ht->capacity)
		return false;
	htKey_t key = _Coords2Key(T, coords);

	size_t i = key % ht->capacity;
	size_t init_i = i;
	while (ht->table[i].valid) {
		// check if overwriting
		if (ht->table[i].key == key) {
			ht->count--; // cancel impending increment
			break;
		}

		// increment but loop around the end
		i = (i + 1) % ht->capacity;

		// check if we just circled around the parking lot
		if (i == init_i)
			return false;
	}
	ht->table[i].valid = true;
	ht->table[i].key = key;
	ht->table[i].value = value;
	ht->count++;
	return true;
}

// returns 0 in too many cases. Not sure if that's okay
float htGet(Tensor * T, tCoord_t * coords) {
	if (!T || !T->values || !coords)
		return 0;
	Hashtable * ht = T->values;
	if (!ht->table)
		return 0;
	htKey_t key = _Coords2Key(T, coords);

	size_t i = key % ht->capacity;
	if (!ht->table[i].valid)
		return 0;

	size_t init_i = i;
	while (ht->table[i].key != key) {
		if (!ht->table[i].valid)
			return 0;

		// increment but loop around the end
		i = (i + 1) % ht->capacity;

		// check if we just circled around the parking lot
		if (i == init_i)
			return 0;
	}
	return ht->table[i].value;
}

#include <stdio.h>
void htePrint(htEntry hte) {
	if (!hte.valid)
		printf("<invalid>\n");
	else
		printf("%llu: %f\n", hte.key, hte.value);
}

void htPrintAll(Hashtable * ht) {
	printf("Hashtable:\n");
	if (!ht->table) {
		printf("  <invalid>\n");
		return;
	}
	for (size_t i = 0; i < ht->capacity; i++) {
		printf("  [%lu] ", i);
		htePrint(ht->table[i]);
	}
}

typedef struct htContext {
	size_t i;
	tCoord_t * coords;
} htContext;

void * htIteratorInit(Tensor * T) {
	htContext * ctx = calloc(sizeof(htContext), 1);
	if (!ctx)
		return 0;
	ctx->coords = calloc(sizeof(tCoord_t), T->order);
	if (!ctx->coords) {
		free(ctx);
		return 0;
	}
	return ctx;
}

void htIteratorCleanup(void * context) {
	htContext * ctx = context;
	if (ctx)
		free(ctx->coords);
	free(ctx);
}

tensorEntry htIteratorNext(Tensor * T, void * context) {
	if (!T || !T->values)
		return (tensorEntry){0};
	Hashtable * ht = T->values;
	if (!ht->table)
		return (tensorEntry){0};
	htContext * ctx = context;

	for (; ctx->i < ht->capacity; (ctx->i)++) {
		if (!ht->table[ctx->i].valid)
			continue;

		(ctx->i)++;
		if (ctx->i >= ht->capacity)
			break;
		htEntry * hte = &ht->table[ctx->i];
		_Key2Coords(T, ctx->coords, hte->key);
		tensorEntry ret = {.coords = ctx->coords, .value = hte->value};
		return ret;
	}
	return (tensorEntry){0};
}

#include "hashtable.h"
#include "stats.h"
#include <stddef.h>

size_t ht_capacity;

typedef unsigned long long htKey_t;
typedef struct htEntry {
	bool valid;
	htKey_t key;
	float value;
} htEntry;
const size_t _hteSize = sizeof(float)+sizeof(htKey_t);

typedef struct Hashtable {
	size_t capacity;
	htEntry * table;
} Hashtable;

static htKey_t _Coords2Key(Tensor * T, tCoord_t * coords) {
	htKey_t key = 0;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		key += (htKey_t)coords[(T->order - 1) - mode]
		       << (mode * HT_KEYGEN_FIELD_SIZE);
	}
	return key;
}

static void _Key2Coords(Tensor * T, tCoord_t * coords, htKey_t key) {
	tCoord_t mask = ~0;
	mask <<= (sizeof(mask) * 8) - HT_KEYGEN_FIELD_SIZE;
	mask >>= (sizeof(mask) * 8) - HT_KEYGEN_FIELD_SIZE;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		coords[(T->order - 1) - mode] =
		    (key >> (mode * HT_KEYGEN_FIELD_SIZE)) & mask;
	}
}

void * htNew() {
	Hashtable * ht = calloc(1, sizeof(Hashtable));
	if (!ht)
		return 0;

	size_t capacity = ht_capacity * HT_TENSOR_READ_OVERPROVISION_FACTOR;
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

	statsGlobal.mul++; // counting hash as MUL
	size_t i = key % ht->capacity;
	size_t init_i = i;
	statsGlobal.mem++;
	while (ht->table[i].valid) {
		if (i != init_i) // don't double-count the first access
			statsGlobal.mem++;

		// check if overwriting
		if (ht->table[i].key == key) {
			T->entryCount--;
			break;
		}

		// increment but loop around the end
		statsGlobal.add++;
		statsGlobal.cmp++;
		i = (i + 1) % ht->capacity;

		// check if we just circled around the parking lot
		statsGlobal.cmp++;
		if (i == init_i)
			return false;
	}
	ht->table[i].valid = true;
	ht->table[i].key = key;
	ht->table[i].value = value;
	T->entryCount++;
	statsGlobal.mem++; // store new value
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

	statsGlobal.mul++; // counting hash as mul
	size_t i = key % ht->capacity;
	statsGlobal.mem++;
	if (!ht->table[i].valid)
		return 0;

	size_t init_i = i;
	statsGlobal.cmp++;
	while (ht->table[i].key != key) {
		if (i != init_i) // don't double-count the first access
			statsGlobal.mem++;

		if (!ht->table[i].valid)
			return 0;

		// increment but loop around the end
		statsGlobal.add++;
		statsGlobal.cmp++;
		i = (i + 1) % ht->capacity;

		// check if we just circled around the parking lot
		statsGlobal.cmp++;
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

void htPrintAll(void * ptr) {
	Hashtable * ht = ptr;
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

size_t htSize(Tensor * T) {
	Hashtable * ht = T->values;
	return _hteSize * ht->capacity;
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

	statsGlobal.cmp++;
	for (; ctx->i < ht->capacity; (ctx->i)++) {
		statsGlobal.mem++;
		if (!ht->table[ctx->i].valid) {
			statsGlobal.cmp++; // loop condition
			continue;
		}

		htEntry * hte = &ht->table[ctx->i];
		statsGlobal.add++;
		(ctx->i)++;
		_Key2Coords(T, ctx->coords, hte->key);
		return (tensorEntry){.coords = ctx->coords, .value = hte->value};
	}
	return (tensorEntry){0};
}

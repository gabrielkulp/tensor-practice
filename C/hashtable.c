#include "hashtable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

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

Hashtable * htNew(size_t capacity) {
	Hashtable * ht = calloc(1, sizeof(Hashtable));
	if (!ht)
		return 0;

	ht->count = 0;
	ht->table = calloc(capacity, sizeof(htEntry));
	ht->capacity = ht->table ? capacity : 0;
	return ht;
}

void htFree(Hashtable * ht) {
	free(ht->table);
	ht->table = 0;
	ht->capacity = 0;
	ht->count = 0;
	free(ht);
}

// makes a new entry or modifies existing one
bool htSet(Hashtable * ht, htKey_t key, float value) {
	if (!ht->table || !ht->capacity)
		return false;

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
float htGet(Hashtable * ht, htKey_t key) {
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

void * htIteratorInit(Hashtable * ht) { return calloc(sizeof(size_t), 1); }
void htIteratorCleanup(void * ctx) { free(ctx); }

htEntry * htIteratorNext(Hashtable * ht, void * ctx) {
	if (!ht->table)
		return 0;
	size_t * i = (size_t *)ctx;

	for (; *i < ht->capacity; (*i)++) {
		if (!ht->table[*i].valid)
			continue;
		htEntry * ret = &ht->table[*i];
		(*i)++;
		return ret;
	}
	return 0;
}

#include <stdlib.h>
#include <stdbool.h>

typedef unsigned long long htKey_t;

typedef struct htEntry {
	bool valid;
	htKey_t key;
	float value;
} htEntry;

typedef struct Hashtable {
	bool valid;
	size_t capacity;
	size_t count;
	htEntry * table;
} Hashtable;

Hashtable * htNew(size_t capacity);
void htFree(Hashtable * ht);

bool htSet(Hashtable * ht, htKey_t key, float value);
float htGet(Hashtable * ht, htKey_t key);

void htPrintAll(Hashtable * ht);
void htPrintEntries(Hashtable * ht);

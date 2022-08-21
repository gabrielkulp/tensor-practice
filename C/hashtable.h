#include <stdbool.h>
#include <stdlib.h>

typedef unsigned long long htKey_t;

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

Hashtable * htNew(size_t capacity);
void htFree(Hashtable * ht);

bool htSet(Hashtable * ht, htKey_t key, float value);
float htGet(Hashtable * ht, htKey_t key);

void htPrintAll(Hashtable * ht);

void * htIteratorInit(Hashtable * ht);
void htIteratorCleanup(void * ctx);
htEntry * htIteratorNext(Hashtable * ht, void * ctx);

#include "bpTree.h"
#include "tensor.h"
#include <stddef.h>
#include <stdio.h>

typedef unsigned long long tKey_t;

typedef struct bptNode {
	bool isLeaf;
	size_t childCount;
	union {
		// since we're storing sparse tensors, a zero float is considered empty
		float values[BPT_ORDER];    // if isLeaf
		void * children[BPT_ORDER]; // if !isLeaf
	};
	tKey_t keys[BPT_ORDER]; // if isLeaf
} bptNode;

static tKey_t _Coords2Key(Tensor * T, tCoord_t * coords) {
	tKey_t key = 0;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		key += (tKey_t)coords[(T->order - 1) - mode]
		       << (mode * BPT_KEYGEN_FIELD_SIZE);
	}
	return key;
}

static void _Key2Coords(Tensor * T, tCoord_t * coords, tKey_t key) {
	tCoord_t mask = ~0;
	mask <<= (sizeof(mask) * 8) - BPT_KEYGEN_FIELD_SIZE;
	mask >>= (sizeof(mask) * 8) - BPT_KEYGEN_FIELD_SIZE;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		coords[(T->order - 1) - mode] =
		    (key >> (mode * BPT_KEYGEN_FIELD_SIZE)) & mask;
	}
}

static void _printKey(Tensor * T, tKey_t key) {
	tCoord_t * coords = calloc(sizeof(tCoord_t), T->order);
	_Key2Coords(T, coords, key);
	coordsPrint(T, coords);
	free(coords);
}

void * bptNew(size_t capacity) {
	bptNode * root = calloc(sizeof(bptNode), 1);
	root->isLeaf = true;
	return root;
}

// Recursive B+ Tree free
void _freeNode(bptNode * n) {
	if (!n)
		return;
	if (!n->isLeaf)
		for (size_t i = 0; i < BPT_ORDER; i++)
			_freeNode(n->children[i]);
	free(n);
}

// Start recursive free from the root
void bptFree(Tensor * T) {
	if (!T || !T->values)
		return;
	bptNode * root = T->values;
	_freeNode(root);
}

// Split leaf node into two nodes, and add new value to one of them.
// Returns a new leaf node that's a sibling of the one you pass in.
bptNode * _splitLeaf(bptNode * node, tKey_t key, float value, size_t idx) {
	const size_t half = BPT_ORDER / 2; // assume BPT_ORDER is even
	bptNode * newNode = calloc(sizeof(bptNode), 1);
	newNode->childCount = half;
	newNode->isLeaf = true;
	node->childCount = half;
	// behavior depends on if insertion point is in old or new node
	if (idx < half) {
		// insertion point in old node
		// so first we can copy over to the new node cleanly
		for (size_t i = 0; i < half; i++) {
			newNode->values[i] = node->values[i + half];
			newNode->keys[i] = node->keys[i + half];
			node->values[i + half] = 0; // mark as empty;
		}

		// then we shift over existing entries in the old node
		for (size_t i = idx; i < half; i++) {
			node->values[i + 1] = node->values[i];
			node->keys[i + 1] = node->keys[i];
		}
		// and add the new value in the gap that created
		node->values[idx] = value;
		node->keys[idx] = key;
		node->childCount++;
		return newNode;
	} else {
		// insertion point in new node (idx >= half)

		// so first we do a shifting copy into the new node
		// by copying over everything before the new entry
		for (size_t i = half; i < idx; i++) {
			newNode->values[i - half] = node->values[i];
			newNode->keys[i - half] = node->keys[i];
		}
		// then everything after
		for (size_t i = idx; i < BPT_ORDER; i++) {
			newNode->values[i - half + 1] = node->values[i];
			newNode->keys[i - half + 1] = node->keys[i];
		}
		// and then add the new entry in the gap
		newNode->values[idx - half] = value;
		newNode->keys[idx - half] = key;
		newNode->childCount++;

		// finally cleanly invalidate copied entries in the old node
		for (size_t i = half; i < BPT_ORDER; i++)
			node->values[i] = 0;

		return newNode;
	}
}

// Split internal node into two nodes, and add new child to one of them.
// Returns a new internal node that's a sibling of the node you pass in.
bptNode * _splitInternal(bptNode * node, bptNode * newChild, size_t idx) {
	const size_t half = BPT_ORDER / 2; // assume BPT_ORDER is even
	bptNode * newNode = calloc(sizeof(bptNode), 1);
	newNode->childCount = half;
	node->childCount = half;
	// behavior depends on if insertion point is in old or new node
	if (idx < half) {
		// insertion point in old node
		// so first we can copy over to the new node cleanly
		for (size_t i = 0; i < half; i++) {
			newNode->children[i] = node->children[i + half];
			newNode->keys[i] = node->keys[i + half];
			node->children[i + half] = NULL; // mark as empty;
		}

		// then we shift over existing entries in the old node
		for (size_t i = half - 1; i >= idx; i--) {
			node->children[i + 1] = node->children[i];
			node->keys[i + 1] = node->keys[i];
		}

		// and add the new value in the gap that created
		node->children[idx] = newChild;
		// node->keys[idx] = node->keys[idx + 1];
		if (idx)
			node->keys[idx - 1] = newChild->keys[0];
		bptNode * afterNew = node->children[idx + 1];
		node->keys[idx] = afterNew->keys[0];
		node->keys[half] = 0; // optional. todo: remove and test
		node->childCount++;
		return newNode;
	} else {
		// insertion point in new node (idx >= half)

		// so first we do a shifting copy into the new node
		// by copying over everything before the new entry
		for (size_t i = half; i < idx; i++) {
			newNode->children[i - half] = node->children[i];
			newNode->keys[i - half] = node->keys[i];
		}
		// then everything after
		for (size_t i = idx; i < BPT_ORDER; i++) {
			newNode->children[i - half + 1] = node->children[i];
			newNode->keys[i - half + 1] = node->keys[i];
		}
		// and then add the new entry in the gap
		newNode->children[idx - half] = newChild;
		if (idx - half)
			newNode->keys[idx - half - 1] = newChild->keys[0];

		// finally cleanly invalidate copied entries in the old node
		for (size_t i = half; i < BPT_ORDER; i++)
			node->children[i] = NULL;

		newNode->childCount++;
		return newNode;
	}
}

// Recursive B+ Tree insertion function.
// Returns NULL or a pointer to a new sibling node if there's a split.
// todo: remove Tensor argument
bptNode * _insert(Tensor * T, bptNode * node, tKey_t key, float value) {
	if (!node)
		return NULL;
	if (node->isLeaf) {
		// todo: binary search
		size_t insertIdx = node->childCount;
		for (size_t i = 0; i < node->childCount; i++) {
			if (key == node->keys[i]) {
				// update existing value instead of inserting
				node->values[i] = value;
				return NULL;
			}
			if (key < node->keys[i]) {
				insertIdx = i;
				break;
			}
		}
		printf("%p leaf index is %lu\n", node, insertIdx);
		if (node->childCount == BPT_ORDER)
			return _splitLeaf(node, key, value, insertIdx);

		// else shift children to add new entry
		if (node->childCount) {
			for (size_t i = node->childCount - 1; i > insertIdx; i--) {
				node->values[i + 1] = node->values[i];
				node->keys[i + 1] = node->keys[i];
			}
			// do last iteration separately in case insertIdx=0
			// so we don't underflow our unsigned iterator
			if (insertIdx < BPT_ORDER - 1) {
				node->values[insertIdx + 1] = node->values[insertIdx];
				node->keys[insertIdx + 1] = node->keys[insertIdx];
			}
		}
		node->values[insertIdx] = value;
		node->keys[insertIdx] = key;
		node->childCount++;
		return NULL;
	} else { // internal node
		// todo: binary search
		size_t insertIdx;
		for (insertIdx = 0; insertIdx < node->childCount - 1; insertIdx++)
			if (key <= node->keys[insertIdx])
				break;
		printf("%p internal index is %lu\n", node, insertIdx);
		bptNode * newChild = _insert(T, node->children[insertIdx], key, value);
		if (!newChild)
			return NULL;

		tKey_t newInterval = newChild->keys[0];
		printf("new interval is ");
		_printKey(T, newInterval);
		putchar('\n');

		// adjust shift point depending on new sort order
		bptNode * oldFirstChild = node->children[insertIdx];
		tKey_t oldInterval = oldFirstChild->keys[0];

		printf("old interval is ");
		_printKey(T, oldInterval);
		putchar('\n');
		bool swap = (newInterval > oldInterval);
		if (swap) {
			insertIdx++;
			printf("  insertion index now at %lu\n", insertIdx);
		}

		// too many children so we need to split and tell our parent
		if (node->childCount == BPT_ORDER)
			return _splitInternal(node, newChild, insertIdx);

		// else shift children to add new entry
		printf("node is %p ", node);
		printf("child count is %lu, insertIdx is %lu\n", node->childCount,
		       insertIdx);
		for (size_t i = node->childCount - 1; i > insertIdx; i--) {
			printf("trying with i=%lu\n", i);
			node->children[i + 1] = node->children[i];
			node->keys[i + 1] = node->keys[i];
		}
		// now finish up the for loop at i=insertIdx in case it's 0.
		// Iterator is unsigned so this prevents underflow
		if (insertIdx != BPT_ORDER - 1) {
			node->children[insertIdx + 1] = node->children[insertIdx];
			node->keys[insertIdx + 1] = node->keys[insertIdx];
		}

		// and then actually insert the new child
		node->children[insertIdx] = newChild;
		if (insertIdx != BPT_ORDER - 1)
			node->keys[insertIdx] = node->keys[insertIdx + 1];
		else
			node->keys[insertIdx] = 0; // optional actually. todo: remove
		node->childCount++;

		// and patch up the intervals (keys)
		if (insertIdx)
			node->keys[insertIdx - 1] = swap ? newInterval : oldInterval;
		if (swap && insertIdx < node->childCount - 1) {
			bptNode * afterInsert = node->children[insertIdx + 1];
			node->keys[insertIdx] = afterInsert->keys[0];
		}

		return NULL; // no new siblings for parent to be aware of
	}
}

// Start insertion from the root. Also handles root splitting.
bool bptSet(Tensor * T, tCoord_t * coords, float value) {
	if (!T || !T->values || !coords)
		return false;

	printf("\nInsert %f at ", value);
	coordsPrint(T, coords);
	putchar('\n');
	while ('\n' != getchar())
		;

	bptNode * root = T->values;
	tKey_t key = _Coords2Key(T, coords);
	bptNode * rootSibling = _insert(T, root, key, value);
	if (rootSibling) {
		// root node split during insertion, so integrate new node
		bptNode * newRoot = calloc(sizeof(bptNode), 1);
		printf("new root! sibling is %p, new root is %p\n", rootSibling,
		       newRoot);
		newRoot->isLeaf = false;
		newRoot->childCount = 2;
		newRoot->children[0] = root;
		newRoot->children[1] = rootSibling;
		newRoot->keys[0] = rootSibling->keys[0];
		T->values = newRoot;
	}
	bptPrintAll(T);
	return true; // insertion success
};

float _search(bptNode * node, tKey_t key) {
	if (!node)
		return 0;
	if (node->isLeaf) {
		// todo: make this a binary search
		for (size_t i = 0; i < node->childCount; i++) {
			if (node->keys[i] == key)
				return node->values[i];
			if (node->keys[i] > key)
				break;
		}
		return 0;
	} else { // node is internal
		// todo: make this a binary search
		for (size_t i = 0; i < node->childCount - 1; i++) {
			if (key <= node->keys[i])
				return _search(node->children[i], key);
		}
		// if it's not in the other children, it might be in the last one
		return _search(node->children[node->childCount - 1], key);
	}
}
float bptGet(Tensor * T, tCoord_t * coords) {
	if (!T || !T->values || !coords)
		return 0;

	bptNode * root = T->values;
	tKey_t key = _Coords2Key(T, coords);
	return _search(root, key);
};

void _print(Tensor * T, bptNode * node, uint depth) {
	for (uint i = 0; i < depth; i++)
		putchar('\t');
	printf("@%p", node);
	if (!node) {
		putchar('\n');
		return;
	}
	printf(", cnt=%lu\n", node->childCount);

	if (node->isLeaf) {
		for (size_t i = 0; i < BPT_ORDER; i++) {
			for (uint i = 0; i < depth; i++)
				putchar('\t');
			if (i >= node->childCount)
				printf("\x1b[30m");
			_printKey(T, node->keys[i]);
			printf(": %f\n", node->values[i]);
			printf("\x1b[0m");
		}
	} else {
		for (size_t i = 0; i < BPT_ORDER; i++) {
			for (uint i = 0; i < depth; i++)
				putchar('\t');
			if (i >= node->childCount)
				printf("\x1b[30m");
			printf("> child with extent ");
			_printKey(T, node->keys[i]);
			putchar('\n');
			_print(T, node->children[i], depth + 1);
			printf("\x1b[0m");
		}
	}
}

void bptPrintAll(Tensor * T) {
	bptNode * root = T->values;
	printf("raw B+ Tree (%p, %p) contents:\n", T, T->values);
	if (!root) {
		printf("\tThere's no root!\n");
		return;
	}
	_print(T, root, 0);
	putchar('\n');
}

typedef struct bptContext {
	size_t child_idx;
	struct bptContext * parent; // previous link
	struct bptContext * tail;   // jump to end of list
} bptContext;

void * bptIteratorInit(Tensor * T) {
	bptPrintAll(T);
	if (!T || !T->values)
		return NULL;
	bptContext * ctx = calloc(sizeof(bptContext), 1);
	if (ctx)
		ctx->tail = ctx;
	return ctx;
}

void bptIteratorCleanup(void * context) {
	bptContext * ctx = context;
	if (!ctx)
		return;

	// free the whole context stack from the tail in
	bptContext * entry = ctx->tail;
	while (ctx->parent) {
		entry = ctx->parent;
		free(ctx);
		ctx = entry;
	}
	free(ctx);
}

tensorEntry bptIteratorNext(Tensor * T, void * context) {
	// todo
	return (tensorEntry){0};
}

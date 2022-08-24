#include "bpTree.h"
#include <stddef.h>

typedef unsigned long long tKey_t;

typedef struct bptNode {
	bool isLeaf;
	size_t childCount;
	union {
		// since we're storing sparse tensors, a zero float is considered empty
		float values[BPT_ORDER];    // if isLeaf
		void * children[BPT_ORDER]; // if !isLeaf
	};
	union {
		tKey_t keys[BPT_ORDER];      // if isLeaf
		tKey_t intervals[BPT_ORDER]; // if !isLeaf
	};
} bptNode;

tKey_t _Coords2Key(Tensor * T, tCoord_t * coords) {
	tKey_t key = 0;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		key += (tKey_t)coords[mode] << (mode * BPT_KEYGEN_FIELD_SIZE);
	}
	return key;
}

void _Key2Coords(Tensor * T, tCoord_t * coords, tKey_t key) {
	tCoord_t mask = ~0;
	mask <<= (sizeof(mask) * 8) - BPT_KEYGEN_FIELD_SIZE;
	mask >>= (sizeof(mask) * 8) - BPT_KEYGEN_FIELD_SIZE;
	for (tMode_t mode = 0; mode < T->order; mode++) {
		coords[mode] = (key >> (mode * BPT_KEYGEN_FIELD_SIZE)) & mask;
	}
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
	for (size_t i = 0; i < BPT_ORDER; i++)
		_freeNode(n->children[i]);
	free(n);
}

// Start recursive free from the root
void bptFree(Tensor * T) {
	if (!T || !T->values)
		return;
	bptNode * root = T->values;
	for (size_t i = 0; i < BPT_ORDER; i++)
		_freeNode(root->children[i]);
}

// Split leaf node into two nodes, and add new value to one of them.
// Returns a new leaf node that's a sibling of the one you pass in.
bptNode * _splitLeaf(bptNode * node, tKey_t key, float value, size_t idx) {
	const size_t half = BPT_ORDER / 2;
	bptNode * newNode = calloc(sizeof(bptNode), 1);
	newNode->childCount = half;
	// first do whichever full

	return NULL;
}

// Split internal node into two nodes, and add new child to one of them.
// Returns a new internal node that's a sibling of the node you pass in.
bptNode * _splitInternal(bptNode * node, bptNode * newChild, size_t idx) {
	return NULL;
}

// Recursive B+ Tree insertion function.
// Returns NULL or a pointer to a new sibling node if there's a split.
bptNode * _insert(bptNode * node, tKey_t key, float value) {
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
		if (node->childCount == BPT_ORDER)
			return _splitLeaf(node, key, value, insertIdx);

		// else shift children to add new entry
		for (size_t i = node->childCount - 1; i >= insertIdx; i--) {
			node->values[i + 1] = node->values[i];
			node->keys[i + 1] = node->keys[i];
		}
		node->values[insertIdx] = value;
		node->keys[insertIdx] = key;
		node->childCount++;
		return NULL;
	} else { // internal node
		// todo: binary search
		size_t insertIdx;
		for (insertIdx = 0; insertIdx < node->childCount; insertIdx++)
			if (key <= node->intervals[insertIdx])
				break;
		bptNode * newChild = _insert(node->children[insertIdx], key, value);
		if (!newChild)
			return NULL;
		else
			return _splitInternal(node, newChild, insertIdx);
	}
}

// Start insertion from the root. Also handles root splitting.
bool bptSet(Tensor * T, tCoord_t * coords, float value) {
	if (!T || !T->values || !coords)
		return false;

	bptNode * root = T->values;
	tKey_t key = _Coords2Key(T, coords);
	bptNode * rootSibling = _insert(root, key, value);
	if (rootSibling) {
		// root node split during insertion, so integrate new node
		bptNode * newRoot = calloc(sizeof(bptNode), 1);
		newRoot->isLeaf = false;
		newRoot->childCount = 2;
		newRoot->children[0] = root;
		newRoot->children[1] = rootSibling;
		newRoot->intervals[0] = rootSibling->intervals[0];
		T->values = newRoot;
	}
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
			if (key <= node->intervals[i])
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

#include <stdio.h>
void _print(bptNode * node, uint depth) {
	if (!node)
		return;

	if (node->isLeaf) {
		for (size_t i = 0; i < BPT_ORDER; i++) {
			for (uint i = 0; i < depth; i++)
				putchar(' ');
			printf("%llu: %f\n", node->keys[i], node->values[i]);
		}
	} else {
		for (size_t i = 0; i < BPT_ORDER; i++) {
			for (uint i = 0; i < depth; i++)
				putchar(' ');

			_print(node->children[i], depth + 1);
		}
	}
}

void bptPrintAll(void * root) {
	if (!root)
		return;
	_print(root, 0);
}

typedef struct bptContext {
	size_t child_idx;
	struct bptContext * parent; // previous link
	struct bptContext * tail;   // jump to end of list
} bptContext;

void * bptIteratorInit(Tensor * T) {
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
	bptPrintAll(T->values);
	return (tensorEntry){0};
}

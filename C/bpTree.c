#include "bpTree.h"
#include "stats.h"
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
const size_t _bptNodeSize = 2+(BPT_ORDER*(sizeof(tKey_t)+sizeof(float)));

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

void * bptNew() {
	bptNode * root = calloc(sizeof(bptNode), 1);
	root->isLeaf = true;
	statsGlobal.mem++;
	return root;
}

// Recursive B+ Tree free
void _freeNode(bptNode * n) {
	if (!n)
		return;
	statsGlobal.mem++;
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

	statsGlobal.mem += 3; // read old node, write both new ones
	statsGlobal.add += 1; // child count increment
	statsGlobal.cmp += 1; // index comparison

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
		for (size_t i = half - 1; i > idx; i--) {
			node->values[i + 1] = node->values[i];
			node->keys[i + 1] = node->keys[i];
		}
		// end with just idx in case idx=0 to avoid underflow
		node->values[idx + 1] = node->values[idx];
		node->keys[idx + 1] = node->keys[idx];

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

	statsGlobal.mem += 3; // read old node, write two new ones
	statsGlobal.add += 1; // child count increment
	statsGlobal.cmp += 1; // index comparison

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
		for (size_t i = half - 1; i > idx; i--) {
			node->children[i + 1] = node->children[i];
			node->keys[i + 1] = node->keys[i];
		}
		// end with just idx in case idx=0 to avoid underflow
		node->children[idx + 1] = node->children[idx];
		node->keys[idx + 1] = node->keys[idx];

		// and add the new value in the gap that created
		node->children[idx] = newChild;
		node->keys[idx] = newChild->keys[0];
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
		newNode->keys[idx - half] = newChild->keys[0];

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
	statsGlobal.mem += 2; // get the node we'll interact with then store it
	if (node->isLeaf) {
		// todo: binary search
		size_t insertIdx = node->childCount;
		for (size_t i = 0; i < node->childCount; i++) {
			statsGlobal.cmp++; // key check
			if (key == node->keys[i]) {
				// update existing value instead of inserting
				statsGlobal.mem++; // save new value

				node->values[i] = value;
				return NULL;
			}
			if (key < node->keys[i]) {
				insertIdx = i;
				break;
			}
		}

		T->entryCount++;
		statsGlobal.cmp++; // count check
		if (node->childCount == BPT_ORDER) {
			return _splitLeaf(node, key, value, insertIdx);
		}

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
			if (key <= node->keys[insertIdx + 1])
				break;
		bptNode * newChild = _insert(T, node->children[insertIdx], key, value);

		if (!newChild) {
			statsGlobal.cmp++;
			if (!insertIdx) {
				bptNode * firstChild = node->children[0];
				node->keys[0] = firstChild->keys[0];
			}
			return NULL;
		}

		statsGlobal.mem++; // look at first child
		tKey_t newInterval = newChild->keys[0];

		// adjust shift point depending on new sort order
		bptNode * oldFirstChild = node->children[insertIdx];
		tKey_t oldInterval = oldFirstChild->keys[0];

		statsGlobal.cmp++; // interval check
		bool swap = (newInterval > oldInterval);
		if (swap)
			insertIdx++;

		// if too many children then we need to split and tell our parent
		statsGlobal.cmp++; // child count check
		if (node->childCount == BPT_ORDER)
			return _splitInternal(node, newChild, insertIdx);

		// else shift children to add new entry
		for (size_t i = node->childCount - 1; i > insertIdx; i--) {
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
		node->keys[insertIdx] = newChild->keys[0];
		node->childCount++;
		statsGlobal.add++; // increment child count
		statsGlobal.mem++; // store new child

		return NULL; // no new siblings for parent to be aware of
	}
}

// Start insertion from the root. Also handles root splitting.
bool bptSet(Tensor * T, tCoord_t * coords, float value) {
	if (!T || !T->values || !coords)
		return false;
	/*
	printf("\nInsert %f at ", value);
	coordsPrint(T, coords);
	putchar('\n');

	while ('\n' != getchar())
	    ;
	*/

	statsGlobal.mem++; // get root node
	bptNode * root = T->values;
	tKey_t key = _Coords2Key(T, coords);
	bptNode * rootSibling = _insert(T, root, key, value);

	bool store_new_root = false;

	// todo: is this actually needed?
	statsGlobal.cmp++;
	if (key < root->keys[0]) {
		store_new_root = true;
		root->keys[0] = key;
	}

	if (rootSibling) {
		// root node split during insertion, so integrate new node
		bptNode * newRoot = calloc(sizeof(bptNode), 1);
		newRoot->isLeaf = false;
		newRoot->childCount = 2;
		newRoot->children[0] = root;
		newRoot->children[1] = rootSibling;
		newRoot->keys[0] = root->keys[0];
		newRoot->keys[1] = rootSibling->keys[0];
		T->values = newRoot;
		store_new_root = true;
	}
	// bptPrintAll(T);
	if (store_new_root)
		statsGlobal.mem++; // something needed to be updated
	return true;            // insertion success
};

// todo: remove tensor argument since it's only for passing to debug print
float _search(Tensor * T, bptNode * node, tKey_t key) {
	if (!node)
		return 0;
	if (node->isLeaf) {
		// todo: make this a binary search
		for (size_t i = 0; i < node->childCount; i++) {
			statsGlobal.cmp++;
			if (node->keys[i] == key)
				return node->values[i];
			if (node->keys[i] > key)
				break;
		}
		return 0;
	} else { // node is internal
		// todo: make this a binary search
		for (size_t i = 1; i < node->childCount; i++) {
			statsGlobal.cmp++;
			if (key < node->keys[i]) {
				statsGlobal.mem++; // fetch child
				return _search(T, node->children[i - 1], key);
			}
		}
		//  if it's not in the other children, it might be in the last one
		statsGlobal.mem++; // fetch child
		return _search(T, node->children[node->childCount - 1], key);
	}
}
float bptGet(Tensor * T, tCoord_t * coords) {
	if (!T || !T->values || !coords)
		return 0;

	statsGlobal.mem++; // get root
	bptNode * root = T->values;
	tKey_t key = _Coords2Key(T, coords);
	return _search(T, root, key);
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
				printf("\x1b[90m");
			_printKey(T, node->keys[i]);
			printf(": %f\n", node->values[i]);
			printf("\x1b[0m");
		}
	} else {
		for (size_t i = 0; i < BPT_ORDER; i++) {
			for (uint i = 0; i < depth; i++)
				putchar('\t');
			if (i >= node->childCount)
				printf("\x1b[90m");
			printf("> child with minimum ");
			_printKey(T, node->keys[i]);
			putchar('\n');
			_print(T, node->children[i], depth + 1);
			printf("\x1b[0m");
		}
	}
}

void bptPrintAll(Tensor * T) {
	bptNode * root = T->values;
	printf("raw B+ Tree (%p->%p) contents:\n", T, T->values);
	if (!root) {
		printf("\tThere's no root!\n");
		return;
	}
	_print(T, root, 0);
	putchar('\n');
}

// todo: consider making the iterator stack a flat array
// instead of a linked list. Decreases pointer chasing.
// How to tell appropriate length of list??
// There's no way to see the maximum tree depth right now.
typedef struct bptIterRecord {
	size_t childIdx;
	struct bptIterRecord * parent; // this linked list is a stack
	struct bptNode * node;         // don't traverse B+ tree
} bptIterRecord;
typedef struct bptContext {
	bptIterRecord * stack;
	tCoord_t * coords;
} bptContext;


static size_t _bptNodeCount(bptNode * node) {
	if (node->isLeaf)
		return 1;

	size_t sum = 1; // count yourself

	// plus all the children
	for (int i = 0; i < node->childCount; i++)
		sum += _bptNodeCount(node->children[i]);
	return sum;
}

size_t bptSize(Tensor * T) {
	bptNode * root = T->values;
	return _bptNodeSize * _bptNodeCount(root);
}


void * bptIteratorInit(Tensor * T) {
	// bptPrintAll(T);
	if (!T || !T->values)
		return NULL;
	bptContext * ctx = calloc(sizeof(bptContext), 1);
	if (!ctx)
		return NULL;
	ctx->coords = calloc(sizeof(tCoord_t), T->order);
	if (!ctx->coords) {
		free(ctx);
		return NULL;
	}
	ctx->stack = calloc(sizeof(bptIterRecord), 1);
	if (!ctx->stack) {
		free(ctx->coords);
		free(ctx);
		return NULL;
	}

	ctx->stack->node = T->values;
	// traverse to the first leaf node
	bptIterRecord * top = ctx->stack;
	statsGlobal.mem++; // fetch root
	while (!top->node->isLeaf) {
		bptIterRecord * newTop = calloc(sizeof(bptIterRecord), 1);
		if (!newTop) {
			bptIteratorCleanup(ctx);
			return NULL;
		}
		statsGlobal.mem++; // fetch next node
		newTop->node = top->node->children[0];
		newTop->parent = top;
		top = newTop;
	}
	ctx->stack = top;
	return ctx;
}

void bptIteratorCleanup(void * context) {
	bptContext * ctx = context;
	if (!ctx)
		return;

	// free the whole context stack from the tail in
	bptIterRecord * top = ctx->stack;
	while (top->parent) {
		ctx->stack = top->parent;
		free(top);
		top = ctx->stack;
	}
	free(ctx->stack);
	free(ctx->coords);
	free(ctx);
}

tensorEntry bptIteratorNext(Tensor * T, void * context) {
	bptContext * ctx = context;
	bptIterRecord * top = ctx->stack;
	// have we run out of values in this node yet?
	statsGlobal.cmp++;
	if (top->childIdx < top->node->childCount) {
		// more values here, so just increment and return

		// once we hit a leaf, return it
		_Key2Coords(T, ctx->coords, top->node->keys[top->childIdx]);
		float val = top->node->values[top->childIdx];
		statsGlobal.add++;
		top->childIdx++;
		return (tensorEntry){.coords = ctx->coords, .value = val};
	}

	// pop and free as needed until we can increment childIdx
	statsGlobal.cmp++;
	while (top->childIdx == top->node->childCount - (!top->node->isLeaf)) {
		// are we done iterating?
		if (!top->parent)
			return (tensorEntry){0};

		top = top->parent;
		free(ctx->stack);
		ctx->stack = top;
		statsGlobal.cmp++; // while condition
	}

	// increment childIdx
	statsGlobal.add++;
	top->childIdx++;

	// traverse to the next leaf
	while (!top->node->isLeaf) {
		bptIterRecord * newTop = calloc(sizeof(bptIterRecord), 1);
		statsGlobal.mem++; // fetch child
		newTop->node = top->node->children[top->childIdx];
		newTop->parent = top;
		top = newTop;
	}
	ctx->stack = top;

	// first entry of new leaf, and point to next entry
	_Key2Coords(T, ctx->coords, top->node->keys[top->childIdx]);
	float val = top->node->values[top->childIdx];
	top->childIdx++;
	return (tensorEntry){.coords = ctx->coords, .value = val};
}

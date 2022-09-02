#include "bpTree.h"
#include "hashtable.h"
#include "stats.h"
#include "tensor.h"
#include "tensorMath.h"
#include <stdio.h>

int main(int argc, char ** argv) {
	statsReset();
	printf("Input tensor A:\n");
	Tensor * A = tensorRead(BPlusTree, "../B.coo");
	tensorPrintMetadata(A);
	statsPrint(statsGet());

	statsReset();
	printf("\nInput tensor B:\n");
	Tensor * B = tensorRead(probingHashtable, "../B.coo");
	tensorPrintMetadata(B);
	statsPrint(statsGet());

	if (!A || !B) {
		printf("Error. Exiting.\n");
		tensorFree(A);
		tensorFree(B);
		return 1;
	}
	Tensor * C = {0};

	/*
	tensorIterator iter = htIterator;
	void * context = iter.init(B);
	tensorEntry item = iter.next(B, context);
	float val1, val2;
	while (item.coords != 0) {
	    val1 = item.value;
	    val2 = tensorGet(A, item.coords);
	    if (val1 != val2) {
	        putchar('B');
	        coordsPrint(B, item.coords);
	        printf(" = %f, and A[\"] = %f\n", val1, val2);
	    }
	    item = iter.next(B, context);
	}
	iter.cleanup(context);
	*/

	putchar('\n');
	for (int i = 0; i < 80; i++)
		putchar('-');
	putchar('\n');

	/*
	printf("\nTrace A with 0, 1 yields\n");
	statsReset();
	C = tensorTrace(BPlusTree, A, 0, 1);
	tensorPrintMetadata(C);
	statsPrint(statsGet());
	ht_capacity = C->entryCount;
	tensorFree(C);

	printf("\nTrace B with 0, 1 yields\n");
	statsReset();
	C = tensorTrace(probingHashtable, B, 0, 1);
	tensorPrintMetadata(C);
	statsPrint(statsGet());
	tensorFree(C);

	putchar('\n');
	for (int i = 0; i < 80; i++)
	    putchar('-');
	putchar('\n');
	*/

	printf("\nContraction on 0, 1 yields\n");
	statsReset();
	C = tensorContract(BPlusTree, A, A, 0, 1);
	tensorPrintMetadata(C);
	Stats bptStats = statsGet();
	size_t bptSize = tensorSize(C);
	statsPrint(bptStats);
	ht_capacity = C->entryCount;
	tensorFree(C);

	printf("\nContraction on 0, 1 yields\n");
	statsReset();
	C = tensorContract(probingHashtable, B, B, 0, 1);
	tensorPrintMetadata(C);
	Stats htStats = statsGet();
	size_t htSize = tensorSize(C);
	statsPrint(htStats);
	tensorFree(C);

	putchar('\n');
	for (int i = 0; i < 80; i++)
		putchar('-');
	putchar('\n');

	puts("Configuration summary:");
	printf("  B+ tree branching factor: %i\n", BPT_ORDER);
	printf("  Hash table overprovision factor: %0.2f\n", HT_OVERPROVISION);
	printf("  Input tensor size: %lu nnz\n", A->entryCount);
	printf("  Output tensor size: %lu nnz\n\n", ht_capacity);

	puts("B+ Tree performance compared to hash table:");
	printf("  RAM transactions: %0.2f%%\n",
	       (float)100 * bptStats.mem / htStats.mem);
	printf("  ALU operations:   %0.2f%%\n",
	       (float)100 * (bptStats.add + bptStats.cmp + bptStats.mul) /
	           (htStats.add + htStats.cmp + htStats.mul));
	printf("  Data structure:   %0.2f%%\n", (float)100 * bptSize / htSize);

	tensorFree(A);
	tensorFree(B);
	return 0;
}

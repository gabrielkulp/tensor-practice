#include "bpTree.h"
#include "hashtable.h"
#include "tensor.h"
#include "tensorMath.h"
#include <stdio.h>

int main(int argc, char ** argv) {
	printf("Input tensor A:\n");
	Tensor * A = tensorRead(probingHashtable, "../B.coo");
	// tensorPrint(A);
	printf("\nInput tensor B:\n");
	Tensor * B = tensorRead(BPlusTree, "../B.coo");
	// tensorPrint(B);
	putchar('\n');

	if (!A || !B) {
		printf("Error. Exiting.\n");
		tensorFree(A);
		tensorFree(B);
		return 1;
	}
	// Tensor * C = {0};

	tensorIterator iter = htIterator;
	void * context = iter.init(A);
	tensorEntry item = iter.next(A, context);
	float val1, val2;
	while (item.coords != 0) {
		val1 = item.value;
		val2 = tensorGet(B, item.coords);
		if (val1 != val2) {
			putchar('A');
			coordsPrint(A, item.coords);
			printf(" = %f, and B[\"] = %f\n", val1, val2);
		}
		item = iter.next(A, context);
	}
	iter.cleanup(context);
	/*
	    printf("\nTrace with 0, 1 is\n");
	    C = tensorTrace(BPlusTree, A, 0, 1);
	    tensorPrint(C);
	    tensorFree(C);

	    printf("\nTrace with 0, 2 is\n");
	    C = tensorTrace(BPlusTree, A, 0, 2);
	    tensorPrint(C);
	    tensorFree(C);

	    printf("\nTrace with 1, 2 is\n");
	    C = tensorTrace(BPlusTree, A, 1, 2);
	    tensorPrint(C);
	    tensorFree(C);

	    printf("\nContraction on 0, 1 is\n");
	    C = tensorContract(BPlusTree, A, B, 0, 1);
	    tensorPrint(C);
	    tensorWrite(C, "C.coo");
	*/
	tensorFree(A);
	tensorFree(B);
	//	tensorFree(C);
	return 0;
}

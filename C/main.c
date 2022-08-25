#include "bpTree.h"
#include "hashtable.h"
#include "tensor.h"
#include "tensorMath.h"
#include <stdio.h>

int main(int argc, char ** argv) {
	printf("Input tensor:\n");
	Tensor * A = tensorRead(BPlusTree, "../T.coo");
	Tensor * B = tensorRead(probingHashtable, "../T.coo");
	if (!A || !B) {
		printf("Error. Exiting.\n");
		tensorFree(A);
		tensorFree(B);
		return 1;
	}
	Tensor * C = {0};
	tensorPrint(A);
	tensorPrint(B);
	/*
	    void * context = htIteratorInit(B);
	    tensorEntry item = htIteratorNext(B, context);
	    while (item.coords != 0) {
	        putchar('B');
	        coordsPrint(B, item.coords);
	        printf(" = %f, and A[\"] = %f\n", item.value,
	               tensorGet(A, item.coords));
	        item = htIteratorNext(B, context);
	    }
	    htIteratorCleanup(context);
	*/
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
	C = tensorContract(probingHashtable, A, B, 0, 1);
	tensorPrint(C);
	// tensorWrite(C, "C.coo");

	tensorFree(A);
	tensorFree(B);
	tensorFree(C);
	return 0;
}

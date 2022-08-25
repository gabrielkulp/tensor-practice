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

	void * context = htIteratorInit(B);
	tensorEntry item = htIteratorNext(B, context);
	while (item.coords != 0) {
		putchar('\n');
		putchar('B');
		coordsPrint(B, item.coords);
		printf(" = %f, and A[\"] = \n", item.value);
		float val = tensorGet(A, item.coords);
		if (val == 0)
			printf("\e[0;31m%f\e[0m\n", val);
		else
			printf("%f\n", val);
		item = htIteratorNext(B, context);
	}
	htIteratorCleanup(context);
	/*
	    printf("\nTrace with 0, 1 is\n");
	    C = tensorTrace(probingHashtable, A, 0, 1);
	    tensorPrint(C);
	    tensorFree(C);

	    printf("\nTrace with 0, 2 is\n");
	    C = tensorTrace(probingHashtable, A, 0, 2);
	    tensorPrint(C);
	    tensorFree(C);

	    printf("\nTrace with 1, 2 is\n");
	    C = tensorTrace(probingHashtable, A, 1, 2);
	    tensorPrint(C);
	    tensorFree(C);

	    printf("\nContraction on 0, 1 is\n");
	    C = tensorContract(probingHashtable, A, B, 0, 1);
	    tensorPrint(C);
	    // tensorWrite(C, "C.coo");
	*/
	tensorFree(A);
	tensorFree(B);
	// tensorFree(C);
	return 0;
}

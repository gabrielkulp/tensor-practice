#include "bpTree.h"
#include "hashtable.h"
#include "stats.h"
#include "tensor.h"
#include "tensorMath.h"
#include <stdio.h>

int main(int argc, char ** argv) {
	statsReset();
	printf("Input tensor A:\n");
	Tensor * A = tensorRead(probingHashtable, "../B.coo");
	tensorPrintMetadata(A);
	statsPrint(statsGet());

	statsReset();
	printf("\nInput tensor B:\n");
	Tensor * B = tensorRead(BPlusTree, "../B.coo");
	tensorPrintMetadata(B);
	statsPrint(statsGet());

	if (!A || !B) {
		printf("Error. Exiting.\n");
		tensorFree(A);
		tensorFree(B);
		return 1;
	}
	Tensor * C = {0};

	putchar('\n');
	for (int i = 0; i < 80; i++)
		putchar('-');
	putchar('\n');

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
	
	printf("\nTrace A with 0, 1 is\n");
	statsReset();
	C = tensorTrace(probingHashtable, A, 0, 1);
	tensorPrintMetadata(C);
	statsPrint(statsGet());
	tensorFree(C);

	printf("\nTrace B with 0, 1 is\n");
	statsReset();
	C = tensorTrace(BPlusTree, B, 0, 1);
	tensorPrintMetadata(C);
	statsPrint(statsGet());
	tensorFree(C);

	putchar('\n');
	for (int i = 0; i < 80; i++)
		putchar('-');
	putchar('\n');

	printf("\nContraction on 0, 1 is\n");
	statsReset();
	C = tensorContract(probingHashtable, A, B, 0, 1);
	tensorPrintMetadata(C);
	statsPrint(statsGet());
	tensorFree(C);

	printf("\nContraction on 0, 1 is\n");
	statsReset();
	C = tensorContract(BPlusTree, A, B, 0, 1);
	tensorPrintMetadata(C);
	statsPrint(statsGet());
	tensorFree(C);

	//tensorWrite(C, "C.coo");
	
	tensorFree(A);
	tensorFree(B);
	// tensorFree(C);
	return 0;
}

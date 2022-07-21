#include <stdio.h>
#include "tensorMath.h"

int main(int argc, char ** argv) {
	printf("Input tensor:\n");
	Tensor * A = tensorRead("../T.coo");
	Tensor * B = tensorRead("../T.coo");
	if (!A || !A->valid || !B || !B->valid) {
		printf("Error. Exiting.\n");
		tensorFree(A);
		tensorFree(B);
		return 1;
	}
	Tensor * C;
	tensorPrint(A);

	printf("\nTrace with 0, 1 is\n");
	C = tensorTrace(A, 0, 1);
	tensorPrint(C);
	tensorFree(C);

	printf("\nTrace with 0, 2 is\n");
	C = tensorTrace(A, 0, 2);
	tensorPrint(C);
	tensorFree(C);

	printf("\nTrace with 1, 2 is\n");
	C = tensorTrace(A, 1, 2);
	tensorPrint(C);
	tensorFree(C);

	printf("\nContraction on 0, 1 is\n");
	C = tensorContract(A, B, 0, 1);
	tensorPrint(C);
	tensorWrite(C, "C.coo");

	tensorFree(A);
	tensorFree(B);
	tensorFree(C);
	return 0;
}

/*
#include "tensor.h"
void tensorTest() {
	unsigned short order = 2;
	unsigned int shape[2] = {4, 4};
	size_t size = 8;

	unsigned int coords[2] = {0, 0};
	float value;

	Tensor * T = tensorNew(order, shape, size);

	printf("Tensor interactive test. Commands are:\n");
	printf("sgpq = set, get, print, quit\n\n");
	char c;
	while (true) {
		printf("\nsgpq > ");
		scanf("%c", &c);
		switch (c) {
			case 's':
				printf("coords=");
				for (unsigned short mode = 0; mode < T->order; mode++) {
					scanf("%u", &coords[mode]);
				}
				printf("value=");
				scanf("%f", &value);
				printf("%i\n", tensorSet(T, coords, value));
				break;
			case 'g':
				printf("coords=");
				for (unsigned short mode = 0; mode < T->order; mode++) {
					scanf("%u", &coords[mode]);
				}
				printf("%f\n", tensorGet(T, coords));
				break;
			case 'p':
				tensorPrint(T);
				break;
			case 'q':
				tensorFree(T);
				return;
		}
		scanf("%c", &c); // gobble newline
	}
}
*/

/*
#include "hashtable.h"
void hashtableTest() {
	htKey_t key;
	float value;

	Hashtable * ht = htNew(8);

	printf("Hashtable interactive test. Commands are:\n");
	printf("sgpq = set, get, print, quit\n\n");
	char c;
	while (true) {
		printf("\nsgpq > ");
		scanf("%c", &c);
		switch (c) {
			case 's':
				printf("key=");
				scanf("%lu", &key);
				printf("value=");
				scanf("%f", &value);
				printf("%i\n", htSet(ht, key, value));
				break;
			case 'g':
				printf("key=");
				scanf("%lu", &key);
				printf("%f\n", htGet(ht, key));
				break;
			case 'p':
				htPrintAll(ht);
				break;
			case 'q':
				htFree(ht);
				return;
		}
		scanf("%c", &c); // gobble newline
	}
}
*/

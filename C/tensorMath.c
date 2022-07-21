#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "tensorMath.h"

Tensor * tensorTrace(Tensor * T, tMode_t a, tMode_t b) {
	if (!T || !T->valid) {
		printf("Tried to calculate trace of invalid tensor\n");
		return 0;
	}
	if (T->order < 2 || a >= T->order || b >= T->order) {
		printf("Trace modes out of range\n");
		return 0;
	}
	if (a == b || T->shape[a] != T->shape[b]) {
		printf("Tried to trace incompatible modes\n");
		return 0;
	}

	// contsruct shape of result tensor
	tCoord_t * CShape = calloc(T->order-2, sizeof(tCoord_t));
	if (!CShape) {
		printf("failed to allocate\n");
		free(CShape);
		return 0;
	}
	tMode_t CMode = 0;
	for (tMode_t m = 0; m < T->order; m++) {
		if (m == a || m == b)
			continue;
		CShape[CMode] = T->shape[m];
		CMode++;
	}

	// allocate tensors and iteration coordinates
	Tensor * C = tensorNew(T->order-2, CShape, 8); // todo: calculate init size
	tCoord_t * CCoords = calloc(C->order, sizeof(tCoord_t));
	tCoord_t * TCoords = calloc(T->order, sizeof(tCoord_t));
	free(CShape);
	if (!TCoords || !CCoords || !C || !C->valid) {
		printf("failed to allocate\n");
		tensorFree(C);
		free(CCoords);
		free(TCoords);
		return 0;
	}

	// iterate through all the coordinates
	bool done = (C->order == 0);
	while (true) {
		float accumulator = 0;
		// actual trace logic
		for (tCoord_t k = 0; k < T->shape[a]; k++) {
			TCoords[a] = k;
			TCoords[b] = k;
			accumulator += tensorGet(T, TCoords);
		}
		bool success = tensorSet(C, CCoords, accumulator);
		if (!success) {
			printf("failed to insert value\n");
			tensorFree(C);
			free(CCoords);
			free(TCoords);
			return 0;
		}

		// get next coordinates or finish
		CMode = 0;
		for (tMode_t mode = 0; CMode < C->order; mode++) {
			if (mode == a || mode == b)
				continue;
			CCoords[CMode]++;
			TCoords[mode]++;
			if (TCoords[mode] == T->shape[mode]) {
				if (CMode == C->order-1) {
					done = true;
					break;
				}
				TCoords[mode] = 0;
				CCoords[CMode] = 0;
				CMode++;
				continue;
			}
			break;
		}
		if (done)
			break;
	}
	free(TCoords);
	free(CCoords);
	return C;
}

Tensor * tensorContract(Tensor * A, Tensor * B, tMode_t a, tMode_t b) {
	if (!A || !A->valid || !B || !B->valid)
		return 0;
	if (!A->order || !B->order || a > A->order || b > B->order)
		return 0;
	if (A->shape[a] != B->shape[b])
		return 0;
	
	// construct shape of result tensor
	tCoord_t * CShape = calloc(A->order+B->order-2, sizeof(tCoord_t));
	tMode_t CMode = 0;
	for (tMode_t m = 0; m < A->order; m++) {
		if (m == a)
			continue;
		CShape[CMode] = A->shape[m];
		CMode++;
	}
	for (tMode_t m = 0; m < B->order; m++) {
		if (m == b)
			continue;
		CShape[CMode] = B->shape[m];
		CMode++;
	}

	// allocate tensor and iteration coordinates
	Tensor * C = tensorNew(A->order+B->order-2, CShape, 128); // todo: calculate init size
	tCoord_t * ACoords = calloc(A->order, sizeof(tCoord_t));
	tCoord_t * BCoords = calloc(B->order, sizeof(tCoord_t));
	tCoord_t * CCoords = calloc(C->order, sizeof(tCoord_t));
	free(CShape);
	if (!ACoords || !BCoords || !CCoords || !C || !C->valid) {
		printf("failed to allocate\n");
		tensorFree(C);
		free(ACoords);
		free(BCoords);
		free(CCoords);
		return 0;
	}

	// iterate through all the coordinates
	bool done = (C->order == 0);
	while (true) {
		float accumulator = 0;
		// actual contraction logic
		for (tCoord_t k = 0; k < A->shape[a]; k++) {
			ACoords[a] = k;
			BCoords[b] = k;
			accumulator += tensorGet(A, ACoords) * tensorGet(B, BCoords);
		}
		if (accumulator != 0) {
			bool success = tensorSet(C, CCoords, accumulator);
			if (!success) {
				printf("failed to insert value\n");
				tensorFree(C);
				free(ACoords);
				free(BCoords);
				free(CCoords);
				return 0;
			}
		}

		// get next coordinates or finish
		CMode = 0;
		tMode_t BMode;
		for (tMode_t mode = 0; CMode < C->order; mode++) {
			BMode = mode - (A->order);
			if (mode == a || BMode == b)
				continue;
			CCoords[CMode]++;
			if (CMode < A->order-1) { // increment A part of C coord
				ACoords[mode]++;
				if (ACoords[mode] == A->shape[mode]) {
					ACoords[mode] = 0;
					CCoords[CMode] = 0;
					CMode++;
					continue;
				}
				break;
			} else { // increment B part of C coord
				BCoords[BMode]++;
				if (BCoords[BMode] == B->shape[BMode]) {
					if (CMode == C->order-1) {
						done = true;
						break;
					}
					BCoords[BMode] = 0;
					CCoords[CMode] = 0;
					CMode++;
					continue;
				}
				break;
			}
		}
		if (done)
			break;
	}

	free(ACoords);
	free(BCoords);
	free(CCoords);
	return C;
}

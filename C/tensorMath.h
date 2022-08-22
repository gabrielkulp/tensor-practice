#pragma once
#include "tensor.h"

Tensor * tensorTrace(enum storageType type, Tensor * T, tMode_t a, tMode_t b);
Tensor * tensorContract(enum storageType type, Tensor * A, Tensor * B,
                        tMode_t a, tMode_t b);

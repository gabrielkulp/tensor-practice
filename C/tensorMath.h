#include "tensor.h"

Tensor * tensorTrace(enum storage_type type, Tensor * T, tMode_t a, tMode_t b);
Tensor * tensorContract(enum storage_type type, Tensor * A, Tensor * B,
                        tMode_t a, tMode_t b);

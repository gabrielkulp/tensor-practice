#!/usr/bin/env python3
import itertools


class Tensor:
    def __init__(self, shape: tuple, values: dict, validate=True):
        self.shape = shape
        self.values = {}

        if validate:
            for mode_size in shape:
                assert mode_size > 0
            for pos in values:
                assert type(pos) == tuple
                assert len(pos) == len(shape)
                for index, mode_size in zip(pos, shape):
                    assert index >= 0
                    assert index < mode_size
                assert type(values[pos]) in (int, float)
                assert pos not in self.values
                self.values[pos] = values[pos]

    def __repr__(self):
        s = f"Tensor(shape={self.shape}, values=\n"
        for key in self.values:
            s += f"    {str(key)}: {str(self.values[key])}\n"
        s += ")\n"
        return s

    def trace(self, mode_a, mode_b):
        assert mode_a < len(self.shape)
        assert mode_b < len(self.shape)
        assert self.shape[mode_a] == self.shape[mode_b]

        # set contract modes to a size of 1 for outer iteration
        free_mode_sizes = list(self.shape)
        free_mode_sizes[mode_a] = 1
        free_mode_sizes[mode_b] = 1

        acc_shape = list(self.shape)
        del acc_shape[max(mode_a, mode_b)]
        del acc_shape[min(mode_a, mode_b)]
        acc_shape = tuple(acc_shape)

        accumulator = {}
        for coords in itertools.product(*tuple(map(range, free_mode_sizes))):
            for k in range(self.shape[mode_a]):
                key = list(coords)
                key[mode_a] = k
                key[mode_b] = k
                key = tuple(key)

                if key not in self.values:
                    continue

                acc_key = list(coords)
                del acc_key[max(mode_a, mode_b)]
                del acc_key[min(mode_a, mode_b)]
                acc_key = tuple(acc_key)

                if acc_key not in accumulator:
                    accumulator[acc_key] = 0
                accumulator[acc_key] += self.values[key]
        return Tensor(acc_shape, accumulator)


def contract(A: Tensor, B: Tensor, contract_mode_a, contract_mode_b):
    assert A.shape[contract_mode_a] == B.shape[contract_mode_b]

    free_modes_a = list(A.shape)
    free_modes_a[contract_mode_a] = 1

    free_modes_b = list(B.shape)
    free_modes_b[contract_mode_b] = 1

    acc_shape = list(A.shape) + list(B.shape)
    del acc_shape[len(A.shape) + contract_mode_b]
    del acc_shape[contract_mode_a]
    acc_shape = tuple(acc_shape)

    accumulator = {}
    for coords_a in itertools.product(*tuple(map(range, free_modes_a))):
        for coords_b in itertools.product(*tuple(map(range, free_modes_b))):
            for k in range(A.shape[contract_mode_a]):
                key_a = list(coords_a)
                key_a[contract_mode_a] = k
                key_a = tuple(key_a)
                if key_a not in A.values:
                    continue

                key_b = list(coords_b)
                key_b[contract_mode_b] = k
                key_b = tuple(key_b)
                if key_b not in B.values:
                    continue

                acc_key = list(coords_a) + list(coords_b)
                del acc_key[len(A.shape) + contract_mode_b]
                del acc_key[contract_mode_a]
                acc_key = tuple(acc_key)

                if acc_key not in accumulator:
                    accumulator[acc_key] = 0
                accumulator[acc_key] += A.values[key_a] * B.values[key_b]
    return Tensor(acc_shape, accumulator)


A = Tensor(
    (3, 4, 4, 2),
    {
        (0, 0, 0, 0): 6,
        (0, 2, 0, 0): 9,
        (0, 3, 0, 0): 8,
        (2, 0, 0, 0): 5,
        (2, 3, 0, 0): 7,
        (0, 0, 3, 0): 6,
        (0, 2, 2, 1): 9,
        (0, 3, 3, 0): 8,
        (2, 0, 3, 1): 5,
        (2, 3, 1, 0): 7,
        (0, 0, 1, 1): 6,
        (0, 2, 0, 0): 9,
        (0, 3, 2, 1): 8,
        (2, 0, 2, 0): 5,
        (2, 3, 0, 1): 7
    }
)
print("A =", A)
print("Tr(A_jk) =", A.trace(1, 2))

A = Tensor(
    (3, 3),
    {
        (0, 0): 1,
        (1, 1): 2,
        (2, 2): 4,
        (0, 2): 3,
        (2, 0): 1,
        (1, 2): 2
    }
)
B = Tensor(
    (3, 3),
    {
        (0, 0): 3,
        (1, 0): 6,
        (2, 0): 9
    }
)
print("A =", A)
print("B =", B)
print("AB =", contract(A, B, 0, 0))

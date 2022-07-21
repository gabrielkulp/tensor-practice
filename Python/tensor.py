#!/usr/bin/env python3

import numpy as np

A = np.random.rand(3, 3)  # ij
B = np.random.rand(3, 3)  # jk
# C = np.ones((5,4,2,4))

# C_ik = Sum_j A_ij B_jk
C = np.zeros((A.shape[0], B.shape[1]))
assert A.shape[1] == B.shape[0]
contraction_len = A.shape[1]

for i in range(C.shape[0]):
    for k in range(C.shape[1]):
        # inner loop contraction
        acc = 0
        for j in range(contraction_len):
            acc += A[i][j] * B[j][k]
            C[i][k] = acc


def print_matrix(mat):
    for j in range(mat.shape[1]):
        print(" ".join([str(round(x, 4)) for x in mat[j]]))


def contract(A, B, a_contract_mode, b_contract_mode):
    # check if this contraction happens within a single tensor
    if id(A) == id(B):
        return np.einsum(A, [a_contract_mode, b_contract_mode])

    return np.tensordot(
        A, B,
        axes=(a_contract_mode, b_contract_mode)
    )


print("A is")
print_matrix(A)
print("\n\nB is")
print_matrix(B)
print("\n\nC is")
print_matrix(C)


print("-----")
print("np contraction returns")
print_matrix(contract(A, B, 1, 0))

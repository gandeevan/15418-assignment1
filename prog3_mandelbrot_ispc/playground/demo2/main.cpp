#include "sum_ispc.h"
#include <stdio.h>
#include <cstdlib>

#define N 16

int main() {
    int *a, *b, *result;
    a = (int *) malloc(N * sizeof(int));
    b = (int *) malloc(N * sizeof(int));
    result = (int *) malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i;
        result[i] = 0;
    }
    ispc::sum(a, b, result, N);
}
#include <stdio.h>
#include <stdint.h>
#include "sum_ispc.h"

int main() {
    int a[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int b[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int result[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    ispc::sum(a, b, result);
}
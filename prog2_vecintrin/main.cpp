#include <stdio.h>
#include <algorithm>
#include <getopt.h>
#include <math.h>
#include "CMU418intrin.h"
#include "logger.h"
using namespace std;

#define EXP_MAX 10

Logger CMU418Logger;

void usage(const char* progname);
void initValue(float* values, int* exponents, float* output, float* gold, unsigned int N);
void absSerial(float* values, float* output, int N);
void absVector(float* values, float* output, int N);
void clampedExpSerial(float* values, int* exponents, float* output, int N);
void clampedExpVector(float* values, int* exponents, float* output, int N);
float arraySumSerial(float* values, int N);
float arraySumVector(float* values, int N);
bool verifyResult(float* values, int* exponents, float* output, float* gold, int N);

int main(int argc, char * argv[]) {
  int N = 16;
  bool printLog = false;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {
    {"size", 1, 0, 's'},
    {"log", 0, 0, 'l'},
    {"help", 0, 0, '?'},
    {0 ,0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "s:l?", long_options, NULL)) != EOF) {

    switch (opt) {
      case 's':
        N = atoi(optarg);
        if (N <= 0) {
          printf("Error: Workload size is set to %d (<0).\n", N);
          return -1;
        }
        break;
      case 'l':
        printLog = true;
        break;
      case '?':
      default:
        usage(argv[0]);
        return 1;
    }
  }


  float* values = new float[N+VECTOR_WIDTH];
  int* exponents = new int[N+VECTOR_WIDTH];
  float* output = new float[N+VECTOR_WIDTH];
  float* gold = new float[N+VECTOR_WIDTH];
  initValue(values, exponents, output, gold, N);

  clampedExpSerial(values, exponents, gold, N);
  clampedExpVector(values, exponents, output, N);

  //absSerial(values, gold, N);
  //absVector(values, output, N);

  printf("\e[1;31mCLAMPED EXPONENT\e[0m (required) \n");
  bool clampedCorrect = verifyResult(values, exponents, output, gold, N);
  if (printLog) CMU418Logger.printLog();
  CMU418Logger.printStats();

  printf("************************ Result Verification *************************\n");
  if (!clampedCorrect) {
    printf("@@@ Failed!!!\n");
  } else {
    printf("Passed!!!\n");
  }

  printf("\n\e[1;31mARRAY SUM\e[0m (bonus) \n");
  if (N % VECTOR_WIDTH == 0) {
    float sumGold = arraySumSerial(values, N);
    float sumOutput = arraySumVector(values, N);
    float epsilon = 0.1;
    bool sumCorrect = abs(sumGold - sumOutput) < epsilon * 2;
    if (!sumCorrect) {
      printf("Expected %f, got %f\n.", sumGold, sumOutput);
      printf("@@@ Failed!!!\n");
    } else {
      printf("Passed!!!\n");
    }
  } else {
    printf("Must have N % VECTOR_WIDTH == 0 for this problem (VECTOR_WIDTH is %d)\n", VECTOR_WIDTH);
  }

  delete[] values;
  delete[] exponents;
  delete[] output;
  delete gold;

  return 0;
}

void usage(const char* progname) {
  printf("Usage: %s [options]\n", progname);
  printf("Program Options:\n");
  printf("  -s  --size <N>     Use workload size N (Default = 16)\n");
  printf("  -l  --log          Print vector unit execution log\n");
  printf("  -?  --help         This message\n");
}

void initValue(float* values, int* exponents, float* output, float* gold, unsigned int N) {

  for (unsigned int i=0; i<N+VECTOR_WIDTH; i++)
  {
    // random input values
    values[i] = -1.f + 4.f * static_cast<float>(rand()) / RAND_MAX;
    exponents[i] = rand() % EXP_MAX;
    output[i] = 0.f;
    gold[i] = 0.f;
  }

}

bool verifyResult(float* values, int* exponents, float* output, float* gold, int N) {
  int incorrect = -1;
  float epsilon = 0.00001;
  for (int i=0; i<N+VECTOR_WIDTH; i++) {
    if ( abs(output[i] - gold[i]) > epsilon ) {
      incorrect = i;
      break;
    }
  }

  if (incorrect != -1) {
    if (incorrect >= N)
      printf("You have written to out of bound value!\n");
    printf("Wrong calculation at value[%d]!\n", incorrect);
    printf("value  = ");
    for (int i=0; i<N; i++) {
      printf("% f ", values[i]);
    } printf("\n");

    printf("exp    = ");
    for (int i=0; i<N; i++) {
      printf("% 9d ", exponents[i]);
    } printf("\n");

    printf("output = ");
    for (int i=0; i<N; i++) {
      printf("% f ", output[i]);
    } printf("\n");

    printf("gold   = ");
    for (int i=0; i<N; i++) {
      printf("% f ", gold[i]);
    } printf("\n");
    return false;
  }
  printf("Results matched with answer!\n");
  return true;
}

void absSerial(float* values, float* output, int N) {
  for (int i=0; i<N; i++) {
    float x = values[i];
    if (x < 0) {
      output[i] = -x;
    } else {
      output[i] = x;
    }
  }
}

// implementation of absolute value using 15418 instrinsics
void absVector(float* values, float* output, int N) {
  __cmu418_vec_float x;
  __cmu418_vec_float result;
  __cmu418_vec_float zero = _cmu418_vset_float(0.f);
  __cmu418_mask maskAll, maskIsNegative, maskIsNotNegative;

//  Note: Take a careful look at this loop indexing.  This example
//  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
//  Why is that the case?
  for (int i=0; i<N; i+=VECTOR_WIDTH) {

    // All ones
    maskAll = _cmu418_init_ones();

    // All zeros
    maskIsNegative = _cmu418_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _cmu418_vload_float(x, values+i, maskAll);               // x = values[i];

    // Set mask according to predicate
    _cmu418_vlt_float(maskIsNegative, x, zero, maskAll);     // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _cmu418_vsub_float(result, zero, x, maskIsNegative);      //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _cmu418_mask_not(maskIsNegative);     // } else {

    // Execute instruction ("else" clause)
    _cmu418_vload_float(result, values+i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _cmu418_vstore_float(output+i, result, maskAll);
  }
}

// accepts and array of values and an array of exponents
//
// For each element, compute values[i]^exponents[i] and clamp value to
// 9.999.  Store result in outputs.
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
  for (int i=0; i<N; i++) {
    float x = values[i];
    int y = exponents[i];
    if (y == 0) {
      output[i] = 1.f;
    } else {
      float result = x;
      int count = y - 1;
      while (count > 0) {
        result *= x;
        count--;
      }
      if (result > 9.999999f) {
        result = 9.999999f;
      }
      output[i] = result;
    }
  }
}

void clampedExpVector(float* values, int* exponents, float* output, int N) {
  const float maxValue = 9.999999f;

  __cmu418_vec_float x; // vector of values
  __cmu418_vec_int y; // vector of exponents
  __cmu418_vec_float result; // vector of results

  __cmu418_mask allOnesMask = _cmu418_init_ones();   // a mask with all the lanes activated
  __cmu418_mask allZeroesMask = _cmu418_init_ones(0); // a mask with all the lanes deactivated

  // a vector of SIMD length with all the elements to 1
  __cmu418_vec_int allOnesVec;
  _cmu418_vset_int(allOnesVec, 1, allOnesMask);

  // a vector of SIMD length with all the elements set to 0
  __cmu418_vec_int allZeroesVec;
  _cmu418_vset_int(allZeroesVec, 0, allOnesMask);

  __cmu418_vec_float maxValueVec;
  _cmu418_vset_float(maxValueVec, maxValue, allOnesMask);

  for (int i=0; i<N; i+=VECTOR_WIDTH) {
    // The length of the array isn't gauranteed to be a multiple of the SIMD_LENGTH
    // So, we need a mask to deactivate the SIMD_LENGTH-LEN(ARRAY)%SIMD_LENGTH
    // lanes in the last iteration of the for loop.
    __cmu418_mask remainingElementsMask;
    int remaining = N - i;
    if(remaining >= VECTOR_WIDTH) {
      remainingElementsMask = allOnesMask;
    } else {
      remainingElementsMask = _cmu418_init_ones(remaining);
    }

    _cmu418_vload_float(x, values+i, remainingElementsMask);
    _cmu418_vload_int(y, exponents+i, remainingElementsMask);
    _cmu418_vset_float(result, 1, allOnesMask);

    // compute the mask of the elements whose pow == 0
    __cmu418_mask zeroPowersMask = _cmu418_init_ones(0);
    _cmu418_veq_int(zeroPowersMask, y, allZeroesVec, remainingElementsMask);

    // compute the mask of elements where pow > 0
    __cmu418_mask nonZeroPowersMask = _cmu418_mask_not(zeroPowersMask);      
    nonZeroPowersMask = _cmu418_mask_and(nonZeroPowersMask, remainingElementsMask);

    // loop until there exists at least one element where pow > 0      
    while(_cmu418_cntbits(nonZeroPowersMask) > 0) {
        // for each element where pow > 0, multiply the result vector by the x vector and 
        // store the computation back in the result vector
        _cmu418_vmult_float(result, x, result, nonZeroPowersMask);

        // reduce the power of each element where pow > 0 by 1 and 
        // recompute the nonZeroPowersMask
        _cmu418_vsub_int(y, y, allOnesVec, nonZeroPowersMask);
        _cmu418_vgt_int(nonZeroPowersMask, y, allZeroesVec, nonZeroPowersMask);
    }
    
    // clamp the results to 9.999999f
    __cmu418_mask clampedMask = allZeroesMask;
    _cmu418_vgt_float(clampedMask, result, maxValueVec, remainingElementsMask);
    _cmu418_vset_float(result, 9.999999f, clampedMask);

    _cmu418_vstore_float(output+i, result, remainingElementsMask);
  }

}

float arraySumSerial(float* values, int N) {
  float sum = 0;
  for (int i=0; i<N; i++) {
    sum += values[i];
  }

  return sum;
}

// Assume N is a power VECTOR_WIDTH == 0
// Assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
  // TODO: Implement your vectorized version of arraySumSerial here

  for (int i=0; i<N; i+=VECTOR_WIDTH) {

  }

  return 0.0;
}


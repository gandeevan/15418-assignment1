# 15418 - Assignment 1
​
## Hardware Specs:

Here's the specifications of the harware used for this assignment:

```
Architecture:            x86_64
  CPU op-mode(s):        32-bit, 64-bit
  Address sizes:         48 bits physical, 48 bits virtual
  Byte Order:            Little Endian
CPU(s):                  16
  On-line CPU(s) list:   0-15
Vendor ID:               AuthenticAMD
  Model name:            AMD Ryzen 7 5800H with Radeon Graphics
    CPU family:          25
    Model:               80
    Thread(s) per core:  2
    Core(s) per socket:  8
    Socket(s):           1
    Stepping:            0
    Frequency boost:     enabled
    CPU(s) scaling MHz:  27%
    CPU max MHz:         4462.5000
    CPU min MHz:         1200.0000
    BogoMIPS:            6387.92
    Flags:               fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx mmxext fxsr_opt pdpe1gb rdtscp lm constant_tsc rep_good nopl nonstop_tsc cpuid extd_apicid aperfmperf rapl pni pclmulqdq monitor ssse3 fma cx16 sse4_1 sse4_2 movbe popcnt aes xsave avx f16c rdrand lahf_lm cmp_legacy svm extapic cr8_legacy abm sse4a misalignsse 3dnowprefetch osvw ibs skinit wdt tce topoext perfctr_core perfctr_nb bpext perfctr_llc mwaitx cpb cat_l3 cdp_l3 hw_pstate ssbd mba ibrs ibpb stibp vmmcall fsgsbase bmi1 avx2 smep bmi2 erms invpcid cqm rdt_a rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local clzero irperf xsaveerptr rdpru wbnoinvd cppc arat npt lbrv svm_lock nrip_save tsc_scale vmcb_clean flushbyasid decodeassists pausefilter pfthreshold avic v_vmsave_vmload vgif v_spec_ctrl umip pku ospke vaes vpclmulqdq rdpid overflow_recov succor smca fsrm
```

## Program 1:
TODO

## Program 2:
TODO

## Program 3:

### Part 1:
> What is the maximum speedup you expect given what you know about these CPUs? Why might the number you observe be less than this ideal? (Hint: Consider the characteristics of the computation you are performing? Describe the parts of the image that present challenges for SIMD execution? Comparing the performance of rendering the different views of the Mandelbrot set may help confirm your hypothesis.)

- Assuming the code is being compiled with the SSE instruction set, with the programCounter value of 8 (i.e. 2x the SIMD width) - I'd expect an ideal speedup of ~8x. This would happen when the following assumptions hold true:
    - 100% lane utilization across all iterations. 
    - Given, the programCount is 8, we are processing 8 elements in one logical iteration of the loop. However, the SIMD width is 4. That probably means the compiler-generated assembly code is suited for out-of-order execution and interleaving of two vector executions in a single logical iteration of the loop. To achieve perfect speed-up, the interleaving of the two instruction streams would have to be near perfect, i.e. one executes while the other is waiting to load/store data and completes execution as soon the load/store is complete.  
- However, I'd probably expect less than ideal speed-up in reality. Here's why: 
    - In the Mandelbrot image, the brightness of each pixel is proportional to the computational cost of determining whether the value is contained in the Mandelbrot set. 
    - View 1 - There's very little pixel brightness variance in the first few rows and the last few rows of the image, which means the computational cost of rendering the pixels in these regions is more or less uniform, i.e. high lane utilization.  However, that's not true in the middle section of the image (especially around the boundaries of the bright pixels). The brightness variance in these rows means the job distribution between the SIMD lanes is not uniform, which means the lanes processing the "low-cost" pixels are turned off while the lanes processing the "high-cost" pixels finish the computation. 
    - View 2 - I'd probably expect the speed-up in view 2 to be worse than that of view 1, because there's a lot more brightness variance in each row compared to view 1 (where the skew was mostly around the boundaries of the bright pixels), which would thus correspond to lower lane utilization due to the reasons described above. 

```
View 1:​
[mandelbrot serial]:            [385.010] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot ispc]:              [85.164] ms
Wrote image file mandelbrot-ispc.ppm
                                (4.52x speedup from ISPC)
```

```
View 2:
​
[mandelbrot serial]:            [240.697] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot ispc]:              [61.878] ms
Wrote image file mandelbrot-ispc.ppm
                                (3.89x speedup from ISPC)
```

### Part 2:

> Run mandelbrot_ispc with the parameter --tasks. What speedup do you observe on view 1? What is the speedup over the version of mandelbrot_ispc that does not partition that computation into tasks?

```
[mandelbrot serial]:            [403.982] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot ispc]:              [89.081] ms
Wrote image file mandelbrot-ispc.ppm
[mandelbrot multicore ispc]:    [45.823] ms
Wrote image file mandelbrot-task-ispc.ppm
                                (4.54x speedup from ISPC)
                                (8.82x speedup from task ISPC)
```

> There is a simple way to improve the performance of mandelbrot_ispc --tasks by changing the number of tasks the code creates. By only changing code in the function mandelbrot_ispc_withtasks(), you should be able to achieve performance that exceeds the sequential version of the code by about 13-14 times! How did you determine how many tasks to create? Why does the number you chose work best?

- I'd expect the speedup to plateau and taper down after a task count of ~32 (which is 2x the number of hardware threads that can run in parallel on this machine). With 2x, the kernel could probably schedule multiple OS threads at once on a single hardware thread and the processor could  interleave the instruction streams while being stalled on memory operations.

- Results:

```
Speedup (tasks = 4) -> 10.82x
Speedup (tasks = 8) -> 16.24x
Speedup (tasks = 16) -> 23.17x
Speedup (tasks = 32) -> 32x
Speedup (tasks = 40) -> 33x
Speedup (tasks = 80) -> 33.84x
Speedup (tasks = 160) -> 33.32x
Speedup (tasks = 800) -> 34x
```

> Extra Credit: (2 points) What are differences between the pthread abstraction (used in Program 1) and the ISPC task abstraction? There are some obvious differences in semantics between the (create/join and (launch/sync) mechanisms, but the implications of these differences are more subtle. Here's a thought experiment to guide your answer: what happens when you launch 10,000 ISPC tasks? What happens when you launch 10,000 pthreads?

- The pthread abstraction creates an OS thread. Each pthread processes 'n' rows, such that the thread executes scalar instructions to process all the pixels in the 'n' rows sequentially. The compiler might generate instructions to make it possible to interleave the loop iterations to hide memory stalls. 
- The launch abstraction creates a 'task'. Each task processes 'n' rows, such that the task executes vector instructions to process a group of pixels in parallel. The group length is equal to the SIMD width of the processor. The compiler might generate instructions to interleave the executions of multiple "groups". There isn't necessarily a 1:1 correspondence between a task and an OS thread.  Based on the intel documentation I think it's a N:M relation between tasks and OS threads. 
- ISPC tasks are lightweight compared to pthreads (wrt to the overhead associated with creation, destruction, memory required to store the state per thread/task, context switching, etc). We'd probably run into performance issues if we spawn 10K pthreads due to the overhead described above. On the other hand, launching 10k tasks doesn't mean 10k OS threads were created. Instead, these tasks are probably queued up and multiplexed over a limited number of OS threads. 

> Excerpts from the ISPC docs:
>
> Inside functions with the task qualifier, two additional built-in variables are provided in addition to taskIndex and taskCount: threadIndex and threadCount. threadCount gives the total number of hardware threads that have been launched by the task system. threadIndex provides an index between zero and threadCount-1 that gives a unique index that corresponds to the hardware thread that is executing the current task. The threadIndex can be used for accessing data that is private to the current thread and thus doesn't require synchronization to access under parallel execution. 


## Program 4:
>Build and run sqrt. Report the ISPC implementation speedup for single CPU core (no tasks) and when using all cores (with tasks). What is the speedup due to SIMD parallelization? What is the speedup due to multi-core parallelization?

```
[serial]: [594.214] ms 
[ispc]: [204.196] ms (2.91x speedup from ISPC)
[task ispc - 64 tasks]: [17.110] ms (34.73x speedup from task ISPC)
```

>Modify the contents of the array values to improve the relative speedup of the ISPC implementations. Describe a very-good-case input that maximizes speedup over the sequential version and report the resulting speedup achieved (for both the with- and without-tasks ISPC implementations). Does your modification improve SIMD speedup? Does it improve multi-core speedup (i.e., the benefit of moving from ISPC without- tasks to ISPC with tasks)? Please explain why.

- The ideal case input to maximize the sequential to ISPC (without tasks) speedup is when all the elements of the array are equal. In this case, there would be a 100% lane utilization during SIMD execution. 
    - All the elements of the array were set to 2. I had expected almost a ~4x speedup, with this example on a SIMD implementation. However, it's around ~3.09, which is not very different from the randomized array. Also, the task-based ISPC speedup went down from 34.73x to 19x. 

```
[sqrt serial]:          [164.798] ms
[sqrt ispc]:            [53.402] ms
[sqrt task ispc]:       [8.560] ms
                                (3.09x speedup from ISPC)
                                (19.25x speedup from task ISPC)
```
- Although I didn't expect this result, now that I think through it - it's not entirely surprising. Here's why: 
    - Let's say the cost of computing the square root of single element in the array is `x`, so the cost of computing the sqroot of 4 (SIMD width) elements in a scalar implementation is `4x`.
    - Similarly the cost of executing a single iteration of the vectorized implementation is `x+d` (where d is the overhead of executing a vector instruction over a scalar instruction). `d` is probably constant, irrespective of the value of `x`
    - The cost of executing one single iteration of the loop, `x` can be further broken down into two components `c+y` . Where `c` is the constant cost independent of the value of arr[i] (incurred due to context switching, scheduling overheads etc).     
    - To maximize the speedup, we need to maximize   `4x/(x+d)`, i.e `4(c+y)/(c+y+d)`.
    - Given c and d are constants, we can maximize `4x/(x+d)` by maximizing y, such that y >> c and y >> d, so that  `4(c+y)/(c+y+d)` ~= `4y/y` ~= 4X speedup. 
- Based on the hypothesis outlined above, I then experimented with all the values of the array set to 2.99.  The results were along the lines of what I'd expected:
```
[sqrt serial]:          [1070.717] ms
[sqrt ispc]:            [307.571] ms
[sqrt task ispc]:       [30.858] ms
                                (3.48x speedup from ISPC)
                                (34.70x speedup from task ISPC)
```
- To further confirm my hypothesis, I ran another experiment, by minimizing y , with all the values of the array set to 1.  I expected that this input should significantly reduce the speedup and the results matched my expectations. 
```
[sqrt serial]:          [12.629] ms
[sqrt ispc]:            [7.691] ms
[sqrt task ispc]:       [8.079] ms
                                (1.64x speedup from ISPC)
                                (1.56x speedup from task ISPC)
```
- The  multi-core speedup (i.e., the benefit of moving from ISPC without- tasks to ISPC with tasks) is more or less the same compared the randomized input set (10.2x vs 11.9x).  This is expected, since:
    - In either scenarios, the job distribution between the threads is more or less uniform. In the base case, this is true since the inputs are randomly generated and in the other one, all the elements of the array are equal. 
    - The cost(work done per thread) >> cost(spawning and syncing on the threads/tasks). Again, this is true in the base case since inputs are randomly generated and in the other one, because, we maximize y by assigning every element in the array to 2.99. 


> Construct a very-bad-case input for sqrt that minimizes speedup for ISPC with no tasks. Describe this input, describe why you chose it, and report the resulting relative performance of the ISPC implementations. What is the reason for the loss in efficiency? (keep in mind we are using the --target=sse4 option for ISPC, which generates 4-wide SIMD instructions).

- Here's an input that minimizes speedup for ISPC with no tasks:
```
for (unsigned int i=0; i<N; i++)
{
    if(i%SIMD_WIDTH==0)
        values[i] = 2.99;
    else
        values[i] = 1;
}
```
- Results -  SIMD performance is worse than the serial execution. 
```
[sqrt serial]:          [689.876] ms
[sqrt ispc]:            [755.136] ms
[sqrt task ispc]:       [73.327] ms
                                (0.91x speedup from ISPC)
                                (9.41x speedup from task ISPC)
```

- In the given scenario, we have an array where every SIMD_WIDTH element is set to 2.99, and all other elements are set to 1. The goal is to compute the square root of each element in the array.
Now, let's consider the computational costs:
    - Computing the square root of 1 has a cost of P. Since 1 is a simple number, the cost P is negligible, meaning it's almost instantaneous.
    - Computing the square root of 2.99 has a cost of Q. This operation is more complex and thus, Q is significantly larger than P.
- In a scalar (non-parallel) implementation, if we were to compute the square root of SIMD_WIDTH elements, the cost would be for three elements with a value of 1 and one element with a value of 2.99. This would be 3P + Q. Given that P is negligible, this cost is approximately equal to Q.
- However, in a SIMD (parallel) implementation, all SIMD_WIDTH elements are processed simultaneously. But there's a catch: the first three lanes (processing the number 1) finish almost immediately, while the last lane (processing the number 2.99) takes longer. This means the first three lanes are essentially waiting idle for the last lane to finish. Additionally, there's an overhead d for executing a vector instruction over a scalar one. So, the cost of a single iteration in the vectorized implementation is Q+d.
- The key insight here is that the overhead of the vector instruction, combined with the idle time of the first three lanes, can make the SIMD approach less efficient than the scalar approach for this specific data distribution. In other words, the overhead of using parallel processing in this case might outweigh its benefits, leading to a loss in efficiency.


## Program 5

> Compile and run saxpy. The program will report the performance of ISPC (without tasks) and ISPC (with tasks) implementations of saxpy. What speedup from using ISPC with tasks do you observe? Explain the performance of this program. Do you think it can be improved?


Results:
```
[saxpy ispc]:           [8.527] ms      [34.951] GB/s   [4.691] GFLOPS
[saxpy task ispc]:      [10.766] ms     [27.682] GB/s   [3.715] GFLOPS
                                (0.79x speedup from use of tasks)
```

- The AMD Ryzen 7 5800H, built on the Zen 3 architecture, equips each core with two 256-bit FMA (Fused Multiply-Add) units tailored for SIMD operations. Given its base frequency of 3.2GHz and presuming the code utilizes 4-wide SIMD instructions, a single hardware thread can theoretically achieve up to 25.6GFLOPs (23.24*10^9). Yet, the ISPC implementation of saxpy only manages around 4.7GFLOPs, which is roughly 20% of the maximum single-core SIMD performance this hardware can offer. This discrepancy arises because the program is memory-bound. For every two arithmetic operations in each loop iteration, there are two loads and one store. The hardware optimizations like data prefetching and out-of-order processing can't sufficiently mask the memory stalls in this scenario.

- Interestingly, the performance of the task-based approach (comprising 64 tasks) lags behind the non-task ISPC implementation. My initial thought was that the relatively short runtime of the program meant the overhead from launching and synchronizing threads negated the benefits of core-based parallelization. However, this theory seemed less likely when the program, even with 200M elements and a runtime spanning hundreds of milliseconds, showed the basic ISPC version outpacing the task-based variant.

- I now believe the real issue is memory bandwidth saturation in the task-based approach. With all threads concurrently reading/writing four data items, memory access becomes a bottleneck. This theory gained traction when the task-based method slightly surpassed the basic ISPC version after reducing the task count from 64 to just 2.

```
./saxpy 
[saxpy ispc]:           [9.399] ms      [31.709] GB/s   [4.256] GFLOPS
[saxpy task ispc]:      [9.020] ms      [33.040] GB/s   [4.434] GFLOPS
                                (1.04x speedup from use of tasks)
```

> Extra Credit: (1 point) Note that the total memory bandwidth consumed computation in main.cpp is TOTAL_BYTES = 4 * N * sizeof(float);. Even though saxpy loads one element from X, one element from Y, and writes on element to result the multiplier by 4 is correct. Why is this the case?

- Here's the assembly of the serial implementation. Ignoring the read/writes to stack memory (because that's probably going to hit the L1 cache), I could only spot 3 memory accesses (2 loads and 1 store) in the serial implementation. 

```
L3:
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	mov	eax, DWORD PTR -4[rbp]	# tmp95, i
	cdqe
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	lea	rdx, 0[0+rax*4]	# _2,
	mov	rax, QWORD PTR -32[rbp]	# tmp96, X
	add	rax, rdx	# _3, _2
	movss	xmm0, DWORD PTR [rax]	# _4, *_3
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	movaps	xmm1, xmm0	# _4, _4
	mulss	xmm1, DWORD PTR -24[rbp]	# _4, scale
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	mov	eax, DWORD PTR -4[rbp]	# tmp97, i
	cdqe
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	lea	rdx, 0[0+rax*4]	# _7,
	mov	rax, QWORD PTR -40[rbp]	# tmp98, Y
	add	rax, rdx	# _8, _7
	movss	xmm0, DWORD PTR [rax]	# _9, *_8
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	mov	eax, DWORD PTR -4[rbp]	# tmp99, i
	cdqe
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	lea	rdx, 0[0+rax*4]	# _11,
	mov	rax, QWORD PTR -48[rbp]	# tmp100, result
	add	rax, rdx	# _12, _11
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	addss	xmm0, xmm1	# _13, _5
# saxpySerial.cpp:10:         result[i] = scale * X[i] + Y[i];
	 movss	DWORD PTR [rax], xmm0	# *_12, _13
# saxpySerial.cpp:9:     for (int i=0; i<N; i++) {
	add	DWORD PTR -4[rbp], 1	# i++,
.L2:
# saxpySerial.cpp:9:     for (int i=0; i<N; i++) {
	mov	eax, DWORD PTR -4[rbp]	# tmp101, i
	cmp	eax, DWORD PTR -20[rbp]	# tmp101, N
	jl	.L3	#,
```

- ISPC implementation: TODO

> Extra Credit: (points handled on a case-by-case basis) Improve the performance of saxpy. We're looking for a significant speedup here, not just a few percentage points. If successful, describe how you did it and what a best-possible implementation on these systems might achieve.

- TODO

## Just an interesting tidbit: 

### Adding two arrays in a loop
- While analyzing the assembly code generated from an ISPC implementation for two arrays, I observed the compiler's strategic approach to structure the assembly in a manner that allows the hardware to interleave multiple instruction streams to hide memory stalls. This aligns with the ISPC documentation's note that the programCount generally is a multiple of the SIMD width.
- In the given context, where the SIMD width is 4 and the program count stands at 8, the compiler optimizes for interleaving by handling the load, add, and store operations for two 128-bit vectors. This interleaving is evident in the assembly code, where the two distinct instruction streams are annotated.

```
# sum.ispc

export void sum(uniform int a[], uniform int b[], uniform int c[], uniform int n) {
  print("programCount: %\n", programCount); # prints 8
  for(int i=0; i<n; i+=programCount) {
    int idx = i + programIndex;
    c[idx] = a[idx] + b[idx];
  }
}
```

```
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
```

```
.LBB1_38:                              
	paddd	%xmm4, %xmm1 # IS1: add a[i], b[i]
	paddd	%xmm5, %xmm0 # IS2: add a[i], b[i]
	...
	...
	...
	<lots of assembly code>
	...
# %bb.3:                              
	movdqu	(%r12,%rbx), %xmm0 # IS1: load a[i]
	movdqu	16(%r12,%rbx), %xmm1 # IS1: load b[i]
	movdqu	(%r15,%rbx), %xmm5 # IS2: load a[i]
	movdqu	16(%r15,%rbx), %xmm4 # IS2: load b[i]
	jmp	.LBB1_38
```

### HOW TO EMIT ASSEMBLY
```
ispc sum.ispc -o sum_ispc.s -O3 --opt=disable-handle-pseudo-memory-ops --emit-asm --target=sse4-x2
g++ -S main.cpp -o main.s
```

### BUILD INSTRUCTIONS
```
ispc sum.ispc -o sum_ispc.o -h sum_ispc.h --pic -O3 --target=sse4-x2
g++ main.cpp sum_ispc.o -o main
```
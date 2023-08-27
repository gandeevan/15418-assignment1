#include <stdio.h>
#include <pthread.h>
#include <iostream>

#include "CycleTimer.h"

#define NUM_CORES 8

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[], 
    int increment = 1);


//
// workerThreadStart --
//
// Thread entrypoint.
void* workerThreadStart(void* threadArgs) {
    double startTime = CycleTimer::currentSeconds();
    WorkerArgs* args = static_cast<WorkerArgs*>(threadArgs);

    int numRowsPerThread = args->height/args->numThreads;
    int startRow = args->threadId;
    unsigned int lastRow = startRow + (args->numThreads*numRowsPerThread);
    if(lastRow >= args->height) {
        lastRow = startRow + (args->numThreads*(numRowsPerThread-1));
    }
    mandelbrotSerial(args->x0, args->y0, args->x1, args->y1,
                     args->width, args->height,
                     startRow, lastRow,
                     args->maxIterations,
                     args->output, args->numThreads);

    double endTime = CycleTimer::currentSeconds();
    printf("Thread %d: %.3f ms\n", args->threadId, 1000.f * (endTime - startTime));
    
    return NULL;
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Multi-threading performed via pthreads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    printf("Running threaded version\n");
    const static int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    pthread_t workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {
        args[i].x0 = x0;
        args[i].x1 = x1;
        args[i].y0 = y0;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].output = output;
        args[i].numThreads = numThreads;
        args[i].threadId = i;
    }

    // Fire up the worker threads.  Note that numThreads-1 pthreads
    // are created and the main app thread is used as a worker as
    // well.
    int core_numbers[NUM_CORES] = {0, 1, 2, 3, 4, 5, 6, 7}; // Assuming a 4-core system

    for (int i=1; i<numThreads; i++) {
        pthread_attr_t attr;
        cpu_set_t cpuset;

        pthread_attr_init(&attr);
        CPU_ZERO(&cpuset);
        CPU_SET(core_numbers[i%NUM_CORES], &cpuset);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
        pthread_create(&workers[i], &attr, workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // wait for worker threads to complete
    for (int i=1; i<numThreads; i++)
        pthread_join(workers[i], NULL);
}

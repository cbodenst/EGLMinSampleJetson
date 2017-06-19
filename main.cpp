/*
 * main.cpp
 *
 * Copyright 2016 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

//
// DESCRIPTION:   Simple EGL stream sample app
//
//

//#define EGL_EGLEXT_PROTOTYPES

#include "cuda_consumer.h"
#include "cuda_producer.h"
#include "eglstrm_common.h"
#include "cudaEGL.h"

/* ------  globals ---------*/

int main(int argc, char **argv)
{
    TestArgs args;
    CUresult curesult = CUDA_SUCCESS;
    unsigned int i, j;
    EGLint streamState = 0;

    test_cuda_consumer_s cudaConsumer;
    test_cuda_producer_s cudaProducer;

    memset(&cudaProducer, 0, sizeof(test_cuda_producer_s));
    memset(&cudaConsumer, 0, sizeof(test_cuda_consumer_s));

    // Hook up Ctrl-C handler
    if(!eglSetupExtensions()) {
        printf("SetupExtentions failed \n");
        curesult = CUDA_ERROR_UNKNOWN;
        goto done;
    }

    if(!EGLStreamInit()) {
        printf("EGLStream Init failed.\n");
        curesult = CUDA_ERROR_UNKNOWN;
        goto done;
    }
    curesult = cudaDeviceCreateProducer(&cudaProducer);
    if (curesult != CUDA_SUCCESS) {
        goto done;
    }
    curesult = cudaDeviceCreateConsumer(&cudaConsumer);
    if (curesult != CUDA_SUCCESS) {
        goto done;
    }
    if (CUDA_SUCCESS != (curesult = cuEGLStreamConsumerConnect(&(cudaConsumer.cudaConn), eglStream))) {
        printf("FAILED Connect CUDA consumer  with error %d\n", curesult);
        goto done;
    }
    else {
        printf("Connected CUDA consumer, CudaConsumer %p\n", cudaConsumer.cudaConn);
    }
    if (CUDA_SUCCESS == (curesult = cuEGLStreamProducerConnect(&(cudaProducer.cudaConn), eglStream, WIDTH, HEIGHT))) {
        printf("Connect CUDA producer Done, CudaProducer %p\n", cudaProducer.cudaConn);
    } else {
        printf("Connect CUDA producer FAILED with error %d\n", curesult);
        goto done;
    }

    // Initialize producer
    args.inputWidth        = WIDTH;
    args.inputHeight       = HEIGHT;
    args.isARGB        = 0;
    args.infile1       = "cuda_f_1.yuv";
    args.infile2       = "cuda_f_2.yuv";
    args.pitchLinearOutput = 1;

    cudaProducerInit(&cudaProducer, g_display, eglStream, &args);
    cuda_consumer_init(&cudaConsumer, &args);

    printf("main - Cuda Producer and Consumer Initialized.\n");

    printf("Running for YUV frame and Pitchlinear\n");
    curesult = cudaProducerTest(&cudaProducer, cudaProducer.fileName1);
    if (curesult != CUDA_SUCCESS) {
        printf("Cuda Producer Test failed for frame 1\n");
        goto done;
    }
    curesult = cudaConsumerTest(&cudaConsumer, cudaConsumer.outFile1);
    if (curesult != CUDA_SUCCESS) {
        printf("Cuda Consumer Test failed for frame = 1\n");
        goto done;
    }
    curesult = cudaProducerTest(&cudaProducer, cudaProducer.fileName2);
    if (curesult != CUDA_SUCCESS) {
        printf("Cuda Producer Test failed for frame = 1\n");
        goto done;
    }

    curesult = cudaConsumerTest(&cudaConsumer, cudaConsumer.outFile2);
    if (curesult != CUDA_SUCCESS) {
        printf("Cuda Consumer Test failed for frame = 2\n");
        goto done;
    }

    if (CUDA_SUCCESS != (curesult = cudaProducerDeinit(&cudaProducer))) {
        printf("Producer Disconnect FAILED. \n");
        goto done;
    }
    if(!eglQueryStreamKHR_wrapper(
                g_display,
                eglStream,
                EGL_STREAM_STATE_KHR,
                &streamState)) {
        printf("Cuda consumer, eglQueryStreamKHR EGL_STREAM_STATE_KHR failed\n");
        curesult = CUDA_ERROR_UNKNOWN;
        goto done;
    }
    if(streamState != EGL_STREAM_STATE_DISCONNECTED_KHR) {
        if (CUDA_SUCCESS != (curesult = cuda_consumer_deinit(&cudaConsumer))) {
            printf("Consumer Disconnect FAILED.\n");
            goto done;
        }
    }
    printf("Producer and Consumer Disconnected \n");

done:
    if(!eglQueryStreamKHR_wrapper(
                g_display,
                eglStream,
                EGL_STREAM_STATE_KHR,
                &streamState)) {
        printf("Cuda consumer, eglQueryStreamKHR EGL_STREAM_STATE_KHR failed\n");
        curesult = CUDA_ERROR_UNKNOWN; 
    }
    if(streamState != EGL_STREAM_STATE_DISCONNECTED_KHR) {
        EGLStreamFini();
    }

    if (curesult == CUDA_SUCCESS) {
        printf("&&&& EGLStream interop test PASSED\n");
    }
    else {
        printf("&&&& EGLStream interop test FAILED\n");
    }
    return 0;
}

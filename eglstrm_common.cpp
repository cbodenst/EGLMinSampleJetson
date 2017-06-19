/*
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
// DESCRIPTION:   Common egl stream functions
//

#include "eglstrm_common.h"

EGLStreamKHR eglStream;
EGLDisplay   g_display;
EGLSurface   eglSurface;
EGLConfig    eglConfig;
EGLContext   eglContext;

#if defined(EXTENSION_LIST)
EXTENSION_LIST(EXTLST_DECL)
typedef void (*extlst_fnptr_t)(void);
static struct {
    extlst_fnptr_t *fnptr;
    char const *name;
} extensionList[] = { EXTENSION_LIST(EXTLST_ENTRY) };

int eglSetupExtensions(void)
{
    unsigned int i;

    for (i = 0; i < (sizeof(extensionList) / sizeof(*extensionList)); i++) {
        *extensionList[i].fnptr = eglGetProcAddress(extensionList[i].name);
        if (*extensionList[i].fnptr == NULL) {
            printf("Couldn't get address of %s()\n", extensionList[i].name);
            return 0;
        }
    }

    return 1;
}

void PrintEGLStreamState(EGLint streamState)
{
    #define STRING_VAL(x) {""#x"", x}
    struct {
        char *name;
        EGLint val;
    } EGLState[9] = {
        STRING_VAL(EGL_STREAM_STATE_CREATED_KHR),
        STRING_VAL(EGL_STREAM_STATE_CONNECTING_KHR),
        STRING_VAL(EGL_STREAM_STATE_EMPTY_KHR),
        STRING_VAL(EGL_STREAM_STATE_NEW_FRAME_AVAILABLE_KHR),
        STRING_VAL(EGL_STREAM_STATE_OLD_FRAME_AVAILABLE_KHR),
        STRING_VAL(EGL_STREAM_STATE_DISCONNECTED_KHR),
        STRING_VAL(EGL_BAD_STREAM_KHR),
        STRING_VAL(EGL_BAD_STATE_KHR),
        { NULL, 0 }
    };
    int i = 0;

    while(EGLState[i].name) {
        if(streamState == EGLState[i].val) {
            printf("%s\n", EGLState[i].name);
            return;
        }
        i++;
    }
    printf("Invalid %d\n", streamState);
}

int EGLStreamInit()
{
    static const EGLint streamAttrMailboxMode[] = { EGL_NONE };
    EGLBoolean eglStatus;

    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_display == EGL_NO_DISPLAY) {
        printf("eglDisplayHandle failed \n");
        return 0;
    }
    else {
        printf("eglDisplay Handle created \n");
    }

    eglStatus = eglInitialize(g_display, 0, 0);
    if (!eglStatus) {
        printf("EGL failed to initialize.\n");
        return 0;
    }

    eglStream = eglCreateStreamKHR(g_display, streamAttrMailboxMode);
    if (eglStream == EGL_NO_STREAM_KHR) {
        printf("EGLStreamInit: Couldn't create eglStream.\n");
        return 0;
    }

    // Set stream attribute
    if(!eglStreamAttribKHR(g_display, eglStream, EGL_CONSUMER_LATENCY_USEC_KHR, 16000)) {
        printf("Consumer: eglStreamAttribKHR EGL_CONSUMER_LATENCY_USEC_KHR failed\n");
        return 0;
    }
    if(!eglStreamAttribKHR(g_display, eglStream, EGL_CONSUMER_ACQUIRE_TIMEOUT_USEC_KHR, 16000)) {
        printf("Consumer: eglStreamAttribKHR EGL_CONSUMER_ACQUIRE_TIMEOUT_USEC_KHR failed\n");
        return 0;
    }

    printf("EGLStream initialized\n");
    return 1;
}

int EGLStreamCreateProducer(void)
{
    EGLint     n = 0;
    // Choose a config and create a context
    EGLint cfg_attr[] = {
        EGL_SURFACE_TYPE, EGL_STREAM_BIT_KHR,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_ALPHA_SIZE, 1,
        EGL_NONE
    };
    if (!eglChooseConfig(g_display, cfg_attr, &eglConfig, 1, &n) || !n) {
        printf("Unable to obtain config that supports stream rendering (error 0x%x)\n", eglGetError());
    }

    EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    eglBindAPI(EGL_OPENGL_ES_API);

    eglContext = eglCreateContext(g_display, eglConfig, EGL_NO_CONTEXT, ctx_attr);
    if (eglContext == EGL_NO_CONTEXT) {
        printf("Unable to create context (error 0x%x)\n", eglGetError());
    }

    printf("Obtained EGLConfig and EGLContext\n");
    // Create a surface to feed the stream
    EGLint srf_attr[] = { EGL_WIDTH, WIDTH, EGL_HEIGHT, HEIGHT, EGL_NONE };
    eglSurface = eglCreateStreamProducerSurfaceKHR(g_display, eglConfig, eglStream,
                                                srf_attr);
    if (eglSurface == EGL_NO_SURFACE) {
        printf("Unable to create rendering surface (error 0x%x)\n", eglGetError());
    }
    printf("Bound layer to rendering surface\n");

    // Make current
    if (!eglMakeCurrent(g_display, eglSurface, eglSurface, eglContext)) {
        printf("Unable to make context/surface current (error 0x%x)\n", eglGetError());
    }

    return 1;
}

void EGLStreamFini(void)
{
    eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(g_display, eglContext);
    eglDestroySurface(g_display, eglSurface);
    eglDestroyStreamKHR(g_display, eglStream);
}

EGLBoolean eglQueryStreamKHR_wrapper(
    EGLDisplay   dpy,
    EGLStreamKHR stream,
    EGLenum      attribute,
    EGLint      *value)
{
    return eglQueryStreamKHR(
                dpy,
                stream,
                attribute,
                value);
}

void eglSwapBuffers_wrapper(void)
{
  eglSwapBuffers(g_display, eglSurface);
}



#endif

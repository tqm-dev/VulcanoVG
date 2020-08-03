/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <c11/threads.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include "egldefines.h"
#include "egldriver.h"
#include "egldisplay.h"

static EGLBoolean
_Initialize(_EGLDriver *drv, _EGLDisplay *disp)
{
	EGLBoolean ret = EGL_FALSE;
	VkInstance vkInstance;

	switch (disp->Platform) {
		case _EGL_PLATFORM_VULKAN:
			vkInstance = (VkInstance)disp->PlatformDisplay;
			if(vkInstance)
				ret = EGL_TRUE;
			break;
		case _EGL_PLATFORM_SURFACELESS:
		case _EGL_PLATFORM_DEVICE:
		case _EGL_PLATFORM_X11:
		case _EGL_PLATFORM_DRM:
		case _EGL_PLATFORM_WAYLAND:
		case _EGL_PLATFORM_ANDROID:
		default:
			return EGL_FALSE;
	}

	return ret;
}

_EGLDriver _eglDriver = {
   .Initialize						= _Initialize,
   .Terminate						= NULL,
   .CreateContext					= NULL,
   .DestroyContext					= NULL,
   .MakeCurrent						= NULL,
   .CreateWindowSurface				= NULL,
   .CreatePixmapSurface				= NULL,
   .CreatePbufferSurface			= NULL,
   .DestroySurface					= NULL,
   .GetProcAddress					= NULL,
   .WaitClient						= NULL,
   .WaitNative						= NULL,
   .BindTexImage					= NULL,
   .ReleaseTexImage					= NULL,
   .SwapInterval					= NULL,
   .SwapBuffers						= NULL,
   .SwapBuffersWithDamageEXT		= NULL,
   .SwapBuffersRegionNOK			= NULL,
   .SetDamageRegion					= NULL,
   .PostSubBufferNV					= NULL,
   .CopyBuffers						= NULL,
   .QueryBufferAge					= NULL,
   .CreateImageKHR					= NULL,
   .DestroyImageKHR					= NULL,
   .CreateWaylandBufferFromImageWL	= NULL,
   .QuerySurface					= NULL,
   .QueryDriverName					= NULL,
   .QueryDriverConfig				= NULL,
   .GetSyncValuesCHROMIUM			= NULL,
   .CreateSyncKHR					= NULL,
   .ClientWaitSyncKHR				= NULL,
   .SignalSyncKHR					= NULL,
   .WaitSyncKHR						= NULL,
   .DestroySyncKHR					= NULL,
   .GLInteropQueryDeviceInfo		= NULL,
   .GLInteropExportObject			= NULL,
   .DupNativeFenceFDANDROID			= NULL,
   .SetBlobCacheFuncsANDROID		= NULL
};


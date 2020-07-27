#include "test.h"
#include <assert.h>

int main(int argc, char **argv)
{
	EGLDisplay dpy;
	EGLConfig config;
	EGLContext ctx;
	EGLBoolean ret;
	EGLint major, minor;

	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(EGL_CAST(EGLint,dpy) == 1);

	ret = eglInitialize(dpy, &major, &minor);
	assert(ret == EGL_TRUE);
	assert(major == 1);
	assert(minor == 2);

	{
		EGLint num_config;
		EGLint attrib_list[3] = {
			EGL_CONFIG_ID, 0xff,
			EGL_NONE
		};
		ret = eglChooseConfig(dpy, attrib_list, &config, 1, &num_config);
		assert(ret == EGL_TRUE);
		assert(EGL_CAST(EGLint,config) == 0xff);
		assert(num_config == 1);
	}

	ret = eglBindAPI(EGL_OPENVG_API);
	assert(ret == EGL_TRUE);

	ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL);
	assert(ctx != EGL_NO_CONTEXT);

	ret = eglDestroyContext(dpy, ctx);
	assert(ret == EGL_TRUE);

	ret = eglTerminate(dpy);
	assert(ret == EGL_TRUE);

	printf("test_egl is done !!! \r\n");

	return EXIT_SUCCESS;
}

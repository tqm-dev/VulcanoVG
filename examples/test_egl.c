#include "test.h"

int main(int argc, char **argv)
{
	EGLDisplay dpy;
	EGLBoolean ret;
	EGLint major, minor;

	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	printf("eglGetDisplay() :%d \r\n", dpy);

	ret = eglInitialize(dpy, &major, &minor);
	printf("eglInitialize() :%d \r\n", ret);
	printf("major :%d \r\n", major);
	printf("minor :%d \r\n", minor);

  return EXIT_SUCCESS;
}

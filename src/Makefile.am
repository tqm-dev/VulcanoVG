lib_LTLIBRARIES =

lib_LTLIBRARIES += libOpenVG.la
libOpenVG_la_CFLAGS = -pedantic -I$(top_builddir)/include
libOpenVG_la_SOURCES =\
	VG/shDefs.h\
	VG/shExtensions.h\
	VG/shArrayBase.h\
	VG/shArrays.h\
	VG/shVectors.h\
	VG/shPath.h\
	VG/shImage.h\
	VG/shPaint.h\
	VG/shGeometry.h\
	VG/shContext.h\
	VG/shExtensions.c\
	VG/shArrays.c\
	VG/shVectors.c\
	VG/shPath.c\
	VG/shImage.c\
	VG/shPaint.c\
	VG/shGeometry.c\
	VG/shPipeline.c\
	VG/shParams.c\
	VG/shContext.c\
	VG/shVgu.c

VG_includedir = $(includedir)/VG
VG_include_HEADERS =\
	$(top_builddir)/include/VG/openvg.h\
	$(top_builddir)/include/VG/vgu.h

lib_LTLIBRARIES += libEGL.la
libEGL_la_CFLAGS = \
	-pedantic -I$(top_builddir)/include \
	-pedantic -I$(top_builddir)/src/VG \
	-pedantic -I$(top_builddir)/src/util
libEGL_la_SOURCES =\
	EGL/eglapi.c\
	EGL/eglarray.c\
	EGL/eglconfig.c\
	EGL/eglcontext.c\
	EGL/eglcurrent.c\
	EGL/egldevice.c\
	EGL/egldisplay.c\
	EGL/egldriver.c\
	EGL/eglglobals.c\
	EGL/eglimage.c\
	EGL/egllog.c\
	EGL/eglsurface.c\
	EGL/eglsync.c\
	EGL/egldriver_vulkan.c

EGL_includedir = $(includedir)/EGL
EGL_include_HEADERS =\
	$(top_builddir)/include/EGL/egl.h\
	$(top_builddir)/include/EGL/eglext.h\
	$(top_builddir)/include/EGL/eglplatform.h\
	$(top_builddir)/include/EGL/khrplatform.h


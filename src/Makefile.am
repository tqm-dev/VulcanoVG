lib_LTLIBRARIES =

lib_LTLIBRARIES += libOpenVG.la
libOpenVG_la_CFLAGS = -pedantic -I$(top_builddir)/include/vg
libOpenVG_la_SOURCES =\
	vg/shDefs.h\
	vg/shExtensions.h\
	vg/shArrayBase.h\
	vg/shArrays.h\
	vg/shVectors.h\
	vg/shPath.h\
	vg/shImage.h\
	vg/shPaint.h\
	vg/shGeometry.h\
	vg/shContext.h\
	vg/shExtensions.c\
	vg/shArrays.c\
	vg/shVectors.c\
	vg/shPath.c\
	vg/shImage.c\
	vg/shPaint.c\
	vg/shGeometry.c\
	vg/shPipeline.c\
	vg/shParams.c\
	vg/shContext.c\
	vg/shVgu.c

VG_includedir = $(includedir)/vg
VG_include_HEADERS =\
	$(top_builddir)/include/vg/openvg.h\
	$(top_builddir)/include/vg/vgu.h

lib_LTLIBRARIES += libEGL.la
libEGL_la_CFLAGS = \
	-pedantic -I$(top_builddir)/include \
	-pedantic -I$(top_builddir)/include/egl \
	-pedantic -I$(top_builddir)/include/vg \
	-pedantic -I$(top_builddir)/src/vg \
	-pedantic -I$(top_builddir)/src/util
libEGL_la_SOURCES =\
	egl/eglapi.c\
	egl/eglarray.c\
	egl/eglconfig.c\
	egl/eglcontext.c\
	egl/eglcurrent.c\
	egl/egldevice.c\
	egl/egldisplay.c\
	egl/egldriver.c\
	egl/eglglobals.c\
	egl/eglimage.c\
	egl/egllog.c\
	egl/eglsurface.c\
	egl/eglsync.c\
	egl/egldriver_vulkan.c

EGL_includedir = $(includedir)/egl
EGL_include_HEADERS =\
	$(top_builddir)/include/egl/egl.h\
	$(top_builddir)/include/egl/eglext.h\
	$(top_builddir)/include/egl/eglplatform.h\
	$(top_builddir)/include/egl/khrplatform.h


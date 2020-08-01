lib_LTLIBRARIES =

lib_LTLIBRARIES += libOpenVG.la
libOpenVG_la_CFLAGS = -ansi -pedantic -I$(top_builddir)/include/vg
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
	-ansi -pedantic -I$(top_builddir)/include/egl \
	-ansi -pedantic -I$(top_builddir)/include/vg \
	-ansi -pedantic -I$(top_builddir)/src/vg
libEGL_la_SOURCES =\
	egl/vuContext.c
EGL_includedir = $(includedir)/egl
EGL_include_HEADERS =\
	$(top_builddir)/include/egl/egl.h\
	$(top_builddir)/include/egl/eglext.h\
	$(top_builddir)/include/egl/eglplatform.h\
	$(top_builddir)/include/egl/khrplatform.h


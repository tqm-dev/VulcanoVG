lib_LTLIBRARIES =

lib_LTLIBRARIES += libOpenVG.la
libOpenVG_la_CFLAGS = -ansi -pedantic -I$(top_builddir)/include/vg
libOpenVG_la_SOURCES =\
	shDefs.h\
	shExtensions.h\
	shArrayBase.h\
	shArrays.h\
	shVectors.h\
	shPath.h\
	shImage.h\
	shPaint.h\
	shGeometry.h\
	shContext.h\
	shExtensions.c\
	shArrays.c\
	shVectors.c\
	shPath.c\
	shImage.c\
	shPaint.c\
	shGeometry.c\
	shPipeline.c\
	shParams.c\
	shContext.c\
	shVgu.c

VG_includedir = $(includedir)/vg
VG_include_HEADERS =\
	$(top_builddir)/include/vg/openvg.h\
	$(top_builddir)/include/vg/vgu.h

lib_LTLIBRARIES += libEGL.la
libEGL_la_CFLAGS = \
	-ansi -pedantic -I$(top_builddir)/include/egl \
	-ansi -pedantic -I$(top_builddir)/include/vg
libEGL_la_SOURCES =\
	vuContext.c
EGL_includedir = $(includedir)/egl
EGL_include_HEADERS =\
	$(top_builddir)/include/egl/egl.h\
	$(top_builddir)/include/egl/eglext.h\
	$(top_builddir)/include/egl/eglplatform.h\
	$(top_builddir)/include/egl/khrplatform.h


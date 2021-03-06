## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
file_jpeg_RC = file-jpeg.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(EXIF_CFLAGS)	\
	$(LCMS_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

libexec_PROGRAMS = file-jpeg

file_jpeg_SOURCES = \
	jpeg.c		\
	jpeg.h		\
	jpeg-icc.c	\
	jpeg-icc.h	\
	jpeg-load.c	\
	jpeg-load.h	\
	jpeg-save.c	\
	jpeg-save.h	\
	jpeg-quality.c  \
	jpeg-quality.h  \
	jpeg-settings.c \
	jpeg-settings.h

if HAVE_LIBEXIF
file_jpeg_SOURCES += \
	jpeg-exif.c	\
	jpeg-exif.h	\
	gimpexif.c	\
	gimpexif.h
endif

file_jpeg_LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(JPEG_LIBS)		\
	$(LCMS_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_jpeg_RC)

if HAVE_LIBEXIF
file_jpeg_LDADD += \
	$(EXIF_LIBS)
endif

noinst_PROGRAMS = jpegqual

jpegqual_SOURCES = \
	jpeg-quality.c  \
	jpeg-quality.h  \
	jpegqual.c	

jpegqual_LDADD = \
	$(JPEG_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)

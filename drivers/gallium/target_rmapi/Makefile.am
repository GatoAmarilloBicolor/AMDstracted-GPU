include $(top_srcdir)/src/gallium/targets/Makefile.gallium

TARGET_DRIVERS = rmapi
TARGET_CPPFLAGS = -DGALLIUM_RMAPI

LIBRARY_INCLUDES = \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(LLVM_CFLAGS) \
	$(LIBDRM_CFLAGS)

LIBRARY_LIBS = \
	$(GALLIUM_DRIVER_LIBS) \
	$(LLVM_LIBS) \
	$(LIBDRM_LIBS)

COMMON_SOURCES = \
	rmapi_screen.c \
	rmapi_context.c \
	rmapi_winsys.c

SOURCES_RMAPI = $(COMMON_SOURCES)

include $(top_srcdir)/src/gallium/targets/Makefile.targets

AUTOMAKE_OPTIONS = nostdinc
AM_CPPFLAGS = -I$(top_srcdir)/Sources -iquote$(top_builddir)/Sources/ObjectivelyGPU

noinst_HEADERS = \
    Config.h

pkgincludedir = $(includedir)/$(PACKAGE_NAME)

pkginclude_HEADERS = \
    Buffer.h \
    CommandBuffer.h \
    ComputePass.h \
    ComputePipeline.h \
    CopyPass.h \
    Framebuffer.h \
    GraphicsPipeline.h \
    Mathlib.h \
    RenderDevice.h \
    RenderPass.h \
    Sampler.h \
    Shader.h \
    Texture.h \
    Types.h

lib_LTLIBRARIES = libObjectivelyGPU.la

libObjectivelyGPU_la_SOURCES = \
    Buffer.c \
    CommandBuffer.c \
    ComputePass.c \
    ComputePipeline.c \
    CopyPass.c \
    Framebuffer.c \
    GraphicsPipeline.c \
    RenderDevice.c \
    RenderPass.c \
    Sampler.c \
    Shader.c \
    Texture.c

libObjectivelyGPU_la_CFLAGS = \
    $(OBJECTIVELY_CFLAGS) \
    $(SDL3_CFLAGS)

libObjectivelyGPU_la_LDFLAGS = \
    -version-info 1:0:0 \
    $(HOST_LDFLAGS)

libObjectivelyGPU_la_LIBADD = \
    $(OBJECTIVELY_LIBS) \
    $(SDL3_LIBS)

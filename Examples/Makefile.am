SUBDIRS = Shaders

noinst_PROGRAMS = \
	Hello \
	HelloCompute

CFLAGS += \
	-isystem $(top_srcdir)/Sources \
	@HOST_CFLAGS@ \
	@OBJECTIVELY_CFLAGS@ \
	@SDL3_CFLAGS@

LDADD = \
	$(top_builddir)/Sources/ObjectivelyGPU/libObjectivelyGPU.la \
	@HOST_LIBS@ \
	@OBJECTIVELY_LIBS@ \
	@SDL3_LIBS@ \
	-lm

Hello_SOURCES = Hello.c
HelloCompute_SOURCES = HelloCompute.c

SUBDIRS = Shaders

noinst_PROGRAMS = \
	Hello \
	HelloCompute

CFLAGS += \
	-I$(top_srcdir)/Sources \
 	-DSHADERS=\"$(abs_srcdir)/Shaders\" \ 
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

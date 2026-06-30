noinst_PROGRAMS = \
	Hello \
	HelloCompute

CFLAGS += \
	-I$(top_srcdir)/Sources \
	-DEXAMPLES=\"$(abs_srcdir)\" \
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

# The compiled .spv/.metal blobs are versioned, so normal builds never need
# glslc/shadercross. Run 'make shaders' after editing a .glsl.
EXTRA_DIST = \
	Hello.vert.glsl \
	Hello.frag.glsl \
	Hello.vert.metal \
	Hello.frag.metal \
	Hello.vert.spv \
	Hello.frag.spv \
	HelloCompute.comp.glsl \
	HelloCompute.vert.glsl \
	HelloCompute.frag.glsl \
	HelloCompute.comp.metal \
	HelloCompute.vert.metal \
	HelloCompute.frag.metal \
	HelloCompute.comp.spv \
	HelloCompute.vert.spv \
	HelloCompute.frag.spv

.PHONY: shaders
shaders:
	$(GLSLC) -fshader-stage=vert Hello.vert.glsl -o Hello.vert.spv
	$(GLSLC) -fshader-stage=frag Hello.frag.glsl -o Hello.frag.spv
	$(GLSLC) -fshader-stage=comp HelloCompute.comp.glsl -o HelloCompute.comp.spv
	$(GLSLC) -fshader-stage=vert HelloCompute.vert.glsl -o HelloCompute.vert.spv
	$(GLSLC) -fshader-stage=frag HelloCompute.frag.glsl -o HelloCompute.frag.spv
	$(SHADERCROSS) Hello.vert.spv -s SPIRV -d MSL -t vertex -o Hello.vert.metal
	$(SHADERCROSS) Hello.frag.spv -s SPIRV -d MSL -t fragment -o Hello.frag.metal
	$(SHADERCROSS) HelloCompute.comp.spv -s SPIRV -d MSL -t compute -o HelloCompute.comp.metal
	$(SHADERCROSS) HelloCompute.vert.spv -s SPIRV -d MSL -t vertex -o HelloCompute.vert.metal
	$(SHADERCROSS) HelloCompute.frag.spv -s SPIRV -d MSL -t fragment -o HelloCompute.frag.metal

AUTOMAKE_OPTIONS = nostdinc

# All HLSL sources and compiled blobs are versioned in the repository.
# Normal builds never invoke shadercross.  Run 'make shadercross' after
# editing a .hlsl file to regenerate the platform blobs.

GLSLC       = glslc
SHADERCROSS = shadercross

HLSL_SOURCES = \
    Hello.vert.hlsl \
    Hello.frag.hlsl \
    HelloCompute.comp.hlsl \
    HelloCompute.vert.hlsl \
    HelloCompute.frag.hlsl

MSL_BLOBS = \
    Hello.vert.msl \
    Hello.frag.msl \
    HelloCompute.comp.msl \
    HelloCompute.vert.msl \
    HelloCompute.frag.msl

SPIRV_BLOBS = \
    Hello.vert.spv \
    Hello.frag.spv \
    HelloCompute.comp.spv \
    HelloCompute.vert.spv \
    HelloCompute.frag.spv

EXTRA_DIST = $(HLSL_SOURCES) $(MSL_BLOBS) $(SPIRV_BLOBS)

# For out-of-tree builds, symlink the pre-built blobs into the build directory
# so SDL Resource resolution (SDL_GetBasePath() + "Shaders") finds them.
all-local:
	@if [ "x$(srcdir)" != "x$(builddir)" ]; then \
	    for f in $(MSL_BLOBS) $(SPIRV_BLOBS); do \
	        test -e "$(builddir)/$$f" || \
	            $(LN_S) "$(abs_srcdir)/$$f" "$(builddir)/$$f"; \
	    done; \
	fi

# -------------------------------------------------------------------------
# shadercross — regenerate all blobs from HLSL source.
# Requires glslc (from shaderc) for HLSL→SPIR-V and shadercross for
# SPIR-V→MSL.
# -------------------------------------------------------------------------

.PHONY: shadercross
shadercross: $(MSL_BLOBS) $(SPIRV_BLOBS)

# Hello — vertex
$(srcdir)/Hello.vert.spv: $(srcdir)/Hello.vert.hlsl
	$(GLSLC) -fshader-stage=vertex -x hlsl -fentry-point=vs_main $< -o $@

$(srcdir)/Hello.vert.msl: $(srcdir)/Hello.vert.spv
	$(SHADERCROSS) $< -s SPIRV -d MSL -t vertex -e vs_main -o $@

# Hello — fragment
$(srcdir)/Hello.frag.spv: $(srcdir)/Hello.frag.hlsl
	$(GLSLC) -fshader-stage=fragment -x hlsl -fentry-point=fs_main $< -o $@

$(srcdir)/Hello.frag.msl: $(srcdir)/Hello.frag.spv
	$(SHADERCROSS) $< -s SPIRV -d MSL -t fragment -e fs_main -o $@

# HelloCompute — compute
$(srcdir)/HelloCompute.comp.spv: $(srcdir)/HelloCompute.comp.hlsl
	$(GLSLC) -fshader-stage=compute -x hlsl -fentry-point=cs_main $< -o $@

$(srcdir)/HelloCompute.comp.msl: $(srcdir)/HelloCompute.comp.spv
	$(SHADERCROSS) $< -s SPIRV -d MSL -t compute -e cs_main -o $@

# HelloCompute — vertex
$(srcdir)/HelloCompute.vert.spv: $(srcdir)/HelloCompute.vert.hlsl
	$(GLSLC) -fshader-stage=vertex -x hlsl -fentry-point=vs_main $< -o $@

$(srcdir)/HelloCompute.vert.msl: $(srcdir)/HelloCompute.vert.spv
	$(SHADERCROSS) $< -s SPIRV -d MSL -t vertex -e vs_main -o $@

# HelloCompute — fragment
$(srcdir)/HelloCompute.frag.spv: $(srcdir)/HelloCompute.frag.hlsl
	$(GLSLC) -fshader-stage=fragment -x hlsl -fentry-point=fs_main $< -o $@

$(srcdir)/HelloCompute.frag.msl: $(srcdir)/HelloCompute.frag.spv
	$(SHADERCROSS) $< -s SPIRV -d MSL -t fragment -e fs_main -o $@

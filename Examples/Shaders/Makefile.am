AUTOMAKE_OPTIONS = nostdinc

# All HLSL sources and compiled blobs are versioned in the repository.
# Normal builds never invoke shadercross.  Run 'make shadercross' after
# editing a .hlsl file to regenerate the platform blobs.

SHADERCROSS = shadercross

HLSL_FILES = \
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

EXTRA_DIST = $(HLSL_FILES) $(MSL_BLOBS) $(SPIRV_BLOBS)

.PHONY: shadercross
shadercross: $(EXTRA_DIST)

$(srcdir)/%.vert.spv: $(srcdir)/%.vert.hlsl
	$(SHADERCROSS) $< -s HLSL -d SPIRV -t vertex -e vs_main -o $@

$(srcdir)/%.vert.msl: $(srcdir)/%.vert.hlsl
	$(SHADERCROSS) $< -s HLSL -d MSL -t vertex -e vs_main -o $@

$(srcdir)/%.frag.spv: $(srcdir)/%.frag.hlsl
	$(SHADERCROSS) $< -s HLSL -d SPIRV -t fragment -e fs_main -o $@

$(srcdir)/%.frag.msl: $(srcdir)/%.frag.hlsl
	$(SHADERCROSS) $< -s HLSL -d MSL -t fragment -e fs_main -o $@

$(srcdir)/%.comp.spv: $(srcdir)/%.comp.hlsl
	$(SHADERCROSS) $< -s HLSL -d SPIRV -t compute -e cs_main -o $@

$(srcdir)/%.comp.msl: $(srcdir)/%.comp.hlsl
	$(SHADERCROSS) $< -s HLSL -d MSL -t compute -e cs_main -o $@

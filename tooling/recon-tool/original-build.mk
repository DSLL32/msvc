# Optional original-toolchain build. Source paths are relative to the binary
# root; retaining the entire path and extension avoids basename collisions.
ORIGINAL_BUILD ?= build
ifneq ($(strip $(ORIGINAL_TOOLCHAIN)),)
ORIGINAL_SOURCES ?= $(wildcard src/*.c src/*.cpp crt/*.c crt/*.cpp)
endif
ORIGINAL_LINK_OUTPUTS := $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
ORIGINAL_OBJECTS := $(addprefix $(ORIGINAL_BUILD)/obj/,$(addsuffix .obj,$(ORIGINAL_SOURCES)))
OBJECTS := $(ORIGINAL_OBJECTS)
ifneq ($(strip $(ORIGINAL_SOURCES)),)
ifneq ($(strip $(filter /% ../%,$(ORIGINAL_SOURCES)) $(findstring /../,$(ORIGINAL_SOURCES))),)
$(error ORIGINAL_SOURCES must be binary-root-relative without .. components)
endif
ifneq ($(filter-out %.c %.cpp,$(ORIGINAL_SOURCES)),)
$(error ORIGINAL_SOURCES supports .c and .cpp sources)
endif
# The toolchain name selects the shared launcher/environment. No local wrapper.
ifneq ($(strip $(ORIGINAL_TOOLCHAIN)),)
# Keep source/library arguments relative so old Windows tools can read them.
ORIGINAL_TOOLCHAIN_ROOT := $(shell realpath -m --relative-to="$(CURDIR)" "$(RECON_TOOL_DIR)/../../toolchains/$(ORIGINAL_TOOLCHAIN)")
ORIGINAL_EXTRA_DEPS += $(RECON_TOOL_DIR)/src/toolchain.rs $(RECON_TOOL_DIR)/src/wine.rs
ORIGINAL_CC ?= $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) CL.EXE --
ORIGINAL_LINK ?= $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) LINK.EXE --
$(ORIGINAL_OBJECTS) $(ORIGINAL_OUTPUT): | $(RECON_TOOL)
endif
ORIGINAL_CFLAGS ?= /nologo /c /Od
# Override either command for a non-MSVC toolchain. These are recursive variables
# so automatic target/source variables are evaluated in the actual recipe.
ORIGINAL_COMPILE ?= $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) /Fo$@ $<
ORIGINAL_LINK_COMMAND ?= $(ORIGINAL_LINK) $(ORIGINAL_LDFLAGS) $(if $(ORIGINAL_MAP),/map:$(ORIGINAL_MAP)) /out:$(ORIGINAL_OUTPUT) $(ORIGINAL_OBJECTS) $(ORIGINAL_LDLIBS)
ORIGINAL_DEPEND_PYTHON ?= python3
ORIGINAL_DEPENDENCIES ?= $(ORIGINAL_BUILD)/original-dependencies.mk
.PHONY: win32 recon-original
win32 recon-original: $(ORIGINAL_LINK_OUTPUTS)
	@test -n "$(ORIGINAL_OUTPUT)" || { echo 'Set ORIGINAL_OUTPUT for an original build' >&2; exit 1; }
$(ORIGINAL_OBJECTS): Makefile $(RECON_TOOL_DIR)/original-build.mk $(RECON_TOOL_DIR)/recon-tool.mk $(ORIGINAL_EXTRA_DEPS) | $(ORIGINAL_DEPENDENCIES) $(ORIGINAL_ORDER_ONLY)
$(ORIGINAL_BUILD)/obj/%.c.obj: %.c
	@mkdir -p $(@D)
	$(ORIGINAL_COMPILE)
$(ORIGINAL_BUILD)/obj/%.cpp.obj: %.cpp
	@mkdir -p $(@D)
	$(ORIGINAL_COMPILE)
ifneq ($(strip $(ORIGINAL_OUTPUT)),)
$(ORIGINAL_LINK_OUTPUTS) &: $(ORIGINAL_OBJECTS) $(ORIGINAL_LDLIBS) Makefile $(RECON_TOOL_DIR)/original-build.mk
	@mkdir -p $(sort $(dir $(ORIGINAL_LINK_OUTPUTS)))
	$(ORIGINAL_LINK_COMMAND)
endif
# One scan shares include parsing/closures across all original sources. The
# dependency file is order-only: changing one source must not rebuild every OBJ.
$(ORIGINAL_DEPENDENCIES): $(ORIGINAL_SOURCES) Makefile $(RECON_TOOL_DIR)/original-deps.py $(RECON_TOOL_DIR)/original-build.mk
	@mkdir -p $(@D)
	$(ORIGINAL_DEPEND_PYTHON) $(RECON_TOOL_DIR)/original-deps.py --output $@ --object-root $(ORIGINAL_BUILD)/obj $(foreach d,$(ORIGINAL_INCLUDE_DIRS),--include $(d)) $(ORIGINAL_SOURCES)
# On the first build, objects create this file through their order dependency.
# Later builds include/remake it before considering any object prerequisites.
ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(wildcard $(ORIGINAL_DEPENDENCIES))
endif
endif

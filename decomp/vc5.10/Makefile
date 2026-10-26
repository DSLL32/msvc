BINARY_DIRS := c1 c1xx c2 cl cvppack cvtres link rcdll rc

.PHONY: all inspect $(BINARY_DIRS) native original fixtures
all: inspect
inspect: $(BINARY_DIRS)

$(BINARY_DIRS):
	$(MAKE) -C $@ inspect

native: $(addprefix native-,$(BINARY_DIRS))
original: $(addprefix original-,$(BINARY_DIRS))
fixtures: $(addprefix fixtures-,$(BINARY_DIRS))

define BINARY_TARGETS
.PHONY: native-$(1) original-$(1) fixtures-$(1)
native-$(1):
	$$(MAKE) -C $(1) native
original-$(1):
	$$(MAKE) -C $(1) $(if $(filter cvppack,$(1)),all,original)
fixtures-$(1):
	$$(MAKE) -C $(1) $(if $(filter cvppack,$(1)),check,fixtures)
endef
$(foreach binary,$(BINARY_DIRS),$(eval $(call BINARY_TARGETS,$(binary))))

# Canonical executable adoption currently includes RC with static RCDLL only.
RECON_TOOL_DIR := $(abspath ../../tooling/recon-tool)
RECON_NATIVE_BIN_DIR ?= $(abspath build/native/bin)
include $(RECON_TOOL_DIR)/native-entry.mk
$(eval $(call recon-native-program,rc,rc,$(RECON_NATIVE_BIN_DIR)/RC.EXE))
.PHONY: native-bin
native-bin: native-bins

# Canonical native executables; other Watcom binaries remain unadopted.
RECON_TOOL_DIR := $(abspath ../../tooling/recon-tool)
RECON_NATIVE_BIN_DIR ?= $(abspath build/native/bin)
.DEFAULT_GOAL := native-bins
include $(RECON_TOOL_DIR)/native-entry.mk

# Canonical Windows basename; the file itself is a native i386 ELF.
$(eval $(call recon-native-program,wcc386,wcc386,$(RECON_NATIVE_BIN_DIR)/WCC386.EXE))

.PHONY: native-bin
native-bin: native-bins

RECON_TOOL_DIR := $(abspath ../../tooling/recon-tool)
RECON_NATIVE_BIN_DIR ?= $(abspath build/native/bin)
.DEFAULT_GOAL := native-bins
include $(RECON_TOOL_DIR)/native-entry.mk

# These are Linux ELF executables with the original Windows tool basenames.
$(eval $(call recon-native-program,cl,cl,$(RECON_NATIVE_BIN_DIR)/CL.EXE))
$(eval $(call recon-native-program,c1,c1,$(RECON_NATIVE_BIN_DIR)/C13232.EXE))
$(eval $(call recon-native-program,c1xx,c1xx,$(RECON_NATIVE_BIN_DIR)/C1XX3232.EXE))
$(eval $(call recon-native-program,c2,c2,$(RECON_NATIVE_BIN_DIR)/C23232.EXE))
$(eval $(call recon-native-program,link,link,$(RECON_NATIVE_BIN_DIR)/LINK.EXE))
$(eval $(call recon-native-program,cvpack,cvpack,$(RECON_NATIVE_BIN_DIR)/CVPACK.EXE))
$(eval $(call recon-native-program,rc,rc,$(RECON_NATIVE_BIN_DIR)/RC.EXE))
$(eval $(call recon-native-program,rcpp,rcpp,$(RECON_NATIVE_BIN_DIR)/RCPP.EXE))

.PHONY: all native native-bin
all native native-bin: native-bins

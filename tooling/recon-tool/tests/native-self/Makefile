RECON_TOOL_DIR := $(abspath ../..)
RECON_LIB := $(abspath ../../../../lib/recon)
BINARY_SOURCES := src/app.c
BINARY_SUPPORT_SOURCES := $(addprefix $(RECON_LIB)/src/,crt.c win32.c process.c compat.c entry.c decimal.c trace.c)
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -I$(RECON_LIB)/include -I$(RECON_LIB)/native
include $(RECON_TOOL_DIR)/recon-tool.mk

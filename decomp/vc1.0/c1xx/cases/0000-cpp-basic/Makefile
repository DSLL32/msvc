CASE_CC = clang
CASE_CPPFLAGS = -I$(CASE_DIR)/../../src -I$(CASE_DIR)/../../../../../lib/recon/include -I$(CASE_DIR)/../../../../../lib/recon/native -DRECON_NATIVE_TEST
CASE_CFLAGS = -Werror=implicit-function-declaration -std=gnu89 -fms-extensions -fasm-blocks
RECON_TOOL_DIR ?= $(abspath ../../../../../tooling/recon-tool)
include $(RECON_TOOL_DIR)/case.mk

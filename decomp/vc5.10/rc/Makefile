RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -Wno-ignored-attributes
BINARY_SOURCES = src/rc.c src/startup.c
BINARY_SUPPORT_SOURCES = scripts/provider.c $(wildcard $(RECON)/src/*.c)
NATIVE_LDFLAGS = -Wl,--wrap=recon_SetConsoleCtrlHandler

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = src/rc.c src/startup.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/rc.exe
ORIGINAL_MAP = build/rc.map
ORIGINAL_CPPFLAGS = /I$(RECON)/include /Isrc
ORIGINAL_CFLAGS = /nologo /c /MD /Od /G3 /W3
ORIGINAL_LDFLAGS = /nologo /subsystem:console /entry:mainCRTStartup
ORIGINAL_LDLIBS = build/rcdll.lib $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 16777216
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --discovery cfg --check-ret --check-imports --check-references --check-ownership --check-extents --include-evidenced-runtime
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
include $(RECON_TOOL_DIR)/recon-tool.mk

.build/pic/src/rc.c.o .build/pic/src/startup.c.o: NATIVE_CPPFLAGS += -Dmain=rc_main
build/obj/src/rc.c.obj: ORIGINAL_CFLAGS = /nologo /c /MD /O2 /Oy- /G3 /W3
build/rcdll.lib: src/rcdll.def scripts/import-library.py | $(RECON_TOOL)
	@mkdir -p $(@D)
	python3 scripts/import-library.py $@ $(RECON_TOOL)
.PHONY: original
original: win32

# Data imports are separate from the common qualified-call graph check.
.PHONY: check-import-data
audit: check-import-data
check-import-data: $(ORIGINAL_OUTPUT) $(RECON_REGISTRY)
	python3 scripts/check-import-data.py

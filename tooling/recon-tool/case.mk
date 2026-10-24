# Included by a case Makefile, either at the binary root or standalone.
CASE_DIR ?= .
BINARY_ROOT ?= ../..
RECON_TOOL_DIR ?= $(abspath $(BINARY_ROOT)/../../../tooling/recon-tool)
CASE_BUILD ?= $(CASE_DIR)/.build
BINARY_LIBRARY ?= $(abspath $(BINARY_ROOT)/.build/libbinary.so)
CASE_SOURCES ?= $(notdir $(wildcard $(addprefix $(CASE_DIR)/,case.c case.cpp direct.c tests.c)))
CASE_CC ?= $(or $(NATIVE_CC),gcc)
CASE_CXX ?= $(or $(NATIVE_CXX),g++)
CASE_LINKER ?= $(if $(filter %.cpp,$(CASE_SOURCES)),$(CASE_CXX),$(CASE_CC))
CASE_OBJECTS := $(addprefix $(CASE_BUILD)/,$(addsuffix .o,$(CASE_SOURCES)))
CASE_CPPFLAGS += -I$(RECON_TOOL_DIR)/include
CASE_CXXFLAGS ?= $(CASE_CFLAGS)
CASE_CXXFLAGS := $(CASE_CXXFLAGS) -m32 -fPIC -O0 -g
CASE_CFLAGS += -m32 -fPIC -O0 -g

# Freeze configuration in target-specific variables before reading another case.
# Private variables cannot leak through the order-only common-library edge.
define recon-case-rules
$(CASE_OBJECTS) $(CASE_BUILD)/case.so: private CASE_CC := $(CASE_CC)
$(CASE_OBJECTS) $(CASE_BUILD)/case.so: private CASE_CXX := $(CASE_CXX)
$(CASE_OBJECTS) $(CASE_BUILD)/case.so: private CASE_LINKER := $(CASE_LINKER)
$(CASE_OBJECTS) $(CASE_BUILD)/case.so: private CASE_CPPFLAGS := $(CASE_CPPFLAGS)
$(CASE_OBJECTS) $(CASE_BUILD)/case.so: private CASE_CFLAGS := $(CASE_CFLAGS)
$(CASE_OBJECTS) $(CASE_BUILD)/case.so: private CASE_CXXFLAGS := $(CASE_CXXFLAGS)
$(CASE_BUILD)/case.so: private CASE_LDFLAGS := $(CASE_LDFLAGS)
$(CASE_BUILD)/case.so: private CASE_LDLIBS := $(CASE_LDLIBS)
$(CASE_BUILD)/case.so: private CASE_OBJECTS := $(CASE_OBJECTS)
$(CASE_BUILD)/case.so: $(CASE_OBJECTS) | $(if $(RECON_CASE_PARENT),$(BINARY_LIBRARY),recon-binary)
	$$(CASE_LINKER) -m32 -shared -Wl,-z,defs $$(CASE_LDFLAGS) -Wl,--no-as-needed -o $$@ $$(CASE_OBJECTS) $(BINARY_LIBRARY) -Wl,-rpath,'$$$$ORIGIN/../../../.build' $$(CASE_LDLIBS)
$(CASE_BUILD)/%.c.o: $(CASE_DIR)/%.c
	@mkdir -p $$(@D)
	$$(CASE_CC) $$(CASE_CPPFLAGS) $$(CASE_CFLAGS) -MMD -MP -c $$< -o $$@
$(CASE_BUILD)/%.cpp.o: $(CASE_DIR)/%.cpp
	@mkdir -p $$(@D)
	$$(CASE_CXX) $$(CASE_CPPFLAGS) $$(CASE_CXXFLAGS) -MMD -MP -c $$< -o $$@
$(CASE_OBJECTS): $(CASE_DIR)/Makefile $(RECON_TOOL_DIR)/case.mk $(if $(RECON_CASE_PARENT),Makefile $(RECON_TOOL_DIR)/recon-tool.mk)
endef
$(eval $(recon-case-rules))
-include $(CASE_OBJECTS:.o=.d)

ifdef RECON_CASE_PARENT
CASE_TARGETS += $(CASE_BUILD)/case.so
$(CASE_DIR): $(CASE_BUILD)/case.so
else
.DEFAULT_GOAL := all
.PHONY: all recon-binary
all: $(CASE_BUILD)/case.so
# Standalone case builds may still request the binary's incremental build.
# Binary-root check/unit builds never recurse.
recon-binary:
ifneq ($(RECON_SKIP_BINARY_BUILD),1)
	+$(MAKE) -C $(BINARY_ROOT) recon-native
endif
endif

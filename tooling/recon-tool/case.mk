# Include from cases/NNNN-name. Override paths where a project layout differs.
BINARY_ROOT ?= ../..
RECON_TOOL_DIR ?= $(abspath $(BINARY_ROOT)/../../../tooling/recon-tool)
CASE_BUILD ?= .build
BINARY_LIBRARY ?= $(abspath $(BINARY_ROOT)/.build/libbinary.so)
CASE_SOURCES ?= $(wildcard case.c case.cpp direct.c tests.c)
CASE_LINKER ?= $(if $(filter %.cpp,$(CASE_SOURCES)),$(NATIVE_CXX),$(NATIVE_CC))
CASE_OBJECTS := $(addprefix $(CASE_BUILD)/,$(addsuffix .o,$(CASE_SOURCES)))
NATIVE_CC ?= gcc
NATIVE_CXX ?= g++
CASE_CPPFLAGS += -I$(RECON_TOOL_DIR)/include
CASE_CFLAGS += -m32 -fPIC -O0 -g
.PHONY: all recon-binary
all: $(CASE_BUILD)/case.so
# Order-only dependency rebuilds the common library incrementally, without
# relinking this case merely because the common implementation changed.
recon-binary:
ifneq ($(RECON_SKIP_BINARY_BUILD),1)
	+$(MAKE) -C $(BINARY_ROOT) recon-native
endif
$(CASE_BUILD)/case.so: $(CASE_OBJECTS) | recon-binary
	$(CASE_LINKER) -m32 -shared -Wl,-z,defs $(CASE_LDFLAGS) -Wl,--no-as-needed -o $@ $(CASE_OBJECTS) $(BINARY_LIBRARY) -Wl,-rpath,'$$ORIGIN/../../../.build' $(CASE_LDLIBS)
$(CASE_BUILD)/%.c.o: %.c
	@mkdir -p $(@D)
	$(NATIVE_CC) $(CASE_CPPFLAGS) $(CASE_CFLAGS) -MMD -MP -c $< -o $@
$(CASE_BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(NATIVE_CXX) $(CASE_CPPFLAGS) $(CASE_CFLAGS) -MMD -MP -c $< -o $@
-include $(CASE_OBJECTS:.o=.d)
$(CASE_OBJECTS): Makefile $(RECON_TOOL_DIR)/case.mk

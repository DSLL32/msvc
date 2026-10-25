# Include from a binary root after defining tool/compiler configuration.
# A dedicated object tree keeps these PIC objects separate from canonical builds.
RECON_TOOL_DIR ?= $(abspath ../../../tooling/recon-tool)
BINARY_BUILD ?= .build
BINARY_LIBRARY ?= $(BINARY_BUILD)/libbinary.so
NATIVE_CC ?= gcc
NATIVE_CXX ?= g++
NATIVE_CPPFLAGS += -iquote src -iquote crt -iquote harness -I$(RECON_TOOL_DIR)/include
NATIVE_CFLAGS += -m32 -fPIC -O0 -g -fno-omit-frame-pointer -fno-inline -finstrument-functions
# Preserve common flags without passing a C language dialect to the C++ driver.
RECON_C_DIALECT_FLAGS := $(foreach dialect,c89 c90 c99 c11 c17 c18 c23 c2x c2y gnu89 gnu90 gnu99 gnu11 gnu17 gnu18 gnu23 gnu2x gnu2y,-std=$(dialect))
NATIVE_CXXFLAGS += $(filter-out $(RECON_C_DIALECT_FLAGS),$(NATIVE_CFLAGS))
# Shim/tracer sources must be uninstrumented and included only in this library.
# Caller provides those via BINARY_SUPPORT_SOURCES, never in individual cases.
BINARY_SOURCES ?= $(wildcard src/*.c src/*.cpp crt/*.c crt/*.cpp)
BINARY_ALL_SOURCES := $(BINARY_SOURCES) binary.c
BINARY_OBJECTS := $(addprefix $(BINARY_BUILD)/pic/,$(addsuffix .o,$(BINARY_ALL_SOURCES)))
# Preserve readable source paths instead of flattening ../ into object names.
BINARY_SOURCE_ROOT ?= $(abspath $(RECON_TOOL_DIR)/../..)
support-path = $(patsubst /%,external/%,$(patsubst $(BINARY_SOURCE_ROOT)/%,%,$(abspath $(1))))
support-object = $(BINARY_BUILD)/support/$(call support-path,$(1)).o
BINARY_TOOL_SUPPORT := $(RECON_TOOL_DIR)/native/entry-hook.c $(RECON_TOOL_DIR)/native/fixture.c
BINARY_SUPPORT_OBJECTS := $(foreach s,$(BINARY_SUPPORT_SOURCES) $(BINARY_TOOL_SUPPORT),$(call support-object,$(s)))
BINARY_LINKER ?= $(if $(filter %.cpp,$(BINARY_ALL_SOURCES) $(BINARY_SUPPORT_SOURCES)),$(NATIVE_CXX),$(NATIVE_CC))
.PHONY: recon-native
recon-native: $(BINARY_LIBRARY)
$(BINARY_LIBRARY): $(BINARY_OBJECTS) $(BINARY_SUPPORT_OBJECTS)
	@mkdir -p $(@D)
	$(BINARY_LINKER) -m32 -shared -Wl,-z,defs -Wl,-Bsymbolic-functions -Wl,-soname,libbinary.so $(NATIVE_LDFLAGS) -o $@ $^ $(NATIVE_LDLIBS) -ldl
$(BINARY_BUILD)/pic/%.c.o: %.c
	@mkdir -p $(@D)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_CFLAGS) -MMD -MP -c $< -o $@
$(BINARY_BUILD)/pic/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(NATIVE_CXX) $(NATIVE_CPPFLAGS) $(NATIVE_CXXFLAGS) -MMD -MP -c $< -o $@
define support-rule
$(call support-object,$(1)): $(1)
	@mkdir -p $$(@D)
	$$(if $$(filter %.cpp,$$<),$$(NATIVE_CXX),$$(NATIVE_CC)) $$(NATIVE_CPPFLAGS) $$(filter-out -finstrument-functions,$$(if $$(filter %.cpp,$$<),$$(NATIVE_CXXFLAGS),$$(NATIVE_CFLAGS))) -MMD -MP -c $$< -o $$@
endef
$(foreach s,$(BINARY_SUPPORT_SOURCES) $(BINARY_TOOL_SUPPORT),$(eval $(call support-rule,$(s))))
-include $(BINARY_OBJECTS:.o=.d) $(BINARY_SUPPORT_OBJECTS:.o=.d)
$(BINARY_OBJECTS) $(BINARY_SUPPORT_OBJECTS): Makefile $(RECON_TOOL_DIR)/binary.mk

# Include from a binary root after defining RECON_TOOL_DIR and BINARY_SOURCES.
# A dedicated object tree keeps these PIC objects separate from canonical builds.
RECON_TOOL_DIR ?= $(abspath ../../../tooling/recon-tool)
BINARY_BUILD ?= .build
BINARY_LIBRARY ?= $(BINARY_BUILD)/libbinary.so
NATIVE_CC ?= gcc
NATIVE_CXX ?= g++
NATIVE_CPPFLAGS += -I$(RECON_TOOL_DIR)/include
NATIVE_CFLAGS += -m32 -fPIC -O0 -g -fno-omit-frame-pointer -fno-inline -finstrument-functions
NATIVE_CXXFLAGS += $(NATIVE_CFLAGS)
# Shim/tracer sources must be uninstrumented and included only in this library.
# Caller provides those via BINARY_SUPPORT_SOURCES, never in individual cases.
BINARY_ALL_SOURCES := $(BINARY_SOURCES) binary.c
BINARY_OBJECTS := $(addprefix $(BINARY_BUILD)/pic/,$(addsuffix .o,$(BINARY_ALL_SOURCES)))
support-object = $(BINARY_BUILD)/support/$(subst /,_,$(subst ..,_up_,$(1))).o
BINARY_SUPPORT_OBJECTS := $(foreach s,$(BINARY_SUPPORT_SOURCES),$(call support-object,$(s)))
BINARY_LINKER ?= $(if $(filter %.cpp,$(BINARY_ALL_SOURCES)),$(NATIVE_CXX),$(NATIVE_CC))
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
	$$(NATIVE_CC) $$(NATIVE_CPPFLAGS) $$(filter-out -finstrument-functions,$$(NATIVE_CFLAGS)) -MMD -MP -c $$< -o $$@
endef
$(foreach s,$(BINARY_SUPPORT_SOURCES),$(eval $(call support-rule,$(s))))
-include $(BINARY_OBJECTS:.o=.d) $(BINARY_SUPPORT_OBJECTS:.o=.d)

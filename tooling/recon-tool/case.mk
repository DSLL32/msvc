# Internal case rules, included once from the binary-root recon-tool.mk.
# Case/test glue uses the binary compilation settings, including instrumentation.
case-sources = $(wildcard $(addprefix $(1)/,case.c case.cpp direct.c tests.c))
case-objects = $(addprefix $(1)/.build/,$(addsuffix .o,$(notdir $(call case-sources,$(1)))))
case-linker = $(if $(filter %.cpp,$(call case-sources,$(1))),$(NATIVE_CXX),$(NATIVE_CC))

define recon-case-rules
$(1)/.build/case.so: $(call case-objects,$(1)) | $(BINARY_LIBRARY)
	$(call case-linker,$(1)) -m32 -shared -Wl,-z,defs $$(CASE_LDFLAGS) -Wl,--no-as-needed -o $$@ $(call case-objects,$(1)) $(BINARY_LIBRARY) -Wl,-rpath,'$$$$ORIGIN/../../../.build' $$(CASE_LDLIBS)
$(1)/.build/%.c.o: $(1)/%.c
	@mkdir -p $$(@D)
	$$(NATIVE_CC) $$(NATIVE_CPPFLAGS) $$(NATIVE_CFLAGS) -MMD -MP -c $$< -o $$@
$(1)/.build/%.cpp.o: $(1)/%.cpp
	@mkdir -p $$(@D)
	$$(NATIVE_CXX) $$(NATIVE_CPPFLAGS) $$(NATIVE_CXXFLAGS) -MMD -MP -c $$< -o $$@
$(call case-objects,$(1)): Makefile $(RECON_TOOL_DIR)/case.mk $(RECON_TOOL_DIR)/recon-tool.mk
# Explicit original-only target, never a dependency of cases/check/unit.
$(1)/.build/original-direct.dll: $(1)/direct.c $(wildcard $(1)/case.h) Makefile $(RECON_TOOL_DIR)/case.mk $(ORIGINAL_DIRECT_DEPS) $(RECON_TOOL_DIR)/wine/direct-support.c $(RECON_TOOL_DIR)/include/recon-original-direct.h $(RECON_TOOL)
	@mkdir -p $$(@D)
	$$(RECON_TOOL) original build-direct $$< --repo $$(RECON_REPO_ROOT) $$(if $$(ORIGINAL_TOOLCHAIN),--toolchain $$(ORIGINAL_TOOLCHAIN)) --output $$@ $$(ORIGINAL_DIRECT_FLAGS)
$(1): $(1)/.build/case.so
endef
$(foreach dir,$(CASE_DIRS),$(eval $(call recon-case-rules,$(dir))))
CASE_OBJECTS := $(foreach dir,$(CASE_DIRS),$(call case-objects,$(dir)))
-include $(CASE_OBJECTS:.o=.d)

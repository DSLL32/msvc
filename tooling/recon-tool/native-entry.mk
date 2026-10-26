# Canonical entry.c and static modules. Explicit lists reuse existing PIC objects;
# fixture hosts/tool support are never implicitly selected for these links.
NATIVE_ROOT_ENTRY ?= entry.c
NATIVE_ENTRY_BUILD ?= $(BINARY_BUILD)/entry
NATIVE_ENTRY_PYTHON ?= python3
NATIVE_ENTRY_LD ?= ld
NATIVE_ENTRY_OBJCOPY ?= objcopy
NATIVE_APP_MAIN ?= recon_entry_app_main
NATIVE_APP_MAIN_SYMBOL ?= main
NATIVE_ENTRY_LINKER ?= $(BINARY_LINKER)
RECON_MODULE_CONSTRUCTORS ?= reject
RECON_MODULE_RESET ?= region
RECON_ENTRY_APP_OBJECTS = $(addprefix $(BINARY_BUILD)/pic/,$(addsuffix .o,$(RECON_ENTRY_SOURCES)))
RECON_ENTRY_SUPPORT_OBJECTS = $(foreach s,$(RECON_ENTRY_SUPPORT_SOURCES),$(call support-object,$(s)))
RECON_ENTRY_HELPER := $(RECON_TOOL_DIR)/entry-build.py
RECON_ENTRY_MAKE := $(RECON_TOOL_DIR)/native-entry.mk
RECON_ENTRY_MODULE_IDS :=
RECON_ENTRY_ASSET_IDS :=
NATIVE_MODULE_OBJECTS :=
NATIVE_MODULE_SCRIPTS :=
NATIVE_ASSET_OBJECTS :=

.PHONY: recon-entry-selection recon-entry-force native-module
recon-entry-force:
recon-entry-selection:
	@test -n '$(strip $(RECON_ENTRY_SOURCES))' || { echo 'Set explicit RECON_ENTRY_SOURCES for canonical native links' >&2; exit 1; }
	@test -z '$(filter-out $(BINARY_ALL_SOURCES),$(RECON_ENTRY_SOURCES))' || { echo 'RECON_ENTRY_SOURCES must select existing binary source objects' >&2; exit 1; }
	@test -z '$(filter-out $(BINARY_SUPPORT_SOURCES),$(RECON_ENTRY_SUPPORT_SOURCES))' || { echo 'RECON_ENTRY_SUPPORT_SOURCES must select existing support objects (no fixture/tool injection)' >&2; exit 1; }
	$(if $(RECON_MODULE_ID),@test -z '$(filter $(abspath $(RECON_TOOL_DIR)/../../lib/recon)/%,$(abspath $(RECON_ENTRY_SUPPORT_SOURCES)))' || { echo 'Static modules must import the shared runtime instead of bundling lib/recon copies' >&2; exit 1; },@true)

# ID=path is explicit and stable, independent of objcopy's input-path spelling.
entry-file-key = $(word 1,$(subst =, ,$(1)))
entry-file-path = $(word 2,$(subst =, ,$(1)))
define recon-native-asset
$(if $(filter $(1),$(RECON_ENTRY_ASSET_IDS)),$(error duplicate native asset ID $(1)))
RECON_ENTRY_ASSET_IDS += $(1)
NATIVE_ASSET_OBJECTS += $(abspath $(NATIVE_ENTRY_BUILD)/assets/$(1).o)
$(abspath $(NATIVE_ENTRY_BUILD)/assets/$(1).o): $(abspath $(2)) $(RECON_ENTRY_HELPER) $(RECON_ENTRY_MAKE)
	@mkdir -p $$(@D)
	$$(NATIVE_ENTRY_PYTHON) $$(RECON_ENTRY_HELPER) asset --id '$(1)' --source '$$(abspath $$<)' --output '$$@' --objcopy '$$(NATIVE_ENTRY_OBJCOPY)'
endef
$(foreach f,$(NATIVE_BIN_FILES),$(eval $(call recon-native-asset,exe_$(call entry-file-key,$(f)),$(call entry-file-path,$(f)))))

# Toolchain coordinator: include this file with DEFAULT_GOAL=native-bins.
define recon-native-program
.PHONY: native-bin-$(1)
native-bin-$(1):
	+$$(MAKE) -C '$(abspath $(2))' native-bin
native-bins: native-bin-$(1)
endef
.PHONY: native-bins

# Call after including recon-tool.mk. Fourth argument: optional ID=project/path.
# Optional RECON_MODULE_<id>_{IMPORTS,CONSTRUCTORS,RESET}; defaults reject/region.
define recon-native-module
$(if $(filter $(1),$(RECON_ENTRY_MODULE_IDS)),$(error duplicate native module ID $(1)))
RECON_ENTRY_MODULE_IDS += $(1)
NATIVE_MODULE_OBJECTS += $(abspath $(NATIVE_ENTRY_BUILD)/modules/$(1)/bundle.o)
NATIVE_MODULE_SCRIPTS += $(abspath $(NATIVE_ENTRY_BUILD)/modules/$(1)/bundle.ld)
$(abspath $(NATIVE_ENTRY_BUILD)/modules/$(1)/bundle.o): recon-entry-force
	+$$(MAKE) -C '$(abspath $(2))' native-module \
	  RECON_MODULE_ID='$(1)' RECON_MODULE_OUTPUT='$$@' \
	  RECON_MODULE_EXPORTS='$(strip $(3))' \
	  RECON_MODULE_IMPORTS='$$(RECON_MODULE_$(1)_IMPORTS)' \
	  RECON_MODULE_CONSTRUCTORS='$$(or $$(RECON_MODULE_$(1)_CONSTRUCTORS),reject)' \
	  RECON_MODULE_RESET='$$(or $$(RECON_MODULE_$(1)_RESET),region)'
$(abspath $(NATIVE_ENTRY_BUILD)/modules/$(1)/bundle.ld): $(abspath $(NATIVE_ENTRY_BUILD)/modules/$(1)/bundle.o)
	@test -f '$$@'
$(foreach f,$(4),$(eval $(call recon-native-asset,$(1)_$(call entry-file-key,$(f)),$(abspath $(2))/$(call entry-file-path,$(f)))))
endef

# The recursive module project uses its own native flags/object prerequisites.
ifneq ($(strip $(RECON_MODULE_ID)),)
RECON_MODULE_SCRIPT := $(dir $(RECON_MODULE_OUTPUT))bundle.ld
RECON_MODULE_CONFIG := $(RECON_MODULE_OUTPUT).options
$(RECON_MODULE_CONFIG): recon-entry-force
	@mkdir -p $(@D)
	$(NATIVE_ENTRY_PYTHON) $(RECON_ENTRY_HELPER) config --output '$@' --value '$(RECON_MODULE_EXPORTS)' --value '$(RECON_MODULE_IMPORTS)' --value '$(RECON_MODULE_CONSTRUCTORS)' --value '$(RECON_MODULE_RESET)' --value '$(RECON_ENTRY_SOURCES)' --value '$(RECON_ENTRY_SUPPORT_SOURCES)' --value '$(RECON_ALIAS_NATIVE_RSP)'
$(RECON_MODULE_OUTPUT) $(RECON_MODULE_SCRIPT) &: $(RECON_MODULE_CONFIG) $(RECON_ENTRY_APP_OBJECTS) $(RECON_ENTRY_SUPPORT_OBJECTS) $(RECON_ALIAS_NATIVE_RSP) Makefile $(RECON_ENTRY_HELPER) $(RECON_ENTRY_MAKE) | recon-entry-selection
	@mkdir -p $(dir $(RECON_MODULE_OUTPUT))
	$(NATIVE_ENTRY_PYTHON) $(RECON_ENTRY_HELPER) bundle --kind module --id '$(RECON_MODULE_ID)' \
	  --output '$(RECON_MODULE_OUTPUT)' --script '$(RECON_MODULE_SCRIPT)' \
	  --constructors '$(RECON_MODULE_CONSTRUCTORS)' --reset '$(RECON_MODULE_RESET)' \
	  $(foreach s,$(RECON_MODULE_EXPORTS),--export '$(s)') $(foreach b,$(RECON_MODULE_IMPORTS),--bind '$(b)') \
	  $(foreach r,$(RECON_ALIAS_NATIVE_RSP),--alias-rsp '$(r)') \
	  --ld '$(NATIVE_ENTRY_LD)' --objcopy '$(NATIVE_ENTRY_OBJCOPY)' $(RECON_ENTRY_APP_OBJECTS) $(RECON_ENTRY_SUPPORT_OBJECTS)
native-module: $(RECON_MODULE_OUTPUT) $(RECON_MODULE_SCRIPT)
else
native-module:
	@echo 'native-module is called through recon-native-module with an explicit module ID' >&2; exit 1
endif

ifneq ($(wildcard $(NATIVE_ROOT_ENTRY)),)
RECON_NATIVE_ROOT_ENTRY := 1
RECON_ENTRY_APP_BUNDLE := $(NATIVE_ENTRY_BUILD)/application.o
RECON_ENTRY_HOST_OBJECT := $(NATIVE_ENTRY_BUILD)/entry.o
RECON_ENTRY_SUPPORT_STAMP := $(NATIVE_ENTRY_BUILD)/support.ok
RECON_ENTRY_APP_CONFIG := $(NATIVE_ENTRY_BUILD)/application.options
$(RECON_ENTRY_APP_CONFIG): recon-entry-force
	@mkdir -p $(@D)
	$(NATIVE_ENTRY_PYTHON) $(RECON_ENTRY_HELPER) config --output '$@' --value '$(NATIVE_APP_MAIN)' --value '$(NATIVE_APP_MAIN_SYMBOL)' --value '$(NATIVE_BIN_IMPORTS)' --value '$(RECON_ENTRY_SOURCES)'
$(RECON_ENTRY_APP_BUNDLE): $(RECON_ENTRY_APP_CONFIG) $(RECON_ENTRY_APP_OBJECTS) Makefile $(RECON_ENTRY_HELPER) $(RECON_ENTRY_MAKE) | recon-entry-selection
	@mkdir -p $(@D)
	$(NATIVE_ENTRY_PYTHON) $(RECON_ENTRY_HELPER) bundle --kind app --output '$@' --main-name '$(NATIVE_APP_MAIN)' --main-symbol '$(NATIVE_APP_MAIN_SYMBOL)' \
	  $(foreach b,$(NATIVE_BIN_IMPORTS),--bind '$(b)') --ld '$(NATIVE_ENTRY_LD)' --objcopy '$(NATIVE_ENTRY_OBJCOPY)' $(RECON_ENTRY_APP_OBJECTS)
$(RECON_ENTRY_HOST_OBJECT): $(NATIVE_ROOT_ENTRY) Makefile $(RECON_ENTRY_MAKE)
	@mkdir -p $(@D)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(filter-out -finstrument-functions,$(NATIVE_CFLAGS)) -URECON_NATIVE_ENTRY -Umain $(NATIVE_BIN_ENTRY_CPPFLAGS) -MMD -MP -c $< -o $@
$(RECON_ENTRY_SUPPORT_STAMP): $(RECON_ENTRY_SUPPORT_OBJECTS) Makefile $(RECON_ENTRY_HELPER) $(RECON_ENTRY_MAKE) | recon-entry-selection
	@mkdir -p $(@D)
	$(NATIVE_ENTRY_PYTHON) $(RECON_ENTRY_HELPER) support $(RECON_ENTRY_SUPPORT_OBJECTS)
	@touch $@
.SECONDEXPANSION:
$(NATIVE_BINARY): $(RECON_ENTRY_APP_BUNDLE) $(RECON_ENTRY_HOST_OBJECT) $(RECON_ENTRY_SUPPORT_OBJECTS) $(RECON_ENTRY_SUPPORT_STAMP) $(RECON_ALIAS_NATIVE_RSP) $$(NATIVE_MODULE_OBJECTS) $$(NATIVE_MODULE_SCRIPTS) $$(NATIVE_ASSET_OBJECTS) $(RECON_ENTRY_MAKE)
	@mkdir -p $(@D)
	$(NATIVE_ENTRY_LINKER) -m32 -no-pie -Wl,-z,defs $(NATIVE_LDFLAGS) -o $@ \
	  $(RECON_ENTRY_HOST_OBJECT) $(RECON_ENTRY_APP_BUNDLE) $(RECON_ENTRY_SUPPORT_OBJECTS) \
	  $(NATIVE_MODULE_OBJECTS) $(NATIVE_ASSET_OBJECTS) $(foreach s,$(NATIVE_MODULE_SCRIPTS),-Wl,-T,$(s)) $(NATIVE_LDLIBS) -ldl
-include $(RECON_ENTRY_HOST_OBJECT:.o=.d)
endif

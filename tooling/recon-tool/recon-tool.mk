# Include once from a binary root, after source lists and compiler configuration.
RECON_TOOL_DIR ?= $(abspath ../../../tooling/recon-tool)
RECON_TOOL ?= $(RECON_TOOL_DIR)/target/i686-unknown-linux-gnu/release/recon-tool
.DEFAULT_GOAL := all
RECON_TOOL_SOURCES := $(wildcard $(RECON_TOOL_DIR)/src/*.rs) $(RECON_TOOL_DIR)/Cargo.toml $(RECON_TOOL_DIR)/Cargo.lock $(RECON_TOOL_DIR)/.cargo/config.toml $(RECON_TOOL_DIR)/build-quiet.sh
$(RECON_TOOL): $(RECON_TOOL_SOURCES)
	@$(RECON_TOOL_DIR)/build-quiet.sh

include $(RECON_TOOL_DIR)/aliases.mk
include $(RECON_TOOL_DIR)/binary.mk
include $(RECON_TOOL_DIR)/original-build.mk

# Discover descriptors directly; cases do not carry their own build files.
CASES_ROOT ?= cases
CASE_DESCRIPTORS := $(wildcard $(CASES_ROOT)/[0-9][0-9][0-9][0-9]-*/case.c $(CASES_ROOT)/[0-9][0-9][0-9][0-9]-*/case.cpp)
CASE_DIRS := $(sort $(patsubst %/,%,$(dir $(CASE_DESCRIPTORS))))
CASE_TARGETS := $(addsuffix /.build/case.so,$(CASE_DIRS))
include $(RECON_TOOL_DIR)/case.mk
CASE_CHECK_TARGETS := $(addsuffix /check,$(CASE_DIRS))
CASE_UNIT_TARGETS := $(addsuffix /unit,$(CASE_DIRS))
define recon-case-check-rules
.PHONY: $(1)/check $(1)/unit
$(1)/check: $(1)/.build/case.so $(RECON_TOOL)
	$$(RECON_TOOL) run $(1) $$(RECON_RUN_FLAGS)
$(1)/unit: $(1)/.build/case.so $(RECON_TOOL)
	$$(RECON_TOOL) test $(1) $$(RECON_TEST_FLAGS)
endef
$(foreach dir,$(CASE_DIRS),$(eval $(call recon-case-check-rules,$(dir))))
.PHONY: all native native-trace cases check replay fixtures unit test-all-cases clean $(CASE_DIRS)
all native native-trace: recon-native
cases: $(CASE_TARGETS)
check replay fixtures: $(CASE_CHECK_TARGETS)
	@test -n "$(CASE_DIRS)" || { echo 'No numbered case directories found' >&2; exit 1; }
clean:
	rm -rf -- $(BINARY_BUILD) $(ORIGINAL_BUILD) $(addsuffix /.build,$(CASE_DIRS))
unit test-all-cases: $(CASE_UNIT_TARGETS)
	@test -n "$(CASE_DIRS)" || { echo 'No numbered case directories found' >&2; exit 1; }

# Optional graph audit: the profile and target-specific flags remain explicit.
# Local Makefiles may append data/grammar checks to audit without overriding a
# shared recipe. Generated metadata/runtime-index rules remain local as well.
ifneq ($(strip $(RECON_AUDIT_PROFILE)),)
RECON_REPO_ROOT ?= $(abspath $(RECON_TOOL_DIR)/../..)
RECON_REGISTRY ?= $(BINARY_BUILD)/functions.json
RECON_AUDIT_REPORT ?= $(ORIGINAL_BUILD)/graph-audit.json
ifeq ($(RECON_AUDIT_REGISTRY_METADATA),1)
RECON_AUDIT_METADATA ?= $(ORIGINAL_BUILD)/graph-metadata.json
endif
include $(RECON_TOOL_DIR)/../graph-audit/graph-audit.mk
ifeq ($(RECON_AUDIT_REGISTRY_RET_CHECKS),1)
RECON_AUDIT_RET_CHECKS ?= $(BINARY_BUILD)/ret-checks.json
RECON_AUDIT_FLAGS += --ret-checks $(RECON_AUDIT_RET_CHECKS)
RECON_AUDIT_PREREQUISITES += $(RECON_AUDIT_RET_CHECKS)
endif
ifeq ($(RECON_AUDIT_REGISTRY_RUNTIME),1)
RECON_AUDIT_ORIGINAL_RUNTIME ?= $(BINARY_BUILD)/runtime-original.json
RECON_AUDIT_FLAGS += --runtime $(RECON_AUDIT_ORIGINAL_RUNTIME)
RECON_AUDIT_PREREQUISITES += $(RECON_AUDIT_ORIGINAL_RUNTIME)
endif
RECON_REGISTRY_OUTPUTS := $(if $(filter 1,$(RECON_AUDIT_REGISTRY_RUNTIME)),$(RECON_AUDIT_ORIGINAL_RUNTIME)) $(RECON_REGISTRY) $(if $(filter 1,$(RECON_AUDIT_REGISTRY_METADATA)),$(RECON_AUDIT_METADATA)) $(if $(filter 1,$(RECON_AUDIT_REGISTRY_RET_CHECKS)),$(RECON_AUDIT_RET_CHECKS))
RECON_PROJECTION_INPUTS := $(if $(RECON_METADATA_REBUILT_MAP),$(ORIGINAL_OUTPUT) $(RECON_METADATA_REBUILT_MAP))
$(RECON_REGISTRY_OUTPUTS) &: binary.c Makefile $(RECON_TOOL_DIR)/recon-tool.mk $(RECON_POLICY) $(RECON_PROJECTION_INPUTS) $(RECON_TOOL) | $(firstword $(CASE_TARGETS))
	@test -n "$(CASE_DIRS)" || { echo 'Registry export needs a numbered case' >&2; exit 1; }
	@test -n "$(RECON_IMAGE_BASE)" || { echo 'Set RECON_IMAGE_BASE for registry export' >&2; exit 1; }
	@mkdir -p $(dir $(RECON_REGISTRY_OUTPUTS))
	$(RECON_TOOL) registry-export $(firstword $(CASE_DIRS)) --image-base $(RECON_IMAGE_BASE) $(if $(RECON_POLICY),--policy $(RECON_POLICY)) --output $(RECON_REGISTRY) $(if $(filter 1,$(RECON_AUDIT_REGISTRY_METADATA)),--metadata-output $(RECON_AUDIT_METADATA)) $(if $(filter 1,$(RECON_AUDIT_REGISTRY_RET_CHECKS)),--ret-checks-output $(RECON_AUDIT_RET_CHECKS)) $(if $(filter 1,$(RECON_AUDIT_REGISTRY_RUNTIME)),--runtime-output $(RECON_AUDIT_ORIGINAL_RUNTIME)) $(if $(RECON_METADATA_REBUILT_MAP),--rebuilt-image $(ORIGINAL_OUTPUT) --rebuilt-map $(RECON_METADATA_REBUILT_MAP))
# CRT naming for the rebuilt image is shared read-only analysis, never Wine.
ifeq ($(RECON_AUDIT_IDENTIFY_RUNTIME),1)
RECON_AUDIT_RUNTIME ?= $(ORIGINAL_BUILD)/runtime.json
RECON_RUNTIME_INPUTS ?= $(ORIGINAL_TOOLCHAIN_ROOT)/lib/LIBC.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/SETARGV.OBJ
ifneq ($(strip $(RECON_RUNTIME_MAP)),)
RECON_RUNTIME_FLAGS += --map $(RECON_RUNTIME_MAP)
RECON_RUNTIME_INPUTS += $(RECON_RUNTIME_MAP)
endif
ifneq ($(strip $(RECON_RUNTIME_OVERRIDE_OBJECTS)),)
RECON_RUNTIME_OVERRIDE_REGISTRY ?= $(RECON_REGISTRY)
RECON_RUNTIME_FLAGS += --override-registry $(RECON_RUNTIME_OVERRIDE_REGISTRY) --override-map $(RECON_RUNTIME_OVERRIDE_MAP) $(foreach o,$(RECON_RUNTIME_OVERRIDE_OBJECTS),--override-object $(o))
RECON_RUNTIME_INPUTS += $(RECON_RUNTIME_OVERRIDE_REGISTRY) $(RECON_RUNTIME_OVERRIDE_MAP) $(RECON_RUNTIME_OVERRIDE_OBJECTS)
endif
$(RECON_AUDIT_RUNTIME): Makefile $(RECON_TOOL_DIR)/recon-tool.mk $(ORIGINAL_OUTPUT) $(RECON_TOOL) $(RECON_RUNTIME_INPUTS)
	@test -n "$(ORIGINAL_TOOLCHAIN)" || { echo 'Runtime identification needs ORIGINAL_TOOLCHAIN' >&2; exit 1; }
	@mkdir -p $(@D)
	$(RECON_TOOL) identify-runtime $(ORIGINAL_TOOLCHAIN) $(ORIGINAL_OUTPUT) --output $@ $(RECON_RUNTIME_FLAGS)
RECON_AUDIT_PREREQUISITES += $(RECON_AUDIT_RUNTIME)
RECON_AUDIT_FLAGS += --rebuilt-runtime $(RECON_AUDIT_RUNTIME)
endif
.PHONY: audit recon-graph-audit
audit: recon-graph-audit
recon-graph-audit: $(GRAPH_AUDIT_BIN) $(RECON_REGISTRY) $(RECON_AUDIT_METADATA) $(RECON_AUDIT_PREREQUISITES)
	@mkdir -p $(dir $(RECON_AUDIT_REPORT))
	"$(GRAPH_AUDIT_BIN)" --profile $(RECON_AUDIT_PROFILE) --project . --repo $(RECON_REPO_ROOT) --config $(RECON_REGISTRY) $(if $(RECON_AUDIT_METADATA),--metadata $(RECON_AUDIT_METADATA)) $(RECON_AUDIT_FLAGS) --report $(RECON_AUDIT_REPORT)
endif

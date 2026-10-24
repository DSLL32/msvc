# Include once from a binary root, after source lists and compiler configuration.
RECON_TOOL_DIR ?= $(abspath ../../../tooling/recon-tool)
RECON_TOOL ?= $(RECON_TOOL_DIR)/target/i686-unknown-linux-gnu/release/recon-tool
.DEFAULT_GOAL := all
RECON_TOOL_SOURCES := $(wildcard $(RECON_TOOL_DIR)/src/*.rs) $(RECON_TOOL_DIR)/Cargo.toml $(RECON_TOOL_DIR)/Cargo.lock $(RECON_TOOL_DIR)/.cargo/config.toml $(RECON_TOOL_DIR)/build-quiet.sh
$(RECON_TOOL): $(RECON_TOOL_SOURCES)
	@$(RECON_TOOL_DIR)/build-quiet.sh

include $(RECON_TOOL_DIR)/binary.mk
include $(RECON_TOOL_DIR)/original-build.mk

# Discover descriptors directly; cases do not carry their own build files.
CASES_ROOT ?= cases
CASE_DESCRIPTORS := $(wildcard $(CASES_ROOT)/[0-9][0-9][0-9][0-9]-*/case.c $(CASES_ROOT)/[0-9][0-9][0-9][0-9]-*/case.cpp)
CASE_DIRS := $(sort $(patsubst %/,%,$(dir $(CASE_DESCRIPTORS))))
CASE_TARGETS := $(addsuffix /.build/case.so,$(CASE_DIRS))
include $(RECON_TOOL_DIR)/case.mk
.PHONY: all native native-trace cases check replay fixtures unit test-all-cases clean $(CASE_DIRS)
all native native-trace: recon-native
cases: $(CASE_TARGETS)
check replay fixtures: $(CASE_TARGETS) $(RECON_TOOL)
	@test -n "$(CASE_DIRS)" || { echo 'No numbered case directories found' >&2; exit 1; }
	$(RECON_TOOL) run $(CASE_DIRS) $(RECON_RUN_FLAGS)
clean:
	rm -rf -- $(BINARY_BUILD) $(ORIGINAL_BUILD) $(addsuffix /.build,$(CASE_DIRS))
unit test-all-cases: $(CASE_TARGETS) $(RECON_TOOL)
	@test -n "$(CASE_DIRS)" || { echo 'No numbered case directories found' >&2; exit 1; }
	$(RECON_TOOL) test $(CASE_DIRS) $(RECON_TEST_FLAGS)

# Optional graph audit: the profile and target-specific flags remain explicit.
# Local Makefiles may append data/grammar checks to audit without overriding a
# shared recipe. Generated metadata/runtime-index rules remain local as well.
ifneq ($(strip $(RECON_AUDIT_PROFILE)),)
RECON_REPO_ROOT ?= $(abspath $(RECON_TOOL_DIR)/../..)
RECON_REGISTRY ?= $(BINARY_BUILD)/functions.json
RECON_AUDIT_REPORT ?= $(ORIGINAL_BUILD)/graph-audit.json
include $(RECON_TOOL_DIR)/../graph-audit/graph-audit.mk
$(RECON_REGISTRY): binary.c Makefile $(RECON_TOOL_DIR)/recon-tool.mk $(RECON_POLICY) $(RECON_TOOL) | $(firstword $(CASE_TARGETS))
	@test -n "$(CASE_DIRS)" || { echo 'Registry export needs a numbered case' >&2; exit 1; }
	@test -n "$(RECON_IMAGE_BASE)" || { echo 'Set RECON_IMAGE_BASE for registry export' >&2; exit 1; }
	@mkdir -p $(@D)
	$(RECON_TOOL) registry-export $(firstword $(CASE_DIRS)) --image-base $(RECON_IMAGE_BASE) $(if $(RECON_POLICY),--policy $(RECON_POLICY)) --output $@
.PHONY: audit recon-graph-audit
audit: recon-graph-audit
recon-graph-audit: $(GRAPH_AUDIT_BIN) $(RECON_REGISTRY) $(RECON_AUDIT_METADATA) $(RECON_AUDIT_PREREQUISITES)
	@mkdir -p $(dir $(RECON_AUDIT_REPORT))
	"$(GRAPH_AUDIT_BIN)" --profile $(RECON_AUDIT_PROFILE) --project . --repo $(RECON_REPO_ROOT) --config $(RECON_REGISTRY) $(if $(RECON_AUDIT_METADATA),--metadata $(RECON_AUDIT_METADATA)) $(RECON_AUDIT_FLAGS) --report $(RECON_AUDIT_REPORT)
endif

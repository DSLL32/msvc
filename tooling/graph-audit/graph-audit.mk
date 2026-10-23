# Include from a binary Makefile, then add $(GRAPH_AUDIT_BIN) to audit prerequisites.
# Invoke that binary directly: Cargo output suppression must not hide tool errors.
ifndef GRAPH_AUDIT_MK_INCLUDED
GRAPH_AUDIT_MK_INCLUDED := 1
GRAPH_AUDIT_SAVED_DEFAULT_GOAL := $(.DEFAULT_GOAL)
ifndef GRAPH_AUDIT_DIR
GRAPH_AUDIT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
endif
GRAPH_AUDIT_BIN := $(GRAPH_AUDIT_DIR)/target/release/graph-audit
export GRAPH_AUDIT_BIN
GRAPH_AUDIT_BUILD_SCRIPT := $(GRAPH_AUDIT_DIR)/build-quiet.sh
GRAPH_AUDIT_BUILD_DEPS := $(GRAPH_AUDIT_DIR)/Cargo.toml $(GRAPH_AUDIT_DIR)/Cargo.lock \
  $(wildcard $(GRAPH_AUDIT_DIR)/src/*.rs $(GRAPH_AUDIT_DIR)/profiles/*.json) \
  $(GRAPH_AUDIT_BUILD_SCRIPT) $(GRAPH_AUDIT_DIR)/graph-audit.mk

.PHONY: graph-audit-tool
graph-audit-tool: $(GRAPH_AUDIT_BIN)

$(GRAPH_AUDIT_BIN): $(GRAPH_AUDIT_BUILD_DEPS)
	@"$(GRAPH_AUDIT_BUILD_SCRIPT)"
	@test -x "$@"
	@touch -c "$@"
ifeq ($(GRAPH_AUDIT_SAVED_DEFAULT_GOAL),)
.DEFAULT_GOAL :=
endif
endif

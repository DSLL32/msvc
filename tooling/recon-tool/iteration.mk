# Opt-in orchestration only. Project hooks/configuration remain in its Makefile.
# Nothing here participates in all/check/unit/default builds.
RECON_ITERATION_INPUT ?= $(RECON_ITERATION_CASE)/capture.bin
RECON_ITERATION_METADATA_PREREQUISITES ?= $(filter-out $(RECON_REGISTRY_OUTPUTS),$(RECON_AUDIT_PREREQUISITES)) $(if $(filter 1,$(RECON_AUDIT_REGISTRY_METADATA)),,$(RECON_AUDIT_METADATA))
RECON_ITERATION_CAPTURE_FLAGS ?= $(if $(RECON_CF_PROFILE),--cf-profile $(RECON_CF_PROFILE))
.PHONY: recon-iteration-config recon-iteration-native recon-iteration-original recon-iteration-metadata
recon-iteration-config:
	$(if $(filter 1,$(RECON_ITERATION_ENABLED)),,$(error Set RECON_ITERATION_ENABLED=1 after declaring the applicable audit flags and target-specific prerequisites/hooks))
	$(if $(RECON_ITERATION_CONFIG),,$(error RECON_ITERATION_CONFIG is required))
	$(file >$(RECON_ITERATION_CONFIG),version=1)
	$(file >>$(RECON_ITERATION_CONFIG),project=$(CURDIR))
	$(file >>$(RECON_ITERATION_CONFIG),case=$(RECON_ITERATION_CASE))
	$(file >>$(RECON_ITERATION_CONFIG),input=$(RECON_ITERATION_INPUT))
	$(file >>$(RECON_ITERATION_CONFIG),library=$(BINARY_LIBRARY))
	$(file >>$(RECON_ITERATION_CONFIG),original=$(ORIGINAL_OUTPUT))
	$(file >>$(RECON_ITERATION_CONFIG),map=$(ORIGINAL_MAP))
	$(file >>$(RECON_ITERATION_CONFIG),image_base=$(RECON_IMAGE_BASE))
	$(file >>$(RECON_ITERATION_CONFIG),policy=$(RECON_POLICY))
	$(file >>$(RECON_ITERATION_CONFIG),cf_profile=$(RECON_CF_PROFILE))
	$(file >>$(RECON_ITERATION_CONFIG),metadata=$(RECON_AUDIT_METADATA))
	$(file >>$(RECON_ITERATION_CONFIG),ret_checks=$(RECON_AUDIT_RET_CHECKS))
	$(file >>$(RECON_ITERATION_CONFIG),registry_metadata=$(RECON_AUDIT_REGISTRY_METADATA))
	$(file >>$(RECON_ITERATION_CONFIG),registry_ret=$(RECON_AUDIT_REGISTRY_RET_CHECKS))
	$(file >>$(RECON_ITERATION_CONFIG),graph=$(GRAPH_AUDIT_BIN))
	$(file >>$(RECON_ITERATION_CONFIG),profile=$(RECON_AUDIT_PROFILE))
	$(file >>$(RECON_ITERATION_CONFIG),audit_flags=$(RECON_AUDIT_FLAGS))
	$(file >>$(RECON_ITERATION_CONFIG),capture_flags=$(RECON_ITERATION_CAPTURE_FLAGS))
	$(file >>$(RECON_ITERATION_CONFIG),check_targets=$(RECON_ITERATION_CHECK_TARGETS))
	$(file >>$(RECON_ITERATION_CONFIG),makefiles=$(MAKEFILE_LIST))
	$(file >>$(RECON_ITERATION_CONFIG),inputs=$(MAKEFILE_LIST) $(BINARY_ALL_SOURCES) $(BINARY_SUPPORT_SOURCES) $(BINARY_TOOL_SUPPORT) $(ORIGINAL_SOURCES) $(ORIGINAL_EXTRA_DEPS) $(ORIGINAL_LDLIBS) $(RECON_POLICY) $(RECON_CF_PROFILE) $(RECON_ITERATION_INPUTS))
	$(file >>$(RECON_ITERATION_CONFIG),depfiles=$(BINARY_OBJECTS:.o=.d) $(BINARY_SUPPORT_OBJECTS:.o=.d) $(patsubst %.o,%.d,$(call case-objects,$(RECON_ITERATION_CASE))) $(ORIGINAL_DEPENDENCIES))
	$(file >>$(RECON_ITERATION_CONFIG),artifacts=$(BINARY_LIBRARY) $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) $(RECON_ITERATION_ARTIFACTS))
	@:
recon-iteration-native: $(RECON_ITERATION_CASE)/.build/case.so $(RECON_ITERATION_NATIVE_PREREQUISITES)
recon-iteration-original: $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
	@test -n "$(ORIGINAL_OUTPUT)" -a -n "$(ORIGINAL_MAP)" || { echo 'Iteration needs ORIGINAL_OUTPUT and ORIGINAL_MAP' >&2; exit 1; }
recon-iteration-metadata: $(RECON_ITERATION_METADATA_PREREQUISITES)

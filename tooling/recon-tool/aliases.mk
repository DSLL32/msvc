# One optional binary-specific identity map; no generated function bodies.
ifneq ($(strip $(RECON_ALIAS_MAP)),)
RECON_ALIAS_BUILD ?= .build/aliases
RECON_ALIAS_COFF ?= $(RECON_ALIAS_BUILD)/symbol-aliases.obj
RECON_ALIAS_NATIVE_RSP ?= $(RECON_ALIAS_BUILD)/symbol-aliases.rsp
$(RECON_ALIAS_COFF) $(RECON_ALIAS_NATIVE_RSP) &: $(RECON_ALIAS_MAP) $(RECON_TOOL_DIR)/symbol-aliases.py $(RECON_TOOL_DIR)/aliases.mk
	@mkdir -p $(RECON_ALIAS_BUILD)
	python3 $(RECON_TOOL_DIR)/symbol-aliases.py $(RECON_ALIAS_MAP) --coff $(RECON_ALIAS_COFF) --native-rsp $(RECON_ALIAS_NATIVE_RSP)
NATIVE_LDFLAGS += -Wl,@$(RECON_ALIAS_NATIVE_RSP)
ORIGINAL_LDLIBS += $(RECON_ALIAS_COFF)
endif

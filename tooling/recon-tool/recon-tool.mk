# Include from binary root. Override RECON_TOOL_DIR for a different layout.
RECON_TOOL_DIR ?= $(abspath ../../../tooling/recon-tool)
RECON_TOOL ?= $(RECON_TOOL_DIR)/target/i686-unknown-linux-gnu/release/recon-tool
RECON_TOOL_SOURCES := $(wildcard $(RECON_TOOL_DIR)/src/*.rs) $(RECON_TOOL_DIR)/Cargo.toml $(RECON_TOOL_DIR)/Cargo.lock $(RECON_TOOL_DIR)/.cargo/config.toml $(RECON_TOOL_DIR)/build-quiet.sh
$(RECON_TOOL): $(RECON_TOOL_SOURCES)
	@$(RECON_TOOL_DIR)/build-quiet.sh

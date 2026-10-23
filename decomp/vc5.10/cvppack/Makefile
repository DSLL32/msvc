PYTHON ?= python3

.PHONY: all native check audit inspect capture-help
inspect:
	$(PYTHON) scripts/inventory.py

# Groundwork only. These targets must not silently run an original or claim
# a native reconstruction exists before the orchestrator releases the project.
all native check audit:
	@echo "CVPACK groundwork only: reconstruction awaits DLL workflow and Rust audit review." >&2
	@exit 2

# Explicit original-only capture, never a fixture dependency.
capture-help:
	$(PYTHON) scripts/capture-help.py

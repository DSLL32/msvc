PYTHON ?= python3

.PHONY: all inspect groundwork check-groundwork native original fixtures
all: groundwork

inspect:
	$(PYTHON) scripts/groundwork.py

groundwork check-groundwork:
	$(PYTHON) scripts/groundwork.py --check cases/inventory.json

native original fixtures:
	@echo "C1.DLL groundwork only: DLL workflow, Rust audit and reconstruction release are pending." >&2
	@exit 2

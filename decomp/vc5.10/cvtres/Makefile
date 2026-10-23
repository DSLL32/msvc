PYTHON ?= python3
.DEFAULT_GOAL := inspect
.PHONY: inspect original native fixtures audit help
inspect:
	$(PYTHON) scripts/pe_inventory.py
help:
	@echo 'Groundwork only. inspect is read-only; build/test targets fail closed.'
original native fixtures audit:
	$(PYTHON) scripts/not-ready.py

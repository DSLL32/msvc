PYTHON ?= python3

.PHONY: all groundwork inspect capture-original-help native original check
all: groundwork
groundwork: inspect
inspect:
	$(PYTHON) scripts/inspect.py
capture-original-help:
	$(PYTHON) scripts/probe-original.py
native original check:
	@echo 'C2 reconstruction is not released: DLL workflow and shared Rust audit gate pending.' >&2
	@exit 2

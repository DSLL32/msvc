PYTHON ?= python3
.PHONY: all inspect native original fixtures check probe-help probe-minimal probe-rich
all: inspect
inspect:
	$(PYTHON) scripts/inspect.py
native original fixtures check:
	@echo 'Not ready: reconstruction is gated on DLL workflow approval and Rust graph-audit readiness.' >&2
	@exit 2
# Explicit unmodified originals; never dependencies of native fixtures/checks.
probe-help:
	$(PYTHON) scripts/probe-original.py help
probe-minimal:
	$(PYTHON) scripts/probe-original.py minimal
probe-rich:
	$(PYTHON) scripts/probe-original.py resources-rich

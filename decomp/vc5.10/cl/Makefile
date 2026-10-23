PYTHON ?= python3
.PHONY: all inspect check native original fixtures
all: inspect
inspect:
	$(PYTHON) scripts/inspect.py --check
check: inspect
native original fixtures:
	@echo '$@: not ready; target reconstruction awaits DLL workflow approval and Rust graph-audit readiness.' >&2
	@exit 2

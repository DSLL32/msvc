PYTHON ?= python3

.PHONY: all native reference-build metadata check
all: native

# GCC's native ABI is authorized. The application scaffold is not yet accepted.
native:
	@printf '%s\n' 'HOLD: application-entry scaffold and capture plan await review; native will use GCC ABI.' >&2
	@exit 2

reference-build:
	@printf '%s\n' 'HOLD: WLIB reconstructed source and original-link recipe await review.' >&2
	@exit 2

metadata:
	$(PYTHON) scripts/metadata.py

check:
	$(PYTHON) scripts/metadata.py --check binary.json

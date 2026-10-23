PYTHON ?= python3

.PHONY: all native reference-build metadata check
all: native

# These are deliberate holds, not successful builds or regression claims.
native:
	@printf '%s\n' 'HOLD: native Watcom EAX/EDX/EBX/ECX ABI support is awaiting review.' >&2
	@exit 2

reference-build:
	@printf '%s\n' 'HOLD: original Watcom compiler/stub execution has not been approved or verified.' >&2
	@exit 2

metadata:
	$(PYTHON) scripts/metadata.py

check:
	$(PYTHON) scripts/metadata.py --check binary.json

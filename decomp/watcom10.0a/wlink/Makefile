.DEFAULT_GOAL := inspect
PYTHON ?= python3

.PHONY: inspect native original fixtures
inspect:
	$(PYTHON) scripts/inventory.py

# Deliberate gates: no application function or reached CRT boundary has yet
# been accepted. Native GCC ABI is authorized; original builds retain Watcom ABI.
native:
	@echo 'HOLD: native i386 target awaits source/frontier and reached CRT boundary review.' >&2
	@exit 2
original:
	@echo 'HOLD: compiler probe passed; target link/source and startup boundaries await review.' >&2
	@exit 2
fixtures:
	@echo 'HOLD: help reference and native reconstruction have not been captured.' >&2
	@exit 2

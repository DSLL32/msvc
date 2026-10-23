.DEFAULT_GOAL := inspect
PYTHON ?= python3

.PHONY: inspect native original fixtures
inspect:
	$(PYTHON) scripts/inventory.py

# Deliberate gates: no application function or Watcom register ABI has yet
# been accepted. These targets must not produce a misleading reconstruction.
native:
	@echo 'HOLD: native i386 target awaits reviewed Watcom ABI and CRT boundary.' >&2
	@exit 2
original:
	@echo 'HOLD: original-toolchain target awaits reviewed compiler/stub invocation.' >&2
	@exit 2
fixtures:
	@echo 'HOLD: help reference and native reconstruction have not been captured.' >&2
	@exit 2

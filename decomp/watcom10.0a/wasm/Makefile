PYTHON ?= python3

.PHONY: all inspect native win32 fixtures
all: inspect

inspect:
	$(PYTHON) scripts/inspect-image.py

# Native GCC ABIs are authorized; CF transport and CRT profiles remain gated.
# They must not run the original compiler or reference loader implicitly.
native win32 fixtures:
	@echo '$@: blocked pending CF masking and Watcom CRT boundary review' >&2
	@exit 2

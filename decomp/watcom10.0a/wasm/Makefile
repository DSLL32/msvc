PYTHON ?= python3

.PHONY: all inspect native win32 fixtures
all: inspect

inspect:
	$(PYTHON) scripts/inspect-image.py

# These gates deliberately fail until the common Watcom ABI/CF review completes.
# They must not run the original compiler or reference loader implicitly.
native win32 fixtures:
	@echo '$@: blocked pending Watcom register ABI and CF boundary review' >&2
	@exit 2

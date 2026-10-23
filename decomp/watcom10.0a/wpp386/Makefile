PYTHON ?= python3

.PHONY: all status inspect native original fixtures
all: status

status:
	@$(PYTHON) scripts/scaffold.py status

inspect:
	@$(PYTHON) scripts/scaffold.py inspect

# Deliberately fail until the shared Watcom ABI/CF transport is reviewed.
# These targets must never produce an apparent successful reconstruction build.
native original fixtures:
	@$(PYTHON) scripts/scaffold.py blocked $@

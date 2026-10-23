PYTHON ?= python3

.PHONY: all status inspect native original fixtures
all: status

status:
	@$(PYTHON) scripts/scaffold.py status

inspect:
	@$(PYTHON) scripts/scaffold.py inspect

# Deliberately fail until the shared CF transport/layout is reviewed.
# Native GCC ABI selection is settled; exact original ABI is audited separately.
# These targets must never produce an apparent successful reconstruction build.
native original fixtures:
	@$(PYTHON) scripts/scaffold.py blocked $@

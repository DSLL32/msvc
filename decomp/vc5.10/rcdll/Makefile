.PHONY: all inspect check-plan native original fixture fixtures test
all: check-plan
inspect:
	python3 scripts/pe_inventory.py
check-plan:
	python3 scripts/check-plan.py
native original fixture fixtures test:
	@echo "RCDLL groundwork only: DLL workflow and Rust graph audit await release; no reconstruction or fixture target exists." >&2
	@exit 125

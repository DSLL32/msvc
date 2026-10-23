.PHONY: all inspect verify-groundwork native original fixtures replay

all: inspect

inspect:
	python3 scripts/inventory.py

verify-groundwork:
	python3 scripts/inventory.py --check cases/pe-inventory.json

native original fixtures replay:
	@echo 'C1XX is groundwork only: DLL harness, Rust audit, and reconstruction release are pending.' >&2
	@exit 2

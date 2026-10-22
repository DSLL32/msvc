PYTHON ?= python3
.PHONY: all check
all: build/walker.exe
build/walker.exe: src/walker.c scripts/build.py Makefile
	$(PYTHON) scripts/build.py
check: build/walker.exe
	$(PYTHON) scripts/check.py

PYTHON ?= python3
NATIVE_CC ?= gcc
RECON = ../../../lib/recon
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_FLAGS = -m32 -std=gnu89 -O0 -g -fno-omit-frame-pointer -fno-pie

.PHONY: all status inspect native original fixtures
all: native

status:
	@$(PYTHON) scripts/scaffold.py status

inspect:
	@$(PYTHON) scripts/scaffold.py inspect

native: build/native/WPP386

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native/WPP386: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

original: build/WPP386.EXE build/wpp386.map

build:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) scripts/original.py | build
	$(PYTHON) scripts/original.py compile $< $@

build/WPP386.EXE build/wpp386.map &: $(OBJECTS) scripts/original.py Makefile
	$(PYTHON) scripts/original.py link $(OBJECTS)

# Built executables contain an unreconstructed INT3 main, not an accepted case.
# No fixture can fall back to an original execution.
fixtures:
	@$(PYTHON) scripts/scaffold.py blocked $@

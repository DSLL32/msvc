CC ?= gcc
PYTHON ?= python3
CPPFLAGS += -DRECON_NATIVE_TEST -Iinclude -Inative
CFLAGS += -m32 -std=gnu89 -O0 -g -Wall -Wextra
SOURCES = src/crt.c src/win32.c src/process.c src/compat.c src/entry.c src/decimal.c
HEADERS = $(wildcard include/*.h native/*.h native/sys/*.h)
.PHONY: all check find-check module-check initterm-check startup-check resource-check status-check jump-storage-check system-check cpp-new-check setjmp3-check format-inserts-check multibyte-surface-check private-heap-check tick-context-check onexit-check editor-api-check mb-ascii-check cxx-check system-profile-check mapped-file-check stdio-profile-check mb-space-check local-memory-check mb-dec-check module-memory-check file-time-check mb-rchr-check mb-nbicmp-check mb-chr-check stdio-ownership-check exports-check trace-check process-check process-stream-check trace-allocation-check adoption-check clean
all: build/check
build:
	mkdir -p $@
build/check: tests/check.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check.c $(SOURCES) -o $@
check: build/check
	./build/check
build/find-probe: tests/find-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/find-probe.c $(SOURCES) -o $@
find-check: build/find-probe
	$(PYTHON) tests/check-find.py build/find-probe
build/module-probe: tests/module-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/module-probe.c $(SOURCES) -o $@
build/check-module: tests/check-module.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check-module.c $(SOURCES) -o $@
module-check: build/module-probe build/check-module
	$(PYTHON) tests/check-module.py build/module-probe build/check-module
build/check-initterm: tests/check-initterm.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check-initterm.c $(SOURCES) -o $@
build/entry-precision-default: tests/entry-precision-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_ENTRY=recon_precision_entry tests/entry-precision-target.c $(SOURCES) -o $@
build/entry-precision-53: tests/entry-precision-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_ENTRY=recon_precision_entry -DRECON_NATIVE_ENTRY_CONTROLFP_53 tests/entry-precision-target.c $(SOURCES) -o $@
initterm-check: build/check-initterm build/entry-precision-default build/entry-precision-53
	./build/check-initterm
	$(PYTHON) tests/check-entry-precision.py build/entry-precision-default build/entry-precision-53
build/check-startup: tests/check-startup.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check-startup.c $(SOURCES) -o $@
startup-check: build/check-startup
	$(PYTHON) tests/check-startup.py build/check-startup
build/resource-target: tests/resource-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/resource-target.c $(SOURCES) -o $@
resource-check: build/resource-target
	$(PYTHON) tests/check-resources.py build/resource-target
build/status-target: tests/status-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_ENTRY=recon_status_entry tests/status-target.c $(SOURCES) -o $@
status-check: build/status-target
	$(PYTHON) tests/check-status.py build/status-target
build/jump-storage8: tests/check-jump-storage.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check-jump-storage.c $(SOURCES) -o $@
build/jump-storage16: tests/check-jump-storage.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_LEGACY_JMP_WORDS=16 tests/check-jump-storage.c $(SOURCES) -o $@
jump-storage-check: build/jump-storage8 build/jump-storage16
	./build/jump-storage8
	./build/jump-storage16
	$(PYTHON) -c "import subprocess; p=subprocess.run(['build/jump-storage8','assert'],capture_output=True); assert p.returncode==125"
build/system-target: tests/system-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/system-target.c $(SOURCES) -o $@
system-check: build/system-target
	$(PYTHON) tests/check-system.py build/system-target
build/check-cpp-new: tests/check-cpp-new.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check-cpp-new.c $(SOURCES) -o $@
build/cpp-new-probe: tests/cpp-new-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/cpp-new-probe.c $(SOURCES) -o $@
cpp-new-check: build/check-cpp-new build/cpp-new-probe
	./build/check-cpp-new
	$(PYTHON) -c "from pathlib import Path; import subprocess; p=subprocess.run(['build/cpp-new-probe'],capture_output=True,check=True); assert not p.stderr and p.stdout==Path('tests/cpp-new-original.stdout').read_bytes()"
	$(PYTHON) -c "import subprocess; p=subprocess.run(['build/check-cpp-new','failure'],capture_output=True); assert p.returncode==125"
build/check-setjmp3: tests/check-setjmp3.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_LEGACY_JMP_WORDS=16 -DRECON_NATIVE_SETJMP3_COUNT0 tests/check-setjmp3.c $(SOURCES) -o $@
setjmp3-check: build/check-setjmp3
	$(PYTHON) tests/check-setjmp3.py build/check-setjmp3
build/format-inserts-target: tests/format-inserts-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/format-inserts-target.c $(SOURCES) -o $@
format-inserts-check: build/format-inserts-target
	$(PYTHON) tests/check-format-inserts.py build/format-inserts-target
build/multibyte-surface: tests/multibyte-surface.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/multibyte-surface.c $(SOURCES) -o $@
multibyte-surface-check: build/multibyte-surface
	$(PYTHON) tests/check-multibyte-surface.py build/multibyte-surface
build/private-heap-target: tests/private-heap-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/private-heap-target.c $(SOURCES) -o $@
private-heap-check: build/private-heap-target
	$(PYTHON) tests/check-private-heap.py build/private-heap-target
build/check-tick-context: tests/check-tick-context.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check-tick-context.c $(SOURCES) -o $@
tick-context-check: build/check-tick-context
	./build/check-tick-context
build/onexit-probe: tests/onexit-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/onexit-probe.c $(SOURCES) -o $@
onexit-check: build/onexit-probe
	$(PYTHON) tests/check-onexit.py build/onexit-probe
build/editor-api-probe: tests/editor-api-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/editor-api-probe.c $(SOURCES) -o $@
editor-api-check: build/editor-api-probe
	$(PYTHON) tests/check-editor-api.py build/editor-api-probe
build/mb-ascii-probe: tests/mb-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/mb-ascii-probe.c $(SOURCES) -o $@
mb-ascii-check: build/mb-ascii-probe
	$(PYTHON) tests/check-mb-ascii.py build/mb-ascii-probe
build/cxx-surface.o: tests/cxx-surface.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
build/cxx-surface: build/cxx-surface.o $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) build/cxx-surface.o $(SOURCES) -o $@
cxx-check: build/cxx-surface
	./build/cxx-surface
build/system-profile-probe: tests/system-profile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/system-profile-probe.c $(SOURCES) -o $@
system-profile-check: build/system-profile-probe
	$(PYTHON) tests/check-system-profile.py build/system-profile-probe
build/mapped-file-probe: tests/mapped-file-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/mapped-file-probe.c $(SOURCES) -o $@
mapped-file-check: build/mapped-file-probe
	$(PYTHON) tests/check-mapped-file.py build/mapped-file-probe
build/stdio-profile-probe: tests/stdio-profile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
stdio-profile-check: build/stdio-profile-probe
	$(PYTHON) tests/check-stdio-profile.py build/stdio-profile-probe
build/mb-space-probe: tests/mb-space-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-space-check: build/mb-space-probe
	$(PYTHON) tests/check-mb-space.py build/mb-space-probe
build/local-memory-probe: tests/local-memory-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
local-memory-check: build/local-memory-probe
	$(PYTHON) tests/check-local-memory.py build/local-memory-probe
build/mb-dec-probe: tests/mb-dec-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-dec-check: build/mb-dec-probe
	$(PYTHON) tests/check-mb-dec.py build/mb-dec-probe
build/module-memory-probe: tests/module-memory-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
module-memory-check: build/module-memory-probe
	$(PYTHON) tests/check-module-memory.py build/module-memory-probe
build/file-time-probe: tests/file-time-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
file-time-check: build/file-time-probe
	$(PYTHON) tests/check-file-time.py build/file-time-probe
build/mb-rchr-probe: tests/mb-rchr-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-rchr-check: build/mb-rchr-probe
	$(PYTHON) tests/check-mb-rchr.py build/mb-rchr-probe
build/mb-nbicmp-probe: tests/mb-nbicmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-nbicmp-check: build/mb-nbicmp-probe
	$(PYTHON) tests/check-mb-nbicmp.py build/mb-nbicmp-probe
build/mb-chr-probe: tests/mb-chr-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-chr-check: build/mb-chr-probe
	$(PYTHON) tests/check-mb-chr.py build/mb-chr-probe
build/stdio-ownership-probe: tests/stdio-ownership-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
stdio-ownership-check: build/stdio-ownership-probe
	$(PYTHON) tests/check-stdio-ownership.py build/stdio-ownership-probe
build/check-exports: tests/check-exports.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
exports-check: build/check-exports
	$(PYTHON) tests/check-exports.py build/check-exports
build/trace-target.o: tests/trace-target.c | build
	$(CC) -m32 -std=gnu89 -O0 -g -finstrument-functions -c $< -o $@
build/trace-pie: build/trace-target.o src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIE -pie $^ -o $@
build/trace-exec: build/trace-target.o src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -no-pie $^ -o $@
trace-check: build/trace-pie build/trace-exec
	python3 tests/check-trace.py build/trace-pie build/trace-exec
build/process-target.o: tests/process-target.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -finstrument-functions -c $< -o $@
build/process-target: build/process-target.o $(SOURCES) src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
process-check: build/process-target
	python3 tests/check-process.py build/process-target
adoption-check:
	$(PYTHON) tests/check-adoption.py $(SOURCES)
process-stream-check:
	$(PYTHON) tests/check-spawn-streams.py
trace-allocation-check:
	$(PYTHON) tests/check-trace-allocation.py
clean:
	rm -rf build

build/fsopen-probe: tests/fsopen-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
fsopen-check: build/fsopen-probe
	$(PYTHON) tests/check-fsopen.py build/fsopen-probe
build/check-acp: tests/check-acp.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
acp-check: build/check-acp
	$(PYTHON) tests/check-acp.py build/check-acp

build/getenv-probe: tests/getenv-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
getenv-check: build/getenv-probe
	$(PYTHON) tests/check-getenv.py build/getenv-probe

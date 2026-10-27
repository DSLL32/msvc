CC ?= gcc
PYTHON ?= python3
CPPFLAGS += -DRECON_NATIVE_TEST -Iinclude -Inative
CFLAGS += -m32 -std=gnu89 -O0 -g -Wall -Wextra
SOURCES = src/crt.c src/win32.c src/process.c src/compat.c src/entry.c src/decimal.c
HEADERS = $(wildcard include/*.h native/*.h native/sys/*.h src/*.h)
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
	$(PYTHON) -c "import subprocess; p=subprocess.run(['build/check-cpp-new','failure'],capture_output=True); assert p.returncode==125; p=subprocess.run(['build/check-cpp-new','delete'],capture_output=True); assert p.returncode==0"
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
build/trace-module.o: tests/trace-module.cpp include/recon-trace.h | build
	$(CXX) -m32 -O0 -g -fPIC -fno-exceptions -fno-rtti -finstrument-functions -c $< -o $@
build/trace-module-tracer.o: src/trace.c include/recon-trace.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c $< -o $@
build/trace-module.so: build/trace-module.o build/trace-module-tracer.o
	$(CC) -m32 -shared $^ -o $@
build/trace-module-driver: tests/trace-module-driver.c include/recon-trace.h | build
	$(CC) $(CFLAGS) $< -ldl -o $@
.PHONY: trace-module-check
trace-module-check: build/trace-module.so build/trace-module-driver
	$(PYTHON) tests/check-trace-module.py build/trace-module.so build/trace-module-driver
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

build/ascii-terminal-probe: tests/ascii-terminal-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
ascii-terminal-check: build/ascii-terminal-probe
	$(PYTHON) tests/check-ascii-terminal.py build/ascii-terminal-probe
build/mktemp-probe: tests/mktemp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mktemp-check: build/mktemp-probe
	$(PYTHON) tests/check-mktemp.py build/mktemp-probe
build/check-errno: tests/check-errno.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
errno-check: build/check-errno
	./build/check-errno

build/writefile-probe: tests/writefile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
writefile-check: build/writefile-probe
	$(PYTHON) tests/check-writefile.py build/writefile-probe

build/read-prefetch-probe: tests/read-prefetch-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
read-prefetch-check: build/read-prefetch-probe
	$(PYTHON) tests/check-read-prefetch.py build/read-prefetch-probe

build/fstat-probe: tests/fstat-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
fstat-check: build/fstat-probe
	$(PYTHON) tests/check-fstat.py build/fstat-probe

build/mapview-ex-probe: tests/mapview-ex-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mapview-ex-check: build/mapview-ex-probe
	$(PYTHON) tests/check-mapview-ex.py build/mapview-ex-probe

build/wide-append-probe: tests/wide-append-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fshort-wchar $< $(SOURCES) -o $@
wide-append-check: build/wide-append-probe
	$(PYTHON) tests/check-wide-append.py build/wide-append-probe

build/mb-icmp-probe: tests/mb-icmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-icmp-check: build/mb-icmp-probe
	$(PYTHON) tests/check-mb-icmp.py build/mb-icmp-probe

build/cpinfo-probe: tests/cpinfo-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
cpinfo-check: build/cpinfo-probe
	$(PYTHON) tests/check-cpinfo.py build/cpinfo-probe

build/stdin-profile-default: tests/stdin-profile.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/stdin-profile-510: tests/stdin-profile.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
stdin-profile-check: build/stdin-profile-default build/stdin-profile-510
	$(PYTHON) tests/check-stdin-profile.py build/stdin-profile-default build/stdin-profile-510

build/wctype-probe: tests/wctype-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
wctype-check: build/wctype-probe
	$(PYTHON) tests/check-wctype.py build/wctype-probe

build/towupper-probe: tests/towupper-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
towupper-check: build/towupper-probe
	$(PYTHON) tests/check-towupper.py build/towupper-probe

build/line-format-probe: tests/line-format-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fshort-wchar $< $(SOURCES) -o $@
line-format-check: build/line-format-probe
	$(PYTHON) tests/check-line-format.py build/line-format-probe

build/vprintf-probe: tests/vprintf-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
vprintf-check: build/vprintf-probe
	$(PYTHON) tests/check-vprintf.py build/vprintf-probe

build/startup-precision-default: tests/startup-precision.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/startup-precision-53: tests/startup-precision.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $< $(SOURCES) -o $@
startup-precision-check: build/startup-precision-default build/startup-precision-53
	$(PYTHON) tests/check-startup-precision.py build/startup-precision-default build/startup-precision-53

build/wide-printf-probe: tests/wide-printf-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fshort-wchar -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
wide-printf-check: build/wide-printf-probe
	$(PYTHON) tests/check-wide-printf.py build/wide-printf-probe

build/printf-x87-probe: tests/printf-x87-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
printf-x87-check: build/printf-x87-probe
	$(PYTHON) tests/check-printf-x87.py build/printf-x87-probe

build/mb-clen-probe: tests/mb-clen-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-clen-check: build/mb-clen-probe
	$(PYTHON) tests/check-mb-clen.py build/mb-clen-probe

build/printf-Lg-x87-probe: tests/printf-Lg-x87-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
printf-Lg-x87-check: build/printf-Lg-x87-probe
	$(PYTHON) tests/check-printf-Lg-x87.py build/printf-Lg-x87-probe

build/cpp-delete-probe: tests/cpp-delete-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/check-cpp-ownership: tests/check-cpp-ownership.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
cpp-delete-check: build/cpp-delete-probe build/check-cpp-ownership
	$(PYTHON) tests/check-cpp-delete.py build/cpp-delete-probe build/check-cpp-ownership

build/mb-span-probe: tests/mb-span-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-span-check: build/mb-span-probe
	$(PYTHON) tests/check-mb-span.py build/mb-span-probe

build/wcstol-probe: tests/wcstol-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fshort-wchar $< $(SOURCES) -o $@
wcstol-check: build/wcstol-probe
	$(PYTHON) tests/check-wcstol.py build/wcstol-probe

build/createfile-errno-probe: tests/createfile-errno-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
createfile-errno-check: build/createfile-errno-probe
	$(PYTHON) tests/check-createfile-errno.py build/createfile-errno-probe

build/fullpath-errno-probe: tests/fullpath-errno-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
fullpath-errno-check: build/fullpath-errno-probe
	$(PYTHON) tests/check-fullpath-errno.py build/fullpath-errno-probe

build/mb-pbrk-probe: tests/mb-pbrk-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-pbrk-check: build/mb-pbrk-probe
	$(PYTHON) tests/check-mb-pbrk.py build/mb-pbrk-probe

build/check-strerror: tests/check-strerror.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
strerror-check: build/check-strerror
	$(PYTHON) tests/check-strerror.py build/check-strerror

.PHONY: access-state-check
build/access-state-probe: tests/access-state-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
access-state-check: build/access-state-probe
	$(PYTHON) tests/check-access-state.py build/access-state-probe

.PHONY: searchenv-check
build/searchenv-probe: tests/searchenv-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
searchenv-check: build/searchenv-probe
	$(PYTHON) tests/check-searchenv.py build/searchenv-probe

.PHONY: return8-check
build/return8-probe: tests/return8-callee.c tests/return8-caller.c tests/return8-shape.h $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/return8-callee.c tests/return8-caller.c $(SOURCES) -o $@
return8-check: build/return8-probe
	$(PYTHON) tests/check-return8.py build/return8-probe

.PHONY: module-provider-check
build/module-provider-probe: tests/module-provider-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
module-provider-check: build/module-provider-probe
	$(PYTHON) tests/check-module-provider.py build/module-provider-probe

.PHONY: tempnam-check
build/tempnam-probe: tests/tempnam-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
tempnam-check: build/tempnam-probe
	$(PYTHON) tests/check-tempnam.py build/tempnam-probe

.PHONY: locale-profile-check
build/locale-profile-probe: tests/locale-profile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/locale-profile-default: tests/locale-profile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
locale-profile-check: build/locale-profile-probe build/locale-profile-default
	$(PYTHON) tests/check-locale-profile.py build/locale-profile-probe build/locale-profile-default

.PHONY: environment-mutation-check
build/environment-mutation-probe: tests/environment-mutation-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/environment-mutation-default: tests/environment-mutation-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
environment-mutation-check: build/environment-mutation-probe build/environment-mutation-default
	$(PYTHON) tests/check-environment-mutation.py build/environment-mutation-probe build/environment-mutation-default

.PHONY: locale-ascii-check
build/locale-ascii-probe: tests/locale-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
locale-ascii-check: build/locale-ascii-probe
	$(PYTHON) tests/check-locale-ascii.py build/locale-ascii-probe

.PHONY: spawn-state-check
build/spawn-state-probe: tests/spawn-state-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
spawn-state-check: build/spawn-state-probe
	$(PYTHON) tests/check-spawn-state.py build/spawn-state-probe

.PHONY: fread-profile-check
build/fread-profile-probe: tests/fread-profile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
fread-profile-check: build/fread-profile-probe
	$(PYTHON) tests/check-fread-profile.py build/fread-profile-probe

.PHONY: source-line-format-check
build/source-line-format-probe: tests/source-line-format-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fshort-wchar $< $(SOURCES) -o $@
source-line-format-check: build/source-line-format-probe
	$(PYTHON) tests/check-source-line-format.py build/source-line-format-probe

.PHONY: unlink-state-check
build/unlink-state-probe: tests/unlink-state-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_DEFAULT_FMODE=0 tests/unlink-state-probe.c $(SOURCES) -o $@
unlink-state-check: build/unlink-state-probe
	$(PYTHON) tests/check-unlink-state.py build/unlink-state-probe

.PHONY: ctype-default-check
build/ctype-default-probe: tests/ctype-default-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/ctype-default-probe.c $(SOURCES) -o $@
build/ctype-default-510: tests/ctype-default-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 tests/ctype-default-probe.c $(SOURCES) -o $@
ctype-default-check: build/ctype-default-probe build/ctype-default-510
	$(PYTHON) tests/check-ctype-default.py build/ctype-default-probe build/ctype-default-510

.PHONY: system-info-profile-check
build/system-info-profile-probe: tests/system-info-profile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/system-info-profile-probe.c $(SOURCES) -o $@
system-info-profile-check: build/system-info-profile-probe
	$(PYTHON) tests/check-system-info-profile.py build/system-info-profile-probe

.PHONY: x87-empty-entry-check
build/x87-empty-entry-probe: tests/x87-empty-entry-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/x87-empty-entry-probe.c $(SOURCES) -o $@
x87-empty-entry-check: build/x87-empty-entry-probe
	$(PYTHON) tests/check-x87-empty-entry.py build/x87-empty-entry-probe

.PHONY: top-down-reserve-check
build/top-down-reserve-probe: tests/top-down-reserve-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/top-down-reserve-probe.c $(SOURCES) -o $@
top-down-reserve-check: build/top-down-reserve-probe
	$(PYTHON) tests/check-top-down-reserve.py build/top-down-reserve-probe

.PHONY: pointer-printf-check
build/pointer-printf-probe: tests/pointer-printf-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/pointer-printf-probe.c $(SOURCES) -o $@
build/pointer-printf-510: tests/pointer-printf-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 tests/pointer-printf-probe.c $(SOURCES) -o $@
pointer-printf-check: build/pointer-printf-probe build/pointer-printf-510
	$(PYTHON) tests/check-pointer-printf.py build/pointer-printf-probe
	$(PYTHON) tests/check-pointer-printf.py build/pointer-printf-510

.PHONY: watcom-surface-check
build/watcom-surface-probe: tests/watcom-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A tests/watcom-surface-probe.c $(SOURCES) -o $@
build/watcom-host-probe: tests/watcom-host-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A tests/watcom-host-probe.c $(SOURCES) -o $@
watcom-surface-check: build/watcom-surface-probe build/watcom-host-probe
	./build/watcom-surface-probe
	./build/watcom-host-probe

.PHONY: watcom-open-check
build/watcom-open-probe: tests/watcom-open-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-open-check: build/watcom-open-probe
	$(PYTHON) tests/check-watcom-open.py $<

.PHONY: mb-strstr-check
build/mb-strstr-probe: tests/mb-strstr-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/mb-strstr-probe.c $(SOURCES) -o $@
mb-strstr-check: build/mb-strstr-probe
	$(PYTHON) tests/check-mb-strstr.py build/mb-strstr-probe

.PHONY: watcom-startup-fp-check
build/watcom-startup-fp: tests/watcom-startup-fp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP tests/watcom-startup-fp-probe.c $(SOURCES) -o $@
build/watcom-startup-fp-default: tests/watcom-startup-fp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/watcom-startup-fp-probe.c $(SOURCES) -o $@
watcom-startup-fp-check: build/watcom-startup-fp build/watcom-startup-fp-default
	./build/watcom-startup-fp
	./build/watcom-startup-fp-default

.PHONY: watcom-first-heap-check
build/watcom-first-heap: tests/watcom-first-heap-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP tests/watcom-first-heap-probe.c $(SOURCES) -o $@
watcom-first-heap-check: build/watcom-first-heap
	$(PYTHON) tests/check-watcom-first-heap.py build/watcom-first-heap

.PHONY: zero-buffer-check
build/zero-buffer-probe: tests/zero-buffer-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/zero-buffer-probe.c $(SOURCES) -o $@
zero-buffer-check: build/zero-buffer-probe
	$(PYTHON) tests/check-zero-buffer.py build/zero-buffer-probe

.PHONY: watcom-setjmp-check
build/watcom-setjmp-probe: tests/watcom-setjmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP tests/watcom-setjmp-probe.c $(SOURCES) -o $@
watcom-setjmp-check: build/watcom-setjmp-probe
	$(PYTHON) tests/check-watcom-setjmp.py build/watcom-setjmp-probe

.PHONY: file-retention-check
build/file-retention-probe: tests/file-retention-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/file-retention-probe.c $(SOURCES) -o $@
build/file-retention-510: tests/file-retention-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 tests/file-retention-probe.c $(SOURCES) -o $@
file-retention-check: build/file-retention-probe build/file-retention-510
	$(PYTHON) tests/check-file-retention.py build/file-retention-probe
	$(PYTHON) tests/check-file-retention.py build/file-retention-510

.PHONY: spawn-cp0-check
build/spawn-cp0-probe: tests/spawn-state-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_TEST_SPAWN_CP0 $< $(SOURCES) -o $@
spawn-cp0-check: build/spawn-cp0-probe
	$(PYTHON) tests/check-spawn-state.py build/spawn-cp0-probe cp0

.PHONY: wcschr-check
build/wcschr-probe: tests/wcschr-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
wcschr-check: build/wcschr-probe
	$(PYTHON) tests/check-wcschr.py build/wcschr-probe

.PHONY: mapview-full-check
build/mapview-full-probe: tests/mapview-full-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mapview-full-check: build/mapview-full-probe
	$(PYTHON) tests/check-mapview-full.py build/mapview-full-probe

.PHONY: local-clock-count-check
build/local-clock-count-probe: tests/local-clock-count-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
local-clock-count-check: build/local-clock-count-probe
	$(PYTHON) tests/check-local-clock-count.py build/local-clock-count-probe

.PHONY: mb-codepage-surface-check
build/mb-codepage-surface: tests/mb-codepage-surface.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/mb-codepage-surface.c $(SOURCES) -o $@
mb-codepage-surface-check: build/mb-codepage-surface
	$(PYTHON) tests/check-mb-codepage-surface.py build/mb-codepage-surface

.PHONY: isolated-ctype-check
build/isolated-ctype-probe: tests/isolated-ctype-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 tests/isolated-ctype-probe.c $(SOURCES) -o $@
isolated-ctype-check: build/isolated-ctype-probe
	test -n "$(ISOLATED_CTYPE_REFERENCE)"
	$(PYTHON) tests/check-isolated-ctype.py build/isolated-ctype-probe "$(ISOLATED_CTYPE_REFERENCE)"

.PHONY: locale-codepage-check
build/locale-codepage-probe: tests/locale-codepage-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 tests/locale-codepage-probe.c $(SOURCES) -o $@
locale-codepage-check: build/locale-codepage-probe
	test -n "$(LOCALE_CODEPAGE_REFERENCE)"
	$(PYTHON) tests/check-locale-codepage.py build/locale-codepage-probe "$(LOCALE_CODEPAGE_REFERENCE)"

.PHONY: seek-last-error-check
build/seek-last-error-probe: tests/seek-last-error-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_STARTUP_CONTROLFP_53 tests/seek-last-error-probe.c $(SOURCES) -o $@
seek-last-error-check: build/seek-last-error-probe
	$(PYTHON) tests/check-seek-last-error.py build/seek-last-error-probe

.PHONY: mbctolower-surface-check
build/mbctolower-surface: tests/mbctolower-surface.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/mbctolower-surface.c $(SOURCES) -o $@
mbctolower-surface-check: build/mbctolower-surface
	$(PYTHON) tests/check-mbctolower-surface.py build/mbctolower-surface

.PHONY: mbctolower-byte-check
build/mbctolower-byte-probe: tests/mbctolower-byte-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
mbctolower-byte-check: build/mbctolower-byte-probe
	test -n "$(MBCTOLOWER_REFERENCE)"
	$(PYTHON) tests/check-mbctolower-byte.py build/mbctolower-byte-probe "$(MBCTOLOWER_REFERENCE)"

.PHONY: sopen-lease-check
build/sopen-lease-probe: tests/sopen-lease-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
sopen-lease-check: build/sopen-lease-probe
	test -n "$(SOPEN_LEASE_REFERENCE)"
	$(PYTHON) tests/check-sopen-lease.py build/sopen-lease-probe "$(SOPEN_LEASE_REFERENCE)"

.PHONY: signature-crc-check
build/signature-crc-probe: tests/signature-crc-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $< $(SOURCES) -o $@
signature-crc-check: build/signature-crc-probe
	test -n "$(SIGNATURE_CRC_REFERENCE)"
	$(PYTHON) tests/check-signature-crc.py build/signature-crc-probe "$(SIGNATURE_CRC_REFERENCE)"

.PHONY: mbslwr-surface-check
build/mbslwr-surface-probe: tests/mbslwr-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mbslwr-surface-check: build/mbslwr-surface-probe
	$(PYTHON) tests/check-mbslwr-surface.py build/mbslwr-surface-probe

.PHONY: tmpnam-surface-check
build/tmpnam-surface-probe: tests/tmpnam-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
tmpnam-surface-check: build/tmpnam-surface-probe
	$(PYTHON) tests/check-tmpnam-surface.py $<

.PHONY: towlower-surface-check
build/towlower-surface-probe: tests/towlower-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
towlower-surface-check: build/towlower-surface-probe
	$(PYTHON) tests/check-towlower-surface.py $<

.PHONY: wcsupr-surface-check
build/wcsupr-surface-probe: tests/wcsupr-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
wcsupr-surface-check: build/wcsupr-surface-probe
	$(PYTHON) tests/check-wcsupr-surface.py $<

.PHONY: wcsupr-ascii-check
build/wcsupr-ascii-probe: tests/wcsupr-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
wcsupr-ascii-check: build/wcsupr-ascii-probe build/wcsupr-surface-probe
	$(PYTHON) tests/check-wcsupr-ascii.py $<
	$(PYTHON) tests/check-wcsupr-surface.py build/wcsupr-surface-probe

.PHONY: conout-headless-check
build/conout-headless-default: tests/conout-headless-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/conout-headless-510: tests/conout-headless-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
conout-headless-check: build/conout-headless-default build/conout-headless-510
	test -n "$(CONOUT_REFERENCE)"
	$(PYTHON) tests/check-conout-headless.py $< "$(CONOUT_REFERENCE)"
	$(PYTHON) tests/check-conout-headless.py build/conout-headless-510 "$(CONOUT_REFERENCE)" 510

.PHONY: iswctype-hex-check
build/iswctype-hex-default: tests/iswctype-hex-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/iswctype-hex-510: tests/iswctype-hex-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
iswctype-hex-check: build/iswctype-hex-default build/iswctype-hex-510
	$(PYTHON) tests/check-iswctype-hex.py $< default
	$(PYTHON) tests/check-iswctype-hex.py build/iswctype-hex-510

.PHONY: mb-nbcopy-check
build/mb-nbcopy-default: tests/mb-nbcopy-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/mb-nbcopy-510: tests/mb-nbcopy-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/mb-nbcopy-watcom: tests/mb-nbcopy-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
mb-nbcopy-check: build/mb-nbcopy-default build/mb-nbcopy-510 build/mb-nbcopy-watcom
	$(PYTHON) tests/check-mb-nbcopy.py $<
	$(PYTHON) tests/check-mb-nbcopy.py build/mb-nbcopy-510
	$(PYTHON) tests/check-mb-nbcopy.py build/mb-nbcopy-watcom watcom

.PHONY: undname-surface-check
build/undname-surface-probe: tests/undname-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
undname-surface-check: build/undname-surface-probe
	$(PYTHON) tests/check-undname-surface.py $<

.PHONY: mb-nbcmp-surface-check
build/mb-nbcmp-surface-probe: tests/mb-nbcmp-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mb-nbcmp-surface-check: build/mb-nbcmp-surface-probe
	$(PYTHON) tests/check-mb-nbcmp-surface.py $<

.PHONY: delete-file-check
build/delete-file-probe: tests/delete-file-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
delete-file-check: build/delete-file-probe
	$(PYTHON) tests/check-delete-file.py $<

.PHONY: wcsicmp-surface-check
build/wcsicmp-surface-probe: tests/wcsicmp-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
wcsicmp-surface-check: build/wcsicmp-surface-probe
	$(PYTHON) tests/check-wcsicmp-surface.py $<

.PHONY: wcsicmp-ascii-check
build/wcsicmp-ascii-default: tests/wcsicmp-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/wcsicmp-ascii-510: tests/wcsicmp-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
wcsicmp-ascii-check: build/wcsicmp-ascii-default build/wcsicmp-ascii-510
	$(PYTHON) tests/check-wcsicmp-ascii.py $< default
	$(PYTHON) tests/check-wcsicmp-ascii.py build/wcsicmp-ascii-510

.PHONY: imagehlp-surface-check
build/imagehlp-surface-probe: tests/imagehlp-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
imagehlp-surface-check: build/imagehlp-surface-probe
	$(PYTHON) tests/check-imagehlp-surface.py $<

.PHONY: heap-realloc-surface-check
build/heap-realloc-surface-probe: tests/heap-realloc-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
heap-realloc-surface-check: build/heap-realloc-surface-probe
	$(PYTHON) tests/check-heap-realloc-surface.py $<

.PHONY: watcom-cold-check
build/watcom-cold-probe: tests/watcom-cold-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-cold-check: build/watcom-cold-probe
	./$<

.PHONY: command-line-check
build/command-line-probe: tests/command-line-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
command-line-check: build/command-line-probe
	$(PYTHON) tests/check-command-line.py $<

.PHONY: temporary-surface-check
build/temporary-surface-probe: tests/temporary-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
temporary-surface-check: build/temporary-surface-probe
	$(PYTHON) tests/check-temporary-surface.py $<

.PHONY: watcom-fill-check
build/watcom-fill-probe: tests/watcom-fill-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-fill-check: build/watcom-fill-probe
	$(PYTHON) tests/check-watcom-fill.py $<

.PHONY: watcom-splitpath2-check
build/watcom-splitpath2-probe: tests/watcom-splitpath2-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-splitpath2-check: build/watcom-splitpath2-probe
	$(PYTHON) tests/check-watcom-splitpath2.py $<

.PHONY: watcom-writable-check
build/watcom-writable-probe: tests/watcom-writable-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-writable-check: build/watcom-writable-probe
	$(PYTHON) tests/check-watcom-writable.py $<

.PHONY: watcom-fpos-check
build/watcom-fpos-probe: tests/watcom-fpos-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-fpos-check: build/watcom-fpos-probe
	$(PYTHON) tests/check-watcom-fpos.py $<

.PHONY: watcom-signal-check
build/watcom-signal-probe: tests/watcom-signal-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-signal-check: build/watcom-signal-probe
	$(PYTHON) tests/check-watcom-signal.py $<

.PHONY: local-fixed-check
build/local-fixed-probe: tests/local-fixed-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -Wl,--wrap=malloc,--wrap=calloc -o $@
local-fixed-check: build/local-fixed-probe
	$(PYTHON) tests/check-local-fixed.py $<

.PHONY: watcom-setmode-check
build/watcom-setmode-probe: tests/watcom-setmode-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-setmode-check: build/watcom-setmode-probe
	$(PYTHON) tests/check-watcom-setmode.py $<

.PHONY: checksum-check
build/checksum-probe: tests/checksum-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
checksum-check: build/checksum-probe
	test -n "$(CHECKSUM_INPUT)" -a -n "$(CHECKSUM_MASTER)"
	$(PYTHON) tests/check-checksum.py $< "$(CHECKSUM_INPUT)" "$(CHECKSUM_MASTER)"

.PHONY: towlower-ascii-check
build/towlower-ascii-probe: tests/towlower-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
towlower-ascii-check: build/towlower-ascii-probe build/towlower-surface-probe
	$(PYTHON) tests/check-towlower-ascii.py $<
	$(PYTHON) tests/check-towlower-surface.py build/towlower-surface-probe

build/trace-exec-driver: tests/trace-exec-driver.c include/recon-trace.h | build
	$(CC) $(CFLAGS) $< -ldl -o $@
build/native-image-module.so: tests/native-image-module.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared tests/native-image-module.c $(SOURCES) -o $@
build/native-image-case.so: tests/native-image-case.c | build
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@
build/native-image-driver: tests/native-image-driver.c | build
	$(CC) $(CFLAGS) $< -ldl -o $@
.PHONY: native-migration-check
native-migration-check: build/trace-module.so build/trace-exec-driver build/native-image-module.so build/native-image-case.so build/native-image-driver
	$(PYTHON) tests/check-native-migration.py $^

.PHONY: self-command-check
build/self-command: tests/self-command.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
self-command-check: build/self-command
	$(PYTHON) tests/check-self-command.py $<

.PHONY: filbuf-capacity-check
build/filbuf-capacity-default: tests/filbuf-capacity-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/filbuf-capacity-510: tests/filbuf-capacity-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
filbuf-capacity-check: build/filbuf-capacity-default build/filbuf-capacity-510
	$(PYTHON) tests/check-filbuf-capacity.py $^

.PHONY: forceinline-placement-check
build/forceinline-placement-probe.o: tests/forceinline-placement-probe.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -fPIC -fno-inline -finstrument-functions -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
build/forceinline-placement-probe: build/forceinline-placement-probe.o $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
forceinline-placement-check: build/forceinline-placement-probe
	$(PYTHON) tests/check-forceinline-placement.py build/forceinline-placement-probe.o $<

.PHONY: pure-virtual-check
build/pure-virtual-probe.o: tests/pure-virtual-probe.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -fPIC -fno-inline -finstrument-functions -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
build/pure-virtual-probe: build/pure-virtual-probe.o $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
pure-virtual-check: build/pure-virtual-probe
	$(PYTHON) tests/check-pure-virtual.py build/pure-virtual-probe.o $<

.PHONY: cpp-delete-binding-check
build/cpp-delete-binding.o: tests/cpp-delete-binding.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -fPIC -fno-inline -finstrument-functions -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
build/cpp-delete-binding: build/cpp-delete-binding.o $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
cpp-delete-binding-check: build/cpp-delete-binding
	$(PYTHON) tests/check-cpp-delete-binding.py build/cpp-delete-binding.o $<

.PHONY: namemap-open-check
build/namemap-open-probe.o: tests/namemap-open-probe.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) '-D__declspec(x)=' -m32 -std=gnu++98 -O0 -g -fPIC -fno-inline -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
build/namemap-open-probe: build/namemap-open-probe.o $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
namemap-open-check: build/namemap-open-probe
	./$<

.PHONY: watcom-storage-check
build/watcom-storage-probe: tests/watcom-storage-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-storage-check: build/watcom-storage-probe
	./$<

.PHONY: ltoa-gcvt-check
build/ltoa-gcvt-probe: tests/ltoa-gcvt-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
ltoa-gcvt-check: build/ltoa-gcvt-probe
	./$<
	./$< live

.PHONY: mbsnbcmp-check
build/mbsnbcmp-probe: tests/mbsnbcmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mbsnbcmp-check: build/mbsnbcmp-probe
	$(PYTHON) tests/check-mbsnbcmp.py $<

.PHONY: watcom-buffering-check
build/watcom-buffering-probe: tests/watcom-buffering-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-buffering-check: build/watcom-buffering-probe
	$(PYTHON) tests/check-watcom-buffering.py $<

.PHONY: watcom-getch-check
build/watcom-getch-probe: tests/watcom-getch-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-getch-check: build/watcom-getch-probe
	$(PYTHON) tests/check-watcom-getch.py $<

.PHONY: heap-destroy-check
build/heap-destroy-probe: tests/heap-destroy-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
heap-destroy-check: build/heap-destroy-probe
	$(PYTHON) tests/check-heap-destroy.py $<

.PHONY: watcom-longjmp-check
build/watcom-longjmp-probe: tests/watcom-longjmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-longjmp-check: build/watcom-longjmp-probe
	$(PYTHON) tests/check-watcom-longjmp.py $<

.PHONY: virtual-decommit-check
build/virtual-decommit-probe: tests/virtual-decommit-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
virtual-decommit-check: build/virtual-decommit-probe
	$(PYTHON) tests/check-virtual-decommit.py $< 510

.PHONY: watcom-qsort-check
build/watcom-qsort-probe: tests/watcom-qsort-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_QSORT_WATCOM_1994 $< $(SOURCES) -o $@
watcom-qsort-check: build/watcom-qsort-probe
	$(PYTHON) tests/check-watcom-qsort.py $< variant

.PHONY: watcom-tmpnam-check
build/watcom-tmpnam-probe: tests/watcom-tmpnam-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-tmpnam-check: build/watcom-tmpnam-probe
	$(PYTHON) tests/check-watcom-tmpnam.py $< watcom

.PHONY: mbscmp-check
build/mbscmp-probe: tests/mbscmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
mbscmp-check: build/mbscmp-probe
	$(PYTHON) tests/check-mbscmp.py $< 510

.PHONY: temp-api-check
build/temp-api-probe: tests/temp-api-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
temp-api-check: build/temp-api-probe
	$(PYTHON) tests/check-temp-api.py $< 510

.PHONY: watcom-directory-surface-check
build/watcom-directory-surface-probe: tests/watcom-directory-surface-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-directory-surface-check: build/watcom-directory-surface-probe
	$(PYTHON) tests/check-watcom-directory-surface.py $<

.PHONY: macroman-ascii-check
build/macroman-ascii-probe: tests/macroman-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
macroman-ascii-check: build/macroman-ascii-probe
	$(PYTHON) tests/check-macroman-ascii.py $< 510

.PHONY: wsprintf-decimal-check
build/wsprintf-decimal-probe: tests/wsprintf-decimal-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
wsprintf-decimal-check: build/wsprintf-decimal-probe
	$(PYTHON) tests/check-wsprintf-decimal.py $< 510

.PHONY: format4509-check
build/format4509-target: tests/format-inserts-target.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
format4509-check: build/format4509-target
	$(PYTHON) tests/check-format4509.py $< 510

.PHONY: virtual-prefix-check
build/virtual-prefix-probe: tests/virtual-prefix-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
virtual-prefix-check: build/virtual-prefix-probe
	$(PYTHON) tests/check-virtual-prefix.py $< 510

.PHONY: os-environment-stage-check
build/os-environment-stage-probe: tests/os-environment-stage-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
os-environment-stage-check: build/os-environment-stage-probe
	$(PYTHON) tests/check-os-environment-stage.py $< 510

.PHONY: link-os-insertion-check
build/link-os-insertion-probe: tests/link-os-insertion-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
link-os-insertion-check: build/link-os-insertion-probe
	$(PYTHON) tests/check-link-os-insertion.py $< 510

.PHONY: pragma-format-check
build/pragma-format-probe: tests/pragma-format-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
pragma-format-check: build/pragma-format-probe
	$(PYTHON) tests/check-pragma-format.py $< 510

.PHONY: mbstok-cold-check
build/mbstok-cold-probe: tests/mbstok-cold-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
mbstok-cold-check: build/mbstok-cold-probe
	$<

.PHONY: watcom-rotl-check
build/watcom-rotl-probe: tests/watcom-rotl-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-rotl-check: build/watcom-rotl-probe
	$<

.PHONY: mbstok-first-check
build/mbstok-first-probe: tests/mbstok-first-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
mbstok-first-check: build/mbstok-first-probe
	$(PYTHON) tests/check-mbstok-first.py $< 510

.PHONY: apstudio-format-check
build/apstudio-format-probe: tests/apstudio-format-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
apstudio-format-check: build/apstudio-format-probe
	$(PYTHON) tests/check-apstudio-format.py $< 510

.PHONY: watcom-memicmp-check
build/watcom-memicmp-probe: tests/watcom-memicmp-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-memicmp-check: build/watcom-memicmp-probe
	$(PYTHON) tests/check-watcom-memicmp.py $< watcom

.PHONY: mbstok-continuation-check
build/mbstok-continuation-probe: tests/mbstok-continuation-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
mbstok-continuation-check: build/mbstok-continuation-probe
	$(PYTHON) tests/check-mbstok-continuation.py $< 510

.PHONY: apstudio-reference-check
build/apstudio-reference-probe: tests/apstudio-reference-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
apstudio-reference-check: build/apstudio-reference-probe
	$(PYTHON) tests/check-apstudio-reference.py $< 510

.PHONY: watcom-splitpath-dos-check
build/watcom-splitpath-dos-probe: tests/watcom-splitpath-dos-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_SPLITPATH_WATCOM_DOS $< $(SOURCES) -o $@
watcom-splitpath-dos-check: build/watcom-splitpath-dos-probe
	$(PYTHON) tests/check-watcom-splitpath-dos.py $< dos

.PHONY: tempnam510-apstudio-check
build/tempnam510-apstudio-probe: tests/tempnam510-apstudio-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
tempnam510-apstudio-check: build/tempnam510-apstudio-probe
	$(PYTHON) tests/check-tempnam510-apstudio.py $< 510

.PHONY: gcvt-integral-check
build/gcvt-integral-probe: tests/gcvt-integral-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
gcvt-integral-check: build/gcvt-integral-probe
	$<

.PHONY: format4506-check
build/format4506-probe: tests/format4506-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
format4506-check: build/format4506-probe
	$(PYTHON) tests/check-format4506.py $<

.PHONY: sprintf510-diagnostic-check
build/sprintf510-diagnostic-probe: tests/sprintf510-diagnostic-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
sprintf510-diagnostic-check: build/sprintf510-diagnostic-probe
	$<

.PHONY: format4067-check
build/format4067-probe: tests/format4067-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
format4067-check: build/format4067-probe
	$(PYTHON) tests/check-format4067.py $<

.PHONY: setjmp3-live-resume-check
build/setjmp3-live-resume-probe: tests/setjmp3-live-resume-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_NATIVE_SETJMP3_COUNT0 -DRECON_LEGACY_JMP_WORDS=16 $< $(SOURCES) -o $@
setjmp3-live-resume-check: build/setjmp3-live-resume-probe
	$(PYTHON) tests/check-setjmp3-live-resume.py $< 510

.PHONY: macroman-reverse-check
build/macroman-reverse-probe: tests/macroman-reverse-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
macroman-reverse-check: build/macroman-reverse-probe
	$<

.PHONY: format2151-check
build/format2151-probe: tests/format2151-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
format2151-check: build/format2151-probe
	$(PYTHON) tests/check-format2151.py $<

.PHONY: watcom-strrev-check
build/watcom-strrev-probe: tests/watcom-strrev-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-strrev-check: build/watcom-strrev-probe
	$<

.PHONY: copyfile-check
build/copyfile-probe: tests/copyfile-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -Wl,--wrap=read,--wrap=write,--wrap=close,--wrap=futimens -o $@
copyfile-check: build/copyfile-probe
	$(PYTHON) tests/check-copyfile.py $<

.PHONY: native-entry-foundation-check
native-entry-foundation-check:
	$(PYTHON) tests/entry-foundation/check.py

.PHONY: native-bootstrap-check
native-bootstrap-check:
	$(PYTHON) tests/native-bootstrap/check.py

.PHONY: native-command-check
native-command-check:
	$(PYTHON) tests/native-bootstrap/check-protocol.py

.PHONY: preferred-view-check
build/preferred-view-probe: tests/preferred-view-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/preferred-view-generic: tests/preferred-view-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
preferred-view-check: build/preferred-view-probe build/preferred-view-generic
	$(PYTHON) tests/check-preferred-view.py $^

.PHONY: watcom-alloca-check
build/watcom-alloca-probe: tests/watcom-alloca-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_WATCOM_ALLOCA_400 $< $(SOURCES) -pthread -o $@
build/watcom-alloca-generic: tests/watcom-alloca-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTEST_OTHER_PROFILE $< $(SOURCES) -pthread -o $@
build/watcom-alloca-unselected: tests/watcom-alloca-unselected.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
watcom-alloca-check: build/watcom-alloca-probe build/watcom-alloca-generic build/watcom-alloca-unselected
	$(PYTHON) tests/check-watcom-alloca.py $^

.PHONY: mbtowc-1252-check
build/mbtowc-1252-probe: tests/mbtowc-1252-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/mbtowc-1252-generic: tests/mbtowc-1252-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mbtowc-1252-check: build/mbtowc-1252-probe build/mbtowc-1252-generic
	$(PYTHON) tests/check-mbtowc-1252.py $^

.PHONY: watcom-open-rw-check
build/watcom-open-rw-probe: tests/watcom-open-rw-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
build/watcom-open-rw-generic: tests/watcom-open-rw-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
watcom-open-rw-check: build/watcom-open-rw-probe build/watcom-open-rw-generic
	$(PYTHON) tests/check-watcom-open-rw.py $^

.PHONY: cold-vc5-imports-check
build/cold-vc5-imports-probe.o: tests/cold-vc5-imports-probe.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -DRECON_CRT_PROFILE_MSVCRT_510 -c $< -o $@
build/cold-vc5-imports-generic.o: tests/cold-vc5-imports-probe.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
build/cold-vc5-imports-probe: build/cold-vc5-imports-probe.o $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $^ -o $@
build/cold-vc5-imports-generic: build/cold-vc5-imports-generic.o $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
cold-vc5-imports-check: build/cold-vc5-imports-probe build/cold-vc5-imports-generic
	$(PYTHON) tests/check-cold-vc5-imports.py $^

.PHONY: watcom-strerror-check
build/watcom-strerror-probe: tests/watcom-strerror-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_WATCOM_STRERROR_ENOENT $< $(SOURCES) -o $@
build/watcom-strerror-generic: tests/watcom-strerror-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/watcom-strerror-shim: tests/watcom-strerror-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_WATCOM_STRERROR_ENOENT -DTEST_SHIM $< $(SOURCES) -o $@
build/watcom-strerror-unselected: tests/watcom-strerror-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A -DTEST_UNSELECTED $< $(SOURCES) -o $@
watcom-strerror-check: build/watcom-strerror-probe build/watcom-strerror-generic build/watcom-strerror-shim build/watcom-strerror-unselected
	$(PYTHON) tests/check-watcom-strerror.py $^

.PHONY: fsopen-read-cache-check
build/fsopen-read-cache-probe: tests/fsopen-read-cache-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/fsopen-read-cache-generic: tests/fsopen-read-cache-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
fsopen-read-cache-check: build/fsopen-read-cache-probe build/fsopen-read-cache-generic
	$(PYTHON) tests/check-fsopen-read-cache.py $^

.PHONY: imagehlp-cold-editor-check
build/imagehlp-cold-editor-probe: tests/imagehlp-cold-editor-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/imagehlp-cold-editor-generic: tests/imagehlp-cold-editor-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/imagehlp-cold-editor-types.o: tests/imagehlp-cold-editor-types.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
imagehlp-cold-editor-check: build/imagehlp-cold-editor-probe build/imagehlp-cold-editor-generic build/imagehlp-cold-editor-types.o
	$(PYTHON) tests/check-imagehlp-cold-editor.py build/imagehlp-cold-editor-probe build/imagehlp-cold-editor-generic

.PHONY: cold-ole-uninitialize-check
build/cold-ole-uninitialize-probe: tests/cold-ole-uninitialize-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/cold-ole-uninitialize-generic: tests/cold-ole-uninitialize-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/cold-ole-uninitialize-types.o: tests/cold-ole-uninitialize-types.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
cold-ole-uninitialize-check: build/cold-ole-uninitialize-probe build/cold-ole-uninitialize-generic build/cold-ole-uninitialize-types.o
	$(PYTHON) tests/check-cold-ole-uninitialize.py build/cold-ole-uninitialize-probe build/cold-ole-uninitialize-generic

.PHONY: msvcrt-ctime-check
build/msvcrt-ctime-probe: tests/msvcrt-ctime-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/msvcrt-ctime-generic: tests/msvcrt-ctime-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/msvcrt-ctime-watcom: tests/msvcrt-ctime-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_WATCOM_100A $< $(SOURCES) -o $@
build/msvcrt-ctime-shim: tests/msvcrt-ctime-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 -DTEST_SHIM $< $(SOURCES) -o $@
msvcrt-ctime-check: build/msvcrt-ctime-probe build/msvcrt-ctime-generic build/msvcrt-ctime-watcom build/msvcrt-ctime-shim
	$(PYTHON) tests/check-msvcrt-ctime.py $^

.PHONY: win32-find-files-check
build/win32-find-files-probe: tests/win32-find-files-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/win32-find-files-generic: tests/win32-find-files-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/win32-find-files-types.o: tests/win32-find-files-types.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -c $< -o $@
win32-find-files-check: build/win32-find-files-probe build/win32-find-files-generic build/win32-find-files-types.o
	$(PYTHON) tests/check-win32-find-files.py build/win32-find-files-probe build/win32-find-files-generic

.PHONY: tempnam510-prefix-check
build/tempnam510-prefix-probe: tests/tempnam510-prefix-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
tempnam510-prefix-check: build/tempnam510-prefix-probe
	$(PYTHON) tests/check-tempnam510-prefix.py $<

.PHONY: cold-mbstrlen-check
build/cold-mbstrlen-probe: tests/cold-mbstrlen-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/cold-mbstrlen-generic: tests/cold-mbstrlen-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
build/cold-mbstrlen-types.o: tests/cold-mbstrlen-types.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) -m32 -std=gnu++98 -O0 -g -nostdinc++ -fno-exceptions -fno-rtti -DRECON_CRT_PROFILE_MSVCRT_510 -c $< -o $@
cold-mbstrlen-check: build/cold-mbstrlen-probe build/cold-mbstrlen-generic build/cold-mbstrlen-types.o
	$(PYTHON) tests/check-cold-mbstrlen.py build/cold-mbstrlen-probe build/cold-mbstrlen-generic

.PHONY: mbstrlen-ascii-check
build/mbstrlen-ascii-probe: tests/mbstrlen-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_CRT_PROFILE_MSVCRT_510 $< $(SOURCES) -o $@
build/mbstrlen-ascii-generic: tests/mbstrlen-ascii-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
mbstrlen-ascii-check: build/mbstrlen-ascii-probe build/mbstrlen-ascii-generic
	$(PYTHON) tests/check-mbstrlen-ascii.py $^

.PHONY: native-system-check
native-system-check:
	$(PYTHON) tests/native-system/check.py

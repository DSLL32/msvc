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
	$(PYTHON) -c "import subprocess; p=subprocess.run(['build/check-cpp-new','failure'],capture_output=True); assert p.returncode==125; p=subprocess.run(['build/check-cpp-new','delete'],capture_output=True); assert p.returncode==125"
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

.PHONY: startup-host-control-check
build/startup-host-control-probe: tests/startup-host-control-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRECON_NATIVE_STARTUP_CONSUME_GLIBC_TUNABLES $< $(SOURCES) -o $@
build/startup-host-control-default: tests/startup-host-control-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SOURCES) -o $@
startup-host-control-check: build/startup-host-control-probe build/startup-host-control-default
	$(PYTHON) tests/check-startup-host-control.py build/startup-host-control-probe build/startup-host-control-default

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

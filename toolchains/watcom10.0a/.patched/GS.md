# Watcom 10.0a WLINK: GS corruption under Wine

Investigated on 2026-10-23 with Debian Wine `10.0~repack-6` (`wine-10.0`),
using a fresh `WINEARCH=win32` prefix under `~/.ai-tmp/`.

## Finding

`toolchains/watcom-10.0a/binnt/WLINK.EXE` changes the x86 `GS` segment register
while reading message resources and leaves it changed. In the tested 32-bit
Wine process, `GS` also supplies glibc's thread-local storage base. Subsequent
Wine calls into libc therefore fault while accessing thread data.

The observed `WriteFile` failure with error 998 (`ERROR_NOACCESS`) is a
consequence of this register corruption, not a bad stdout buffer or an
inherent inability of the linker to write console output.

The executable involved is the complete native Win32 linker. It is **not**
one of Watcom's `binnt` compiler loader stubs. No `binb` compiler or `W32RUN`
loader participates in the failing linker execution path. The separate
`binnt/WCC386.EXE` and `binnt/WPP386.EXE` loaders were subsequently tested with
their original backing compilers; both compiled successfully without patches.

## Failure sequence and evidence

1. At the linker's entry point, `GS=0x63`, the glibc TLS selector in this process.
   `DS=0x2b` is the flat data selector. These selector values are observations
   from this environment, not constants to hardcode in a general fix.
2. The resource-string reader beginning at virtual address `0x004346D0`
   executes `mov gs, word ptr [ebp+0x18]` at `0x004347C1`, loading `0x2b`.
   It stores string bytes through this segment. At `0x00434803`, it also uses
   `lgs edx, fword ptr [ebp+0x14]` before writing the terminating NUL.
   The reader does not save and restore the caller's `GS`.
3. After the reader returns, the first failing banner write reaches
   `KERNEL32.WriteFile` and Wine's Unix-side `NtWriteFile`. A native GDB trace
   catches libc's `_setjmp` faulting on `xor %gs:0x18,%ecx`, with `GS=0x2b`.
   This accesses low unmapped memory instead of the thread's pointer guard.
   Other observed libc faults include `lseek64` reading `GS:0x14`.
4. Wine converts the caught syscall fault into `STATUS_ACCESS_VIOLATION`
   (`0xC0000005`), which the Win32 API maps to `ERROR_NOACCESS` (998).
   Watcom treats the failed write as fatal. Attempts to report the error use
   the same broken I/O path, leaving no stdout diagnostics and exit status 1.

The ordinary Wine trace also shows successful file reads before resource
loading and I/O failures after `GS` changes. This explains why the symptom is
broader than stdout. With tracing enabled, Wine's own native diagnostic code
can fault too. Watcom's `longjmp` eventually restores the original selector
while unwinding the failed link, but that happens after the damage.

The reader's structure matches Open Watcom's WRes `GetString`: 128-byte reads,
resource ID masked with 15, length-prefixed strings, an output pointer, and a
terminating NUL. This is structural identification using related source,
not proof that the modern source is identical to the 1994 implementation.

## Diagnostic patch

The tested patch uses `DS` for the reader's output accesses and avoids changing
`GS`. All observed output pointers in this Win32 path have selector `DS=0x2b`.
It changes four sites, preserving executable size and instruction addresses:

| Virtual address | File offset | Original bytes | Replacement bytes | Change |
|---|---|---|---|---|
| `0x004347C1` | `0x00024BC1` | `8E 6D 18` | `90 90 90` | Remove `mov gs,[ebp+0x18]`. |
| `0x004347E4` | `0x00024BE4` | `65` | `3E` | Change the string-byte store's segment prefix from GS to DS. |
| `0x00434803` | `0x00024C03` | `0F B5 55 14` | `8B 55 14 90` | Replace `lgs edx,[ebp+0x14]` with `mov edx,[ebp+0x14]; nop`. |
| `0x0043480B` | `0x00024C0B` | `65` | `3E` | Change the terminating-NUL store's segment prefix from GS to DS. |

For these addresses, the PE code section starts at VA `0x00410000` and file
offset `0x400`, so `file_offset = VA - 0x00410000 + 0x400`.

The original executable is 209,744 bytes. SHA-256 identities:

```text
Original binnt/WLINK.EXE:
ae8bb02cce0e92ed1a0c5c003a7312bb05f8bf3c252e1efa27a4835fb83b9ff1

Patched WLINK-DS.EXE:
509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b
```

This is a diagnostic patch validated for this exact binary and its flat Win32
resource pointers. It is not a general replacement for arbitrary far-pointer
semantics, nor has it been validated for other Watcom releases or hosts.

The following standalone Python script reproduces the patch into a new file.
Save it as `patch-gs.py` outside the repository and supply input and output paths:

```python
from hashlib import sha256
from pathlib import Path
import sys

source, target = map(Path, sys.argv[1:])
if source.resolve() == target.resolve() or target.exists():
    raise SystemExit("Choose a new output file; the input must remain unchanged")
data = bytearray(source.read_bytes())
expected = "ae8bb02cce0e92ed1a0c5c003a7312bb05f8bf3c252e1efa27a4835fb83b9ff1"
if sha256(data).hexdigest() != expected:
    raise SystemExit("Input is not the investigated Watcom 10.0a Win32 linker")
patches = [
    (0x24BC1, "8e6d18", "909090"),
    (0x24BE4, "65", "3e"),
    (0x24C03, "0fb55514", "8b551490"),
    (0x24C0B, "65", "3e"),
]
for offset, before, after in patches:
    old, new = bytes.fromhex(before), bytes.fromhex(after)
    if data[offset:offset + len(old)] != old or len(old) != len(new):
        raise SystemExit(f"Unexpected bytes at {offset:#x}")
    data[offset:offset + len(old)] = new
with target.open("xb") as output:
    output.write(data)
print(target.resolve())
print(sha256(data).hexdigest())
```

## Validation

- Unmodified linker: a minimal LE link exits 1 with empty stdout. `wlink ?`
  exits 0 but prints only the banner, not a complete working help listing.
- Patched linker: the same minimal LE link exits 0 with normal messages.
  Its captured trace has no `handle_syscall_fault` or `c0000005` diagnostics.
- Full WC2 link: copied the 1,988 inputs named by `decomp/wc2/WAR2.LNK`
  (1,987 objects plus the stub) into scratch storage. Linked the same existing
  object set first with the patched Win32 linker under Wine and then with
  the original `toolchains/watcom-10.0a/bin/WLINK.EXE` under DOSBox.
  Both exited 0, and their resulting executables were byte-for-byte identical:

  ```text
  Size:    757604 bytes
  SHA-256: da46c16654273be13df677c41d82d3eccc291be91cc0ab4d4abbb40928bebf9b
  ```

This was a link comparison using existing objects, not a fresh reconstruction
or compilation of every source. No repository toolchain executable, build
routing, or production output was changed by the investigation.

## Reproduction environment and retained artifacts

The investigation directory is:

```text
/root/.ai-tmp/wlink-investigation.qij_r46v
```

It contains the patched linker, patch script, minimal sources and objects,
copied full-link inputs under `full/`, and these principal captures:

| Artifact | Contents |
|---|---|
| `probe.py` | Wine environment and minimal-link commands. |
| `trace.stderr` | Unpatched execution with `+timestamp,+pid,+seh,+file,+virtual,+relay`. |
| `gdb.log` | Native fault instructions, registers, and call stacks. |
| `lgs.log`, `segments.log` | Selector observations at EXE entry, resource loading, and unwind. |
| `ds-link.stdout`, `ds-link.stderr` | Patched minimal-link output and trace. |
| `wine-patched.stdout`, `dos.stdout` | Full-link diagnostics. |
| `wine-patched.exe`, `dos.exe` | Identical full WC2 link results. |

Wine was invoked directly, with `WINEPREFIX` pointing to `prefix/` in that
directory, `WINEDEBUG=-all` except for trace captures, and
`WINEDLLOVERRIDES=winemenubuilder.exe=d;mscoree=d;mshtml=d`.
`WATCOM` was the Windows-form path
`Z:\root\scbeta-decomp\tmp6\toolchains\watcom-10.0a`.
For example, from the scratch `full/` directory:

```sh
WINEPREFIX=/root/.ai-tmp/wlink-investigation.qij_r46v/prefix \
WINEDEBUG=-all \
WINEDLLOVERRIDES='winemenubuilder.exe=d;mscoree=d;mshtml=d' \
WATCOM='Z:\root\scbeta-decomp\tmp6\toolchains\watcom-10.0a' \
timeout -k 5s 45s wine ../WLINK-DS.EXE @WAR2.LNK
```

Use a separate scratch output directory when repeating the DOS comparison,
or save the Wine result before the DOS linker overwrites `bin/WAR2.EXE`.
The DOS command is the repository's `decomp/wc2/tools/wlink-dosbox.sh`, with
`-C` pointing to the scratch input directory and `@WAR2.LNK` as its argument.

No `wineserver -p` was used. The investigation also avoided `sctool watcom`,
whose `prepare_wine()` requests a persistent server. The test prefix's server
was explicitly stopped after the linker investigation. The later compiler
probes exited naturally; a prefix-specific `wineserver -w` confirmed shutdown.
When repeating tests, stop and wait for only the test prefix's server.

For native GDB, disabling `libthread_db` loading was necessary because the
debugger's thread support otherwise failed after the program changed `GS`:

```text
set auto-load libthread-db off
set libthread-db-search-path /nonexistent
```

## Wine workarounds and scope

Wine has an older targeted recovery for invalid `GS` values, shipped in 1.5.24.
In Wine 10, `check_invalid_gs()` is reached for certain segment/general
protection faults. WLINK's flat selector instead causes page faults on low
addresses, which are not repaired by that branch.

Developers have also discussed restoring `GS` at PE/Unix boundaries, repairing
and retrying faulting instructions, and application-specific glibc layout
patches. The latter concern Windows thread-data offsets overlapping glibc
data and do not directly solve WLINK replacing the selector.

Wine's new WoW64 architecture, with 32-bit PE code calling 64-bit Unix
libraries, is a plausible alternative worth testing. It was **not tested**
in this investigation; an ordinary 64-bit prefix does not establish that the
Wine build uses the new architecture. Only the diagnostic patch and DOSBox
were validated with the full WC2 link inputs.

## References

- [Wine 10.0 x86 signal handling and syscall dispatch](https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/ntdll/unix/signal_i386.c).
- [Original invalid-GS recovery commit](https://github.com/wine-mirror/wine/commit/64ddb263d25dfceeeae8457a0131f1a2af292383)
  and [Wine bug 28795 resolution](https://www.winehq.org/pipermail/wine-bugs/2021-July/558777.html).
- [2013 proposal to repair GS in the fault handler](https://www.winehq.org/pipermail/wine-patches/2013-February/122118.html).
- [2019 developer discussion of GS/glibc conflicts and possible solutions](https://www.winehq.org/pipermail/wine-devel/2019-May/146014.html).
- [Related Open Watcom WRes string-reader source](https://github.com/open-watcom/open-watcom-v2/blob/master/bld/wres/c/loadstr.c).
- [Wine 9.0 explanation of new WoW64](https://list.winehq.org/hyperkitty/list/wine-announce%40list.winehq.org/message/TB7WYKWZF4DWNQLWN3XZM2NLEAF6MKST/).
- Existing repository context: [WC2 toolchain notes](decomp/wc2/TOOLCHAIN.md)
  and [Watcom provenance](toolchains/README.md). Their older stdout-only
  diagnosis should be read together with the root cause established here.

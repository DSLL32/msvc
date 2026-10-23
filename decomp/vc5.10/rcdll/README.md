# RCDLL groundwork

Target: `toolchains/5.10/bin/RCDLL.DLL`. The supplied toolchain has no
`RCDLL.EXE`; the requested binary folder remains `rcdll`. Reconstruction is
paused pending the DLL workflow decision and the Rust graph audit. No function
has been reconstructed, accepted, unmasked, or counted toward coverage.

`make inspect` prints a read-only PE inventory. `make check-plan` verifies that
inventory and copied input provenance. Native, original-build and fixture targets
fail with status 125 until explicitly released; neither command invokes Wine.

## Image and external interfaces

PE32 i386 DLL, preferred base `0x71c20000`, entry RVA `0x12d20`, image size
`0x30000`; no TLS directory or delay imports. Section and complete named import
inventory: [cases/pe-inventory.json](cases/pe-inventory.json). Imports are from
`MSVCRT.dll`, `KERNEL32.dll` and `USER32.dll`; there is no separate RCPP dependency.
`.text` also holds IAT/import/export/debug data, so masking must distinguish
instructions and encountered text data from PE loader metadata.

| Export | Ordinal | RVA | Calling convention |
| --- | ---: | ---: | --- |
| Handler | 1 | `0x8a46` | stdcall, one DWORD argument, `ret 4` |
| RC | 2 | `0x8b46` | stdcall, six 32-bit slots, `ret 24` |

These signatures follow actual export-entry instructions and caller stack setup,
not export-name decoration. The original RC.EXE statically imports both names.
Its main registers `Handler` with `SetConsoleCtrlHandler`, invokes
`RC(0, 0, diagnostic_callback, 0, argc, argv)`, unregisters the handler and exits
with RC's result. The RC worker owns original driver probes and callback analysis.

At RC's entry, argument 1 is stored as a window handle at `0x71c45f60`, argument 3
as the diagnostic callback at `0x71c45f5c`, argument 4 as a resource callback at
`0x71c45f58`; argument 2 is unused by this wrapper. Arguments 5/6 pass to internal
compiler routine VA `0x71c27443` with caller cleanup. The compiler mutates argv
pointers while consuming switches: the harness must supply writable argument
storage. RC returns the compiler's result unchanged.

Diagnostic calls at VA `0x71c228fe`, `0x71c28b8e` and `0x71c28bcd` push
`message, 0, 0` without caller cleanup: callback is stdcall with three slots,
`callback(0, 0, char *message)`. The RC.EXE callback returns with `ret 12`, mutates
trailing paired CR/LF characters and escapes percent signs before printing.
The DLL can instead send messages `0x419` and `0x420` to argument 1 using
`SendMessageA`; its return can trigger cancellation. Argument 4 is another
stdcall three-slot callback (VA `0x71c2ba1b`/`0x71c2baba`) with resource records
whose pointed layouts still need recovery. It is NULL in the CLI driver and
must not be assigned an invented typed record ABI yet.

## DLL lifecycle and testing design

The PE entry at VA `0x71c32d20` wraps CRT initialization at `0x71c32bb0` and the
application DllMain at `0x71c28b29`. Both consume three stdcall slots. On process
attach DllMain stores HINSTANCE at `0x71c45f64` and calls
`DisableThreadLibraryCalls`; it returns TRUE for all reasons. No TLS directory is
present. The application stores compiler, callback, FILE and heap state globally;
this is not evidence of reentrancy or safe simultaneous RC invocations.

For acceptance, a Windows host must load the *masked* DLL under a debugger before
LoadLibrary triggers its entry point. The first expected trap is inside DLL
startup, before export lookup/call. Record actual load base and normalize logical
locations as module SHA256 plus RVA. A host calling an unmasked original DLL and
then comparing only the exported call would omit initialization from the graph.
No masking or acceptance host exists here yet.

The corresponding native i386 host can statically link reconstructed DLL C plus
the shared shims, explicitly call the preserved entry/attach sequence, invoke RC
through its original stdcall signature, then detach in the captured order. This
avoids requiring ELF dynamic-loader support solely to exercise a DLL interface.
The host is fixture infrastructure, not an invented application function. Keep
module identity and original callback edges in traces/audits; filter only the
host/shared runtime. Start each case in a fresh process. Add explicit repeated
invoke/load/unload cases later rather than assume globals reset like a fresh EXE.
A future DSO target needs shared tracing/module support first.

Console Ctrl+C invokes Handler on the Windows control thread. This requires a
separate synchronized case and is not part of ordinary help/resource fixtures;
a native signal handler cannot silently substitute different threading or
lifetime semantics. Handler's argument test distinguishes Ctrl+C (0) from other
console events, so preserve that behavior when reconstructed.

The DLL uses its saved HINSTANCE to read its own message table: VA `0x71c22c64`
calls `FormatMessageA(0xa00, HINSTANCE, message_id, 0, buffer, 0x1000, NULL)`.
Flags are FROM_HMODULE | IGNORE_INSERTS. The PE has message-table type 11 and
version type 16, language `0x409`. Shared shims currently lack target-module
resource registration/FROM_HMODULE lookup; the maintainer has been notified.
MSVCRT imports include 16-bit wide string/conversion operations, `_setjmp3` and
`longjmp`; existing shim coverage must be evaluated against actual reached calls,
not replaced with host wchar_t/FILE assumptions. No speculative shim changes
were requested.

## Seeded cases

[cases/plan.json](cases/plan.json) records ordered proposals and SHA256 source
provenance. `resources-rich/source` is the complete VC1 input corpus: menus,
dialogs, strings, icon/cursor/bitmap, accelerators, raw/custom/message resources,
version records, languages, includes, macros and conditions. It needs original
5.10 captures with 5.10 headers before becoming an oracle. Binary `.ico`, `.cur`,
`.bmp`, `.bin` files here are *input assets*, not target data or captured output.
`preprocessor/source` is token/directive input for later adaptation; it is not
currently a valid standalone resource script. No old outputs, traces, manifests,
masked images or acceptance histories were copied.

Use the shared prefix and fixed R:/S: mapping. Scratch namespace:
`~/.ai-tmp/msvc2/vc5.10/rcdll`. Original probes/captures must be explicit and
separate from native regressions. Future fixtures compare cached original
streams/status/resources and traces without invoking Wine. Record split build,
setup, execution and checking times. Coordinate resource-format cases with the
RC and CVTRES workers; contact `/root/recon_lib_maintenance` for shared changes.

Original-driver groundwork receipts are owned by RC:
`../rc/cases/help/probe` records `/help` exit 0, 589 stdout bytes, empty stderr
(5.582 seconds). `../rc/cases/minimal/probe` records a 30.03-second timeout,
empty streams and no output resource. Other binaries also stalled before startup;
shared Wine health is under investigation, so this does **not** establish an
RCDLL failure. No valid 5.10 resource oracle was produced. All further Wine
launches are on hold; this worker launched no Wine process of its own.

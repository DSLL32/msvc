# C1XX.DLL groundwork

This folder is a preparation scaffold. No target functions have been reconstructed,
accepted or unmasked. Builds and fixtures deliberately fail until the DLL workflow,
Rust graph auditor and reconstruction release are ready.

Target: `toolchains/5.10/bin/C1XX.DLL`, SHA-256
`e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9`.
`make inspect` prints read-only PE metadata; `make verify-groundwork` checks it
against `cases/pe-inventory.json`. The script has no Wine dependency and does not
discover or approve function graphs.

## PE and export contract

The target is an i386 PE32 DLL, preferred base `0x10400000`, image size
`0x117000`, entry RVA `0x2e3c6`. Its internal export module name is `c1n32_l.dll`.
It has relocations and no PE TLS directory. Do not assume the preferred base is
the actual loaded base; original traps and pointers need module-relative identity.

| Ordinal | Exact exported name | RVA | Observed ABI |
| --- | --- | --- | --- |
| 1 | `_AbortCompilerPass@4` | `0x903ad` | stdcall, one DWORD slot, `ret 4` |
| 2 | `_InvokeCompilerPass@12` | `0x384d4` | stdcall, three DWORD slots, `ret 12` |

CL analysis by the sibling worker identifies the invocation sequence at CL
`0x4056c6`: push `((pass.flags >> 3) & 1)`, `argv`, `argc`, then call the resolved
export. Thus the provisional signature is `int WINAPI InvokeCompilerPass(int,
char **, int)`; the third slot is a Boolean mode, not a callback table. C1XX stores
that slot at preferred VA `0x104cb008` and passes only argc/argv to its internal
cdecl driver at `0x10438bfd`. The return value is preserved across cleanup.
The mode affects internal object-method choices; its full semantic name remains
unproven. No host callback pointer is established by this export ABI.

These are static observations from the original, not captured runtime evidence:

* PE entry `0x1042e3c6` dispatches CRT initialization at `0x1042e46a` and the
  application DLL entry at `0x1042e52a`. The application entry stores its module
  handle at `0x104cb028`, calls `DisableThreadLibraryCalls` for process attach and
  returns true. The CRT path uses imported MSVCRT data and `_initterm`.
* Invoke first calls initialization at `0x10438550`. Its first-invocation branch
  preserves initialized frontend state; the later branch restores state, clears
  frontend BSS and runs the `.initfe` pointer range `0x104df000..0x104df0ac`.
  Repeated invocation cannot be treated as unrelated command executions.
* Invoke installs an x86 SEH frame and calls cleanup at `0x10434673`, which invokes
  registered internal callbacks in reverse order. Those callbacks are target
  function-graph edges, not replayable external child processes.
* Abort has no observed use of its stack argument. It manages a semaphore and
  cancellation state, waits for completion, and has an `ExitProcess(0x8000)`
  timeout path. It is not merely a Boolean flag assignment. Cancellation and
  repeated invocation require separate later cases before their behavior is claimed.

The section table and every import name/ordinal are recorded in the inventory.
The executable `.text` has virtual size `0xb7670`; frontend state is split among
`.bssfe`, `.datafe`, `.rdatafe` and `.initfe`. `.rsrc` holds diagnostic strings.
None of these sections is copied into reconstruction source.

The saved HMODULE is observable through resources. At `0x10491860`, a diagnostic
cache initializer walks ID/pointer entries at `0x104cd898`, calls
`LoadStringA(saved_module, id, stack_buffer, 256)` and duplicates each result.
The wrapper at `0x104918ba` initializes that cache lazily. A separate stdcall
helper at `0x10491987` calls `LoadStringA(saved_module, id, 0x104c5390, 256)`.
These are the observed direct readers of the saved module variable. No
`FindResource` or `GetModuleFileName` import exists. A native harness module must
therefore identify the target DLL's resources; its own executable handle is not
an equivalent substitute. Capture encountered resource IDs/bytes and preserve
missing-ID, capacity and language behavior when this boundary is reached.

## Dependencies and shared support

Direct imports are MSVCRT.dll, KERNEL32.dll, USER32.dll, mspdb50.dll, ole32.dll and
OLEAUT32.dll. OLEAUT imports are by ordinals 6, 8, 9, 10, 12, 161 and 164.
MSPDB dependencies include C++ PDB/NameMap/ILStore interfaces and minimal-rebuild
helpers. Imports alone do not prove any feature is reached by the first case.

Unlike vc1.0's frontends, this binary imports its CRT dynamically. `_iob`,
`_mbctype` and `_adjust_fdiv` are data imports; `_setjmp3`, `_except_handler3`,
longjmp, `__unDName` and wide-character functions need an encountered-contract
check. The existing 32-byte shared FILE model and limited default-C `mbtowc`
support must not be assumed equivalent to this MSVCRT merely by symbol name.
The shared native SEH helpers currently execute ordinary try bodies and do not
dispatch arbitrary exception handlers. The recon maintainer has been informed;
no speculative API additions have been requested.

## Proposed DLL capture and native harness

Use a small original Win32 host which loads the exact DLL, resolves the exact
decorated names, invokes the export with CL's actual pass argv/mode, records its
return and unloads it. First capture the untouched target's ordered attach,
invoke, internal cleanup and detach behavior. CL-side pass argv must be captured
for this toolchain; command-line CL flags below are proposals, not DLL argv.

For fresh masked comparisons, keep the DLL's entry code masked as well as its
exported/internal code. Loading may trap before `LoadLibrary` returns; the
debugger must identify the loaded module and compare that lifecycle trap with
the native harness. Do not silently run an unmasked attach path or skip its
target functions. Account for relocations and import thunks separately from
accepted target code. Loader/CRT boundaries require the orchestrator's agreed
policy and exact evidence; this scaffold grants no exemption.

The native i386 harness can link reconstructed sources and shared shim sources
directly, call the preserved lifecycle/export functions with stdcall ABIs and
emit the shared trace. It need not load the original PE or use Wine. Keep host
startup outside the target graph; use typed module/pointer identities when
comparing state. Preserve the reconstructed Win32 DLL build and exact export
names/ordinals. Do not turn the reconstruction into a CLI-only replacement.

Before regression fixtures exist, explicitly freeze original pass inputs,
environment, return values, raw streams, frontend IR/artifacts, entry trace and
dependency requests. Native regressions consume those cached references only.
PDB/COM state is in-process mutable state: replaying one final file is not a
substitute for the reached method contracts. Start without `/Zi` or minimal
rebuild to reduce the initial dependency surface, then add those cases as needed.

Original probes/builds use the shared prefix
`~/.ai-tmp/vc1.0-capture/wine`; `R:` is the repo and
`S:\vc5.10\c1xx` is this worker's scratch namespace. Prefer relative paths,
never remap drives, create a private prefix or stop the shared server.

## Seed cases and release gates

`cases/plan.json` starts with minimal C++, followed by the existing vc1.0 C++
themes: basic construction/operators, virtual diamond, copy/assignment, stack
lifetime and new/delete. Only source was copied. No old IR, traces, addresses,
expected objects or acceptance records were carried across.

Sources are not yet validated with this toolchain. The next authorized work is
to agree on the host/lifecycle workflow and check exact original pass capture,
then wait for the Rust audit tool and explicit reconstruction release. There is
no current native fixture and no claimed coverage percentage.

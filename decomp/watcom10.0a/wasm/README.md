# Watcom 10.0a WASM

Six cases are complete: `/?` (exit 1), four successful assembly workloads,
and a source-configuration diagnostic workload (exit 7).
The buffer transform combines includes, macros, conditionals, typed records,
integer/x87 instructions, branches and external relocations (485-byte OMF).
The DOS record formatter adds small-model segments, IRP, macro-local labels,
stack arguments/locals, COMMON and debug lines (561-byte OMF, 54 lines without
diagnostics). The flat32 vector library adds CPU/model and name options,
include-path lookup, aliases, string conditionals, register arguments, quoted
identifiers, TBYTE data and embedded-code data records (362-byte OMF). Each
object matches exactly. The segmented286 x87 library adds emulation patch
fixups, explicit segment frames and C naming (359-byte OMF), also matching
exactly. The diagnostic workload exercises included macro error directives,
string/value/defined-symbol conditions, macro-call notes and alignment warnings.
Its seven errors and one warning match exactly; redirected stdout suppresses
`/fe` error-file creation, and the erroneous object is deleted.
Inputs and references are frozen in numbered captures;
fixtures never invoke Wine.

The reconstruction covers 390 application functions and 93,357 unique original
code bytes. One empty function, `PObjUnRegList`, is statically qualified at the
RET shared with `PObjRegList`: zero additional bytes and explicitly excluded
from the assembly cases' entry orders, without a runtime observation claim.
Original CRT bodies execute unchanged and receive no application credit.
Unsupported instruction fields and pending functions still trap. The original
empty-line read-before-buffer defect remains in the period build; the native
branch uses the documented, twice-observed no-join outcome. Native `GetAssume`
projects the original selector 8 read onto its actual `ModuleInfo.flat_idx`
owner rather than depending on host global placement; other unknown selectors
remain unsupported. The original WOMP warning callers have an incompatible
`PrtMsg` declaration: period builds preserve their actual calls, while native
builds explicitly trap those unreached malformed-call paths. The proper
assembler diagnostic interface retains its two independent `va_list` arguments.

Use `make -j4 native cases win32`, `make -j4 check`, and `make -j4 audit`.
All 390 graph/reference/ownership/extent checks pass, with 308 ordinary RET
contracts and 82 finite cold-error, tail-wrapper or exit checks. Pending error
handlers stay masked. Reports and raw artifacts remain in scratch.

Final cached native fixtures: help 16.50 ms, buffer transform 16.64 ms,
DOS records 15.65 ms, flat32 library 15.51 ms, segmented x87 library 15.79 ms
and diagnostics 13.72 ms (setup 8.20 ms, execution 2.32 ms, checks 3.20 ms).
Parallel Make check took 98.98 ms; the final full graph and finite RET audit
took 767.59 ms. The last explicit original capture took 1.82 s. Native and
period builds passed; their final incremental build time was not measured.
These are measured closeout runs, not clean-build timings.

The captured original is `.patched/binb/WASM.EXE` through the byte-identical
`.patched/binnt/WASM.EXE` loader, hash-bound by `harness/cf-profile.json`. Its
four fixed-size GS-preservation substitutions are at CF offsets `1e611`
(`8e6d18`→`909090`), `1e634` (`65`→`3e`), `1e653`
(`0fb55514`→`8b551490`), and `1e65b` (`65`→`3e`). Resource bytes and relocation
sites are unchanged. This help path supplies the caller-owned 128-byte buffer,
capacity 128 and DS far selector; no general segmented-pointer support is claimed.
Historical patch creation/qualification remains in Git commits `8b898631` and
`25bcbf9e`; historical captures are not the current oracle.

Released-source adaptations retain copyright/license headers and their source
paths from Open Watcom commit `6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`
(read-only reference: `~/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`). Shipped
1994 bytes govern differences from that source, including scanner behavior,
token IDs, resource layouts and the two distinct private hash bodies. Watcom
builds retain the original register/far-pointer ABIs; native builds use the
explicitly authorized GCC ABI and shared `lib/recon` support.

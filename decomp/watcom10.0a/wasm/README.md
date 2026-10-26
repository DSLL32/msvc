# Watcom 10.0a WASM

Four cases are complete: `/?` (exit 1) and three successful assembly workloads.
The buffer transform combines includes, macros, conditionals, typed records,
integer/x87 instructions, branches and external relocations (485-byte OMF).
The DOS record formatter adds small-model segments, IRP, macro-local labels,
stack arguments/locals, COMMON and debug lines (561-byte OMF, 54 lines without
diagnostics). The flat32 vector library adds CPU/model and name options,
include-path lookup, aliases, string conditionals, register arguments, quoted
identifiers, TBYTE data and embedded-code data records (362-byte OMF). Each
object matches exactly. Inputs and references are frozen in numbered captures;
fixtures never invoke Wine.

The reconstruction covers 375 application functions and 90,487 unique original
code bytes. One empty function, `PObjUnRegList`, is statically qualified at the
RET shared with `PObjRegList`: zero additional bytes and explicitly excluded
from the assembly cases' entry orders, without a runtime observation claim.
Original CRT bodies execute unchanged and receive no application credit.
Unsupported instruction fields and pending functions still trap. The original
empty-line read-before-buffer defect remains in the period build; the native
branch uses the documented, twice-observed no-join outcome. Native `GetAssume`
projects the original selector 8 read onto its actual `ModuleInfo.flat_idx`
owner rather than depending on host global placement; other unknown selectors
remain unsupported.

Use `make -j4 native cases win32`, `make -j4 check`, and `make -j4 audit`.
All 375 graph/reference/ownership/extent checks pass, with 296 ordinary RET
contracts and 79 finite cold-error, tail-wrapper or exit checks. Pending error
handlers stay masked. Reports and raw artifacts remain in scratch.

Final cached native fixtures: help 13.54 ms, buffer transform 15.65 ms,
DOS records 15.39 ms and flat32 library 15.40 ms (setup 7.27 ms, execution
5.16 ms, checks 2.97 ms). Parallel Make check took 70.72 ms; final incremental
native/case/period build setup took 4.65 s. Full graph and finite RET audits pass.
The final explicit flat32 original capture took 1.78 s. These are measured
closeout runs, not clean-build timings.

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

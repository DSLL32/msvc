# Watcom 10.0a WASM

Three cases are complete: `/?` (exit 1), `/q /fo=transform.obj transform.asm`
(exit 0), and `/d1 /fo=records.obj records.asm` (exit 0). The first assembly
workload combines includes, macros, conditionals, typed records, integer/x87
instructions, branches and external relocations; its 485-byte OMF matches exactly.
The DOS workload adds small-model segments, IRP, macro-local labels, stack
arguments/locals, COMMON and debug lines. Its 561-byte OMF matches exactly, with
54 lines, zero warnings and zero errors. Inputs and references are frozen in the
numbered captures; fixtures never invoke Wine.

The reconstruction covers 359 application functions and 87,477 unique original
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
All 359 graph/reference/ownership/extent checks pass, with 282 ordinary RET
contracts and 77 finite cold-error, tail-wrapper or exit checks. Pending error
handlers stay masked. Reports and raw artifacts remain in scratch.

Final native fixtures: help 13.34 ms, buffer transform 15.45 ms, DOS records 13.46 ms
(DOS setup 7.26 ms, execution 3.07 ms, checks 3.13 ms). Parallel Make check took 68.42 ms;
incremental native/case/period build setup took 138.73 ms; the full audit took
709.63 ms. The final explicit original capture took 1.29 s. These are measured
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

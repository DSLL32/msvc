# First-case acceptance protocol

Only `dll-lifecycle-and-exports` is released. Initial scaffold review passed;
the CRT entry has its first trial body, while CRT_INIT, DllMain and both exports
still trap. The orchestrator approved the first real CRT-entry candidate; later functions
continue under this same protocol without a per-function review pause.
The rebuilt Win32 DLL uses an explicit entrypoint compiled from the same CRT C
as the native harness. Its graph audit therefore checks that handwritten body,
not a shipped linker startup against itself.

The initial fresh pair is checked by `scripts/check-pair.py`: actual original
DLL-load hFile bytes must equal the canonical C1 and shipped MSVCRT; the relocated
snapshot is taken only at the verified guarded CRT entry. Raw original trap PC
and three arguments must match the named native SIGTRAP and GDB arguments. Native
PC must belong to the actual ELF symbol and follow an INT3 instruction. The
validator checks all raw artifact hashes and binds both captures to current
candidate/config/native/rebuilt bytes. This proves an initial matching frontier,
not a completed function, CRT body, graph or lifecycle case.

For each trial, the candidate's exact original ranges and metadata must be the
same immutable configuration used for the original mask and Rust graph audit.
Include the trial body itself in the graph/ABI audit, not just its previously
accepted prefix. PGO-split CRT code needs explicit code-chunk metadata/tool support
or an exact archive+relocation-call/global/text-reference identity proof; do not
unmask broad envelopes or treat target CRT code as a synthetic harness shortcut.
Keep unresolved callees as named traps. Define contextual globals and compare
their meaningful typed states, heap objects, callback slots and retained module
identity as they become live.

Require fresh masked-original and native runs after the trial, with either an
explicit nonempty matching next frontier and state, or independently derived real
completion on both sides. An exit/status receipt alone or two absent frontiers
are insufficient. Reject stale executables/configs, wrong trials, and altered or
missing raw trap evidence even when receipt hashes have been updated consistently.
Run the original compiler and Rust graph/RET/ownership/extent checks on each trial.
Record exact accepted function/chunk ownership, source identity and evidence before
promoting it. Current `functions.json` records the accepted CRT-entry support function. The
separate immutable `trial.json` included that real candidate and both original
code chunks; its Rust audit checked one logical function owning exactly 224 bytes.

Auditor versions are separate from execution identity. Preserve old raw execution
and audit records; re-audit unchanged execution inputs with an updated Rust tool
and write a fresh validation receipt referencing those raw records. An auditor
update alone does not require Wine or a new native execution.

The shared registered-module contract now replaces the provisional pre-trap
token. Raw original HMODULE and native token map by verified target identity;
they are not required to have the same address. Reason/reserved, current CRT
attach count, null raw callback and Win32 last-error state compare exactly.
The native harness calls faithful CRT attach/detach support; its export catalog
resolves both stub addresses without invoking them. The actual Win32 DLL artifact
must preserve the original export names, ordinals, signatures, callee cleanup and
internal export module name. Nonpreferred-base behavior is untested.

After this one case completes, freeze native-only status/streams/state/application
trace regressions and pause. Do not invoke either compiler-pass export or start
the queued minimal compilation.

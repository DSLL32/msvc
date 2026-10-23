# First startup wrapper trial

Historical contract for acceptance-0001; later accepted callees are listed in README.md.

`recon_dll_entry`, original RVA 12d20..12ddf, is embedded CRT support, not counted
as an application function. The complete original body and all return paths were
read and reconstructed; no range extends into the INT3 alignment following it.

The body reads attach count at RVA15800 and optional raw-DllMain callback pointer
at RVA29468. Both are initially zero. On process attach with NULL callback it
forwards the three arguments to CRT initialization at RVA12bb0, which remains an
ABI-correct named trap. No writes, allocations, output, or external API calls
occur before that frontier. GDB and original raw data compare both encountered
globals, all scalar arguments, the caller edge, empty streams and trap identity.

The reserved third argument is an opaque process-startup context. The original
full wrapper only loads it from its incoming stack slot and pushes it unchanged
to the optional callback, CRT initialization and DllMain. It never dereferences,
arithmetically transforms, or branches on the pointer. The raw callback remains
NULL in this trial, explicitly checked on both sides. Native storage preserves
nonNULL context identity across forwarding, with both original/native raw words
retained in the proof. This is not normalization of an arbitrary pointer or an
invented NULL. CRT initialization and application DllMain bodies are separate
functions; their own eventual contracts must be established when reconstructed.

The captured original host is actual static-import RC.EXE. A LoadLibrary host
with NULL reserved context is not substituted. Target/module is at its preferred
base; nonpreferred-base validation remains pending.

Common Rust graph comparison includes this trial along with the accepted prefix,
using the same exact functions.json that generates the masked ranges. It checks
the direct CRT-init and DllMain edges, conditional indirect callback, RET12,
function references, ownership and full extent. Initial scaffold checks alone do
not permit promotion; the orchestrator must review this typed contract first.

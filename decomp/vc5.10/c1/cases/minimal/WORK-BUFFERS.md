# Work-buffer initializer

The genuine initializer is12830..12873 (67 original bytes). It calls the
separate cdecl allocator12873 twice with reserve0x500000/commit0x4000, stores
both base pointers and computes each initial limit as base+0x3ffb. Four original
BSS DWORDs51850/5001c/51854/50024 become two contextual CompilerWorkBuffer objects;
no other original data is copied. Pointer consumers15342 and15373 establish
work-buffer roles without claiming a larger lexer or allocation layout.

The first allocator remains a named INT3 stub with both real formals. The
candidate's fresh normal and restore runs stop there. A separate native GDB
observation at actual preprologue initialize_work_buffers entry matches the
original12830 entry trap. This anchors main's phase write independently; actual
work entry to allocator then proves no semantic state stores before the call.
Both CALL instructions/returns, raw original stack words, native formal inventory
and all currently modeled state are checked. Entire original data/BSS/snapshot/
globals remain unchanged across that first call boundary.

All16 new pointer bytes have canonical zero defaults and are included in the
modeled BSS reset. The explicitly labeled restore auxiliary seeds owned snapshot
interior pointers4200/4300/4400/4500, verifies those exact identities before reset,
and proves they are cleared before any semantic consumer. Native BSS is804 bytes:
777 known bytes plus27 compiler padding bytes. The52 known initialized bytes and
6496 unmodeled initialized bytes are unchanged; unknown value-record tails remain
unmodeled with a hard frontier before semantic interpretation.

The complete67-byte body, both original5.10/native builds and full strict Rust
call graph/import/RET/ownership/extent checks pass. The allocator body and actual
VirtualAlloc calls remain unaccepted at this checkpoint. Existing shared support
will be assessed against actual reached contracts at that boundary; no alignment,
commit/decommit or failure semantics are assumed from an opaque pointer.

Frozen normal/restore proof, fresh boundary/reset/ABI controls and reused generic
control identities are recorded in evidence/work-allocator-*/acceptance-proof.json.
The cached lifecycle fixture remains native-only; minimal compilation is not yet
a passing regression fixture.

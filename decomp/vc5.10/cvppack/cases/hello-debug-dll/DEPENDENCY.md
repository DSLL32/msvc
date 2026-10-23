# Encountered MSPDB boundary

`original-probe-0003/` repeats the first successful original packing with the
same input and output hashes. It observes all 21 static references to the eleven
MSPDB imports, plus caller-side virtual-call entries/returns. Actual loaded
MSPDB50 file bytes are hashed from its LOAD_DLL hFile, independently of import
names. Only WidenTi::fCreate is imported on this execution; no PDB database API is
called. `widen-contract.json` is decoded from the raw records by
`scripts/widen-contract.py`.

There are exactly 21 sequential create/query/emit/destroy lifetimes. Factory
arguments are `(WidenTi **out, 256, 0)` and result1. Each object is queried once
through vtable offset0x20, emitted once through0x24 and destroyed through0.
Offset0x0c (type-record widening) is not reached. Query/emit take ECX=this and
four stack arguments with16-byte callee cleanup. The descriptor is four DWORDs:
output bytes, fixup pair count, output pointer, fixup pointer. Query starts with
all four fields zero and returns required capacities with null pointers. The
caller allocates the two buffers, fills their pointers and invokes emit with the
same complete initialized input bytes and flags0. Emit writes the captured output
and fixups, updates actual counts and preserves both pointers. Query capacities,
actual counts, full initialized input before/after emit, complete output/fixup
capacities before/after emit, written buffers and results are captured
for every instance, including duplicates. Every raw LastError observation is
retained; errno is not read by CVPACK on this successful interface path.

CVPACK's factory wrapper is410290..4102a6. Its output remains an opaque pointer
in main symbol-conversion caller405250..4053ba: it passes the pointer to wrappers,
then destroys it. The wrappers4102b0/4102e0/410300 only read object[0] to obtain
the vptr; no target read of dependency-private fields occurs. The separate
4102c0 wrapper for offset0x0c is unencountered. Provider allocation, private object
fields and original vtable function addresses are not part of native response
storage. Original objects reuse the same address only after destruction; no
simultaneous live object or ownership transfer beyond that lifecycle occurs.

The proposed native host provider will bind the entire frozen DLL input, enforce
all21 exact lifetimes and their query/emit byte inputs, capacities, flags, typed
vtable slot and argument aliases. It will replay only captured initialized writes,
preserve all untouched buffer bytes and pointer fields, and reject unexpected
creation, slot, sequence, state, flags, input or premature exit. Vtables contain
native typed callbacks. This is a case dependency response, outside the target
function graph and application coverage; it is not reconstructed MSPDB logic.

Factory output and query/emit output allocations are write-only at this boundary:
query's descriptor is fully initialized, emit's descriptor capacities/pointers are
fully initialized, and emit output buffers have no initialized input contract.
The provider must compare the initialized input/descriptor fields, overwrite only
captured written extents, and leave capacity tails untouched. The native target
must continue to allocate and consume those buffers itself.

The third probe adds precisely the review-requested observations. The decoder
now derives `input_unchanged` from actual post-emit bytes and asserts each
instance's own output/fixup capacity tail equals its own pre-emit tail. All21
instances pass. No omitted tail or dead-input assumption is used. Captured full
buffers remain in the contract for independent checking; native replay writes
only captured written prefixes and preserves its own capacity tails.

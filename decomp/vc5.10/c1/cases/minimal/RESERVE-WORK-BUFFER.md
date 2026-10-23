# Reserve and commit work buffers

reserve_work_buffer owns62 hot bytes12873..128b1 and30 cold bytes3043b..30459.
It reserves the requested address space with VirtualAlloc, commits the requested
prefix, calls the existing named fatal stub on either failure, and returns the
reservation. Source uses those genuine Win32 calls; no allocation bytes or API
semantics are implemented locally. The fatal helper remains masked and receives
no reconstruction credit.

The original caches VirtualAlloc in EDI and makes two indirect calls. Common
Rust must-value import resolution uses only the two finite EDI-preservation
contracts in audit-metadata.json. The witness binds original instructions,
actual SDK WINAPI declaration and actual KERNEL32/KernelBase provider bytes;
fresh captures additionally verify loaded hFile hashes, export RVA, resolved IAT
and EDI after every successful call. Every cached site must resolve to the exact
qualified import; missing/extra/unresolved sites reject. The ordinary cdecl fatal
wrapper's unused normal-return contract is finite ABI support, with no recursive
CRT reconstruction or body unmasking.

Both fresh normal and labeled restore runs reach scan_internal_switches128b1.
They independently record actual main/work/allocator entries and returns. All
four VirtualAlloc calls have complete raw stack arguments, return/stack cleanup,
register preservation, LastError0 and x87 control027f checked. The original's
VirtualQueryEx records prove each5MiB PRIVATE reservation, then a16KiB READWRITE
committed prefix and4fc000 reserved remainder. Native /proc mappings verify the
same scoped protections. Both complete16KiB committed blocks are independently
retained and compare equal to zero bytes. The rest is opaque reserved address
space: no defined contents or64KiB alignment equivalence is claimed.

The initializer's four pointer fields retain exact owned allocation identities:
each limit is its own base+3ffb, allocations are distinct and disjoint. Main then
registers the real cleanup pointer and stores its actual argc/argv. Full original
BSS changes are limited to these encountered fields; original initialized data,
snapshot and plain globals remain unchanged. All currently modeled native state
has the same result. Initializer restore proofs remain anchored at real main
entry, before these legitimate later writes.

Fresh53 normal/53 restore allocation controls and generic47/100 controls pass.
The original5.10 incremental build passes (time not measured); the recorded
native build takes1.560s. Original/native captures take1.870/0.753s normal and
1.317/1.503s restore, Rust audit0.027s, cached native lifecycle4.56ms.
The helper's failure path, later low-bit address use and VirtualFree DECOMMIT
semantics have not been exercised. Shared maintainer confirms no shim change
is needed for the currently reached reserve/commit/base+3ffb scope.

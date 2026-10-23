# Ordered PCH wrapper/member closure

This is the bounded two-body closure approved after the preceding57-function
prefix. Wrapper37, RVA38ad8..38ae2 (10 bytes), sets ECX=104c64b8 and tail-jumps
to member3bd41..3bd53 (18 bytes). The member preserves EDI, clears exactly17
DWORDs through its receiver, returns that receiver in EAX, and executes RET0.
It takes no stack argument. Each function remains a separate application vertex.

The original tail call inherits the initializer loop's actual return385f3. The
native original-style C++ wrapper is compiled at O0 and calls its genuine
thiscall member; its raw member returnPC is consequently inside that wrapper.
Both actual return PCs are observed and bound to their respective callers,
with ECX/EDI/ESP at the real preprologue entry, EAX=this, EDI preservation and
ESP+4 at the actual return. No replacement ABI, assembly wrapper, constructor
side effect, compiler-generated virtual function, RTTI or EH support is used.

## Independently grounded receiver

The exact68-byte PchContext at c64b8 is all modeled; its referenced image,
arena and page-map pointees remain opaque. Four scalars and13 pointer/handle
identities occupy the17 original/native DWORD slots without object padding:

| Offset | Field and own C1XX evidence |
| --- | --- |
| 00 | int image_count: ReadFile writes4 bytes into receiver at3c8ed;3c97b and345a6 use signed loop bounds |
| 04 | PchImage *root_image: zero3c938;9af85 constructs a root image from arena/context, stores result9af8a, then invokes its vtable writer9afc3 |
| 08 | PchImage **images: count*4 allocation3c950, pointer stored3c971, entries filled3c98d and read345af |
| 0c | DWORD stored_file_position: SetFilePointer(FILE_CURRENT)3c9a1 result compared against the field read from file3c9bd |
| 10 | const char *file_name: saved3c898, passed unchanged to CreateFileA9af1e and diagnostics9af32 |
| 14 | LONG file_offset: saved3c89b, used as SetFilePointer's file-begin distance3c8b5 |
| 18 | HANDLE file: CreateFileA result3c8a4/9af25, CloseHandle3451b |
| 1c..30 | six HANDLE mapping_handles: direct CloseHandle at345d8 and cold6f23f/24a/255/260/26b close all six and clear the corresponding slots |
| 34 | int loaded_image_count:3c976 seeds count; context passed from7f5e5 into9acc1, where9ad04 clears it and9ad10..39 increments and signed-compares it |
| 38 | CompilerArena *scratch_arena: passed into image constructor9af85, then arena cleanup2e58a at9aff2, cleared9aff7; arena cleanup uses VirtualFree on its arena fields |
| 3c | PchImage **original_pages:9b09d indexes by old pointed-value>>16 and applies selected image's relocation delta |
| 40 | PchImage **mapped_pages:9b043 indexes by pointer-field address>>16, checks mapped image bounds and visited bitmap before remapping |

Provider identities come from this PE: c91a4 CreateFileA, c91b4 ReadFile,
c91b8 SetFilePointer and c91e4 CloseHandle. None is invoked by the initializer.
All loaded original field defaults are verified zero. No original four-byte
gap after the preceding prototype or any pointed object contents are modeled.

Native BSS is980 bytes:951 semantic bytes and29 natural padding bytes. The
existing two-byte alignment gap now precedes PchContext; it is not an original
data-field placeholder. Initialized projection remains89 semantic bytes;
8431 original initialized bytes are still unmodeled. The real allocation8520,
full43-entry callback table, existing typed copy/restore and hard frontiers
for unknown data consumers remain unchanged.

## Separate prospective and joint execution roles

`pch-prospective-trials.json` freezes the real wrapper reaching the member's
named INT3 with the leaf wholly masked. Its receipts explicitly say raw-only,
and preparer/auditor/promotion guards prohibit acceptance. The common strict
graph tool independently rejects that stage as "unidentified tail-call ABI".
Its native source content contains the genuine stub. It is never relabeled as
a successful single-candidate trial, and future validation reads its immutable
execution snapshots without generating an acceptance audit.

The current `pch-joint-trials.json` instead binds a fresh ordered JOINT trial,
with both actual10+18-byte bodies restored and both genuine source bodies
compiled in Win32/native artifacts. The current strict common graph audit
checks58 source functions plus the genuine import thunk, exact ownership of
both candidates, full extents, RET cleanup, imports and function references.
There is no RET waiver. Both individual acceptance records, if approved,
will explicitly identify the same joint proof and their own extents.

Four fresh joint pairs exercise natural input, both nonzero-state restore
modes and a declared member-entry data auxiliary. The restore probes seed every
modeled PCH field before BSS reset and verify all are cleared before entry.
The separate entry auxiliary seeds scalar values17/23/-11/29 and13 distinct
owned snapshot interior pointer/handle identities +4..+52. No pointer is
called or dereferenced; receiver and code are unchanged. Actual member code
then clears all68 bytes. A one-shot original breakpoint arms only at actual
member entry using stack[0], observes the genuine caller return, and restores
the original EB byte/EIP. Native GDB observes its actual caller return without
forcing it. Complete original BSS is compared before/after with only these68
bytes allowed to change; all previously modeled native neighbors are unchanged.
This does not assert native equality for unmodeled original BSS bytes.

No Wine or PE helper is used in native captures, checks, debugging or fixtures.
Original Wine use is only the explicit compiler/reference capture recorded in
the packet. Earlier completed lifecycle fixture timing13.18ms is reused as a
historical measurement; the partial C++ compilation is not yet a completed
regression fixture. Fresh original/native frontier and control timings are
recorded separately, rather than called fixture runtime.

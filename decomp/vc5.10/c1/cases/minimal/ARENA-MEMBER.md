# Arena reservation member

CompilerArena::Initialize owns98 hot bytes12a18..12a7a plus5 cold bytes3051f..30524.
The Win32 build uses the real implicit member convention; the native build uses
the shared thiscall annotation. Both compile the same full readable source.
The source preserves the descriptor assignment, reserve-size helper, exact
VirtualAlloc call flags, failure return and separate Reset member.

Both fresh cases currently execute only the descriptor assignment before trapping
in arena_reservation_bytes12a7a. Actual preprologue member ECX/stack/formals, its
full invocation/CRT state and the independent primary entry are recorded. The
next helper receives the same actual descriptor0 pointer with a single real
formal. The member's live this/descriptor/address formals are independently read
from its caller frame, preserving the three-slot member contract.

The only observed change is arena0.descriptor: original module+65780 maps to the
actual native descriptor-table object. Every other modeled field and complete
original data/BSS/snapshot region remains unchanged. No pointer is normalized
without an explicit owner identity. No allocation, conversion, Reset or failure
branch executes yet; those branches receive no executed behavior credit.

Shared2d317c1d adds the SDK spelling MEM_TOP_DOWN=0x100000 only. It changes no
VirtualAlloc semantics. The later original coefficient is -0.01; its _ftol
compiler-support boundary will require actual conversion evidence when reached.
No local CRT body has been added.

Strict61-source/3122-code+16-text-data-byte audit, both complete paired positives,
exports and native lifecycle pass. Fresh26 targeted controls reject altered
member invocation/CRT/receiver/formals, helper parameters/caller frame, descriptor
ownership, neighboring arenas/fields, snapshot/initialized state and actual
original store/call/return bytes. They exercise the real state validators in2.57s.
Independent restore positives pass; unchanged prior guards and the same normal
focused result are explicitly reused with frozen hashes rather than rerun.

Original/native builds3.958/1.934s; original/native captures2.120/1.841s normal,
2.471/2.927s restore; audit0.031s. Cached native lifecycle passes in4.31ms excluding
build. Minimal compilation remains incomplete. Evidence is retained under
`evidence/arena-member-{normal,clear-aux}`.

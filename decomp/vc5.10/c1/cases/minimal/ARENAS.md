# Arena initialization

The wrapper12999..129db plus cold30501..30510 owns81 code bytes. It first calls
initialize_primary_arenas129db, then initializes the other descriptor-controlled
arenas and default pools. All three callees remain distinct named INT3 functions;
the reservation member preserves genuine thiscall ECX plus two stack slots/RET8.
Fresh normal and restore runs stop in the first call before any descriptor or
arena-object consumer executes. Later source branches receive no executed credit.

Six encountered20-byte descriptor records are defined as contextual typed data:
kind, reserve_bytes, commit_unit, flags and alignment. The complete120 actual
original bytes independently match the source records. No section is imported,
and these ordinary data bytes receive no code or .text ownership credit.

Each32-byte BSS arena has cursor, committed_limit, base, committed_bytes,
descriptor, reserve_bytes, reset_value and mapped_view. The exact writer/use map
is in ARENA-PLAN.json. reset_value describes only the proven zero store; its
broader role remains unrecovered. Future unknown semantics remain hard frontiers.
All192 bytes have independent original BSS zero-default proof. Native/original
strides and fields match; there is no invented padding field. These objects stay
outside the6548-byte initialized snapshot. Known initialized state remains64B,
with6484 bytes UNMODELED. Known native BSS is969B plus27B compiler padding.

Labeled restore input gives every object distinct nonzero integers and owned
snapshot-interior pointer sentinels. Those pointers are never dereferenced. All
48 fields are observed before and after genuine BSS clearing, at actual wrapper
entry and at the next named trap. All other modeled state and complete original
regions stay unchanged across the wrapper. The earlier full GetSystemInfo proof
ends at its separate actual arena entry, preserving the correct boundary.

Both positive pairs and strict59-source/2943-code-byte plus16-text-data-byte
ownership audit pass. Fresh full-pipeline controls12/12 reject entry/formal/PC/
caller/invocation and raw call/return mutations. Focused target-state controls
126/174 independently reject every new field map, every native default, every
descriptor field and every restore seed. These invoke the actual state validator;
they are explicitly distinct from full-pipeline checks. Prior unchanged protocol,
parser, allocation and SYSTEM_INFO controls are reused through frozen hashes.

Original/native builds3.508/1.909s; original/native captures1.668/1.769s normal,
2.471/2.487s restore; audit0.029s. Focused controls take18.85/25.84s separately.
The cached native lifecycle fixture passes in4.57ms; minimal compilation remains
incomplete. Evidence is in evidence/arena-wrapper-{normal,clear-aux}.

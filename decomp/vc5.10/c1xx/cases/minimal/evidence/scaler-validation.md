# Reservation-size calculator, compiler support and entry x87

This packet leaves the calculator unpromoted for concrete review. It contains
one57-byte application candidate and an existing6-byte compiler import thunk,
with zero application credit for the thunk. Source is an ordinary signed64 cast,
explicit consumed low32 and commit-unit rounding/fallback. The original compiler
and LINK emit _ftol automatically. Native emits nine actual x87 instructions;
there is no local CRT implementation or fabricated native application wrapper.

The shared empty-entry profile is applied exactly once by the host, after its
existing precision53 setup and before any target DLL lifecycle instruction.
Each case binds its own complete original receipt, earliest raw112 bytes and
loaded target/canonicalCRT provider identity. Actual native prestate0x027f/0/
0xffff, arguments/reference, application count0->1 and actual DLL-entry module/
reason/NULL values are checked. No calculator/frontier reseeding occurs.

All three fresh paired cases reach the genuine CompilerArena::Reset stub.
The normal input100 produces signedQWORD-52428800 and rounded50MiB; both restore
modes supply actual input41 and produce -21495808/20.5MiB. Full raw80 live stack
values, CW/status/tag/TOP, native extra FILD/FLD temporary operands/depths,
temporary truncation CW and restored CW are checked. Canonical loaded_ftol
IAT/export/39code bytes and its EDX:EAX result match the actual native FISTP
QWORD stack output, then the whole rounded return/RET0/nonvolatile state.
Empty register payload and IP/DP/selectors are explicitly outside equivalence.

The real following VirtualAlloc uses NULL, rounded bytes, RESERVE|TOP_DOWN and
READWRITE. Its actual caller, provider, RET16, nonvolatile registers, error203
and full live FP state agree. Original VirtualQueryEx and native ownership plus
/proc maps prove opaque reserve-only PROT_NONE regions and64KiB alignment.
Global highest-address ordering and unread reserved payload are not claimed.
Full original BSS/data/saved/support guards cover conversion, size assignment,
API execution and final member continuation. Native modeled/padding/saved/
external storage guards cover every arithmetic checkpoint. Final typed fields
and Reset ECX/no-stackargs/caller are checked independently.

`scaler-trials.json` points to immutable normal and both restore executions.
Prototype normal1 is retained as analysis; normal2 adds the full finalBSS guard
and generator source snapshot.28 new raw FP/profile/ABI/state/allocation controls
pass in93.70s;8 bounded import-mask policy controls are separate. The strict full
common audit is mandatory independently of those raw semantic controls.
Original captures took9.48–9.74s, native GDB5.18–5.36s. The completed lifecycle
fixture is separate:13.46ms total,1.90ms execution,0Wine.

Observers are bounded to the current first-calculator/first-Reset frontier.
Unmasking Reset will run later descriptor iterations; their repeated math/API
observations and receiver identities must be explicitly extended before the
next promotion, rather than treating this one-call proof as loop execution.

Direct reusable-expression-template workflow
============================================

This is an external code-level test, not an ordinary C source reachability
claim. A raw direct-call/reference inventory found only the copier's recursive
self-calls and no absolute entry-address literals for these two helpers; that
does not prove universal unreachability. The ordinary minimal source initializes
the compiler naturally and supplies real integer, double and char-array CType
owners. The marker is the real finish_declarations entry before finalization.
The source and original CL transport are captured twice. C2 object snapshots
are reference context only, not an invoked native backend or reconstruction.

Acceptance order
----------------
Starting at accepted count1010, make_reusable_zero (3c420..3c42c,12 bytes) was
accepted first, then copy_expression_tree (3c430..3c61d,493 bytes). Each had a
fresh original masked entry and matching native named entry before its source
body, followed by twice-original restored whole-body observations. Raw argument
slots, return EAX, RET0/RET8 cleanup, nonvolatile registers, FPU control and
private stack guards are retained. GDB at the first machine instruction can
print stale source-level argument locations; raw stack slots are authoritative.
No handwritten CRT body was added or promoted.

The original debugger prepares only a separate owned input object/private stack
and CPU call context. The native external object wraps main and the marker and
uses a guarded ucontext stack. It is outside the reconstructed graph and is not
instrumented as an application function. No compiler/QC/diagnostic global is
seeded. Normal cached execution invokes native ELF directly, with no debugger,
Wine or original refresh. main still receives only its executable argv and the
captured MSC_CMD_FLAGS contract. stderr aliases stdout as observed in CL.

Six defined contexts
--------------------
0: distinct cdecl/noargs zero constructor; low flag byte gains0x08.
1: mode6 integer leaf; 2: mode6 unary; 3: mode6 binary with two children.
4: mode2 char-array string with five bytes, allocated to eight-byte alignment.
5: mode2 twelve-byte extended value (exact1.5 payload).

Mode6 fixes node allocation to compiler pool2 and skips type interning. Mode2
uses that same real pool through the ordinary indexed path and real interned
type owners. All source fields are defined in separately owned80-byte input.
The initialized pool is empty, not prefilled: its first allocation takes an
existing compiler free-block. Raw free-list/header and prospective data bytes
prove ownership. The accepted allocator clears exactly that old header's bytes
field (observed60), not its full3ff4-byte capacity. The independent contract
predicts all80 observed output bytes from that prior memory and precise stores:
zero-constructor unwritten fields, all20 copied header bytes, child/parent links,
exact string/extended bytes, aligned string tail and untouched remaining bytes.
No broad pointer/padding normalization is used. Raw addresses remain in evidence;
comparisons establish their owned graph identities separately on each side.

Mode6 string/extended payloads are held: the original indexes the supplied6 as a
payload pool beyond the six-element array, aliasing diagnostic_arena. Fixed
node pool2 does not establish payload ownership. Native guards stop immediately
before those unproved allocator accesses; Win32 behavior is unchanged. Permanent,
minus-one and other pools remain untested. No equivalence is claimed for them.

Evidence and corrections
------------------------
manifest.json hashes the complete two-baseline raw evidence for each unit and
records derived contracts. Original direct_call.arguments contains the external
input descriptor0, which the debugger resolves to its owned input pointer.
Each derived-arguments.json binds the immutable event/receipt hashes and records
the actual pointer/mode stack arguments. It corrects interpretation, not history.
Raw receipt/events are never relabelled or rewritten.

Initial observer holds (empty pool and prospective slot) and the native lookup
hold are retained. The first native string body faulted due to a missing
copy_bytes_end declaration: inferred cdecl conflicted with the existing RET12
callee. Including pch.h repairs the actual call cleanup. Raw fault and repaired
native string RET8/nonvolatile/stack observations are retained. Both corrected
payload units and all previous units pass. This was a target declaration issue,
not a shared compatibility change.

Normal commands from the C1 root:
  make -j4 native-expression-tests
  python3 scripts/check-expression-tests.py
  python3 scripts/check-expression-rejections.py

make check/replay include the six new units with the52 public and15 older direct
units. The existing older assembler direct harness still uses native GDB;
these new expression fixtures do not. Explicit original-only capture remains
scripts/capture-frontier.py with --expression-unit, the real marker and explicit
selected --direct-function/--direct-arguments/--direct-body, outside fixtures.
validation.json binds final build, source inputs, checks, timing and coverage.

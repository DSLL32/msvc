C13232.EXE reconstruction (project vc1.0, toolchain1.0-nt)
========================================================

Eighteen cases are complete: minimal C, optimized Win32 hello, DLL,
numeric/varargs, explicit PCH creation/use, do-while, nested SEH, inline
assembly, browser output, /E, /EP, /P, preserved comments, /Zg prototypes,
and automatic-PCH creation, reuse and changed-macro rebuild.

There are710 accepted source functions and833 accepted original entries.
Accepted original extents cover215743/277234 code-section bytes (77.819820%):
196494 frontend bytes,19075 CRT bytes and174 import-thunk bytes. This is a
union clipped to code-section VirtualSize, including shared text data; it is
not branch coverage. Unvisited functions retain named trapping stubs.

Native build and checks
-----------------------
Run from this directory:

  make -j4
  make -j4 check
  make -j4 check CASE=numeric-varargs
  make -j4 check CASE=pch-use
  make -j4 native-check
  make -j4 native-adoption-check

The native i386 build uses Clang -O0 -g. check/replay compare cached original
first-entry projections, status, raw streams, complete file sets and artifacts.
Shared lib/recon C sources are linked directly; target runtime.o and shims are
excluded from application instrumentation. The original BINMODE.OBJ default is
retained through RECON_DEFAULT_FMODE=0x8000. Fixtures never invoke Wine or
recapture references. Unknown definitions and dependencies fail.

cases/original-entry-references.json binds18 completed case histories to fresh
shared-prefix original first-entry captures. All699 accepted application entries
are checked with accepted-prefix seeding and exact expected order. Every native
case checks status, raw streams and complete artifacts. Earlier migration checks
are recorded in cases/shared-prefix-validation.json; automatic-PCH completion and
current all18 regression receipts are in cases/automatic-pch/validation.json.
The native cache is not silently refreshed. Regenerate it explicitly with
scripts/native-traces.py --write after a verified original-toolchain map and
acceptance metadata change.

Case                  irex  irsy  irin  irgl  Other outputs
minimal                866    13     3    50
hello-win32           2066   422    46   629
hello-dll             2129   499    49   669
numeric-varargs       2330   535   149   402  irdb63, msvc.pdb574
pch-create              94    33     3    68  state.pch38156
pch-use               1071   116    47    87
do-while               888    39     3    50
seh                   1134   114     6    79
inline-asm            1100   107     6    75
browser                971    69     6    79  test.sbr245
preprocessor-E           -     -     -     -  stdout381
preprocessor-EP          -     -     -     -  stdout292
preprocessor-P           -     -     -     -  test.i381
comments                 -     -     -     -  stdout337
prototypes               -     -     -     -  stdout360
automatic-pch-create    179    67     6    77  state.pch38021
automatic-pch-reuse     999    67     6    85  stdout26; state.pch unchanged
automatic-pch-changed   179    67     6    77  state.pch38022

All primary intermediates match raw bytes. Numeric stderr retains the captured
line24 C4069 CRLF warning. Only identified timestamp DWORDs at irdb+14 and
msvc.pdb+48 are ignored, after raw equality between both fields is checked.
No path/pointer normalization occurs. The exact PDB path is
S:\vc1.0\c1\numeric-varargs\msvc.pdb. The actual native PCH producer must
match the reference before its bytes are supplied to the consumer; selecting
a dependent case runs its producer first. Header mtimes are pinned input data
for automatic PCH. Original CL's stdout/stderr alias is preserved where required.

Original builds and reference capture
-------------------------------------
These explicit operations are separate from fixtures:

  make -j2 audit-win32
  python3 scripts/coverage.py
  python3 scripts/build-capture-helper.py

Original builds/captures use ~/.ai-tmp/vc1.0-capture/wine. Fixed R: maps to the
repository and S: to ~/.ai-tmp/msvc2. No tool remaps those drives or stops the
shared server. Relative paths are used where output permits them. Observable
absolute case paths use S:\vc1.0\c1\<case>; C1 serializes each fixed original
cwd and snapshots its complete contents. Native runs use independent host
namespaces through per-process RECON_DRIVE_R/S mappings.

src/capture-original.c is a separate original-only debugging utility, never
linked into C1 or executed by fixtures. It accepts an ordered entry plan,
restores exactly one instruction byte on a matched breakpoint, rewinds EIP,
and retains register/stack/pending-buffer evidence at a terminal trap.
Wrong entry order is rejected. scripts/capture-frontier.py validates the
plan, frozen source dependencies and exact one-entry MSC_CMD_FLAGS environment.
Full-reference captures and masked captures are explicit, never silent.

Original compiler audits check710 function graphs and instruction boundaries,
280 state/default aliases,105 PCH roots and12 callbacks, the21-entry function
shape cache, scalar options, pragmas, intrinsic groups and serialization data.
The C grammar audit checks376 states,57904 actions/timings,227 productions and
34216 gotos. The assembler audit checks136 states,6936 actions/errors,
136 lookahead/default timings,83 productions and3128 gotos, plus contextual
operand/register/mnemonic tables. Logical model and linked C rows both match.

Source layout and active queue
-------------------------------
Source lives in src/, scripts in scripts/, metadata in cases/. Function
ownership, extents, accepted order and text data are recorded in functions.json.
Frontend subsystems keep their contextual data and semantic wire structures;
no copied image sections or runtime original-image reads are used by C1.
store_extended retains its documented ESI/x87 ABI and a native-only entry marker
that preserves registers, flags and floating state. The source also compiles
with the original Win32 compiler; recon.h is empty there.

The approved queue is cases/approved-queue.json. Comment preservation and
prototype output and automatic PCH are complete. The worker is paused pending
the next selected case. Existing sources,
original options/environments, dependencies, outputs and acceptance ranges are
durable. Independent grammar/layout/ABI analysis remains available in scripts/
and cases/analysis/. Measured builds, setup, reference capture, native execution
and checks are reported separately in current validation receipts.

# Help capture and scaffold review

This is the initial proposal retained for context; the baseline was subsequently
approved and executed, with its timeout receipt and diagnostics under this case.
No masking or reconstruction acceptance has occurred. Native tests use GCC i386 ABI; original-toolchain
Win32 builds preserve Watcom ABI. No adapters between individual app functions.

## Explicit original capture

Create separate empty `unmasked` and `masked` directories under
`/root/.ai-tmp/msvc2/watcom10.0a/wlib/help/`. From `unmasked`, run the equivalent
of this subprocess argument vector, with stdin containing 64 newline bytes then
EOF, both streams redirected separately to raw binary files, and a 30-second
process-specific timeout:

```
/usr/bin/wine R:\toolchains\watcom10.0a\binnt\WLIB.EXE -?
```

Set `WINEPREFIX=/root/.ai-tmp/vc1.0-capture/wine`, `WINEDEBUG=-all`,
`WATCOM=R:\toolchains\watcom10.0a`, `LANG=C`, `LC_ALL=C`, `TZ=UTC0`.
Remove `WLIB` and `WLIB_OPTIONS` if present so a caller's command-line defaults
cannot silently influence the help case. Retain the remaining host environment,
record its complete values before launch, and separately capture the complete
target-visible environment and actual raw command line at application entry.
Retaining and recording remaining values avoids inventing an unproved minimized
Windows environment. Do not remap drives, change prefix registry configuration,
stop the shared wineserver, or kill any process outside this capture's ownership.

The newline input is intentional: the newer source paginates help with `getch`.
The historical binary's console/EOF behavior is not yet proved. Record all
actual console API reads and their return state. If redirected input cannot
drive this invocation deterministically, hold and report the real transport
problem rather than synthesize keystrokes or silently change the case.

Record original SHA256, exact invocation, stdin bytes, both environments, exit
status, raw streams, directory inventory and every produced file. No expected
status or help bytes are assumed from the newer source. Run twice only if needed
to establish determinism; retain both raw receipts and explain any difference.

## Entry, masking and CRT boundary

The native PE32 loads directly under Wine; WLIB needs neither a CF payload
launcher nor WLINK's GS patch. Its untouched image stays hash-bound. Create a
case-local masked copy, retaining the original overlay and PE metadata exactly.

Startup's `0x41afea` call targets application candidate `main@0x412a67`, with
EAX=`argc` and EDX=`argv`. Initial debugger capture must confirm that call and
record each argv string/vector owner, raw command line and reached startup
state. The first native source is an INT3 `main` stub with corresponding GCC
formals; its entry is the first application frontier, not a CRT reconstruction.

Do not mask the whole BEGTEXT section and then permit an arbitrary startup range:
it contains executable instructions, literal data and CRT routines. Before
accepting the first app function, identify reached startup/runtime bodies from
shipped `lib386/NT/CLIB3R.LIB` (and associated startup objects if present), recording
member identity, relocations, exact original extents and called import boundaries.
Initially it is permissible to arm a debugger breakpoint at `main` solely for
entry discovery; that is not a successful fully masked replay or app acceptance.
Any unidentified callee remains a named INT3 app/support frontier until its
identity is established. No handwritten CRT bodies and no blanket range waiver.

Once extents and code-versus-data ownership are established, mask executable
instructions of unaccepted app/support functions, leave only proven library/
startup/import boundaries and encountered `.text` data live, and capture the
same next logical frontier against native execution. Preserve original Watcom
arguments/returns in its Win32 audit; compare semantic values and ownership with
GCC-native formals. Do not compare incidental register allocation.

## Native and original builds

Use GNU make `-j4`. Candidate native recipe uses `gcc -m32 -std=gnu89 -O0 -g
-fno-inline -fno-pie`, `RECON_NATIVE_TEST`, shared recon include/native paths and
normal GCC calling conventions. Compile shared `lib/recon/src/*.c` directly at
final `-no-pie` link; instrument only application C files for native entry traces.
An INT3 stub is permitted inline assembly, not reconstructed logic. No finished
help fixture exists until cached references and trace/stream/artifact checks do.

The common WCC386 compile-only probe passed with status 0 and no warnings in
0.661 seconds; receipt is in shared scratch
`watcom10.0a/wcc386/abi-probe/receipt.json`. WLIB's own link recipe remains
unverified. Expected form is WCC386 `-bt=nt -mf -od -d2`, period headers and
default Watcom register ABI, then WLINK `system nt`/CLIB3R against reconstructed
objects. Use the separately verified patched WLINK only as approved by the
orchestrator. Do not claim a compiler check until the actual probe/build passes.

WLIB's 26-byte Watcom `FILE` differs from the current 32-byte MS CRT compatibility
layout. Its actual reached stdio/file APIs and ownership/flags must be observed
and sent to `/root/recon_lib_maintenance` before enabling such operations. A
native executable that merely traps at main needs no FILE profile. Unknown CRT
semantics, service failures and unproved application callees remain explicit
holds, not permissive fallback or successful fixture results.

Native regressions execute ELF only. Compare cached status, raw streams,
artifacts and filtered application entry traces; never capture originals from a
fixture target. Record elapsed build/setup/execution/check times.

# Authored CF arena fixture

Instrumentation plumbing only. No original Watcom application is reconstructed,
run, selected or accepted here. The two tiny Win32 programs have been compiled
with MSVC 5.10, and Rust has exported their actual decoded rows. **Neither
authored program has been executed.** Runtime requires the separate combined
broker/controller plan review.

`arena-loader.c` is a no-CRT PE with a custom entry. Its only normal input is one
raw command-line CF filename. It loads a bounded fixture, allocates RWX private
memory, applies checked grouped DWORD fixups, publishes three globals and calls
`probe_ready`. Ready only indirect-calls the published arena entry, then loops.
No arena byte executes before the Ready boundary. The explicit optional suffix
`thread-negative` creates one authored thread after publication and before Ready;
its callback would set `probe_thread_marker` to `0x11223344`. That mode is only
for proving the unchanged broker denies CREATE_THREAD before callback execution.

`arena-payload.c` is a real separately linked no-CRT PE. `payload.cf` contains its
initialized virtual image at logical base zero, including zero-initialized
virtual bytes. Only actual PE HIGHLOW DWORD addends are normalized by the actual
ImageBase; the original disjoint HIGHLOW locations become grouped CF fixups.
The stream follows SizeOfImage. No object-file conversion, instruction rewriting,
fake PE, second CFG walker or production backend is involved.

There is one fixture family: a known returning leaf, direct/indirect near CALLs,
direct/indirect near JMP helpers, unknown direct/indirect destinations, and one
deliberately forged near return. The exact JMP/return-slot helpers use documented
naked inline assembly solely because ordinary C cannot specify those transfers.
All other logic is ordinary C. `profile.json` owns the actual function bounds,
marker, initialized function-pointer slots, header/fixups and child globals.
Unknown code would write `0xdeadbeef` and must remain unexecuted at a frontier.

Build-only regeneration:

```sh
python3 tooling/watcom-capture/tests/cf-arena/prepare.py --build-only
```

This performs explicit MSVC compile/link checks through the shared Wine prefix,
then static Rust audits. There is no child-runtime code in this preparer. The
reusable build inputs are individually source/tool/hash bound; an unchanged
payload/loader may be reused with its prior build receipt instead of rebuilt.
Compilation failures and static failures are evidence, never silently discarded.

The three exact Rust commands are `command.json`, `ret-audit/command.json` and
`ready-audit/command.json`. They use the shared release binary directly with
`--profile cl --discovery cfg --export-instructions` over actual PE/MAP/CF inputs.
They do not run Cargo, Wine or a child. Full rows and ownership/extent checks are
in `payload-export.json`; `payload-ret-export.json` adds strict RET checks for the
returning leaf, unknown leaf, direct JMP and forged-RET helper. Ready is a paired
PE self-comparison in `ready-export.json`. The accepted-name list in these
authored static inputs selects test owners and grants no application acceptance.

The first attempted strict all-owner RET audit correctly rejected
`_probe_jump_indirect` as an unresolved indirect tail ABI. Its source, commands,
compiler logs and failed audit are retained under `failures/attempt1-indirect-tail-ret/`.
The full export therefore has **checked RET = null**, not checked zero. A separate
strict subset verifies actual returning paths. The authored tail contract is
explicit: the near JMP retains the existing live caller frame; an admitted leaf
has RET0, while an unknown destination must stop before its body. The controller
must verify that contract and real target/stack state. This is not a production
audit exemption or permission derived from null RET facts.

WCC owns the broker/Gate adapter. Its combined runtime plan may place the exact
`payload.cf` bytes at a stable owned `S:` path, stop at the real Ready first byte,
read/verify the entire relocated allocation and register that observed private
arena through the production CF admission path. `profile.json` supplies all
exact RVAs; callers must verify hashes rather than copy addresses from prose.
Header-entry variants must be individually hashed and described in the runtime
plan; no duplicate variant binaries are stored here.

No runtime success, stack-return enforcement, CREATE_THREAD denial or frontier
non-execution claim is made by these build/static receipts.

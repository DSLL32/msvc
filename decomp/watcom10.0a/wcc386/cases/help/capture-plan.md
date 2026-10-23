# Proposed help capture and frontier protocol

This is a review proposal, not an implemented or validated controller.

## Unmodified baseline

Use a new empty working directory
`/root/.ai-tmp/msvc2/watcom10.0a/wcc386/help-baseline` and run exactly:

```
wine 'R:\toolchains\watcom10.0a\binnt\WCC386.EXE' '?'
```

Set `WINEPREFIX=/root/.ai-tmp/vc1.0-capture/wine`, `WINEDEBUG=-all`, and
`WATCOM=R:\toolchains\watcom10.0a`; remove Unix WCC386/WCC/INCLUDE overrides.
Redirect stdin from empty bytes and retain binary stdout/stderr, status, elapsed
time, original hashes, command, working directory and output-file inventory.
Use no terminal, shell child, compiler or linker. Limit execution to 30 seconds.
The stub's module-name transformation selects its matching binb payload; the
successful compiler probe already exercised that route. Capture actual Windows
environment inputs separately before any later state-equivalence claim.

## Relocated image observation

Prefer the existing Win32 debug-event controller pattern used by VC captures,
with a small separately reviewed CF launch observer. Bind its source/build hash
and exact event protocol before running it. At CREATE_PROCESS, derive the real
stub module base rather than assuming the preferred PE base. Install a checked
one-byte breakpoint at stub RVA `0x10763`, immediately before `_InvokePgm`.

At that breakpoint, read CodeLoadAddr/BaseAddr at stub RVAs `0x30004`/`0x30008`,
CodeEntryPoint at `0x30000`, and the actual thread registers/parameter block.
Require the two allocation pointers to agree and entry to equal CF base plus
`0x2fe00`. Reconstruct expected loaded bytes from the immutable file and all
11801 exact DWORD additions; verify the complete initialized image before any
mask write. Uninitialized allocation tail receives no invented contents.

Retain original bytes, relocation ownership and pre/post-mask snapshots. Plant
the entry trap only after relocation. Restore the stub launch instruction and
run through the specifically verified launch sequence to that trap. `_InvokePgm`
sets AH=2, ECX=0, and BX=CS; EDX is the far gateway and EDI the parameter block.
Do not interpret entry EBX as the allocation base. The actual entry values and
loaded data, not the 2002 source alone, determine the contract.

## Fail-closed discovery

The CF image has no proved complete code-section map. Masking only known
functions must therefore never authorize execution of unknown code elsewhere.
Begin with the CF entry trapped; classify startup/library functions separately
before allowing their instructions. Use instruction stepping initially, with
the next transfer inspected before execution. Stop on undecoded or unsupported
instructions rather than guessing.

For direct/conditional branches, loop edges and fallthrough, validate the actual
destination against accepted code ownership. For calls, resolve direct or
register/memory targets, validate their readable operand ownership, and plant a
verified target-entry trap before executing an unknown transfer. This preserves
the genuine pushed return address while preventing the target's first original
instruction from executing. Track actual call/return nesting; validate RET and
far-return targets and stack cleanup before transfer. Indirect jumps and tail
calls require the same target guard. A raw PC address match is insufficient.

Do not freely run a CRT or OS operation that can call back into unguarded
application code. Callback targets and additional threads must be covered by the
same pre-execution rule or execution stays stopped. The far gateway is a
positively identified service boundary, not a waiver for unknown payload code.
Capture reached six-register/flags input and result, pointer-owned bytes,
handle/file/stream changes and errors before replacing a particular service.

Mandatory controller negatives include an unlisted direct target, an indirect
target, an escaped fallthrough, an unmatched return, and an unlisted callback.
Each must stop before a target-side marker changes. Wrong relocation bytes,
wrong image base, missing trap installation or unowned data writes must also
reject. These controls precede any application acceptance. Native fixture runs
use cached evidence and never invoke this original-only controller.

## First application/native build

Static CRT function `0x485c0` loads argc/argv and calls CF offset `0x1a99b`, a
candidate conventional `main`. Its instructions ignore the incoming values,
initialize three globals, call `0x1a9c5`, call `0x1183`, and return a global.
This differs from the 2002 `cintmain.c`; do not transplant that source wholesale.
No candidate is accepted yet.

After the original entry and compatibility-prefix state are established, add a
typed `main` INT3 stub and named reached stubs. Build the same Win32 source with
Watcom and native GCC i386, using debug symbols and the shared recon C sources
directly at native link. The user permits GCC calling conventions for native
tests; prove semantic arguments/ownership there and audit the actual register,
stack, callback and aggregate ABI in the original Watcom-built PE separately.

Shared FILE support needs an explicit Watcom profile: the shipped stdio FILE is
packed to one byte and 26 bytes, unlike the existing 32-byte MSVC layout. No raw
casts or target-local FILE compatibility should bypass the shared maintainer.
Likewise startup/command-tail/environment/stream identity needs a proved shared
contract before entry-state equivalence is claimed. Build targets remain blocked
until these prerequisites have a concrete implementation and review.

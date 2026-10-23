# WLIB main frontier: initial analysis

This document records the initial read-only proposal. Subsequent reviewed work
reconstructed main and captured the masked original prefix; see
`prefix-capture-proposal.json`, `bootstrap-prefix-provenance.json` and
`original-prefix-01/`. The original common-broker dependency below was superseded
by the reviewed minimal extension to WLIB's existing PE helper. Native heap and
startup compatibility remain the current paired-acceptance gates.

No application function is accepted. This analysis executed neither WLIB nor its
reconstruction, and changed no reconstructed source or compiler flags.
`edges.json` binds the actual 1994 bytes, 13 call sites, nine distinct callees,
header contracts and later source/library evidence. Addresses below are VAs;
function ends in metadata are exclusive.

`main` is `0x412a67..0x412af5` (142 bytes). Watcom passes argc in EAX and argv in
EDX; main saves them in ECX/EBX. It invokes a heap check, the unidentified empty
hook, resource initialization, four signal registrations, then setjmp. Its zero
AL path initializes tool state and processes commands. It then releases tool
state/resources, calls the empty hook again, and returns error-flag bit 6 as
integer 0/1. There are no indirect calls in main. Its four pointer references to
`0x412a5c` are real application callback references, not calls or CRT vertices.
That callback ignores its signal argument, writes `0x430010 = 1`, and returns.

| Callee | Classification and evidence | Semantic formals / original ABI |
| --- | --- | --- |
| `417ec3` | `_heapchk`/`_nheapchk` CRT candidate; free-list, heap-walk and `_HEAP*` cases match source/library | `int(void)`, EAX result, RET0; result ignored; private heap state still matters |
| `411fdf` | Exact one-byte RET, application/support until identified | No consumed formal, RET0, no state writes; both sites share this physical vertex |
| `414bfe` | `InitMsg` candidate, resource support | argv0 in EAX, status in EAX, RET0; resource handle/filename/status state |
| `4181ae` | `signal` CRT candidate | signal in EAX, callback in EDX, previous handler in EAX, RET0 |
| `4185e8` | `_setjmp` CRT candidate | special EAX jmp_buf formal, EAX result, RET0; main tests AL |
| `412af5` | Tool initialization, application | argc EAX / argv EDX, RET0; initializes options, pools and parser state |
| `412c40` | Options/commands, application | No formal established; AL result ignored by main, RET0 |
| `413278` | Tool teardown, application | No formal established; retains distinct tail edge to `4119f4` |
| `414ec6` | `FiniMsg` candidate, resource support | No formal, EAX status, RET0; actual CloseResFile edge |

These names are evidence levels, not acceptance or symbol information recovered
from WLIB. The shipped NT `CLIB3R.LIB` publics identify `nheapchk`, `signlwnt` and
`stjmp386`; every parsed module record checksum was checked. Their bodies are
not byte-identical to this target: signal's TLS offsets are `78/7c` versus the
library's `58/5c`, and setjmp's segment-store prefixes/instruction encodings
also differ. Source correlation is to the exact 2002 checkout, not a claim that
its newer subsystem/DLL architecture was present in 1994. No new CRT source is
proposed. `411fdf` must not be renamed InitMem/FiniMem or exempted by proximity.

## Canonical audit result

`graph-audit-receipt.json` records the Rust tool invocation with
`--profile cl --discovery cfg --map-format watcom10`, the real Watcom-built
scaffold PE and unmodified MAP. The isolated `audit-candidate.json` selects main
only to demonstrate an expected failure, not as a project acceptance manifest.
In 5.35 ms the canonical CFG engine reported exactly the nine original targets;
the current INT3 scaffold instead has a compiler-generated `__CHK` edge.
The expected failure is retained. Original main has no stack-check call. A
future original-compiler flag such as `-s` needs a compile/MAP/body proof before
adoption; do not silently filter the added edge or reinterpret the failure as
successful graph reconstruction. No new decoder/controller is needed here.

## Smallest next execution plan, for review

1. Reuse `tooling/watcom-capture/` and its shared Win32 broker/gate. WCC confirms
   that PE/CF application operations are not yet implemented there. Do not fork
   the resource diagnostic helper into a second acceptance controller: its
   unmasked reference results establish inputs/outputs, not function acceptance.
2. Add a reviewed PE descriptor for the immutable patched copy and pristine
   parent hash, exact relocated image identity, main pin `412a67`, native named
   main, and case inputs below. Explicitly inventory any CRT bootstrap ownership
   needed to reach main while retaining the whole-code INT3 masking policy.
   Startup must not receive a blanket interval/free-run exemption. Unknown
   initialization callbacks, code/data extents or transfers remain HOLDs. This
   bootstrap proof and the common broker PE operation are current real gates.
3. Obtain fresh matching original/native traps before either main body executes.
   Original formals are argc EAX/argv EDX; GCC i386 native formals are semantic
   argc/argv stack arguments. Compare values and owned byte objects, never raw
   pointer addresses or original/GCC register identity. The common transport
   must prove the owned original trap, exception/thread state and actual stack.
4. Only after review, reconstruct main with named trapping application callees,
   preserving all 13 sites, callback references, AL branch and final flag test.
   Build with the original compiler and native GCC; audit the genuine Watcom
   PE graph/ABIs. The next first call is the public `_heapchk` boundary, not an
   application function to hand-reconstruct. Stop there until its typed native
   surface/provider and state contract have been reviewed. No assumed success0.
5. Once that reached boundary is evidenced, the next application frontier is
   the unidentified RET hook. Define later resource/signal/jump state only as
   encountered. Their future requirements below are not authorization to skip
   application/resource helpers or implement broad compatibility now.

## Exact input and state needs

Use the separately cached bare `?` case, not a relabelled `-?` diagnostic.
Its actual argv is `[R:\toolchains\watcom10.0a\.patched\WLIB.EXE, "?"]`, cwd an
empty case directory, stdin 64 LF bytes in a pipe followed by EOF, stdout/stderr
fresh regular files. Root-authorized references give exit 0, 1,870 stdout bytes
SHA256 `5dcc9a3c70461c3a960807635dd3ddf6f52c9799dfd1c80c5c4f89dfa830fc54`,
empty stderr and no artifacts. Full host environment was privately recorded;
complete target Windows environment was not observed. The native launcher can
supply the observed argv0 as ordinary process input and use the shared drive
mapping shim for the hash-bound self-image resource input. It must never invoke
Wine or recapture from a fixture. Concrete native provider setup still needs
maintainer review; default Linux argv0 would change an application input.

At main, read argc, all argc+1 argv slots and bounded NUL strings, but establish
actual vector/string allocation ownership before claiming capacities. Capture
native logical counterparts. Do not inject private state/callbacks or jump EIP
to main to avoid original initialization.

For `_heapchk`, retain actual arguments (none), entry/return, return value,
heap-clean flag `430450`, heap-root state and the actual access/release callbacks
(initial file pointers at `420990/420998` are `418d82`, but startup may change
runtime state). Prove ownership/provider and whether private state affects a
later public call. Main ignores the result, which does not itself license a
constant-success shim.

For later `signal`, observe all registrations and actual returned handler
identities before defining a profile. Watcom numbers are ABRT1, BREAK7, TERM6,
INT4 and SIG_IGN/DFL/ERR are pointer tokens1/2/3. Host/MS values differ. Prove any
actual callback delivery separately; unknown delivery stays denied.

For later `_setjmp`, the shipped header declares a 52-byte `unsigned[13]`
object, special caller EAX formal and returns-twice behavior. Capture all52
before/after bytes, actual48-byte write coverage and untouched final DWORD,
return/caller continuation, preserved registers and FS-chain semantics. A
reviewed host context registry may be reused; original Windows registers/FS
are not copied into native host state. Main's AL consumption remains explicit.

The shim maintainer has confirmed these are separate reached contracts; there
is no existing Watcom signal/jump profile. FILE26 and later allocator/resource
paths remain separate gates. All failures stop dependent execution and retain
actual owned-process exit evidence and failed provenance.

# Initial help capture proposal

This is a reviewable plan, not an executed capture. No status, output, application
extent or service-boundary equivalence is assumed. Use the common CF loader,
masker and observer work coordinated by WCC386; do not introduce a fake PE image.

The two unmodified baseline runs described below were subsequently approved and
executed: both exit 1 with identical 6,228-byte stdout, empty stderr and no created
files. See `reference/baseline-receipt.json`. The observer/masking phases remain
unexecuted; `observer-proposal.md` separates the next launch-only preflight.

## Invocation and environment

Create fixed empty work directory
`/root/.ai-tmp/msvc2/watcom10.0a/wpp386/help/work` and separate evidence directories
for each run. Its Windows cwd is `S:\watcom10.0a\wpp386\help\work`. Capture an
input inventory before each run and an output inventory afterward; stdin is an
empty ordinary file and stdout/stderr are separate ordinary files. Keep the cwd
and visible program path identical across repeated captures.

The unmodified baseline invocation, from that work directory, will be:

```sh
env -u WPP386 WINEPREFIX=/root/.ai-tmp/vc1.0-capture/wine \
    WINEDEBUG=-all LC_ALL=C LANG=C TZ=UTC \
    WATCOM='R:\toolchains\watcom10.0a' \
    wine 'R:\toolchains\watcom10.0a\binnt\WPP386.EXE' '?' \
    < ../reference-a/stdin.bin > ../reference-a/stdout.bin \
    2> ../reference-a/stderr.bin
```

Run B substitutes only the evidence-directory suffix, after archiving and
emptying the owned work directory. Preserve the actual launch environment and
the Windows environment block observed in the CF parameter block; inherited
Wine defaults are evidence, not assumed constants. Record consumed variables
and option-source files. WPP386 option injection is absent. No INCLUDE or PATH
override is needed for a no-source help case unless actual observations prove
otherwise. Do not remap drives, create a prefix, or stop the shared server.

Bare `?` is deliberate: the later source recognizes it before option processing.
Confirm that the 1994 program displays help rather than silently changing the
case to `/help` or `/?`. Redirected handles should avoid interactive paging;
if the original still waits for input, stop and report that behavior for review.
Retain raw exit code, streams, timing and every created file. No output hash or
success status is specified until observed.

## Allocation, relocation and first entry

The common Win32 debugger observer will launch the exact PE stub with these same
arguments and handles. Derive the stub module base from its load event rather
than assuming preferred VA `0x400000`. At loader completion (currently identified
stub RVA `0x102f5`) capture `CodeLoadAddr`/`BaseAddr` from actual stub globals and
the CF allocation mapping, and verify loaded bytes against the original payload
after applying all 15,085 decoded relocations. Preserve original-file hashes and
the observed load path; the stub's BINNT-to-BINB rewrite must resolve to the
copied repository payload.

The launch breakpoint is stub RVA `0x10763` (preferred VA `0x410763`), immediately
before the call to `_InvokePgm`; CF relocation has completed at this point. Use
that breakpoint for the first actual payload launch proof and for subsequent
post-relocation masking. Before transfer, retain the real register context and
the EDI parameter block:
program name, command tail, environment pointer, break flag and copyright
pointer. Stub `_InvokePgm` loads AH=2, CX=CS and transfers through ESI with a far
return frame. Verify rather than seed these values. Record the actual far-call
gateway and confirm the first payload PC is allocated-base + `0x664f4`.
Logical application IDs are payload offsets; absolute addresses remain in raw
evidence, with pointer comparisons using independently proven owners/offsets.

The first observer preflight is inspection only and must not claim acceptance of
startup or an application function. The existing app `main` candidate `0x43fef`
and startup/`__CMain` candidates need separate provenance and extent validation.
Do not skip unknown callbacks or count startup/library code as application work.

## Masking and the accepted-function loop

Build a reviewed executable/data extent map before enabling the case. The CF
payload is not a PE section and includes data at offset zero, typed tables,
startup code, CRT code and a relocation stream later cleared as BSS. The common
masker must trap executable application extents while preserving proven data,
relocation inputs and reviewed compatibility/startup code. Any range not yet
classified remains a gate, not an automatically exempt runtime region.

Install the mask through the observer at the stub launch breakpoint, after
loader relocation and before first payload execution, leaving the source
toolchain immutable. Verify each original/masked range and the mask-plan hash.
Disk masking is unsuitable: the loader would add its base to trap bytes at
DWORD relocation sites. Never mask the whole allocation or use allocated
addresses as stable logical IDs.

At each application frontier collect fresh original and native entry evidence.
Decode original Watcom register/stack formals and native GCC formals separately;
compare their semantic values, proven pointer owners/aliases, defined bytes and
effects. Require the original Win32 build's exact Watcom signature/ABI and audit
its graph. Do not require native raw registers to equal the originals. Reject
unknown boundaries, unexplained control-flow differences and unowned pointers.

## Gateway and native executable

Observe actual entries/returns through payload wrapper `0x665c7` and the supplied
far-call gateway. Record the service selector and all consumed register lanes,
carry/result, flags and exact input/output memory ranges with ownership. Classify
reached library wrappers individually using actual body/library/source evidence.
The reconstruction may use normal Win32/CRT APIs at this reviewed compatibility
boundary; differences there do not authorize dropping application calls.

Native target will compile readable reconstructed C with GCC
`-m32 -std=gnu89 -O0 -g -fno-pie -fno-omit-frame-pointer`, ordinary GCC ABI, and
`-DRECON_NATIVE_TEST` plus shared recon include directories. Compile the shared
shim C sources directly at final link, `-no-pie`; use `make -j4`. Native tracing
instruments application objects and uses shared tracing support; filter only
positively classified compatibility entries. No ABI veneers, private CRT bodies
or OMF translation. Request any new profile/API support from the shared maintainer.

The original build will use the shipped WCC386/WLINK tools and original Watcom
headers/libraries through the shared prefix; WCC386 owns the common compile/link
probe. WCC386's shared probe has now successfully compiled with
`-bt=nt -3r -od -d2`, producing OMF32 and confirming the register convention;
its receipt is under
`/root/.ai-tmp/msvc2/watcom10.0a/wcc386/abi-probe/`. The final WLINK invocation
and complete executable build still need verification before this Makefile
recipe is enabled. The current `main` scaffold only traps and grants no coverage.

After capture approval and validated transport, reference capture is explicit.
Native fixtures only read the frozen case inputs, status, stream/artifact masters
and filtered trace. They must never launch Wine, a reference observer or a helper
which recaptures an original. Report setup/build/execution/check elapsed time
separately where measured.

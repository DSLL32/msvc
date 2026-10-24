Direct library/module diagnostics (991 accepted frontiers)
=========================================================

Scope and ownership

This external direct-support workflow reconstructs two LINK-owned cdecl bodies:
add_default_libraries at35760..3579a (58bytes, two actual formals) and
print_module_debug_info at35520..35584 (100bytes, no formals and two actual global
owner inputs). It does not establish public command-line reach of either body.
There is no new CRT implementation, import acceptance or shared API change.

The real original main(3, argv) first performs a full CodeView link using the
owned original-toolchain helpers.lib. After main genuinely returns0, the existing
owned-stack/sentinel seam invokes real create_image and uses its LibraryList for
populated and empty comma-separated library inputs. It then prints the two live
ModuleDebugInfo records produced by that main. The external caller restores the
actual CRT continuation, saved main result and nonvolatiles; support-call return
values never become process status. Native __wrap_main delegates to __real_main
with the same three arguments, appends the same calls and returns the saved result.
No nested detour, private-global seed, fabricated module record or new stepper is
used. Source metadata, owner edges, guard bytes and constructor output are checked.

The 76-byte module rows and owned library/module/source strings remain live through
these calls; source and rows remain immutable. create_image does not replace the
global module owner. DefaultLibrary is12bytes: next/name pointers and enabled byte
at8. Uninitialized padding9..11 is retained raw and checked for within-image
immutability; it is not serialized as defined output or compared as cross-image
payload. The actual comma delimiter pointer has its contextual4-byte definition
in options.c. Ordinary format literals remain at their point of use.

Inputs and exact effects

The original-backed archive contains two C++ modules with real COFF/CodeView data.
modules.rsp requests both COFF and CodeView, /nopack and an explicit MAP file.
There is no CVPACK, PDB provider, helper child or execution of the generated PE.
The first unspecified-subsystem probe really failed23; its raw state/files remain
separate. Explicit /subsystem:console is the corrected ordinary input. An explicit
MAP also leaves stdout open after normal main, allowing appended diagnostics.

The successful baseline has203-byte stdout, empty stderr, exit0, a433-byte external
driver report, a2097152-byte image and1228-byte MAP. Full files and input inventory
are exact. The baseline captured time(&TimeDateStamp) result1792809429 is a native
creation input through existing RECON_FIXED_TIME, not output rewriting. There is
zero normalization. The image independently contains COFF, CodeView NB05 and FPO
debug directories. The report records actual module names and the deduplicated
LIBCMT.LIB/USER32.lib/GDI32.lib list; the empty parser leaves it unchanged.

Fresh acceptance and retained differences

Fresh masked originals stop at the same two new functions in order, observe both
formals or the complete hidden owner inputs, then return through the full workflow
and normal CRT exit. Unexpected callees remain masked. Native GDB compares actual
state and follows normal continuation. The first observer attempted a zero-length
memory read for the no-formal function; that failed packet remains, with the
observer correction documented separately.

Raw original post-main LastError1224 versus native0 and dead x87 C0(0100) versus0
are retained. Parent-reviewed analysis/state-qualification.json permits only those
inherited differences for this proven no-consumer interval. Original DR0 watches
TEB+34 and a trapped GetLastError, native hardware access watch and named API guard,
all run from actual main return through normal exit. Neither side records a read
or new thread. There are no host/shim/observer execution exclusions. errno/DOS,
consumed returns, control/tags/exceptions and other status bits remain strict.
The FCOMP/FNSTSW versus FUCOMIP/EFLAGS producer difference is explicit; empty-tag
stale payload/IP bytes are raw inert state under the same no-FP-consumer proof,
not byte-equal values. The ambient provider discrepancy remains unexplained.
A future reader, FP consumer, unknown path or new thread reopens this qualification.
35 offline controls check the scope and reject other state/access changes.

An accounting gate caught native baseline-clock artifacts being compared against
a later fresh original clock. Those were baseline matches, not fresh-pair matches.
analysis/clock-input-correction.json retains both clocks and the failed comparison.
A single native-only reobservation used the fresh original's actual1792810165 clock;
acceptance/closure/fresh-pair.json now binds that input and both raw file hashes.
The fresh EXE, MAP, streams and report are exact with zero normalization. No original
repeat or output repair was used. The no-read packet alone makes no file-equality
claim. Both application acceptances were appended only after this gate passed.

Verification and accounting

All86 cached native cases pass in8.827s; native/trace build11.415s and trace-cache
projection1.142s are separate. The new traced fixture totals.273854s: setup.228061s
(including.209384s trace preparation), direct ELF6.477ms, oracle31.091ms and trace
comparison.148ms. Output-only totals.051995s with actual ELF5.367ms. GDB acceptance
is separate; final clock-aligned observation.213693s. Original observations are
explicit capture operations outside fixtures, never fixture dependencies.

Original VC1 make-j4 build21.195s passes. Common CFG/ABI/reference/ownership/extent
checks cover830graphs;813 callback,512 contextual-data and41 ECX checks pass.
Meaningful graph negatives and35 focused state controls pass. Normal check/replay
dryrun contains only native build/check commands. Detailed split timings, source
hashes, receipts and raw scratch paths are in timings.json, native-validation.json,
audits.json and compiled-source.json. No recursive evidence/dependency archive.

There are796 reconstructed application functions (759 public/37 direct), with16
historical handwritten CRT overrides counted separately. New158extent bytes are
also158 decoded instructions. App-only union228478/281634=81.1259%; support-inclusive
union259081/281634=91.9921%. The partial known target inventory193366/194043 is not a
complete nonCRT denominator. Unaccepted12996 unclassified bytes remain pending,
not CRT. Direct support coverage is not dynamic branch or public CLI coverage.

Ordinary commands from binary root:
  make -j4 native-trace-check
  python3 scripts/check-module-diagnostics.py
  python3 scripts/check-module-diagnostics-controls.py
Canonical original-toolchain build/audits are explicit separate operations.

C1XX3232.EXE (vc1.0 / 1.0-nt)
============================

PARTIAL reconstruction, PAUSED. Last reviewed verified checkpoint: 3c751d03a,
1297 frontiers / 1191 application source functions plus seven historical CRT
source functions. Tagged administrative WIP checkpoint: 17c78f8c6.

The retained accept_preprocessor_token body and old 1298-frontier advancement
were not finally reviewed. binary.c explicitly leaves that function UNIMPLEMENTED
(unaccepted) even though its WIP source executes. The browser fixture checks the
retained unreviewed scanner checkpoint; migration replay is not function
acceptance. Last reviewed boundary is accept_preprocessor_token at RVA0x8140;
current WIP observation reaches scan_preprocessor_token at RVA0x8180.

Build and run
-------------
    make -j4                 # shared native i386 PIC/O0/debug libbinary.so
    make -j4 check           # eight native-only saved-reference cases
    make -j4 unit            # narrow forked native policy/control tests
    make -j4 win32           # explicit original-compiler build, uses shared Wine
    make -j4 audit           # separate static graph/ABI, data and grammar checks

The shared recon-tool.mk discovers numbered cases and owns native/original
object mapping, case builds, replay and unit targets. The shared recon-tool owns
execution, tracing, stream/file checks and cleanup.
Each case.so contains only its adapter/descriptor/tests and links libbinary.so;
application/shim/tracer state is present once per isolated executor. Application
changes rebuild that shared library without relinking unchanged case libraries.
Normal fixtures never invoke Wine or refresh original references. Explicit
original captures use the common tool outside regressions, shared repo prefix,
fixed R:/S:/T: drives. No private prefix or per-case drive mapping is retained.

Native builds use the current shared lib/recon sources directly; no historical
shim snapshots or per-build copies are retained.
RECON_NATIVE_STARTUP_CONTROLFP_53 and binary-default fmode0x8000 are retained.
Original compilation still uses the historical Win32 source and BINMODE.OBJ.

Cases and provenance
--------------------
Six complete frontend cases: cpp_basic, virtual_diamond, copy_assignment,
stack_lifetime, new_delete, state_machine_defined_debug. Each checks exact
streams, status and complete frontend artifacts against the cached original.
state_machine_debug remains a partial case at flush_current_function: its valid
input encounters the unsupported native empty-closed-scope stale-local path.
member_registry_browser is the separate partial WIP checkpoint described above;
capture.bin stores both final six-artifact oracle and partial file/presence assertions.
Its exact visible cwd S:\vc1.0\c1xx\member-registry-browser-driver is real input,
not normalization. Inputs include registry.cpp and registry.h unchanged.

Trace orders are original UNSEEDED encounters after each descriptor's explicit
seen-RVA seed. Those seeds reset for every run; no case depends on an earlier run.
The historical captures did not record full independent order for later cases.
Import/CRT/shim/case-test glue entries are excluded. binary.c is the single
function registry; the audit configuration is generated from that descriptor.
policy-data.json retains target data/grammar/callback and native-limit policies.
runtime.json is the read-only library/member-identification audit input, with no
application implementation credit; binary.c remains the discovered-function registry.

The previous frontier/argument-observation trees were repeated captures of these
eight invocation workloads, not separate public cases. They and the superseded
runners/observer/evidence packages were retired during migration. Historical raw
records remain in tagged commit17c78f8c6; old evidence/ paths in source comments
refer to that history. The inactive registry v1 proposal was rejected because
original model2-to3 member-pointer conversion lost adjustment8 and its runtime
equality check failed. It is not silently promoted to a passing case; v2 avoids
those conversions. Its source/failure is preserved in that tagged history.

The optional tests retain ten native unsupported-path guards, ten original-backed
multiply returns, and small type/name/overload controls. Old point-in-time observer
mutation campaigns and callee-mock acceptance harnesses are superseded; they are
not claims that all historical GDB injections remain fixtures.

Boundaries
----------
Sixteen exact native limitations remain in policy-data.json and source. They
include original unwritten locals (lookahead, empty include/name, null overload
iterators, debug-query/output scope and missing-colon false result), unknown
internal type/member-pointer domains and the unmodeled browser category prefix.
These traps preserve honest unsupported paths; no broad malformed-only or public
reachability claim is implied. The wide string-name branch remains unvalidated.

Seven old runtime.c bodies are retained unchanged by user direction, support-only
credit: setvbuf, sopen, freebuf, fseek, ftell, tzset, isindst. Their necessity for
current cached paths was not established; no additional CRT bodies are authorized.
The older decimal parser uses the approved compatible shipped-library binding,
not a new body or application credit. runtime-compatible.json binds unchanged
original/library/member identities, established ABI/callees and bounded conversion
contract to the historical proof. Later reached mismatches must hold, not relax it.

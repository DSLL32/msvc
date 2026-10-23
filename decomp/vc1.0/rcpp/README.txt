RCPP.EXE, MSVC 1.0 NT / vc1.0
============================

Normal regression work builds and runs native i386 Linux C with -O0 -g.
Source retains original Win32 APIs, style and function ABIs. Shared compatibility
and tracing C from lib/recon is compiled directly at the final link, without an
archive. Include <recon.h> last; that header is empty in the original Win32 build.

    make -j4 native native-trace
    make -j4 check
    make -j4 replay
    make -j4 native-check
    make -j4 native-reject-check

check/replay use native cached application-entry traces, status, streams and
complete output bytes. native-check selects the output-only build. No fixture
invokes Wine, a Windows reconstruction or an original executable. Original
inputs/outputs are immutable references; there is no automatic recapture.
No fields or paths are normalized. Each case receives its full source tree and
captured argv, with fixed R: mapped to the repository. CL/_CL_/LINK/_LINK_/MSC_CMD_FLAGS
are cleared. Every fixture launch uses a deny-all process broker. A real native ELF self-spawn
rejection verifies that unknown dependencies cannot execute. Stale trace metadata
also fails.

There are 19 production cases and one direct option-helper case. They retain the
349,826-byte resources-rich result; pragma/character/float/macro/include and
conditional cases; -P/-C/-J/-Zp/-Ze; CP932 and UTF16; warning/fatal/partial-file
behavior; integer suffixes; and the original 0-to-46-byte interrupt output.
The target's signed high-bit U-expression behavior, empty RC10056 message and
CP932 backslash diagnostics remain exact. The direct helper runs its existing
ten examples plus stack-cleanup/nonvolatile-register check against real linked C.

cases/native-traces.json projects existing original-backed acceptance metadata
onto all 112 application definitions. It excludes identified CRT/import/shim
vertices on both sides. Native objects are checked for missing/new/ambiguous
application definitions. Each case seeds the accepted application prefix and
checks its first-encounter segment and captured terminal status. This is neither
branch coverage nor a record of every repeated call. Cache preparation is an
explicit metadata-only operation, not original reference capture:

    python3 scripts/native-traces.py --write

The native FIFO case verifies a real blocked read with /proc, no readable bytes
and an unflushed zero-byte output, then delivers actual SIGINT with os.kill.
The shared registered-handler mapping reaches rc_control_handler, flushes/closes
streams and exits one; all 46 output bytes and empty streams match the original.
Native delivery uses a same-thread POSIX signal handler. The original Windows
OS-created callback-thread, retained ReadFile-return barrier and exclusive file
sharing observations are unsupported native state checks and are recorded as
such; they are not counted as reproduced Windows state. No callback is invoked
directly by the fixture runner.

Independent native-linked probes retain 40,048 classifier calls and 131,080
message lookups, using cached original stdout/digests from cases/native-abi.json.
Message codes 2045/2412 require distinct addresses despite identical spelling;
two contextual local static objects preserve that existing observable identity
against native compiler literal pooling. Original source graph logic is intact.

CRT boundary and canonical verification
---------------------------------------

The user explicitly permits native substitution inside individually identified
embedded CRT boundaries. Native application graph/ABIs remain strict. The native
entry shim calls rc_main with argc/argv/environ, omitting identified startup.c,
arguments.c, runtime.c and CRT stubs. Their original Win32 implementations remain
in the canonical build. Recovered _sopen/_getbuf/_filbuf/_freebuf still override
shared weak fallbacks and are uninstrumented CRT code. The originating allocator
runtime variant is unidentified; exact STDENVP relocation evidence classifies it
as embedded CRT, rather than an application allocator.

Explicit original-toolchain compilation and static ABI/graph/data diagnostics:

    make -j4 win32 audit
    make coverage
    make capture-original

Wine is used only for these explicit toolchain operations or original-only
capture. scripts/reference.py refuses any executable other than the untouched
original. There is no paired reconstructed-Wine regression fallback. Static PE
indexing, original raw-size/BSS rules, complete CFG call-set and extent-boundary
checks remain available. Named INT3 stubs remain for unaccepted CRT only.

The accepted original extent union remains 35,590 / 40,448 bytes (87.99%), including
CRT/imports/shared text and with padding in the denominator. It is not native-only
or branch coverage. All identified application/support bodies are accepted; the
remaining identified bytes are 2,287 CRT + 54 CRT-used import thunks, plus 2,517
alignment/zero-fill bytes. No new reconstruction was done for native adoption.

Layout and evidence
-------------------

src/ contains readable reconstruction C/headers and native probe C. scripts/
contains native checks and explicit original-toolchain/capture/static analysis.
Make runs scripts from this binary root; cases/ holds new native metadata and
receipts, build/ holds generated objects, and ~/.ai-tmp/vc1.0-rcpp holds scratch.
Explicit toolchain/capture operations use ~/.ai-tmp/vc1.0-capture/wine only.
Fixed R: maps the repo; S: maps ~/.ai-tmp/msvc2. RCPP captures are isolated
under S:\vc1.0\rcpp\<case> with an RCPP capture lease. Never remap drives
or stop the shared server. Native fixtures have no Wine dependency.

cases/native-all-validation.json summarizes adoption, build/setup/execution/check
times, exact results, rejections and unsupported state observations. Native trace
and output-only receipts are separate; normal native checks have zero capture time. Explicit shared-prefix migration
capture timing is recorded separately. Historical original-backed reconstruction receipts and
frontier acceptance order remain available in their existing metadata.

After isolating shared tracer startup storage from the application allocator,
ten fresh traced resources-rich runs passed with exact entries and output. No
application initialization or trace-filter workaround was added.

Shared-prefix maintenance
-------------------------

All 19 production references were explicitly captured twice into
cases/shared-prefix/<case>; ordinary cases include fresh original entry-trap
proofs, and the console case includes its real OS callback proof. Only the rich
output changed (fixed R: header paths). No raw historical reference was relabelled.
The shared resources-rich fixture remains untouched for RC. Old RCPP-only raw
fixtures and independent semantic analyses remain historical evidence, outside
the active registrations. cases/shared-prefix/migration-validation.json records
the inventory, exact checks, split timings and retirement of four unused private
prefixes. No application code, frontier or coverage changed.

Host graph audit
----------------

    make graph-audit
    make audit-static

These targets use existing canonical build/rcpp.exe and build/rcpp.map, without
Wine, compiling the reconstruction or recapturing references. The shared Rust
helper builds quietly once, then its binary runs directly with diagnostics intact.
Consolidated CFG discovery checks all 161 accepted non-import functions, RET
cleanup, indexed function references, disjoint backed ownership and whole extent
tails. Accepted-index checks cover all 179 original and 179 rebuilt extents,
including 18 import thunks per side. A narrow Python residual compares all
startup calls, including detached SEH blocks, with the Rust graph. Contextual
data, shipped-library provenance and classifier/message/ABI probes remain Python
checks. scripts/audit-graph.py no longer contains an application CFG walker.

cases/rust-graph-audit-validation.json records the host-tool flags, final
161-function/358-extent positive, two deliberate graph/extent rejections,
remaining startup-only Python scope and native-suite/ABI timings. This migration
used existing canonical artifacts and invoked no Wine build or reference capture.

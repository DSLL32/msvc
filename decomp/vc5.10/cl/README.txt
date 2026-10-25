VC5 CL: 112/112 identified application functions implemented; 18,545/18,545
application extent bytes. Runtime/import support adds no application credit.
No named application traps remain. This measures extents, not branches or input
domains; runtime/import support and the 41 padding bytes are separate.

make -j4 check unit        cached native IO cases and optional narrow tests
make -j4 recon-original    original compiler build
make -j4 audit             shared runtime identification and Rust graph/ABI audit

The 14 public IO cases preserve existing captured invocations and check native
first-entry order, full DWORD status, streams and files. Case 0014 is an internal
DIRECT test of deferred_from_jobs on a genuine source_jobs-created two-Job chain
at prepare_jobs entry: NULL, tail and head, initialized links, owner aliases and
preserved inputs/ABI. Its 32-byte expected record comes from the masked original.
This establishes internal behavior, not public MRE reachability. A separate warm
two-source /Gm /Zi /c original run completed without reaching that helper.
No frontend/backend dependency is continued after the DIRECT marker. There are
no additional unit callbacks. Case 0015 additionally checks the internal pass
configuration callback on genuine driver state and App-allocated Option records,
including disabled and unknown selectors; this does not invent public bc syntax.

Typed C1/C2, LINK and six-call MRE providers validate complete known requests
before applying captured dependency effects; unknown or missing calls fail.
Provider setup runs after the execution checkpoint. Fixtures never invoke Wine.
Case 0016 exercises console_rows and a separate identical public CONOUT$ open
under the actual inherited headless association: fallback 25 and LastError 6,
with explicit canonical CRT error inputs 111/222/333. Console-open success,
CSBI/CloseHandle and public help pagination are unvisited. Case 0017 calls the
inactive control handler on real initialized driver state, checking actual
cleanup entry order and full DWORD status 4. Its original nonreturn capture
proves the typed callback entry before accepting the target exit. It does not
claim delivered console-event equivalence, post-exit globals or active DLL abort.
The old failed AllocConsole analysis remains preserved in scratch/history.

Application code stays in src/. Native adapters, provider callbacks and import
checks live in harness/; the strict CL contract helper remains in scripts/.
No binary-specific CRT source is compiled: the original build uses shipped CRT
libraries, and native compatibility comes from lib/recon/.

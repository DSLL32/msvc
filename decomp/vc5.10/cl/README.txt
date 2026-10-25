VC5 CL: 109/112 identified application functions implemented; 18,279/18,545
application extent bytes. Runtime/import support adds no application credit.
Three named traps remain. This measures extents, not branches or input domains.

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
no additional unit callbacks.

Typed C1/C2, LINK and six-call MRE providers validate complete known requests
before applying captured dependency effects; unknown or missing calls fail.
Provider setup runs after the execution checkpoint. Fixtures never invoke Wine.
The original console research and failed AllocConsole analysis remain in scratch
and Git history; console_rows remains a named trap.

Application code stays in src/. Native adapters, provider callbacks and import
checks live in harness/; the strict CL contract helper remains in scripts/.
No binary-specific CRT source is compiled: the original build uses shipped CRT
libraries, and native compatibility comes from lib/recon/.

VC5 CL: 108/112 identified application functions implemented; 18,238/18,545
application extent bytes. Runtime/import support adds no application credit.
Four named traps remain. This measures extents, not branches or input domains.

make -j4 check unit        cached native IO cases and optional narrow tests
make -j4 recon-original    original compiler build
make -j4 audit             shared runtime identification and Rust graph/ABI audit

The 14 cases preserve existing captured invocations. Migration added no original
capture or function acceptance. IO replays check native first-entry order, full
DWORD status, streams and files. There are no additional unit callbacks.

Typed C1/C2, LINK and six-call MRE providers validate complete known requests
before applying captured dependency effects; unknown or missing calls fail.
Provider setup runs after the execution checkpoint. Fixtures never invoke Wine.
The original console research and failed AllocConsole analysis remain in scratch
and Git history; console_rows remains a named trap.

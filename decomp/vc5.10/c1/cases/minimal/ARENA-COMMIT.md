# Arena commit and complete default pool sequence

`CompilerArena::CommitExtent` owns87 hot bytes9883..98da and30 cold bytes
2b898..2b8b6. It checks the signed used/reserved extent, rounds the required
commit to the descriptor's unit, calls actual VirtualAlloc(MEM_COMMIT), and
updates committed_limit and committed_bytes. The two fatal branches retain
their original calls with codes76/60; neither is exercised here.

Both fresh pairs reach the genuine command-line parser12b49(argc11,sameargv).
The proof records all nine pool bindings in order8,7,0,1,2,3,4,5,6; all nine
Refill and AllocatePages calls/returns; and the sole CommitExtent call/return.
The native58-event sequence and28 returns are actual GDB observations. Every
receiver, argument, formal inventory, caller, return pointer, cleanup, callee-saved
register set and whole modeled state is checked against the original chronology.
The original initializer and six-arena prefix remain independently validated.

The actual provider and arguments identify a64KiB in-range private commit.
VirtualQueryEx proves the writable committed prefix and inaccessible reserved
tail; native mappings and retained allocation ownership match. Complete64KiB
freshly zeroed bytes match after the API and at the final parser frontier. Later
pool requests reuse that commit. The final cursor advances9 pages, each pool
receives its own returned slice, and no reserved-tail bytes are read or claimed.
Initialized state remains64 known/6484 UNMODELED bytes.

The original-only helper adds14 exact observation points inside accepted source,
verifies their relocated bytes/ownership and reinserts each trap by single-step.
Both original runs reached the parser directly. The first native debugger attempt
was rejected because the older work-buffer API observer allowed only its prior
callers. An exact handoff to the new commit observer fixed the controller; the
target/rebuilt DLL/ELF and both original captures stayed unchanged. The failed
native diagnostic is retained without an acceptance receipt.

Strict graph/ABI/ownership checks pass68 source functions/3580 code bytes plus16
switch-data bytes. Application accounting is66 functions/3117 code bytes. Both
paired positives,36 fresh affected guards and18 refreshed protocol/staleness
guards pass. Earlier unchanged semantic campaigns are explicitly reused with
their frozen hashes. Original/native builds took1.861/1.638s, audit0.033s,
original/native captures4.126/10.226s normal and3.121/11.618s restore. Affected
guards took16.765s; core-guard elapsed time was not separately measured. Native
lifecycle fixtures pass in4.11ms excluding build. Ordinary frozen proof files
and retained external identities are in `evidence/arena-commit-*`.

Minimal compilation continues at command-line parsing; it has not completed.

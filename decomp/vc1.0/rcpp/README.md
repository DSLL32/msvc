RCPP.EXE reconstruction for MSVC 1.0 NT. This finished application is paused;
remaining CRT code is outside reconstruction scope.

Run from this directory:

```
make -j4 check unit
make -j4 win32
make audit
```

The shared recon-tool rules build one i386 `-O0 -g` PIC `.build/libbinary.so` and
20 dynamically linked case libraries. Native fixtures never invoke Wine or
refresh references. Cases preserve 19 original public invocations and one direct
option-helper invocation, including exact input bytes, exit status, streams,
complete artifacts and independently seeded original application-entry orders.
`binary.c` is the sole function registry; all 112 application bodies and their
owned text extents are unchanged by migration. CRT/import entries get no App
implementation credit. Seven unavailable CRT bodies retain their known identities
without invented code extents. Historical accepted extent union remains
35,590 / 40,448 bytes (87.99%); that includes CRT/imports and is not branch coverage.

The direct helper retains ten examples and stack/nonvolatile ABI checks. Its two
forked unit tests retain 40,048 classifier calls and 131,080 message lookups,
including repeated-pointer and distinct-label address behavior. The full frozen
original message stream is an input in that case's capture and is compared byte
for byte; no generated message implementation or local hashing framework is used.

The console case creates a private FIFO during its IO adapter. A bounded native
controller observes the actual blocked read syscall, exact FIFO descriptor/path,
and zero-byte result before sending real SIGINT. The shared supervisor reaps the
controller and cleans the workspace. This reproduces exit 1, empty streams and
the 46-byte flushed prefix. Original Windows OS-created callback-thread,
ReadFile-return barrier and exclusive-sharing state remain unsupported native
observations; the case does not claim otherwise.

Original compilation uses recon-tool's shared 1.0-nt launcher. Canonical graph
and ABI checks use the shared Rust auditor; explicit startup SEH roots are finite
policy metadata, with rebuilt addresses resolved from the current MAP. Runtime
identification uses the shared read-only identifier. RCPP retains its user-approved
qualification: individually identified embedded CRT boundaries may substitute
compatible native runtime behavior despite differing internal graphs; the exact
originating older runtime is unknown. No new CRT reconstruction was done.

`src/` contains application reconstruction. The existing binary-specific runtime
support lives in `crt/`: `startup.c` and `arguments.c` preserve the embedded
startup/argument contracts; `runtime.c` preserves its process-heap allocator;
`file-runtime.c` preserves its file-open flags and FILE/buffer behavior.
`legacy-ctype.c` and its header retain the observed older CRT classification
prefix used by signed character indices. `stubs.c` holds the two unavailable CRT
frontiers, with no new implementation credit; `rcpp.h` declares these runtime
interfaces. Compatible common runtime support remains in `lib/recon/`.
Case-specific console and direct/unit glue remains in its numbered case; there
are no separate binary-specific native hosts or shared case helpers.

Migration snapshot, authoring files and diagnostics are under
`~/.ai-tmp/msvc2/vc1.0/rcpp/standardization/`; case directories contain only
standard descriptors, captures and narrow direct/unit glue. Superseded receipts,
runners and historical evidence remain available in Git history and the snapshot.

The shared runner isolates Rust host allocations with prefixed jemalloc while
leaving target C malloc/free behavior unchanged. This preserves the frozen rich
case entry order despite its use of untouched type fields; no oracle, seed or
application initialization was changed. All 20 replay invocations and both
nonempty unit groups pass. The shared audit retains the exact historical 179
accepted entry names and ranges on both sides, including 18 import endpoints.

Layout verification: native/case build 2.129 s; parallel replay/unit targets
0.328 s; original compiler build 8.742 s; shared runtime identification,
registry export and 161-graph/358-extent audit together 0.400 s. These checks used
the existing captures. Diagnostic receipts remain in scratch.

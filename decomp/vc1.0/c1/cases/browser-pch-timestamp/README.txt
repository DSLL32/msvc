Browser-bearing explicit PCH and source timestamps
=================================================

One producer/consumer workflow compiles /FR /Z7 /Ycapi.h and /Yuapi.h,
using a packed record API, callback types, nested argument/token/string macros
and __TIMESTAMP__ strings. The untouched compiler/linker and checksum program
succeed. Source/header mtimes are fixed at1768480496000000000ns; original
no-TZ CRT defaults yield Thu Jan15 04:34:56 2026. The initial UTC expectation
failed only the runtime checksum and is retained as draft1; corrected draft2
validates the actual legacy timezone contract.

Two complete original CL child transports and all before/after files are
frozen. All raw IR/browser/PCH outputs agree exactly across repeats. No
normalization applies. The native consumer must use actual native producer
PCH and browser files; a frozen original C2 object is exposed only after the
entire producer output contract passes. Fixtures never run Wine or backends.

Complete at953 accepted entries: four application functions196bytes plus
_fstat/import662bytes. Full34-phase native trace/output/dependency suite,
32 rejections, six graph/ABI/mask tests, three flag mutations and289 state/
alias checks pass. validation.json binds raw first-entry histories, metadata
return observations, shared CRT provenance and separate measured timings.

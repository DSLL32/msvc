# Borland C++ 4.0 BCC32

The no-argument case is complete: the original and native executable return 0,
with identical 1,458-byte stdout and empty stderr. The original uses a quick
exit after printing usage; its 512-byte stream and 80-byte formatting buffers
leave the final help text buffered, so the observed output ends mid-line. The
case retains that behavior and compares against a cached original capture.

Reconstructed application bodies are the exception-handler initializer,
`main`, the encountered message lookup, banner printer and usage path. Only
message IDs 25, 26 and 41 are defined; other IDs trap. Other unreconstructed
application callees remain named traps. Borland SIGINT delivery, DOS date/time
and exception dispatch currently fail explicitly in the native support layer.

Build with `make -j4`, `make -j4 native-bin`, or `make -j4 recon-original`.
The period build compiles with BCC32 and links separately with the shared
TLINK32 compatibility copy. `make -j4 check` is native-only; `make -j4 audit`
also builds the period image and checks the function graph, references,
returns and extent ownership.

The no-argument fixture's completed native run took 5.444 ms: 3.056 ms setup,
1.669 ms execution and 0.718 ms checks. The final original capture used for
comparison and raw artifact exports are retained in the iteration scratch
journal, outside this project.

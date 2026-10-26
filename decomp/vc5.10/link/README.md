# VC5.10 LINK

304 application functions / 96,662 original code bytes are reconstructed
(23.30% of the 414,810-byte whole `.text` extent; this denominator includes CRT
and text data). Thirteen shipped CRT/import boundaries add no application
credit. `binary.c` is the sole function and extent registry.

The saved `/help` and Win32 hello cases are complete. Hello links the original
`/O2 /G5` window object with the shipped kernel32 library, exits with status 0,
and produces the matching 24,064-byte executable. Its fixture checks the full
application entry order, streams and artifact inventory. Only the identified
COFF `TimeDateStamp` DWORD at file offset `0xb8` is normalized. The final fresh
original retained the artifact's full SHA; replacing only that DWORD in the
native output reproduced it. This is a derived hash comparison, not a claim
that the latest original artifact bytes were retained.

`make -j4 check unit` runs all four cached native cases, including 115 narrow
hello tests. No fixture invokes Wine. The final hello replay took 26.5 ms:
11.8 ms setup, 8.1 ms execution and 6.6 ms checks. The parallel regression/unit
batch took 0.40 s. The final parallel native/original compiler build took
10.51 s. These are measured closeout runs, not clean-build timings.

`make -j4 win32 audit` builds with the original compiler and checks the full
317-vertex graph, imports, references, ownership and extents. Return-cleanup
checks cover 302 ordinary application bodies and six CRT bodies; the seven
import thunks remain in the graph. The two genuine C++ lifetime owners have
separate normal-return, cleanup-action and qualified frame-handler checks.
The full static closeout took 1.41 s. `/OPT:NOICF` keeps distinct pending App
trap stubs from being merged by the reconstructed Win32 link.

The `0002-symbol-buffer-dtor` and `0003-definition-buffer-cleanup` DIRECT cases
qualify the authentic 12-byte and 20-byte owners with NULL state and genuine
startup allocations. The latter also checks `Set` with live spans and a NULL
callback. They establish internal lifecycle behavior, not ordinary CLI reach.
Their selected cleanup bodies remain real traced App functions. Only the two
compiler lifetime TUs and the canonical buffer instantiation use the justified
optimization/instrumentation settings; Win32 retains its actual EH actions.

Hello exercises the ordinary x86, nonincremental, no-debug path. Optional PDB,
resource, architecture and callback domains retain named traps or native guards
where unqualified. The diagnostic `current_phase` owner supports the observed
36-byte prefix with guards before copies and concatenations; its original total
capacity is unknown. The earlier 32 bytes were only a modeled prefix. Native
qualification guards use the compiler's actual `__linux__` macro; this correction
does not establish wider PDB support.

Application bodies and original constructor registration are in `src/`.
`harness/startup.c` owns native setup and invocation, including the build-time
`main` → `link_main` alias. `crt/crt-imports.h` declares the shipped `_iob` data
import instead of the newer SDK accessor; it implements no CRT routine.
Root policy files contain only live finite audit contracts.

The workload uses stable `T:\vc5.10\link\0001-hello-win32`. Older path-dependent
references and pre-migration research remain historical scratch/Git evidence.
The completed EXE case is paused for review before the approved DLL workload.

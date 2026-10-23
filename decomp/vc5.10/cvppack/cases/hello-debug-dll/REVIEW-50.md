# CVPACK 5.10: 50-function review

The genuine embedded-CodeView DLL case is in progress. `/help` remains complete.
The current matching boundary is `initialize_module_type_storage` (no arguments).
No reconstructed path has yet entered MSPDB; the approved typed case provider
remains outside application coverage.

- Source: `src/pack.c`, `publics.c`, `legacy-types.c`, `primitive-types.c`,
  `module-types.c`, with recovered layouts in `types.h`, `image.h`, `pack.h`.
  `__fastcall` expresses the actual ECX type-index argument; no dummy transport
  arguments or implementation assembly were added. `widen.h` retains the genuine
  reviewed C++ interface for the future wrapper bodies.
- Graph: candidate-inclusive Rust CFG/RET/reference/ownership/extent checks pass.
  `scripts/switch-data.py` independently proves the rebuilt dispatcher's exact
  39-byte switch data using COFF relocations, linked bytes and the original class
  map. Other encountered original switch data belongs to its owning functions.
- Data: 99 object identities, 23 completed reads, two initialized module payloads,
  four complete public nodes and their bucket/arena aliases match. Only defined
  prefixes are compared; unused allocation tails retain ownership/capacity checks.
  Public padding is initialized and compared. The type page's four-byte signature
  is checked against its actual allocator and segment-table alias.
- Proof: `evidence/0050-read-new-module-types/` holds fresh original/native raw;
  `checkpoint-static/` and `checkpoint-revalidation.json` revalidate that same raw
  after the historical source-status adapter fix. Original observations were not
  relabeled or rerun for static-tool changes.
- Support: 50 application functions / 13,832 code-extent bytes; 353 associated
  switch-data bytes. Denominator 62,662 includes CRT, unknown code and padding:
  22.07% accepted extent, not non-CRT or branch coverage. Four CRT bodies/400 bytes
  and four import thunks/24 bytes remain separate.
- Checks: `make -j4 native check audit-native final-check`; eleven additional
  native-only suites recorded in `checkpoint50-controls.json` (131 semantic
  negatives, 517 primitive inputs, positive 21-object provider sequence), plus
  24 help-controller negatives. Fresh batch 2.81 s, supplemental padding check
  0.94 s, help final capture/audit/pair/negatives 0.11/0.25/0.12/1.11 s.
  Help fixture 13.6 ms; partial debug fixture about 0.72 s including GDB. Latest
  original/native build 3.26/1.97 s; original/native capture 2.32/0.51 s and
  pair comparison 0.29 s. Native checks never invoke Wine.

Next: initialize the type-record table, then validate actual streamed records and
advance to the already reviewed opaque Widen boundary when reached. Keep its
strict per-instance inputs, sequences, full capacities, native-owned tails and
actual thiscall ABI. The unobserved Record slot remains a fail-closed stub.

The import-review correction is in `import-audit-review/0003-qualified-stable/`.
All 50 functions now pass qualified direct and register-cached import checks,
including EBP. The original direct `_iob` data import is preserved instead of
adding the SDK accessor. Fourteen reviewed ordinary-return contracts contain
exactly twenty live register facts; removing each contract or each fact is
rejected. These are explicit provider/SDK ABI assumptions, not reconstruction
of CRT internals. Main also requires all five exact cached `free` sites and only
the genuine GetProcAddress-derived checksum callback as unresolved. The rebuilt
callback has an independent stack-local instruction witness.

Fresh native-only raw is
`evidence/0050-read-new-module-types/native-qualified-imports-0002/`.
The original target, mask, candidate, configuration and execution inputs remain
unchanged. Its historical audit/metadata are checked against their original
receipt; a strict current sidecar separately binds the current PE/MAP/ELF/tool,
regenerated COFF/import metadata and both raw receipts. Twelve stale/swapped/
forged-sidecar controls pass with their exact intended rejection reasons,
including refreshed listed hashes and omitted cached sites. No original was
rerun. An earlier parallel-build race remains explicitly superseded in `0002`;
the stable checks used a single build followed by validators without relinking.
Incremental build took 0.027 s, native capture 0.418 s,
strict 50-function audit 0.457 s, all 34 removal controls 0.661 s,
and twelve sidecar controls 1.741 s. Native help/debug fixtures took
13.0 ms/0.620 s (the latter includes GDB).
The 24 help controller checks took 1.348 s. Exact hashes,
commands, frozen executables and fresh/reused accounting are in `review.json`.

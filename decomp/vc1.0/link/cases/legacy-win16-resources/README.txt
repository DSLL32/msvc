Legacy Win16 .res conversion

The382-byte generated Win16 input contains a monochrome DIB, popup menu,
font-bearing dialog with ordinal menu/class name branches,16counted strings,
two accelerators, RCDATA and a named custom resource. Its outer Win16 records
are decoded from the target's read_resource_name/convert_resource_file; Microsoft's
16-bit classic dialog layout is the primary format reference linked in the
source generator. Input generation is fixture data, never implementation code.
The accompanying286-byte test.obj is copied unchanged from the original-CL
minimal-C fixture; /entry:main /nodefaultlib /subsystem:console avoids CRT inputs.

Two untouched originals succeed. Complete2MBlegacy.exe and legacy.map repeat
identically after only PE/resource-directory timestamps and their map rendering.
The pure artifacts-resources.py parser walks the PE resource-directory tree,
checks bounds and identifies those fields structurally. All other bytes,
including padding, resource ordering, cursor alignments and paths, are exact.
Full expected files and original statuses/streams are frozen before acceptance.

Ten whole native functions are now readable C in src/resource-legacy.c. A typed
16-entry table recovers the original converter/debug callback graph. Debug-only
printers remain named INT3 stubs; they were not entered or accepted. Typed packed
Win16/Win32 headers express13/14/18-byte layouts with compile-time checks.
Byte-copy and Unicode/name helpers preserve their native cursor rules, including
the unusual fixed-count Unicode path that advances output by count bytes even
though it writes WCHARs. That path is statically reconstructed; this fixture
uses the zero-count terminated-string helper path. No replacement semantics.

Every newly accepted function has fresh parallel original masked/native GDB
entry and physical partial-stream evidence. Final fully masked/restored original
exits0 without another trap. Fresh original entry capture records all10 names.
The native full executable, map, streams/status and first entries match.
All38native cases pass622application entries in2.380s external, one sample.
Canonical+native builds/audits take27.280s as one unsplit make-j4 invocation;
earlier frontier builds are unmeasured.649graphs/804original801rebuilt extent
records,639callback sets,454data definitions and41ECX cleanup checks pass.

New application CFG bodies contain1979instruction bytes and101alignment bytes,
without text data. No CRT/import acceptance or application credit is added.
Coverage228981/281634=81.3045%,798frontiers is an extent measure, not branch
coverage. validation.json preserves timing components and raw capture locations.
The approved ordinary OMF LIDATA/EXESTR family follows without an idle pause.

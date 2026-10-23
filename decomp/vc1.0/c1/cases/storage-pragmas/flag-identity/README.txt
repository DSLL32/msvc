Flag identity correction after storage checkpoint 91c249f3

The old output-matching receipts remain unchanged. They did not prove the
internal word identity: alloc_text dispatch and its new callee both used the
parser word for a scanner flag. The new declaration/runtime-initializer paths
also selected that wrong word. The corrected scanner bits are 0x20000,0x80000
and 0x40 respectively. Raw BYTE[scanner+2]&8 additionally exposed six older
0x08000000 literals in five declaration/tag routines; the correct bit is 0x80000.

all-access-audit.json inventories 231 original and 218 canonical accesses in 89
functions. Their normalized tested/cleared/set/copied mask unions match.
The differing instruction counts come from compiler loads/stores and shared
exits. Original operands identify scanner 0x6bb44, parser 0x6bb48, upper WORDs
0x6bb46/0x6bb4a independently of source annotations. Extra accepted shared
text and CRT/import extents are screened too. The PCH writer passes live
block 0x6b630,length 0x51c to fwrite; its reader passes the same block/length
to fread. The two final DWORD offsets 1300/1304 correspond to the separately
asserted portable PchCompilerImage fields.

For all nine affected application functions, original.events holds debugger
observations before the first instruction and at the caller return. The
native debugger reads separately named scanner_flags/parser_flags variables.
Both full 32-bit words match at every recorded boundary, including examples
where only scanner is nonzero and others where only parser is nonzero.
The debugger preserves instructions/registers and rejects recursive probes
or zero observations. It does not write the observed globals. These are
explicit fresh original captures outside native fixture execution.

Three private PE mutations are rejected: swapped word, old erroneous tag
mask and incorrect upper-half offset. All 26 completed native phases retain
exact traces/status/streams/artifacts and the 11 existing oracle negatives
still reject. validation.json binds the evidence and its exact source hashes.
The four diagnostic functions already added before this correction remain
under an unfinished case; this repair adds no function credit.

Original-toolchain __based and vbtable names

The immutable names.obj was compiled from sources/names.cpp by the untouched
VC1.0 compiler with /nologo /c /Od /G3 /MT /Fonames.obj names.cpp. Two explicit
original-only LINK -dump /symbols /undecorate captures agree exactly:2098 stdout
bytes, empty stderr, exit0. No output normalization. The source/compiler/object
hashes and environment are in manifest.json and capture/*.json.

The native original spelling deliberately says __based(NYI:*) rather than
inventing modern formatting. The object contains actual ??_8Derived vbtable
symbols. Fresh masked-original/native GDB frontiers reached parse_based and
parse_vbtable_type, and the terminal pair matches the entire frozen streams.
A first mask omitted the based parser's9-entry36-byte inline switch table and
caused an original AV. That run is rejected; the full table was restored and a
new matching vbtable-entry-fixed pair preceded acceptance.

capture/frontiers preserves compact raw GDB/state evidence, including the failed
mask probe. validation.json references full scratch build/helper logs and their
hashes. Native cached regressions invoke no original executable or Wine.

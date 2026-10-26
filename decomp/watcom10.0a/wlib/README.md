# Watcom 10.0a WLIB

Seven cases are complete: basic help, archive update, in-place C++ update,
PE DLL/COMMON import, NE/NLM export conversion with C++ listing, and unmatched
member diagnostics, plus nested-response syntax termination. They cover replacement/deletion/addition/extraction,
explicit and executable-derived imports, dictionary construction, alphabetical
and module listings, COMDAT/local-public records, C++ demangling and backup
preservation. The NE input exercises resident names and nonresident ordinals;
the NLM input has a genuine count-based public table. Their combined C++ object
adds operator new/assignment/relational names, integer templates, recursive local
names, a based pointer and an anonymous enum. Six exit 0 and the syntax-error case exits 1; all match streams, final files
and application entry order. The last five cases observe 166, 200, 172, 209 and 198 first entries respectively.
Case-sensitive missing deletion/extraction diagnostics retain counted names and
produce the original warnings while successful extraction still completes.

The reconstruction has 241 qualified application functions covering 28,568 unique
original code bytes. Shared chunks count once. The empty CommitDictionaryBlock
entry shares another function's RET and is explicitly excluded from entry order.
There is no verified complete non-CRT denominator yet. All 241 bodies retain
function-graph/ownership/extent checks; 234 have strict RET checks and six
normal-return owners with cold nonlocal exits retain the finite residual in
scripts/. ReportSyntaxError has a separate exact direct-exit residual; its final
period-compiler CALL exit has an explicit terminating-call graph declaration. AbortCommand and other pending functions remain named traps.

`make -j4 native cases check unit audit win32` builds the native i386 DSO,
runs cached fixtures and checks the source with the original Watcom compiler.
Fixtures never invoke Wine or recapture. Source preserves the 1994 graph/Watcom
ABI; native GCC uses the authorized semantic projection through lib/recon.
Initial OpenWatcom commit 6b9cb443 supplied selected resource/demangler source
and executable-header types; their notices remain in source. Identified CRT
providers have zero application reconstruction credit.

The COMMON input exposes the original extended-size decoder defect: after the
0x81 marker it leaves the final size byte to be read as the next name length.
That desynchronization is retained; 0x84/0x88 advancement is also preserved from
the actual instructions, without claiming their standalone dynamic reach.
The native listing uses the identified 1994 Watcom qsort variant. Backup rename
behavior is qualified for closed regular files and absent destinations only.

The pristine image is unchanged. Original capture uses the separately patched
GS resource-reader image, SHA256
0927524a7d8f90207996257ecd32e09670016917284de5203b0015d439dda92e.
Reached resource writers retain their proved 128-byte/flat-DS owner conditions.
Native self-resource input reproduces the exact frozen masked-image recipe.
Executable conversion uses an independently unique internal native temporary;
progress checks select meaningful outputs and normal exit checks full inventory,
including removal of the temporary. No PID or filename normalization is used.

The syntax-error pair took 1304.167/12.659 ms original/native, with native setup
7.900 ms, execution 2.099 ms and checks 2.660 ms. Its final canonical cached check
took 10.667 ms. All seven cached cases and unit checks passed in 105.359 ms wall.
The default parallel native/period rebuild took 2436.989 ms. Canonical final
full audit, including its incremental native build, took 1273.550 ms; the frozen
full static qualification took 482.930 ms. Diagnostics and input producers stay
in scratch; each case contains case.c and capture.bin only. Console/nonlocal
errors, alternate resource formats and a few demangler/listing paths remain.

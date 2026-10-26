# Watcom 10.0a WLIB

Four cases are complete: basic help, archive update, in-place C++ update and
PE DLL/COMMON import. They cover replacement/deletion/addition/extraction,
explicit and executable-derived imports, dictionary construction, alphabetical
and module listings, COMDAT/local-public records, C++ demangling and backup
preservation. All exit 0 with identical streams, final files and application entry
order. The last three cases observe 166, 200 and 172 first entries respectively.

The reconstruction has 227 qualified application functions covering 27,151 unique
original code bytes. Shared chunks count once. The empty CommitDictionaryBlock
entry shares another function's RET and is explicitly excluded from entry order.
There is no verified complete non-CRT denominator yet. All 227 bodies retain
function-graph/ownership/extent checks; 221 have strict RET checks and six
normal-return owners with cold nonlocal exits retain the finite residual in
scripts/. AbortCommand and other pending functions remain named traps.

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

The final DLL/COMMON pair took 1412.100/10.981 ms original/native, with native
setup 5.893 ms, execution 2.645 ms and checks 2.442 ms. The final cached replay took
14.090 ms. Its final parallel native/period build took 1414.366 ms and full audit
310.588 ms. All four cached cases and unit checks pass. Diagnostics and input
producers stay in scratch; each case contains case.c and capture.bin only.
Console/nonlocal errors, alternate executable/resource formats and further
C++ demangler paths remain to be reconstructed.

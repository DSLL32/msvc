# Watcom 10.0a WLIB

Three cases are complete: basic help, archive update and in-place C++ update.
The archive update uses response/environment options to replace, delete, extract and add OMF
members, create an import member, strip lines, trim headers, rebuild the
library dictionary, and produce alphabetical and module listings. All three cases
exit 0 with identical streams, artifacts and application entry order.
The C++ case replaces the library in place, preserves its exact backup, extracts
a named member, processes COMDAT/local-public records, and lists more than 200
publics through constructors, operators, templates and other demangler paths.
Genuine OMF COMMON coverage remains deferred.

The reconstruction has 213 application functions covering 24,667 unique original
code bytes. The archive and C++ cases observe 166 and 200 first entries respectively. The empty
`CommitDictionaryBlock` shares another function's RET; its entry is explicitly
seeded on both sides, excluded from entry ordering and adds no coverage bytes.
There is no verified complete non-CRT denominator yet.

`make -j4 native cases check unit audit win32` builds the native i386 DSO,
runs cached native fixtures, audits application graph/ABIs and checks the source
with the original Watcom compiler. Fixtures never invoke Wine or recapture.
All 213 bodies retain graph/ownership/extent checks; 207 have strict RET checks.
Six normal-return owners with cold nonlocal exits use the explicit residual in
`scripts/`, while the unreconstructed AbortCommand remains masked.

Source follows the 1994 graph and period Watcom ABI. Native GCC uses the
authorized semantic ABI projection through `lib/recon/`. Initial OpenWatcom
commit 6b9cb443 supplied resource and demangler source; its notices are retained
in those files. Identified CRT spans have zero application reconstruction credit.
Full 74-byte `__CMain`, 257-byte `_IsTable` and actual shared tails stay explicit.

The pristine image is unchanged. Original capture uses the separately patched
GS resource-reader image (SHA256 0927524a7d8f90207996257ecd32e09670016917284de5203b0015d439dda92e).
Usage and the reached message formatter each supply a proved 128-byte stack
buffer and DS-derived selector. Other owners gain no segmented-pointer waiver.
Native self-resource input reproduces the exact frozen masked-image recipe.
Console handling, nonlocal errors and additional object/demangler paths remain
to be reconstructed. Native listing uses the identified 1994 Watcom qsort
variant, preserving the original order of equal symbol names. Backup updates
currently qualify closed regular files and absent rename destinations only.

The final archive original/native pair took 1306.544/13.635 ms; native setup,
execution and checking took 7.903/3.183/2.550 ms. Its parallel incremental
native/period build took 1094.351 ms. Final cached native replay took 8.998 ms.
All diagnostic reports remain in scratch; cases contain source and capture.bin.

The C++ final original/native pair took 1418.458/22.565 ms; native setup,
execution and checking took 6.835/12.528/3.202 ms. Cached native replay took
19.996 ms. Its incremental registry build took 147.011 ms; the preceding
changed-source native/period build took 1855.691 ms. All three cached cases,
case unit checks, the complete graph/ownership/extent audit, strict RET checks
and six nonlocal residuals pass, as does the period compiler build.

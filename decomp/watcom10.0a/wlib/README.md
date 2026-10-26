# Watcom 10.0a WLIB

Two cases are complete: basic help and a substantial archive update. The latter
uses response/environment options to replace, delete, extract and add OMF
members, create an import member, strip lines, trim headers, rebuild the
library dictionary, and produce alphabetical and module listings. Both cases
exit 0 with identical streams, artifacts and application entry order.

The reconstruction has 171 application functions covering 19,558 unique original
code bytes. The archive case observes 166 first entries. The empty
`CommitDictionaryBlock` shares another function's RET; its entry is explicitly
seeded on both sides, excluded from entry ordering and adds no coverage bytes.
There is no verified complete non-CRT denominator yet.

`make -j4 native cases check unit audit win32` builds the native i386 DSO,
runs cached native fixtures, audits application graph/ABIs and checks the source
with the original Watcom compiler. Fixtures never invoke Wine or recapture.
All 171 bodies retain graph/ownership/extent checks; 165 have strict RET checks.
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
Console handling, nonlocal errors, in-place backup updates and additional
object/demangler paths remain to be reconstructed.

The final archive original/native pair took 1306.544/13.635 ms; native setup,
execution and checking took 7.903/3.183/2.550 ms. Its parallel incremental
native/period build took 1094.351 ms. Final cached native replay took 8.998 ms.
All diagnostic reports remain in scratch; cases contain source and capture.bin.

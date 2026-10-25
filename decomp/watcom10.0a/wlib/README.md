# Watcom 10.0a WLIB

The basic help case is complete: bare `?` with an explicit regular stdin file
containing 64 LF bytes. Both binaries exit 0 with identical help output and the
same 56 application first entries. Reconstructed App code totals 4,768 bytes.
Reconstruction is paused after this one case.

`make -j4 native cases check unit audit win32` builds the native i386 DSO,
runs cached native fixtures, audits application graph/ABIs and checks source
with the original Watcom compiler. Fixtures never invoke Wine or recapture.
All 56 App bodies retain graph/ownership/extent checks. Strict RET checks cover
54; two cold nonlocal tails have the explicit check in `scripts/`.

The source retains the 1994 function graph and period Watcom ABI, including
resource lookup/parsing, flat-DS far-buffer formals, output and cleanup. Native
GCC uses the authorized semantic ABI projection through `lib/recon/`.
OpenWatcom initial-commit source 6b9cb443 supplied SDK algorithms; their license
is retained in `src/resources.c`. CRT providers are finite identified support
spans in `binary.c`, with zero application reconstruction credit. Full 74-byte
`__CMain`, 257-byte `_IsTable` and encountered shared epilogues remain explicit.

The pristine image is unchanged. Original capture uses the separately patched
GS resource-reader copy (SHA256 0927524a7d8f90207996257ecd32e09670016917284de5203b0015d439dda92e).
Only the selected Usage→MsgGet buffer is qualified: its live 128-byte stack
object, DS-derived selector and capacity follow the actual caller bytes.
The native self-resource input is the exact frozen masked-image recipe in
`capture.bin`; it is never replaced with an unmasked executable. Console,
non-help commands, signal delivery and nonlocal error recovery remain pending.

The final measured pair took 487.759 ms for explicit original capture and 8.974 ms
for native replay (setup 5.770, execution 1.409, checks 1.796 ms). The corresponding
parallel native/period build took 1,456.524 ms. Diagnostics remain in scratch.

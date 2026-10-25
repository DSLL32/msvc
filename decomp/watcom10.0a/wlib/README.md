# Watcom 10.0a WLIB

Fresh standardized reconstruction of the native PE32 `binnt/WLIB.EXE`.
The one numbered case is actual-supported bare `?`, with a fresh regular stdin
file containing 64 LF bytes. Old pipe captures are not its oracle.

`make -j4 native cases` uses the shared native i386 DSO/case harness.
`make -j4 win32` checks source with the original Watcom compiler through the
shared launcher. Fixtures never launch Wine or recapture originals.

The first App main is currently an unimplemented INT3. No fresh original-backed
capture or application acceptance exists yet. The shared capture owner is
qualifying the finite GS resource compatibility conditions before execution.
`harness/startup-runtime.json` records the individually reviewed CRT/bootstrap
catalog, including full 74-byte `__CMain` separately from its 69-byte startup execution
subset, exact 257-byte `_IsTable`, shared RET and backward heap trampoline.
`harness/gs-boundary.json` records the bounded reader/owned-buffer conditions;
`harness/gs-patched-copy-provenance.json` retains historical patch provenance.
The pristine toolchain image remains untouched.

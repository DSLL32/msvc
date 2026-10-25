WLINK 10.0a is being restarted with the shared reconstruction scaffold and
one numbered bare `?` help case. Native builds use the shared i386 DSO runner;
`make -j4 native cases` builds it. Original Watcom builds use the shared launcher
and retain register ABI (`-3r`) and disabled added stack checks (`-s`).

The fresh registry accepts zero functions. Main is a named INT3 stub. Previous
minimal source/cases remain available in Git history and carry no acceptance
credit into this scaffold.

Only the exact GS-compatible `.patched/WLINK.EXE` may be captured; pristine
WLINK remains immutable. The patch manifest/provenance lives beside that copy.
The shared owner is qualifying the PE selector/owned-buffer guard; hashing the
copy alone does not authorize an unguarded application capture. Unknown startup
support remains masked. Native fixtures never invoke Wine.

Complete only this basic help case, then pause. The genuine main-only noreturn
contract must be retained when main is reconstructed; returning functions keep
strict RET checks. No generic RET waiver or blanket startup CRT interval.

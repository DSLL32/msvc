# WPP386 10.0a

The initial bare `?` help case is complete: 98 reconstructed application
functions, covering 6,827 unique bytes of original code. This is 0.98% of the
whole CF code extent (that denominator includes CRT); compilation is not yet
reconstructed. Further work is paused after this one authorized case.

`cases/0000-help` compares the complete application entry order, help output,
exit status 1 and artifacts against its saved original capture. Fixtures run
natively without Wine. The final original capture took 2.175 s; its native
comparison took 8.444 ms; final cached replay took 8.195 ms (setup 3.708,
execution 3.201, checks 1.286 ms). The parallel final incremental native/period
build took 1.572 s; the full audit target took 153 ms.

Build with `make -j4 native cases win32`; run cached regressions with
`make -j4 check` and the function graph/ownership/extent/RET audit with
`make -j4 audit`. The Win32 build uses original Watcom register conventions;
native i386 tests use GCC's ABI through the shared `lib/recon` layer.

The native harness preserves the pre-main FILE initialization's real compiler
heap allocations and five owned stream nodes, then their normal close/purge
lifetime. The original CRT and gateway execute unchanged during reference
capture. Binary-specific CRT adapters retain the compiler's allocator callbacks;
they do not interpose on the host allocator or overlay native FILE structures.
Unsupported paths retain named traps or explicit boundary failures.

Source is correlated with the initial Open Watcom release, commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`; reused files retain its notices and
record the relevant source paths and adaptations. The actual 1994 image remains
authoritative for behavior, function extents and calling conventions.

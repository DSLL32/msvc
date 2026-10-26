# WCC386, Watcom 10.0a

The initial `?` help case is complete and reconstruction is paused. Native
execution matches the original's banner, 99 help lines, exit status 1 and
application entry order, including startup and shutdown allocator callbacks.
Other application paths still contain named trapping stubs.

The target is `toolchains/watcom10.0a/binb/WCC386.EXE`, whose CF payload runs
through the corresponding `binnt` loader during original captures. Original
addresses in `binary.c` are CF image offsets. The shared debugger masks the
conservative CODE envelope and restores declared functions and encountered
text data; fixtures use only the native i386 DSO and cached capture.

Completed coverage is **74 logical application functions and 7,443 unique
original code bytes**. The matched trace contains 73 first entries.
`CPragmaInit` is a separately proved empty function sharing `CTypeInit`'s final
`RET`; that one entry is seeded on both sides and contributes no additional
bytes. Its frontier was not observed. A total application-code denominator
has not been established, so these counts are not a coverage percentage.

Run these commands from this folder:

```sh
make -j4 check       # cached native regression; no Wine
make -j4 original    # original Watcom compiler/linker build check
make -j4 audit       # shared graph, ownership, extent and ABI checks
```

The full audit covers 77 paired owners, including three justified CRT
adapters, with strict return-cleanup checks for 70 ordinary application
functions. Four finite nonlocal-exit residuals are checked separately in
`scripts/check-nonlocal-tails.py`; their raw graph edges remain present.
`scripts/audit-policy.py` binds `Wait_for_return`'s actual 60-byte rebuilt body
and any zero alignment gap to the current object, image and MAP.

`harness/startup.c` models genuine FILE-list allocation and cleanup, reversed
startup environment pointers and initializer order. Its callbacks use the
reconstructed private heap before `main` and during shutdown. Binary-specific
CRT glue in `crt/` retains the older CRT's real application allocator callbacks;
it does not interpose on host or shim allocation.

Source was adapted where useful from Open Watcom's initial commit
`6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`, checked against the 1994 image.
Applicable source notices and the project `COPYING` are retained. Differences
in older structures, tables and control flow are reconstructed from this target.

Final clean validation took 3.320 s for the parallel original/native build and
0.161 s for the full audit. The cached fixture took 7.874 ms: setup 4.625 ms,
execution 2.212 ms and checks 1.036 ms. The final fresh original/native pair
took 1,573.763/7.772 ms. Diagnostic reports remain in owned scratch; the case
contains only its source descriptor and `capture.bin`.

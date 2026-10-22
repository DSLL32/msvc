Direct utility cases, not RC command-line reachability
====================================================

The frozen original oracles cover is_space_byte (0x0100f3d0..0x0100f3fb,
stdcall RET4) and copy_basename (0x0100f530..0x0100f58d, stdcall RET8).
Native fixtures call these existing stateless functions directly via the small
src/native-main.c adapter. No production callers are introduced.

Whitespace tests all 256 low-byte inputs; ff, TAB, LF, CR and space return -1,
others return zero. Six pathname examples cover empty/plain/mixed separators,
drive paths and trailing separators. Basename counts include NUL; every byte of
96-byte guarded output records must match the original capture. Original first
returns additionally proved stdcall cleanup and nonvolatile registers.

    make -j4 native-trace
    python3 scripts/check-native.py --trace --case direct-whitespace --case direct-basename

An explicit original-only verifier remains separate:
    make capture-direct-reference

Its src/direct-run.c debugger invokes untouched original functions and observes
returns at an entry breakpoint. Controller-directed exit zero means direct test
completion, not natural RC CLI success. The verifier never runs a reconstruction
and is not a fixture dependency. The original result/stream bytes are immutable.

These are existing target-specific CRT boundaries, separated from application
reconstruction. This move adds no CRT implementation or coverage.

- `startup.c`: preserves the embedded entry/lifecycle graph, initializer and
  argv/environment setup, binary-mode/onexit state, shared `_setargv` identity,
  and main/exit calls under its SEH filter. Generic startup changes that graph.
- `onexit.c`: preserves the sentinel-selected DLL `_onexit` path versus the
  `__dllonexit` list-bounds path. The public SDK declaration would select an
  additional static wrapper.
- `precision.c`: preserves the startup `_PC_53`/`_MCW_PC` control-word helper
  (`0x10000`/`0x30000`) and its import-library thunk.
- `math-error.c`: the dormant callback remains a named INT3 stub with no native
  graph binding, implementation or coverage credit.
- `c2-crt.h`: preserves the target's direct CRT data imports and startup ABI
  declarations, avoiding extra SDK accessor vertices.

Behavioral compatibility still comes from shipped CRT libraries and `lib/recon/`.

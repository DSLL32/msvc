# Watcom 10.0a WASM

The initial `/?` case is complete: 68 application functions, 10,309 original code
bytes, matching entry order, output and exit status 1. Reconstruction is paused;
no second case is authorized. Original CRT bodies execute unchanged and receive
no application credit.

Use `make -j4 native cases win32` for the native i386 DSO and original Watcom
compilability check, `make -j4 check` for the cached native fixture, and
`make -j4 audit` for graph, references, ownership and extent checks. The audit
checks 65 ordinary RET contracts plus the finite assertion/help-exit residual
in `scripts/check-error-tails.py`; pending error handlers remain masked.
Generated audit reports and reconstruction diagnostics stay under
`~/.ai-tmp/msvc2/watcom10.0a/wasm/`.

The final native fixture measured 10.33 ms: setup 5.57 ms, execution 2.71 ms,
checks 2.05 ms. A warm Make run took 65.47 ms; native rebuild plus fixture took
1.06 s. The explicit original capture took 1.34 s; period rebuild plus final
audit took 2.78 s. Fixtures never
invoke Wine; original builds and explicit captures use the shared repo prefix.

The captured original is `.patched/binb/WASM.EXE` through the byte-identical
`.patched/binnt/WASM.EXE` loader, hash-bound by `harness/cf-profile.json`. Its
four fixed-size GS-preservation substitutions are at CF offsets `1e611`
(`8e6d18`→`909090`), `1e634` (`65`→`3e`), `1e653`
(`0fb55514`→`8b551490`), and `1e65b` (`65`→`3e`). Resource bytes and relocation
sites are unchanged. This help path supplies the caller-owned 128-byte buffer,
capacity 128 and DS far selector; no general segmented-pointer support is claimed.
Historical patch creation/qualification remains in Git commits `8b898631` and
`25bcbf9e`; historical captures are not the current oracle.

Released-source adaptations retain copyright/license headers and their source
paths from Open Watcom commit `6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`
(read-only reference: `~/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`). Shipped
1994 bytes govern differences from that source, including scanner behavior,
token IDs, resource layouts and the two distinct private hash bodies. Watcom
builds retain the original register/far-pointer ABIs; native builds use the
explicitly authorized GCC ABI and shared `lib/recon` support.

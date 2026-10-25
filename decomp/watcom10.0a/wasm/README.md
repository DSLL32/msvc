# Watcom 10.0a WASM

Fresh shared recon-tool scaffold for one initial `/?` case. Source is application
code under `src/`; the native i386 DSO and flat numbered case use the shared Make
rules. No application function is implemented or accepted yet.

`make -j4 native cases win32` builds the native scaffold and checks compilation
with the original Watcom toolchain through `recon-tool original tool`.
`make -j4 check` is native-only; it requires a genuine shared-backend
`cases/0001-help/capture.bin`, which has not yet been captured. It must not silently
recapture or execute Wine.

The original is the approved GS compatibility copy under
`toolchains/watcom10.0a/.patched/binb/WASM.EXE`, loaded through its unchanged
`.patched/binnt/WASM.EXE` sibling. `harness/gs-patch.json` preserves exact raw/patch
hashes, the four fixed-size substitutions, untouched resource-tail identity and
Git-qualified prior evidence. The patch is valid for the observed flat resource
path only: every reached far-reader selector must equal actual DS, with owned
pointer/capacity, thread/context and callback lifetime checks. Historical capture
helpers are retired, not fixture dependencies.

Shared CF capture currently qualifies finite WCC geometry only. WASM capture,
CRT/service closure and GS ownership support must be reviewed/implemented by the
shared tool owner before progress; no WCC-profile reuse or startup bypass.
The first pending application entry is `main`, CF RVA `cb28..cb5d`.

Released-source reference: `/root/.ai-tmp/watcom10/open-watcom-initial-6b9cb443`,
commit `6b9cb4438959d845a2b4dd4bf19f7908ecd608e9`. Useful code may be reused
with its original license/copyright, after checking actual 1994 behavior, graph
and Watcom ABI. Native GCC ABI is explicitly authorized.

Pause after this single help case is complete; no second case is authorized.

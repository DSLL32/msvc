# AGENTS.md

Use ~/.ai-tmp for scratch space, creating folders under here as required.
For original builds/reference captures, use the shared repo Wine prefix
`~/.ai-tmp/vc1.0-capture/wine` (win32). Its fixed drives are `R:` → repo root
(`/root/msvc2`), `S:` → shared scratch (`~/.ai-tmp/msvc2`), and `T:` →
ephemeral case workspaces (`/tmp/recon`). Use separate
binary/case subdirectories; do not remap these drives or stop the shared server.
Prefer relative case paths; use stable absolute paths only when behavior requires
them. Fixtures remain native-only. Use the shared prefix for all binaries; do not
create private prefixes or per-case drive mappings. When migrating old fixtures,
explicitly recapture path-dependent references before retiring obsolete transport
state; never relabel old raw evidence.

## Folder layout

 - toolchains/
   Original MSVC toolchains with one folder per version (e.g. 7.1-sp1, 1.0-nt)
 - decomp/
   Reconstructions, with one folder per project - if project consists of more than one binary, one subfolder for each under the project
   Within each binary folder, keep reconstructed source in src/ and scripts in
   scripts/. Keep the Makefile at the binary root; invoke its scripts from that
   root so existing case and build paths remain stable. Keep case metadata under cases/.
 - lib/recon/
   Shared native Win32/CRT compatibility and tracing support.

## Reconstruction of a binary

Functionally equivalent reconstruction preserving original function graph, individual function ABIs, and any exported functions' signature, name, and ordinal.
The original EXE will have its entire code segment masked with INT3 traps, and our reconstruction should also trap at the entry point. 

Then, for some case:
 - Invoke the modified original EXE and the native reconstruction with the case
   to obtain fresh matching evidence when accepting new functions.
 - On trap: 
   * assert that they trap in the same logical place. If they don't, debug, fix, and rerun the case - we may have wrongly determined a function's extent in a previous reconstruction.
   * else, this is a new function to reconstruct. Determine its extent, reconstruct it (potentially replacing a prior stub function), then unmask the instructions + its .text data (e.g. jump tables or constants) in the original EXE. Calls to functions not yet reconstructed should have named stub functions whose bodies trap. Un-stub calls to the new function in prior reconstructions.
   * rerun the case.
 - On no trap: 
   * if case successful - producing identical output to the original - move on to another and repeat this process. Else, debug, fix, and rerun.

## Native builds and testing

Use native i386 Linux builds during reconstruction, debugging and fixture runs.
Build with debug symbols and use GDB for named breakpoints, INT3 stubs and state
inspection. Source remains Win32 C: preserve the original APIs, libraries, coding
style and function ABIs.
It must still compile with the original toolchain; verify that build as work
progresses even when all testing is native.

`lib/recon/` supplies native Win32/CRT shims and tracing. Its dedicated maintainer
(`/root/recon_lib_maintenance`) owns shared changes; binary workers should contact
it directly for shim changes or advice. Escalate reconstruction policy and
cross-project decisions to the orchestrating agent. Include `<recon.h>` last
in each reconstructed C/header file's include list; it is empty in the Win32 build.
Keep host compatibility in this shared layer and compile its C files directly at
native final link. Use `make -j4`. Wine is for explicit original-toolchain builds
and reference capture, outside fixture execution.

Regressions compare native entry traces, status, streams and artifacts against
cached original references. Never silently recapture or run originals on each
regression. Filter CRT/import/shim entries on both sides, seed accepted prefixes,
and share seen-state across synchronous children. Retain ABI/function-graph audits.

Use `tooling/graph-audit/` for function graph discovery and comparison, with the
applicable check flags. Retain separate target-specific checks where needed;
report common gaps to the tool maintainer and orchestrating agent.
Build through `graph-audit.mk` or `build-quiet.sh`, then invoke the binary directly.
Keep successful Cargo warnings/build chatter in logs; show the tool's output and
errors, and build diagnostics if compilation fails. Do not use `cargo run` per audit.

Fixtures must never invoke Wine, directly or through helpers/children. Capture
inputs and outputs of otherwise-required Windows invocations beforehand. Verify
invocation and complete input before replaying captured responses; unknown
dependencies fail. Normalize only identified clock fields.
Record elapsed fixture times, splitting build/setup/execution/checks where measured.

## Reconstruction source

Preserve known original symbol names. Where names are unknown, prefer
conventional names such as `main`, `DllMain`, and `_DllMainCRTStartup`.
Adapt test harnesses around those names rather than renaming reconstructed functions.

Reconstruct program logic as readable, idiomatic (for the time period and platform) C, recovering meaningful function signatures, types, structures, and control flow. Do not use mechanically copied or generated assembly as the reconstructed implementation. Use inline assembly only where required to preserve behavior or an ABI that the original C toolchain cannot express, and document the reason. Disassemblers, decompilers, and generated assembly listings are permitted for analysis and verification; this restriction applies to reconstruction source.

Do not bulk copy data sections into the reconstruction in any format. Define data as it is encountered during reconstruction, placing it somewhere that makes sense in the current context.
Keep ordinary string literals at their point of use. Introduce named string objects only when sharing, mutation, or observable address identity requires them.
While we must preserve the original function graph, you have full ownership of source layout, identifiers, libraries, headers, etc. Code in a style faithful to the reconstruction target. Feel free to reorganize code or update function, type, and field names as your knowledge of the target grows during reconstruction.
Compile with the original toolchain that built the target. Create a GNU Makefile in the binary's folder to do the build - invoking the toolchain via wine - and support parallel original-toolchain and native build targets.
Commit as you please, suggested is per function reconstruction or other substantial work.

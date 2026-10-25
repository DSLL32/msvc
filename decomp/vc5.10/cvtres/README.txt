CVTRES 5.00.1668.1

Application reconstruction is complete for the identified 22 functions/7445
code bytes. Four CRT bodies 400 bytes and four import thunks 24 bytes are runtime
boundaries, not reconstruction credit. binary.c is the sole function registry.
src/ contains application source and encountered resources only. Application
C/header/resource bodies are unchanged by the layout cleanup. Native setup and
main adapters live in harness/; binary.c contains registry metadata only. There
are no binary-specific CRT implementations, so no crt/ folder is needed.

Nine numbered I/O cases preserve saved original arguments, inputs, full streams,
status, complete object bytes and unseeded App entry order. The target's 18
encountered resource messages are saved native-provider inputs, not a PE dump.
Duplicate cases expect status 1 and a present empty object. The ARM-without-CE
case preserves the original status 0 despite its diagnostic. No timestamp
normalization or new original captures were introduced.

make -j4 check    native cached replay; never Wine
make -j4 unit     existing narrow native unsupported-depth guard
make -j4 win32    original 5.10 compiler through the shared launcher
make -j4 audit    shared registry/runtime projection and Rust graph/ABI checks

The shared audit checks 22 App functions, RET cleanup, references, ownership,
whole extents, imports and exact import-site inventories, including 45 original
cached-register calls. The dynamic resource-loader call remains explicitly
opaque. SDK/provider-bound callee-preservation contracts are in abi-witness.json.
The rebuilt read_pixel switch's owned data and exact targets are symbolic policy.
Its classmap/opcodes/six local COFF relocations and source/code identity were
verified once against the current build during migration; the old scalar
certificate framework is retired, not presented as a recurring shared check.

Extent completion is not universal input/branch/API equivalence. Original
unsupported-depth fallthrough has AX/low-byte/residual-high-EAX limitations.
Preserve that Win32 behavior; the native-only guard traps instead of inventing
an answer. The single guard unit establishes no public 16bpp case or reachability.
Other message IDs, formats and API paths remain outside the validated scope.
The application remains idle; migration adds no function acceptance or case.

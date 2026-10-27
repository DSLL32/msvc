# Canonical native entry points

This is the agreed design. Shared build and runtime support is implemented;
binary adoption is in progress. Implement ordinary native executables first. A dual-use executable that can
also be loaded with `dlopen` has been discussed and is deferred.

## Executables

Every executable reconstruction has an `entry.c` at its binary root. It defines
the native `main`, configures the shim, and invokes the original application
entry. Where the application requires an existing reconstructed CRT startup,
invoke that startup so its initialization precedes the application entry.
Retain approved direct-App CRT-boundary bridges; do not invoke retained Win32
CRT startup merely because it exists.

Compile root `entry.c` only for `native-bin`. It is native host code, outside
the reconstructed function graph, and is excluded from the Win32 build.
Keep original function names, signatures and calling conventions in `src/`
and `crt/`; shared build rules handle the collision between the native wrapper's
`main` and the original `main` without recompiling application objects.
Rename App `main` and its internal references together on a private partial-link
copy; keep the fixture objects untouched. Aliasing `main` to startup would recurse
when that startup calls App `main`.

The wrapper supplies actual arguments and environment, an owned Windows command
line, executable/module identity, resources, and other required startup state.
It registers statically linked dependencies where needed. Ordinary execution
must not depend on case names, captured responses, or fixture-specific entry
profiles. Unsupported application paths retain their normal reconstruction
traps.
Preserve required FP precision, file mode, per-object ABI flags, writable
argument/environment text and exit cleanup. Define startup ownership explicitly
so arguments and environment are not bound twice. A small common native helper
owns writable argv/environment, the Windows command line and actual executable/
cwd paths. It does not run reconstructed CRT startup, set FP/file mode, or
register binary resources and DLLs; those remain explicit in each `entry.c`.
Disable any generated shim `main` when supplying root `entry.c`; configuring environment variables after
constructors run is not sufficient to initialize constructor-owned state.

## Statically linked DLLs

Native executables may contain their required reconstructed DLLs as static
module bundles. They do not need a separate executable wrapper for those DLLs.
The original-toolchain build still produces real Win32 DLLs with their original
exports, ordinals and ABIs.

The common make include provides a rule taking a module identifier, DLL project
directory, required native symbols and an optional list of files to embed.
Illustrative interface (optional fourth argument omitted):

```make
$(eval $(call recon-native-module,c1xx,../c1xx,\
    InvokeCompilerPass AbortCompilerPass _DllMainCRTStartup))
```

The shared rule:

1. Builds/reuses explicitly selected reconstruction and required startup objects,
   excluding fixture hosts, tests and duplicate shim implementations. Existing
   source/support lists mix these categories and cannot be selected wholesale.
2. Partially links those objects into one relocatable object.
3. Gives the requested symbols unique linker names, such as
   `c1xx_InvokeCompilerPass`, and localizes the other defined symbols. Internal
   references remain valid; undefined shim/dependency references stay external.
4. Embeds any supplied files and adds the bundle and asset objects to the
   consumer's native link with correct prerequisites.

The retained symbols include the lifecycle entry needed for DLL startup.
Localization must also isolate application data symbols. No source renaming,
calling-convention changes, or function merging is required.
Account for ELF constructors separately: localization does not postpone their
execution until DLL attachment. Direct consumer imports need linker bindings
to the retained namespaced symbols, including required data imports.

## Module registration and shim ownership

Extend recon-lib's module registration so `entry.c` can configure a DLL name,
its export names/ordinals and statically linked function pointers, plus its
attach/detach entry. `LoadLibrary` and `GetProcAddress` resolve these configured
modules; static data imports and direct function imports bind at link time.
Preserve initialization ordering, load references and detach behavior. Remove
fixture-only restrictions such as exactly one export lookup from the ordinary
module-loading path. An unavailable module must fail normally; never fall back
to the original executable or Wine.
Use the actual typed DLL-entry ABI and handle attach failure. Detach must run
on ordinary application `exit()` as well as wrapper return, with appropriate
static/dynamic attachment semantics. Existing observer callbacks are not DLL
entry points. Registered modules must also satisfy the application's consumed
filename/existence probes before loading.
VC5.10 CL genuinely unloads/reloads frontends between files in its non-grouped
path. Static bundle globals remain resident: attachment alone does not establish
a fresh DLL data image. Before CL adoption, verify existing frontend resets and
provide any required reset of the bundle's own initial data/BSS, without resetting
shared CRT/process state. RC/RCDLL's static lifetime avoids this particular seam.
For that reset, shared rules generate a small linker-script fragment grouping
each DLL's data/BSS (with required alignment padding) into one contiguous writable
region with start/end symbols. Save its initial image after relocation, before
DLL initialization, and restore it before reattachment. Shared shim/CRT state
stays outside the region. Preserve ordinary initialization/cleanup ordering;
symbol localization alone does not provide this layout or state reset.
Initially use a buffer and `memcpy` for snapshot/restore. Reading raw bytes from
the ELF file would lose runtime relocations; a mapping-based COW reset would
require additional page-layout/mapping support. Recon-tool's separate case
executors already supply process isolation; this reset is for within-process
DLL reloads.
The first implementation rejects constructor-bearing reset bundles. Explicit
pre-main constructor execution is available only with reset disabled; it does
not provide fresh-image reload semantics. TLS bundles remain unsupported.

Link one common Win32 shim state into each native process, so the EXE and its
DLL code agree on module handles, resources, standard handles, command line,
memory bookkeeping and console handling. Last-error state is shared across
modules but belongs to the thread. State layouts, meanings and managing
operations must not vary silently with per-binary preprocessor flags.

CRT state follows the original runtime's ownership: binaries using one CRT DLL
share its appropriate state; separately linked CRTs retain separate state.
Standalone fixture flag differences do not establish different runtime owners:
VC5.10 CL initializes the shared MSVCRT `_fmode` to zero and C1/C1XX do not
reset it. Their canonical integration uses that one shared state; the frontend
fixtures' fallback `0x4000` is not a separate DLL requirement. RC now selects
MSVCRT510 like RCDLL; its three existing I/O cases and six rejection tests pass.
The `0x4000` fallback originated in the first VC1.0 LINK shim, not verified VC5
startup state. Defer cleaning up that inherited default and its consumers until
it will not disturb ongoing reconstruction builds; do not change the live default
as part of preparation. For the relevant open paths, both zero and `0x4000`
select text mode, so the fallback is not a blocker for combining CL/C1/C1XX.
Their canonical shared runtime should nevertheless initialize the observed zero.
Do not merge profile-dependent CRT state or discard required code variants just
to make the static link succeed. The static design avoids requiring a separate
process-state shared library.

## Embedded companion files

Embed required shipped companion files, such as VC1.0's `CL.ERR`, `CL32.MSG`,
`C1.ERR`, `C23.ERR` and `LINK.ERR`, in canonical native executables. Shared build rules
link their exact bytes into read-only sections; do not generate bulk C arrays.
Accept file lists for both executable assets and static DLL module assets.
The rules own conversion, collision-free symbol names, dependencies and linking;
`entry.c` only registers the resulting spans under their runtime paths.
Use `objcopy -I binary -O elf32-i386 -B i386` with
`--rename-section .data=.rodata,alloc,load,readonly,data,contents` to create the
linkable object. An input named `CL.ERR` supplies `_binary_CL_ERR_start` and
`_binary_CL_ERR_end`; shared rules give these symbols stable asset-specific
names where necessary. Track the source file as a make prerequisite.
`entry.c` registers each required logical path and its byte span with recon-lib.
An illustrative interface is `recon_register_embedded_file(path, data, size)`.

Serve registered files through existing CRT and Win32 file operations, preserving
independent open positions, reads, seeks, EOF, size queries and CRT text-mode
translation. Reject writes to embedded files. A memory-backed descriptor backend
may reuse existing file handling instead of introducing another `FILE` layout;
ordinary execution needs no extracted companion files on disk.
Anonymous memory-backed descriptors must not create extracted or temporary
filesystem files.
Cover `access`/`stat` and other consumed path probes as well as opening. Separate
opens need independent offsets; duplicating one backing descriptor is insufficient.

Embedded module resources also need an in-memory registration path: existing
host resource readers may use POSIX I/O and bypass shim file registration.
Where a tool opens its own program path as a message-resource container, provide
the required resource-bearing container through that path; the native ELF alone
does not contain the original resource layout. Preserve existing resource scope
limits rather than copying an entire original executable by default.

Use explicit registrations and normal Windows path matching, including only
needed aliases. Do not redirect arbitrary files by basename. Unregistered paths,
including user-supplied alternate catalogs, inputs and outputs, retain ordinary
filesystem behavior. Registration belongs to native startup; existing fixture
inputs and comparisons do not change implicitly.

## Build and test integration

A Makefile at `decomp/<project>/` coordinates the toolchain's canonical native
executables and their module dependencies. Binary Makefiles retain `native-bin`;
shared rules perform compilation, localization and linking. Preserve GNU Make
jobserver parallelism (`make -j4`) and incremental dependency tracking.

Define canonical sibling executable names and Windows-visible installation
paths, keeping module identity, `_pgmptr`, argv and cwd coherent. EXE dependencies
remain real native children, including self-spawn; static DLL bundles do not
replace them. Preserve the original command-line serialization/tokenization,
environment, handles and synchronous status. Children must start with their
actual supplied environment, even when it omits `PATH` and all `RECON_*` settings.

For the old Microsoft CRT spawn boundary, preserve its joined Windows command
line rather than treating packed option strings as Linux argv elements.
Canonical wrappers can opt into protocol v1 through root-only
`NATIVE_BIN_ENTRY_CPPFLAGS += -DRECON_NATIVE_ENTRY_PROTOCOL_V1`. The read-only
ELF marker advertises support for a sealed raw-command-line memfd inherited
at descriptor 198; startup consumes and closes it before calling App code.
Reject an occupied slot without overwriting it. Unmarked external programs
retain ordinary exec arguments and receive no transport descriptor. This
does not carry fixture state or change the child environment.

Reuse the existing native reconstruction objects and flags. Cases continue to
use the same application code through `libbinary.so`, with their own host setup
and explicitly captured dependency responses where the case requires those.
Root `entry.c` is not added to case libraries. This work does not silently change
accepted cases, capture new references, or launch Wine during native tests.
Exclude fixture-only providers, hooks and tests from executable links as well as
DLL bundles, while retaining genuinely required native startup support. Existing
unsupported dependency/shim domains remain explicit failures.

Verify canonical startup with ordinary CLI invocations and actual reconstructed
dependencies, including VC5.10 RC/RCDLL and CL/C1XX; VC1.0 uses child EXEs.
Check no-argument startup and, when supported, explicit help separately against
the original. Exercise the installed native executable as well as library
fixtures. Record the actual diagnostic and status; an unrecognized-option test
does not qualify as help.
Continue original-compiler builds
and function-graph/ABI checks; native host glue grants no reconstruction credit.

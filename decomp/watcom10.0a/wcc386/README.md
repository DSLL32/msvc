# Watcom 10.0a WCC386

The reconstruction target is `binb/WCC386.EXE`'s embedded CF image. The identical
`binnt/WCC386.EXE` / WPP386 / WASM PE stubs load and relocate that image and supply
OS services. Neither the LX wrapper nor the PE stub is application coverage.

`make -j4 inspect` checks the immutable original identities and CF relocation
metadata without executing either executable. `make -j4 native` builds a named
main INT3 stub, and `make -j4 original` compiles and links that stub with Watcom.
Fixture acceptance is blocked pending the CF mask and service contracts. There
are zero accepted functions. Isolated compiler/link probe receipts are under
`cases/compiler-abi/`; stub validation is under `cases/scaffold/`.

## Initial case

`cases/help/case.json` proposes `?` with redirected standard streams and empty
stdin, avoiding terminal-dependent pagination. Its unmodified original baseline
is captured under `cases/help/reference/`: status 1, stdout 4509 bytes, empty
stderr, 0.503 seconds. This is not masked/native application acceptance. Use the shared repo Wine prefix only for
explicit capture/build work; eventual fixtures run native i386 binaries against
cached originals and must never invoke Wine.

## Image and ABI contract to establish

The CF header at file offset `0x50` identifies file load offset `0x2200`, loaded
size `0x8209c`, relocation offset `0x7c448`, allocation size `0x91000`, and initial
entry offset `0x2fe00`. Eight relocation groups contain 11801 DWORD base fixups.
Logical identities must be offsets from the observed allocated CF image base.
CF has no PE section table: establish executable/data extents before masking;
never mask the entire loaded image or disguise it as a PE audit input.

The stub's loader is at `0x4101cc`; relocation application is at `0x41018c`.
Its far service gateway `0x410788` calls `0x41078e`, which saves six registers
as EAX/EBX/ECX/EDX/ESI/EDI, calls the C dispatcher `0x410bef`, restores flags
through SAHF, and restores the resulting register values. The 2002 references
`bld/w32loadr/{loader.c,loader.h,int21.asm,int21nt.c}` explain the same structure,
but actual 1994 instructions and capture evidence determine the contract.

The permitted Win32/CRT substitution stops at individually proved service
boundaries: record complete meaningful register/flag inputs and results,
owned input/output memory, stream/file changes and errors. It does not excuse
unknown application helpers or permit target-local CRT reconstruction.

The user permits GCC calling conventions in native tests. Compare actual native
semantic arguments and ownership with the original meanings, without pretending
their raw argument registers match. The original Watcom-built Win32 version must
still preserve and audit the target's real function ABIs, including callbacks and
aggregate returns. The compiler probe confirms three arguments in EAX/EDX/EBX,
a fourth in ECX, a fifth on the stack with callee cleanup, and an eight-byte
structure return through a hidden ESI pointer. These observations do not replace
per-function ABI proofs. No OMF converter, PE loader or assembly veneers are
needed for the approved native calling-convention policy. Watcom 10.0a rejects
`__asm` statements; the stub uses an aux byte intrinsic to emit INT3 inline,
without introducing another function vertex.

Service capture and an explicit CF auditor input contract remain prerequisites.
Mask only proved code extents after loader relocation: relocating a disk INT3
mask would modify the trap bytes at fixup sites. Startup also zeros consumed
relocation storage, so distinguish pre-startup image verification from later
application state. The original compiler must remain usable; `<recon.h>` goes
last in each reconstructed include list and all shared shim changes go through
`/root/recon_lib_maintenance`.

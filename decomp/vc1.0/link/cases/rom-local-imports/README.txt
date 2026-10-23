ROM, local imports, duplicate warning, MAP and LINK_REPRO

Original CL /c /O2 /G5 compiles provider.c and client.c. The provider exports a
normal local_fn; the client declares it dllimport, and both define duplicate_value.
The ordinary public LINK invocation includes /ROM /warn:2 /nodefaultlib /entry:main,
/subsystem:console /out:rom.exe /map:rom.map. Two untouched-original runs exit0
without /force, warning LNK4006 for the duplicate and LNK4049 for the local import.
LINK_REPRO names the fixed S: repro subdirectory and emits its response and exact
copies of both original COFF inputs. spec.json and capture/original-validation.json
pin the complete inputs, environment, status, streams and five artifacts.

The output is i386 ROM COFF at offset0, not an MZ/PE executable. artifacts-rom.py
validates that structure and identifies only its COFF timestamp at4 and the MAP
hexadecimal/ctime clock fields. Copied object timestamps and response bytes remain
exact. No pointer, path, output-order or padding normalization is permitted.
No claim is made to execute the ROM on a hardware loader.

Fresh masked-original/native GDB acceptance closes seven application functions:
reproduce_input1fd70, format_input_reference25300, warn_unused_libraries1ccea,
format_undefined_references25ad0, adjust_rom_section_header222e0,
emit_rom_relocations1f280 and close_trace_file1fe00. The unused-library walker is
actually reached by /warn:2; its original entry is unaligned after the preceding
RET. The original omits cleanup calls from both reference/library walkers.
The section adjuster takes the i386 early return in this case. MIPS header mapping
and ROM HIGHADJ/REFLO branches were recovered statically, not dynamically exercised.

The original BaseFixup has size16, rva+8 and value+12. The prior size12 source was
incorrect despite matching prior serialized PE/Mac outputs; those artifacts did
not establish the physical internal layout. base-fixup-layout.json supplies
independent original writer/comparator/serializer instruction evidence, compiler
layout assertions and fresh matching live typed records at the ROM serializer.
DWORD+4 and WORD padding+2 remain unconsumed/undefined, without invented semantic
values or explicit initialization. Only type/rva/value are compared. The two
sorted8-byte output pairs occur at ROM offset2048. All39 prior native PE/Mac
cases pass after the correction; no historical12-byte state sample is relabeled.

The reached shipped CRT _system body is separately identified by exact original
and canonical-library bytes/relocations; it adds176 support extent bytes and zero
application credit. Original entry/return errno is0. It invokes only the two
pinned cmd.exe /c copy children with status0 and empty streams, as frozen in the
raw original events. The first helper rejection was a transport limitation,
retained in capture/transport-rejected, not an original semantic failure.

Native fixtures use shared recon_system and SpawnBroker with the strict response
in cases/native-boundaries/rom-repro. Full current source bytes, command, relevant
environment, cwd, destination absence and parent directory must match before
frozen copied bytes are applied. There is no Wine, PE child, shell emulator or
/bin/sh execution. Unknown/unbrokered commands fail explicitly. Host-only rejection
checks cover changed input/invocation/environment and invalid output state.

validation.json records the seven fresh entry comparisons, typed state, the fresh
whole-original seven-entry trace, exact artifact equivalence and all40 native
trace/output regressions (632 application entries). Canonical compiler and660
Rust graphs,649 callback sets,455 contextual data and41 ECX checks pass. Coverage
is230971/281634=82.0110%,809 frontiers:1186 new application instruction bytes,
36 alignment and176 CRT support extent bytes. This is not branch coverage or a
complete application-body denominator. Timings retain measured components and
label unmeasured incremental build time honestly; raw transcripts stay in scratch.

C1 numeric/varargs/debug case, original toolchain 1.0-nt

Queued original CL case:
    CL.EXE /Zi /Od /G5 /W3 numeric-varargs.c

Source combines floating literals/exponents/suffixes, float/double/long-double
and integer conversions, packed/nested structures, signed/unsigned bitfields,
unions, enumeration types, a wide string, variadic function pointers, va_list
argument traversal and floating sprintf. /Zi adds native debug/type records.
The generated program verifies the sum, formatting, conversions and record
values and exits 0 with empty streams. It was run from the original build.

C1 is queued after hello-dll. manifest.json contains the actual captured
application/command/environment, target/header/message-file hashes and portable
MSC_CMD_FLAGS. No command-line arguments are passed to C13232.EXE; its explicit
environment contains only that variable, including its final space. Substitute
only {bin}/{include}, and use the relative intermediate prefix ir. Inputs and
debug records otherwise retain their original names and bytes.

Independent original C1 replay returns 0 with empty stdout. Exact stderr is:
  numeric-varargs.c(24) : warning C4069: long double is the same precision as double
The warning is captured with original CRLF; no modern precision assumption or
warning suppression is used. All four raw outputs match the original CL child
capture exactly: irex2330, irsy535, irin149, irgl402 bytes.

The /Zi run also creates irdb and msvc.pdb. Two fresh original captures at
canonical Windows cwd N:\numeric-varargs retain an identical raw PDB path
N:\numeric-varargs\msvc.pdb. Each paired label needs its own private prefix
and N: mapping, so outputs cannot collide and no paths are normalized.
Auxiliary oracles: irdb51 bytes, msvc.pdb574 bytes. Compare all auxiliary bytes
except the shared timestamp DWORD at irdb+14 and msvc.pdb+48; require both values
to agree within each run before normalization. The PDB format signature and
full file-set receipts are recorded in validation.json. Reconstruction checkpoint
ff868c1 established native provenance: begin_program_database passes time(0),
create_program_database writes it at PDB header byte 48, and
write_program_database_reference copies program_database_signature into the
LF_TYPESERVER record at irdb byte 14. Age and all remaining bytes are exact.
The simple minimal-c verifier does not configure this namespace; use the C1
harness numeric-varargs case for the complete six-output check.

Original C2/LINK snapshots and the successful original executable are retained
as original-only references. They are not assigned to those workers by this
case. The retained backend snapshot predates canonical PDB capture and is not
a complete independent C2 case; that needs fresh auxiliary-input capture.
No normalization is allowed for the first four C1 output files. The reference
object/executable hashes identify captures; timestamp normalization for future
backend/link cases must be separately established.

Capture used ../minimal-c/capture-cl.c at original CL's CreateProcessA thunk
RVA0x11000, copying each phase input before its consumer launched. No diagnostic
driver options were added. Original binaries remain untouched; scratch/helper
logs are in ~/.ai-tmp/vc1.0-numeric-varargs/, using the parent's private prefix.

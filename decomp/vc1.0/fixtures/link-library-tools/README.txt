Original-validated archive, dump and edit lifecycle for LINK.EXE 1.0-nt

The authoritative command order derives from link-coverage commit 68789b5:
create -> list -> extract -> remove -> list-trimmed -> dump-library -> dump-object
-> dump-dll -> edit -> dump-edited. The queued reconstruction order is shared
create setup, hello-debug-dll, then the remaining lifecycle. Each manifest phase
also has frozen inputs so it can reproduce independently.

All phases use LIB={toolchain}/lib and LINK32_DB=27 (original buffered backend),
with CL/_CL_/LINK/_LINK_/MSC_CMD_FLAGS unset. The created archive is 4184 bytes;
it is never trimmed or patched. Original mapped library allocation under Wine
is intentionally not substituted for this authorized buffered setup.

Extraction must exactly reproduce the input alpha object under its long member
name. Removal leaves only that member and excludes beta.obj. Archive output
normalization allows only archive member date fields. Embedded object timestamps
are frozen input bytes and are not ignored. Dump streams are exact, including
all timestamps and ordering read from those frozen inputs.

The editor modifies a copied debug DLL to stack reserve/commit 0x180000/0x2000
and heap reserve/commit 0x140000/0x2000. It emits original LNK4058 because checksum
calculation is unsupported on this OS; preserve the warning and exact bytes.
No checksum success is claimed. The following header dump is likewise exact.

Run the untouched original independently:
  python3 ../link-debug-dll/capture-original.py --fixture .

artifacts.py shares the structural parser from ../link-debug-dll/artifacts.py.
All original invocations and outputs passed independent fixture replay; see
original-validation.json. Reconstruction acceptance/replay belongs under LINK.

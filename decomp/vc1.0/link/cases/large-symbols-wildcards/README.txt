4096 defined external symbols and relative wildcard inputs

Two original CL /c /O2 /G5 translation units define2048 ordinary external ints
each. unit0 additionally defines entry(), which checks the first value in each
object. The source generator, complete C sources, original compiler command,
streams and input hashes are frozen here. No reconstruction implementation is
generated. LINK uses /nodefaultlib /entry:entry /subsystem:console, a normal
relative unit?.obj operand and full executable/map output. Omitting subsystem
caused automatic main/WinMain detection and original exit23; that rejected probe
is preserved separately and contributes no acceptance.

Two untouched originals succeed with identical status/streams and complete
2097152-byte large.exe plus287196-byte large.map after only the PE timestamp
and the map's rendering of that timestamp. No filenames, ordering, paths or
padding are normalized. The native fixture uses the LINK-owned shared-prefix
reference, with fixed R:/S: identity and relative inputs. It never launches Wine.

Two whole application functions are now readable C: expand_wildcards
00020630..000206e0 copies the original directory prefix, emits the bracketed
filename list and classifies each _find result; expand_hash00026860..00026a10
splits one segmented linear-hash bucket at a time, retaining old/new chain
order. Their original CFGs contain591 genuine instruction bytes and17 alignment
bytes, without inline data. Both preserve the original cdecl interfaces and
original direct/callback graph. Capacity failure keeps the original hash flag.
GDB first-split state changes1024buckets/split0 to1025buckets/split1 at3073entries.
The large corpus also exercises subsequent splits; saturation/doubling branches
are statically reconstructed, not separately claimed as dynamically covered.

The CRT _find vertex is byte-exact to shipped LIBC _SETARGV.OBJ in original and
canonical reconstruction, including every Find* relocation. Its144-byte extent
contains143 instruction bytes and1 alignment byte. The shared maintainer's
original differential proof covers30raw rows, persistent filename pointer,
sorted names, DOS wildcard behavior, errno/LastError and close/exhaustion. Our
fresh original/native GDB entry and partial streams, then full final artifacts,
provide the current-case proof. Native correspondence is at _find, not a claimed
per-import GDB match. Only FindNextFileA was a newly reached import; its6-byte
IAT thunk and SDK stdcall ABI are verified. FindFirstFileA/FindClose were already
accepted. Other imports remain trapped. CRT/imports receive zero application
credit, and no public Find* shim was introduced just to reproduce CRT internals.

All37 native cases pass612 application entries plus exact status, streams and
full artifacts. The full suite takes2.375s external in one sample, excluding
builds; the new case takes0.216s. Canonical original-toolchain plus native-trace
build and all audits take27.168s as one unsplit concurrent make -j4 invocation.
639 graph checks include10 explicitly evidenced CRT vertices;794original/
791rebuilt raw accepted extents,629callback sets,453data definitions and41ECX
return-cleanup checks pass. Four graph/extent mutation checks, six reference/
namespace rejections and two CVPACK boundary rejections pass. validation.json
records separate capture/build/fixture components and raw scratch evidence.

Current accepted extent226901/281634=80.5659%,788frontiers. This is not branch
coverage. Application bodies contribute608new extent bytes, CRT144 and import6.
The approved OMF and large-symbol/wildcard families are both complete. Remaining
suggestions in remaining.json are analysis only; no extra scenario was started.

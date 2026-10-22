Two proposed LINK coverage scenarios, validated on the untouched original

Status: designed and original-validated; not queued reconstruction assignments.
catalog.json records the exact commands. original-validation.json records 15
successful original invocations and independent artifact checks. Scratch logs
and generated inputs/products are under ~/.ai-tmp/vc1.0-link-coverage/.

Measured baseline at LINK commit 2678296:
  400 accepted frontiers, 110,787 / 281,634 original .text bytes = 39.3372%.
  The numerator is the union of accepted native functions, CRT ranges, shared
  text data and import thunks. The denominator is .text VirtualSize, excluding
  raw-file alignment beyond it. This is accepted extent coverage, not dynamic
  instruction, basic-block or branch coverage. coverage.json retains ranges.
  267 / 380 named driver catalog entries are accepted, but the catalog does not
  identify every native function. That ratio is not whole-binary function coverage.

1. hello-debug-dll

Derived from hello-dll, with one exported data variable; retains the same GUI.
Compile hello-debug.c with original CL /c /O2 /G5 /Zi /Gy /MT. Compile the two
C++ sources with those same flags; inline.h deliberately creates duplicate
COMDAT definitions whose addresses are retained by the two modules. Copy
alpha.obj to member-with-a-long-name-alpha.obj and build helpers.lib as below.
Copy resources.res from ../resources-rich/rc/resources.res. Copy the .def and
order file beside the inputs, and invoke LINK.EXE -link @debug-link.rsp.

The .def adds an aliased entry, a NONAME ordinal-only entry, a kernel32 forwarder,
exported data, and both C++ library functions. Options enable full COFF/CodeView
debugging, map output, function ordering, checksum handling and verbose library
search, and exclude OLDNAMES. Resources exercise the input-conversion path.
Independent inspection confirms COFF, CodeView and FPO debug directories and
nonempty resource/export directories. Original link succeeds with normal
mapped I/O and with the buffered mode used by scenario 2.

Expected newly reached areas include definition parsing, order-file parsing,
map creation/symbol sorting/undecoration, debug symbol/auxiliary/line writers,
CodeView packing, resource conversion and checksum handling. These are targets
for reconstruction, not claims that all branches of those functions ran.

2. archive-dump-edit

With LIB pointing at the original library directory and LINK32_DB=27:
  LINK -lib /out:helpers.lib member-with-a-long-name-alpha.obj beta.obj
  LINK -lib /list helpers.lib
  LINK -lib /extract:member-with-a-long-name-alpha.obj helpers.lib
  LINK -lib /remove:beta.obj /out:trimmed.lib helpers.lib
  LINK -lib /list trimmed.lib
  LINK -dump /all helpers.lib
  LINK -dump /all member-with-a-long-name-alpha.obj
  LINK -dump /all hello-debug.dll
  [copy hello-debug.dll to edited.dll]
  LINK -edit /stack:0x180000,0x2000 /heap:0x140000,0x2000 /release edited.dll
  LINK -dump /headers edited.dll

LINK32_DB=27 is this target's input-controlled switch to its buffered file
backend, reaching the pending fopen/read/write/seek/close paths. It also avoids
the original mapped writer's 2 MB zero tail causing its sequential library
listing to report corruption under Wine. No generated archive is trimmed or
patched. Extraction reproduces alpha.obj exactly; trimmed.lib omits beta.obj.
Independent PE inspection confirms the requested stack and heap header values.
The editor emits LNK4058 for checksum calculation unsupported on this OS; this
is recorded original behavior, not a successful checksum-calculation claim.

The dumper and editor are modes inside this same LINK.EXE. This scenario targets
their large independent dispatches and formatters as well as ordinary archive
creation/editing, long member names and buffered file access.

Path toward 99%

These two scenarios target substantial uncovered Windows functionality. Their
coverage gain cannot be predicted honestly before reconstruction/replay. Whole
binary 99% also needs targeted diagnostics and malformed inputs, OMF/ROM cases,
Macintosh/m68k object/resource/relocation paths, and analysis of remaining CRT
and unreachable code. Two natural Windows scenarios cannot establish that
result; measure the residual masked ranges after each completed scenario.

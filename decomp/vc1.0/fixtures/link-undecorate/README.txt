Original-only C++ undecoration fixture, target VC1.0 NT LINK.EXE.

Original CL /nologo /c /Od /G3 /MT /Fonames.obj names.cpp succeeds. Original LINK
-lib /out:names.lib names.obj uses LINK32_DB=27. Frozen names.lib is the exact
input for -dump /symbols /undecorate names.lib. Two independent original dump
runs return0 with exactly5511 stdout bytes and empty stderr; no normalization.

Source covers constructors/destructors and deleting-destructor aliases, inheritance,
virtual methods/vftables, operator+ and conversion operator, overloads, enums,
references, array references, member/function pointers, stdcall/fastcall and
const/volatile qualification. Untouched CL rejected an initial template declaration
with C2143; unsupported syntax was removed before freezing. No template output
coverage is claimed. These are fixture inputs, not reconstruction implementations.

From repository root:
 python3 decomp/vc1.0/fixtures/link-undecorate/capture-original.py
Scratch receipt: ~/.ai-tmp/vc1.0-link/omf-undecorate-capture.

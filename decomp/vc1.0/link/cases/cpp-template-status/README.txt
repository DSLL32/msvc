Old template/name-status formatter inputs

The original CL rejects the template source probe preserved under capture/.
The valid grammar.obj is instead an explicit COFF input:one4-byte .data section,
external symbol names, and a declared string table. It is not compiler output.
The authored names follow the native parser's ? template-scope dispatch and
EOF/status branches. Each string has four NUL bytes so truncated names do not
read into the next symbol string. No object or output repair is applied.

Two explicit original-only LINK -dump /symbols /undecorate captures agree:
1074 stdout bytes, empty stderr, exit0, no normalization. The Box-qualified
spellings remain decorated; incomplete template/function names preserve native
" ?? " markers. Quoted based-segment input also exposes original keyword19,
__segname("; the existing contextual lookup now retains that missing entry.

scripts/build-undecorate-grammar-input.py regenerates the object byte-identically.
Its manifest hash records the version at capture; a later optional symbol-name
CLI extension supports cpp-pointer-status without changing the default object.

Four full bodies were reached and accepted:parse_template, status_write,
name_plus_status and name_assign_status. ECX member methods use existing
ignored-EDX fastcall interfaces with exact original RET4/RET8 cleanup. There is
no new assembly implementation. Native GDB matched first entries and final
frozen streams. capture/frontiers preserves evidence; validation.json references
scratch logs and the shared31-case family receipt. Fixtures remain native-only.

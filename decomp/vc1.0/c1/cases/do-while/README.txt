One-function native reconstruction trial: do-while
================================================

Input: int main () { do { return 0; } while (0); }

This valid minimal-C variant preserves the original minimal case's frontend
arguments and MSC_CMD_FLAGS, including the trailing space. No header is included;
the only external data dependency is the original C1.ERR message catalog, pinned
in manifest.json. The source filename remains test.c. The child has no command
arguments and its original-only launch receives exactly one environment entry:
MSC_CMD_FLAGS. The portable IR prefix is ir.

The immutable reference files were captured by explicit original-only Wine
launches outside fixture execution. A private debugger stopped the original
whose code sections were entirely INT3-masked, except the previous 687 accepted
logical entries and their full shared ranges. The first remaining trap is
original VA 0x330b0, begin_do_statement. Native GDB independently stopped at the
named stub with parse_translation_unit as its caller. The four pending FILE
buffers match raw at that entry, including empty symbol/line buffers.

The complete function is [0x330b0,0x330e3), 51 bytes, cdecl void(void), with no
shared text data. It pushes continue_labels with 5, do_labels with 7 and
break_labels with 6, then emits continue_labels->value.label. Direct callees are
existing push_label_stack and emit_code_label; no new callee or ABI adapter was
introduced. Source lives with other control-statement functions in src/parser.c.

A second masked-original run restored exactly this complete function in
addition to the old prefix. It exits0. Two independent untouched-original runs
also exit 0, with empty stdout/stderr, identical complete file sets and raw
irex 888, irsy 39, irin 3, irgl 50. Native execution reproduces those exact bytes.
No timestamp, pointer or path normalization is permitted for this case.

The entry trap, after-unmask terminal event, exact captures and provenance are
in manifest.json and evidence/. The original-only debugger's source is pinned
by commit/path/hash and its binary by hash; it is not a fixture dependency.
Native GDB logs preserve the initial trap, named entry, all four pending buffers,
label-stack count 0 before and 3 after, and normal exit. Absolute scratch paths in
those logs record where this capture happened, not a required runtime namespace.

Normal native regression:
  make -j4 check CASE=do-while

The case is registered with the ordinary suite and cached original-backed
application trace. Acceptance appends only begin_do_statement, index 687. Its
terminal is exit 0; this case has no next unreconstructed callee. The trial stops
after this one function. validation.json records measured timing and workflow
observations, without assigning a further case.

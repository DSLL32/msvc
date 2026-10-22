RCPP directive-tail and oversized-macro cases

Both cases were captured twice from the untouched 1.0-nt RCPP.EXE in fresh
isolated working directories. Status, raw streams, exact output file set and
all output bytes matched between independent runs. manifest.json pins the
executable, source and oracle hashes. No normalization is permitted.

Copy source/ into a clean working directory. Substitute {bin} and {include}
with the original toolchain paths, clear unset_environment, and invoke the
arguments from manifest.json. Both cases use -E and write result.i. The -ef
argument retains the original child invocation convention; RCPP's diagnostic
messages come from its embedded catalog rather than that C1.ERR file.

directive-tail places unexpected tokens after #endif. Original status is 0,
stdout is empty, stderr contains warning RC4067, and result.i is 70 bytes.
It is intended to reach the currently trapping rc_skip_line function.

oversized-macro passes 240 whitespace-separated words (1,200 bytes) as one
function-macro argument. Original status is 1, stdout is empty, stderr contains
"oversized-macro.rc(2) : fatal error RC10056: " followed by CRLF, and the
partial result.i is 32 bytes. The empty diagnostic message is original behavior.
It is intended to reach the currently unaccepted rc_macro_fatal function.

Capture script: ~/.ai-tmp/vc1.0-idle-cases/rcpp-targeted.py
Capture prefix: ~/.ai-tmp/vc1.0-hello-dll/wine (parent agent's private prefix).
Maintain all 14 prior RCPP cases and the existing graph/ABI requirements.

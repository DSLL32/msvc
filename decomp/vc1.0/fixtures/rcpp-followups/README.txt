RCPP pragma, expression, encoding, option and diagnostic follow-ups

Queued with the RCPP reconstruction worker. Every case is captured from the
untouched original RCPP.EXE; manifest.json pins source bytes, arguments, status,
raw stdout/stderr and exact full/partial outputs. No normalization is allowed.
Copy source/ into an isolated case directory, substitute {bin} and {include}
with original toolchain paths, clear the listed environment variables and run
the selected arguments. -ef names the original C1.ERR message file.

Positive cases cover pragmas, character/escape #if expressions, floating token
lexing, macro stringizing/pasting, #line, -P/-C/-J, -Zp2/-Ze, -CP932 and a
UTF-16LE BOM input. Native output, including preserved pragmas, is the oracle.
Negative cases cover #error, missing includes, unterminated conditionals, macro
arity warnings, unknown escapes and invalid options.

The CP932 backslash-trail input deliberately records the original warning and
newline-in-constant error. The safe CP932 input and UTF-16 input succeed. Do not
replace these observed old-version behaviors with modern preprocessor behavior.
The raw input files are preserved as CP932/UTF-16 bytes, not converted to UTF-8.

A divide-by-zero exploration faults in the original and is excluded from this
stream-oracle suite; its Wine debugger output was not retained as a fixture.
Maintain the prior rich resource case and its graph/ABI requirements. The
user-approved exception remains limited to individually identified RCPP CRT
boundaries, retaining external ABI and observable behavior.

Parent original capture scratch: ~/.ai-tmp/vc1.0-idle-cases/.

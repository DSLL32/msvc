Comment-preserving preprocessing
===============================

Original CL /nologo /C /E test.c produces337 bytes of stdout, no stderr or
artifacts from its C1 child. The source and included header exercise block and
line comments, quotes/backslashes in comments and escaped physical lines.
Two standalone untouched original C1 runs reproduce every byte.

The reconstruction completes at778 accepted entries after whole block/line
comment-copy functions. Three fresh masked original/native observations match,
and all fourteen native regression cases pass; see validation.json.
Normal completed-case regressions remain native-only; no original is recaptured.

RC KANJI, wide-string and child-preprocessor follow-ups

Queued with the RC reconstruction worker. manifest.json records immutable raw
source bytes, original target/child hashes, commands, statuses, streams and
exact complete outputs. No bytes or fields are normalized.

Copy source/ into an isolated working directory, point WINEPATH and INCLUDE
at the original 1.0-nt toolchain, clear the listed inherited compiler options,
and run original RC.EXE with the selected arguments.

kanji:
  /x /l411 /Iinclude /foencoding.res encoding.rc
  Native KANJI directive defines lead-byte ranges 81..9f and e0..fc. Source is
  CP932, with Japanese menu/dialog/string text and escaped wide RCDATA. Both
  source LANGUAGE and command-line default language are Japanese (0x411).
  Original exits 0 with empty streams and exact 480-byte encoding.res.

wide-long:
  /x /l411 /fowide-long.res wide-long.rc
  A 300-character wide string and explicit Japanese Unicode escapes. Original
  exits 0 with empty streams and exact 700-byte output. Do not invent a warning
  merely because a modern tool/version or provisional branch suggests one.

preprocessor-error:
  /x /fofailed.res preprocessor-error.rc
  A missing include makes original RCPP fail; RC must report its own terminating
  RW1016 after the child's RC1015, exit 1, and leave no output/temp files.

RC 3.10 has no general code-page switch in its recovered argument dispatcher.
This suite uses its actual KANJI directive and /l option, preserving original
byte behavior without host locale changes or process-ID overrides. Quiet runs
avoid verbose temporary names whose process-ID component is environmental.
RCPP's separate -CP932 and UTF-16 input cases belong to ../rcpp-followups.

The reconstruction should preserve the native graph/ABIs and all prior cases.
RC continues to invoke original RCPP, independently of the RCPP worker. Once
both finish, composed execution may be checked separately.

Original capture tooling/logs: ~/.ai-tmp/vc1.0-idle-cases/.

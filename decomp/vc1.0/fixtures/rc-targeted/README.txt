RCINCLUDE and scanf-pushback cases, original toolchain 1.0-nt

Queued after resources-encoding. manifest.json pins original/child identities,
raw inputs, commands, exact output hashes and empty streams. No normalization.
Copy source/ into an isolated working directory and run each selected command
with the original toolchain environment, clearing inherited compiler options.

rcinclude:
  RC.EXE /x /forcinclude.res rcinclude.rc
  Mixed-case native RCINCLUDE, leading/trailing whitespace, two included files,
  an ordinary #include inside one, and shared macros. Native RCINCLUDE takes a
  bare filename token, unlike #include's quoted form. Original exits 0 and
  produces exact 344-byte rcinclude.res. Independent record inspection verifies
  menu200, RCDATA202 and both STRINGTABLE blocks, plus the mandatory null record.
  Targets native trim_token and its current boundary handling.

scanf-pushback:
  RC.EXE /x /l411z /fopushback.res pushback.rc
  The original hex scanf consumes 411 then pushes back non-EOF 'z'; RC accepts
  the trailing text. Original exits 0 and produces exact 68-byte resource with
  language 0x411. This exercises the old CRT ungetc branch not reached by the
  earlier /l411 case. Preserve the original parsing behavior.

Both cases have empty stdout/stderr and exactly the expected file set after
temporary cleanup. The manifest and validation.json record captured evidence.
RC continues to use original RCPP. Parent capture scratch/tooling is under
~/.ai-tmp/vc1.0-idle-cases/.

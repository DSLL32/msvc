These two successful cases were chosen by auditing the actual unaccepted executable-byte complement, rather than only the named stubs. Both were captured twice against the untouched original RCPP.EXE before accepting their pending functions. Each original exited zero with empty stdout/stderr and identical raw products. `long-suffix` produces 408 bytes; `unsigned-suffix` produces 486 bytes. Both retain the complete unchanged source tree and create only `result.i`.

The cases use zero, one, hexadecimal boundary values `0x7fffffff`, `0x80000000`, `0xfffffffe` and `0xffffffff`, uppercase/lowercase suffixes, macros, and ordinary arithmetic, bitwise, shift and comparison `#if` expressions. Values are expressed in hexadecimal where necessary to retain the target's successful integer-token behavior. The original treats high-bit `U` constants as signed values in its preprocessor evaluator; the captured `unsigned_high_bit_signed_expression` and `unsigned_all_bits_minus_one` lines preserve that behavior.

Masked original and reconstruction reached `_rc_long_kind@4` together for the first case and `_rc_unsigned_kind@4` together for the second. Their complete extents are respectively `[0100cb80,0100cb91)` (17 bytes) and `[0100cba0,0100cbb5)` (21 bytes), with no alignment or alias bytes counted. The original status-zero captures are the oracles. The separately labelled `frontier-*.events` files record intentional masking/debugger stops, not native failure outcomes.

`manifest.json` pins all source, target/dependency, raw stream/output and capture hashes. No bytes or fields are normalized. The normal inherited environment is used with CL, _CL_, LINK, _LINK_ and MSC_CMD_FLAGS cleared; T: maps to the original 1.0-nt toolchain. The argument lists remain relative and unchanged.

Reproduce original captures after building the normal RCPP analysis debugger:

```
make -C decomp/vc1.0/rcpp -j2 all
python3 decomp/vc1.0/fixtures/rcpp-remaining/capture.py
```

`baseline-coverage.json` preserves the pre-case accepted union. `gap-map.json` and `GAP-AUDIT.md` retain the complete static gap/caller accounting; `gap-audit.py` can regenerate that accounting against the original without invoking it. The scope was narrowed to read-only coverage analysis and successful preprocessing cases after automatic safety review stopped the proposed exception-dispatch investigation. No exception, crash, dispatch or resource-failure experiment was created or run for this fixture.

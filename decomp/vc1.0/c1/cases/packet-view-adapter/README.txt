Typed packet view and callback adapter
=====================================
One successful /Z7 /Od /G5 /W4 unit exercises qualified data views, nested
pointer/array typedefs and correctly typed stdcall callbacks. Incompatible
object-pointer conversion is used solely for unsigned-char access to the first
four payload bytes. A pointer discarding const is only read; all flag accesses
retain volatile. No padding, addresses or incompatible callback calls enter
the checksum. Unsigned negation uses defined modular arithmetic.

Untouched original full compile/link and checksum execution pass. Its exact
warnings include C4133, C4090, C4146 and two C4221 automatic-address initializer
warnings. Native fixtures will replay captured C1 inputs only; backend/runtime
validation is original-only evidence, not reconstructed backend execution.

Complete at969 entries: three whole functions,296frontend extent bytes.
All37 native phases,41rejections, six graph/ABI/mask checks, three flag
mutations,289state/alias checks and both grammars pass. validation.json binds
all measured repetitions/failures, raw extents and compiled shared inputs.
Original cache symbol-bit0x800 branch reads an unwritten local; it is preserved
and unvisited. Provisional prefix helpers remain traps.

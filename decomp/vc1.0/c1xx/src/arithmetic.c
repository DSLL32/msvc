/* Integer-width bounds used by the frontend's addition checks. */
#include <limits.h>
#include "class-members.h"
#include <recon.h>

/* Original 0xa3860/0xa3878/0xa3890. Widths one, two and four carry the
   target integer bounds; the other encountered slots are reserved zeroes. */
const unsigned addition_unsigned_max[6] = { 0, UCHAR_MAX, USHRT_MAX, 0, ULONG_MAX, 0 };
const int addition_signed_max[6] = { 0, SCHAR_MAX, SHRT_MAX, 0, LONG_MAX, 0 };
const int addition_signed_min[6] = { 0, SCHAR_MIN, SHRT_MIN, 0, LONG_MIN, 0 };

/* Original 0x74830..0x74940, stdcall 16. Compare magnitudes before
   multiplying, avoiding a target-width overflowing product. The signed
   opposite-sign switch deliberately has no initialized fallback in Win32;
   normal compiler integer widths are 1, 2 and 4. Native execution rejects
   that original stale-local path for malformed/internal descriptors. */
int __stdcall multiplication_overflows(unsigned left, unsigned right,
    short unsigned_arithmetic, unsigned short bytes)
{
    unsigned left_magnitude, right_magnitude, threshold;
    if (unsigned_arithmetic) {
        threshold = addition_unsigned_max[bytes];
        if (!left) return 0;
        return threshold / left < right;
    }
    left_magnitude = (int)left < 0 ? 0U - left : left;
    right_magnitude = (int)right < 0 ? 0U - right : right;
    if (((int)left > 0) != ((int)right > 0)) {
        switch (bytes) {
        case 1:
            threshold = (unsigned char)(0U - (unsigned)addition_signed_min[bytes]);
            break;
        case 2:
            threshold = (unsigned short)(0U - (unsigned)addition_signed_min[bytes]);
            break;
        case 4:
            threshold = 0U - (unsigned)addition_signed_min[bytes];
            break;
#ifdef RECON_NATIVE_TEST
        default:
            /* The original loads an unwritten local at 0x748cc. Never
               substitute an unrelated native stack byte or guessed value. */
            __asm { int 3 }
#endif
        }
    } else threshold = addition_signed_max[bytes];
    if (!left_magnitude) return 0;
    return right_magnitude > threshold / left_magnitude;
}

/* Original 0x74950..0x74a03. Both subtraction paths retain the machine's
   32-bit wrap; signedness affects the final comparisons, not that arithmetic. */
int __stdcall addition_overflows(int left, unsigned right, short unsigned_arithmetic, unsigned short bytes)
{
    int threshold;
    if (unsigned_arithmetic)
        return addition_unsigned_max[bytes] - (unsigned)left < right;
    if ((left < 0) != ((int)right < 0)) return 0;
    if (left > 0) {
        threshold = (int)((unsigned)addition_signed_max[bytes] - (unsigned)left);
        return (int)right > threshold;
    }
    threshold = (int)((unsigned)addition_signed_min[bytes] - (unsigned)left);
    return (int)right < threshold;
}

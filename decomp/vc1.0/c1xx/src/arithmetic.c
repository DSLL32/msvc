/* Integer-width bounds used by the frontend's addition checks. */
#include <limits.h>
#include "class-members.h"
#include <recon.h>

/* Original 0xa3860/0xa3878/0xa3890. Widths one, two and four carry the
   target integer bounds; the other encountered slots are reserved zeroes. */
const unsigned addition_unsigned_max[6] = { 0, UCHAR_MAX, USHRT_MAX, 0, ULONG_MAX, 0 };
const int addition_signed_max[6] = { 0, SCHAR_MAX, SHRT_MAX, 0, LONG_MAX, 0 };
const int addition_signed_min[6] = { 0, SCHAR_MIN, SHRT_MIN, 0, LONG_MIN, 0 };

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

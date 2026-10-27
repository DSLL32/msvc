#include <stdint.h>
#include <recon-tool.h>
#include <recon.h>
extern int __cdecl integer_difference_out_of_range(long long, unsigned long long, unsigned short, unsigned short);
static int signed_difference_edges(void)
{
    const long long maximum = 0x7fffffffffffffffLL;
    const long long minimum = -maximum - 1;
    return !integer_difference_out_of_range(8, 1, 0, 4) &&
        integer_difference_out_of_range(2147483647LL, (unsigned long long)-1LL, 0, 4) &&
        integer_difference_out_of_range(-2147483647LL-1, 1, 0, 4) &&
        !integer_difference_out_of_range(-10, 10, 0, 4) &&
        integer_difference_out_of_range(maximum, (unsigned long long)-1LL, 0, 8) &&
        integer_difference_out_of_range(minimum, 1, 0, 8) &&
        !integer_difference_out_of_range(-1, 1, 1, 4);
}
const ReconTest initializer_tests[1] = {
    { "signed difference width edges and unsigned bypass", 1, signed_difference_edges }
};

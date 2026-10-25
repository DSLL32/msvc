#include <recon-tool.h>
#include "extended.h"
#include <recon.h>
static int extended_integer(void)
{
    int value=42, result=0; Extended number;
    integer_to_extended(&value,&number);
    if(number.low!=0||number.high!=0xa8000000U||number.exponent!=0x4004||number.format!=4)return 0;
    return !extended_to_integer(&number,&result)&&result==42;
}
static int extended_double(void)
{
    DoubleBits bits={0,0x3ff40000}, result={0,0}; Extended number;
    double_to_extended(&bits,&number);
    if(extended_to_double(&number,&result))return 0;
    return result.low==bits.low&&result.high==bits.high;
}

const ReconTest c2_tests[] = {
    { "raw x87 integer/store private ABI roundtrip", 1, extended_integer },
    { "exact-double extended conversion", 1, extended_double },
};

#include "pair.h"
Pair __stdcall return_pair(unsigned value)
{
    Pair result;
    result.words[0] = value;
    result.words[1] = 0x55667788;
    return result;
}

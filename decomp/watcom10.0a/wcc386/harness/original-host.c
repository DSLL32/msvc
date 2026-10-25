/* Link host for period-compiler ABI checks only; not reconstructed App main. */
#include "memory.h"
#include <recon.h>
int main(void)
{
    return MemAlloc(8) != 0;
}

#include "c2.h"
#include <recon.h>

C2Real c2_real_zero, c2_real_one, c2_real_two;

void c2_initialize_constants(void)
{
    int value = 0;
    c2_real_from_int(&value, &c2_real_zero);
    value = 1;
    c2_real_from_int(&value, &c2_real_one);
    value = 2;
    c2_real_from_int(&value, &c2_real_two);
}

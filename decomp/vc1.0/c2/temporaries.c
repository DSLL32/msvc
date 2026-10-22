#include "temporaries.h"
TemporaryRegister *temporary_registers;
/* Original [00058530,00058540). */
void reset_temporary_registers(void)
{
    temporary_registers = 0;
}

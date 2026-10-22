/* Register-variable records shared by global and machine allocation. */
#include "register-variables.h"
/* Original [0007c730,0007c8b0). Constructor/lookup indexing selects a
   128-byte table from the register's class bits (00, 4000 or 8000), then
   selects one of 32 hash heads with its low five bits. The reset clears
   exactly 96 pointers; record contents remain owned by their arena. */
RegisterVariable *register_variable_buckets[REGISTER_VARIABLE_CLASSES][REGISTER_VARIABLE_BUCKETS];

/* Original [0002db30,0002db50). */
void reset_global_register_map(void)
{
    unsigned group, bucket;
    for (group = 0; group < REGISTER_VARIABLE_CLASSES; ++group)
        for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket)
            register_variable_buckets[group][bucket] = 0;
}

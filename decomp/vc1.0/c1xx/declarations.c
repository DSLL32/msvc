/* Declaration lookup uses the first word of its declaration records as a key. */
#include "declarations.h"

HashTable *declaration_table; /* original 0x9f340 */

/* Original 0x31f00..0x31f13. */
void initialize_declaration_table(void)
{
    declaration_table = new_hash_table(16, 0, HASH_FIRST_WORD, EQUAL_FIRST_WORD);
}

#include "case.h"
/* First block, with "quotes", a backslash \ and slash /.
   Second physical line. */
int value = SCALE(4); // first line comment
// A continued line comment \
still comment text
int after = value + \
1; /* final block */

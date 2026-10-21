/* Diagnostic vocabulary used by message_text, original table 0x6f4b8. */
#include "c1.h"

typedef struct MessageText {
    unsigned code;
    char *text;
} MessageText;

static MessageText message_vocabulary[] = {
    { 301, "left" },
    { 302, "right" },
    { 303, "UNNAMED" },
    { 304, "<unnamed>" },
    { 305, "trailing ',' used for variable argument list" },
    { 313, "warning" },
    { 314, "error" },
    { 315, "fatal error" },
    { 322, "operand" },
    { 323, "opcode" },
    { 333, "anonymous union" },
    { 336, "private" },
    { 337, "protected" },
    { 339, "compiler intermediate" },
    { 340, "compiler generated" },
    { 341, "source" },
    { 342, "inline function definition" },
    { 343, "include" },
    { 344, "precompiled header" },
    { 345, "overloaded function type" },
    { 350, "Using precompiled header" },
    { 351, "Creating precompiled header" },
    { 352, "Creating new precompiled header" },
    { 0, 0 }
};

/* Original 0x4c350..0x4c372. */
char *message_text(unsigned code)
{
    MessageText *entry = message_vocabulary;
    while (entry->code && entry->code != code)
        ++entry;
    return entry->text;
}

/* MSC_CMD_FLAGS argv extension, preserving original quote and DBCS rules. */
#include "options.h"
#include "characters.h"

/* Original 0x73c60..0x73d2a. */
void read_env_arguments(char *text)
{
    int quoted = 0;
    char **slot;
    char *output;
    unsigned char c;
    if (!text)
        return;
    slot = extend_argument_vector();
    while (*text) {
        while (*text == ' ')
            ++text;
        if (!*text)
            continue;
        output = text;
        *slot++ = text;
        ++argument_count;
        while (*text) {
            c = *text;
            if (c == ' ' && !quoted)
                break;
            switch (character_class[c]) {
            case CC_ESCAPE_OR_END:
                if (c == '?')
                    goto emit;
                c = *++text;
                if (c == '"')
                    goto emit;
                *output++ = '\\';
                if (!c || (c == ' ' && !quoted))
                    goto end_argument;
                goto emit;
            case CC_QUOTE:
                quoted = !quoted;
                ++text;
                continue;
            case CC_LEAD_BYTE:
                *output++ = c;
                c = *++text;
                goto emit;
            }
        emit:
            *output++ = c;
            ++text;
        }
    end_argument:
        if (*text)
            ++text;
        *output = 0;
    }
    *slot = 0;
}

/* Original 0x73c00..0x73c59. */
char **extend_argument_vector(void)
{
    char **source, **end;
    if (!argument_vector_allocation) {
        end = argument_vector_allocation = checked_malloc(800);
        source = argument_vector;
        while ((*end = *source++) != 0)
            ++end;
        argument_vector = argument_vector_allocation;
    } else {
        end = argument_vector;
        while (*end)
            ++end;
    }
    return end;
}

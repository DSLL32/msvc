/* MSC_CMD_FLAGS argv extension, preserving original quote and DBCS rules. */
#include "options.h"
#include "characters.h"

/* Original 0x4b4b0..0x4b57a. */
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
        ++driver_argc;
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

/* Original 0x4b450..0x4b4a9. */
char **extend_argument_vector(void)
{
    char **source, **end;
    if (!argument_vector_allocation) {
        end = argument_vector_allocation = checked_malloc(800);
        source = driver_argv;
        while ((*end = *source++) != 0)
            ++end;
        driver_argv = argument_vector_allocation;
    } else {
        end = driver_argv;
        while (*end)
            ++end;
    }
    return end;
}

/* Original 0x2d420..0x2d443. */
char *next_argument(void)
{
    if (--driver_argc <= 0)
        return 0;
    return *++driver_argv;
}

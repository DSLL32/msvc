#include "c2.h"
#include <recon.h>

char *c2_current_option, *c2_option_operand;

int __fastcall c2_match_option(C2Option *table, char *input,
                              char *(__cdecl *next)(void), int mode)
{
    char *text, *pattern, *value;
    if (!input)
        return 0;
    c2_current_option = input;
    for (; table->kind; ++table) {
        text = input;
        pattern = table->pattern;
        for (;;) {
            switch (*pattern) {
            case '\0':
                if (*text)
                    goto mismatch;
                value = NULL;
                if (table->kind & 0x20) {
                    value = next();
                    c2_option_operand = value;
                }
                c2_apply_option(table, value, mode);
                return table->result;
            case '#':
                value = text;
                if (!*value) {
                    value = next();
                    c2_option_operand = value;
                }
                c2_apply_option(table, value, mode);
                return table->result;
            case '*':
                if (!*text || !c2_wildcard_match(pattern, text, table->kind))
                    goto mismatch;
                c2_apply_option(table, text, mode);
                return table->result;
            case '-':
                if (*text != '-')
                    goto mismatch;
                ++text;
                ++pattern;
                break;
            default:
                if (*pattern++ != *text++)
                    goto mismatch;
                break;
            }
        }
    mismatch: ;
    }
    return 0;
}

int __fastcall c2_wildcard_match(char *pattern, char *input, unsigned kind)
{
    char *tail = pattern + 1, *end = input;
    if (!*tail)
        return 1;
    while (tail[1])
        ++tail;
    while (end[1])
        ++end;
    while (*tail == *end) {
        --tail;
        --end;
    }
    return tail == pattern && end >= input;
}

void __fastcall c2_apply_option(C2Option *option, char *value, int mode)
{
    C2Selector *selectors, *selector;
    struct StringList { int remaining; char *values[1]; } *list;
    char *pattern;
    switch (option->kind) {
    case OPT_ENABLE:
        *(int *)option->value = 1;
        break;
    case OPT_DISABLE:
        *(int *)option->value = 0;
        break;
    case OPT_STRING_ONCE:
        if (*(char **)option->value) {
            pattern = _strdup(option->pattern);
            pattern[strlen(pattern)-1] = '\0';
            c2_fatal(93, pattern, *(char **)option->value, value);
            break;
        }
        /* First occurrence uses the ordinary string assignment. */
    case OPT_STRING:
        *(char **)option->value = mode ? _strdup(value) : value;
        break;
    case OPT_INTEGER:
        *(int *)option->value = parse_option_integer(value);
        break;
    case OPT_SELECTOR:
        selectors = (C2Selector *)option->value;
        while (*value) {
            for (selector = selectors; selector->selector; ++selector)
                if (*value == (char)selector->selector)
                    break;
            if (selector->selector) {
                switch (selector->kind) {
                case OPT_ENABLE:
                    *(int *)selector->value = 1;
                    break;
                case OPT_DISABLE:
                    *(int *)selector->value = 0;
                    break;
                case OPT_INTEGER:
                    *(int *)selector->value = parse_option_integer(value);
                    while (*value >= '0' && *value <= '9')
                        ++value;
                    --value;
                    break;
                default:
                    c2_report_internal("E:\\utc\\src\\\\Common\\getflags.c", 211);
                    break;
                }
            } else
                c2_fatal(48, (int)*value, c2_current_option);
            ++value;
        }
        break;
    case OPT_STRING_LIST:
        list = (struct StringList *)option->value;
        if (list->remaining > 0)
            list->values[--list->remaining] = mode ? _strdup(value) : value;
        else
            c2_fatal(94, option->pattern, value);
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\Common\\getflags.c", 221);
        break;
    }
}

/* 0040393a..00403b49: canonical option signature for minimal-rebuild records. */
#include "cl.h"
#include <recon.h>

char *option_signature(Driver *state)
{
    char *quoted = allocate(1024), *absolute = allocate(1024);
    char **items, **next_item;
    char *out, *result, *end, *include;
    Option *option;
    int count, used, length, total, i;
    unsigned char selector;
    count = state->include_path && use_standard_include ? 1 : 0;
    for (option = state->options; option; option = option->next)
        if (option_matching_selector(option, "Mm"))
            ++count;
    items = allocate(count * sizeof(char *));
    next_item = items;
    used = total = 0;
    for (option = state->options; option; option = option->next) {
        selector = (unsigned char)option_matching_selector(option, "Mm");
        if (!selector)
            continue;
        option_buffer[0] = '-';
        end = append_text(option_buffer + 1, option->name);
        if (option->value) {
            if (selector == 'm') {
                if (!_fullpath(absolute, option->value, 1024))
                    strcpy(absolute, option->value);
                _mbslwr((unsigned char *)absolute);
                quote_argument(quoted, absolute);
            } else
                quote_argument(quoted, option->value);
            append_text(end, quoted);
        }
        *next_item++ = duplicate(option_buffer);
        ++used;
        total += strlen(option_buffer) + 1;
    }
    if (state->include_path && use_standard_include) {
        length = quoted_length(state->include_path) + 6;
        include = allocate(length);
        strcpy(include, "-inc=");
        quote_argument(include + 5, state->include_path);
        _mbslwr((unsigned char *)include);
        total += length;
        items[used] = include;
    }
    qsort(items, count, sizeof(char *), compare_option_strings);
    result = out = allocate(total + 1);
    for (i = 0; i < count; ++i) {
        out = append_text(out, items[i]);
        *out++ = ' ';
    }
    out[-1] = 0;
    for (i = 0; i < count; ++i)
        release(items[i]);
    release(items);
    release(absolute);
    release(quoted);
    return result;
}

/* 004010f7..00401118: first matching selector byte, zero if absent. */
int option_matching_selector(Option *option, const char *selectors)
{
    const char *match = strpbrk(option->matching, selectors);
    return match ? (unsigned char)*match : 0;
}

/* 00403b49..00403b5d: comparator for the array of signature strings. */
int compare_option_strings(const void *left, const void *right)
{
    return strcmp(*(char * const *)left, *(char * const *)right);
}

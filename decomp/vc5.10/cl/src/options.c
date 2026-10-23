/* Effective compiler-pass option records and their ownership. */
#include "cl.h"
#include <recon.h>

Option *free_options;

/* 00401392..004013ef: reuse nodes, retaining private copies of each field. */
Option *new_option(char *name, char *value, char *matching, unsigned flags)
{
    Option *option;
    if (free_options) {
        option = free_options;
        free_options = option->next;
    } else
        option = allocate(sizeof(Option));
    memset(option, 0, sizeof(Option));
    option->flags = flags;
    option->value = duplicate(value);
    option->name = duplicate(name);
    option->matching = duplicate(matching);
    return option;
}

/* 00401664..00401677: append a node or a complete chain without copying. */
void append_option(Option **head, Option *option)
{
    while (*head)
        head = &(*head)->next;
    *head = option;
}

/* 00401552..00401664: expand comma-separated per-pass option specifications. */
Option *expand_options(char *pattern, Option *context)
{
    Option *head = NULL;
    char name[32], passes[16], *out, *payload;
    static char value[1024];
    unsigned flags;
    passes[0] = '\0';
    while (*pattern) {
        flags = 0;
        pattern = skip_space(pattern);
        if (!*pattern)
            break;
        if (*pattern != '=' && *pattern != '#' && *pattern != '-' && *pattern != '~') {
            out = passes;
            while (*pattern != '=' && *pattern != '#' && *pattern != '-' && *pattern != '~')
                *out++ = *pattern++;
            *out = '\0';
        }
        if (*pattern == '#') flags = 8;
        else if (*pattern == '-') flags = 1;
        else if (*pattern == '~') flags = 32;
        pattern = expand_word(pattern + 1, name, context->name);
        if (*pattern == '!' || *pattern == ':') {
            if (*pattern == '!')
                flags |= 4;
            pattern = expand_word(pattern + 1, value, context->value);
            payload = strlen(value) ? value : NULL;
        } else
            payload = NULL;
        if (*pattern == ',')
            ++pattern;
        append_option(&head, new_option(name, payload, passes, flags));
    }
    return head;
}

/* 0040148d..00401552: substitution and delimiters in a rule's output word. */
char *expand_word(char *pattern, char *buffer, char *replacement)
{
    char *out = buffer, *alternative = NULL;
    unsigned character;
    *out = '\0';
    while (*pattern) {
        character = (unsigned char)*pattern++;
        switch (character) {
        case '!': case ')': case ',': case ':': case '|':
            --pattern;
            goto complete;
        case '@':
            break;
        case '(':
            *out = '\0';
            pattern = expand_alternative(pattern - 1, out, replacement, &alternative);
            out = buffer + strlen(buffer);
            break;
        case '*':
            *out = '\0';
            if (show_commands)
                out = append_literal(buffer, replacement);
            else
                out = append_escaped(buffer, replacement);
            break;
        case '<':
            pattern = apply_extension(pattern, buffer, alternative);
            out = buffer + strlen(buffer);
            break;
        case '\\':
            *out++ = *pattern++;
            break;
        default:
            *out++ = (char)character;
            break;
        }
    }
complete:
    *out = '\0';
    return pattern;
}

/* 00401ed0..00401f70: detach matches, retaining protected default origins. */
Option *detach_options(Option **head, Option *patterns, Option **defaults)
{
    Option *removed = NULL, *option, *next, *origin, *prior;
    for (; patterns; patterns = patterns->next) {
        for (option = *head; option; option = next) {
            next = option->next;
            if (!options_match(option, patterns))
                continue;
            if (defaults && (((patterns->flags & 1) && !(option->flags & 33)) ||
                ((patterns->flags & 32) && !(option->flags & 32)))) {
                origin = option->origin;
                if (!origin)
                    continue;
                for (prior = *defaults; prior && prior->origin != origin; prior = prior->next)
                    ;
                if (!prior)
                    append_option(defaults, copy_option(origin));
            } else
                append_option(&removed, unlink_option(head, option));
        }
    }
    return removed;
}

/* 00401e1b..00401e35 */
void delete_options(Option *options)
{
    Option *next;
    while (options) {
        next = options->next;
        delete_option(options);
        options = next;
    }
}

/* 00401677..00401743: names and trailing-? argument wildcards. */
int options_match(Option *option, Option *pattern)
{
    int length, pattern_length;
    if (strcmp(option->name, "?") && strcmp(pattern->name, "?") &&
        strcmp(option->name, pattern->name))
        return 0;
    if (!option->value)
        return !pattern->value || !strcmp(pattern->value, "?");
    if (!pattern->value)
        return !strcmp(option->value, "?");
    length = strlen(option->value);
    pattern_length = strlen(pattern->value);
    if (length >= 1 && option->value[length - 1] == '?')
        return !strncmp(option->value, pattern->value, length - 1);
    if (pattern_length >= 1 && pattern->value[pattern_length - 1] == '?')
        return !strncmp(option->value, pattern->value, pattern_length - 1);
    return !strcmp(option->value, pattern->value);
}

/* 00401e35..00401e64: release payloads and recycle the fixed-size node. */
void delete_option(Option *option)
{
    release(option->value);
    release(option->name);
    release(option->matching);
    option->next = free_options;
    free_options = option;
}

/* 00401440..0040148d: retain the alternative for later extension handling. */
char *expand_alternative(char *pattern, char *out, char *replacement, char **alternative)
{
    static char fallback[32];
    pattern = expand_word(pattern + 1, out, replacement);
    pattern = expand_word(pattern + 1, fallback, replacement);
    if (!*out)
        strcpy(out, fallback);
    *alternative = fallback;
    return pattern + 1;
}

/* 00402933..00402ab5: disable options whose required alternatives are absent. */
void finish_options(void)
{
    Option *option, *wanted, *node, *emitted;
    OptionRule *rule;
    char *copy, *part, *next, *spelling, *requirements;
    for (option = driver->other_options; option; option = option->next) {
        rule = (OptionRule *)option->origin;
        if ((option->flags & 16) || !rule->extra || !rule->add)
            continue;
        copy = duplicate((char *)rule->extra);
        part = copy;
        do {
            next = (char *)_mbschr((unsigned char *)part, ';');
            if (next)
                *next++ = '\0';
            wanted = expand_options(part, option);
            if (!has_any_option(driver->other_options, wanted)) {
                if (!(option->flags & 1)) {
                    spelling = allocate(80);
                    requirements = allocate(80);
                    spelling[0] = '/';
                    strcpy(spelling + 1, option->name);
                    requirements[0] = '\0';
                    for (node = wanted; node; node = node->next) {
                        if (node != wanted)
                            strcat(requirements, load_string(node->next ? 320 : 321));
                        strcat(requirements, "/");
                        strcat(requirements, node->name);
                    }
                    warning(4007, spelling, requirements);
                    release(requirements);
                    release(spelling);
                }
                emitted = expand_options(rule->add, option);
                delete_options(detach_options(&driver->options, emitted, NULL));
                delete_options(emitted);
                next = NULL;
            }
            delete_options(wanted);
            part = next;
        } while (part);
        release(copy);
    }
}

/* 00402ab5..00402af4: any non-superseded original option may satisfy a rule. */
int has_any_option(Option *options, Option *patterns)
{
    Option *pattern;
    for (; options; options = options->next)
        for (pattern = patterns; pattern; pattern = pattern->next)
            if (options_match(options, pattern) && !(options->flags & 16) &&
                !(pattern->flags & 16))
                return 1;
    return 0;
}

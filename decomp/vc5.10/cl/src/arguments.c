/* Command-line word splitting, 00404cd1..00404ed1. */
#include "cl.h"
#include <recon.h>

char **split_arguments(char *text, int remove_quotes)
{
    unsigned char *source, *out, *buffer;
    char **arguments;
    int count = 0, used, i, quoted = 0;
    unsigned slashes, doubled_quotes;
    int copy_character;

    if (!text || !*(source = (unsigned char *)skip_space(text)))
        return NULL;
    buffer = out = allocate(strlen((char *)source) + 1);
    while (*source) {
        do {
            doubled_quotes = 0;
            slashes = 0;
            copy_character = 1;
            while (!_mbsncmp(source, (unsigned char *)"\\", 1)) {
                source = _mbsinc(source);
                ++slashes;
            }
            if (!_mbsncmp(source, (unsigned char *)"\"", 1)) {
                if (!(slashes & 1)) {
                    if (quoted && !_mbsncmp(_mbsinc(source), (unsigned char *)"\"", 1)) {
                        source = _mbsinc(source);
                        if (!remove_quotes)
                            doubled_quotes = 1;
                    } else
                        copy_character = !remove_quotes;
                    quoted = !quoted;
                }
                if (remove_quotes)
                    slashes >>= 1;
            }
            while (slashes--) {
                _mbccpy(out, (unsigned char *)"\\");
                out = _mbsinc(out);
            }
            while (doubled_quotes--) {
                _mbccpy(out, (unsigned char *)"\"");
                out = _mbsinc(out);
            }
            if (!*source || (!quoted && _ismbcspace(*source)))
                break;
            if (copy_character) {
                _mbccpy(out, source);
                out = _mbsinc(out);
            }
            source = _mbsinc(source);
        } while (1);
        *out++ = '\0';
        ++count;
        source = (unsigned char *)skip_space((char *)source);
    }
    arguments = allocate((count + 1) * sizeof(char *));
    source = buffer;
    used = 0;
    for (i = 0; i < count; ++i) {
        source = (unsigned char *)skip_space((char *)source);
        if (*source)
            arguments[used++] = (char *)source;
        source += strlen((char *)source) + 1;
    }
    arguments[used] = NULL;
    return arguments;
}

/* Count a null-terminated argument vector, 00402136..0040214f. */
int count_arguments(char **args)
{
    int count = 0;
    if (args)
        while (*args++)
            ++count;
    return count;
}

/* 0040214f..004022e9: command-line switches and the first response-file line. */
int has_nologo(char **args, int count)
{
    int found = 0, i;
    char line[1024], *argument, *match;
    FILE *file;
    for (i = 0; i < count; ++i) {
        argument = args[i];
        if ((*argument == '-' || *argument == '/') &&
            !strncmp(argument + 1, "nologo", 6)) {
            if (!argument[7]) {
                found = 1;
                no_logo = 1;
            } else if (argument[7] == '-' && !argument[8]) {
                no_logo = 0;
                found = 1;
            }
        } else if (*argument == '@') {
            file = fopen(argument + 1, "r");
            if (file) {
                if (fgets(line, 1023, file)) {
                    match = strstr(line + 1, "nologo");
                    while (match) {
                        if ((match[-1] == '-' || match[-1] == '/') &&
                            (match == line + 1 || _mbschr((unsigned char *)" \t\"", (unsigned char)match[-2]))) {
                            if (!match[6] || _mbschr((unsigned char *)" \t\"\n", (unsigned char)match[6])) {
                                found = 1;
                                no_logo = 1;
                            } else if (match[6] == '-' &&
                                (!match[7] || _mbschr((unsigned char *)" \t\"\n", (unsigned char)match[7]))) {
                                no_logo = 0;
                                found = 1;
                            }
                        }
                        match = strstr(match + 1, "nologo");
                    }
                }
                fclose(file);
            }
        }
    }
    return found;
}

int source_count;
int options_after_link, link_warning;

/* 00401000..00401063: recognize source and linker-input extensions. */
int classify_input(char *name)
{
    char extension[_MAX_EXT];
    int kind;
    _splitpath(name, NULL, NULL, NULL, extension);
    if (*extension) {
        for (kind = 0; input_kinds[kind].category != 2; ++kind)
            if (!_stricmp(extension + 1, input_kinds[kind].extension))
                return kind;
    }
    return 8;
}

/* 00401094..004010d8: append an input and count source-language inputs. */
void add_input(char *name, int kind, int explicit_kind)
{
    Input *input = create_input(name, kind);
    Input **tail = &driver->inputs;
    input->explicit_kind = (unsigned short)explicit_kind;
    while (*tail)
        tail = &(*tail)->next;
    *tail = input;
    if (!input_kinds[kind].category)
        ++source_count;
}

/* 00401aed..00401be7: response files, input names and driver options. */
void parse_arguments(char **args, int count)
{
    int index = 0;
    char *argument;
    int previous;
    SpecialOption *special;
    while (index < count) {
        argument = args[index];
        if (*argument == '@') {
            read_response(argument + 1);
            ++index;
        } else {
            previous = source_count;
            if (*argument != '-' && *argument != '/') {
                add_input(argument, classify_input(argument), 0);
                ++index;
            } else {
                special = find_special(argument + 1);
                if (special && argument[1 + strlen(special->name)]) {
                    ++index;
                    apply_special(special, argument);
                } else
                    match_option(args, &index);
                if (source_count && previous == source_count)
                    options_after_link = 1;
            }
            if (options_after_link && previous != source_count && !link_warning) {
                warning(4026);
                link_warning = 1;
            }
        }
    }
}

/* 00402006..0040203f: grouped switches whose suboptions may be combined. */
SpecialOption *find_special(char *name)
{
    static SpecialOption groups[] = {
        { "EH", "a-:c-:s-" },
        { "G", "3:4:5:6:d:e:f:h*:i-:m-:p#:r:s*:t#:x-:y:z:A:B:D:E*:M:R-:X-" },
        { "O", "1:2:a-:b#:d:g-:i-:p-:s:t:w-:x:y-:V#" },
        { "Z", "7:a:d:e:g:i:l:m#:n:p#:s:B*:M-" },
        { NULL, NULL }
    };
    int i;
    for (i = 0; groups[i].pattern; ++i)
        if (has_prefix(groups[i].name, name))
            return &groups[i];
    return NULL;
}

/* 00402110..00402136: bytewise, case-sensitive prefix recognition. */
int has_prefix(char *prefix, char *text)
{
    while (*prefix)
        if (*prefix++ != *text++)
            return 0;
    return 1;
}

/* 00401be7..00401e1b: apply one rule and retain its effective options. */
int match_option(char **args, int *index)
{
    OptionRule *rule;
    Option *option, *added = NULL, *patterns, *removed, *defaults, *discard;
    Option *node, *previous;
    rule = find_rule(args, *index);
    if (!rule) {
        append_option(&unrecognized_options, new_option(args[*index], NULL, NULL, 0));
        ++*index;
        return 0;
    }
    option = read_option(rule, args, index);
    if (!option)
        return 1;
    if (option->flags & 2) {
        append_option(&unrecognized_options, option);
        return 0;
    }
    option->origin = (Option *)rule;
    append_option(&driver->other_options, option);
    if (rule->add) {
        added = expand_options(rule->add, option);
        if (added) {
            defaults = NULL;
            delete_options(detach_options(&driver->options, added, &defaults));
            delete_options(defaults);
            for (node = added; node; node = node->next)
                node->origin = option;
        }
    }
    if (rule->replace) {
        patterns = expand_options(rule->replace, option);
        defaults = NULL;
        removed = detach_options(&driver->options, patterns, &defaults);
        previous = NULL;
        for (node = removed; node; node = node->next) {
            if (node->origin)
                node->origin->flags |= 16;
            if (!(node->flags & 33)) {
                if (!previous || previous != node->origin)
                    option_conflict(4025, node, option);
                previous = node->origin;
            }
        }
        delete_options(removed);
        delete_options(patterns);
        for (node = defaults; node; node = node->next) {
            if (node->origin && node->origin->name) {
                discard = NULL;
                patterns = expand_options(node->origin->name, node);
                delete_options(detach_options(&added, patterns, &discard));
                delete_options(patterns);
                delete_options(discard);
            }
        }
        delete_options(defaults);
    }
    if (rule->conflict) {
        patterns = expand_options(rule->conflict, option);
        removed = detach_options(&driver->options, patterns, NULL);
        for (node = removed; node; node = node->next) {
            if (node->origin)
                node->origin->flags |= 16;
            if (!(node->flags & 33))
                option_conflict(2016, node, option);
        }
        delete_options(removed);
        delete_options(patterns);
    }
    if (added)
        append_option(&driver->options, added);
    return 1;
}

/* 004049c6..004049f0 */
char *skip_space(char *text)
{
    while (*text && _ismbcspace((unsigned char)*text))
        text = (char *)_mbsinc((unsigned char *)text);
    return text;
}

/* 004049f0..00404a14 */
char *append_literal(char *out, char *text)
{
    if (text)
        strcat(out, text);
    return out + strlen(out);
}

/* 00404a14..00404a80: double percent signs in option substitutions. */
char *append_escaped(char *out, char *text)
{
    unsigned char *source = (unsigned char *)text;
    unsigned char *position = (unsigned char *)out + strlen(out);
    if (source) {
        while (*source) {
            _mbccpy(position, source);
            position = _mbsinc(position);
            if (!_mbsncmp(source, (unsigned char *)"%", 1)) {
                _mbccpy(position, source);
                position = _mbsinc(position);
            }
            source = _mbsinc(source);
        }
        *position = '\0';
    }
    return (char *)position;
}

/* 0040203f..00402110: turn grouped switches into individual rule invocations. */
void apply_special(SpecialOption *special, char *argument)
{
    char word[32], *words[2], *out, *pattern;
    int length, index;
    word[0] = *argument++;
    length = strlen(special->name);
    memcpy(word + 1, argument, length);
    argument += length;
    words[0] = word;
    words[1] = NULL;
    while (*argument) {
        out = word + length + 1;
        pattern = strchr(special->pattern, (unsigned char)*argument);
        *out++ = *argument++;
        if (pattern) {
            while (*++pattern && *pattern != ':') {
                if (*pattern == '#') {
                    /* The target calls the exported function, not the SDK macro. */
                    while ((isdigit)((unsigned char)*argument))
                        *out++ = *argument++;
                } else if (*pattern == '*') {
                    while (*argument)
                        *out++ = *argument++;
                    break;
                } else if (*pattern == *argument)
                    *out++ = *argument++;
                else
                    break;
            }
        }
        *out = '\0';
        index = 0;
        match_option(words, &index);
    }
}

/* 00402af4..00402b8a: add an extension, or replace it for a second '<'. */
char *apply_extension(char *pattern, char *filename, char *alternative)
{
    char discarded[256], *position;
    int replace = *pattern == '<';
    if (replace)
        ++pattern;
    position = find_last_separator(filename, "\\/");
    if (position && !position[1])
        position = append_literal(filename, alternative ? alternative : "%b");
    else {
        position = (char *)_mbsrchr((unsigned char *)(position ? position : filename), '.');
        if (position) {
            if (!replace)
                position = discarded;
        } else
            position = filename + strlen(filename);
    }
    *position++ = '.';
    while (*pattern && *pattern != ',')
        *position++ = *pattern++;
    *position = '\0';
    return pattern;
}

/* 00404c94..00404cd1: last occurrence of any selected multibyte character. */
char *find_last_separator(char *text, char *separators)
{
    char *last = NULL, *position;
    while (*separators) {
        position = (char *)_mbsrchr((unsigned char *)text, (unsigned char)*separators);
        if (position && (!last || position > last))
            last = position;
        separators = (char *)_mbsinc((unsigned char *)separators);
    }
    return last;
}

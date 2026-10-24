#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include <recon.h>

__declspec(dllimport) unsigned long __cdecl SigForPbCb(const void *, long,
                                                       unsigned long);
int match_option_suffix(const char *pattern, char *argument, unsigned action);
void apply_option_action(CompilerOptionRule *rule, char *value, int copy);
int parse_option_integer(const char *value);
void append_option_vector(CompilerVector *vector, char *value);

/* The punctuation is part of the option grammar. '-' accepts either switch
 * introducer, '%' suppresses the signature, '#' accepts a remaining or next
 * value, '$' does so without hashing, and '*' requires a matching suffix. */
int match_option(CompilerOptionRule *rules, char *argument,
                 char *(__cdecl *next)(void), int copy)
{
    CompilerOptionRule *rule;
    const char *pattern, *introducer;
    char *text, *value;
    long length;
    int hash_argument = 1;

    if (!argument)
        return 0;
    frontend_bss.current_argument = argument;
    for (rule = rules; rule->action; ++rule) {
        pattern = rule->pattern;
        text = argument;
        for (;;) {
            switch (*pattern) {
            case '%':
                hash_argument = 0;
                ++pattern;
                continue;
            case '-':
                for (introducer = "-/"; *introducer; ++introducer)
                    if (*introducer == *text)
                        break;
                if (!*introducer)
                    goto next_rule;
                ++pattern;
                ++text;
                continue;
            case '*':
                if (!*text || !match_option_suffix(pattern, text, rule->action))
                    goto next_rule;
                apply_option_action(rule, text, copy);
                goto hash_original;
            case '$':
                hash_argument = 0;
                /* Fall through: '#' has the same value ownership. */
            case '#':
                if (*text) {
                    apply_option_action(rule, text, copy);
                } else {
                    value = next();
                    apply_option_action(rule, value, copy);
                    if (hash_argument) {
                        for (length = 0; value[length]; ++length)
                            ;
                        frontend_bss.source_signature = SigForPbCb(value, length,
                                                        frontend_bss.source_signature);
                    }
                }
                goto hash_original;
            case '\0':
                if (*text)
                    goto next_rule;
                if (rule->action & 0x20) {
                    value = next();
                    apply_option_action(rule, value, copy);
                    for (length = 0; value[length]; ++length)
                        ;
                    frontend_bss.source_signature = SigForPbCb(value, length,
                                                    frontend_bss.source_signature);
                } else {
                    apply_option_action(rule, NULL, copy);
                }
                goto hash_original;
            default:
                if (*pattern++ != *text++)
                    goto next_rule;
                continue;
            }
        }
next_rule:
        ;
    }
    return 0;

hash_original:
    if (hash_argument) {
        for (length = 0; argument[length]; ++length)
            ;
        frontend_bss.source_signature = SigForPbCb(argument, length,
                                                  frontend_bss.source_signature);
    }
    return rule->result;
}

int match_option_suffix(const char *pattern, char *argument, unsigned action)
{
    __asm { int 3 }
    return 0;
}

void apply_option_action(CompilerOptionRule *rule, char *value, int copy)
{
    CompilerZRule *letter;
    char *text, *name;
    unsigned length;

    switch (rule->action) {
    case 1:
        *(int *)rule->target = 1;
        break;
    case 5:
        *(int *)rule->target = 0;
        break;
    case 0x22:
        *(char **)rule->target = copy ? _strdup(value) : value;
        break;
    case 0x23:
        for (text = value; *text; ++text) {
            for (letter = (CompilerZRule *)rule->target; letter->letter; ++letter)
                if ((char)letter->letter == *text)
                    break;
            if (!letter->letter)
                report_compiler_diagnostic(48, (int)*text,
                                            frontend_bss.current_argument);
            else if (letter->action == 1)
                *letter->target = 1;
            else if (letter->action == 5)
                *letter->target = 0;
        }
        break;
    case 0x24:
        *(int *)rule->target = parse_option_integer(value);
        break;
    case 0x26:
        append_option_vector(*(CompilerVector **)rule->target,
                             copy ? _strdup(value) : value);
        break;
    case 0x27:
        if (!*(char **)rule->target) {
            *(char **)rule->target = copy ? _strdup(value) : value;
        } else {
            name = _strdup(rule->pattern);
            for (length = 0; name[length]; ++length)
                ;
            name[length - 1] = '\0';
            report_compiler_diagnostic(46, name, *(char **)rule->target, value);
        }
        break;
    }
}

int parse_option_integer(const char *value)
{
    const char *digits = value;
    int number = 0;

    /* These are genuine public imports, rather than SDK ctype table macros. */
    while ((isspace)((unsigned char)*digits))
        ++digits;
    if (!(isdigit)((unsigned char)*digits))
        goto invalid;
    number = (int)atol(digits);
    if (number >= 65535 || number < 0)
        goto invalid;
    return number;

invalid:
    report_compiler_diagnostic(49, value);
    return number;
}

void append_option_vector(CompilerVector *vector, char *value)
{
    __asm { int 3 }
}

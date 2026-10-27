#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "compiler-state.h"
#include <recon.h>

__declspec(dllimport) unsigned long __cdecl SigForPbCb(const void *, long,
                                                       unsigned long);
int __cdecl match_option_suffix(const char *, char *, unsigned);
void __cdecl apply_option_action(CompilerOptionRule *, char *, int);
int __cdecl parse_option_integer(const char *);
void __cdecl append_option_vector(CompilerVector *, void *);

/* The punctuation belongs to the option grammar, rather than the input:
 * '-' accepts either switch introducer, '%' suppresses the signature,
 * '#' accepts a remaining/following value, '$' does so without hashing,
 * and '*' matches a suffix. The action's 0x20 flag requests a following value
 * even when the grammar itself ended at the end of the input argument. */
int __cdecl match_option(CompilerOptionRule *rules, char *argument,
                        char *(__cdecl *next)(void), int copy)
{
    CompilerOptionRule *rule;
    const char *pattern;
    const char *introducer;
    char *text;
    char *value;
    long length;
    int hash_argument = 1;

    if (!argument)
        return 0;
    c1_bss.current_argument = argument;
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
                /* Fall through: the value has the same ownership as '#'. */
            case '#':
                if (!*text) {
                    value = next();
                    apply_option_action(rule, value, copy);
                    if (hash_argument) {
                        for (length = 0; value[length]; ++length)
                            ;
                        c1_bss.pdb_signature = SigForPbCb(value, length,
                                                        c1_bss.pdb_signature);
                    }
                } else {
                    apply_option_action(rule, text, copy);
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
                    c1_bss.pdb_signature = SigForPbCb(value, length,
                                                    c1_bss.pdb_signature);
                } else {
                    apply_option_action(rule, NULL, copy);
                    goto hash_original;
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
        c1_bss.pdb_signature = SigForPbCb(argument, length,
                                        c1_bss.pdb_signature);
    }
    return rule->result;
}

int __cdecl match_option_suffix(const char *pattern, char *argument,
                                unsigned action)
{
    const char *suffix = pattern + 1;
    char *tail = argument;
    (void)action; /* The original never reads its third incoming slot. */
    if (!*suffix) return 1;
    if (!*argument) {
#ifdef __linux__
        recon_unsupported("option suffix comparison has no preceding input byte");
#else
        __asm { int 3 }
#endif
    }
    while (suffix[1]) ++suffix;
    while (tail[1]) ++tail;
    while (*tail == *suffix) {
        /* The original continues into preceding storage if the whole input
         * matches, or if '*' itself matches. No such preceding-byte input
         * contract is established for these unsupported alternatives. */
        if (tail == argument || suffix == pattern) {
#ifdef __linux__
            recon_unsupported("option suffix comparison reaches unmodeled preceding storage");
#else
            __asm { int 3 }
#endif
        }
        --tail;
        --suffix;
    }
    return suffix == pattern && tail >= argument;
}

/* The table's actions describe the destination type, independently of the
 * pattern punctuation. Copying is requested by the caller, not inferred from
 * whether a string happens to reside in the command line. */
void __cdecl apply_option_action(CompilerOptionRule *rule, char *value, int copy)
{
    CompilerZRule *letter;
    char *name;
    unsigned length;

    switch (rule->action) {
    case 1:
        *(int *)rule->target = 1;
        break;
    case 5:
        *(int *)rule->target = 0;
        break;
    case 0x27:
        if (*(char **)rule->target) {
            name = _strdup(rule->pattern);
            for (length = 0; name[length]; ++length)
                ;
            name[length - 1] = '\0';
            compiler_error(46, name, *(char **)rule->target, value);
            return;
        }
        /* First occurrence follows the ordinary string-assignment action. */
    case 0x22:
        *(char **)rule->target = copy ? _strdup(value) : value;
        break;
    case 0x23:
        for (; *value; ++value) {
            letter = (CompilerZRule *)rule->target;
            while (letter->letter &&
                   (unsigned char)letter->letter != (unsigned char)*value)
                ++letter;
            if (letter->letter) {
                if (letter->action == 1)
                    *letter->target = 1;
                else if (letter->action == 5)
                    *letter->target = 0;
            } else {
                compiler_error(48, *value, c1_bss.current_argument);
            }
        }
        break;
    case 0x24:
        *(int *)rule->target = parse_option_integer(value);
        break;
    case 0x26:
        append_option_vector(*(CompilerVector **)rule->target,
                             copy ? _strdup(value) : value);
        break;
    }
}

/* The public ctype imports are genuine calls in this build. Parentheses
 * prevent a modern header from replacing them with its private table macro. */
int __cdecl parse_option_integer(const char *value)
{
    const char *digits = value;
    int number = 0;

    while ((isspace)((unsigned char)*digits))
        ++digits;
    if (!(isdigit)((unsigned char)*digits))
        goto invalid;
    number = (int)atol(digits);
    if (number >= 65535 || number < 0)
        goto invalid;
    return number;

invalid:
    compiler_error(49, value);
    return number;
}

void __cdecl append_option_vector(CompilerVector *vector, void *item)
{
    __asm { int 3 }
}

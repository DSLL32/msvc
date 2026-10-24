#include "compiler-state.h"
#include <recon.h>

__declspec(dllimport) unsigned long __cdecl SigForPbCb(const void *, long,
                                                       unsigned long);
int __cdecl match_option_suffix(const char *, char *, unsigned);
void __cdecl apply_option_action(CompilerOptionRule *, char *, int);

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
    __asm { int 3 }
    return 0;
}

void __cdecl apply_option_action(CompilerOptionRule *rule, char *value, int copy)
{
    __asm { int 3 }
}

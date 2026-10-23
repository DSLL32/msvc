/* Driver option grammar scanned by find_rule, 00406340..004070ec.
 * Records express recognized spelling, emitted pass options, conflicts,
 * replacements, and requirements or the @-mode handler. */
#include "cl.h"
#include <recon.h>

OptionRule option_rules[] = {
    { "Bbb2", "2M=*", NULL, NULL, NULL },
    { "Bk:#", "D=Bk,=bil:*,m=*:*", NULL, NULL, NULL },
    { "bzalign", "2M=*", NULL, NULL, NULL },
    { "B2!+", "DM=bp:2*", NULL, NULL, NULL },
    { "Bp2!+", "DM=bx:2*", NULL, NULL, NULL },
    { "^dos", "2=*", NULL, NULL, NULL },
    { "EHa", "P2M=*,PM-D!_CPPUNWIND", NULL, "=EHa-,=EHs", NULL },
    { "EHa-", "D=*", NULL, "=EHa,=D!_CPPUNWIND", NULL },
    { "EHs", "P2M=*,PM-D!_CPPUNWIND", NULL, "=EHs-,=EHa", NULL },
    { "EHs-", "D=*", NULL, "=EHs,=D!_CPPUNWIND", NULL },
    { "EHc", "PM=*", NULL, "=EHc-", "=EHa,=EHs,=GX" },
    { "EHc-", "D=*", NULL, "=EHc", NULL },
    { "FA:#", "D=bA:2M a,2M=FA!a*", NULL, "=FA:?,=bA:?", NULL },
    { "Fa:#", "2m=Fa:(*|%b),D=bA:2M a,-bo:Fa Fa (*|%b)", NULL, "=Fa:?,=bA:?,=bo:Fa?", NULL },
    { "Fc:#", "2m=Fa:(*|%b),D=bA:2M asc,-bo:Fa Fc (*|%b)", NULL, "=FA:?,=Fa:?,=bA:?,=bo:Fa?", NULL },
    { "Fl:#", "2m=Fa:(*|%b),D=bA:2M ac,-bo:Fa Fl (*|%b)", NULL, "=FA:?,=Fa:?,=bA:?,=bo:Fa?", NULL },
    { "FP:?#", NULL, NULL, NULL, NULL },
    { "Fs:?#", NULL, NULL, NULL, NULL },
    { "G3", "D=*,1PM-D!_M_IX86=300,1P2M-G4", NULL, "=G4,=G5,=G6,=GB,=D!_M_IX86=?", NULL },
    { "G4", "D=*,1PM-D!_M_IX86=400,1P2M-G4", NULL, "=G3,=G5,=G6,=GB,=D!_M_IX86=?", NULL },
    { "G5", "D=*,1PM-D!_M_IX86=500,1P2M-G5", NULL, "=G3,=G4,=G6,=GB,=D!_M_IX86=?", NULL },
    { "G6", "D=*,1PM-D!_M_IX86=600,1P2M-G6", NULL, "=G3,=G4,=G5,=GB,=D!_M_IX86=?", NULL },
    { "GB", "D=*,1PM-D!_M_IX86=500,1P2M-G5", NULL, "=G3,=G4,=G5,=G6,=D!_M_IX86=?", NULL },
    { "Gd", "1PM=*", "=Gr,=Gz", NULL, NULL },
    { "Ge", "D=*,2M=Gs!0", NULL, "=Gs:?", NULL },
    { "Gi", "1P2M=*", "=Z7", "=Gi-", NULL },
    { "Gi-", "D=*", NULL, "=Gi", NULL },
    { "Gr", "1MP=*", "=Gd,=Gz", NULL, NULL },
    { "Gs:$x[0,10-4294967295,4096]", "1P=*,2M=*!(*|4096)", NULL, "=Ge,=Gs:?", NULL },
    { "GM", "1PM=*", NULL, NULL, NULL },
    { "GX", "D=*,P2M-EHs,PM-EHc,-D!_CPPUNWIND", NULL, "=GX-,-EHa,-EHs-,-EHc-", NULL },
    { "GX-", "D=*", NULL, "=GX,=D!_CPPUNWIND,=EHa,=EHs", NULL },
    { "Gz", "1PM=*", "=Gd,=Gr", NULL, NULL },
    { "GE:?#", NULL, NULL, NULL, NULL },
    { "Gp:?#", NULL, NULL, NULL, NULL },
    { "Gt:?#", NULL, NULL, NULL, NULL },
    { "nl!*x", "2M=H:*", NULL, NULL, NULL },
    { "o!+", "D=Fe:(*|%b)<%X,C=out\\:!(*|%b)<%X", NULL, NULL, NULL },
    { "O", "1PM=Ot", NULL, "=Os", NULL },
    { "O1", "D=*,1P-Gs,1PM-Gf,-Og,-Os,-Oy,-Ob!1,2M-Gs!4096,-Gy", NULL, "=Od,=Ox,=O2,-Ge,-Gs:?,-GF,-Oi,-Ot,-Oy-,-Ob:?", NULL },
    { "O2", "D=*,1P-Gs,1PM-Gf,-Og,-Oi,-Ot,-Oy,-Ob!1,2M-Gs!4096,-Gy", NULL, "=Od,=O1,-Ge,-Gs:?,-GF,-Os,-Oy-,-Ob:?", NULL },
    { "Ob!$d[0-2]", "1PM=*!*", NULL, "=Ob:?", NULL },
    { "Od", "D=*,1PM-Ob!0,-Ot", NULL, "=Ox,=O1,=O2,=Og,-Gf,-Gy,-Oi,-Os,-Oy,-Ob:?", NULL },
    { "Op", "1PM=*", NULL, "=Op-", NULL },
    { "Op-", "D=*", NULL, "=Op", NULL },
    { "Os", "1PM=*", NULL, "=Ot", NULL },
    { "Ot", "1PM=*", NULL, "=Os", NULL },
    { "Ox", "D=*,1P-Gs,1PM-Og,-Oi,-Ot,-Oy,-Ob!1,2M-Gs!4096", NULL, "=Od,=O1,-Ge,-Gs:?,-Os,-Oy-,-Ob:?", NULL },
    { "Oy", "1PM=*", NULL, "=Oy-", NULL },
    { "Oy-", "D=*", NULL, "=Oy", NULL },
    { "OV:?#", NULL, NULL, NULL, NULL },
    { "QI0f", "2M=*", NULL, "=QI0f-", NULL },
    { "QI0f-", "D=*", NULL, "=QI0f", NULL },
    { "QIfdiv", "2M=*", NULL, "=QIfdiv-", NULL },
    { "QIfdiv-", "D=*", NULL, "=QIfdiv", NULL },
    { "^xenix", NULL, NULL, "=dos", NULL },
    { "QIf", "2M=*", NULL, NULL, NULL },
    { "ehopt", "2M=*", NULL, "=noehopt", NULL },
    { "noehopt", "2M=*", NULL, "=ehopt", NULL },
    { "debug_thread", "2M=*", NULL, NULL, NULL },
    { "force_thread", "1P=thread,2M=*", NULL, NULL, NULL },
    { "thread", "1PM=thread,2=force_thread,=debug_thread", NULL, NULL, NULL },
    { "?", "D=bh", NULL, NULL, NULL },
    { "^bC!*", "C=!*", NULL, NULL, NULL },
    { "^bS!*", "S=!*", NULL, NULL, NULL },
    { "Bd", "D=bv,1P2=*", NULL, NULL, NULL },
    { "Bz", "D=bv,=*", NULL, "-Gm", NULL },
    { "B1!+", "DM=bp:1*", NULL, NULL, NULL },
    { "Bx!+", "DM=bp:P*", NULL, NULL, NULL },
    { "Bl!+", "D=bp:C*", NULL, NULL, NULL },
    { "Brepro", "D1P2CM=*", NULL, NULL, NULL },
    { "Bp1!+", "DM=bx:1*", NULL, NULL, NULL },
    { "Bpx!+", "DM=bx:P*", NULL, NULL, NULL },
    { "Bpl!+", "DM=bx:C*", NULL, NULL, NULL },
    { "Bt", "D=*", NULL, NULL, NULL },
    { "c", "D=bd:C", NULL, NULL, NULL },
    { "C", "1P=*", NULL, NULL, "=E,=EP,=P" },
    { "cbstring", "1P2M=*", NULL, NULL, NULL },
    { "D!@", NULL, NULL, NULL, (void *)define_macro },
    { "d1!+", "1PM=!*", NULL, NULL, NULL },
    { "d2!+", "2M=!*", NULL, NULL, NULL },
    { "E", "1P=E,D=bE,=br,=bd:.,=ba:1,=ba:P", NULL, "-Gm", NULL },
    { "EP", "1P=E,=*,D=bE,=br,=bd:.,=ba:1,=ba:P", NULL, "-Gm", NULL },
    { "^f!+", "1P2=f:%f", NULL, NULL, NULL },
    { "F!*x", "C=stack\\:!*", NULL, "=stack\\::?", NULL },
    { "Fd:#", "1P=*!(*|vc50)<pdb,D2=*!(*|vc50)<<idb", NULL, "=Fd:?", NULL },
    { "FD", "1P=*", NULL, NULL, NULL },
    { "Fe:#", "D=Fe:(*|%b)<%X,C=out\\:!(*|%b)<%X", NULL, "=Fe:?,=out\\::?", NULL },
    { "FI!*", "1Pm=*:*", NULL, NULL, NULL },
    { "Fm:#", "D=bm,=*:*,C=map\\:!%m", NULL, "=Fm:?,=map\\::?", NULL },
    { "Fo:#", "D1P2=*!(*|%b)<obj,D-bo:Fo Fo (*|%b)", NULL, "=*:?,=bo:Fo?", NULL },
    { "Fp:#", "1Pm=*!(*|vc50)<pch", NULL, "=Fp:?", NULL },
    { "Fr:#", "D1Pm=*:(*|%b)<sbr,D-bo:Fr Fr (*|%b)", NULL, "=Fr:?,=FR:?,=bo:Fr?", NULL },
    { "FR:#", "D1Pm=*:(*|%b)<sbr,D-bo:Fr FR (*|%b)", NULL, "=Fr:?,=FR:?,=bo:Fr?", NULL },
    { "GA", "2M=*", NULL, "=GD", NULL },
    { "GD", "2M=*", NULL, "=GA", NULL },
    { "Gf", "1PM=*", NULL, "=GF", NULL },
    { "GF", "1PM=*", NULL, "=Gf", NULL },
    { "Gh", "2M=*", NULL, NULL, NULL },
    { "Gm", "DP-*", NULL, "=Gm-,-Bz,-E,-EP,-P,-Zg,-Zs", "=Zi" },
    { "Gm-", "D=*", NULL, "=Gm", NULL },
    { "GR", "PM=*,-D!_CPPRTTI", NULL, "=GR-", NULL },
    { "GR-", "D=*", NULL, "=GR,=D!_CPPRTTI", NULL },
    { "GT", "2M=*", NULL, NULL, NULL },
    { "Gy", "2M=*", NULL, NULL, NULL },
    { "help", "D=bh", NULL, NULL, NULL },
    { "HELP", "D=bh", NULL, NULL, NULL },
    { "H!*x", "2M=*:*", NULL, "=H:?", NULL },
    { "I!+", "1Pm=*:*", NULL, NULL, NULL },
    { "^il!+", "1P2=*:*", NULL, NULL, NULL },
    { "J", "1PM=*,-D!_CHAR_UNSIGNED", NULL, NULL, NULL },
    { "link:@", NULL, NULL, NULL, (void *)link_arguments },
    { "LD", "D=*,C=dll,=implib\\:!%x.lib,1PM-D!_MT,2M-MT", NULL, "-ML,-MD,-MLd,-MTd,-MDd", NULL },
    { "LDd", "D=*,C=dll,=implib\\:!%x.lib,1PM-D!_MT,2M-MTd", NULL, "-ML,-MT,-MD,-MLd,-MDd,-D!_DEBUG", NULL },
    { "MD", "2M=*,1PM-D!_MT,-D!_DLL", NULL, "=ML,=MT,=MLd,=MTd,=MDd,-D!_DEBUG,=:defaultlib\\:?", NULL },
    { "ML", "2M=*", NULL, "=MD,=MT,=MLd,=MTd,=MDd,-D!_DEBUG,-D!_MT,-D!_DLL", NULL },
    { "MT", "2M=*,1PM-D!_MT", NULL, "=MD,=ML,=MLd,=MTd,=MDd,-D!_DEBUG,-D!_DLL", NULL },
    { "MDd", "2M=*,1PM-D!_DEBUG,-D!_MT,-D!_DLL", NULL, "=MLd,=MTd,=ML,=MT,=MD,=:defaultlib\\:?", NULL },
    { "MLd", "2M=*,1PM-D!_DEBUG", NULL, "=MDd,=MTd,=MD,=MT,=ML,-D!_MT,-D!_DLL", NULL },
    { "MTd", "2M=*,1PM-D!_DEBUG,-D!_MT", NULL, "=MDd,=MLd,=MD,=ML,=MT,-D!_DLL", NULL },
    { "noBool", "PM=*", NULL, "=noBool-", NULL },
    { "noBool-", "D=*", NULL, "=noBool", NULL },
    { "nologo", "1P=*,C=nologo", NULL, "=nologo-", NULL },
    { "nologo-", "D=*", NULL, "=nologo", NULL },
    { "Oa", "1PM=*", NULL, "=Oa-", NULL },
    { "Oa-", "D=*", NULL, "=Oa", NULL },
    { "Og", "1PM=*", NULL, "=Og-", NULL },
    { "Og-", "D=*", NULL, "=Og", NULL },
    { "Oi", "1PM=*", NULL, "=Oi-", NULL },
    { "Oi-", "D=*", NULL, "=Oi", NULL },
    { "Ow", "1PM=*", NULL, "=Ow-", NULL },
    { "Ow-", "D=*", NULL, "=Ow", NULL },
    { "P", "D=br,=bd:.,=ba:1,=ba:P,1P=E,=Fi:%b.i", NULL, "-Gm", NULL },
    { "^pc!+", "1P=*:*", NULL, NULL, NULL },
    { "Tc!@", NULL, NULL, NULL, (void *)c_input },
    { "Tp!@", NULL, NULL, NULL, (void *)cpp_input },
    { "To!@", NULL, NULL, NULL, (void *)object_input },
    { "TC", "D=bt:C", NULL, "=bt:?", NULL },
    { "TO", "D=bt:O", NULL, "=bt:?", NULL },
    { "TP", "D=bt:P", NULL, "=bt:?", NULL },
    { "U!*", "D=bU:*", NULL, NULL, NULL },
    { "u", "D=bu", NULL, NULL, NULL },
    { "V!*", "1PM=*:*", NULL, NULL, NULL },
    { "vd!$d[0,1]", "PM=*!*", "=vd:?", NULL, NULL },
    { "vmb", "PM=*", "=vmg", NULL, NULL },
    { "vmg", "PM=*", "=vmb", NULL, NULL },
    { "vmm", "PM=*", "=vms,=vmv", NULL, NULL },
    { "vms", "PM=*", "=vmm,=vmv", NULL, NULL },
    { "vmv", "PM=*", "=vmm,=vms", NULL, NULL },
    { "WX", "1P2M=*", NULL, "=WX-", NULL },
    { "WX-", "D=*", NULL, "=WX", NULL },
    { "W!*d[0-4,1]", "1P2M=*:*", NULL, "=W:?", NULL },
    { "w", "1P2M=W:0", NULL, "=W:?", NULL },
    { "X", "D=*", NULL, NULL, NULL },
    { "Yc:#", "1Pm=*!*", NULL, "=Yc:?,=YX:?,=ZM", NULL },
    { "Yd", "1PM=*", NULL, NULL, NULL },
    { "Yu:#", "1Pm=*!*", NULL, "=Yu:?,=YX:?", NULL },
    { "YX:#", "1Pm=*!*", NULL, "=Yc:?,=Yu:?,=ZM", NULL },
    { "Yl:#", "1PM=*!*", NULL, NULL, NULL },
    { "Za", "1P2M=*,1PM-Op", "=Ze,=ZBe", "-Op-,-D!_MSC_EXTENSIONS", NULL },
    { "Ze", "1PM=*,-D!_MSC_EXTENSIONS", "=Za", NULL, NULL },
    { "ZB:$d[32,128,64]", "1PM=*!(*|64),-D!_INTEGRAL_MAX_BITS=(*|64)", NULL, "=*:?,=D!_INTEGRAL_MAX_BITS=?", NULL },
    { "ZBe", "1PM=*", "=Za", NULL, NULL },
    { "Zd", "2=*,C=!debug", NULL, "=Zi,=Z7,=!Zi", NULL },
    { "Z7", "1P=*,2=!Zi,C=!debug", "=Gi", "=Zi,=Zd", NULL },
    { "Zi", "1P=*,2=!Zi,C=!debug", NULL, "=Z7,=Zd", NULL },
    { "Zg", "D=bd:.,=ba:1,=ba:P,=br,1P=*", NULL, "-Gm", NULL },
    { "Zl", "2M=*", NULL, "=:defaultlib\\:?", NULL },
    { "ZM", "D-*", NULL, "-ZM-,-Yc:?,-YX:?", NULL },
    { "ZM-", "D-*", NULL, "-ZM", NULL },
    { "Zm!#d[0-2000,100]", "1PM=*!*", NULL, "=Zm:?", NULL },
    { "Zn", "1PM=*", NULL, NULL, "=FR:?,=Fr:?" },
    { "Zp:$d[1,2,4,16,8]", "1PM=*!(*|1)", NULL, "=*:?", NULL },
    { "Zs", "D=bd:.,=ba:1,=ba:P", NULL, "-Gm", NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

/* 00401e64..00401ed0: the longest positive match wins; ties retain order. */
OptionRule *find_rule(char **args, int index)
{
    OptionRule *rule, *best = NULL;
    int length, longest = 0;
    for (rule = option_rules; rule->pattern; ++rule) {
        char *pattern = rule->pattern;
        if (*pattern == '^')
            ++pattern;
        if (*pattern != args[index][1])
            continue;
        length = match_pattern(rule, args, index);
        if (length > 0 && (!longest || length > longest)) {
            longest = length;
            best = rule;
        }
    }
    return best;
}

/* 004013ef..00401440: ^ marks defaults, :/! introduce option arguments. */
int match_pattern(OptionRule *rule, char **args, int index)
{
    char *argument = args[index], *position = argument + 1;
    char *pattern = rule->pattern;
    if (*pattern == '^') {
        if (!show_commands)
            return 0;
        ++pattern;
    }
    while (*pattern != ':' && *pattern != '!') {
        if (*pattern != *position)
            return 0;
        if (!*pattern)
            break;
        ++pattern;
        ++position;
    }
    if (!*pattern && *position != ' ' && *position)
        return 0;
    return position - argument;
}

/* 00401205..00401392: separate option name and argument under its rule. */
Option *read_option(OptionRule *rule, char **args, int *index)
{
    char name[32], value[1024];
    char *pattern = rule->pattern, *argument = args[*index] + 1, *out = name;
    char *payload;
    int optional, allow_switch, next_word;
    unsigned flags = 0;
    typedef Option *(*OptionHandler)(char **, int *);
    if (*pattern == '^')
        ++pattern;
    while (*pattern && *pattern != ':' && *pattern != '!') {
        if (*pattern != *argument)
            fatal(0);
        *out++ = *argument++;
        ++pattern;
    }
    *out = '\0';
    if (!*pattern) {
        ++*index;
        return new_option(name, NULL, NULL, 0);
    }
    optional = *pattern++ == ':';
    if (*pattern == '?') {
        flags = 2;
        ++pattern;
        memmove(name + 1, name, 31);
        name[0] = args[*index][0];
    }
    switch (*pattern++) {
    case '@':
        return ((OptionHandler)rule->extra)(args, index);
    case '+': allow_switch = 1; next_word = 1; break;
    case '*': allow_switch = 0; next_word = 1; break;
    case '$': allow_switch = 0; next_word = 0; break;
    case '#': allow_switch = 1; next_word = 0; break;
    default: allow_switch = 1; next_word = 1; break;
    }
    ++*index;
    if (next_word && !*argument)
        argument = args[(*index)++];
    if (argument && *argument &&
        (allow_switch || (*argument != '-' && *argument != '/'))) {
        strcpy(value, argument);
        payload = value;
    } else {
        if (!optional) {
            fatal(2004, name);
            return NULL;
        }
        payload = NULL;
    }
    if (payload && *pattern)
        validate_value(name, payload, pattern);
    return new_option(name, payload, NULL, flags);
}

/* 00401118..00401205: normalize a numeric argument and enforce its ranges. */
void validate_value(char *name, char *value, char *constraints)
{
    char *number = value, *end;
    unsigned long result, low, high, temporary;
    int base = 10;
    if (*constraints++ == 'x' && value[0] == '0') {
        if (value[1] == 'D' || value[1] == 'T' || value[1] == 'd' || value[1] == 't')
            number += 2;
        else
            base = 0;
    }
    result = strtoul(number, &end, base);
    if (*end) {
        fatal(2021, name, value);
        return;
    }
    if (*constraints++ != '[') {
        format_number(result, value, 10);
        return;
    }
    for (;;) {
        low = high = strtoul(constraints, &end, 10);
        if (*end == '-') {
            high = strtoul(end + 1, &end, 10);
            if (high < low) {
                temporary = high; high = low; low = temporary;
            }
        }
        if (result >= low && result <= high) {
            format_number(result, value, 10);
            return;
        }
        if (*end != ',')
            break;
        constraints = end + 1;
    }
    warning(4014, result, name, low);
    format_number(low, value, 10);
}

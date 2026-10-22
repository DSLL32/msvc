/* Command line transport used by this RCPP, before its option parser. */
#include "rcpp.h"
static char module_name[MAX_PATH];
char *rc_program_name;

/* Original [01009400,010094a2). Count first, then allocate one argv/text block. */
void rc_setargv(void)
{
    char *command;
    char **arguments;
    int count, bytes;
    GetModuleFileNameA(NULL, module_name, sizeof(module_name));
    rc_program_name = module_name;
    command = *rc_command_line ? rc_command_line : module_name;
    rc_parse_command(command, NULL, NULL, &count, &bytes);
    arguments = rc_malloc(count * sizeof(char *) + bytes);
    if (!arguments) rc_runtime_abort(8);
    rc_parse_command(command, arguments, (char *)(arguments + count), &count, &bytes);
    rc_argc = count - 1;
    rc_argv = arguments;
}

/* Original [010094b0,0100961c). A null argv/text buffer selects the sizing
 * pass. The program name has its own quoting rules; later arguments implement
 * backslash parity before quotes and the original doubled-quote behavior. */
void rc_parse_command(char *command, char **argv, char *text,
                      int *argument_count, int *text_bytes)
{
    unsigned char *p = (unsigned char *)command;
    unsigned char ch;
    unsigned slashes;
    int quoted = 0, copy;
#define EMIT(c) { if (text) *text++ = (char)(c); ++*text_bytes; }
    *text_bytes = 0;
    *argument_count = 1;
    if (argv) *argv++ = text;
    if (*p == '"') {
        ++p;
        while (*p && *p != '"') { EMIT(*p); ++p; }
        EMIT(0);
        if (*p == '"') ++p;
    } else {
        do {
            ch = *p++;
            EMIT(ch);
        } while (ch > ' ');
        if (!ch) --p;
        else if (text) text[-1] = 0;
    }
    while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        if (argv) *argv++ = text;
        ++*argument_count;
        for (;;) {
            copy = 1;
            slashes = 0;
            while (*p == '\\') { ++p; ++slashes; }
            if (*p == '"') {
                if (!(slashes & 1)) {
                    if (quoted && p[1] == '"') ++p;
                    else copy = 0;
                    quoted = !quoted;
                }
                slashes /= 2;
            }
            while (slashes) { EMIT('\\'); --slashes; }
            if (!*p || (!quoted && (*p == ' ' || *p == '\t'))) break;
            if (copy) { EMIT(*p); }
            ++p;
        }
        EMIT(0);
    }
    if (argv) *argv = NULL;
    ++*argument_count;
#undef EMIT
}

/* Diagnostic reporting and severity wrappers. */
#include "numbers.h"
void __stdcall rc_report_diagnostic(int, unsigned, const char *);
char *rc_reported_filename = rc_source_name;
int rc_mirror_diagnostics;
FILE *rc_diagnostic_file;

/* Original [01007c20,01007c70). */
void __stdcall rc_error(unsigned code)
{
    rc_report_diagnostic(2000, code, rc_diagnostic_buffer);
    if (++rc_error_count > 100) {
        rc_message_format = rc_message(1003);
        rc_format(rc_diagnostic_buffer, rc_message_format, 100);
        rc_fatal(1003);
    }
}

/* Original [01007c70,01007c9b). */
void __stdcall rc_fatal(unsigned code)
{
    rc_report_diagnostic(1000, code, rc_diagnostic_buffer);
    rc_exit(++rc_error_count);
}

/* Original [01007ca0,01007cb7). */
void __stdcall rc_warning(unsigned code)
{
    rc_report_diagnostic(4000, code, rc_diagnostic_buffer);
}

/* Original [01007cc0,01007eb7). All formatting/copies stay in this function,
 * including its optional diagnostic mirror. The only defined nonzero severity
 * inputs used by the target are 1000 (fatal), 2000 (error) and 4000 (warning). */
void __stdcall rc_report_diagnostic(int severity, unsigned code, const char *message)
{
    char line[512], number[32];
    char *out = line;
    const char *text, *severity_text;
    unsigned length;
    if (rc_line_number > 0 && rc_reported_filename) {
        rc_format(line, "%s(%d) : ", rc_reported_filename, rc_line_number);
        while (*out) ++out;
    }
    if (severity) {
        switch (severity) {
        case 1000: severity_text = rc_message(315); break;
        case 2000: severity_text = rc_message(314); break;
        case 4000: severity_text = rc_message(313); break;
        }
        text = severity_text;
        while ((*out++ = *text++) != 0) ;
        --out;
        rc_format(number, " %s%d: ", "RC", code);
        text = number;
        while ((*out++ = *text++) != 0) ;
        --out;
        text = message;
        while ((*out++ = *text++) != 0) ;
    }
    for (length = 0; line[length]; ++length) ;
    fwrite(line, length, 1, stderr);
    fwrite("\n", 1, 1, stderr);
    if (rc_mirror_diagnostics && rc_diagnostic_file) {
        fwrite(line, length, 1, rc_diagnostic_file);
        fwrite("\n", 1, 1, rc_diagnostic_file);
    }
}

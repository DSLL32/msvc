/* Filename substitutions used by driver settings and compiler passes. */
#include "cl.h"
#include <recon.h>

/* 00404587..0040474d */
char *expand_filename(char *out, char *text, Driver *state)
{
    char *start = out, *filename, *temporary;
    char basename[256];
    CompilerPass *pass;
    int sequence;
    if (state->current_input) {
        filename = state->current_input->name;
        sequence = state->current_input->sequence;
        pass = state->current_pass;
    } else {
        filename = "";
        sequence = -1;
        pass = NULL;
    }
    while (*text) {
        if (*text++ != '%') {
            *out++ = text[-1];
            continue;
        }
        *out = 0;
        switch (*text++) {
        case '%': *out++ = '%'; break;
        case 'B':
            file_basename(basename, filename);
            out = append_literal(out, (char *)_mbsupr((unsigned char *)basename));
            break;
        case 'b':
            file_basename(basename, filename);
            out = append_literal(out, basename);
            break;
        case 'e':
            temporary = pass ? pass->path : NULL;
            out = append_literal(out, temporary ? temporary : "");
            break;
        case 'f': out = append_literal(out, filename); break;
        case 'm':
            temporary = allocate(1024);
            expand_filename(temporary, image_name, state);
            make_map_name(out, temporary);
            out += strlen(out);
            release(temporary);
            break;
        case 't':
            out = append_literal(out, state->temporary);
            if (state->current_jobs && sequence != -1) {
                out[-6] = (char)('a' + sequence / 10);
                out[-5] = (char)('0' + sequence % 10);
            }
            break;
        case 'x':
            file_directory(basename, image_name);
            out = append_literal(out, basename);
            break;
        case 'X': out = append_literal(out, is_dll ? "dll" : "exe"); break;
        }
    }
    *out = 0;
    return start;
}

/* 0040490d..00404928 */
char *file_basename(char *out, char *name)
{
    _splitpath(name, NULL, NULL, out, NULL);
    return out;
}

/* 00404aa4..00404ae4 */
char *join_text(char *left, char *right)
{
    unsigned length;
    char *result;
    length = strlen(right);
    result = allocate(length + strlen(left) + 1);
    strcpy(result, left);
    strcat(result, right);
    return result;
}

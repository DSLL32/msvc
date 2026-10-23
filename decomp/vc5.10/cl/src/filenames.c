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

/* 00404928..00404975: retain filename and extension without its path. */
char *file_name(char *out, char *name)
{
    char basename[256], extension[256];
    _splitpath(name, NULL, NULL, basename, extension);
    _makepath(out, "", "", basename, extension);
    return out;
}

/* 00402fd1..004030d8: compiler directory, current directory, then PATH. */
char *find_file(char *directory, char *name, int *found)
{
    char path[1024], component[256];
    *found = 1;
    _splitpath(name, NULL, component, NULL, NULL);
    if (*component && strchr("\\/", *component)) {
        if (_access(name, 4))
            *found = 0;
        return duplicate(name);
    }
    strcpy(path, directory);
    strcat(path, name);
    if (!_access(path, 4))
        return duplicate(path);
    strcpy(path, ".\\");
    strcat(path, name);
    if (!_access(path, 4) || search_path(name, path, sizeof(path)))
        return duplicate(path);
    *found = 0;
    return duplicate(name);
}

/* 004030d8..00403190: scan semicolon-delimited PATH entries. */
int search_path(char *name, char *out, int capacity)
{
    char *path = getenv("PATH"), *cursor;
    int length;
    if (path) {
        while (*path) {
            while (*path == ' ' || *path == ';')
                ++path;
            if (!*path)
                break;
            cursor = out;
            while (*path && *path != ';')
                *cursor++ = *path++;
            *cursor = 0;
            if (!*out)
                continue;
            length = strlen(out);
            if (out[length - 1] != '\\' && out[length - 1] != ':' && out[length - 1] != '/') {
                out[length] = '\\';
                out[length + 1] = 0;
            }
            strcat(out, name);
            length = strlen(out);
            if (length >= capacity)
                fatal(0);
            if (!_access(out, 4))
                return length;
        }
    }
    *out = 0;
    return 0;
}

/* 00404806..0040490d: bound expansion and Windows argument quoting. */
int expanded_length(char *text, Driver *state)
{
    char *name, basename[260], *path;
    CompilerPass *pass;
    int length = 0;
    if (state->current_input) {
        name = state->current_input->name;
        pass = state->current_pass;
    } else {
        name = "";
        pass = NULL;
    }
    while (*text) {
        switch ((unsigned char)*text++) {
        case '\\':
        case '\"': length += 2; break;
        case '%':
            switch ((unsigned char)*text++) {
            case '%': ++length; break;
            case 'B':
            case 'b':
                file_basename(basename, name);
                length += quoted_length(basename);
                break;
            case 'e':
                path = pass ? pass->path : NULL;
                if (path)
                    length += quoted_length(path);
                break;
            case 'f': length += quoted_length(name); break;
            case 'm': length += 1024; break;
            case 't': length += quoted_length(state->temporary); break;
            case 'x':
                file_directory(basename, image_name);
                length += quoted_length(basename);
                break;
            case 'X': length += 3; break;
            }
            break;
        default: ++length; break;
        }
    }
    return length;
}


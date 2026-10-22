#include "cl.h"

/* [00018f2b,00018f4f) */
void usage(void)
{
    banner(0);
    write_message(1, "usage: cl [ option... ] filename... [ -link linkoption... ]\n");
    exit(0);
}

/* [0001920c,00019228) */
int option_callback_1920c(char **args, int *index)
{
    force_input("Tc", args, index, 0);
    return 0;
}

/* [000191b4,0001920c): attached or following explicit input filename. */
void force_input(char *option, char **args, int *index, int kind)
{
    int next = *index;
    char *name = args[next++] + 1, *p = option;
    *index = next;
    while (*p++) ++name;
    if (!*name) { name = args[next++]; *index = next; }
    if (name && *name) add_input(name, kind);
    else fatal(2004, option);
}
/* [0001a757,0001a7b3) */
void fatal(int code, ...)
{
    va_list ap;
    if (!code) code = 2000;
    write_message(2, "Command line error D%d : ", code);
    va_start(ap, code);
    write_message_list(2, diagnostic_text(driver->error_file, code), ap);
    write_message(2, "\n");
    driver_exit(2);
}

/* [0001ab77,0001abc3) */
void warning(int code, ...)
{
    char *text = diagnostic_text(driver->error_file, code);
    va_list ap;
    write_message(2, "Command line warning D%d : ", code);
    va_start(ap, code);
    write_message_list(2, text, ap);
    write_message(2, "\n");
}

/* [0001a910,0001a936) */
void write_message_list(int fd, char *format, va_list ap)
{
    unsigned size;
    char *text = format_message(&size, format, ap);
    write(fd, text, size);
}

/* [0001aa63,0001ab77) */
char *diagnostic_text(char *filename, int code)
{
    FILE *file = fopen(filename, "r");
    char line[256], number[12], *s, *p, *digit, *result = "";
    if (!file) return "";
    while (fgets(line, 256, file)) {
        s = skip_space(line);
        if (!is_decimal(*s)) continue;
        digit = number;
        while (is_decimal(*s)) *digit++ = *s++;
        *digit = 0;
        if (atoi(number) != code) continue;
        s = skip_space(s) + 1;
        p = s;
        while (*p != '"') {
            if (*p++ == '\\') {
                if (*p == 'n' || *p == 't') {
                    p[-1] = *p == 'n' ? '\n' : '\t';
                    strcpy(p, p + 1);
                }
            }
        }
        *p = 0;
        result = strdup(s);
        break;
    }
    fclose(file);
    return result;
}

/* [00017f2a,00017f87): report the spelling that introduced a derived option. */
void warn_override(int code, Item *old, Item *item)
{
    void (__cdecl *report)(int, ...) = code >= 4000 && code <= 4099 ? warning : fatal;
    if (old->origin) old = old->origin;
    report(code, old->name, old->extra ? old->extra : "",
           item->name, item->extra ? item->extra : "");
}

/* [000176d4,000176f8) */
Item *copy_item(Item *item)
{
    Item *copy = new_item(item->name, item->extra, item->value.groups, item->flags);
    copy->origin = item->origin;
    return copy;
}

/* [0001842b,0001857b) */
static int response_depth, response_line = 1;
void read_response_file(char *name)
{
    extern int echo_response;
    FILE *file;
    char *buffer, **args;
    unsigned length;
    if (++response_depth > 13) fatal(2035, name);
    file = fopen(name, "r");
    if (!file) fatal(2022, name);
    buffer = allocate(1024);
    while (fgets(buffer, 1023, file)) {
        length = strlen(buffer);
        if (buffer[length - 1] == '\n') buffer[length - 1] = 0;
        if (!quiet) {
            write_message(2, response_line ? "cl " : "   ");
            write_message(2, "%s\n", buffer);
            response_line = 0;
            echo_response = 1;
        }
        args = split_arguments(buffer);
        if (args) {
            parse_arguments(args, count_arguments(args));
            release(args[0]);
            release(args);
        }
    }
    if (!feof(file)) fatal(2034, name);
    fclose(file);
    release(buffer);
    --response_depth;
}

/* [00019014,000190d0): preserve raw linker arguments for both linker types. */
int option_callback_19014(char **args, int *index)
{
    int next = *index;
    char *s = args[next] + 5;
    if (*s) {
        append_items(&driver->options, new_item(s, NULL, "L", 8));
        append_items(&driver->options, new_item(s, NULL, "C", 8));
    }
    for (++next; args[next]; ++next) {
        append_items(&driver->options, new_item(args[next], NULL, "L", 8));
        append_items(&driver->options, new_item(args[next], NULL, "C", 8));
    }
    *index = next;
    return 0;
}

/* [00019198,000191b4) */
int option_callback_19198(char **args, int *index)
{
    force_input("To", args, index, 3);
    return 0;
}

/* [00019228,00019244) */
int option_callback_19228(char **args, int *index)
{
    force_input("Tp", args, index, 1);
    return 0;
}

/* [00018fd6,00019014) */
int any_match(Item *items, Item *patterns)
{
    Item *pattern;
    for (; items; items = items->next)
        for (pattern = patterns; pattern; pattern = pattern->next)
            if (match_item(items, pattern)) return 1;
    return 0;
}

/* [000170a0,000170fd): /u removes default predefines from the option list. */
void driver_switch_170a0(Item *item)
{
    static int pending_predefines = 1;
    Item *p, *next;
    if (!pending_predefines) return;
    pending_predefines = 0;
    for (p = driver->options; p; p = next) {
        next = p->next;
        if ((p->flags & 1) && !strcmp(p->name, "D"))
            recycle_item(detach_item(&driver->options, p));
    }
}

/* [0001713c,0001719a) */
void driver_switch_1713c(Item *item)
{
    Profile *profile;
    PassRecord *pass;
    char kind = item->extra[0];
    for (profile = profiles; profile->extension; ++profile)
        for (pass = profile->passes; pass->executable; ++pass)
            if (kind == '.' || (char)pass->letter_prefix == kind)
                pass->executable = copy_string(item->extra + 1);
}

/* [0001719c,000171fb) */
void driver_switch_1719c(Item *item)
{
    Profile *profile;
    PassRecord *pass;
    char kind = item->extra[0];
    for (profile = profiles; profile->extension; ++profile)
        for (pass = profile->passes; pass->executable; ++pass)
            if (kind == '.' || (char)pass->letter_prefix == kind)
                pass->loader = copy_string(item->extra + 1);
}

/* [00017368,000173ac) */
void driver_switch_17368(Item *item)
{
    Profile *profile;
    PassRecord *pass;
    char kind = item->extra[0];
    for (profile = profiles; profile->extension; ++profile)
        for (pass = profile->passes; pass->executable; ++pass)
            if (kind == '.' || (char)pass->letter_prefix == kind)
                pass->enabled = 1;
}

/* [00017010,0001709d): DLL output suppresses generated import-lib switches
   when an explicit export object is already among the inputs. */
void driver_switch_17010(Item *item)
{
    static int enabled = 1;
    Input *input;
    Item *pattern, *expanded;
    if (!enabled) return;
    enabled = 1;
    make_dll = 1;
    for (input = driver->inputs; input; input = input->next) {
        if (input->kind != 6) continue;
        pattern = new_item("implib\\:", NULL, NULL, 0);
        expanded = expand_options("=implib\\::?", pattern);
        free_items(select_options(driver, expanded, NULL));
        recycle_item(pattern);
        free_items(expanded);
        break;
    }
}

/* [0001a2fc,0001a34d) */
char *output_stem(char *out, char *name)
{
    char drive[4], directory[256], stem[256];
    _splitpath(name, drive, directory, stem, NULL);
    _makepath(out, drive, directory, stem, "");
    return out;
}

/* [00019f3c,00019ffe) */
char *map_filename(char *out, char *name)
{
    extern int global_26030;
    char drive[4], directory[256], stem[256], extension[256];
    char *ext;
    if (!global_26030) return "nul.map";
    if (map_file) {
        _splitpath(map_file, drive, directory, stem, extension);
        if (!stem[0]) filename_stem(stem, name);
        ext = extension[0] ? extension : "map";
    } else {
        _splitpath(name, drive, directory, stem, NULL);
        ext = "map";
    }
    _makepath(out, drive, directory, stem, ext);
    return out;
}

/* [000173ac,00017557): combine /FA listing modes and /Fa output selection. */
void driver_switch_173ac(Item *item)
{
    char *mode = item->extra, *suffix, *p;
    char filename[2048];
    Item *format, *name, *old_format, *old_name;
    int source, invalid;
    if (!*mode) return;
    suffix = strchr(mode, ' ');
    if (suffix) *suffix++ = 0;
    format = new_item("FA", "?", mode, 4);
    name = new_item("Fa", "?", mode, 0);
    old_format = select_options(driver, format, NULL);
    old_name = select_options(driver, name, NULL);
    if (!old_format) {
        release(format->extra);
        format->extra = copy_string(suffix);
        old_format = format;
    } else {
        suffix = old_format->extra;
        recycle_item(format);
    }
    if (!old_name) {
        release(name->extra);
        name->extra = copy_string("%b");
        old_name = name;
    } else recycle_item(name);
    source = 0;
    strcpy(filename, old_name->extra);
    if (suffix) {
        invalid = 0;
        for (p = suffix; *p; ++p) {
            switch (*p) {
            case 'c': source = 1; break;
            case 'a': case 's': break;
            default: invalid = 1; break;
            }
        }
        if (invalid) warning(4015, "/FA", suffix + 1);
    }
    expand_extension(source ? "cod" : "asm", filename, NULL);
    release(old_name->extra);
    old_name->extra = copy_string(filename);
    append_items(&driver->options, old_format);
    append_items(&driver->options, old_name);
    item->extra[0] = 0;
}

/* The IDE transport installs three callbacks and owns a 128-byte frame. */
typedef struct IdeCallbacks {
    int (__cdecl *descriptor)(int);
    int (__cdecl *write)(int, char *, int);
    int (__cdecl *abort)(char *, int);
    char *buffer;
    unsigned capacity;
} IdeCallbacks;
IdeCallbacks *ide_callbacks;
int ide_fd;
int ide_abort(char *, int);
int ide_write(int, char *, int);
int ide_descriptor(int);
void ide_install(IdeCallbacks *);

/* [0001ac84,0001acc7) */
void initialize_ide(void)
{
    IdeCallbacks *callbacks = malloc(sizeof(IdeCallbacks));
    callbacks->descriptor = ide_descriptor;
    callbacks->write = ide_write;
    callbacks->abort = ide_abort;
    callbacks->buffer = malloc(128);
    callbacks->capacity = 128;
    ide_install(callbacks);
}

/* [0001acc7,0001acd4) */
int ide_abort(char *filename, int line)
{
    fatal(0);
    return 0;
}

/* [0001acd4,0001ad00) */
int ide_write(int fd, char *buffer, int size)
{
    fflush(&_iob[fd]);
    return write(fd, buffer, size);
}

/* [0001ad00,0001ad08) */
int ide_descriptor(int fd)
{
    return fd;
}

/* [0001ae4c,0001ae65) */
void ide_install(IdeCallbacks *callbacks)
{
    ide_callbacks = callbacks;
    ide_fd = ide_callbacks->descriptor(2);
}

/* [0001ad08,0001ae4c) */
int ide_message(int code, ...)
{
    int result = 0;
    unsigned length;
    va_list ap;
    va_start(ap, code);
    if (!ide_callbacks->buffer) ide_callbacks->abort("d:\\usr\\cuda\\src\\driver\\ideapi.c", 113);
    if (!code) result = sprintf(ide_callbacks->buffer, "%s%d", "@I", code);
    else if (code >= 1 && code <= 4)
        result = sprintf(ide_callbacks->buffer, "%s%d%s", "@I", code, va_arg(ap, char *));
    else if (code >= 5 && code <= 6)
        result = sprintf(ide_callbacks->buffer, "%s%d%lu", "@I", code, va_arg(ap, unsigned long));
    else ide_callbacks->abort("d:\\usr\\cuda\\src\\driver\\ideapi.c", 149);
    if (result != -1 && result) {
        length = strlen(ide_callbacks->buffer);
        if (!length) ide_callbacks->abort("d:\\usr\\cuda\\src\\driver\\ideapi.c", 156);
        if (ide_callbacks->capacity < length)
            ide_callbacks->abort("d:\\usr\\cuda\\src\\driver\\ideapi.c", 157);
        if (strncmp(ide_callbacks->buffer, "@I", 2))
            ide_callbacks->abort("d:\\usr\\cuda\\src\\driver\\ideapi.c", 158);
        result = ide_callbacks->write(ide_fd, ide_callbacks->buffer, length);
    }
    return result;
}

#include <sys/timeb.h>
static struct _timeb timer_start, timer_end;

/* [0001abc3,0001abd1) */
void start_timer(void)
{
    _ftime(&timer_start);
}

/* [0001abd1,0001ac56) */
void report_timer(char *program)
{
    char milliseconds[8];
    int seconds, fraction, digits;
    write_message(1, "time(%s)=", program);
    _ftime(&timer_end);
    seconds = timer_end.time - timer_start.time;
    fraction = (int)timer_end.millitm - timer_start.millitm;
    if (fraction < 0) { fraction += 1000; --seconds; }
    strcpy(milliseconds, "00");
    digits = unsigned_digits(fraction, milliseconds + 2, 10);
    write_message(1, "%d.%ss\n", seconds, milliseconds + digits - 1);
}

/* [0001afcc,0001affa) */
BOOL WINAPI control_handler(DWORD code)
{
    extern int child_running, child_interrupted;
    child_interrupted = 1;
    if (!child_running) {
        SetConsoleCtrlHandler(NULL, TRUE);
        driver_exit(4);
    }
    return TRUE;
}

/* [00019ae1,00019d9f): positional response syntax for the legacy OMF linker. */
void write_omf_response(Driver *d)
{
    extern int global_26034;
    char *buffer = allocate(260), *expanded = allocate(260), *output;
    PassRecord *pass = d->current_pass;
    FILE *file;
    Input *input;
    Item *item;
    int first;
    strcpy(buffer, d->temporary);
    strcat(buffer, "lk");
    if (global_26034) file = NULL;
    else {
        file = fopen(buffer, "w");
        if (!file) fatal(2018);
    }
    first = 1;
    for (input = d->inputs; input; input = input->next) {
        if (input->kind != 3 && input->kind != 7) continue;
        if (!first) file_message(file, "+\n");
        file_message(file, "\"%s\"", input->filename);
        first = 0;
    }
    expand_filename(buffer, executable_file, d);
    output = copy_string(buffer);
    file_message(file, "\n\"%s\"", output);
    file_message(file, "\n\"%s\"", map_filename(buffer, output));
    first = 1;
    for (input = d->inputs; input; input = input->next) {
        if (input->kind != 4) continue;
        if (!first) file_message(file, "+");
        file_message(file, "\n\"%s\"", input->filename);
        first = 0;
    }
    if (first) file_message(file, "\n");
    first = 1;
    for (item = d->options; item; item = item->next) {
        if (!is_for_pass(item, pass)) continue;
        if (!first) file_message(file, " ");
        else first = 0;
        if (!(item->flags & 8)) file_message(file, "%c", (char)(pass->letter_prefix >> 8));
        file_message(file, "%s", expand_filename(buffer, item->name, d));
        if (item->extra) {
            if (!(item->flags & 4)) file_message(file, " ");
            quote_if_space(buffer, expand_filename(expanded, item->extra, d));
            file_message(file, "%s", buffer);
        }
    }
    if (first) file_message(file, "\n");
    first = 1;
    for (input = d->inputs; input; input = input->next) {
        if (input->kind != 5) continue;
        if (!first) file_message(file, "+\n");
        file_message(file, "\n\"%s\"", input->filename);
        first = 0;
    }
    file_message(file, ";\n");
    if (file) fclose(file);
    release(output);
    release(expanded);
    release(buffer);
}

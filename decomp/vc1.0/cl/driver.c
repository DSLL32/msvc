#include "cl.h"

/* [0001affa,0001b007) */
void install_control_handler(void)
{
    SetConsoleCtrlHandler(control_handler, TRUE);
}


/* [0001a587,0001a6b8): Windows quote/backslash rules for CL environment strings. */
char **split_arguments(char *s)
{
    int count = 0, quoted = 0, slashes, emit, i;
    char *start, *out, **args, *p;
    if (!s) return NULL;
    s = skip_space(s);
    if (!*s) return NULL;
    start = out = allocate(strlen(s) + 1);
    while (*s) {
        for (;;) {
            slashes = 0;
            emit = 1;
            while (*s == '\\') { ++s; ++slashes; }
            if (*s == '"') {
                if (!(slashes & 1)) {
                    if (quoted && s[1] == '"') ++s;
                    else emit = 0;
                    quoted = !quoted;
                }
                slashes >>= 1;
            }
            while (slashes--) *out++ = '\\';
            if (!*s || (!quoted && (*s == ' ' || *s == '\t'))) break;
            if (emit) *out++ = *s;
            ++s;
        }
        *out++ = 0;
        ++count;
        s = skip_space(s);
    }
    args = allocate(count * 4 + 4);
    p = start;
    for (i = 0; count--; ++i) {
        args[i] = p;
        p += strlen(p) + 1;
    }
    args[i] = NULL;
    return args;
}


/* [0001857b,0001859a) */
int count_arguments(char **args)
{
    int count = 0;
    if (args) while (*args++) ++count;
    return count;
}


/* [0001859a,000186cb) */
int has_nologo(char **args, int count)
{
    int i, found;
    FILE *file;
    char buffer[1024], *p, *s;
    for (i = 0; i < count; ++i) {
        s = args[i];
        if ((*s == '-' || *s == '/') && !strcmp(s + 1, "nologo")) return 1;
        if (*s == '@') {
            found = 0;
            file = fopen(s + 1, "r");
            if (file) {
                while (fgets(buffer, 1023, file)) {
                    p = strstr(buffer, "nologo");
                    if (!p || p <= buffer || (p[-1] != '-' && p[-1] != '/')) continue;
                    if (p != buffer + 1 && !strchr(" \t\"", p[-2])) continue;
                    if (p[6] && !strchr(" \t\"\n", p[6])) continue;
                    found = 1;
                    break;
                }
                fclose(file);
            }
            if (found) return 1;
        }
    }
    return 0;
}


/* [00018e74,00018f2b) */
void banner(int ide)
{
    char *name = "Microsoft (R) ";
    char *product = "32-bit C/C++ Optimizing Compiler Version ";
    char *version = "8.00      ";
    char *copyright = "\nCopyright (c) Microsoft Corp ";
    char *years = "1984-1993. All rights reserved.\n\n";
    extern char ide_buffer[2048];
    if (!ide) {
        write_message(2, name);
        write_message(2, product);
        write_message(2, version);
        write_message(2, copyright);
        write_message(2, years);
    } else {
        ide_message(0);
        sprintf(ide_buffer, "%s %s %s\n", name, product, version);
        ide_message(1, ide_buffer);
        sprintf(ide_buffer, "%s %s\n", copyright, years);
        ide_message(2, ide_buffer);
    }
}
char ide_buffer[2048];


/* [0001a8e9,0001a910) */
void write_message(int fd, char *format, ...)
{
    unsigned length;
    char *message;
    va_list ap;
    va_start(ap, format);
    message = format_message(&length, format, ap);
    write(fd, message, length);
}


/* [0001a81a,0001a8e9) */
char *format_message(unsigned *length, char *format, va_list ap)
{
    static char buffer[2048];
    char *out = buffer, *s;
    int spec;
    while (*format) {
        if (*format != '%') *out++ = *format++;
        else {
            ++format;
            spec = *format++;
            switch (spec) {
            case 'c': *out++ = va_arg(ap, int); break;
            case 'd': case 'x':
                out += unsigned_digits(va_arg(ap, unsigned), out, spec == 'd' ? 10 : 16);
                break;
            case 's':
                s = va_arg(ap, char *);
                strcpy(out, s);
                out += strlen(s);
                break;
            default: *out++ = '%'; *out++ = spec; break;
            }
        }
    }
    *out = 0;
    *length = out - buffer;
    if ((int)*length >= 2048) fatal(0);
    return buffer;
}


/* [00018e6e,00018e74) */
char *program_name(char *arg)
{
    return _pgmptr;
}


/* [00018d89,00018e6e) */
Driver *initialize_driver(char *exe, Driver *d)
{
    char temp[260], *s;
    int length;
    extern char *concatenate(char *, char *);
    memset(d, 0, 0x170);
    d->directory = program_directory(exe);
    d->error_file = join_path(d->directory, "cl.err");
    d->include_path = getenv("INCLUDE");
    if (!d->temporary) {
        s = getenv("TMP");
        if (s) {
            strcpy(temp, s);
            length = strlen(temp);
            if (temp[length-1] != '\\' && temp[length-1] != ':' && temp[length-1] != '/') {
                temp[length] = '\\';
                temp[length+1] = 0;
            }
            d->temporary = concatenate(temp, "XXXXXX");
        } else d->temporary = copy_string("XXXXXX");
        d->temporary = mktemp(d->temporary);
    }
    return d;
}


/* [00018f4f,00018fd6) */
char *program_directory(char *exe)
{
    char drive[260], directory[256], filename[256], extension[256];
    int n;
    _splitpath(exe, drive, directory, filename, extension);
    n = strlen(directory);
    if (n > 0 && directory[n-1] != '\\' && directory[n-1] != ':' && directory[n-1] != '/') {
        directory[n] = '\\';
        directory[n+1] = 0;
    }
    return copy_string(strcat(drive, directory));
}


/* [0001ac6b,0001ac84) */
char *copy_string(char *s)
{
    return s ? strdup(s) : NULL;
}


/* [00018bdd,00018ca0) */
char *join_path(char *dir, char *name)
{
    char buffer[260];
    extern int search_path(char *, char *, int);
    strcpy(buffer, dir);
    strcat(buffer, name);
    if (!access(buffer, 4)) return copy_string(buffer);
    buffer[0] = '.'; buffer[1] = '\\'; buffer[2] = 0;
    strcat(buffer, name);
    if (!access(buffer, 4)) return copy_string(buffer);
    if (search_path(name, buffer, 260)) return strdup(buffer);
    return copy_string(name);
}


/* [00019244,00019306) */
int search_path(char *name, char *buffer, int limit)
{
    char *path = getenv("PATH"), *out;
    int n;
    if (path) while (*path) {
        while (*path == ' ' || *path == ';') ++path;
        if (!*path) break;
        out = buffer;
        while (*path && *path != ';') *out++ = *path++;
        *out = 0;
        if (!*buffer) continue;
        n = strlen(buffer);
        if (buffer[n-1] != '\\' && buffer[n-1] != ':' && buffer[n-1] != '/') {
            buffer[n] = '\\'; buffer[n+1] = 0;
        }
        strcat(buffer, name);
        n = strlen(buffer);
        if (n >= limit) fatal(0);
        if (!access(buffer, 4)) return n;
    }
    *buffer = 0;
    return 0;
}


/* [0001a3e8,0001a431) */
char *concatenate(char *a, char *b)
{
    char *out;
    unsigned size = strlen(b);
    size += strlen(a) + 1;
    out = allocate(size);
    strcpy(out, a);
    strcat(out, b);
    return out;
}


/* [0001a431,0001a455) */
void *allocate(unsigned size)
{
    void *p = malloc(size);
    if (!p) fatal(0);
    return p;
}


/* [00018335,0001842b) */
int input_count;
int mixed_options, mixed_warning;
void parse_arguments(char **args, int count)
{
    int i = 0;
    char *arg;
    int previous;
    AttachedOption *kind;
    while (i < count) {
        arg = args[i];
        if (*arg == '@') { read_response_file(arg + 1); ++i; continue; }
        previous = input_count;
        if (*arg != '-' && *arg != '/') {
            add_input(arg, classify_input(arg));
            ++i;
        } else {
            kind = scan_option_kind(arg[1]);
            if (kind && arg[2]) { ++i; parse_attached_option(kind, arg); }
            else parse_option(args, &i);
            if (input_count && input_count == previous) mixed_options = 1;
        }
        if (mixed_options && input_count != previous && !mixed_warning) {
            warning(4026);
            mixed_warning = 1;
        }
    }
}


/* [000181e5,00018215) */
AttachedOption *scan_option_kind(char ch)
{
    static AttachedOption kinds[] = {
        { 'G', "3:4:5:d:e:f:h*:p#:r:s*:t#:y:z:E*" },
        { 'O', "1:2:a-:b#:d:g-:i-:p-:s:t:w-:x:y-:V#" },
        { 'Z', "7:a:d:e:g:i:l:n:p#:s" },
        { 0, NULL }
    };
    AttachedOption *p;
    for (p = kinds; p->patterns; ++p) if ((char)p->letter == ch) return p;
    return NULL;
}


/* [00017f87,000181e5) */
int parse_option(char **args, int *index)
{
    OptionRule *rule = find_rule(args, *index), *old_rule;
    Item *item, *expanded, *selected, *removed, *p, *last;
    Item *result = NULL;
    if (!rule) {
        append_items(&pending, new_item(args[*index], NULL, 0, 0));
        ++*index;
        return 0;
    }
    item = read_option(rule, args, index);
    if (!item) return 1;
    if (item->flags & 2) { append_items(&pending, item); return 0; }
    item->origin = rule;
    append_items(&driver->other_options, item);
    if (rule->expansion) {
        result = expand_options(rule->expansion, item);
        if (result) {
            removed = NULL;
            free_items(select_options(driver, result, &removed));
            free_items(removed);
            for (p = result; p; p = p->next) p->origin = item;
        }
    }
    if (rule->conflicts) {
        expanded = expand_options(rule->conflicts, item);
        removed = NULL;
        selected = select_options(driver, expanded, &removed);
        if (selected) {
            last = NULL;
            for (p = selected; p; p = p->next) {
                if (p->origin) ((Item *)p->origin)->flags |= 0x10;
                if (!(p->flags & 1)) {
                    if (!last || p->origin != last) warn_override(4025, p, item);
                    last = p->origin;
                }
            }
            free_items(selected);
        }
        free_items(expanded);
        for (p = removed; p; p = p->next) {
            if (p->origin) {
                old_rule = ((Item *)p->origin)->origin;
                if (old_rule->conflicts) {
                    expanded = expand_options(old_rule->conflicts, p);
                    free_items(select_options(&result, expanded, NULL));
                    free_items(expanded);
                }
            }
        }
        free_items(removed);
    }
    if (rule->requires) {
        expanded = expand_options(rule->requires, item);
        selected = select_options(driver, expanded, NULL);
        if (selected) {
            for (p = selected; p; p = p->next) {
                if (p->origin) ((Item *)p->origin)->flags |= 0x10;
                if (!(p->flags & 1)) warn_override(2016, p, item);
            }
            free_items(selected);
        }
        free_items(expanded);
    }
    if (result) append_items(&driver->options, result);
    return 1;
}


/* [00017a6d,00017ad4) */
OptionRule *find_rule(char **args, int index)
{
    int best = 0, length;
    OptionRule *rule, *result = NULL;
    for (rule = option_rules; rule->syntax; ++rule) {
        if (rule->syntax[rule->syntax[0] == '^'] != args[index][1]) continue;
        length = match_rule(rule, args, index);
        if (length > 0 && (!best || best < length)) { best = length; result = rule; }
    }
    return result;
}


/* [00017a14,00017a6d) */
int match_rule(OptionRule *rule, char **args, int index)
{
    char *argument = args[index], *p = argument + 1, *syntax = rule->syntax;
    if (*syntax == '^') { if (!show_commands) return 0; ++syntax; }
    while (*syntax != ':' && *syntax != '!') {
        if (*p != *syntax) return 0;
        if (!*syntax) break;
        ++p; ++syntax;
    }
    if (!*syntax && *p != ' ' && *p) return 0;
    return p - argument;
}


/* [00017866,00017a14) */
Item *read_option(OptionRule *rule, char **args, int *index)
{
    char name[32], value[260], *out = name, *arg = args[*index] + 1, *spec = rule->syntax;
    int optional, next = 0, allow_switch = 0;
    unsigned flags = 0;
    if (*spec == '^') ++spec;
    while (*spec && *spec != ':' && *spec != '!') {
        if (*arg != *spec) fatal(0);
        *out++ = *arg++;
        ++spec;
    }
    *out = 0;
    if (!*spec) { ++*index; return new_item(name, NULL, 0, 0); }
    switch (*spec++) {
    case ':': optional = 1; break;
    case '!': optional = 0; break;
    default: fatal(0); optional = 0;
    }
    if (*spec == '?') {
        flags = 2; ++spec;
        memmove(name + 1, name, 31);
        name[0] = args[*index][0];
    }
    switch (*spec++) {
    case '#': next = 0; allow_switch = 1; break;
    case '$': next = 0; allow_switch = 0; break;
    case '*': next = 1; allow_switch = 0; break;
    case '+': next = 1; allow_switch = 1; break;
    case '@': return ((Item *(__cdecl *)(char **, int *))rule->special)(args, index);
    }
    ++*index;
    if (next && !*arg) { arg = args[*index]; ++*index; }
    if (!arg || !*arg || (!allow_switch && (*arg == '-' || *arg == '/'))) {
        if (!optional) { fatal(2004, name); return NULL; }
        return new_item(name, NULL, 0, flags);
    }
    strcpy(value, arg);
    if (*spec) validate_value(name, value, spec);
    return new_item(name, value, 0, flags);
}


/* [00017668,000176d4) */
Item *recycled_items;
Item *new_item(char *name, char *value, char *groups, unsigned flags)
{
    Item *item;
    if (recycled_items) { item = recycled_items; recycled_items = item->next; }
    else item = allocate(0x18);
    memset(item, 0, 0x18);
    item->flags = flags;
    item->extra = copy_string(value);
    item->name = copy_string(name);
    item->value.groups = copy_string(groups);
    return item;
}


/* [000176f8,00017713) */
void append_items(Item **head, Item *item)
{
    while (*head) head = &(*head)->next;
    *head = item;
}


/* [00017c8b,00017d99) */
Item *expand_options(char *spec, Item *item)
{
    Item *result = NULL;
    char groups[24], name[32], *p, *out, *extra;
    unsigned flags;
    groups[0] = 0;
    while (*spec) {
        flags = 0;
        p = skip_space(spec);
        if (!*p) break;
        if (*p != '=' && *p != '#' && *p != '-') {
            out = groups;
            while (*p != '=' && *p != '#' && *p != '-') *out++ = *p++;
            *out = 0;
        }
        if (*p == '#') flags = 8;
        else if (*p == '-') flags = 1;
        spec = expand_text(p + 1, name, item->name);
        if (*spec == ':' || *spec == '!') {
            if (*spec == '!') flags |= 4;
            spec = expand_text(spec + 1, ide_buffer, item->extra);
            extra = strlen(ide_buffer) ? ide_buffer : NULL;
        } else extra = NULL;
        if (*spec == ',') ++spec;
        append_items(&result, new_item(name, extra, groups, flags));
    }
    return result;
}


/* [0001a382,0001a3a1) */
char *skip_space(char *s)
{
    while (*s && (*s == ' ' || *s == '\t')) ++s;
    return s;
}


/* [00017b26,00017c03) */
char *expand_text(char *spec, char *out, char *replacement)
{
    char *p = out, *alternate = NULL;
    int c;
    *out = 0;
    while (*spec) {
        c = *spec++;
        switch (c) {
        case ':': case '!': case ')': case ',': case '|':
            --spec;
            goto done;
        case '(':
            *p = 0;
            spec = expand_choice(spec - 1, p, replacement, &alternate);
            p = out + strlen(out);
            break;
        case '*':
            *p = 0;
            p = append_optional(out, replacement);
            break;
        case '<':
            spec = expand_extension(spec, out, alternate);
            p = out + strlen(out);
            break;
        case '\\': *p++ = *spec++; break;
        case '@': break;
        default: *p++ = c; break;
        }
    }
done:
    *p = 0;
    return spec;
}


/* [0001a3a1,0001a3c7) */
char *append_optional(char *out, char *s)
{
    if (s) strcat(out, s);
    return out + strlen(out);
}


/* [00017e8d,00017f2a) */
Item *select_options(void *context, Item *spec, Item **removed)
{
    Item **head = context;
    Item *result = NULL, *item, *next, *p;
    for (; spec; spec = spec->next) {
        for (item = *head; item; item = next) {
            next = item->next;
            if (!match_item(item, spec)) continue;
            if (removed && (spec->flags & 1) && !(item->flags & 1)) {
                if (!item->origin) continue;
                for (p = *removed; p; p = p->next)
                    if (p->origin == item->origin) break;
                if (!p) append_items(removed, copy_item(item->origin));
            } else append_items(&result, detach_item(head, item));
        }
    }
    return result;
}


/* [00017649,00017668) */
void free_items(Item *item)
{
    Item *next;
    while (item) { next = item->next; recycle_item(item); item = next; }
}


/* [00017d99,00017e8d) */
int match_item(Item *a, Item *b)
{
    int alen, blen;
    if (strcmp(a->name, "?") && strcmp(b->name, "?") && strcmp(a->name, b->name)) return 0;
    if (!a->extra) return !b->extra || !strcmp(b->extra, "?");
    if (!b->extra) return !strcmp(a->extra, "?");
    alen = strlen(a->extra);
    blen = strlen(b->extra);
    if (alen >= 1 && a->extra[alen-1] == '?') return !strncmp(a->extra, b->extra, alen-1);
    if (blen >= 1 && b->extra[blen-1] == '?') return !strncmp(a->extra, b->extra, blen-1);
    return !strcmp(a->extra, b->extra);
}


/* [00017767,00017866) */
void validate_value(char *name, char *value, char *spec)
{
    char *input = value, *end;
    int base = 10;
    unsigned number, low, high, swap;
    if (*spec++ == 'x' && input[0] == '0') {
        if (input[1] == 'D' || input[1] == 'T' || input[1] == 'd' || input[1] == 't') input += 2;
        else base = 0;
    }
    number = strtoul(input, &end, base);
    if (*end) { fatal(2021, name, value); return; }
    if (*spec++ != '[') { unsigned_digits(number, value, 10); return; }
    for (;;) {
        low = strtoul(spec, &end, 10);
        if (*end == '-') {
            high = strtoul(end + 1, &end, 10);
            if (high < low) { swap = low; low = high; high = swap; }
        } else high = low;
        if (low <= number && number <= high) { unsigned_digits(number, value, 10); return; }
        spec = end + 1;
        if (*end != ',') break;
    }
    warning(4014, number, name, low);
    unsigned_digits(low, value, 10);
}


/* [0001a7c5,0001a81a) */
int unsigned_digits(unsigned value, char *out, unsigned base)
{
    static char digits[] = "0123456789abcdef";
    char reversed[32], *p = reversed + 1;
    int length = 0;
    reversed[0] = 0;
    do { *p++ = digits[value % base]; value /= base; } while (value);
    while ((*out++ = *--p) != 0) ++length;
    return length;
}


/* [0001761c,00017649) */
void recycle_item(Item *item)
{
    release(item->extra);
    release(item->name);
    item->next = recycled_items;
    recycled_items = item;
}


/* [0001ac56,0001ac6b) */
void release(void *p)
{
    if (p) free(p);
}


/* [00018215,00018335) */
void parse_attached_option(AttachedOption *kind, char *arg)
{
    char buffer[32], *args[2], *s = arg, *out, *pattern;
    int index;
    args[0] = buffer; args[1] = NULL;
    buffer[0] = *s++;
    buffer[1] = *s++;
    if (buffer[1] == 'O') {
        if (strstr(s, "y1")) warning(4099, "Oy1", "Oy-");
        else if (strstr(s, "y2")) warning(4099, "Oy2", "Oy");
    }
    while (*s) {
        pattern = strchr(kind->patterns, *s);
        buffer[2] = *s++;
        out = buffer + 3;
        if (pattern) {
            ++pattern;
            while (*pattern && *pattern != ':') {
                if (*pattern == '#') {
                    while (is_decimal(*s)) *out++ = *s++;
                } else if (*pattern == '*') {
                    while (*s) *out++ = *s++;
                    break;
                } else {
                    if (*s != *pattern) break;
                    *out++ = *s++;
                }
                ++pattern;
            }
        }
        *out = 0;
        index = 0;
        parse_option(args, &index);
    }
}


/* [0001a34d,0001a366) */
int is_decimal(int c)
{
    return c >= '0' && c <= '9';
}


/* [00017ad4,00017b26) */
char *expand_choice(char *spec, char *out, char *replacement, char **alternate)
{
    static char fallback[32];
    char *end;
    *out = 0;
    end = expand_text(spec + 1, out, replacement);
    end = expand_text(end + 1, fallback, replacement);
    if (!*out) strcpy(out, fallback);
    *alternate = fallback;
    return end + 1;
}


/* [00017c03,00017c8b) */
char *expand_extension(char *spec, char *out, char *alternate)
{
    char ignored[256], *p = last_of(out, "\\/"), *dest;
    if (p && !p[1]) dest = append_optional(out, alternate ? alternate : "%b");
    else {
        if (!p) p = out;
        dest = strrchr(p, '.') ? ignored : out + strlen(out);
    }
    *dest++ = '.';
    while (*spec && *spec != ',') *dest++ = *spec++;
    *dest = 0;
    return spec;
}


/* [0001a54d,0001a587) */
char *last_of(char *s, char *set)
{
    char *result = NULL, *p;
    for (; *set; ++set) {
        p = strrchr(s, *set);
        if (p && (!result || p > result)) result = p;
    }
    return result;
}


/* [000190d0,00019198): /D callback. */
int option_callback_190d0(char **args, int *index)
{
    char *s = args[*index] + 2, *p;
    Item *item;
    ++*index;
    if (!*s) { s = args[*index]; ++*index; }
    if (s && *s) {
        p = strchr(s, '#');
        if (p) *p = '=';
        p = strchr(s, '=');
        if (p) *p = 0;
        item = new_item("D", s, NULL, 0);
        remove_define(item);
        recycle_item(item);
        if (p) *p = '=';
        append_items(&driver->options, new_item("D", s, "1P", 4));
    } else fatal(2004, "D");
    return 0;
}


/* [00017100,0001713c) */
void remove_define(Item *item)
{
    Item *spec = expand_options("=D:*,=D:*=?", item);
    free_items(select_options(driver, spec, NULL));
    free_items(spec);
}


/* [00018ca0,00018d89) */
void finish_options(Driver *d)
{
    char buffer[512];
    static char driver_selector[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'D'};
    Item *item;
    DriverSwitch *entry;
    int found;
    for (item = d->options; item; item = item->next) {
        if (!is_for_pass(item, driver_selector)) continue;
        found = 0;
        for (entry = driver_switches; entry->name && !found; ++entry) {
            if (item->name[0] != entry->name[0] || strcmp(item->name, entry->name)) continue;
            found = 1;
            switch (entry->kind) {
            case 1: *(int *)entry->target = 1; break;
            case 2: *(int *)entry->target = 0; break;
            case 3:
                if (item->extra) {
                    expand_filename(buffer, item->extra, d);
                    *(char **)entry->target = copy_string(buffer);
                }
                break;
            case 4: ((void (__cdecl *)(Item *))entry->target)(item); break;
            }
        }
        if (!found) fatal(0);
    }
}


/* [00017713,0001773c) */
int is_for_pass(Item *item, void *pass)
{
    char *p = item->value.groups;
    while (*p) if (*p++ == *((char *)pass + 16)) return 1;
    return 0;
}


/* [00019d9f,00019f3c) */
char *expand_filename(char *out, char *spec, Driver *d)
{
    Input *input = d->current_input;
    char *filename = input ? input->filename : "";
    int kind = input ? input->kind : 7;
    char buffer[260], *p = out, *s, *temporary;
    while (*spec) {
        if (*spec++ != '%') { *p++ = spec[-1]; continue; }
        *p = 0;
        switch (*spec++) {
        case 'B':
            filename_stem(buffer, filename);
            s = strupr(buffer);
            p = append_optional(p, s);
            break;
        case 'X': p = append_optional(p, make_dll ? "dll" : "exe"); break;
        case 'b':
            filename_stem(buffer, filename);
            p = append_optional(p, buffer);
            break;
        case 'e':
            s = d->message_files[kind][d->pass_index];
            p = append_optional(p, s ? s : "");
            break;
        case 'f': p = append_optional(p, filename); break;
        case 'm':
            temporary = allocate(260);
            expand_filename(temporary, executable_file, d);
            map_filename(p, temporary);
            p += strlen(p);
            release(temporary);
            break;
        case 't': p = append_optional(p, d->temporary); break;
        case 'x':
            output_stem(buffer, executable_file);
            p = append_optional(p, buffer);
            break;
        }
    }
    *p = 0;
    return out;
}


/* [0001a2de,0001a2fc) */
char *filename_stem(char *out, char *name)
{
    _splitpath(name, NULL, NULL, out, NULL);
    return out;
}


/* [000171fc,00017274): output-name restrictions for multiple inputs. */
void driver_switch_171fc(Item *item)
{
    char *p;
    if (*item->extra) {
        if ((int)input_count >= 2) {
            p = strchr(item->extra, ' ');
            *p++ = 0;
            if (!strstr(p, "%b") && !strchr("\\/", p[strlen(p)-1]))
                fatal(2036, item->extra, p);
        }
        *item->extra = 0;
    }
}


/* [000172e4,00017368): copy pass enablement between compiler formats. */
void driver_switch_172e4(Item *item)
{
    char from = item->extra[0], to = item->extra[1];
    Profile *profile;
    PassRecord *pass;
    int enabled = 0, found = 0;
    for (profile = profiles; profile->extension && !found; ++profile) {
        for (pass = profile->passes; pass->executable && !found; ++pass) {
            if ((char)pass->letter_prefix == from) { enabled = pass->enabled; found = 1; }
        }
    }
    if (enabled) {
        for (profile = profiles; profile->extension; ++profile)
            for (pass = profile->passes; pass->executable; ++pass)
                if ((char)pass->letter_prefix == to) pass->enabled = enabled;
    }
}


/* [00017274,000172e4): disable named passes, or every pass for '.'. */
void driver_switch_17274(Item *item)
{
    Profile *profile;
    PassRecord *pass;
    int all = item->extra[0] == '.';
    for (profile = profiles; profile->extension; ++profile)
        for (pass = profile->passes; pass->executable; ++pass)
            if (all || strchr(item->extra, (char)pass->letter_prefix)) pass->enabled = 0;
}


/* [0001a989,0001aa63): print paginated CL32.MSG help. */
void load_messages(char *name)
{
    char line[84];
    FILE *file = fopen(name, "r");
    int rows, n = 0, c;
    if (!file) fatal(2022, name);
    rows = console_rows() - 1;
    if (rows < 1) rows = 1;
    while (fgets(line, 81, file)) {
        if (n && n % rows == 0) {
            write_message(1, "  (press <return> to continue)");
            do { c = getchar(); } while (c != '\n' && c != EOF);
        }
        ++n;
        write_message(1, line);
    }
    fclose(file);
    exit(0);
}


/* [0001b01d,0001b081) */
int console_rows(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE console;
    BOOL ok;
    int rows;
    console = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (console == INVALID_HANDLE_VALUE) return 25;
    ok = GetConsoleScreenBufferInfo(console, &info);
    CloseHandle(console);
    if (!ok) return 25;
    rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    return rows > 0 ? rows : 1;
}

/* [00017557,000175ae) */
int classify_input(char *name)
{
    char *extension = name + strlen(name);
    int kind;
    while (extension >= name) {
        if (*extension == '.') {
            ++extension;
            for (kind = 0; profiles[kind].extension; ++kind)
                if (!stricmp(extension, profiles[kind].extension)) return kind;
            break;
        }
        --extension;
    }
    return 7;
}


/* [000175dd,0001761c) */
void add_input(char *name, int kind)
{
    Input *input = new_input(name, kind), **tail = &driver->inputs;
    while (*tail) tail = &(*tail)->next;
    *tail = input;
    if (!profiles[kind].link) ++input_count;
}


/* [000175ae,000175dd) */
Input *new_input(char *name, int kind)
{
    Input *input = allocate(12);
    input->kind = kind;
    input->filename = copy_string(name);
    input->next = NULL;
    return input;
}


/* [00018a85,00018bdd): resolve option prerequisites after driver actions. */
void prepare_inputs(void)
{
    Item *item, *requirements, *p, *expanded;
    OptionRule *rule;
    char *name, *description;
    for (item = driver->other_options; item; item = item->next) {
        rule = item->origin;
        if ((item->flags & 0x10) || !rule->special || !rule->expansion) continue;
        requirements = expand_options(rule->special, item);
        if (!any_match(driver->other_options, requirements)) {
            if (!item->flags) {
                name = allocate(80);
                description = allocate(80);
                name[0] = '/';
                strcpy(name + 1, item->name);
                for (p = requirements; p; p = p->next) {
                    if (p == requirements) strcpy(description, "/");
                    else strcat(description, p->next ? ", /" : " or /");
                    strcat(description, p->name);
                }
                warning(4007, name, description);
                release(description);
                release(name);
            }
            expanded = expand_options(rule->expansion, item);
            free_items(select_options(driver, expanded, NULL));
            free_items(expanded);
        }
        free_items(requirements);
    }
}


/* [0001b007,0001b01d) */
void prepare_execution(void)
{
    extern int global_26078;
    if (global_26078) dup2(1, 2);
}


/* [000193cd,0001959e) */
void run_driver(Driver *d)
{
    extern int global_22400;
    Input *input, *next, *created;
    PassRecord *passes;
    int failed = 0;
    if (global_22400) { initialize_ide(); banner(1); }
    for (input = d->inputs; input; input = next) {
        next = input->next;
        if (input->kind == 3 && !d->first_link) d->first_link = input;
        if (!profiles[input->kind].link) {
            d->current_input = input;
            write_message(2, input->filename);
            write_message(2, "\n");
            if (global_22400) {
                sprintf(ide_buffer, "Compiling: %s\n", input->filename);
                ide_message(3, ide_buffer);
            }
            passes = profiles[input->kind].passes;
            if (passes) {
                errors = run_passes(passes, d, compile_pass);
                if (errors) failed = errors;
                finish_options(d);
                if (object_file) {
                    created = new_input(object_file, 3);
                    created->next = input->next;
                    input->next = created;
                    if (!d->first_link) d->first_link = created;
                }
                cleanup_files();
            }
        }
    }
    if (failed) { errors = failed; return; }
    d->current_input = d->first_link ? d->first_link : d->inputs;
    finish_options(d);
    passes = profiles[3].passes;
    if (passes) {
        errors = run_passes(passes, d, link_pass);
        if (errors) return;
        if (executable_file) {
            created = new_input(executable_file, 8);
            created->next = d->inputs->next;
            d->inputs->next = created;
        }
    }
    for (input = d->inputs; input; input = next) {
        next = input->next;
        if (profiles[input->kind].link == 2) {
            d->current_input = input;
            passes = profiles[input->kind].passes;
            if (passes) {
                errors = run_passes(passes, d, compile_pass);
                if (errors) return;
            }
        }
    }
}


/* [00019383,000193cd) */
int run_passes(PassRecord *passes, Driver *d, int (__cdecl *callback)(Driver *))
{
    int index;
    for (index = 0; passes[index].executable; ++index) {
        if (!passes[index].enabled) continue;
        d->pass_index = index;
        d->current_pass = &passes[index];
        initialize_pass(d);
        errors = callback(d);
        if (errors) break;
    }
    return errors;
}


/* [00019306,00019383) */
void initialize_pass(Driver *d)
{
    PassRecord *pass = d->current_pass;
    int kind = d->current_input->kind, index = d->pass_index;
    char *directory;
    if (!d->programs[kind][index]) d->programs[kind][index] = join_path(d->directory, pass->executable);
    if (pass->field14) d->loaders[kind][index] = join_path(d->directory, pass->field14);
    if (pass->alternate) {
        directory = program_directory(d->programs[kind][index]);
        d->message_files[kind][index] = join_path(directory, pass->alternate);
        release(directory);
    }
}


/* [0001959e,00019980): construct and invoke one compiler phase. */
int compile_pass(Driver *d)
{
    extern int global_2601c, global_26040, global_26034, global_2607c;
    char *expanded = allocate(260), *quoted = allocate(260);
    PassRecord *pass = d->current_pass;
    int kind = d->current_input->kind, index = d->pass_index;
    char *program = d->programs[kind][index], *loader = d->loaders[kind][index];
    unsigned size = strlen(program) + 1, count = 0, length;
    Item *item;
    char **arguments, **tail, **trace;
    char *storage, *pos, *part, *separator;
    int result = 0;
    output_produced = 1;
    if (loader) { size += strlen(loader) + 1; ++count; }
    for (item = d->options; item; item = item->next) {
        if (!is_for_pass(item, pass)) continue;
        ++count;
        size += expanded_length(item->name, d) + 2;
        if (item->extra) { ++count; size += expanded_length(item->extra, d) + 2; }
    }
    if (d->include_path && global_2601c) {
        count += 2;
        size += nonblank_length(d->include_path) + 6;
        part = d->include_path;
        while ((separator = strchr(part, ';')) != NULL) {
            count += 2; size += 6; part = separator + 1;
        }
    }
    arguments = allocate(count * 4 + 8);
    tail = arguments;
    storage = pos = allocate(size);
    if (loader) { *tail++ = pos; pos = append_string(pos, loader) + 1; }
    *tail++ = pos;
    pos = append_string(pos, program);
    for (item = d->options; item; item = item->next) {
        if (!is_for_pass(item, pass)) continue;
        *tail++ = ++pos;
        if (!(item->flags & 8)) *pos++ = (char)(pass->letter_prefix >> 8);
        expand_filename(expanded, item->name, d);
        pos = append_string(pos, expanded);
        if (item->extra) {
            if (!(item->flags & 4)) *tail++ = ++pos;
            expand_filename(quoted, item->extra, d);
            quote_if_space(expanded, quoted);
            pos = append_string(pos, expanded);
        }
    }
    if (d->include_path && global_2601c &&
        ((char)pass->letter_prefix == '1' || (char)pass->letter_prefix == 'P')) {
        part = d->include_path;
        for (;;) {
            separator = strchr(part, ';');
            length = separator ? (unsigned)(separator - part) : strlen(part);
            if (length > strspn(part, " \t")) {
                *tail++ = ++pos;
                pos = append_string(pos, "-I") + 1;
                *tail++ = pos;
                if (separator) {
                    memcpy(pos, part, length);
                    pos[length] = 0;
                    pos += length + 1;
                } else pos = append_string(pos, part);
            }
            if (!separator) break;
            part = separator + 1;
        }
    }
    *tail = NULL;
    if (global_26040) {
        trace = arguments;
        write_message(2, "`");
        if (loader) { write_message(2, "%s ", loader); ++trace; }
        write_message(2, "%s", program);
        for (++trace; *trace; ++trace) write_message(2, " %s", *trace);
        write_message(2, "'\n");
    }
    if (!global_26034) {
        if (global_2607c) start_timer();
        result = spawn_pass(loader ? 3 : 0, loader ? loader : program,
                            loader ? NULL : pass->parameter, arguments);
        if (global_2607c) report_timer(program);
    }
    release(arguments);
    release(storage);
    release(quoted);
    release(expanded);
    return result;
}


/* [0001a1d1,0001a2de): reserve enough room for expansion and quoting. */
unsigned expanded_length(char *text, Driver *d)
{
    char buffer[260], *filename = d->current_input ? d->current_input->filename : "", *s;
    int kind = d->current_input ? d->current_input->kind : 7;
    unsigned length = 0;
    while (*text) {
        switch (*text++) {
        case '"': case '\\': length += 2; break;
        case '%':
            switch (*text++) {
            case 'B': case 'b':
                filename_stem(buffer, filename);
                length += nonblank_length(buffer);
                break;
            case 'X': length += 3; break;
            case 'e':
                s = d->message_files[kind][d->pass_index];
                if (s) length += nonblank_length(s);
                break;
            case 'f': length += nonblank_length(filename); break;
            case 'm': length += 260; break;
            case 't': length += nonblank_length(d->temporary); break;
            case 'x':
                output_stem(buffer, executable_file);
                length += nonblank_length(buffer);
                break;
            }
            break;
        default: ++length; break;
        }
    }
    return length;
}


/* [0001a455,0001a4ae): size after command-line escaping. */
unsigned nonblank_length(char *text)
{
    unsigned length = 0, slashes = 0;
    int quoted = 0;
    for (; *text; ++text) {
        if (is_space(*text)) quoted = 1;
        if (*text == '"') length += slashes + 1;
        if (*text == '\\') ++slashes;
        else slashes = 0;
        ++length;
    }
    if (quoted) length += slashes + 2;
    return length;
}


/* [0001a366,0001a382) */
int is_space(int c)
{
    return c == ' ' || c == '\t';
}


/* [0001a3c7,0001a3e8) */
char *append_string(char *dst, char *src)
{
    strcpy(dst, src);
    return dst + strlen(dst);
}


/* [0001a4ae,0001a54d) */
char *quote_if_space(char *out, char *text)
{
    char *p = out;
    int slashes = 0, quoted = 0;
    while (*text) {
        if (is_space(*text)) quoted = 1;
        if (*text == '"') while (slashes-- >= 0) *p++ = '\\';
        if (*text == '\\') ++slashes;
        else slashes = 0;
        *p++ = *text++;
    }
    if (quoted) {
        memmove(out + 1, out, p - out);
        *out = '"';
        ++p;
        while (slashes-- > 0) *p++ = '\\';
        *p++ = '"';
    }
    *p = 0;
    return out;
}


/* [0001ae65,0001afcc): run a pass using its environment-variable protocol. */
int child_running, child_interrupted;
int spawn_pass(int mode, char *program, char *environment, char **arguments)
{
    char buffer[2048], *environment_block[2], *short_arguments[2], *p;
    char **arg;
    int inherited, result;
    switch (mode) {
    case 0:
        inherited = 0;
        if (environment) {
            p = append_string(buffer, environment);
            for (arg = arguments + 1; *arg; ++arg) {
                p = append_string(p, *arg);
                *p++ = ' ';
            }
            *p = 0;
            environment_block[0] = buffer;
            environment_block[1] = NULL;
            short_arguments[0] = arguments[0];
            short_arguments[1] = NULL;
            arguments = short_arguments;
        } else environment_block[0] = NULL;
        break;
    case 1: case 3: inherited = 1; break;
    default: fatal(0); inherited = 0;
    }
    flushall();
    errno = 0;
    child_interrupted = 0;
    child_running = 1;
    if (inherited) result = spawnvp(0, program, (const char * const *)arguments);
    else result = spawnvpe(0, program, (const char * const *)arguments,
                           (const char * const *)environment_block);
    child_running = 0;
    if (child_interrupted || ((result >> 8) & 255) == 0x80) {
        driver_exit(4);
        fatal(0);
    }
    if (result < 0 && errno) { fatal(2027, program); return 4; }
    if (result) {
        if (result == 525) fatal(2030, program);
        result = 2;
    }
    return result;
}


/* [0001a6b8,0001a757) */
void cleanup_files(void)
{
    extern int global_2602c;
    static char *extensions[] = { "ex", "sy", "gl", "in", "st", "db", "lk", "df.def", NULL };
    char buffer[260], *end, **extension;
    if (global_2602c || !driver->temporary) return;
    if (strlen(driver->temporary) >= 260) fatal(0);
    strcpy(buffer, driver->temporary);
    end = buffer + strlen(buffer);
    for (extension = extensions; *extension; ++extension) {
        strcpy(end, *extension);
        unlink(buffer);
    }
}


/* [00019980,00019ae1): write a response file and invoke a linker. */
int link_pass(Driver *d)
{
    extern int global_26040, global_26034, global_2607c;
    char command[2048], *pos = command, **arguments;
    int kind = d->current_input->kind, index = d->pass_index, result = 0;
    PassRecord *pass = d->current_pass;
    char *program = d->programs[kind][index], *loader = d->loaders[kind][index];
    output_produced = 1;
    if (loader) pos = append_string(pos, loader);
    pos = append_string(pos, program);
    if (coff_format) {
        write_coff_response(d);
        pos = append_string(pos, " -link");
    } else write_omf_response(d);
    *pos++ = ' ';
    *pos = 0;
    pos = append_string(pos, "@");
    pos = append_string(pos, d->temporary);
    append_string(pos, "lk");
    if (strlen(command) >= 2048) fatal(0);
    arguments = split_arguments(command);
    if (global_26040) write_message(2, "`%s'\n", command);
    if (!global_26034) {
        if (global_2607c) start_timer();
        result = spawn_pass(1, loader ? loader : program, loader ? NULL : pass->parameter, arguments);
        if (global_2607c) report_timer(program);
    }
    release(arguments);
    return result;
}


/* [00019ffe,0001a1d1) */
void write_coff_response(Driver *d)
{
    extern int global_26034;
    char *buffer = allocate(260), *expanded = allocate(260);
    PassRecord *pass = d->current_pass;
    FILE *file;
    Item *item;
    Input *input;
    strcpy(buffer, d->temporary);
    strcat(buffer, "lk");
    if (global_26034) file = NULL;
    else {
        file = fopen(buffer, "w");
        if (!file) fatal(2018);
    }
    for (item = d->options; item; item = item->next) {
        if (!is_for_pass(item, pass)) continue;
        if (!(item->flags & 8)) file_message(file, "%c", (char)(pass->letter_prefix >> 8));
        file_message(file, "%s", expand_filename(buffer, item->name, d));
        if (item->extra) {
            if (!(item->flags & 4)) file_message(file, " ");
            quote_if_space(buffer, expand_filename(expanded, item->extra, d));
            file_message(file, "%s", buffer);
        }
        file_message(file, "\n");
    }
    for (input = d->inputs; input; input = input->next) {
        if (input->kind != 5) continue;
        quote_if_space(buffer, input->filename);
        file_message(file, "%cdef:%s\n", (char)(pass->letter_prefix >> 8), buffer);
    }
    for (input = d->inputs; input; input = input->next) {
        if (profiles[input->kind].link != 1 || input->kind == 5) continue;
        quote_if_space(buffer, input->filename);
        file_message(file, "%s\n", buffer);
    }
    if (file) fclose(file);
    release(expanded);
    release(buffer);
}


/* [0001a936,0001a989) */
void file_message(FILE *file, char *format, ...)
{
    extern int global_26040;
    unsigned length;
    char *message;
    va_list ap;
    va_start(ap, format);
    message = format_message(&length, format, ap);
    if (file) write(fileno(file), message, length);
    if (global_26040) write(2, message, length);
}


/* [0001a7b3,0001a7c5) */
void driver_exit(int status)
{
    cleanup_files();
    exit(status);
}

/* [0001773c,00017767): unlink a known list member and return it alone. */
Item *detach_item(Item **head, Item *item)
{
    Item *found = NULL;
    while (*head) {
        found = *head;
        if (found == item) {
            *head = found->next;
            found->next = NULL;
            break;
        }
        head = &found->next;
    }
    return found;
}

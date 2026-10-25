/* Browse-information stream setup and record transport. */
#include "browser.h"
#include "initialization.h"
#include "cpp-context.h"
#include "frontend.h"
#include <direct.h>
#include <recon.h>

/* Original 0x2bfe0..0x2c04f. Use the browser's separate temporary
   filename, with binary-default w+ access and ordinary CRT buffering. */
void begin_browser_output(void)
{
    char filename[256];
    format_browser_temporary_name(filename);
    browser_file = fopen(filename, "w+");
    if (!browser_file)
        report_fatal(83, message_text(340), filename);
    setvbuf(browser_file, 0, 0, 4096);
    begin_browser_records();
}

/* Original 0x2acd0..0x2ad37. Replace the final character of the
   requested browse filename with a distinct temporary suffix. */
void __stdcall format_browser_temporary_name(char *filename)
{
    char *source = options.option_Fr;
    char *cursor = filename;
    char *last;
    char suffix = 't';
    while ((*cursor++ = *source++) != 0)
        ;
    cursor = filename;
#ifdef RECON_NATIVE_TEST
    /* Empty names skip the original writer of entryESP-4 last. */
    if (!*cursor) { __asm { int 3 } }
#endif
    while (*cursor) last = cursor++;
    last[1] = 0;
    while (suffix == *last) {
        if (++suffix > 'z') suffix = 'a';
    }
    *last = suffix;
}

/* Original 0x2c3f0..0x2c3f9. Begin a browser stream through its
   ordinary queued-record interface. */
void begin_browser_records(void)
{
    queue_browser_record(0, 0);
}

int browser_record_count; /* original 0x94b3c: pending browser-ring entries */

/* Original 0x2bf10..0x2bfab. Keep adjacent records that may cancel
   or merge, and drain older entries once that lookahead window is stable. */
void __stdcall queue_browser_record(unsigned char kind, void *subject)
{
    BrowserRecordHeader *first, *second, *last, *previous;
    append_browser_record(kind, subject, source_line_number);
    first = first_browser_record();
    second = second_browser_record();
    last = browser_record_from_end(0);
    previous = browser_record_from_end(1);
    if (first && second) {
        if (first->kind == 4 && second->kind == 3) {
            if (second->flags & 1) pop_first_browser_record();
        } else {
            if (previous->kind == 8 && last->kind == 9) {
                pop_last_browser_record();
                pop_last_browser_record();
                return;
            }
            if (last->kind == 8 && previous->kind == 8) return;
        }
        while (browser_record_count > 2)
            emit_browser_record(pop_first_browser_record());
    }
}

/* Original 0x94b40, 0x94b44..0x94e43 and 0x9ebd4. A shared 32-entry
   ring, not a serialized byte image. Payload fields are encountered in the
   append/fill helpers; structure padding has no runtime comparison credit. */
int browser_record_head;
BrowserRecord browser_records[32];
int browser_records_suppressed;

/* Original 0x2bd00..0x2bd8f. Initialize the next ring slot, retaining
   flags other than owned-name bit 1, then fill it before publishing count. */
void __stdcall append_browser_record(unsigned char kind, void *subject, unsigned line)
{
    int index;
    BrowserRecord *record;
    if (browser_records_suppressed) return;
    index = browser_record_head + browser_record_count;
    if (index >= 32) index -= 32;
    record = &browser_records[index];
    record->header.kind = kind;
    record->line = line;
    record->name = 0;
    record->name_identity = 0;
    record->header.flags &= ~2;
    record->attributes = 0;
    record->category = 0;
    record->symbol_index = 0;
    fill_browser_record(record, subject);
    ++browser_record_count;
}

/* Original 0x2bbb0..0x2bc78 and 13-way switch 0x2bc7c..0x2bcaf.
   Each record kind has its own payload producer. The current source line
   overrides the provisional line supplied by the append interface. */
void __stdcall fill_browser_record(BrowserRecord *record, void *subject)
{
    record->line = source_line_number;
    switch (record->header.kind) {
    case 1: fill_browser_filename(record); break;
    case 3: fill_browser_declaration(record, (CppSymbol *)subject); break;
    case 4: fill_browser_reference(record, (CppSymbol *)subject); break;
    case 11: fill_browser_symbol(record, (CppSymbol *)subject); break;
    case 12: fill_browser_base(record, subject); break;
    case 13: fill_browser_symbol(record, (CppSymbol *)subject); break;
    case 0xfd: fill_browser_macro(record, (MacroDefinition *)subject); break;
    case 0xfe: fill_browser_macro_reference(record, (MacroDefinition *)subject); break;
    case 0xff: fill_browser_generated_symbol(record); break;
    }
}

/* Original 0x2bd90..0x2bdaf. The oldest live ring entry. */
BrowserRecordHeader *first_browser_record(void)
{
    if (browser_record_count < 1) return 0;
    return &browser_records[browser_record_head].header;
}

/* Original 0x2bdb0..0x2bdd8. The next entry after the ring head. */
BrowserRecordHeader *second_browser_record(void)
{
    int index;
    if (browser_record_count < 2) return 0;
    index = browser_record_head + 1;
    if (index >= 32) index -= 32;
    return &browser_records[index].header;
}

/* Original 0x2bde0..0x2be18. Index zero denotes the newest record. */
BrowserRecordHeader *__stdcall browser_record_from_end(int index)
{
    int slot = browser_record_head + browser_record_count - index - 1;
    if (index + 1 > browser_record_count) return 0;
    if (slot >= 32) slot -= 32;
    return &browser_records[slot].header;
}

/* Original 0x2c460..0x2c471. Queue a newly assigned macro identity. */
void __stdcall browser_macro(MacroDefinition *definition)
{
    queue_browser_record(0xfd, definition);
}

/* Original 0x2b6b0..0x2b6e3. Macro names use interner identity;
   the parameter-count byte is tested unsigned by the original CMP/SBB. */
void __stdcall fill_browser_macro(BrowserRecord *record, MacroDefinition *macro)
{
    record->header.flags &= ~2;
    record->name = macro->name;
    record->attributes = 0;
    record->name_identity = macro->name;
    record->category = (unsigned char)macro->parameter_count < 1 ? 5 : 6;
    record->symbol_index = macro->browser_index;
    record->header.kind = 3;
}

/* Original 0x2be50..0x2be8d. Remove the oldest live record without
   overwriting its payload, so the caller can emit it after removal. */
BrowserRecordHeader *pop_first_browser_record(void)
{
    BrowserRecordHeader *record;
    if (!browser_record_count) return 0;
    record = &browser_records[browser_record_head].header;
    ++browser_record_head;
    --browser_record_count;
    if (browser_record_head >= 32) browser_record_head = 0;
    return record;
}

unsigned browser_last_line = (unsigned)-1; /* original 0x9ebd8 */
CppSymbol *browser_last_reference; /* original 0x94b20 */
BrowserRecord browser_record_template; /* original 0x94b24: empty BSS template */

/* Original 0x2b5b0..0x2b65f and 17-way switch 0x2b660..0x2b6a3.
   Insert line changes only for record kinds that carry source locations. */
void __stdcall emit_browser_record(BrowserRecordHeader *record)
{
    BrowserRecord *entry = (BrowserRecord *)record;
    BrowserRecord line_record;
    if (!record || browser_records_suppressed) return;
    if (record->kind == 0xfe || record->kind == 0xfd || record->kind == 0xfc)
        return;
    switch (record->kind) {
    case 0: case 1: case 2: case 8: case 9: case 10:
    case 14: case 15: case 16:
        break;
    default:
        if (entry->line != browser_last_line) {
            line_record = browser_record_template;
            browser_file_cookie = 0;
            browser_last_reference = 0;
            browser_last_line = entry->line;
            line_record.line = entry->line;
            line_record.header.kind = 2;
            emit_browser_record(&line_record.header);
        }
    }
    auxiliary_ir_stream = browser_file;
    /* Original0x91878 is the existing -Zn field, not separate state. */
    serialize_browser_record(entry, (unsigned)options.z_n < 1U);
}

/* Original 0x8cff8..0x8eff7: one bit per 16-bit browse symbol index.
   The eight masks at 0x9ebe0 are the encountered index&7 vocabulary. */
unsigned char browser_symbol_references[8192];
unsigned char browser_symbol_bit_masks[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/* The original expands this FILE word-write inline, including restoring
   _cnt before the fwrite fallback. Keep it inline to preserve the graph. */
#define BROWSER_WRITE_SHORT(value_) do { \
    if ((auxiliary_ir_stream->_cnt -= 2) >= 0) { \
        *(unsigned short *)auxiliary_ir_stream->_ptr = (value_); \
        auxiliary_ir_stream->_ptr += 2; \
    } else { \
        auxiliary_ir_stream->_cnt += 2; \
        fwrite(&(value_), 2, 1, auxiliary_ir_stream); \
    } \
} while (0)
#define BROWSER_TRACK_INDEX(index_) \
    (browser_symbol_references[(index_) >> 3] |= \
        browser_symbol_bit_masks[(index_) & 7])

/* Original 0x2ad40..0x2b115 and sixteen switch slots 0x2b118..0x2b157.
   Browse records use their own fixed-width index and attribute encoding. */
void __stdcall serialize_browser_record(BrowserRecord *record, int track_symbols)
{
    unsigned index;
    unsigned short line;
    char *directory;
    putc((signed char)record->header.kind, auxiliary_ir_stream);
    switch (record->header.kind) {
    case 0:
        putc(2, auxiliary_ir_stream);
        putc(0, auxiliary_ir_stream);
        putc(7, auxiliary_ir_stream);
        putc(0, auxiliary_ir_stream);
        directory = browser_working_directory();
        write_ir_string(directory);
        break;
    case 1: case 15:
        write_ir_string(record->name);
        break;
    case 2:
        line = (unsigned short)record->line;
        BROWSER_WRITE_SHORT(line);
        break;
    case 3:
        if (track_symbols && ((record->attributes & 1) ||
            ((record->category == 1 || record->category == 4) &&
                !(record->attributes & 0x20)) ||
            record->category == 16 || record->category == 17)) {
            index = record->symbol_index;
            BROWSER_TRACK_INDEX(index);
        }
        putc((signed char)record->category, auxiliary_ir_stream);
        BROWSER_WRITE_SHORT(record->attributes);
        index = record->symbol_index;
        putc((unsigned char)index, auxiliary_ir_stream);
        putc((unsigned char)(index >> 8), auxiliary_ir_stream);
        if (record->name) {
            write_ir_string(record->name);
            if (record->header.flags & 2) {
                free(record->name);
                record->name = 0;
                record->header.flags &= ~2;
            }
        } else write_ir_string("");
        break;
    case 4: case 5: case 11: case 13:
        index = record->symbol_index;
        if (track_symbols) BROWSER_TRACK_INDEX(index);
        putc((unsigned char)index, auxiliary_ir_stream);
        putc((unsigned char)(index >> 8), auxiliary_ir_stream);
        break;
    case 12:
        index = record->symbol_index;
        if (track_symbols) BROWSER_TRACK_INDEX(index);
        putc((unsigned char)index, auxiliary_ir_stream);
        putc((unsigned char)(index >> 8), auxiliary_ir_stream);
        putc((signed char)record->category, auxiliary_ir_stream);
        break;
    }
}
#undef BROWSER_TRACK_INDEX
#undef BROWSER_WRITE_SHORT

char *read_browser_string(void);

/* Original0x94b14/18/1c: filtering owns a separate string buffer,
   output stream and symbol-index-rollover flag. */
char *browser_read_buffer;
FILE *browser_filtered_file;
int browser_symbols_overflowed;

/* Original 0x2b160..0x2b1d9, cdecl. Read a NUL-terminated name
   into the filter's shared256-byte buffer. Retain the original extra tail
   consumption when the terminator occupies exactly its final byte. */
char *read_browser_string(void)
{
    char *cursor = browser_read_buffer;
    do {
        *cursor = (char)getc(browser_file);
    } while (*cursor++);
    if (browser_read_buffer - cursor == -256)
        while (getc(browser_file)) ;
    return browser_read_buffer;
}

/* Original 0x2b1e0..0x2b561, stdcall4; sixteen text dispatch
   slots at0x2b564..0x2b5a3. Decode only each record kind's defined fields.
   The name buffer is shared; clear only its ownership bit before forwarding. */
unsigned char __stdcall deserialize_browser_record(BrowserRecord *record)
{
    record->header.kind = (unsigned char)getc(browser_file);
    if (browser_file->_flag & 0x10) return 0xff;
    switch (record->header.kind) {
    case 0:
        getc(browser_file);
        getc(browser_file);
        getc(browser_file);
        getc(browser_file);
        read_browser_string();
        break;
    case 1: case 15:
        record->name = read_browser_string();
        break;
    case 2:
        record->line = 0;
        ((unsigned char *)&record->line)[0] = (unsigned char)getc(browser_file);
        ((unsigned char *)&record->line)[1] = (unsigned char)getc(browser_file);
        break;
    case 3:
        record->category = (unsigned char)getc(browser_file);
        ((unsigned char *)&record->attributes)[0] = (unsigned char)getc(browser_file);
        ((unsigned char *)&record->attributes)[1] = (unsigned char)getc(browser_file);
        record->symbol_index = 0;
        ((unsigned char *)&record->symbol_index)[0] = (unsigned char)getc(browser_file);
        ((unsigned char *)&record->symbol_index)[1] = (unsigned char)getc(browser_file);
        record->name = read_browser_string();
        break;
    case 4: case 5: case 11: case 13:
        record->symbol_index = 0;
        ((unsigned char *)&record->symbol_index)[0] = (unsigned char)getc(browser_file);
        ((unsigned char *)&record->symbol_index)[1] = (unsigned char)getc(browser_file);
        break;
    case 12:
        record->symbol_index = 0;
        ((unsigned char *)&record->symbol_index)[0] = (unsigned char)getc(browser_file);
        ((unsigned char *)&record->symbol_index)[1] = (unsigned char)getc(browser_file);
        record->category = (unsigned char)getc(browser_file);
        break;
    }
    record->header.flags &= ~2;
    return 1;
}

/* Original 0x2bfd0..0x2bfd4. Drain pending records at an include
   boundary or before finalizing the browser file; the original tail-jumps. */
void browser_include_begin(void)
{
    flush_browser_records();
}

/* Original 0x2c050..0x2c2c4. Remove unreferenced declarations when
   indices fit the reference bitmap; otherwise retain the complete stream. */
void finish_browser_output(void)
{
    char filename[256];
    BrowserRecord record, line_record;
    ArenaPool *pool;
    unsigned line;
    if (error_count) browser_compilation_error();
    browser_include_begin();
    close_intermediate(browser_file);
    format_browser_temporary_name(filename);
    if (!browser_symbols_overflowed && !options.z_n) {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < 256) refill_arena(&compiler_arena, pool);
        browser_read_buffer = (char *)pool->next;
        pool->remaining -= 256;
        pool->next += 256;
        browser_filtered_file = fopen(options.option_Fr, "w+");
        if (!browser_filtered_file)
            report_fatal(83, message_text(340), options.option_Fr);
        setvbuf(browser_filtered_file, 0, 0, 4096);
        browser_last_line = (unsigned)-1;
        rewind(browser_file);
        auxiliary_ir_stream = browser_filtered_file;
        /* The frozen stream begins with header, line0 and builtin names;
           line0 becomes1 before the first location comparison. The original
           earlier unwritten-local load is discarded on that defined path. */
        while (deserialize_browser_record(&record) != 0xff) {
            switch (record.header.kind) {
            case 0:
                break;
            case 2:
                line = record.line;
                if (!line) line = 1;
                continue;
            case 3:
                if (!(browser_symbol_references[record.symbol_index >> 3] &
                    browser_symbol_bit_masks[record.symbol_index & 7])) continue;
                /* Retained declarations share the normal line transition. */
            default:
                if (line != browser_last_line) {
                    line_record = browser_record_template;
                    browser_file_cookie = 0;
                    browser_last_reference = 0;
                    browser_last_line = line;
                    line_record.line = line;
                    line_record.header.kind = 2;
                    serialize_browser_record(&line_record, 0);
                }
                break;
            }
            serialize_browser_record(&record, 0);
        }
        if (fclose(browser_file) || fclose(browser_filtered_file) || _unlink(filename))
            report_fatal(83, message_text(340), options.option_Fr);
    } else {
        _unlink(options.option_Fr);
        if (fclose(browser_file) || rename(filename, options.option_Fr))
            report_fatal(83, message_text(340), options.option_Fr);
    }
}

/* Original 0x69aa0..0x69af7. Reserve a complete 256-byte path in
   compiler pool 2, retaining _getcwd's return on ordinary builds. */
char *browser_working_directory(void)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    char *directory;
    if (pool->remaining < 256)
        refill_arena(&compiler_arena, pool);
    directory = (char *)pool->next;
    pool->remaining -= 256;
    pool->next += 256;
    if (!options.configuration.option_Brepro)
        return _getcwd(directory, 256);
    directory[0] = '.';
    directory[1] = 0;
    return directory;
}

/* Original 0x2c3e0..0x2c3e9. Announce the active source filename. */
void browser_open_file(void)
{
    queue_browser_record(1, 0);
}

/* Original 0x2b7a0..0x2b7c3. File records own a copy of the active
   filename, rather than keeping an interner-name identity. */
void fill_browser_filename(BrowserRecord *record)
{
    record->header.flags |= 2;
    record->name = duplicate_browser_text(source_filename);
    record->name_identity = 0;
}

/* Original 0x2b750..0x2b793. The two original inline string scans
   precede allocation and copying respectively; retain the terminating NUL. */
char *duplicate_browser_text(char *text)
{
    char *end, *copy;
    unsigned bytes, index;
    for (end = text; *end; ++end)
        ;
    copy = malloc(end - text + 1);
    for (end = text; *end; ++end)
        ;
    bytes = end - text + 1;
    for (index = 0; index < bytes; ++index)
        copy[index] = text[index];
    return copy;
}

/* Original 0x2c430..0x2c43e. Queue a resolved declaration symbol. */
void __stdcall browser_declaration(CppSymbol *symbol)
{
    queue_browser_record(3, symbol);
}

/* Original 0x9ebe8..0x9ebf3: encountered ordinary-symbol prefix only.
   The following twelve bytes are not owned here. This does not establish
   the historical array's full extent; see browser-declaration evidence. */
unsigned char browser_declaration_categories[12] = {
    0, 0, 17, 17, 0, 10, 1, 1, 2, 4, 3, 0
};
/* Original 0x9ec00..0x9ec0f, independently indexed by flags & 3. */
unsigned browser_access_attributes[4] = { 0, 0x400, 0x800, 0x40 };

/* Original 0x2b800..0x2ba84. Resolve the browse name, storage/access
   attributes and category while retaining the real declaration identity. */
void __stdcall fill_browser_declaration(BrowserRecord *record, CppSymbol *symbol)
{
    CppSymbol *alias;
    char *decorated;
    unsigned kind = symbol->kind & 31;
    unsigned storage;
    unsigned short flags;
    int member;
    if (kind == 11 || (symbol->flags & 0x80)) {
        record->header.kind = 0xfc;
        return;
    }
    if (symbol->declaration_flags & 0x3000) record->header.flags |= 1;
    else record->header.flags &= ~1;
    record->header.flags &= ~2;
    fill_browser_symbol(record, symbol);
    kind = symbol->kind & 31;
    if ((kind == 6 || kind == 7) && !(symbol->declaration_flags & 0x200) &&
        symbol->name != name_main && symbol->name != name_winmain &&
        symbol->name != name_dllmain) {
        decorated = write_decorated_name(0, symbol, 0, 0);
        if (error_count) {
            suppress_browser_records();
            record->header.kind = 0xfc;
            return;
        }
        record->header.flags |= 2;
        record->name = duplicate_browser_text(decorated);
        record->name_identity = 0;
    }
    kind = symbol->kind & 31;
    if (kind >= 1 && kind <= 13 && kind != 5 && (symbol->attributes & 0x40)) {
        alias = kind == 3 ? static_member_alias(symbol) : symbol_alias(symbol);
        record->name = alias->name;
        record->name_identity = 0;
    } else if (*symbol->name == '?') {
        record->name = symbol->name;
        record->name_identity = 0;
        symbol = static_member_from_alias(symbol);
    }
    storage = symbol->storage[0] & 7;
    if (storage == 7) {
        record->attributes = 0;
        record->category = 7;
        return;
    }
    kind = symbol->kind & 31;
    member = kind == 2 || kind == 3 ||
        (kind == 7 && (symbol->declaration_flags & 0x400));
    if (storage == 1 || storage == 2 || storage == 3) {
        if (storage == 3) record->attributes = 2;
        else if (!member) record->attributes = 0x40;
        if (kind == 7 ? !(symbol->flags & 4) :
            storage == 1 && !(symbol->attributes & 4)) record->attributes |= 0x20;
    } else if (declaration_mode >= 2) record->attributes |= 1;
    if (member) {
        record->attributes |= browser_access_attributes[symbol->flags & 3];
        if (symbol->storage[1] & 2) record->attributes |= 0x200;
        if (!symbol->index) {
            symbol->index = browser_symbol_limit--;
            record->symbol_index = symbol->index;
        }
        set_browser_access(&record->attributes, symbol->flags & 3);
    }
    kind = symbol->kind & 31;
    if (kind == 4) {
        flags = symbol->type->flags;
        if ((flags & 0x408) == 0x408) record->category = 15;
        else if ((flags & 0x402) == 0x402) record->category = 8;
        else if ((flags & 0x404) == 0x404) record->category = 11;
        else if (flags & 0x200) record->category = 9;
    } else if (kind == 7 && (symbol->declaration_flags & 0x400))
        record->category = 16;
    else {
#ifdef RECON_NATIVE_TEST
        /* Only the encountered ordinary-symbol prefix is reconstructed.
           Other rows are unknown data, not a proved bug in the original. */
        if (kind >= sizeof(browser_declaration_categories)) { __asm { int 3 } }
#endif
        record->category = browser_declaration_categories[kind];
    }
}

/* Original 0x2b7d0..0x2b7f5. Existing record names retain their ownership
   flag and identity; an absent name takes the symbol's interned spelling. */
void __stdcall fill_browser_symbol(BrowserRecord *record, CppSymbol *symbol)
{
    record->symbol_index = symbol->index;
    if (!record->name) {
        record->header.flags &= ~2;
        record->name = symbol->name;
        record->name_identity = symbol->name;
    }
}

/* Original 0x2c440..0x2c451, stdcall4. */
void __stdcall reference_browser_macro(MacroDefinition *definition)
{
    queue_browser_record(0xfe, definition);
}

/* Original 0x2ba90..0x2baa3. Kind FE carries a MacroDefinition,
   not a scope symbol: its browser identity is the DWORD at offset 16. */
void __stdcall fill_browser_macro_reference(BrowserRecord *record, MacroDefinition *definition)
{
    record->header.kind = 4;
    record->symbol_index = definition->browser_index;
}

/* Original 0x2bed0..0x2bf04. Only a pending declaration with the same
   interned name can have its reference attribute changed. */
int __stdcall browser_name_reference(int reference, char *name)
{
    BrowserRecord *record = find_browser_declaration(name);
    if (!record) return 0;
    if (reference) record->attributes |= 0x20;
    else record->attributes &= ~0x20;
    return 1;
}

/* Original 0x2be90..0x2bec4. Stop at the newest declaration, even if
   its interned name differs; older declarations are not candidates. */
BrowserRecord *__stdcall find_browser_declaration(char *name)
{
    int index = 0;
    BrowserRecord *record;
    record = (BrowserRecord *)browser_record_from_end(index);
    while (record) {
        if (record->header.kind == 3) {
            if (record->name_identity == name) return record;
            return 0;
        }
        record = (BrowserRecord *)browser_record_from_end(++index);
    }
    return 0;
}

int browser_tag_depth; /* original 0x9ec14 */

/* Original 0x2c360..0x2c36f. */
void browser_begin_tag(void)
{
    ++browser_tag_depth;
    queue_browser_record(8, 0);
}

/* Original 0x2c490..0x2c49e, stdcall4. */
void __stdcall browser_enter_tag(CppSymbol *tag)
{
    queue_browser_record(11, tag);
}

/* Original 0x2b6f0..0x2b740. Replace the mutually exclusive browse
   access bits, retaining the original WORD stores. */
void __stdcall set_browser_access(unsigned short *attributes, unsigned char access)
{
    if (access == 1) {
        *attributes |= 0x400;
        *attributes &= 0xf7bf;
    } else if (access == 2) {
        *attributes |= 0x800;
        *attributes &= 0xfbbf;
    } else {
        *attributes |= 0x40;
        *attributes &= 0xf3ff;
    }
}

/* Original 0x2c330..0x2c353. Unmatched ends clamp the nesting depth
   without emitting a closing record. */
void browser_end_tag(void)
{
    if (browser_tag_depth <= 0) {
        browser_tag_depth = 0;
        return;
    }
    --browser_tag_depth;
    queue_browser_record(9, 0);
}

/* Original 0x2c3d0..0x2c3dd. Drain the queue after publishing the
   include-return record, before the caller restores its source context. */
void browser_include_end(void)
{
    queue_browser_record(10, 0);
    flush_browser_records();
}

/* Original 0x2bfb0..0x2bfc1. Remove each record before emitting it. */
void flush_browser_records(void)
{
    BrowserRecordHeader *record;
    while ((record = pop_first_browser_record()) != 0)
        emit_browser_record(record);
}

/* Original 0x2c420..0x2c42e, stdcall4. */
void __stdcall browser_type_reference(CppSymbol *symbol)
{
    queue_browser_record(4, symbol);
}

/* Original 0x2bab0..0x2bb0e. Repeated, hidden and implicit-this names
   produce no record. Qualified names use the resolution's selected target. */
void __stdcall fill_browser_reference(BrowserRecord *record, CppSymbol *symbol)
{
    unsigned kind;
    record->header.kind = 0xfc;
    if (symbol == browser_last_reference || (symbol->flags & 0x80) ||
        symbol->name == name_this) return;
    if ((symbol->kind & 31) == 18) {
        symbol = (CppSymbol *)((QualifiedDeclarator *)symbol)->resolution->binding->target;
        kind = symbol->kind & 31;
        if (kind == 16 || kind == 17) return;
    }
    fill_browser_symbol(record, symbol);
    record->header.kind = 4;
}

/* Original 0x2be20..0x2be4f. The payload remains valid after removal. */
BrowserRecordHeader *pop_last_browser_record(void)
{
    int index = browser_record_head + browser_record_count - 1;
    BrowserRecordHeader *record;
    if (!browser_record_count) return 0;
    if (index >= 32) index -= 32;
    record = &browser_records[index].header;
    --browser_record_count;
    return record;
}

/* Original 0x2c400..0x2c40e, stdcall4. */
void __stdcall browser_base_class(BaseClassLink *base)
{
    queue_browser_record(12, base);
}

/* Original 0x9ec10..0x9ec13, indexed only by the two access bits. */
static unsigned char browser_base_categories[4] = { 0, 2, 8, 4 };

/* Original 0x2bb10..0x2bb4d. The base link names its class through
   the existing context index; virtual-base state adds the low category bit. */
void __stdcall fill_browser_base(BrowserRecord *record, BaseClassLink *base)
{
    fill_browser_symbol(record, cpp_context_stack->items[base->context_index]);
    record->category = browser_base_categories[base->flags & 3];
    if (base->state & 1) record->category |= 1;
}

/* Original0x2c410..0x2c41f, stdcall4. Friend declarations retain their
   distinct browser event even when naming an existing function. */
void __stdcall browser_friend_reference(CppSymbol *symbol)
{
    queue_browser_record(13,symbol);
}

/* Original0x2c4f0..0x2c529. Close the browser prefix before snapshotting;
   the PCH marker requires retaining the complete browser stream at finish. */
void begin_browser_pch_record(void)
{
    BrowserRecord record = browser_record_template;
    flush_browser_records();
    record.header.kind = 16;
    emit_browser_record(&record.header);
    browser_symbols_overflowed = 1;
}

/* Original0x2c4a0..0x2c4e8, stdcall4. Reopen a browser PCH source with
   an owned filename, retaining the template's remaining record fields. */
void __stdcall restore_pch_browser_name(char *filename)
{
    BrowserRecord record = browser_record_template;
    flush_browser_records();
    record.header.kind = 15;
    record.header.flags |= 2;
    record.name = duplicate_browser_text(filename);
    emit_browser_record(&record.header);
}

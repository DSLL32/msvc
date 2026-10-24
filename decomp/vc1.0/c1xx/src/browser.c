/* Browse-information stream setup and record transport. */
#include "browser.h"
#include "initialization.h"
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
    case 0xfe: fill_browser_scope_end(record, (CppSymbol *)subject); break;
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
unsigned browser_output_pass; /* original 0x91878: nonzero skips index tracking */
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
    serialize_browser_record(entry, browser_output_pass < 1U);
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

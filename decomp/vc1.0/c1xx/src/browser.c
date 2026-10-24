/* Browse-information stream setup and record transport. */
#include "browser.h"
#include "initialization.h"
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
    case 0xfd: fill_browser_macro(record, subject); break;
    case 0xfe: fill_browser_scope_end(record, (CppSymbol *)subject); break;
    case 0xff: fill_browser_generated_symbol(record); break;
    }
}

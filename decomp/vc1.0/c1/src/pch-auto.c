/* Automatic precompiled-header candidate validation and event replay. */
#include <stdlib.h>
#include <sys/stat.h>
#include "pch-relocate.h"
#include "source-input.h"
#include "browser.h"
#include "pragma.h"
#include "literal-names.h"
#include "macros.h"
#include "macro-expand.h"
#include <recon.h>

WorkingStack *pch_dependency_stack; /* original0x6de18 */
PchRecord *pch_event_buffer; /* original0x5b56c */
char *pch_rebuild_reason; /* original0x6de2c */

/* Original0x45880..0x45b33. A candidate must match the header and complete
   command-settings record before recorded macro/include dependencies replay.
   Otherwise retain the reason and start collecting a replacement. */
void begin_automatic_pch(void)
{
    PchHeader header;
    PchSettingsImage current;
    unsigned i, length;
    short index;
    int compatible;
    Vector *vector;
    char description[256];
    initialize_work_stack(&pch_dependency_stack);
    pch_mode = 2;
    prepare_pch_output("msvc.pch");
    pch_file = fopen(options.option_Fp,"r");
    if (pch_file) {
        for (i = 0; i < 8; ++i) header.signature[i] = "DTJPCH0"[i];
        header.format = 0x80117112;
        header.symbol_bytes = pch_symbol_bytes;
        header.global_bytes = pch_global_bytes;
        header.expression_bytes = pch_expression_bytes;
        header.line_bytes = pch_line_bytes;
        header.debug_bytes = pch_debug_bytes;
        header.browser_bytes = pch_browser_bytes;
        header.event_bytes = pch_event_bytes;
        header.event_offset = pch_event_offset;
        header.browser_offset = browser_saved_position;
        header.page_count = permanent_page_count;
        header.reserved = pch_header_reserved;
        fread(&header,1,sizeof(header),pch_file);
        pch_symbol_bytes = header.symbol_bytes;
        pch_global_bytes = header.global_bytes;
        pch_expression_bytes = header.expression_bytes;
        pch_line_bytes = header.line_bytes;
        pch_debug_bytes = header.debug_bytes;
        pch_browser_bytes = header.browser_bytes;
        pch_event_bytes = header.event_bytes;
        pch_event_offset = header.event_offset;
        browser_saved_position = header.browser_offset;
        permanent_page_count = header.page_count;
        pch_header_reserved = header.reserved;
        if (pch_file->_flag & 0x20)
            report_fatal(84,message_text(344),options.option_Fp);
        compatible = header.format == 0x80117112;
        for (i = 0; i < 8; ++i)
            if (header.signature[i] != "DTJPCH0"[i]) compatible = 0;
        if (compatible) {
            fread(&saved_pch_settings,1,sizeof(saved_pch_settings),pch_file);
            CAPTURE_PCH_SETTINGS(current,options,database_path,pch_object_name,i);
            for (i = 0; i < sizeof(current); ++i)
                if (((unsigned char *)&current)[i] != ((unsigned char *)&saved_pch_settings)[i]) break;
            if (i == sizeof(current)) {
                pch_event_buffer = malloc(pch_event_bytes);
                intermediate_status = fseek(pch_file,pch_event_offset,0);
                if (intermediate_status)
                    report_error(86,message_text(344),options.option_Fp,pch_diagnostic_name,679);
                fread(pch_event_buffer,1,pch_event_bytes,pch_file);
                pch_record_cursor = pch_event_buffer;
                vector = options.defines;
                for (index = 0; index < vector->count; ) {
                    pch_record_macro("",vector->items[index++],0);
                    if (pch_mode == 1) break;
                }
                if (pch_mode != 1) {
                    process_pch_string_event(4,0,browser_current_directory());
                    if (pch_mode != 1) {
                        vector = options.include_directories;
                        for (index = 0; index < vector->count; ) {
                            process_pch_string_event(4,0,vector->items[index++]);
                            if (pch_mode == 1) break;
                        }
                        if (pch_mode != 1) { pch_include_count = 0; return; }
                    }
                }
            }
        }
    }
    if (pch_file)
        format_text(description,"Incompatible options; %s %s: ",message_text(352),options.option_Fp);
    else format_text(description,"%s %s",message_text(351),options.option_Fp);
    for (length = 0; description[length]; ++length) ;
    pch_rebuild_reason = checked_malloc(length + 1);
    for (i = 0; i <= length; ++i) pch_rebuild_reason[i] = description[i];
    begin_pch_rebuild();
}

/* Original0x454c0. */
void begin_pch_rebuild(void)
{
    Vector *vector;
    short index;
    if (pch_write_enabled) return;
    pch_mode = 1;
    if (pch_file) {
        intermediate_status = fclose(pch_file);
        if (intermediate_status == -1)
            report_error(82,message_text(344),options.option_Fp,pch_diagnostic_name,531);
    }
    pch_write_enabled = 1;
    initialize_work_stack(&pch_event_stack);
    if (!options.option_E && !options.prototypes_only && options.debug_info)
        begin_debug_info();
    vector = options.defines;
    for (index = 0; index < vector->count; )
        pch_record_macro("",vector->items[index++],0);
    process_pch_string_event(4,0,browser_current_directory());
    vector = options.include_directories;
    for (index = 0; index < vector->count; )
        process_pch_string_event(4,0,vector->items[index++]);
}

/* Original0x44e40..0x44e72. The event header separates its payload
   length from the event-specific value before appending to the stack. */
void write_pch_record_header(unsigned kind, unsigned value, unsigned bytes)
{
    PchRecord record;
    record.kind = kind;
    record.payload_bytes = bytes;
    record.value = value;
    write_initializer_bytes(&record,sizeof(record),1,pch_event_stack);
}

/* Original0x44ec0..0x44f82. Replay compares the recorded signed16-bit
   byte count, first in DWORDs then trailing bytes, including the terminator. */
int __stdcall process_pch_string_event(unsigned kind, unsigned value, char *text)
{
    unsigned bytes, count;
    unsigned *left, *right;
    unsigned char *tail_left, *tail_right;
    char *saved_name;
    if (!pch_mode) return 0;
    if (pch_mode == 1) { pch_record_string(kind,value,text); return 0; }
    if (pch_mode != 2) return 0;
    if (pch_record_cursor->kind == kind && pch_record_cursor->value == value) {
        bytes = (unsigned)(int)(short)pch_record_cursor->payload_bytes;
        left = (unsigned *)(pch_record_cursor + 1);
        right = (unsigned *)text;
        for (count = bytes >> 2; count; --count)
            if (*left++ != *right++) goto mismatch;
        tail_left = (unsigned char *)left;
        tail_right = (unsigned char *)right;
        for (count = bytes & 3; count; --count)
            if (*tail_left++ != *tail_right++) goto mismatch;
        pch_record_cursor = (PchRecord *)((char *)(pch_record_cursor + 1) +
            pch_record_cursor->payload_bytes);
        return 1;
    }
mismatch:
    saved_name = copy_string(interned_name,2);
    if (!pch_replay_mismatch()) { intern_identifier(saved_name); return 0; }
    return 1;
}

/* Original0x44e80..0x44ebf. PCH strings retain a16-bit payload length
   including NUL; the writer uses that same length for the following bytes. */
void pch_record_string(unsigned kind, unsigned value, char *text)
{
    unsigned length = 1;
    char *cursor = text;
    while (*cursor++) ++length;
    length &= 0xffff;
    write_pch_record_header(kind,value,length);
    write_initializer_bytes(text,length,1,pch_event_stack);
}

/* Original0x45200..0x45349. Compare conditional spelling on replay;
   capture records the same bytes and recognizes a standalone hdrstop. */
int pch_find_conditional(void)
{
    char prefix[8];
    unsigned i, bytes, saved_scanner;
    finish_initializer_buffer(pch_dependency_stack);
    if (pch_mode == 1) {
        prefix[7] = 0;
        reset_work_stack(pch_dependency_stack);
        read_work_stack(pch_dependency_stack,prefix,8);
        if (prefix[7] == ' ' || prefix[7] == '\n') {
            prefix[7] = 0;
            for (i = 0; i < 8 && prefix[i] == "hdrstop"[i]; ++i) ;
            if (i == 8) {
                saved_scanner = scanner_flags & 0x10000;
                scanner_flags &= ~0x10000UL;
                finish_pch_capture();
                replay_pch_directive();
                if (saved_scanner) scanner_flags |= 0x10000;
                return 0;
            }
        }
        bytes = write_pch_event_stack(pch_dependency_stack,0,0);
        write_pch_record_header(3,0,bytes & 0xffff);
        write_pch_event_stack(pch_dependency_stack,0,pch_event_stack);
        replay_pch_directive();
        return 0;
    }
    reset_work_stack(pch_dependency_stack);
    if (pch_record_cursor->kind == 3 && compare_work_stack(pch_dependency_stack,
            pch_record_cursor + 1,pch_record_cursor->payload_bytes)) {
        pch_record_cursor = (PchRecord *)((char *)(pch_record_cursor + 1) +
            pch_record_cursor->payload_bytes);
        return 1;
    }
    if (pch_replay_mismatch()) return 1;
    replay_pch_directive();
    return 0;
}

/* Original0x451b0. */
void replay_pch_directive(void)
{
    char *filename = source_buffer_cursor;
    char *source = source_filename;
    reset_work_stack(pch_dependency_stack);
    source_replay_stack = pch_dependency_stack;
    do { *source_buffer_cursor++ = *source++; } while (source[-1]);
    open_source_file(filename,2);
    ++source_text_cursor;
    scanner_flags |= 0x10000;
}
/* Original0x2a130. */
int __stdcall compare_work_stack(WorkingStack *stack, void *bytes, unsigned count)
{ __asm { int 3 } return 0; }

/* Original0x2a0a0..0x2a122. Flatten segmented bytes to a FILE or
   another work stack; return the accumulated sixteen-bit byte count. */
unsigned short __stdcall write_pch_event_stack(WorkingStack *stack, FILE *file,
    WorkingStack *destination)
{
    WorkingBlock *block;
    unsigned bytes;
    unsigned short count = 0;
    reset_work_stack(stack);
    for (block = (WorkingBlock *)stack->read; block; block = block->next) {
        bytes = block->used_bytes;
        if (!bytes) bytes = 2040;
        if (file) fwrite(block,1,(unsigned short)bytes,file);
        else if (destination) write_initializer_bytes(block,1,(unsigned short)bytes,destination);
        count += (unsigned short)bytes;
    }
    return count;
}

/* Original0x45150..0x451ac. After evaluating replayed directive text,
   pop the temporary source and record or recover its conditional value. */
int __stdcall pch_resolve_condition(int condition)
{
    unsigned *saved;
    if (include_depth) close_source_file();
    if (pch_mode == 1) {
        write_pch_record_header(6,condition,0);
        return condition;
    }
    if (pch_mode == 2) {
        saved = &pch_record_cursor->value;
        pch_record_cursor = (PchRecord *)((char *)(pch_record_cursor + 1) +
            pch_record_cursor->payload_bytes);
        return *saved;
    }
    return condition;
}

/* Original0x454a0..0x454b8. File dependency events retain the included
   pathname and the status helper's modification-time value. */
void __stdcall pch_record_include(char *name)
{
    pch_record_string(5,pch_file_mtime(name),name);
}

/* Original0x45350..0x4537a. File dependencies store the legacy CRT's
   modification time, or zero when the pathname has no status record. */
int __stdcall pch_file_mtime(char *name)
{
    struct _stat status;
    if (_stat(name,&status)) return 0;
    return status.st_mtime;
}

/* Original per-module dispatcher [00404c10,00404eae), with its compact
 * subsection switch data [00404eb0,00404ed7). */
#include "pack.h"
#include "widen.h"
#include <recon.h>

ModuleInfo *current_module_info;
unsigned module_type_low, module_symbol_flags, current_module_flags;
unsigned line_entry_index, symbol_entry_index, type_entry_index;
unsigned current_type_count, has_precompiled, pending_module_bytes;
unsigned total_symbol_bytes, total_line_bytes, strip_module_lines;
unsigned short module_symbol_count, module_reference_count, module_procedure_count;
unsigned short current_type_index, current_module_has_types;
unsigned char *current_module_buffer;

int pack_module(unsigned short module)
{
    unsigned index;
    unsigned short identifier;
    CodeViewDirectoryEntry *entry;
    ModuleIndex *selected = &module_index[module];
    module_type_low = 0x0fffffff;
    module_symbol_count = module_reference_count = module_procedure_count = 0;
    module_symbol_flags = 0;
    current_type_index = 0;
    current_module_flags = line_entry_index = symbol_entry_index = 0;
    type_entry_index = current_type_count = has_precompiled = 0;
    current_module_info = selected->info;
    pending_module_bytes = 0;
    index = selected->directory_index;
    identifier = selected->module;
    initialize_single_module(&directory_entries[index], current_module_info);
    for (++index; index < directory_count; ++index) {
        entry = &directory_entries[index];
        if (entry->module != identifier) break;
        if (!entry->size) continue;
        switch (entry->subsection) {
        case 0x121: read_module_types(entry, 0); break;
        case 0x12f:
            has_precompiled = 1;
            read_module_types(entry, 1);
            prepare_precompiled_types(current_module_info);
            break;
        case 0x123: read_module_symbols(index, entry); break;
        case 0x124: read_module_local_symbols(index, entry); break;
        case 0x127: read_module_source_lines(index, entry); break;
        }
    }
    if (symbol_entry_index && (*(unsigned *)symbol_data == 1 || *(unsigned *)symbol_data == 2)) {
        entry = &directory_entries[symbol_entry_index];
        pack_symbols(symbol_data, entry);
        total_symbol_bytes += entry->size;
    }
    if (line_entry_index) {
        entry = &directory_entries[line_entry_index];
        if (strip_module_lines) entry->size = 0;
        else if (*(unsigned *)type_data != 1 && *(unsigned *)type_data != 2) {
            decode_legacy_lines(type_data, entry->size);
            rewrite_legacy_lines(type_data, entry, current_module_buffer, current_module_info);
        } else {
            total_line_bytes += entry->size;
            scan_module_symbols(type_data, entry->size, &module_symbol_count, &module_reference_count);
            rewrite_module_symbols(type_data, entry, current_module_info, &module_symbol_count, &module_reference_count);
        }
    }
    if (type_entry_index) {
        current_module_has_types = 1;
        rewrite_module_global_types(module_data, &directory_entries[type_entry_index], current_module_info);
    }
    return 1;
}

/* Original module payload ownership setup [00404b50,00404c07). */
void initialize_single_module(CodeViewDirectoryEntry *entry, ModuleInfo *info)
{
    current_module_buffer = malloc(entry->size);
    if (!current_module_buffer) fatal_error(1001, NULL, NULL);
    if (entry->offset != image_read_mode) {
        image_read_mode = entry->offset;
        seek_cached_file(input_file, codeview_file_offset + entry->offset, 0);
    }
    if (read_cached_file(input_file, current_module_buffer, entry->size) != entry->size)
        fatal_error(1005, NULL, NULL);
    image_read_mode += entry->size;
    entry->data = current_module_buffer;
    info->size = entry->size;
    info->data = current_module_buffer;
}

/* Read a module's public symbols and widen its type indices [00405130,004051d8). */
void read_module_symbols(unsigned index, CodeViewDirectoryEntry *entry)
{
    if (entry->offset != image_read_mode) {
        image_read_mode = entry->offset;
        seek_cached_file(input_file, codeview_file_offset + entry->offset, 0);
    }
    if (read_cached_file(input_file, symbol_data, entry->size) != entry->size)
        fatal_error(1007, "Publics", module_name(current_module_info));
    image_read_mode += entry->size;
    entry->size = widen_subsection((unsigned char **)&symbol_data, entry->size, &source_symbol_bytes);
    symbol_entry_index = index;
}

/* Convert an older symbol subsection through the opaque WidenTi interface
 * [00405250,004053ba). The four-byte signature remains in the target buffer. */
unsigned widen_subsection(unsigned char **buffer, unsigned size, unsigned *capacity)
{
    WidenOutput output = { 0, 0, NULL, NULL };
    WidenTi *object;
    unsigned input_size, result = size, index;
    if (*(unsigned *)*buffer == 1) {
        *(unsigned *)*buffer = 2;
        input_size = size - 4;
        if (input_size) {
            object = (WidenTi *)size;
            if (!create_widen(&object) || !query_widen(object, &output, *buffer + 4, input_size))
                fatal_error(1001, NULL, NULL);
            if (output.bytes > pack_buffer_size) {
                pack_buffer = resize_bytes(pack_buffer, output.bytes);
                pack_buffer_size = output.bytes;
            }
            if (output.fixups > pack_fixup_count) {
                pack_fixups = resize_bytes(pack_fixups, output.fixups * 8);
                pack_fixup_count = output.fixups;
            }
            output.output = pack_buffer;
            output.records = pack_fixups;
            if (!emit_widen(object, &output, *buffer + 4, input_size, 0))
                fatal_error(1001, NULL, NULL);
            result = output.bytes + 4;
            if (result > *capacity) {
                *capacity = result;
                *buffer = resize_bytes(*buffer, result);
            }
            for (index = 0; index < output.bytes; ++index)
                (*buffer)[index + 4] = output.output[index];
            release_widen(object);
        }
    }
    return result;
}

/* Process aligned or unaligned public-symbol records [0040aa00,0040ab87). */
unsigned public_scope_depth;
void pack_symbols(void *symbols, CodeViewDirectoryEntry *entry)
{
    union {
        unsigned char bytes[512];
        unsigned short words[256];
        unsigned dwords[128];
    } record;
    unsigned char *cursor, *end;
    unsigned short length, kind;
    unsigned index;
    public_scope_depth = 0;
    if (!entry->size) return;
    cursor = (unsigned char *)symbols + 4;
    end = (unsigned char *)symbols + entry->size;
    while (cursor < end) {
        if ((unsigned)cursor & 1) {
            length = (unsigned short)((cursor[1] << 8) + cursor[0] + 2);
            kind = (unsigned short)((cursor[3] << 8) + cursor[2]);
        } else {
            length = (unsigned short)(*(unsigned short *)cursor + 2);
            kind = *(unsigned short *)(cursor + 2);
        }
        for (index = 0; index < length; ++index) record.bytes[index] = cursor[index];
        if (kind == 0x1009) {
            if (module_type_low != 1 && module_type_low != 2) {
                if (minimum_pack) record.words[4] = 0;
                else record.dwords[1] = map_legacy_index(record.dwords[1]);
            } else if (minimum_pack) {
                if (record.dwords[1] >= 0x1000) record.dwords[1] = 0;
            } else record.dwords[1] = map_type_index(record.dwords[1]);
        } else if (kind == 0x103) {
            if (module_type_low != 1 && module_type_low != 2) {
                if (minimum_pack) record.words[4] = 0;
                else record.words[4] = (unsigned short)map_legacy_index(record.words[4]);
            } else if (minimum_pack) {
                if (record.words[4] >= 0x1000) record.words[4] = 0;
            } else record.words[4] = (unsigned short)map_type_index(record.words[4]);
        }
        insert_public_symbol(record.bytes, symbol_record_key);
        cursor += length;
    }
}

/* Read the signature and select the module type-record format [00404ee0,0040506d). */
unsigned module_type_entries, module_type_list_used, module_precompiled_limit;
void read_module_types(CodeViewDirectoryEntry *entry, int precompiled)
{
    unsigned size;
    unsigned char *buffer;
    line_data = type_segments[0];
    if (entry->offset != image_read_mode) {
        image_read_mode = entry->offset;
        seek_cached_file(input_file, codeview_file_offset + entry->offset, 0);
    }
    if (read_cached_file(input_file, line_data, 4) != 4) fatal_error(1005, NULL, NULL);
    module_type_entries = module_type_list_used = 0;
    module_type_low = *(unsigned *)line_data;
    if (module_type_low && module_type_low <= 2) {
        module_type_base = 0x1000;
        read_new_module_types(entry->size - 4, precompiled);
        image_read_mode += entry->size;
    } else {
        size = entry->size - 4;
        buffer = line_data;
        if (buffer[0] != 1 && !(buffer[0] == 0 && (buffer[3] == 0xa6 || buffer[3] == 0xa7)))
            fatal_error(1007, "Types", module_name(current_module_info));
        module_type_low = 0;
        module_type_base = 0x200;
        if (read_cached_file(input_file, buffer + 4, size) != size) fatal_error(1005, NULL, NULL);
        image_read_mode += entry->size;
        read_legacy_module_types(line_data, entry->size);
    }
    if (!module_precompiled_limit) module_precompiled_limit = type_record_count;
    if (!module_type_entries) reset_type_name_cache();
}

/* Read the local-symbol subsection and retain its start [00405070,00405123). */
unsigned char *module_symbol_records;
void read_module_local_symbols(unsigned index, CodeViewDirectoryEntry *entry)
{
    if (entry->offset != image_read_mode) {
        image_read_mode = entry->offset;
        seek_cached_file(input_file, codeview_file_offset + entry->offset, 0);
    }
    if (read_cached_file(input_file, type_data, entry->size) != entry->size)
        fatal_error(1007, "Symbols", module_name(current_module_info));
    image_read_mode += entry->size;
    entry->size = widen_subsection((unsigned char **)&type_data, entry->size, &source_line_bytes);
    module_symbol_records = type_data;
    line_entry_index = index;
}

/* Read source-module line information [004053c0,0040544e). */
void read_module_source_lines(unsigned index, CodeViewDirectoryEntry *entry)
{
    if (entry->offset != image_read_mode) {
        image_read_mode = entry->offset;
        seek_cached_file(input_file, codeview_file_offset + entry->offset, 0);
    }
    if (read_cached_file(input_file, module_data, entry->size) != entry->size)
        fatal_error(1005, "Source Module", module_name(current_module_info));
    image_read_mode += entry->size;
    type_entry_index = index;
}

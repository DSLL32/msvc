/* Original per-module dispatcher [00404c10,00404eae), with its compact
 * subsection switch data [00404eb0,00404ed7). */
#include "pack.h"
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
        case 0x124: read_module_lines(index, entry); break;
        case 0x127: read_module_global_types(index, entry); break;
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
            decode_new_lines(type_data, entry->size, &module_symbol_count, &module_reference_count);
            rewrite_new_lines(type_data, entry, current_module_info, &module_symbol_count, &module_reference_count);
        }
    }
    if (type_entry_index) {
        current_module_has_types = 1;
        rewrite_module_global_types(module_data, &directory_entries[type_entry_index], current_module_info);
    }
    return 1;
}

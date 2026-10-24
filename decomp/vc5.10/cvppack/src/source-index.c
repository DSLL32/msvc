/* Combine per-module source-file offsets and counted filenames
 * [0040f100,0040f3fa). Historical logical label: print_statistics. */
#include "source-index.h"
#include <recon.h>

#pragma intrinsic(memcpy, memset)

unsigned source_name_hash[50], source_file_index_size;
unsigned char *source_file_index;

void build_source_file_index(void)
{
    SourceModuleIndex modules, *module_tail = &modules, *block, *next_block;
    SourceNameIndex names = { NULL, 0, NULL, 0 }, *name_tail = &names, *name, *next_name;
    ModuleInfo *info;
    unsigned char *source, *file;
    const unsigned char *file_name;
    unsigned *offsets, *output_offsets;
    unsigned name_bytes = 0, total_files = 0, written_files = 0;
    unsigned short next_module = 1;
    unsigned short *starts, *counts;
    int file_index;
    /* Only the sentinel count/link are initialized in the original. Its
     * unused moduleWORD retains this frame's own unwritten value. */
    modules.files = 0;
    modules.next = NULL;
    memset(source_name_hash, 0, sizeof(source_name_hash));
    for (info = module_list; info; info = info->next) {
        source = info->source_lines;
        if (!source) continue;
        block = allocate_bytes(sizeof(SourceModuleIndex) + *(unsigned short *)source * 4);
        block->next = NULL;
        block->module = info->module;
        block->files = *(unsigned short *)source;
        module_tail->next = block;
        module_tail = block;
        offsets = (unsigned *)(block + 1);
        for (file_index = 0; file_index < *(unsigned short *)source; ++file_index) {
            file = source + ((unsigned *)(source + 4))[file_index];
            file_name = file + 4 + *(unsigned short *)file * 12;
            name_tail = append_source_name(file_name, name_tail, name_bytes);
            *offsets++ = name_bytes;
            name_bytes += *file_name + 1;
            ++total_files;
        }
    }
    /* The original takes its size from the last name, including its unchecked
     * name pointer if a source-module table contains no file entries. */
    source_file_index_size = (name_tail->offset + *name_tail->name +
        (module_count + (unsigned short)total_files) * 4 + 8) & ~3U;
    source_file_index = allocate_bytes(source_file_index_size);
    *(unsigned short *)source_file_index = module_count;
    *(unsigned short *)(source_file_index + 2) = (unsigned short)total_files;
    starts = (unsigned short *)(source_file_index + 4);
    counts = starts + module_count;
    output_offsets = (unsigned *)(source_file_index + 4 + module_count * 4);
    for (block = modules.next; block; block = block->next) {
        while (next_module < block->module) {
            starts[next_module - 1] = counts[next_module - 1] = 0;
            ++next_module;
        }
        starts[next_module - 1] = (unsigned short)written_files;
        counts[next_module - 1] = block->files;
        written_files += block->files;
        memcpy(output_offsets, block + 1, block->files * 4);
        output_offsets += block->files;
        ++next_module;
    }
    while (next_module <= module_count) {
        starts[next_module - 1] = counts[next_module - 1] = 0;
        ++next_module;
    }
    source = source_file_index + 4 + (module_count + (unsigned short)total_files) * 4;
    for (name = names.next; name; name = name->next) {
        memcpy(source, name->name, *name->name + 1);
        source += *source + 1;
    }
    for (block = modules.next; block; block = next_block) {
        next_block = block->next;
        free(block);
    }
    for (name = names.next; name; name = next_name) {
        next_name = name->next;
        free(name);
    }
}

/* Convert grouped C6 filename/segment/line pairs to a source-module record. */
#include "image.h"
#include <recon.h>
#pragma intrinsic(memcpy)

void pack_new_lines(void *lines, CvDirectoryEntry *entry, ModuleInfo *module)
{
    unsigned char *input = module_data;
    unsigned char *end = input+entry->size;
    unsigned char *filename = NULL, *buffer, *output, *file, *file_end;
    unsigned char *range, *offset, *numbers, *previous_end;
    unsigned short file_segments[20], segments[256];
    unsigned long file_offsets[20], file_headers[20];
    unsigned short files = 0, segment_count = 0, name_length = 0;
    unsigned short segment, count, i, j;
    unsigned long file_bytes = 0, total = 0, header_size;
    unsigned long stride = is_pe_image ? 6 : 4;
    int new_file;

    file_segments[0] = 0;
    while (input < end) {
        new_file = !filename || strncmp((const char *)input, (const char *)filename, name_length+1);
        if (new_file) {
            if (filename) {
                file_offsets[files] = total;
                file_headers[files] = (file_segments[files]*12+6+name_length+3)&~3UL;
                file_bytes += file_headers[files];
                ++files;
                /* Preserve the original's doubled spacing between files. */
                total += file_bytes;
                total += file_bytes;
            }
            filename = input;
            name_length = (unsigned short)(signed char)*input;
            file_bytes = 0;
            file_segments[files] = 0;
            file_offsets[files] = total;
        }
        input += *input+1;
        segment = *(unsigned short *)input;
        input += 2;
        if (file_segments[files] < 256) ++file_segments[files];
        for (i = 0; i < segment_count; ++i)
            if (segments[i] == segment) break;
        if (i == segment_count && segment_count < 256)
            segments[segment_count++] = segment;
        count = *(unsigned short *)input;
        input += 2+count*stride;
        file_bytes += count*6+4+(count&1)*2;
    }
    file_headers[files] = (file_segments[files]*12+5+name_length+3)&~3UL;
    total += file_bytes+file_headers[files];
    ++files;
    header_size = 4+files*4+segment_count*10+(segment_count&1)*2;
    total += header_size;
    if (total >= 0x10000UL) {
        warning(8, "source lines", module_name(module));
        entry->location.data = NULL;
        entry->size = 0;
        module->line_size = 0;
        module->lines = NULL;
        return;
    }
    buffer = malloc((unsigned short)total);
    if (!buffer) fatal_error(14, NULL, NULL);
    output = buffer;
    if (!output) fatal_error(14, NULL, NULL);
    *(unsigned short *)output = files;
    *(unsigned short *)(output+2) = segment_count;
    output += 4;
    for (i = 0; i < files; ++i) {
        *(unsigned long *)output = file_offsets[i]+header_size;
        output += 4;
    }
    for (i = 0; i < segment_count; ++i) {
        *(unsigned long *)output = 0;
        *(unsigned long *)(output+4) = 0;
        output += 8;
    }
    for (i = 0; i < segment_count; ++i) {
        *(unsigned short *)output = segments[i];
        output += 2;
    }
    if (segment_count&1) {
        *(unsigned short *)output = 0;
        output += 2;
    }

    input = lines;
    filename = NULL;
    files = 0;
    previous_end = output;
    while (input < end) {
        if (!filename || strncmp((const char *)input, (const char *)filename, name_length+1)) {
            file = buffer+header_size+file_offsets[files];
            /* The doubled file spacing above left these original output
             * bytes unwritten. Keep the layout and write zeroes to its gaps. */
            while (previous_end < file) *previous_end++ = 0;
            file_end = file+file_headers[files];
            count = file_segments[files];
            *(unsigned short *)file = count;
            *(unsigned short *)(file+2) = 0;
            offset = file+4;
            range = offset+count*4;
            output = range+count*8;
            filename = input;
            name_length = (unsigned short)(signed char)*input;
            *output++ = (unsigned char)name_length;
            memcpy(output, filename+1, name_length);
            output += name_length;
            while (output < file_end) *output++ = 0;
            ++files;
        }
        input += *input+1;
        segment = *(unsigned short *)input;
        input += 2;
        count = *(unsigned short *)input;
        input += 2;
        *(unsigned long *)offset = file_end-buffer;
        offset += 4;
        *(unsigned long *)range = 0;
        *(unsigned long *)(range+4) = 0;
        range += 8;
        output = file_end;
        *(unsigned short *)output = segment;
        *(unsigned short *)(output+2) = count;
        output += 4;
        numbers = output+count*4;
        for (j = 0; j < count; ++j) {
            *(unsigned short *)numbers = *(unsigned short *)input;
            numbers += 2;
            input += 2;
            if (is_pe_image) {
                *(unsigned long *)output = *(unsigned long *)input;
                input += 4;
            } else {
                *(unsigned long *)output = *(unsigned short *)input;
                input += 2;
            }
            output += 4;
        }
        if (count&1) {
            *(unsigned short *)numbers = 0;
            numbers += 2;
        }
        file_end = numbers;
        previous_end = file_end;
    }
    entry->location.data = buffer;
    entry->size = (unsigned short)total;
    module->line_size = (unsigned short)total;
    module->lines = buffer;
    if (current_module->lines) rebuild_line_ranges(current_module);
}

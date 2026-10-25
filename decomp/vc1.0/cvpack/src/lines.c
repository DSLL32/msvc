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

/* Temporary ownership while clipping each file's line pairs to module ranges. */
typedef struct LineRange {
    unsigned short segment, reserved;
    unsigned long first, last;
    unsigned short count;
    unsigned char pairs[1];
} LineRange;
typedef struct LineFile {
    unsigned short count, reserved;
    unsigned char *name;
    LineRange *ranges[1];
} LineFile;
#pragma intrinsic(memset)

void rebuild_line_ranges(ModuleInfo *module)
{
    unsigned char *record = module->record;
    unsigned char *old = module->lines, *header, *buffer, *file, *source;
    unsigned char *name, *block, *output;
    unsigned long header_size, total, cursor, offset, count_bytes;
    unsigned long *file_offsets, *module_ranges;
    unsigned short *module_segments;
    unsigned short files, contributions, i, j, k, n, count, copied;
    ModuleContribution *contribution;
    LineFile *item;
    LineRange *range;

    if (!record) fatal_error(14, NULL, NULL);
    if (!old) fatal_error(14, NULL, NULL);
    files = *(unsigned short *)old;
    contributions = *(unsigned short *)(record+4);
    header_size = (4+files*4+contributions*10+3)&~3UL;
    total = header_size;
    header = malloc(header_size);
    if (!header) fatal_error(13, NULL, NULL);
    memset(header, 0, header_size);
    *(unsigned short *)header = files;
    *(unsigned short *)(header+2) = contributions;
    file_offsets = (unsigned long *)(header+4);
    module_ranges = (unsigned long *)(header+4+files*4);
    module_segments = (unsigned short *)(header+4+files*4+contributions*8);
    contribution = (ModuleContribution *)(record+8);
    for (i = 0; i < contributions; ++i) {
        module_segments[i] = contribution[i].segment;
        module_ranges[i*2] = contribution[i].offset;
        module_ranges[i*2+1] = contribution[i].offset+contribution[i].size;
    }
    for (i = 0; i < files; ++i) {
        source = old+((unsigned long *)(old+4))[i];
        n = *(unsigned short *)source;
        name = source+4+n*12;
        item = malloc(8+contributions*4);
        if (!item) fatal_error(13, NULL, NULL);
        file_offsets[i] = (unsigned long)item;
        memset(item, 0, 8+contributions*4);
        item->name = malloc(*name+1);
        if (!item->name) fatal_error(13, NULL, NULL);
        memcpy(item->name, name, *name+1);
        count = 0;
        for (j = 0; j < contributions; ++j) {
            contribution = (ModuleContribution *)(record+8)+j;
            count = 0;
            for (k = 0; k < n; ++k) {
                block = old+((unsigned long *)(source+4))[k];
                if (*(unsigned short *)block == contribution->segment)
                    for (copied = 0; copied < *(unsigned short *)(block+2); ++copied) {
                        offset = ((unsigned long *)(block+4))[copied];
                        if (offset >= contribution->offset &&
                            offset < contribution->offset+contribution->size)
                            ++count;
                    }
            }
            if (!count) continue;
            /* Retain the original allocation spacing and final record offsets. */
            total = (total+(count+1)*6+3)&~3UL;
            range = malloc(16+count*6);
            item->ranges[item->count++] = range;
            range->segment = contribution->segment;
            range->count = count;
            copied = 0;
            for (k = 0; k < n; ++k) {
                block = old+((unsigned long *)(source+4))[k];
                if (*(unsigned short *)block == contribution->segment) {
                    unsigned short index, pairs = *(unsigned short *)(block+2);
                    for (index = 0; index < pairs; ++index) {
                        offset = ((unsigned long *)(block+4))[index];
                        if (offset >= contribution->offset &&
                            offset < contribution->offset+contribution->size) {
                            ((unsigned long *)range->pairs)[copied] = offset;
                            ((unsigned short *)(range->pairs+count*4))[copied] =
                                ((unsigned short *)(block+4+pairs*4))[index];
                            ++copied;
                        }
                    }
                }
                if (copied == range->count) break;
            }
            range->first = ((unsigned long *)range->pairs)[0];
            range->last = ((unsigned long *)range->pairs)[copied-1];
        }
        total = (total+item->count*14+*item->name+7+3)&~3UL;
    }
    buffer = malloc(total);
    if (!buffer) fatal_error(14, NULL, NULL);
    if (!buffer) fatal_error(14, NULL, NULL);
    memset(buffer, 0, total);
    memcpy(buffer, header, header_size);
    cursor = header_size;
    for (i = 0; i < files; ++i) {
        item = (LineFile *)((unsigned long *)(buffer+4))[i];
        ((unsigned long *)(buffer+4))[i] = cursor;
        file = buffer+cursor;
        *(unsigned short *)file = item->count;
        *(unsigned short *)(file+2) = 0;
        memcpy(file+4+item->count*12, item->name, *item->name+1);
        cursor = (cursor+item->count*14+*item->name+7+3)&~3UL;
        for (j = 0; j < item->count; ++j) {
            range = item->ranges[j];
            ((unsigned long *)(file+4))[j] = cursor;
            ((unsigned long *)(file+4+item->count*4))[j*2] = range->first;
            ((unsigned long *)(file+4+item->count*4))[j*2+1] = range->last;
            output = buffer+cursor;
            *(unsigned short *)output = range->segment;
            *(unsigned short *)(output+2) = range->count;
            count_bytes = range->count*6;
            memcpy(output+4, range->pairs, count_bytes);
            cursor = (cursor+(range->count+1)*6+3)&~3UL;
            free(range);
        }
        free(item->name);
        free(item);
    }
    free(header);
    sort_line_ranges(record, buffer);
    free(module->lines);
    module->lines = buffer;
    module->line_size = total;
}

typedef struct LineSortEntry {
    unsigned short segment, reserved;
    unsigned long start;
    unsigned long *end;
} LineSortEntry;
typedef char LineSortEntrySize[(sizeof(LineSortEntry)==12)?1:-1];

void sort_line_ranges(void *record, unsigned char *lines)
{
    unsigned char *module_record = record;
    unsigned char *file, *block;
    unsigned short count = 0, used = 0, i, j;
    unsigned short files = *(unsigned short *)lines;
    unsigned short contributions = *(unsigned short *)(module_record+4);
    ModuleContribution *contribution, *best;
    LineSortEntry *entries;
    unsigned long *ranges;

    for (i = 0; i < files; ++i) {
        file = lines+((unsigned long *)(lines+4))[i];
        count += *(unsigned short *)file;
    }
    entries = malloc(count*12);
    for (i = 0; i < files; ++i) {
        file = lines+((unsigned long *)(lines+4))[i];
        ranges = (unsigned long *)(file+4+*(unsigned short *)file*4);
        for (j = 0; j < *(unsigned short *)file; ++j) {
            block = lines+((unsigned long *)(file+4))[j];
            entries[used].segment = *(unsigned short *)block;
            entries[used].start = ranges[j*2];
            entries[used].end = &ranges[j*2+1];
            ++used;
        }
    }
    qsort(entries, count, 12, compare_line_ranges);
    for (i = 0; i < count; ++i) {
        best = NULL;
        contribution = (ModuleContribution *)(module_record+8);
        for (j = 0; j < contributions; ++j, ++contribution)
            if (contribution->segment == entries[i].segment &&
                contribution->offset+contribution->size > entries[i].start &&
                (!best || best->offset > contribution->offset))
                best = contribution;
        if (i+1 < count && entries[i+1].segment == entries[i].segment &&
            best->offset+best->size > entries[i+1].start)
            *entries[i].end = entries[i+1].start-1;
        else
            *entries[i].end = best->offset+best->size-1;
    }
    contribution = (ModuleContribution *)(module_record+8);
    for (i = 0; i < contributions; ++i, ++contribution) {
        j = 0;
        for (;;) {
            if (j && (entries[j].segment > contribution->segment ||
                      (entries[j].segment == contribution->segment &&
                       contribution->offset > entries[j].start))) {
                --j;
                break;
            }
            if (entries[j].segment == contribution->segment &&
                entries[j].start == contribution->offset)
                break;
            ++j;
            if (j == count) {
                --j;
                break;
            }
        }
        if (entries[j].segment == contribution->segment)
            entries[j].end[-1] = contribution->offset;
    }
    free(entries);
}

int __cdecl compare_line_ranges(const void *left, const void *right)
{
    const LineSortEntry *a = left, *b = right;
    int order = (int)a->segment-(int)b->segment;
    if (order) return order;
    if (a->start < b->start) return -1;
    if (a->start > b->start) return 1;
    return 0;
}

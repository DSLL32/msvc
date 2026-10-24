/* Buffered packed CodeView output and executable-header repair. */
#include "output.h"
#include <recon.h>

unsigned char *output_buffer;
unsigned long output_capacity, output_used;
int output_handle;
PackedSignature packed_signature = { { 'N', 'B', '0', '9' }, 0 };
long packed_file_size;
unsigned long source_line_bytes;

/* The original inlines this fast path around its buffered writer. */
#define OUTPUT_BYTES(source, count) do { \
    const unsigned char *emit_source = (const unsigned char *)(source); \
    unsigned long emit_size = (count), emit_i; \
    if (output_capacity-output_used >= emit_size) { \
        for (emit_i = 0; emit_i < emit_size; ++emit_i) \
            output_buffer[output_used+emit_i] = emit_source[emit_i]; \
        output_used += emit_size; \
    } else if (!write_output(emit_source, (unsigned short)emit_size)) \
        fatal_error(20, NULL, NULL); \
} while (0)

void write_packed_debug(void)
{
    ModuleInfo *module;
    CvDirectoryEntry *entry;
    unsigned long count = 0, i, directory, debug_size, remainder, rounded;
    long change;
    IMAGE_SECTION_HEADER section;
    IMAGE_DEBUG_DIRECTORY debug;
    IMAGE_OPTIONAL_HEADER optional;

    for (module = module_list; module; module = module->next) {
        ++count;
        if (module->symbol_size) ++count;
        if (module->line_size) ++count;
    }
    directory_entry_count = count;
    count = 0;
    for (module = module_list; module; module = module->next) {
        entry = &directory_entries[count++];
        entry->subsection = 0x120;
        entry->module = module->module;
        entry->location.data = module->record;
        entry->size = module->record_size;
        if (module->symbol_size) {
            entry = &directory_entries[count++];
            entry->subsection = 0x125;
            entry->module = module->module;
            entry->size = module->symbol_size;
            entry->location.data = module->symbols;
        }
        if (module->line_size) {
            entry = &directory_entries[count++];
            entry->subsection = 0x127;
            entry->module = module->module;
            entry->size = module->line_size;
            entry->location.data = module->lines;
            source_line_bytes += module->line_size;
        }
    }
    if (verbose == 1) printf("Line/Address size\t\t= %8.ld\n", source_line_bytes);
    qsort(directory_entries, directory_entry_count, sizeof(*directory_entries),
          compare_output_directory);
    cv_file_offset = (cv_file_offset+3)&~3L;
    if (seek_file(input_file, cv_file_offset, 0) == -1) fatal_error(20, NULL, NULL);
    initialize_output(input_file);
    OUTPUT_BYTES(&packed_signature, sizeof(packed_signature));
    cv_read_offset = tell_file(input_file)+output_used;
    for (i = 0; i < directory_entry_count; ++i) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[i];
        if (entry->size > 65535) {
            for (module = module_list; module && module->module != entry->module;
                 module = module->next) {}
            if (entry->subsection == 0x125) warning(8, "symbols", module_name(module));
            else if (entry->subsection == 0x127)
                warning(8, "source lines", module_name(module));
            else warning(8, NULL, module_name(module));
            entry->size = 0;
        }
        write_subsection(i);
    }
    cv_read_offset = tell_file(input_file)+output_used;
    write_global_publics(&directory_entries[directory_entry_count],
                         cv_read_offset-cv_file_offset);
    ++directory_entry_count;
    cv_read_offset = tell_file(input_file)+output_used;
    write_global_symbols(&directory_entries[directory_entry_count],
                         cv_read_offset-cv_file_offset);
    ++directory_entry_count;
    cv_read_offset = tell_file(input_file)+output_used;
    write_static_symbols(&directory_entries[directory_entry_count],
                         cv_read_offset-cv_file_offset);
    ++directory_entry_count;
    cv_read_offset = tell_file(input_file)+output_used;
    entry = &directory_entries[directory_entry_count];
    entry->subsection = 0x128;
    entry->module = 0xffff;
    entry->location.data = libraries;
    entry->size = libraries_size;
    write_subsection(directory_entry_count);
    cv_read_offset = tell_file(input_file)+output_used;
    ++directory_entry_count;
    entry = &directory_entries[directory_entry_count];
    entry->subsection = 0x12b;
    entry->module = 0xffff;
    if (already_packed) {
        entry->size = write_old_packed_types();
        entry->location.offset = cv_read_offset-cv_file_offset;
    } else {
        entry->size = write_packed_types();
        entry->location.offset = cv_read_offset-cv_file_offset;
        if (verbose == 1)
            printf("Initial type size\t\t= %8.ld\nCompacted type size\t\t= %8.ld\n",
                   total_local_type_bytes, entry->size);
    }
    if (segment_map) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[++directory_entry_count];
        entry->subsection = 0x12d;
        entry->module = 0xffff;
        entry->location.data = segment_map;
        entry->size = segment_map_size;
        write_subsection(directory_entry_count);
    }
    if (segment_names) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[++directory_entry_count];
        entry->subsection = 0x12e;
        entry->module = 0xffff;
        entry->location.data = segment_names;
        entry->size = segment_names_size;
        write_subsection(directory_entry_count);
    }
    if (source_file_index) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[++directory_entry_count];
        entry->subsection = 0x133;
        entry->module = 0xffff;
        entry->location.data = source_file_index;
        entry->size = source_file_index_size;
        write_subsection(directory_entry_count);
    }
    cv_read_offset = tell_file(input_file)+output_used;
    cv_directory_header.header_size = 16;
    ++directory_entry_count;
    cv_directory_header.entry_size = 12;
    cv_directory_header.count = directory_entry_count;
    cv_directory_header.next = cv_directory_header.flags = 0;
    directory = tell_file(input_file)-cv_file_offset+output_used;
    OUTPUT_BYTES(&cv_directory_header, sizeof(cv_directory_header));
    OUTPUT_BYTES(directory_entries, (unsigned short)(cv_directory_header.count*12));
    debug_size = tell_file(input_file)-cv_file_offset+output_used+8;
    OUTPUT_BYTES(packed_signature.signature, 4);
    OUTPUT_BYTES(&debug_size, 4);
    if (!flush_output()) fatal_error(20, NULL, NULL);
    packed_file_size = tell_file(input_file);
    if (is_pe_image) {
        if (seek_file(input_file, section_header_offset, 0) == -1 ||
            read_file(input_file, &section, sizeof(section)) != sizeof(section))
            fatal_error(20, NULL, NULL);
        if (seek_file(input_file, debug_directory_offset+section.PointerToRawData, 0) == -1 ||
            read_file(input_file, &debug, sizeof(debug)) != sizeof(debug))
            fatal_error(20, NULL, NULL);
        debug.SizeOfData = debug_size;
        if (seek_file(input_file, debug_directory_offset+section.PointerToRawData, 0) == -1 ||
            write_file(input_file, &debug, sizeof(debug)) != sizeof(debug))
            fatal_error(20, NULL, NULL);
        if (debug_section_offset) {
            change = packed_file_size-original_file_size;
            if (seek_file(input_file, optional_header_offset, 0) == -1 ||
                read_file(input_file, &optional, sizeof(optional)) != sizeof(optional))
                fatal_error(20, NULL, NULL);
            if (seek_file(input_file, debug_section_offset, 0) == -1 ||
                read_file(input_file, &section, sizeof(section)) != sizeof(section))
                fatal_error(20, NULL, NULL);
            section.SizeOfRawData += change;
#ifdef RECON_NATIVE_TEST
            if (!optional.FileAlignment || !optional.SectionAlignment) __asm { int 3 }
#endif
            rounded = section.SizeOfRawData;
            remainder = rounded%optional.FileAlignment;
            if (remainder) rounded += optional.FileAlignment-remainder;
            section.Misc.VirtualSize = rounded;
            if (seek_file(input_file, debug_section_offset, 0) == -1 ||
                write_file(input_file, &section, sizeof(section)) != sizeof(section))
                fatal_error(20, NULL, NULL);
            rounded = section.Misc.VirtualSize+section.VirtualAddress;
            remainder = rounded%optional.SectionAlignment;
            if (remainder) rounded += optional.SectionAlignment-remainder;
            optional.SizeOfImage = rounded;
            if (seek_file(input_file, optional_header_offset, 0) == -1 ||
                write_file(input_file, &optional, sizeof(optional)) != sizeof(optional))
                fatal_error(20, NULL, NULL);
        }
    }
    truncate_file(input_file, packed_file_size);
    if (seek_file(input_file, cv_file_offset+4, 0) == -1 ||
        write_file(input_file, &directory, 4) != 4) fatal_error(20, NULL, NULL);
}

int __cdecl compare_output_directory(const void *left, const void *right)
{
    const CvDirectoryEntry *a = left, *b = right;

    if (a->subsection == 0x120 || b->subsection == 0x120) {
        if (a->subsection == 0x120 && b->subsection != 0x120) return -1;
        if (a->subsection != 0x120 && b->subsection == 0x120) return 1;
    }
    if (b->module > a->module) return -1;
    if (b->module < a->module) return 1;
    if (b->subsection > a->subsection) return -1;
    if (b->subsection < a->subsection) return 1;
    return 0;
}

void initialize_output(int handle)
{
    output_capacity = 0xffe8;
    do {
        output_buffer = malloc(output_capacity);
        if (output_buffer) break;
        output_capacity >>= 1;
    } while (output_capacity >= 0x1000);
    if (!output_buffer) output_capacity = 0;
    output_used = 0;
    output_handle = handle;
}

unsigned long output_padding;
unsigned char output_zeroes[4] = { 0, 0, 0, 0 };

void write_subsection(unsigned long index)
{
    CvDirectoryEntry *entry = &directory_entries[index];
    const unsigned char *data;

    if (entry->size) {
        data = entry->location.data;
        if (!data) fatal_error(13, NULL, NULL);
        OUTPUT_BYTES(data, (unsigned short)entry->size);
    }
    entry->location.offset = cv_read_offset-cv_file_offset;
    output_padding = 4-(entry->size & 3);
    if (output_padding != 4) OUTPUT_BYTES(output_zeroes, output_padding);
}

/* The packed symbol subsection precedes its name and address hash streams. */
typedef struct SymbolHashHeader {
    unsigned short name_kind, address_kind;
    unsigned long symbols, names, addresses;
} SymbolHashHeader;
typedef char SymbolHashHeaderSize[(sizeof(SymbolHashHeader)==16)?1:-1];

void write_global_publics(CvDirectoryEntry *entry, unsigned long offset)
{
    unsigned long zero = 0, bucket, bytes, padding;
    SymbolHashHeader header;
    GrowingBuffer names, addresses;
    PublicSymbol *symbol;
    BufferBlock *block;

    header.name_kind = header.address_kind = 0;
    header.symbols = header.names = header.addresses = 0;
    public_symbols_size += pad_symbols(public_symbol_hash, 4);
    header.symbols = public_symbols_size;
    build_symbol_name_hash(hash_public, &header.name_kind, &header.names,
                           &names, public_symbols_count, public_symbol_hash);
    build_symbol_address_hash(&header.address_kind, &header.addresses,
                              &addresses, public_symbols_count, public_symbol_hash);
    entry->subsection = 0x12a;
    entry->module = 0xffff;
    entry->location.offset = offset;
    entry->size = header.symbols+header.names+header.addresses+sizeof(header);
    OUTPUT_BYTES(&header, sizeof(header));
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (symbol = public_symbol_hash[bucket]; symbol; symbol = symbol->next) {
            bytes = *(unsigned short *)symbol->record+2;
            OUTPUT_BYTES(symbol->record, bytes);
        }
    }
    if (header.name_kind) {
        for (block = first_buffer_block(&names); block; block = next_buffer_block(block))
            OUTPUT_BYTES(block->data, block->used);
    }
    if (header.address_kind) {
        for (block = first_buffer_block(&addresses); block; block = next_buffer_block(block))
            OUTPUT_BYTES(block->data, block->used);
    }
    padding = 4-(entry->size & 3);
    if (padding != 4) OUTPUT_BYTES(&zero, padding);
    if (verbose == 1) printf("Public symbol size\t\t= %8.1ld\n", header.symbols);
}

unsigned long pad_symbols(void **buckets, unsigned long alignment)
{
    unsigned short position = 0, extra = (unsigned short)(alignment+4), padding;
    unsigned long bucket, total = 0, i;
    PublicSymbol *symbol, *previous, *node;

    for (bucket = 0; bucket < 4096; ++bucket) {
        previous = NULL;
        for (symbol = buckets[bucket]; symbol; symbol = symbol->next) {
            if ((unsigned long)*(unsigned short *)symbol->record+extra+position+2 > 4096) {
                padding = (unsigned short)(4096-position);
                node = allocate_symbol((unsigned short)(padding+20));
                node->hash = 0xffff;
                node->name_offset = 0xffff;
                node->segment = 0;
                node->offset = 0;
#ifdef RECON_NATIVE_TEST
                if (padding < 4) __asm { int 3 }
#endif
                for (i = 0; i < padding; ++i) node->record[i] = 0;
                *(unsigned short *)node->record = (unsigned short)(padding-2);
                *(unsigned short *)(node->record+2) = 0x402;
                node->next = symbol;
                if (previous) previous->next = node;
                else buckets[bucket] = node;
                total += padding;
                position = 0;
            }
            position += (unsigned short)(*(unsigned short *)symbol->record+2);
            previous = symbol;
        }
        if (bucket == 4095 && alignment >= 4) {
            node = allocate_symbol(28);
            node->hash = 0xffff;
            node->name_offset = 0xffff;
            node->segment = 0;
            node->offset = 0;
            *(unsigned long *)node->record = 0xffffffff;
            *(unsigned long *)(node->record+4) = 0xffffffff;
            *(unsigned short *)node->record = 6;
            *(unsigned short *)(node->record+2) = 0x402;
            node->next = symbol;
            if (previous) previous->next = node;
            else buckets[bucket] = node;
            total += 8;
        }
    }
    return total;
}

typedef struct SymbolHashPair {
    unsigned long offset, hash;
} SymbolHashPair;
typedef struct NameHashOverflow {
    struct NameHashOverflow *next;
    SymbolHashPair pairs[5];
} NameHashOverflow;
typedef struct NameHashBucket {
    unsigned short count, reserved;
    NameHashOverflow *overflow;
    SymbolHashPair pairs[12];
} NameHashBucket;
typedef char NameHashBucketSize[(sizeof(NameHashBucket)==104)?1:-1];
typedef char NameHashOverflowSize[(sizeof(NameHashOverflow)==44)?1:-1];

void build_symbol_name_hash(SymbolHash hash, unsigned short *kind,
                            unsigned long *size, GrowingBuffer *buffer,
                            unsigned long count, void **buckets)
{
    NameHashOverflow *overflow_pages[10], **link, *block;
    NameHashBucket **pages, *bucket;
    PublicSymbol *symbol;
    unsigned short bucket_count, page_count, overflow_count = 0;
    unsigned short i, j, part, remainder, remaining, take;
    unsigned long quotient, bytes, offset = 0, position = 0;

    for (i = 0; i < 10; ++i) overflow_pages[i] = NULL;
    if (!count) return;
    quotient = count/10;
    if (quotient >= 65535) quotient = 65535;
    bucket_count = (unsigned short)quotient;
    if (bucket_count <= 6) bucket_count = 6;
    bucket_count = (unsigned short)(bucket_count+1)&0xfffe;
#ifdef RECON_NATIVE_TEST
    if (!bucket_count) __asm { int 3 }
#endif
    page_count = (unsigned short)(bucket_count/629+1);
    pages = checked_alloc(page_count*4);
    for (i = 0; i < page_count; ++i) {
        bytes = page_count-i == 1 ? (bucket_count*104)%0xff88 : 0xff88;
        pages[i] = checked_zero(bytes);
    }
    for (i = 0; i < 4096; ++i) {
        for (symbol = buckets[i]; symbol; symbol = symbol->next) {
            if (*(unsigned short *)(symbol->record+2) != 0x402) {
                j = (unsigned short)(symbol->hash%bucket_count);
                bucket = &pages[j/629][j%629];
                if (bucket->count < 12) {
                    bucket->pairs[bucket->count].offset = position;
                    bucket->pairs[bucket->count].hash = symbol->hash;
                } else {
                    link = &bucket->overflow;
                    remainder = (unsigned short)((bucket->count-12)%5);
                    while (*link) {
                        if (remainder && !(*link)->next) break;
                        link = &(*link)->next;
                    }
                    if (!remainder) {
                        part = (unsigned short)(overflow_count/1488);
#ifdef RECON_NATIVE_TEST
                        if (part >= 10) __asm { int 3 }
#endif
                        if (!overflow_pages[part]) overflow_pages[part] = checked_zero(0xffe0);
                        *link = &overflow_pages[part][overflow_count%1488];
                        ++overflow_count;
                    }
                    (*link)->pairs[remainder].offset = position;
                    (*link)->pairs[remainder].hash = symbol->hash;
                }
                ++bucket->count;
            }
            position += *(unsigned short *)symbol->record+2;
        }
    }
    initialize_buffer(buffer, 4096);
    append_buffer(buffer, &bucket_count, 2);
    append_fill(buffer, 0, 2);
    for (i = 0; i < bucket_count; ++i) {
        bucket = &pages[i/629][i%629];
        append_buffer(buffer, &offset, 4);
        offset += bucket->count*8;
    }
    for (i = 0; i < bucket_count; ++i) {
        bucket = &pages[i/629][i%629];
        append_buffer(buffer, &bucket->count, 2);
        append_fill(buffer, 0, 2);
    }
    for (i = 0; i < bucket_count; ++i) {
        bucket = &pages[i/629][i%629];
        block = bucket->overflow;
        remaining = bucket->count;
        take = remaining < 12 ? remaining : 12;
        append_buffer(buffer, bucket->pairs, take*8);
        remaining -= take;
        while (remaining) {
            take = remaining < 5 ? remaining : 5;
            append_buffer(buffer, block->pairs, take*8);
            remaining -= take;
            block = block->next;
        }
    }
    *kind = 10;
    *size = (bucket_count+count)*8+4;
    for (i = 0; i < page_count; ++i) free(pages[i]);
    free(pages);
    for (i = 0; i < 10; ++i) free(overflow_pages[i]);
}

void *append_fill(GrowingBuffer *buffer, unsigned char value, unsigned long size)
{
    BufferBlock *block;
    unsigned char *result;
    unsigned long i;

    if (buffer->block_size < size) return NULL;
    if (buffer->available < size) {
        block = checked_zero(sizeof(*block));
        block->data = checked_alloc(buffer->block_size);
        if (buffer->last) buffer->last->next = block;
        else buffer->first = block;
        ++buffer->block_count;
        buffer->last = block;
        buffer->next = block->data;
        buffer->available = buffer->block_size;
    }
    result = buffer->next;
    for (i = 0; i < size; ++i) result[i] = value;
    buffer->next += size;
    buffer->available -= size;
    buffer->last->used += size;
    return result;
}

typedef struct AddressHashEntry {
    PublicSymbol *symbol;
    unsigned long stream_offset;
} AddressHashEntry;
typedef struct AddressHashSegment {
    unsigned long count, used;
    AddressHashEntry *entries;
} AddressHashSegment;
typedef char AddressHashEntrySize[(sizeof(AddressHashEntry)==8)?1:-1];
typedef char AddressHashSegmentSize[(sizeof(AddressHashSegment)==12)?1:-1];

void build_symbol_address_hash(unsigned short *kind, unsigned long *size,
                               GrowingBuffer *buffer, unsigned long count,
                               void **buckets)
{
    unsigned short max_segment = 0;
    unsigned long i, j, bucket, orphans = 0, position = 0, offset = 0;
    AddressHashSegment *segments, *segment;
    AddressHashEntry *entry;
    PublicSymbol *symbol;

    if (!count) return;
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (symbol = buckets[bucket]; symbol; symbol = symbol->next) {
            if (*(unsigned short *)(symbol->record+2) != 0x402 &&
                symbol->segment > max_segment) max_segment = symbol->segment;
        }
    }
    segments = checked_zero(max_segment*sizeof(*segments));
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (symbol = buckets[bucket]; symbol; symbol = symbol->next) {
            if (*(unsigned short *)(symbol->record+2) != 0x402) {
                if (!symbol->segment) ++orphans;
                else ++segments[symbol->segment-1].count;
            }
        }
    }
    for (i = 0; i < max_segment; ++i) {
        if (segments[i].count)
            segments[i].entries = checked_zero(segments[i].count*sizeof(AddressHashEntry));
    }
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (symbol = buckets[bucket]; symbol; symbol = symbol->next) {
            if (*(unsigned short *)(symbol->record+2) != 0x402 && symbol->segment) {
                segment = &segments[symbol->segment-1];
                entry = &segment->entries[segment->used++];
                entry->symbol = symbol;
                entry->stream_offset = position;
            }
            position += *(unsigned short *)symbol->record+2;
        }
    }
    for (i = 0; i < max_segment; ++i)
        qsort(segments[i].entries, segments[i].count, sizeof(AddressHashEntry),
              compare_symbol_address);
    initialize_buffer(buffer, 4096);
    append_buffer(buffer, &max_segment, 2);
    append_fill(buffer, 0, 2);
    for (i = 0; i < max_segment; ++i) {
        append_buffer(buffer, &offset, 4);
        offset += segments[i].count*8;
    }
    for (i = 0; i < max_segment; ++i) {
        append_buffer(buffer, &segments[i].count, 2);
        append_fill(buffer, 0, 2);
    }
    for (i = 0; i < max_segment; ++i) {
        for (j = 0; j < segments[i].count; ++j) {
            entry = &segments[i].entries[j];
            append_buffer(buffer, &entry->stream_offset, 4);
            append_buffer(buffer, &entry->symbol->offset, 4);
        }
    }
    *kind = 12;
    *size = (max_segment-orphans+count)*8+4;
    for (i = 0; i < max_segment; ++i)
        if (segments[i].entries) free(segments[i].entries);
    free(segments);
}

int __cdecl compare_symbol_address(const void *left, const void *right)
{
    const AddressHashEntry *a = left, *b = right;
    return (int)(a->symbol->offset-b->symbol->offset);
}

BufferBlock *first_buffer_block(const GrowingBuffer *buffer)
{
    return buffer->first;
}

BufferBlock *next_buffer_block(const BufferBlock *block)
{
    return block->next;
}

void write_global_symbols(CvDirectoryEntry *entry, unsigned long offset)
{
    unsigned long zero = 0, bucket, bytes, padding;
    SymbolHashHeader header;
    GrowingBuffer names, addresses;
    PublicSymbol *symbol;
    BufferBlock *block;

    header.name_kind = header.address_kind = 0;
    header.symbols = header.names = header.addresses = 0;
    global_symbols_size += pad_symbols(global_symbol_hash, 4);
    header.symbols = global_symbols_size;
    build_symbol_name_hash(hash_public, &header.name_kind, &header.names,
                           &names, global_symbols_count, global_symbol_hash);
    build_symbol_address_hash(&header.address_kind, &header.addresses,
                              &addresses, global_symbols_count, global_symbol_hash);
    entry->subsection = 0x129;
    entry->module = 0xffff;
    entry->location.offset = offset;
    entry->size = header.symbols+header.names+header.addresses+sizeof(header);
    OUTPUT_BYTES(&header, sizeof(header));
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (symbol = global_symbol_hash[bucket]; symbol; symbol = symbol->next) {
            bytes = *(unsigned short *)symbol->record+2;
            OUTPUT_BYTES(symbol->record, bytes);
        }
    }
    if (header.name_kind) {
        for (block = first_buffer_block(&names); block; block = next_buffer_block(block))
            OUTPUT_BYTES(block->data, block->used);
    }
    if (header.address_kind) {
        for (block = first_buffer_block(&addresses); block; block = next_buffer_block(block))
            OUTPUT_BYTES(block->data, block->used);
    }
    padding = 4-(entry->size & 3);
    if (padding != 4) OUTPUT_BYTES(&zero, padding);
    if (verbose == 1)
        printf("Initial symbol size\t\t= %8.ld\nFinal symbol size\t\t= %8.ld\nGlobal symbol size\t\t= %8.1ld\n",
               total_new_type_bytes, total_packed_symbol_bytes, header.symbols);
}

void write_static_symbols(CvDirectoryEntry *entry, unsigned long offset)
{
    unsigned long zero = 0, bucket, bytes, padding;
    SymbolHashHeader header;
    GrowingBuffer names, addresses;
    PublicSymbol *symbol;
    BufferBlock *block;

    header.name_kind = header.address_kind = 0;
    header.symbols = header.names = header.addresses = 0;
    local_symbol_reference_bytes += pad_symbols(type_hash_extra_c, 0);
    header.symbols = local_symbol_reference_bytes;
    build_symbol_name_hash(hash_public, &header.name_kind, &header.names,
                           &names, local_symbol_references, type_hash_extra_c);
    build_symbol_address_hash(&header.address_kind, &header.addresses,
                              &addresses, local_symbol_references, type_hash_extra_c);
    entry->subsection = 0x134;
    entry->module = 0xffff;
    entry->location.offset = offset;
    entry->size = header.symbols+header.names+header.addresses+sizeof(header);
    OUTPUT_BYTES(&header, sizeof(header));
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (symbol = type_hash_extra_c[bucket]; symbol; symbol = symbol->next) {
            bytes = *(unsigned short *)symbol->record+2;
            OUTPUT_BYTES(symbol->record, bytes);
        }
    }
    if (header.name_kind) {
        for (block = first_buffer_block(&names); block; block = next_buffer_block(block))
            OUTPUT_BYTES(block->data, block->used);
    }
    if (header.address_kind) {
        for (block = first_buffer_block(&addresses); block; block = next_buffer_block(block))
            OUTPUT_BYTES(block->data, block->used);
    }
    padding = 4-(entry->size & 3);
    if (padding != 4) OUTPUT_BYTES(&zero, padding);
}

/* NB09 global types: relative offsets, then records in 48K logical pages. */
unsigned long write_packed_types(void)
{
    unsigned long count = (unsigned short)(next_type_index-0x1000);
    unsigned long signature = 1, zero = 0, index, chunk, total;
    unsigned short page_used = 0, length, padding, fill;
    unsigned char *record, *end;
    BufferBlock *block;

    build_type_offsets();
    OUTPUT_BYTES(&signature, sizeof(signature));
    OUTPUT_BYTES(&count, sizeof(count));
    for (index = 0; index < count; index += chunk) {
        chunk = count-index;
        if (chunk >= 512) chunk = 512;
        OUTPUT_BYTES(type_pages[index/512], chunk*4);
    }
    total = count*4+8;
    for (block = first_buffer_block(&type_symbol_storage); block;
         block = next_buffer_block(block)) {
        record = block->data;
        end = record+block->used;
        while (record < end) {
            length = (unsigned short)(*(unsigned short *)record+2);
            padding = (unsigned short)(3-((length-1)&3));
            if ((unsigned long)page_used+length+padding > 0xc000) {
                fill = (unsigned short)(0xc000-page_used);
                total += fill;
                if (fill) {
                    fill = (unsigned short)((unsigned short)(fill+3)>>2);
                    do { OUTPUT_BYTES(&zero, 4); } while (--fill);
                }
                page_used = 0;
            }
            page_used = (unsigned short)(page_used+length+padding);
            OUTPUT_BYTES(record, length);
            if (padding) OUTPUT_BYTES(&zero, padding);
            record += length;
            total += length+padding;
        }
    }
    output_padding = 4-(total&3);
    if (output_padding != 4) OUTPUT_BYTES(output_zeroes, output_padding);
    return total;
}

/* Replace the type-record pointers with offsets into the emitted stream. */
void build_type_offsets(void)
{
    unsigned short index = 0, page_used = 0, length;
    unsigned long offset = 0, record_size, aligned;
    unsigned char **slot;

    while (index < (unsigned short)(next_type_index-0x1000)) {
        slot = &type_pages[index/512][index%512];
        length = (unsigned short)(*(unsigned short *)*slot+2);
        record_size = length;
        aligned = record_size+3-((record_size-1)&3);
        if ((unsigned long)page_used+aligned > 0xc000) {
            offset += 0xc000-page_used;
            page_used = 0;
        }
        *slot = (unsigned char *)offset;
        offset += aligned;
        page_used = (unsigned short)(page_used+(unsigned short)aligned);
        ++index;
    }
}

unsigned short flush_output(void)
{
    unsigned long size = output_used;
    if (write_file(output_handle, output_buffer, size) != size) return 0;
    free(output_buffer);
    output_capacity = 0;
    return 1;
}

/* Optional output pointers request a read-only checksum instead of an update. */
int update_image_checksum(const char *name, unsigned long *old_sum,
                          unsigned long *new_sum)
{
    FILE *stream;
    HANDLE mapping;
    void *view;
    /* Preserve the original CRT calls despite stdio's _fileno macro. */
    stream = fopen(name, new_sum ? "rb" : "r+b");
    if (!stream) return 1;
    mapping = CreateFileMappingA((HANDLE)_get_osfhandle((_fileno)(stream)), NULL,
                                new_sum ? PAGE_READONLY : PAGE_READWRITE, 0, 0, NULL);
    if (!mapping) { fclose(stream); return 2; }
    view = MapViewOfFile(mapping, new_sum ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(mapping);
    if (!view) { fclose(stream); return 3; }
    checksum_image(view, _filelength((_fileno)(stream)), old_sum, new_sum);
    UnmapViewOfFile(view);
    fclose(stream);
    return 0;
}

void *checksum_image(void *image, unsigned long size, unsigned long *old_sum,
                     unsigned long *new_sum)
{
    unsigned char *header;
    unsigned short sum, part;
    unsigned long result;
    if (old_sum) *old_sum = 0;
    sum = (unsigned short)checksum_words(0, image, (size+1)>>1);
    header = image_nt_header(image);
    if (header) {
        if (old_sum) *old_sum = *(unsigned long *)(header+0x58);
        part = *(unsigned short *)(header+0x58);
        sum = (unsigned short)(sum-(part>sum)-part);
        part = *(unsigned short *)(header+0x5a);
        sum = (unsigned short)(sum-(part>sum)-part);
    }
    result = (unsigned long)sum+size;
    if (new_sum) *new_sum = result;
    else {
#ifdef RECON_NATIVE_TEST
        /* The original faults here on an invalid image and no output pointer. */
        if (!header) __asm { int 3 }
#endif
        *(unsigned long *)(header+0x58) = result;
    }
    return header;
}

/* End-around carries, equivalent to the original unrolled DWORD accumulator. */
unsigned long checksum_words(unsigned long sum, const unsigned short *data,
                             unsigned long words)
{
    unsigned long previous;
    words = (words << 1) >> 1;  /* Original byte-count conversion is 32-bit. */
    while (words--) {
        previous = sum;
        sum += *data++;
        if (sum < previous) ++sum;
    }
    sum = (sum & 0xffff)+(sum >> 16);
    return (sum+(sum >> 16)) & 0xffff;
}
#undef OUTPUT_BYTES

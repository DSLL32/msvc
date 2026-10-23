#include "cvtres.h"
#include <io.h>
#include <string.h>
#include <recon.h>

ResourcePadding resource_padding;
ResourceIdentifier *resource_strings;
ResourceEntry *resource_files;
ResourceType *resource_id_types, *resource_named_types;
WORD name_entry_count, name_directory_count, type_count;
WORD id_type_count, named_type_count, resource_count, language_entry_count;
BYTE *directory_buffer;

const char *resource_type_names[21] = {
    NULL, "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING",
    "FONTDIR", "FONT", "ACCELERATOR", "RCDATA", "MESSAGETABLE",
    "GROUP_CURSOR", NULL, "GROUP_ICON", NULL, "VERSION", "DLGINCLUDE",
    NULL, "PLUGPLAY", "VXD"
};

IMAGE_FILE_HEADER object_header = { 0, 3, 0, 0, 0, 0, 0x100 };
IMAGE_SECTION_HEADER debug_section = {
    ".debug$S", {0}, 0, 0, 0, 0, 0, 0, 0, 0x42100040
};
IMAGE_SECTION_HEADER directory_section = {
    ".rsrc$01", {0}, 0, 0, 0, 0, 0, 0, 0, 0x40000040
};
IMAGE_SECTION_HEADER data_section = {
    ".rsrc$02", {0}, 0, 0, 0, 0, 0, 0, 0, 0x40000040
};
IMAGE_SYMBOL compiler_symbol = { {"@comp.id"}, 0x60684, -1, 0, 3, 0 };
IMAGE_SYMBOL debug_symbol = { {".debug$S"}, 0, 1, 0, 3, 1 };
IMAGE_SYMBOL directory_symbol = { {".rsrc$01"}, 0, 2, 0, 3, 1 };
IMAGE_SYMBOL data_symbol = { {".rsrc$02"}, 0, 3, 0, 3, 1 };
IMAGE_SYMBOL resource_symbol = { {"R0000000"}, 0, 3, 0, 3, 0 };
IMAGE_RELOCATION resource_relocation;
typedef struct CodeViewRecord {
    WORD length, type;
    DWORD flags;
} CodeViewRecord;
CodeViewRecord object_record = {0, 9, 0};
CodeViewRecord compiler_record = {0, 1, 0x08000800};

int __stdcall convert_resources(FILE *input, FILE *output, unsigned long size,
                                int writable, unsigned long timestamp)
{
    DWORD data_size, header_size, position, root_end, type_end, name_end;
    DWORD string_offset, directory_size, alignment, padding, symbol_index;
    ResourceHeader *header;
    ResourceIdentifier *type, *name, *string;
    ResourceEntry *entry, *last = NULL, *language;
    ResourceType *group;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *root_entry, *name_entry, *language_entry;
    IMAGE_RESOURCE_DIRECTORY *type_directory, *name_directory;
    IMAGE_RESOURCE_DATA_ENTRY *data_entry;
    IMAGE_AUX_SYMBOL auxiliary;
    BYTE *string_data;
    BYTE length;
    DWORD i, count;
    int type_pass, name_pass, converted;
    char symbol_name[32];
    const char *compiler_name = "Microsoft CVTRES 5.00.1668.1";
    DWORD debug_signature = 1;

    if (size) do {
        read_bytes(input, &data_size, 4);
        read_bytes(input, &header_size, 4);
        if (header_size < 8) {
            error(1107, input_name);
            return 0;
        }
        if (!data_size) position = seek_file(input, header_size - 8, SEEK_CUR);
        else {
            header = allocate_block(24);
            header->data_size = data_size;
            header->header_size = header_size;
            type = read_identifier(input);
            name = read_identifier(input);
            position = seek_file(input, 0, SEEK_CUR);
            while (position & 3) position = seek_file(input, 1, SEEK_CUR);
            read_bytes(input, &header->data_version, 16);
            if (type->kind == 1 || type->value.ordinal != 17) {
                if (verbose) {
                    if (type->kind == 1)
                        printf(load_message(5001), type->text, 32);
                    else if (type->value.ordinal <= 20 && resource_type_names[type->value.ordinal])
                        printf(load_message(5002), resource_type_names[type->value.ordinal], 32);
                    else printf(load_message(5003), type->value.ordinal, 32);
                    if (name->kind == 1) printf(load_message(5004), name->text, 32);
                    else printf(load_message(5005), name->value.ordinal, 32);
                    printf(load_message(5006), header->language, header->memory_flags,
                           header->data_size);
                }
                entry = insert_resource(type, name, header);
                entry->offset = seek_file(input, 0, SEEK_CUR);
                entry->symbol_index = resource_count++;
                if (!last) resource_files = entry;
                else last->next_file = entry;
                last = entry;
            }
            position = seek_file(input, header->data_size, SEEK_CUR);
            while (position & 3) position = seek_file(input, 1, SEEK_CUR);
        }
    } while (position < size);

    root_end = 16 + 8 * type_count;
    type_end = root_end + 16 * type_count + 8 * name_entry_count;
    name_end = type_end + 16 * name_directory_count + 8 * language_entry_count;
    string_offset = name_end + 16 * resource_count;
    directory_size = (string_offset + resource_padding.name_bytes + 3) & ~3UL;
    directory_buffer = allocate_block(directory_size + 4);
    while ((DWORD)directory_buffer & 3) ++directory_buffer;
    init_directory((IMAGE_RESOURCE_DIRECTORY *)directory_buffer, 0, 0, timestamp,
                   named_type_count, id_type_count);
    root_entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(directory_buffer + 16);
    type_directory = (IMAGE_RESOURCE_DIRECTORY *)(directory_buffer + root_end);
    name_directory = (IMAGE_RESOURCE_DIRECTORY *)(directory_buffer + type_end);
    data_entry = (IMAGE_RESOURCE_DATA_ENTRY *)(directory_buffer + name_end);
    string_data = directory_buffer + string_offset;

    /* Named types precede ordinal types; each type likewise emits named then
     * ordinal names. A name's first language node owns its language count. */
    for (type_pass = 0; type_pass < 2; ++type_pass) {
        group = type_pass ? resource_id_types : resource_named_types;
        for (; group; group = group->next) {
            root_entry->Name = type_pass ? group->identifier->value.ordinal :
                (string_offset + group->identifier->name_offset) | 0x80000000UL;
            root_entry->OffsetToData = ((BYTE *)type_directory - directory_buffer) | 0x80000000UL;
            ++root_entry;
            init_directory(type_directory, 0, 0, timestamp, group->name_count, group->id_count);
            name_entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(type_directory + 1);
            for (name_pass = 0; name_pass < 2; ++name_pass) {
                entry = name_pass ? group->ids : group->names;
                while (entry) {
                    init_directory(name_directory, 0, 0, timestamp, 0, entry->language_count);
                    language_entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(name_directory + 1);
                    name_entry->Name = name_pass ? entry->name->value.ordinal :
                        (string_offset + entry->name->name_offset) | 0x80000000UL;
                    name_entry->OffsetToData = ((BYTE *)name_directory - directory_buffer) | 0x80000000UL;
                    count = entry->language_count;
                    while (count--) {
                        language = entry;
                        language_entry->Name = language->header->language;
                        language_entry->OffsetToData = (BYTE *)data_entry - directory_buffer;
                        language->data_entry_offset = (BYTE *)data_entry - directory_buffer;
                        init_data_entry(data_entry, 0, 0, 0, 0);
                        ++data_entry;
                        ++language_entry;
                        entry = language->next;
                    }
                    ++name_entry;
                    name_directory = (IMAGE_RESOURCE_DIRECTORY *)language_entry;
                }
            }
            type_directory = (IMAGE_RESOURCE_DIRECTORY *)name_entry;
        }
    }
    for (string = resource_strings; string; string = string->value.next) {
        for (i = 0; i < string->byte_count; ++i)
            string_data[i] = ((BYTE *)&string->length)[i];
        string_data += string->byte_count;
        while ((DWORD)string_data & 1) ++string_data;
    }

    object_header.TimeDateStamp = timestamp;
    object_header.Machine = target_machine;
    if (writable) {
        directory_section.Characteristics |= 0x80000000UL;
        data_section.Characteristics |= 0x80000000UL;
    }
    debug_section.PointerToRawData = 140;
    seek_file(output, 140, SEEK_SET);
    write_bytes(output, &debug_signature, 4);
    for (i = 0; output_name[i]; ++i) {}
    length = (BYTE)i;
    object_record.length = length + 7;
    write_bytes(output, &object_record, 8);
    write_bytes(output, &length, 1);
    write_bytes(output, output_name, length);
    for (i = 0; compiler_name[i]; ++i) {}
    length = (BYTE)i;
    compiler_record.length = length + 7;
    write_bytes(output, &compiler_record, 8);
    write_bytes(output, &length, 1);
    write_bytes(output, compiler_name, length);
    debug_section.SizeOfRawData = seek_file(output, 0, SEEK_CUR) - debug_section.PointerToRawData;
    if (debug_section.SizeOfRawData & 1) write_bytes(output, &resource_padding, 1);
    directory_section.PointerToRawData = seek_file(output, 0, SEEK_CUR);
    directory_section.NumberOfRelocations = resource_count;
    directory_section.SizeOfRawData = directory_size;
    directory_section.PointerToRelocations = directory_section.PointerToRawData + directory_size;
    data_section.PointerToRawData = directory_section.PointerToRelocations + 10 * resource_count;
    seek_file(output, data_section.PointerToRawData, SEEK_SET);
    alignment = windows_ce ? 4 : 8;
    for (entry = resource_files; entry; entry = entry->next_file) {
        position = seek_file(input, entry->offset, SEEK_SET);
        while (position & 3) position = seek_file(input, 1, SEEK_CUR);
        entry->offset = seek_file(output, 0, SEEK_CUR) - data_section.PointerToRawData;
        converted = 0;
        if ((windows_ce || windows_ce_emulation) && !windows_ce_noconvert &&
            (entry->type->value.ordinal == 2 || entry->type->value.ordinal == 3))
            converted = convert_ce_image(windows_ce, input, output, entry, entry->type->value.ordinal);
        if (!converted) copy_bytes(input, output, entry->header->data_size);
        padding = (alignment - entry->header->data_size % alignment) % alignment;
        if (padding) write_bytes(output, &resource_padding, padding);
        ((IMAGE_RESOURCE_DATA_ENTRY *)(directory_buffer + entry->data_entry_offset))->Size = entry->header->data_size;
    }
    data_section.SizeOfRawData = seek_file(output, 0, SEEK_CUR) - data_section.PointerToRawData;
    if (data_section.SizeOfRawData & 1) write_bytes(output, &resource_padding, 1);
    object_header.PointerToSymbolTable = seek_file(output, 0, SEEK_CUR);
    object_header.NumberOfSymbols = resource_count + 7;
    write_bytes(output, &compiler_symbol, 18);
    for (i = 0; i < 18; ++i) auxiliary.File.Name[i] = 0;
    auxiliary.Section.Length = debug_section.SizeOfRawData;
    auxiliary.Section.NumberOfRelocations = debug_section.NumberOfRelocations;
    write_bytes(output, &debug_symbol, 18);
    write_bytes(output, &auxiliary, 18);
    auxiliary.Section.Length = directory_section.SizeOfRawData;
    auxiliary.Section.NumberOfRelocations = directory_section.NumberOfRelocations;
    write_bytes(output, &directory_symbol, 18);
    write_bytes(output, &auxiliary, 18);
    auxiliary.Section.Length = data_section.SizeOfRawData;
    auxiliary.Section.NumberOfRelocations = data_section.NumberOfRelocations;
    write_bytes(output, &data_symbol, 18);
    write_bytes(output, &auxiliary, 18);
    for (entry = resource_files; entry; entry = entry->next_file) {
        sprintf(symbol_name, "$R%06lX", entry->offset);
        resource_symbol.N.Name.Short = resource_symbol.N.Name.Long = 0;
        strncpy((char *)resource_symbol.N.ShortName, symbol_name, 8);
        resource_symbol.Value = entry->offset;
        write_bytes(output, &resource_symbol, 18);
    }
    write_bytes(output, &resource_padding, 4);
    _chsize(output->_file, seek_file(output, 0, SEEK_CUR));
    seek_file(output, 0, SEEK_SET);
    write_bytes(output, &object_header, 20);
    write_bytes(output, &debug_section, 40);
    write_bytes(output, &directory_section, 40);
    write_bytes(output, &data_section, 40);
    seek_file(output, directory_section.PointerToRawData, SEEK_SET);
    write_bytes(output, directory_buffer, directory_size);
    seek_file(output, directory_section.PointerToRelocations, SEEK_SET);
    resource_relocation.Type = resource_machine_kind;
    symbol_index = 7;
    for (entry = resource_files; entry; entry = entry->next_file) {
        resource_relocation.VirtualAddress = entry->data_entry_offset;
        resource_relocation.SymbolTableIndex = symbol_index++;
        write_bytes(output, &resource_relocation, 10);
    }
    return 1;
}

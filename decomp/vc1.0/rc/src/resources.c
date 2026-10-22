/* Resource script dispatch, original 0x01009e70..0x0100a489.
 * Two adjacent resource-kind switch tables occupy 0x0100a48c..0x0100a514. */
#include "resource.h"
#include <string.h>
#include <wchar.h>
#include <recon.h>
#pragma intrinsic(strlen, strcmp)

DWORD default_resource_version, default_characteristics;
unsigned int resource_count;
WORD font_count;
struct KanjiRange kanji_ranges[2];
BOOL font_directory_written;
struct ResourceInfo *string_table_resource;
void *dialog_scratch;

BOOL __cdecl compile_resources(void)
{
    DWORD zero = 0, header_size = 32;
    WORD ordinal_marker = 0xffff;
    struct ResourceInfo *resource;
    struct ResourceType *type;

    resource_count = 0;
    font_count = 0;
    initialize_resource_types();
    /* The mandatory null Win32 .res record is written field by field. */
    write_file(resource_output, &zero, 4);
    write_file(resource_output, &header_size, 4);
    write_file(resource_output, &ordinal_marker, 2);
    write_file(resource_output, &zero, 2);
    write_file(resource_output, &ordinal_marker, 2);
    write_file(resource_output, &zero, 2);
    write_file(resource_output, &zero, 4);
    write_file(resource_output, &zero, 2);
    write_file(resource_output, &zero, 2);
    write_file(resource_output, &zero, 4);
    write_file(resource_output, &zero, 4);
    allocate_resource_data();
    for (;;) {
        if (!parse_resource_identifier())
            break;
        if (!strcmp(current_token.spelling, "KANJI")) {
            advance_argument(); kanji_ranges[0].first = parse_word_expression();
            advance_argument(); kanji_ranges[0].last = parse_word_expression();
            advance_argument(); kanji_ranges[1].first = parse_word_expression();
            advance_argument(); kanji_ranges[1].last = parse_word_expression();
            kanji_mode = 1;
        } else if (!strcmp(current_token.spelling, "LANGUAGE"))
            default_language = (default_language & 0xffff0000UL) | parse_language();
        else if (!strcmp(current_token.spelling, "VERSION")) {
            next_token(0);
            if (current_token.kind != 20)
                parser_error("VERSION not a number", "");
            default_resource_version = current_token.value;
        } else if (!strcmp(current_token.spelling, "CHARACTERISTICS")) {
            next_token(0);
            if (current_token.kind != 20)
                parser_error("CHARACTERISTICS not a number", "");
            default_characteristics = current_token.value;
        } else {
            if (verbose) {
                fprintf(diagnostics, ".");
                fflush(diagnostics);
            }
            resource = resource_alloc(sizeof(*resource));
            resource->language = (WORD)default_language;
            resource->version = default_resource_version;
            resource->characteristics = default_characteristics;
            if (!current_token.length) {
                if (strlen(current_token.spelling) > 256) {
                    fprintf(diagnostics, "\n%s (%d) : warning RW4006: Name String too long - truncated at 256\n",
                            source_filename, current_token.line);
                    current_token.spelling[256] = 0;
                    current_token.length = 255;
                }
                ansi_to_wide(current_token.text, current_token.spelling,
                              strlen(current_token.spelling) + 1);
                resource->name = duplicate_wide(current_token.text);
            } else
                resource->ordinal = current_token.length;
            type = 0;
            if (resource->name && !wcscmp(resource->name, L"STRINGTABLE")) {
                next_token(0);
                resource->memory_flags = 0x1030;
                while (parse_memory_flag(resource))
                    ;
            } else {
                if (!parse_resource_identifier())
                    break;
                if (!current_token.length) {
                    if (strlen(current_token.spelling) > 256) {
                        fprintf(diagnostics, "\n%s (%d) : warning RW4006: Type String too long - truncated at 256\n",
                                source_filename, current_token.line);
                        current_token.spelling[256] = 0;
                        current_token.length = 255;
                    }
                    ansi_to_wide(current_token.text, current_token.spelling,
                                  strlen(current_token.spelling) + 1);
                    type = find_resource_type(current_token.text, 0);
                } else
                    type = find_resource_type(0, current_token.length);
                if (!type)
                    return current_token.attributes < 1;
                next_token(0);
                switch (type->ordinal) {
                case 1: case 3: case 4: case 5: case 8: case 17:
                    resource->memory_flags = 0x1030;
                    break;
                case 12: case 14:
                    resource->memory_flags = 0x1010;
                    break;
                default:
                    resource->memory_flags = 0x30;
                    break;
                }
                while (parse_memory_flag(resource))
                    ;
            }
            if (!type) {
                if (resource->name && !wcscmp(resource->name, L"STRINGTABLE")) {
                    if (!parse_string_table(resource))
                        break;
                } else
                    parser_error("Invalid Type", "");
            } else {
                begin_resource_data();
                resource->data_size = 0;
                switch (type->ordinal) {
                case 1: case 3:
                    resource->data_size = read_resource_file();
                    resource->data_size = read_icon_file_header(resource->data_size);
                    break;
                case 2:
                    resource->data_size = read_resource_file();
                    resource->data_size = read_bitmap_file_header();
                    break;
                case 4:
                    put_word(0);
                    put_word(0);
                    parse_menu(0, resource);
                    break;
                case 5:
                    dialog_scratch = resource_alloc(2078);
                    parse_dialog(resource);
                    write_resource(type, resource);
                    resource_free(dialog_scratch);
                    goto next_resource;
                case 7:
                    font_directory_written = TRUE;
                    resource->data_size = read_resource_file();
                    break;
                case 8:
                    resource->data_size = read_resource_file();
                    if (resource->name)
                        parser_error("Font names must be ordinals", "");
                    if (!parse_font(resource))
                        goto next_resource;
                    ++font_count;
                    break;
                case 9:
                    parse_accelerators(resource);
                    break;
                case 10:
                    parse_raw_data(resource);
                    break;
                case 11:
                    resource->data_size = read_resource_file();
                    break;
                case 12:
                    split_icon_file(type, resource, 1);
                    goto next_resource;
                case 14:
                    split_icon_file(type, resource, 3);
                    goto next_resource;
                case 16:
                    parse_version_info();
                    break;
                case 17:
                    parse_dialog_include(resource);
                    break;
                default:
                    if (current_token.kind == TOKEN_BEGIN)
                        parse_raw_data(resource);
                    else
                        resource->data_size = read_resource_file();
                    break;
                }
                write_resource(type, resource);
            }
        }
next_resource:
        if (current_token.kind == TOKEN_EOF)
            break;
    }
    if (!font_directory_written && font_count)
        emit_font_directory();
    if (string_table_resource)
        emit_string_tables(string_table_resource);
    release_resource_data();
    return current_token.attributes < 1;
}

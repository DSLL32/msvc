/* Original main [00404620,004048b5), including its SEH filter and handler.
 * Acceptance and original-byte restoration are recorded in functions.json. */
#include "cvpack.h"
#include <recon.h>

int input_file = -1, show_logo = 1, banner_pending = 1, checksum_required;
unsigned short module_count, current_module_has_types;
void *debug_directory, *line_data, *symbol_data, *type_data, *module_data;

int main(int argc, char **argv, char **environment)
{
    char temporary[260];
    char *source, *destination;
    unsigned int module, size;
    unsigned short result = 0;
    HMODULE image_help;
    CheckSumMappedFileProc checksum;
    PIMAGE_NT_HEADERS headers;
    DWORD old_checksum, new_checksum;
    void *image;
    __try {
        initialize_messages("cvpack.err", _pgmptr, 1);
        _setmode(_fileno(stdout), _O_TEXT);
        parse_options(argc, argv);
        files_initialize(16, 16, 2, 16, 2, 1);
        if (show_logo && banner_pending) print_banner();
        source = argv[argc-1];
        destination = temporary;
        do { *destination++ = *source; } while (*source++);
        select_input_name(temporary);
        initialize_type_tables();
        read_image_debug();
        initialize_module_pack();
        for (module = 0; module < module_count; ++module) pack_module(module);
        free(debug_directory);
        free(line_data);
        free(symbol_data);
        free(type_data);
        free(module_data);
        free_widen_buffers();
        free_type_tables();
        if (current_module_has_types) print_statistics();
        flush_packed_file();
        if (checksum_required) {
            image_help = LoadLibraryA("imagehlp");
            if (image_help) {
                headers = NULL;
                checksum = (CheckSumMappedFileProc)GetProcAddress(image_help, "CheckSumMappedFile");
                if (checksum) {
                    size = get_file_size(input_file);
                    image = map_file_region(input_file, 0, size);
                    if (image) headers = checksum(image, size, &old_checksum, &new_checksum);
                }
                if (headers) headers->OptionalHeader.CheckSum = new_checksum;
                else warning(4010, NULL, NULL);
            }
        }
        if (input_file != -1) {
            close_file(input_file, 1);
            input_file = -1;
        }
        exit(result);
    } __except (1) {
        return fprintf(stderr, "\n\n***** %s INTERNAL ERROR, exception code = 0x%lx *****\n\n",
                       argv[0], GetExceptionCode());
    }
    return result;
}

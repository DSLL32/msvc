/* Original main [00017000,000174b6), including its exception filter/handler. */
#include "cvpack.h"
#include <recon.h>

int show_logo = 1, banner_pending = 1, cvnew = 1;
int feedback, pcode, input_file, already_packed, checksum_required;
unsigned char mapped_io, is_pe_image;
unsigned short module_count, statistics_enabled;
char executable_name[260];
void *directory_data, *module_data, *type_data, *symbol_data, *global_type_data;

int main(int argc, char **argv, char **environment)
{
    char temporary[260], drive[4], directory[256], filename[256], extension[256];
    char message[80], progress[200];
    char *flags, *source, *destination;
    unsigned short result = 0, module;
    unsigned count;
    __try {
        _setmode(_fileno(stdout), _O_TEXT);
        flags = getenv("_MSC_IDE_FLAGS");
        feedback = flags != NULL;
        if (feedback) feedback = strncmp(flags, "-FEEDBACK ", 10) == 0;
        if (feedback) {
            source = "@I0\n";
            for (count = 0; source[count]; ++count) {}
            _write(2, source, count);
            sprintf(progress, "%s %d.%02d.%02d\n",
                    "@I1Microsoft (R) Debugging Information Compactor  Version", 4, 25, 0);
            for (count = 0; progress[count]; ++count) {}
            _write(2, progress, count);
            source = "@I2Copyright (c) Microsoft Corp 1987-1993. All rights reserved.\n";
            for (count = 0; source[count]; ++count) {}
            _write(2, source, count);
        }
        parse_options(argc, argv);
        files_initialize(16, 16, 2, 16, 2, mapped_io, fatal_error);
        if (show_logo == 1 && banner_pending == 1) print_banner();
        source = argv[argc-1];
        destination = temporary;
        do { *destination++ = *source; } while (*source++);
        source = temporary;
        destination = executable_name;
        do { *destination++ = *source; } while (*source++);
        _splitpath(executable_name, drive, directory, filename, extension);
        if (!extension[0]) {
            source = ".exe";
            destination = extension;
            do { *destination++ = *source; } while (*source++);
        }
        _makepath(executable_name, drive, directory, filename, extension);
        if (feedback) {
            _snprintf(message, 80, "%s%s\n", "@I3Packing: ", executable_name);
            for (count = 0; message[count]; ++count) {}
            _write(2, message, count);
        }
        input_file = open_file(executable_name, 0x8002);
        if (input_file == -1) {
            input_file = open_file(executable_name, 0x8000);
            fatal_error(input_file == -1 ? 15 : 16, executable_name, NULL);
        }
        initialize_type_tables();
        read_image_debug();
        if (!already_packed)
            for (module = 0; module < module_count; ++module) pack_module(module);
        if (is_pe_image && cvnew) strip_debug(input_file);
        free(directory_data);
        free(global_type_data);
        free(symbol_data);
        free(type_data);
        free(module_data);
        write_debug_directory();
        if (statistics_enabled) print_statistics();
        if (feedback) {
            source = "@I4Writing Packed Info\n";
            for (count = 0; source[count]; ++count) {}
            _write(2, source, count);
        }
        free_type_tables();
        close_file(input_file, 1);
        if (checksum_required && update_image_checksum(temporary, 0, 0))
            warning(10, NULL, NULL);
        if (pcode) {
            if (show_logo)
                result = (unsigned short)_spawnlp(_P_WAIT, "mpc", "mpc", argv[argc-1], NULL);
            else
                result = (unsigned short)_spawnlp(_P_WAIT, "mpc", "mpc", "/nologo", argv[argc-1], NULL);
            /* The original zero-extends the short before comparing with -1. */
            if (result == -1) fatal_error(28, NULL, NULL);
        }
        exit(result);
    } __except (1) {
        return fprintf(stderr, "\n\n***** %s INTERNAL ERROR, exception code = 0x%lx *****\n\n",
                argv[0], GetExceptionCode());
    }
    return result;
}

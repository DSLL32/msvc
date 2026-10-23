#include "cvtres.h"
#include <stdlib.h>
#include <string.h>
#include <share.h>
#include <time.h>
#include <recon.h>

unsigned short target_machine;
unsigned short resource_machine_kind;
char *input_name, *output_name;
int verbose, windows_ce, windows_ce_emulation, windows_ce_noconvert;
int writable_resources = 1;

int main(int argc, char **argv, char **envp)
{
    int i, no_logo = 0, ce_machine = 0, mips_r41xx = 0, converted;
    char *option, *machine_name;
    char drive[_MAX_DRIVE], directory[_MAX_DIR];
    char filename[_MAX_FNAME], extension[_MAX_EXT];
    char input_path[_MAX_PATH], output_path[_MAX_PATH];
    FILE *input, *output;
    long input_size;

    if (argc == 1) usage(0);
    for (i = 1; i < argc; ++i) {
        option = argv[i];
        if (*option != '/' && *option != '-') {
            input_name = option;
            continue;
        }
        ++option;
        if (!_strnicmp(option, "machine:", 8)) {
            option += 8;
            if (!_stricmp(option, "IX86") || !_stricmp(option, "X86")) {
                target_machine = 0x14c;
                resource_machine_kind = 7;
            } else if (!_stricmp(option, "ALPHA")) {
                target_machine = 0x184;
                resource_machine_kind = 0x10;
            } else if (!_stricmp(option, "ARM")) {
                target_machine = 0x1c0;
                resource_machine_kind = 2;
                ce_machine = 1;
            } else if (!_stricmp(option, "MIPS")) {
                target_machine = 0x166;
                resource_machine_kind = 0x22;
                ce_machine = 1;
            } else if (!_stricmp(option, "MIPSR41XX")) {
                target_machine = 0x166;
                resource_machine_kind = 0x22;
                mips_r41xx = 1;
                ce_machine = 1;
            } else if (!_stricmp(option, "PPC")) {
                target_machine = 0x1f0;
                resource_machine_kind = 0x10;
                ce_machine = 1;
            } else if (!_stricmp(option, "SH3")) {
                target_machine = 0x1a2;
                resource_machine_kind = 0x10;
                ce_machine = 1;
            } else if (!_stricmp(option, "SH4")) {
                target_machine = 0x1a6;
                resource_machine_kind = 0x10;
                ce_machine = 1;
            } else if (!_stricmp(option, "OMNI")) {
                target_machine = 0xace1;
                resource_machine_kind = 2;
            } else usage(1);
        } else if (!_stricmp(option, "nologo")) no_logo = 1;
        else if (!_stricmp(option, "o")) output_name = argv[++i];
        else if (!_strnicmp(option, "out:", 4)) output_name = option + 4;
        else if (!_stricmp(option, "readonly") || !_stricmp(option, "r")) writable_resources = 0;
        else if (!_stricmp(option, "verbose") || !_stricmp(option, "v")) verbose = 1;
        else if (!_stricmp(option, "windowsce")) windows_ce = 1;
        else if (!_stricmp(option, "windowsce:emulation")) windows_ce_emulation = 1;
        else if (!_stricmp(option, "windowsce:noconvert")) windows_ce_noconvert = 1;
        else if (!_stricmp(option, "Brepro")) { }
        else if (!_stricmp(option, "I386") || !_stricmp(option, "IX86")) {
            target_machine = 0x14c;
            resource_machine_kind = 7;
        } else if (!_stricmp(option, "ALPHA")) {
            target_machine = 0x184;
            resource_machine_kind = 0x10;
        } else if (!_stricmp(option, "MIPS")) {
            target_machine = 0x166;
            resource_machine_kind = 0x22;
            ce_machine = 1;
        } else if (!_stricmp(option, "PPC")) {
            target_machine = 0x1f0;
            resource_machine_kind = 0x10;
            ce_machine = 1;
        } else usage(1);
    }
    if (!input_name) usage(1);
    if (!no_logo) {
        printf(load_message(8003), "5.00.1668.1");
        puts(load_message(8004));
    }
    if (ce_machine && !windows_ce) {
        switch (target_machine) {
        case 0x166: machine_name = mips_r41xx ? "MIPSR41XX" : "MIPS"; break;
        case 0x1a2: machine_name = "SH3"; break;
        case 0x1a6: machine_name = "SH4"; break;
        case 0x1c0: machine_name = "ARM"; break;
        case 0x1f0: machine_name = "PPC"; break;
        default: machine_name = (char *)mips_r41xx; break;
        }
        error(1109, machine_name, "/WINDOWSCE");
    }
    if (!target_machine) {
        warning(4001, "IX86");
        target_machine = 0x14c;
        resource_machine_kind = 7;
    }
    _splitpath(input_name, drive, directory, filename, extension);
    if (!*extension) {
        _makepath(input_path, drive, directory, filename, ".res");
        input_name = input_path;
    }
    input = _fsopen(input_name, "rb", _SH_DENYWR);
    if (!input) { error(1101, input_name); exit(1); }
    input_size = seek_file(input, 0, SEEK_END);
    seek_file(input, 0, SEEK_SET);
    if (!output_name) {
        _makepath(output_path, drive, directory, filename, ".obj");
        output_name = output_path;
    }
    output = _fsopen(output_name, "wb", _SH_DENYRW);
    if (!output) { error(1108, output_name); exit(1); }
    _tzset();
    converted = convert_resources(input, output, input_size, writable_resources, 0);
    fclose(input);
    fclose(output);
    exit(!converted);
    return 0;
}

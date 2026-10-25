#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "link.h"
#include "initializers.h"
#include "../crt/crt-imports.h"
#include <recon.h>

FILE *diagnostic_output;
char *program_name;
const char *selected_tool_name;
int selected_tool_kind;
unsigned char let_exceptions_escape, logo_pending;
char current_phase[32];
const char *default_phase = "SetupPhase";

typedef struct ToolMode {
    const char *name;
    unsigned prefix_length;
    int kind;
    int (__stdcall *run)(int, char **);
} ToolMode;

ToolMode tool_modes[] = {
    { "Linker",          4, 1, run_linker },
    { "Library Manager", 3, 2, run_librarian },
    { "Dumper",          4, 3, run_dumper },
    { "Editor",          4, 4, run_editor },
    { "Helper",          4, 0, run_helper },
    { NULL,              0, 0, NULL }
};

int __cdecl main(int argc, char **argv, char **environment)
{
    char *debug_options;
    char basename[_MAX_FNAME];
    unsigned short tool;
    int i;
    DWORD exception_status;
    void *delay_info;
    EXCEPTION_RECORD exception_record;
    CONTEXT context;

    diagnostic_output = stdout;
    __try {
        SetConsoleCtrlHandler(console_control, TRUE);
        debug_options = NULL;
        for (i = 1; i < argc; ++i) {
            if (argv[i][0] == '-' && argv[i][1] == 'd' &&
                argv[i][2] == 'b' &&
                (!argv[i][3] || (isdigit)((signed char)argv[i][3]))) {
                debug_options = argv[i] + 3;
                memmove(argv + i, argv + i + 1,
                        (argc - i - 1) * sizeof(char *));
                --argc;
                --i;
            }
        }
        initialize_debug(debug_options, "LINK_DB");
        _splitpath(argv[0], NULL, NULL, basename, NULL);
        program_name = _strupr(basename);
        initialize_file_cache(16, 14, 18, 16);
        initialize_memory();
        tool = 0;
        if (argc >= 2 && (argv[1][0] == '/' || argv[1][0] == '-')) {
            while (tool_modes[tool].name) {
                if (!_strnicmp(argv[1] + 1, tool_modes[tool].name,
                               tool_modes[tool].prefix_length) &&
                    !argv[1][tool_modes[tool].prefix_length + 1]) {
                    for (i = 1; i < argc; ++i) argv[i] = argv[i + 1];
                    --argc;
                    break;
                }
                ++tool;
            }
            if (!tool_modes[tool].name) tool = 0;
        }
        switch (tool_modes[tool].kind) {
        case 3:
            break;
        case 4:
            setvbuf(stdout, NULL, _IOFBF, 0);
            setvbuf(stderr, NULL, _IOFBF, 0);
            break;
        default:
            setvbuf(stdout, NULL, _IOFBF, 2048);
            setvbuf(stderr, NULL, _IOFBF, 2048);
            break;
        }
        selected_tool_name = tool_modes[tool].name;
        selected_tool_kind = tool_modes[tool].kind;
        return tool_modes[tool].run(argc, argv);
    }
    __except (save_exception(let_exceptions_escape, (exception_status = GetExceptionCode()),
                            GetExceptionInformation(), &delay_info,
                            &exception_record, &context)) {
        if (logo_pending) print_logo();
        if (delay_info) report_delayed_import(exception_status, delay_info, 1);
        printf("\n%s : error : Internal error during %s\n",
               current_phase[0] ? current_phase : program_name, default_phase);
        printf("\n  ExceptionCode            = %08lX\n"
               "  ExceptionFlags           = %08lX\n"
               "  ExceptionAddress         = %08lX\n"
               "  NumberParameters         = %08lX\n",
               exception_record.ExceptionCode, exception_record.ExceptionFlags,
               exception_record.ExceptionAddress, exception_record.NumberParameters);
        for (i = 0; (unsigned)i < exception_record.NumberParameters; ++i)
            printf("  ExceptionInformation[%2u] = %08lX\n", i,
                   exception_record.ExceptionInformation[i]);
        printf("\nCONTEXT:\n"
               "  Eax    = %08lX  Esp    = %08lX\n"
               "  Ebx    = %08lX  Ebp    = %08lX\n"
               "  Ecx    = %08lX  Esi    = %08lX\n"
               "  Edx    = %08lX  Edi    = %08lX\n"
               "  Eip    = %08lX  EFlags = %08lX\n"
               "  SegCs  = %08lX  SegDs  = %08lX\n"
               "  SegSs  = %08lX  SegEs  = %08lX\n"
               "  SegFs  = %08lX  SegGs  = %08lX\n"
               "  Dr0    = %08lX  Dr3    = %08lX\n"
               "  Dr1    = %08lX  Dr6    = %08lX\n"
               "  Dr2    = %08lX  Dr7    = %08lX\n",
               context.Eax, context.Esp, context.Ebx, context.Ebp,
               context.Ecx, context.Esi, context.Edx, context.Edi,
               context.Eip, context.EFlags, context.SegCs, context.SegDs,
               context.SegSs, context.SegEs, context.SegFs, context.SegGs,
               context.Dr0, context.Dr3, context.Dr1, context.Dr6,
               context.Dr2, context.Dr7);
        fflush(NULL);
        return -2;
    }
}

/* Command dispatcher [00026b40,00026fe0). */
#include "link.h"
extern int _fltused;
int *floating_runtime = &_fltused;
char initial_program_name[] = "CVTRES";
char *executable_path, *program_name = initial_program_name, *tool_name;
int tool_kind, debug_display;
unsigned char debug_requested;
unsigned char abort_link = 1;
char abort_file[256];
char setup_phase[] = "SetupPhase";
char *abort_phase = setup_phase;
Tool tools[] = {
 {"Linker",4,0,1,linker_main}, {"Library Manager",3,0,2,library_main},
 {"Dumper",4,0,3,dumper_main}, {"Editor",4,0,4,editor_main},
 {"Helper",4,0,0,helper_main}, {"",0,0,0,0}
};
int main(int argc, char **argv)
{
    char *debug = NULL, *drive_dummy, drive[4], directory[256], name[256], extension[256];
    char **replacement;
    int i, insert = 1, mapped = 1;
    unsigned short index = 0;
    __try {
        SetConsoleCtrlHandler(control_handler, TRUE);
        executable_path = argv[0];
        for (i = 1; i < argc; ++i) {
            if (!strncmp(argv[i], "-db", 3)) {
                debug = argv[i] + 3;
                memmove(argv+i, argv+i+1, argc * 4 - i - 1);
                --argc; --i;
            }
        }
        debug_initialize(debug, "LINK32_DB");
        if (debug_requested) debug_display = -1;
        if (debug_enabled(27)) mapped = 0;
        files_initialize(30,16,14,8,6,mapped,file_failure);
        _splitpath(argv[0], drive, directory, name, extension);
        program_name = _strupr(name);
        if (argc >= 2 && (argv[1][0] == '/' || argv[1][0] == '-')) {
            for (index = 0; tools[index].length; ++index) {
                if (!_strnicmp(argv[1]+1, tools[index].name, tools[index].length)) {
                    insert = 0; break;
                }
            }
        }
        if (insert) {
            replacement = malloc((argc+1)*4);
            if (!replacement) {
                fprintf(stdout,"%s: malloc() failed.\n",program_name);
                return 1;
            }
            for (index = (unsigned short)argc; index > 1; --index)
                replacement[index] = argv[index-1];
            replacement[0] = argv[0]; replacement[1] = "-link";
            ++argc; argv = replacement;
        }
        if (argv[1][0] == '/' || argv[1][0] == '-') {
            for (index = 0; tools[index].length; ++index)
                if (!_strnicmp(argv[1]+1,tools[index].name,tools[index].length)) break;
        } else index = 5;
        if (!tools[index].length) return driver_usage();
        for (i=0; i<argc; ++i) argv[i]=argv[i+1];
        --argc;
        if (tools[index].kind != 3) {
            if (tools[index].kind == 4) {
                setvbuf(stdout,NULL,0,0); setvbuf(stderr,NULL,0,0);
            } else {
                setvbuf(stdout,NULL,0,2048); setvbuf(stderr,NULL,0,2048);
            }
        }
        tool_name=tools[index].name; tool_kind=tools[index].kind;
        return tools[index].run(argc,argv);
    } __except (1) {
        if (abort_link) print_banner();
        fprintf(stdout,"\n\n***** %s INTERNAL ERROR during %s",tools[index].name,abort_phase);
        if (abort_file[0]) fprintf(stdout," while processing file %s\n",abort_file);
        fprintf(stdout," *****\n\n");
        return -1;
    }
}

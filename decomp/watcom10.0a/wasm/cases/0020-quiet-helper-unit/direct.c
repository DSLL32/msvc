#include <string.h>
#include "direct.h"
#include <recon.h>
int quiet_unit(void)
{
    global_options options;
    module_info module;
    unsigned long line;
    options=Options;module=ModuleInfo;line=LineNumber;
    if (BeQuiet() != 1) return 0;
    return memcmp(&options,&Options,sizeof(options)) == 0 &&
        memcmp(&module,&ModuleInfo,sizeof(module)) == 0 && LineNumber == line;
}

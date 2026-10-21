/* Optional LINK_REPRO response-file capture. */
#include "linker-types.h"
FILE *repro_stream;
char *repro_directory;

void initialize_log(void)
{
    char path[_MAX_PATH], *destination;
    const char *source;
    repro_directory=getenv("LINK_REPRO");
    if (!repro_directory) return;
    if (!_strcmpi(repro_directory,_fullpath(path,".",sizeof(path)))) {
        warning_message(program_name,117,repro_directory,NULL);
        repro_directory=NULL;
        return;
    }
    warning_message(program_name,116,repro_directory,NULL);
    source=repro_directory;
    destination=path;
    while ((*destination++=*source++)!=0) ;
    --destination;
    source="\\link.rsp";
    while ((*destination++=*source++)!=0) ;
    repro_stream=fopen(path,"wt");
    if (!repro_stream) fatal_message(program_name,47,path);
}

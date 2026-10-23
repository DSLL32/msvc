/* Optional LINK_REPRO response-file capture. */
#include "linker-types.h"
#include <recon.h>
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

/* Original [0001fd70,0001fe00). Preserve the original command processor boundary
 * and its ignored return value; native fixtures replay only registered copies. */
void reproduce_input(const char *name,unsigned char input)
{
    char filename[_MAX_FNAME],extension[_MAX_EXT],command[540];
    sprintf(command,"copy %s %s\\.",name,repro_directory);
    system(command);
    if (input) {
        _splitpath(name,NULL,NULL,filename,extension);
        fprintf(repro_stream,".\\%s%s\n",filename,extension);
    }
}

/* Original [0001fe00,0001fe10). The return from fclose is discarded. */
void close_trace_file(void)
{
    fclose(repro_stream);
}

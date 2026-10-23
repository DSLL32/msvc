#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "link.h"
#include <recon.h>

extern char *repro_directory;
FILE *repro_response;
void __cdecl link_warning(const char *,unsigned,...);
void __cdecl link_fatal(const char *,unsigned,...);

/* The original compiler expands this standard string operation inline. */
#pragma intrinsic(strcat)
void __cdecl initialize_repro(void)
{
    char directory[MAX_PATH], current[MAX_PATH];
    char *response_name;
    repro_directory = getenv("LINK_REPRO");
    if (!repro_directory) return;
    _fullpath(directory, repro_directory, MAX_PATH);
    _fullpath(current, ".", MAX_PATH);
    if (!_stricmp(directory, current)) {
        link_warning(NULL,4046,repro_directory);
        repro_directory = NULL;
        return;
    }
    link_warning(NULL,4045,repro_directory);
    response_name = getenv("LINK_REPRO_NAME");
    if (!response_name) response_name = "\\link.rsp";
    strcat(directory,response_name);
    repro_response = fopen(directory,"wt");
    if (!repro_response) link_fatal(NULL,1144,directory);
}

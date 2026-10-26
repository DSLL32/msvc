#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "link.h"
#include "../crt/crt-imports.h"
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

/* Original494444 bit0 guards the single initialization of494448. */
static BYTE repro_name_state,explicit_repro_name;
#pragma intrinsic(strlen,strcpy,strcat)
void __stdcall copy_repro_file(const char *name,BYTE copy_object)
{
    char destination[MAX_PATH],basename[_MAX_FNAME],extension[_MAX_EXT];
    unsigned original_length,number=1;
    BYTE copy=1;
    HANDLE file;
    if(!(repro_name_state&1)) {
        repro_name_state|=1;
        explicit_repro_name=(getenv("LINK_REPRO_NAME")!=NULL);
    }
    _splitpath(name,NULL,NULL,basename,extension);
    original_length=strlen(basename);
    for(;;) {
        strcpy(destination,repro_directory);
        strcat(destination,"\\");
        strcat(destination,basename);
        strcat(destination,extension);
        file=CreateFileA(destination,0,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE) {
            CloseHandle(file);
            break;
        }
        if(explicit_repro_name) {
            copy=0;
            break;
        }
        sprintf(basename+original_length,".%u",++number);
    }
    if(copy) {
        BOOL copied=CopyFileA(name,destination,FALSE);
        printf("Copying: %s%s",name,copied?"\n":" - FAILED\n");
    }
    fflush(stdout);
    if(copy_object)fprintf(repro_response,"\".\\%s%s\"\n",basename,extension);
}

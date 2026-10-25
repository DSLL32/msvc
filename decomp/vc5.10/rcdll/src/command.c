#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <setjmp.h>
#include "rcdll.h"
#include "command.h"
#include "resources.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strlen, strcpy, strcat, strcmp, _setjmp)
#endif

/* Command state shared with resource parsing and the in-process preprocessor. */
jmp_buf rc_abort_context;
int rc_result;
HANDLE rc_heap;
char *rc_include_paths;
char rc_output_path[260];
char rc_input_path[260];
char rc_preprocessed_path[260];
char rc_error_text[2048];
char rc_notice_text[520];
char *rc_input_temporary, *rc_output_temporary;
FILE *rc_output_file, *rc_preprocessed_file;
char *rc_preprocessor_arguments[256];
UINT rc_codepage, rc_saved_codepage;
WORD rc_language;
unsigned rc_default_language=0x409;
int rc_mac, rc_preprocess_only, rc_apstudio, rc_verbose;
int rc_new_style, rc_warnings, rc_special_type_count;
WCHAR rc_special_file[257];
WCHAR *rc_special_types[16];

int __cdecl main(int count, char **arguments)
{
    char *defines[100], *undefines[100];
    char drive[4], directory[256], name[256], extension[256];
    char base_paths[260], full_output[260], codepage_string[12];
    char *include_end, *new_paths, *p, *part, *comma, *slash, **out;
    int include_current=1, include_source=1, define_count=0, undefine_count=0;
    int option, code, argument_count, i;
    unsigned capacity=520, needed;
    DWORD message;

    if (setjmp(rc_abort_context)) return rc_result;
    rc_heap=rc_initialize();
    if (!rc_heap) {
        rc_format(rc_error_text,2048,rc_message(1120),0x01000000);
        rc_fatal(rc_error_text);
    }
    rc_include_paths=rc_allocate(520);
    include_end=rc_include_paths;
    while (count>1) {
        if (!rc_is_option(*arguments[1])) break;
        option=toupper(arguments[1][1]);
        switch (option) {
        case '?': case 'H':
            rc_emit("\n");
            rc_format(rc_error_text,2048,rc_message(10001),"5.00.1472.1",1472);
            rc_emit(rc_error_text);
            rc_emit(rc_message(20001));
            rc_emit("\n");
            return 0;
        case 'B':
            if (toupper(arguments[1][2])!='R') break;
            if (isdigit(arguments[1][3])) arguments[1]+=3;
            else if (arguments[1][3]==':') arguments[1]+=4;
            else if (--count<=1) {
                rc_fatal(rc_message(1209));
                break;
            } else ++arguments;
            if (!*arguments[1]) {
                rc_fatal(rc_message(1209));
                break;
            }
            code=atoi(arguments[1]);
            if (!code || (unsigned)code>32767) rc_fatal(rc_message(1210));
            rc_language=(WORD)code;
            break;
        case 'C':
            if (!arguments[1][2]) { --count; ++arguments; } else arguments[1]+=2;
            if (!arguments[1]) rc_fatal(rc_message(1204));
            rc_codepage=atoi(arguments[1]);
            if (!rc_codepage) rc_fatal(rc_message(1205));
            if (!IsValidCodePage(rc_codepage)) rc_fatal(rc_message(1206));
            break;
        case 'D':
            if (!arguments[1][2]) { --count; ++arguments; } else arguments[1]+=2;
            defines[define_count++]=arguments[1];
            if (define_count>100) {
                rc_format(rc_error_text,2048,rc_message(1105),define_count);
                rc_fatal(rc_error_text);
            }
            break;
        case 'F':
            if (toupper(arguments[1][2])!='O') {
                message=1103;
                goto bad_option;
            }
            if (!arguments[1][3]) { --count; ++arguments; } else arguments[1]+=3;
            if (count<2) rc_fatal(rc_message(1101));
            strcpy(rc_output_path,arguments[1]);
            break;
        case 'I':
            if (!arguments[1][2]) { --count; ++arguments; } else arguments[1]+=2;
            if (!arguments[1]) rc_fatal(rc_message(1201));
            needed=strlen(rc_include_paths)+strlen(arguments[1])+1;
            if (needed>=capacity) {
                capacity=strlen(rc_include_paths)+strlen(arguments[1])+520;
                new_paths=rc_allocate(capacity);
                strcpy(new_paths,rc_include_paths);
                rc_release(rc_include_paths);
                rc_include_paths=new_paths;
                include_end=new_paths+strlen(new_paths)+1;
            }
            if (include_end!=rc_include_paths) include_end[-1]=';';
            do { *include_end++=*arguments[1]; } while (*arguments[1]++);
            break;
        case 'L':
            if (!arguments[1][2]) { --count; ++arguments; } else arguments[1]+=2;
            if (!arguments[1]) rc_fatal(rc_message(1202));
            if (sscanf(arguments[1],"%x",&rc_default_language)!=1) rc_fatal(rc_message(1203));
            while (*arguments[1]++);
            break;
        case 'M': rc_mac=1; goto chained_option;
        case 'N': rc_new_style=1; goto chained_option;
        case 'P': rc_preprocess_only=1; break;
        case 'R': goto chained_option;
        case 'S': rc_apstudio=1; break;
        case 'U':
            if (!arguments[1][2]) { --count; ++arguments; } else arguments[1]+=2;
            undefines[undefine_count++]=arguments[1];
            if (undefine_count>100) {
                rc_format(rc_error_text,2048,rc_message(1104),undefine_count);
                rc_fatal(rc_error_text);
            }
            break;
        case 'V': rc_verbose=1; goto chained_option;
        case 'W': rc_warnings=1; goto chained_option;
        case 'X':
            include_current=0;
            if (arguments[1][2]=='!') { include_source=0; ++arguments[1]; }
chained_option:
            if (!arguments[1][2]) break;
            arguments[1][1]='-';
            ++arguments[1];
            continue;
        case 'Z':
            if (!arguments[1][2]) { --count; ++arguments; } else arguments[1]+=2;
            if (!arguments[1]) rc_fatal(rc_message(1211));
            slash=strchr(arguments[1],'/');
            if (!slash) rc_fatal(rc_message(1212));
            *slash=0;
            MultiByteToWideChar(0,1,slash+1,-1,rc_special_file,257);
            part=arguments[1];
            do {
                comma=strchr(part,',');
                if (comma) *comma=0;
                if (*part) {
                    if (rc_special_type_count>15) rc_fatal(rc_message(1213));
                    rc_special_types[rc_special_type_count]=rc_allocate((strlen(part)+1)*2);
                    MultiByteToWideChar(0,1,part,-1,rc_special_types[rc_special_type_count],257);
                    ++rc_special_type_count;
                }
                part=comma;
                if (comma) { *comma=','; part=comma+1; }
            } while (part);
            *slash='/';
            while (*arguments[1]++);
            break;
        default:
            message=1106;
bad_option:
            rc_format(rc_error_text,2048,rc_message(message),arguments[1]);
            rc_fatal(rc_error_text);
            break;
        }
        --count;
        ++arguments;
    }
    if (count!=2 || !*arguments[1]) rc_fatal(rc_message(1107));
    if (rc_verbose) {
        rc_format(rc_error_text,2048,rc_message(10001),"5.00.1472.1",1472);
        printf(rc_error_text);
        printf("%s\n",rc_message(10002));
    }
    if (!rc_codepage) {
        p=getenv("RCCODEPAGE");
        if (!p) rc_codepage=GetACP();
        else {
            rc_codepage=atoi(p);
            if (!rc_codepage || !IsValidCodePage(rc_codepage)) rc_fatal(rc_message(1207));
        }
    }
    rc_saved_codepage=rc_codepage;
    if (rc_verbose) printf("Using codepage %d as default\n",rc_codepage);
    _splitpath(arguments[1],drive,directory,name,extension);
    if (!*directory && !*drive) strcpy(base_paths,".;");
    else { strcpy(base_paths,drive); strcat(base_paths,directory); strcat(base_paths,";.;"); }
    needed=strlen(base_paths)+strlen(rc_include_paths)+1;
    if (needed>=capacity) {
        capacity=strlen(base_paths)+strlen(rc_include_paths)+520;
        new_paths=rc_allocate(capacity);
        strcpy(new_paths,rc_include_paths);
        rc_release(rc_include_paths);
        rc_include_paths=new_paths;
    }
    new_paths=rc_allocate(capacity);
    if (!include_source) { strcpy(new_paths,rc_include_paths); strcat(new_paths,";"); strcat(new_paths,base_paths); }
    else { strcpy(new_paths,base_paths); strcat(new_paths,rc_include_paths); }
    rc_release(rc_include_paths);
    rc_include_paths=new_paths;
    include_end=new_paths+strlen(new_paths)+1;
    if (!*extension) strcpy(extension,".RC");
    else if (!strcmp(extension,".RES")) rc_fatal(rc_message(1208));
    _makepath(rc_input_path,drive,directory,name,extension);
    if (rc_preprocess_only) _makepath(rc_preprocessed_path,NULL,NULL,name,".rcpp");
    if (!*rc_output_path) _makepath(rc_output_path,drive,directory,name,rc_mac?".RSC":".RES");
    rc_input_temporary=rc_allocate(260);
    _fullpath(full_output,rc_output_path,260);
    _splitpath(full_output,drive,directory,NULL,NULL);
    _makepath(rc_input_temporary,drive,directory,"RCXXXXXX","");
    _mktemp(rc_input_temporary);
    rc_output_temporary=rc_allocate(260);
    _makepath(rc_output_temporary,drive,directory,"RDXXXXXX","");
    _mktemp(rc_output_temporary);
    rc_preprocessor_arguments[0]="RCPP";
    rc_output_file=fopen(rc_output_path,"w+b");
    if (!rc_output_file) {
        rc_format(rc_error_text,2048,rc_message(1109),rc_output_path);
        rc_fatal(rc_error_text);
    } else {
        if (rc_mac) rc_seek(rc_output_file,256,0);
        if (rc_verbose) {
            rc_format(rc_error_text,2048,rc_message(10102),rc_output_path);
            printf(rc_error_text);
        }
        rc_preprocessor_arguments[1]=_strdup("-CP");
        _itoa(rc_codepage,codepage_string,10);
        rc_preprocessor_arguments[2]=codepage_string;
        rc_preprocessor_arguments[3]=_strdup("-f");
        rc_preprocessor_arguments[4]=_strdup(rc_input_temporary);
        rc_preprocessor_arguments[5]=_strdup("-g");
        rc_preprocessor_arguments[6]=_strdup(rc_preprocess_only?rc_preprocessed_path:rc_output_temporary);
        rc_preprocessor_arguments[7]=_strdup("-DRC_INVOKED");
        argument_count=8;
        out=rc_preprocessor_arguments+8;
        if (rc_apstudio) { *out++=_strdup("-DAPSTUDIO_INVOKED"); ++argument_count; }
        if (rc_mac) { *out++=_strdup("-D_MAC"); ++argument_count; }
        *out++=_strdup("-D_WIN32");
        *out++=_strdup("-pc\\:/");
        *out++=_strdup("-E");
        argument_count+=3;
        p=rc_include_paths;
        if (include_current) {
            *out++=_strdup("-I.");
            ++argument_count;
            if (include_end!=rc_include_paths) include_end[-1]=';';
            p=getenv("INCLUDE");
            if (!p) *include_end=0;
            else {
                needed=strlen(p)+strlen(rc_include_paths)+1;
                if (needed>=capacity) {
                    new_paths=rc_allocate(strlen(p)+strlen(rc_include_paths)+520);
                    strcpy(new_paths,rc_include_paths);
                    rc_release(rc_include_paths);
                    rc_include_paths=new_paths;
                }
                strcat(rc_include_paths,p);
            }
            p=rc_include_paths;
        }
        while (*p) {
            part=p;
            while (*p && *p!=';') p=CharNextA(p);
            if (*p) { *p=0; --p; }
            if (*part && *part!='%') {
                *out++=_strdup("-I");
                *out++=_strdup(part);
                argument_count+=2;
            }
            if (*p) { p[1]=';'; p+=2; }
        }
        for (i=0;i<define_count;++i) { *out++=_strdup("-D"); *out++=defines[i]; argument_count+=2; }
        for (i=0;i<undefine_count;++i) { *out++=_strdup("-U"); *out++=undefines[i]; argument_count+=2; }
        if (argument_count>256) rc_fatal(rc_message(1102));
        if (rc_verbose) {
            printf("RC:");
            for (i=0;i<argument_count;++i) { wsprintfA(rc_error_text," %s",rc_preprocessor_arguments[i]); printf(rc_error_text); }
            printf("\n");
        }
        if (!rc_prepare_input(rc_input_path)) rc_fatal(rc_error_text);
        if (rc_preprocess(argument_count,rc_preprocessor_arguments,0)) rc_fatal(rc_message(1116));
    }
    if (rc_preprocess_only) {
        sprintf(rc_notice_text,"Preprocessed File Created in: %s\n",rc_preprocessed_path);
        rc_fatal(rc_notice_text);
    }
    if (rc_verbose) printf("\n%s",rc_input_path);
    rc_preprocessed_file=fopen(rc_output_temporary,"rb");
    if (!rc_preprocessed_file) rc_fatal(rc_message(2180));
    if (!rc_initialize_resource()) rc_fatal(rc_message(22103));
    rc_initialize_input(rc_preprocessed_file);
    rc_codepage=rc_saved_codepage;
    rc_parse_resources();
    if (!rc_flush_resources(rc_output_file)) rc_fatal(rc_message(22204));
    if (!rc_mac) rc_align_output(rc_output_file);
    rc_cleanup();
    HeapDestroy(rc_heap);
    return rc_result;
}

char rc_input_name[260];

HANDLE __stdcall rc_initialize(void)
{
    rc_language=128;
    rc_result=0;
    rc_codepage=0;
    rc_font_resource_count=0;
    rc_input_temporary=NULL;
    rc_output_temporary=NULL;
    rc_hwb_resource_offset=0;
    rc_resource_types=NULL;
    rc_verbose=0;
    rc_mac=0;
    rc_input_name[0]=0;
    rc_output_path[0]=0;
    return HeapCreate(1,0,0);
}

int __stdcall rc_is_option(char value)
{
    return value=='/' || value=='-';
}

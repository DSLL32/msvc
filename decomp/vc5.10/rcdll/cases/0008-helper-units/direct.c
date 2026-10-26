/* Deterministic typed native units. No original invocation or parser controller. */
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "resources.h"
#include "macro-expansion.h"
#include <recon-tool.h>
#include <recon.h>

extern int rc_mac;
extern DWORD rc_last_resource_error_line,rc_last_resource_error_file;
extern unsigned rc_last_resource_error_code;
static char received[8192];
static unsigned received_length;
static int WINAPI collect_message(DWORD first,DWORD second,char *message)
{
    unsigned length=strlen(message);
    if(first||second||length>=sizeof(received)-received_length)return 1;
    memcpy(received+received_length,message,length+1);
    received_length+=length;
    return 0;
}
static int prepare_diagnostics(void)
{
    ReconModuleSpec spec;
    HMODULE module;
    extern FILE *rc_output_file;
    spec.name="RCDLL.dll";
    spec.path="R:\\toolchains\\5.10\\bin\\RCDLL.DLL";
    spec.sha256="bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0";
    spec.flags=RECON_MODULE_NO_TLS;
    module=recon_register_module(&spec);
    if(!module)return 0;
    recon_load_module_resources(module,RECON_RESOURCE_FILE);
    if(!DllMain(module,DLL_PROCESS_ATTACH,NULL))return 0;
    rc_heap=rc_initialize();
    if(!rc_heap)return 0;
    strcpy(rc_output_path,"helper-output.res");
    rc_output_file=fopen(rc_output_path,"w+b");
    if(!rc_output_file)return 0;
    wcscpy(rc_filename_storage,L"unit.rc");
    rc_preparing_line=7;
    rc_prepared_lines=9;
    strcpy(rc_resource_filename,"unit.rc");
    rc_resource_line=11;
    rc_resource_token.line=13;rc_resource_token.file_index=1;
    rc_diagnostic=collect_message;
    received_length=0;received[0]=0;
    return 1;
}
int input_error_unit(void)
{
    if(!prepare_diagnostics())return 0;
    strcpy(rc_error_text,"typed input diagnostic");
    rc_input_error(2101);
    if(rc_result!=1||strcmp(received,
       "unit.rc(7) : error RC2101: typed input diagnostic"))return 0;
    rc_cleanup();
    return HeapDestroy(rc_heap)!=0;
}
int input_fatal_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    strcpy(rc_error_text,"typed fatal diagnostic");
    resumed=setjmp(rc_abort_context);
    if(!resumed){rc_input_fatal(2101);return 0;}
    return resumed==1&&rc_result==1&&
        !strcmp(received,"unit.rc(7) : fatal error RC2101: typed fatal diagnostic");
}

int resource_input_fatal_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    resumed=setjmp(rc_abort_context);
    if(!resumed){rc_resource_input_fatal(2101);return 0;}
    return resumed==1&&rc_result==1&&
        !strcmp(received,"unit.rc (11): error RC2101 : invalid directive in preprocessed RC file\r\n\n\n\n");
}
int resource_data_fatal_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    rc_resource_data_file=fopen("input.dat","w+b");
    if(!rc_resource_data_file)return 0;
    resumed=setjmp(rc_abort_context);
    if(!resumed){rc_resource_data_fatal(2171);return 0;}
    return resumed==1&&rc_result==1&&
        !strcmp(received,"unit.rc (13): error RC2171 : unknown DIB header format\r\n\n\n\n");
}
int resource_name_fatal_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    resumed=setjmp(rc_abort_context);
    if(!resumed){rc_resource_input_fatal_name(2104,L"CAPTION");return 0;}
    return resumed==1&&rc_result==1&&
        !strcmp(received,"unit.rc (11): error RC2104 : undefined keyword or key name: CAPTION\r\n\n\n\n");
}
int resource_file_fatal_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    rc_resource_data_file=fopen("input.dat","w+b");
    if(!rc_resource_data_file)return 0;
    resumed=setjmp(rc_abort_context);
    if(!resumed){rc_resource_file_fatal(2175,L"typed.res");return 0;}
    return resumed==1&&rc_result==1&&
        !strcmp(received,"unit.rc (13): error RC2175 : resource file typed.res is not in 3.00 format\r\n\n\n\n");
}
int resource_error_name_unit(void)
{
    if(!prepare_diagnostics())return 0;
    rc_resource_error_name(2135,"missing.bmp");
    if(rc_result!=1||rc_last_resource_error_line!=13||
       rc_last_resource_error_file!=1||rc_last_resource_error_code!=2135||
       strcmp(received,"\nunit.rc (13): error RC2135 : file not found: missing.bmp\r\n"))return 0;
    rc_cleanup();return HeapDestroy(rc_heap)!=0;
}
int handler_unit(void)
{
    if(!prepare_diagnostics())return 0;
    if(Handler(1)!=FALSE||received_length)return 0;
    if(Handler(0)!=FALSE)return 0;
    return !strcmp(received,"\nRC terminated by user\r\n");
}
/* Valid typed legacy icon records, not a mutated modern ICO file. */
int legacy_icon_unit(void)
{
    struct Shape {short x,y,width,height,row;BYTE planes,bpp;} shape={0,0,32,32,4,1,1};
    BYTE pixels[260],tail[260];WORD tag,expected;FILE *file;
    unsigned mac,extended,kind,i;long size,result;
    if(sizeof(shape)!=12||!prepare_diagnostics())return 0;
    rc_initialize_resource_buffer();rc_reset_resource_buffer();
    if(rc_convert_icon_file(0)!=0)return 0;
    for(i=0;i<sizeof(pixels);++i)pixels[i]=(BYTE)(i^0x5a);
    for(mac=0;mac<2;++mac)for(extended=0;extended<2;++extended)
    for(kind=1;kind<=3;kind+=2){
        file=fopen("legacy.ico","w+b");if(!file)return 0;
        tag=(WORD)kind;size=extended?274:270;
        if(fwrite(&tag,1,2,file)!=2||fwrite(&shape,1,12,file)!=12||
           fwrite(pixels,1,size-14,file)!=(unsigned)(size-14))return 0;
        if(fseek(file,0,SEEK_SET))return 0;
        rc_reset_resource_buffer();rc_resource_file(file);rc_mac=mac;
        result=rc_convert_icon_file(size);
        expected=(WORD)(extended?kind:0x0100|kind);
        if(mac)expected=(WORD)((expected<<8)|(expected>>8));
        if(result!=(extended?6:270)||rc_resource_buffer_used!=(extended?2:14)||
           *(WORD *)rc_resource_buffer!=expected||ftell(file)!=(extended?270:14))return 0;
        if(!extended&&memcmp(rc_resource_buffer+2,&shape,12))return 0;
        if(fread(tail,1,extended?4:256,file)!=(extended?4:256)||
           memcmp(tail,pixels+(extended?256:0),extended?4:256))return 0;
        fclose(file);rc_resource_data_file=NULL;
    }
    rc_cleanup();return HeapDestroy(rc_heap)!=0;
}

int macro_buffer_fatal_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    resumed=setjmp(rc_abort_context);
    if(!resumed){rc_macro_buffer_fatal(10056);return 0;}
    /* 10056 is the actual callers' ID; pristine message blocks omit it. */
    return resumed==1&&rc_result==1&&rc_preparing_line==9&&
        !strcmp(received,"unit.rc(9) : fatal error RC10056: ");
}

int resource_error_repeat_unit(void)
{
    int resumed;
    if(!prepare_diagnostics())return 0;
    resumed=setjmp(rc_abort_context);
    if(!resumed){
        rc_resource_error_name(2135,"missing.bmp");
        if(rc_result!=1)return 0;
        rc_resource_error_name(2135,"missing.bmp");
        return 0;
    }
    return resumed==2&&rc_result==2&&rc_last_resource_error_line==13&&
        rc_last_resource_error_file==1&&rc_last_resource_error_code==2135&&
        !strcmp(received,"\nunit.rc (13): error RC2135 : file not found: missing.bmp\r\n\n\n\n");
}

int input_error_limit_unit(void)
{
    int resumed,index;
    if(!prepare_diagnostics())return 0;
    strcpy(rc_error_text,"typed input diagnostic");
    resumed=setjmp(rc_abort_context);
    if(!resumed){
        /* Exercise the real counter progression, without seeding its value. */
        for(index=0;index<101;++index)rc_input_error(2101);
        return 0;
    }
    return resumed==102&&rc_result==102&&rc_error_template&&
        strstr(received,"unit.rc(7) : fatal error RC1003: error count exceeds 100; stopping compilation\r\n")!=NULL;
}

#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include "preprocess.h"
#include "resources.h"
#include "command.h"
#include <recon-tool.h>
#include <recon-fixture.h>
#include <recon.h>
int option_suffix_unit(void)
{
    return rc_match_option(L"*tail",L"prefix-tail")==1&&
        !rc_match_option(L"*tail",L"prefix-Tail")&&
        !rc_match_option(L"*tail",L"different")&&
        rc_match_option(L"*",L"")==1;
}
static int mac_byte_map_unit(void)
{
    BYTE reference[128],text[258];
    char path[4096];FILE *file;unsigned i;
    snprintf(path,sizeof(path),"%s/mac-byte-map.bin",recon_case_assets());
    file=fopen(path,"rb");if(!file)return 0;
    if(fread(reference,1,sizeof(reference),file)!=sizeof(reference)||
       fgetc(file)!=EOF){fclose(file);return 0;}
    fclose(file);
    for(i=0;i<258;++i)text[i]=0x55;
    for(i=0;i<256;++i)text[i+1]=(BYTE)i;
    rc_select_mac_byte_map(1252);
    rc_map_mac_bytes((char *)text+1,256);
    if(text[0]!=0x55||text[257]!=0x55)return 0;
    for(i=0;i<256;++i)
        if(text[i+1]!=(i<128?(BYTE)i:reference[i-128]))return 0;
    rc_codepage=1251;text[1]=0x80;
    rc_map_mac_bytes((char *)text+1,1);
    if(text[1]!=reference[0])return 0; /* Selection remains sticky. */
    rc_select_mac_byte_map(1251);rc_codepage=1251;
    for(i=0;i<256;++i)text[i+1]=(BYTE)i;
    rc_map_mac_bytes((char *)text+1,256);
    if(rc_mac_byte_map)return 0;
    for(i=0;i<256;++i)if(text[i+1]!=(BYTE)i)return 0;
    rc_codepage=1252;rc_map_mac_bytes((char *)text+1,0);
    if(!rc_mac_byte_map)return 0;
    rc_map_mac_bytes((char *)text+1,-1);
    for(i=0;i<256;++i)if(text[i+1]!=(BYTE)i)return 0;
    return text[0]==0x55&&text[257]==0x55;
}
extern int input_error_unit(void);
extern int input_fatal_unit(void);
extern int resource_input_fatal_unit(void),resource_data_fatal_unit(void);
extern int resource_name_fatal_unit(void),resource_file_fatal_unit(void);
extern int resource_error_name_unit(void),handler_unit(void),legacy_icon_unit(void);
extern int macro_buffer_fatal_unit(void),resource_error_repeat_unit(void);
extern int input_error_limit_unit(void);
const ReconTest helper_tests[]={
 {"option-wildcard-suffix",1,option_suffix_unit},
 {"mac-byte-map-domain",1,mac_byte_map_unit},
 {"input-error-report",1,input_error_unit},
 {"input-fatal-live-context",1,input_fatal_unit},
 {"resource-input-fatal",1,resource_input_fatal_unit},
 {"resource-data-fatal",1,resource_data_fatal_unit},
 {"resource-name-fatal",1,resource_name_fatal_unit},
 {"resource-file-fatal",1,resource_file_fatal_unit},
 {"resource-error-name",1,resource_error_name_unit},
 {"console-handler",1,handler_unit},
 {"legacy-icon-record",1,legacy_icon_unit},
 {"macro-buffer-fatal",1,macro_buffer_fatal_unit},
 {"resource-error-repeat",1,resource_error_repeat_unit},
 {"input-error-limit",1,input_error_limit_unit}
};

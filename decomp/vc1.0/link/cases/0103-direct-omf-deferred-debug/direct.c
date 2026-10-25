/* Genuine converter-owned type map, symbol rows and deferred list lifecycle. */
#include <string.h>
#include <stddef.h>
#include "../../src/omf.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
typedef char DeferredLayout12[sizeof(OmfDeferredDebugType)==12&&offsetof(OmfDeferredDebugType,next)==8?1:-1];
typedef struct GuardedTypes {unsigned long before;unsigned char bytes[259];unsigned long after;} GuardedTypes;
static GuardedTypes input_types;
static unsigned char original_types[259];
static char tool[]="cvtomf",input[]="input.obj";
static char *arguments[]={tool,input,NULL};
static char names[2][8]={"refA","refB"};
static IMAGE_SYMBOL before_symbols[128];
static unsigned long report[128],auxiliary_rows[2],type_indices[2]={2,5};
static OmfDeferredDebugType *nodes[2];
unsigned deferred_phase;
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define converter (*(int (__cdecl *)(int,char **))AT(0x38010))
#define append_symbol (*(unsigned long (__cdecl *)(const char *,unsigned long,short,unsigned char,unsigned short,unsigned char))AT(0x38dc0))
#define append_auxiliary (*(unsigned long (__cdecl *)(const IMAGE_AUX_SYMBOL *))AT(0x38ff0))
#define write_types (*(void (__cdecl *)(unsigned long,const void *,unsigned long))AT(0x3acb0))
#define convert_types (*(void (__cdecl *)(void))AT(0x3a960))
#define register_type (*(void (__cdecl *)(unsigned,unsigned long))AT(0x3ad60))
#define resolve_types (*(void (__cdecl *)(void))AT(0x3ada0))
#define finish_types (*(void (__cdecl *)(void))AT(0x3ad40))
#define serialize_symbols (*(void (__cdecl *)(const char *))AT(0x38480))
#define flush_streams (*(int (__cdecl *)(FILE *))AT(0x40e00))
#define symbol_vector (*(PIMAGE_SYMBOL *)AT(0x70f90))
#define symbol_count (*(unsigned long *)AT(0x4c5cc))
#define type_map (*(OmfDebugType **)AT(0x67ec0))
#define type_count (*(unsigned short *)AT(0x67ed0))
#define pending_types (*(OmfDeferredDebugType **)AT(0x67ee4))
#else
#define converter omf_main
#define append_symbol omf_append_symbol
#define append_auxiliary omf_append_auxiliary
#define write_types omf_write_debug_type_data
#define convert_types omf_convert_debug_types
#define register_type omf_register_debug_fixup
#define resolve_types omf_resolve_debug_types
#define finish_types omf_finish_debug_types
#define serialize_symbols omf_write_coff
#define flush_streams fflush
#define symbol_vector omf_coff_symbols
#define symbol_count omf_coff_symbol_count
#define type_map omf_debug_type_map
#define type_count omf_debug_type_count
#define pending_types omf_deferred_debug_types
#endif
static int read_types(void)
{
    HANDLE file;DWORD count;unsigned char extra;
    input_types.before=0x11223344;input_types.after=0x55667788;
    file=CreateFileA("types.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,input_types.bytes,sizeof(input_types.bytes),&count,NULL)||count!=sizeof(input_types.bytes)||
       !ReadFile(file,&extra,1,&count,NULL)||count||!CloseHandle(file))return 0;
    memcpy(original_types,input_types.bytes,sizeof(original_types));return 1;
}
static int input_unchanged(void)
{
    return input_types.before==0x11223344&&input_types.after==0x55667788&&
        !memcmp(input_types.bytes,original_types,sizeof(original_types));
}
static void floating_state(unsigned long *row)
{
    unsigned short cw,sw;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    row[0]=cw;row[1]=sw;
}
static int check_serialized(unsigned long count)
{
    HANDLE file;DWORD got;IMAGE_FILE_HEADER header;IMAGE_SYMBOL symbols[128];unsigned i;
    file=CreateFileA("deferred.obj",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,&header,sizeof(header),&got,NULL)||got!=sizeof(header)||
       header.Machine!=IMAGE_FILE_MACHINE_I386||header.NumberOfSections||
       header.NumberOfSymbols!=count||header.PointerToSymbolTable!=20||count>128||
       !ReadFile(file,symbols,count*18,&got,NULL)||got!=count*18||!CloseHandle(file))return 0;
    for(i=0;i<2;++i)
        if(((IMAGE_AUX_SYMBOL *)&symbols[auxiliary_rows[i]])->Sym.TagIndex!=report[8+i*8+2])return 0;
    return !memcmp(symbols,before_symbols,count*18);
}
static int write_report(void)
{
    HANDLE file;DWORD count;
    file=CreateFileA("deferred.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
static int workflow(void)
{
    IMAGE_AUX_SYMBOL auxiliary;unsigned i,j,k;unsigned long count,index,tag;
    PIMAGE_SYMBOL vector;OmfDebugType *map;OmfDebugType saved_types[2];
    memset(report,0,sizeof(report));report[0]=0x31464444;report[1]=1;
    if(!read_types()||converter(2,arguments)||pending_types||symbol_count)return 0;
    deferred_phase=0;write_types(0,input_types.bytes,sizeof(input_types.bytes));
    convert_types();
    if(type_count!=17||!type_map||pending_types||!input_unchanged())return 0;
    map=type_map;report[2]=type_count;
    for(i=0;i<2;++i){
        saved_types[i]=map[type_indices[i]];tag=saved_types[i].value.tag;
        if(tag>=symbol_count||memcmp(symbol_vector[tag].N.ShortName,i?"Node":"Value",i?5:6))return 0;
        report[8+i*8]=type_indices[i];report[8+i*8+2]=tag;
        report[8+i*8+3]=saved_types[i].type;report[8+i*8+4]=saved_types[i].size;
        index=append_symbol(names[i],0,-1,3,saved_types[i].type,1);
        memset(&auxiliary,0,sizeof(auxiliary));auxiliary.Sym.Misc.TotalSize=(unsigned short)saved_types[i].size;
        auxiliary_rows[i]=append_auxiliary(&auxiliary);
        if(auxiliary_rows[i]!=index+1)return 0;
        report[8+i*8+1]=auxiliary_rows[i];
    }
    vector=symbol_vector;count=symbol_count;
    if(count>128)return 0;
    memcpy(before_symbols,vector,count*18);
    for(i=0;i<2;++i){
        deferred_phase=1+i;
        /* The actual resolver consumes a row index. The dormant structure
         * caller instead scales by18 twice; that separate defect is not fixed. */
        register_type(type_indices[i],auxiliary_rows[i]);
        nodes[i]=pending_types;
        if(!nodes[i]||nodes[i]->type!=type_indices[i]||nodes[i]->offset!=auxiliary_rows[i]||
           nodes[i]->next!=(i?nodes[0]:NULL)||(i&&nodes[i]==nodes[0]))return 0;
        if(symbol_vector!=vector||symbol_count!=count||type_map!=map||
           memcmp(vector,before_symbols,count*18)||!input_unchanged())return 0;
        for(j=0;j<2;++j)if(memcmp(&map[type_indices[j]],&saved_types[j],sizeof(OmfDebugType)))return 0;
    }
    deferred_phase=3;resolve_types();
    /* Both list nodes are freed by the genuine resolver. Never read them again. */
    if(pending_types||symbol_vector!=vector||symbol_count!=count||type_map!=map)return 0;
    for(i=0;i<count;++i)for(k=0;k<18;++k){
        int changed=0;
        for(j=0;j<2;++j)if(i==auxiliary_rows[j]&&k<4)changed=1;
        if(!changed&&((unsigned char *)vector)[i*18+k]!=((unsigned char *)before_symbols)[i*18+k])return 0;
    }
    for(i=0;i<2;++i){
        if(((IMAGE_AUX_SYMBOL *)&vector[auxiliary_rows[i]])->Sym.TagIndex!=saved_types[i].value.tag||
           memcmp(&map[type_indices[i]],&saved_types[i],sizeof(OmfDebugType)))return 0;
    }
    memcpy(before_symbols,vector,count*18);resolve_types();
    if(pending_types||memcmp(vector,before_symbols,count*18)||!input_unchanged())return 0;
    report[3]=count;floating_state(report+4);
    finish_types(); /* Map is freed; do not inspect its retained dangling global. */
    deferred_phase=4;serialize_symbols("deferred.obj");
    if(symbol_vector||symbol_count||!check_serialized(count)||flush_streams(NULL))return 0;
    return write_report();
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    recon_original_finish(context,workflow());
}
#else
int run_omf_deferred_debug(void){return workflow();}
#endif

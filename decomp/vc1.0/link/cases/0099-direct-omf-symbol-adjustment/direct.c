/* External caller of the converter's real symbol API. No dangling bucket is
 * read after convert_symbol_types releases the nodes. */
#include <string.h>
#include <stddef.h>
#include "../../src/omf.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
typedef char AdjustSymbol24[sizeof(OmfSymbol)==24?1:-1];
typedef char AdjustSymbolFields[offsetof(OmfSymbol,value)==4 && offsetof(OmfSymbol,kind)==8?1:-1];
typedef char AdjustCoff18[sizeof(IMAGE_SYMBOL)==18 && offsetof(IMAGE_SYMBOL,Value)==8?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define converter (*(int (__cdecl *)(int,char **))AT(0x38010))
#define initialize_symbols (*(void (__cdecl *)(void))AT(0x37490))
#define append_symbol (*(unsigned long (__cdecl *)(const char *,unsigned long,short,unsigned char,unsigned short,unsigned char))AT(0x38dc0))
#define insert_symbol (*(void (__cdecl *)(char *,unsigned long,unsigned short,short,unsigned short,unsigned short))AT(0x371f0))
#define lookup_symbol (*(OmfSymbol *(__cdecl *)(const char *))AT(0x37270))
#define adjust_symbol (*(void (__cdecl *)(const char *,unsigned long))AT(0x35fe0))
#define convert_symbols (*(void (__cdecl *)(void))AT(0x37d40))
#define serialize_symbols (*(void (__cdecl *)(const char *))AT(0x38480))
#define flush_streams (*(int (__cdecl *)(FILE *))AT(0x40e00))
#define symbol_vector (*(PIMAGE_SYMBOL *)AT(0x70f90))
#define symbol_count (*(unsigned long *)AT(0x4c5cc))
#define symbol_capacity (*(unsigned long *)AT(0x511e8))
#define symbol_buckets (*(OmfSymbol ***)AT(0x5d218))
#define converter_name (*(char **)AT(0x551f0))
#else
#define converter omf_main
#define initialize_symbols omf_initialize_symbols
#define append_symbol omf_append_symbol
#define insert_symbol omf_insert_symbol
#define lookup_symbol omf_lookup_symbol
#define adjust_symbol omf_adjust_symbol_value
#define convert_symbols omf_convert_symbol_types
#define serialize_symbols omf_write_coff
#define flush_streams fflush
#define symbol_vector omf_coff_symbols
#define symbol_count omf_coff_symbol_count
#define symbol_capacity omf_coff_symbol_capacity
#define symbol_buckets omf_symbol_buckets
#define converter_name omf_program_name
#endif
typedef struct OwnedName {
    unsigned long before;
    char text[16];
    unsigned long after;
} OwnedName;
static OwnedName names[3]={
    {0x11223344,"local_a",0x55667788},
    {0x11223344,"pub_b",0x55667788},
    {0x11223344,"local_c",0x55667788}
};
static char tool[]="cvtomf",input[]="input.obj";
static char *arguments[]={tool,input,NULL};
static const unsigned long initial_values[3]={0x10,0xfffffff0UL,0x80};
static const unsigned long deltas[5]={0x25,0xfffffffeUL,0x30,0,0xffffffffUL};
static const unsigned short selected[5]={0,0,1,2,2};
static const unsigned long expected[5]={0x35,0x33,0x20,0x80,0x7f};
static OmfSymbol *nodes[3];
static OmfSymbol before_nodes[3];
static IMAGE_SYMBOL before_symbols[3];
static OwnedName before_names[3];
static unsigned long report[128];
unsigned long adjustment_phase;
static void floating_state(unsigned long *row)
{
    unsigned short cw,sw;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    row[0]=cw;row[1]=sw;
}
static int write_report(void)
{
    HANDLE file;DWORD count;
    file=CreateFileA("symbols.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
static int check_output(void)
{
    HANDLE file;DWORD count;unsigned char extra;
    IMAGE_FILE_HEADER header;IMAGE_SYMBOL symbols[6];unsigned long string_size;
    file=CreateFileA("adjusted.obj",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,&header,sizeof(header),&count,NULL)||count!=sizeof(header)||
       !ReadFile(file,symbols,sizeof(symbols),&count,NULL)||count!=sizeof(symbols)||
       !ReadFile(file,&string_size,4,&count,NULL)||count!=4||
       !ReadFile(file,&extra,1,&count,NULL)||count||!CloseHandle(file))return 0;
    return header.Machine==IMAGE_FILE_MACHINE_I386 && !header.NumberOfSections &&
        header.TimeDateStamp && header.PointerToSymbolTable==20 && header.NumberOfSymbols==6 &&
        !header.SizeOfOptionalHeader && !header.Characteristics && !string_size &&
        symbols[0].Value==0x33 && symbols[1].Value==0x20 && symbols[2].Value==0x7f &&
        symbols[0].SectionNumber==-1 && symbols[1].SectionNumber==-1 && symbols[2].SectionNumber==-1;
}
static int workflow(void)
{
    unsigned i,j,k;unsigned long index,*row;PIMAGE_SYMBOL original_vector;
    /* A genuine COFF input makes the accepted command entry establish its
     * program-name owner, then close without allocating converter symbols. */
    if(converter(2,arguments)||converter_name!=tool||symbol_count||symbol_vector)return 0;
    initialize_symbols();
    for(i=0;i<211;++i)if(symbol_buckets[i])return 0;
    for(i=0;i<3;++i){
        index=append_symbol(names[i].text,initial_values[i],-1,(unsigned char)(i==1?2:3),0,0);
        if(index!=i)return 0;
        insert_symbol(names[i].text,index,(unsigned short)(i==1?3:2),-1,(unsigned short)(i+1),0);
        nodes[i]=lookup_symbol(names[i].text);
        if(!nodes[i]||nodes[i]->name!=names[i].text||nodes[i]->value!=index||
           nodes[i]->kind!=(i==1?3:2)||nodes[i]->section!=-1||nodes[i]->ordinal!=i+1)return 0;
        before_nodes[i]=*nodes[i];
    }
    if(symbol_count!=3||symbol_capacity!=0x20000)return 0;
    original_vector=symbol_vector;
    memcpy(before_names,names,sizeof(names));
    memset(report,0,sizeof(report));report[0]=0x314a4441;report[1]=2;
    floating_state(report+2);
    for(adjustment_phase=0;adjustment_phase<5;++adjustment_phase){
        i=selected[adjustment_phase];row=report+8+adjustment_phase*12;
        memcpy(before_symbols,symbol_vector,sizeof(before_symbols));
        row[0]=i;row[1]=deltas[adjustment_phase];row[2]=symbol_vector[i].Value;row[3]=nodes[i]->kind;
        adjust_symbol(names[i].text,deltas[adjustment_phase]);
        if(symbol_vector!=original_vector||symbol_count!=3||symbol_vector[i].Value!=expected[adjustment_phase])return 0;
        for(j=0;j<3;++j){
            if(memcmp(nodes[j],&before_nodes[j],sizeof(OmfSymbol)))return 0;
            for(k=0;k<sizeof(IMAGE_SYMBOL);++k)
                if((j!=i||k<8||k>=12)&&((unsigned char *)symbol_vector)[j*18+k]!=((unsigned char *)before_symbols)[j*18+k])return 0;
        }
        if(memcmp(names,before_names,sizeof(names)))return 0;
        row[4]=symbol_vector[i].Value;floating_state(row+5);
    }
    /* This real API consumes and frees every node. The vector remains owned
     * until serialization; neither nodes nor bucket entries are read again. */
    convert_symbols();
    if(symbol_count!=6||symbol_vector!=original_vector)return 0;
    for(i=0;i<3;++i)report[80+i]=symbol_vector[i].Value;
    report[83]=symbol_count;
    serialize_symbols("adjusted.obj");
    if(symbol_vector||symbol_count||!check_output()||flush_streams(NULL))return 0;
    floating_state(report+84);
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
int run_omf_symbol_adjustment(void){return workflow();}
#endif

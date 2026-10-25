/* Direct serialization of genuine converter-owned resource entries. The input
 * payload is copied from the actual produced COFF section, not guessed offsets
 * in the original .res. No parser globals or resource owners are manufactured. */
#include <string.h>
#include <stddef.h>
#include <io.h>
#include "../../src/resources.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
typedef char NamePrefix16[offsetof(ResourceName,text)==16&&offsetof(ResourceName,length)==14?1:-1];
typedef char Entry32[sizeof(ResourceEntry)==32&&offsetof(ResourceEntry,record)==20?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define converter (*(int (__cdecl *)(int,char **))AT(0x32870))
#define serialize (*(int (__cdecl *)(int,int,unsigned long))AT(0x33db0))
#define file_open (*(int (__cdecl *)(const char *,int,...))AT(0x48100))
#define file_close (*(int (__cdecl *)(int))AT(0x455f0))
#define owners (*(ResourceEntry **)AT(0x6fc48))
#define owner_count (*(unsigned short *)AT(0x6fc68))
#define actual_32bit (*(int *)AT(0x4c5c8))
#else
extern ResourceEntry *resource_file_entries;
extern unsigned short resource_count;
extern int resource_32bit;
extern int link_native_serialize_legacy_resources(int,int,unsigned long);
#define converter convert_resources
#define serialize link_native_serialize_legacy_resources
#define file_open _open
#define file_close _close
#define owners resource_file_entries
#define owner_count resource_count
#define actual_32bit resource_32bit
#endif
static char tool[]="cvtres",machine[]="-i386",option[]="-o";
static char input[]="source.res",output[]="resources.obj";
static char *arguments[]={tool,machine,option,output,input,NULL};
static unsigned char coff[2048],legacy[28],payload[8];
static unsigned long report[128];
static ResourceEntry saved_entries[2];
static ResourceRecord saved_records[2];
static unsigned char saved_names[2][22],saved_types[2][22];
ResourceEntry *serialization_owners[2];
int serialization_input,serialization_output;
unsigned serialization_phase;
static int fixture_read_file(const char *name,void *buffer,unsigned long capacity,unsigned long *size)
{
    HANDLE file;DWORD count;unsigned char extra;
    file=CreateFileA(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,buffer,capacity,&count,NULL))return 0;
    *size=count;
    return ReadFile(file,&extra,1,&count,NULL)&&!count&&CloseHandle(file);
}
static int fixture_write_file(const char *name,const void *buffer,unsigned long size)
{
    HANDLE file;DWORD count;
    file=CreateFileA(name,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,buffer,size,&count,NULL)&&count==size&&CloseHandle(file);
}
static void floating_state(unsigned long *row)
{
    unsigned short cw,sw;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    row[0]=cw;row[1]=sw;
}
static int owner_snapshot(void)
{
    unsigned i;ResourceEntry *entry=owners;
    if(owner_count!=2||actual_32bit)return 0;
    for(i=0;i<2;++i){
        if(!entry||!entry->record||!entry->type||!entry->name||
           entry->type->kind!=1||entry->name->kind!=1||
           entry->type->length!=1||entry->name->length!=1||
           entry->type->text[0]!='X'||entry->type->text[1]||
           entry->name->text[0]!=(unsigned short)('Y'+i)||entry->name->text[1]||
           entry->position!=i*4||entry->record->size!=4||entry->record->flags||
           entry->sequence!=i||entry->record->header_size)return 0;
        serialization_owners[i]=entry;saved_entries[i]=*entry;saved_records[i]=*entry->record;
        memcpy(saved_names[i],entry->name,22);memcpy(saved_types[i],entry->type,22);
        report[16+i*8]=entry->position;report[17+i*8]=entry->record->size;
        report[18+i*8]=entry->type->length;report[19+i*8]=entry->name->length;
        report[20+i*8]=entry->name->text[0];report[21+i*8]=entry->sequence;
        entry=entry->file_next;
    }
    return !entry&&serialization_owners[0]!=serialization_owners[1]&&
        serialization_owners[0]->name!=serialization_owners[1]->name&&
        serialization_owners[0]->record!=serialization_owners[1]->record&&
        serialization_owners[0]->type==serialization_owners[1]->type;
}
static int workflow(void)
{
    unsigned long size,offset,length,i;IMAGE_FILE_HEADER *header;IMAGE_SECTION_HEADER *section;
    memset(report,0,sizeof(report));report[0]=0x31534552;report[1]=1;
    if(owners||owner_count||!fixture_write_file("payload.bin",payload,0))return 0;
    serialization_input=file_open("payload.bin",0x8000);
    serialization_output=file_open("empty.res",0x8301,0x180);
    if(serialization_input<0||serialization_output<0)return 0;
    serialization_phase=0;
    report[2]=serialize(serialization_input,serialization_output,0);
    if(report[2]!=1||file_close(serialization_input)||file_close(serialization_output))return 0;
    if(!fixture_read_file("empty.res",legacy,sizeof(legacy),&size)||size)return 0;
    serialization_phase=1;
    if(converter(5,arguments)||!owner_snapshot()||
       !fixture_read_file("resources.obj",coff,sizeof(coff),&size)||size<100)return 0;
    header=(IMAGE_FILE_HEADER *)coff;section=(IMAGE_SECTION_HEADER *)(coff+60);
    if(header->Machine!=0x14c||header->NumberOfSections!=2||
       memcmp(section->Name,".rsrc$02",8)||section->SizeOfRawData!=8)return 0;
    offset=section->PointerToRawData;length=section->SizeOfRawData;
    if(offset>size||length>size-offset)return 0;
    memcpy(payload,coff+offset,length);report[3]=offset;report[4]=size;
    if(memcmp(payload,"\x10\x20\x30\x40\x50\x60\x70\x80",8)||
       !fixture_write_file("payload.bin",payload,length))return 0;
    serialization_input=file_open("payload.bin",0x8000);
    serialization_output=file_open("legacy.res",0x8301,0x180);
    if(serialization_input<0||serialization_output<0)return 0;
    serialization_phase=2;floating_state(report+8);
    report[5]=serialize(serialization_input,serialization_output,0);
    floating_state(report+10);
    if(report[5]!=1||file_close(serialization_input)||file_close(serialization_output))return 0;
    for(i=0;i<2;++i){
        ResourceEntry *entry=serialization_owners[i];
        if(memcmp(entry,&saved_entries[i],32)||memcmp(entry->record,&saved_records[i],24)||
           memcmp(entry->name,saved_names[i],22)||memcmp(entry->type,saved_types[i],22))return 0;
    }
    if(!fixture_read_file("legacy.res",legacy,sizeof(legacy),&size)||size!=28)return 0;
    for(i=0;i<2;++i){
        unsigned char *record=legacy+i*14;
        if(memcmp(record,i?"X\0Z\0":"X\0Y\0",4)||record[4]||record[5]||
           record[6]!=4||record[7]||record[8]||record[9]||memcmp(record+10,payload+i*4,4))return 0;
    }
    report[6]=size;report[7]=1;
    return fixture_write_file("serialization.bin",report,sizeof(report));
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
int run_resource_serialization(void){return workflow();}
#endif

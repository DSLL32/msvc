/* Macintosh CODE-resource IDs and per-section tables. */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "options.h"
#include "link-pass.h"
#include "mac.h"
#include <recon.h>

#pragma intrinsic(memcpy,strlen)

unsigned short mac_code_section_capacity;
extern unsigned char mac_image;

typedef struct MacResourceType {
    unsigned long type;
    unsigned short *ids;
    unsigned short count,next_id;
    struct MacResourceType *next;
} MacResourceType;
MacResourceType *mac_resource_types;
int compare_mac_resource_ids(const void *,const void *);
MacResourceType *find_mac_resource_type(unsigned long,unsigned short);
unsigned short next_mac_resource_id(MacResourceType *);
void sort_mac_resource_ids(void);
unsigned char record_mac_resource_id(MacResourceType *,unsigned short,unsigned char);
short mac_max_resource_id;

MacSectionResource *mac_section_resources;
extern Contribution *mac_resource_map;
unsigned short mac_resource_record_count,mac_data_resource_id;
unsigned short mac_original_code_section_count;
extern unsigned short mac_section_count;
void initialize_mac_code_section(Section *,Image *);

/* Original [00036750,00036866). */
void build_mac_resources(Image *image)
{
    Section *section;
    Group *group;
    Contribution *contribution;
    unsigned long kind;
    mac_original_code_section_count=mac_code_section_count;
    mac_code_section_count=1;
    for (section=image->sections.first;section;section=section->next) {
        if (section->characteristics&0x800) continue;
        if (!section->raw_size) {
            for (group=section->groups;group;group=group->next)
                for (contribution=group->first;contribution;contribution=contribution->next) {
                    if (contribution->flags&0x800) continue;
                    if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
                        continue;
                    if (contribution->raw_size) goto nonempty;
                }
            continue;
        }
nonempty:
        kind=section->characteristics&0x1e0;
        if (kind==0x20) {
            if (!strncmp(section->name,".jtable",8)) continue;
            section->index=mac_code_section_count;
            ++mac_section_count;
            ++mac_code_section_count;
            initialize_mac_code_section(section,image);
        } else if (kind==0x80 || kind==0x40) {
            ++mac_section_count;
        }
    }
}

/* Original [00039d20,00039d3d); allocation contents remain uninitialized. */
void initialize_mac_section_numbers(unsigned short count)
{
    mac_section_resources=malloc(count*sizeof(MacSectionResource));
}

/* Original [00039f10,00039f4d). */
void write_mac_resources(void)
{
    seek_file(output_file,mac_resource_map->file_offset,0);
    write_file(output_file,mac_section_resources,mac_resource_map->group->section->raw_size);
}

/* Original [00039d40,00039f02).  Resource-map records use the target's
 * reversed four-byte type spelling and native-order resource number. */
void record_mac_section_resource(unsigned long mask,Section *section)
{
    MacSectionResource *record=&mac_section_resources[mac_resource_record_count];
    unsigned short id=((unsigned short *)&section->mac_resource_id)[1];
    if (!strncmp(section->name,".jtable",8)) id=0;
    if (mask==0x20) {
        record->type[0]=(unsigned char)(section->mac_type>>24);
        record->type[1]=(unsigned char)(section->mac_type>>16);
        record->type[2]=(unsigned char)(section->mac_type>>8);
        record->type[3]=(unsigned char)section->mac_type;
        record->id=id;
    } else if (mask==0x40 || mask==0x80 || !strncmp(section->name,".dfix",8)) {
        memcpy(record->type,"ATAD",4);
        record->id=mac_data_resource_id;
    } else if (!strncmp(section->name,".mscv",8)) {
        memcpy(record->type,"VCSM",4);
        record->id=0;
    } else if (!strncmp(section->name,".swap",8)) {
        memcpy(record->type,"PAWS",4);
        record->id=0;
    } else {
        record->type[0]=record->type[1]=record->type[2]=record->type[3]=0;
        record->id=0;
    }
    ++mac_resource_record_count;
}

/* Original [00039f50,0003a0e3).  Gather reserved IDs before automatic
 * assignment, preserving the signed WORD comparison used by the target. */
void initialize_mac_resources(void)
{
    Argument *argument=section_overrides.first;
    MacResourceType *type;
    unsigned short index;
    unsigned long resource_type,length;
    char *text,*last,*at;
    for (index=0;index<section_overrides.count;++index,argument=argument->next) {
        type=NULL;
        last=strrchr(argument->text,',');
        if (strchr(argument->text,',')==last || !last) continue;
        text=last+1;
        if (!_strnicmp("RESOURCE=",text,9)) {
            text+=9;
            length=strlen(text);
            at=strchr(text,'@');
            if ((!at && length!=4) || (at && at-text!=4))
                fatal_message(program_name,40,argument->text);
            strncpy((char *)&resource_type,text,4);
            text+=4;
            type=find_mac_resource_type(resource_type,section_overrides.count);
        }
        if (*text=='@') {
            if (!type)
                type=find_mac_resource_type(0x45444f43,section_overrides.count);
            ++text;
        } else if (*text) {
            fatal_message(program_name,40,argument->text);
        }
        if (*text)
            record_mac_resource_id(type,(unsigned short)atoi(text),1);
    }
    sort_mac_resource_ids();
    type=find_mac_resource_type(0x45444f43,section_overrides.count);
    if (type->count) mac_max_resource_id=type->ids[type->count-1];
    mac_max_resource_id=(short)(mac_max_resource_id>(int)mac_code_section_count?
        mac_max_resource_id:mac_code_section_count);
}

/* Original [00022090,000222da).  The low word is the internal CODE index;
 * the high word holds the resource number selected here. */
void set_mac_resource(IMAGE_SECTION_HEADER *header,Section *section,unsigned char apply)
{
    Argument *argument=section_overrides.first;
    unsigned short index;
    unsigned char explicit_id=0;
    char *text,*first,*last;
    unsigned long length;

    for (index=0;index<section_overrides.count;++index,argument=argument->next) {
        first=strchr(argument->text,',');
        if (!first) continue;
        length=first-argument->text;
        text=first+1;
        if (strncmp(argument->text,section->name,length) || section->name[length])
            continue;
        if (*text!=',' && !apply) {
            header->Characteristics&=0x0fffffff;
            while (text && *text && *text!=',') {
                switch (toupper((unsigned char)*text)) {
                case 'E': header->Characteristics|=0x20000000; break;
                case 'R': header->Characteristics|=0x40000000; break;
                case 'S': header->Characteristics|=0x10000000; break;
                case 'W': header->Characteristics|=0x80000000; break;
                default: fatal_message(program_name,40,argument->text); break;
                }
                ++text;
            }
        }
        last=strrchr(argument->text,',');
        first=strchr(argument->text,',');
        if (first==last || !last) continue;
        text=last+1;
        if (!_strnicmp("RESOURCE=",text,9)) {
            if (!(section->characteristics&0x20))
                fatal_message(program_name,4,"Renaming of resource type only allowed for code sections");
            text+=9;
            strncpy((char *)&section->mac_type,text,4);
            if (apply) return;
            text+=4;
        }
        if (*text=='@') {
            ((unsigned short *)&section->mac_resource_id)[1]=(unsigned short)atoi(text+1);
            explicit_id=1;
        } else if (*text) {
            fatal_message(program_name,40,argument->text);
        }
        if (!*text)
            ((unsigned short *)&section->mac_resource_id)[1]=next_mac_resource_id(
                find_mac_resource_type(section->mac_type,section_overrides.count));
    }
    if (mac_image && !explicit_id && !apply)
        ((unsigned short *)&section->mac_resource_id)[1]=next_mac_resource_id(
            find_mac_resource_type(section->mac_type,section_overrides.count));
}

/* Original [00037a40,00037b22).  The doubled WORD capacity also determines
 * the original copy size, including its wraparound behavior. */
void grow_mac_code_sections(void)
{
    MacFixupList *old;
    unsigned long bytes;

    if (!mac_code_section_capacity)
        mac_code_section_capacity=1000;
    else
        mac_code_section_capacity*=2;
    bytes=(mac_code_section_capacity&0xfffe)*12;
    old=mac_section_code_fixups;
    mac_section_code_fixups=calloc(mac_code_section_capacity,sizeof(MacFixupList));
    if (!mac_section_code_fixups)
        fatal_message(NULL,5,"");
    if (old) {
        memcpy(mac_section_code_fixups,old,bytes);
        free(old);
    }
    old=mac_section_data_fixups;
    mac_section_data_fixups=calloc(mac_code_section_capacity,sizeof(MacFixupList));
    if (!mac_section_data_fixups)
        fatal_message(NULL,5,"");
    if (old) {
        memcpy(mac_section_data_fixups,old,bytes);
        free(old);
    }
}

/* Original [00037b30,00037b62). */
void initialize_mac_section(Section *section)
{
    if (mac_code_section_count+1>=mac_code_section_capacity)
        grow_mac_code_sections();
    /* The original updates only the low resource-ID word. */
    *(unsigned short *)&section->mac_resource_id=mac_code_section_count;
    ++mac_code_section_count;
}

/* Original [0003a160,0003a1bb); no allocation failure checks. */
MacResourceType *find_mac_resource_type(unsigned long type,unsigned short capacity)
{
    MacResourceType *entry;
    for (entry=mac_resource_types;entry;entry=entry->next)
        if (entry->type==type) return entry;
    entry=calloc(1,sizeof(*entry));
    entry->ids=calloc(capacity,sizeof(*entry->ids));
    entry->type=type;
    entry->next_id=1;
    entry->next=mac_resource_types;
    mac_resource_types=entry;
    return entry;
}
/* Original [0003a240,0003a27c). */
unsigned short next_mac_resource_id(MacResourceType *type)
{
    while (bsearch(&type->next_id,type->ids,type->count,sizeof(*type->ids),
            compare_mac_resource_ids))
        ++type->next_id;
    return type->next_id++;
}
/* Original [0003a120,0003a153). */
void sort_mac_resource_ids(void)
{
    MacResourceType *type;
    for (type=mac_resource_types;type;type=type->next)
        qsort(type->ids,type->count,sizeof(*type->ids),compare_mac_resource_ids);
}
int compare_mac_resource_ids(const void *left,const void *right) { __asm { int 3 } }
unsigned char record_mac_resource_id(MacResourceType *type,unsigned short id,unsigned char insert) { __asm { int 3 } }

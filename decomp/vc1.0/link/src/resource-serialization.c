/* Legacy resource stream serializer; original [00043db0,00044096). */
#include "resources.h"
#include <recon.h>
extern ResourceEntry *resource_file_entries;
extern unsigned short resource_count;
extern int resource_debug;
extern const char *resource_type_names[18];

int serialize_legacy_resources(int input,int output,unsigned long size)
{
    unsigned char ordinal_marker=0xff;
    unsigned short flags;
    ResourceName *type,*name;
    ResourceRecord *record;
    ResourceEntry *entry,*last=NULL;
    unsigned long data_size,header_size;
    long position,size_position,end;
    long skip;

    if(size) {
        /* Original defect: skip (also printed as Size) and flags are never
         * initialized on this input path. Native host policy rejects this
         * path before entry. Preserve the original Win32 implementation. */
        do {
            resource_read(input,&data_size,4);
            resource_read(input,&header_size,4);
            record=resource_allocate(sizeof(*record));
            record->header_size=header_size;
            record->size=data_size;
            type=read_resource_name(input);
            name=read_resource_name(input);
            resource_seek(input,0,SEEK_CUR);
            resource_read(input,&record->data_version,16);
            position=resource_seek(input,0,SEEK_CUR);
            if((unsigned short)(unsigned long)type!=15) {
                if(resource_debug) {
                    if((unsigned long)type&0xffff0000UL)
                        printf("Adding resource - Type:%s, ",(char *)type->text);
                    else if(type->value.id<=17)
                        printf("Adding resource - Type:%s, ",resource_type_names[(unsigned long)type]);
                    else printf("Adding resource - Type:%ld, ",(unsigned long)type);
                    if((unsigned long)name&0xffff0000UL)
                        printf("Name:%s, ",(char *)name->text);
                    else printf("Name:%ld, ",(unsigned long)name);
                    printf("Flags:%x, Size:%ld\n",flags,skip);
                }
                entry=insert_resource(type,name,record);
                entry->record->flags=flags;
                entry->position=position;
                entry->sequence=resource_count++;
                if(!last)resource_file_entries=entry;
                else last->file_next=entry;
                last=entry;
            }
            position=resource_seek(input,skip,SEEK_CUR);
        } while((unsigned long)position<size);
    }
    for(entry=resource_file_entries;entry;entry=entry->file_next) {
        type=entry->type;name=entry->name;
        resource_seek(input,entry->position,SEEK_SET);
        /* This legacy format tests the pointer high word, not ResourceName.kind,
         * and writes length+1 bytes from the UTF16 text. One ASCII character is
         * represented correctly; longer names and modern ordinal owners retain
         * the original representation mismatch. */
        if((unsigned long)type&0xffff0000UL)
            resource_write(output,type->text,(unsigned long)type->length+1);
        else {
            resource_write(output,&ordinal_marker,1);
            resource_write(output,&type,2);
        }
        if((unsigned long)name&0xffff0000UL)
            resource_write(output,name->text,(unsigned long)name->length+1);
        else {
            resource_write(output,&ordinal_marker,1);
            resource_write(output,&name,2);
        }
        resource_write(output,&entry->record->flags,2);
        size_position=resource_seek(output,0,SEEK_CUR);
        resource_write(output,&entry->record->size,4);
        copy_resource_data(input,output,entry->record->size);
        end=resource_seek(output,0,SEEK_CUR);
        resource_seek(output,size_position,SEEK_SET);
        resource_write(output,&entry->record->size,4);
        resource_seek(output,end,SEEK_SET);
    }
    return 1;
}

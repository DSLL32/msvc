/* Win16 resource templates converted to their Win32 representation. */
#include "resources.h"
#include <recon.h>

extern unsigned char *resource_input_buffer,*resource_output_buffer;
extern char *resource_input_name;
extern int resource_debug,resource_verbose;
int resource_name_to_unicode(unsigned short *,const char *,unsigned long);

typedef struct ResourceConversion {
    const char *name;
    unsigned long (*convert)(const unsigned char *,unsigned long,unsigned char *);
    void (*display_input)(const unsigned char *,unsigned long);
    void (*display_output)(const unsigned char *,unsigned long);
} ResourceConversion;

void resource_align_word(unsigned char **);
void resource_align_dword(unsigned char **);
void resource_copy_bytes(unsigned char **,const unsigned char **,unsigned long);
void resource_copy_unicode(unsigned char **,const unsigned char **,unsigned long);
void resource_copy_identifier(unsigned char **,const unsigned char **);
unsigned long convert_legacy_accelerators(const unsigned char *,unsigned long,unsigned char *);
unsigned long convert_legacy_menu(const unsigned char *,unsigned long,unsigned char *);
unsigned long convert_legacy_dialog(const unsigned char *,unsigned long,unsigned char *);
unsigned long convert_legacy_strings(const unsigned char *,unsigned long,unsigned char *);
void display_legacy_strings(const unsigned char *,unsigned long);
void display_legacy_menu(const unsigned char *,unsigned long);
void display_legacy_dialog(const unsigned char *,unsigned long);

/* Win16 headers are byte-packed; Win32 template fields are WORD-aligned. */
#pragma pack(1)
typedef struct LegacyDialog {
    unsigned long style;
    unsigned char controls;
    short x,y,cx,cy;
} LegacyDialog;
typedef struct LegacyControl {
    short x,y,cx,cy;
    unsigned short id;
    unsigned long style;
} LegacyControl;
typedef struct ResourceDialog {
    unsigned long style,extended_style;
    unsigned short controls;
    short x,y,cx,cy;
} ResourceDialog;
typedef struct ResourceControl {
    unsigned long style,extended_style;
    short x,y,cx,cy;
    unsigned short id;
} ResourceControl;
#pragma pack()
typedef char legacy_dialog_size[(sizeof(LegacyDialog)==13)?1:-1];
typedef char legacy_control_size[(sizeof(LegacyControl)==14)?1:-1];
typedef char resource_dialog_size[(sizeof(ResourceDialog)==18)?1:-1];
typedef char resource_control_size[(sizeof(ResourceControl)==18)?1:-1];

/* Original type dispatch at0007ff98, shared by conversion and diagnostics. */
ResourceConversion legacy_resource_types[16]={
    {NULL,NULL,NULL,NULL},
    {"Cursor",NULL,NULL,NULL},
    {"Bitmap",NULL,NULL,NULL},
    {"Icon",NULL,NULL,NULL},
    {"Menu",convert_legacy_menu,display_legacy_menu,NULL},
    {"Dialog",convert_legacy_dialog,display_legacy_dialog,NULL},
    {"StringTable",convert_legacy_strings,display_legacy_strings,NULL},
    {"FontDirectory",NULL,NULL,NULL},
    {"Font",NULL,NULL,NULL},
    {"Accelerator",convert_legacy_accelerators,NULL,NULL},
    {"RCData",NULL,NULL,NULL},
    {"ErrorTable",NULL,NULL,NULL},
    {"Group-Cursor",NULL,NULL,NULL},
    {"Nothing",NULL,NULL,NULL},
    {"Group-Icon",NULL,NULL,NULL},
    {"Name-Table",NULL,NULL,NULL}
};

/* Original [000440a0,00044270). */
int convert_legacy_resource(int input,int output,ResourceRecord *record,
                           unsigned short type,unsigned short name)
{
    ResourceConversion *format=NULL;
    unsigned long old_size=record->size;
    const unsigned char *data;
    if (type && type<=15) format=&legacy_resource_types[type];
    if (resource_verbose) {
        if (format && format->name) printf("Converting %s.%d\n",format->name,name);
        else printf("Converting %d.%d\n",type,name);
    }
    if (old_size>=0xfff0) {
        if (!format || !format->convert) {
            copy_resource_data(input,output,old_size);
            return old_size;
        }
        fatal_message(resource_input_name,2,0);
    }
    resource_read(input,resource_input_buffer,(unsigned short)old_size);
    if (resource_debug) {
        if (!format) goto unchanged;
        if (format->display_input) format->display_input(resource_input_buffer,old_size);
    }
    if (format && format->convert) {
        record->size=format->convert(resource_input_buffer,old_size,resource_output_buffer);
        if (resource_debug && format->display_output)
            format->display_output(resource_output_buffer,record->size);
        if (record->size>=0xfff0) fatal_message(program_name,3,0);
        data=resource_output_buffer;
    } else {
unchanged:
        record->size=old_size;
        data=resource_input_buffer;
    }
    resource_write(output,data,(unsigned short)record->size);
    if (resource_debug) {
        if (format && format->name) printf("Converted %s.%d",format->name,name);
        else printf("Converted %d.%d",type,name);
        printf("  OldSize: %ld  NewSize: %ld\n\n\n",old_size,record->size);
    }
    return record->size;
}

/* Named frontiers; each is replaced after its original/native entry matches. */
/* Original [00044270,00044290). */
void resource_align_word(unsigned char **cursor)
{
    if ((unsigned long)*cursor&1) *(*cursor)++=0;
}
/* Original [00044290,000442c0). */
void resource_align_dword(unsigned char **cursor)
{
    unsigned short padding=(4-((unsigned long)*cursor&3))&3;
    while (padding--) *(*cursor)++=0;
}
/* Original [000442c0,00044340). A zero count requests a terminated string. */
#pragma intrinsic(strcpy,strlen,memcpy)
void resource_copy_bytes(unsigned char **output,const unsigned char **input,
                         unsigned long count)
{
    if (!count) {
        strcpy((char *)*output,(const char *)*input);
        count=strlen((const char *)*output)+1;
    } else memcpy(*output,*input,count);
    *output+=count;
    *input+=count;
}
#pragma function(strcpy,strlen,memcpy)
/* Original [00044340,000443d0). The fixed-count path advances both cursors
 * by count bytes, although conversion writes WCHARs; retain that native rule. */
#pragma intrinsic(strlen)
void resource_copy_unicode(unsigned char **output,const unsigned char **input,
                           unsigned long count)
{
    if (!count) {
        count=strlen((const char *)*input)+1;
        if (!resource_name_to_unicode((unsigned short *)*output,(const char *)*input,count))
            warning_message(resource_input_name,2,0,0);
        *output+=count*2;
    } else {
        if (!resource_name_to_unicode((unsigned short *)*output,(const char *)*input,count))
            warning_message(resource_input_name,2,0,0);
        *output+=count;
    }
    *input+=count;
}
#pragma function(strlen)
/* Original [00044430,00044490). Ordinals use the Win32 FFFF marker. */
void resource_copy_identifier(unsigned char **output,const unsigned char **input)
{
    if (**input==0xff) {
        (*output)[0]=0xff;
        (*output)[1]=0xff;
        (*output)[2]=(*input)[1];
        (*output)[3]=(*input)[2];
        *output+=4;
        *input+=3;
    } else {
        resource_copy_unicode(output,input,0);
        resource_align_word(output);
    }
}
/* Original [00044490,00044510). Five-byte entries become eight-byte entries. */
unsigned long convert_legacy_accelerators(const unsigned char *input,
                                         unsigned long size,unsigned char *output)
{
    const unsigned char *source=input;
    unsigned char *destination=output;
    while ((unsigned long)(source-input)<size) {
        *(unsigned short *)destination=source[0];
        *(unsigned short *)(destination+2)=*(const unsigned short *)(source+1);
        *(unsigned short *)(destination+4)=*(const unsigned short *)(source+3);
        *(unsigned short *)(destination+6)=0;
        source+=5;
        destination+=8;
    }
    return destination-output;
}
/* Original [00044510,000445b0). Menu header, item flags/IDs and ANSI text. */
unsigned long convert_legacy_menu(const unsigned char *input,unsigned long size,
                                  unsigned char *output)
{
    const unsigned char *source=input;
    unsigned char *destination=output;
    resource_copy_bytes(&destination,&source,4);
    while ((unsigned long)(source-input)<size) {
        resource_copy_bytes(&destination,&source,(*source&0x10)?2:4);
        resource_copy_unicode(&destination,&source,0);
        resource_align_word(&destination);
    }
    return destination-output;
}
/* Original [000445b0,000448b0). Convert the template and each control. */
unsigned long convert_legacy_dialog(const unsigned char *input,unsigned long size,
                                    unsigned char *output)
{
    const LegacyDialog *old_dialog=(const LegacyDialog *)input;
    ResourceDialog *dialog=(ResourceDialog *)output;
    const unsigned char *source=input+sizeof(*old_dialog);
    unsigned char *destination=output+sizeof(*dialog);
    const LegacyControl *old_control;
    ResourceControl *control;
    unsigned char extra;
    dialog->style=old_dialog->style;
    dialog->extended_style=0;
    dialog->controls=old_dialog->controls;
    dialog->x=old_dialog->x;
    dialog->y=old_dialog->y;
    dialog->cx=old_dialog->cx;
    dialog->cy=old_dialog->cy;
    resource_copy_identifier(&destination,&source);
    resource_copy_identifier(&destination,&source);
    resource_copy_unicode(&destination,&source,0);
    resource_align_word(&destination);
    if (old_dialog->style&0x40) {
        resource_copy_bytes(&destination,&source,2);
        resource_copy_unicode(&destination,&source,0);
    }
    resource_align_dword(&destination);
    while ((unsigned long)(source-input)<size) {
        old_control=(const LegacyControl *)source;
        control=(ResourceControl *)destination;
        control->style=old_control->style;
        control->extended_style=0;
        control->x=old_control->x;
        control->y=old_control->y;
        control->cx=old_control->cx;
        control->cy=old_control->cy;
        control->id=old_control->id;
        source+=sizeof(*old_control);
        destination+=sizeof(*control);
        if (*source&0x80) {
            *destination++=0xff;
            *destination++=0xff;
            *destination++=*source++;
        } else resource_copy_unicode(&destination,&source,0);
        resource_align_word(&destination);
        resource_copy_identifier(&destination,&source);
        *destination=*source++;
        extra=*destination++;
        resource_align_dword(&destination);
        if (extra) {
            resource_copy_bytes(&destination,&source,extra);
            resource_align_dword(&destination);
        }
    }
    return destination-output;
}
/* Original [00044930,000449a0). Counted ANSI strings become counted WCHARs. */
unsigned long convert_legacy_strings(const unsigned char *input,unsigned long size,
                                     unsigned char *output)
{
    unsigned char *destination=output;
    unsigned short length;
    while (size) {
        length=*input++;
        --size;
        *(unsigned short *)destination=length;
        destination+=2;
        if (length) {
            if (!resource_name_to_unicode((unsigned short *)destination,
                                           (const char *)input,length))
                warning_message(resource_input_name,2,0,0);
            destination+=length*2;
            input+=length;
            size-=length;
        }
    }
    return destination-output;
}
void display_legacy_strings(const unsigned char *input,unsigned long size) { __asm { int 3 } }
void display_legacy_menu(const unsigned char *input,unsigned long size) { __asm { int 3 } }
void display_legacy_dialog(const unsigned char *input,unsigned long size) { __asm { int 3 } }

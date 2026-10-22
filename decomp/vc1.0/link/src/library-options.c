/* Library-manager switches share the collected argument lists with the linker. */
#include "library.h"
#include <recon.h>
#pragma intrinsic(strlen,strcat)
extern char *output_name,default_output_name[],*def_filename;
extern unsigned char verbose_link,mac_image;

void process_library_options(Image *image)
{
    Argument *argument=switches.first;
    unsigned short index;
    unsigned char created,imports;
    char filename[256],extension[256],library_name[512];
    char *text;
    for (index=0;index<switches.count;++index,argument=argument->next) {
        text=argument->text;
        if (!_strnicmp(text,"verbose",7)) verbose_link=1;
        else if (!_strnicmp(text,"name:",4)) {
            _splitpath(text+5,NULL,NULL,filename,extension);
            if (*filename) image->options.library_image_name=duplicate_string(filename);
            if (*extension) image->options.default_extension=duplicate_string(extension);
        } else if (!_strnicmp(text,"out:",4)) output_name=text+4;
        else if (!_strnicmp(text,"def:",4)) {
            image->options.library_definition=1;
            if (text[4]) {
                def_filename=text+4;
                if (output_name==default_output_name) {
                    _splitpath(def_filename,NULL,NULL,library_name,NULL);
                    strcat(library_name,".lib");
                    output_name=duplicate_string(library_name);
                }
            }
        } else if (!_strnicmp(text,"subsystem:",10)) {
            if (!_strnicmp(text+10,"native",6)) image->optional.Subsystem=1;
            else if (!_strnicmp(text+10,"windows",7)) image->optional.Subsystem=2;
            else if (!_strnicmp(text+10,"console",7)) image->optional.Subsystem=3;
            else if (!_strnicmp(text+10,"posix",5)) image->optional.Subsystem=7;
            else warning_message(program_name,0x52,text+10,"");
        } else if (!_strnicmp(text,"export:",7)) append_argument_node(&exports,text+7,NULL);
        else if (!_strnicmp(text,"include:",8))
            lookup_external(image->symbols,(unsigned short)(strlen(text+8)<=8),text+8,&created,&imports);
        else if (!_strnicmp(text,"machine:",8)) {
            if (!_strnicmp(text+8,"i386",4) || !_strnicmp(text+8,"ix86",4)) image->file.Machine=0x14c;
            else if (!_strnicmp(text+8,"i860",4)) image->file.Machine=0x14d;
            else if (!_strnicmp(text+8,"mips",4)) image->file.Machine=0x162;
            else if (!_strnicmp(text+8,"M68K",4)) {
                image->file.Machine=0x268;
                mac_image=1;
            } else warning_message(program_name,0x53,text+8,"i386, i860 or mips");
        } else if (!_strcmpi(text,"nologo")) banner_pending=0;
        else if (!_strnicmp(text,"debugtype:",10)) {
            if (!_strnicmp(text+10,"coff",4)) image->options.debug_types=9;
            else if (!_strnicmp(text+10,"cv",2)) image->options.debug_types=10;
            else if (!_strnicmp(text+10,"both",4)) image->options.debug_types=11;
            else fatal_message(program_name,0x14,text);
        } else if (!_strnicmp(text,"remove:",7)) remove_members_requested=1;
        else if (!_strnicmp(text,"list:",5)) {
            image->options.library_list=1;
            if (!text[5]) fatal_message(program_name,0x14,text);
            library_listing_name=text+5;
        } else if (!_strnicmp(text,"list",4)) image->options.library_list=1;
        else if (!_strnicmp(text,"extract:",8)) {
            if (!other_inputs.first) fatal_message(program_name,0x22,"");
            extract_member=text+8;
        } else warning_message(program_name,0x73,text,"");
    }
}


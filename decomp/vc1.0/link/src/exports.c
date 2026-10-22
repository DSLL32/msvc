/* Build an import-library command and feed its generated .exp back to pass one. */
#include "link-pass.h"
#include <process.h>
#include <recon.h>
#pragma intrinsic(strlen)
extern char *output_name;

typedef union ExportArgument {
    unsigned long offset;
    char *text;
} ExportArgument;

unsigned char create_import_library(Image *image,const char *definition,const char *tool)
{
    TextBuffer command={0};
    ExportArgument *arguments;
    Argument *argument,generated;
    unsigned short count,index;
    char drive[_MAX_DRIVE],directory[_MAX_DIR],filename[_MAX_FNAME];
    char extension[_MAX_EXT],library_path[_MAX_FNAME],export_path[_MAX_FNAME];
    const char *library;
    if (!exports.count && (!definition || !*definition)) return 0;
    if (find_section_by_name(".edata",&image->sections)) {
        if (definition && *definition) warning_message(program_name,0x7b,definition,"");
        return 0;
    }
    arguments=malloc((unsigned short)(exports.count+input_files.count+11)*sizeof(*arguments));
    if (!arguments) out_of_memory();
    /* Keep offsets while append_text may relocate the command's storage. */
    arguments[0].offset=append_text(&command,tool,strlen(tool)+1);
    arguments[1].offset=append_text(&command,"-lib",5);
    arguments[2].offset=append_text(&command,"-def:",5);
    append_text(&command,definition,strlen(definition)+1);
    _splitpath(output_name,drive,directory,filename,extension);
    arguments[3].offset=append_text(&command,"-out:",5);
    library=import_library_name;
    if (!library) {
        _makepath(library_path,drive,directory,filename,".lib");
        library=library_path;
    }
    append_text(&command,library,strlen(library)+1);
    arguments[4].offset=append_text(&command,"-name:",6);
    append_text(&command,filename,strlen(filename));
    append_text(&command,extension,strlen(extension)+1);
    _splitpath(library,drive,directory,filename,NULL);
    _makepath(export_path,drive,directory,filename,".exp");
    arguments[5].offset=append_text(&command,"-machine:ix86",14);
    arguments[6].offset=append_text(&command,"-nologo",8);
    count=7;
    argument=input_files.first;
    for (index=0;index<input_files.count;++index) {
        arguments[count++].offset=append_text(&command,argument->text,strlen(argument->text)+1);
        argument=argument->next;
    }
    argument=exports.first;
    for (index=0;index<exports.count;++index) {
        if (!argument->original) {
            arguments[count++].offset=append_text(&command,"-export:",8);
            append_text(&command,argument->text,strlen(argument->text)+1);
        }
        argument=argument->next;
    }
    for (index=0;index<count;++index)
        arguments[index].text=command.data+arguments[index].offset;
    arguments[count].text=NULL;
    if (verbose_link) {
        printf("Generating .exp file ...\ncommand:");
        for (index=0;index<count;++index) printf(" %s",arguments[index].text);
        printf("\n");
    }
    if (_spawnvp(_P_WAIT,tool,(const char *const *)arguments))
        fatal_message(program_name,0x2c,"");
    release_text(&command);
    free(arguments);
    generated.text=generated.original=export_path;
    process_input_argument(&generated,image);
    return 1;
}

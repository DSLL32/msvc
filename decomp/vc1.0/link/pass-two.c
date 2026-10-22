/* Emit the already selected object modules in library/module list order. */
#include "pass-two.h"
#include "archive.h"

void pass_two(Image *image)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    Module *module;
    const char *name;
    if (verbose_link) printf("Start Pass2\n");
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while (next_module(&modules)) {
            module=modules.current;
            if (module->library->name) {
                archive_data_offset=module->name.member_offset;
                name=module->library->name;
            } else {
                archive_data_offset=0;
                name=module->name.display_name;
            }
            input_file=open_file(name,0x8000,0);
            process_module_output(image,module);
            close_file(input_file,(unsigned char)(module->library->name==NULL));
        }
    }
    if (verbose_link) printf("End Pass2\n");
}


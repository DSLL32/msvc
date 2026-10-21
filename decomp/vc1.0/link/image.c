/* Initial COFF, PE32, and linker settings. */
#include "image.h"
void create_image(Image **output)
{
    Image *image;
    *output=calloc(1,sizeof(Image));
    if (!*output) out_of_memory();
    image=*output;
    image->file.Machine=IMAGE_FILE_MACHINE_I386;
    image->file.SizeOfOptionalHeader=224;
    image->file.Characteristics=IMAGE_FILE_32BIT_MACHINE;
    image->optional.Magic=0x10b;
    image->optional.MajorLinkerVersion=2;
    image->optional.MinorLinkerVersion=23;
    image->optional.ImageBase=0x10000;
    image->optional.SectionAlignment=0x1000;
    image->optional.FileAlignment=0x200;
    image->optional.MajorOperatingSystemVersion=1;
    image->optional.SizeOfStackReserve=0x100000;
    image->optional.SizeOfStackCommit=0x1000;
    image->optional.SizeOfHeapReserve=0x100000;
    image->optional.SizeOfHeapCommit=0x1000;
    image->optional.NumberOfRvaAndSizes=16;
    image->options.portable_image=1;
    image->options.debug_types=6;
    image->options.default_extension=".dll";
    image->options.optimize=1;
    initialize_symbols(&image->symbols);
    image->libraries.first=NULL;
    image->libraries.tail=&image->libraries.first;
    image->libraries.exclude_defaults=0;
    image->libraries.default_libraries=NULL;
    image->sections.first=NULL;
    image->sections.tail=&image->sections.first;
}

void set_subsystem_version(IMAGE_OPTIONAL_HEADER *header)
{
    switch (header->Subsystem) {
    case IMAGE_SUBSYSTEM_NATIVE:
        header->MajorSubsystemVersion=1;
        header->MinorSubsystemVersion=0;
        break;
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        header->MajorSubsystemVersion=3;
        header->MinorSubsystemVersion=10;
        break;
    case IMAGE_SUBSYSTEM_POSIX_CUI:
        header->MajorSubsystemVersion=19;
        header->MinorSubsystemVersion=90;
        break;
    default:
        header->MajorSubsystemVersion=header->MinorSubsystemVersion=0;
        break;
    }
}

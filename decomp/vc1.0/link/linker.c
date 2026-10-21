/* Command-line driver and post-link CodeView packing. */
#include "image.h"
#include "arguments.h"
int incremental_file;
unsigned long pe_header_offset;
unsigned long symbol_record_size, output_file_size;
char *entry_point;
unsigned char reproducible, small_alignment, reference_option_explicit;
unsigned long debug_options;
/* The default name is also a sentinel tested by pass one. */
char default_output_name[]="coff.out";
char *output_name=default_output_name;
FILE *map_stream=stdout;
extern char *repro_directory;


void link_usage(void);
void initialize_log(void);

int linker_main(int argc,char **argv)
{
    Image *image;
    unsigned long file_alignment,section_alignment;
    unsigned char pack;
    int result;
    if (argc<2) link_usage();
    initialize_log();
    create_image(&image);
    collect_arguments(argc,argv,"LINK");
    process_options(image,0,0,0);
    if (banner_pending) print_banner();
    if ((image->file.Characteristics&IMAGE_FILE_DLL) && !image->options.base_specified)
        image->optional.ImageBase=0x400000;
    symbol_record_size=0x12;
    if (image->optional.Subsystem) {
        if (!image->optional.MajorSubsystemVersion && !image->optional.MinorSubsystemVersion)
            set_subsystem_version(&image->optional);
        if (!entry_point) select_entry_point(image,0);
    } else if (image->file.Characteristics&IMAGE_FILE_DLL) {
        image->optional.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_GUI;
        if (!image->optional.MajorSubsystemVersion && !image->optional.MinorSubsystemVersion)
            set_subsystem_version(&image->optional);
        if (!entry_point) select_entry_point(image,0);
    }
    file_alignment=image->optional.FileAlignment;
    section_alignment=image->optional.SectionAlignment;
    if (file_alignment>section_alignment || section_alignment%file_alignment)
        warning_message(program_name,0x72,section_alignment,file_alignment);
    if (image->optional.SectionAlignment<16) image->optional.SectionAlignment=16;
    if (image->optional.SectionAlignment<0x1000)
        image->optional.FileAlignment=image->optional.SectionAlignment;
    if (reproducible && image->file.SizeOfOptionalHeader==224)
        image->file.SizeOfOptionalHeader=168;
    if (!image->options.rom) small_alignment=image->optional.SectionAlignment!=0x10000;
    image->options.debug_types=debug_options;
    if (!image->options.debug_types) image->options.debug_types=2;
    if (image->options.debug_types&2) image->options.debug_types |= 1;
    if (image->options.debug_types&3) image->options.debug_types |= 8;
    if (!reference_option_explicit)
        image->options.eliminate_unreferenced=image->options.debug_level==0;
    result=link_passes(image,&pack);
    close_all_files();
    free_argument_names();
    if (pack) {
        fflush(stdout);
        result=run_cvpack(&image->options);
        if (result) warning_message(program_name,0x62,"","");
        else if (output_file_size) {
            incremental_file=open_file(output_name,0x8000,0);
            seek_file(incremental_file,pe_header_offset+20,SEEK_SET);
            read_file(incremental_file,&image->optional,image->file.SizeOfOptionalHeader);
            close_file(incremental_file,1);
        }
    }
    if (output_file_size && output_file_size<image->optional.SizeOfImage)
        warning_message(program_name,0x54,image->optional.SizeOfImage,output_file_size);
    if (image->options.map_format==2) write_map(2,image,output_name);
    fclose(map_stream);
    if (repro_directory) close_trace_file();
    return result;
}

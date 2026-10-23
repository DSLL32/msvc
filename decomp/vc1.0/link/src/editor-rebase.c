/* Embedded public REBASE driver. The mapped-image children keep their vertices. */
#include "editor-image.h"
#include "link-pass.h"
#include <recon.h>
#pragma intrinsic(strcpy,memset)
extern int resource_verbose;
extern void __tzset(void);
char *editor_image_name;
int editor_usage_requested,rebase_extract_debug,rebase_show_all;
int rebase_protect_system,rebase_sum_only,rebase_down;
unsigned long rebase_total_size,rebase_initial_base;
FILE *rebase_coffbase_stream;
char rebase_symbol_directory[260];
EditorWorkspace editor_workspace;
RelocationProcessor rebase_relocation_processor;

/* Original [0004fbe0,00050260), including the28-entry switch table. */
int rebase_main(int argc,char **argv,char **environment)
{
    HMODULE module;
    unsigned long base,size,rounded,length;
    char *text,*basename,*end,*dot;
    char fullpath[260];
    long delta;
    int option;
    module=GetModuleHandleA("ntdll");
    if (module)
        rebase_relocation_processor=(RelocationProcessor)GetProcAddress(module,"LdrProcessRelocationBlock");
    if (!rebase_relocation_processor)
        rebase_relocation_processor=fallback_process_relocations;
    editor_usage_requested=0;
    resource_verbose=0;
    rebase_down=0;
    rebase_sum_only=0;
    rebase_protect_system=1;
    rebase_show_all=0;
    editor_image_name=NULL;
    base=rebase_initial_base;
    __tzset();
    if (argc<2) goto usage;
    while (--argc) {
        text=*++argv;
        if (*text=='/' || *text=='-') {
            for (++text;*text;++text) {
                option=toupper(*text);
                switch (option) {
                case '?': editor_usage_requested=1; break;
                case 'B':
                    /* Native post-decrement test, including its missing-value rule. */
                    if (!argc--) goto usage;
                    rebase_initial_base=strtoul(*++argv,&end,16);
                    base=rebase_initial_base;
                    break;
                case 'C':
                    --argc;
                    rebase_coffbase_stream=fopen(*++argv,"wt");
                    if (!rebase_coffbase_stream) {
                        editor_rebase_error("can't open file %s",*argv);
                        ExitProcess(99);
                    }
                    break;
                case 'D': rebase_down=1; break;
                case 'L': rebase_show_all=1; break;
                case 'S':
                    fprintf(stdout,"\n");
                    rebase_sum_only=1;
                    break;
                case 'V': resource_verbose=1; break;
                case 'X':
                    if (!argc--) goto usage;
                    strcpy(rebase_symbol_directory,*++argv);
                    rebase_extract_debug=1;
                    break;
                case 'Z': rebase_protect_system=0; break;
                default:
                    fprintf(stderr,"REBASE: Invalid switch - /%c\n",*text);
                    editor_usage_requested=1;
                }
            }
            if (editor_usage_requested) goto usage;
            continue;
        }
        if (!rebase_initial_base) {
            fprintf(stderr,"REBASE: -b switch must specify a non-zero base\n");
            exit(99);
        }
        editor_image_name=text;
        length=GetFullPathNameA(text,sizeof(fullpath),fullpath,&basename);
        if (!length || length>sizeof(fullpath)) basename=editor_image_name;
        delta=0;
        if (map_rebase_image(editor_image_name,&rebase_workspace,rebase_sum_only!=0)) {
            size=rebase_workspace.headers->OptionalHeader.SizeOfImage;
            rounded=(size+0xffffUL)&0xffff0000UL;
            if (rebase_down) base-=rounded;
            if (rebase_sum_only)
                fprintf(stdout,"REBASE: %16s mapped to 0x%08x (size 0x%08x)\n",basename,base,rounded);
            else if (relocate_rebase_image(&rebase_workspace,base,&delta)) {
                if ((delta || rebase_show_all) && resource_verbose)
                    fprintf(stdout,"REBASE: %16s rebased to 0x%08x (size 0x%08x)\n",basename,base,rounded);
                if (rebase_coffbase_stream) {
                    dot=strstr(basename,".");
                    if (dot) *dot=0;
                    fprintf(rebase_coffbase_stream,"%s 0x%08x 0x%08x\n",basename,base,rounded);
                }
            } else {
                editor_rebase_error("can't rebase; %s invalid or linked with /FIXED",basename);
                exit(99);
            }
            if (!rebase_down) base+=rounded;
            rebase_total_size+=rounded;
            UnmapViewOfFile(rebase_workspace.base);
            if (rebase_workspace.file!=INVALID_HANDLE_VALUE) CloseHandle(rebase_workspace.file);
            memset(&rebase_workspace,0,sizeof(rebase_workspace));
        } else if (!rebase_workspace.system_image)
            editor_rebase_error("can't load file %s",basename);
    }
    fprintf(stdout,"\n");
    fprintf(stdout,"REBASE: Total Size of mapping 0x%08x\n",rebase_total_size);
    if (rebase_down)
        fprintf(stdout,"REBASE: Range 0x%08x - 0x%08x\n",base,base+rebase_total_size);
    else fprintf(stdout,"REBASE: Range 0x%08x - 0x%08x\n",base-rebase_total_size,base);
    exit(0);
    return 0;
usage:
    fprintf(stderr,"usage: REBASE -b InitialBase [switches] image-names... \n");
    fprintf(stderr,"              [-?] display this message\n");
    fprintf(stderr,"              [-c coffbasefilename] generate coffbase.txt\n");
    fprintf(stderr,"              [-v] verbose output\n");
    fprintf(stderr,"              [-z] allow system file rebasing\n");
    fprintf(stderr,"              [-d] top down rebase\n");
    fprintf(stderr,"              [-l] show all image bases, whether they were changed or not.\n");
    fprintf(stderr,"              [-s] just sum image range\n");
    fprintf(stderr,"              [-x symbol directory] extract debug info into separate .DBG file first\n");
    exit(99);
    return 0;
}
#pragma function(strcpy,memset)

/* Original [00050520,00050770), including its SEH filter and handler. */
int map_rebase_image(const char *name,RebaseImage *image,int read_only)
{
    HANDLE file,mapping;
    image->file=INVALID_HANDLE_VALUE;
    image->system_image=0;
    file=CreateFileA(name,read_only?GENERIC_READ:(GENERIC_READ|GENERIC_WRITE),
        FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file==INVALID_HANDLE_VALUE) {
        editor_rebase_error("can't open file %s",name);
        return 0;
    }
    mapping=CreateFileMappingA(file,NULL,read_only?PAGE_READONLY:PAGE_READWRITE,0,0,NULL);
    if (!mapping) {
        editor_rebase_error("can't access file %s",name);
        CloseHandle(file);
        return 0;
    }
    image->base=MapViewOfFile(mapping,read_only?FILE_MAP_READ:FILE_MAP_WRITE,0,0,0);
    CloseHandle(mapping);
    if (!image->base) {
        editor_rebase_error("can't access file %s",name);
        CloseHandle(file);
        return 0;
    }
    __try {
        if (((IMAGE_DOS_HEADER *)image->base)->e_magic!=IMAGE_DOS_SIGNATURE) {
            editor_rebase_error("%s is not an .exe or .dll file",name);
            UnmapViewOfFile(image->base);
            CloseHandle(file);
            return 0;
        }
        image->headers=(PIMAGE_NT_HEADERS)(image->base+((IMAGE_DOS_HEADER *)image->base)->e_lfanew);
        if (image->headers->Signature!=IMAGE_NT_SIGNATURE) {
            editor_rebase_error("%s is not an .exe or .dll file",name);
            CloseHandle(file);
            UnmapViewOfFile(image->base);
            return 0;
        }
        if (!image->headers->FileHeader.SizeOfOptionalHeader) {
            editor_rebase_error("%s is not an .exe or .dll file",name);
            CloseHandle(file);
            UnmapViewOfFile(image->base);
            return 0;
        }
        if (image->headers->OptionalHeader.ImageBase>=0x80000000UL && rebase_protect_system) {
            image->system_image=1;
            CloseHandle(file);
            UnmapViewOfFile(image->base);
            return 0;
        }
        image->section_count=image->headers->FileHeader.NumberOfSections;
        image->sections=(PIMAGE_SECTION_HEADER)(image->headers+1);
        image->current_section=image->sections;
        if (read_only) CloseHandle(file);
        else image->file=file;
        return 1;
    } __except (1) {
        return 0;
    }
}

/* Original [000502f0,00050430). OS relocation processing retains its boundary. */
int relocate_rebase_image(RebaseImage *image,unsigned long base,long *delta)
{
    unsigned long remaining,count,old_checksum,new_checksum;
    IMAGE_BASE_RELOCATION *block;
    unsigned short *words;
    void *page;
    if (base==image->headers->OptionalHeader.ImageBase) return 1;
    block=mapped_image_directory(image->base,0,IMAGE_DIRECTORY_ENTRY_BASERELOC,&remaining);
    if (!block || !remaining) return 0;
    *delta=base-image->headers->OptionalHeader.ImageBase;
    while (remaining) {
        remaining-=block->SizeOfBlock;
        count=(block->SizeOfBlock-8)>>1;
        words=(unsigned short *)(block+1);
        if (!count) continue;
        page=mapped_rebase_rva(block->VirtualAddress,image);
        if (!page) {
            image->headers->Signature=0xffffffffUL;
            return 0;
        }
        block=(IMAGE_BASE_RELOCATION *)rebase_relocation_processor(page,count,words,*delta);
        if (!block) {
            image->headers->Signature=0xffffffffUL;
            return 0;
        }
    }
    image->headers->OptionalHeader.ImageBase=base;
    time((time_t *)&image->headers->FileHeader.TimeDateStamp);
    if (image->file!=INVALID_HANDLE_VALUE) {
        image->headers->OptionalHeader.CheckSum=0;
        checksum_mapped_image(image->base,GetFileSize(image->file,NULL),&old_checksum,&new_checksum);
        image->headers->OptionalHeader.CheckSum=new_checksum;
    }
    FlushViewOfFile(image->base,0);
    return 1;
}

/* Locate one PE directory in an image mapping or a raw file mapping. */
void *WINAPI mapped_image_directory(void *base,unsigned char mapped,
    unsigned short entry,unsigned long *size)
{
    PIMAGE_NT_HEADERS headers=mapped_image_headers(base);
    PIMAGE_SECTION_HEADER section;
    unsigned long rva,i;
    if(entry>=headers->OptionalHeader.NumberOfRvaAndSizes)return NULL;
    rva=headers->OptionalHeader.DataDirectory[entry].VirtualAddress;
    if(!rva)return NULL;
    *size=headers->OptionalHeader.DataDirectory[entry].Size;
    if(mapped || rva<headers->OptionalHeader.SizeOfHeaders)
        return (unsigned char *)base+rva;
    section=(PIMAGE_SECTION_HEADER)((unsigned char *)headers+24+
        headers->FileHeader.SizeOfOptionalHeader);
    for(i=0;i<headers->FileHeader.NumberOfSections;i++,section++)
        if(rva>=section->VirtualAddress &&
           rva<section->VirtualAddress+section->SizeOfRawData)
            return (unsigned char *)base+section->PointerToRawData+
                rva-section->VirtualAddress;
    return NULL;
}

/* An unrecognized DOS signature leaves the supplied pointer unchanged. */
PIMAGE_NT_HEADERS WINAPI mapped_image_headers(void *base)
{
    PIMAGE_NT_HEADERS headers=(PIMAGE_NT_HEADERS)base;
    if(((IMAGE_DOS_HEADER *)base)->e_magic==IMAGE_DOS_SIGNATURE) {
        headers=(PIMAGE_NT_HEADERS)((unsigned char *)base+
            ((IMAGE_DOS_HEADER *)base)->e_lfanew);
        if(headers->Signature!=IMAGE_NT_SIGNATURE)headers=NULL;
    }
    return headers;
}

/* The rebase loader maintains a section cache independent of PE RVA values. */
void *mapped_rebase_rva(unsigned long rva,RebaseImage *image)
{
    PIMAGE_SECTION_HEADER section=image->current_section;
    unsigned long i;
    void *address=NULL;
    if(rva>=section->VirtualAddress &&
       rva<section->VirtualAddress+section->SizeOfRawData)
        address=image->base+section->PointerToRawData+rva-section->VirtualAddress;
    else {
        section=image->sections;
        for(i=0;i<image->section_count;i++,section++)
            if(rva>=section->VirtualAddress &&
               rva<section->VirtualAddress+section->SizeOfRawData) {
                address=image->base+section->PointerToRawData+rva-section->VirtualAddress;
                image->current_section=section;
                break;
            }
    }
    if(!address)editor_rebase_error("invalid file -- bad address %lx",(void *)rva);
    return address;
}

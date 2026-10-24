/* Public BIND driver and its mapped-image cache. */
#include "editor-image.h"
#include <recon.h>
#pragma intrinsic(strlen,strcpy,memset)
extern int resource_verbose;
BoundImageList bind_images;
int bind_read_only,bind_show_imports;
char *bind_search_path;

/* Public BIND diagnostic wrapper [00028750,00028791). */
void editor_bind_error(const char *format,const void *value)
{
    char message[340];
    _snprintf(message,sizeof(message),format,value);
    fatal_message(program_name,0x35,message);
}

/* Original4ed60..4f070; this embedded driver terminates the process. */
int bind_main(int argc,char **argv,char **environment)
{
    char *text;
    unsigned long previous,checksum;
    PIMAGE_NT_HEADERS headers;
    editor_usage_requested=0;
    resource_verbose=0;
    bind_read_only=1;
    bind_show_imports=0;
    bind_search_path=NULL;
    editor_image_name=NULL;
    bind_images.forward=bind_images.back=(BoundImage *)&bind_images;
    while(--argc) {
        text=*++argv;
        if(*text=='/' || *text=='-') {
            for(++text;*text;++text) {
                switch(toupper(*text)) {
                case '?':editor_usage_requested=1;break;
                case 'I':bind_show_imports=1;break;
                case 'P':--argc;bind_search_path=*++argv;break;
                case 'U':bind_read_only=0;break;
                case 'V':resource_verbose=1;break;
                default:
                    editor_bind_error("BIND: Invalid switch - /%c\n",(void *)(long)*text);
                    editor_usage_requested=1;
                }
            }
            if(editor_usage_requested) {
                fprintf(stderr,"usage: BIND [switches] image-names... \n");
                fprintf(stderr,"            [-?] display this message\n");
                fprintf(stderr,"            [-v] verbose output\n");
                fprintf(stderr,"            [-i] display imports\n");
                fprintf(stderr,"            [-u] update the image\n");
                fprintf(stderr,"            [-p dll search path]\n");
                ExitProcess(99);
            }
            continue;
        }
        editor_image_name=text;
        if(resource_verbose)
            fprintf(stdout,"BIND: binding %s using DllPath %s\n",text,
                bind_search_path?bind_search_path:"Default");
        if(!map_bind_image(text,&bind_workspace,0,bind_read_only!=0)) {
            editor_bind_error("failure mapping and loading %s\n",editor_image_name);
            continue;
        }
        bind_workspace.name=LocalAlloc(LMEM_ZEROINIT,strlen(editor_image_name)+1);
        strcpy(bind_workspace.name,editor_image_name);
        bind_imports(&bind_workspace);
        if(!bind_read_only && bind_workspace.file!=INVALID_HANDLE_VALUE) {
            headers=mapped_image_headers(bind_workspace.base);
            headers->OptionalHeader.CheckSum=0;
            checksum_mapped_image(bind_workspace.base,GetFileSize(bind_workspace.file,NULL),
                &previous,&checksum);
            headers->OptionalHeader.CheckSum=checksum;
        }
        UnmapViewOfFile(bind_workspace.base);
        if(bind_workspace.file!=INVALID_HANDLE_VALUE)CloseHandle(bind_workspace.file);
        LocalFree(bind_workspace.name);
        memset(&bind_workspace,0,sizeof(bind_workspace));
    }
    ExitProcess(0);
    return 0;
}

/* Original4f880..4faf0. A failed direct open gets one DLL/EXE path search. */
int map_bind_image(const char *name,BoundImage *image,int dll,int read_only)
{
    const char *filename=name;
    char path[260],*part;
    unsigned long searched=0;
    HANDLE file,mapping;
    image->file=INVALID_HANDLE_VALUE;
    for(;;) {
        file=CreateFileA(filename,read_only?GENERIC_READ:GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        if(file!=INVALID_HANDLE_VALUE)break;
        if(searched) {
            editor_bind_error("open %s failed",filename);
            return 0;
        }
        searched=SearchPathA(bind_search_path,name,dll?".dll":".exe",sizeof(path),path,&part);
        if(!searched || searched>=sizeof(path)) {
            editor_bind_error("open %s failed",filename);
            return 0;
        }
        filename=path;
    }
    mapping=CreateFileMappingA(file,NULL,read_only?PAGE_READONLY:PAGE_READWRITE,0,0,NULL);
    if(!mapping) {
        editor_bind_error("create map of %s failed",filename);
        CloseHandle(file);
        return 0;
    }
    image->base=MapViewOfFile(mapping,read_only?FILE_MAP_READ:FILE_MAP_WRITE,0,0,0);
    CloseHandle(mapping);
    if(!image->base) {
        editor_bind_error("map of %s failed",filename);
        CloseHandle(file);
        return 0;
    }
    if(((IMAGE_DOS_HEADER *)image->base)->e_magic!=IMAGE_DOS_SIGNATURE) {
        editor_bind_error("%s is not an .exe or .dll file",filename);
        UnmapViewOfFile(image->base);
        CloseHandle(file);
        return 0;
    }
    if(resource_verbose)fprintf(stdout,"BIND: %s mapped at 0x%lx\n",filename,image->base);
    image->headers=(PIMAGE_NT_HEADERS)(image->base+((IMAGE_DOS_HEADER *)image->base)->e_lfanew);
    if(image->headers->Signature!=IMAGE_NT_SIGNATURE) {
        editor_bind_error("%s is not an .exe or .dll file",filename);
        CloseHandle(file);
        UnmapViewOfFile(image->base);
        return 0;
    }
    if(image->headers->OptionalHeader.MajorLinkerVersion<2 ||
       image->headers->OptionalHeader.MinorLinkerVersion<5) {
        editor_bind_error("bad linker version number",NULL);
        CloseHandle(file);
        UnmapViewOfFile(image->base);
        return 0;
    }
    image->section_count=image->headers->FileHeader.NumberOfSections;
    image->current_section=image->sections=(PIMAGE_SECTION_HEADER)(image->headers+1);
    if(read_only)CloseHandle(file);
    else image->file=file;
    return 1;
}

/* Original4f540..4f880, including its resolver SEH filter/handler. */
void bind_imports(BoundImage *image)
{
    IMAGE_IMPORT_DESCRIPTOR *descriptor;
    IMAGE_EXPORT_DIRECTORY *exports;
    BoundImage *dll;
    char *name;
    unsigned long directory_size,count,bytes,index,first_forwarder,*tail;
    unsigned long *lookup,*bound,*output,*thunks;
    int valid;
    SYSTEMTIME system_time;
    FILETIME file_time;
    descriptor=mapped_image_directory(image->base,0,IMAGE_DIRECTORY_ENTRY_IMPORT,&directory_size);
    if(!descriptor)return;
    for(;descriptor->Name;descriptor++) {
        name=mapped_bound_rva(descriptor->Name,image);
        if(!name)continue;
        if(resource_verbose)fprintf(stdout,"BIND: Import %s\n",name);
        dll=load_bind_image(name);
        if(!dll)continue;
        exports=mapped_image_directory(dll->base,0,IMAGE_DIRECTORY_ENTRY_EXPORT,&directory_size);
        if(!exports) {
            editor_bind_error("Unable to locate exports in %s",name);
            continue;
        }
        if(!mapped_bound_rva(exports->AddressOfNames,dll)) {
            editor_bind_error("Unable to locate address of names in %s",name);
            continue;
        }
        if(!mapped_bound_rva(exports->AddressOfNameOrdinals,dll)) {
            editor_bind_error("Unable to locate address of name ordinals in %s",name);
            continue;
        }
        if(!mapped_bound_rva(exports->AddressOfFunctions,dll)) {
            editor_bind_error("Unable to locate address of name ordinals in %s",name);
            continue;
        }
        if(descriptor->TimeDateStamp &&
           descriptor->TimeDateStamp==dll->headers->FileHeader.TimeDateStamp && !bind_show_imports)continue;
        lookup=mapped_bound_rva(descriptor->Characteristics,image);
        if(!lookup)continue;
        for(count=0;lookup[count];count++);
        bytes=count*4;
        bound=LocalAlloc(LMEM_ZEROINIT,bytes);
        if(!bound) {
            editor_bind_error("out of memory",NULL);
            continue;
        }
        valid=1;
        tail=&first_forwarder;
        output=bound;
        for(index=0;index<count;index++,lookup++,output++) {
            __try {
                valid=resolve_bound_import(lookup,image,bound,output,dll,exports,&tail);
            } __except(EXCEPTION_EXECUTE_HANDLER) { valid=0; }
            if(!valid)break;
        }
        thunks=mapped_bound_rva(descriptor->FirstThunk,image);
        if(!thunks) {
            editor_bind_error("Unable to locate thunks in image",NULL);
            valid=0;
        }
        if(valid && !bind_read_only) {
            *tail=0xffffffffUL;
            descriptor->ForwarderChain=first_forwarder;
            memmove(thunks,bound,bytes);
            descriptor->TimeDateStamp=dll->headers->FileHeader.TimeDateStamp;
            /* Original passes the module workspace, not image->base. */
            FlushViewOfFile(image,0);
            GetSystemTime(&system_time);
            if(SystemTimeToFileTime(&system_time,&file_time))
                SetFileTime(image->file,NULL,NULL,&file_time);
        }
        LocalFree(bound);
    }
}

/* Original4f460..4f4f0: distinct from the REBASE image layout/cache. */
void *mapped_bound_rva(unsigned long rva,BoundImage *image)
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
    if(!address)editor_bind_error("invalid file: bad address %lx",(void *)rva);
    return address;
}

/* Original4f370..4f460. Cache nodes retain ownership until process exit. */
BoundImage *load_bind_image(const char *name)
{
    BoundImage *image,*head=(BoundImage *)&bind_images;
    for(image=bind_images.forward;image!=head;image=image->forward)
        if(!_strcmpi(name,image->name))return image;
    image=LocalAlloc(LMEM_ZEROINIT,sizeof(*image));
    if(!image) {
        editor_bind_error("bad file format or out of memory",NULL);
        return NULL;
    }
    image->name=LocalAlloc(LMEM_ZEROINIT,strlen(name)+1);
    if(!image->name) {
        editor_bind_error("bad file format or out of memory",NULL);
        return NULL;
    }
    strcpy(image->name,name);
    if(!map_bind_image(name,image,1,1))return NULL;
    image->forward=head;
    image->back=bind_images.back;
    bind_images.back->forward=image;
    bind_images.back=image;
    return image;
}

/* Original4f070..4f370. Resolve a name/hint or ordinal, and link forwarders. */
int resolve_bound_import(const unsigned long *lookup,BoundImage *image,
    unsigned long *first,unsigned long *output,BoundImage *dll,
    IMAGE_EXPORT_DIRECTORY *exports,unsigned long **tail)
{
    typedef struct ImportName { unsigned short hint; char name[1]; } ImportName;
    struct { unsigned short unconsumed; char name[30]; } ordinal_name;
    ImportName *name;
    unsigned long *names,*functions,exports_size,exports_base;
    unsigned short *ordinals,index,hint;
    unsigned char *left,*right;
    void *directory;
    names=mapped_bound_rva(exports->AddressOfNames,dll);
    ordinals=mapped_bound_rva(exports->AddressOfNameOrdinals,dll);
    functions=mapped_bound_rva(exports->AddressOfFunctions,dll);
    if(*lookup&0x80000000UL) {
        index=(unsigned short)(*lookup-exports->Base);
        if(index>=exports->NumberOfFunctions)return 0;
        sprintf(ordinal_name.name,"Ordinal%x",index);
        name=(ImportName *)&ordinal_name;
        /* The original verbose hint local is not assigned on this branch. */
    } else {
        name=mapped_bound_rva(*lookup,image);
        if(!name) {
            editor_bind_error("invalid file (unable to locate ImportName)",NULL);
            return 0;
        }
        if(bind_show_imports)
            fprintf(stdout,"%s refers to %s . %s\n",image->name,dll->name,name->name);
        index=(unsigned short)(exports->NumberOfFunctions+1);
        hint=name->hint;
        if(hint<exports->NumberOfNames) {
            right=mapped_bound_rva(names[hint],dll);
            if(right) {
                left=(unsigned char *)name->name;
                while(*left && *left==*right) { ++left; ++right; }
                if(*left==*right)index=ordinals[hint];
            }
        }
        if(index>=exports->NumberOfFunctions) {
            for(hint=0;hint<exports->NumberOfNames;hint++) {
                right=mapped_bound_rva(names[hint],dll);
                if(!right)continue;
                left=(unsigned char *)name->name;
                while(*left && *left==*right) { ++left; ++right; }
                if(*left==*right) { index=ordinals[hint]; break; }
            }
            if(index>=exports->NumberOfFunctions)return 0;
        }
    }
    *output=functions[index]+dll->headers->OptionalHeader.ImageBase;
    directory=mapped_image_directory(dll->base,1,IMAGE_DIRECTORY_ENTRY_EXPORT,&exports_size);
    exports_base=(unsigned long)directory-(unsigned long)dll->base+
        dll->headers->OptionalHeader.ImageBase;
    if(resource_verbose)
        fprintf(stdout,"BIND: %s @ %08x MappedBase: %08x  Exports: %08x  ImageBase: %08x  ExportsBase: %08x  Size: %04x\n",
            name->name,*output,dll->base,exports,dll->headers->OptionalHeader.ImageBase,exports_base,exports_size);
    if(exports_base<*output && *output<exports_base+exports_size) {
        **tail=(unsigned long)(output-first);
        *tail=output;
        if(resource_verbose)
            fprintf(stdout,"BIND: Hint %lx Forwarder %s not snapped [%lx]\n",(unsigned long)hint,name->name,*output);
    } else if(resource_verbose)
        fprintf(stdout,"BIND: Hint %lx Name %s Bound to %lx\n",(unsigned long)hint,name->name,*output);
    return 1;
}

/* Original [0004f4f0,0004f537). Change only the first import section's
 * read/write bits; other section characteristics remain as supplied. */
void make_import_section_read_only(BoundImage *image)
{
    PIMAGE_SECTION_HEADER section;
    unsigned long index;
    section=image->sections;
    for (index=0;index<image->section_count;++index,++section) {
        if (!_strcmpi((const char *)section->Name,".idata")) {
            section->Characteristics &= 0x3fffffffUL;
            section->Characteristics |= IMAGE_SCN_MEM_READ;
            break;
        }
    }
}

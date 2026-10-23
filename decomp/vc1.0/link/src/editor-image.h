#ifndef LINK_EDITOR_IMAGE_H
#define LINK_EDITOR_IMAGE_H
#include "editor.h"
#include <stddef.h>
#include <recon.h>

/* Original rebase workspace at6d470, with cached section lookup state. */
typedef struct RebaseImage {
    unsigned long unconsumed;
    unsigned char system_image;
    HANDLE file;
    unsigned char *base;
    PIMAGE_NT_HEADERS headers;
    PIMAGE_SECTION_HEADER current_section;
    unsigned long section_count;
    PIMAGE_SECTION_HEADER sections;
} RebaseImage;
typedef char RebaseImageLayout[(sizeof(RebaseImage)==32 &&
    offsetof(RebaseImage,file)==8 && offsetof(RebaseImage,base)==12 &&
    offsetof(RebaseImage,headers)==16 && offsetof(RebaseImage,current_section)==20 && offsetof(RebaseImage,sections)==28)?1:-1];

typedef unsigned short *(WINAPI *RelocationProcessor)(void *,unsigned long,unsigned short *,long);
typedef struct BoundImage {
    struct BoundImage *forward,*back;
    char *name;
    HANDLE file;
    unsigned char *base;
    PIMAGE_NT_HEADERS headers;
    PIMAGE_SECTION_HEADER current_section;
    unsigned long section_count;
    PIMAGE_SECTION_HEADER sections;
} BoundImage;
typedef union EditorWorkspace {
    RebaseImage rebase;
    BoundImage bound;
} EditorWorkspace;
typedef char BoundImageLayout[(sizeof(BoundImage)==36 &&
    offsetof(BoundImage,name)==8 && offsetof(BoundImage,file)==12 &&
    offsetof(BoundImage,base)==16 && offsetof(BoundImage,headers)==20 &&
    offsetof(BoundImage,current_section)==24 && offsetof(BoundImage,section_count)==28 &&
    offsetof(BoundImage,sections)==32 && sizeof(EditorWorkspace)==36)?1:-1];
extern EditorWorkspace editor_workspace;
#define rebase_workspace editor_workspace.rebase
#define bind_workspace editor_workspace.bound
typedef struct BoundImageList {
    BoundImage *forward,*back;
} BoundImageList;
typedef char BoundImageListLayout[(sizeof(BoundImageList)==8)?1:-1];
extern BoundImageList bind_images;
extern int bind_read_only,bind_show_imports;
extern char *bind_search_path;
int map_bind_image(const char *,BoundImage *,int,int);
void bind_imports(BoundImage *);
void *mapped_bound_rva(unsigned long,BoundImage *);
BoundImage *load_bind_image(const char *);
int resolve_bound_import(const unsigned long *,BoundImage *,unsigned long *,unsigned long *,
    BoundImage *,IMAGE_EXPORT_DIRECTORY *,unsigned long **);
void editor_bind_error(const char *,const void *);
extern RelocationProcessor rebase_relocation_processor;
extern char *editor_image_name;
extern int editor_usage_requested,rebase_extract_debug,rebase_show_all;
extern int rebase_protect_system,rebase_sum_only,rebase_down;
extern unsigned long rebase_total_size,rebase_initial_base;
extern FILE *rebase_coffbase_stream;
extern char rebase_symbol_directory[260];
int map_rebase_image(const char *,RebaseImage *,int);
int relocate_rebase_image(RebaseImage *,unsigned long,long *);
unsigned short *WINAPI fallback_process_relocations(void *,unsigned long,unsigned short *,long);
void editor_rebase_error(const char *,const void *);
PIMAGE_NT_HEADERS WINAPI mapped_image_headers(void *);
void *WINAPI mapped_image_directory(void *,unsigned char,unsigned short,unsigned long *);
void *mapped_rebase_rva(unsigned long,RebaseImage *);
PIMAGE_NT_HEADERS checksum_mapped_image(void *,unsigned long,unsigned long *,unsigned long *);
#endif

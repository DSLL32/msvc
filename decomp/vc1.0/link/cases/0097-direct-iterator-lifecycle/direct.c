/* Real constructors install all callbacks. Caller-owned iterator padding starts
 * defined; each end call must preserve every byte and all live owner storage. */
#include <string.h>
#include "../../src/image.h"
#include "../../src/comdat.h"
#include "../../src/symbols.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define make_image (*(void (__cdecl *)(Image **))AT(0x7000))
#define init_modules (*(void (__cdecl *)(void (__cdecl *)(char *,int,char *),Module **))AT(0x2ae60))
#define init_graph (*(void (__cdecl *)(void (__cdecl *)(char *,int,char *)))AT(0x70f0))
#define error_callback (*(void (__cdecl *)(char *,int,char *))AT(0x16b10))
#define make_library (*(Library *(__cdecl *)(const char *,unsigned long,unsigned char,LibraryList *))AT(0x2b340))
#define make_module (*(Module *(__cdecl *)(const char *,const char *,unsigned long,unsigned short,unsigned long,unsigned long,unsigned short,unsigned short,unsigned short,Library *,unsigned char *))AT(0x2b230))
#define make_contribution (*(Contribution *(__cdecl *)(const char *,unsigned long,unsigned short,unsigned short,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long,Module *,unsigned char,SectionList *,IMAGE_OPTIONAL_HEADER *))AT(0x2b0b0))
#define make_graph (*(ComdatModule *(__cdecl *)(Module *))AT(0x7160))
#define make_node (*(ComdatNode *(__cdecl *)(Contribution *,ComdatModule *,const char *,ComdatNode **,unsigned char))AT(0x71d0))
#define make_edge (*(ComdatEdge *(__cdecl *)(const char *,ComdatNode *,Contribution *,unsigned char))AT(0x72a0))
#define make_root (*(ComdatRoot *(__cdecl *)(const char *,External *,Contribution *,ComdatRoot **))AT(0x73a0))
#define init_nodes (*(void (__cdecl *)(ComdatNodeEnumerator *,ComdatNode *))AT(0x7920))
#define init_edges (*(void (__cdecl *)(ComdatEdgeEnumerator *,ComdatNode *))AT(0x7980))
#define init_roots (*(void (__cdecl *)(ComdatRootEnumerator *,ComdatRoot *))AT(0x7a00))
#define init_undefined (*(void (__cdecl *)(UndefinedEnumerator *,SymbolTable *))AT(0x15fe0))
#define init_contributions (*(void (__cdecl *)(ContributionEnumerator *,Module *))AT(0x2bf30))
#define end_nodes (*(void (__cdecl *)(ComdatNodeEnumerator *))AT(0x7970))
#define end_edges (*(void (__cdecl *)(ComdatEdgeEnumerator *))AT(0x79f0))
#define end_roots (*(void (__cdecl *)(ComdatRootEnumerator *))AT(0x7a50))
#define end_undefined (*(void (__cdecl *)(UndefinedEnumerator *))AT(0x16020))
#define end_contributions (*(void (__cdecl *)(ContributionEnumerator *))AT(0x2bfb0))
#define release_graph (*(void (__cdecl *)(void))AT(0x7150))
#else
#define make_image create_image
#define init_modules initialize_modules
#define init_graph initialize_comdat
#define error_callback file_failure
#define make_library create_library
#define make_module link_native_create_module
extern Module *link_native_create_module(const char *,const char *,unsigned long,unsigned short,
    unsigned long,unsigned long,unsigned short,unsigned short,unsigned short,Library *,unsigned char *);
#define make_contribution create_contribution
#define make_graph create_comdat_module
#define make_node create_comdat_node
#define make_edge add_comdat_edge
#define make_root add_comdat_root
#define init_nodes initialize_comdat_node_enumerator
#define init_edges initialize_comdat_edge_enumerator
#define init_roots initialize_comdat_root_enumerator
#define init_undefined initialize_undefined_enumerator
#define init_contributions initialize_contribution_enumerator
#define end_nodes end_comdat_node_enumerator
#define end_edges end_comdat_edge_enumerator
#define end_roots end_comdat_root_enumerator
#define end_undefined end_undefined_enumerator
#define end_contributions end_contribution_enumerator
#define release_graph release_comdat_storage
#endif
static Image *image;
static Module *module,*empty_module,*synthetic_module;
static Library *library;
static ComdatModule *graph;
static ComdatNode *nodes,*first_node,*second_node;
static ComdatRoot *roots;
static unsigned char module_before[sizeof(Module)+2*sizeof(Contribution)];
static unsigned char nodes_before[2*sizeof(ComdatNode)],edges_before[sizeof(ComdatEdge)];
static unsigned char roots_before[2*sizeof(ComdatRoot)];
static unsigned long report[128],used;
unsigned long iterator_kind,iterator_phase;
static int owners_unchanged(void)
{
    return !memcmp(module_before,module,sizeof(module_before))&&
        !memcmp(nodes_before,graph->nodes,sizeof(nodes_before))&&
        !memcmp(edges_before,first_node->edges,sizeof(edges_before))&&
        roots&&roots->next&&roots->next->next==NULL&&
        !memcmp(roots_before,roots,sizeof(ComdatRoot))&&
        !memcmp(roots_before+sizeof(ComdatRoot),roots->next,sizeof(ComdatRoot));
}
#define END(scan,expected) do { \
    unsigned char before[sizeof(scan)]; \
    if((scan).end!=(expected)||used+4>128)return 0; \
    memcpy(before,&(scan),sizeof(scan)); \
    (scan).end(&(scan)); \
    if(memcmp(before,&(scan),sizeof(scan))||!owners_unchanged())return 0; \
    report[used++]=iterator_kind;report[used++]=iterator_phase; \
    report[used++]=sizeof(scan);report[used++]=1; \
} while(0)
static int workflow(void)
{
    Contribution *a,*b;unsigned char inserted;
    ComdatNodeEnumerator ns;ComdatEdgeEnumerator es;ComdatRootEnumerator rs;
    UndefinedEnumerator us;ContributionEnumerator cs;
    unsigned long visits;unsigned short cw,sw;HANDLE file;DWORD count;
    make_image(&image);init_modules(error_callback,&synthetic_module);init_graph(error_callback);
    library=make_library("enum.lib",0,0,&image->libraries);
    module=make_module("enum.obj","enum.obj",0,0x14c,0,0,0,0,2,library,&inserted);
    if(!inserted||module->section_count!=2)return 0;
    empty_module=make_module("empty.obj","empty.obj",0,0x14c,0,0,0,0,0,library,&inserted);
    if(!inserted||empty_module->section_count)return 0;
    a=make_contribution(".text$a",4,1,0,0,0,0,0,0x60000020,0,module,0,&image->sections,&image->optional);
    b=make_contribution(".text$b",8,0,0,0,0,4,0,0x60000020,4,module,0,&image->sections,&image->optional);
    if(a!=module->contributions||b!=a+1||module->contribution_count!=2)return 0;
    graph=make_graph(module);
    first_node=make_node(a,graph,"first",&nodes,0);
    second_node=make_node(b,graph,"second",&nodes,0);
    if(graph->node_count!=2||nodes!=second_node||second_node->next!=first_node||first_node->next)return 0;
    if(make_edge(NULL,first_node,b,0)!=first_node->edges||first_node->edge_count!=1||second_node->edge_count)return 0;
    make_root(NULL,NULL,a,&roots);make_root(NULL,NULL,b,&roots);
    if(!roots||!roots->next||roots->next->next)return 0;
    memcpy(module_before,module,sizeof(module_before));memcpy(nodes_before,graph->nodes,sizeof(nodes_before));
    memcpy(edges_before,first_node->edges,sizeof(edges_before));memcpy(roots_before,roots,sizeof(ComdatRoot));
    memcpy(roots_before+sizeof(ComdatRoot),roots->next,sizeof(ComdatRoot));
    memset(report,0,sizeof(report));report[0]=0x31544945;report[1]=1;used=8;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    report[2]=cw;report[3]=sw;
    iterator_kind=1;
    for(iterator_phase=0;iterator_phase<4;++iterator_phase){
        memset(&ns,0xa5,sizeof(ns));init_nodes(&ns,iterator_phase?nodes:NULL);
        if(iterator_phase==2&&!ns.next(&ns))return 0;
        if(iterator_phase==3){visits=0;while(ns.next(&ns))if(++visits>2)return 0;if(visits!=2)return 0;}
        END(ns,end_nodes);
    }
    iterator_kind=2;
    for(iterator_phase=0;iterator_phase<4;++iterator_phase){
        memset(&es,0xa5,sizeof(es));init_edges(&es,iterator_phase?first_node:second_node);
        if(iterator_phase==2&&!es.next(&es))return 0;
        if(iterator_phase==3){visits=0;while(es.next(&es))if(++visits>1)return 0;if(visits!=1)return 0;}
        END(es,end_edges);
    }
    iterator_kind=3;
    for(iterator_phase=0;iterator_phase<4;++iterator_phase){
        memset(&rs,0xa5,sizeof(rs));init_roots(&rs,iterator_phase?roots:NULL);
        if(iterator_phase==2&&!rs.next(&rs))return 0;
        if(iterator_phase==3){visits=0;while(rs.next(&rs))if(++visits>2)return 0;if(visits!=2)return 0;}
        END(rs,end_roots);
    }
    iterator_kind=4;
    for(iterator_phase=0;iterator_phase<2;++iterator_phase){
        memset(&us,0xa5,sizeof(us));init_undefined(&us,image->symbols);
        if(iterator_phase&&us.next(&us))return 0;
        END(us,end_undefined);
    }
    iterator_kind=5;
    for(iterator_phase=0;iterator_phase<4;++iterator_phase){
        memset(&cs,0xa5,sizeof(cs));init_contributions(&cs,iterator_phase?module:empty_module);
        if(iterator_phase==2&&!cs.next(&cs))return 0;
        if(iterator_phase==3){visits=0;while(cs.next(&cs))if(++visits>2)return 0;if(visits!=2)return 0;}
        END(cs,end_contributions);
    }
    report[4]=used;release_graph();
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    report[5]=cw;report[6]=sw;
    file=CreateFileA("iterators.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    recon_original_finish(context,workflow());
}
#else
int run_iterator_lifecycle(void){return workflow();}
#endif

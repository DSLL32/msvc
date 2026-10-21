/* COMDAT reachability graph and its synthetic root contribution. */
#include "comdat.h"
Contribution comdat_root_contribution;
ComdatNode comdat_root_node;
ArenaBlock *comdat_heap;
void (*comdat_error)(char *,int,char *);
unsigned char comdat_enabled;

void initialize_comdat(void (*error)(char *,int,char *))
{
    comdat_error=error;
    comdat_root_contribution.graph=&comdat_root_node;
    comdat_root_node.contribution=&comdat_root_contribution;
    comdat_root_node.edge_count=0;
    comdat_root_node.name=" linker defined TCE node";
    comdat_root_node.edges=NULL;
    comdat_root_node.next=NULL;
    comdat_heap=NULL;
    comdat_enabled=1;
}

ComdatModule *create_comdat_module(Module *module)
{
    ComdatModule *graph=allocate_arena(&comdat_heap,sizeof(*graph));
    if (!graph) comdat_error(NULL,5,"");
    graph->module=module;
    graph->nodes=allocate_arena(&comdat_heap,module->section_count*sizeof(ComdatNode));
    if (!graph->nodes) comdat_error(NULL,5,"");
    return graph;
}

ComdatNode *create_comdat_node(Contribution *contribution,ComdatModule *module,
    const char *name,ComdatNode **list,unsigned char synthetic)
{
    ComdatNode *node;
    if (synthetic) {
        node=allocate_arena(&comdat_heap,sizeof(*node));
        if (!node) comdat_error(NULL,5,"");
    } else node=&module->nodes[module->node_count++];
    node->contribution=contribution;
    node->edges=allocate_arena(&comdat_heap,contribution->relocation_count*sizeof(ComdatEdge));
    if (!node->edges) comdat_error(NULL,5,"");
    if (name) {
        node->name=duplicate_comdat_name(name);
        if (!node->name) comdat_error(NULL,5,"");
    }
    if (*list) node->next=*list;
    *list=node;
    return node;
}

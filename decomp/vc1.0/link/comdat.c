/* COMDAT reachability graph and its synthetic root contribution. */
#include "comdat.h"
#pragma intrinsic(strlen,strcpy)
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

/* Original [000238e0,00023910). A module owns the ordinary section nodes. */
void attach_comdat_node(Module *module,Contribution *contribution,const char *name)
{
    contribution->graph=create_comdat_node(contribution,module->comdat,name,&comdat_nodes,0);
}

/* Original [000173a0,00017420). Roots may name a symbol or bind a graph item. */
ComdatRoot *add_comdat_root(const char *name,External *external,
    Contribution *contribution,ComdatRoot **list)
{
    ComdatRoot *root=allocate_arena(&comdat_heap,sizeof(*root));
    if (!root) comdat_error(NULL,5,"");
    if (name) { root->target.name=name; root->kind=3; }
    else if (external) { root->target.external=external; root->kind=2; }
    else if (contribution) { root->target.contribution=contribution; root->kind=1; }
    if (*list) root->next=*list;
    *list=root;
    return root;
}

ComdatEdge *add_comdat_edge(const char *name,ComdatNode *node,
                           Contribution *contribution,unsigned char linker_defined)
{
    ComdatEdge *edge;
    unsigned short old_capacity,capacity;
    unsigned long index;
    /* Synthetic linker nodes grow beyond the relocation count of an input. */
    if (linker_defined && node->edge_count>=node->contribution->relocation_count) {
        old_capacity=node->contribution->relocation_count;
        capacity=old_capacity?(unsigned short)(old_capacity*2):16;
        node->edges=realloc(node->edges,(unsigned long)capacity*sizeof(*node->edges));
        if (!node->edges) comdat_error(NULL,5,"");
        for (index=old_capacity;index<capacity;++index) {
            node->edges[index].name=NULL;
            node->edges[index].external=NULL;
            node->edges[index].contribution=NULL;
        }
        node->contribution->relocation_count=capacity;
    }
    edge=&node->edges[node->edge_count++];
    if (name) {
        edge->name=duplicate_comdat_name(name);
        if (!edge->name) comdat_error(NULL,5,"");
        edge->contribution=NULL;
    } else {
        edge->contribution=contribution;
        edge->name=NULL;
    }
    return edge;
}

char *duplicate_comdat_name(const char *name)
{
    char *copy=allocate_arena(&comdat_heap,strlen(name)+1);
    strcpy(copy,name);
    return copy;
}

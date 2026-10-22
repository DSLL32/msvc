#ifndef LINK_COMDAT_H
#define LINK_COMDAT_H
#include "modules.h"
typedef struct External External;
typedef struct ComdatEdge {
    char *name;
    External *external;
    Contribution *contribution;
} ComdatEdge;
struct ComdatNode {
    Contribution *contribution;
    unsigned short edge_count, flags;
    char *name;
    ComdatEdge *edges;
    ComdatNode *next;
};
struct ComdatModule {
    Module *module;
    unsigned long node_count;
    ComdatNode *nodes;
};
typedef struct ComdatRoot {
    union { const char *name; External *external; Contribution *contribution; } target;
    struct ComdatRoot *next;
    unsigned short kind;
} ComdatRoot;
extern ComdatRoot *comdat_roots;
extern ComdatNode *comdat_nodes;
void attach_comdat_node(Module *,Contribution *,const char *);
ComdatRoot *add_comdat_root(const char *,External *,Contribution *,ComdatRoot **);
void initialize_comdat(void (*)(char *,int,char *));
ComdatModule *create_comdat_module(Module *);
ComdatNode *create_comdat_node(Contribution *,ComdatModule *,const char *,ComdatNode **,unsigned char);
ComdatEdge *add_comdat_edge(const char *,ComdatNode *,Contribution *,unsigned char);
char *duplicate_comdat_name(const char *);
#endif

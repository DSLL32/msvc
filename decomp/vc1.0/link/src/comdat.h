#ifndef LINK_COMDAT_H
#define LINK_COMDAT_H
#include "modules.h"
#include <recon.h>
typedef struct External External;
typedef struct ComdatEdge {
    char *name;
    ComdatNode *node;
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
typedef struct ComdatNodeEnumerator {
    unsigned char (*next)(struct ComdatNodeEnumerator *);
    void (*end)(struct ComdatNodeEnumerator *);
    ComdatNode *current,*first;
} ComdatNodeEnumerator;
typedef struct ComdatEdgeEnumerator {
    unsigned char (*next)(struct ComdatEdgeEnumerator *);
    void (*end)(struct ComdatEdgeEnumerator *);
    unsigned short index;
    ComdatEdge *current;
    ComdatNode *node;
} ComdatEdgeEnumerator;
typedef struct ComdatRootEnumerator {
    unsigned char (*next)(struct ComdatRootEnumerator *);
    void (*end)(struct ComdatRootEnumerator *);
    ComdatRoot *current,*first;
} ComdatRootEnumerator;
void initialize_comdat_node_enumerator(ComdatNodeEnumerator *,ComdatNode *);
unsigned char next_comdat_node(ComdatNodeEnumerator *);
void end_comdat_node_enumerator(ComdatNodeEnumerator *);
void initialize_comdat_edge_enumerator(ComdatEdgeEnumerator *,ComdatNode *);
unsigned char next_comdat_edge(ComdatEdgeEnumerator *);
void end_comdat_edge_enumerator(ComdatEdgeEnumerator *);
void initialize_comdat_root_enumerator(ComdatRootEnumerator *,ComdatRoot *);
unsigned char next_comdat_root(ComdatRootEnumerator *);
void end_comdat_root_enumerator(ComdatRootEnumerator *);
void visit_comdat_node(ComdatNode *);
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

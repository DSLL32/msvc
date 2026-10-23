#ifndef LINK_COMDAT_DIAGNOSTICS_H
#define LINK_COMDAT_DIAGNOSTICS_H
#include "comdat.h"
#include <recon.h>
const char *comdat_usage_label(ComdatNode *node);
int print_comdat_node(ComdatNode *node);
int print_comdat_edge(ComdatEdge *edge);
int dump_comdat_graph(ComdatNode *first);
int debug_printf(const char *format,...);
int open_debug_log(const char *path);
#endif

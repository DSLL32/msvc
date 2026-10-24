/* Named ABIs awaiting direct diagnostic frontier acceptance. */
#include <stdarg.h>
#include "comdat-diagnostics.h"
#include <recon.h>
/* Original [00017520,00017538). Pointer identity is retained by the data table. */
char *comdat_usage_labels[2]={"R","N"};
const char *comdat_usage_label(ComdatNode *node)
{
    return comdat_usage_labels[(node->contribution->flags&0x2000)?0:1];
}
/* Original [00017540,000175d3). Print only the diagnostic fields used here. */
int print_comdat_node(ComdatNode *node)
{
    debug_printf("%s",comdat_usage_label(node));
    debug_printf("|sec=%.8s",node->contribution->group->section->name);
    debug_printf("|grp=%.8s",node->contribution->group->name);
    debug_printf("|con=0x%p",node->contribution);
    debug_printf("|mod=%s",node->contribution->module->path);
    if (node->name) debug_printf("|%s()",node->name);
    return debug_printf("\n");
}
/* Original [000175e0,000175f1). A resolved edge owns the actual node pointer. */
int print_comdat_edge(ComdatEdge *edge)
{
    return print_comdat_node(edge->node);
}
/* Original [00017600,000176df). The caller owns the graph; no mutation. */
int dump_comdat_graph(ComdatNode *first)
{
    ComdatNodeEnumerator nodes;
    ComdatEdgeEnumerator edges;
    ComdatNode *node;
    debug_printf("Dump of comdat dependency graph for TCE\n");
    debug_printf("---------------------------------------\n");
    initialize_comdat_node_enumerator(&nodes,first);
    while (next_comdat_node(&nodes)) {
        node=nodes.current;
        debug_printf("PNOD|");
        print_comdat_node(node);
        initialize_comdat_edge_enumerator(&edges,node);
        while (next_comdat_edge(&edges)) {
            debug_printf("    PEDG|");
            print_comdat_edge(edges.current);
        }
        debug_printf("\n");
    }
    return fflush(stdout);
}
/* Original [0003a880,0003a8ff). Suppressing both routes leaves the original
 * result undefined; do not invent an initialized return for that unvisited case. */
FILE *debug_log;
int debug_printf(const char *format,...)
{
    va_list arguments;
    int result;
    va_start(arguments,format);
    if (!debug_enabled(2)) result=vfprintf(stdout,format,arguments);
    if (debug_enabled(1)) {
        if (debug_log || open_debug_log("db.log"))
            result=vfprintf(debug_log,format,arguments);
    }
    va_end(arguments);
    return result;
}
/* Closing with NULL retains the original pointer; subsequent file use is
 * disabled by the flag. Do not replace this with a pointer-clearing repair. */
int open_debug_log(const char *path)
{
    if (debug_log) fclose(debug_log);
    if (!path) {
        debug_flags[1]=0;
        return 0;
    }
    debug_log=fopen(path,"w");
    if (!debug_log) {
        perror(path);
        return 0;
    }
    setbuf(debug_log,NULL);
    debug_flags[1]=1;
    return 1;
}

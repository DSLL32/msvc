#ifndef GLOBAL_WORK_H
#define GLOBAL_WORK_H
#include "functions.h"
/* The region tree and predecessor/successor links have distinct owners. */
typedef struct FlowRegion FlowRegion;
typedef struct FlowLink FlowLink;
extern WorkItem *global_entry_work;
extern FlowRegion *global_flow_regions;
extern FlowLink *free_flow_links;
extern int global_work_capacity, global_flow_changed;
void reset_flow_label_state(void);
void initialize_flow_labels(void);
WorkItem * __fastcall partition_global_work(Expression *expressions);
void resolve_flow_labels(void);
void __fastcall prepare_flow_blocks(WorkItem *work);
void __fastcall refresh_flow_edges(WorkItem *work);
void __fastcall number_work_blocks(WorkItem *work);
void __fastcall simplify_flow_branches(WorkItem *work);
void __fastcall merge_flow_blocks(WorkItem *work);
void __fastcall compute_work_dominators(WorkItem *work);
void __fastcall find_flow_regions(WorkItem *work);
unsigned __fastcall simplify_flow_regions(FlowRegion *regions);
void __fastcall refresh_region_entries(WorkItem *work);
WorkItem *__fastcall reduce_flow_regions(FlowRegion *regions);
void __fastcall analyze_flow_regions(FlowRegion *regions);
void __fastcall finish_flow_regions(FlowRegion *regions);
void __fastcall remove_empty_flow_blocks(WorkItem *work);
void reset_flow_labels(void);
void *__cdecl build_work_lists(const char *format, ...);
void __fastcall set_flow_expression_input(Expression *expressions);
Expression *next_flow_expression(void);
Expression *__fastcall fold_adjacent_flow_labels(Expression *expression);
void __fastcall putback_flow_expression(Expression *expression);
void __fastcall define_flow_label(SymbolHeader *label, WorkItem *work);
void __fastcall reference_flow_label(SymbolHeader *label, WorkItem *work);
#endif

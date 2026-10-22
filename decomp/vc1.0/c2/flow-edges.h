#ifndef FLOW_EDGES_H
#define FLOW_EDGES_H
#include "global-work.h"
#include "machine.h"
/* Source exception-scope records are distinct from the optimizer region tree. */
typedef struct ExceptionScope {
    struct ExceptionScope *next;
    unsigned kind, state, identifier, field10;
    long parent_identifier;
    SymbolHeader *handler_label, *filter_label, *alternate_label;
} ExceptionScope;
typedef unsigned (__fastcall *MachineSymbolVisitor)(SymbolHeader *symbol, void *context);
void __fastcall add_flow_predecessor(WorkItem *work, WorkItem *predecessor);
void __fastcall add_flow_successor(WorkItem *work, WorkItem *successor);
void __fastcall add_protected_flow_edge(WorkItem *work, WorkItem *successor);
void __fastcall release_flow_link(FlowLink *link);
Node *__fastcall normalize_flow_targets(Expression *expressions);
void __fastcall visit_machine_symbols(MachineInstruction *instruction,
    MachineSymbolVisitor visitor, void *context);
unsigned __fastcall reference_assembly_flow_symbol(SymbolHeader *symbol, void *context);
ExceptionScope *__fastcall lookup_exception_scope(ExceptionScope *scopes, long identifier);
unsigned __fastcall normalize_assembly_flow_symbol(SymbolHeader *symbol, void *context);
void __fastcall connect_flow_blocks(WorkItem *source, WorkItem *target);
void __fastcall remove_flow_reference(FlowLink **links, WorkItem *work);
Expression *__fastcall last_flow_expression(Expression *expressions);
void __fastcall remove_flow_work(WorkItem *work);
void __fastcall destroy_flow_work(WorkItem *work);
void __fastcall transfer_flow_region_work(WorkItem *source, WorkItem *replacement);
void __fastcall mark_reachable_work(WorkItem *work);
void __fastcall forget_exception_label(Function *function, SymbolHeader *label);
void __fastcall visit_flow_order(WorkItem *work);
int __fastcall ensure_flow_branch(WorkItem *source, WorkItem *target);
void __fastcall sort_flow_successors(FlowLink **links, WorkItem *following);
int __fastcall count_ordinary_flow_links(FlowLink *links);
Node *new_flow_label_node(void);
void __fastcall replace_flow_region_last(FlowRegion *region, WorkItem *replacement);
FlowRegion *new_flow_region(void);
WorkItem *__fastcall find_irreducible_flow_end(WorkItem *work);
FlowLink *__fastcall partition_loop_predecessors(FlowLink *predecessors, WorkItem *header);
void __fastcall redirect_flow_label(WorkItem *work, SymbolHeader *old_label, SymbolHeader *new_label);
void __fastcall assign_flow_region(FlowRegion *region);
void __fastcall scan_irreducible_predecessors(WorkItem *work);
void __fastcall clear_flow_visit_marks(WorkItem *work);
int __fastcall work_in_flow_region(WorkItem *work, FlowRegion *region);
#endif

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
#endif

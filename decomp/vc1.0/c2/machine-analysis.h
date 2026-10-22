#ifndef MACHINE_ANALYSIS_H
#define MACHINE_ANALYSIS_H
#include "machine.h"
typedef struct MachineRegionEdge {
    struct MachineRegionEdge *next;
    SymbolHeader *label;
} MachineRegionEdge;
typedef char MachineRegionEdgeSizeCheck[sizeof(MachineRegionEdge) == 8 ? 1 : -1];
/* Constructor483a0 allocates96 bytes. The label attachment encountered by
   scanning is this region, whose entry-label back-pointer is at0x14. Fields
   awaiting their consumers retain offset names instead of inferred meaning. */
struct MachineFlowRegion {
    MachineFlowRegion *parent, *next, *children;
    MachineInstruction *first, *last, *label, *exit_label;
    MachineRegionEdge *references[3];
    unsigned field28, field2c, field30, field34, field38;
    unsigned field3c, field40, field44;
    int first_block, last_block;
    int pending_references, initial_references;
    unsigned field58, flags;
};
typedef char MachineFlowRegionSizeCheck[sizeof(MachineFlowRegion) == 96 ? 1 : -1];
extern MachineFlowRegion *machine_region_root;
extern int machine_analysis_needs_label_cleanup;
void __fastcall build_machine_regions(MachineInstruction *instructions);
void __fastcall analyze_machine_regions(MachineFlowRegion *region, unsigned flags);
MachineFlowRegion *__fastcall new_machine_region(MachineFlowRegion *parent, MachineInstruction *boundary, MachineInstruction *label);
int __fastcall count_region_label_references(MachineFlowRegion *region, SymbolHeader *label);
MachineFlowRegion *__fastcall process_machine_region_reference(MachineInstruction *instruction, MachineFlowRegion *region, SymbolHeader *label);
SymbolHeader *__fastcall machine_operand_label(MachineInstruction *instruction);
void __fastcall replace_machine_region_suffix(MachineFlowRegion *parent, MachineFlowRegion *before, MachineFlowRegion *regions);
MachineRegionEdge *__fastcall add_machine_region_reference(MachineFlowRegion *region, SymbolHeader *label, unsigned chain);
int __fastcall close_machine_region(MachineFlowRegion *parent, MachineFlowRegion *region);
void __fastcall finish_closed_machine_region(MachineFlowRegion *region, MachineFlowRegion *parent);
MachineFlowRegion *__fastcall process_special_machine_region_reference(MachineFlowRegion *region, SymbolHeader *label);
#endif

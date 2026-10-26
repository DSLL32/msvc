#ifndef MACHINE_ANALYSIS_H
#define MACHINE_ANALYSIS_H
#include "machine.h"
#include <recon.h>
typedef struct MachineRegionEdge {
    struct MachineRegionEdge *next;
    SymbolHeader *label;
} MachineRegionEdge;
typedef char MachineRegionEdgeSizeCheck[sizeof(MachineRegionEdge) == 8 ? 1 : -1];
/* Constructor483a0 allocates96 bytes. The label attachment encountered by
   scanning is this region, whose entry-label back-pointer is at0x14. Fields
   awaiting their consumers retain offset names instead of inferred meaning. */
typedef struct MachineRegionRegister {
    struct MachineRegionRegister *next;
    unsigned short number, field06;
    unsigned flags, uses, definitions;
    MachineInstruction *definition;
    unsigned increment, field1c;
} MachineRegionRegister;
typedef char MachineRegionRegisterSizeCheck[sizeof(MachineRegionRegister) == 32 ? 1 : -1];
struct MachineFlowRegion {
    MachineFlowRegion *parent, *next, *children;
    MachineInstruction *first, *last, *label, *exit_label;
    MachineRegionEdge *references[3];
    MachineRegionRegister *registers[8]; /*28..48: register number&7 chains*/
    int first_block, last_block;
    int pending_references, initial_references;
    union { unsigned field58;
        struct { unsigned short split_color, saved_split_register; };
    };
    unsigned flags;
};
typedef char MachineFlowRegionSizeCheck[sizeof(MachineFlowRegion) == 96 ? 1 : -1];
extern MachineFlowRegion *machine_region_root;
extern MachineBlockHeader *region_insertion_block;
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
int __fastcall promote_region_immediates(MachineFlowRegion *region);
extern MachineFlowRegion *active_machine_region;
void __fastcall prepare_machine_region_transform(MachineFlowRegion *region);
int __fastcall retarget_machine_region_entry(LabelSymbol *label, LabelSymbol *replacement, MachineInstruction *first, MachineInstruction *last);
void __fastcall prepare_region_insertion(MachineFlowRegion *region);
void __fastcall replace_machine_definition(unsigned identifier, MachineInstruction *previous, MachineInstruction *replacement);
void __fastcall record_machine_definition_range(unsigned short number, MachineInstruction *first, MachineInstruction *last);
void __fastcall record_region_register(MachineFlowRegion *region, unsigned short number, int add, MachineBlockHeader *preserved_definition);
void __fastcall transfer_machine_region_references(MachineFlowRegion *region, MachineFlowRegion *parent);
MachineInstruction *__fastcall next_machine_region_record(unsigned kind, MachineFlowRegion *region,
    MachineInstruction *previous, MachineFlowRegion **children);
void __fastcall simplify_machine_region_operations(MachineFlowRegion *region);
void __fastcall transform_machine_region_blocks(MachineFlowRegion *region);
MachineRegionRegister *__fastcall find_machine_region_register(MachineFlowRegion *region,
    unsigned short number, int create);
int __fastcall register_is_invariant_in_regions(unsigned short number, MachineFlowRegion *regions);
int __fastcall register_dead_after_region(unsigned short number, MachineFlowRegion *region, int check_entry);
int __fastcall instruction_precedes_region_entry(MachineInstruction *instruction);
#endif

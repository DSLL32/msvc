#ifndef C2_FINAL_SCHEDULE_H
#define C2_FINAL_SCHEDULE_H
#include "function-records.h"
#include "frame-layout.h"
#include <recon.h>

typedef struct C2ScheduleWeights {
    short fixed_resource_weight;
    short path_weight, outgoing_weight, critical_weight;
    short pair_weight;
    short read_weight, write_weight;
    unsigned short pending0e;
} C2ScheduleWeights;
typedef struct C2ScheduleOpcodeCost {
    unsigned result_delay;
    unsigned char pending04[8];
} C2ScheduleOpcodeCost;
typedef struct C2ScheduleResource {
    short capacity, spacing, issued;
    unsigned short pending06;
    int occupied;
} C2ScheduleResource;
typedef struct C2ScheduleDependency {
    struct C2ScheduleDependency *next_outgoing, *next_incoming;
    struct C2ScheduleNode *source, *destination;
    unsigned flags;
    unsigned short delay;
    unsigned char properties, pending17;
} C2ScheduleDependency;
typedef char C2ScheduleDependencySizeCheck[sizeof(C2ScheduleDependency) == 24 ? 1 : -1];
typedef char C2ScheduleDependencyDelayCheck[offsetof(C2ScheduleDependency, delay) == 20 ? 1 : -1];
typedef struct C2ScheduleMemory C2ScheduleMemory;
/* The scheduler receives the existing frame-slot member chain. */
typedef C2FrameSlotUse C2ScheduleRegisterAlias;
typedef struct C2ScheduleNode {
    struct C2ScheduleNode *previous, *next;
    C2ScheduleDependency *incoming, *outgoing;
    struct C2ScheduleNode *ready_next, *ready_previous;
    unsigned pending18;
    C2Expression *record;
    unsigned short incoming_count, outgoing_count, remaining_incoming;
    unsigned short pending26;
    unsigned priority, current_priority;
    unsigned ready_cycle;
    unsigned short path_length;
    unsigned short number;
    unsigned char cost, properties, flags3a, pending3b;
} C2ScheduleNode;
struct C2ScheduleMemory {
    C2Expression *operand;
    C2ScheduleNode *node;
    C2ScheduleMemory *next;
};
typedef char C2ScheduleMemorySizeCheck[sizeof(C2ScheduleMemory) == 12 ? 1 : -1];
typedef char C2ScheduleMemoryNextCheck[offsetof(C2ScheduleMemory, next) == 8 ? 1 : -1];
typedef struct C2ScheduleGraph {
    union {
        C2FlowGraph flow;
        struct {
            unsigned char pending00[36];
            unsigned node_count, edge_count;
            unsigned char pending2c[8];
        };
    };
    unsigned char pending34[4];
} C2ScheduleGraph;
typedef char C2ScheduleGraphSizeCheck[sizeof(C2ScheduleGraph) == 56 ? 1 : -1];
typedef char C2ScheduleNodeSizeCheck[sizeof(C2ScheduleNode) == 60 ? 1 : -1];
typedef char C2ScheduleNodeRecordCheck[offsetof(C2ScheduleNode, record) == 28 ? 1 : -1];
typedef char C2ScheduleNodeFlagsCheck[offsetof(C2ScheduleNode, flags3a) == 58 ? 1 : -1];
typedef struct C2ScheduleRegisterUse {
    C2FunctionRegister *reg;
    C2ScheduleNode *node;
    struct C2ScheduleRegisterUse *next;
} C2ScheduleRegisterUse;
typedef char C2ScheduleRegisterUseSizeCheck[sizeof(C2ScheduleRegisterUse) == 12 ? 1 : -1];

typedef void (__fastcall *C2ScheduleNodeHook)(C2ScheduleNode *);
typedef void (__fastcall *C2ScheduleEdgeHook)(C2ScheduleNode *, C2ScheduleNode *, C2ScheduleDependency *, unsigned);
typedef short (__fastcall *C2ScheduleSelector)(C2ScheduleNode *, C2ScheduleNode **, int);
typedef char ScheduleWeightsSizeCheck[sizeof(C2ScheduleWeights) == 16 ? 1 : -1];
typedef char ScheduleOpcodeCostSizeCheck[sizeof(C2ScheduleOpcodeCost) == 12 ? 1 : -1];
typedef char ScheduleResourceSizeCheck[sizeof(C2ScheduleResource) == 12 ? 1 : -1];
typedef char ScheduleResourceOccupiedCheck[offsetof(C2ScheduleResource, occupied) == 8 ? 1 : -1];

extern short schedule_issue_width, schedule_resource_count, schedule_cpu_index;
extern C2ScheduleResource schedule_resources[4];
extern int schedule_function_flag8, schedule_function_flag16, schedule_aggregate_return;
extern unsigned short schedule_parameter_4921f8;
extern C2ScheduleWeights *schedule_weights;
extern C2ScheduleOpcodeCost *schedule_opcode_costs;
extern C2ScheduleSelector schedule_select_node;
extern int schedule_final_mode;
void __fastcall initialize_schedule_function(C2Function *function);
extern C2ScheduleRegisterUse *schedule_register_reads, *schedule_register_writes;
void __fastcall add_schedule_register_dependencies(C2FunctionRegister *reg, C2ScheduleNode *node, unsigned flags);
unsigned __fastcall schedule_node_operand_cost(C2ScheduleNode *node);
#endif

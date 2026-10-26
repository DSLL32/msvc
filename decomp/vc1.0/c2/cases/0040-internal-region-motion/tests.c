#include "../../src/machine-region-motion.h"
#include "../../src/instruction-selection.h"
#include <recon-tool.h>
#include <recon.h>
/* The original4b010 helper preserves each destination annotation bit,
   including independently defined0x8000. Source reserved bits are copied;
   these are deliberate defined inputs, unlike the partial App stack locals. */
static int annotations(void)
{
    MachineOperand source,destination;
    source.flags=0x12340001;source.value.reg.value=17;
    destination.flags=0xc401;destination.value.reg.value=19;
    replace_region_operand_value(&source,&destination);
    if(destination.flags!=0x1234c401||destination.value.reg.value!=17)return 0;
    source.flags=0x1234fc01;destination.flags=1;
    replace_region_operand_value(&source,&destination);
    return destination.flags==0x12340001&&destination.value.reg.value==17;
}
static int displacement(void)
{
    MachineOperand left,right;MachineAddress a,b;
    left.flags=right.flags=OPERAND_BASED;left.value.address=&a;right.value.address=&b;
    a.first.displacement=48;b.first.displacement=16;
    if(region_operand_displacement(&left,&right)!=32)return 0;
    a.first.displacement=-16;b.first.displacement=16;
    if(region_operand_displacement(&left,&right)!=-32)return 0;
    left.flags=right.flags=OPERAND_REGISTER;left.value.reg.value=right.value.reg.value=17;
    return region_operand_displacement(&left,&right)==0;
}
const ReconTest region_motion_tests[]={
    {"replacement preserves existing analysis annotations",1,annotations},
    {"matching address bases retain signed displacement differences",1,displacement}
};

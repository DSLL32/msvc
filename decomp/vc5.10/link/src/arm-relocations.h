#ifndef LINK_ARM_RELOCATIONS_H
#define LINK_ARM_RELOCATIONS_H
#include <recon.h>
/* Private ARM COFF names from the target's relocation-name table; the shipped
 * 5.10 WINNT.H does not declare these ARM relocation constants. */
enum ArmRelocationType {
    ARM_REL_ABSOLUTE=0,ARM_REL_ADDR32=1,ARM_REL_ADDR32NB=2,
    ARM_REL_BRANCH24=3,ARM_REL_BRANCH11=4,
    ARM_REL_SECTION=14,ARM_REL_SECREL=15
};
#endif

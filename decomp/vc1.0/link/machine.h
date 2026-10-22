#ifndef LINK_MACHINE_H
#define LINK_MACHINE_H
#include "image.h"
#include "arguments.h"
typedef void (*ApplyRelocations)(Contribution *,void *,PIMAGE_SYMBOL,Image *);
extern ApplyRelocations apply_relocations;
void apply_i386_relocations(Contribution *,void *,PIMAGE_SYMBOL,Image *);
void apply_m68k_relocations(Contribution *,void *,PIMAGE_SYMBOL,Image *);
#endif

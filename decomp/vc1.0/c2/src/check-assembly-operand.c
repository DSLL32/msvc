/* Isolated operand decoder: symbol lookup/identifier seams are explicit. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instruction-selection.h"
#include "function-input.h"
#include <recon.h>
#undef printf
#undef sscanf
#undef exit
MachineAddress *free_machine_addresses;
static SymbolHeader test_symbol;
static unsigned allocated, looked_up;
void *__fastcall arena_alloc(unsigned arena, unsigned bytes)
{
    if (arena != 1 || bytes != sizeof(MachineAddress)) abort();
    ++allocated;
    return calloc(1, bytes);
}
void __fastcall compiler_internal_error(char *source, unsigned code)
{
    if (strcmp(source,"@(#)codegen.c:1.167")) abort();
    printf("ERR %x\n", code);
    exit(90);
}
SymbolHeader *__fastcall lookup_symbol(ScopeHash *scope, unsigned identifier)
{
    if (scope != (ScopeHash *)&test_symbol) abort();
    ++looked_up;
    return identifier == 7 ? &test_symbol : NULL;
}
unsigned char *__fastcall decode_assembly_operand(MachineOperand *,unsigned char *,ScopeHash *);
int main(int argc, char **argv)
{
    unsigned char bytes[100];
    unsigned count, i, value, row[7];
    unsigned char *end;
    MachineOperand operand;
    MachineAddress *address;
    if (argc!=2 || strlen(argv[1])>sizeof(bytes)*2 || strlen(argv[1])%2) return 2;
    count = strlen(argv[1])/2;
    for(i=0;i<count;++i) {
        if(sscanf(argv[1]+2*i,"%2x",&value)!=1)return 3;
        bytes[i]=value;
    }
    end = decode_assembly_operand(&operand, bytes, (ScopeHash *)&test_symbol);
    memset(row,0,sizeof(row));
    row[0] = operand.flags;
    value = operand.flags&15;
    if ((value>=3 && value<=7)||value==12) {
        address=operand.value.address;
        row[2]=address->first.displacement;
        row[3]=address->symbol==&test_symbol?42:0;
        row[4]=address->base.value;
        row[5]=address->index.value;
    } else if(value==OPERAND_SYMBOL) row[1]=operand.value.symbol==&test_symbol?42:0;
    else row[1]=operand.value.constant;
    printf("%u %u %u",(unsigned)(end-bytes),allocated,looked_up);
    for(i=0;i<7;++i)printf(" %u",row[i]);
    printf("\n");
    return 0;
}

/* Standalone ABI witness, never part of the reconstructed application graph. */
#include "debug-info.h"
#include <recon.h>
int __fastcall open_object_probe(ProgramDatabase *self, unsigned unused_edx,
    char *path, char *mode, unsigned signature)
{
    return self == (ProgramDatabase *)0x12345678 && path[0] == 'p' &&
        mode[0] == 'w' && signature == 0x87654321;
}
int __fastcall reload_object_probe(ProgramDatabase *self)
{
    return self == (ProgramDatabase *)0x12345678;
}
void __fastcall destroy_list_probe(PdbBufferList *self)
{
    self->count = 23;
}
int main(void)
{
    PdbBufferList list;
    list.count = 7;
    if (!open_object_probe((ProgramDatabase *)0x12345678, 0,
            "probe.pdb", "wc", 0x87654321)) return 1;
    if (!reload_object_probe((ProgramDatabase *)0x12345678)) return 2;
    destroy_list_probe(&list);
    return list.count == 23 ? 0 : 3;
}

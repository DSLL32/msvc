/* Original-toolchain input for a real initialized-data DLL. The 486 routine is
 * inspected by the dumper only and is never an entry point or called. */
unsigned char payload[3UL*1024*1024+257]={0x61,0x62,0x63};
unsigned long payload_tail=0x13579bdfUL;
unsigned char *payload_pointer=payload+123;

int __stdcall ModuleEntry(void *module,unsigned long reason,void *reserved)
{
    return payload_pointer[0]==0 && payload_tail==0x13579bdfUL;
}

void diagnostic_486(void)
{
    __asm {
        xadd dword ptr [esi],eax
        cmpxchg byte ptr [esi],cl
        cmpxchg dword ptr [esi],eax
    }
}

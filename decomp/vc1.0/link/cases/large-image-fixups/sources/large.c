/* Original-toolchain input for a real initialized-data DLL. The 486 routine is
 * inspected by the dumper only and is never an entry point or called. */
unsigned char blob[3UL*1024*1024+257]={0x61,0x62,0x63};
unsigned long tail=0x13579bdfUL;
unsigned char *ptr=blob+123;

int __stdcall E(void *module,unsigned long reason,void *reserved)
{
    return ptr[0]==0 && tail==0x13579bdfUL;
}

void ops486(void)
{
    __asm {
        xadd dword ptr [esi],eax
        cmpxchg byte ptr [esi],cl
        cmpxchg dword ptr [esi],eax
    }
}

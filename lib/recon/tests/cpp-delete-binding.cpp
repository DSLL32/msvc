#include <new.h>
#include <assert.h>
#include <recon.h>

static void save_fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
void release_storage(void *pointer) { ::operator delete(pointer); }

int main(int argc,char **argv)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    unsigned char before[108],after[108];unsigned n,phase;
    void *owned=::operator new(32);
    ReconCppAllocation *head=recon_cpp_allocations,old;
    (void)argv;
    memset(owned,0xa5,32);
    if(argc>1) __asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(phase=0;phase<2;++phase) {
        memcpy(&old,head,sizeof(old));
        for(n=0;n<sizeof(errors)/sizeof(errors[0]);++n) {
            errno=errors[n][0];recon_doserrno=errors[n][1];
            recon_SetLastError(errors[n][2]);
            save_fp(before);release_storage(NULL);save_fp(after);
            assert(!memcmp(before,after,sizeof(before)));
            assert(errno==(int)errors[n][0] && recon_doserrno==errors[n][1] &&
                   recon_GetLastError()==errors[n][2]);
            assert(recon_cpp_allocations==head && !memcmp(head,&old,sizeof(old)));
            if(!phase) {
                unsigned i;for(i=0;i<32;++i)assert(((unsigned char*)owned)[i]==0xa5);
            }
        }
        if(!phase) {release_storage(owned);assert(!head->live);}
    }
    if(argc>1) __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}

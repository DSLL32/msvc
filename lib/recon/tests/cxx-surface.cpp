#include <assert.h>
#include <float.h>
#include <mbstring.h>
#include <sys/timeb.h>
#include <setjmp.h>
#include <recon.h>
class Counter {
public:
    int value;
    int RECON_THISCALL set(int);
};
int RECON_THISCALL Counter::set(int input) { value=input; return value; }
extern "C" int cxx_shared_surface(void)
{
    Counter counter; jmp_buf saved; struct _timeb now;
    counter.value=0; assert(counter.set(7)==7 && counter.value==7);
    assert(_mbsncmp((const unsigned char *)"a",(const unsigned char *)"b",1)==-1);
    assert(__p__fmode()==&recon_fmode);
    _ftime(&now); assert(now.time>0);
    _controlfp(0,0); SetLastError(123); assert(GetLastError()==123);
    if(!setjmp(saved)) longjmp(saved,9);
    return 0;
}
int main(void) { return cxx_shared_surface(); }

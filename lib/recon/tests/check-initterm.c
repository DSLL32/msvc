#include <assert.h>
#include <recon.h>

static ReconInitializer callbacks[6];
static int sequence[3],count;
static void last(void) { sequence[count++]=3; errno=EDOM; SetLastError(92); }
static void replaced(void) { sequence[count++]=99; }
static void first(void) { sequence[count++]=1; callbacks[4]=last; }
static void middle(void) { sequence[count++]=2; }

int main(void)
{
    assert(_adjust_fdiv==0);
    callbacks[0]=NULL; callbacks[1]=first; callbacks[2]=NULL;
    callbacks[3]=middle; callbacks[4]=replaced; callbacks[5]=replaced;
    _initterm(callbacks,callbacks+5);
    assert(count==3 && sequence[0]==1 && sequence[1]==2 && sequence[2]==3);
    assert(errno==EDOM && GetLastError()==92);
    _initterm(callbacks+5,callbacks+5);
    _initterm(callbacks+5,callbacks);
    assert(count==3 && errno==EDOM && GetLastError()==92);
    _adjust_fdiv=1;
    _initterm(callbacks,callbacks+1);
    assert(_adjust_fdiv==1);
    return 0;
}

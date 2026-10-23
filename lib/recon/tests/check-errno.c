#include <assert.h>
#include <limits.h>
#include <recon.h>
int main(void)
{
    int *first=_errno();char *end;long value;
    assert(first==&errno && _errno()==first);*first=0;
    value=strtol("123",&end,10);assert(value==123 && !*end && !*first);
    value=strtol("999999999999999999999999999999",&end,10);
    assert(value==LONG_MAX && !*end && *first==ERANGE);
    *first=111;assert(errno==111);return 0;
}

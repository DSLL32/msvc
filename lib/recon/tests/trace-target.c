#include <stdio.h>
#include <errno.h>

static int __attribute__((noinline)) hidden(int value)
{
    return value+1;
}
int __attribute__((noinline)) alpha(int value)
{
    return hidden(value);
}
int __attribute__((fastcall,noinline)) beta(int a,int b)
{
    return a+b;
}
int main(void)
{
    errno=EDOM;
    if (alpha(2)!=3 || alpha(3)!=4 || beta(5,6)!=11 || errno!=EDOM) return 1;
    puts("trace target output");
    return 0;
}

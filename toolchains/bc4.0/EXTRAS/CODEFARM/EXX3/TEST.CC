// Note that in all other examples the data organizations is always global.
// That does not contradict to the principles of object-oriented programming.
// In fact, one of the big advantages of our library is that data objects
// and data relations are completely orthogonal (independent). Making
// organizations global only emphasizes the independence. Forcing them
// into local scope introduces an artificial dependency.
//
// This example shows that if you want the organization in a local scope,
// you can do it in a way which is simpler and more elegant than using
// templates or any other library.
// 
// This version of the program is for C++ Ver.2.0 AT&T on SUN.

#include <stdio.h>
#define ZZmain
#include "zzincl.h"

class A {
    static A *entry;
    int a;
    ZZ_EXT_A
public:
    int val(void){return a;};
    A(){entry=NULL;}
    A(int i);
    A* next(void);
    ZZ_LOCAL_SINGLE_RING(myRing,A,A);
};
ZZ_HYPER_UTILITIES(util);

A* A::next(void){return myRing.fwd(this);}
A::A(int i){a=i; entry=myRing.add(entry,this);}

int main(void){
    A aa,*a1,*a2;

    a1=new A(1);
    a2=new A(2);

    a2=a1->next();
    printf("%d %d\n",a1->val(),a2->val());

    // a1=myRing.fwd(a2);  causes compiler error, unknown myRing
}
#include "zzfunc.c"

// This is a continuation of the same test. Files zzincl.h and zzfunc.c
// are the same. 
// util.open() is the opposite of util.save(). It loads the data
// from the file back into memory, and returns new key entries (in this
// case only one key entry, pointer to the Head object).
//
// Just to show a better interface, I encapsulated all the prints under Head.
// If you compare the four lines with test1.cc, they are exactly the same.
//
// In order to run, just type: test2

#include <stdio.h>
#define ZZmain
#include "zzincl.h"

class Head {
    ZZ_EXT_Head
    void prtAll(void);
};

class Employee {
    int IDnum;
    ZZ_EXT_Employee
public:
    Employee(int idn,char *name);
    Employee(){}; // required if other constructors present
    void prt();
};

ZZ_HYPER_SINGLE_COLLECT(list1,Head,Employee);
ZZ_HYPER_SINGLE_COLLECT(list2,Head,Employee);
ZZ_HYPER_NAME(eName,Employee);
ZZ_HYPER_UTILITIES(util);

void Head::prtAll(void){
    Employee *e;
    list1_iterator it1(this); // print list1
    while(e=it1++)e->prt();
    list2_iterator it2(this); // print list2
    while(e=it2++)e->prt();
}

Employee::Employee(int idn, char *name){char *p;
    IDnum=idn; p=util.strAlloc(name); eName.add(this,p);}
void Employee::prt(void){printf("%d %s\n",IDnum,eName.fwd(this));}

int main(void){
    Head *h; char *v[1],*t[1];

    util.open("myfile",1,v,t);        // reload data from disk
    if(strcmp("Head",t[0]))return(1); // cheching type
    h=(Head *)(v[0]);                 // returned key entry
    h->prtAll();                      // print all data
    return(0);
}
#include "zzfunc.c"

// This is a very basic example in C++. We want to place Employees
// in two linked lists (or collections) at the same time. One list will
//  contain all Employees, the other only those that start with B. After
//  setting up both lists, we save all the data file "myFile".
// 
// Several details may help you to understand this program:
// ZZ_EXT_<class_name> is a standard statement that must be in every class
//    involved in the automatic data structures
// ZZ_HYPER_SINGLE_COLLECTION(id,TOP,BOT) declares a singly-linked collection
//    which, in our library, means a singly-linked ring of objects type BOT,
//    with an encapsulated entry stored in the object of type TOP.
// ZZ_HYPER_NAME(id,TYPE) declares a variable length name on object type TYPE
//    Names are treated as objects. There can be any number of names associated
//    with any class.
// ZZ_HYPER_UTILITIES() declares that we will be using memory management or
//    disk saving utilities
// Attributes such as Employee IDnum remain in classes as usual.
// util.save() collects all objects, starting from one or several "key
// entries", and following all the internal pointers. When reloading
// the data (see test2.cc), new key entries provide a convenient handle
// to the new data.
//
// This program does not read any external data. In order to run it, just
// type: test1
// You may want to look at test2.c in this directory. The file reads 
// the data file from the disk, and prints the two linked lists.

#include <stdio.h>
#define ZZmain
#include "zzincl.h"

class Head {
    ZZ_EXT_Head
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

Employee::Employee(int idn, char *name){char *p;
    IDnum=idn; p=util.strAlloc(name); eName.add(this,p);}
void Employee::prt(void){printf("%d %s\n",IDnum,eName.fwd(this));}

int main(void){
    int i; Head *h; Employee *e; char *v[1],*t[1];
    static char * names[4]={"Black","White","Green","Brown"};
   
    h=new Head;
    for(i=0;i<4;i++){
        e=new Employee(i,names[i]);
        list1.add(h,e);
        if(names[i][0]=='B')list2.add(h,e);
    }
    list1_iterator it1(h);
    while(e=it1++)e->prt();
    list2_iterator it2(h);
    while(e=it2++)e->prt();

    v[0]=(char *)h; t[0]="Head";
    util.save("myfile",1,v,t);
    return(0);
}
#include "zzfunc.c"

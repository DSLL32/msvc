// This example traverses the base objects of a heterogenous
// list, and saves/retrieves the list in one command

#include <stdio.h>
#define ZZ_INHERIT
#define ZZmain
#define ZZascii
#include "zzincl.h"

class Root {  // empty class, keeps the entire data organization
    ZZ_EXT_Root
};
ZZ_FORMAT(Root,"");

class Shape {
    ZZ_EXT_Shape
public:
    virtual void prt(void){};
};
ZZ_FORMAT(Shape,"");
    
class Square : public Shape{ 
    int x;
    ZZ_EXT_Square
public:
    void prt(void){printf("Square %d\n",x);};
    void set(int a){x=a;};
};
ZZ_FORMAT(Square,"%d,x");

class Rectangle : public Shape{
    int x,y;
    ZZ_EXT_Rectangle
public:
    void prt(void){printf("Rectangle %dx%d\n",x,y);};
    void set(int a,int b){x=a; y=b;};
};
ZZ_FORMAT(Rectangle,"%d %d,x,y");
ZZ_HYPER_SINGLE_COLLECT(all,Root,Shape);
ZZ_HYPER_UTILITIES(util);
#define BF 80
char buff[BF];

int main(void)
{
    char c[20],*v[1],*t[1];;
    int x,y;
    Root *rt;
    Square *sp;
    Rectangle *rp;
    void prtData(Root *);

    rt=new Root;
    while(gets(buff)){ // read in all input data
        sscanf(buff,"%c",c);
        switch(c[0]) {
            case 'S':
                sp=new(Square);
                sscanf(buff,"%c %d",&c,&x);
                sp->set(x);
                all.add(rt,(Shape *)sp);
                break;
            case 'R':
                rp=new(Rectangle);
                sscanf(buff,"%c %d %d",&c,&x,&y);
                rp->set(x,y);
                all.add(rt,(Shape *)rp);
                break;
            default: printf("wrong input code\n");
        }
    }

    prtData(rt); // print all data as loaded
    v[0]=(char *)rt; t[0]="Root";
    util.save("myFile",1,v,t); // v,t are already set
    util.open("myFile",1,v,t); // returns new v,t
    rt=(Root *)(v[0]);
    prtData(rt); // though in different location, result should be the same
    return(0);
}
//-------------------------------------------------------------------------
void prtData(Root *rr) //print out all data
{
    Shape *pp;

    all_iterator it(rr);
    ITERATE(it,pp){
        pp->prt();
    }
}
#include "zzfunc.c"

// Test of access to the internal type tables and hidden pointers

#include <stdio.h>
#define ZZmain
#include "zzincl.h"

class Coord {
    ZZ_EXT_Coord
    int x[3];
public:
    void virtual ini(void){};
};

class Object {
    int id;
    ZZ_EXT_Object
public:
    void virtual prt(void) = 0;
};

class Shape : virtual public Object {
    ZZ_EXT_Shape
public:
    void virtual prt(){};
};
class Tool : virtual public Object {
    ZZ_EXT_Tool
    int t;
    Coord c;
public:
    void virtual prt(){};
};
class Image: public Tool, public Shape {
    ZZ_EXT_Image
    int toolID;
public:
    void prt(){};
};
ZZ_HYPER_SINGLE_TREE(tree,Image);
ZZ_HYPER_SINGLE_TRIANGLE(col,Shape,Tool);
ZZ_HYPER_TYPE(type);

int main(void){
    Shape *sp;
    Tool *tt;
    Image *ip;
    zzTypeInfo inf;
    int i,sz,tp,t1,t2,n;
    void *vp,**vv;
    char *p,*m,c;

    // Test Part 1: Basic type and casting
    ip=new Image;
    tp=ZZgetType(Shape); // macro to get type index
    sp=(Shape *)ip;
    vp=(void *)sp;

    printf("Shape type=%d\n",tp);
    type.true(&vp,&tp); // updates vp and tp to the true image (Image type)
    type.info(tp,&inf);  // returns info for type tp
    if(vp==(void *)sp)
          printf("new correct pointer, %s type=%d\n",inf.name,tp);

    // Test Part 2: Iterating through sub-classes and members
    type_iterator it; // declare iterator, will be used more times
    n=type.num(); // get total number of registered types
    for(t1=0;t1<n;t1++){ // go through all registered types
        type.info(t1,&inf); // get info on this type, we need the name
        printf("%s= ",inf.name);
        it.start(t1); // start iterator of sub-types
        for(t2= ++it; t2>=0; t2= ++it){ // type=0 is a valid value
            type.info(t2,&inf);
            printf("%s(%d):%s ",it.name(),inf.virt,inf.name);
        }
        printf("\n");
    }
    
    ip=new Image;
    tp=ZZgetType(Image);

    // Test Part 3: Traverse pointers on a given object
    ip=new Image;
    tp=ZZgetType(Image);
    printf("\nImage:\n");
    type_pointers ptrs(ip,tp); // traverse HYPER pointers on sp
    for(vv= ++ptrs,i=0; vv; vv= ++ptrs, i++){
        printf("pointer no.=%d to type=%d\n",i,ptrs.nxtType());
    }
    
    tt=new Tool;
    tp=ZZgetType(Tool);
    printf("\nTool:\n");
    ptrs.start(tt,tp); // traverse HYPER pointers on sp
    for(vv= ++ptrs,i=0; vv; vv= ++ptrs, i++){
        printf("pointer no.=%d to type=%d\n",i,ptrs.nxtType());
    }
    
    // Test Part 4: print hidden virt.fun. pointers within an object
    tp=ZZgetType(Image);
    type.info(tp,&inf);
    sz=inf.size;  // sizeof(Image) could be used instead

    // print the two mask
    printf("\nmask: ");
    for(i=0, m=inf.mask2; i<sz; i++, m++){
        if(*m=='\0')c='0'; else if(*m==1)c='1'; else c=*m;
        printf("%c",c);
    }

    // virtual pointers on the given Image object
    printf("\nobject start=%d size=%d bytes\n",inf.mask1,sz);
    for(i=0, p=inf.mask1, m=inf.mask2; i<sz; i++,p++,m++){
            vv=(void **)p;
        if(*m=='F')printf("virt.fun.ptr at=%d value=%d offset=%d\n",vv,*vv,i);
        if(*m=='C')printf("v.class  ptr at=%d value=%d offset=%d\n",vv,*vv,i);
    }
    return(0);
}
#include "zzfunc.c"

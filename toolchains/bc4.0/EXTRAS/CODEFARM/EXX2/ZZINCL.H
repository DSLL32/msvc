#ifndef zzinclUsed
#define zzinclUsed
#ifdef ZZmain
char *ZZorgcVersion={"Version 3.1"};
#else
extern char *ZZorgcVersion;
#endif
#include "environ.h"
#ifndef FILE
#include <stdio.h>
#endif
#include "C:\CODE\TEST\..\zzcomb.h"
#include "C:\CODE\TEST\..\lib\msgs.h"

class ZZZiClass{};
#ifndef ZZmain
extern
#endif

ZZ_LOCAL char *ZZselfArr[1]
#ifdef ZZmain
={
"ZZendMark"}
#endif
;
#define ZZnID 1

#define ZZ_EXT_Rectangle \
\
public:\
    void *zzThisIs(int *type){*type=0; return((void*)this);};\
    void ZZinitSet(void){ZZS_Rectangle};\
    Rectangle(){};\
   ~Rectangle(){ZZ_CHECK(Rectangle);};\
    Rectangle(ZZZiClass *x):Shape(x){ZZS_Rectangle};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}\
    int zz_inp_Rectangle(FILE *,Rectangle *);\
    int zz_out_Rectangle(FILE *,Rectangle *);\
    void zz_ipt_Rectangle(Rectangle *);\
    void zz_opt_Rectangle(FILE *,Rectangle *);


#define ZZ_EXT_Square \
\
public:\
    void *zzThisIs(int *type){*type=1; return((void*)this);};\
    void ZZinitSet(void){ZZS_Square};\
    Square(){};\
   ~Square(){ZZ_CHECK(Square);};\
    Square(ZZZiClass *x):Shape(x){ZZS_Square};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}\
    int zz_inp_Square(FILE *,Square *);\
    int zz_out_Square(FILE *,Square *);\
    void zz_ipt_Square(Square *);\
    void zz_opt_Square(FILE *,Square *);


#define ZZ_EXT_Shape \
friend class ZZHall;\
    Shape *ZZsall;\
public:\
    virtual void *zzThisIs(int *type){*type=2; return((void*)this);};\
    void ZZinitSet(void){ZZS_Shape};\
    Shape(){};\
   ~Shape(){ZZ_CHECK(Shape);};\
    Shape(ZZZiClass *){ZZS_Shape};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}\
    int zz_inp_Shape(FILE *,Shape *);\
    int zz_out_Shape(FILE *,Shape *);\
    void zz_ipt_Shape(Shape *);\
    void zz_opt_Shape(FILE *,Shape *);


#define ZZ_EXT_Root \
friend class ZZHall;\
    Shape *ZZcall;\
public:\
    void *zzThisIs(int *type){*type=3; return((void*)this);};\
    void ZZinitSet(void){ZZS_Root};\
    Root(){};\
   ~Root(){ZZ_CHECK(Root);};\
    Root(ZZZiClass *){ZZS_Root};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}\
    int zz_inp_Root(FILE *,Root *);\
    int zz_out_Root(FILE *,Root *);\
    void zz_ipt_Root(Root *);\
    void zz_opt_Root(FILE *,Root *);

#define ZZIRectangle ZZZIRectangle()
ZZ_LOCAL int ZZZIRectangle(void);
#define ZZJRectangle ZZZJRectangle()
ZZ_LOCAL int ZZZJRectangle(void);
#define ZZISquare ZZZISquare()
ZZ_LOCAL int ZZZISquare(void);
#define ZZJSquare ZZZJSquare()
ZZ_LOCAL int ZZZJSquare(void);
#define ZZIShape ZZZIShape()
ZZ_LOCAL int ZZZIShape(void);
#define ZZJShape ZZZJShape()
ZZ_LOCAL int ZZZJShape(void);
#define ZZIRoot ZZZIRoot()
ZZ_LOCAL int ZZZIRoot(void);
#define ZZJRoot ZZZJRoot()
ZZ_LOCAL int ZZZJRoot(void);
#define ZZIint ZZZIint()
ZZ_LOCAL int ZZZIint(void);
#define ZZIfloat ZZZIfloat()
ZZ_LOCAL int ZZZIfloat(void);
#define ZZIchar ZZZIchar()
ZZ_LOCAL int ZZZIchar(void);
#ifdef ZZmain
ZZ_LOCAL int ZZZIint(void){return(-6);}
ZZ_LOCAL int ZZZIfloat(void){return(-7);}
ZZ_LOCAL int ZZZIchar(void){return(-1);}

#endif
#ifdef ZZmain

ZZ_LOCAL char *ZZptrList[]={
    "aZZsall[1",
    "aZZcall[1"
};

ZZ_LOCAL struct ZZstrLST ZZstrList[5]={
  {"Rectangle", 0, 0, 0, -1, -1, -1, -1, 0, 0, {0,0}, 0}, 
  {"Square", 0, 0, 0, -1, -1, -1, -1, 0, 1, {0,0}, 0}, 
  {"Shape", 0, 0, 1, -1, -1, -1, -1, 0, 2, {0,0}, 4}, 
  {"Root", 0, 0, 1, -1, -1, -1, -1, 1, 2, {0,0}, 0}, 
  {"ZZendMark", -1, -1, -1, -1, -1, -1, -1, 2, 2, {0, 0}, 0}};

#define ZZstrNum 4

ZZ_LOCAL int ZZnxtType[2]={
 2,
 2
};

ZZ_LOCAL struct ZZtypeHier ZZinhList[3]={
 2, -1, 0, "ZZinherit",
 2, -1, 0, "ZZinherit",
 -1, -1, -1, "ZZendMark"
};
#else
extern char *ZZptrList[];
extern struct ZZstrLST ZZstrList[];
extern int ZZnxtType[];
extern struct ZZtypeHier ZZinhList[];
#endif
#define ZZS_Rectangle {Rectangle *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=this; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
 ZZi=ZZi; ZZstrList[0].start=0;\
  ZZinhList[0].offset=ZZstrList[2].start;\
 ZZstrList[0].size=(int)sizeof(Rectangle);}
#define ZZS_Square {Square *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=this; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
 ZZi=ZZi; ZZstrList[1].start=0;\
  ZZinhList[1].offset=ZZstrList[2].start;\
 ZZstrList[1].size=(int)sizeof(Square);}
#define ZZS_Shape {Shape *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=this; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[2].start=ZZptrToInt((char *)(&(ZZp->ZZsall)))-ZZi;\
 ZZstrList[2].size=(int)sizeof(Shape);}
#define ZZS_Root {Root *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=this; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[3].start=ZZptrToInt((char *)(&(ZZp->ZZcall)))-ZZi;\
 ZZstrList[3].size=(int)sizeof(Root);}
#define ZZ1all Root
#define ZZX1all "Root"
#define ZZW1all 3
#define ZZ2all Shape
#define ZZX2all "Shape"
#define ZZW2all 2
#define ZZVall  



#ifdef ZZmain
ZZ_LOCAL int ZZbindASCII={1};
#ifdef SUN
ZZ_LOCAL int ZZnoLeak={0};
#else
ZZ_LOCAL int ZZnoLeak={1};
#endif
ZZ_LOCAL int ZZinhFLG={1};
ZZ_LOCAL int ZZvClassPresent={0};
ZZ_LOCAL int ZZallocPtr={8};
#endif
#ifndef ZZmain
extern int ZZbindASCII;
extern int ZZnoLeak;
extern int ZZinhFLG;
extern int ZZvClassPresent;
extern int ZZallocPtr;
#endif
class Rectangle;
class Square;
class Shape;
class Root;
#endif

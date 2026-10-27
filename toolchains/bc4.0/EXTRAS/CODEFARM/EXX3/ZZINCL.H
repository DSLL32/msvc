#ifndef zzinclUsed
#define zzinclUsed
#ifdef ZZmain
char *ZZorgcVersion={"Version 3.00"};
#else
extern char *ZZorgcVersion;
#endif
#include "../lib/environ.h"

#include "../zzcomb.h"
#include "../lib/msgs.h"

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

#define ZZ_EXT_A \
friend class ZZHmyRing;\
    A *ZZfmyRing;\
public:\
    void ZZinitSet(void){ZZS_A};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}

#define ZZIA 0
#define ZZJA -1
#define ZZIint   -6
#define ZZIfloat -7
#define ZZIchar  -1

#ifdef ZZmain

ZZ_LOCAL char *ZZptrList[]={
    "aZZfmyRing[1"
};

ZZ_LOCAL struct ZZstrLST ZZstrList[2]={
  "A", 0, 0, 1, -1, -1, -1, -1, 0, 0, 0, 0, 
  "ZZendMark", -1, -1, -1, -1, -1, -1, -1, 1, 0, 0, 0};


ZZ_LOCAL int ZZnxtType[1]={
 0
};

ZZ_LOCAL struct ZZtypeHier ZZinhList[1]={
 -1, -1, -1, "ZZendMark"
};
#else
extern char *ZZptrList[];
extern struct ZZstrLST ZZstrList[];
extern int ZZnxtType[];
extern struct ZZtypeHier ZZinhList[];
#endif
#define ZZS_A {A *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=(A *)NULL; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[0].start=ZZptrToInt((char *)(&(ZZp->ZZfmyRing)))-ZZi;\
 ZZstrList[0].size=(int)sizeof(A);}
#define ZZ1myRing A
#define ZZX1myRing "A"
#define ZZW1myRing 0
#define ZZ2myRing A
#define ZZX2myRing "A"
#define ZZW2myRing 0



#ifdef ZZmain
ZZ_LOCAL int ZZbindASCII={0};
#ifdef SUN
ZZ_LOCAL int ZZnoLeak={0};
#else
ZZ_LOCAL int ZZnoLeak={1};
#endif
ZZ_LOCAL int ZZinhFLG={1};
ZZ_LOCAL int ZZvClassPresent={0};
ZZ_LOCAL int ZZallocPtr={1};
#endif
#ifndef ZZmain
extern int ZZbindASCII;
extern int ZZnoLeak;
extern int ZZinhFLG;
extern int ZZvClassPresent;
extern int ZZallocPtr;
#endif
#ifdef ZZ_ANSI
#ifdef ZZmain
ZZ_LOCAL int ZZioFunction(FILE *fp,int io,int type,char *p){
fp=fp; io=io; type=type; p=p; return(0);};
#endif
#else
#ifdef ZZmain
ZZ_LOCAL int ZZioFunction(fp,io,type,p)
FILE *fp;int io; int type;char *p;{
fp=fp; io=io; type=type; p=p; return(0);};
#endif
#endif
class A;
#endif

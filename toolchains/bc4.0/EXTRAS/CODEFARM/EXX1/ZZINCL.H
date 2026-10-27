#ifndef zzinclUsed
#define zzinclUsed
#ifdef ZZmain
char *ZZorgcVersion={"Version 3.00"};
#else
extern char *ZZorgcVersion;
#endif
#include "C:\CODE\TEST\..\lib\environ.h"
#ifndef FILE
#include <stdio.h>
#endif
#include "C:\CODE\TEST\..\zzcomb.h"
#include "C:\CODE\TEST\..\lib\msgs.h"

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

#define ZZ_EXT_Employee \
friend class ZZHlist1;\
friend class ZZHlist2;\
friend class ZZHeName;\
    char *ZZneName;\
    Employee *ZZslist2;\
    Employee *ZZslist1;\
public:\
    void ZZinitSet(void){ZZS_Employee};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}


#define ZZ_EXT_Head \
friend class ZZHlist1;\
friend class ZZHlist2;\
    Employee *ZZclist2;\
    Employee *ZZclist1;\
public:\
    void ZZinitSet(void){ZZS_Head};\
    void * operator new(unsigned size){void *ZZmassAlloc(int,int);\
           return (char *)(ZZmassAlloc((int)size,1));}\
    void operator delete(void *ptr){void ZZmassFree(void *);\
           ZZmassFree(ptr);}

#define ZZIEmployee 0
#define ZZJEmployee -1
#define ZZIHead 1
#define ZZJHead -1
#define ZZIint   -6
#define ZZIfloat -7
#define ZZIchar  -1

#ifdef ZZmain

ZZ_LOCAL char *ZZptrList[]={
    "aZZneName[1",
    "aZZslist2[1",
    "aZZslist1[1",
    "aZZclist2[1",
    "aZZclist1[1"
};

ZZ_LOCAL struct ZZstrLST ZZstrList[3]={
  "Employee", 0, 0, 3, -1, -1, -1, -1, 0, 0, 0, 0, 
  "Head", 0, 0, 2, -1, -1, -1, -1, 3, 0, 0, 0, 
  "ZZendMark", -1, -1, -1, -1, -1, -1, -1, 5, 0, 0, 0};


ZZ_LOCAL int ZZnxtType[5]={
 -1,0,0,
 0,0
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
#define ZZS_Employee {Employee *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=(Employee *)NULL; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[0].start=ZZptrToInt((char *)(&(ZZp->ZZneName)))-ZZi;\
 ZZstrList[0].size=(int)sizeof(Employee);}
#define ZZS_Head {Head *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=(Head *)NULL; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[1].start=ZZptrToInt((char *)(&(ZZp->ZZclist2)))-ZZi;\
 ZZstrList[1].size=(int)sizeof(Head);}
#define ZZ1eName Employee
#define ZZX1eName "Employee"
#define ZZW1eName 0
#define ZZ1list2 Head
#define ZZX1list2 "Head"
#define ZZW1list2 1
#define ZZ2list2 Employee
#define ZZX2list2 "Employee"
#define ZZW2list2 0
#define ZZ1list1 Head
#define ZZX1list1 "Head"
#define ZZW1list1 1
#define ZZ2list1 Employee
#define ZZX2list1 "Employee"
#define ZZW2list1 0




#ifdef ZZmain
ZZ_LOCAL int ZZbindASCII={0};
#ifdef SUN
ZZ_LOCAL int ZZnoLeak={0};
#else
ZZ_LOCAL int ZZnoLeak={1};
#endif
ZZ_LOCAL int ZZinhFLG={1};
ZZ_LOCAL int ZZvClassPresent={0};
ZZ_LOCAL int ZZallocPtr={10};
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
class Employee;
class Head;
#endif

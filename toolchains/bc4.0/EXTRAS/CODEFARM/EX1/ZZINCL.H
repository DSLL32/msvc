#ifndef zzinclUsed
#define zzinclUsed
#ifdef ZZmain
char *ZZorgcVersion={"Version 3.00"};
#else
extern char *ZZorgcVersion;
#endif
#include "environ.h"
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
    char *ZZneName;\
    Employee *ZZfbyName;\
    Employee *ZZfbySalary;


#define ZZIEmployee 0
#define ZZJEmployee -1
#define ZZIint   -6
#define ZZIfloat -7
#define ZZIchar  -1

#ifdef ZZmain

ZZ_LOCAL char *ZZptrList[]={
    "aZZneName[1",
    "aZZfbyName[1",
    "aZZfbySalary[1"
};

ZZ_LOCAL struct ZZstrLST ZZstrList[2]={
  "Employee", 0, 0, 3, -1, -1, -1, -1, 0, 0, 0, 0, 
  "ZZendMark", -1, -1, -1, -1, -1, -1, -1, 3, 0, 0, 0};


ZZ_LOCAL int ZZnxtType[3]={
 -1,0,0
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
#define ZZ1eName Employee
#define ZZX1eName "Employee"
#define ZZW1eName 0
#define ZZVeName  
#define ZZ1byName Employee
#define ZZX1byName "Employee"
#define ZZW1byName 0
#define ZZVbyName  
#define ZZ1bySalary Employee
#define ZZX1bySalary "Employee"
#define ZZW1bySalary 0
#define ZZVbySalary  

#define ZZ_A_TRAVERSE_byName ZZ_A_TRAVERSE_SINGLE_RING
#define ZZ_FORWARD_eName ZZ_FORWARD_NAME
#define ZZ_A_TRAVERSE_bySalary ZZ_A_TRAVERSE_SINGLE_RING
#define ZZ_SORT_byName ZZ_SORT_SINGLE_RING
#define ZZ_SORT_bySalary ZZ_SORT_SINGLE_RING
#define ZZ_ADD_bySalary ZZ_ADD_SINGLE_RING
#define ZZ_ADD_byName ZZ_ADD_SINGLE_RING
#define ZZ_ADD_eName ZZ_ADD_NAME


#ifdef ZZmain
ZZ_LOCAL int ZZbindASCII={0};
#ifdef SUN
ZZ_LOCAL int ZZnoLeak={0};
#else
ZZ_LOCAL int ZZnoLeak={1};
#endif
ZZ_LOCAL int ZZinhFLG={0};
ZZ_LOCAL int ZZvClassPresent={0};
ZZ_LOCAL int ZZallocPtr={3};
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
#endif

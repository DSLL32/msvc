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

#define ZZ_EXT_Pin \
    Pin *ZZsbyNet;\
    Pin *ZZsbyBlock;\
    Net *ZZpbyNet;\
    Block *ZZpbyBlock;



#define ZZ_EXT_Net \
    char *ZZnnName;\
    Pin *ZZcbyNet;\
    Net *ZZfnChain;



#define ZZ_EXT_Block \
    char *ZZnbName;\
    Pin *ZZcbyBlock;\
    Block *ZZfbChain;


#define ZZIPin 0
#define ZZJPin -1
#define ZZINet 1
#define ZZJNet -1
#define ZZIBlock 2
#define ZZJBlock -1
#define ZZIint   -6
#define ZZIfloat -7
#define ZZIchar  -1

#ifdef ZZmain

ZZ_LOCAL char *ZZptrList[]={
    "aZZsbyNet[1",
    "aZZsbyBlock[1",
    "aZZpbyNet[1",
    "aZZpbyBlock[1",
    "aZZnnName[1",
    "aZZcbyNet[1",
    "aZZfnChain[1",
    "aZZnbName[1",
    "aZZcbyBlock[1",
    "aZZfbChain[1"
};

ZZ_LOCAL struct ZZstrLST ZZstrList[4]={
  "Pin", 0, 0, 4, -1, -1, -1, -1, 0, 0, 0, 0, 
  "Net", 0, 0, 3, -1, -1, -1, -1, 4, 0, 0, 0, 
  "Block", 0, 0, 3, -1, -1, -1, -1, 7, 0, 0, 0, 
  "ZZendMark", -1, -1, -1, -1, -1, -1, -1, 10, 0, 0, 0};


ZZ_LOCAL int ZZnxtType[10]={
 0,0,1,2,
 -1,0,1,
 -1,0,2
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
#define ZZS_Pin {Pin *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=(Pin *)NULL; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[0].start=ZZptrToInt((char *)(&(ZZp->ZZsbyNet)))-ZZi;\
 ZZstrList[0].size=(int)sizeof(Pin);}
#define ZZS_Net {Net *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=(Net *)NULL; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[1].start=ZZptrToInt((char *)(&(ZZp->ZZnnName)))-ZZi;\
 ZZstrList[1].size=(int)sizeof(Net);}
#define ZZS_Block {Block *ZZp; int ZZptrToInt(char *pp),ZZi;\
 ZZp=(Block *)NULL; ZZp=ZZp;\
ZZi=ZZptrToInt((char *)ZZp);\
ZZstrList[2].start=ZZptrToInt((char *)(&(ZZp->ZZnbName)))-ZZi;\
 ZZstrList[2].size=(int)sizeof(Block);}
#define ZZ1nName Net
#define ZZX1nName "Net"
#define ZZW1nName 1
#define ZZVnName  
#define ZZ1bName Block
#define ZZX1bName "Block"
#define ZZW1bName 2
#define ZZVbName  
#define ZZ1byNet Net
#define ZZX1byNet "Net"
#define ZZW1byNet 1
#define ZZ2byNet Pin
#define ZZX2byNet "Pin"
#define ZZW2byNet 0
#define ZZVbyNet  
#define ZZ1byBlock Block
#define ZZX1byBlock "Block"
#define ZZW1byBlock 2
#define ZZ2byBlock Pin
#define ZZX2byBlock "Pin"
#define ZZW2byBlock 0
#define ZZVbyBlock  
#define ZZ1nChain Net
#define ZZX1nChain "Net"
#define ZZW1nChain 1
#define ZZVnChain  
#define ZZ1bChain Block
#define ZZX1bChain "Block"
#define ZZW1bChain 2
#define ZZVbChain  

#define ZZ_PARENT_byBlock ZZ_PARENT_SINGLE_TRIANGLE
#define ZZ_A_TRAVERSE_byNet ZZ_A_TRAVERSE_SINGLE_TRIANGLE
#define ZZ_PARENT_byNet ZZ_PARENT_SINGLE_TRIANGLE
#define ZZ_A_TRAVERSE_byBlock ZZ_A_TRAVERSE_SINGLE_TRIANGLE
#define ZZ_FORWARD_bName ZZ_FORWARD_NAME
#define ZZ_A_TRAVERSE_bChain ZZ_A_TRAVERSE_SINGLE_RING
#define ZZ_ADD_byNet ZZ_ADD_SINGLE_TRIANGLE
#define ZZ_ADD_byBlock ZZ_ADD_SINGLE_TRIANGLE
#define ZZ_ADD_nChain ZZ_ADD_SINGLE_RING
#define ZZ_ADD_nName ZZ_ADD_NAME
#define ZZ_FORWARD_nName ZZ_FORWARD_NAME
#define ZZ_ADD_bChain ZZ_ADD_SINGLE_RING
#define ZZ_ADD_bName ZZ_ADD_NAME


#ifdef ZZmain
ZZ_LOCAL int ZZbindASCII={1};
#ifdef SUN
ZZ_LOCAL int ZZnoLeak={0};
#else
ZZ_LOCAL int ZZnoLeak={1};
#endif
ZZ_LOCAL int ZZinhFLG={0};
ZZ_LOCAL int ZZvClassPresent={0};
ZZ_LOCAL int ZZallocPtr={30};
#endif
#ifndef ZZmain
extern int ZZbindASCII;
extern int ZZnoLeak;
extern int ZZinhFLG;
extern int ZZvClassPresent;
extern int ZZallocPtr;
#endif
#endif

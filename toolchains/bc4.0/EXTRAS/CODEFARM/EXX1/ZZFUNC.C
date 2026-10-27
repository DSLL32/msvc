#include "zzincl.h"
#ifdef FLOATLINK
#include <math.h>
#endif
void ZZFP(ZZH,util) :: save(char *file,int n,char **v, char **t){
                                            ZZ_SAVE(file,n,v,t);};
void ZZFP(ZZH,util) :: deep(char *file,void *v, char *t){ZZ_DEEP(file,v,t);};
void ZZFP(ZZH,util) :: open(char *file,int n,char **v, char **t){
                                            ZZ_OPEN(file,n,v,t);};
void ZZFP(ZZH,util) :: clear(int n,char **v, char **t){ZZ_CLEAR(n,v,t);};
char* ZZFP(ZZH,util) :: getID(char *p){char *n;ZZ_GET_SELF_ID(p,n);return(n);};
void ZZHlist2 :: add(ZZ1list2 *s,ZZ2list2 *t){ZZ_ADD_SINGLE_COLLECT(list2,s,t); };
void ZZHlist2 :: del(ZZ1list2 *p,ZZ2list2 *c){ZZ_DELETE_SINGLE_COLLECT(list2,p,c); };
void ZZHlist2 :: merge(ZZ2list2 *s,ZZ2list2 *t,ZZ1list2 *p){ZZ_MERGE_SINGLE_COLLECT(list2,s,t,p); };
void ZZHlist2 :: sort(ZZTsortFun cmpF,ZZ1list2 *p){ZZ_SORT_SINGLE_COLLECT(list2,cmpF,p);};
void ZZHlist1 :: add(ZZ1list1 *s,ZZ2list1 *t){ZZ_ADD_SINGLE_COLLECT(list1,s,t); };
void ZZHlist1 :: del(ZZ1list1 *p,ZZ2list1 *c){ZZ_DELETE_SINGLE_COLLECT(list1,p,c); };
void ZZHlist1 :: merge(ZZ2list1 *s,ZZ2list1 *t,ZZ1list1 *p){ZZ_MERGE_SINGLE_COLLECT(list1,s,t,p); };
void ZZHlist1 :: sort(ZZTsortFun cmpF,ZZ1list1 *p){ZZ_SORT_SINGLE_COLLECT(list1,cmpF,p);};

ZZ_LOCAL void ZZ_GENERAL_EXT(void){
  char *p;
  void ZZsetMask(void);
 int i,ZZptrToInt(char *);
  static int repeat={0};
#ifdef FLOATLINK
double x; x=sin(0.0); x=x;
#endif
#ifdef ZORTECH
{void dumbZortechLinker(); dumbZortechLinker();}
#endif
  if(repeat)return;else repeat=1;
  ZZorgcVersion=ZZorgcVersion;
  p=NULL;
  p=p;
 ZZsList=ZZstrList;
  ZZpList= &(ZZptrList[0]);
  ZZiList= &(ZZinhList[0]);
  if(!(ZZstrList[0].vObj))
    ZZstrList[0].vObj=(char *)((Employee *)(new Employee));
  if(!(ZZstrList[1].vObj))
    ZZstrList[1].vObj=(char *)((Head *)(new Head));
for(i=0;i<2;i++){
  ((Head *)(ZZstrList[1].vObj))->ZZinitSet();
  ((Employee *)(ZZstrList[0].vObj))->ZZinitSet();
}
ZZsetMask();
#ifdef ZZcCalloc
ZZcallocFlg=1;
#else
ZZcallocFlg=0;
#endif
}

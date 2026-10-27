#include "zzincl.h"
#ifdef FLOATLINK
#include <math.h>
#endif
ZZ1myRing* ZZHmyRing :: add(ZZ1myRing *s,ZZ1myRing *t){ZZ_ADD_SINGLE_RING(myRing,s,t); return(s);};
ZZ1myRing* ZZHmyRing :: del(ZZ1myRing *s,ZZ1myRing *t){ZZ_DELETE_SINGLE_RING(myRing,s,t); return(s);};
ZZ1myRing* ZZHmyRing :: merge(ZZ1myRing *s,ZZ1myRing *t){
                               ZZ_MERGE_SINGLE_RING(myRing,s,t,NULL); return(s);};
ZZ1myRing* ZZHmyRing :: sort(ZZTsortFun cmpF,ZZ1myRing *p){
                                 ZZ_SORT_SINGLE_RING(myRing,cmpF,p); return(p);};

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
    ZZstrList[0].vObj=(char *)((A *)(new A));
for(i=0;i<2;i++){
  ((A *)(ZZstrList[0].vObj))->ZZinitSet();
}
ZZsetMask();
#ifdef ZZcCalloc
ZZcallocFlg=1;
#else
ZZcallocFlg=0;
#endif
}

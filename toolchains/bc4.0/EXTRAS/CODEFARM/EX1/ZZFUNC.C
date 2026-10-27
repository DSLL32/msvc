#include "zzincl.h"
#ifdef FLOATLINK
#include <math.h>
#endif

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
for(i=0;i<2;i++){
  ZZS_Employee;
}
ZZsetMask();
#ifdef ZZcCalloc
ZZcallocFlg=1;
#else
ZZcallocFlg=0;
#endif
}

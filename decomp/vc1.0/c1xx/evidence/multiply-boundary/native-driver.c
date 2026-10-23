#include <stdio.h>
extern int __attribute__((stdcall)) multiplication_overflows(unsigned,unsigned,short,unsigned short);
static unsigned v[][4]={{0U,4U,0U,4U},{255U,2U,1U,1U},{4294967168U,1U,0U,1U},{4294967168U,4294967295U,0U,1U},{4294934528U,1U,0U,2U},{4294934528U,4294967295U,0U,2U},{2147483648U,1U,0U,4U},{2147483648U,4294967295U,0U,4U},{2147483647U,2U,0U,4U},{4294967295U,2U,1U,4U}};
int main(int argc,char **argv){unsigned i;int r;if(argc>1){multiplication_overflows(1,~0U,0,3);return 125;}for(i=0;i<sizeof(v)/sizeof(v[0]);++i){r=multiplication_overflows(v[i][0],v[i][1],(short)v[i][2],(unsigned short)v[i][3]);printf("result %u %08x %08x %u %u %d\n",i,v[i][0],v[i][1],v[i][2],v[i][3],r);}return 0;}

#include "zzincl.h"
#ifdef FLOATLINK
#include <math.h>
#endif

int ZZioFunction(FILE *fp,int io,int type,char *p)
{
  int zz_out_Pin(FILE *f,Pin *ptr),zz_inp_Pin(FILE *f,Pin *ptr);
  void zz_opt_Pin(FILE *f,Pin *ptr),zz_ipt_Pin(Pin *ptr);
  int zz_out_Net(FILE *f,Net *ptr),zz_inp_Net(FILE *f,Net *ptr);
  void zz_opt_Net(FILE *f,Net *ptr),zz_ipt_Net(Net *ptr);
  int zz_out_Block(FILE *f,Block *ptr),zz_inp_Block(FILE *f,Block *ptr);
  void zz_opt_Block(FILE *f,Block *ptr),zz_ipt_Block(Block *ptr);
  /* Control of this function:
   *  io==1 prints the whole object;
   *  io==0 reads the whole object (pointers and zz_inp_..())
   *  io==2 reads only the attributes (call to zz_inp_..()).
   *  io==3 debugging print
   */

#ifdef ZZ_ANSI
  void *ZZnewObj(int size,int ini);
#else
  void *ZZnewObj();
#endif
  static char remChar=' ';
  char **s;
  int skipPtr,i;

  /* decide when to skip printing pointers */
  skipPtr=0;
  if(type>=0){
      if(ZZstrList[type].numPtr<=0 && ZZstrList[type].timeIndex<0)skipPtr=1;
  }

  /* note everywhere here, fgets cannot handle 0-length string */
  if(io==0 && !skipPtr){if(!fgets(ZZbuff,ZZ_BUFS,fp))return(1);}

  switch(type){
    case -8: /* block of general bytes */
      if(io==1)fprintf(fp,"%u\n",*p);
      else {
         sscanf(ZZbuff,"%u",&i);
         *p=i;
      }
      break;

    case -7: /* float or an array of these */
      if(io==1)fprintf(fp,"%f\n",*((float *)p));
      else {
         sscanf(ZZbuff,"%f",(float *)p);
      }
      break;

    case -6: /* int or an array of these */
      if(io==1)fprintf(fp,"%d\n",*((int *)p));
      else {
         sscanf(ZZbuff,"%d",(int *)p);
      }
      break;

    case -5: /* char * or an array of these */
      s=(char**)p;
#ifdef LARGE_POINTER
      if(io==1)fprintf(fp,"%lu\n",*s);
      else {
          sscanf(ZZbuff,"%lu",s);
      }
#else
      if(io==1)fprintf(fp,"%d\n",*s);
      else {
          sscanf(ZZbuff,"%d",s);
      }
#endif
      break;

    case -4: /* array header */
      if(ZZnTbType>=9){
          if(io==1){
#ifdef LARGE_POINTER
            fprintf(fp,"%d %d %d %d %d %d %lu\n",
#else
            fprintf(fp,"%d %d %d %d %d %d %d\n",
#endif
              (((ZZarray *)p)->num),
              (((ZZarray *)p)->waterM),
              (((ZZarray *)p)->strType),
              (((ZZarray *)p)->size),
              (((ZZarray *)p)->incr),
              (((ZZarray *)p)->hshType),
              (((ZZarray *)p)->array));
          }
          else {
#ifdef LARGE_POINTER
            sscanf(ZZbuff,"%d %d %d %d %d %d %lu",
#else
            sscanf(ZZbuff,"%d %d %d %d %d %d %d",
#endif
              &(((ZZarray *)p)->num),
              &(((ZZarray *)p)->waterM),
              &(((ZZarray *)p)->strType),
              &(((ZZarray *)p)->size),
              &(((ZZarray *)p)->incr),
              &(((ZZarray *)p)->hshType),
              &(((ZZarray *)p)->array));
          }
      }
      else {  /* previous versions did not have hshType */
          if(io==1){
#ifdef LARGE_POINTER
            fprintf(fp,"%d %d %d %d %d %lu\n",
#else
            fprintf(fp,"%d %d %d %d %d %d\n",
#endif
              (((ZZarray *)p)->num),
              (((ZZarray *)p)->waterM),
              (((ZZarray *)p)->strType),
              (((ZZarray *)p)->size),
              (((ZZarray *)p)->incr),
              (((ZZarray *)p)->array));
          }
          else {
#ifdef LARGE_POINTER
            sscanf(ZZbuff,"%d %d %d %d %d %lu",
#else
            sscanf(ZZbuff,"%d %d %d %d %d %d",
#endif
              &(((ZZarray *)p)->num),
              &(((ZZarray *)p)->waterM),
              &(((ZZarray *)p)->strType),
              &(((ZZarray *)p)->size),
              &(((ZZarray *)p)->incr),
              &(((ZZarray *)p)->array));
          }
      }
      break;

    case -3: /* property array */
      if(io==1){
#ifdef LARGE_POINTER
        fprintf(fp,"%d %lu\n",
#else
        fprintf(fp,"%d %d\n",
#endif
          (((struct ZZaProp *)p)->size),
          (((struct ZZaProp *)p)->array));
      }
      else {
#ifdef LARGE_POINTER
        sscanf(ZZbuff,"%d %lu",
#else
        sscanf(ZZbuff,"%d %d",
#endif
          &(((struct ZZaProp *)p)->size),
          &(((struct ZZaProp *)p)->array));
      }
      break;

    case -2: /* property header */
      if(remChar=='c'){
        if(io==1){
#ifdef LARGE_POINTER
          fprintf(fp,"%lu %lu %c\n",
#else
          fprintf(fp,"%d %d %c\n",
#endif
            (((struct ZZproperty *)p)->codeName),
            (((struct ZZproperty *)p)->next),
            (((struct ZZproperty *)p)->propType.cProp[0]));
        }
        else {
#ifdef LARGE_POINTER
          sscanf(ZZbuff,"%lu %lu %c",
#else
          sscanf(ZZbuff,"%d %d %c",
#endif
          &(((struct ZZproperty *)p)->codeName),
          &(((struct ZZproperty *)p)->next),
          &(((struct ZZproperty *)p)->propType.cProp[0]));
        }
      }
      else if(remChar=='f'){
        if(io==1){
#ifdef LARGE_POINTER
          fprintf(fp,"%lu %lu %f\n",
#else
          fprintf(fp,"%d %d %f\n",
#endif
            (((struct ZZproperty *)p)->codeName),
            (((struct ZZproperty *)p)->next),
            (((struct ZZproperty *)p)->propType.fProp));
        }
        else {
#ifdef LARGE_POINTER
          sscanf(ZZbuff,"%lu %lu %f",
#else
          sscanf(ZZbuff,"%d %d %f",
#endif
            &(((struct ZZproperty *)p)->codeName),
            &(((struct ZZproperty *)p)->next),
            &(((struct ZZproperty *)p)->propType.fProp));
        }
      }
      else if(remChar=='i'){
        if(io==1){
#ifdef LARGE_POINTER
          fprintf(fp,"%lu %lu %d\n",
#else
          fprintf(fp,"%d %d %d\n",
#endif
            (((struct ZZproperty *)p)->codeName),
            (((struct ZZproperty *)p)->next),
            (((struct ZZproperty *)p)->propType.iProp));
        }
        else {
#ifdef LARGE_POINTER
          sscanf(ZZbuff,"%lu %lu %d",
#else
          sscanf(ZZbuff,"%d %d %d",
#endif
            &(((struct ZZproperty *)p)->codeName),
            &(((struct ZZproperty *)p)->next),
            &(((struct ZZproperty *)p)->propType.iProp));
        }
      }
      else {
        if(io==1){
#ifdef LARGE_POINTER
          fprintf(fp,"%lu %lu %lu\n",
#else
          fprintf(fp,"%d %d %d\n",
#endif
            (((struct ZZproperty *)p)->codeName),
            (((struct ZZproperty *)p)->next),
            (((struct ZZproperty *)p)->propType.arrayProp));
        }
        else {
#ifdef LARGE_POINTER
          sscanf(ZZbuff,"%lu %lu %lu",
#else
          sscanf(ZZbuff,"%d %d %d",
#endif
            &(((struct ZZproperty *)p)->codeName),
            &(((struct ZZproperty *)p)->next),
            &(((struct ZZproperty *)p)->propType.arrayProp));
        }
      }
      break;

    case -1: /* text string - usually a name */
      if(io==1)fprintf(fp,"%s\n",p);
      else {
        if(ZZbuff[0]=='\n')strcpy(p,""); /* empty string */
        else sscanf(ZZbuff,"%s",p);
      }
      /* remember the first character, which is important for properties */
      remChar=p[0];
      break;

    case 0:
      if(io==1)zz_opt_Pin(fp,(Pin *)p);
      if(io==1 || io==3)zz_out_Pin(fp,(Pin *)p);
      else {
          if(io==0)zz_ipt_Pin((Pin *)p);
         if(zz_inp_Pin(fp,(Pin *)p))return(1);
      }
      break;

    case 1:
      if(io==1)zz_opt_Net(fp,(Net *)p);
      if(io==1 || io==3)zz_out_Net(fp,(Net *)p);
      else {
          if(io==0)zz_ipt_Net((Net *)p);
         if(zz_inp_Net(fp,(Net *)p))return(1);
      }
      break;

    case 2:
      if(io==1)zz_opt_Block(fp,(Block *)p);
      if(io==1 || io==3)zz_out_Block(fp,(Block *)p);
      else {
          if(io==0)zz_ipt_Block((Block *)p);
         if(zz_inp_Block(fp,(Block *)p))return(1);
      }
      break;

    default: printf("ASCII IO wrong type=%d\n",type); break;
  }
return(0);
}

int zz_inp_Pin(FILE *fp,Pin *p)
{
    if(!fgets(ZZbuff,ZZ_BUFS,fp))return(1);
    sscanf(ZZbuff,"%d %d %d %d",&(p->x1),&(p->y2),&(p->x2),&(p->y2));
    return(0);
}

int zz_inp_Net(FILE *fp,Net *p)
{
    fp=fp; p=p;
    return(0);
}

int zz_inp_Block(FILE *fp,Block *p)
{
    if(!fgets(ZZbuff,ZZ_BUFS,fp))return(1);
    sscanf(ZZbuff,"%d %d %d %d",&(p->x1),&(p->y2),&(p->x2),&(p->y2));
    return(0);
}

int zz_out_Pin(FILE *fp,Pin *p)
{
    if(ZZdebFLG)fprintf(fp," x1=%d y2=%d x2=%d y2=%d\n",p->x1,p->y2,p->x2,p->y2);
    else fprintf(fp,"%d %d %d %d\n",p->x1,p->y2,p->x2,p->y2);
    return(0);
}

int zz_out_Net(FILE *fp,Net *p)
{
    fp=fp; p=p;
    return(0);
}

int zz_out_Block(FILE *fp,Block *p)
{
    if(ZZdebFLG)fprintf(fp," x1=%d y2=%d x2=%d y2=%d\n",p->x1,p->y2,p->x2,p->y2);
    else fprintf(fp,"%d %d %d %d\n",p->x1,p->y2,p->x2,p->y2);
    return(0);
}

void zz_ipt_Pin(Pin *p){
#ifdef LARGE_POINTER
        sscanf(ZZbuff,"%lu %lu %lu %lu",
#else
        sscanf(ZZbuff,"%d %d %d %d",
#endif
          &(((Pin *)p)->ZZsbyNet),
          &(((Pin *)p)->ZZsbyBlock),
          &(((Pin *)p)->ZZpbyNet),
          &(((Pin *)p)->ZZpbyBlock));
}

void zz_ipt_Net(Net *p){
#ifdef LARGE_POINTER
        sscanf(ZZbuff,"%lu %lu %lu",
#else
        sscanf(ZZbuff,"%d %d %d",
#endif
          &(((Net *)p)->ZZnnName),
          &(((Net *)p)->ZZcbyNet),
          &(((Net *)p)->ZZfnChain));
}

void zz_ipt_Block(Block *p){
#ifdef LARGE_POINTER
        sscanf(ZZbuff,"%lu %lu %lu",
#else
        sscanf(ZZbuff,"%d %d %d",
#endif
          &(((Block *)p)->ZZnbName),
          &(((Block *)p)->ZZcbyBlock),
          &(((Block *)p)->ZZfbChain));
}

void zz_opt_Pin(FILE *fp,Pin *p){
#ifdef LARGE_POINTER
        fprintf(fp,"%lu %lu %lu %lu\n",
#else
        fprintf(fp,"%d %d %d %d\n",
#endif
          (((Pin *)p)->ZZsbyNet),
          (((Pin *)p)->ZZsbyBlock),
          (((Pin *)p)->ZZpbyNet),
          (((Pin *)p)->ZZpbyBlock));
}

void zz_opt_Net(FILE *fp,Net *p){
#ifdef LARGE_POINTER
        fprintf(fp,"%lu %lu %lu\n",
#else
        fprintf(fp,"%d %d %d\n",
#endif
          (((Net *)p)->ZZnnName),
          (((Net *)p)->ZZcbyNet),
          (((Net *)p)->ZZfnChain));
}

void zz_opt_Block(FILE *fp,Block *p){
#ifdef LARGE_POINTER
        fprintf(fp,"%lu %lu %lu\n",
#else
        fprintf(fp,"%d %d %d\n",
#endif
          (((Block *)p)->ZZnbName),
          (((Block *)p)->ZZcbyBlock),
          (((Block *)p)->ZZfbChain));
}

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
  ZZS_Block;
  ZZS_Net;
  ZZS_Pin;
}
ZZsetMask();
#ifdef ZZcCalloc
ZZcallocFlg=1;
#else
ZZcallocFlg=0;
#endif
}

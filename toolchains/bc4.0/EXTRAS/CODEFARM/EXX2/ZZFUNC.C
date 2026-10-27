#include "zzincl.h"
#ifdef FLOATLINK
#include <math.h>
#endif

int ZZioFunction(FILE *fp,int io,int type,char *p)
{
  int zz_out_Rectangle(FILE *f,Rectangle *ptr),zz_inp_Rectangle(FILE *f,Rectangle *ptr);
  void zz_opt_Rectangle(FILE *f,Rectangle *ptr),zz_ipt_Rectangle(Rectangle *ptr);
  int zz_out_Square(FILE *f,Square *ptr),zz_inp_Square(FILE *f,Square *ptr);
  void zz_opt_Square(FILE *f,Square *ptr),zz_ipt_Square(Square *ptr);
  int zz_out_Shape(FILE *f,Shape *ptr),zz_inp_Shape(FILE *f,Shape *ptr);
  void zz_opt_Shape(FILE *f,Shape *ptr),zz_ipt_Shape(Shape *ptr);
  int zz_out_Root(FILE *f,Root *ptr),zz_inp_Root(FILE *f,Root *ptr);
  void zz_opt_Root(FILE *f,Root *ptr),zz_ipt_Root(Root *ptr);
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
  char **s,*t;
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
        for(t=ZZbuff; *t!='\n'; t++)continue;
        t[0]='\0';
        strcpy(p,ZZbuff);
      }
      /* remember the first character, which is important for properties */
      remChar=p[0];
      break;

    case 0:
      if(io==1)((Rectangle *)p)->zz_opt_Rectangle(fp,(Rectangle *)p);
      if(io==1 || io==3)((Rectangle *)p)->zz_out_Rectangle(fp,(Rectangle *)p);
      else {
          if(io==0)((Rectangle *)p)->zz_ipt_Rectangle((Rectangle *)p);
         if(((Rectangle *)p)->zz_inp_Rectangle(fp,(Rectangle *)p))return(1);
      }
      break;

    case 1:
      if(io==1)((Square *)p)->zz_opt_Square(fp,(Square *)p);
      if(io==1 || io==3)((Square *)p)->zz_out_Square(fp,(Square *)p);
      else {
          if(io==0)((Square *)p)->zz_ipt_Square((Square *)p);
         if(((Square *)p)->zz_inp_Square(fp,(Square *)p))return(1);
      }
      break;

    case 2:
      if(io==1)((Shape *)p)->zz_opt_Shape(fp,(Shape *)p);
      if(io==1 || io==3)((Shape *)p)->zz_out_Shape(fp,(Shape *)p);
      else {
          if(io==0)((Shape *)p)->zz_ipt_Shape((Shape *)p);
         if(((Shape *)p)->zz_inp_Shape(fp,(Shape *)p))return(1);
      }
      break;

    case 3:
      if(io==1)((Root *)p)->zz_opt_Root(fp,(Root *)p);
      if(io==1 || io==3)((Root *)p)->zz_out_Root(fp,(Root *)p);
      else {
          if(io==0)((Root *)p)->zz_ipt_Root((Root *)p);
         if(((Root *)p)->zz_inp_Root(fp,(Root *)p))return(1);
      }
      break;

    default: printf("ASCII IO wrong type=%d\n",type); break;
  }
return(0);
}

int Rectangle::zz_inp_Rectangle(FILE *fp,Rectangle *p)
{
    if(!fgets(ZZbuff,ZZ_BUFS,fp))return(1);
    sscanf(ZZbuff,"%d %d",&(p->x),&(p->y));
    return(0);
}

int Square::zz_inp_Square(FILE *fp,Square *p)
{
    if(!fgets(ZZbuff,ZZ_BUFS,fp))return(1);
    sscanf(ZZbuff,"%d",&(p->x));
    return(0);
}

int Shape::zz_inp_Shape(FILE *fp,Shape *p)
{
    fp=fp; p=p;
    return(0);
}

int Root::zz_inp_Root(FILE *fp,Root *p)
{
    fp=fp; p=p;
    return(0);
}

int Rectangle::zz_out_Rectangle(FILE *fp,Rectangle *p)
{
    if(ZZdebFLG)fprintf(fp," x=%d y=%d\n",p->x,p->y);
    else fprintf(fp,"%d %d\n",p->x,p->y);
    return(0);
}

int Square::zz_out_Square(FILE *fp,Square *p)
{
    if(ZZdebFLG)fprintf(fp," x=%d\n",p->x);
    else fprintf(fp,"%d\n",p->x);
    return(0);
}

int Shape::zz_out_Shape(FILE *fp,Shape *p)
{
    fp=fp; p=p;
    return(0);
}

int Root::zz_out_Root(FILE *fp,Root *p)
{
    fp=fp; p=p;
    return(0);
}

void Rectangle::zz_ipt_Rectangle(Rectangle *p){
#ifdef LARGE_POINTER
#else
#endif
 p=p;
}

void Square::zz_ipt_Square(Square *p){
#ifdef LARGE_POINTER
#else
#endif
 p=p;
}

void Shape::zz_ipt_Shape(Shape *p){
#ifdef LARGE_POINTER
        sscanf(ZZbuff,"%lu",
#else
        sscanf(ZZbuff,"%d",
#endif
          &(((Shape *)p)->ZZsall));
}

void Root::zz_ipt_Root(Root *p){
#ifdef LARGE_POINTER
        sscanf(ZZbuff,"%lu",
#else
        sscanf(ZZbuff,"%d",
#endif
          &(((Root *)p)->ZZcall));
}

void Rectangle::zz_opt_Rectangle(FILE *fp,Rectangle *p){
#ifdef LARGE_POINTER
#else
#endif
 fp=fp; p=p;
}

void Square::zz_opt_Square(FILE *fp,Square *p){
#ifdef LARGE_POINTER
#else
#endif
 fp=fp; p=p;
}

void Shape::zz_opt_Shape(FILE *fp,Shape *p){
#ifdef LARGE_POINTER
        fprintf(fp,"%lu\n",
#else
        fprintf(fp,"%d\n",
#endif
          (((Shape *)p)->ZZsall));
}

void Root::zz_opt_Root(FILE *fp,Root *p){
#ifdef LARGE_POINTER
        fprintf(fp,"%lu\n",
#else
        fprintf(fp,"%d\n",
#endif
          (((Root *)p)->ZZcall));
}
void ZZFP(ZZH,util) :: save(char *file,int n,char **v, char **t){
                                            ZZ_SAVE(file,n,v,t);};
void ZZFP(ZZH,util) :: deep(char *file,void *v, char *t){ZZ_DEEP(file,v,t);};
void ZZFP(ZZH,util) :: open(char *file,int n,char **v, char **t){
                                            ZZ_OPEN(file,n,v,t);};
void ZZFP(ZZH,util) :: clear(int n,char **v, char **t){ZZ_CLEAR(n,v,t);};
char* ZZFP(ZZH,util) :: getID(char *p){char *n;ZZ_GET_SELF_ID(p,n);return(n);};
void ZZVall ZZHall :: add(ZZ1all *s,ZZ2all *t){ZZ_ADD_SINGLE_COLLECT(all,s,t); };
void ZZVall ZZHall :: del(ZZ1all *p,ZZ2all *c){ZZ_DELETE_SINGLE_COLLECT(all,p,c); };
void ZZVall ZZHall :: merge(ZZ2all *s,ZZ2all *t,ZZ1all *p){ZZ_MERGE_SINGLE_COLLECT(all,s,t,p); };
void ZZVall ZZHall :: sort(ZZTsortFun cmpF,ZZ1all *p){ZZ_SORT_SINGLE_COLLECT(all,cmpF,p);};

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
  if(!(ZZstrList[0].vObj[0])){
    ZZstrList[0].vObj[0]=(char *)(new Rectangle((ZZZiClass *)NULL));
    ZZstrList[0].vObj[1]=(char *)(new Rectangle((ZZZiClass *)NULL));
  }
  if(!(ZZstrList[1].vObj[0])){
    ZZstrList[1].vObj[0]=(char *)(new Square((ZZZiClass *)NULL));
    ZZstrList[1].vObj[1]=(char *)(new Square((ZZZiClass *)NULL));
  }
  if(!(ZZstrList[2].vObj[0])){
    ZZstrList[2].vObj[0]=(char *)(new Shape((ZZZiClass *)NULL));
    ZZstrList[2].vObj[1]=(char *)(new Shape((ZZZiClass *)NULL));
  }
  if(!(ZZstrList[3].vObj[0])){
    ZZstrList[3].vObj[0]=(char *)(new Root((ZZZiClass *)NULL));
    ZZstrList[3].vObj[1]=(char *)(new Root((ZZZiClass *)NULL));
  }
for(i=0;i<2;i++){
  ((Root *)(ZZstrList[3].vObj[0]))->ZZinitSet();
  ((Shape *)(ZZstrList[2].vObj[0]))->ZZinitSet();
  ((Square *)(ZZstrList[1].vObj[0]))->ZZinitSet();
  ((Rectangle *)(ZZstrList[0].vObj[0]))->ZZinitSet();
}
ZZsetMask();
#ifdef ZZcCalloc
ZZcallocFlg=1;
#else
ZZcallocFlg=0;
#endif
}
ZZ_LOCAL int ZZZIRectangle(void){return(0);}
ZZ_LOCAL int ZZZJRectangle(void){return(-1);}
ZZ_LOCAL int ZZZISquare(void){return(1);}
ZZ_LOCAL int ZZZJSquare(void){return(-1);}
ZZ_LOCAL int ZZZIShape(void){return(2);}
ZZ_LOCAL int ZZZJShape(void){return(-1);}
ZZ_LOCAL int ZZZIRoot(void){return(3);}
ZZ_LOCAL int ZZZJRoot(void){return(-1);}
ZZ_LOCAL void ZZvSet(int tp){
  switch(tp){
    case 0: (void)new Rectangle((ZZZiClass *)NULL); break;
    case 1: (void)new Square((ZZZiClass *)NULL); break;
    case 2: (void)new Shape((ZZZiClass *)NULL); break;
    case 3: (void)new Root((ZZZiClass *)NULL); break;
    default: ZZmsgD(27,tp); ZZerror=ZZerror|040;
  }
}
ZZ_LOCAL void *ZZtopType(void *ptr,int tp,int *newTp){
  switch(tp){
    case 0: ptr=(void *)(((Rectangle *)ptr)->zzThisIs(newTp)); break;
    case 1: ptr=(void *)(((Square *)ptr)->zzThisIs(newTp)); break;
    case 2: ptr=(void *)(((Shape *)ptr)->zzThisIs(newTp)); break;
    case 3: ptr=(void *)(((Root *)ptr)->zzThisIs(newTp)); break;
    default: ZZmsgD(27,tp); ZZerror=ZZerror|040;
  }
  return(ptr);
}

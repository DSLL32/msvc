
/* This program demonstrates the use of the OrgC/C++ library
 * on processing a "netlist", arrangement very common in programs
 * dealing with electronic circuits.
 *
 * We have three object types: Block, Net, and Pin.
 * Blocks have Pins, to which the Nets connect. Each Pin is on
 * exactly one Block; it can be on one or no Net. Both relations
 * Block->Pin, and Net->Pin are examples of what we call AGGREGATE.
 * For example, when using SINGLE_AGGREGATE, Pins on one Block will
 * form (internally, without you worrying about it) a singly-linked
 * ring. Block will have a pointer to one of the Pins, and each Pin
 * will point back to the Block.
 * The program also has to keep a link list (ring) of all Blocks and
 * all Nets. All names are treated as objects.
 *
 * As you can see, the declarations of all objects are very simple and
 * clean, because all the pointers (and there are quite many in this
 * case) are missing. All the information about the data organization
 * and inter-object relations is self-contained in the ZZ_ORG_ statements.
 *
 * Format of the input data:
 *  for blocks: B blockName x1 y1 x2 y2
 *  for pins:   P netName x1 y1 x2 y2
 * Pins of the each block are listed right after the block record,
 * with coordinates that are relative to the lowerLeft corner of the block.
 *
 * The purpose of this program is to read data from the input file,
 * and perform a multiple loop algorithm, which finds:
 *   For each Block, list of all Nets connected to it.
 *      For each Net, run through all the Pins, and find the range
        of their x-coordinates (min x, max x).
 * Note that in order to find the Pin coordinate, one has to go back
 * to the Block, because the Pin coordinates are usually stored relative
 * to the Block position.
 *
 * In this case, saving in the ASCII format is requested. As a test,
 * the data is saved, removed from memory, and then restored from the file
 * before printing the results. For more details on how to specify 
 * key entries (when saving data to disk), see examples in ex1.
 *
 * ZZ_FORMAT() statements describe how (and which) attributes to store.
 * The format specification is similar to regular IO. All the pointers
 * and organization related variables, including the names are stored
 * automatically.
 *
 * To run the program, type:     test < inp
 * where inp is the input file (see above for the format description).
 */

#define ZZascii
#include <stdio.h>
#define ZZmain
#include "zzincl.h"

typedef struct Block Block;
typedef struct Net Net;
typedef struct Pin Pin;

struct Block {
    ZZ_EXT_Block    /* let the program to hide the pointers here */
    int x1,y1,x2,y2; /* corners of the block */
};
ZZ_FORMAT(Block,"%d %d %d %d,x1,y2,x2,y2");

struct Net {
    ZZ_EXT_Net    /* let the program to hide the pointers here */
};
ZZ_FORMAT(Net,"");

struct Pin {
    ZZ_EXT_Pin    /* let the program to hide the pointers here */
    int x1,y1,x2,y2; /* pin, relative to the block lower-left corner  */
};
ZZ_FORMAT(Pin,"%d %d %d %d,x1,y2,x2,y2");

#define INF 10000
#define MAX(A,B) ((A)<(B) ? (B) : (A))
#define MIN(A,B) ((A)>(B) ? (B) : (A))

ZZ_ORG_SINGLE_RING(bChain,Block);           /* list of all Blocks */
ZZ_ORG_SINGLE_RING(nChain,Net);             /* list of all Nets */
ZZ_ORG_SINGLE_AGGREGATE(byBlock,Block,Pin); /* Pins grouped by Block */
ZZ_ORG_SINGLE_AGGREGATE(byNet,Net,Pin);     /* Pins grouped by Net */
ZZ_ORG_NAME(bName,Block);                   /* Block name */
ZZ_ORG_NAME(nName,Net);                     /* Net name */

#define BSIZE 80
char name[BSIZE],buff[BSIZE]; /* buffers to read input */

int main(void)
{
    int x1,y1,x2,y2;
    char c,*nn,*v[2],*t[2];
    Block *bp,*br,*bEntry;
    Net *np,*nEntry;
    Pin *pp,*pr;

    bEntry=(Block *)NULL; nEntry=(Net *)NULL; /* initialize empty lists */
    while(fgets(buff,BSIZE,stdin)){
        sscanf(buff,"%c %s %d %d %d %d",&c,name,&x1,&y1,&x2,&y2);
        if(c=='B'){
            ZZ_PLAIN_ALLOC(Block,1,bp); /* allocate new Block, returns bp */
            bp->x1=x1; bp->y1=y1; bp->x2=x2; bp->y2=y2;
            ZZ_STRING_ALLOC(name,nn);   /* allocate & copy the name */
            ZZ_ADD(bName,bp,nn);        /* add name to the Block */
            ZZ_ADD(bChain,bEntry,bp);   /* add bp to the list of Blocks */
        }
        else if(c=='P'){
            ZZ_PLAIN_ALLOC(Pin,1,pp);   /* allocate new Pin */
            pp->x1=x1; pp->y1=y1; pp->x2=x2; pp->y2=y2;
            /* is this net already listed ? */
            ZZ_A_TRAVERSE_SINGLE_RING(nChain,nEntry,np){
                ZZ_FORWARD(nName,np,nn); /* get name of Net np */
                if(!strcmp(name,nn))ZZ_A_BREAK;
            }ZZ_A_END;
            if(!np){ /* Net not found, create a new one */
                ZZ_PLAIN_ALLOC(Net,1,np); /* allocate a new Net */
                ZZ_STRING_ALLOC(name,nn); /* allocate and copy the name */
                ZZ_ADD(nName,np,nn);      /* add name to the Net */
                ZZ_ADD(nChain,nEntry,np); /* add np to the list of Nets */
            }
            ZZ_ADD(byBlock,bp,pp);        /* add Pin pp the the Block */
            ZZ_ADD(byNet,np,pp);          /* add Pin pp the the Net */
        }
    }                                 /* all data read in */

    /* set the key entries before saving the data to disk */
    v[0]=(char*)bEntry; v[1]=(char*)nEntry;
    t[0]="Block";       t[1]="Net";

    ZZ_SAVE("myFile",2,v,t);  /* save data to disk */
    ZZ_CLEAR(2,v,t);          /* remove all data from memory */
    ZZ_OPEN("myFile",2,v,t);  /* restore data back into memory */
    if(ZZerror){printf("reading a wrong file..\n"); return(0);}

    bEntry=(Block*)v[0]; nEntry=(Net*)v[1]; /* recover key entries */

    /* algorithm of finding the x-span for all connected nets */
    ZZ_A_TRAVERSE(bChain,bEntry,bp){        /* traverse all Blocks */
        ZZ_FORWARD(bName,bp,nn);            /* get the Block name */
        printf("block=%s nets=",nn);
        ZZ_A_TRAVERSE(byBlock,bp,pp){       /* traverse pins on block bp */
            ZZ_PARENT(byNet,pp,np);         /* get Net for the Pin */
            x1=INF; x2= -INF;
            ZZ_A_TRAVERSE(byNet,np,pr){     /* traverse pins of Net np */
                ZZ_PARENT(byBlock,pr,br);   /* get the Block of pin pr */
                x1=MIN(x1,br->x1 + pr->x1); /* offset Pin by Block position */
                x2=MAX(x2,br->x1 + pr->x2); 
            }ZZ_A_END;
            ZZ_FORWARD(nName,np,nn);        /* get the Net name */
            printf(" %s(%d,%d)",nn,x1,x2);  /* print Net and x-range */
        }ZZ_A_END;
        printf("\n");
    }ZZ_A_END;

    return(0);
}
#include "zzfunc.c"

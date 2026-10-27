HOW TO USE THE C LIBRARY
--------------------------
There is no  similar  C  library,  which would allow you to handle generic data
structures,  and still maintain full typing,   and  automatic  storage  to disk
(persistence).   When you use our C library,  you get many features  still  not
available in most  C++   class  libraries.   For  example,   your  data will be
automatically  protected   against   allocation   errors,   dangling  pointers,
misconnected lists, and other treacherous bugs.

In this short introduction, we will concentrate on practical use, not on how the
library was implemented. For example, consider the following problem:

We are given a set of towns,  which are located in different states.  The towns
are connected by two transportation networks: highways and airline connections.
The problem is to code a program, which will determine the least expensive route
between a given pair of towns.

The situation  is not easy to represent without full graphics,  here is a crude
diagram:

 ---------state 1------------              ------state 2-------
 |                          |              |                  |
 |   hhh*hhhhhhhhhh*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa*     |
 |   h             a hhhh   |              |            a     |
 |   h             a    h   |              |    *       a     |
 |   h             a    h   |              |    h       a     |
 |   h             a    *   |              |    h       a     |
 |   *aaaaaaaaaaaaaa    h   |              |    h       a     |
 |   a                  h   |              |    hhhhhhhh*     |
 ----a------------------h----              |            a     |
     a                  h                  |            a     |
 ----a----state 3-------h----              |        *   *     |
 |   a                  h   |              |        h   a     |
 |   aa*hhhhhhh*        h   |              |        h   a     |
 |             h        h*hhhhhhhhhhhhhhhhhhhhhhhhhh*aaaa     |
 |             h        h   |              |                  |
 |             h      *hh   |              --------------------
 |     *hhhhhhh*hhhhhhh     |
 |                          |          aaaaa = airline connection
 ----------------------------          hhhhh = highway
                                           * = town

The program will have several parts, which will be described separately.

Part 1: General includes.
------------------------
 #define ZZmain       // must be in your main program
 #include <stdio.h>
 #include "zzincl.h"  // will be generated automatically

Note that at the time you code the program, file zzincl.h does not exist yet.
Our code generator will generate this file  (and also zzfunc.c, which can be
compiled separately) before your first compilation.


Part 2: Declaration of classes.
-------------------------------
Design classes as usual, with two exceptions:

(1) There is no reference to library classes or pointers that form
    the data relations (graph, aggregate, ...). Data and relations,
    at least in your code, appear disconnected.
(2) Each class has one special line ZZ_EXT_... which simply repeats
    the name of the class, and registers the class as being automatically
    managed. The statement is always the same, whether the object participates
    in 1 linked list or 15 hash tables.

    typedef struct State State;
    typedef struct Town Town;
    typedef struct Highway Highway;
    typedef struct AirLink AirLink;

    struct State {
        ZZ_EXT_State
        float gasPrice;
        /* ... any other data members */
    };
    struct Town {
        ZZ_EXT_Town
        float cost;       /* for the algorithm */
        Town *trace;      /* route traceback */
        /* ... any other data members */
    };
    struct Highway {
        ZZ_EXT_Highway
        int distance;
        /* ... any other data members */
    };
    struct AirLink {
        ZZ_EXT_AirLink
        float fare;
        /* ... any other data members */
    };


Part 3: Declaration of the organization.
---------------------------------------

 ZZ_ORG_SINGLE_GRAPH(air, Town, AirLink); 
 ZZ_ORG_SINGLE_GRAPH(hwy,Town, Highway);
 ZZ_ORG_DOUBLE_RING(states, State);  
 ZZ_ORG_SINGLE_AGGREGATE(byState, State, Town);

Each line declares one data organization.  All our keywords use the prefix ZZ,
and ORG is an abbreviation for "organization".

For example,  here  the  first line declares a graph with Towns as nodes,   and
AirLinks as edges;  air is the  instance  of the GRAPH organization.  The words
SINGLE and DOUBLE denote singly and doubly linked organizations.   AGGREGATE is
used when one object type contains several objects of another type.   We always
use  RING  to represent  a linked-list   (we do not recommend the use of a NULL
ending list).   Performance  and  access  are  about the same,  and RING allows
better run-time error checking.

The library provides all basic data organizations including hash tables, dynamic
arrays, ER models, properties, virtual arrays, and trees,  thus  allowing you to
instantly construct very fast, efficient data organizations. For a complete list
of available organizations, see file A:\CLASSES.

Part 4: User program.
--------------------
Once the organization has been declared in this manner,  you can operate on the
data  using  functions  provided  by the library.  These macros are similar  in
functionality and syntax, and therefore easy to remember and use. For example:

    ZZ_ADD(states,..)  adds a new state to the ring of states,
    ZZ_ADD(byState,..) adds a town to a state, or
    ZZ_ADD(hwy,..) adds a new highway between two towns.

When appropriate, these macros hide calls to a compiled functions.

Here are some other examples of  what  you  can  do  (this  is not a meaningful
program, only unrelated examples of function calls):

    Highway *hp;
    State *sp,*ss;
    Town *tp,*tt[2];

    ZZ_PLAIN_ALLOC(Town,1,tp); /* allocates a new, disconnected town */
    ZZ_ADD(byState,sp,tp); /* adds town tp under state sp */
    ZZ_DELETE(hwy,tt,hp); /* disconnects highway hp from graph "hwy" */
    ZZ_FORWARD(states,sp,ss);  /* returns next state after sp */
    ZZ_A_TRAVERSE(states,ss,sp){ /* loops through all states */
          printf("gasPrice=%f\n",sp->>gasPrice); 
    }

Traverse macros ZZ_A_TRAVERSE(.. ){ ... }ZZ_A_END; are automatically provided by
the library. They traverse either the entire set (for example a ring, list, or a
collection), or a subset.  For example, for trees, they traverse the children of
a given node; for graphs, they traverse edges adjacent to a given node.

In one command,  you can save this entire organization (two independent graphs,
aggregation, and a circular list)  to disk.  You point to any object,  tell its
type,   and  the  library  will automatically traverse all pointers,  visit all
objects, and save them to disk:

    char *v[1]; char *t[1];
    State *sp;

    v[0]=(char *)ss;         /* entry into the data network */
    t[0]="State";            /* type of the entry object  */
    ZZ_SAVE("myFile",1,v,t); /* saves all data to file "myFile" */

In case your data is not completely connected via internal pointers, you provide
several entries, one for each set.  This is the reason for using the arrays v[],
t[] and not single pointers.

When  retrieving  data  from disk,  you proceed in the  reverse  order.   Macro
ZZ_OPEN() retrieves the data from disk, and returns new pointers to the objects
to which you originally pointed in array v[]:

    char *v[1]; char *t[1];
    State *sp;

    ZZ_OPEN("myFile",1,v,t);   /* saves all data to file "myFile" */
    if(strcmp("State",t[0])){  /* ... error ... */  }
    ss=(State *)v[0];

For more complex examples and complete programs, look at directories A:\EX*

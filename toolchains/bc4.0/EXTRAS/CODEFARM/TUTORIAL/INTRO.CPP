HOW TO USE THE C++ LIBRARY
--------------------------
Internally,  our library  has been built on a completely different concept than
other class libraries. The result is a complete data management layer, which is
more efficient, highly automatic, easier to use, and is better protected against
errors.

In this short introduction,   we  will  concentrate on the practical use of the
library, not on how it was implemented.  As an example,  consider the following
problem:

We are given a set of towns,  which are located in different states.  The towns
are connected by two transportation networks: highways and airline connections.
The problem is to code a program, which will determine the least expensive route
between a given pair of towns.

The situation is not easy to represent without full  graphics,  here is a crude
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

The program has several parts, which will be described separately.

Part 1: General includes.
------------------------
 #define ZZmain       // must be in your main program
 #include <stdio.h>
 #include "zzincl.h"  // will be generated automatically

Note that when you code the program, the file  zzincl.h does not exist yet. Our
code generator generates this file  (and also  zzfunc.c,  which can be compiled
separately) before your first compilation.


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

    class State {
        float gasPrice;
        ZZ_EXT_State
    public:
        // ... any functions, including virt.functions, constructors, etc.
    };
    class Town {
        float cost;         // for the algorithm
        Town *trace;        // route traceback
        ZZ_EXT_Town
    public:
        // ... any functions, including virt.functions, constructors, etc.
    };
    class Highway {
        int distance;
        ZZ_EXT_Highway
    public:
        // ... any functions, including virt.functions, constructors, etc.
    };
    class AirLink {
        float fare;
        ZZ_EXT_AirLink
    public:
        // ... any functions, including virt.functions, constructors, etc.
    };


Part 3: Declaration of the organization.
---------------------------------------

 ZZ_HYPER_SINGLE_GRAPH(air, Town, AirLink); 
 ZZ_HYPER_SINGLE_GRAPH(hwy,Town, Highway);
 ZZ_HYPER_DOUBLE_RING(states, State);  
 ZZ_HYPER_SINGLE_AGGREGATE(byState, State, Town);
 ZZ_HYPER_UTILITIES(util);

Each line declares  one data organization.  All our keywords use the prefix ZZ,
and HYPER is an abbreviation for the "hyper class",  a new concept on which the
library is built.

For  example  here  the  first line declares a graph with Towns as nodes,   and
AirLinks as edges;  air is the  instance  of the GRAPH organization.  The words
SINGLE and DOUBLE denote singly and doubly linked organizations.   AGGREGATE is
used when one object type contains several objects of another type.   We always
use RING  to represent a linked-list   (we do not recommend  the use of  a NULL
ending list). Performance and access are about the same, and RING allows better
run-time error checking.  UTILITIES declare  a class with memory management and
disk saving utilities.

The library provides all basic data organizations including hash tables,dynamic
arrays, ER models, properties, virtual arrays, and trees, thus  allowing you to
instantly construct very fast,efficient data organizations. For a complete list of available organizations, see file A:\CLASSES.

Part 4: User program.
--------------------
Once the organization has been declared in this manner,  you can operate on the
data  using functions provided by the library.  These functions are similar  in
functionality and syntax, and therefore easy to remember and use. For example,

    states.add(..)  adds a new state to the ring of states,
    byState.add(..) adds a town to a state, or
    hwy.add(..) adds a new highway between two towns.

Here are some  other  examples  of  what  you  can do (this is not a meaningful
program, only unrelated examples of function calls):

    Highway *hp;
    State *sp,*ss;
    Town *tp;

    tp=new Town; // allocates new town, and initializes it as disconnected
    byState.add(sp,tp); // adds town tp under state sp
    hwy.del(hp); // disconnects highway hp from graph "hwy"
    ss=states.fwd(sp);  // returns next state after sp
    states_iterator sIter(ss); // starts iterator from ss
    while(sp=sIter++){ // loops through all states    
          printf("gasPrice=%f\n",sp->>gasPrice); 
    }

Iterators  are  automatically  provided by the library.  The iterator traverses
either the entire set (for ring, list or collection), or a subset. For example,
the tree iterator traverses the children  of  a given node;  the graph iterator
traverses edges adjacent to a given node.

In one command,  you can save this entire organization (two independent graphs,
aggregation, and a circular list)  to disk.  You point to any object,  tell its
type,   and  the  library  will automatically traverse all pointers,  visit all
objects, and save them to disk:

    char *v[1],*t[1];
    State *sp;

    v[0]=(State*)ss;           // entry into the data network
    t[0]="State";              // type of the entry object 
    util.save("myFile",1,v,t); // saves all data to file "myFile"

In case your data is not completely connected via internal pointers, you provide
several entries, one for each set.  This is the reason for using the arrays v[],
t[] and not single pointers. Operations save/open are one of the few exceptions,
where you have to cast. All other operations are fully typed.

When retrieving data from disk,  you proceed in the  reverse  order.   Function
util.open()   retrieves  the  data  from disk,  and returns new pointers to the
objects to which you originally pointed in array v[]:

    char *v[1],*t[1];
    State *sp;

    util.open("myFile",1,v,t); // saves all data to file "myFile"
    if(strcmp("State",t[0])){  /* ... error ... */  }
    ss=(State *)v[0];

For more complex examples and complete programs, look at directories A:\EXX*

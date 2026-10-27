/* This is the first part of the test, which reads Employee records
 * (name,salary) from the input file, and puts them into two parallel
 * linked lists. It sorts the two lists, prints them, and then
 * saves all the data to disk (file "myFile").
 *
 * Comments:
 *  For all data organizations, we use rings and not NULL ending lists.
 * Rings are better protected against pointer errors. The only difference
 * in use (the main reason most people use NULL ending rings) is a more
 * complex end condition when traversing the whole list. We do this for
 * you; in our library, using rings is just as easy as using lists the
 * usual way.
 *  Note that struct Employee does not contain any pointers.
 * The Employee name is handled as a separate object, which allows
 * to save it automatically. An object can any number of names.
 *  bySalary and byName are not variables, they are identifiers of
 * the organization; they allow the use of generic commands like ZZ_ADD()
 * that can be applied to many organizations.
 *  When sorting a data set, you have to provide a compare function just
 * like when using qsort(). Here, sfun() compares salaries of two Employees,
 * nfun() compares names of two Employees.
 *  Do you know that sorting lists (rings) is faster and more efficient
 * than using qsort?
 *  For each list (ring) you have to keep an external entry (sEntry,nEntry).
 * When saving data to disk, you have to provide one or several "key entries"
 * Array v[] must contain pointers to key entries, t[] their types.
 * Key entries have a double purpose: They work as seeds for the expansion
 * algorithm which jumps from one object to another via internal pointers,
 * collects all objects, and saves them to disk. They also represent
 * important data entry points, which you want to recover when reading 
 * the data back into memory. In this case, you obviously want to recover
 * the entry points for the both rings.
 *  Since there is no other specification, the program saves in the default
 * binary format.
 */

#include <stdio.h>
#define ZZmain
#include "zzincl.h"

typedef struct Employee Employee;
struct Employee {
    ZZ_EXT_Employee   /* standard for any organization(s) */
    long salary;
};

ZZ_ORG_SINGLE_RING(bySalary,Employee);
ZZ_ORG_SINGLE_RING(byName,Employee);
ZZ_ORG_NAME(eName,Employee);  /* name treated as an object */

#define BSIZE 80

int main(int argc,char **argv)
{
    char name[BSIZE],buff[BSIZE],*nn,*v[2],*t[2];
    FILE *file1,*file2;
    Employee *nEntry,*sEntry,*pe;
    int sfun(char *p1,char *p2),nfun(char *p1,char *p2);
    long sal;
    void printAll(FILE *fp,Employee *sEntry,Employee *nEntry);

    /* if no inp/out file given, go for the standard IO */
    if(argc<=1)file1=stdin;  else file1=fopen(argv[1],"r");
    if(argc<=2)file2=stdout; else file2=fopen(argv[2],"w");

    nEntry=sEntry=(Employee *)NULL; /* initialize both lists as empty */
    while(fgets(buff,BSIZE,file1)){ /* read input line by line */
        sscanf(buff,"%s %lu",name,&sal);
        ZZ_PLAIN_ALLOC(Employee,1,pe); /* allocates & initializes 1 Employee */
        ZZ_STRING_ALLOC(name,nn); /* allocates name, copies name into it */
        ZZ_ADD(eName,pe,nn); /* adds name nn to Employee pe */
        pe->salary=sal;
        ZZ_ADD(byName,nEntry,pe);   /* add pe to first ring */
	ZZ_ADD(bySalary,sEntry,pe); /* add pe to second ring */
    }; /* all input read in */

    ZZ_SORT(bySalary,sfun,sEntry);  /* sort the first ring */
    ZZ_SORT(byName,nfun,nEntry);    /* sort the second ring */
    printAll(file2,sEntry,nEntry);        /* print all the data */
    
    /* prepare key entries for the saving */
    v[0]=(char*)sEntry; v[1]=(char*)nEntry;
    t[0]="Employee";    t[1]="Employee";

    ZZ_SAVE("myFile",2,v,t); /* save to disk, 2 key entries given */
    return(0);
}
/* ------------------------------------------------------ */
void printAll(FILE *fp,Employee *sEntry, Employee *nEntry)
{
    Employee *pe;
    char *nn;

    ZZ_A_TRAVERSE(bySalary,sEntry,pe){ /* traverse the first ring */
        ZZ_FORWARD(eName,pe,nn); /* for Employee pe, get its name nn */
        fprintf(fp,"%s\t%lu\n",nn,pe->salary);
    }ZZ_A_END;

    fprintf(fp,"\n");
    ZZ_A_TRAVERSE(byName,nEntry,pe){ /* traverse the second ring */
        ZZ_FORWARD(eName,pe,nn); /* for Employee pe, get its name nn */
	fprintf(fp,"%s\t%lu\n",nn,pe->salary);
    }ZZ_A_END;
}
/* ------------------------------------------------------ */
int sfun(char *s1,char *s2)  /* comparing salaries */
{
    Employee *p1,*p2;

    p1=(Employee *)s1; p2=(Employee *)s2;
    if(p1->salary < p2->salary) return(-1);
    if(p1->salary > p2->salary) return(1);
    return(0);
}
/* ------------------------------------------------------ */
int nfun(char *s1,char *s2)  /* comparing names */
{
    Employee *p1,*p2;
    char *n1,*n2;

    p1=(Employee *)s1; p2=(Employee *)s2;
    ZZ_FORWARD(eName,p1,n1);
    ZZ_FORWARD(eName,p2,n2);
    return(strcmp(n1,n2));
}
#include "zzfunc.c"

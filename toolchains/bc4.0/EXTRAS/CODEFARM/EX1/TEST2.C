/* This is the seond part of the test, which reads the data from the disk 
 * file, and prints it. The same data declaration (organization) is used.
 * Note that OrgC/C++ libraries are smart and, if you store in the ASCII
 * format, you can modify both the objects (like the Employee structure here)
 * and the organization (add or delete ZZ_ORG_.. statements), and the
 * program still can recover data from the old disk files.
 *
 * ZZ_OPEN() is a complete reverse of ZZ_SAVE(), it returns the key entries
 * that you supplied when you saved to disk. The key entry pointers typically
 * change, because any time you reload the data, they move to generally 
 * a different memory location.
 *
 * Since the data objects and data organization is the same for both programs,
 * files zzincl.h and zzfunc.c can be re-used without even running 
 * the code generator/preprocessor again.
 */

#include <stdio.h>
#define ZZmain
#include "zzincl.h"

typedef struct Employee Employee;
struct Employee {
    ZZ_EXT_Employee
    long salary;
};

ZZ_ORG_SINGLE_RING(bySalary,Employee);
ZZ_ORG_SINGLE_RING(byName,Employee);
ZZ_ORG_NAME(eName,Employee);

int main(void)
{
    char *v[2],*t[2];
    Employee *nEntry,*sEntry;
    void printAll(FILE *fp,Employee *sEntry,Employee *nEntry);

    ZZ_OPEN("myFile",2,v,t); /* restore data from disk, including key entries */
    sEntry=(Employee *)(v[0]); /* recover the key entries */
    nEntry=(Employee *)(v[1]);
    printAll(stdout,sEntry,nEntry); /* print the data again */
    return(0);
}
/* ------------------------------------------------------ */
/* This function is identical to test1.c, could be compiled separately */
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
#include "zzfunc.c"

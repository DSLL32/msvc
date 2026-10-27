/*	This is a simple "C" program.			*/
/*	It purposely contains several syntax errors.	*/
/*	Compile with Borland Turbo C		        */


#include "conio.h"
#include "stdio.h"
#include "stddef.h"
#include "stdlib.h"

#define	FOREVER 1

main()
{

int num1, num2;
char inline[80];

	printf("\nWelcome to the simple calculator.  You enter two numbers");
	printf("\nand the computer will add them.  Enter \"0\" to quit.");

	do {
	
	    printf("\n\nEnter first number: ");
	    num0 = atoi(gets(inline));	/* Should be "num1 = ..." */
	    if (num1 == 0) break;

	    printf("\nEnter second number: ");
	    num2 = atoi(gets(inline));

	    printf("\nThe sum = %d\n",num1+num2);

	} while (FORVER);		/* Should be "} while (FOREVER);" */

	printf("\nThe simple calculator is now done.");
	
}

/*
 *   ------------------------------------------------------------
 *  |  example.c                                                 |
 *  |                                                            |
 *  |  C-Index/II and C-Index/PC Example Database Program        |
 *  |                                                            |
 *  |  Trio Systems                                              |
 *  |  936 E. Green St.  Suite 105                               |
 *  |  Pasadena, CA  91106                                       |
 *  |  818/594-9706                                              |
 *  |  FAX: 818/584-0364                                         |
 *  |                                                            |
 *  |  (c) Copyright 1983 - 1993                                 |
 *   ---------------------------------------------------------
 */


/*
 *
 *  include files
 *
 */


#include "compflag.h"			/* compiler specific header information */
#include "cindex.h"				/* c-index primary file header */
#include "example.h"				/* example program header */

/*******************************************************************
 * !!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *	For TURBO C Users:
 *
 * You must set the stack size with the following statement
 * once in every program that accesses C-Index.  This will
 * ensure that your stack is large enough.  In some circumstances
 * you may need an even larger stack.  Refer to the README file
 * on the source code disk for more instructions.
 *
 */

#ifdef TC
		extern unsigned _stklen = 8192;		/* specify a 8K stack */
#endif

/********************************************************************/


char chrfld[FIELDS][CHRLEN];	/* input field storage */
int curfld;			/* current field number */
int nxtfld;			/* next field number */
CFILE datafile;			/* psp for data file */
char databuf[DBUFLEN];		/* work buffer for data file */
FIELD keydef;			/* key definition for searches */
FIELD datalist[FLDDEFS];	/* data list for data file */
int curindx;			/* current index # for next/prev */



/*
 *
 *  main
 *
 */


main()
{
	int cc;			/* condition code from open/close */

	/**/

	initdsp();		/* initialize the display routines */

	initdlst();		/* initialize the datalist array */

	cc = dmopen(&datafile, "example.fil", databuf, DBUFLEN, SHARED);
	if (cc != OK)
	{
		cc = dmcreate(&datafile, "example.fil", databuf, DBUFLEN, SHARED);
		if (cc != OK)
		{
		  printf("\n\nError opening or creating data file: %d\n", cc);
		}
	}

	if (cc == OK)
	{
		curindx = COMPIND; /* default to company index on next/prev */
		dspscrn();	   /* display screen form */
		clrform();	   /* clear and display blank fields */
		inpscrn();	   /* do input from screen */

		clrscr();	   /* clear screen for exit */

		cc = dclose(&datafile);
		if (cc != OK)
			printf("\n\nError closing data file: %d\n", cc);
	}

	enddsp();		/* release display */

	return(0);
}

/* ------------------------------------------------------------ */

/*  file processing functions  */


/*
 *
 *  initialize data file datalist
 *
 */

void initdlst()
{
	int i;

	/**/

	for (i=0; i < FIELDS; i++)
	{
		datalist[i].fldtype = STRING;		/* string type field*/
		datalist[i].fldlen = CHRLEN;		/* length of field */
		datalist[i].fldptr = &chrfld[i][0];	/* field 0 */

		switch (i)
		{
			case 0:				 /* company name */
			  datalist[i].keytype = STRING;  /* string type key */
			  datalist[i].dupflag = UNQKEY;  /* unique key */
			  datalist[i].fldindex = COMPIND;/* index # 1 */
			break;

			case 1:				 /* contact name */
			  datalist[i].keytype = STRING;	 /* string type key */
			  datalist[i].dupflag = DUPKEY;	 /* duplicate key */
			  datalist[i].fldindex = CONTIND;/* index # 2 */
			break;

			case 8:				 /* product name */
			  datalist[i].keytype = STRING;	 /* string type key */
			  datalist[i].dupflag = DUPKEY;	 /* duplicate key */
			  datalist[i].fldindex = PRODIND;/* index # 3 */
			break;

			default:
			  datalist[i].keytype = NONKEY;	 /* not a key type */
			  datalist[i].dupflag = NONKEY;	 /* not a key */
			  datalist[i].fldindex = 0;	 /* index # 0 - none */
			break;			
		}
	}

	datalist[FIELDS].fldtype = ENDLIST;

}




/*
 *
 *  find record by company name - index # 1
 *
 */

void findcomp()
{
	char srchval[80];		/* search string */
	int keyfunc;			/* function key for input request */

	/**/

	keyfunc = getcn("Search Company Name: ", srchval);
	if (keyfunc != KEYCR || strlen(srchval) == 0)
		dsperr("Cancelled Company Search");
	else
		search(srchval, COMPIND);		/* do the search */

}



/*
 *
 *  find record by contact name - index # 2
 *
 */

void findcont()
{
	char srchval[80];		/* search string */
	int keyfunc;			/* function key for input request */

	/**/

	keyfunc = getcn("Search Contact Name: ", srchval);
	if (keyfunc != KEYCR || strlen(srchval) == 0)
		dsperr("Cancelled Contact Search");
	else
		search(srchval, CONTIND);		/* do the search */

}




/*
 *
 *  find record by product purchased - index # 3
 *
 */

void findprod()
{
	char srchval[80];		/* search string */
	int keyfunc;			/* function key for input request */

	/**/

	keyfunc = getcn("Search Product Name: ", srchval);
	if (keyfunc != KEYCR || strlen(srchval) == 0)
		dsperr("Cancelled Product Search");
	else
		search(srchval, PRODIND);		/* do the search */

}




/*
 *
 *  common code for searching
 *
 */

void search(value, index)
char *value;
int index;
{

	int cc;				/* condition code from find */
	int keyfunc;			/* function key for input request */
	char srchval[80];		/* search string */
 	int srchtype;			/* type of search for dfind */

	/**/

	dspmsg("1=EQUAL 2=GREAT 3=GREATEQ 4=LESS 5=LESSEQ");
	keyfunc = getcn("Type of search: ", srchval);
	if (keyfunc != KEYCR || strlen(srchval) == 0)
		dsperr("Cancelled Search");
	else
	{
		switch (atoi(srchval))
		{
			case 1:
				srchtype = EQUAL;
				break;

			case 2:
				srchtype = GREAT;
				break;

			case 3:
				srchtype = GREATEQ;
				break;

			case 4:
				srchtype = LESS;
				break;

			case 5:
				srchtype = LESSEQ;
				break;

			default:
				dspmsg("assuming EQUAL type search");
				break;

		}

		curindx = index;	/* save index number for next/prev */

		cc = dfind(&datafile, index, value, STRING, srchtype);

		switch (cc)
		{

			case OK:
			readdata();		/* read the data record */
			break;

			case GREATER:
			dsperr("Exact match not found - next higher shown (GREATER)");
			readdata();		/* read the data record */
			break;

			case LESSTHAN:
			dsperr("Exact match not found - next lower shown (LESSTHAN)");
			readdata();		/* read the data record */
			break;

			case FAIL:
			dsperr("Key not found - form cleared (FAIL)");
			clrform();
			break;

			default:
			dspcc("Error searching", cc);
			clrform();
			break;
		}
	}

}



/*
 *
 *  read the data record located
 *
 */

void readdata()
{
	int cc;			/* condition code returned by cindex+ */

	/**/

	cc = dread(&datafile, datalist);
	if (cc == OK)
	{
		dspflds();			/* display record found */
	}
	else
	{
		clrform();
		dspcc("Error reading data", cc);
	}

}




/*
 *
 *  delete current record
 *
 */

void delrec()
{
	int cc;				/* condition code */

	/**/

	cc = ddelete(&datafile, datalist);

	clrform();				/* clear fields on screen */

	if (cc != OK)
		dspcc("Failed deleting current record", cc);

}




/*
 *
 *  add new record to file
 *
 */

void addrec()
{
	int cc;				/* condition code from add */

	/**/

	cc = dadd(&datafile, datalist);

	if (cc != OK)
		dspcc("Error adding new record to file", cc);

}




/*
 *
 *  update current record with new data from screen
 *
 */

void updrec()
{
	int cc;			/* condition code from update */

	/**/

	cc = dupdate(&datafile, datalist);

	if (cc != OK)
		dspcc("Error updating record", cc);

}



/*
 *
 *  read next record in index last searched
 *
 */

void nxtrec()
{
	int cc;			/* condition code from next read */

	/**/

	cc = dseq(&datafile, curindx, NEXT);

	if (cc != OK)
	{
		dspcc("Error searching for next record", cc);
		clrform();			/* clear form on screen */
	}
	else
	{
		readdata();			/* read the data record */
	}

}



/*
 *
 *  read previous record in index last searched
 *
 */

void prevrec()
{

	int cc;			/* condition code from previous read */

	/**/

	cc = dseq(&datafile, curindx, PREV);

	if (cc != OK)
	{
		dspcc("Error searching for previous record", cc);
		clrform();			/* clear form on screen */
	}
	else
	{
		readdata();			/* read the data record */
	}

}




/*
 *
 *  read last record in index last searched
 *
 */

void lastrec()
{
	int cc;				/* condition code from last read */

	/**/

	cc = dseq(&datafile, curindx, LAST);

	if (cc != OK)
	{
		dspcc("Error searching for last record", cc);
		clrform();			/* clear form on screen */
	}
	else
	{
		readdata();			/* read the data record */
	}

}



/*
 *
 *  read first record in index last searched
 *
 */

void firstrec()
{

	int cc;				/* condition code from last read */

	/**/

	cc = dseq(&datafile, curindx, FIRST);

	if (cc != OK)
	{
		dspcc("Error searching for first record", cc);
		clrform();			/* clear form on screen */
	}
	else
	{
		readdata();			/* read the data record */
	}

}


/*
 *
 *
 *	lock the current record
 *
 *
 */

void lockrec()
{
	int cc;		/* return from lock operation */

	/**/

	cc = msetrec(&datafile);	/* lock current record */

	if (cc != OK)
		dspcc("Error locking current record", cc);
	else
		dspmsg("Locked current record");

}


/*
 *
 *	unlock the current record
 *
 */

void unlockrec()
{
	int cc;

	/**/

	cc = mclrrec(&datafile);	/* unlock the record */

	if (cc != OK)
		dspcc("Error unlocking current record", cc);
	else
		dspmsg("Unlocked current record");

}

/*
 *
 *	test lock of current record
 *
 */

void tstlock()
{
	int cc;

	/**/

	cc = mtstrec(&datafile);

	switch (cc)
	{
		case RECSFLK:
		dspmsg("Record Self Locked (RECSFLK)");
		break;

		case RECSHLK:
		dspmsg("Record Locked by Sharing Process (RECSHLK)");
		break;

		case RECUNLK:
		dspmsg("Record Unlocked (RECUNLK)");
		break;

		default:
		dspcc("Error testing lock", cc);
		break;
	}

}



/*
 *
 *  display condition code error
 *
 */

void dspcc(message, cc)
char *message;
int cc;
{
	char errmsg[80];

	/**/

	sprintf(errmsg, "%s: %d", message, cc);
	switch (cc)		/* add known name */
	{
		case FAIL:
		strcat(errmsg, "   (FAIL)");
		break;

		case RECSHLK:
		strcat(errmsg, "   (RECSHLK)");
		break;

		case RECSFLK:
		strcat(errmsg, "   (RECSFLK)");
		break;

		case RECUNLK:
		strcat(errmsg, "   (RECUNLK)");
		break;

		case RECNOLK:
		strcat(errmsg, "   (RECNOLK)");
		break;

		case NOCUR:
		strcat(errmsg, "   (NOCUR)");
		break;

		default:
		strcat(errmsg, "   (refer to manual)");
		break;
	}

	dsperr(errmsg);

}

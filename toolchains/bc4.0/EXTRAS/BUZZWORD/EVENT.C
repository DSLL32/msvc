#include "order.h"

/*
 * WINGEN Events
 *
 */


#define     RETURNSTRINGCOUNT 5                                           /* maximum number of return strings needed at one time */
char   far  ReturnStrings[RETURNSTRINGCOUNT][256] ;                       /* hold EVENT function return strings here */
int         NextAvailableString = 0 ;                                     /* counter to cycle thru return strings */


#if defined(__ZTC__) || defined(__WC386__)
#define pow10(x) pow(10, x)
#endif

/**************************************************************************

Function:       ABS(X)
Description:    Returns the absolute value of the number X
Prototype:      double ABS(double X) ;

***************************************************************************/

double ABS(double X)
   {
   if (X < 0.0)
      return(-X) ;
   else
      return(X) ;
   }

/**************************************************************************

Function:       ACOS(X)
Description:    Returns the arc cosine of X where X is between -1 and 1
Prototype:      double ACOS(double X) ;

***************************************************************************/

#if defined(__WC386__)
double wd_ACOS(double X)
#else
double ACOS(double X)
#endif
   {
   if (X > 1.0 || X < -1.0)
      return(WD_ERR) ;

   return(acos(X)) ;
   }


/**************************************************************************

Function:       ADDMENU("MenuName", Position)
Description:    Creates a new menu command
Prototype:      void ADDMENU(LPSTR MenuName, UINT Position) ;

***************************************************************************/

void ADDMENU(LPSTR MenuName, UINT Position)
   {
   }

/**************************************************************************

Function:       ADDMENUITEM("MenuName", "MenuItemName", Position)
Description:    Adds a new item under a menu command
Prototype:      void ADDMENUITEM(LPSTR MenuName,
                                            LPSTR MenuItemName,
                                            UINT Position) ;

***************************************************************************/


void ADDMENUITEM(LPSTR MenuName, LPSTR MenuItemName, UINT Position)
   {
   }

/*****************************************************************************

   FUNCTION:    APPEXIT

   PURPOSE:     close the window and release memory

*****************************************************************************/

void APPEXIT(void)                                                        /* Happy Trails (astalavesta, baby) */
   {
   DestroyWindow(hwnd) ;                                                  /* terminate window */
   return ;                                                               /* return to calling function */
   }


/**************************************************************************

Function:       APPNEW()
Description:    Same as File | New from ObjectVision
Prototype:      void APPNEW(void) ;

Notes:          This function is only valid within an interpreted
                environment.  Because code generated with WinGEN is compiled
                into an .exe file there is no need or possibility for
                implementing this function.

***************************************************************************/

void APPNEW(void)
   {
   }

/****************************************************************************

   FUNCTION:    APPOPEN (newexe)

   DESCRIPTION: open a window application
                newexe: name of application

****************************************************************************/

void APPOPEN(LPSTR newexe)
   {
   char buffer[256] ;
   char appl[129] ;
   int i, rc ;

   lstrcpy(appl, newexe) ;                                                /* get filename */
   i = strlen(appl) ;                                                     /* get length of filename */
   while (i > 0 && appl[i] != '.')                                        /* while we haven't found a dot */
      i-- ;                                                               /* get one character back */
   if (appl[i] == '.')                                                    /* if we found a dot */
      appl[i] = 0 ;                                                       /* replace it by 0 (NULL-termination) */
   lstrcat(appl, ".exe") ;                                                /* add ".exe" to string */

   rc = WinExec(appl, SW_SHOW) ;                                          /* activate application in its own window */
   if (rc < 32)                                                           /* if an error code was returned */
      {
      lstrcpy(buffer, "The following Windows Error Message was encountered ") ;
      lstrcat(buffer, "attempting to run application: ") ;
      lstrcat(buffer, appl) ;
      lstrcat(buffer, "\n\n") ;                                            /* prepare error message */
      switch(rc)                                                           /* identify error code */
         {                                                                 /* and get corresponding message */
         case 0:
            lstrcat(buffer, "System was out of memory, executable file was corrupt, or relocations were invalid.") ;
            break ;
         case 2:
            lstrcat(buffer, "File was not found.") ;
            break ;
         case 3:
            lstrcat(buffer, "Path was not found.") ;
            break ;
         case 5:
            lstrcat(buffer, "Attempt was made to dynamically link to a task, or there was a sharing or network error.") ;
            break ;
         case 6:
            lstrcat(buffer, "Library required separate data segments for each task.") ;
            break ;
         case 8:
            lstrcat(buffer, "There was insufficient memory to start the application") ;
            break ;
         case 10:
            lstrcat(buffer, "Windows version was incorrect.") ;
            break ;
         case 11:
            lstrcat(buffer, "Executable file image was invalid or not a Windows application.") ;
            break ;
         case 12:
            lstrcat(buffer, "Application was designed for a different operating system.") ;
            break ;
         case 13:
            lstrcat(buffer, "Application was designed for MS-DOS 4.0") ;
            break ;
         case 14:
            lstrcat(buffer, "Type of executable file was unknown.") ;
            break ;
         case 15:
            lstrcat(buffer, "Attempt was made to load a real-mode application for an earlier version of Windows.") ;
            break ;
         case 16:
            lstrcat(buffer, "Attempt was made to load a 2nd instance of an application with not read-only data segments.") ;
            break ;
         case 19:
            lstrcat(buffer, "Attempt was made to load a compressed executable file that must be decompressed first.") ;
            break ;
         case 20:
            lstrcat(buffer, "One of the required DLL files was invalid or corrupt.") ;
            break ;
         case 21:
            lstrcat(buffer, "Application requires Microsoft Windows 32-bit extensions.") ;
            break ;
         default:
            lstrcat(buffer, "Unknown Microsoft Windows Error Return Code.") ;
            break ;
         }
      MessageBox(NULL, buffer, NULL, MB_ICONSTOP | MB_OK) ;               /* display error message in message box */
      InvalidateRect(forms[currentformindex].hWnd, NULL, TRUE) ;          /* invalidate rectangle */
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update window */
      }
   else                                                                   /* load was successful, application run */
      DestroyWindow(hwnd) ;                                               /* end application */
   return ;                                                               /* return to calling function  */
   }

/**************************************************************************

Function:       ASCIIOPEN("LinkName", "FileName", "OVFields", Option)
Description:    Opens the ASCII file "FileName" and links it to your app
Prototype:      void ASCIIOPEN(LPSTR LinkName, LPSTR FileName,
                                          LPSTR OVFields, UINT Option) ;

***************************************************************************/

void ASCIIOPEN(LPSTR LinkName, LPSTR FileName, LPSTR OVFields, UINT Option)
   {
   }

/**************************************************************************

Function:       ASIN(X)
Description:    Returns the arc sine of the number X  (-1 <= X <= 1)
Prototype:      double ASIN(double X) ;

***************************************************************************/

#if defined(__WC386__)
double wd_ASIN(double X)
#else
double ASIN(double X)
#endif
   {
   if (X > 1.0 || X < -1.0)
      return(WD_ERR) ;

   return(asin(X)) ;
   }


/******************************************************************************

   FUNCTION:    ASSIGNNUM(fieldname, newnumber)

   DESCRIPTION: assign the given number to the given field
                fieldname:      name of field
                newnumber:      the new number to be assigned

******************************************************************************/

void ASSIGNNUM(LPSTR fieldname, double newnumber)
   {
   int i, unlock = 0 ;                                                    /* reset variables */
   char buffer[129] ;

   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(fieldname, fields[i]->Label))                          /* if fieldname corresponds with label */
#else
      if (!lstrcmpi((LPSTR)fieldname, (LPSTR)fields[i]->Label))                          /* if fieldname corresponds with label */
#endif
         {
         if (fields[i]->str == NULL)                                       /* if string is empty */
            {
            unlock = 1 ;                                                  /* remember: we lock something */
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* copy buffer to string, lock buffer */
            }
         if (fields[i]->str != NULL)                                       /* if string isn't empty */
            {
            sprintf(buffer, "%1.*lf", fields[i]->Decimals, (double) newnumber) ;     /* print number into buffer */
            lstrcpy(fields[i]->str, buffer) ;                              /* copy buffer into string */
            if (unlock)                                                   /* if we locked the buffer */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock it */
            }
#if !defined(__WC386__)
         InvalidateRect(NULL, &fields[i]->ValueRect, FALSE) ;
#else
         InvalidateRect(NULL, (LPRECT)&fields[i]->ValueRect, FALSE) ;
#endif
         return ;                                                         /* return to calling function */
         }
      }
   return ;                                                               /* return to calling function */
   }


/******************************************************************************

   FUNCTION:    ASSIGNSTR(fieldname, newstring)

   DESCRIPTION: assign the given string to the given field
                fieldname:      name of field
                newstring:      string to be assigned

******************************************************************************/

void ASSIGNSTR(LPSTR fieldname, LPSTR newstring)
   {
   int i, unlock = 0 ;                                                    /* reset variables */
   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(fieldname, fields[i]->Label))                          /* if fieldname corresponds to label */
#else
      if (!lstrcmpi((LPSTR)fieldname, (LPSTR)fields[i]->Label))                          /* if fieldname corresponds to label */
#endif
         {
         if (fields[i]->str == NULL)                                       /* if string is empty */
            {
            unlock = 1 ;                                                  /* set unlock true (we're locking something) */
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* lock buffer, and copy it to string */
            }
         if (fields[i]->str != NULL)                                       /* if string isn't empty */
            {
            lstrcpy(fields[i]->str, newstring) ;                           /* copy newstring into string */
            if (unlock)                                                   /* if we locked the buffer */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock it */
            }
#if !defined(__WC386__)
         InvalidateRect(NULL, &fields[i]->ValueRect, FALSE) ;
#else
         InvalidateRect(NULL, (LPRECT)&fields[i]->ValueRect, FALSE) ;
#endif
         return ;                                                         /* return to calling function */
         }
      }
   return ;                                                               /* return to calling function */
   }


/**************************************************************************

Function:       ATAN(X)
Description:    Returns the arc tangent of the number X
Prototype:      double ATAN(double X) ;
Notes:          The return value is the angle (in Radians) whose tangent
                is X.

***************************************************************************/

#if defined(__WC386__)
double wd_ATAN(double X)
#else
double ATAN(double X)
#endif
   {
   if (X == 0.0)
      return(WD_ERR) ;

   return(atan(X)) ;
   }

/**************************************************************************

Function:       ATAN2(X)
Description:    Returns the arc tangent of the number X
Prototype:      double ATAN2(double X, double Y) ;
Notes:          Returns the tangent of the angle represented by the point
                with coordinates (X,Y).  The result is the angle (in radians)
                whose tangent is Y/X.  The result is between -PI and PI with
                the quadrant chosen appropriately.

***************************************************************************/

#if defined(__WC386__)
double wd_ATAN2(double X, double Y)
#else
double ATAN2(double X, double Y)
#endif
   {
   if (X == 0.0 && Y == 0.0)
      return(WD_ERR) ;

   return(atan2(X, Y)) ;
   }

/**************************************************************************

Function:       AVG(List)
Description:    Calculates the average of the items in List
Prototype:      double AVG(double L1, ...)

Notes:          ObjectVision allows the AVG function to have up to 14
                parameters.  While C and C++ do allow for variable number
                of parameters in a function call, Pascal and dBASE for
                Windows do not.  Due to this limitation in Pascal and dBASE
                we have chosen to implement this function as a group of
                functions with differing numbers of parameters.  WinGEN will
                handle the translation from your ObjectVision AVG functions
                to this newer format for you automatically.

***************************************************************************/

double AVG(double L1, double L2)
   {
   return( (L1 + L2) / 2.0 ) ;
   }

double AVG3(double L1, double L2, double L3)
   {
   return( (L1 + L2 + L3) / 3.0 ) ;
   }

double AVG4(double L1, double L2, double L3, double L4)
   {
   return( (L1 + L2 + L3 + L4) / 4.0 ) ;
   }

double AVG5(double L1, double L2, double L3, double L4, double L5)
   {
   return( (L1 + L2 + L3 + L4 + L5) / 5.0 ) ;
   }

double AVG6(double L1, double L2, double L3, double L4, double L5, double L6)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6) / 6.0 ) ;
   }

double AVG7(double L1, double L2, double L3, double L4, double L5, double L6, double L7)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7) / 7.0 ) ;
   }

double AVG8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8) / 8.0 ) ;
   }

double AVG9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9) / 9.0 ) ;
   }

double AVG10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10) / 10.0 ) ;
   }

double AVG11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11) / 11.0 ) ;
   }

double AVG12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11 + L12) / 12.0 ) ;
   }

double AVG13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11 + L12 + L13) / 13.0 ) ;
   }

double AVG14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14)
   {
   return( (L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11 + L12 + L13 + L14) / 14.0 ) ;
   }

/**************************************************************************

Function:       BLANK
Description:    Allows you to return a blank for an invalid conlusion in a
                value tree

Prototype:      double BLANK(void) ;

***************************************************************************/

double BLANK(void)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       BTRVOPEN("LinkName", "Dictionary", "TableName", "DataFields",
                         "OVReadFields", "OVWriteFields", IndexNo, Option)

Description:    Opens a Btrieve file

Prototype:      void BTRVOPEN(LPSTR LinkName, LPSTR Dictionary,
                                         LPSTR TableName, LPSTR DataFields,
                                         LPSTR OVReadFields,
                                         LPSTR OVWriteFields, UINT IndexNo,
                                         UINT Option) ;

***************************************************************************/

void BTRVOPEN(LPSTR LinkName, LPSTR Dictionary, LPSTR TableName,
                         LPSTR Datafields, LPSTR OVReadFields,
                         LPSTR OVWriteFields, UINT IndexNo, UINT Option)
   {
   }

/**************************************************************************

Function:       CHAR(Code)
Description:    Returns the character associated with the given code.
Prototype:      int CHAR(UINT Code) ;

***************************************************************************/

int CHAR(UINT Code)
   {
   if (Code < 256)
      return((char)Code) ;
   else
      return(WD_ERR) ;
   }

/**************************************************************************

Function:       CHECKMENUITEM("MenuName", "MenuItemName")
Description:    Places a check mark next to a menu item name
Prototype:      void CHECKMENUITEM(LPSTR MenuName,
                                              LPSTR MenuItemName) ;

***************************************************************************/

void CHECKMENUITEM(LPSTR MenuName, LPSTR MenuItemName)
   {
   }


/**************************************************************************

Function:       CHOOSESTR(Number, List)
Description:    Returns the appropriate string from the List of strings

Prototypes:     LPSTR CHOOSESTR(UINT Number, LPSTR S1, ...)
                double CHOOSENUM(UINT Number, double D1, ...)

Notes:          ObjectVision allows you to use the @CHOOSE function with
                either strings or numeric values.  At a source code level
                you need to know which is being passed to operate on the
                passed values correctly.  This function also allows a
                variable number of parameters.  Due to restrictions in Pascal
                and dBASE for Windows we have taken the lowest common
                demoninator approach to programming these functions and
                have provided CHOOSESTR and CHOOSENUM in several varieties
                allowing for a variable number of parameters.  Any conversion
                necessary from your ObjectVision files is done automatically
                by WinGEN.

***************************************************************************/

LPSTR CHOOSESTR(UINT Number, LPSTR S0)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR2(UINT Number, LPSTR S0, LPSTR S1)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR3(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR4(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR5(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR6(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR7(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR8(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                              LPSTR S5, LPSTR S6, LPSTR S7)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR9(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                              LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      case 8:
         return(S8) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR10(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                               LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      case 8:
         return(S8) ;
      case 9:
         return(S9) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR11(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                               LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9, LPSTR S10)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      case 8:
         return(S8) ;
      case 9:
         return(S9) ;
      case 10:
         return(S10) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR12(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                               LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9,
                               LPSTR S10, LPSTR S11)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      case 8:
         return(S8) ;
      case 9:
         return(S9) ;
      case 10:
         return(S10) ;
      case 11:
         return(S11) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR13(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                               LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9,
                               LPSTR S10, LPSTR S11, LPSTR S12)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      case 8:
         return(S8) ;
      case 9:
         return(S9) ;
      case 10:
         return(S10) ;
      case 11:
         return(S11) ;
      case 12:
         return(S12) ;
      }
   return(NULL) ;
   }

LPSTR CHOOSESTR14(UINT Number, LPSTR S0, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4,
                               LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9,
                               LPSTR S10, LPSTR S11, LPSTR S12, LPSTR S13)
   {
   switch(Number)
      {
      case 0:
         return(S0) ;
      case 1:
         return(S1) ;
      case 2:
         return(S2) ;
      case 3:
         return(S3) ;
      case 4:
         return(S4) ;
      case 5:
         return(S5) ;
      case 6:
         return(S6) ;
      case 7:
         return(S7) ;
      case 8:
         return(S8) ;
      case 9:
         return(S9) ;
      case 10:
         return(S10) ;
      case 11:
         return(S10) ;
      case 12:
         return(S10) ;
      case 13:
         return(S10) ;
      }
   return(NULL) ;
   }

double CHOOSENUM(UINT Number, double N0)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      }
   return(NULL) ;
   }

double CHOOSENUM2(UINT Number, double N0, double N1)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      }
   return(NULL) ;
   }

double CHOOSENUM3(UINT Number, double N0, double N1, double N2)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      }
   return(NULL) ;
   }

double CHOOSENUM4(UINT Number, double N0, double N1, double N2, double N3)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      }
   return(NULL) ;
   }

double CHOOSENUM5(UINT Number, double N0, double N1, double N2, double N3, double N4)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      }
   return(NULL) ;
   }

double CHOOSENUM6(UINT Number, double N0, double N1, double N2, double N3, double N4, double N5)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      }
   return(NULL) ;
   }

double CHOOSENUM7(UINT Number, double N0, double N1, double N2, double N3, double N4, double N5, double N6)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      }
   return(NULL) ;
   }

double CHOOSENUM8(UINT Number, double N0, double N1, double N2, double N3, double N4,
                               double N5, double N6, double N7)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      }
   return(NULL) ;
   }

double CHOOSENUM9(UINT Number, double N0, double N1, double N2, double N3, double N4,
                               double N5, double N6, double N7, double N8)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      case 8:
         return(N8) ;
      }
   return(NULL) ;
   }

double CHOOSENUM10(UINT Number, double N0, double N1, double N2, double N3, double N4,
                                double N5, double N6, double N7, double N8, double N9)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      case 8:
         return(N8) ;
      case 9:
         return(N9) ;
      }
   return(NULL) ;
   }

double CHOOSENUM11(UINT Number, double N0, double N1, double N2, double N3, double N4,
                                double N5, double N6, double N7, double N8, double N9, double N10)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      case 8:
         return(N8) ;
      case 9:
         return(N9) ;
      case 10:
         return(N10) ;
      }
   return(NULL) ;
   }

double CHOOSENUM12(UINT Number, double N0, double N1, double N2, double N3, double N4,
                                double N5, double N6, double N7, double N8, double N9,
                                double N10, double N11)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      case 8:
         return(N8) ;
      case 9:
         return(N9) ;
      case 10:
         return(N10) ;
      case 11:
         return(N11) ;
      }
   return(NULL) ;
   }

double CHOOSENUM13(UINT Number, double N0, double N1, double N2, double N3, double N4,
                                double N5, double N6, double N7, double N8, double N9,
                                double N10, double N11, double N12)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      case 8:
         return(N8) ;
      case 9:
         return(N9) ;
      case 10:
         return(N10) ;
      case 11:
         return(N11) ;
      case 12:
         return(N12) ;
      }
   return(NULL) ;
   }

double CHOOSENUM14(UINT Number, double N0, double N1, double N2, double N3, double N4,
                                double N5, double N6, double N7, double N8, double N9,
                                double N10, double N11, double N12, double N13)
   {
   switch(Number)
      {
      case 0:
         return(N0) ;
      case 1:
         return(N1) ;
      case 2:
         return(N2) ;
      case 3:
         return(N3) ;
      case 4:
         return(N4) ;
      case 5:
         return(N5) ;
      case 6:
         return(N6) ;
      case 7:
         return(N7) ;
      case 8:
         return(N8) ;
      case 9:
         return(N9) ;
      case 10:
         return(N10) ;
      case 11:
         return(N11) ;
      case 12:
         return(N12) ;
      case 13:
         return(N13) ;
      }
   return(NULL) ;
   }


/**************************************************************************

Function:       CLOSE("LinkName")
Description:    Closes a currently open link
Prototype:      void CLOSE(LPSTR LinkName) ;

***************************************************************************/

void CLOSE(LPSTR LinkName)
   {
   CLOSEDDE(LinkName) ;
   return ;
   }



/**************************************************************************

Function:       CODE("String")
Description:    Returns the ANSI value for the first character of String
Prototype:      int CODE(LPSTR String) ;

***************************************************************************/

int CODE(LPSTR String)
   {
   if (String != NULL && *String != '\0')
      return(*String) ;

   return(WD_ERR) ;
   }

/**************************************************************************

Function:       COLUMNAVG("ColumnName")
Description:    Returns the average value of the visible entries in a column
Prototype:      double COLUMNAVG(LPSTR ColumnName) ;

***************************************************************************/

double COLUMNAVG(LPSTR ColumnName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       COLUMNCOUNT("ColumnName")
Description:    Counts the number of visible values in the column
Prototype:      double COLUMNCOUNT(LPSTR ColumnName) ;

***************************************************************************/

double COLUMNCOUNT(LPSTR ColumnName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       COLUMNMAX(ColumnName)
Description:    Returns the maximum visible value in the column ColumnName
Prototype:      double COLUMNMAX(LPSTR ColumnName) ;

***************************************************************************/

double COLUMNMAX(LPSTR ColumnName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       COLUMNMIN(ColumnName)
Description:    Returns the minimum visible value in the column ColumnName
Prototype:      double COLUMNMIN(LPSTR ColumnName) ;

***************************************************************************/

double COLUMNMIN(LPSTR ColumnName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       COLUMNSUM(ColumnName)
Description:    Returns the sum of the visible values in ColumnName
Prototype:      double COLUMNSUM(LPSTR ColumnName) ;

***************************************************************************/

double COLUMNSUM(LPSTR ColumnName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       COS(X)
Description:    Returns the cosine of the angle X (where X is in radians)
Prototype:      double COS(double X) ;

***************************************************************************/

#if defined(__WC386__)
double wd_COS(double X)
#else
double COS(double X)
#endif
   {
   if (X > 1.0 || X < -1.0)
      return(WD_ERR) ;

   return(cos(X)) ;
   }

/**************************************************************************

Function:       CTERM(Rate, Fv, Pv)
Description:    Calculates the number of time periods required for an
                investment of Pv to reach a value of Fv, while earning
                interest of Rate per compounding period.
Prototype:      double CTERM(double Rate, double Fv, double Pv) ;

***************************************************************************/

double CTERM(double Rate, double Fv, double Pv)
   {
   if (Rate <= 0.0 || Fv < Pv || Pv <= 0.0)
      return(WD_ERR) ;

   return( log(Fv / Pv) / log(1 + (Rate / 100)) ) ;
   }

/**************************************************************************

Function:       CURRENTFILE
Description:    Returns the name of the current application
Prototype:      LPSTR CURRENTFILE(void) ;
Notes:          In ObjectVision this returns the name of the currently
                opened .OVD file.  In a WinGEN generated application this
                will return the name of the .exe file.

***************************************************************************/

LPSTR CURRENTFILE(void)
   {
   return(NULL) ;
   }


/**************************************************************************

Function:       CURRENTPATH
Description:    Returns the path where the current .exe file is located
Prototype:      LPSTR CURRENTPATH(void) ;

***************************************************************************/

LPSTR CURRENTPATH(void)
   {
   return(NULL) ;
   }


/**************************************************************************

Function:       DATESERIAL(Yr, Mo, Day)
Description:    Returns the date serial # for the given date
Prototype:      long DATESERIAL(int Yr, int Mo, int Day) ;

***************************************************************************/

long DATESERIAL(int Yr, int Mo, int Day)
   {
   int j, i = 1800 ;
   long serial = -36523L ;

   if (Yr >= 0 && Yr <= 299)          /* Quattro Pro compatibility */
      Yr += 1800 ;

   while (i < Yr)
      {
      serial += 365L ;
      if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
         serial += 1L ;
      i++ ;
      }
   j = 1 ;
   while (j < Mo)
      {
      switch(j)
         {
         case 1:
            serial += 31L ;
            break ;
         case 2:
            serial += 28L ;
            if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
               serial += 1L ;
            break ;
         case 3:
            serial += 31L ;
            break ;
         case 4:
            serial += 30L ;
            break ;
         case 5:
            serial += 31L ;
            break ;
         case 6:
            serial += 30L ;
            break ;
         case 7:
            serial += 31L ;
            break ;
         case 8:
            serial += 31L ;
            break ;
         case 9:
            serial += 30L ;
            break ;
         case 10:
            serial += 31L ;
            break ;
         case 11:
            serial += 30L ;
            break ;
         }
      j++ ;
      }

   serial += Day ;

   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       DATEVALUE("DateString")
Description:    Returns the date serial # for the given date
                date must be in 1 of the following 5 formats
                     1.  DD-MMM-YY   "04-Jul-87"
                     2.  DD-MMM      "04-Jul"                assuming current year
                     3.  MMM-YY      "Jul-87"                assuming 1st day of month
                     4.  MM/DD/YY    "11/9/95"               Windows short format
                     5.  MMMM D, YYYY  January 1, 1992       Windows long format
                slashes and hyphens are interchangeable

Prototype:      long DATEVALUE(LPSTR DateValue) ;

***************************************************************************/

long DATEVALUE(LPSTR DateValue)
   {
   int i, dateformat, commas, seps, alpha ;
   int year, mon, day ;
   commas = seps = alpha = 0 ;

   /* determine date format */
   i = 0 ;
   while(DateValue[i])
      {
      if (DateValue[i] == '/' || DateValue[i] == '-')
         seps++ ;
      if (DateValue[i] == ',')
         commas++ ;
      if (DateValue[i] >= 'a' && DateValue[i] <= 'z')
         alpha = 1 ;
      if (DateValue[i] >= 'A' && DateValue[i] <= 'Z')
         alpha = 1 ;
      i++ ;
      }
   if (commas)
      dateformat = 5 ;
   else if (seps == 1 && DateValue[0] >= '0' && DateValue[0] <= '9')
      dateformat = 2 ;
   else if (seps == 1)
      dateformat = 3 ;
   else if (alpha)
      dateformat = 1 ;
   else
      dateformat = 4 ;

   switch(dateformat)
      {
      case 1:
      case 2:
         day = atoi(DateValue) ;
         i = 0 ;
         while(DateValue[i] && DateValue[i] != '-' && DateValue[i] != '/')       /* skip day */
            i++ ;
         while(DateValue[i] == '-' || DateValue[i] == '/')                       /* skip separator */
            i++ ;
         mon = MonTextToNumber(&DateValue[i]) ;
         if (dateformat == 2)
            year = YEAR(NOW()) ;
         else
            {
            while(DateValue[i] && DateValue[i] != '-' && DateValue[i] != '/')       /* skip month */
               i++ ;
            while(DateValue[i] == '-' || DateValue[i] == '/')                       /* skip separator */
               i++ ;
            year = atoi(&DateValue[i]) ;
            if (year < 100)
               year += 1900 ;
            }
         return (DATESERIAL(year, mon, day)) ;
      case 3:
         day = 1 ;
         mon = MonTextToNumber(DateValue) ;
         i = 0 ;
         while(DateValue[i] && DateValue[i] != '-' && DateValue[i] != '/')       /* skip day */
            i++ ;
         while(DateValue[i] == '-' || DateValue[i] == '/')                       /* skip separator */
            i++ ;
         year = atoi(&DateValue[i]) ;
         if (year < 100)
            year += 1900 ;
         return (DATESERIAL(year, mon, day)) ;
      case 4:
         mon = atoi(DateValue) ;
         i = 0 ;
         while(DateValue[i] && DateValue[i] != '-' && DateValue[i] != '/')       /* skip month */
            i++ ;
         while(DateValue[i] == '-' || DateValue[i] == '/')                       /* skip separator */
            i++ ;
         day = atoi(&DateValue[i]) ;
         while(DateValue[i] && DateValue[i] != '-' && DateValue[i] != '/')       /* skip day */
            i++ ;
         while(DateValue[i] == '-' || DateValue[i] == '/')                       /* skip separator */
            i++ ;
         year = atoi(&DateValue[i]) ;
         if (year < 100)
            year += 1900 ;
         return (DATESERIAL(year, mon, day)) ;
      case 5:
         mon = atoi(DateValue) ;
         i = 0 ;
         while(DateValue[i] && DateValue[i] != ' ')       /* skip month */
            i++ ;
         while(DateValue[i] == ' ')                       /* skip separator */
            i++ ;
         day = atoi(&DateValue[i]) ;
         while(DateValue[i] && DateValue[i] != ',')       /* skip day */
            i++ ;
         while(DateValue[i] == ',')                       /* skip separator */
            i++ ;
         year = atoi(&DateValue[i]) ;
         if (year < 100)
            year += 1900 ;
         return (DATESERIAL(year, mon, day)) ;
      }
   return(NOW()) ;
   }

int MonTextToNumber(LPSTR p)
   {
   if (p == NULL || p[0] == 0)
      return(1) ;

   if ((p[0] == 'J' || p[0] == 'j') &&
       (p[1] == 'A' || p[1] == 'a'))
      return(1) ;
   if (p[0] == 'F' || p[0] == 'f')
      return(2) ;
   if ((p[0] == 'M' || p[0] == 'm') &&
       (p[1] == 'A' || p[1] == 'a') &&
       (p[2] == 'R' || p[2] == 'r'))
      return(3) ;
   if ((p[0] == 'A' || p[0] == 'a') &&
       (p[1] == 'P' || p[1] == 'p'))
      return(4) ;
   if ((p[0] == 'M' || p[0] == 'm') &&
       (p[1] == 'A' || p[1] == 'a') &&
       (p[2] == 'Y' || p[2] == 'y'))
      return(5) ;
   if ((p[0] == 'J' || p[0] == 'j') &&
       (p[1] == 'U' || p[1] == 'u') &&
       (p[2] == 'N' || p[2] == 'n'))
      return(6) ;
   if ((p[0] == 'J' || p[0] == 'j') &&
       (p[1] == 'U' || p[1] == 'u') &&
       (p[2] == 'L' || p[2] == 'l'))
      return(7) ;
   if ((p[0] == 'A' || p[0] == 'a') &&
       (p[1] == 'U' || p[1] == 'u'))
      return(8) ;
   if (p[0] == 'S' || p[0] == 'f')
      return(9) ;
   if (p[0] == 'O' || p[0] == 'o')
      return(10) ;
   if (p[0] == 'N' || p[0] == 'n')
      return(11) ;
   if (p[0] == 'D' || p[0] == 'd')
      return(12) ;
   return(1) ;
   }


/**************************************************************************

Function:       DAY(DateTimeNumber)
Description:    Returns the day portion of the DateTimeNumber (1-31)
Prototype:      int DAY(double DateTimeNumber) ;

***************************************************************************/

int DAY(long DateTimeNumber)
   {
   int i = 1800 ;
   long days, oldserial, serial = -36522L ;

   oldserial = serial ;
   while (serial <= DateTimeNumber)
      {
      oldserial = serial ;
      serial += 365 ;
      if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
         serial += 1L ;
      i++ ;
      }
   i-- ;
   days = serial - oldserial ;
   if (days <= 31L)
      return((int) days) ;
   days -= 31L ;
   if (days <= 28L || (days <= 29L && (!(i % 4) && ((i % 100) || !(i % 400)))))
      return((int) days) ;
   days -= 28L ;
   if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
      days-- ;
   if (days <= 31L)
      return((int) days) ;
   days -= 31L ;
   if (days <= 30L)
      return((int) days) ;
   days -= 30L ;
   if (days <= 31L)
      return((int) days) ;
   days -= 31L ;
   if (days <= 30L)
      return((int) days) ;
   days -= 30L ;
   if (days <= 31L)
      return((int) days) ;
   days -= 31L ;
   if (days <= 31L)
      return((int) days) ;
   days -= 31L ;
   if (days <= 30L)
      return((int) days) ;
   days -= 30L ;
   if (days <= 31L)
      return((int) days) ;
   days -= 31L ;
   if (days <= 30L)
      return((int) days) ;
   days -= 30L ;
   return((int) days) ;
   }

/**************************************************************************

Function:       DBOPEN("LinkName", "FileName", "DataFields",
                       "OVReadFields", "OVWriteFields", "IndexFile", Option)

Description:    Opens a dBASE file

Prototype:      void DBOPEN(LPSTR LinkName, LPSTR Filename,
                                         LPSTR DataFields,
                                         LPSTR OVReadFields,
                                         LPSTR OVWriteFields, LPSTR IndexFile,
                                         UINT Option) ;

***************************************************************************/

void DBOPEN(LPSTR LinkName, LPSTR Filename, LPSTR DataFields,
                       LPSTR OVReadFields, LPSTR OVWriteFields, LPSTR IndexFile,
                       UINT Option)
   {
   }

/**************************************************************************

Function:       DDB(Cost, Salvage, Life, Period)
Description:    Determines accelerated depreciation values for an asset
Prototype:      double DDB(double Cost, double Salvage,
                                      double Life, double Period) ;

***************************************************************************/

double DDB(double Cost, double Salvage, double Life, double Period)
   {
   return(WD_TRUE) ;
   }


/* DDE Global Variables */
struct WINEXE_DDE_INFO ddeinfo[MAX_DDE_COUNT] ;                           /* hold dde information here */
DWORD          idInstDDE = 0L ;                                           /* global dde application-instance identifier */
FARPROC        lpProcDDE ;                                                /* dde callback function */
HSZ            hServerDDE = NULL ;                                        /* dde string for server name */
HSZ            hTopicDDE = NULL ;                                         /* dde string for topic name */
int            ServerModeDDE = 0 ;                                        /* are we in DDE Server mode ? */

/**************************************************************************

Function:       DDEEXECUTE("LinkName", "[Commands]")
Description:    Send commands to the linked DDE application
Prototype:      void DDEEXECUTE(LPSTR LinkName, LPSTR Commands) ;

***************************************************************************/

void DDEEXECUTE(LPSTR LinkName, LPSTR Commands)
   {
#if !defined(__WC386__)
   char buffer[129] ;
   HDDEDATA hData ;
   int i ;

   for (i = 0 ; i < MAX_DDE_COUNT ; i++)
      if (!lstrcmp(ddeinfo[i].name, LinkName))
         break ;
   if (i >= MAX_DDE_COUNT)
      {
      lstrcpy(buffer, "Unknown Link Name: ") ;
      lstrcat(buffer, LinkName) ;
      lstrcat(buffer, " in function DDEEXECUTE.") ;
      MessageBox(NULL, buffer, NULL, MB_OK) ;
      return ;
      }
   if (!ddeinfo[i].status)
      {
      lstrcpy(buffer, "Link: ") ;
      lstrcat(buffer, LinkName) ;
      lstrcat(buffer, " not active in function DDEEXECUTE.") ;
      MessageBox(NULL, buffer, NULL, MB_OK) ;
      return ;
      }

   hData = DdeCreateDataHandle(idInstDDE, (LPBYTE) Commands, lstrlen(Commands) + 1, 0L, NULL, CF_TEXT, 0) ;
   DdeClientTransaction((LPBYTE) hData, -1, ddeinfo[i].hConv, NULL, NULL, XTYP_EXECUTE, 1000, NULL) ;
   return ;
#endif
   }

/**************************************************************************

Function:       DDEOPEN("LinkName", "Application", "Document", "RemoteNames",
                        "OVFields")

Description:    Opens a link to another application using DDE

Prototype:      void DDEOPEN(LPSTR LinkName, LPSTR Application,
                                        LPSTR Document, LPSTR RemoteNames,
                                        LPSTR OVFields) ;

***************************************************************************/

void DDEOPEN(LPSTR LinkName, LPSTR Application, LPSTR Document,
                        LPSTR RemoteNames, LPSTR OVFields)
   {
#if !defined(__WC386__)
   int i, j, savechs, savechc, flidx ;
   UINT dderc ;
   char buffer[256] ;
   LPSTR sptr, sptr1 ;
   LPSTR cptr, cptr1 ;
   HSZ hszfld ;

   /* check for link already open */
   for (i = 0 ; i < MAX_DDE_COUNT ; i++)
      {
      if (!lstrcmp(ddeinfo[i].name, LinkName) && ddeinfo[i].status)
         {
         lstrcpy(buffer, "DDE Link: ") ;
         lstrcat(buffer, LinkName) ;
         lstrcat(buffer, " already open.") ;
         MessageBox(NULL, buffer, NULL, MB_OK) ;
         return ;
         }
      }

   /* find first available slot */
   for (i = 0 ; i < MAX_DDE_COUNT ; i++)
      if (!ddeinfo[i].status)
         break ;

   if (i == MAX_DDE_COUNT)
      {
      MessageBox(NULL, "Maximum number of DDE Links already open.", NULL, MB_OK) ;
      return ;
      }

   if (idInstDDE == 0L)          /* check for previous call to DdeInitialize */
      {
      lpProcDDE = MakeProcInstance((FARPROC) DdeCallBack, hInst) ;
      if (!lpProcDDE)
         {
         MessageBox(NULL, "Unable to create DDE callback procedure.", NULL, MB_OK) ;
         return ;
         }
      dderc = DdeInitialize((LPDWORD) &idInstDDE, (PFNCALLBACK) lpProcDDE, APPCLASS_STANDARD, 0L) ;
      if (dderc != DMLERR_NO_ERROR)
         {
         wsprintf((LPSTR) buffer,
                  (LPSTR) "Error code %d returned from call to DdeInitialize.",
                  dderc) ;
         MessageBox(NULL, buffer, NULL, MB_OK) ;
         FreeProcInstance((FARPROC) lpProcDDE) ;
         return ;
         }
      }

   if (!lstrcmpi(Application, "VISION"))        /* calling an objectvision ovd, convert to .exe and bypass objectvision */
      {
      lstrcpy(buffer, Document) ;      /* ovd file name, remove extension */
      j = lstrlen(buffer) - 1 ;
      while (j >= 0 && buffer[j] != '.')
         j-- ;
      if (buffer[j] == '.')
         buffer[j] = 0 ;
#if defined(__C700__)
      _strupr(buffer) ;
#else
      strupr(buffer) ;
#endif
      ddeinfo[i].hService = DdeCreateStringHandle(idInstDDE, buffer, 0) ;
      ddeinfo[i].hTopic = DdeCreateStringHandle(idInstDDE, "FieldChanges", 0) ;
      }
   else
      {
      ddeinfo[i].hService = DdeCreateStringHandle(idInstDDE, Application, 0) ;
      ddeinfo[i].hTopic = DdeCreateStringHandle(idInstDDE, Document, 0) ;
      }
   ddeinfo[i].hConv = DdeConnect(idInstDDE, ddeinfo[i].hService, ddeinfo[i].hTopic, (LPVOID) NULL) ;
   if (!ddeinfo[i].hConv)
      {
      MessageBox(NULL, "Unable to connect to server via call to DdeConnect().", NULL, MB_OK) ;
      DdeFreeStringHandle(idInstDDE, ddeinfo[i].hService) ;
      DdeFreeStringHandle(idInstDDE, ddeinfo[i].hTopic) ;
      return ;
      }

   ddeinfo[i].status = 1 ;
   lstrcpy(ddeinfo[i].name, LinkName) ;
   if (RemoteNames != NULL)
      lstrcpy(ddeinfo[i].sflds, RemoteNames) ;
   else
      ddeinfo[i].sflds[0] = 0 ;
   if (OVFields != NULL)
      lstrcpy(ddeinfo[i].cflds, OVFields) ;
   else
      ddeinfo[i].cflds[0] = 0 ;

   /* dde initialization complete, now inform the server of fields we need to watch for changes */
   if (!ddeinfo[i].sflds[0] || !ddeinfo[i].cflds[0])        /* no fields specified */
      return ;

   sptr = ddeinfo[i].sflds ;            /* pointer to server fields */
   cptr = ddeinfo[i].cflds ;            /* pointer to client fields */
   while (*sptr)
      {
      sptr1 = sptr ;                    /* find end of this server field */
      while (*sptr1 && *sptr1 != ',')
         sptr1++ ;
      savechs = *sptr1 ;
      *sptr1 = 0 ;

      cptr1 = cptr ;                    /* find end of this client field */
      while (*cptr1 && *cptr1 != ',')
         cptr1++ ;
      savechc = *cptr1 ;
      *cptr1 = 0 ;
      for (flidx = 0 ; flidx < FIELD_COUNT ; flidx++)        /* find client field in list of fields */
         if (!lstrcmpi(fields[flidx]->Label, cptr))
            break ;

      hszfld = DdeCreateStringHandle(idInstDDE, sptr, 0) ;
      if (!DdeClientTransaction(NULL, NULL, ddeinfo[i].hConv, hszfld, CF_TEXT, XTYP_ADVSTART, 1000, NULL))
         {
         lstrcpy(buffer, "Unable to advise server for field changes on field: ") ;
         lstrcat(buffer, sptr) ;
         MessageBox(NULL, buffer, "Warning", MB_OK) ;
         }
      if (flidx < FIELD_COUNT)
         fields[flidx]->hFieldDDE[i] = hszfld ;
      else
         {
         DdeFreeStringHandle(idInstDDE, hszfld) ;
         lstrcpy(buffer, "Client field: ") ;
         lstrcat(buffer, " not found.") ;
         MessageBox(NULL, buffer, "Warning", MB_OK) ;
         }

      *sptr1 = savechs ;
      *cptr1 = savechc ;
      if (savechs == 0)
         break ;
      sptr = sptr1 + 1 ;
      cptr = cptr1 + 1 ;
      }

   return ;
#endif
   }

/* DDE Callback function.  Contains both client and server code */
HDDEDATA EXPENTRY DdeCallBack(WORD wType, WORD wFmt, HCONV hConv, HSZ hsz1, HSZ hsz2,
                              HDDEDATA hData, DWORD lData1, DWORD lData2)
   {
#if !defined(__WC386__)
   char buffer[256] ;
   WORD i, j ;
   HSZPAIR hszPair[2] ;                                                   /* only have 1 topic (FieldChanges) */
   HSZ hString ;
   int found, ddeslot ;

   for (ddeslot = 0, i = 0 ; i < MAX_DDE_COUNT ; i++)
      {
      if (ddeinfo[i].status && ddeinfo[i].hConv == hConv)
         {
         ddeslot = i ;
         break ;
         }
      }

   /* server code */

   if (wType == XTYP_CONNECT)                                             /* connect to server */
      {
      if (hServerDDE == hsz2)
         {
         if (hTopicDDE == hsz1)
            return( (HDDEDATA) TRUE) ;
         }
      }
   if (wType == XTYP_WILDCONNECT)                                         /* wild card connect */
      {
      if (hsz2 != hServerDDE && hsz2 != NULL)
         return( (HDDEDATA) FALSE) ;

      for (i = 0, j = 0 ; i < 1 ; i++)
         {
         if (hsz1 == NULL || hsz1 == hTopicDDE)
            {
            hszPair[j].hszSvc = hServerDDE ;
            hszPair[j].hszTopic = hTopicDDE ;
            j++ ;
            }
         }
      hszPair[j].hszSvc = (HSZ) NULL ;
      hszPair[j].hszTopic = (HSZ) NULL ;
      return( (HDDEDATA) DdeCreateDataHandle(idInstDDE, (LPBYTE) &hszPair[0], sizeof(hszPair), 0L, 0, CF_TEXT, 0) ) ;
      }
   if (wType == XTYP_ADVSTART)    /* as server we are being advised to notify the client when a server field changes */
      {
      if (wFmt == CF_TEXT && hTopicDDE == hsz1)
         {
         for (found = 0, i = 0 ; i < FIELD_COUNT ; i++)
            {
            hString = DdeCreateStringHandle(idInstDDE, fields[i]->Label, 0) ;
            if (hString == hsz2)         /* hsz2 contains the field label to watch for changes on */
               {
               found = 1 ;
               fields[i]->hFieldDDE[ddeslot] = hString ;
               }
            else
               DdeFreeStringHandle(idInstDDE, hString) ;
            }
         if (found)
            {
            ServerModeDDE = 1 ;                   /* Now in DDE Server mode */
            return( (HDDEDATA) TRUE) ;
            }
         else
            return( (HDDEDATA) FALSE) ;
         }
      }
   if (wType == XTYP_ADVREQ || wType == XTYP_REQUEST)  /* as server we must notify the client of a field change (XTYP_ADVREQ)  */
      {                                                /* as server we have been requested to return data       (XTYP_REQUEST) */
      if (wFmt == CF_TEXT && hTopicDDE == hsz1 && hsz2 != NULL)
         {
         for (i = 0 ; i < FIELD_COUNT ; i++)
            if (fields[i]->hFieldDDE[ddeslot] == hsz2)
               {
               if (fields[i]->str == NULL)
                  fields[i]->str = GlobalLock(fields[i]->Buffer) ;
               return((HDDEDATA)DdeCreateDataHandle(idInstDDE, (LPBYTE) fields[i]->str,
                                                    lstrlen(fields[i]->str) + 1, 0L, hsz2, CF_TEXT, 0)) ;
               }
         }
      }
   if (wType == XTYP_ADVSTOP)  /* as server we are being told to stop notifying client of changes in a given field */
      {
      for (i = 0 ; hsz2 != NULL && i < FIELD_COUNT ; i++)
         if (fields[i]->hFieldDDE[ddeslot] == hsz2)
            {
            DdeFreeStringHandle(idInstDDE, fields[i]->hFieldDDE[ddeslot]) ;
            fields[i]->hFieldDDE[ddeslot] = NULL ;
            }
      }
   if (wType == XTYP_POKE)  /* as server we have been 'poked' with new data */
      {
      for (i = 0 ; hsz2 != NULL && i < FIELD_COUNT ; i++)
         if (fields[i]->hFieldDDE[ddeslot] == hsz2 && wFmt == CF_TEXT)
            {
            if (fields[i]->str == NULL)
               fields[i]->str = GlobalLock(fields[i]->Buffer) ;
            DdeGetData(hData, (LPBYTE) fields[i]->str, fields[i]->Length, 0L) ;
            RedrawFields(NULL) ;
            return( (HDDEDATA) DDE_FACK ) ;
            }
      return((HDDEDATA) DDE_FNOTPROCESSED) ;
      }
   if (wType == XTYP_EXECUTE)  /* as server we have been ordered to 'execute' someone or something */
      {
      DdeGetData(hData, (LPBYTE) buffer, 256, 0L) ;
      ExecuteDDECommand(buffer) ;
      return( (HDDEDATA) DDE_FACK ) ;
      }

   /* client code */

   if (wType == XTYP_ADVDATA)  /* as client we have been told that a server field has changed */
      {
      for (i = 0 ; hsz2 != NULL && i < FIELD_COUNT ; i++)
         if (fields[i]->hFieldDDE[ddeslot] == hsz2)
            {
            if (fields[i]->str == NULL)
               fields[i]->str = GlobalLock(fields[i]->Buffer) ;
            DdeGetData(hData, (LPBYTE) fields[i]->str, fields[i]->Length, 0L) ;
            RedrawFields(NULL) ;
            return( (HDDEDATA) DDE_FACK ) ;
            }
      }

#endif
   return( (HDDEDATA) FALSE ) ;
   }

void ExecuteDDECommand(LPSTR Command)
   {
#if !defined(__WC386__)
   char buffer[256] ;
   LPSTR oper, arg[11] ;
   int args = 0 ;
   int i, unlock, instring ;

   for (i = 0 ; i < 11 ; i++)
      arg[i] = NULL ;
   lstrcpy(buffer, Command) ;

   i = 0 ;
   while (buffer[i] == '[' || buffer[i] == '@')
      i++ ;

   oper = &buffer[i] ;
   instring = 0 ;
   while (buffer[i])            /* parse out the arguments, null terminate each one */
      {
      switch(buffer[i])
         {
         case '(':
            buffer[i] = 0 ;
            break ;
         case '"':
            instring = (instring) ? 0 : 1 ;
            if (instring)
               arg[args++] = &buffer[i + 1] ;
            else
               buffer[i] = 0 ;
            break ;
         }
      i++ ;
      }

   /* now we have the operator and the arguments parsed out */
   if (!lstrcmpi(oper, "FIELDCLEAR"))           /* FIELDCLEAR operation */
      {
      for (i = 0 ; arg[0] != NULL && i < FIELD_COUNT ; i++)
         if (!lstrcmpi(arg[0], fields[i]->Label))
            {
            if (fields[i]->str == NULL)
               {
               unlock = 1 ;
               fields[i]->str = GlobalLock(fields[i]->Buffer) ;
               }
            fields[i]->str[0] = 0 ;
            if (unlock)
               {
               GlobalUnlock(fields[i]->Buffer) ;
               fields[i]->str = NULL ;
               }
            }
      }

   /* add additional commands here */

   return ;
#endif
   }

/* DDE initialization code */
BOOL InitializeDDEML(void)
   {
#if !defined(__WC386__)
   char buffer[129] ;
   UINT dderc ;
   int i ;
   DWORD dwWinFlags ;

   dwWinFlags = GetWinFlags() ;
   if (dwWinFlags & WF_PMODE)
      ;    /* okay to run */
   else
      {
      MessageBox(NULL, "DDEML services NOT available in REAL mode Windows", NULL, MB_OK) ;
      return(FALSE) ;
      }

   for (i = 0 ; i < MAX_DDE_COUNT ; i++)
      {
      ddeinfo[i].status = 0 ;
      ddeinfo[i].name[0] = 0 ;
      }

   lpProcDDE = MakeProcInstance((FARPROC) DdeCallBack, hInst) ;
   if (!lpProcDDE)
      {
      MessageBox(NULL, "Unable to create DDE callback procedure.", NULL, MB_OK) ;
      return(FALSE) ;
      }
   dderc = DdeInitialize((LPDWORD) &idInstDDE, (PFNCALLBACK) lpProcDDE, APPCLASS_STANDARD, 0L) ;
   if (dderc != DMLERR_NO_ERROR)
      {
      wsprintf((LPSTR) buffer,
               (LPSTR) "Error code %d returned from call to DdeInitialize.",
               dderc) ;
      MessageBox(NULL, buffer, NULL, MB_OK) ;
      FreeProcInstance((FARPROC) lpProcDDE) ;
      return(FALSE) ;
      }
   hServerDDE = DdeCreateStringHandle(idInstDDE, DDEServerName, 0) ;
   hTopicDDE = DdeCreateStringHandle(idInstDDE, "FieldChanges", 0) ;
   DdeNameService(idInstDDE, hServerDDE, NULL, DNS_REGISTER) ;

   /* now that DDEML is up and running, open all defined DDE links */
   i = 0 ;
   while (ddelinks[i].LinkName != NULL)
      {
      DDEOPEN(ddelinks[i].LinkName, ddelinks[i].Server, ddelinks[i].Topic, ddelinks[i].ClientFields, ddelinks[i].ServerFields) ;
      i++ ;
      }

#endif
   return(TRUE) ;
   }

/* DDE clean up code */
BOOL CleanUpDDEML(void)
   {
#if !defined(__WC386__)
   int i ;

   if (lpProcDDE)
      {
      DdeNameService(idInstDDE, hServerDDE, NULL, DNS_UNREGISTER) ;
      for (i = 0 ; i < MAX_DDE_COUNT ; i++)         /* disconnect all open connections */
         {
         if (ddeinfo[i].status)
            DdeDisconnect(ddeinfo[i].hConv) ;
         ddeinfo[i].status = 0 ;
         ddeinfo[i].hConv = NULL ;
         }
      if (hServerDDE != NULL)
         DdeFreeStringHandle(idInstDDE, hServerDDE) ;
      if (hTopicDDE != NULL)
         DdeFreeStringHandle(idInstDDE, hTopicDDE) ;
      DdeUninitialize(idInstDDE) ;
      FreeProcInstance((FARPROC) lpProcDDE) ;
      hServerDDE = NULL ;
      hTopicDDE = NULL ;
      idInstDDE = 0L ;
      lpProcDDE = NULL ;
      }
   ServerModeDDE = 0 ;
#endif
   return(TRUE) ;
   }

/* Close the given DDE Link */

void CLOSEDDE(LPSTR LinkName)
   {
#if !defined(__WC386__)
   int i, ddeslot ;

   for (ddeslot = -1, i = 0 ; i < MAX_DDE_COUNT ; i++)
      {
      if (!lstrcmp(ddeinfo[i].name, LinkName))
         {
         ddeslot = i ;
         break ;
         }
      }
   if (ddeslot >= 0)
      {
      for (i = 0 ; i < FIELD_COUNT ; i++)
         {
         if (fields[i]->hFieldDDE[ddeslot] != NULL)
            DdeFreeStringHandle(idInstDDE, fields[i]->hFieldDDE[ddeslot]) ;
         fields[i]->hFieldDDE[ddeslot] = NULL ;
         }
      if (ddeinfo[ddeslot].hService != NULL)
         DdeFreeStringHandle(idInstDDE, ddeinfo[ddeslot].hService) ;
      if (ddeinfo[ddeslot].hTopic != NULL)
         DdeFreeStringHandle(idInstDDE, ddeinfo[ddeslot].hTopic) ;
      DdeDisconnect(ddeinfo[ddeslot].hConv) ;
      ddeinfo[ddeslot].hConv = NULL ;
      ddeinfo[ddeslot].status = 0 ;
      ddeinfo[ddeslot].name[0] = 0 ;
      }
   return ;
#endif
   }


/**************************************************************************

Function:       DDEPOKESTR("LinkName", "RemoteName", Value)
Description:    Sends a value to a DDE linked application

Prototypes:     void DDEPOKESTR(LPSTR LinkName, LPSTR RemoteName,
                                           LPSTR Value) ;
                void DDEPOKENUM(LPSTR LinkName, LPSTR RemoteName,
                                           double Value) ;

Notes:          ObjectVision allows the automatic interchange of numeric and
                string values when calling functions.  Programs using C, C++,
                Pascal, and dBASE must know what type of value is being sent
                so we have converted the simple DDEPOKE to DDEPOKESTR and
                DDEPOKENUM.  This conversion is handled automatically for you
                by WinGEN when you load in an .OVD file.

***************************************************************************/

void DDEPOKESTR(LPSTR LinkName, LPSTR RemoteName, LPSTR Value)
   {
#if !defined(__WC386__)
   char buffer[129] ;
   HSZ hszfld ;
   HDDEDATA hData ;
   int i ;

   for (i = 0 ; i < MAX_DDE_COUNT ; i++)
      if (!lstrcmp(ddeinfo[i].name, LinkName))
         break ;
   if (i >= MAX_DDE_COUNT)
      {
      lstrcpy(buffer, "Unknown Link Name: ") ;
      lstrcat(buffer, LinkName) ;
      lstrcat(buffer, " in function DDEPOKESTR.") ;
      MessageBox(NULL, buffer, NULL, MB_OK) ;
      return ;
      }
   if (!ddeinfo[i].status)
      {
      lstrcpy(buffer, "Link: ") ;
      lstrcat(buffer, LinkName) ;
      lstrcat(buffer, " not active in function DDEPOKESTR.") ;
      MessageBox(NULL, buffer, NULL, MB_OK) ;
      return ;
      }

   hszfld = DdeCreateStringHandle(idInstDDE, RemoteName, 0) ;
   hData = DdeCreateDataHandle(idInstDDE, (LPBYTE) Value, lstrlen(Value) + 1, 0L, hszfld, CF_TEXT, 0) ;
   DdeClientTransaction((LPBYTE) hData, -1, ddeinfo[i].hConv, hszfld, CF_TEXT, XTYP_POKE, 1000, NULL) ;
   DdeFreeStringHandle(idInstDDE, hszfld) ;
   return ;
#endif
   }

void DDEPOKENUM(LPSTR LinkName, LPSTR RemoteName, double Value)
   {
#if !defined(__WC386__)
   char buffer[129] ;
   sprintf(buffer, "%lf", (double) Value) ;
   DDEPOKESTR(LinkName, RemoteName, (LPSTR) buffer) ;
#endif
   return ;
   }



/**************************************************************************

Function:       DEGREES(X)
Description:    Converts the angle X from radians to degrees
Prototype:      double DEGREES(double X) ;

***************************************************************************/

double DEGREES(double X)
   {
#if !defined(__ZTC__)
   return(X * 180.0 / PI()) ;  /* uses the PI function */
#else
   return(X * 180.0 / wd_PI()) ;
#endif
   }


/**************************************************************************

Function:       DELETEMENU("MenuName")
Description:    Deletes a Menu Command (i.e. "File", "Edit", etc.)
Prototype:      void DELETEMENU(LPSTR MenuName) ;

***************************************************************************/

void DELETEMENU(LPSTR MenuName)
   {
   }


/**************************************************************************

Function:       DELETEMENUITEM("MenuName", "MenuItemName")
Description:    Delete a menu item name (i.e. "Open", "Save", etc...)
Prototype:      void DELETEMENUITEM(LPSTR MenuName,
                                               LPSTR MenuItemName) ;

***************************************************************************/

void DELETEMENUITEM(LPSTR MenuName, LPSTR MenuItemName)
   {
   }

/**************************************************************************

Function:       ERR
Description:    Returns the value ERR
Prototype:      double ERR(void) ;

***************************************************************************/

double ERR(void)
   {
   return(WD_ERR) ;
   }

/**************************************************************************

Function:       EVENT("Item", "Event")
Description:    Sends an event to a control
Prototype:      void EVENT(LPSTR Item, LPSTR Event) ;
Notes:          This function can send either a standard event or a
                user-defined event to a control.  If the Item parameter is
                blank, then the event is sent to the stack.  Item is first
                searched for as a Field, then as a Form, then to the Stack.

***************************************************************************/

void EVENT(LPSTR Item, LPSTR Event)
   {
   }

/**************************************************************************

Function:       EXACT("String1", "String2")
Description:    Returns WD_TRUE if the strings are exactly alike
Prototype:      double EXACT(LPSTR String1, LPSTR String2) ;
Notes:          As a reference on the details of this function please see
                the ObjectVision Reference Guide on this function.

***************************************************************************/

double EXACT(LPSTR String1, LPSTR String2)
   {
   if (!lstrcmp(String1, String2))
      return(WD_TRUE) ;
   else
      return(WD_FALSE) ;
   }


/**************************************************************************

Function:       EXEC("CommandLine", ShowOption)
Description:    Open and execute other Windows applications
Prototype:      void EXEC(LPSTR CommandLine, UINT ShowOption) ;
                ShowOption: 0==Normal, 1==Minimized, 2=Maximized

***************************************************************************/

void EXEC(LPSTR CommandLine, UINT ShowOption)
   {
   char buffer[256] ;
   int rc ;

   if (ShowOption == 1)
      rc = WinExec(CommandLine, SW_SHOWMINIMIZED) ;
   else if (ShowOption == 2)
      rc = WinExec(CommandLine, SW_SHOWMAXIMIZED) ;
   else
      rc = WinExec(CommandLine, SW_SHOW) ;
   if (rc < 32)                                                           /* if an error code was returned */
      {
      lstrcpy(buffer, "The following Windows Error Message was encountered ") ;
      lstrcat(buffer, "attempting to EXEC command: ") ;
      lstrcat(buffer, CommandLine) ;
      lstrcat(buffer, "\n\n") ;                                            /* prepare error message */
      switch(rc)                                                           /* identify error code */
         {                                                                 /* and get corresponding message */
         case 0:
            lstrcat(buffer, "System was out of memory, executable file was corrupt, or relocations were invalid.") ;
            break ;
         case 2:
            lstrcat(buffer, "File was not found.") ;
            break ;
         case 3:
            lstrcat(buffer, "Path was not found.") ;
            break ;
         case 5:
            lstrcat(buffer, "Attempt was made to dynamically link to a task, or there was a sharing or network error.") ;
            break ;
         case 6:
            lstrcat(buffer, "Library required separate data segments for each task.") ;
            break ;
         case 8:
            lstrcat(buffer, "There was insufficient memory to start the application") ;
            break ;
         case 10:
            lstrcat(buffer, "Windows version was incorrect.") ;
            break ;
         case 11:
            lstrcat(buffer, "Executable file image was invalid or not a Windows application.") ;
            break ;
         case 12:
            lstrcat(buffer, "Application was designed for a different operating system.") ;
            break ;
         case 13:
            lstrcat(buffer, "Application was designed for MS-DOS 4.0") ;
            break ;
         case 14:
            lstrcat(buffer, "Type of executable file was unknown.") ;
            break ;
         case 15:
            lstrcat(buffer, "Attempt was made to load a real-mode application for an earlier version of Windows.") ;
            break ;
         case 16:
            lstrcat(buffer, "Attempt was made to load a 2nd instance of an application with not read-only data segments.") ;
            break ;
         case 19:
            lstrcat(buffer, "Attempt was made to load a compressed executable file that must be decompressed first.") ;
            break ;
         case 20:
            lstrcat(buffer, "One of the required DLL files was invalid or corrupt.") ;
            break ;
         case 21:
            lstrcat(buffer, "Application requires Microsoft Windows 32-bit extensions.") ;
            break ;
         default:
            lstrcat(buffer, "Unknown Microsoft Windows Error Return Code.") ;
            break ;
         }
      MessageBox(NULL, buffer, NULL, MB_ICONSTOP | MB_OK) ;               /* display error message in message box */
      InvalidateRect(forms[currentformindex].hWnd, NULL, TRUE) ;          /* invalidate rectangle */
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update window */
      }
   else                                                                   /* load was successful, application run */
      DestroyWindow(hwnd) ;                                               /* end application */
   }

/**************************************************************************

Function:       EXP(X)
Description:    Returns e raised to the Xth power (X <= 43)
Prototype:      double EXP(double X) ;

***************************************************************************/

double EXP(double X)
   {
   if (X > 43.0)
      return(WD_ERR) ;

   return (exp(X)) ;
   }


/**************************************************************************

Function:       FIELDCALCULATE(FieldName)
Description:    Calculates the value of a field
Prototype:      double FIELDCALCULATE(LPSTR FieldName) ;

***************************************************************************/

double FIELDCALCULATE(LPSTR FieldName)
   {
   return(FIELDNUM(FieldName)) ;
   }

/**************************************************************************

Function:       FIELDCLEAR(FieldName)
Description:    Clears the value of FieldName
Prototype:      double FIELDCLEAR(LPSTR FieldName) ;

***************************************************************************/

double FIELDCLEAR(LPSTR fieldname)
   {
   int i, unlock = 0 ;
   for (i = 0 ; i < FIELD_COUNT ; i++)
      {
      if (!lstrcmpi(fieldname, fields[i]->Label) && fields[i]->formid == forms[currentformindex].id)
         {
         if (fields[i]->str == NULL)
            {
            unlock = 1 ;
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;
            }
         fields[i]->str[0] = 0 ;
         if (unlock)
            GlobalUnlock(fields[i]->Buffer) ;
         return(WD_TRUE) ;
         }
      }
   return(WD_TRUE) ;
   }

/******************************************************************************

   FUNCTION:    FIELDFIND(fieldname)

******************************************************************************/

void FIELDFIND(LPSTR fieldname)
   {
   int i ;
   for (i = 0 ; i < FIELD_COUNT ; i++)
      {
      if (!lstrcmpi(fieldname, fields[i]->Label) && fields[i]->formid == forms[currentformindex].id)
         {
         FieldEdit(i) ;
         break ;
         }
      }
   return ;                                                               /* return to calling function */
   }

/*****************************************************************************

   FUNCTION:    FIELDNUM(fieldname)

   DESCRIPTION: return the numeric value of the given field
                fieldname:      name of field

*****************************************************************************/

double FIELDNUM(LPSTR fieldname)
   {
   int i, unlock = 0 ;                                                    /* reset variables */
   double doub ;
   char buffer[129] ;
   LPHANDLE ph ;

   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(fieldname, fields[i]->Label))                          /* if fieldname corresponds to label */
#else
      if (!lstrcmpi((LPSTR)fieldname, (LPSTR)fields[i]->Label))                          /* if fieldname corresponds to label */
#endif
         {
         if (fields[i]->ColBuffer != NULL)                                 /* if it's a table */
            {
            if (fields[i]->str != NULL)                                    /* if string isn't empty */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock buffer */
            fields[i]->str = NULL ;                                        /* clear string */
            ph = (LPHANDLE) GlobalLock(fields[i]->ColBuffer) ;             /* lock column buffer, get its handle */
            if (ph != NULL)                                               /* if handle exists */
               {
               if (ph[0] != NULL)                                         /* if first element exists */
                   fields[i]->Buffer = ph[0] ;                             /* copy it to field buffer */
               GlobalUnlock(fields[i]->ColBuffer) ;                        /* unlock column buffer */
               }
            }
         doub = 0.0 ;                                                     /* reset doub */
         if (fields[i]->str == NULL)                                       /* if string is empty */
            {
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* copy buffer to string, lock buffer */
            unlock = 1 ;                                                  /* we locked buffer */
            }
         if (fields[i]->str != NULL)                                       /* if string isn't empty */
            {
            lstrcpy(buffer, fields[i]->str) ;                              /* copy string to buffer */
            doub = atof(buffer) ;                                         /* transform buffer to float into doub */
            if (unlock)                                                   /* if we locked buffer */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock it */
            }
         return(doub) ;                                                   /* return to calling function with fieldvalue (doub) */
         }
      }
   return(0.0) ;                                                          /* return to calling function with failure code */
   }


/*****************************************************************************

   FUNCTION:    FIELDSTR(fieldname)

   DESCRIPTION: return the string value of the given field
                fieldname:      name of field

*****************************************************************************/

LPSTR FIELDSTR(LPSTR fieldname)
   {
   int i, unlock = 0 ;
   LPHANDLE ph ;
   LPSTR ptr ;
   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(fieldname, fields[i]->Label))                          /* if fieldname corresponds with label */
#else
      if (!lstrcmpi((LPSTR)fieldname, (LPSTR)fields[i]->Label))                          /* if fieldname corresponds with label */
#endif
         {
         if (fields[i]->ColBuffer != NULL)                                 /* if it's a table */
            {
            if (fields[i]->str != NULL)                                    /* if string isn't empty */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock the buffer */
            fields[i]->str = NULL ;                                        /* clear string */
            ph = (LPHANDLE) GlobalLock(fields[i]->ColBuffer) ;             /* lock columnbuffer and get handle */
            if (ph != NULL)                                               /* if handle exists */
               {
               if (ph[0] != NULL)                                         /* if first element exists */
                   fields[i]->Buffer = ph[0] ;                             /* load it into buffer */
               GlobalUnlock(fields[i]->ColBuffer) ;                        /* unlock column buffer */
               }
            }
         if (fields[i]->str == NULL)                                       /* if not locked */
            {
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* lock buffer */
            unlock = 1 ;                                                  /* must unlock on exit because we found it unlocked */
            }
         if (fields[i]->str == NULL)                                       /* if unable to lock */
            return(NULL) ;                                                /* return NULL */
#if !defined(__WC386__)
         lstrcpy(ReturnStrings[NextAvailableString], fields[i]->str) ;     /* copy field string value to return string buffer */
         ptr = ReturnStrings[NextAvailableString] ;                       /* save return pointer */
#else
         lstrcpy((LPSTR)ReturnStrings[NextAvailableString], (LPSTR)fields[i]->str) ;     /* copy field string value to return string buffer */
         ptr = (LPSTR)ReturnStrings[NextAvailableString] ;                       /* save return pointer */
#endif
         NextAvailableString++ ;                                          /* increment for next available string buffer */
         NextAvailableString %= RETURNSTRINGCOUNT ;                       /* wrap around if at maximum */
         if (unlock)
            {
            GlobalUnlock(fields[i]->Buffer) ;                              /* unlock buffer */
            fields[i]->str = NULL ;                                        /* reset lock handle to null */
            }
         return(ptr) ;                                                    /* return to calling function with string */
         }
      }
   return(NULL) ;                                                         /* return to calling function with failure code */
   }

/**************************************************************************

Function:       FILTERACTIVATE("LinkName", "FilterName")
Description:    Activates a link filter
Prototype:      void FILTERACTIVATE(LPSTR LinkName,
                                               LPSTR FilterName) ;

***************************************************************************/

BOOL FILTERACTIVATE(LPSTR LinkName, LPSTR FilterName)
   {
   return(TRUE) ;
   }

/**************************************************************************

Function:       FILTERDEACTIVATE("LinkName")
Description:    Deactivates a link filter
Prototype:      void FILTERDEACTIVATE(LPSTR LinkName) ;

***************************************************************************/

void FILTERDEACTIVATE(LPSTR LinkName)
   {
   }

/**************************************************************************

Function:       FIND("SubString", "String", StartNumber)
Description:    Searches "String" for StartNumber occurence of "SubString"
Prototype:      double FIND(LPSTR SubString, LPSTR String,
                                       UINT StartNumber) ;

***************************************************************************/

double FIND(LPSTR SubString, LPSTR String, UINT StartNumber)
   {
   LPSTR   lp, lp2 ;
   int     i ;

   if (SubString == NULL || String == NULL)
      return(WD_ERR) ;

   if (StartNumber > lstrlen(String))
      return(WD_ERR) ;

   if (StartNumber > (lstrlen(String) - lstrlen(SubString)))
      return(WD_ERR) ;

   if (lstrlen(SubString) > lstrlen(String))
      return(WD_ERR) ;

   lp = String ;
   for (i = 0 ; i < StartNumber ; i++)
      lp = AnsiNext(lp) ;

#if !defined(__ZTC__) && !defined(__WC386__)
   lp2 = _fstrstr(String, SubString) ;
#else
#if defined(__ZTC__)
   lp2 = strstr((char *)String, (char *)SubString);
#endif
#if defined(__WC386__)
   lp2 = (LPSTR)_fstrstr(MK_FP32(String), MK_FP32(SubString)) ;
#endif
#endif

   if (lp2 == NULL)
      return(WD_ERR) ;

   return((double)(lp2 - String)) ;

   }


/**************************************************************************

Function:       FORMAT(Num, Format, Places)
Description:    Formats a numeric value and returns a string
Prototype:      LPSTR FORMAT(double Num, UINT Format, UINT Places) ;

***************************************************************************/

LPSTR FORMAT(double Num, UINT Format, UINT Places)
   {
   return(NULL) ;
   }


/**************************************************************************

Function:       FORMCLEAR("FormName")
Description:    Clears the specified form
Prototype:      void FORMCLEAR(LPSTR FormName) ;

***************************************************************************/

void FORMCLEAR(LPSTR FormName)
   {
   }

/*****************************************************************************

   FUNCTION:    FORMCLOSE (ptr)

   PURPOSE:     close form
                ptr:

*****************************************************************************/

void FORMCLOSE(LPSTR ptr)
   {
   int i ;
   RECT bigRect ;
   for (i = 0 ; i < FORM_COUNT ; i++)                                     /* for all forms */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(forms[i].Label, ptr))                                 /* if it's the wanted form */
#else
      if (!lstrcmpi((LPSTR)forms[i].Label, (LPSTR)ptr))                                 /* if it's the wanted form */
#endif
         {
         if (forms[i].hWnd != NULL)                                       /* if form is active */
            {
#if !defined(__WC386__)
            GetWindowRect(hwndMDIClient, (RECT far *) &bigRect) ;
            GetWindowRect(forms[i].hWnd, (RECT far *) &forms[i].PosRect) ;
#else
            GetWindowRect(hwndMDIClient, (LPRECT) &bigRect) ;
            GetWindowRect(forms[i].hWnd, (LPRECT) &forms[i].PosRect) ;
#endif
            forms[i].PosRect.left -= bigRect.left ;
            forms[i].PosRect.top -= bigRect.top ;
            forms[i].PosRect.right -= bigRect.left ;
            forms[i].PosRect.bottom -= bigRect.top ;
            SendMessage (hwndMDIClient, WM_MDIDESTROY, forms[i].hWnd, NULL) ;       /* destroy it */
            }
         forms[i].hWnd = NULL ;                                           /* set it to inactive */
         break ;
         }
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    FORMSELECT (ptr)

   PURPOSE:     select new form
                ptr:

*****************************************************************************/

BOOL FORMSELECT(LPSTR ptr)
   {
   int i, j, k, selnum, listcount, listvert, listhorz, listwide, listhigh ;
   int xoff, yoff, xbutoff, ybutoff, y, x ;
   HFONT   hFont ;
   HFONT   hOldFont ;
   HDC hDC ;
   DWORD dwSize1, dwStyle ;
   MDICREATESTRUCT mcs;
   char buffer[129] ;
   RECT bigRect ;

   for (i = 0 ; i < FORM_COUNT ; i++)                                     /* for all forms */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(forms[i].Label, ptr))                                 /* if you found wanted form */
#else
      if (!lstrcmpi((LPSTR)forms[i].Label, (LPSTR)ptr))                                 /* if you found wanted form */
#endif
         {
         if (forms[i].hWnd == NULL)                                       /* amd it's not in use */
            {
            currentformindex = i ;                                        /* remember formindex */
            mcs.szTitle = (LPSTR) forms[i].Label ;                        /* setup to call WM_MDICREATE */
            mcs.szClass = "WinExeForm" ;
            mcs.hOwner  = hInst;
            if (forms[i].PosRect.left < 0)
               {
#if !defined(__WC386__)
               GetClientRect(hwndMDIClient, (RECT far *) &bigRect) ;
#else
               GetClientRect(hwndMDIClient, (LPRECT) &bigRect) ;
#endif
               x = (bigRect.right - (forms[i].Rect.right - forms[i].Rect.left)) / 2 ;
               if (x < 0)
                  x = 0 ;
               y = (bigRect.bottom - (forms[i].Rect.bottom - forms[i].Rect.top)) / 2 ;
               if (y < 0)
                  y = 0 ;
               mcs.x       = x ;
               mcs.y       = y ;
               mcs.cx      = forms[i].Rect.right - forms[i].Rect.left ;
               mcs.cy      = forms[i].Rect.bottom - forms[i].Rect.top ;
               }
            else
               {
               mcs.x       = forms[i].PosRect.left ;
               mcs.y       = forms[i].PosRect.top ;
               mcs.cx      = forms[i].PosRect.right - forms[i].PosRect.left ;
               mcs.cy      = forms[i].PosRect.bottom - forms[i].PosRect.top ;
               }

#if 0
            mcs.x       = forms[i].Rect.left ;
            mcs.y       = forms[i].Rect.top ;
            mcs.cx      = forms[i].Rect.right - forms[i].Rect.left ;
            mcs.cy      = forms[i].Rect.bottom - forms[i].Rect.top ;
#endif
            mcs.style   = MDIS_ALLCHILDSTYLES | WS_VSCROLL | WS_HSCROLL ;

            mcs.lParam  = NULL ;

            forms[i].hWnd = (WORD) SendMessage (hwndMDIClient, WM_MDICREATE, 0,
                                        (LONG) (LPMDICREATESTRUCT) &mcs) ;    /* tell the MDI client to create a child form */
            if (forms[i].hWnd == NULL)                                    /* if form isn't active */
               {
               lstrcpy(buffer, "Unable to Create Form Window: ") ;        /* write error message */
#if !defined(__WC386__)
               lstrcat(buffer, forms[i].Label) ;                          /* add form name to it */
#else
               lstrcat((LPSTR)buffer, (LPSTR)forms[i].Label) ;                          /* add form name to it */
#endif
               MessageBox(NULL, (LPSTR)buffer, NULL, MB_SYSTEMMODAL | MB_ICONHAND | MB_OK) ;  /* display error message in messagebox */
               return(FALSE) ;                                                   /* return to calling function */
               }
            SetWindowLong(forms[i].hWnd, 0, (LONG) forms[i].id) ;         /* set window id */

            for (j = 0 ; j < BUTTON_COUNT ; j++)                          /* create the buttons for this form */
               {
               if (buttons[j].formid != forms[i].id)                      /* if the button isn't for this menu type */
                  continue ;                                              /* continue search */

               CheckButtonSize(forms[i].hWnd, j) ;                        /* check size of button */
               buttons[j].hWnd = CreateWindow("BUTTON", NULL,
                              BS_OWNERDRAW | WS_CHILD | WS_VISIBLE,
                              buttons[j].Rect.left - 1, buttons[j].Rect.top - 1,
                              2 + buttons[j].Rect.right - buttons[j].Rect.left,
                              2 + buttons[j].Rect.bottom - buttons[j].Rect.top,
                              forms[i].hWnd, buttons[j].id, hInst, NULL) ;          /* create button */
               }

            for (j = 0 ; j < FIELD_COUNT ; j++)                           /* create selection list field buttons */
               {
               if (fields[j]->formid != forms[i].id)                       /* if the field isn't for this menu type */
                  continue ;                                              /* continue search */
               if (fields[j]->FieldType == WINEXE_FLD_COMBO)               /* combo box */
                  {
                  dwStyle = CBS_DROPDOWN | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL ;
                  listhigh = 1 + fields[j]->Rect.bottom - fields[j]->Rect.top ;
                  if (listhigh < 2 * abs(fonts[fields[j]->ValueFont].lfHeight))
                     listhigh = 6 * abs(fonts[fields[j]->ValueFont].lfHeight) ;
#if !defined(__WC386__)
                  fields[j]->hWnd = CreateWindow("COMBOBOX", fields[j]->Label, dwStyle,
#else
                  fields[j]->hWnd = CreateWindow("COMBOBOX", (LPSTR)fields[j]->Label, dwStyle,
#endif
                              fields[j]->Rect.left, fields[j]->Rect.top,
                              1 + fields[j]->Rect.right - fields[j]->Rect.left,
                              listhigh, forms[i].hWnd, fields[j]->id, hInst, NULL) ;          /* create combobox */
                  listcount = 0 ;
                  for (k = 0 ; k < FIELD_LIST_COUNT ; k++)     /* count selections for this field */
                     {
                     if (fieldlists[k].fieldid != fields[j]->id)
                        continue ;
                     SendMessage(fields[j]->hWnd, CB_ADDSTRING, 0, (LPARAM) (LPCSTR) fieldlists[k].Label) ;
                     listcount++ ;
                     }
                  if (listcount == 0)
                     fields[j]->automatic = 1 ;
                  else
                     fields[j]->automatic = 0 ;
                  ShowWindow(fields[j]->hWnd, SW_HIDE) ;
                  continue ;
                  }
               if (fields[j]->FieldType == WINEXE_FLD_SELECT)              /* selection list box */
                  {
                  dwStyle = LBS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_VSCROLL ;
                  if (fields[j]->HideLabel)
                     dwStyle |= WS_BORDER ;
                  else
                     dwStyle |= WS_CAPTION ;
                  listhigh = 1 + fields[j]->Rect.bottom - fields[j]->Rect.top ;
                  if (listhigh < 2 * abs(fonts[fields[j]->ValueFont].lfHeight))
                     listhigh = 6 * abs(fonts[fields[j]->ValueFont].lfHeight) ;
#if !defined(__WC386__)
                  fields[j]->hWnd = CreateWindow("LISTBOX", fields[j]->Label, dwStyle,
#else
                  fields[j]->hWnd = CreateWindow("LISTBOX", (LPSTR)fields[j]->Label, dwStyle,
#endif
                              fields[j]->Rect.left, fields[j]->Rect.top,
                              1 + fields[j]->Rect.right - fields[j]->Rect.left,
                              listhigh, forms[i].hWnd, fields[j]->id, hInst, NULL) ;          /* create listbox */
                  listcount = 0 ;
                  for (k = 0 ; k < FIELD_LIST_COUNT ; k++)     /* count selections for this field */
                     {
                     if (fieldlists[k].fieldid != fields[j]->id)
                        continue ;
                     SendMessage(fields[j]->hWnd, LB_ADDSTRING, 0, (LPARAM) (LPCSTR) fieldlists[k].Label) ;
                     listcount++ ;
                     }
                  if (listcount == 0)
                     fields[j]->automatic = 1 ;
                  else
                     fields[j]->automatic = 0 ;
                  ShowWindow(fields[j]->hWnd, SW_HIDE) ;
                  continue ;
                  }
               if (fields[j]->FieldType == WINEXE_FLD_BOOL)              /* true/false check box */
                  {
                  fields[j]->hWnd = CreateWindow("BUTTON", NULL,
                              BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
                              fields[j]->Rect.left, fields[j]->Rect.top, 18, 18,
                              forms[i].hWnd, fields[j]->id, hInst, NULL) ;
                  continue ;
                  }
               if (fields[j]->FieldType != WINEXE_FLD_CHECK &&
                   fields[j]->FieldType != WINEXE_FLD_RADIO)
                  continue ;
               xoff = yoff = 0 ;
               hDC = GetDC(forms[i].hWnd) ;                               /* get device context for current handle */
               if (fields[j]->bTop == TRUE || fields[j]->bOutline == TRUE)
                  yoff = 1 ;
               if (fields[j]->bLeft == TRUE || fields[j]->bOutline == TRUE)
                  xoff = 1 ;

#if !defined(__WC386__)
               hFont = CreateFontIndirect(&fonts[fields[j]->LabelFont]) ;  /* get vertical size of field label */
#else
               hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[j]->LabelFont]) ;  /* get vertical size of field label */
#endif
               hOldFont = SelectObject(hDC, hFont) ;
#if !defined(__WC386__)
               dwSize1 = GetTextExtent(hDC, fields[j]->Label, lstrlen(fields[j]->Label)) ;
#else
               dwSize1 = GetTextExtent(hDC, (LPSTR)fields[j]->Label, lstrlen((LPSTR)fields[j]->Label)) ;
#endif
               if (abs(fonts[fields[j]->LabelFont].lfHeight) > HIWORD(dwSize1))
                  yoff += abs(fonts[fields[j]->LabelFont].lfHeight) ;
               else
                  yoff += HIWORD(dwSize1) ;
               SelectObject(hDC, hOldFont) ;  /* reselect the old font */
               DeleteObject(hFont) ;          /* Delete the font */

#if !defined(__WC386__)
               hFont = CreateFontIndirect(&fonts[fields[j]->ValueFont]) ;  /* get vertical size of selection text */
#else
               hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[j]->ValueFont]) ;  /* get vertical size of selection text */
#endif
               hOldFont = SelectObject(hDC, hFont) ;      /* set the new display font */
               listhigh = abs(fonts[fields[j]->ValueFont].lfHeight) ;
               listcount = listwide = 0 ;
               for (k = 0 ; k < FIELD_LIST_COUNT ; k++)     /* count selections for this field */
                  {
                  if (fieldlists[k].fieldid != fields[j]->id)
                     continue ;
                  listcount++ ;
                  dwSize1 = GetTextExtent(hDC, (LPSTR) fieldlists[k].Label, lstrlen((LPSTR)fieldlists[k].Label)) ;
                  if (LOWORD(dwSize1) > listwide)
                     listwide = (int) LOWORD(dwSize1) ;
                  }
               listwide += 12 + listhigh ;  /* allow room for checkbox/radio button */
               listvert = fields[j]->Rect.bottom - (fields[j]->Rect.top + yoff) ;
               listvert /= listhigh ;
               listhorz = fields[j]->Rect.right - (fields[j]->Rect.left + xoff) ;
               listhorz /= listwide ;
               if (listhorz < 1)
                  listhorz = 1 ;
               if (listvert < 1)
                  listvert = 1 ;

               selnum = 0 ;
               ybutoff = yoff ;
               xbutoff = xoff ;
               for (k = 0 ; k < FIELD_LIST_COUNT ; k++)
                  {
                  if (fieldlists[k].fieldid != fields[j]->id)
                     continue ;
                  if (fields[j]->FieldType == WINEXE_FLD_CHECK)                          /* check boxes */
                     {
                     fieldlists[k].hWnd = CreateWindow("BUTTON", NULL,
                              BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
                              fields[j]->Rect.left + xbutoff,
                              fields[j]->Rect.top + ybutoff, 18, 18,
                              forms[i].hWnd, fieldlists[k].id, hInst, NULL) ;          /* create button */
                     }
                  else                                                                  /* radio buttons */
                     {
                     if (selnum == 0)
                        fieldlists[k].hWnd = CreateWindow("BUTTON", NULL,
                              BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE | WS_GROUP,
                              fields[j]->Rect.left + xbutoff,
                              fields[j]->Rect.top + ybutoff, 18, 18,
                              forms[i].hWnd, fieldlists[k].id, hInst, NULL) ;          /* create button */
                     else
                        fieldlists[k].hWnd = CreateWindow("BUTTON", NULL,
                              BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE,
                              fields[j]->Rect.left + xbutoff,
                              fields[j]->Rect.top + ybutoff, 18, 18,
                              forms[i].hWnd, fieldlists[k].id, hInst, NULL) ;          /* create button */
                     }
                  fieldlists[k].Rect.top = fields[j]->Rect.top + ybutoff ;
                  fieldlists[k].Rect.left = fields[j]->Rect.left + xbutoff ;
                  fieldlists[k].Rect.bottom = listhigh ;
                  fieldlists[k].Rect.right = listwide ;
                  if (listvert >= listcount || !((selnum + 1) % listhorz))
                     {
                     xbutoff = xoff ;
                     ybutoff += listhigh ;
                     }
                  else
                     xbutoff += listwide ;
                  selnum++ ;
                  }

               SelectObject(hDC, hOldFont) ;  /* reselect the old font */
               DeleteObject(hFont) ;          /* Delete the font */
               ReleaseDC(forms[i].hWnd, hDC) ;
               }

            for (j = 0 ; j < TABLE_COUNT ; j++)                           /* create the table scroll bars for this form */
               {
               if (tables[j].formid != forms[i].id || !tables[j].ScrollBar)         /* if it's not the correct table */
                  continue ;                                              /* continue search */
               tables[j].ScrollhWnd = CreateWindow("scrollbar", NULL,
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_VERT,
                              tables[j].Rect.right,
                              tables[j].Rect.top + tables[j].TblLblSize + tables[j].ColLblSize,
                              12,
                              1 + tables[j].Rect.bottom - tables[j].Rect.top -
                                    tables[j].TblLblSize - tables[j].ColLblSize,
                              forms[i].hWnd, tables[j].id, hInst, NULL) ;           /* create scrollbar */
               SetScrollRange(tables[j].ScrollhWnd, SB_CTL, 1, 100, FALSE) ;        /* adjust range (don't paint it) */
               SetScrollPos(tables[j].ScrollhWnd, SB_CTL, 1, TRUE) ;      /* place scrollbox */
               }
            }
         if (forms[i].hWnd != NULL)                                       /* if form is active */
            {
            SendMessage (hwndMDIClient, WM_MDIACTIVATE, forms[i].hWnd, NULL) ;      /* activate a client for it */
            if (IsIconic(forms[i].hWnd))                                  /* if form is just an icon */
               ShowWindow(forms[i].hWnd, SW_RESTORE) ;                    /* reactivate the window */
            }
         break ;
         }
      }
   return(TRUE) ;                                                           /* return to calling function */
   }

/**************************************************************************

Function:       FV(Pmt, Rate, Nper)
Description:    Returns the future value of an investment
Prototype:      double FV(double Pmt, double Rate, double Nper) ;

***************************************************************************/

double FC(double Pmt, double Rate, double Nper)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       FVAL(Rate, Nper, Pmt, Pv, Type)
Description:    Updated version of FV
Prototype:      double FVAL(double Rate, double Nper, double Pmt,
                                       double Pv, double Type) ;

***************************************************************************/

double FVAL(double Rate, double Nper, double Pmt, double Pv,
                       double Type)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       HOUR(DateTimeNumber)
Description:    Returns the hour portion of the date time number (0-23)
Prototype:      int HOUR(double DateTimeNumber) ;

***************************************************************************/

int HOUR(double DateTimeNumber)
   {
   long dateserial ;
   double timeserial ;
   int hour ;

   dateserial = (long) DateTimeNumber ;
   timeserial = DateTimeNumber - (double) dateserial ;
   hour = timeserial * 24.0 ;

   return(hour) ;
   }

/**************************************************************************

Function:       INT(X)
Description:    Returns the integer portion of X, dropping decimal places
Prototype:      double INT(double X) ;

***************************************************************************/

double INT(double X)
   {
   int y ;
   y = X ;
   return((double) y) ;
   }


/**************************************************************************

Function:       IPAYMT(Rate, Per, Nper, Pv, Fv, Type)
Description:    Tells what portion of a loan payment is interest
Prototype:      double IPAYMT(double Rate, double Per, double Nper,
                                         double Pv, double Fv, double Type) ;

***************************************************************************/

double IPAYMT(double Rate, double Per, double Nper, double Pv,
                         double Fv, double Type)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       IRATE(Nper, Pmt, Pv, Fv, Type)
Description:    Updated version of RATE
Prototype:      double IRATE(double Nper, double Pmt, double Pv,
                                        double Fv, double Type) ;

***************************************************************************/

double IRATE(double Nper, double Pmt, double Pv, double Fv,
                        double Type)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       IRR(Guess, ColumnName)
Description:    Internal rate of return on an investment
Prototype:      double IRR(double Guess, LPSTR ColumnName) ;

***************************************************************************/

double IRR(double Guess, LPSTR ColumnName)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       ISBLANK(FieldName)
Description:    Checks if a field has no value
Prototype:      double ISBLANK(LPSTR FieldName) ;

***************************************************************************/

double ISBLANK(LPSTR fieldname)
   {
   double retval ;
   int i, unlock = 0 ;
   LPHANDLE ph ;
   LPSTR ptr ;
   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
#if !defined(__WC386__)
      if (!lstrcmpi(fieldname, fields[i]->Label))                          /* if fieldname corresponds with label */
#else
      if (!lstrcmpi((LPSTR)fieldname, (LPSTR)fields[i]->Label))                          /* if fieldname corresponds with label */
#endif
         {
         if (fields[i]->ColBuffer != NULL)                                 /* if it's a table */
            {
            if (fields[i]->str != NULL)                                    /* if string isn't empty */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock the buffer */
            fields[i]->str = NULL ;                                        /* clear string */
            ph = (LPHANDLE) GlobalLock(fields[i]->ColBuffer) ;             /* lock columnbuffer and get handle */
            if (ph != NULL)                                               /* if handle exists */
               {
               if (ph[0] != NULL)                                         /* if first element exists */
                   fields[i]->Buffer = ph[0] ;                             /* load it into buffer */
               GlobalUnlock(fields[i]->ColBuffer) ;                        /* unlock column buffer */
               }
            }
         if (fields[i]->str == NULL)                                       /* if not locked */
            {
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* lock buffer */
            unlock = 1 ;                                                  /* must unlock on exit because we found it unlocked */
            }
         if (fields[i]->str == NULL)                                       /* if unable to lock */
            return(NULL) ;                                                /* return NULL */
         ptr = fields[i]->str ;
         retval = 0 ;
         while (*ptr)
            {
            if (*ptr != ' ')
               {
               retval = 1 ;
               break ;
               }
            ptr++ ;
            }
         if (unlock)
            {
            GlobalUnlock(fields[i]->Buffer) ;                              /* unlock buffer */
            fields[i]->str = NULL ;                                        /* reset lock handle to null */
            }
         return(retval) ;                                                    /* return to calling function with string */
         }
      }
   return(NULL) ;                                                         /* return to calling function with failure code */
   }

/**************************************************************************

Function:       ISCOMPLETE("FormName")
Description:    Checks to see if a form is completed
Prototype:      double ISCOMPLETE(LPSTR FormName) ;

***************************************************************************/

double ISCOMPLETE(LPSTR FormName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LEFT("String", Num)
Description:    Returns the leftmost Num characters of String
Prototype:      LPSTR LEFT(LPSTR String, double Num) ;

***************************************************************************/

LPSTR LEFT(LPSTR String, double Num)
   {
   LPSTR ptr ;
#if !defined(__WC386__)
   lstrcpy(ReturnStrings[NextAvailableString], String) ;
   ptr = ReturnStrings[NextAvailableString] ;                       /* save return pointer */
#else
   lstrcpy((LPSTR)ReturnStrings[NextAvailableString], String) ;
   ptr = (LPSTR)ReturnStrings[NextAvailableString] ;                       /* save return pointer */
#endif
   NextAvailableString++ ;                                          /* increment for next available string buffer */
   NextAvailableString %= RETURNSTRINGCOUNT ;                       /* wrap around if at maximum */
   ptr[(int) Num] = 0 ;
   return(ptr) ;
   }


/**************************************************************************

Function:       LENGTH("String")
Description:    Calculates the length of String
Prototype:      double LENGTH(LPSTR String) ;

***************************************************************************/

double LENGTH(LPSTR String)
   {
   return(lstrlen(String)) ;
   }


/**************************************************************************

Function:       LINKAVG("LinkName", "DataField")
Description:    Calculates the average of "DataField" in "LinkName"
Prototype:      double LINKAVG(LPSTR LinkName, LPSTR DataField) ;

***************************************************************************/

double LINKAVG(LPSTR LinkName, LPSTR DataField)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       LINKCOUNT("LinkName")
Description:    Counts # records in LinkName, including Filters and
                restricted ranges.

Prototype:      double LINKCOUNT(LPSTR LinkName) ;

***************************************************************************/

double LINKCOUNT(LPSTR LinkName)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LINKMAX("LinkName", "DataField")
Description:    Returns the max value of values in entire database "DataField"
Prototype:      double LINKMAX(LPSTR LinkName, LPSTR DataField) ;

***************************************************************************/

double LINKMAX(LPSTR LinkName, LPSTR DataField)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LINKMIN("LinkName", "DataField")
Description:    Finds minimum value of "DataField" in "LinkName"
Prototype:      double LINKMIN(LPSTR LinkName, LPSTR DataField) ;

***************************************************************************/

double LINKMIN(LPSTR LinkName, LPSTR DataField)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LINKSUM("LinkName", "DataField")
Description:    Finds the sum of all records for "DataField" in "LinkName"
Prototype:      double LINKSUM(LPSTR LinkName, LPSTR DataField) ;

***************************************************************************/

double LINKSUM(LPSTR LinkName, LPSTR DataField)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LINKVALUE("LinkName", "DataField")
Description:    Returns the value of "DataField" in current record (NUMERIC)
Prototype:      double LINKVALUE(LPSTR LinkName, LPSTR DataField) ;

***************************************************************************/

double LINKVALUE(LPSTR LinkName, LPSTR DataField)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LN(X)
Description:    Natural logarithm of X (X > 0)
Prototype:      double LN(double X) ;

***************************************************************************/

double LN(double X)
   {
   if (X <= 0)
      return(WD_ERR) ;

   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       LOG(X)
Description:    Base 10 logarithm of X (X > 0)
Prototype:      double LOG(double X) ;

***************************************************************************/

#if defined(__WC386__)
double wd_LOG(double X)
#else
double LOG(double X)
#endif
   {
   if (X <= 0)
      return(WD_ERR) ;

   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       LOWER("String")
Description:    Returns the lowercase version of "String"
Prototype:      LPSTR LOWER(LPSTR String) ;

***************************************************************************/

LPSTR LOWER(LPSTR String)
   {
   int len = 0 ;
   while (String[len])
      {
      if (String[len] >= 'A' && String[len] <= 'Z')
         String[len] = 'a' + String[len] - 'A' ;
      len++ ;
      }
   return(String) ;
   }


/**************************************************************************

Function:       MAX(List)
Description:    Returns maximum value in List
Prototype:      double MAX(double d1, ...) ;
Notes:          ObjectVision allows @ functions to have a variable number
                of parameters.  In order to allow WinGEN to be able to
                easily generate code to work with C/C++, Pascal, and dBASE
                for Windows we have chosen to take the lowest common
                denominator method for programming this function.  To this
                end there are several MAX functions present each with a
                different number of parameters.  WinGEN automatically handles
                the conversion of the normal MAX function to these newer
                formats for all of your .OVD files.

***************************************************************************/

double MAX(double L1)
   {
   return(WD_TRUE) ;
   }

double MAX2(double L1, double L2)
   {
   return(WD_TRUE) ;
   }

double MAX3(double L1, double L2, double L3)
   {
   return(WD_TRUE) ;
   }

double MAX4(double L1, double L2, double L3, double L4)
   {
   return(WD_TRUE) ;
   }

double MAX5(double L1, double L2, double L3, double L4, double L5)
   {
   return(WD_TRUE) ;
   }

double MAX6(double L1, double L2, double L3, double L4, double L5, double L6)
   {
   return(WD_TRUE) ;
   }

double MAX7(double L1, double L2, double L3, double L4, double L5, double L6, double L7)
   {
   return(WD_TRUE) ;
   }

double MAX8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8)
   {
   return(WD_TRUE) ;
   }

double MAX9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9)
   {
   return(WD_TRUE) ;
   }

double MAX10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10)
   {
   return(WD_TRUE) ;
   }

double MAX11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11)
   {
   return(WD_TRUE) ;
   }

double MAX12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12)
   {
   return(WD_TRUE) ;
   }

double MAX13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13)
   {
   return(WD_TRUE) ;
   }

double MAX14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       MESSAGE("String")
Description:    Displays String as information for the user
Prototype:      void MESSAGE(LPSTR String) ;

***************************************************************************/

void MESSAGE(LPSTR String)
   {
   MessageBox(NULL, String, "Message", MB_OK) ;
   }

/**************************************************************************

Function:       MID("String", StartNumber, Num)
Description:    Returns the Num characters of a string starting at StartNumber
Prototype:      LPSTR MID(LPSTR String, UINT StartNumber, UINT Num) ;

***************************************************************************/

LPSTR MID(LPSTR String, UINT StartNumber, UINT Num)
   {
   return(NULL) ;
   }

/**************************************************************************

Function:       MIN(List)
Description:    Returns minimum value in List
Prototype:      double MIN(double d1, ...) ;
Notes:          ObjectVision allows @ functions to have a variable number
                of parameters.  In order to allow WinGEN to be able to
                easily generate code to work with C/C++, Pascal, and dBASE
                for Windows we have chosen to take the lowest common
                denominator method for programming this function.  To this
                end there are several MIN functions present each with a
                different number of parameters.  WinGEN automatically handles
                the conversion of the normal MIN function to these newer
                formats for all of your .OVD files.

***************************************************************************/

double MIN(double L1)
   {
   return(WD_TRUE) ;
   }

double MIN2(double L1, double L2)
   {
   return(WD_TRUE) ;
   }

double MIN3(double L1, double L2, double L3)
   {
   return(WD_TRUE) ;
   }

double MIN4(double L1, double L2, double L3, double L4)
   {
   return(WD_TRUE) ;
   }

double MIN5(double L1, double L2, double L3, double L4, double L5)
   {
   return(WD_TRUE) ;
   }

double MIN6(double L1, double L2, double L3, double L4, double L5, double L6)
   {
   return(WD_TRUE) ;
   }

double MIN7(double L1, double L2, double L3, double L4, double L5, double L6, double L7)
   {
   return(WD_TRUE) ;
   }

double MIN8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8)
   {
   return(WD_TRUE) ;
   }

double MIN9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9)
   {
   return(WD_TRUE) ;
   }

double MIN10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10)
   {
   return(WD_TRUE) ;
   }

double MIN11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11)
   {
   return(WD_TRUE) ;
   }

double MIN12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12)
   {
   return(WD_TRUE) ;
   }

double MIN13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13)
   {
   return(WD_TRUE) ;
   }

double MIN14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       MINUTE(DateTimeNumber)
Description:    Returns the minute portion of DateTimeNumber
Prototype:      int MINUTE(double DateTimeNumber) ;

***************************************************************************/

int MINUTE(double DateTimeNumber)
   {
   long dateserial ;
   double timeserial ;
   long minutes ;

   dateserial = (long) DateTimeNumber ;
   timeserial = DateTimeNumber - (double) dateserial ;
   minutes = (long) (timeserial * 24.0 * 60.0) ;

   return((int) minutes % 60) ;
   }

/**************************************************************************

Function:       MOD(X,Y)
Description:    Returns the remainder of X/Y
Prototype:      double MOD(double X, double Y) ;

***************************************************************************/

double MOD(double X, double Y)
   {
   if (Y == 0)
      return(WD_ERR) ;
   else
      return(fmod(X, Y)) ;
   }


/**************************************************************************

Function:       MONTH(DateTimeNumber)
Description:    Returns the month portion of DateTimeNumber
Prototype:      int MONTH(double DateTimeNumber) ;

***************************************************************************/

int MONTH(double DateTimeNumber)
   {
   int i = 1800 ;
   long days, oldserial, serial = -36522L ;

   oldserial = serial ;
   while (serial <= DateTimeNumber)
      {
      oldserial = serial ;
      serial += 365 ;
      if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
         serial += 1L ;
      i++ ;
      }
   i-- ;
   days = serial - oldserial ;
   if (days <= 31L)
      return(1) ;
   days -= 31L ;
   if (days <= 28L || (days <= 29L && (!(i % 4) && ((i % 100) || !(i % 400)))))
      return(2) ;
   days -= 28L ;
   if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
      days-- ;
   if (days <= 31L)
      return(3) ;
   days -= 31L ;
   if (days <= 30L)
      return(4) ;
   days -= 30L ;
   if (days <= 31L)
      return(5) ;
   days -= 31L ;
   if (days <= 30L)
      return(6) ;
   days -= 30L ;
   if (days <= 31L)
      return(7) ;
   days -= 31L ;
   if (days <= 31L)
      return(8) ;
   days -= 31L ;
   if (days <= 30L)
      return(9) ;
   days -= 30L ;
   if (days <= 31L)
      return(10) ;
   days -= 31L ;
   if (days <= 30L)
      return(11) ;
   days -= 30L ;
   return(12) ;
   }


/**************************************************************************

Function:       NA
Description:    Returns the special value NA
Prototype:      double NA(void) ;

***************************************************************************/

double NA(void)
   {
   return(WD_TRUE) ;
   }
/*****************************************************************************

   FUNCTION:    NOOP

   DESCRIPTION: do nothing, no operation, dormant, no effects, no affects, no defects period

*****************************************************************************/

void NOOP(void)
   {
   return ;                                                               /* return to calling function (do nothing) */
   }

/**************************************************************************

Function:       NOT(Logical)
Description:    Returns the NOT of Logical
Prototype:      double NOT(double Logical) ;

***************************************************************************/

double NOT(double Logical)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       NOW
Description:    Returns the Date/Time Serial # for the current time
Prototype:      double NOW(void) ;

***************************************************************************/

double NOW(void)
   {
   time_t curtime ;
   struct tm *ptm ;
   int year, mon, day, hour, min, sec ;
   double serial ;

   curtime = time(NULL) ;
   ptm = localtime(&curtime) ;

   sec = ptm->tm_sec ;
   min = ptm->tm_min ;
   hour = ptm->tm_hour ;
   day = ptm->tm_mday ;
   mon = ptm->tm_mon + 1 ;
   year = ptm->tm_year + 1900 ;

   serial = DATESERIAL(year, mon, day) + TIME(hour, min, sec) ;

   return(serial) ;
   }

/**************************************************************************

Function:       NPER(Rate, Pmt, Pv, Fv, Type)
Description:    Updated version of CTERM and TERM
Prototype:      double NPER(double Rate, double Pmt, double Pv,
                                       double Fv, double Type) ;

***************************************************************************/

double NPER(double Rate, double Pmt, double Pv, double Fv,
                       double Type)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       NPV(Rate, ColumnName, Type)
Description:    Calculates current value of a set of estimated cash flow.
Prototype:      double NPV(double Rate, LPSTR ColumnName,
                                      double Type) ;

***************************************************************************/

double NPV(double Rate, LPSTR ColumnName, double Type)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       PAYMT(Rate, Nper, Pv, Fv, Type)
Description:    Figures Payment, Interest, and Principal
Prototype:      double PAYMT(double Rate, double Nper, double Pv,
                                        double Fv, double Type) ;

***************************************************************************/

double PAYMT(double Rate, double Nper, double Pv, double Fv,
                        double Type)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       PI
Description:    Returns the value of PI
Prototype:      double PI(void) ;

***************************************************************************/

#if !defined(__ZTC__)
double PI(void)
#else
double wd_PI(void)
#endif
   {
   return(3.141592653589793238L) ;
   }


/**************************************************************************

Function:       PMT(Pv, Rate, Nper)
Description:    Calculates fully amortized payments
Prototype:      double PMT(double Pv, double Rate, double Nper) ;

***************************************************************************/

double PMT(double Pv, double Rate, double Nper)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       PPAYMT(Rate, Per, Nper, Pv, Fv, Type)
Description:    Principal part of payment
Prototype:      double PPAYMT(double Rate, double Per, double Nper,
                                         double Pv, double Fv, double Type) ;

***************************************************************************/

double PPAYMT(double Rate, double Per, double Nper, double Pv,
                         double Fv, double Type)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       PRINTALL
Description:    Prints all forms with the current information
Prototype:      void PRINTALL(void) ;

***************************************************************************/

void PRINTALL(void)
   {
   }

/**************************************************************************

Function:       PRINTFORM("FormName")
Description:    Prints out the form FormName
Prototype:      void PRINTFORM(LPSTR FormName) ;

***************************************************************************/

void PRINTFORM(LPSTR FormName)
   {
   }

/**************************************************************************

Function:       PRINTLINK("LinkName")
Description:    Prints out the contents of a link (with Filters and
                Restricted Ranges)
Prototype:      void PRINTLINK(LPSTR LinkName) ;

***************************************************************************/

void PRINTLINK(LPSTR LinkName)
   {
   }

/**************************************************************************

Function:       PROPER("String")
Description:    Converts a string to proper case
Prototype:      LPSTR PROPER(LPSTR String) ;

***************************************************************************/

LPSTR PROPER(LPSTR String)
   {
   return(NULL) ;
   }

/**************************************************************************

Function:       PV(Pmt, Rate, Nper)
Description:    Present Value of an investment
Prototype:      double PV(double Pmt, double Rate, double Nper) ;

***************************************************************************/

double PV(double Pmt, double Rate, double Nper)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       PVAL(Rate, Nper, Pmt, Fv, Type)
Description:    Updated version of PV
Prototype:      double PVAL(double Rate, double Nper, double Pmt,
                                       double Fv, double Type) ;

***************************************************************************/

double PVAL(double Rate, double Nper, double Pmt, double Fv,
                       double Type)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       PXOPEN("LinkName", "TableName", "DataFields", "OVReadFields",
                       "OVWriteFields", "SecIndexField", Option)
Description:    Open a Paradox table
Prototype:      void PXOPEN(LPSTR LinkName, LPSTR TableName,
                                       LPSTR DataFields, LPSTR OVReadFields,
                                       LPSTR OVWriteFields,
                                       LPSTR SecIndexField, UINT Option) ;

***************************************************************************/

void PXOPEN(LPSTR LinkName, LPSTR TableName, LPSTR DataFields,
                                       LPSTR OVReadFields,
                                       LPSTR OVWriteFields,
                                       LPSTR SecIndexField, UINT Option)
   {
   }


/**************************************************************************

Function:       RADIANS(X)
Description:    Converts the angle X (in degrees) to radians
Prototype:      double RADIANS(double X) ;

***************************************************************************/

double RADIANS(double X)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       RATE(Fv, Pv, Nper)
Description:    Calculates the interest rate for investment of Pv to be worth
                Fv within Nper compounding periods.
Prototype:      double RATE(double Fv, double Pv, double Nper) ;

***************************************************************************/

double RATE(double Fv, double Pv, double Nper)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       REGISTER("OVAlias", "ArgTypes", "ArgHelp", "LibName",
                         "FuncName", Type)
Description:    Registers a function from a DLL as callable from your app

Prototype:      void REGISTER(LPSTR OVAlias, LPSTR ArgTypes,
                                         LPSTR ArgHelp, LPSTR LibName,
                                         LPSTR FuncName, UINT Type) ;

***************************************************************************/

void REGISTER(LPSTR OVAlias, LPSTR ArgTypes, LPSTR ArgHelp,
                         LPSTR LibName, LPSTR FuncName, UINT Type)
   {
   }

/**************************************************************************

Function:       REPEAT("String", Num)
Description:    Returns Num copies of String
Prototype:      LPSTR REPEAT(LPSTR String, UINT Num) ;

***************************************************************************/

LPSTR REPEAT(LPSTR String, UINT Num)
   {
   return(NULL) ;
   }


/**************************************************************************

Function:       REPLACE("String", StartNum, Num, "NewString")
Description:    Replace characters in String with NewString
Prototype:      LPSTR REPLACE(LPSTR String, UINT StartNum, UINT Num,
                                         LPSTR NewString) ;

***************************************************************************/

LPSTR REPLACE(LPSTR String, UINT StartNum, UINT Num, LPSTR NewString)
   {
   return(NULL) ;
   }


/**************************************************************************

Function:       RESTOREMENU
Description:    Restores the ObjectVision Standard menus
Prototype:      void RESTOREMENU(void) ;
Notes:          Since your application is no longer running in ObjectVision,
                this function now restores the menu to the default WinGEN
                application menu (File | Exit).

***************************************************************************/

void RESTOREMENU(void)
   {
   }

/**************************************************************************

Function:       RIGHT("String", Num)
Description:    Returns the rightmost Num characters of String
Prototype:      LPSTR RIGHT(LPSTR String, UINT Num) ;

***************************************************************************/

LPSTR RIGHT(LPSTR String, UINT Num)
   {
   int len, i ;
   LPSTR ptr ;
   len = lstrlen(String) ;
   if (Num >= len)
      i = 0 ;
   else
      i = len - Num ;
#if !defined(__WC386__)
   lstrcpy(ReturnStrings[NextAvailableString], &String[i]) ;
   ptr = ReturnStrings[NextAvailableString] ;                       /* save return pointer */
#else
   lstrcpy((LPSTR)ReturnStrings[NextAvailableString], &String[i]) ;
   ptr = (LPSTR)ReturnStrings[NextAvailableString] ;                       /* save return pointer */
#endif
   NextAvailableString++ ;                                          /* increment for next available string buffer */
   NextAvailableString %= RETURNSTRINGCOUNT ;                       /* wrap around if at maximum */
   return(ptr) ;
   }


/**************************************************************************

Function:       ROUND(X, Num)
Description:    Round X to Num decimal places
Prototype:      double ROUND(double X, int Num) ;

***************************************************************************/

double ROUND(double X, int Num)
   {

   double      dblInt ;
   char        buffer[513] ;
   int         i = 0, j ;
   char        *endptr ;

   if ((Num > 15) || (Num < -15))
      return(WD_ERR) ;

   if (Num == 0)
      {
      modf(X, &dblInt) ;
      return(dblInt) ;
      }

   if (Num > 0)           /* round to the right of the decimal point, Num places */
      {
      dblInt = X ;
#if defined(__C700__)
      dblInt += (5 * pow(10, -(Num+1))) ;
#if !defined(__ZTC__)
      _gcvt(dblInt, 255, buffer) ;
#endif
#else
      dblInt += (5 * pow10( -(Num+1) )) ;
#if !defined(__ZTC__)
      gcvt(dblInt, 255, buffer) ;
#endif
#endif

      j = lstrlen((LPSTR)buffer)+1 ;
      for (i = 0 ; i < lstrlen((LPSTR)buffer) ; i++)
         {
         if (buffer[i] == '.')
            j = i ;
         }

      if (j == lstrlen((LPSTR)buffer)+1)
         return(dblInt) ;

      buffer[j+Num+1] = '\0' ;

      dblInt = strtod(buffer, &endptr) ;
      return(dblInt) ;
      }
   else
      {
      Num = -Num ;
      modf(X, &dblInt) ;
#if defined(__C700__)
      dblInt += (5 * pow(10, Num-1)) ;
#if !defined(__ZTC__)
      _gcvt(dblInt, 255, buffer) ;
#endif
#else
      dblInt += (5 * pow10( Num-1 )) ;
#if !defined(__ZTC__)
      gcvt(dblInt, 255, buffer) ;
#endif
#endif

      j = lstrlen((LPSTR)buffer)-1 ;
      for (i = j ; i > (j - Num) ; i--)
         buffer[i] = (char)48 ;

      dblInt = strtod(buffer, &endptr) ;
      return(dblInt) ;
      }
   }


/**************************************************************************

Function:       SAVE
Description:    This function saves all information into an .ovd file
Prototype:      void SAVE(void) ;
Notes:          Since you are now running from an .exe file, you are no
                longer able to save out a current .ovd file.

***************************************************************************/

void SAVE(void)
   {
   }

/**************************************************************************

Function:       SAVEAS("FileName")
Description:    This function saves all information into an .ovd file
Prototype:      void SAVEAS(LPSTR FileName) ;
Notes:          Since you are now running from an .exe file, you are no
                longer able to save out a current .ovd file.

***************************************************************************/

void SAVEAS(LPSTR FileName)
   {
   }

/**************************************************************************

Function:       SECOND(DateTimeNumber)
Description:    Returns the seconds portion of DateTimeNumber
Prototype:      int SECOND(double DateTimeNumber) ;

***************************************************************************/

int SECOND(double DateTimeNumber)
   {
   long dateserial ;
   double timeserial ;
   long seconds ;

   dateserial = (long) DateTimeNumber ;
   timeserial = DateTimeNumber - (double) dateserial ;
   seconds = (long) (timeserial * 24.0 * 60.0 * 60.0) ;

   return((int) seconds % 60) ;
   }

/**************************************************************************

Function:       SELECTEDFIELD
Description:    Returns the name of the currently selected field
Prototype:      LPSTR SELECTEDFIELD(void) ;

***************************************************************************/

LPSTR SELECTEDFIELD(void)
   {
   if (forms[currentformindex].curfld >= 0)
      return(fields[forms[currentformindex].curfld]->Label) ;
   return(NULL) ;
   }

/****************************************************************************

   FUNCTION:    SELECTEDFORM

   DESCRIPTION: return the name of the currently selected form

*****************************************************************************/
LPSTR SELECTEDFORM(void)
   {
#if !defined(__WC386__)
   return(forms[currentformindex].Label) ;                                /* return to calling function with formname */
#else
   return((LPSTR)forms[currentformindex].Label) ;                                /* return to calling function with formname */
#endif
   }



/**************************************************************************

Function:       SETTITLE("Title")
Description:    Replaces the ObjectVision title
Prototype:      void SETTITLE(LPSTR Title) ;

***************************************************************************/

void SETTITLE(LPSTR Title)
   {
   }


/**************************************************************************

Function:       SIN(X)
Description:    Returns the sine of X (X is in radians)
Prototype:      double SIN(double X) ;

***************************************************************************/

#if defined(__WC386__)
double wd_SIN(double X)
#else
double SIN(double X)
#endif
   {
   return( sin( X ) ) ;
   }


/**************************************************************************

Function:       SLN(Cost, Salvage, Life)
Description:    Straight Line depreciation
Prototype:      double SLN(double Cost, double Salvage, double Life) ;

***************************************************************************/

double SLN(double Cost, double Salvage, double Life)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       SQRT(X)
Description:    Returns the square root of X (X >= 0)
Prototype:      double SQRT(double X) ;

***************************************************************************/

#if defined(__WC386__)
double wd_SQRT(double X)
#else
double SQRT(double X)
#endif
   {
   return( pow( X , 0.5) ) ;
   }

LPSTR STRCAT(LPSTR dest, LPSTR src)
   {
   LPSTR ptr ;

   if (dest == NULL || src == NULL)
      return (NULL) ;

#if !defined(__WC386__)
   ptr = ReturnStrings[NextAvailableString] ;                             /* save return pointer */
#else
   ptr = (LPSTR)ReturnStrings[NextAvailableString] ;                             /* save return pointer */
#endif
   lstrcpy(ptr, dest) ;                                                   /* copy the first half */
   lstrcat(ptr, src) ;                                                    /* concatenate the last half */
   NextAvailableString++ ;                                                /* increment for next available string buffer */
   NextAvailableString %= RETURNSTRINGCOUNT ;                             /* wrap around if at maximum */
   return(ptr) ;
   }

/**************************************************************************

Function:       SUM(List)
Description:    Returns the SUM of List
Prototype:      double SUM(double L1, ...) ;
Notes:          ObjectVision allows @ functions to have a variable number
                of parameters.  In order to allow WinGEN to be able to
                easily generate code to work with C/C++, Pascal, and dBASE
                for Windows we have chosen to take the lowest common
                denominator method for programming this function.  To this
                end there are several SUM functions present each with a
                different number of parameters.  WinGEN automatically handles
                the conversion of the normal SUM function to these newer
                formats for all of your .OVD files.

***************************************************************************/

double SUM(double L1)
   {
   return(L1) ;
   }

double SUM2(double L1, double L2)
   {
   return(L1 + L1) ;
   }

double SUM3(double L1, double L2, double L3)
   {
   return(L1 + L2 + L3) ;
   }

double SUM4(double L1, double L2, double L3, double L4)
   {
   return(L1 + L2 + L3 + L4) ;
   }

double SUM5(double L1, double L2, double L3, double L4, double L5)
   {
   return(L1 + L2 + L3 + L4 + L5) ;
   }

double SUM6(double L1, double L2, double L3, double L4, double L5, double L6)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6) ;
   }

double SUM7(double L1, double L2, double L3, double L4, double L5, double L6, double L7)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7) ;
   }

double SUM8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8) ;
   }

double SUM9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9) ;
   }

double SUM10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10) ;
   }

double SUM11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11) ;
   }

double SUM12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11 + L12) ;
   }

double SUM13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11 + L12 + L13) ;
   }

double SUM14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14)
   {
   return(L1 + L2 + L3 + L4 + L5 + L6 + L7 + L8 + L9 + L10 + L11 + L12 + L13 + L14) ;
   }


/**************************************************************************

Function:       SYD(Cost, Salvage, Life, Period)
Description:    Depreication amounts
Prototype:      double SYD(double Cost, double Salvage, double Life,
                                      double Period) ;

***************************************************************************/

double SYD(double Cost, double Salvage, double Life, double Period)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       TAN(X)
Description:    Tangent of X (X is in radians)
Prototype:      double TAN(double X) ;

***************************************************************************/

double TAN(double X)
   {
   return(tan(X)) ;
   }


/**************************************************************************

Function:       TERM(Pmt, Rate, Fv)
Description:    Number of periods required to accumulate an investment
Prototype:      double TERM(double Pmt, double Rate, double Fv) ;

***************************************************************************/

double TERM(double Pmt, double Rate, double Fv)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       TIME(Hr, Min, Sec)
Description:    Returns the date/time serial # for the passed Time
Prototype:      double TIME(UINT Hr, UINT Min, UINT Sec) ;

***************************************************************************/

double TIME(int Hr, int Min, int Sec)
   {
   double timeserial ;

   timeserial = Hr / 24.0 ;
   timeserial += (Min / 60.0) / 24.0 ;
   timeserial += ((Sec / 60.0) / 60.0) / 24.0 ;

   return(timeserial) ;
   }

/**************************************************************************

Function:       TIMEVALUE("TimeString")
Description:    Converts TimeString to a valid date/time serial #
Prototype:      double TIMEVALUE(LPSTR TimeString) ;

***************************************************************************/

double TIMEVALUE(LPSTR TimeString)
   {
   double serial ;
   long lserial ;
   int i, hour, min, sec, pm = 0 ;
   if (TimeString == NULL)     /* return current time */
      {
      serial = NOW() ;
      lserial = (long) serial ;
      return(serial - lserial) ;
      }

   i = 0 ;
   while (TimeString[i])
      {
      if (TimeString[i] == 'P' || TimeString[i] == 'p')
         pm = 1 ;
      i++ ;
      }

   i = 0 ;
   hour = atoi(TimeString) ;
   if (pm)
      hour += 12 ;
   while (TimeString[i] && TimeString[i] != ':')       /* skip over hour */
      i++ ;
   while (TimeString[i] == ':')                        /* skip over separator */
      i++ ;
   min = atoi(&TimeString[i]) ;
   while (TimeString[i] && TimeString[i] != ':')       /* skip over minutes */
      i++ ;
   while (TimeString[i] == ':')                        /* skip over separator */
      i++ ;
   if (TimeString[i])
      sec = atoi(&TimeString[i]) ;
   else
      sec = 0 ;

   return(TIME(hour, min, sec));
   }


/**************************************************************************

Function:       TODAY
Description:    Returns the date/time serial # for today
Prototype:      long TODAY(void) ;

***************************************************************************/

long TODAY(void)
   {
   return((long) NOW()) ;
   }


/**************************************************************************

Function:       TRIM("String")
Description:    Removes extra spaces from String
Prototype:      LPSTR TRIM(LPSTR String) ;

***************************************************************************/

LPSTR TRIM(LPSTR String)
   {
   BOOL    bSpace = FALSE ;
   char    buffer[256] ;
   LPSTR   lpDest ;

   lpDest = buffer ;
   bSpace = bSpace ;


   while (*String == ' ')
      String++ ;

   while (*String != '\0')
      {

      if ((*String == ' ') && (bSpace == FALSE))
         {
         *lpDest = *String ;
         lpDest++ ;
         String++ ;
         bSpace = TRUE ;
         continue ;
         }

      if ((*String == ' ') && (bSpace == TRUE))
         {
         String++ ;
         continue ;
         }

      if (*String != ' ')
         {
         bSpace = FALSE ;
         *lpDest = *String ;
         lpDest++ ;
         String++ ;
         }
      }

   return(lpDest) ;
   }


/**************************************************************************

Function:       TYPE(Value)
Description:    Returns the type of the value
Prototype:      double TYPENUM(double Value) ;
                double TYPESTR(LPSTR Value) ;
                double TYPELOG(double Value) ;
                double TYPEERR(double Value) ;

***************************************************************************/

double TYPENUM(double Value)
   {
   return(WD_TRUE) ;
   }
double TYPESTR(LPSTR Value)
   {
   return(WD_TRUE) ;
   }
double TYPELOG(double Value)
   {
   return(WD_TRUE) ;
   }
double TYPEERR(double Value)
   {
   return(WD_TRUE) ;
   }

/**************************************************************************

Function:       UNCHECKMENUITEM("MenuName", "MenuItemName")
Description:    Unchecks a menu item
Prototype:      void UNCHECKMENUITEM(LPSTR MenuName, LPSTR MenuItemName) ;

***************************************************************************/

void UNCHECKMENUITEM(LPSTR MenuName, LPSTR MenuItemName)
   {
   }


/**************************************************************************

Function:       UPPER("String")
Description:    Converts String to uppercase
Prototype:      LPSTR UPPER(LPSTR String) ;

***************************************************************************/

LPSTR UPPER(LPSTR String)
   {
   int len = 0 ;
   while (String[len])
      {
      if (String[len] >= 'a' && String[len] <= 'z')
         String[len] = 'A' + String[len] - 'a' ;
      len++ ;
      }
   return(String) ;
   }


/**************************************************************************

Function:       VERSION
Description:    Returns the version number of ObjectVision
Prototype:      double VERSION(void) ;
Notes:          Since you are not running under ObjectVision, this will
                return the version of WinGEN.

***************************************************************************/

double VERSION(void)
   {
   return(WD_TRUE) ;
   }


/**************************************************************************

Function:       WEEKDAY(DateTimeNumber)
Description:    Returns the day of the week of DateTimeNumber
Prototype:      int WEEKDAY(double DateTimeNumber) ;

***************************************************************************/

int WEEKDAY(double DateTimeNumber)
   {
   long serial ;
   int day ;
   serial = (long) DateTimeNumber ;
   if (serial < 0)
      {
      serial = - serial ;
      day = (int) (serial % 7L) ;
      return (7 - day) ;
      }
   else
      {
      day = (int) (serial % 7L) ;
      if (day == 0)
         day = 7 ;
      return(day) ;
      }
   }

/**************************************************************************

Function:       WHILEFIELD(Field, Event)
Description:    Returns the day of the week of DateTimeNumber
Prototype:      LPSTR WEEKDAY(double DateTimeNumber) ;

***************************************************************************/

LPSTR WHILEFIELD(LPSTR Fieldname, LPSTR Eventname)
   {
   return(NULL) ;
   }


/**************************************************************************

Function:       YEAR(DateTimeNumber)
Description:    Returns the year part of DateTimeNumber
Prototype:      double YEAR(double DateTimeNumber) ;

***************************************************************************/

int YEAR(double DateTimeNumber)
   {
   int i = 1800 ;
   long serial = -36522L ;

   while (serial <= DateTimeNumber)
      {
      serial += 365 ;
      if (!(i % 4) && ((i % 100) || !(i % 400)))       /* leap year */
         serial += 1L ;
      i++ ;
      }
   i-- ;
   return(i) ;
   }



#include "order.h"

/* Source Code to Support No dbms */

void BOTTOM(LPSTR dbfname)
   {
   return ;
   }

void CLEAR(LPSTR dbfname)    /* clear data entry fields */
   {
   return ;
   }

void CLEARALL(void)    /* clear data entry fields */
   {
   return ;
   }

void DELETE(LPSTR dbfname)
   {
   return ;
   }

void INSERT(LPSTR dbfname)
   {
   return ;
   }

int LOCATE(LPSTR dbfname, int InexactFlag, LPSTR keyfield)
   {
   return(0) ;
   }

int LOCATE2(LPSTR dbfname, int InexactFlag, LPSTR keyfield1, LPSTR keyfield2)
   {
   return(0) ;
   }

int LOCATE3(LPSTR dbfname, int InexactFlag, LPSTR keyfield1, LPSTR keyfield2, LPSTR keyfield3)
   {
   return(0) ;
   }

void LOCATEFIELD(LPSTR dbfname, int InexactFlag, LPSTR keyfield)
   {
   return ;
   }

void NEXT(LPSTR dbfname)
   {
   return ;
   }

void PAGEDN(LPSTR dbfname)
   {
   return ;
   }

void PAGEUP(LPSTR dbfname)
   {
   return ;
   }

void PREVIOUS(LPSTR dbfname)
   {
   return ;
   }

void RESUME(void)
   {
   return ;
   }

void STORE(LPSTR dbfname)
   {
   return ;
   }

BOOL TOP(LPSTR dbfname)
   {
   return(TRUE) ;
   }

void UPDATE(LPSTR dbfname)
   {
   return ;
   }

BOOL InitDatabaseEngine(void)           /* intialize paradox engine and global variables */
   {
   return(TRUE) ;
   }

BOOL OpenDatabases(void)      /* handled by InitDatabaseEngine() above */
   {
   return(TRUE) ;
   }

void CloseDatabases(void)           /* close the paradox tables */
   {
   return ;
   }

void ReadDatabaseBuffer(int dbidx, int tblrow)    /* copy current db_VISTA record buffer to data entry field variables */
   {
   return ;
   }

/* check for change in value of table restriction field */
int PassesRestrictions(int dbidx, int tblidx)
   {
   return(1) ;
   }

/* return record count for the given database */
long RecordCount(int dbidx)
   {
   return(0L) ;
   }

/* goto the given record address */
long GotoRecord(int dbidx, long recaddr)
   {
   return(1L) ;
   }

void WriteDatabaseBuffer(int dbidx)    /* copy data entry fields to current paradox record buffer */
   {
   return ;
   }


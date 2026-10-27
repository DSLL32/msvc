
 /*****************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
	Module: delget.c
	Programmer:	Nam Hoang

 *****************************************************************/
 /*******************************************************************
  myDeleteCommand: Delete a command which is selected in list box. 
*********************************************************************/
static BOOL myDeleteCommand(HWND hwnd)
{
	int  nDeleteIndex;
	char szBuffer[40];

	
	if (LB_ERR == ( nDeleteIndex =( ListBox_GetCurSel(hCommListBox))) )
	{
		return FALSE;   									// no memo is selected to delete
	}

	SROnOff(hwnd,0);

	sSRWord.wIndex =  nDeleteIndex + ADDACTION+1; // index in struct sSRWord

	wsprintf((LPSTR)szBuffer," Delete %s",
				(LPSTR) lpSRContext->sCommInfo[sSRWord.wIndex].szCommName );
	SetDlgItemText(hwnd,IDD_STATUS,szBuffer);

	if( SRDeleteCommand( hwnd, &sSRWord))
	{
	  return FALSE;
	}

	ListBox_DeleteString(hCommListBox,nDeleteIndex);
	
	SRSave( hwnd, sSRWord.szUserName);

	nDeleteIndex = sSRWord.wIndex - ADDACTION-1;  	// index in struct Memo

	DeleteAMemo(hwnd,nDeleteIndex);

	AddMemoFile(hwnd,gnNumMemos);

	PostMessage(hwnd,WM_COMMAND,IDD_GET,0L);		 // set get-mode

   return TRUE;
}

/***************************************************************************
 mySetDeleteMode: to set the Delete Mode from another mode. It will
		delete a memo if there is a highlight memo inside the list box and then it will
		reset to Get-Mode.
**********************************************************************************/
static void mySetDeleteMode(HWND hwnd)
{

	if(gnAddStatus) // last mode is "Add"
	{
		gnAddStatus = 0;
	}

	gnAction = DELETEACTION;

	SRActionOnOff(hwnd,FALSE);

	if( myDeleteCommand(hwnd)) // is there a current highlight in the list box
		return;

	SetDlgItemText(hwnd,IDD_STATUS," Say a memo name to delete");

}


/************************************************************************************
 myDeleteRoutine: to set up delete mode and to delete a memo command in the list box
**************************************************************************************/
static void myDeleteRoutine(HWND hwnd)
{
	int nError,i,nDeleteIndex;

	if(gnAction != DELETEACTION) 				//   "Delete mode " not  set up yet
	  mySetDeleteMode(hwnd);    				// set up delete mode
	else  											// "Delete mode"  already set up
	  myDeleteCommand(hwnd);   				// delete a memo
}

/********************************************************************
 myGetRoutine: to set up "Get Mode" from another mode.
**********************************************************************/
static void myGetRoutine(HWND hwnd)
{
	 int i;
	 /* check current mode */
	 if(gnAction == DELETEACTION)
    {
		 SRActionOnOff(hwnd,TRUE); 				// turn action on
		if(gnNumMemos<=0)							 	// there is no memo
		{
			lpSRContext->sCommInfo[DELETEACTION].wData = 1;   // disable the "delete" comm
			SREnableCommand(hwnd,lpSRContext);	// VA is turned off already, so don't have to
															// SROnOff(hwnd,0) before this function
			Button_Enable(GetDlgItem(hwnd,IDD_DELETE),0);

      }
	 }
	 else if(gnAddStatus)
	 {
		 if(gnAddStatus == IDD_TR_ACTION)
       {
			 Button_Enable(GetDlgItem(hwnd,IDD_ADD),1);
			 Button_Enable(GetDlgItem(hwnd,IDD_DELETE),1);
			 Button_Enable(GetDlgItem(hwnd,IDD_STOPRECORD),FALSE); // disable "stop" button
		 	 SetDlgItemText(hwnd,IDD_EDIT,0);
		 }
		 gnAddStatus = 0;
    }

    /* check what current number of memo records */
	 if(gnNumMemos > 0)
	 {
			SetDlgItemText(hwnd,IDD_STATUS," Say a memo name, \"Add\" or \"Delete\"");
         if(gnNumMemos==1)
				Button_Enable(GetDlgItem(hwnd,IDD_DELETE),1);  // enable delete button
	 }
	 else  // there is no memo 
		 SetDlgItemText(hwnd,IDD_STATUS," Say \"Add\" to create a new memo");

	 gnAction = GETACTION;  // set current mode  is get-mode

	 SROnOff(hwnd,1);
	 myRequest(hwnd);      // update lpSRContext after delete or add command.

}

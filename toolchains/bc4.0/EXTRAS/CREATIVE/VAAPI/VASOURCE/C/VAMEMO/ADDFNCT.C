
/* ***********************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
 Module:	addfnct.c
 Programmer:	Nam Hoang
****************************************************************************/
FARPROC wpEditWndProc = NULL;
HWND ghEditMemo;
LRESULT CALLBACK EditMemoSubClassProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

BOOL WINAPI _export AddCommProc(HWND hwnd, UINT message,
							 UINT wParam, LONG lParam);
void myAddComm(HWND hwnd);
void myPreEditCommand(HWND hwnd);

/* myAddComm: it is a start point to add a new memo. It enalbes the  edit window to type a new name
*/
void myAddComm(HWND hwnd)
{
	SROnOff(hwnd,0); 				// turn VA Off
	gnAddStatus= IDD_EDIT; 				

	Button_Enable(GetDlgItem(hwnd,IDD_ADD),0);
	Button_Enable(GetDlgItem(hwnd,IDD_DELETE),0);

	SetDlgItemText(hwnd,IDD_STATUS," Type a new memo name. Press \"Enter\" when done.");

	EnableWindow(ghEditMemo,TRUE); 	//enable edit window for typing a memo nam

	SetFocus(ghEditMemo);

}

/*
	myPreEditCommand: will add an temp memo ( xxxxxxxxxx) and check to see
		that the index of memos in the listbox or lpSRContext and in the mem struct are same
		or not. If the index is wrong, it will sendmessage to wm_close to terminate the program.
		The case of wrong index only appears when an user deletes this app file in VA userbox and
      then add its app file again, and come back to this application to add a new memo.
*/
void myPreEditCommand(HWND hwnd)
{

	SROnOff(hwnd,0); 									 // turn vassist off

	lstrcpy(sSRWord.szCommName,"xxxxxxxxxx");  // temp. memo

	SRAddCommand(hwnd, &sSRWord);
	gnIndex = lpSRContext->wNumCommands;
                                              // update lpSRContext after add a command
	SRRequest(hwnd,lpSRContext); 					
	if(gnNumMemos != (lpSRContext->wNumCommands-ADDACTION-2))  // wrong index
	{
		sSRWord.wIndex =  gnIndex;
	 	SRDeleteCommand(GetParent( hwnd), &sSRWord);  // delete the temporary memo command
		PostMessage(hwnd,WM_CLOSE,1,0L);
      return;
   }

	SROnOff(hwnd,1);
}

/* myEditComm: It forces an user to type a name and hit return key before it trains its voice.
	This function is called inside subclass: EditMemoSubclassProc.
*/
void myEditComm(HWND hwnd)
{
	char szBuffer[33];
	int nError;
	Edit_GetText(ghEditMemo, szBuffer,
						 sizeof(szBuffer));


	if(szBuffer[0] != 0) // It cannot be empty
	{
   	 /* rename the temporary name for real memo name */	
		sSRWord.wIndex = gnIndex;
		lstrcpy(sSRWord.szCommName,szBuffer);
		nError = SRRenameCommand(GetParent(hwnd), &sSRWord);  

		if(nError )   // Error with function SRRename
		{
			ReleaseCapture();
			sSRWord.wIndex =  gnIndex; 			
			SRDeleteCommand(GetParent( hwnd), &sSRWord);  // delete the temporary memo command

			Button_Enable(GetDlgItem(GetParent(hwnd),IDD_ADD),1);
			Button_Enable(GetDlgItem(GetParent(hwnd),IDD_DELETE),1);
			Button_Enable(GetDlgItem(GetParent(hwnd),IDD_STOPRECORD),FALSE); // disable "stop" button
			SetDlgItemText(GetParent(hwnd),IDD_EDIT,0);

			MessageBox(hwnd," Cannot add this memo!","Error!",MB_OK);

			PostMessage(GetParent(hwnd),WM_COMMAND,IDD_GET,0L);
			return ;
		}
		EnableWindow(hwnd,FALSE);
		PostMessage(GetParent(hwnd),WM_RECORDMEMO,0,0L);

	}
	else // if it was empty after hitting a return key, set focus back to edit control
	{
		SetFocus(hwnd);
	}

}


/*
	EditMemoSubClassProc: this is sub class for edit a memo to recognize a return key
*/
LRESULT CALLBACK EditMemoSubClassProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LRESULT lResult=0;
	BOOL bCallOrigWndProc = TRUE;
	switch(uMsg)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_RETURN:
				 if(gnAddStatus==IDD_EDIT)
				 {
             	myEditComm(ghEditMemo);
				 }
				 break;	
				default:
      	   	break;
			}
			break;
	}

	if(bCallOrigWndProc)
		lResult=CallWindowProc(wpEditWndProc,hWnd,uMsg,wParam,lParam);

	return(lResult);
}


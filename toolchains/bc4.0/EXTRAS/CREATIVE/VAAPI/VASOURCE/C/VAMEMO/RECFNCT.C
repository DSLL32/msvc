/*************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
Module: recfnct.c
Programmer: Nam Hoang
**************************************************************/
BOOL CALLBACK _export RecordMemoProc(HWND hDlg,UINT message,UINT wParam,LONG lParam);
BOOL gbTrainActionOption = FALSE;

/* myRecordMemo(): record a memo.
*/
void myRecordMemo(HWND hwnd)
{
	
	char szBuffer[200],cRetString[80];
	DWORD dwRes,i;

	SROnOff(hwnd,0); 					// turn off engine before it records

	dwRes = mciSendString ((LPSTR)"close goober", NULL, 0, NULL);  // close other opening mci

	GetVoiceFileName(gnNumMemos); // set up a new voice file

	dwRes = mciSendString ("open new type waveaudio alias goober",
									 cRetString, 80, NULL);
	if (dwRes)
	{
     ReleaseCapture();
	  mciGetErrorString(dwRes,cRetString,80);
	  myRequest(hwnd);
	  return ;
	}

	dwRes = mciSendString ((LPSTR)"record goober from 0 to 10000 notify",
									cRetString, 80, hwnd);

	if (dwRes)
	{
	  ReleaseCapture();
	  mciGetErrorString(dwRes,cRetString,80);
	  myRequest(hwnd);
	  return;
	}

	gnAddStatus = IDD_RECORDMEMO;

	Button_Enable(GetDlgItem(hwnd,IDD_STOPRECORD),TRUE);  // enable STOP button

	SetDlgItemText(hwnd,IDD_STATUS,
	  " Please record  memo. 10 seconds max or press \"Stop\" when done.");

	SetFocus(GetDlgItem(hwnd,IDD_STOPRECORD));

}

/* mySaveMemo: When recording is over, it saves a new memo command and updates the structure Memo and
	file ---.mem

*/
void mySaveMemo(HWND hwnd)
{
	char szBuffer[200];
	DWORD dwRes,i;

	dwRes = mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);

	wsprintf((LPSTR)szBuffer,"save goober %s",(LPSTR)Memo[gnNumMemos].szVoiceFile);

	dwRes = mciSendString ((LPSTR)szBuffer, NULL, 0, NULL);

	if (dwRes)
	{

		Button_Enable(GetDlgItem(hwnd,IDD_ADD),1);
		Button_Enable(GetDlgItem(hwnd,IDD_DELETE),1);
		Button_Enable(GetDlgItem(hwnd,IDD_STOPRECORD),FALSE); // disable "stop" button

		sSRWord.wIndex =  gnIndex; // index in struct sSRWord
		SRDeleteCommand( hwnd, &sSRWord);

		SetDlgItemText(hwnd,IDD_EDIT,0);
		MessageBox(hwnd,"Cannot add this command. Try again","Error",MB_OK);

		SendMessage(hwnd,WM_COMMAND,IDD_GET,0L);
		return;
	}
	gnNumMemos++;
	if(gnNumMemos==1) // first  memo  is add into struct, enable the "delete" command
	{
		lpSRContext->sCommInfo[DELETEACTION].wData = 0;   // enable the "delete" comm
		SREnableCommand(hwnd,lpSRContext);
	}

	dwRes = mciSendString ((LPSTR)"close goober", NULL, 0, NULL);

	AddMemoFile(hwnd,gnNumMemos);   // save and update into struct Memo and file ---.mem

	if(gnNumMemos != (lpSRContext->wNumCommands-ADDACTION-1))
	{
		SendMessage(hwnd,WM_CLOSE,0,0L);
      return ;
   }
	if(gbTrainActionOption) // if train action option is chosen
	{

		Button_Enable(GetDlgItem(hwnd,IDD_STOPRECORD),FALSE);
		PostMessage(hwnd,WM_COMMAND,IDD_TR_ACTION,0L);
   }
	else
	{
		Button_Enable(GetDlgItem(hwnd,IDD_ADD),TRUE);
		Button_Enable(GetDlgItem(hwnd,IDD_DELETE),TRUE);
		Button_Enable(GetDlgItem(hwnd,IDD_STOPRECORD),FALSE);
		SetDlgItemText(hwnd,IDD_EDIT,0);
		gnAddStatus = 0;
		SendMessage(hwnd,WM_COMMAND,IDD_GET,0L);
		PostMessage(hwnd,WM_SAVE,0,0L);  		// save a memo

	}
}


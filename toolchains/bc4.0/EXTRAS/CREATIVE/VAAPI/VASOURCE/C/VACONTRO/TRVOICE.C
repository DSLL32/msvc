#include <windows.h>
#include <windowsx.h>
#include <memory.h>
#include "apicli.h"
#include <vasapi.h>



extern WORD	msgActionStart, msgActionStop, msgSpeechStart, msgSpeechStop;
extern WORD	msgLinkTerminated, msgSignal, msgAppSwitch;


extern LPSRCONTEXT	lpSRContext;
extern LPSRUSERLIST	lpSRUserList;
extern SRWORD	sSRWord;
extern HWND hDlgModeless;

extern int gnIndex;

/* HideOkButton: - hides ok button, shows cancel button and assign sSRWord.wIndex = gnIndex
*/
void HideOkButton(HWND hOk,HWND hCancel)
{
	
	int i;
	if(IsWindowEnabled(hOk))
	{
		sSRWord.wIndex = gnIndex;  // index of a command to train
		EnableWindow(hOk,FALSE);
		ShowWindow(hOk,SW_HIDE);
		ShowWindow(hCancel,SW_SHOWNORMAL);
		for (i=0;i<3;i++)
			EnableWindow(GetDlgItem(GetParent(hOk),ADD_NOTRAIN+i),FALSE);
	}
}

BOOL WINAPI _export VoiceTrainProcModel(HWND hDlg,UINT message,
				UINT wParam,LONG lParam)
{
	#define  MY_QUIT WM_USER+201

	static  	nNumTrains = 0; // number trainings for each command
	static 	BOOL bFirstTime = FALSE;
	static 	BOOL bLink =FALSE;
	static 	int i= 0;
   int j;
	if( message == msgAppSwitch)
	{

		if( wParam) // GAIN FOCUS
		{
			SRNotify(hDlg,1);
			if(!bFirstTime)
			{
				SRRequest(hDlg,lpSRContext);
				SetDlgItemText(hDlg,ADD_EDIT,
					(LPSTR) lpSRContext->sCommInfo[gnIndex].szCommName);

				bFirstTime = TRUE;
			}
			SREnableCommand(hDlg,lpSRContext);
			SROnOff(hDlg,0);
			return 0;
		}
		if(wParam == 0) // LOSE FOCUS
		{
			SRNotify(hDlg,1);
			SRRequest(hDlg,lpSRContext);
			return 0;

		}

	}

	if	(message == msgSpeechStart)
	{
		return 0;
	}
	if(message == msgSpeechStop)
	{

		if(HIWORD(lParam))
		{

			MessageBox(hDlg,"Command which was just trained is similiar to another one or was bad recording! Record again","Error",MB_OK);

		  //	SetWindowText(GetDlgItem(hDlg,IDCANCEL),"OK");
		  //	SetWindowWord(GetDlgItem(hDlg,IDCANCEL),GWW_ID,IDOK);

			PostMessage(hDlg,WM_COMMAND,IDOK,0L);
			return 0;
		}

		Button_SetCheck(GetDlgItem(hDlg,ADD_NOTRAIN+nNumTrains-i),0);

		if(i<nNumTrains)
		{
			i++;
		  //	SetWindowText(GetDlgItem(hDlg,IDCANCEL),"OK");
		  //	SetWindowWord(GetDlgItem(hDlg,IDCANCEL),GWW_ID,IDOK);
			Button_SetCheck(GetDlgItem(hDlg,ADD_NOTRAIN+nNumTrains-i),1);
			PostMessage(hDlg,WM_COMMAND,IDOK,0);
		}
		else
			SendMessage(hDlg,MY_QUIT,0,0L);// finish the training
		return 0;
	}
	if(message==msgLinkTerminated)
	{
		bFirstTime = bLink = FALSE;
		i=0;
		nNumTrains = 0;
		return 0;
   }
	switch(message)
	{
		case WM_INITDIALOG:
			Button_SetCheck(GetDlgItem(hDlg,ADD_NOTRAIN),1);
			bLink = FALSE;
			return 0;

		case WM_ACTIVATE:
			if(!bLink)
				bLink = !SRInitializeLink(hDlg);
			return 0;

		case MY_QUIT:
			SROnOff(hDlg,0);
			SRSave(hDlg,sSRWord.szUserName);
			SRTerminateLink(hDlg);
			EndDialog(hDlg,0);
         return 0;

		case WM_COMMAND:
		switch(wParam)
		{
			case ADD_NOTRAIN:
			case ADD_NOTRAIN2:
			case ADD_NOTRAIN3:
				nNumTrains = wParam-ADD_NOTRAIN;
      		return 0;

			case IDOK:
				//SetWindowText(GetDlgItem(hDlg,IDOK),"Cancel");
			  //	SetWindowWord(GetDlgItem(hDlg,IDOK),GWW_ID,IDCANCEL);

           // for(j=0;j<3;j++)
			  //		EnableWindow(GetDlgItem(hDlg,ADD_NOTRAIN+j),FALSE);
			  HideOkButton(GetDlgItem(hDlg,IDOK),GetDlgItem(hDlg,IDCANCEL));

				if(i>0)
				{
					sSRWord.bNew = 0;

				}
				else
				{
					sSRWord.bNew = 1;

				}

				 // over-write the old voice if it exists
				SRTrainVoice(hDlg, &sSRWord);

				return 0;

			case IDCANCEL:
				SRAbortVoiceTraining(hDlg);
				PostMessage(hDlg,MY_QUIT,0,0L);
				return 0;
		} // end of switch wm_command


	}
	return 0;

}

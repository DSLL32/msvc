/*  VCS_ID
 *  $Filename:   printdlg.cxx
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:18:36  $
 */
 
extern "C"
{
	#include <stdio.h>
}

#define InclForms
#define InclPrinters
#define InclProfiles

#include <ObjectPM.hpp>
#include "printdlg.hpp"


PrintDlg :: PrintDlg(wWindow *parent, wPrinterSetup *_origSetup) : wFormWindow (D_PRINTERS, 1, 1, DlgModal)
{
	wButtonField *bfldp;

	AddField( bfldp = new wPushButtonField(B_JOBPROP) );
	bfldp->SetFieldClickFn(FORMMETHOD(PrintDlg,ProcJobProperties));

	AddListRegion( prLReg = new wListRegion(L_PRINTERLIST, LrNoClear | LrNoDelete) );

	prLReg->SetIndent(8);
	prLReg->AddGroupObject(NULL, 62);
	prLReg->AddGroupObject(NULL, 4, ColBlank);
	prLReg->AddGroupObject(NULL, 154);
	prLReg->AddGroupObject(NULL, 4, ColBlank);
	prLReg->AddGroupObject(NULL, 40);

	SetActionButtons(B_CANCEL, B_OK, NULL);
	origSetup = _origSetup;
	currSetup = _origSetup;
	currPrinter = -1;
	fChanged = FALSE;

	FormUp(parent);
}

PrintDlg :: ~PrintDlg()
{
	delete prlist;
}

short PrintDlg :: Init()
{
	HourGlass();
	wPrinterInfo pi;

	prlist = pi.EnumPrintDestinations();

	if (!prlist || !prlist->Entries())
	{
		DisableActionButton();
		GetField(B_JOBPROP)->Disable();
	}
	else
	{
		char buf[80];
		short i = 0;

		for (prlist->First(); prlist->Current(); prlist->Next(), i++)
		{
			wPrinterSetup *p = prlist->Current();
			sprintf(buf, "%s!T%s!T%s", (const char *)p->GetPhysicalName(), 
			        (const char *)p->GetObjectName(), 
					(const char *)p->GetPort());

			prLReg->InsertItemText(buf);

			if (origSetup->GetPort() == p->GetPort())
				currPrinter = i;
		}
		if (currPrinter >= 0)
			prLReg->SelectItem(currPrinter);
	}
	HourGlass();
	return TRUE;
}

short PrintDlg :: ProcJobProperties(wButtonField *)
{
	short item = prLReg->GetCurrentItem();
	if (item != currPrinter)
 	{
 		currSetup = (*prlist)[item];
		currPrinter = item;
	}
	fChanged++;

	currSetup->PostJobProperties();
	return TRUE;
}

short PrintDlg :: Save()
{
	short item = prLReg->GetCurrentItem();
	if (item != currPrinter)
 	{
 		currSetup = (*prlist)[item];
		fChanged++;
	}

	if (currSetup != origSetup)
	{
		// remove the PrinterSetup object from the list
		// to prevent it from being deleted when "prlist" is
		// deleted in our constructor

		if (prlist->Find(currSetup))
			prlist->Remove();

		delete origSetup;
	}

	return TRUE;
}

short PrintDlg :: ConfirmQuit()
{
	currSetup = origSetup;
	fChanged = FALSE;

	return TRUE;
}

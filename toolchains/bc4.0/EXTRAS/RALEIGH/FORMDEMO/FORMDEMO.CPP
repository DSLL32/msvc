/*  VCS_ID
 *  $Filename:   formdemo.cpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:20:32  $
 */

#define InclGraphics
#define InclControls
#define InclForms
#define InclHelp
#include <ObjectPM.hpp>
#include "formdemo.hpp"

// Start of the application
short bMainThread :: Start()		
{
	// construct and initialize an instance of the 
	// IPF help manager

	wHelpManager help("formdemo.hlp");
    help.SetHelpWindowTitle("Help for FormDemo");
   	help.SetHelpTable(MAIN_HELPTABLE);
	help.SetKeysHelpPanel(KEYSHELP);


	AppWindow w;	    	// Contstruct the top level frame window
	Exec();				// Go to the message loop.

	return 0;
}

AppWindow :: AppWindow()
{	
	CreateWindow(FaTitleBar | FaTaskList | FaSysMenu | FaSizeBorder | FaMinMax);
	SetCaption("FormWindow Demo");					  // Set captions
	SetSwitchTitle("FormWindow Demo Program");
	SetIcon(icon = new wIcon(ResIcon, I_FORMDEMO));   // Set the app's icon

	// This section create a menu bar and then adds items to the
	// sub menu itself.  It is also possible to construct the menu
	// from the resource file if desired

	menubar = new wMenu(this, 100, "~Form\\~Help\\");
	menubar->SetSubMenuItems(SM_HELP, MI_HELP, "~Help for help...;~General help...;~Keys help...;Help ~index;");
	
	// connect to HelpManager object
	AssociateHelp();	
	help = ThisThread->HelpInstance();

	np = new FormNoteBook(this, IdNotebook, wPointl(2, 0, DlgUnits),
						 wDimension(296, 150, DlgUnits), WsVisible | BkSpiralBinding);

	np->SetMajorTabDimension(wDimension(42, 12, DlgUnits));
	np->SetMinorTabDimension(wDimension(40, 12, DlgUnits));
	wNoteBookPageList *pages = np->Pages();
	data = new FormData;

	forms[0]  = new StringForm(pages, data);
	forms[1]  = new CharForm(pages, data);
	forms[2]  = new MLEStringForm(pages, data);
	forms[3]  = new DateForm(pages, data);
	forms[4]  = new TimeForm(pages, data);
	forms[5]  = new ShortForm(pages, data);
	forms[6]  = new LongForm(pages, data);
	forms[7]  = new FloatForm(pages, data);
	forms[8]  = new MoneyForm(pages, data);
	forms[9]  = new ButtonForm(pages, data);
	forms[10] = new ListForm(pages, data);

	ChangeFrameSize( CalcFrameSize(wDimension(310, 160, DlgUnits)) );
	ChangePosition(PosCenter);
	np->GotoPage((*pages)[1]);
	Show();
	ToTop();
}

AppWindow :: ~AppWindow()
{				   
	delete menubar;
	delete icon;
}


void AppWindow :: Paint()
{
	WindowPS()->Erase(WindowRectangle(), ClrPaleGray);		  // Erase the client window
}

// The Close member is called when the "Close" menu option is chosen
// from the system menu.  Returning TRUE from this function allows the
// exit process to continue.  Otherwise, a return of FALSE will prevent
// the application from being closed

long AppWindow :: Close(wMessage)
{
	return TRUE;
}


// Command event to process menu selections

long AppWindow :: MenuCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case MI_HELP:
			help->ShowHelp();
			break;

		case MI_EXTHELP:
			help->ShowExtendedHelp();
			break;

		case MI_KEYHELP:
			help->ShowKeysHelp();
			break;

		case MI_INDEX:
			help->ShowIndex();
			break;

	}
	return FALSE;
}


void FormNoteBook :: PageSelected(wNBPageMsg m)
{
	wNoteBookPage *pw;
	static bool fReSelect = FALSE;
	static bool fFirstCall = TRUE;

	if (fFirstCall)
	{
		// This event always occurs when the first page is added to
		// a notebook.  Since we want the second page to be at the
		// top, we'll skip this event and set the top page after all
		// pages have been constructed.

		fFirstCall = FALSE;
		return;
	}

	if (fReSelect)
	{
		fReSelect = FALSE;
		return;
	}

	pw = m.CurrentTopPage();
	wFormWindow *pgWin;

	// If there is a current top page, dismiss it's form (dialog)
	//
	if (pw)
	{
		if (pw == m.NewTopPage())
			return;

		if ((pgWin = (wFormWindow *)pw->GetWindow()) != NULL)
		{
			if (!pgWin->Exit(AcFormExit, NULL))
			{
				fReSelect = TRUE;	// validation error occured
				GotoPage(pw);		// re-select current top page
				return;
			}
			pgWin->Dismiss();
			pw->SetWindow(NULL);
		}
	}

	// For a new page, find the form associated with the new page
	// and create its window.
	//
	if ((pw = m.NewTopPage()) != NULL)
	{
		if (pw->GetPageHandle() >= IdMajorSections)
		{
			// major tab was selected in section with minor tab pages
			wNoteBookPageList *pl = Pages();
			pl->Find(pw);
			if (pl->Next())		// select the page after the major tab page
				GotoPage(pl->Current());

			return;
		}
		wFormWindow **forms = ((AppWindow *)ParentWindow())->GetDataForms();
		wFormWindow *fp = forms[pw->GetPageHandle()];

		fp->FormUp(this);
		fp->ChangePosition(wPointl(0, 0));
		pw->SetWindow(fp);
		fp->Show();
		(ThisThread->HelpInstance())->SetActiveWindow(fp);
	}
}


// Data

FormData :: FormData()
		  : mleString(NULL, 512)
{
	shortUInt = commaUInt = zpadUInt = blankUInt = signedInt =  0;
	ssignedInt = base36UInt = octalUInt = binaryUInt = hexUInt = 0;

	ULong = commaULong = zpadULong = blankULong = signedLong =  0;
	ssignedLong = base36ULong = octalULong = binaryULong = hexULong = 0;

	dbl = fixedPrec = sdbl = commaDbl = expNot = 0.0;
	*uprString = *lwrString = 0;
	mleString = "This is a multi-line edit control.";
	c = n = u = yn = tf = 0;
}

// Forms ...

StringForm :: StringForm(wNoteBookPageList *pgList, FormData *data)
			: wFormWindow(D_STRINGS, 5, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtString, 32, E_STRING, &data->string, NULL) );
	AddField( new wEditField(FtString, 32, E_NBRONLY, &data->numString, "%9") );
	AddField( new wEditField(FtString, 32, E_LETONLY, &data->letString, "%A") );
	AddField( new wEditField(FtCharArray, 32, E_ALLUPPER, data->uprString, "%!") );
	AddField( new wEditField(FtCharArray, 32, E_ALLLOWER, data->lwrString, "%~") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor, ""));
	pg->SetTab("Character");
	pg->SetPageHandle(IdCharTypes);

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMinor | BpStatusTextOn, ""));
	pg->SetStatusLine("Characters 1 of 3");
	pg->SetTab("String");
	pg->SetPageHandle(IdStringPage);
}

CharForm :: CharForm(wNoteBookPageList *pgList, FormData *data)
			: wFormWindow(D_CHARS, 5, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtChar, 1, E_CHAR, &data->c, NULL) );
	AddField( new wEditField(FtChar, 1, E_NUMERIC, &data->n, "%9") );
	AddField( new wEditField(FtChar, 1, E_TOUPPER, &data->u, "%!") );
	AddField( new wEditField(FtBool, 1, E_BOOLYN, &data->yn, "%rYN") );
	AddField( new wEditField(FtBool, 1, E_BOOLTF, &data->tf, "%!") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMinor | BpStatusTextOn, ""));
	pg->SetStatusLine("Characters 2 of 3");
	pg->SetTab("Char");
	pg->SetPageHandle(IdCharPage);
}

MLEStringForm :: MLEStringForm(wNoteBookPageList *pgList, FormData *data)
			: wFormWindow(D_MLESTRING, 1, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtMLString, 256, E_MLESTRING, &data->mleString, NULL) );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMinor | BpStatusTextOn, ""));
	pg->SetStatusLine("Characters 3 of 3");
	pg->SetTab("MLE");
	pg->SetPageHandle(IdMLEStringPage);
}


DateForm :: DateForm(wNoteBookPageList *pgList, FormData *data)
			: wFormWindow(D_DATES, 5, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtDate, 10, E_LONGDATE, &data->longDate, "%r/%m/%d/%4y") );
	AddField( new wEditField(FtDate, 8, E_SHORTDATE, &data->shortDate, "%r/%1m/%d/%y") );
	AddField( new wEditField(FtDate, 8, E_NLSDATE, &data->nlsDate, "%c") );
	AddField( new wEditField(FtDate, 24, E_MONTHBYNAME, &data->stringDate, "%B %d, %4y") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor | BpStatusTextOn, ""));
	pg->SetTab("Date");
	pg->SetPageHandle(IdDatePage);
}


TimeForm :: TimeForm(wNoteBookPageList *pgList, FormData *data)
			: wFormWindow(D_TIME, 5, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtTime, 8, E_LONGTIME, &data->f24Time, "%H:%M:%S") );
	AddField( new wEditField(FtTime, 11, E_SHORTTIME, &data->f12Time, "%I:%M %p") );
	AddField( new wEditField(FtTime, 11, E_NLSTIME, &data->nlsTime, "%C") );
	AddField( new wEditField(FtTime, 24, E_TIMESTAMP, &data->timestamp, "%r-%R.%4y/%m/%d/%H:%M:%S:%h") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor | BpStatusTextOn, ""));
	pg->SetTab("Time");
	pg->SetPageHandle(IdTimePage);
}

ShortForm :: ShortForm(wNoteBookPageList *pgList, FormData *data)
		   : wFormWindow(D_INTS, 10, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtUShort, 5, E_SHORT, &data->shortUInt) );
	AddField( new wEditField(FtUShort, 4, E_ZEROPAD, &data->zpadUInt, "%w4") );
	AddField( new wEditField(FtUShort, 5, E_INITBLANK, &data->blankUInt, NULL, FsInitBlank) );
	AddField( new wEditField(FtShort, 6, E_SIGNED, &data->signedInt) );
	AddField( new wEditField(FtShort, 6, E_SHOWSIGN, &data->ssignedInt, "%+") );
	AddField( new wEditField(FtUShort, 6, E_COMMA, &data->commaUInt, "%,") );
	AddField( new wEditField(FtUShort, 6, E_OCTAL, &data->octalUInt, "%b8") );
	AddField( new wEditField(FtUShort, 16, E_BINARY, &data->binaryUInt, "%b2") );
	AddField( new wEditField(FtUShort, 4, E_HEX, &data->hexUInt, "%b16") );
	AddField( new wEditField(FtUShort, 3, E_BASE36, &data->base36UInt, "%b36") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor, ""));
	pg->SetTab("Integer");
	pg->SetPageHandle(IdIntegers);

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMinor | BpStatusTextOn, ""));
	pg->SetTab("Short");
	pg->SetStatusLine("Integers 1 of 2");
	pg->SetPageHandle(IdShortPage);
}

LongForm :: LongForm(wNoteBookPageList *pgList, FormData *data)
		  : wFormWindow(D_LONGS, 10, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtULong, 10, E_LONG, &data->ULong) );
	AddField( new wEditField(FtULong, 10, E_ZEROPAD, &data->zpadULong, "%w10") );
	AddField( new wEditField(FtULong, 10, E_INITBLANK, &data->blankULong, NULL, FsInitBlank) );
	AddField( new wEditField(FtLong, 11, E_SIGNED, &data->signedLong) );
	AddField( new wEditField(FtLong, 12, E_SHOWSIGN, &data->ssignedLong, "%+") );
	AddField( new wEditField(FtULong, 13, E_COMMA, &data->commaULong, "%,") );
	AddField( new wEditField(FtULong, 12, E_OCTAL, &data->octalULong, "%b8") );
	AddField( new wEditField(FtULong, 32, E_BINARY, &data->binaryULong, "%b2") );
	AddField( new wEditField(FtULong, 8, E_HEX, &data->hexULong, "%b16") );
	AddField( new wEditField(FtULong, 6, E_BASE36, &data->base36ULong, "%b36") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMinor | BpStatusTextOn, ""));
	pg->SetTab("Long");
	pg->SetStatusLine("Integers 2 of 2");
	pg->SetPageHandle(IdLongPage);
}


FloatForm :: FloatForm(wNoteBookPageList *pgList, FormData *data)
		  : wFormWindow(D_FLOATS, 5, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtDouble, 64, E_DOUBLE, &data->dbl, NULL, FsInitBlank) );
	AddField( new wEditField(FtDouble, 64, E_FOURPL, &data->fixedPrec, "%d4%z") );
	AddField( new wEditField(FtDouble, 64, E_SHOWSIGN, &data->sdbl, "%+") );
	AddField( new wEditField(FtDouble, 64, E_COMMASEP, &data->commaDbl, "%,") );
	AddField( new wEditField(FtDouble, 64, E_EXPNOT, &data->expNot, "%e") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor | BpStatusTextOn, ""));
	pg->SetTab("Float");
	pg->SetPageHandle(IdFloatPage);
}


MoneyForm :: MoneyForm(wNoteBookPageList *pgList, FormData *data)
		  : wFormWindow(D_MONEY, 5, 0, DlgModeless)
{
	wNoteBookPage *pg;

	AddField( new wEditField(FtMoney, 24, E_AMOUNT, &data->amt, NULL, FsInitBlank) );
	AddField( new wEditField(FtMoney, 24, E_CURRENCYIND, &data->indMoney, "%$") );
	AddField( new wEditField(FtMoney, 24, E_SHOWSIGN, &data->showMoney, "%$%+") );
	AddField( new wEditField(FtMoney, 24, E_COMMASEP, &data->commaMoney, "%$%,") );
	AddField( new wEditField(FtMoney, 24, E_PARENNEG, &data->parenMoney, "%$%(") );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor | BpStatusTextOn, ""));
	pg->SetTab("Money");
	pg->SetPageHandle(IdMoneyPage);
}

short MoneyForm :: Init()
{
	return 0;
}

ButtonForm :: ButtonForm(wNoteBookPageList *pgList, FormData *data)
		  : wFormWindow(D_BUTTONS, 6, 0, DlgModeless)
{
	wNoteBookPage *pg;
	wPushButtonField *pf;

	AddField( new wRadioButtonField(B_WINDOW, 3, &data->radBtn) );
	AddField( new wCheckBoxField(B_FOUNDATION, &data->foundChk) );
	AddField( new wCheckBoxField(B_GRAPHICS, &data->graphChk) );
	AddField( new wCheckBoxField(B_KITCHEN, &data->ksinkChk) );

	AddField( pf = new wPushButtonField( B_UNDO) ); 
	pf->SetFieldClickFn( FORMMETHOD(ButtonForm, ProcUndo) );

	AddField( pf = new wPushButtonField( B_MORE) );
	pf->SetFieldClickFn( FORMMETHOD(ButtonForm, DoMore) );

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor | BpStatusTextOn, ""));
	pg->SetTab("Button");
	pg->SetPageHandle(IdButtonPage);
	SetActionButtons(0, 0, B_HELP);
}

short ButtonForm :: ProcUndo(short)
{
	GetField(B_WINDOW)->Update();
	GetField(B_FOUNDATION)->Update();
	GetField(B_GRAPHICS)->Update();
	GetField(B_KITCHEN)->Update();

	return 0;	
}

short ButtonForm :: DoMore(short)
{
	MoreForm fm(&info);
	fm.FormUp(this);

	// reset the help window association
   	(ThisThread->HelpInstance())->SetActiveWindow(this);
	return 0;	
}

MoreForm :: MoreForm(UserInfo *data)
		  : wFormWindow(D_MOREPOPUP, 3, 0, DlgModal)
{
	AddField( new wEditField(FtString, 32, E_NAME, &data->name, NULL, FsRequired) );
	AddField( new wEditField(FtString, 32, E_COMPANY, &data->company) );
	AddField( new wEditField(FtString, 20, E_PHONE, &data->phone) );
	ip = data;
	
    SetOptions(FmEnterExit);
	SetMessageWindow(T_MESSAGES);
	SetActionButtons(DID_CANCEL, DID_OK, DID_HELP);
}

short MoreForm :: Init()
{
	if (ip->name.Length() == 0)
		DisplayMessage("Please enter some info about yourself");

	SetValError(0);
   	(ThisThread->HelpInstance())->SetActiveWindow(this);
	return 0;
}

ListForm :: ListForm(wNoteBookPageList *pgList, FormData *)
		  : wFormWindow(D_LISTR, 4, 1, DlgModeless)
{
	wNoteBookPage *pg;
	wListRegion *lreg;

	AddField( new wEditField(FtLong, 6, E_PARTNO, &partno, NULL, FsRequired) );
	AddField( new wEditField(FtString, 32, E_DESCR, &desc, NULL, FsRequired) );
	AddField( new wEditField(FtMoney, 10, E_AMOUNT, &amt, "%$%,", FsRequired | FsLastRegFld) );
	AddField( new wEditField(FtMoney, 11, E_TOTAL, &total, "%$%,") );

	// create the list region and assoicated edit fields with columns
	AddListRegion( lreg = new wListRegion(L_LINEITEMS) );
	lreg->AddGroupObject(E_PARTNO, 40);
	lreg->AddGroupObject(E_DESCR, 112);
	lreg->AddGroupObject(E_AMOUNT, 50);
	lreg->SetProcItemFn(FORMMETHOD(ListForm, ProcItem));

	pgList->InsertPage(pg = new wNoteBookPage(NULL, BpMajor | BpStatusTextOn, ""));
	pg->SetTab("List");
	pg->SetPageHandle(IdListPage);

	SetMessageWindow(T_MESSAGES);
	SetActionButtons(NULL, NULL, DID_HELP);
}

short ListForm :: Init()
{
	wListRegion *lreg = GetListRegion(L_LINEITEMS);

	// create some sample line items...
	lreg->InsertItemText("7601!T486 Personal Computer!T$ 1,995.95");
	lreg->InsertItemText("5900!TVGA Color Monitor!T$ 290.00");
	lreg->InsertItemText("1911!T19.2KBaud Fax/Modem!T$ 200.00");

	partno = 0;
	amt = 0L;
	desc = "";
	total = 248595L;
	return 0;
}

short 
ListForm :: ProcItem(wListRegion *regp, char *itemtext, char **prevdata, short cmd)
{
	char mbuf[12];
	bMoney newVal;

	if (cmd != RegDelete)
	{
		regp->ExtractColumnText(itemtext, mbuf, 2);		// get money value
		newVal =mbuf;
	}
	switch (cmd)
	{
		case RegInsert:								// new item entered?
			regp->SetInsertIndex(LiEnd);
			total += newVal;
			break;
		
		case RegModify:								// edit existing line item?
		{
	   		regp->SetInsertIndex(regp->GetCurrentItem());
			bMoney oldVal(prevdata[2]);
			total -= oldVal;
			total += newVal;
			break;
		}
		case RegDelete:
		{
			bMoney oldVal(prevdata[2]);
			total -= oldVal;
		}
	}
	GetField(E_TOTAL)->Update();
	return TRUE;
}



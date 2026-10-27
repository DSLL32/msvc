/*  VCS_ID
 *  $Filename:   mdiedit.cpp
 *  $Author  :   Mike Flis
 *  $Revision:   1.5  $
 *  $Date:   06 Jan 1992 00:13:56  $
 */
 
#include "mdiedit.hpp"

#include <stdio.h> 
#include <string.h>

#define WM_QUIT  	0x002a
#define WM_ACTIVATE 0x000d

long	carr[] = {ClrWhite, ClrBlack, ClrBlue, ClrRed, ClrPink, ClrGreen, 
	ClrCyan, ClrYellow, ClrDarkGray, ClrDarkBlue, ClrDarkRed, ClrDarkPink,
	ClrDarkGreen, ClrDarkCyan, ClrBrown, ClrPaleGray};

short bMainThread :: Start()
{
	Editor	ed; 
	int		lp;

	if (argc > 1)
	{
		File	*f;

		for (lp=1 ; lp < argc ; lp++)
		{
			f = new File(argv[lp]);
			if (!f->Exists())
			{
				char buf[300];
				sprintf(buf, "File %s does not exist", argv[lp]);
				wMessageBox mb(&ed, buf, "Error", MbIconExclamation | MbOk);
			}
			else
				new Document(&ed, argv[lp]);
			delete f;
		}
		ed.TileDocuments();
	}

	Exec();

	return 0;
}

Editor :: Editor() : wMdiDesktop(W_MENU, W_TILE, W_CASCADE, W_FIRST)
{
	static Accel	a[] = {
		{AsAlt | AsVirtualKey, VkBackspace, E_UNDO},
		{AsShift,	  VkDelete, E_CUT},
		{AsControl,  VkInsert, E_COPY},
		{AsShift,    VkInsert, E_PASTE},
		{AsChar | AsControl,   'f',          E_FIND},
		{AsVirtualKey,          VkF3,        F_QUIT},
		{AsChar | AsControl,   'o',          F_OPEN},
		{AsChar | AsControl,   's',          F_SAVE},
		{AsChar | AsControl,   'n',          F_NEW},
		{AsChar | AsControl,   'a',          E_SELECTALL}
	};

	// COLORS STRING FOR MENU
	char *MENUCOLOR="~White;~Black;B~lue;~Red;~Pink;~Green;~Cyan;~Yellow;\
~Dark Gray;Dark Bl~ue;Dark R~ed;Dark P~ink;Dark Gree~n;Dark Cya~n;Br~own;P~ale Gray";

	char *OPTIONS="~Undo\tAlt+Backspace;-;Cu~t\tShift+Delete;~Copy\tCtrl+Insert;\
~Paste\tShift+Insert;-;Cl~ear;-;~Find...\tCtrl+F;Select ~all\tCtrl+A;";

	CreateWindow(OpmStdFrame);

	SetCaption("MDI Editor");
	SetSwitchTitle("MDI Editor");
	
	menu = new wMenu(this, F_MENU, "~File\\~Edit\\~Options\\~Window\\");
	menu->SetSubMenuItems(F_MENU, F_NEW, "~New\tCtrl+N;~Open...\tCtrl+O;-;~Save\tCtrl+S;Save ~As...;~Close;-;~Quit\tF3;");
	menu->SetSubMenuItems(E_MENU, E_UNDO, OPTIONS);
	menu->AppendSubMenu( O_MENU, O_FOREG, "~Foreground");
	menu->AppendSubMenu( O_MENU, O_BACKG, "~Background");
	menu->SetSubMenuItems(O_MENU, O_FONT, "F~ont");
	menu->SetSubMenuItems(O_MENU, O_TABSTOPS, "~Tabstops");
	menu->SetSubMenuItems(O_MENU, O_WWRAP, "~Word Wrap");
	menu->SetSubMenuItems(O_FOREG, O_FGCOLOR, MENUCOLOR);
	menu->SetSubMenuItems(O_BACKG, O_BGCOLOR, MENUCOLOR);
	menu->SetSubMenuItems(W_MENU, W_TILE, "~Tile;~Cascade;-;");

	SetIcon(icon = new wIcon(ResIcon, I_OBJPM));   

	accel = new wAccelTable(this);
	accel->Set(a, 10);
	cs = 0;

	fgc = wColor(ClrBlack);
	bgc = wColor(ClrWhite);
	tab = 64;
	wwrap = FALSE;

	wPresSpace	ps(this);
	font = new wFont((wStdFont)0, NormalFont, &ps);

	int lp;

	for (lp=0 ; lp < 16 && carr[lp] != (long)fgc; lp++)
		; 

	menu->CheckItem(O_FGCOLOR + lp);
	for (lp=0 ; lp < 16 && carr[lp] != (long)bgc; lp++)
		;
 
	menu->CheckItem(O_BGCOLOR + lp);
	Show();
	ToTop();
}

Editor :: ~Editor()
{
	delete menu;
	delete accel;
	delete icon;
}

#define MFIRST	0
#define MLAST   1

// This method takes care of enabling/disabling the menu items

long Editor :: MenuAction(wMenuActionMsg m)
{
	static ushort menuBlocks[4][2] = { {F_SAVE, F_CLOSE},
		   {E_UNDO, E_SELECTALL}, { 0, 0 }, { W_TILE, W_CASCADE} };

	if (m.actionType() == MENUINIT)
	{
		Document *top = (Document*)TopMdiDocument();
		ushort iMenu, i;

		iMenu = (m.usId() / 100) - 1;
		for (i = menuBlocks[iMenu][MFIRST];
			i <= menuBlocks[iMenu][MLAST] && menuBlocks[iMenu][MFIRST]; i++)
		{
			// if 'top' is NULL, then there are no document windows open.
			// if this is so, then most menu items must be turned off

			if (top)
				menu->EnableItem(i);
			else
				menu->DisableItem(i);
		}
		if (top && iMenu == 1)		// Edit menu?
		{
			wRange r = top->mle->GetSelection();
			int fSelection =  r.Low() != r.High();
			for (i = E_CUT; i <= E_CLEAR; i++)
			{
				if (fSelection)
					menu->EnableItem(i);
				else
					menu->DisableItem(i);
			}
			if (top->mle->GetUndoStatus())
				menu->EnableItem(E_UNDO);
			else
				menu->DisableItem(E_UNDO);
		}
	}
	return m.actionType() == MENUSELECT;
}

long Editor :: MenuCommand(wCommandMsg m)
{
	Document		*top = (Document*)TopMdiDocument();
	long			oldc;
	short			lp;
	Document		*doc;

	// SET FOREGROUND COLOR
	if (m.usCmd() >= O_FGCOLOR && m.usCmd() < O_BGCOLOR)
	{
		fgc = carr[m.usCmd() - O_FGCOLOR];
		for (DocList()->First() ; (doc = (Document*)(DocList()->Current())) != 0 ; DocList()->Next())
			oldc = (long)(doc->mle->SetForeground(fgc));

		for (lp=0 ; lp < 16 && carr[lp] != oldc; lp++)
			;
 
		menu->UnCheckItem(lp + O_FGCOLOR);
		menu->CheckItem(m.usCmd());
	}

	// SET BACKGROUND COLOR
	if (m.usCmd() >= O_BGCOLOR && m.usCmd() < O_BGCOLOR + 18)
	{
		bgc = carr[m.usCmd() - O_BGCOLOR];
		for (DocList()->First() ; (doc = (Document*)(DocList()->Current())) != 0 ; DocList()->Next())
			oldc = (long)(doc->mle->SetBackground(bgc));

		for (lp=0 ; lp < 16 && carr[lp] != oldc; lp++)
			;
 
		menu->UnCheckItem(lp + O_BGCOLOR);
		menu->CheckItem(m.usCmd());
	}

	// SET FONT
	if (m.usCmd() == O_FONT)
		Fonts();

	switch (m.usCmd())
	{
		case F_NEW:
			new Document(this);
			break;

		case F_OPEN:
			Open();
			break;

		case F_SAVE:
			top->Save();
			break;

		case F_SAVEAS:
			top->SaveAs();
			break;

		case F_CLOSE:

			if (top->Close(wMessage()))
				delete top;

			break;

		case F_QUIT:
			if (top)
			{
				if (CloseAll())
					PostMessage(wMessage(WM_QUIT, 0, 0));
			}
			break;

		case E_UNDO:
			top->mle->Undo();
			break;

		case E_CUT:
			top->mle->CutSelection();
			break;

		case E_COPY:
			top->mle->CopySelection();
			break;

		case E_PASTE:
			top->mle->PasteSelection();
			break;

		case E_CLEAR:
			top->mle->ClearSelection();
			break;

		case E_FIND:
			{
				ushort	found;

				SearchDlg	s(this, EDIT_SEARCH, &search, &replace, &cs);
				switch(s.GetResult())
				{
					case ID_CANCEL:
						return FALSE;	
					case ID_SEARCH:
						top->mle->DisableUpdate();
						found = top->mle->Search(search, PosFirst, PosLast,
							MlSelectMatch | (cs ? MlCaseSensitive : 0));
						break;
					case ID_REPLACE:				   
						top->mle->DisableUpdate();
						found = top->mle->SearchReplace(search, replace,
							PosFirst, PosLast, (cs ? MlCaseSensitive : 0));
						break;
					case ID_REPLACEALL:
						top->mle->DisableUpdate();
						found = TRUE;
						top->mle->SearchReplace(search, replace,
							PosFirst, PosLast, 
							MlChangeAll | (cs ? MlCaseSensitive : 0));
						break;
				}
				top->mle->EnableUpdate();
				if (!found)
				{
					wMessageBox	mb(this, "Search string not found", "Search",
						MbIconAsterisk | MbOk);
				}
			}
			break;

		case E_SELECTALL:
			top->mle->SetSelection(wRange(0, top->mle->GetNumChars()));
			break;

		case O_TABSTOPS:
			{
				ushort	tmp;
				TabStops	ts(this, &tab);
				if ((tmp = ts.GetResult()) != 0)
				{
					tab = tmp;
					if (top)
					{
						Document	*doc;
	
						for (DocList()->First() ; (doc=(Document*)(DocList()->Current())) != 0 ; DocList()->Next())
							doc->mle->SetTabStop(tmp);
					}
				}
			}
			break;

		case O_WWRAP:
			{
				wwrap = !wwrap;
				for (DocList()->First() ; (doc = (Document*)(DocList()->Current())) != 0 ; DocList()->Next())
					(doc->mle->SetWrap(wwrap));
				menu->ToggleCheckItem(O_WWRAP);
			}
			break;

		default:
			return FALSE;
	}
	return FALSE;
}

long Editor :: Close(wMessage)
{
	return CloseAll();
}

bool Editor :: CloseAll()
{
	for (docList->First() ; docList->Current() ; docList->Next())
		if (!docList->Current()->Close(wMessage()))
			return FALSE;
	return TRUE;
}

bool Editor :: Open()
{
	bStringList			*sl;
	wStdFileOpenDialog	fdlg;
	Document			*doc;
	wPointl				pos(5,5);

	fdlg.ChangeStyle(FdaCenter | FdaHelpButton | FdaMultipleSel);
	fdlg.SetInitialDrive('C');
	if (fdlg.CreateWindow(this) == DidOK)
	{
		sl = fdlg.GetFilesChosen();
		for (sl->First() ; sl->Current() ; sl->Next())
		{
			doc = new Document(this, *(sl->Current()));
			doc->ChangePosition(pos);
			pos += wPointl(10,10);
		}
		delete sl;
		return TRUE;
	}
	return FALSE;
}

bool Editor :: Fonts()
{
	wStdFontDialog	fdlg;
	Document		*doc;
	wPresSpace		ps(this);

	fdlg.ChangeStyle(FnaCenter | FnaBitmapOnly);
	fdlg.SetInitialFont(font);
	fdlg.SetPreviewString("ABCDEabcde");
	fdlg.SetScreenPresSpace(&ps);
	if (fdlg.CreateWindow(this) == DidOK)
	{
		delete font;

		font = fdlg.GetFont();

		for (DocList()->First() ; (doc = (Document*)(DocList()->Current())) != 0 ; DocList()->Next())
			(doc->mle->SetFont(font));
		return TRUE;
	}
	return FALSE;
}

static	short unnamed;

Document :: Document(wWindow *parent) : wMdiDocument(MDI_MAXTITLEAUTO)
{
	char 		buf[80];
	wDimension 	d=parent->GetSize();

	CreateWindow(parent, FaTitleBar | FaSysMenu | FaMinMax | FaSizeBorder);
	mle = 0;

	ChangeFrameSize(wDimension(d.xWidth() / 3, d.yHeight() / 3));
	ChangePosition(wPointl(5,5));
	mle = new wMLEntry(this, 0, wPointl(0,0), GetSize(), MlHscroll | MlVscroll);

	SetIcon(icon = new wIcon(ResIcon, I_OBJPM));

	sprintf(buf, "Untitled #%d", ++unnamed);
	SetCaption(buf);
	fname = buf;
	saveas = TRUE;

	mle->SetForeground(((Editor*)parent)->ForeG());
	mle->SetBackground(((Editor*)parent)->BackG());

	mle->SetFont( ((Editor*)parent)->Font() );
	mle->SetTabStop(((Editor*)parent)->Tab());
	mle->SetWrap(((Editor*)parent)->WordWrap());

	mle->SetFirstChar(0);		// REPOSITION TO FIRST CHARACTER
	mle->SetCursorPosition(0);	// PLACE CURSOR
	mle->EnableUpdate();
	mle->Show();				// SHOW IT
	mle->SetFocus();			// GIVE IT FOCUS SO CURSOR APPEARS
	mle->SetModified(FALSE);


	Show();
}

Document :: Document(wWindow *parent, const char *fn) : wMdiDocument(MDI_MAXTITLEAUTO)
{
	short		err;
	File		f(fn);
	bFileName	nm;
	wDimension 	d=parent->GetSize();

	CreateWindow(parent, FaTitleBar | FaSysMenu | FaMinMax | FaSizeBorder);
	mle = 0;

	SetIcon(icon = new wIcon(ResIcon, I_OBJPM));   

	nm.Parse(fn);
	SetCaption(nm.Filename());

	HourGlass();

	modified = FALSE;
	saveas = FALSE;
	fname = fn;
	ChangeFrameSize(wDimension(d.xWidth() / 3, d.yHeight() / 3));
	ChangePosition(wPointl(5,5));
	mle = new wMLEntry(this, 0, wPointl(0,0), GetSize(), MlHscroll | MlVscroll);	

	if ((err = f.Open("r")) == 0)
	{
		ushort	numb;
		long	pos = 0;
		bString str(NULL, 4096);

		while ((numb = (ushort)f.Read((char *)(const char *)str, 1, 4096)) != 0)
		{
			str.AdjustLength(numb);
			mle->Import(str, &pos, MlCfText);
		}
		f.Close();
	}
	else
	{
		char	buf[300];
		sprintf(buf, "Error %d opening %s", err, fn);
		wMessageBox	mb(this, buf, "Error", MbIconExclamation | MbOk);
	}

	HourGlass();

	mle->SetForeground(((Editor*)parent)->ForeG());
	mle->SetBackground(((Editor*)parent)->BackG());

	mle->SetFont( ((Editor*)parent)->Font() );
	mle->SetTabStop(((Editor*)parent)->Tab());
	mle->SetWrap(((Editor*)parent)->WordWrap());

	mle->SetFirstChar(0);		// REPOSITION TO FIRST CHARACTER
	mle->SetCursorPosition(0);	// PLACE CURSOR
	mle->EnableUpdate();
	mle->Show();				// SHOW IT
	mle->SetFocus();			// GIVE IT FOCUS SO CURSOR APPEARS
	mle->SetModified(FALSE);
	Show();
}

Document :: ~Document()
{
	delete mle;
	delete icon;
}

long Document :: Size(wSizeMsg m)
{
	// RESIZE THE MLE TO FILL THE WINDOW
	if (mle)
		mle->ChangeSize(wDimension(m.cxNew(), m.cyNew()));
	return FALSE;
}

long Document :: Close(wMessage)
{
	if ((modified = modified | mle->GetModified()) != 0)
	{
		char	buf[300];

		sprintf(buf, "Do you want to save %s before closing?", (const char*)fname);
		wMessageBox	mb(this, buf, "Close", MbIconQuestion | MbYesNoCancel);
		if (mb.MessBoxVal() == MrYes)
			return Save();
		return (mb.MessBoxVal() == MrNo);
	}
	return TRUE;
}

long Document :: Activate(wActivateMsg m)
{
	char	buf[500], *cp;

	if (m.fActive())
	{
		ushort	len;
		wMdiDesktop *	desk = (wMdiDesktop *)ParentWindow();

		bString str = desk->GetCaption();
		strcpy(buf, str);
		if ((cp = strchr(buf, '-')) != NULL)
			*(cp-1) = 0;

		strcat(buf, " - ");
		strcat(buf, GetCaption());
		desk->SetCaption(buf);
	}
	return FALSE;
}

bool Document :: Save()
{
	short	err, ret;

	if (saveas)
		return SaveAs();

	HourGlass();

	File f(fname);
	if ((err = f.Open("w+")) == 0)
	{
		long		loc = 0;
		bString 	str(NULL, 4096);

		while (loc < mle->GetNumChars())
		{
			mle->Export(str, &loc, MlCfText);
			f.Write(str, str.Length(), 1);
		}
		f.Close();
		mle->SetModified(FALSE);
		modified = FALSE;
		ret = TRUE;
	}
	else
	{
		char	buf[300];
		sprintf(buf, "Error %d saving %s", err, (const char*)fname);
		wMessageBox	mb(this, buf, "Error", MbIconExclamation | MbOk);
		ret = FALSE;
	}			 
	HourGlass();

	return ret;
}

bool Document :: SaveAs()
{
	wStdFileSaveAsDialog fdlg;
	char 	*fn;
	bString	oldfname = fname;
	bool	oldsaveas = saveas;

	fdlg.ChangeStyle(FdaCenter | FdaHelpButton);
	fdlg.SetInitialDrive('C');
	if (fdlg.CreateWindow(this) == DidOK)
	{
		fname = fdlg.GetFirstFile();
		saveas = FALSE;
		if (Save())
		{
			SetCaption(fname);
			Activate((wActivateMsg&)wMessage(WM_ACTIVATE, (MPARAM)TRUE, (MPARAM)hWindow()));
			return TRUE;
		}
		else
		{
			fname = oldfname;
			saveas = oldsaveas;
			return FALSE;
		}
	}
	return FALSE;
}

long TabStops :: InitDlg(wInitDlgMsg m)
{
 	spin = new wSpinButton(this, TABSTOPS);
	spin->SetRange(wRange(1,200));
	spin->SetValue( (long)(*((ushort*)(m.pCreateParams()))) );
	spin->SetFocus();
	ChangePosition(PosCenter, OwnerWindow());
	return FALSE;
}

long TabStops :: PushButtonCommand(wCommandMsg m)
{
	if (m.usCmd() == TAB_OK)
	{
		ushort tmp = (ushort)spin->GetValue();
		if (tmp > 200)
			tmp = 200;
		if (tmp < 1)
			tmp = 1;
		Dismiss(tmp);
	}
	if (m.usCmd() == TAB_CANCEL)
		Dismiss(0);

	return FALSE;
}

long SearchDlg :: InitDlg(wInitDlgMsg)
{
	srch = new wEntry(this, SEARCH);
	repl = new wEntry(this, REPLACE);
	cs = new wCheckBox(this, CASE);
	srch->SetText(*search);
	repl->SetText(*replace);

	// SELECT ALL CURRENT FIND TEXT
	if (!search->IsBlank())
		srch->SetSelection(wRange(0, search->Length()));

	cs->SetCheck(*sens);		// SET CHECKBOX TO PREVIOUS STATE
	ChangePosition(PosCenter, OwnerWindow());	// CENTER DIALOG WINDOW IN PARENT
	ToTop();

	srch->SetFocus();	// SET FOCUS TO THE ENTRY FIELD
	return TRUE;
}

long SearchDlg :: PushButtonCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case ID_SEARCH:
			*search = srch->GetText();  	// STORE SEARCH STRING
			*sens = cs->CheckState();	// STORE CASE SENSITIVE CHECKBOX STATE
			Dismiss(ID_SEARCH);			// DISMISS THE DIALOG WINDOW
			return TRUE;

		case ID_REPLACE:
		case ID_REPLACEALL:
			*search = srch->GetText();		// STORE SEARCH STRING
			*replace = repl->GetText();	// STORE REPLACE STRING
			*sens = cs->CheckState();		// STORE CASE SENSITIVE CHECKBOX STATE
			Dismiss(m.usCmd());				// DISMISS THE DIALOG WINDOW
			return TRUE;


		case ID_CANCEL:
			Dismiss(ID_CANCEL);
			return TRUE;
	}
	return FALSE;
}


/*  VCS_ID
 *  $Filename:   mdiedit.hpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.4  $
 *  $Date:   06 Jan 1992 00:14:28  $
 */
 
#define InclGraphics
#define InclMisc
#define InclOS2Objects
#define InclFiles
#define InclMdi
#define InclProfiles
#define InclStdDialogs
#include <ObjectPM.hpp>
#include "mdidlg.h"

#define F_MENU		100
#define F_NEW		101
#define F_OPEN		102
#define F_SAVE		103
#define F_SAVEAS	104
#define F_CLOSE		105
#define F_QUIT		106

#define E_MENU		200
#define E_UNDO		201
#define E_CUT		202
#define E_COPY		203
#define E_PASTE		204
#define E_CLEAR		205
#define E_FIND		206
#define E_SELECTALL	207

#define O_MENU		300
#define O_FOREG		301
#define O_BACKG		302
#define O_FONT		303
#define O_TABSTOPS	304
#define O_FGCOLOR	305
#define O_BGCOLOR	323
#define O_FONTEND	359
#define O_WWRAP		360

#define W_MENU		400
#define W_TILE		401
#define W_CASCADE	402
#define W_FIRST		403

class Editor : public wMdiDesktop
{
	private:
		wMenu *		menu;
		wAccelTable *	accel;
		wIcon *		icon;
		wColor		fgc, bgc;
		wFont		*font;
		ushort		tab;
		bString 	search, replace;
		short		cs;
		bool		wwrap;

		void    Paint() { WindowPS()->Erase(); }
		long    MenuCommand(wCommandMsg m);
		long    MenuAction(wMenuActionMsg m);
		long    Close(wMessage);
		bool	CloseAll();
		bool	Open();
		bool	Fonts();

	public:
		Editor();
		~Editor();

		wColor	ForeG() { return fgc; }
		wColor	BackG() { return bgc; }
		wFont	*Font() { return font; }
		ushort	Tab() { return tab; }
		bool WordWrap() { return wwrap; }
};

class Document : public wMdiDocument
{
	private:
		short	modified,
				saveas;
		String	fname;
		wIcon *	icon;
		long 	Size(wSizeMsg m);
		long 	Activate(wActivateMsg);

	public:
		wMLEntry	*mle;

		Document(wWindow *parent);
		Document(wWindow *parent, const char *fn);
		~Document();

		bool Save();
		bool SaveAs();
		long Close(wMessage);
};


class TabStops : public wDialogWindow
{
	private:
		wSpinButton *	spin;
	public:
		TabStops(wWindow *parent, void *v) : 
			wDialogWindow(OPT_TABS) { CreateWindow(parent, v); }
		~TabStops() { delete spin; }
		long InitDlg(wInitDlgMsg m);
		long PushButtonCommand(wCommandMsg m);
};

class SearchDlg : public wDialogWindow
{
	private:
		wEntry		*srch, *repl;
		wCheckBox	*cs;
		bString		*search,  *replace ;
		short		*sens;
	public:
		SearchDlg(wWindow *parent, ushort id, bString *s, bString *r, short *c) : 
			wDialogWindow(id, DlgModal) 
			{ search=s; replace=r; sens=c; CreateWindow(parent); }
  		long InitDlg(wInitDlgMsg);
		long PushButtonCommand(wCommandMsg);
};


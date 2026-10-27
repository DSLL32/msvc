/*  VCS_ID
 *  $Filename:   formdemo.hpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:20:34  $
 */

// Hello application class definitions....

#include "formdemo.h"

// window & page id's

#define IdNotebook		101
#define IdStringPage  	0
#define IdCharPage		1
#define IdMLEStringPage	2
#define IdDatePage		3
#define IdTimePage		4
#define IdShortPage		5
#define IdLongPage		6
#define IdFloatPage		7
#define IdMoneyPage		8
#define IdButtonPage	9
#define IdListPage		10
#define IdCntrPage		11

#define FORM_WINDOWS 12

#define IdMajorSections 30
#define IdCharTypes		IdMajorSections + 0
#define IdIntegers      IdMajorSections + 1


class FormData
{
	public:
		FormData();

		bString string;
		bString numString;
		bString letString;
		bString mleString;
		char 	uprString[33];
		char 	lwrString[33];
		bDate	longDate;
		bDate	shortDate;
		bDate	nlsDate;
		bDate	stringDate;
		bTime	f24Time;
		bTime	f12Time;
		bTime	nlsTime;
		bTimeStamp timestamp;
		ushort  shortUInt;
		ushort  commaUInt;
		ushort  zpadUInt;
		ushort  blankUInt;
		short   signedInt;
		short   ssignedInt;
		ushort  base36UInt;
		ushort  octalUInt;
		ushort  binaryUInt;
		ushort  hexUInt;
		ulong   ULong;
		ulong   commaULong;
		ulong   zpadULong;
		ulong   blankULong;
		long   	signedLong;
		long   	ssignedLong;
		ulong  	base36ULong;
		ulong  	octalULong;
		ulong  	binaryULong;
		ulong  	hexULong;
		double  dbl;
		double  fixedPrec;
		double 	sdbl;
		double  commaDbl;
		double  expNot;
		bMoney  amt;
		bMoney	indMoney;
		bMoney  showMoney;
		bMoney  commaMoney;
		bMoney	parenMoney;
		char	c;
		char	n;
		char	u;
		char	yn;
		char	tf;
		short   radBtn;
		short	foundChk;
		short	graphChk;
		short	ksinkChk;
};

class FormNoteBook : public wNoteBook
{
	public:
		FormNoteBook(wWindow *parent, ushort id, wPointl p, wDimension d, ulong style)
					: wNoteBook(parent, id, p, d, style)	{}

		void PageSelected(wNBPageMsg m);
};

class StringForm : public wFormWindow
{
	public:
		StringForm(wNoteBookPageList *pg, FormData *);
};

class CharForm : public wFormWindow
{
	public:
		CharForm(wNoteBookPageList *pg, FormData *);
};

class MLEStringForm : public wFormWindow
{
	public:
		MLEStringForm(wNoteBookPageList *pg, FormData *);
};

class DateForm : public wFormWindow
{
	public:
		DateForm(wNoteBookPageList *pg, FormData *);
};

class TimeForm : public wFormWindow
{
	public:
		TimeForm(wNoteBookPageList *pg, FormData *);
};

class ShortForm : public wFormWindow
{
	public:
		ShortForm(wNoteBookPageList *pg, FormData *);
};

class LongForm : public wFormWindow
{
	public:
		LongForm(wNoteBookPageList *pg, FormData *);
};

class FloatForm : public wFormWindow
{
	public:
		FloatForm(wNoteBookPageList *pg, FormData *);
};

class MoneyForm : public wFormWindow
{
	public:
		MoneyForm(wNoteBookPageList *pg, FormData *);
		short Init();
};

class MoreForm;
class UserInfo
{
	private:
		friend class MoreForm;
		bString name, company, phone;

	public:
		UserInfo()		{}
		UserInfo(char *n, char *c, char *p) : name(n), company(c), phone(p) {}

		// ...
};

class ListForm : public wFormWindow
{
	private:
		ulong partno;
		bString desc;
		bMoney amt, total;

	public:
		ListForm(wNoteBookPageList *pg, FormData *);
		short ProcItem(wListRegion *regp, char *itemtext, char **prevdata, short cmd);
		short Init();
};

class MoreForm : public wFormWindow
{
	private:
		UserInfo* ip;

	public:
		MoreForm(UserInfo *);
		short Init();
};


class ButtonForm : public wFormWindow
{
	private:
		UserInfo info;

	public:
		ButtonForm(wNoteBookPageList *pg, FormData *);

		short ProcUndo(short);
		short DoMore(short);
};


class AppWindow : public wFrameWindow
{
	private:
		wMenu *menubar;
		wIcon *icon;
		FormNoteBook *np;
		wFormWindow *forms[FORM_WINDOWS];
		FormData *data;
		wHelpManager *help;

	public:
		AppWindow();
		~AppWindow();

		long Close(wMessage);
		void Paint();
		long MenuCommand(wCommandMsg);
		wFormWindow **GetDataForms()	{ return forms; }
};





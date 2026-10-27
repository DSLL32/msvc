/*  VCS_ID
 *  $Filename:   controls.hpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:20:34  $
 */
 

#include "controls.h"
#include "about.h"

class AppWindow : public wFrameWindow
{
	private:
		wIcon *icon;
		wPageWindow *topPage;

	public:
		AppWindow();
		~AppWindow();
		long MenuCommand(wCommandMsg);
		long Close(wMessage);
		void Paint();
};

class ControlDemoPage : public wPageWindow
{
	private:
		wMLEntry *pmle;
		wPushButton *pclr;
		char *demoName;

	public:
		ControlDemoPage(char *name, char *demoName);
		short PageIn();
		short PageOut();
		long  ProcClearMLE(wMessage);
		long  ExitDemo(wMessage);
		void  Paint();
		virtual void PaintStats(bool = FALSE);
		void  AddNotifierMessage(char *msg);
};

class DemoListBox;

class ListBoxPage : public ControlDemoPage
{
	private:
		DemoListBox *lb;
		ushort nextitem;

	public:
		ListBoxPage(char *name);
		short PageIn();
		short PageOut();
		long  MenuAction(wMenuActionMsg m);
		long  MenuCommand(wCommandMsg);
		long  AddItem(wCommandMsg m);
		long  DeleteItem(wCommandMsg m);
		void  PaintStats(bool fPSGiven = FALSE);
};

class DemoListBox : public wListBox
{
	private:
		ControlDemoPage *demopg;

	public:
		DemoListBox(ControlDemoPage *pg, ushort id);

		void Select(wControlMsg);
		void Enter(wControlMsg);
		void FocusGained(wControlMsg);
		void FocusLost(wControlMsg);
		void Scroll(wControlMsg);
};


class ButtonPage : public ControlDemoPage
{
	private:
		wPushButton *buttons[4];
		wRadioButton *radios[3];
		wCheckBox *checkBox;
		wGroupBox *groupBox;
		wIcon *icons[3];

	public:
		ButtonPage(char *name);
		short PageIn();
		short PageOut();
		long  Control(wControlMsg);
		long  PushButtonCommand(wCommandMsg);
};

class DemoSlider : public wSlider
{
	private:
		ControlDemoPage *demopg;
	
	public:
		DemoSlider(ControlDemoPage *pg, ushort id, wPointl p, wDimension d, ulong style,
			ushort Increments1, ushort Spacing1 = 0, ushort Increments2 = 0, ushort Spacing2 = 0);

		void Changed(wSliderPositionMsg);
		void FocusGained(wControlMsg);
		void FocusLost(wControlMsg);
		void SliderDrag(wSliderPositionMsg);
};
		

class SliderPage : public ControlDemoPage
{
	private:
		DemoSlider *slider;

		void SetupSlider();

	public:
		SliderPage(char *name);
		short PageIn();
		short PageOut();
//		long  ProcNotifiers(wControlMsg);
};

class EntryPage : public ControlDemoPage
{
	private:
		wEntry *entry;
		wMLEntry *mle;

	public:
		EntryPage(char *name);
		short PageIn();
		short PageOut();
		long  MenuAction(wMenuActionMsg m);
		long  MenuCommand(wCommandMsg);
		long  ProcNotifiers(wControlMsg);
};

class ValueSetPage : public ControlDemoPage
{
	private:
		wValueSet *color;

	public:
		ValueSetPage(char *name);
		short PageIn();
		short PageOut();
		long  ProcNotifiers(wControlMsg);
};

class SpinPage : public ControlDemoPage
{
	private:
		wSpinButton *aspin, *nspin, *master, *slave;
		wStaticControl *atitle, *ntitle, *mstitle;

	public:
		SpinPage(char *name);
		short PageIn();
		short PageOut();
		long  ProcNotifiers(wControlMsg);
};

class CellBoxPage : public ControlDemoPage
{
	private:
		wCellBox *cb;
		wBitmap *bmp, *bmp1;
		ushort    nextitem;

	public:
		CellBoxPage(char *name);
		short PageIn();
		short PageOut();
		long  ProcNotifiers(wControlMsg);
		long  AddItem(wCommandMsg m);
		long  DeleteItem(wCommandMsg m);
		void  PaintStats(bool fPSGiven = FALSE);
		long  MenuAction(wMenuActionMsg m);
		long  MenuCommand(wCommandMsg);
};

class AboutDialog : public wDialogWindow
{
	public:
		AboutDialog(wWindow *owner) : wDialogWindow (D_ABOUT) { CreateWindow(owner); }
		long InitDlg(wInitDlgMsg)               { ChangePosition(PosCenter, OwnerWindow()); return FALSE; }
		long PushButtonCommand(wCommandMsg)     { Dismiss(); return FALSE; }
};

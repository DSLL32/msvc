/*  VCS_ID
 *  $Filename:   graph.hxx
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:18:28  $
 */
 
#include "about.h"
#include "graph.h"

class OptionsDialog;

// Top level window class for graph class

class GraphWindow : public wFrameWindow
{
	private:
		wIcon *icon;
		wMenu *menubar;

		wFont *scFonts[2];
		wFont **fonts;
		wWindowDevice *winDC;
		short aRotation;
		long xGraphPos;
		long yGraphPos;
		double scale;
		wSubPicture *bracket;

		void Paint();
		void PrepPS();
		void DestroyPS();
		void CreateFonts(wFont **);
		void DrawBracket(wDimension siz);
		void PrintGraph();
		long MenuCommand(wCommandMsg);
		wPrinterSetup *prSetup;
		OptionsDialog *optionsDialog;
		friend class OptionsDialog;

	public:
		GraphWindow();
		~GraphWindow();
};

// Options Dialog

class OptionsDialog : public wFormWindow
{
	private:
		short Init();

	public:
		OptionsDialog(GraphWindow *win);
};

class AboutDialog : public wDialogWindow
{
	public:
		AboutDialog(wWindow *owner) : wDialogWindow (D_ABOUT) { CreateWindow(owner); }
		long InitDlg(wInitDlgMsg)               { ChangePosition(PosCenter, OwnerWindow()); return FALSE; }
		long PushButtonCommand(wCommandMsg)     { Dismiss(); return FALSE; }
};


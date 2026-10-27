/*  VCS_ID
 *  $Filename:   puzzle.hpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:44:02  $
 */
 
#include "about.h"
#include "puzzle.h"

// Top level window class for graph class

class Tile : public wPicture
{
	private:
		short n;

	public:
		Tile(wPresSpace *ps, short tileNo) : 
			wPicture(ps, FALSE, FALSE) { n = tileNo; } 
		short TileNumber() { return n; }
};
				   
class Puzzle
{
	private:
		wSubPicture *tilePic;
		short blankTile;
		short targets[5];
		POINTL tilePos[16];
		wDimension tileSize;
		wPresSpace *ps;
		wFont *font;
		bool jumbled;

		void CalcTargets();

	public:
		Puzzle(wPresSpace *_ps);
		void  MoveTiles(short tileFrom, short tileTo, bool fDraw = TRUE);
		short BlankTile() { return blankTile; }
		wDimension& TileSize() { return tileSize; }
		short HitTest(wPointl p);
		bool  IsTarget(short n);
		bool  IsSolved();
		void  Jumble();
		void  Solve();
		wPresSpace *pPS() { return ps; }
};


class PuzzleFrame : public wFrameWindow
{
	private:
		wIcon *icon;
		wMenu *menubar;
		Puzzle *puz;

		void PrepPictures();
		void Paint();
		long MenuCommand(wCommandMsg);
		long ButtonClick(wButtonClickMsg);
		long Size(wSizeMsg);

	public:
		PuzzleFrame();
		~PuzzleFrame();
};

class AboutDialog : public wDialogWindow
{
	public:
		AboutDialog(wWindow *owner) : 
			wDialogWindow(D_ABOUT) { CreateWindow(owner); }
		long InitDlg(wInitDlgMsg)               { ChangePosition(PosCenter, OwnerWindow()); return FALSE; }
		long PushButtonCommand(wCommandMsg)     { Dismiss(); return FALSE; }
};


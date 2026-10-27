/*  VCS_ID
 *  $Filename:   puzzle.cpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:43:58  $
 */
 
#define InclGraphics
#define InclFixedMath
#include <ObjectPM.hpp>
#include "puzzle.hpp"	// include the class definitions

extern "C"
{
	#include <stdlib.h>
	#include <time.h>
}

short bMainThread :: Start()		// Program execution starts here
{	  
	PuzzleFrame w;	    // Contstruct the top level frame window
	Exec();				// Go to the message loop.

	return 0;
}

PuzzleFrame :: PuzzleFrame()
{												   
	CreateWindow(FaTaskList | FaSysMenu | FaMinMax | FaTitleBar | FaSizeBorder);
	SetCaption("Puzzle");					   	   // Set captions
	SetSwitchTitle("ObjectPM Puzzle");
	SetIcon(icon = new wIcon(ResIcon, I_OBJPM));   // Set the app's icon

	// This section create a menu bar and then adds items to the 
	// sub menu itself.  It is also possible to construct the menu
	// from the resource file if desired

	menubar = new wMenu(this, MB_ACTION, "~Action\\~Exit\\");
	menubar->SetSubMenuItems(MB_ACTION, MI_JUMBLE, "~Jumble;~Solve");
	menubar->SetSubMenuItems(MB_EXIT, MI_EXIT, "Exit\tAlt+F4;Continue;");

	// Create a temporary system menu object for the purposes of adding
	// a seperator and the "About..." menu item.

	wSysMenu sm(this);
	sm.AppendSeparator();
	sm.AppendItem(SC_ABOUT, "~About...");

	// Create the presentation space and then the puzzle itself...

    WindowPS() = new wPresSpace(this, PuLoMetric, PcRetain); 
	puz = new Puzzle(WindowPS());

	// Size the frame window based on the puzzle size...

	wDimension winSiz = puz->TileSize();
	winSiz.xWidth() *= 4;
	winSiz.xWidth() += 20;
	winSiz.yHeight() *= 4;
	winSiz.yHeight() += 20;
	ChangeFrameSize(CalcFrameSize(winSiz));
	ChangePosition(PosCenter);
	Show();							   // Make the frame visible
	ToTop();
}

PuzzleFrame :: ~PuzzleFrame()
{				   
	delete icon;
	delete menubar;
	delete WindowPS();
	delete puz;
}

// The MenuCommand member is sent a Command message each time
// an item on the menu bar is selected.

long PuzzleFrame :: MenuCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case MI_JUMBLE:
			HourGlass();
			puz->Jumble();
			HourGlass();
			break;

		case MI_SOLVE:
			puz->Solve();
			break;

		case MI_EXIT:
			CurrentThread->Quit();
			break;

		case SC_ABOUT:
			{
				AboutDialog aboutWin(this);		 // Create the "about" box
				return FALSE;
			}
	}
	RePaint();		// invalidate the client window 

	return FALSE;
}



long PuzzleFrame :: ButtonClick(wButtonClickMsg m)
{
	short n;

	if (m.usButtonNum() == 1 && m.usActivity() == BUTTONDOWN)
	{
		ToTop();
		wPointl p (m.x(), m.y());
		n = puz->HitTest(p);
		if (puz->IsTarget(n))
		{
			puz->MoveTiles(n, puz->BlankTile());
			if (puz->IsSolved())
				wMessageBox(this,"You have solved the puzzle.","Congratulations",MbEnter | MbIconExclamation | MbApplModal);								
		}
			
	}

	return TRUE;
}

// Scale the puzzle when the frame window is resized
// The factor of 4 is required to account for the 4 tiles across and down.
// Note that the wPresSpace Reset method does not reset the page scale.

long PuzzleFrame :: Size(wSizeMsg msg)
{
	wDimension d = puz->TileSize();	
	puz->pPS()->SetPageScale( (double)msg.cxNew() / (4 * d.xWidth()), (double)msg.cyNew () / (4 * d.yHeight()));
	return TRUE;
}

void PuzzleFrame :: Paint()
{
	WindowPS()->Reset();
	WindowPS()->Erase();
	(WindowPS()->PresPictures())->DrawPictures();	
}

Puzzle :: Puzzle(wPresSpace *_ps)
{
	short x, y, z;
	long xPos, yPos;

	ps = _ps;
	jumbled = FALSE;

	// Obtain the size of the square tiles in pels and then
	// change the PS to use PELS instead of LOMETRIC units

	tileSize = wDimension(125, 125);
	ps->ConvertToPels((wPointl *)&tileSize);
	ps->ChangePageUnits(PuPels);
	
	for (y = 0, yPos = 0; y < 4; y++, yPos += tileSize.yHeight())
	{
		for (z = x = (12 - y * 4), xPos = 0; x < (z + 4); x++, xPos += tileSize.xWidth())
		{
			tilePos[x].x = xPos;
			tilePos[x].y = yPos;
		}
	}

	// Create the drawing tools to use

	wFontDefList *flist = ps->EnumFonts("Times New Roman Bold");
	if (flist->First())
		font = new wFont(flist->Current(), ps);
	
	wPen pen(ps);
	wTextPen textp(ps);
	wFillPattern fillp(ps);
	fillp.SetColor(ClrPaleGray);

	// Construct the tile subpicture.  This object
	// will be called by each of the 15 individual tiles

	tilePic = new wSubPicture(ps);
	pen.PenDown();
	textp.PenDown(font);
	fillp.SetAsCurrent();

	pen.MoveTo(0, 0);
	pen.Box(wPointl(tileSize.xWidth() - 1, tileSize.yHeight() - 1), DoOutlineFill);

	// Set the size of the characters for the remainder of 
	// calling wPicture object

	font->SetCharacterBoxSize(wDimension(tileSize.xWidth() * .7, tileSize.yHeight() * .7));

	// close the sub picture
	tilePic->Close();

	// Re-set the character box to be the same as it will be 
	// in the wSubPicture just drawn (For centering)

	font->SetCharacterBoxSize(wDimension(tileSize.xWidth() * .7, tileSize.yHeight() * .7));


	// Construct the 16 tile Pictures.
  
	{
		wPointl *tilePts = (wPointl *)tilePos;
		short i;
		Tile *ptile;
		wRectl r;
		char cp[3];

		for (i = 0; i < 16; i++, tilePts++)
		{
			// Compute the text size for centering
			// TextSize must be called outside the wPicture, i.e., the segment
			// to work
			itoa(i + 1, cp, 10);
		   	wDimension dim = textp.TextSize(cp);

			ptile = new Tile(ps, i);

			// Set the position of the new tile by 
			// using a wPicture (segment) transformation

			ptile->SetTransl(*tilePts);

			if (i < 15)	  // draw a normal tile with centered text
			{
				tilePic->Draw();
				textp.Display( (long)((tileSize.xWidth() - dim.xWidth() ) / 2), (long)(((tileSize.yHeight() - dim.yHeight() ) / 2) + 4) , cp);					
			}															 
			else	 	  // draw the blank tile	
			{
				fillp.SetColor(ClrWhite);
				pen.MoveTo(0, 0);
				pen.Box(wPointl(tileSize.xWidth() - 1, tileSize.yHeight() - 1), DoFill);
				blankTile = i;
			}
			ptile->Close();
			ptile->SetAttribute(AttrFastChain, FALSE);
		}
	}
	CalcTargets();
}

// given the blank tile at some position, this method determines
// the tiles (by position) that can move into the blank tile's space.
// This method must be called each time the position of the blank tile
// changes

void Puzzle :: CalcTargets()
{
	short i = 0;

	if (blankTile % 4)						// move from the left?
		targets[i++] = blankTile - 1;

	if ((blankTile + 1) % 4)				// move from the right?
		targets[i++] = blankTile + 1;

	if (blankTile > 3)						// move from the top?
		targets[i++] = blankTile - 4;		

	if (blankTile < 12)						// move from the bottom?
		targets[i++] = blankTile + 4;		

	targets[i] = -1;  // terminate the list
}

void Puzzle :: MoveTiles(short tileFrom, short tileTo, bool fDraw)
{
	Tile *tFrom, *tTo, *tNextTo;
	short fSecondMove = TRUE;
	wPictureList *plist = ps->PresPictures();

	if (tileFrom == tileTo)
		return;

	// check for moving adjacent tiles.  If so, only one "move"
	// operation is needed to switch them

	if (abs(tileFrom - tileTo) == 1)
		fSecondMove = FALSE;

	tFrom = (Tile *)((*plist)[tileFrom]);
	tTo   = (Tile *)((*plist)[tileTo]);
	plist->Find(tTo);		// set the iterator

	if (tileTo != 15)		// is the "to" tile the last in the list?
	{
		plist->Next();
		tNextTo = (Tile *)(plist->Current());
		plist->Prev();
	}
	else tNextTo = NULL; 	// yes

	// arrange, adjust position, and  draw the 'to' tile

	plist->Arrange(tFrom, NULL, (tileFrom == tileTo + 1)? RelPosAfter: RelPosBefore);
	tTo->SetTransl(((wPointl *)tilePos)[tileFrom]);
	if (fDraw)
		tTo->Draw();

	// set the current list position to the 'from' tile and move it

	if (fSecondMove)
	{
		plist->Next();
		if (tNextTo)
			plist->Arrange(tNextTo, NULL, RelPosBefore);
		else
			plist->ArrangeToTail(NULL);
	}

	// adjust position, and  draw the 'from' tile

	tFrom->SetTransl(((wPointl *)tilePos)[tileTo]);
	if (fDraw)
		tFrom->Draw();

	// if the 'to' tile was the current blank tile, then the 'from'
	// tile is now the blank one.  We need to re-set it, and calculate
	// the new target positions...

	if (tileTo == blankTile)
	{
		blankTile = tileFrom;
		CalcTargets();
	}
}

// This method returns the tile that contains the point
// or -1 if the point is not over any of the tiles

short Puzzle :: HitTest(wPointl p)
{
	wPointl *tilePts = (wPointl *)tilePos;
	short i;
	wRectl r;

	// Translate the device coodinates back to pre-scaled page coodinates
	// in order to perform the hit test

    ps->Convert(&p, 1, DeviceSpace, DefaultPage);

	for (i = 0; i < 16; i++, tilePts++)
	{
		r = wRectl(*tilePts, tileSize);
		if (r.PtInRectangle(p))
			return i;
	}
	return -1;
}

bool Puzzle :: IsTarget(short n)
{
	short i;

	for (i = 0; targets[i] >= 0; i++)
	{
		if (targets[i] == n)
			return TRUE;
	}
	return FALSE;
}

void Puzzle :: Jumble()
{
	time_t t;
	short i, a, b;
	
	ps->Reset();
	srand((unsigned)time(&t));
	for (i = 0; i < 200; i++)
	{
		a = rand() % 16;
		b = rand() % 16;
		if (a != blankTile)
			MoveTiles(a, b, FALSE);
	}
	jumbled = TRUE;
}

void Puzzle :: Solve()
{
	wPictureList *plist = ps->PresPictures();
	short i, t;

	for (i = 0; i < 15; i++)
	{
		while((t = ((Tile *)((*plist)[i]))->TileNumber()) != i)
			MoveTiles(i, t, FALSE);	
	}
	blankTile = 15;
	jumbled = FALSE;
	CalcTargets();
}


// This method tests to see if the puzzle was really solved.
// The puzzle must have been previously jumbled to be considered solved.

bool Puzzle :: IsSolved()
{
	wPictureList *plist = ps->PresPictures();
	short i;

	if (jumbled == FALSE)
		return FALSE;

	if (blankTile == 15)
	{
		for (i = 0; i < 15; i++)
		{
			if ( ((Tile *)((*plist)[i]))->TileNumber() != i)
				return FALSE;           
		}
		jumbled = FALSE;
		return TRUE;			
	}
	else
		return FALSE;		
}

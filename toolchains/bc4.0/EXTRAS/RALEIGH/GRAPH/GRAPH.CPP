/*  VCS_ID
 *  $Filename:   graph.cxx
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:18:26  $
 */
 
// The Graph Demo Program
// Written by John Pompeii, 1991
//
// The purpose of this program is to demonstrate the ObjectPM
// drawing tools, as well as demonstrate the techniques required
// build WYSIWYG programs.  The graph is rendered using wPen, wTextPen, 
// wFillPattern, and outline wFont objects.  By not using any of the 
// standard bitmap fonts, it is possible to accurately reproduce the
// drawing on all of OS/2's output devices.


extern "C"
{
	#include <stdio.h>
	#include <stdlib.h>
}

#define InclPrinters
#define InclGraphics
#define InclFixedMath
#define InclProfiles
#define InclForms


#include <ObjectPM.hpp>
#include "graph.hpp"
#include "printdlg.hpp"


short bMainThread :: Start()		// Program execution starts here
{
	GraphWindow w;	    		// Contstruct the top level frame window
	Exec();						// Go to the message loop.

	return 0;
}

GraphWindow :: GraphWindow()
{
	CreateWindow(OpmStdFrame);					   // Create the frame
	SetCaption("Graph");					   	   // Set captions
	SetSwitchTitle("Graph Demo Program");
	SetIcon(icon = new wIcon(ResIcon, I_OBJPM));   // Set the app's icon

	// This section create a menu bar and then adds items to the
	// sub menu itself.  It is also possible to construct the menu
	// from the resource file if desired

	menubar = new wMenu(this, MB_GRAPH, "~Graph\\~Exit\\");
	menubar->SetSubMenuItems(MB_GRAPH, MI_OPTIONS, "~Options...;Printer ~Setup...;-;Print");
	menubar->SetSubMenuItems(MB_EXIT, MI_EXIT, "Exit\tAlt+F4;Continue;");

	// Create a temporary system menu object for the purposes of adding
	// a seperator and the "About..." menu item.

	wSysMenu sm(this);
	sm.AppendSeparator();
	sm.AppendItem(SC_ABOUT, "~About...");

	optionsDialog = new OptionsDialog(this);

	// Setup the initial position, scaling, and rotation...

	xGraphPos = 25;
	yGraphPos = 25;
	scale = 1.0;
	aRotation = 0;

	// Create a device representing the client window

	winDC = new wWindowDevice(this);

	// Create a PrinterInfo object and construct a default
	// PrinterSetup object...

	wPrinterInfo pi;
	prSetup = pi.GetDefaultPrinter();

	// Create the initial wPresSpace object and some tools

	PrepPS();
	Show();
	ToTop();
}

GraphWindow :: ~GraphWindow()
{
	DestroyPS();
	delete icon;
	delete optionsDialog;
}


void GraphWindow :: PrepPS()
{
	if ( prSetup && (prSetup->GetPort()).Length() )
	{
		// Create a wPresSpace object that has the exact page
		// dimensions of the current print destination.	 Although it
		// it is not important in this application, true WYSIWYG apps
		// must do this so that the display and printer are in sync.

		wPrinter printer(prSetup);
		WindowPS() = new wPresSpace(&printer, PuLoMetric, PcReAssociate + PcRetain);
		WindowPS()->AssociateDevice(winDC);
	}
	else
		WindowPS() = new wPresSpace(winDC, PuLoMetric, PcReAssociate + PcRetain);

	CreateFonts(scFonts);
	fonts = scFonts;

	// create a "bracket" wSubPicture

	bracket = new wSubPicture(WindowPS());
	DrawBracket(wDimension(370, 50));
	bracket->Close();
}

void GraphWindow :: DestroyPS()
{
	delete bracket;
	delete scFonts[0];
	delete scFonts[1];
	WindowPS()->DisAssociateDevice();
	delete WindowPS();
	WindowPS() = NULL;
}

void GraphWindow :: CreateFonts(wFont **f)
{
	// Create the fonts using the "painfull" method.  The painfull method
	// includes querying the fonts from the PS and creating the desired
	// fonts using FontDef objects instead of the simple enumerated font
	// types.  This is important because the wFont constructor that uses
	// FontDef objects does not use a machine dependent font metric value
	// called lMatch.

	// lMatch values are simply an index that allow the PM to create a
	// font very quickly.  Without it, it must use a "best fit" algorythm
	// to create a font that matches the desired font as much as possible.
	// Although it is slower, the best fit method is independent of
	// lMatches, and thus, when the "Create wFont" orders are stored in a
	// print file or metafile no lMatches are used.  This makes these files
	// conform to SAA standards as well as being machine independent.

	wFontDefList *fonts = 0;

	for (;;)
	{
		// First check if the PostScript fonts are installed...

		fonts = WindowPS()->EnumFonts("Helvetica");
		if (fonts && fonts->First())
		{
			f[0] = new wFont((*fonts)(), WindowPS());
			f[0]->ChangePointSize(8);
			delete fonts;

			fonts = WindowPS()->EnumFonts("Helvetica Bold");
			f[1] = new wFont((*fonts)(), WindowPS());
			f[1]->ChangePointSize(10);
			break;
		}
		if (fonts) delete fonts;

		// next, try the core Helv font.  Althought this should never
		// fail,  it is possible on the OS/2 1.3 installation to not install
		// this font!  It seems IBM may have gone a bit far in trying to 
		// let users reduce the disk storage required for OS/2.

		fonts = WindowPS()->EnumFonts("Helv", FfOutline);
		if (fonts && fonts->First())
		{
			f[0] = new wFont((*fonts)(), WindowPS());
			f[0]->ChangePointSize(8);

			(*fonts)()->fsSelection = BoldFont;
			f[1] = new wFont((*fonts)(), WindowPS());
			f[1]->ChangePointSize(10);
			break;
		}

		f[0] = new wFont(SysMono10, 0, WindowPS());
		f[1] = new wFont(SysMono12, 0, WindowPS());
		break;
	}
	if (fonts) delete fonts;
}


// The MenuCommand member is sent a Command message each time
// an item on the menu bar is selected.

long GraphWindow :: MenuCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case MI_OPTIONS:
			optionsDialog->FormUp(this);
			if (optionsDialog->GetResult())
				break;							// repaint if "OK" pressed
			else
				return FALSE;


		case MI_PRINT:
			PrintGraph();
			break;

		case MI_SETUP:
			{
				PrintDlg pd(this, prSetup);
				prSetup = pd.GetSetup();
				
				if (pd.IsChanged())
				{
					// re-create the wPresSpace if the printer setup changed.
					// athough we are being lazy here, a real application 
					// would most likely verify that either the printer 
 					// or the form changed before going through all this work.

					HourGlass();
					DestroyPS();	// destroy current PS
					PrepPS();		// create new PS with updated page size
					HourGlass();
					break;
				}
			}
			return FALSE;

		case SC_ABOUT:
			{
				AboutDialog aboutWin(this);		 // Create the "about" box
				return FALSE;
			}

		case MI_EXIT:
			CurrentThread->Quit();
			return FALSE;

	}
	RePaint();		// invalidate the client window 

	return FALSE;
}

void GraphWindow :: PrintGraph()
{
	wFont *prFonts[2];

	HourGlass();
	wPrinter printer(prSetup);
	WindowPS()->AssociateDevice(&printer);

	printer.StartPrintJob("Hardcopy Graph");
	CreateFonts(prFonts);
	fonts = prFonts;

	Paint();
	printer.EndPrintJob();

	delete prFonts[0];
	delete prFonts[1];
	fonts = scFonts;

	WindowPS()->AssociateDevice(winDC);
	HourGlass();
}


void GraphWindow :: Paint()
{

	static ushort vals[] = { 150, 130, 250, 75, 100, 83, 120, 150, 200, 180, 200, 160 }; 
	char wbuf[40];
	int i;

	// Create some drawing tools to work with.  Unlike fonts, pens
	// and textpens are fast to create.	 

	wPen pen(WindowPS());
	wFillPattern fillPat(WindowPS());
	fillPat.SetBackMix(BmOverPaint);
	wTextPen textPen(WindowPS());
	textPen.SetBufferSize(40);

	// Adjust the page orientation 

	WindowPS()->Erase();

	if (aRotation)
		WindowPS()->SetPageRotation(aRotation);

	if (scale != 1)
		WindowPS()->SetPageScale(scale, scale, TransformAdd);

	WindowPS()->SetPageOrigin(wPointl(xGraphPos * 10, yGraphPos * 10), TransformAdd);

	// Draw the axis of the graph in a wide line (at the page origin)
	// Note that we must use a path to do this since it is the only
	// mechanism available to draw lines wider that 1 pel.

	pen.MoveTo(0, 700);
	pen.SetGeomLineWidth(3);
	pen.BeginPath();
	pen.LineTo(0, 0);
	pen.LineTo(1250, 0);
	pen.PaintPath();

	// Draw the horizontal grid	lines 

	pen.SetColor(ClrPaleGray);

	for (i = 1; i < 8; i++)
	{
		pen.MoveTo(2, i * 100);
		pen.LineTo(1250, i * 100);
	}

	// Draw the colored bars using a wFillPattern object to 
	// to fill the interior of the boxes drawn with the wPen object

	pen.SetColor(ClrBlack);

	for (i = 0; i < 12; i++)
	{
		switch (i % 4)
		{
			case 0:
				fillPat.SetColor(ClrDarkRed);
				fillPat.SetPattern(PatDense8);
				break;
			case 1:
				fillPat.SetColor(ClrDarkBlue);	 
				fillPat.SetPattern(PatDense7);
				break;
			case 2:
				fillPat.SetColor(ClrDarkGreen);	 
				fillPat.SetPattern(PatDense6);
				break;
			case 3:
				fillPat.SetColor(ClrRed);	 
				fillPat.SetPattern(PatSolid);
				break;
		}
		pen.Box(wRectl(wPointl(100 * i, 2), wDimension(101, vals[i] * 2)), 
		        DoOutlineFill);
	}

	// Now using an 8 point outline font, draw the horizontal units

	textPen.PenDown(fonts[0]); 	// PenDown causes the wTextPen and the
	                            // wFont to be the current wTextPen and wFont

	long yBaseline = -((long)((float)textPen.MaxCharHeight() * 1.5));

	for (i = 0; i < 12; i++)
		textPen.Printf((100 * i) + 23, yBaseline, "Q%d", (i % 4) + 1);
	

	// draw the brackets and years

	for (i = 0; i < 3; i++)
 	{
		char year[5];
		wPointl p;
		
		p.x() = (i * 400) + 15;
		p.y() = -((textPen.MaxCharHeight() * 2) + 50);
		bracket->Draw(p, TransformAdd);

		sprintf(year, "%d", 1988 + i);
		textPen.Display(p.x() + (185 - (textPen.TextSize(year).xWidth()) / 2),
		                p.y() - 50, year);
	}

	// Draw the y-axis units
 
	float o = 0.0;

	for (i = 0; i < 8; i++, o += .5)
	{
		char nBuf[8];

		sprintf(nBuf, "%.2f", o);
		textPen.Display(-(30 + textPen.TextSize(nBuf).xWidth()),    // x-pos
		               (i * 100) - (textPen.MaxCharHeight() / 2),  	// y-pos
			           nBuf);								      	// text
	}

	// draw the y-axis title

	textPen.PenDown(fonts[1]);
	textPen.SetBaselineAngle(wPointl(0, 10));
	textPen.Display(wPointl(-150, 100), "Earnings per share");
	textPen.SetBaselineAngle(wPointl(1, 0));

	// draw the graph's title

	textPen.Display(425, 625, "History of Earnings");
	WindowPS()->SetPageOrigin(wPointl(0, 0));
}

void GraphWindow :: DrawBracket(wDimension siz)
{
	long basHeight, tailHeight, basArc, tailArc;
	wPen p (WindowPS());

 	basHeight  = siz.yHeight() / 3;
	tailHeight = siz.yHeight() - basHeight;

	siz.xWidth() /= 2;
	basArc  = (long)(((float)basHeight * 0.7) + 0.5);
	tailArc = (long)(((float)tailHeight * 0.7) + 0.5);

	WindowPS()->SetModelTransl(wPointl(siz.xWidth(), 0), TransformAdd);

	// run through a loop twice.  First to draw the right side of the
	// brace, and then secondly, draw the left side using a reflection
	// transform

	for (short i = 0; i < 2; i++)
	{
		p.MoveTo(0,0);
		p.Arc(wPointl(basHeight - basArc, basArc), wPointl(basHeight, basHeight));
		p.LineTo(siz.xWidth() - tailHeight, basHeight);
		p.Arc(wPointl((siz.xWidth() - tailHeight) + tailArc, basHeight + (tailHeight - tailArc)),
			 (wPointl&)siz);

		if (!i)
			WindowPS()->SetModelScale(-1, 1, wPointl(0, 0), TransformPreempt);
	}
}

/* Options Form.... */

OptionsDialog :: OptionsDialog(GraphWindow *win) :
	wFormWindow(D_OPTIONS, 4, 0, DlgModal)
{
	// define wEditField objects

	AddField( new wEditField(FtLong, 4, E_XPOS, &win->xGraphPos, NULL, FsRequired) );
	AddField( new wEditField(FtLong, 4, E_YPOS, &win->yGraphPos, NULL, FsRequired) );
	AddField( new wEditField(FtDouble, 6, E_SCALE, &win->scale, NULL, FsRequired) );
	AddField( new wEditField(FtShort, 4, E_ROTATE, &win->aRotation, NULL, FsRequired) );
	SetActionButtons(BUT_CANCEL, BUT_OK, 0);
}

short OptionsDialog :: Init()
{
	ChangePosition(PosCenter, OwnerWindow());
	return TRUE; 
}


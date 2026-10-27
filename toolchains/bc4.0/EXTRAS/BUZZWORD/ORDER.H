
#if !defined(RCCOMP)                 /* resource compiler does not need include files */

#if !defined(__WC386__)
#include <windows.h>                 /* required for all Windows applications */
#else
#define INCLUDE_DDE_H
#include <win386.h>
#endif
#include <ddeml.h>                   /* DDEML header file */
#if !defined(__ZTC__) && !defined(__WC386__)
#include <memory.h>                  /* memory manipulation functions      */
#endif
#include <stdio.h>                   /* standard input/output routines     */
#include <ctype.h>                   /* type declarations for compiler     */
#include <io.h>                      /* more input/output routines         */
#include <fcntl.h>                   /* flag control functions             */
#include <stdlib.h>                  /* standard declarations of functions */
#include <string.h>                  /* string functions                   */
#include <math.h>                    /* 'rithmetic stuff */
#include <time.h>


#endif        /* !defined(RCCOMP) */


#define  FORM_COUNT  2
#define  TEXT_COUNT  34
#define  BITMAP_COUNT  1
#define  BUTTON_COUNT  11
#define  RECT_COUNT  4
#define  LINE_COUNT  21
#define  FIELD_COUNT  47
#define  TABLE_COUNT  1
#define  FIELD_LIST_COUNT  4
#define  MENUBAR_COUNT  0
#define  TOTAL_COUNT  112


#define  WINEXE_YDBU  20
#define  WINEXE_XDBU  10

#define  IDC_BUTTON_BMP  7204
#define  IDC_BUTTON_CHECK  7202
#define  IDC_BUTTON_RADIO  7201
#define  IDC_BUTTON_OK  7221
#define  IDC_BUTTON_CANCEL  7222
#define  IDC_BUTTON_HELP  7223
#define  IDC_BUTTON_YES  7224
#define  IDC_BUTTON_NO  7225
#define  IDC_BUTTON_ABORT  7835
#define  IDC_BUTTON_RETRY  7836
#define  IDC_BUTTON_IGNORE  50
#define  IDC_BUTTON_PUSH  7203
#define  IDC_BUTTON_DEL   7217
#define  WINEXE_FLD_GENERAL  0
#define  WINEXE_FLD_ALPHANUM  1
#define  WINEXE_FLD_PICTURE  2
#define  WINEXE_FLD_SCROLL  3
#define  WINEXE_FLD_FIXED  4
#define  WINEXE_FLD_PERCENT  5
#define  WINEXE_FLD_FINAN  6
#define  WINEXE_FLD_CUR  7
#define  WINEXE_FLD_DATE  8
#define  WINEXE_FLD_NUMPIC  9
#define  WINEXE_FLD_SELECT  10
#define  WINEXE_FLD_COMBO  11
#define  WINEXE_FLD_CHECK  12
#define  WINEXE_FLD_RADIO  13
#define  WINEXE_FLD_BOOL  14
#define  WINEXE_FLD_MEMO  15
#define  WINEXE_FLD_ALGN_NO  0
#define  WINEXE_FLD_ALGN_LEFT  1
#define  WINEXE_FLD_ALGN_RIGHT  2
#define  WINEXE_FLD_ALGN_CENTER  3
#define  WINEXE_FLD_ALGN_JUSTIFY  4

#define IDM_WINEXE_EXIT        9999
#define IDM_idFirstChild      10000

#define PICHARS "&!#9ALNXYCMDalnxycmd"          /* Field edit picture characters */
#define FORM_TYPE              1
#define BITMAP_TYPE            2
#define BUTTON_TYPE            3
#define TEXT_TYPE              4
#define RECT_TYPE              5
#define LINE_TYPE              6
#define FIELD_TYPE             7
#define FIELD_LIST_TYPE        8
#define TABLE_TYPE             9
#define MENUBAR_TYPE          10

#define WINEXE_PAINTCURFLDONLY   1
#define WINEXE_PAINTALLFIELDS    2

#define MAX_DDE_COUNT            3        /* maximum number of concurrent DDE conversations */

struct WINEXE_ORDER                    /* on screen display order */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this item is on */
   int            type ;               /* type of item in question */
   } ;

struct WINEXE_FORM                     /* form info (MDI child window) */
   {
   int      id ;                       /* unique id */
   char     Label[81] ;                /* Name of the form  */
   RECT     Rect ;                     /* screen location of the form */
   RECT     PosRect ;                  /* new position screen location of the form */
   HWND     hWnd ;                     /* window handle */
   /* field edit stuff */
   BOOL     EditMode ;                 /* are we in field edit mode */
   BOOL     EditAll ;                  /* editting all fields ? */
   BOOL     InsMode ;                  /* are we in insert mode */
   int      EndKey ;                   /* key pressed to exit field edit mode */
   int      curfld ;                   /* index of current field being editted */
   int      curtblrow ;                /* if editting a table column, current row number of table */
   int      caretpos ;                 /* current edit/cursor position in field */
   int      prevpos ;                  /* previous edit/cursor position in field */
   int      charoff ;                  /* characters offset in displaying field */
   int      xcaretpos ;                /* caret position */
   int      ycaretpos ;                /* caret position */
   int      xOffset ;
   int      yOffset ;
   int      xMaxPix ;
   int      yMaxPix ;
   } ;

struct WINEXE_BITMAP                   /* bitmap info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this bitmap is on */
   char far       Filename[31] ;       /* filename containing bitmap */
   BOOL           fullscreen ;         /* full screen bitmap ? */
   RECT           Rect ;               /* bitmap position and size */
   RECT           orgRect ;            /* original bitmap position and size */
   HBITMAP        handle ;             /* handle used to display the bitmap */
   int            loaderror ;          /* error on load ? if so do not retry */
   } ;

struct WINEXE_TEXT                     /* text info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this line of text is on */
   int            Row ;                /* Row Position on screen for this text control */
   int            Col ;                /* Column for the text control */
   COLORREF       bgColor ;            /* Background Color for this control */
   COLORREF       fgColor ;            /* Foreground Color for this control */
   int            NoBg ;               /* do net set background color */
   int            string ;             /* index into string array */
   int            LogFont ;            /* Font specific information for this control */
   } ;

struct WINEXE_RECT                     /* rectangle info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this rectangle is on */
   RECT           Rect ;               /* holds the coordinates of the rectangle */
   BOOL           bRound ;             /* round corners? */
   BOOL           bOval ;              /* oval?          */
   BOOL           bFill ;              /* fill pattern?  */
   int            iWidth ;             /* width of the line (1-4 pixels)        */
   BOOL           bOutline ;           /* outline all edges at once?            */
   BOOL           bRight, bLeft ;      /* outline the edges individually?       */
   BOOL           bTop, bBottom ;      /* outline the edges individually?       */
   COLORREF       fgColor, bgColor ;   /* the various colors that are necessary */
   COLORREF       bordColor ;          /* the various colors that are necessary */
   WORD           wPattern ;           /* control # of the pattern they want    */
   LOGBRUSH       LogBrush ;           /* logbrush of fill pattern              */
   BOOL           bOpaque ;            /* is background to be opaque?           */
   BOOL           bTrans ;             /* is background to be transparent?      */
   } ;

struct WINEXE_LINE                     /* line info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this rectangle is on */
   RECT           Rect ;               /* holds the coordinates of the line */
   int            Width ;              /* width of the line (1-6 pixels)        */
   int            Style ;              /* style PS_SOLID, PS_DASH, etc. */
   COLORREF       Color ;              /* line color */
   } ;

struct WINEXE_FIELD                    /* data input field info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this field is on */
   int            tableid ;            /* table id this field is on (if a table column) */
   char far       *Label ;             /* on screen label */
   char far       *Picture ;           /* picture clause */
   char far * far flds[40] ;           /* read/write to these database fields */
                              /* stored as { fieldname, filename, linkname, RW, ... NULL, NULL, NULL } */
   char far * far locflds[30] ;        /* do locates on these database fields */
                              /* stored as { fieldname, filename, linkname, ... NULL, NULL, NULL } */
   HSZ            hFieldDDE[MAX_DDE_COUNT] ;      /* dde string handles for field names */
   int            FieldType ;          /* field type */
   int            CharOp ;             /* character field options */
   int            NumOp ;              /* numeric field options */
   int            SelOp ;              /* selection field options */
   int            Length ;             /* field length */
   int            Decimals ;           /* number of decimals */
   int            HideLabel ;          /* hide the label ? */
   int            Alignment ;          /* alignment style */
   int            ProtNoOv ;           /* protection no override */
   int            ProtNoTree ;         /* protection no tree */
   int            Order ;              /* field order */
   int            automatic ;          /* if a selection field is it automatic ? */
   BOOL           bOutline ;           /* outline */
   BOOL           bTop ;               /* outline - top */
   BOOL           bLeft ;              /* outline - left */
   BOOL           bBottom ;            /* outline - bottom */
   BOOL           bRight ;             /* outline - right */
   COLORREF       LabelFg ;            /* label font foreground color */
   COLORREF       LabelBg ;            /* label font background color */
   COLORREF       ValueFg ;            /* value font foreground color */
   COLORREF       ValueBg ;            /* value font background color */
   COLORREF       BorderColor ;        /* border color */
   int            NoBg ;               /* do not set a background color */
   RECT           Rect ;               /* field area */
   RECT           ValueRect ;          /* holds the coordinates of the input area */
   RECT           ColRect ;            /* for table fields, holds the coordinates of the table column area */
   HANDLE         Buffer ;             /* field buffer */
   LPSTR          str ;                /* lock handle for buffer */
   HANDLE         ColBuffer ;          /* table Column value buffer */
   HWND           hWnd ;               /* handle for combo boxes & listbox type fields */
   HBRUSH         BkgBrush ;           /* background brush for radio buttons & check boxes */
   int            LabelFont ;          /* label font */
   int            ValueFont ;          /* value font */
   } ;

struct WINEXE_FIELD_LIST               /* field list item info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this field list item is on */
   int            fieldid ;            /* field id this field list item is associated with */
   int            FieldType ;          /* field type */
   char far       Label[81] ;          /* on screen label */
   RECT           Rect ;               /* button position */
   HWND           hWnd ;               /* Button Handle */
   } ;

struct WINEXE_TABLE                    /* table info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this table is on */
   char far       Tablename[81] ;      /* table name */
   char far       Filename[81] ;       /* file name of dbf file */
   int            Rows ;               /* number of rows */
   int            Cols ;               /* number of columns */
   RECT           Rect ;               /* entire table area */
   int            TblLblSize ;         /* table label size */
   int            ColLblSize ;         /* column label size */
   int            AvgRowSize ;         /* average row size */
   int            AvgColSize ;         /* average column size */
   int            restrict ;           /* table is restricted ? */
   COLORREF       TblLblFg ;           /* table foreground color */
   COLORREF       TblLblBg ;           /* table background color */
   COLORREF       ColLblFg ;           /* column heading foreground color */
   COLORREF       ColLblBg ;           /* column heading background color */
   COLORREF       BorderColor ;        /* border color */
   BOOL           ScrollBar ;          /* scroll bar present */
   HWND           ScrollhWnd ;         /* handle for the scroll bar */
   int            LineWidth ;          /* line width */
   long           RowRecNum[100] ;     /* row record number */
   RECT           ColRect[256] ;       /* all column rectangle dimensions */
   int            TableFont ;          /* table & column heading font */
   } ;

struct WINEXE_BUTTON                   /* button info */
   {
   int            id ;                 /* unique id */
   int            formid ;             /* form id this button is on */
   char far       Label[81] ;          /* user inputted label */
   char far       BitmapFn[81] ;       /* bitmap filename */
   int            AutoButton ;         /* auto button id */
   int            Type ;               /* button type */
   RECT           Rect ;               /* button area */
   RECT           bitmapRect ;         /* bitmap rectangle */
   HBITMAP        handle ;             /* handle used to display bitmap buttons */
   BOOL           AttrTabStop ;        /* tab stop */
   BOOL           AttrDisabled ;       /* disabled */
   BOOL           AttrGroup ;          /* group button */
   BOOL           AttrVisible ;        /* visible button */
   BOOL           AttrBorder ;         /* border on button */
   int            LabelFont ;          /* table & column heading font */
   int            loaderror ;          /* error on load ? if so do not retry */
   HWND           hWnd ;               /* window handle */
   } ;

struct WINEXE_MENUBAR                  /* menu bar item info */
   {
   int            id ;                 /* unique id */
   int            menuid ;             /* menu id this menu bar item is associated with */
   char far       Label[81] ;          /* on screen label */
   } ;

struct WINEXE_DDE_LINK                 /* dde link definition */
   {
   LPSTR         LinkName ;            /* dde link name */
   LPSTR         Server ;              /* dde link server */
   LPSTR         Topic ;               /* dde link topic */
   LPSTR         ClientFields ;        /* dde link client field names */
   LPSTR         ServerFields ;        /* dde link server field names */
   } ;

struct WINEXE_DDE_INFO                 /* dde link runtime information */
   {
   char far       name[41] ;           /* link name */
   HCONV          hConv ;              /* dde conversation handle */
   HSZ            hService ;           /* string handle to service (server) */
   HSZ            hTopic ;             /* string handle to topic */
   char far       sflds[1025] ;        /* server (remote) field names (comma delimited) */
   char far       cflds[1025] ;        /* client field names (comma delimited) */
   int            status ;             /* status, 1==in use, 0==not in use */
   } ;

extern struct WINEXE_DDE_INFO ddeinfo[] ;                                 /* hold dde information here */
extern DWORD          idInstDDE ;                                         /* global dde application-instance identifier */
extern FARPROC        lpProcDDE ;                                         /* dde callback function */
extern HSZ            hServerDDE ;                                        /* dde string for server name */
extern HSZ            hTopicDDE ;                                         /* dde string for topic name */
extern char far       DDEServerName[] ;                                   /* this application's executable name */
extern int            ServerModeDDE ;                                     /* are we in DDE Server mode ? */

extern HANDLE      hInst;                                                 /* current instance                   */
extern HWND        hwnd;                                                  /* handle to main window              */
extern HWND        hwndMDIClient ;                                        /* handle to MDI client window        */
extern HCURSOR     hCurWait;                                              /* Hourglass cursor                   */
extern int         currentformindex ;                                     /* current form                       */
extern int         PaintFieldValueOnly ;                                  /* used for data input                */
extern int         NoScreenRefresh ;                                      /* do not paint the screen */
extern char   far  FieldChangeBuffer[] ;                                  /* used to detect field change events */
extern int         FieldChangeLocateTableRow ;                            /* used for table relationships on field changes */
extern int         FieldChangeLocating ;                                  /* check if field change              */
extern char   far  ReturnStrings[][256] ;                                 /* hold EVENT function return strings here */
extern int         NextAvailableString ;                                  /* counter to cycle thru return strings */
extern char far    WindowTitle[] ;
extern char far * far strings[] ;
extern struct WINEXE_ORDER far paintorder[] ;
extern struct WINEXE_FORM far forms[] ;
extern struct WINEXE_BITMAP far bitmaps[] ;
extern struct WINEXE_BUTTON far buttons[] ;
extern struct WINEXE_TEXT far text[] ;
extern struct WINEXE_RECT far rects[] ;
extern struct WINEXE_LINE far lines[] ;
extern struct WINEXE_FIELD far *fields[] ;
extern struct WINEXE_FIELD_LIST far fieldlists[] ;
extern struct WINEXE_TABLE far tables[] ;
extern struct WINEXE_MENUBAR far menubars[] ;
extern struct WINEXE_DDE_LINK far ddelinks[] ;
extern LOGFONT fonts[] ;

int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);

#if defined(__cplusplus)
extern "C" {
#endif

long FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);
long FAR PASCAL ChildWndProc(HWND, UINT, WPARAM, LPARAM) ;
HDDEDATA EXPENTRY DdeCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD) ;

#if defined(__cplusplus)
}
#endif

extern void ScreenResAdjust(RECT far *, double, double) ;
int PassesRestrictions(int, int) ;
int FindIndex(int, int) ;
int IsSameFile(LPSTR, LPSTR) ;
int IsFieldInFile(int, LPSTR, int *, int *) ;
int GetFieldInFile(int, LPSTR, LPSTR) ;
void EventLockOutFile(LPSTR, int) ;
int IsEventLockOutFile(LPSTR) ;
void PaintForm(HWND, int, int) ;
void PaintBitmap(HWND, int, int, LPRECT) ;
void PaintButton(HWND, int, int, DRAWITEMSTRUCT FAR *) ;
void CheckButtonSize(HWND, int) ;
void PaintText(HWND, int, int, LPRECT) ;
void PaintRect(HWND, int, int, LPRECT) ;
HBRUSH PaintRectBack(int) ;
HBRUSH PaintRectFill(int) ;
void PaintLine(HWND, int, int, LPRECT) ;
void PaintField(HWND, int, int, LPRECT) ;
void PaintFieldValue(HWND, int, int) ;
void PaintTable(HWND, int, int, LPRECT) ;
BOOL WinExeLoadBitmap(HWND, int, char far *, LPRECT, LPRECT, HBITMAP far *) ;
BOOL WinExeLoadBitmapRead(int, LPSTR, DWORD) ;
void WinExeLoadBitmapError(HWND, char far *) ;
void WinExeLoadBitmapInvalid(HWND, char far *) ;
void WinExeLoadBitmapNoMem(HWND, char far *) ;
void WinExeLoadBitmapNoLock(HWND, char far *) ;

void APPEXIT(void) ;
void APPOPEN(LPSTR) ;
void ASSIGNNUM(LPSTR, double) ;
void ASSIGNSTR(LPSTR, LPSTR) ;
void BOTTOM(LPSTR) ;
void CLEAR(LPSTR) ;
void CLEARALL(void) ;
void DDEEXECUTE(LPSTR, LPSTR) ;
void DDEOPEN(LPSTR, LPSTR, LPSTR, LPSTR, LPSTR) ;
BOOL InitializeDDEML(void) ;
BOOL CleanUpDDEML(void) ;
void CLOSEDDE(LPSTR) ;
void DDEPOKESTR(LPSTR, LPSTR, LPSTR) ;
void DDEPOKENUM(LPSTR, LPSTR, double) ;
void DELETE(LPSTR) ;
void EXEC(LPSTR, UINT) ;
void ExecuteDDECommand(LPSTR) ;
void FIELDFIND(LPSTR) ;
double FIELDNUM(LPSTR) ;
LPSTR FIELDSTR(LPSTR) ;
BOOL FORMSELECT(LPSTR) ;
void FillSelectionBox(int flidx) ;
void FORMCLOSE(LPSTR) ;
void INSERT(LPSTR) ;
int LOCATE(LPSTR, int, LPSTR) ;
int LOCATE2(LPSTR, int, LPSTR, LPSTR) ;
int LOCATE3(LPSTR, int, LPSTR, LPSTR, LPSTR) ;
void LOCATEFIELD(LPSTR, int, LPSTR) ;
void NEXT(LPSTR) ;
void NOOP(void) ;
void PAGEDN(LPSTR) ;
void PAGEUP(LPSTR) ;
void PREVIOUS(LPSTR) ;
void RESUME(void) ;
LPSTR SELECTEDFORM(void) ;
LPSTR STRCAT(LPSTR dest, LPSTR src) ;
void STORE(LPSTR) ;
BOOL TOP(LPSTR) ;
void UPDATE(LPSTR) ;
BOOL InitDatabaseEngine(void) ;
BOOL OpenDatabases(void) ;
void CloseDatabases(void) ;
void ReadDatabaseBuffer(int, int) ;
void ReadField(int, int, LPSTR) ;
void WriteDatabaseBuffer(int) ;
void WriteField(int, LPSTR, LPSTR) ;
long RecordCount(int) ;
long GotoRecord(int, long) ;
void RedrawFields(LPSTR) ;
void FieldAllocBuffers(void) ;
void FieldEdit(int) ;
void FieldEditEnd(int, int) ;
void FieldKeyInput(int, int) ;
void FieldPictureApply(LPSTR, LPSTR, int) ;
int FieldPicturePosition(int, LPSTR, int, int) ;
void FieldPictureShift(LPSTR, LPSTR, int, int, int, int) ;


/* dib stuff */

// Some macros.

#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

#define szMDIChild "MyDIBMDI"          // Class name of MDI children.
#define DRAGCURSOR "DragCursor"        // Name of dragging cursor in .RC file.

#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))

// Global variables.

extern HANDLE hInst;                   // Handle to this instance
extern HWND   hWndMDIClient;           // MDI Client's window handle.

//  Defines for File Save format combo box

#define ID_FS_BEGIN       IDS_PM
#define ID_FS_END         IDS_24

// WIDTHBYTES takes # of bits in a scan line and rounds up to nearest
//  word.

#define WIDTHBYTES(bits)      (((bits) + 31) / 32 * 4)

   // Given a pointer to a DIB header, return TRUE if is a Windows 3.0 style
   //  DIB, false if otherwise (PM style DIB).

#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD) (lpbi)) == sizeof (BITMAPINFOHEADER))

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

void     RealizeDIBPalette    (HDC hDC, LPBITMAPINFO lpbmi);
WORD     DIBNumColors         (LPSTR lpbi);
LPSTR    FindDIBBits          (LPSTR lpbi);
WORD     PaletteSize          (LPSTR lpbi);
HPALETTE CreateDIBPalette     (HANDLE hDIB);
DWORD    DIBHeight            (LPSTR lpDIB);
DWORD    DIBWidth             (LPSTR lpDIB);
HBITMAP  DIBToBitmap          (HANDLE hDIB, HPALETTE hPal);
HANDLE   BitmapToDIB          (HBITMAP hBitmap, HPALETTE hPal);
void     InitBitmapInfoHeader (LPBITMAPINFOHEADER lpBmInfoHdr,
                                            DWORD dwWidth,
                                            DWORD dwHeight,
                                              int nBPP);

/*         error codes for DIB routines            */

enum {
      ERR_MIN = 0,                     // All error #s >= this value
      ERR_NOT_DIB = 0,                 // Tried to load a file, NOT a DIB!
      ERR_MEMORY,                      // Not enough memory!
      ERR_READ,                        // Error reading file!
      ERR_LOCK,                        // Error on a GlobalLock()!
      ERR_OPEN,                        // Error opening a file!
      ERR_CREATEPAL,                   // Error creating palette.
      ERR_GETDC,                       // Couldn't get a DC.
      ERR_CREATECHILD,                 // Error creating MDI child.
      ERR_CREATEDDB,                   // Error create a DDB.
      ERR_STRETCHBLT,                  // StretchBlt() returned failure.
      ERR_STRETCHDIBITS,               // StretchDIBits() returned failure.
      ERR_NODIBORDDB,                  // Error painting -- need BOTH DDB & DIB.
      ERR_SETDIBITSTODEVICE,           // SetDIBitsToDevice() failed.
      ERR_STARTDOC,                    // Error calling StartDoc().
      ERR_NOGDIMODULE,                 // Couldn't find GDI module in memory.
      ERR_SETABORTPROC,                // Error calling SetAbortProc().
      ERR_STARTPAGE,                   // Error calling StartPage().
      ERR_NEWFRAME,                    // Error calling NEWFRAME escape.
      ERR_ENDPAGE,                     // Error calling EndPage().
      ERR_ENDDOC,                      // Error calling EndDoc().
      ERR_ANIMATE,                     // Only one DIB animated @ a time.
      ERR_NOTIMERS,                    // No timers avail for pal animation.
      ERR_NOCLIPWINDOW,                // Now current window for clipboard.
      ERR_CLIPBUSY,                    // Clipboard is busy.
      ERR_NOCLIPFORMATS,               // During paste can't find DIB or DDB.
      ERR_SETDIBITS,                   // Error calling SetDIBits().
      ERR_FILENOTFOUND,                // Error opening file in GetDib()
      ERR_WRITEDIB,                    // Error writing DIB file.
      ERR_MAX                          // All error #s < this value
     };


void DIBError (int ErrNo);

// Size of window extra bytes (we store a handle to a PALINFO structure).

#define PAL_CBWNDEXTRA  (1 * sizeof (WORD))

typedef struct
   {
   HPALETTE hPal;                      // Handle to palette being displayed.
   WORD     wEntries;                  // # of entries in the palette.
   int      nSquareSize;               // Size of palette square (see PAL_SIZE)
   HWND     hInfoWnd;                  // Handle to the info bar window.
   int      nRows, nCols;              // # of Rows/Columns in window.
   int      cxSquare, cySquare;        // Pixel width/height of palette square.
   WORD     wEntry;                    // Currently selected palette square.
   } PALINFO, FAR *LPPALINFO;

   // Window Words.

#define WW_PAL_HPALINFO 0              // Handle to PALINFO structure.

   // The following define is for CopyPaletteChangingFlags().

#define DONT_CHANGE_FLAGS -1

   // The following is the palette version that goes in a
   //  LOGPALETTE's palVersion field.

#define PALVERSION   0x300

// This is an enumeration for the various ways we can display
//  a palette in PaletteWndProc().

enum PAL_SIZE
   {
   PALSIZE_TINY = 0,
   PALSIZE_SMALL,
   PALSIZE_MEDIUM,
   PALSIZE_LARGE
   };


// Function prototypes.
long FAR PASCAL PaletteWndProc (HWND hwnd,
                                UINT message,
                                WPARAM wParam,
                                LPARAM lParam);
void SetPaletteWindowsPal (HWND hWnd, HPALETTE hPal);
HPALETTE GetSystemPalette (void);
HPALETTE CopyPaletteChangingFlags (HPALETTE hPal, BYTE bNewFlag);
void MyAnimatePalette (HWND hWnd, HPALETTE hPal);
int ColorsInPalette (HPALETTE hPal);

#define CopyPalette(hPal)  CopyPaletteChangingFlags (hPal,(BYTE) DONT_CHANGE_FLAGS)
#define CopyPalForAnimation(hPal) CopyPaletteChangingFlags (hPal, PC_RESERVED)




#define WD_ERR      -1
#define WD_OK       1

#define WD_NA       2
#define WD_FALSE    0
#define WD_TRUE     !WD_FALSE
#define WD_YES      WD_TRUE
#define WD_NO       WD_FALSE


#if !defined(__WC386__)
extern double   ACOS(double X) ;
extern double   ASIN(double X) ;
extern double   ATAN(double X) ;
extern double   ATAN2(double X, double Y) ;
extern double   COS(double X) ;
extern double   EXP(double X) ;
extern double   LOG(double X) ;
extern double   SIN(double X) ;
extern double   SQRT(double X) ;
#else
extern double   wd_ACOS(double X) ;
extern double   wd_ASIN(double X) ;
extern double   wd_ATAN(double X) ;
extern double   wd_ATAN2(double X, double Y) ;
extern double   wd_COS(double X) ;
extern double   wd_EXP(double X) ;
extern double   wd_LOG(double X) ;
extern double   wd_SIN(double X) ;
extern double   wd_SQRT(double X) ;
#endif

extern double   ABS(double X) ;
extern void     ADDMENU(LPSTR MenuName, UINT Position) ;
extern void     ADDMENUITEM(LPSTR MenuName, LPSTR MenuItemName, UINT Position) ;
extern void     APPNEW(void) ;
extern void     ASCIIOPEN(LPSTR LinkName, LPSTR FileName, LPSTR OVFields, UINT Option) ;
extern double   AVG(double L1, double L2) ;
extern double   AVG3(double L1, double L2, double L3) ;
extern double   AVG4(double L1, double L2, double L3, double L4) ;
extern double   AVG5(double L1, double L2, double L3, double L4, double L5) ;
extern double   AVG6(double L1, double L2, double L3, double L4, double L5, double L6) ;
extern double   AVG7(double L1, double L2, double L3, double L4, double L5, double L6, double L7) ;
extern double   AVG8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8) ;
extern double   AVG9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9) ;
extern double   AVG10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10) ;
extern double   AVG11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11) ;
extern double   AVG12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12) ;
extern double   AVG13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13) ;
extern double   AVG14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14) ;
extern double   BLANK(void) ;
extern void     BTRVOPEN(LPSTR LinkName, LPSTR Dictionary, LPSTR TableName, LPSTR Datafields, LPSTR OVReadFields, LPSTR OVWriteFields, UINT IndexNo, UINT Option) ;
extern int      CHAR(UINT Code) ;
extern void     CHECKMENUITEM(LPSTR MenuName, LPSTR MenuItemName) ;
extern LPSTR    CHOOSESTR(UINT Number, LPSTR S1) ;
extern LPSTR    CHOOSESTR2(UINT Number, LPSTR S1, LPSTR S2) ;
extern LPSTR    CHOOSESTR3(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3) ;
extern LPSTR    CHOOSESTR4(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4) ;
extern LPSTR    CHOOSESTR5(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5) ;
extern LPSTR    CHOOSESTR6(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6) ;
extern LPSTR    CHOOSESTR7(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7) ;
extern LPSTR    CHOOSESTR8(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8) ;
extern LPSTR    CHOOSESTR9(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9) ;
extern LPSTR    CHOOSESTR10(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9, LPSTR S10) ;
extern LPSTR    CHOOSESTR11(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9, LPSTR S10, LPSTR S11) ;
extern LPSTR    CHOOSESTR12(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9, LPSTR S10, LPSTR S11, LPSTR S12) ;
extern LPSTR    CHOOSESTR13(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9, LPSTR S10, LPSTR S11, LPSTR S12, LPSTR S13) ;
extern LPSTR    CHOOSESTR14(UINT Number, LPSTR S1, LPSTR S2, LPSTR S3, LPSTR S4, LPSTR S5, LPSTR S6, LPSTR S7, LPSTR S8, LPSTR S9, LPSTR S10, LPSTR S11, LPSTR S12, LPSTR S13, LPSTR S14) ;
extern double   CHOOSENUM(UINT Number, double N1) ;
extern double   CHOOSENUM2(UINT Number, double N1, double N2) ;
extern double   CHOOSENUM3(UINT Number, double N1, double N2, double N3) ;
extern double   CHOOSENUM4(UINT Number, double N1, double N2, double N3, double N4) ;
extern double   CHOOSENUM5(UINT Number, double N1, double N2, double N3, double N4, double N5) ;
extern double   CHOOSENUM6(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6) ;
extern double   CHOOSENUM7(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7) ;
extern double   CHOOSENUM8(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8) ;
extern double   CHOOSENUM9(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8, double N9) ;
extern double   CHOOSENUM10(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8, double N9, double N10) ;
extern double   CHOOSENUM11(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8, double N9, double N10, double N11) ;
extern double   CHOOSENUM12(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8, double N9, double N10, double N11, double N12) ;
extern double   CHOOSENUM13(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8, double N9, double N10, double N11, double N12, double N13) ;
extern double   CHOOSENUM14(UINT Number, double N1, double N2, double N3, double N4, double N5, double N6, double N7, double N8, double N9, double N10, double N11, double N12, double N13, double N14) ;
extern void     CLOSE(LPSTR LinkName) ;
extern int      CODE(LPSTR String) ;
extern double   COLUMNAVG(LPSTR ColumnName) ;
extern double   COLUMNCOUNT(LPSTR ColumnName) ;
extern double   COLUMNMAX(LPSTR ColumnName) ;
extern double   COLUMNMIN(LPSTR ColumnName) ;
extern double   COLUMNSUM(LPSTR ColumnName) ;
extern double   CTERM(double Rate, double Fv, double Pv) ;
extern LPSTR    CURRENTFILE(void) ;
extern LPSTR    CURRENTPATH(void) ;
extern long     DATESERIAL(int Yr, int Mo, int Day) ;
extern long     DATEVALUE(LPSTR DateValue) ;
extern int      MonTextToNumber(LPSTR p) ;
extern int      DAY(long DateTimeNumber) ;
extern void     DBOPEN(LPSTR LinkName, LPSTR Filename, LPSTR DataFields, LPSTR OVReadFields, LPSTR OVWriteFields, LPSTR IndexFile, UINT Option) ;
extern double   DDB(double Cost, double Salvage, double Life, double Period) ;
extern void     DDEEXECUTE(LPSTR LinkName, LPSTR Commands) ;
extern void     DDEOPEN(LPSTR LinkName, LPSTR Application, LPSTR Document, LPSTR RemoteNames, LPSTR OVFields) ;
extern void     DDEPOKESTR(LPSTR LinkName, LPSTR RemoteName, LPSTR Value) ;
extern void     DDEPOKENUM(LPSTR LinkName, LPSTR RemoteName, double Value) ;
extern double   DEGREES(double X) ;
extern void     DELETEMENU(LPSTR MenuName) ;
extern void     DELETEMENUITEM(LPSTR MenuName, LPSTR MenuItemName) ;
extern double   ERR(void) ;
extern void     EVENT(LPSTR Item, LPSTR Event) ;
extern double   EXACT(LPSTR String1, LPSTR String2) ;
extern void     EXEC(LPSTR CommandLine, UINT ShowOption) ;
extern double   FIELDCALCULATE(LPSTR FieldName) ;
extern double   FIELDCLEAR(LPSTR FieldName) ;
extern BOOL     FILTERACTIVATE(LPSTR LinkName, LPSTR FilterName) ;
extern void     FILTERDEACTIVATE(LPSTR LinkName) ;
extern double   FIND(LPSTR SubString, LPSTR String, UINT StartNumber) ;
extern LPSTR    FORMAT(double Num, UINT Format, UINT Places) ;
extern void     FORMCLEAR(LPSTR FormName) ;
extern double   FC(double Pmt, double Rate, double Nper) ;
extern double   FVAL(double Rate, double Nper, double Pmt, double Pv, double Type) ;
extern int      HOUR(double DateTimeNumber) ;
extern double   INT(double X) ;
extern double   IPAYMT(double Rate, double Per, double Nper, double Pv, double Fv, double Type) ;
extern double   IRATE(double Nper, double Pmt, double Pv, double Fv, double Type) ;
extern double   IRR(double Guess, LPSTR ColumnName) ;
extern double   ISBLANK(LPSTR FieldName) ;
extern double   ISCOMPLETE(LPSTR FormName) ;
extern LPSTR    LEFT(LPSTR String, double Num) ;
extern double   LENGTH(LPSTR String) ;
extern double   LINKAVG(LPSTR LinkName, LPSTR DataField) ;
extern double   LINKCOUNT(LPSTR LinkName) ;
extern double   LINKMAX(LPSTR LinkName, LPSTR DataField) ;
extern double   LINKMIN(LPSTR LinkName, LPSTR DataField) ;
extern double   LINKSUM(LPSTR LinkName, LPSTR DataField) ;
extern double   LINKVALUE(LPSTR LinkName, LPSTR DataField) ;
extern double   LN(double X) ;
extern LPSTR    LOWER(LPSTR String) ;
extern double   MAX(double L1) ;
extern double   MAX2(double L1, double L2) ;
extern double   MAX3(double L1, double L2, double L3) ;
extern double   MAX4(double L1, double L2, double L3, double L4) ;
extern double   MAX5(double L1, double L2, double L3, double L4, double L5) ;
extern double   MAX6(double L1, double L2, double L3, double L4, double L5, double L6) ;
extern double   MAX7(double L1, double L2, double L3, double L4, double L5, double L6, double L7) ;
extern double   MAX8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8) ;
extern double   MAX9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9) ;
extern double   MAX10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10) ;
extern double   MAX11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11) ;
extern double   MAX12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12) ;
extern double   MAX13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13) ;
extern double   MAX14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14) ;
extern void     MESSAGE(LPSTR String) ;
extern LPSTR    MID(LPSTR String, UINT StartNumber, UINT Num) ;
extern double   MIN(double L1) ;
extern double   MIN2(double L1, double L2) ;
extern double   MIN3(double L1, double L2, double L3) ;
extern double   MIN4(double L1, double L2, double L3, double L4) ;
extern double   MIN5(double L1, double L2, double L3, double L4, double L5) ;
extern double   MIN6(double L1, double L2, double L3, double L4, double L5, double L6) ;
extern double   MIN7(double L1, double L2, double L3, double L4, double L5, double L6, double L7) ;
extern double   MIN8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8) ;
extern double   MIN9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9) ;
extern double   MIN10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10) ;
extern double   MIN11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11) ;
extern double   MIN12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12) ;
extern double   MIN13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13) ;
extern double   MIN14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14) ;
extern int      MINUTE(double DateTimeNumber) ;
extern double   MOD(double X, double Y) ;
extern int      MONTH(double DateTimeNumber) ;
extern double   NA(void) ;
extern double   NOT(double Logical) ;
extern double   NOW(void) ;
extern double   NPER(double Rate, double Pmt, double Pv, double Fv, double Type) ;
extern double   NPV(double Rate, LPSTR ColumnName, double Type) ;
extern double   PAYMT(double Rate, double Nper, double Pv, double Fv, double Type) ;
#if !defined(__ZTC__)
extern double   PI(void) ;
#else
extern double   wd_PI(void) ;
#endif
extern double   PMT(double Pv, double Rate, double Nper) ;
extern double   PPAYMT(double Rate, double Per, double Nper, double Pv, double Fv, double Type) ;
extern void     PRINTALL(void) ;
extern void     PRINTFORM(LPSTR FormName) ;
extern void     PRINTLINK(LPSTR LinkName) ;
extern LPSTR    PROPER(LPSTR String) ;
extern double   PV(double Pmt, double Rate, double Nper) ;
extern double   PVAL(double Rate, double Nper, double Pmt, double Fv, double Type) ;
extern void     PXOPEN(LPSTR LinkName, LPSTR TableName, LPSTR DataFields, LPSTR OVReadFields, LPSTR OVWriteFields, LPSTR SecIndexField, UINT Option) ;
extern double   RADIANS(double X) ;
extern double   RATE(double Fv, double Pv, double Nper) ;
extern void     REGISTER(LPSTR OVAlias, LPSTR ArgTypes, LPSTR ArgHelp, LPSTR LibName, LPSTR FuncName, UINT Type) ;
extern LPSTR    REPEAT(LPSTR String, UINT Num) ;
extern LPSTR    REPLACE(LPSTR String, UINT StartNum, UINT Num, LPSTR NewString) ;
extern void     RESTOREMENU(void) ;
/* extern void     RESUME(void) ; */
extern LPSTR    RIGHT(LPSTR String, UINT Num) ;
extern double   ROUND(double X, int Num) ;
extern void     SAVE(void) ;
extern void     SAVEAS(LPSTR FileName) ;
extern int      SECOND(double DateTimeNumber) ;
extern LPSTR    SELECTEDFIELD(void) ;
extern void     SETTITLE(LPSTR Title) ;
extern double   SLN(double Cost, double Salvage, double Life) ;
extern double   SUM(double L1) ;
extern double   SUM2(double L1, double L2) ;
extern double   SUM3(double L1, double L2, double L3) ;
extern double   SUM4(double L1, double L2, double L3, double L4) ;
extern double   SUM5(double L1, double L2, double L3, double L4, double L5) ;
extern double   SUM6(double L1, double L2, double L3, double L4, double L5, double L6) ;
extern double   SUM7(double L1, double L2, double L3, double L4, double L5, double L6, double L7) ;
extern double   SUM8(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8) ;
extern double   SUM9(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9) ;
extern double   SUM10(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10) ;
extern double   SUM11(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11) ;
extern double   SUM12(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12) ;
extern double   SUM13(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13) ;
extern double   SUM14(double L1, double L2, double L3, double L4, double L5, double L6, double L7, double L8, double L9, double L10, double L11, double L12, double L13, double L14) ;
extern double   SYD(double Cost, double Salvage, double Life, double Period) ;
extern double   TAN(double X) ;
extern double   TERM(double Pmt, double Rate, double Fv) ;
extern double   TIME(int Hr, int Min, int Sec) ;
extern double   TIMEVALUE(LPSTR TimeString) ;
extern long     TODAY(void) ;
extern LPSTR    TRIM(LPSTR String) ;
extern double   TYPENUM(double Value) ;
extern double   TYPESTR(LPSTR Value) ;
extern double   TYPELOG(double Value) ;
extern double   TYPEERR(double Value) ;
extern void     UNCHECKMENUITEM(LPSTR MenuName, LPSTR MenuItemName) ;
extern LPSTR    UPPER(LPSTR String) ;
extern double   VERSION(void) ;
extern int      WEEKDAY(double DateTimeNumber) ;
extern LPSTR    WHILEFIELD(LPSTR Fieldname, LPSTR Eventname)  ;
extern int      YEAR(double DateTimeNumber) ;


/* Source Code Header File for No DBMS Support */

struct WINEXE_DATABASES               /* database information */
   {
   char           far *name ;          /* table name */
   char           far *link ;          /* objectvision link name (from .ovd file) */
   char           far *record ;        /* record transfer buffer */
   int            fieldcount ;         /* number of fields in this table */
   int            handle ;             /* database handle */
   long           recordnumber ;       /* current record number */
   long           recordcount ;        /* number of records */
   int            autoinsert ;         /* automatic insert of new record when done editting ? */
   int            autoupdate ;         /* automatic update of existing record when done editting ? */
   int            eventlock ;          /* event lock out flag */
   } ;

extern struct WINEXE_DATABASES far databases[] ;


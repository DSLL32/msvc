#include "order.h"

/*****************************************************************************/
/*                                                                           */
/*  PROGRAM: FILENAME.C                                                      */
/*                                                                           */
/*  COPYRIGHT: (C) Buzzwords International Inc.                              */
/*                                                                           */
/*  VERSION/REVISION:                                                        */
/*                                                                           */
/*  DATE/TIME:                                                               */
/*                                                                           */
/*  PURPOSE: WINGEN generated output                                         */
/*                                                                           */
/*  FUNCTIONS:                                                               */
/*                                                                           */
/*      Uses the Windows Multiple Document Interface (MDI) system            */
/*                                                                           */
/*  COMMENTS:                                                                */
/*                                                                           */
/*      Windows can have several copies of your application running at the   */
/*      same time.  The variable hInst keeps track of which instance this    */
/*      application is so that processing will be to the correct window.     */
/*                                                                           */
/*  REQUIRED FILES FOR COMPILATION:                                          */
/*                                                                           */
/*      FILENAME.C      - Source Code                                        */
/*                                                                           */
/*      FILENAME.H      - Programspecific Header                             */
/*      WINDOWS.H       - Windows Applications                               */
/*      MEMORY.H        - Memory manipulation                                */
/*      STDIO.H         - Standard I/O declaration routines                  */
/*      CTYPE.H         - Type declarations for compiler                     */
/*      IO.H            - More I/O declarations                              */
/*      FCNTL.H         - Flag handling for opening files                    */
/*      STDLIB.H        - Miscellaneous basic functions                      */
/*      STRING.H        - String functions                                   */
/*                                                                           */
/*      FILENAME.MAK    - Borland Makefile                                   */
/*      FILENAME.MMK    - Microsoft Makefile                                 */
/*      FILENAME.RC     - Resource File                                      */
/*      FILENAME.DEF    - Module Definition File                             */
/*      FILENAME.DDL    - Raima database definition language file            */
/*      FILENAME.HCD    - POET class definition file                         */
/*                                                                           */
/*      WG-OK1.BMP      - Bitmap files                                       */
/*      WG_OK2.BMP                                                           */
/*      WG_OK3.BMP                                                           */
/*      WG_CNCL1.BMP                                                         */
/*      WG_CNCL2.BMP                                                         */
/*      WG_CNCL3.BMP                                                         */
/*      WG_RTRY1.BMP                                                         */
/*      WG_RTRY2.BMP                                                         */
/*      WG_RTRY3.BMP                                                         */
/*      WG_IGN1.BMP                                                          */
/*      WG_IGN2.BMP                                                          */
/*      WG_IGN3.BMP                                                          */
/*      WG_HELP1.BMP                                                         */
/*      WG_HELP2.BMP                                                         */
/*      WG_HELP3.BMP                                                         */
/*      WG_ABRT1.BMP                                                         */
/*      WG_ABRT2.BMP                                                         */
/*      WG_ABRT3.BMP                                                         */
/*      WG_YES1.BMP                                                          */
/*      WG_YES2.BMP                                                          */
/*      WG_YES3.BMP                                                          */
/*      WG_NO1.BMP                                                           */
/*      WG_NO2.BMP                                                           */
/*      WG_NO3.BMP                                                           */
/*                                                                           */
/*      WG_ICO.ICO      - Icon file                                          */
/*                                                                           */
/*  REQUIRED PROGRAMS FOR COMPILATION:                                       */
/*                                                                           */
/*      C-Compiler                                                           */
/*      Linker                                                               */
/*      Resource Compiler                                                    */
/*                                                                           */
/*****************************************************************************/

HANDLE      hInst;                                                        /* current instance                   */
HWND        hwnd;                                                         /* handle to main window              */
HWND        hwndMDIClient ;                                               /* handle to MDI client window        */
HCURSOR     hCurWait;                                                     /* Hourglass cursor                   */
int         currentformindex = 0 ;                                        /* current form                       */
int         PaintFieldValueOnly = 0 ;                                     /* used for data input                */
char   far  FieldChangeBuffer[256] ;                                      /* used to detect field change events */
int         FieldChangeLocateTableRow = -1 ;                              /* used for table relationships on field changes */
int         FieldChangeLocating = 0 ;                                     /* check if field change              */
int         NoScreenRefresh = 0 ;                                         /* do not paint the screen */

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/

#pragma argsused                                                          /* disable warning for not using unneccessary arguments */
int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
   {
   MSG msg;                                                               /* message                            */

   if (!hPrevInstance)                                                    /* Other instances of app running?    */
      if (!InitApplication(hInstance))                                    /* Initialize shared things           */
         return (FALSE);                                                  /* Exits if unable to initialize      */

   /* Perform initializations that apply to a specific instance              */
   /* (save instance handle and create window)                               */

   if (!InitInstance(hInstance, nCmdShow))                                /* if there's no instance   */
      return (FALSE);                                                     /* exit                     */

   /* Acquire and dispatch messages until a WM_QUIT message is received.     */

   while (GetMessage(&msg, NULL, NULL, NULL))                             /* as long as there's a message */
      {
      if (!TranslateMDISysAccel(hwndMDIClient, &msg))                     /* if message not known */
         {
         TranslateMessage(&msg);                                          /* translate problematic message to readable form */
         DispatchMessage(&msg);                                           /* display problematic message        */
         }
      }
    return (msg.wParam);                                                  /* Returns the value from PostQuitMessage */
   }


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other
        instances of the application are running.  This function performs
        initialization tasks that can be done once for any number of running
        instances.

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass()
        function.  Since all instances of this application use the same window
        class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
   {
   WNDCLASS  wc;

   /* Fill in window class structure with parameters that describe the       */
   /* main window.                                                           */

   wc.style = NULL;                                                       /* Class style(s).                    */
#if !defined(__WC386__)
   wc.lpfnWndProc = MainWndProc;                                          /* Function to retrieve messages for  */
#else
   wc.lpfnWndProc = (WNDPROC)MainWndProc;                                 /* Function to retrieve messages for  */
#endif
                                                                          /*   windows of this class.           */
   wc.cbClsExtra = 0;                                                     /* No per-class extra data.           */
   wc.cbWndExtra = 0;                                                     /* No per-window extra data.          */
   wc.hInstance = hInstance;                                              /* Application that owns the class.   */
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);                            /* load icon                */
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);                              /* load cursor (arrow)      */
   wc.hbrBackground = GetStockObject(WHITE_BRUSH);                        /* backgrd white  */
   wc.lpszMenuName =  "Form54Menu";   /* Name of menu resource in .RC file. */
   wc.lpszClassName = "WinExeWClass";                                     /* Name used in call to CreateWindow. */

   if (!RegisterClass(&wc))                                               /* register window class, if unsuccessful */
      return(FALSE) ;                                                     /* return with failure code           */

   /* Register the MDI child class */

   wc.style         = CS_DBLCLKS ;                                        /* send doubleclicks to window        */
   wc.lpfnWndProc   = (WNDPROC) ChildWndProc ;                            /* direct valid messages to procedure */
   wc.lpszMenuName  = NULL ;                                              /* no default menu                    */
   wc.cbWndExtra    = 4 ;                                                 /* store WinExe form id in 1st four bytes */
   wc.hbrBackground = GetStockObject(WHITE_BRUSH) ;                       /* white background */
   wc.lpszClassName = "WinExeForm" ;                                      /* name of the window class */
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION) ;                           /* identify the class icon  */

   if (!RegisterClass(&wc))                                               /* register window class, if not successful */
      {                                                                   /* unregister window class, free allocated memory */
      UnregisterClass("WinExeWClass", hInst) ;
           return(FALSE) ;                                                /* return with failure code           */
      }
   return(TRUE) ;                                                         /* registration successful            */
   }


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

        This function is called at initialization time for every instance of
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.

        In this case, we save the instance handle in a static variable and
        create and display the main program window.

****************************************************************************/

BOOL InitInstance(HANDLE hInstance, int nCmdShow)
   {
   /* Save the instance handle in static variable, which will be used in  */
   /* many subsequence calls from this application to Windows.            */

   hInst = hInstance;
   hCurWait = LoadCursor(NULL, IDC_WAIT);

   /* Create a main window for this application instance.  */

   hwnd = CreateWindow(
       "WinExeWClass",                                                    /* See RegisterClass() call.          */
#if !defined(__WC386__)
       WindowTitle,                                                       /* Text for window title bar.         */
#else
       (LPSTR)WindowTitle,
#endif

       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,                             /* Window style.            */
       CW_USEDEFAULT,                                                     /* Default horizontal position.       */
       CW_USEDEFAULT,                                                     /* Default vertical position.         */
       CW_USEDEFAULT,                                                     /* Default width.                     */
       CW_USEDEFAULT,                                                     /* Default height.                    */
       NULL,                                                              /* Overlapped windows have no parent. */
       NULL,                                                              /* Use the window class menu.         */
       hInstance,                                                         /* This instance owns this window.    */
       NULL                                                               /* Pointer not needed.                */
   );

   if (!hwnd)                                                             /* if window could not be created     */
       return (FALSE);                                                    /* return with failure code           */

   FieldAllocBuffers() ;                                                  /* allocate memory for field buffers  */

   /* Make the window visible; update its client area; and return "success" */

   ShowWindow(hwnd, nCmdShow);                                            /* Show the window for the 1st time   */

#if !defined(__WC386__)
   FORMSELECT(forms[0].Label) ;                                           /* select primary (1st) form          */
#else
   FORMSELECT((LPSTR)forms[0].Label) ;                                    /* select primary (1st) form          */
#endif

   UpdateWindow(hwnd);                                                    /* Lets see something finally.        */
   SendMessage (hwndMDIClient, WM_MDIACTIVATE, forms[0].hWnd, NULL) ;     /* activate the primary form          */

   if (InitDatabaseEngine())                                              /* intialize the database engine      */
      OpenDatabases() ;                                                   /* open all databases                 */

   InitializeDDEML() ;                                                    /* fire up DDEML (if present)         */

   return (TRUE);                                                         /* return with success code           */
   }

/****************************************************************************

    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages
              HWND:     handle of window
              UINT:     message
              WPARAM:   first message parameter
              LPARAM:   last message parameter

****************************************************************************/

long FAR PASCAL MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
   {
   int i, index ;
   CLIENTCREATESTRUCT ccs;                                                /* info about menu and first MDI child window */
   RECT bigRect ;                                                         /* definition of upper-left and lower-right corner */

   switch (message)                                                       /* identify message                   */
      {
      case WM_CREATE:                                                     /* message: create window             */
         hwnd = hWnd ;                                                    /* remember handle                    */
         ccs.hWindowMenu = NULL ;                                         /* reset menu handle                  */
         ccs.idFirstChild = IDM_idFirstChild ;                            /* set child window identifier of the first child */
              /*  Create the MDI client filling the client area */
         hwndMDIClient = CreateWindow (
                                 "mdiclient",
                                 NULL,
                                 WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                 0,
                                 0,
                                 0,
                                 0,
                                 hWnd,
                                 0xCAC,
                                 hInst,
                                 (LPSTR)&ccs);

         ShowWindow (hwndMDIClient, SW_SHOW);                             /* actibare window for client */
#if !defined(__WC386__)
         GetClientRect(hWnd, (RECT far *) &bigRect) ;                     /* define window size */
#else
         GetClientRect(hWnd, (LPRECT) &bigRect) ;
#endif

         break ;                                                          /* end of window creation   */

      case WM_COMMAND:                                                    /* message: command from application menu */
         index = FindIndex(MENUBAR_TYPE, (int) wParam) ;                  /* load index     */
         if (index >= 0 && menubars[index].id == wParam)                  /* if menu click occured */
            {
            break ;
            }
         switch(wParam)                                                   /* identify parameter                 */
            {
            case IDM_WINEXE_EXIT:                                         /* if wParam = 9999 (Exit-Code)       */
               DestroyWindow(hwnd);                                       /* close window                       */
               break;
            default:
                    /* send other messages to default processing, to get message out of queue */
               return (DefFrameProc(hWnd, hwndMDIClient, message, wParam, lParam)) ;
            }

      case WM_DESTROY:                                                    /* message: window being destroyed    */
         CloseDatabases() ;                                               /* close databases                    */
         CleanUpDDEML() ;                                                 /* shutdown DDE service */
         for (i = 0 ; i < BITMAP_COUNT ; i++)                             /* for all bitmaps          */
            if (bitmaps[i].handle != NULL)                                /* if handle still exists   */
               {
               DeleteObject(bitmaps[i].handle) ;                          /* delete bitmaps           */
               bitmaps[i].handle = NULL ;                                 /* reset handle             */
               }
         for (i = 0 ; i < BUTTON_COUNT ; i++)                             /* delete all bitmaps on buttons */
            if (buttons[i].handle != NULL)                                /* if button handle exists  */
               {
               DeleteObject(buttons[i].handle) ;                          /* delete button            */
               buttons[i].handle = NULL ;                                 /* reset button handle      */
               }
         for (i = 0 ; i < FIELD_COUNT ; i++)                             /* delete all field brush handles */
            if (fields[i]->BkgBrush != NULL)
               {
               DeleteObject(fields[i]->BkgBrush) ;
               fields[i]->BkgBrush = NULL ;
               }
         PostQuitMessage(0) ;
         break;

#if defined(NO_ERROR)                /* is Faircom c-tree Plus in use ? */
      case WM_ACTIVATEAPP:
         if( wParam )
            SetCtreeGV( ctWNGV );    /* Set the ctree global variables pntr */
         break;
#endif

      default:                                                            /* Passes it on if unproccessed       */
         return (DefFrameProc(hWnd, hwndMDIClient, message, wParam, lParam)) ;
      }
   return (NULL);                                                         /* return to calling function         */
   }


/*****************************************************************************

    FUNCTION: ChildWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for child window
              HWND:     handle of window
              UINT:     message
              WPARAM:   first message parameter
              LPARAM:   last message parameter

*****************************************************************************/

long FAR PASCAL ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
   {
   int newid, index, i, j, tblindex, rc, fldid, flindex ;
   long sumcheck ;
   HWND hWndAct ;
   HMENU newmenu ;
   char buffer[81] ;
   POINT pt ;
   int xscroll, yscroll, xmaxscroll, ymaxscroll, upscroll, xrange, yrange ;
   RECT bigRect ;
   DWORD sel ;

   switch (message)                                                       /* identify message                   */
      {
      case WM_CREATE:                                                     /* message: create child window       */
         ShowScrollBar(hWnd, SB_HORZ, FALSE) ;                            /* hide horizontal scrollbar */
         ShowScrollBar(hWnd, SB_VERT, FALSE) ;                            /* hide vertical scrollbar   */
         SetFocus(hWnd) ;                                                 /* send all subsequent keyboard input to this window */
         ShowWindow(hWnd, SW_SHOWNORMAL) ;                                /* set window visibility     */
         SetWindowLong(hWnd, 0, (LONG) forms[currentformindex].id) ;      /* set window id */
         break ;

      case WM_MDIACTIVATE:                                                /* message: activate different child window */
         if ((WORD) wParam == 0)                                          /* only process if I am becoming the active window */
            break ;                                                       /* end, I can't be a different one    */
         hWndAct = (HWND) LOWORD(lParam) ;                                /* get lower 16-bits of last message parameter */
         newid = -1 ;                                                     /* reset newid                        */
         if (hWndAct != (HWND) NULL)                                      /* if the 16 bits are not empty       */
            {
            /* get the first longword of the extra window memory as newid */
            newid = (int) GetWindowLong(hWndAct, 0) ;
            }
         if (newid > 0)                                                   /* if the extra window memory has a valid entry */
            {
            currentformindex = FindIndex(FORM_TYPE, newid) ;              /* get the appropriate form */
            wsprintf((LPSTR) buffer, (LPSTR) "Form%dMenu", newid) ;       /* write name in buffer */
            newmenu = LoadMenu(hInst, (LPCSTR) buffer) ;                  /* set up menu for this form */
            if (newmenu != NULL)                                          /* if setup is successful   */
               {
               DestroyMenu(GetMenu(hwnd)) ;                               /* destroy former menu      */
               SetMenu(hwnd, newmenu) ;                                   /* install new menu         */
               }
            InvalidateRect(hWnd, NULL, FALSE) ;
            }                                                             /* end child window activation        */
         break ;

      case WM_CTLCOLOR:                                                   /* control color */
         if (HIWORD(lParam) == CTLCOLOR_BTN)
            {
            for (i = 0 ; i < FIELD_COUNT ; i++)                           /* for field list selection items */
               if (fields[i]->hWnd == LOWORD(lParam))                      /* boolean field */
                  {
                  if (fields[i]->BkgBrush == NULL)
                     fields[i]->BkgBrush = CreateSolidBrush(fields[i]->ValueBg) ;
                  SetTextColor((HDC) wParam, fields[i]->ValueFg) ;
                  SetBkColor((HDC) wParam, fields[i]->ValueBg) ;
                  return(fields[i]->BkgBrush) ;
                  }
            for (j = 0 ; j < FIELD_LIST_COUNT ; j++)                      /* for field list selection items */
               if (fieldlists[j].hWnd == LOWORD(lParam))                  /* radio button & check boxes */
                  {
                  i = FindIndex(FIELD_TYPE, fieldlists[j].fieldid) ;
                  if (fields[i]->BkgBrush == NULL)
                     fields[i]->BkgBrush = CreateSolidBrush(fields[i]->ValueBg) ;
                  SetTextColor((HDC) wParam, fields[i]->ValueFg) ;
                  SetBkColor((HDC) wParam, fields[i]->ValueBg) ;
                  return(fields[i]->BkgBrush) ;
                  }
            }
         return (DefMDIChildProc(hWnd, message, wParam, lParam)) ;        /* clear stack */

      case WM_PAINT:                                                      /* message:  repaint window           */
         newid = (int) GetWindowLong(hWnd, 0) ;                           /* get menu type          */
         PaintForm(hWnd, -1, (int) newid) ;                               /* call repaint function  */
         break ;                                                          /* end repaint                        */

      case WM_DRAWITEM:                                                   /* message:  repaint box              */
         index = FindIndex(BUTTON_TYPE, (int) wParam) ;                   /* get button type */
         if (buttons[index].id == wParam)                                 /* if button exists         */
            PaintButton(hWnd, index, (int) wParam, (DRAWITEMSTRUCT FAR *) lParam) ;  /* call repaint function */
         else                                                             /* call default handling to get message off the stack */
            return (DefMDIChildProc(hWnd, message, wParam, lParam));
         break ;                                                          /* end repaint                        */

      case WM_SETFOCUS:                                                   /* message:  gained input focus       */
         /* if edit mode is set (editing allowed) and we're in the correct window */
         if (forms[currentformindex].EditMode && hWnd == forms[currentformindex].hWnd)
            {
            i = forms[currentformindex].curfld ;                          /* remember current field   */
            CreateCaret(hWnd, NULL,
               ((abs(fonts[fields[i]->ValueFont].lfHeight) * .1) < 6 ? abs(fonts[fields[i]->ValueFont].lfHeight) * .1 : 5),
                 abs(fonts[fields[i]->ValueFont].lfHeight));                      /* calculate caret size for fit */
            /* activate new caret */
            SetCaretPos(forms[currentformindex].xcaretpos, forms[currentformindex].ycaretpos) ;
            ShowCaret(hWnd) ;                                             /* display caret                      */
            }
         break ;                                                          /* end focus handling                 */

      case WM_VSCROLL:                                                    /* message: vertical scrollbar clicked */
         if (HIWORD(lParam) == 0)                                         /* if first 16-bit if lParam zeroes   */
            {
            upscroll = 0 ;                                                /* reset upscroll                     */
            GetClientRect(hWnd, &bigRect) ;                               /* retrieve client coordinates */
                /* retrieve current minimum and maximum scroll bar positions */

#if !defined(__WC386__)
            GetScrollRange(hWnd, SB_HORZ, &xscroll, &xmaxscroll) ;        /* horizontal */
            GetScrollRange(hWnd, SB_VERT, &yscroll, &ymaxscroll) ;        /* vertical */
#else
            GetScrollRange(hWnd, SB_HORZ, (LPINT)&xscroll, (LPINT)&xmaxscroll) ;        /* horizontal */
            GetScrollRange(hWnd, SB_VERT, (LPINT)&yscroll, (LPINT)&ymaxscroll) ;        /* vertical */
#endif

                /* get current position of scrollbox */
            xscroll = GetScrollPos(hWnd, SB_HORZ) ;                       /* horizontal     */
            yscroll = GetScrollPos(hWnd, SB_VERT) ;                       /* vertical       */
            switch(wParam)                                                /* identify first message parameter   */
               {
               case SB_TOP:                                               /* if top of scrollbar clicked        */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  SetScrollPos(hWnd, SB_VERT, 0, TRUE) ;                  /* set scrollpos at top */
                  break ;
               case SB_BOTTOM:                                            /* if bottom of scrollbar clicked     */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  SetScrollPos(hWnd, SB_VERT, ymaxscroll, TRUE) ;         /* set scrollpos at bottom */
                  break ;
               case SB_LINEUP:                                            /* if lineup clicked                  */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  yscroll -= 8 ;                                          /* adjust position                    */
                  if (yscroll < 0)                                        /* if you'd pass the top              */
                     yscroll = 0 ;                                        /* stay at top                        */
                  SetScrollPos(hWnd, SB_VERT, yscroll, TRUE) ;            /* set new position */
                  break ;
               case SB_LINEDOWN:                                          /* if linedown clicked                */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  yscroll += 8 ;                                          /* adjust position                    */
                  if (yscroll > ymaxscroll)                               /* if you'd pass the bottom */
                     yscroll = ymaxscroll ;                               /* stay at bottom           */
                  SetScrollPos(hWnd, SB_VERT, yscroll, TRUE) ;            /* set new position */
                  break ;
               case SB_PAGEUP:                                            /* if pageup is clicked               */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  yscroll -= bigRect.bottom ;                             /* adjust position          */
                  if (yscroll < 0)                                        /* if you'd pass the top              */
                     yscroll = 0 ;                                        /* stay at top                        */
                  SetScrollPos(hWnd, SB_VERT, yscroll, TRUE) ;            /* set new position */
                  break ;
               case SB_PAGEDOWN:                                          /* if pagedown is clicked             */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  yscroll += bigRect.bottom ;                             /* adjust position          */
                  if (yscroll > ymaxscroll)                               /* if you'd pass the bottom */
                     yscroll = ymaxscroll ;                               /* stay at bottom           */
                  SetScrollPos(hWnd, SB_VERT, yscroll, TRUE) ;            /* set new position */
                  break ;
               case SB_THUMBPOSITION:                                     /* if scrollbox is moved              */
                  upscroll = 1 ;                                          /* remember: movement occured         */
                  yscroll = LOWORD(lParam) ;                              /* get position (lower 16-bit of last parameter */
                  SetScrollPos(hWnd, SB_VERT, yscroll, TRUE) ;            /* set new position */
                  break ;
               }
            if (upscroll)                                                 /* if movement occured                */
               {
               newid = (int) GetWindowLong(hWnd, 0) ;                     /* get window id  */
                  /* get new current position of scrollbox */
               xscroll = GetScrollPos(hWnd, SB_HORZ) ;                    /* horizontal     */
               yscroll = GetScrollPos(hWnd, SB_VERT) ;                    /* vertical       */
               for (i = 0 ; i < BUTTON_COUNT ; i++)                       /* for all buttons */
                  if (buttons[i].formid == newid)                         /* if existing in that window */
                     {
                     MoveWindow(buttons[i].hWnd,
                                buttons[i].Rect.left - 1 - xscroll,
                                buttons[i].Rect.top - 1 - yscroll,
                                2 + buttons[i].Rect.right - buttons[i].Rect.left,
                                2 + buttons[i].Rect.bottom - buttons[i].Rect.top,
                                FALSE) ;                                  /* move button, too          */
                     }
               for (i = 0 ; i < FIELD_LIST_COUNT ; i++)                   /* for field list selection items */
                  if (fieldlists[i].formid == newid && fieldlists[i].hWnd != NULL)
                     {
                     MoveWindow(fieldlists[i].hWnd,
                                fieldlists[i].Rect.left - xscroll,
                                fieldlists[i].Rect.top - yscroll,
                                fieldlists[i].Rect.right,
                                fieldlists[i].Rect.bottom,
                                FALSE) ;
                     }
               for (i = 0 ; i < FIELD_COUNT ; i++)                   /* for field list selection items */
                  if (fields[i]->formid == newid && fields[i]->hWnd != NULL)
                     {
                     MoveWindow(fields[i]->hWnd,
                                fields[i]->Rect.left - xscroll,
                                fields[i]->Rect.top - yscroll,
                                1 + fields[i]->Rect.right - fields[i]->Rect.left,
                                1 + fields[i]->Rect.bottom - fields[i]->Rect.top,
                                FALSE) ;
                     }
               InvalidateRect(hWnd, &bigRect, TRUE) ;
               UpdateWindow(hWnd) ;                                       /* force window update       */
               }
            }
         else                                                            /* table scroll bar (first 16 bits of lParam not zeroes) */
            {
            for (i = 0 ; i < TABLE_COUNT ; i++)                           /* for all tables            */
               if (tables[i].ScrollhWnd == HIWORD(lParam))
                  break ;                                                 /* check which table was clicked       */
            if (i >= TABLE_COUNT)                                         /* if none, do nothing                 */
               break ;
            /* get current position of scrollbar */
            yscroll = GetScrollPos(tables[i].ScrollhWnd, SB_CTL) ;
            j = 0 ;                                                       /* reset internal index (erase possible "garbage" value of variable) */
            while (databases[j].name != NULL)                             /* if there are databasenames */
               {
#if !defined(__WC386__)
               if (IsSameFile(databases[j].name, tables[i].Filename))
#else
               if (IsSameFile((LPSTR)databases[j].name, (LPSTR)tables[i].Filename))
#endif
                  break ;                                                 /* check if the name is a filename, if so, search no longer */
#if !defined(__WC386__)
               if (IsSameFile(databases[j].link, tables[i].Filename))
#else
               if (IsSameFile((LPSTR)databases[j].link, (LPSTR)tables[i].Filename))
#endif
                  break ;                                                 /* check if the link is a filename, if so, search no longer */
               j++ ;                                                      /* go to the next name                */
               }
            databases[j].recordcount = RecordCount(j) ;                   /* get number of records */
            if (databases[j].recordcount < 1)                             /* if there is no record    */
               databases[j].recordcount = 1 ;                             /* let it have one          */
            switch(wParam)                                                /* identify first message parameter   */
               {
               case SB_TOP:                                               /* if top of scrollbar is clicked     */
                  SetScrollPos(tables[i].ScrollhWnd, SB_CTL, 1, TRUE) ;   /* set new position */
#if !defined(__WC386__)
                  TOP(tables[i].Filename) ;                               /* get to top of database   */
#else
                  TOP((LPSTR)tables[i].Filename) ;                               /* get to top of database   */
#endif
                  break ;
               case SB_BOTTOM:                                            /* if bottom of scrollbar is clicked  */
                  SetScrollPos(tables[i].ScrollhWnd, SB_CTL, (int) databases[j].recordcount, TRUE) ;  /* set new position */
#if !defined(__WC386__)
                  BOTTOM(tables[i].Filename) ;                            /* get to bottom of database */
#else
                  BOTTOM((LPSTR)tables[i].Filename) ;                            /* get to bottom of database */
#endif
                  break ;
               case SB_LINEUP:                                            /* if lineup is clicked               */
                  yscroll-- ;                                             /* reduce linecounter by one          */
                  if (yscroll < 1)                                        /* if linecounter < 1                 */
                     yscroll = 1 ;                                        /* set it to one                      */
                  SetScrollPos(tables[i].ScrollhWnd, SB_CTL, yscroll, TRUE) ;   /* set new position */
#if !defined(__WC386__)
                  PREVIOUS(tables[i].Filename) ;                          /* get the previous record of the database */
#else
                  PREVIOUS((LPSTR)tables[i].Filename) ;                          /* get the previous record of the database */
#endif
                  break ;
               case SB_LINEDOWN:                                          /* if linedown is clicked             */
                  yscroll++ ;                                             /* increase linecounter by one        */
                  if (yscroll > databases[j].recordcount)                 /* if linecounter greater than recordcount */
                     yscroll = (int) databases[j].recordcount ;           /* set linecounter to recordcount */
                  SetScrollPos(tables[i].ScrollhWnd, SB_CTL, yscroll, TRUE) ;   /* set new position */
#if !defined(__WC386__)
                  NEXT(tables[i].Filename) ;                              /* get next record of database */
#else
                  NEXT((LPSTR)tables[i].Filename) ;                              /* get next record of database */
#endif
                  break ;
               case SB_PAGEUP:                                            /* if pageup is clicked               */
                  yscroll -= tables[i].Rows - 1 ;                         /* decrease linecounter by number of displayed lines -1 */
                  if (yscroll < 1)                                        /* if linecounter < 1                 */
                     yscroll = 1 ;                                        /* set it to 1                        */
                  SetScrollPos(tables[i].ScrollhWnd, SB_CTL, yscroll, TRUE) ;  /* set new position */
#if !defined(__WC386__)
                  PAGEUP(tables[i].Filename) ;                            /* get previous "page" of database */
#else
                  PAGEUP((LPSTR)tables[i].Filename) ;                            /* get previous "page" of database */
#endif
                  break ;
               case SB_PAGEDOWN:                                          /* if pagedown is clicked             */
                  yscroll += tables[i].Rows - 1 ;                         /* increase linecounter by number of displayed lines -1 */
                  if (yscroll > databases[j].recordcount)                 /* if linecounter greater than recordcount */
                     yscroll = (int) databases[j].recordcount ;           /* set linecounter to recordcount */
                  SetScrollPos(tables[i].ScrollhWnd, SB_CTL, yscroll, TRUE) ;  /* set new position */
#if !defined(__WC386__)
                  PAGEDN(tables[i].Filename) ;                            /* get next "page" of database */
#else
                  PAGEDN((LPSTR)tables[i].Filename) ;                            /* get next "page" of database */
#endif
                  break ;
               case SB_THUMBPOSITION:                                     /* if scrollbox clicked    */
                  SetScrollPos(hWnd, SB_CTL, LOWORD(lParam), TRUE) ;      /* set new position */
                  while (yscroll < LOWORD(lParam))                        /* as long linecounter smaller than new line */
                     {
                     yscroll++ ;                                          /* increase linecounter by one       */
#if !defined(__WC386__)
                     NEXT(tables[i].Filename) ;                           /* get next record of the database */
#else
                     NEXT((LPSTR)tables[i].Filename) ;                           /* get next record of the database */
#endif
                     }
                  while (yscroll > LOWORD(lParam))                        /* as long linecounter greater than new line */
                     {
                     yscroll-- ;                                          /* decrease linecounter by one       */
#if !defined(__WC386__)
                     PREVIOUS(tables[i].Filename) ;                       /* get previous record of database */
#else
                     PREVIOUS((LPSTR)tables[i].Filename) ;                       /* get previous record of database */
#endif
                     }
                  break ;
               }
            }
         break ;

      case WM_HSCROLL:                                                    /* message: horizontal scroll bar clicked */
         upscroll = 0 ;                                                   /* nothing moved (yet)                */
         GetClientRect(hWnd, &bigRect) ;                                  /* retrieve client coordinates */
                                                                          /* retrieve minimum and maximum scrollbar positions */
#if !defined(__WC386__)
         GetScrollRange(hWnd, SB_HORZ, &xscroll, &xmaxscroll) ;           /* horizontal */
         GetScrollRange(hWnd, SB_VERT, &yscroll, &ymaxscroll) ;           /* vertical */
#else
         GetScrollRange(hWnd, SB_HORZ, (LPINT)&xscroll, (LPINT)&xmaxscroll) ;           /* horizontal */
         GetScrollRange(hWnd, SB_VERT, (LPINT)&yscroll, (LPINT)&ymaxscroll) ;           /* vertical */
#endif
                                                                          /* get current position of scrollbar */
         xscroll = GetScrollPos(hWnd, SB_HORZ) ;                          /* horizontal     */
         yscroll = GetScrollPos(hWnd, SB_VERT) ;                          /* vertical       */
         switch(wParam)                                                   /* identify first message parameter   */
            {
            case SB_LEFT:                                                 /* if left of scrollbar clicked       */
               upscroll = 1 ;                                             /* remember: movement occured         */
               SetScrollPos(hWnd, SB_HORZ, 0, TRUE) ;                     /* go to the very left */
               break ;
            case SB_RIGHT:                                                /* if right of scrollbar clicked      */
               upscroll = 1 ;                                             /* remember: movement occured         */
               SetScrollPos(hWnd, SB_HORZ, xmaxscroll, TRUE) ;            /* go to the very right */
               break ;
            case SB_LINELEFT:                                             /* if lineleft is clicked             */
               upscroll = 1 ;                                             /* remember: movement occured         */
               xscroll -= 16 ;                                            /* adjust position                    */
               if (xscroll < 0)                                           /* if you'd go across the left border */
                  xscroll = 0 ;                                           /* stay at the very left              */
               SetScrollPos(hWnd, SB_HORZ, xscroll, TRUE) ;               /* set new position */
               break ;
            case SB_LINERIGHT:                                            /* if lineright is clicked            */
               upscroll = 1 ;                                             /* remember: movement occured         */
               xscroll += 16 ;                                            /* adjust position                    */
               if (xscroll > xmaxscroll)                                  /* if you'd go across the right border */
                  xscroll = xmaxscroll ;                                  /* stay at the very right   */
               SetScrollPos(hWnd, SB_HORZ, xscroll, TRUE) ;               /* set new position */
               break ;
            case SB_PAGELEFT:                                             /* if pageleft is clicked             */
               upscroll = 1 ;                                             /* remember: movement occured         */
               xscroll -= bigRect.right ;                                 /* adjust position                    */
               if (xscroll < 0)                                           /* if you'd go across the left border */
                  xscroll = 0 ;                                           /* stay at the left border            */
               SetScrollPos(hWnd, SB_HORZ, xscroll, TRUE) ;               /* set new position */
               break ;
            case SB_PAGERIGHT:                                            /* if pageright is clicked */
               upscroll = 1 ;                                             /* remember: movement occured         */
               xscroll += bigRect.right ;                                 /* adjust position */
               if (xscroll > xmaxscroll)                                  /* if you'd go across the right border */
                  xscroll = xmaxscroll ;                                  /* stay at the right border */
               SetScrollPos(hWnd, SB_HORZ, xscroll, TRUE) ;               /* set new position */
               break ;
            case SB_THUMBPOSITION:                                        /* if scrollbox is moved */
               upscroll = 1 ;                                             /* remember: movement occured         */
               xscroll = LOWORD(lParam) ;                                 /* adjust position */
               SetScrollPos(hWnd, SB_HORZ, xscroll, TRUE) ;               /* set new position */
               break ;
            }
         if (upscroll)                                                    /* if movement occured */
            {
            newid = (int) GetWindowLong(hWnd, 0) ;                        /* set newid to window id */
            xscroll = GetScrollPos(hWnd, SB_HORZ) ;                       /* get new current horizontal position */
            yscroll = GetScrollPos(hWnd, SB_VERT) ;                       /* get new current vertical position */
            for (i = 0 ; i < BUTTON_COUNT ; i++)                          /* for all buttons */
               if (buttons[i].formid == newid)                            /* if displayed in that window */
                  {
                  MoveWindow(buttons[i].hWnd,
                             buttons[i].Rect.left - 1 - xscroll,
                             buttons[i].Rect.top - 1 - yscroll,
                             2 + buttons[i].Rect.right - buttons[i].Rect.left,
                             2 + buttons[i].Rect.bottom - buttons[i].Rect.top,
                             FALSE) ;                                     /* move button(s) to new position */
                  }
            for (i = 0 ; i < FIELD_LIST_COUNT ; i++)                   /* for field list selection items */
               if (fieldlists[i].formid == newid && fieldlists[i].hWnd != NULL)
                  {
                  MoveWindow(fieldlists[i].hWnd,
                             fieldlists[i].Rect.left - xscroll,
                             fieldlists[i].Rect.top - yscroll,
                             fieldlists[i].Rect.right,
                             fieldlists[i].Rect.bottom,
                             FALSE) ;
                  }
            for (i = 0 ; i < FIELD_COUNT ; i++)                   /* for field list selection items */
               if (fields[i]->formid == newid && fields[i]->hWnd != NULL)
                  {
                  MoveWindow(fields[i]->hWnd,
                             fields[i]->Rect.left - xscroll,
                             fields[i]->Rect.top - yscroll,
                             1 + fields[i]->Rect.right - fields[i]->Rect.left,
                             1 + fields[i]->Rect.bottom - fields[i]->Rect.top,
                             FALSE) ;
                  }
            InvalidateRect(hWnd, &bigRect, TRUE) ;                        /* delete old buttons */
            UpdateWindow(hWnd) ;                                          /* paint new window */
            }
         break ;

      case WM_LBUTTONDOWN:                                                /* message: left mouse button pressed */
         /* get coordinates and add current scrollbox position */
         pt.x = LOWORD(lParam) + GetScrollPos(hWnd, SB_HORZ) ;            /* x coordinate */
         pt.y = HIWORD(lParam) + GetScrollPos(hWnd, SB_VERT) ;            /* y coordinate */
         for (i = 0 ; i < FIELD_COUNT ; i++)                              /* as long as there are fields */
            {
            if (fields[i]->tableid < 0 && fields[i]->formid == forms[currentformindex].id &&
#if !defined(__WC386__)
                PtInRect(&fields[i]->Rect, pt))                            /* normal field? */
#else
                PtInRect((LPRECT)&fields[i]->Rect, pt))                            /* normal field? */
#endif
               {
               FieldEdit(i) ;                                             /* call editing function */
               break ;
               }
            else if (fields[i]->tableid > 0 && fields[i]->formid == forms[currentformindex].id &&
#if !defined(__WC386__)
                     PtInRect(&fields[i]->ColRect, pt))                    /* table column? */
#else
                     PtInRect((LPRECT)&fields[i]->ColRect, pt))                    /* table column? */
#endif
               {
               tblindex = FindIndex(TABLE_TYPE, fields[i]->tableid) ;      /* go to field */
               fields[i]->ValueRect.top = tables[tblindex].Rect.top +
                                tables[tblindex].TblLblSize + tables[tblindex].ColLblSize +
                                tables[tblindex].LineWidth ;              /* activate field */
               forms[currentformindex].curtblrow =
                    (pt.y - fields[i]->ValueRect.top) /
                    tables[tblindex].AvgRowSize ;                         /* position field in row */
               FieldEdit(i) ;                                             /* call editing function */
               break ;
               }
            }
         if (i >= FIELD_COUNT)                                            /* if there is no field */
            return (DefMDIChildProc(hWnd, message, wParam, lParam));      /* send message to default to get it off stack */
         break ;

      case WM_KEYDOWN:                                                    /* message: key pressed */
         if (forms[currentformindex].EditMode)                            /* if edit active */
            FieldKeyInput(wParam, 0) ;                                    /* process keyvalue in field */
         else                                                             /* if not active */
            return (DefMDIChildProc(hWnd, message, wParam, lParam));      /* send message to default to get it off stack */
         break ;

      case WM_CHAR:                                                       /* message: key pressed and released */
         if (forms[currentformindex].EditMode)                            /* if edit active */
            FieldKeyInput(0, wParam) ;                                    /* process key value in field */
         else                                                             /* if not active */
            return (DefMDIChildProc(hWnd, message, wParam, lParam));      /* send message to default to get it off stack */
         break ;

      case WM_COMMAND:                                                    /* message: command recieved (menu, control message, acc keystroke... */
         index = FindIndex(BUTTON_TYPE, (int) wParam) ;                   /* check for button */
         if (index >= 0 && buttons[index].id == wParam)                   /* if button pressed, and action defined */
            {                                                             /* do something */
            if (buttons[index].id == 117)
               {
               if ((TRUE))
                  {
                  CLEARALL() ;
                  }
               }
            if (buttons[index].id == 118)
               {
               if ((TRUE))
                  {
                  STORE("orders") ;
                  }
               }
            if (buttons[index].id == 119)
               {
               if ((TRUE))
                  {
                  CLEAR("customers") ;
                  FORMSELECT("Customers") ;
                  }
               }
            if (buttons[index].id == 166)
               {
               if ((TRUE))
                  {
                  STORE("customers") ;
                  }
               }
            if (buttons[index].id == 167)
               {
               if ((TRUE))
                  {
                  FORMCLOSE("Customers") ;
                  RESUME() ;
                  }
               }
            if (buttons[index].id == 168)
               {
               if ((TRUE))
                  {
                  DELETE("customers") ;
                  }
               }
            if (buttons[index].id == 169)
               {
               if ((TRUE))
                  {
                  CLEAR("customers") ;
                  }
               }
            if (buttons[index].id == 171)
               {
               if ((TRUE))
                  {
                  NEXT("customers") ;
                  }
               }
            if (buttons[index].id == 172)
               {
               if ((TRUE))
                  {
                  PREVIOUS("customers") ;
                  }
               }
            if (buttons[index].id == 173)
               {
               if ((TRUE))
                  {
                  TOP("customers") ;
                  }
               }
            if (buttons[index].id == 174)
               {
               if ((TRUE))
                  {
                  BOTTOM("customers") ;
                  }
               }
            break ;
            }
         index = FindIndex(FIELD_LIST_TYPE, (int) wParam) ;               /* check for field radio/checkbox button */
         if (index >= 0 && fieldlists[index].id == wParam)                /* if button pressed, and action defined */
            {
            fldid = fieldlists[index].fieldid ;
            flindex = FindIndex(FIELD_TYPE, fldid) ;
            if (fields[flindex]->str == NULL)
               fields[flindex]->str = GlobalLock(fields[flindex]->Buffer) ;     /* lock the buffer */
            if (fields[flindex]->str != NULL)
               {
               if (fields[flindex]->FieldType == WINEXE_FLD_RADIO)         /* radio button */
                  {
                  if (SendMessage(fieldlists[index].hWnd, BM_GETCHECK, 0, 0L))        /* button is set */
#if !defined(__WC386__)
                     lstrcpy(fields[flindex]->str, fieldlists[index].Label) ;            /* set field contents to button text */
#else
                     lstrcpy((LPSTR)fields[flindex]->str, (LPSTR)fieldlists[index].Label) ;            /* set field contents to button text */
#endif
                  }
               if (fields[flindex]->FieldType == WINEXE_FLD_CHECK)         /* check box */
                  {
                  j = 0 ;
                  sumcheck = 0L ;
                  for (i = 0 ; i < FIELD_LIST_COUNT ; i++)                            /* as long as there are fields */
                     {
                     if (fieldlists[index].fieldid == fieldlists[i].fieldid)          /* same field */
                        {
                        if (SendMessage(fieldlists[i].hWnd, BM_GETCHECK, 0, 0L))      /* button is set */
                           sumcheck += pow(2, j) ;
                        j++ ;
                        }
                     }
                  sprintf(fields[flindex]->str, "%ld", sumcheck) ;
                  }
               GlobalUnlock(fields[flindex]->Buffer) ;
               fields[flindex]->str = NULL ;
               }
            break ;
            }
         index = FindIndex(FIELD_TYPE, (int) wParam) ;                  /* check for field listbox/combox/boolean button */
         if (index >= 0 && fields[index]->id == wParam)                  /* if control pressed, and action defined */
            {
            if (fields[index]->str == NULL)
               fields[index]->str = GlobalLock(fields[index]->Buffer) ;     /* lock the buffer */
            if (fields[index]->str != NULL)
               {
               if (fields[index]->FieldType == WINEXE_FLD_COMBO)          /* combo box */
                  {
                  if (HIWORD(lParam) == CBN_SELCHANGE || HIWORD(lParam) == CBN_CLOSEUP)
                     {
                     sel = SendMessage(fields[index]->hWnd, CB_GETCURSEL, 0, 0L) ;
                     if (sel != CB_ERR)
                        SendMessage(fields[index]->hWnd, CB_SETCURSEL, (WPARAM) sel, 0L) ;
                     }
                  SendMessage(fields[index]->hWnd, WM_GETTEXT, (WPARAM) fields[index]->Length, (LPARAM) ((LPCSTR) fields[index]->str)) ;
                  }
               if (fields[index]->FieldType == WINEXE_FLD_SELECT)          /* listbox */
                  {
                  sel = SendMessage(fields[index]->hWnd, LB_GETCURSEL, 0, 0L) ;
                  if (sel != LB_ERR)
                     SendMessage(fields[index]->hWnd, LB_GETTEXT, (WPARAM) sel, (LPARAM) ((LPCSTR) fields[index]->str)) ;
                  }
               if (fields[index]->FieldType == WINEXE_FLD_BOOL)          /* check box */
                  {
                  if (SendMessage(fields[index]->hWnd, BM_GETCHECK, 0, 0L))
                     lstrcpy(fields[index]->str, "Y") ;
                  else
                     lstrcpy(fields[index]->str, "N") ;
                  }
               if (fields[index]->FieldType == WINEXE_FLD_SELECT && (HIWORD(lParam) == LBN_DBLCLK || HIWORD(lParam) == LBN_SELCHANGE))
                  {
                  FieldEditEnd(index, 0) ;
                  }
               else if (fields[index]->FieldType == WINEXE_FLD_COMBO && HIWORD(lParam) == CBN_CLOSEUP)
                  {
                  FieldEditEnd(index, 0) ;
                  }
               else
                  {
                  GlobalUnlock(fields[index]->Buffer) ;
                  fields[index]->str = NULL ;
                  }
               }
            break ;
            }
         return (DefMDIChildProc(hWnd, message, wParam, lParam));         /* send undefined message to default to get it off stack */

      case WM_SIZE:                                                       /* message: window size changed */
         bigRect.top = 0 ;                                                /* set top of window to position 0 (top) */
         bigRect.left = 0 ;                                               /* set left of window to position 0 (left) */
         bigRect.bottom = HIWORD(lParam) ;                                /* set bottom of window to new position */
         bigRect.right = LOWORD(lParam) ;                                 /* set right of window to new position */
         xscroll = GetScrollPos(hWnd, SB_HORZ) ;                          /* get current horizontal position of scrollbox */
         yscroll = GetScrollPos(hWnd, SB_VERT) ;                          /* get current vertical position of scrollbox */
         if (forms[currentformindex].yMaxPix > bigRect.bottom)            /* if formheight greater than new windowheight */
            {
#if  !defined(__WC386__)
            GetScrollRange(hWnd, SB_VERT, &rc, &yrange) ;                 /* get old scrollbarposition */
#else
            GetScrollRange(hWnd, SB_VERT, (LPINT)&rc, (LPINT)&yrange) ;                 /* get old scrollbarposition */
#endif
            SetScrollRange(hWnd, SB_VERT, 0, forms[currentformindex].yMaxPix - bigRect.bottom, FALSE) ;  /* update scrollbarposition */
            if (yrange < 1 || yscroll < 1)                                /* if you are at top of form */
               SetScrollPos(hWnd, SB_VERT, 0, TRUE) ;                     /* set new scrollbox position at top */
            else                                                          /* if not, calculate new position and set it */
               SetScrollPos(hWnd, SB_VERT, ((forms[currentformindex].yMaxPix - bigRect.bottom) * yscroll) / yrange, TRUE) ;
            }
         else                                                             /* if formheight smaller than new windowheight */
            {
            SetScrollPos(hWnd, SB_VERT, 0, FALSE) ;                       /* set scrollbox at top, don't bother to draw */
            ShowScrollBar(hWnd, SB_VERT, FALSE) ;                         /* don't bother to show a scrollbar */
            }
         if (forms[currentformindex].xMaxPix > bigRect.right)             /* if formwidth greater than new windowwidth */
            {
#if !defined(__WC386__)
            GetScrollRange(hWnd, SB_HORZ, &rc, &xrange) ;                 /* get old scrollbarposition */
#else
            GetScrollRange(hWnd, SB_HORZ, (LPINT)&rc, (LPINT)&xrange) ;                 /* get old scrollbarposition */
#endif
            SetScrollRange(hWnd, SB_HORZ, 0, forms[currentformindex].xMaxPix - bigRect.right, FALSE) ;  /* update scrollbarposition */
            if (xrange < 1 || xscroll < 1)                                /* if you are at the very left of the form */
               SetScrollPos(hWnd, SB_HORZ, 0, TRUE) ;                     /* set new scrollboxposition at the very left */
            else                                                          /* if not, calculate and set new scrollboxposition */
               SetScrollPos(hWnd, SB_HORZ, ((forms[currentformindex].xMaxPix - bigRect.right) * xscroll) / xrange, TRUE) ;
            }
         else                                                             /* if formwidth smaller than new windowwidth */
            {
            SetScrollPos(hWnd, SB_HORZ, 0, FALSE) ;                       /* set scrollbox to left, don't bother to draw */
            ShowScrollBar(hWnd, SB_HORZ, FALSE) ;                         /* don't bother to show a scrollbar */
            }
         InvalidateRect(hWnd, &bigRect, TRUE) ;                           /* delete old window */
         return (DefMDIChildProc(hWnd, message, wParam, lParam)) ;        /* clear stack */

      case WM_CLOSE:
         for (i = 0 ; i < FORM_COUNT ; i++)
            {
            if (forms[i].hWnd == hWnd)
               {
#if !defined(__WC386__)
               GetWindowRect(hwndMDIClient, (RECT far *) &bigRect) ;
               GetWindowRect(forms[i].hWnd, (RECT far *) &forms[i].PosRect) ;
#else
               GetWindowRect(hwndMDIClient, (LPRECT) &bigRect) ;
               GetWindowRect(forms[i].hWnd, (LPRECT) &forms[i].PosRect) ;
#endif
               forms[i].PosRect.left -= bigRect.left ;
               forms[i].PosRect.top -= bigRect.top ;
               forms[i].PosRect.right -= bigRect.left ;
               forms[i].PosRect.bottom -= bigRect.top ;
               forms[i].hWnd = NULL ;
               break ;
               }
            }
         return (DefMDIChildProc(hWnd, message, wParam, lParam)) ;

      case WM_MOVE:                                                       /* message: window has been moved */
         InvalidateRect(hWnd, NULL, FALSE) ;                              /* fall thru (delete old window) */
         /* fall thru */
      case WM_CHILDACTIVATE:                                              /* message: window activated, moved, sized */
      case WM_GETMINMAXINFO:                                              /* message: window minimal or maximal size */
      case WM_MENUCHAR:                                                   /* message: undefined mnemonic menu character key pressed */
      case WM_SYSCOMMAND:                                                 /* message: system menu command selected */
      default:                                                            /* message not recognizable */
         return (DefMDIChildProc(hWnd, message, wParam, lParam)) ;        /* clear stack */
      }
   return (NULL);                                                         /* end handling of child window messages */
   }


/*****************************************************************************

   FUNCTION:    FindIndex (type, id)

   PURPOSE:     find array index based on unique identifier
                type:   type
                id:     menu index

*****************************************************************************/

int FindIndex(int type, int id)        /* find array index based on unique identifier */
   {
   int index ;

   switch(type)                                                           /* identify type */
      {
      case FORM_TYPE:                                                     /* if it's a form */
         for (index = 0 ; index < FORM_COUNT ; index++)                   /* check all form types */
            if (forms[index].id == id)                                    /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case BITMAP_TYPE:                                                   /* it's a bitmap */
         for (index = 0 ; index < BITMAP_COUNT ; index++)                 /* check all bitmap types */
            if (bitmaps[index].id == id)                                  /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case BUTTON_TYPE:                                                   /* it's a button */
         for (index = 0 ; index < BUTTON_COUNT ; index++)                 /* check all button types */
            if (buttons[index].id == id)                                  /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case TEXT_TYPE:                                                     /* it's text */
         for (index = 0 ; index < TEXT_COUNT ; index++)                   /* check all text types */
            if (text[index].id == id)                                     /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case RECT_TYPE:                                                     /* it's a rectangle */
         for (index = 0 ; index < RECT_COUNT ; index++)                   /* check all rectangle types */
            if (rects[index].id == id)                                    /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case LINE_TYPE:                                                     /* it's a line */
         for (index = 0 ; index < LINE_COUNT ; index++)                   /* check all line types */
            if (lines[index].id == id)                                    /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case FIELD_TYPE:                                                    /* it's a field */
         for (index = 0 ; index < FIELD_COUNT ; index++)                  /* checl all field types */
            if (fields[index]->id == id)                                   /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case FIELD_LIST_TYPE:                                               /* it's a field list */
         for (index = 0 ; index < FIELD_LIST_COUNT ; index++)             /* chekc all field lists */
            if (fieldlists[index].id == id)                               /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case TABLE_TYPE:                                                    /* it's a table */
         for (index = 0 ; index < TABLE_COUNT ; index++)                  /* check all table types */
            if (tables[index].id == id)                                   /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      case MENUBAR_TYPE:                                                  /* it's a menubar */
         for (index = 0 ; index < MENUBAR_COUNT ; index++)                /* chekc all menubar types */
            if (menubars[index].id == id)                                 /* if form corresponds menu type */
               return (index) ;                                           /* return with index value to calling function */
         break ;
      }

   return(0) ;                                                            /* return with failure message to calling function */
   }


void FillSelectionBox(int flidx)                                          /* fill this fields selection box */
   {
   int dbidx, rc ;
   long currec, count, i ;
   LPSTR dbfname ;
   HCURSOR hOldCur ;

   if (fields[flidx]->str == NULL)
      fields[flidx]->str = GlobalLock(fields[flidx]->Buffer) ;
#if !defined(__WC386__)
   lstrcpy(FieldChangeBuffer, fields[flidx]->str) ;                        /* save to detect field change events */
#else
   lstrcpy((LPSTR)FieldChangeBuffer, (LPSTR)fields[flidx]->str) ;                        /* save to detect field change events */
#endif

   if (!fields[flidx]->automatic)
      {
      if (fields[flidx]->str != NULL && fields[flidx]->FieldType == WINEXE_FLD_SELECT)
         {
         rc = (int) SendMessage(fields[flidx]->hWnd, LB_FINDSTRING, -1, (LPARAM) (LPCSTR) fields[flidx]->str) ;
         if (rc != LB_ERR && rc >= 0)
            SendMessage(fields[flidx]->hWnd, LB_SETCURSEL, (WPARAM) rc, 0L) ;    /* set current selection */
         }
      if (fields[flidx]->str != NULL && fields[flidx]->FieldType == WINEXE_FLD_COMBO)
         {
         rc = (int) SendMessage(fields[flidx]->hWnd, CB_FINDSTRING, -1, (LPARAM) (LPCSTR) fields[flidx]->str) ;
         if (rc != CB_ERR && rc >= 0)
            SendMessage(fields[flidx]->hWnd, CB_SETCURSEL, (WPARAM) rc, 0L) ;    /* set current selection */
         }
      return ;
      }

   if (fields[flidx]->FieldType == WINEXE_FLD_SELECT)
      {
      SendMessage(fields[flidx]->hWnd, LB_RESETCONTENT, 0, 0L) ;
      SendMessage(fields[flidx]->hWnd, LB_ADDSTRING, 0, (LPARAM) (LPCSTR) "<Unknown>") ;
      }
   if (fields[flidx]->FieldType == WINEXE_FLD_COMBO)
      {
      SendMessage(fields[flidx]->hWnd, CB_RESETCONTENT, 0, 0L) ;
      SendMessage(fields[flidx]->hWnd, CB_ADDSTRING, 0, (LPARAM) (LPCSTR) "<Unknown>") ;
      }

   if (fields[flidx]->hWnd == NULL)
      return ;
#if !defined(__WC386__)
   dbfname = fields[flidx]->flds[1] ;
#else
   dbfname = (LPSTR)fields[flidx]->flds[1] ;
#endif
   if (dbfname == NULL)
      return ;

   dbidx = 0 ;
   while (databases[dbidx].name != NULL)
      {
#if !defined(__WC386__)
      if (IsSameFile(databases[dbidx].name, dbfname) ||
          IsSameFile(databases[dbidx].link, dbfname))
#else
      if (IsSameFile((LPSTR)databases[dbidx].name, (LPSTR)dbfname) ||
          IsSameFile((LPSTR)databases[dbidx].link, (LPSTR)dbfname))
#endif
         break ;
      dbidx++ ;
      }
   if (databases[dbidx].name == NULL)
      return ;

   currec = databases[dbidx].recordnumber ;
   count = databases[dbidx].recordcount ;
   if (count < 1L)
      return ;

   hOldCur = SetCursor(hCurWait) ;
   NoScreenRefresh = 1 ;                                                  /* suspend screen painting */
   if (fields[flidx]->FieldType == WINEXE_FLD_SELECT)
      SendMessage(fields[flidx]->hWnd, LB_RESETCONTENT, 0, 0L) ;
   if (fields[flidx]->FieldType == WINEXE_FLD_COMBO)
      SendMessage(fields[flidx]->hWnd, CB_RESETCONTENT, 0, 0L) ;

   TOP(dbfname) ;
   for (i = 0 ; i < count ; i++)
      {
      if (fields[flidx]->str == NULL)
         fields[flidx]->str = GlobalLock(fields[flidx]->Buffer) ;
      if (fields[flidx]->FieldType == WINEXE_FLD_SELECT)
         SendMessage(fields[flidx]->hWnd, LB_ADDSTRING, 0, (LPARAM) (LPCSTR) fields[flidx]->str) ;
      if (fields[flidx]->FieldType == WINEXE_FLD_COMBO)
         SendMessage(fields[flidx]->hWnd, CB_ADDSTRING, 0, (LPARAM) (LPCSTR) fields[flidx]->str) ;
      if (i < count - 1L)       /* do not skip beyond eof */
         NEXT(dbfname) ;
      }

   NoScreenRefresh = 0 ;                                                  /* resume screen painting */

   if (currec > 0L)                                                       /* go back to the current record */
      {
      NoScreenRefresh = 1 ;                                               /* suspend screen painting */
      TOP(dbfname) ;
      while (databases[dbidx].recordnumber != currec && databases[dbidx].recordnumber > 0L)
         NEXT(dbfname) ;
      NoScreenRefresh = 0 ;                                               /* resume screen painting */
      ReadDatabaseBuffer(dbidx, -1) ;    /* update on screen data-entry fields */
      RedrawFields(dbfname) ;
      if (fields[flidx]->FieldType == WINEXE_FLD_SELECT)
         SendMessage(fields[flidx]->hWnd, LB_SETCURSEL, (WPARAM) currec - 1, 0L) ;    /* set current selection */
      if (fields[flidx]->FieldType == WINEXE_FLD_COMBO)
         SendMessage(fields[flidx]->hWnd, CB_SETCURSEL, (WPARAM) currec - 1, 0L) ;    /* set current selection */
      }
   else
      CLEAR(dbfname) ;

   if (fields[flidx]->FieldType == WINEXE_FLD_COMBO)
      SendMessage(fields[flidx]->hWnd, CB_SHOWDROPDOWN, 1, 0L) ;           /* show the drop down list */

   SetCursor(hOldCur) ;                                                   /* re-select old cursor state */
   return ;
   }



/*****************************************************************************

   FUNCTION:    RedrawFields (dbfname)

   PURPOSE:     redraw fields
                dbfname:        database filename

*****************************************************************************/

void RedrawFields(LPSTR dbfname)
   {
   int i ;

   if (NoScreenRefresh)
      return ;

   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
      if (fields[i]->formid != forms[currentformindex].id)                 /* if it's not the correct field */
         continue ;                                                       /* continue search */
      if (dbfname == NULL || IsFieldInFile(i, dbfname, NULL, NULL))       /* if no database filename, or file empty */
#if !defined(__WC386__)
         InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, FALSE) ;     /* delete the rectangle */
#else
         InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, FALSE) ;     /* delete the rectangle */
#endif
      }
   if (!FieldChangeLocating)                                              /* if we're not just looking for a field change */
      {
      PaintFieldValueOnly = WINEXE_PAINTALLFIELDS ;                       /* let's paint all fields */
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update the window */
      PaintFieldValueOnly = 0 ;                                           /* and remember we did it */
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    FieldAllocBuffers

   PURPOSE:     allocate field memory buffers

*****************************************************************************/

void FieldAllocBuffers(void)
   {
   int tblindex, newcolbuffer, i, j ;
   LPHANDLE ph ;
   long dbu ;
   double yratio, xratio ;

   dbu = GetDialogBaseUnits() ;
   if (HIWORD(dbu) != WINEXE_YDBU || LOWORD(dbu) != WINEXE_XDBU)
      {
      xratio = (double) LOWORD(dbu) / (double) WINEXE_XDBU ;
      yratio = (double) HIWORD(dbu) / (double) WINEXE_YDBU ;
      for (i = 0 ; i < FORM_COUNT ; i++)
         ScreenResAdjust(&forms[i].Rect, xratio, yratio) ;
      for (i = 0 ; i < BITMAP_COUNT ; i++)
         ScreenResAdjust(&bitmaps[i].Rect, xratio, yratio) ;
      for (i = 0 ; i < TEXT_COUNT ; i++)
         {
         text[i].Row *= xratio ;
         text[i].Col *= yratio ;
         }
      for (i = 0 ; i < RECT_COUNT ; i++)
         ScreenResAdjust(&rects[i].Rect, xratio, yratio) ;
      for (i = 0 ; i < LINE_COUNT ; i++)
         ScreenResAdjust(&lines[i].Rect, xratio, yratio) ;
      for (i = 0 ; i < FIELD_COUNT ; i++)
         {
         ScreenResAdjust(&fields[i]->Rect, xratio, yratio) ;
         ScreenResAdjust(&fields[i]->ValueRect, xratio, yratio) ;
         ScreenResAdjust(&fields[i]->ColRect, xratio, yratio) ;
         }
      for (i = 0 ; i < TABLE_COUNT ; i++)
         {
         ScreenResAdjust(&tables[i].Rect, xratio, yratio) ;
         tables[i].TblLblSize *= yratio ;
         tables[i].ColLblSize *= yratio ;
         tables[i].AvgRowSize *= yratio ;
         tables[i].AvgColSize *= xratio ;
         for (j = 0 ; j < 256 ; j++)
            ScreenResAdjust(&tables[i].ColRect[j], xratio, yratio) ;
         }
      for (i = 0 ; i < BUTTON_COUNT ; i++)
         ScreenResAdjust(&buttons[i].Rect, xratio, yratio) ;
      i = 0 ;
      while (fonts[i].lfHeight != 0)
         {
         fonts[i].lfWidth *= xratio ;
         fonts[i].lfHeight *= yratio ;
         i++ ;
         }
      }

   for (i = 0 ; i < FIELD_COUNT ; i++)                                    /* for all fields */
      {
      if (fields[i]->tableid > 0)                                          /* is this a table column field ? */
         {
         tblindex = FindIndex(TABLE_TYPE, fields[i]->tableid) ;            /* set table index */
         if (forms[currentformindex].curtblrow < 0)                       /* if not in row 0 */
            forms[currentformindex].curtblrow = 0 ;                       /* start in row 0 of table */
         fields[i]->ValueRect.top = tables[tblindex].Rect.top +
                                   tables[tblindex].TblLblSize + tables[tblindex].ColLblSize +
                                   tables[tblindex].LineWidth +
                                   (forms[currentformindex].curtblrow *
                                    tables[tblindex].AvgRowSize) ;        /* check size of table (top) */
         fields[i]->ValueRect.bottom = fields[i]->ValueRect.top + tables[tblindex].AvgRowSize
                                      - (1 + tables[tblindex].LineWidth) ;     /* (bottom) */
         if (fields[i]->str != NULL)                                       /* if field string has a content */
            GlobalUnlock(fields[i]->Buffer) ;                              /* unlock buffer */
         fields[i]->str = NULL ;                                           /* delete string */
         newcolbuffer = 0 ;                                               /* reset new columnbuffer switch */
         if (fields[i]->ColBuffer == NULL)                                 /* if buffer not existent */
            {
            fields[i]->ColBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                              tables[tblindex].Rows * sizeof(HANDLE)) ;  /* allocate memory for array column buffer */
            newcolbuffer = 1 ;                                            /* remember: it's a new buffer */
            }
         if (fields[i]->ColBuffer == NULL)                                 /* if buffer still not existent */
            {
            forms[currentformindex].EditMode = 0 ;                        /* don't allow editing */
            MessageBox(NULL, "Unable to allocate memory for Array Column buffer.", NULL, MB_ICONEXCLAMATION | MB_OK) ; /* display error message */
            return ;                                                      /* return to calling function */
            }
         ph = (LPHANDLE) GlobalLock(fields[i]->ColBuffer) ;                /* memory locked? */
         if (ph == NULL)                                                  /* if not */
            {
            GlobalFree(fields[i]->ColBuffer) ;                             /* free the memory */
            forms[currentformindex].EditMode = 0 ;                        /* don't allow editing any more */
            MessageBox(NULL, "Unable to lock memory for Array Column buffer.",
                       NULL, MB_ICONEXCLAMATION | MB_OK) ;                /* display error message */
            return ;                                                      /* return to calling function */
            }
         if (newcolbuffer)                                                /* first time in this arary column ? */
            {
            for (j = 0 ; j < tables[tblindex].Rows ; j++)                 /* get memory for each row value in this column */
               {
               if (fields[i]->Length < 1)                                  /* if we don't know how long fields are */
                  fields[i]->Length = 40 ;                                 /* let's use 40 */
               ph[j] = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, fields[i]->Length + 1) ;     /* allocate memory for that */
               if (ph[j] == NULL)                                         /* if memory couldn't be allocated */
                  {
                  forms[currentformindex].EditMode = 0 ;                  /* don't allow editing */
                  MessageBox(NULL, "Unable to allocate memory for field buffer in Array Column.",
                             NULL, MB_ICONEXCLAMATION | MB_OK) ;          /* display error message */
                  return ;                                                /* return to calling function */
                  }
               }
            }
         fields[i]->Buffer = ph[forms[currentformindex].curtblrow] ;       /* remember the number of fields */
         GlobalUnlock(fields[i]->ColBuffer) ;                              /* unlock the column buffer memory */
         }
      else                                                                /* so it's not a table column field */
         forms[currentformindex].curtblrow = -1 ;                         /* remember that */

      if (fields[i]->Buffer == NULL)                                       /* if field in buffer is non-existent */
         {
         if (fields[i]->Length < 1)                                        /* if we don't know the length */
            fields[i]->Length = 40 ;                                       /* let's assume 40 */
         fields[i]->Buffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, fields[i]->Length + 1) ;     /* allocate memory for it */
         }
      if (fields[i]->Buffer == NULL)                                       /* if still not existent */
         {
         forms[currentformindex].EditMode = 0 ;                           /* don't allow editing */
         MessageBox(NULL, "Unable to allocate memory for field buffer.", NULL, MB_ICONEXCLAMATION | MB_OK) ; /* display error message */
         return ;                                                         /* return to calling function */
         }
      }
   return ;                                                               /* return to calling function */
   }

/* adjust rectangle for screen resolution */
void ScreenResAdjust(RECT far *p, double xratio, double yratio)
   {
   double numb ;

   numb = p->left ;
   numb *= xratio ;
   numb += 0.5 ;
   p->left = numb ;

   numb = p->top ;
   numb *= yratio ;
   numb += 0.5 ;
   p->top = numb ;

   if (p->right > 0)
      {
      p->right-- ;
      numb = p->right ;
      numb *= xratio ;
      numb += 0.5 ;
      p->right = numb ;
      p->right++ ;
      }

   if (p->bottom > 0)
      {
      p->bottom-- ;
      numb = p->bottom ;
      numb *= yratio ;
      numb += 0.5 ;
      p->bottom = numb ;
      p->bottom++ ;
      }

   return ;
   }


/*****************************************************************************

   FUNCTION:    FieldEdit (i)

   PURPOSE:     allow the operator to edit a field
                i:      field

*****************************************************************************/

void FieldEdit(int i)                  /* allow the operator to edit a field */
   {
   int pos ;
   char mybuf[81] ;
   double numb ;
   int tblindex ;
   LPHANDLE ph ;

   forms[currentformindex].curfld = -1 ;                                  /* reset the current field pointer */
   forms[currentformindex].EditMode = 0 ;                                 /* disable edit mode */
   if (fields[i]->formid == forms[currentformindex].id)                    /* are we in the correct field? */
      forms[currentformindex].curfld = i ;                                /* save this field index */
   else
      return ;                                                            /* return to calling function */

   if (fields[i]->FieldType == WINEXE_FLD_CHECK)
      return ;
   if (fields[i]->FieldType == WINEXE_FLD_RADIO)
      return ;
   if (fields[i]->FieldType == WINEXE_FLD_BOOL)
      return ;
   if (fields[i]->FieldType == WINEXE_FLD_SELECT)
      {
      if (fields[i]->hWnd != NULL)
         {
         DestroyCaret() ;                                                       /* no longer caret */
         FillSelectionBox(i) ;
         ShowWindow(fields[i]->hWnd, SW_SHOW) ;
         SetFocus(fields[i]->hWnd) ;
         }
      return ;
      }
   if (fields[i]->FieldType == WINEXE_FLD_COMBO)
      {
      if (fields[i]->hWnd != NULL)
         {
         DestroyCaret() ;                                                       /* no longer caret */
         FillSelectionBox(i) ;
         ShowWindow(fields[i]->hWnd, SW_SHOW) ;
         SetFocus(fields[i]->hWnd) ;
         }
      return ;
      }

   forms[currentformindex].EditMode = 1 ;                                 /* now in edit mode */

   if (fields[i]->tableid > 0)                                             /* is this a table column field ? */
      {
      tblindex = FindIndex(TABLE_TYPE, fields[i]->tableid) ;               /* set the tableindex */
      if (forms[currentformindex].curtblrow < 0)                          /* if we don't know the row */
         forms[currentformindex].curtblrow = 0 ;                          /* start in row 0 of table */
      fields[i]->ValueRect.top = tables[tblindex].Rect.top +
                                tables[tblindex].TblLblSize + tables[tblindex].ColLblSize +
                                tables[tblindex].LineWidth +
                                (forms[currentformindex].curtblrow *
                                 tables[tblindex].AvgRowSize) ;           /* find out how big the field is (top) */
      fields[i]->ValueRect.bottom = fields[i]->ValueRect.top + tables[tblindex].AvgRowSize
                                   - (1 + tables[tblindex].LineWidth) ;                       /* (bottom) */
      if (fields[i]->str != NULL)                                          /* if string exists */
         GlobalUnlock(fields[i]->Buffer) ;                                 /* unlock the buffer */
      fields[i]->str = NULL ;                                              /* delete string */
      ph = (LPHANDLE) GlobalLock(fields[i]->ColBuffer) ;                   /* can we lock the memory? */
      if (ph == NULL)                                                     /* if not */
         {
         GlobalFree(fields[i]->ColBuffer) ;                                /* free memory */
         forms[currentformindex].EditMode = 0 ;                           /* leave edit mode */
         MessageBox(NULL, "Unable to lock memory for Array Column buffer.",
                    NULL, MB_ICONEXCLAMATION | MB_OK) ;                   /* display error message */
         return ;                                                         /* return to calling function */
         }
      fields[i]->Buffer = ph[forms[currentformindex].curtblrow] ;          /* set the pointer to current row */
      GlobalUnlock(fields[i]->ColBuffer) ;                                 /* unlock the column buffer memory */
      }
   else                                                                   /* it's not a table column field */
      forms[currentformindex].curtblrow = -1 ;                            /* remember that */

   if (fields[i]->str == NULL)                                             /* if string empty */
      fields[i]->str = GlobalLock(fields[i]->Buffer) ;                      /* copy buffer into string (and lock buffer) */
   if (fields[i]->str == NULL)                                             /* if still empty */
      {
      GlobalFree(fields[i]->Buffer) ;                                      /* free buffer memory */
      forms[currentformindex].EditMode = 0 ;                              /* leave edit mode */
      MessageBox(NULL, "Unable to lock memory for field buffer.", NULL, MB_ICONEXCLAMATION | MB_OK) ;   /* display error message */
      return ;                                                            /* return to calling function */
      }

   switch (fields[i]->FieldType)                                           /* identify field type */
      {
      case WINEXE_FLD_FIXED:                                              /* numeric fields */
      case WINEXE_FLD_PERCENT:
      case WINEXE_FLD_FINAN:
      case WINEXE_FLD_CUR:
      case WINEXE_FLD_NUMPIC:
         numb = atof(fields[i]->str) ;                                     /* get the number */
         sprintf(mybuf, "%1.*lf", fields[i]->Decimals, numb) ;             /* display it */
         lstrcpy(fields[i]->str, (LPSTR) mybuf) ;                          /* copy it from buffer to string */
         break ;
      }

#if !defined(__WC386__)
   FieldPictureApply(fields[i]->str, fields[i]->Picture, fields[i]->FieldType) ;                  /* apply picture to field */
#else
   FieldPictureApply((LPSTR)fields[i]->str, (LPSTR)fields[i]->Picture, fields[i]->FieldType) ;                  /* apply picture to field */
#endif
   pos = -1 ;                                                             /* reset position */
#if !defined(__WC386__)
   pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'r', pos) ;      /* determine caret position */
#else
   pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'r', pos) ;      /* determine caret position */
#endif
   forms[currentformindex].caretpos = pos ;                               /* remember new caret position */
   CreateCaret(forms[currentformindex].hWnd, NULL,
      ((abs(fonts[fields[i]->ValueFont].lfHeight) * .1) < 6 ? abs(fonts[fields[i]->ValueFont].lfHeight) * .1 : 5),
        abs(fonts[fields[i]->ValueFont].lfHeight));                               /* create the new caret */

#if !defined(__WC386__)
   lstrcpy(FieldChangeBuffer, fields[i]->str) ;                            /* save to detect field change events */
#else
   lstrcpy((LPSTR)FieldChangeBuffer, (LPSTR)fields[i]->str) ;                            /* save to detect field change events */
#endif

   PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                         /* order repaint of current field */
#if !defined(__WC386__)
   InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;       /* invalidate old field rectamgle */
#else
   InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;       /* invalidate old field rectamgle */
#endif
   UpdateWindow(forms[currentformindex].hWnd) ;                           /* update the window */
   PaintFieldValueOnly = 0 ;                                              /* repaint finished */
   ShowCaret(forms[currentformindex].hWnd) ;                              /* show caret */
   SetFocus(forms[currentformindex].hWnd) ;                               /* and activate field again */

   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    FieldEditEnd (i, vkey)

   PURPOSE:     done with editing a field
                i:      field
                vkey:   virtual key

*****************************************************************************/

void FieldEditEnd(int i, int vkey)
   {
   int j, k, curord, neword, gonext, newfld, pos, fchange ;
   int tblidx ;
   char tmpbuf[81] ;
   double numb ;


   switch (fields[i]->FieldType)                                           /* identify field type */
      {
      case WINEXE_FLD_SELECT:
      case WINEXE_FLD_COMBO:
         if (fields[i]->hWnd != NULL)
            ShowWindow(fields[i]->hWnd, SW_HIDE) ;
         if (fields[i]->str == NULL)
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* lock the buffer */
         PaintFieldValueOnly = 0 ;
#if !defined(__WC386__)
         InvalidateRect(forms[currentformindex].hWnd, &fields[i]->Rect, TRUE) ;
#else
         InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->Rect, TRUE) ;
#endif
         UpdateWindow(forms[currentformindex].hWnd) ;                     /* update the window */
         break ;
      case WINEXE_FLD_FIXED:                                              /* reformat numeric fields */
      case WINEXE_FLD_PERCENT:
      case WINEXE_FLD_FINAN:
      case WINEXE_FLD_CUR:
      case WINEXE_FLD_NUMPIC:
         pos = forms[currentformindex].caretpos ;                         /* remember caret position */
         if (vkey != VK_RETURN || pos == 0)                               /* if caretpos 0, and key NOT return */
            {
            if (fields[i]->Picture[0] == 0)                                /* and no picture clause set */
               pos = fields[i]->Length + 1 ;                               /* set caretpos at end of field */
            else                                                          /* so there is a picture clause set */
#if !defined(__WC386__)
               pos = lstrlen(fields[i]->Picture) + 1 ;                     /* set caret at end of picture */
#else
               pos = lstrlen((LPSTR)fields[i]->Picture) + 1 ;                     /* set caret at end of picture */
#endif
            }
         else if (pos == forms[currentformindex].prevpos)                 /* caretpos isn't 0, or return pressed and caret still in old position */
            pos++ ;                                                       /* advance caret one to right */
         j = k = 0 ;                                                      /* reset these variables */
         while (k < pos)                                                  /* while you're left of caretpos */
            {
            if (fields[i]->str[k] != ' ' && fields[i]->str[k] != '\t')      /* if the character at pos isn't a blank or a tab */
               tmpbuf[j++] = fields[i]->str[k] ;                           /* remember it in tmpbuf */
            k++ ;                                                         /* go to next k pos */
            }
         tmpbuf[j] = 0 ;                                                  /* well, add a 0 at end of tmpbuf (NULL-terminated string) */
         numb = atof(tmpbuf) ;                                            /* change tmpbuf to float value for numb (numeric fields!) */
         sprintf(fields[i]->str, "%1.*lf", fields[i]->Decimals, numb) ;     /* print tmpbuf to field */
         PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                   /* repaint the field */
#if !defined(__WC386__)
         InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;           /* delete the old field */
#else
         InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;           /* delete the old field */
#endif
         UpdateWindow(forms[currentformindex].hWnd) ;                     /* update the window */
         PaintFieldValueOnly = 0 ;                                        /* finished repainting! */
         break ;                                                          /* numeric fields reformatted */
      }

   fchange = 0 ;                                                          /* reset fieldchange variable */
#if !defined(__WC386__)
   if (fields[i]->locflds[0] != NULL && lstrcmp(fields[i]->str, FieldChangeBuffer))             /* Field Change Event ! */
#else
   if (fields[i]->locflds[0] != NULL && lstrcmp((LPSTR)fields[i]->str, (LPSTR)FieldChangeBuffer))             /* Field Change Event ! */
#endif
      fchange = 1 ;                                                       /* save for after clean up */

   forms[currentformindex].EditMode = 0 ;                                 /* no longer edit mode */
   DestroyCaret() ;                                                       /* no longer caret */
   GlobalUnlock(fields[i]->Buffer) ;                                       /* unlock buffer memory */
   fields[i]->str = NULL ;                                                 /* empty string buffer */
   forms[currentformindex].EndKey = vkey ;                                /* remember actual key */

   /* find the next field based on field order to edit */
   curord = fields[i]->Order ;                                             /* get current order */
   neword = -1 ;                                                          /* reset neworder variable */
   switch(vkey)                                                           /* identify actual key pressed */
      {
      case VK_RETURN:                                                     /* next field */
      case VK_RIGHT:
      case VK_DOWN:
      case VK_TAB:
         gonext = 1 ;                                                     /* se we'll go to next field */
         break ;
      case VK_LEFT:                                                       /* previous field */
      case VK_UP:
         gonext = 0 ;                                                     /* we're not going to the next */
         break ;
      default:                                                            /* we don't know the key */
         forms[currentformindex].curtblrow = -1 ;                         /* deactivate current table row */
         gonext = -1 ;
         break ;
      }
   for (j = 0 ; gonext >= 0 && j < FIELD_COUNT ; j++)                                    /* for all fields */
      {
      if (fields[j]->formid != forms[currentformindex].id)                 /* if form isn't for this menu type */
         continue ;                                                       /* continue search */
      if (fields[j]->FieldType == WINEXE_FLD_CHECK)
         continue ;
      if (fields[j]->FieldType == WINEXE_FLD_RADIO)
         continue ;
      if (fields[j]->FieldType == WINEXE_FLD_BOOL)
         continue ;
      if (gonext)                                                         /* if we're going to the next field */
         {
         if (fields[j]->Order > curord &&
             (neword < 0 || fields[j]->Order < neword))                    /* if order found is higher than the current one, and neworder not set or higher */
            {
            neword = fields[j]->Order ;                                    /* set the new order */
            newfld = j ;                                                  /* and remember which field we're in */
            }
         }
      else                                                                /* so we're going to the previous field */
         {
         if (fields[j]->Order < curord &&
             (neword < 0 || fields[j]->Order > neword))                    /* if order found is lower than the current one, and neworder not set or smaller */
            {
            neword = fields[j]->Order ;                                    /* set the new order */
            newfld = j ;                                                  /* and remember which field we're in */
            }
         }
      }

#if !defined(__WC386__)
   if (gonext >= 0 && (neword < 0 || !IsSameFile(fields[i]->flds[1], fields[newfld]->flds[1])))
#else
   if (gonext >= 0 && (neword < 0 || !IsSameFile((LPSTR)fields[i]->flds[1], (LPSTR)fields[newfld]->flds[1])))
#endif
      {
      j = 0 ;                                                             /* reset variable */
      while (databases[j].name != NULL)                                   /* as long as there is a database */
         {
#if !defined(__WC386__)
         if (IsSameFile(databases[j].name, fields[i]->flds[1]) ||
             IsSameFile(databases[j].link, fields[i]->flds[2]))            /* if field belongs to the database */
#else
         if (IsSameFile((LPSTR)databases[j].name, (LPSTR)fields[i]->flds[1]) ||
             IsSameFile((LPSTR)databases[j].link, (LPSTR)fields[i]->flds[2]))            /* if field belongs to the database */
#endif
            {
            if (fields[i]->tableid > 0)
               tblidx = FindIndex(TABLE_TYPE, fields[i]->tableid) ;        /* set the index */
#if !defined(__WC386__)
            if (fields[i]->tableid > 0 && !IsSameFile(tables[tblidx].Filename, databases[j].name))    /* not table master file */
#else
            if (fields[i]->tableid > 0 && !IsSameFile((LPSTR)tables[tblidx].Filename, (LPSTR)databases[j].name))    /* not table master file */
#endif
               ;
            else if (fields[i]->tableid > 0 && forms[currentformindex].curtblrow >= 0)    /* and the table and form is known */
               {
               tblidx = FindIndex(TABLE_TYPE, fields[i]->tableid) ;        /* set the index */
               if (tables[tblidx].RowRecNum[forms[currentformindex].curtblrow] > 0)           /* and if the row exists */
                  {
                  if (GotoRecord(j, tables[tblidx].RowRecNum[forms[currentformindex].curtblrow]))       /* and the record is active */
                     databases[j].recordnumber = tables[tblidx].RowRecNum[forms[currentformindex].curtblrow] ;    /* remember the record */
                  else                                                    /* record is not active */
                     databases[j].recordnumber = -1 ;                     /* remember it */
                  }
               else                                                       /* row doesn't exist */
                  databases[j].recordnumber = -1 ;                        /* remember it */
               }
            else if (fields[i]->tableid > 0)                               /* so just the table is known */
               databases[j].recordnumber = -1 ;                           /* clear recordnumber */
            if (databases[j].autoinsert && databases[j].recordnumber < 1L)          /* if there is a new record */
               {
#if !defined(__WC386__)
               INSERT(databases[j].name) ;                                /* insert the record in database */
#else
               INSERT((LPSTR)databases[j].name) ;                                /* insert the record in database */
#endif
               if (fields[i]->tableid > 0 && forms[currentformindex].curtblrow >= 0)           /* if there is something in the record */
                  tables[tblidx].RowRecNum[forms[currentformindex].curtblrow] = databases[j].recordnumber ;       /* update recordcount */
               }
            else if (databases[j].autoupdate && databases[j].recordnumber > 0L)     /* if a record has changed */
#if !defined(__WC386__)
               UPDATE(databases[j].name) ;                                /* update record in database */
#else
               UPDATE((LPSTR)databases[j].name) ;                                /* update record in database */
#endif
            break ;                                                       /* discontinue database search */
            }
         j++ ;                                                            /* continue search */
         }
      }

   FieldChangeLocateTableRow = forms[currentformindex].curtblrow ;        /* remember in which row change occured */
   if (fields[i]->tableid < 0 && neword < 0)                               /* if field and order not known */
      forms[currentformindex].curtblrow = -1 ;                            /* invalidate that row */

   if (fchange >= 0)                                                      /* make the field change event happen */
      {
      k = 0 ;                                                             /* reset variable */
      while (fields[i]->locflds[k] != NULL)                                /* if fields have a location */
         {
#if !defined(__WC386__)
         LOCATEFIELD(fields[i]->locflds[k+1], 0, fields[i]->locflds[k]) ;   /* update location */
#else
         LOCATEFIELD((LPSTR)fields[i]->locflds[k+1], 0, (LPSTR)fields[i]->locflds[k]) ;   /* update location */
#endif
         k += 3 ;                                                         /* go to next field pack */
         }
      FieldChangeLocateTableRow = -1 ;                                    /* finished relocating */
#if !defined(__WC386__)
      for (k = 0 ; k < MAX_DDE_COUNT && ServerModeDDE ; k++)              /* DDEML server mode field changes */
         {
         if (fields[i]->hFieldDDE[k] != NULL)
            DdePostAdvise(idInstDDE, hTopicDDE, fields[i]->hFieldDDE[k]) ;
         }
#endif
      }

   if (neword > 0)                                                        /* if there is a neworder */
      FieldEdit(newfld) ;                                                 /* edit it */
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    FieldKeyInput (vkey, ckey)

   PURPOSE:     manage key input to field
                vkey:   virtual key code form WM_KEYDOWN
                ckey:   ASCII character key code form WM_CHAR

*****************************************************************************/

void FieldKeyInput(int vkey, int ckey)      /* vkey & ckey will not both have non-zero values at the same time */
   {
   int i, pos ;
   char tmpbuf[81] ;
   double numb ;

   i = forms[currentformindex].curfld ;                                   /* remember current field */

   if (ckey == 26 || vkey == VK_CLEAR)                                    /* control y (clear field) or CLEAR is pressed */
      {
      forms[currentformindex].prevpos = forms[currentformindex].caretpos ;          /* remember caret position */
      pos = 0 ;                                                           /* reset pos */
      while (fields[i]->str[pos])                                          /* as long as there is a character */
         fields[i]->str[pos++] = ' ' ;                                     /* clear it, go to next position */
      pos = -1 ;                                                          /* reset pos */
#if !defined(__WC386__)
      pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'r', pos) ;   /* get field position in window */
#else
      pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'r', pos) ;   /* get field position in window */
#endif
      forms[currentformindex].caretpos = pos ;                            /* position caret to it */
      PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                      /* repaint the field */
#if !defined(__WC386__)
      InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;    /* delete the old field */
#else
      InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;    /* delete the old field */
#endif
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update the window */
      PaintFieldValueOnly = 0 ;                                           /* repaint done! */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_HOME)                                                   /* move to 1st column in field */
      {
      forms[currentformindex].prevpos = forms[currentformindex].caretpos ;          /* remember caret position */
      pos = -1 ;                                                          /* reset pos */
#if !defined(__WC386__)
      pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'r', pos) ;   /* get field position in window */
#else
      pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'r', pos) ;   /* get field position in window */
#endif
      forms[currentformindex].caretpos = pos ;                            /* set caret to that position */
      PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                      /* repaint that field */
#if !defined(__WC386__)
      InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;    /* invalidate old field */
#else
      InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;    /* invalidate old field */
#endif
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update the window */
      PaintFieldValueOnly = 0 ;                                           /* painting done! */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_END)                                                    /* move to last column in field */
      {
      forms[currentformindex].prevpos = forms[currentformindex].caretpos ;          /* remember caret position */
      pos = lstrlen(fields[i]->str) + 1 ;                                  /* set pos to end of string */
#if !defined(__WC386__)
      pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'l', pos) ;   /* get end-of-field position in window */
#else
      pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'l', pos) ;   /* get end-of-field position in window */
#endif
      forms[currentformindex].caretpos = pos ;                            /* set caret to that position */
      PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                      /* repaint that field */
#if !defined(__WC386__)
      InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;    /* invalidate old field */
#else
      InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;    /* invalidate old field */
#endif
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update the window */
      PaintFieldValueOnly = 0 ;                                           /* painting done! */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_LEFT || vkey == VK_RIGHT)                               /* left or right arrow key */
      {
      forms[currentformindex].prevpos = forms[currentformindex].caretpos ;          /* remember old caret position */
      pos = forms[currentformindex].caretpos ;                            /* use that position as base */
      if (vkey == VK_LEFT)                                                /* if left arrow key */
#if !defined(__WC386__)
         pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'l', pos) ;          /* determine caret position */
#else
         pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'l', pos) ;          /* determine caret position */
#endif
      else                                                                /* so, right arrow key */
#if !defined(__WC386__)
         pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'r', pos) ;          /* determine caret position */
#else
         pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'r', pos) ;          /* determine caret position */
#endif
      if (pos == forms[currentformindex].caretpos)                        /* if the new position is the same */
         {
         /* FieldEditEnd(i, vkey) ; */                                    /* uncomment for move to prev/next field */
         return ;                                                         /* return to calling function */
         }
      else if (vkey == VK_RIGHT && fields[i]->str[pos] == 0)               /* if we go right, and string has no entry for that pos */
         fields[i]->str[pos] = ' ' ;                                       /* put a blank there */

      forms[currentformindex].caretpos = pos ;                            /* set the new caret position */
      PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                      /* repaint that field */
#if !defined(__WC386__)
      InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;    /* invalidate old field */
#else
      InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;    /* invalidate old field */
#endif
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update the window */
      PaintFieldValueOnly = 0 ;                                           /* painting done */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_BACK || vkey == VK_DELETE)                              /* backspace or delete key */
      {
      forms[currentformindex].prevpos = forms[currentformindex].caretpos ;          /* remember current caret position */
      pos = forms[currentformindex].caretpos ;                            /* set pos to that */
      if (vkey == VK_BACK)                                                /* if backspace */
         {
#if !defined(__WC386__)
         pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'l', pos) ;     /* get the caret to left pos in window */
#else
         pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'l', pos) ;     /* get the caret to left pos in window */
#endif
         if (pos == forms[currentformindex].caretpos)                     /* if we're already very left */
            {
            FieldEditEnd(i, vkey) ;                                       /* call FieldEditEnd function */
            return ;                                                      /* return to calling function */
            }
         fields[i]->str[pos] = fields[i]->str[forms[currentformindex].caretpos] ;     /* read what's there */
         if (fields[i]->str[pos] == 0)                                     /* if nothing's there */
            fields[i]->str[pos] = ' ' ;                                    /* put a blank there */
         }
#if !defined(__WC386__)
      FieldPictureShift(fields[i]->str, fields[i]->Picture, 'r', 'l', pos, fields[i]->Length) ;  /* shift the picture (string) */
#else
      FieldPictureShift((LPSTR)fields[i]->str, (LPSTR)fields[i]->Picture, 'r', 'l', pos, fields[i]->Length) ;  /* shift the picture (string) */
#endif
      forms[currentformindex].caretpos = pos ;                            /* set mew caret position */
      PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                      /* repaint the field */
#if !defined(__WC386__)
      InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;    /* invalidate old rectangle */
#else
      InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;    /* invalidate old rectangle */
#endif
      UpdateWindow(forms[currentformindex].hWnd) ;                        /* update the window */
      PaintFieldValueOnly = 0 ;                                           /* painting done */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_INSERT)                                                 /* insert key */
      {
      forms[currentformindex].InsMode = (forms[currentformindex].InsMode) ? 0 : 1 ; /* toggle insert mode */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_DOWN || vkey == VK_UP || vkey == VK_RETURN)             /* up, down, or return */
      {
      FieldEditEnd(i, vkey) ;                                             /* transfer request to FieldEditEnd function */
      return ;                                                            /* return to calling function */
      }

   if (vkey == VK_ESCAPE || vkey == VK_PRIOR || vkey == VK_NEXT || ckey == '\t')    /* escape, prior, next, or tab */
      {
      FieldEditEnd(i, vkey) ;                                             /* transfer request to FieldEditEnd function */
      return ;                                                            /* return to calling function */
      }

   if (ckey == '.')                                                       /* point */
      {
      switch (fields[i]->FieldType)                                        /* identify field type */
         {
         case WINEXE_FLD_FIXED:                                           /* reformat numeric fields */
         case WINEXE_FLD_PERCENT:
         case WINEXE_FLD_FINAN:
         case WINEXE_FLD_CUR:
         case WINEXE_FLD_NUMPIC:
            lstrcpy(tmpbuf, fields[i]->str) ;                              /* copy string into temporary buffer */
            pos = forms[currentformindex].caretpos ;                      /* remember caret position */
            tmpbuf[pos] = 0 ;
            numb = atof(tmpbuf) ;                                         /* convert string to float in numb (numeric field!) */
            if (fields[i]->Picture[0] == 0)
               sprintf(fields[i]->str, "%1.*lf", fields[i]->Decimals, numb) ;  /* prepare it for output, regarding decimal point pos */
            else
#if !defined(__WC386__)
               sprintf(fields[i]->str, "%*.*lf", lstrlen(fields[i]->Picture), fields[i]->Decimals, numb) ;
#else
               wsprintf((LPSTR)fields[i]->str, (LPSTR)"%*.*lf", lstrlen((LPSTR)fields[i]->Picture), fields[i]->Decimals, numb) ;
#endif
            pos = lstrlen(fields[i]->str) - fields[i]->Decimals ;                 /* adjust position according decimals */
            forms[currentformindex].caretpos = pos ;                      /* adjust caret position */
            forms[currentformindex].prevpos = pos - 1 ;                   /* previous position is one place left */
            PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                /* repaint field */
#if !defined(__WC386__)
            InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;        /* invalidate old field */
#else
            InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;        /* invalidate old field */
#endif
            UpdateWindow(forms[currentformindex].hWnd) ;                  /* update the window */
            PaintFieldValueOnly = 0 ;                                     /* painting done! */
            return ;                                                      /* return to calling function */
         }
      /* do not return */
      }

   if (ckey >= ' ')                                                       /* printable character */
      {
      pos = forms[currentformindex].prevpos = forms[currentformindex].caretpos ;    /* remember caret position */
      if (fields[i]->Picture[0] == 0)                                      /* if no picture clause set */
         {
         if (forms[currentformindex].InsMode)                             /* if we're in insert mode */
#if !defined(__WC386__)
            FieldPictureShift(fields[i]->str, fields[i]->Picture, 'r', 'r', pos, fields[i]->Length) ;      /* shift string at right one pos to the right */
#else
            FieldPictureShift(fields[i]->str, (LPSTR)fields[i]->Picture, 'r', 'r', pos, fields[i]->Length) ;      /* shift string at right one pos to the right */
#endif
         fields[i]->str[pos] = ckey ;                                      /* and put the character in vacant pos */
         if (forms[currentformindex].caretpos < fields[i]->Length - 1)     /* if we're not at the right of field */
            forms[currentformindex].caretpos++ ;                          /* move caret one to the right */
         PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                   /* paint field */
#if !defined(__WC386__)
         InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;           /* invalidate rectangle */
#else
         InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;           /* invalidate rectangle */
#endif
         UpdateWindow(forms[currentformindex].hWnd) ;                     /* update window */
         PaintFieldValueOnly = 0 ;                                        /* painting done */
         return ;                                                         /* return to calling function */
         }
      else if (fields[i]->Picture[pos] == 'X' ||                           /* picture clause is set, identify it and check for validity */
               fields[i]->Picture[pos] == '@' ||
               fields[i]->Picture[pos] == '!' ||
               (fields[i]->Picture[pos] == '&' && isalpha(ckey)) ||
               (fields[i]->Picture[pos] == '#' && ckey >= '0' && ckey <= '9') ||
               (fields[i]->Picture[pos] == '#' && (ckey == ' ' || ckey == '+' || ckey == '-')) ||
               (fields[i]->Picture[pos] == '9' && isdigit(ckey)) ||
               (fields[i]->Picture[pos] == '9' && (ckey == ' ' || ckey == '+' || ckey == '-')))
         {
         if (fields[i]->Picture[pos] == '!' || fields[i]->Picture[pos] == '&')        /* if these clauses are set */
            ckey = toupper(ckey) ;                                        /* convert lower case chars to upper case */
         if (forms[currentformindex].InsMode)                             /* if insert mode is set, shift rest of string one to right */
#if !defined(__WC386__)
            FieldPictureShift(fields[i]->str, fields[i]->Picture, 'r', 'r', pos, lstrlen(fields[i]->Picture)) ;
#else
            FieldPictureShift((LPSTR)fields[i]->str, (LPSTR)fields[i]->Picture, 'r', 'r', pos, lstrlen((LPSTR)fields[i]->Picture)) ;
#endif
         fields[i]->str[pos] = ckey ;                                      /* put character into vacant position */
#if !defined(__WC386__)
         pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'r', pos) ;     /* set pos one to right */
#else
         pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'r', pos) ;     /* set pos one to right */
#endif
         forms[currentformindex].caretpos = pos ;                         /* remember it as current caret position */
         PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                   /* repaint field */
#if !defined(__WC386__)
         InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;      /* invalidate old rectangle */
#else
         InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;      /* invalidate old rectangle */
#endif
         UpdateWindow(forms[currentformindex].hWnd) ;                     /* update window */
         PaintFieldValueOnly = 0 ;                                        /* repaint done! */
         return ;                                                         /* return to calling function */
         }
      else if (((fields[i]->Picture[pos] == 'A' || fields[i]->Picture[pos] == '?') && isalpha(ckey)) ||
               (fields[i]->Picture[pos] == 'N' && isalnum(ckey)) ||
               (fields[i]->Picture[pos] == 'L' && (toupper(ckey) == 'F' || toupper(ckey) == 'T')) ||
               (fields[i]->Picture[pos] == 'L' && (toupper(ckey) == 'Y' || toupper(ckey) == 'N')) ||
               (toupper(fields[i]->Picture[pos]) == 'D' && isdigit(ckey)) ||
               (toupper(fields[i]->Picture[pos]) == 'C' && isdigit(ckey)) ||
               (toupper(fields[i]->Picture[pos]) == 'M' && isdigit(ckey)) ||
               (toupper(fields[i]->Picture[pos]) == 'Y' && isdigit(ckey)))      /* check for more picture clauses and test validity */
         {
         if (fields[i]->Picture[pos] == '!' || fields[i]->Picture[pos] == '&')   /* if these clauses set */
            ckey = toupper(ckey) ;                                        /* convert lower case chars to upper case */
         if (forms[currentformindex].InsMode)                             /* if insert mode true, shift rest of string one to right */
#if !defined(__WC386__)
            FieldPictureShift(fields[i]->str, fields[i]->Picture, 'r', 'r', pos,  lstrlen(fields[i]->Picture)) ;
#else
            FieldPictureShift((LPSTR)fields[i]->str, (LPSTR)fields[i]->Picture, 'r', 'r', pos,  lstrlen((LPSTR)fields[i]->Picture)) ;
#endif
         fields[i]->str[pos] = ckey ;                                      /* fill key character into vacant position */
#if !defined(__WC386__)
         pos = FieldPicturePosition(fields[i]->Length, fields[i]->Picture, 'r', pos) ;     /* set pos one right */
#else
         pos = FieldPicturePosition(fields[i]->Length, (LPSTR)fields[i]->Picture, 'r', pos) ;     /* set pos one right */
#endif
         forms[currentformindex].caretpos = pos ;                         /* set caret to that position */
         PaintFieldValueOnly = WINEXE_PAINTCURFLDONLY ;                   /* repaint field */
#if !defined(__WC386__)
         InvalidateRect(forms[currentformindex].hWnd, &fields[i]->ValueRect, TRUE) ;      /* invalidate old rectangle */
#else
         InvalidateRect(forms[currentformindex].hWnd, (LPRECT)&fields[i]->ValueRect, TRUE) ;      /* invalidate old rectangle */
#endif
         UpdateWindow(forms[currentformindex].hWnd) ;                     /* update window */
         PaintFieldValueOnly = 0 ;                                        /* repaint done! */
         return ;                                                         /* return to calling function */
         }
      }

   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    FieldPictureApply (str, pic, fieldalignment)

   PURPOSE:     apply a picture to field
                str:    string
                pic:    picture clause

*****************************************************************************/

void FieldPictureApply(LPSTR str, LPSTR pic, int fieldtype)
   {
   LPSTR orgstr, orgpic ;
   if (pic == NULL || pic[0] == 0)                                        /* if there is no picture clause, or it's empty */
      return ;                                                            /* return to calling function */

   orgstr = str ;
   orgpic = pic ;
   switch (fieldtype)                                                     /* identify field type */
      {
      case WINEXE_FLD_FIXED:                                              /* numeric fields */
      case WINEXE_FLD_PERCENT:
      case WINEXE_FLD_FINAN:
      case WINEXE_FLD_CUR:
      case WINEXE_FLD_NUMPIC:
#if defined(__C700__)
         _strrev(str) ;
         _strrev(pic) ;
#else
         strrev(str) ;
         strrev(pic) ;
#endif
         break ;
      }

   while (*pic)                                                           /* while there is a picture clause */
      {
      if (!*str)
         {
         *str = ' ' ;                                                     /* set it to blank */
         *(str + 1) = 0 ;
         }
      if (*pic == '!' || *pic == '&')                                     /* uppercase clause set */
         {
         if (*str >= 'a' && *str <= 'z')                                  /* if lowercase detected */
            *str = (char)('A' + (int)*str - 'a') ;                        /* convert to uppercase */
         }
      else if (strchr(PICHARS,*pic) == (char *) 0)                        /* so char is OK */
         *str = *pic ;                                                    /* take it */
      str++ ;                                                             /* get next character */
      pic++ ;                                                             /* get next clause */
      }

   switch (fieldtype)                                                     /* identify field type */
      {
      case WINEXE_FLD_FIXED:                                              /* numeric fields */
      case WINEXE_FLD_PERCENT:
      case WINEXE_FLD_FINAN:
      case WINEXE_FLD_CUR:
      case WINEXE_FLD_NUMPIC:
#if defined(__C700__)
         _strrev(orgstr) ;
         _strrev(orgpic) ;
#else
         strrev(orgstr) ;
         strrev(orgpic) ;
#endif
         break ;
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    FieldPicturePosition (len, pic, direc, curpos)

   PURPOSE:     returns position of next non-template spot in given string
                len:    string length
                pic:    picture clause
                direc:  direction
                curpos: caret position

   EXAMPLES:    FieldPicturePosition(5,"(AAA)",'r',2) returns 3
                FieldPicturePosition(5,"(AAA)",'r',3) returns 3
                FieldPicturePosition(5,"(AAA)",'l',3) returns 2
                FieldPicturePosition(5,"(AAA)",'l',2) returns 1
                FieldPicturePosition(5,"(AAA)",'l',1) returns 1

*****************************************************************************/

int FieldPicturePosition(int len, LPSTR pic, int direc, int curpos)
   {
   int orgpos ;
   orgpos = curpos ;                                                      /* remember caret position */
   curpos = (direc == 'l') ? curpos - 1 : curpos + 1 ;                    /* if direction is left, set cursorposition one left, else one right */
   if (pic == NULL || pic[0] == 0)                                        /* if no picture clause set */
      {
      if (curpos >= 0 && curpos < len)                                    /* if cursor not outside string area */
         return(curpos) ;                                                 /* return to calling function with new caret position */
      else                                                                /* we are outside string area */
         return(orgpos) ;                                                 /* return to calling function with old caret position */
      }
   while (curpos >= 0 && pic[curpos] && strchr(PICHARS,pic[curpos]) == (char *) 0)  /* as long as we are in string area, and picture clause is valid */
      curpos = (direc == 'l') ? curpos - 1 : curpos + 1 ;                 /* if direction is left, set cursorposition one left, else one right */
   if (curpos >= 0 && pic[curpos] && strchr(PICHARS,pic[curpos]) != (char *) 0)    /* if we are inside string area, and picture clause isn't valid */
      return(curpos) ;                                                    /* return to calling function with new caret position */
   else                                                                   /* we are outside */
      return(orgpos) ;                                                    /* return to calling function with old caret position */
   }


/*****************************************************************************

   FUNCTION:    FieldPictureShift (str, pic, side, direc, curpos, len)

   PURPOSE:     shift data in the string right or left one character
                   based on the current picture string
                str:    the string to shift
                pic:    input picture template
                side:   which side of the string to shift
                direc:  direction to shift the string
                curpos: caret position
                len:    length of string

   EXAMPLES:    FieldPictureShift("(123)-456-7890","(999)-999-999",'r','r',6) =
                    "(123)- 45-6789"
                FieldPictureShift("(123)-456-7890","(999)-999-999",'r','l',6) =
                    "(123)-567-890 "
                FieldPictureShift("(123)-456-7890","(999)-999-999",'l','l',6) =
                    "(234)- 56-7890"
                FieldPictureShift("(123)-456-7890","(999)-999-999",'l','r',6) =
                    "( 12)-356-7890"

   WARNING:     does not check to see if new position for data is allowed by
                picture clause. (e.g. may move letters into number spots)

*****************************************************************************/

void FieldPictureShift(LPSTR str, LPSTR pic, int side, int direc, int curpos, int len)
   {
   int tmpos, orgpos, begpos ;

   orgpos = curpos ;                                                      /* remember caret position */
   if (side == 'r' && direc == 'r')                                       /* if side and direction are right */
      {
      begpos = lstrlen(str) + 1 ;                                         /* lengthen string area by one */
      if (begpos > len)                                                   /* if provided length is smaller */
         begpos = len ;                                                   /* adjust it */
      curpos = FieldPicturePosition(len, pic, 'l', begpos) ;              /* caretposition one left of right border (if possible) */
      tmpos = FieldPicturePosition(len, pic, 'l', curpos) ;               /* tmpos one left of caret (if possible) */
      while (curpos > orgpos && tmpos != curpos)                          /*as long as caretposition is greater than the original, and is still moveable */
         {
         str[curpos] = str[tmpos] ;                                       /* replace character by shifted character */
         curpos = tmpos ;                                                 /* set caret one to left */
         tmpos = FieldPicturePosition(lstrlen(str), pic, 'l', curpos) ;   /* tmpos one left of caret (if possible) */
         }
      str[orgpos] = ' ' ;                                                 /* fill now open position with blank */
      }
   else if (side == 'l' && direc == 'l')                                  /* if side and direction are left */
      {
      curpos = FieldPicturePosition(lstrlen(str), pic,'r',-1) ;           /* caretposition very left */
      tmpos = FieldPicturePosition(lstrlen(str), pic, 'r', curpos) ;      /* tmpos right of caret (if possible) */
      while (curpos < orgpos && tmpos != curpos)                          /* as long as caretposition is smaller than the original, and is still moveable */
         {
         str[curpos] = str[tmpos] ;                                       /* replace character by shifted character */
         curpos = tmpos ;                                                 /* set caret one right */
         tmpos = FieldPicturePosition(lstrlen(str), pic, 'r', curpos) ;   /* tmpos one right of caret (if possible) */
         }
      str[orgpos] = ' ' ;                                                 /* fill now open position wigh blank */
      }
   else if (side == 'r' && direc == 'l')                                  /* if side is right and direction left */
      {
      tmpos = FieldPicturePosition(lstrlen(str), pic, 'r', curpos) ;      /* tmpos one right of curpos (if possible) */
      while (curpos >= 0 && str[curpos] && tmpos != curpos)               /* as long as curpos is in field, and moveable to right */
         {
         str[curpos] = str[tmpos] ;                                       /* replace character by shifted character */
         curpos = tmpos ;                                                 /* set caret one to right */
         tmpos = FieldPicturePosition(lstrlen(str), pic, 'r', curpos) ;   /* set tmpos right of caret (if possible) */
         }
      str[ FieldPicturePosition(lstrlen(str), pic, 'l', lstrlen(str)) ] = ' ' ;     /* fill now open position with blank */
      }
   else if (side == 'l' && direc == 'r')                                  /* if side is left and direction is right */
      {
      tmpos = FieldPicturePosition(lstrlen(str), pic, 'l', curpos) ;      /* tmpos one left of curpos (if possible) */
      while (curpos >= 0 && str[curpos] && tmpos != curpos)               /* as long as caretpos is in field, and moveable to left */
         {
         str[curpos] = str[tmpos] ;                                       /* replace character with shifted character */
         curpos = tmpos ;                                                 /* set caret one left */
         tmpos = FieldPicturePosition(lstrlen(str), pic, 'l', curpos) ;   /* set tmpos one left of caret (if possible) */
         }
      str[ FieldPicturePosition(lstrlen(str), pic, 'r', -1) ] = ' ' ;     /* fill now open position with blank */
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    IsSameFile (fn1, fn2)

   PURPOSE:     compare 2 filenames, case is not important, file extension need
                   not be present but must match if present on both files
                fn1, fn2:       filename1, filename2

*****************************************************************************/

int IsSameFile(LPSTR fn1, LPSTR fn2)
   {
   int i = 0 ;                                                            /* lettercounter to zero (left) */

   if (fn1 == NULL || fn2 == NULL)                                        /* if filename doesn't exist */
      return(0) ;                                                         /* return to calling function with failure message */
   while (fn1[i] && fn2[i] && toupper(fn1[i]) == toupper(fn2[i]))         /* as long as files have letters, and these are equal for both files */
      i++ ;                                                               /* increase lettercounter by one (one to the right) */
   if (fn1[i] == 0 && fn2[i] == 0)                                        /* both filenames ended then? */
      return(1) ;                                                         /* return to calling function with success message */
   if (fn1[i] == '.' && fn2[i] == 0)                                      /* filename 2 ended, prefixes same? */
      return(1) ;                                                         /* return to calling function with success message */
   if (fn2[i] == '.' && fn1[i] == 0)                                      /* filename 1 ended, prefixes same? */
      return(1) ;                                                         /* return to calling function with success message */
   return(0) ;                                                            /* return to calling function with failure message */
   }


/*****************************************************************************

   FUNCTION:    IsFieldInFile (i, fn, *readpriv, *writepriv)

   PURPOSE:     tests if given field is in given file
                i:              fieldindex
                fn:             filename
                *readpriv:      read status
                *writepriv:     write status

*****************************************************************************/

int IsFieldInFile(int i, LPSTR fn, int *readpriv, int *writepriv)
   {
   int k, j = 0 ;                                                         /* reset k and j */
   int found = 0 ;                                                        /* reset found marker */
   while (fields[i]->flds[j] != NULL)                                      /* as long as there are fields */
      {
      if ((j % 4) == 3)                                                   /* read/write status */
         {
         if (found)                                                       /* if something's found */
            {
            if (readpriv != NULL)                                         /* if reading is possible (pointer exists) */
               *readpriv = 0 ;                                            /* don't allow it yet (set value 0) */
            if (writepriv != NULL)                                        /* if writing is possible */
               *writepriv = 0 ;                                           /* don't allow it yet (set value 0) */
#if !defined(__WC386__)
            for (k = 0 ; k < lstrlen(fields[i]->flds[j]) ; k++)            /* as long as the fileheader hasn't ended */
#else
            for (k = 0 ; k < lstrlen((LPSTR)fields[i]->flds[j]) ; k++)            /* as long as the fileheader hasn't ended */
#endif
               {
               if (readpriv != NULL && fields[i]->flds[j][k] == 'R')       /* if file gives read clearence */
                 *readpriv = 1 ;                                          /* allow reading */
               if (writepriv != NULL && fields[i]->flds[j][k] == 'W')      /* if file gives write clearence */
                  *writepriv = 1 ;                                        /* allow writing */
               }
            return(1) ;                                                   /* return to calling function with success message */
            }
         j++ ;                                                            /* increment j */
         continue ;                                                       /* continue fieldsearch */
         }
      if ((j % 4) == 0)                                                   /* skip fieldname */
         {
         j++ ;                                                            /* increment j */
         continue ;                                                       /* continue fieldsearch */
         }
#if !defined(__WC386__)
      if (IsSameFile(fields[i]->flds[j], fn))                              /* filename or linkname */
#else
      if (IsSameFile((LPSTR)fields[i]->flds[j], (LPSTR)fn))                              /* filename or linkname */
#endif
         found = 1 ;                                                      /* remember: we found it */
      j++ ;                                                               /* increment j */
      }
   return(0) ;                                                            /* return to calling function with failure message */
   }


/*****************************************************************************

   FUNCTION:    GetFIeldInFile (i, fn, fld)

   PURPOSE:     if the given field is in the given file return that file's
                   corresponding field name
                i:      fieldindex
                fn:     filename
                fld:    fieldstring

*****************************************************************************/

int GetFieldInFile(int i, LPSTR fn, LPSTR fld)
   {
   int j = 0 ;
   while (fields[i]->flds[j] != NULL)                                      /* as long as there are fields */
      {
      if ((j % 4) == 0 || (j % 4) == 3)                                   /* skip fieldname and RW */
         {
         j++ ;                                                            /* increment j */
         continue ;                                                       /* continue search */
         }
#if !defined(__WC386__)
      if (IsSameFile(fields[i]->flds[j], fn))                              /* filename or linkname */
#else
      if (IsSameFile((LPSTR)fields[i]->flds[j], (LPSTR)fn))                              /* filename or linkname */
#endif
         {
#if !defined(__WC386__)
         lstrcpy(fld, fields[i]->flds[j - 1]) ;                            /* copy field from file */
#else
         lstrcpy((LPSTR)fld, (LPSTR)fields[i]->flds[j - 1]) ;                            /* copy field from file */
#endif
         return(1) ;                                                      /* return to calling function with success message */
         }
      j++ ;                                                               /* increment j */
      }
   fld[0] = 0 ;                                                           /* empty field */
   return(0) ;                                                            /* return to calling function with failure message */
   }


/*****************************************************************************

   FUNCTION:    EventLockOutFile (fn, flag)

   PURPOSE:     lock out or unlock a field event change on the given database
                fn:     filename
                flag:   flag

*****************************************************************************/

void EventLockOutFile(LPSTR fn, int flag)
   {
   int i = 0 ;                                                            /* reset internal database index */
   while (databases[i].name != NULL)                                      /* as long as there are databases */
      {
#if !defined(__WC386__)
      if (IsSameFile(fn, databases[i].name))                              /* if databasename is same as filename */
#else
      if (IsSameFile((LPSTR)fn, (LPSTR)databases[i].name))                              /* if databasename is same as filename */
#endif
         databases[i].eventlock = flag ;                                  /* set the eventlock marker */
      i++ ;                                                               /* continue search */
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    IsEventLockOutFile (fn)

   PURPOSE:     check, if the given database field change event is locked out
                fn:     filename

*****************************************************************************/

int IsEventLockOutFile(LPSTR fn)
   {
   int i = 0 ;                                                            /* reset internal database index */
   while (databases[i].name != NULL)                                      /* as long as there is a database */
      {
#if !defined(__WC386__)
      if (IsSameFile(fn, databases[i].name) && databases[i].eventlock)    /* if databasename is same as filename, and eventlock set */
#else
      if (IsSameFile((LPSTR)fn, (LPSTR)databases[i].name) && databases[i].eventlock)    /* if databasename is same as filename, and eventlock set */
#endif
         return(1) ;                                                      /* return to calling function with success message */
      i++ ;                                                               /* continue search */
      }
   return(0) ;                                                            /* return to calling function with failure message */
   }


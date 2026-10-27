#include "order.h"
/*
 * Screen Paint Routines
 *
 */
/*****************************************************************************

   FUNCTION: PaintForm (hWnd, formindex, id)

   PURPOSE:  Update display
             hWnd:      WindowHandle
             formindex: was set to -1, will get formtype in function
             id:        current menu type

*****************************************************************************/

void PaintForm(HWND hWnd, int formindex, int id)
   {
   int i ;                                                                /* for index handling */
   PAINTSTRUCT ps ;                                                       /* for paintstructure handling */
   RECT Rect, intrect, bigRect, saveRect ;                                /* for window size handling */
   int xscroll, yscroll ;                                                 /* for scrollbox positions */

   if (formindex < 0)                                                     /* initially set to -1 */
      formindex = FindIndex(FORM_TYPE, id) ;                              /* set formindex to formtype for this menutype */

   BeginPaint(hWnd, &ps) ;                                                /* Begin updating the display now */
   saveRect.top = ps.rcPaint.top ;                                        /* save windowtop position */
   saveRect.left = ps.rcPaint.left ;                                      /* save windowleft position */
   saveRect.bottom = ps.rcPaint.bottom ;                                  /* save windowbottom position */
   saveRect.right = ps.rcPaint.right ;                                    /* save windowright position */

   forms[formindex].xMaxPix = forms[formindex].yMaxPix = 0 ;              /* set formsize to 0 */
   xscroll = GetScrollPos(hWnd, SB_HORZ) ;                                /* get horizontal scrollbox position */
   yscroll = GetScrollPos(hWnd, SB_VERT) ;                                /* get vertical scrollbox position */
   forms[formindex].xOffset = -xscroll ;                                  /* remember horizontal scrollbox position */
   forms[formindex].yOffset = -yscroll ;                                  /* remember vertical scrollbox position */

   for (i = 0 ; i < TOTAL_COUNT ; i++)                                    /* check through all forms */
      {
      if (paintorder[i].formid != id)                                     /* if the paintform isn't correct for the menu type */
         continue ;                                                       /* continue looking for it */

      switch (paintorder[i].type)                                         /* identify the correct painttype */
         {
         case BITMAP_TYPE:                                                /* it's a bitmap */
            if (!PaintFieldValueOnly)                                     /* and if we have to paint it */
               PaintBitmap(hWnd, -1, paintorder[i].id, &ps.rcPaint) ;     /* call PaintBitmap function to do it */
            break ;
         case BUTTON_TYPE:                                                /* it's a button */
            /* PaintButton(hWnd, -1, paintorder[i].id, 0) ; */            /* but isn't in use yet */
            break ;
         case TEXT_TYPE:                                                  /* it's text */
            if (!PaintFieldValueOnly)                                     /* and if we have to paint it */
               PaintText(hWnd, -1, paintorder[i].id, &ps.rcPaint) ;       /* call PaintText function to do it */
            break ;
         case RECT_TYPE:                                                  /* it's a rectangle */
            if (!PaintFieldValueOnly)                                     /* and if we have to paint it */
               PaintRect(hWnd, -1, paintorder[i].id, &ps.rcPaint) ;       /* call PaintRect function to do it */
            break ;
         case LINE_TYPE:                                                  /* it's a line */
            if (!PaintFieldValueOnly)                                     /* and if we have to paint it */
               PaintLine(hWnd, -1, paintorder[i].id, &ps.rcPaint) ;       /* call PaintLine function to do it */
            break ;
         case FIELD_TYPE:                                                 /* it's a field */
            PaintField(hWnd, -1, paintorder[i].id, &ps.rcPaint) ;         /* call PaintField function to paint it */
            break ;
         case TABLE_TYPE:                                                 /* it's a table */
            PaintTable(hWnd, -1, paintorder[i].id, &ps.rcPaint) ;         /* call Paint Table function to paint it */
            break ;
         }
      }

   GetClientRect(hWnd, &bigRect) ;                                        /* Get the client coordinates */

   for (i = 0 ; !PaintFieldValueOnly && i < BUTTON_COUNT ; i++)           /* if we have to paint, check buttons */
      if (buttons[i].formid == id)                                        /* if there is a button for the menu tyoe */
         {
         /* position the button rectangle at the right place in the window */
         Rect.top = buttons[i].Rect.top + forms[formindex].yOffset - 1 ;       /* top */
         Rect.left = buttons[i].Rect.left + forms[formindex].xOffset - 1 ;     /* left */
         Rect.bottom = buttons[i].Rect.bottom + forms[formindex].yOffset + 1;  /* bottom */
         Rect.right = buttons[i].Rect.right + forms[formindex].xOffset + 1 ;   /* right */
         if (IntersectRect(&intrect, &ps.rcPaint, &Rect))                  /* if button is in area to be redrawn */
            {
            MoveWindow(buttons[i].hWnd,
                       buttons[i].Rect.left - 1 - GetScrollPos(hWnd, SB_HORZ),
                       buttons[i].Rect.top - 1 - GetScrollPos(hWnd, SB_VERT),
                       2 + buttons[i].Rect.right - buttons[i].Rect.left,
                       2 + buttons[i].Rect.bottom - buttons[i].Rect.top,
                       FALSE) ;                                           /* move the button window into position */
            GetClientRect(buttons[i].hWnd, &Rect) ;                       /* get the old position */
            InvalidateRect(buttons[i].hWnd, &Rect, TRUE) ;                /* delete old rectangle */
            UpdateWindow(buttons[i].hWnd) ;                               /* draw the new one */
            }
         }

   ps.rcPaint.top = saveRect.top ;                                        /* reset windowtop position */
   ps.rcPaint.left = saveRect.left ;                                      /* reset windowleft position */
   ps.rcPaint.bottom = saveRect.bottom ;                                  /* reset windowbottom position */
   ps.rcPaint.right = saveRect.right ;                                    /* reset windowright position */
   EndPaint(hWnd, &ps);                                                   /* Finished updating the display */

   return ;                                                               /* return to calling function */
   }



/****************************************************************************

   FUNCTION:    PaintBitmap (hWnd, index, id, rcPaint)

   PURPOSE:     Paint a bitmap
                hWnd:           Window handle
                index:          set to -1, gets bitmap type index in function
                id:             menu type
                rcPaint:        paint structure

***************************************************************************/

void PaintBitmap(HWND hWnd, int index, int id, LPRECT rcPaint)
   {
   HDC hDC, hmemDC ;                                                      /* for handling the device context */
   HBITMAP hOldBitmap ;                                                   /* used to hold the old bitmap */
   RECT Rect, intrect ;                                                   /* used for rectangles */
   int formindex ;                                                        /* internal form index */

   if (index < 0)                                                         /* index initially set to -1 */
      index = FindIndex(BITMAP_TYPE, id) ;                                /* set index to bitmap type index for the menu type */
   formindex = FindIndex(FORM_TYPE, bitmaps[index].formid) ;              /* set formindex to correct type */

   /* if new size of rectangle bigger than preset formsize (was set to 0!) */
   if (bitmaps[index].Rect.right > forms[formindex].xMaxPix)              /* horizontal size */
      forms[formindex].xMaxPix = bitmaps[index].Rect.right ;              /* adjust it */
   if (bitmaps[index].Rect.bottom > forms[formindex].yMaxPix)             /* vertical size */
      forms[formindex].yMaxPix = bitmaps[index].Rect.bottom ;             /* adjust it */

   /* adjust the window rectangle to the old display area */
   Rect.top = bitmaps[index].Rect.top + forms[formindex].yOffset ;        /* top */
   Rect.left = bitmaps[index].Rect.left + forms[formindex].xOffset ;      /* left */
   Rect.bottom = bitmaps[index].Rect.bottom + forms[formindex].yOffset ;  /* bottom */
   Rect.right = bitmaps[index].Rect.right + forms[formindex].xOffset ;    /* right */


   if (!IntersectRect(&intrect, rcPaint, &Rect))                          /* if no rectangle intersections */
      return ;                                                            /* return to calling function (do nothing) */
   if (rcPaint->top > Rect.top)                                           /* if top of painting area outside view rectangle */
      rcPaint->top = Rect.top ;                                           /* set top of painiting area at top of rectangle */
   if (rcPaint->left > Rect.left)                                         /* if left of painting area outside view rectangle */
      rcPaint->left = Rect.left ;                                         /* set left of painiting area at top of rectangle */
   if (rcPaint->bottom < Rect.bottom)                                     /* if bottom of painting area outside view rectangle */
      rcPaint->bottom = Rect.bottom ;                                     /* set bottom of painiting area at top of rectangle */
   if (rcPaint->right < Rect.right)                                       /* if right of painting area outside view rectangle */
      rcPaint->right = Rect.right ;                                       /* set right of painiting area at top of rectangle */

   if (bitmaps[index].handle == NULL && !bitmaps[index].loaderror)        /* if bitmap not in use and no loaderror occured */
      {
      WinExeLoadBitmap(hWnd, 0, bitmaps[index].Filename,
           (LPRECT) &bitmaps[index].Rect, (LPRECT) &bitmaps[index].orgRect,
           (HANDLE far *) &bitmaps[index].handle) ;                       /* then load it */
      }

   if (bitmaps[index].handle != NULL)                                     /* if bitmap is known */
      {
      hDC = GetDC(hWnd) ;                                                 /* get device context for current handle */
      hmemDC = CreateCompatibleDC(hDC) ;                                  /* create a compatible area */
      hOldBitmap = SelectObject(hmemDC, bitmaps[index].handle) ;          /* copy the bitmap */
      if (hOldBitmap != NULL)                                             /* if the bitmap exists */
         {
         if (bitmaps[index].fullscreen != FALSE)                          /* if bitmap is fullscreen */
            {
            GetClientRect(hWnd, &Rect) ;                                  /* get size of rectangle */
            StretchBlt(hDC, Rect.left + forms[formindex].xOffset,
                            Rect.top + forms[formindex].yOffset,
                            Rect.right - Rect.left,
                            Rect.bottom - Rect.top,
                            hmemDC, 0, 0,
                            bitmaps[index].Rect.right - bitmaps[index].Rect.left,
                            bitmaps[index].Rect.bottom - bitmaps[index].Rect.top,
                            SRCCOPY) ;                                    /* copy bitmap into rectangle and adjust for fit */
            }
         else                                                             /* so it's not fullscreen */
            {
            StretchBlt(hDC, bitmaps[index].Rect.left + forms[formindex].xOffset,
                            bitmaps[index].Rect.top + forms[formindex].yOffset,
                            bitmaps[index].Rect.right - bitmaps[index].Rect.left,
                            bitmaps[index].Rect.bottom - bitmaps[index].Rect.top,
                            hmemDC, 0, 0,
                            bitmaps[index].orgRect.right - bitmaps[index].orgRect.left,
                            bitmaps[index].orgRect.bottom - bitmaps[index].orgRect.top,
                            SRCCOPY) ;                                    /* copy it into rectangle and adjust position */
            }
         SelectObject(hmemDC, hOldBitmap) ;                               /* re-select old bitmap */
         }
      else                                                                /* the bitmap doesn't exist */
         {
         MessageBox(hWnd, "Error selecting bitmap for display.", NULL, MB_ICONEXCLAMATION | MB_OK) ;  /* tell, we can't display it */
         InvalidateRect(hWnd, NULL, TRUE) ;                               /* and free reserved memory */
         }
      DeleteDC(hmemDC) ;                                                  /* release reserved memory of bitmap copy */
      ReleaseDC(hWnd, hDC) ;                                              /* release device context (so others can use it) */
      }
   else                                                                   /* the bitmap is unknown */
      bitmaps[index].loaderror = 1 ;                                      /* remember that (loaderror occured) */

   return ;                                                               /* return to calling function */
   }

/*****************************************************************************

   FUNCTION:    PaintButton (hWnd, index, id, lpdis)

   PURPOSE:     Paint a button
                hWnd:           Window handle
                index:          set to -1, gets button type index of menu type
                id:             menu type
                lpdis:          set to 0,

*****************************************************************************/

#pragma argsused                                                          /* no warnings for not-used arguments */
void PaintButton(HWND hWnd, int index, int id, DRAWITEMSTRUCT FAR * lpdis)
   {
   HBITMAP   hBitOld ;                                                    /* Handles for bitmaps for the buttons */
   HDC  hDC, hDCMem ;                                                     /* handle to device context */
   HPEN hOldPen, hPen ;                                                   /* handles for old & new pens */
   DWORD   dwFG, dwBG;                                                    /* hold old FG/BG colors */
   HFONT   hFont;                                                         /* Used to create the font to display the text */
   HFONT   hOldFont;                                                      /* Used to hold the old font */
   HRGN hRgn ;                                                            /* for creating clipping region */
   LOGBRUSH brush ;                                                       /* top define brush type */
   HBRUSH hOldBrush, hBrush ;                                             /* handles for old & new brushes */
   DWORD dwSize1 ;                                                        /* for bitmap size */
   int wide, high, xoff, yoff ;
   RECT Rect, orgRect, bitmapRect ;                                       /* used for rectangles */

   if (index < 0)                                                         /* if index < 0 (was initially set to -1) */
      index = FindIndex(BUTTON_TYPE, id) ;                                /* set it to button type index for menu type */

   hDC = lpdis->hDC ;                                                     /* pass the structure */

   /* calculate position of button (and use appropriate offset) */
   Rect.top = 1 ;
   Rect.left = 1 ;
   Rect.bottom = 1 + buttons[index].Rect.bottom - buttons[index].Rect.top ;
   Rect.right = 1 + buttons[index].Rect.right - buttons[index].Rect.left ;
   bitmapRect.top = Rect.top + buttons[index].bitmapRect.top - buttons[index].Rect.top ;
   bitmapRect.left = Rect.top + buttons[index].bitmapRect.left - buttons[index].Rect.left ;
   bitmapRect.bottom = Rect.top + buttons[index].bitmapRect.bottom - buttons[index].bitmapRect.top ;
   bitmapRect.right = Rect.top + buttons[index].bitmapRect.right - buttons[index].bitmapRect.left ;

   if (lpdis->itemState & ODS_FOCUS)                                      /* if button has focus */
      DrawFocusRect(lpdis->hDC, &lpdis->rcItem) ;

   if (buttons[index].BitmapFn[0] > ' ' && buttons[index].handle == NULL &&
       !buttons[index].loaderror)                                         /* if bitmap exists, is known and hasn't a loaderror */
      WinExeLoadBitmap(buttons[index].hWnd, 0, buttons[index].BitmapFn, (LPRECT) &bitmapRect,
                       (LPRECT) &orgRect, (HANDLE far *) &buttons[index].handle) ;                 /* then load it */
   if (buttons[index].handle == NULL)                                     /* if the handle is empty */
      buttons[index].loaderror = 1 ;                                      /* we have a loaderror (remember it) */

   if (buttons[index].Type == IDC_BUTTON_BMP)                             /* if it's a bitmap button */
      {
      if (buttons[index].BitmapFn[0] > ' ' && buttons[index].handle != NULL)             /* and it exists and is known */
         {
         hDCMem = CreateCompatibleDC(hDC) ;                               /* create a compatible area */
         hBitOld = SelectObject(hDCMem, buttons[index].handle) ;          /* copy the bitmap */
         if (lpdis->itemState & ODS_SELECTED)                             /* button pushed ? */
            BitBlt(hDC, Rect.left + 1,
                        Rect.top + 1,
                        bitmapRect.right,
                        bitmapRect.bottom,
                        hDCMem, 0, 0, SRCCOPY) ;                          /* display the bitmap */
         else
            BitBlt(hDC, Rect.left,
                        Rect.top,
                        bitmapRect.right,
                        bitmapRect.bottom,
                        hDCMem, 0, 0, SRCCOPY) ;                          /* display the bitmap */
         SelectObject(hDCMem, hBitOld) ;                                  /* get the old bitmap */
         DeleteDC(hDCMem) ;                                               /* free the reserved memory */
         }
      return ;                                                            /* return to calling function */
      }

   if (buttons[index].Type == IDC_BUTTON_CHECK)                           /* if it's this button */
      {
      hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)) ;                       /* create this pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      Rectangle(hDC, Rect.left,
                     Rect.top + 6,
                     Rect.left + fonts[buttons[index].LabelFont].lfHeight - 2,
                     Rect.bottom + - 6) ;                                 /* draw the rectangle */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen                */
      }

   if (buttons[index].Type == IDC_BUTTON_RADIO)                           /* if it's a radio button */
      {
      hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)) ;                       /* create this pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      wide = fonts[buttons[index].LabelFont].lfHeight - 2 ;               /* height of button font -2 */
      MoveTo(hDC, 2 + Rect.left + (wide / 2),
                  6 + Rect.top) ;
      LineTo(hDC, 2 + Rect.left + wide,
                  6 + Rect.top + (wide / 2)) ;
      MoveTo(hDC, 2 + Rect.left + wide,
                  6 + Rect.top + (wide / 2)) ;
      LineTo(hDC, 2 + Rect.left + (wide / 2),
                  6 + Rect.top + wide) ;
      MoveTo(hDC, 2 + Rect.left + (wide / 2),
                  6 + Rect.top + wide) ;
      LineTo(hDC, 2 + Rect.left,
                  6 + Rect.top + (wide / 2)) ;
      MoveTo(hDC, 2 + Rect.left,
                  6 + Rect.top + (wide / 2)) ;
      LineTo(hDC, 2 + Rect.left + (wide / 2),
                  6 + Rect.top) ;                                         /* draw the radiobutton frame */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen                */
      }

   switch(buttons[index].AutoButton)                                      /* identify autobutton type */
      {
      case IDC_BUTTON_OK:                                                 /* these buttons are bitmapped based */
      case IDC_BUTTON_CANCEL:
      case IDC_BUTTON_DEL:
      case IDC_BUTTON_HELP:
      case IDC_BUTTON_YES:
      case IDC_BUTTON_NO:
      case IDC_BUTTON_ABORT:
      case IDC_BUTTON_RETRY:
      case IDC_BUTTON_IGNORE:

         if (buttons[index].Type != IDC_BUTTON_PUSH || buttons[index].BitmapFn[0] >= ' ' ||
             buttons[index].Label[0] >= ' ')                              /* these take precedence */
            break ;

         if (buttons[index].handle != NULL)
            DeleteObject(buttons[index].handle) ;
         /* if one of the followin AutoButtons is selected and known, load it */
         switch(buttons[index].AutoButton)
            {
            case IDC_BUTTON_OK:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_OK_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_OK") ;
               break ;
            case IDC_BUTTON_DEL:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_DEL_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_DEL") ;
               break ;
            case IDC_BUTTON_CANCEL:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_CANCEL_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_CANCEL") ;
               break ;
            case IDC_BUTTON_HELP:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_HELP_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_HELP") ;
               break ;
            case IDC_BUTTON_YES:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_YES_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_YES") ;
               break ;
            case IDC_BUTTON_NO:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_NO_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_NO") ;
               break ;
            case IDC_BUTTON_ABORT:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_ABORT_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_ABORT") ;
               break ;
            case IDC_BUTTON_RETRY:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_RETRY_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_RETRY") ;
               break ;
            case IDC_BUTTON_IGNORE:
               if (lpdis->itemState & ODS_SELECTED)
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_IGNORE_DOWN") ;
               else
                  buttons[index].handle = LoadBitmap(hInst, "BUTTON_IGNORE") ;
               break ;
            }
         hDCMem = CreateCompatibleDC(hDC) ;                               /* create a compatible area */
         hBitOld = SelectObject(hDCMem, buttons[index].handle) ;          /* copy the bitmap */
         BitBlt(hDC, Rect.left, Rect.top, 63, 39, hDCMem, 0, 0, SRCCOPY) ;     /* display the button */
         SelectObject(hDCMem, hBitOld) ;                                  /* re-select old bitmap */
         DeleteDC(hDCMem) ;                                               /* free reserved memory (device context) */
         return ;                                                         /* return to calling function */
      }

   if (buttons[index].Type == IDC_BUTTON_PUSH)                            /* if it's a push button */
      {
      brush.lbStyle = BS_SOLID ;                                          /* define brush style */
      brush.lbColor = RGB(192, 192, 192) ;                                /* define brush color */
      brush.lbHatch = NULL ;                                              /* define brush pattern */
      hBrush = CreateBrushIndirect(&brush) ;                              /* create this brush */
      hOldBrush = SelectObject(hDC, hBrush) ;                             /* select it */
      hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)) ;                       /* create this pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      Rectangle(hDC, Rect.left,
                     Rect.top,
                     Rect.right + 1,
                     Rect.bottom + 1) ;                                   /* draw this rectangle */
      SelectObject(hDC, hOldBrush) ;                                      /* re-select the old brush */
      DeleteObject(hBrush) ;                                              /* delete the newer brush */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen                */

      if (lpdis->itemState & ODS_SELECTED)                                /* button pushed ? */
         {
         hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)) ;                    /* create black pen */
         hOldPen = SelectObject(hDC, hPen) ;                              /* select the pen into the device context */
         MoveTo(hDC, Rect.left + 1,
                     Rect.top + 1) ;
         LineTo(hDC, Rect.right - 1,
                     Rect.top + 1) ;
         MoveTo(hDC, Rect.left + 1,
                     Rect.top + 2) ;
         LineTo(hDC, Rect.right - 1,
                     Rect.top + 2) ;

         MoveTo(hDC, Rect.left + 1,
                     Rect.top + 3) ;
         LineTo(hDC, Rect.left + 1,
                     Rect.bottom - 1) ;
         MoveTo(hDC, Rect.left + 2,
                     Rect.top + 3) ;
         LineTo(hDC, Rect.left + 2,
                     Rect.bottom - 1) ;

         MoveTo(hDC, Rect.right - 1,
                     Rect.top + 2) ;
         LineTo(hDC, Rect.right - 1,
                     Rect.bottom - 1) ;
         MoveTo(hDC, Rect.left + 2,
                     Rect.bottom - 1) ;
         LineTo(hDC, Rect.right,
                     Rect.bottom - 1) ;                                      /* draw this */
         SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
         DeleteObject(hPen) ;                                                /* delete the newer pen                */
         }
      else
         {
         hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255)) ;                 /* create this pen */
         hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
         MoveTo(hDC, Rect.left + 1,
                     Rect.top + 1) ;
         LineTo(hDC, Rect.right - 2,
                     Rect.top + 1) ;
         MoveTo(hDC, Rect.left + 1,
                     Rect.top + 2) ;
         LineTo(hDC, Rect.right - 3,
                     Rect.top + 2) ;
         MoveTo(hDC, Rect.left + 1,
                     Rect.top + 3) ;
         LineTo(hDC, Rect.left + 1,
                     Rect.bottom - 1) ;
         MoveTo(hDC, Rect.left + 2,
                     Rect.top + 3) ;
         LineTo(hDC, Rect.left + 2,
                     Rect.bottom - 2) ;                                      /* draw this */
         SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
         DeleteObject(hPen) ;                                                /* delete the newer pen                */

         hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128)) ;                 /* create the newer pen */
         hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
         MoveTo(hDC, Rect.right - 2,
                     Rect.top + 3) ;
         LineTo(hDC, Rect.right - 2,
                     Rect.bottom - 1) ;
         MoveTo(hDC, Rect.right - 1,
                     Rect.top + 2) ;
         LineTo(hDC, Rect.right - 1,
                     Rect.bottom - 1) ;
         MoveTo(hDC, Rect.left + 3,
                     Rect.bottom - 2) ;
         LineTo(hDC, Rect.right,
                     Rect.bottom - 2) ;
         MoveTo(hDC, Rect.left + 2,
                     Rect.bottom - 1) ;
         LineTo(hDC, Rect.right,
                     Rect.bottom - 1) ;                                      /* draw this */
         SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
         DeleteObject(hPen) ;                                                /* delete the newer pen                */
         }
      }

   /* so button was pushed, there is a bitmap and the button has a handle to it */
   if (buttons[index].Type == IDC_BUTTON_PUSH && buttons[index].BitmapFn[0] > ' ' && buttons[index].handle != NULL)
      {
      hDCMem = CreateCompatibleDC(hDC) ;                                  /* create a device context for memory */
      hBitOld = SelectObject(hDCMem, buttons[index].handle) ;             /* remember old bitmap */

      yoff = (Rect.bottom - Rect.top - buttons[index].bitmapRect.bottom) / 2 ;      /* center the bitmap vertically */

      if (lpdis->itemState & ODS_SELECTED)                                /* button pushed ? */
         BitBlt(hDC, Rect.left + 6,
                     Rect.top + yoff + 1,
                     bitmapRect.right + 1,
                     bitmapRect.bottom + 1,
                     hDCMem, 0, 0, SRCCOPY) ;                                /* display the bitmap */
      else
         BitBlt(hDC, Rect.left + 5,
                     Rect.top + yoff,
                     bitmapRect.right,
                     bitmapRect.bottom,
                     hDCMem, 0, 0, SRCCOPY) ;                                /* display the bitmap */
      SelectObject(hDCMem, hBitOld) ;                                     /* re-select old bitmap */
      DeleteDC(hDCMem) ;                                                  /* delete device context for bitmap memory */
      }
   if (buttons[index].Label[0] >= ' ')                                    /* if the label is a printable character */
      {
#if !defined(__WC386__)
      hFont = CreateFontIndirect(&fonts[buttons[index].LabelFont]) ;      /* load labelfont */
#else
      hFont = CreateFontIndirect((LPLOGFONT)&fonts[buttons[index].LabelFont]) ;      /* load labelfont */
#endif
      hOldFont = SelectObject(hDC, hFont) ;                               /* set the new display font */
      dwSize1 = GetTextExtent(hDC, (LPSTR) buttons[index].Label, lstrlen((LPSTR) buttons[index].Label)) ; /* get stringsize */
      wide = LOWORD(dwSize1) ;                                            /* obtain its width (lower 16bit of dwSize) */
      high = HIWORD(dwSize1) ;                                            /* obtain its height (upper 16bit of dwSize) */

      yoff = (Rect.bottom - Rect.top - high) / 2 ;                        /* center rectangle vertically */
      xoff = (Rect.right - Rect.left - wide) / 2 ;                        /* and horizontally */
      if (yoff < 4)                                                       /* if it's too far up */
         yoff = 4 ;                                                       /* move it down */
      if (xoff < 4)                                                       /* if it's too far left */
         xoff = 4 ;                                                       /* move it right */
      if (lpdis->itemState & ODS_SELECTED)                                /* button pushed ? */
         {
         xoff++ ;
         yoff++ ;
         }

      hRgn = CreateRectRgn(Rect.left + 4,
                           Rect.top + 4,
                           Rect.right - 4,
                           Rect.bottom - 4) ;                             /* create a region for display rectangle */
      SelectClipRgn(hDC, hRgn) ;                                          /* select it as active clip region */
      if (buttons[index].handle != NULL && buttons[index].BitmapFn[0] > ' ')  /* if there's a button handle and a bitmap for the button */
         xoff = buttons[index].bitmapRect.right + 10 ;                    /* move the text right (to make room for bitmap) */
      dwFG = SetTextColor(hDC, RGB(0, 0, 0)) ;                            /* set the foreground color */
      dwBG = SetBkColor(hDC, RGB(192, 192, 192)) ;                        /* set the background color */
      if (buttons[index].Type != IDC_BUTTON_PUSH)                         /* if button wasn't pushed */
         SetBkColor(hDC, dwBG) ;                                          /* restore the old background color */

      TextOut(hDC, Rect.left + xoff,
                   Rect.top + yoff,
#if !defined(__WC386__)
                   buttons[index].Label, lstrlen((LPSTR) buttons[index].Label)) ;   /* display the text in the rectangle */
#else
                   (LPSTR)buttons[index].Label, lstrlen((LPSTR) buttons[index].Label)) ;   /* display the text in the rectangle */
#endif

      SetTextColor(hDC, dwFG) ;                                           /* restore the old foreground color */
      SetBkColor(hDC, dwBG) ;                                             /* restore the old background color */
      DeleteObject(hRgn) ;                                                /* delete the region */
      SelectObject(hDC, hOldFont) ;                                       /* reselect the old font */
      DeleteObject(hFont) ;                                               /* delete the font */
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    CheckButtonSize (hWnd, index)

   PURPOSE:     Check button size
                hWnd:           window handle
                index:          index of button in use

*****************************************************************************/

void CheckButtonSize(HWND hWnd, int index)
   {
   HDC  hDC ;                                                             /* handle to device context */
   char buffer[129] ;                                                     /* character buffer */
   int wide, high ;                                                       /* used for size values */
   DWORD dwSize1 ;                                                        /* used for textsize */
   HFONT   hFont;                                                         /* used to create the font to display the text */
   HFONT   hOldFont;                                                      /* used to hold the old font */
   RECT    Rect ;                                                         /* used for the rectangle */

   if (buttons[index].Type != IDC_BUTTON_PUSH || buttons[index].Label[0] < ' ')   /* if button not activated of has no printable value */
      return ;                                                            /* return to calling function */

   switch(buttons[index].AutoButton)                                      /* identify AutoButton */
      {
      case IDC_BUTTON_OK:                                                 /* these buttons are bitmapped based */
      case IDC_BUTTON_CANCEL:
      case IDC_BUTTON_HELP:
      case IDC_BUTTON_YES:
      case IDC_BUTTON_NO:
      case IDC_BUTTON_ABORT:
      case IDC_BUTTON_RETRY:
      case IDC_BUTTON_IGNORE:
         return ;                                                         /* return to calling function */
      }

   lstrcpy((LPSTR) buffer, (LPSTR) buttons[index].Label) ;                /* copy the name */

   hDC = GetDC(hWnd) ;                                                    /* get window device context  */
#if !defined(__WC386__)
   hFont = CreateFontIndirect(&fonts[buttons[index].LabelFont]) ;         /* load new font */
#else
   hFont = CreateFontIndirect((LPLOGFONT)&fonts[buttons[index].LabelFont]) ;         /* load new font */
#endif
   hOldFont = SelectObject(hDC, hFont) ;                                  /* set the new display font */
   dwSize1 = GetTextExtent(hDC, (LPSTR) buffer, lstrlen((LPSTR)buffer)) ; /* calculate string size */
   SelectObject(hDC, hOldFont) ;                                          /* reselect the old font */
   DeleteObject(hFont) ;                                                  /* delete the font */

   wide = LOWORD(dwSize1) ;                                               /* get width of text */
   high = HIWORD(dwSize1) ;                                               /* get height of text */
   if (abs(fonts[buttons[index].LabelFont].lfHeight) > high)              /* if preset height is greater than calculated height */
      high = abs(fonts[buttons[index].LabelFont].lfHeight) ;              /* set it to preset height */

   /* define rectangle according to calculated text size */
   Rect.top = buttons[index].Rect.top ;                                   /* top */
   Rect.left = buttons[index].Rect.left ;                                 /* left */
   Rect.bottom = buttons[index].Rect.top + high + 10 ;                    /* bottom (top+height+10) */
   Rect.right = buttons[index].Rect.left + wide + 16 ;                    /* right (left+width+16) */

   if (buttons[index].BitmapFn[0] > ' ')                                  /* if there's a bitmap for the button */
      {
      Rect.right += 10 + buttons[index].bitmapRect.right - buttons[index].bitmapRect.left ; /* adjust rectangle to display it */
      if (Rect.bottom - Rect.top < buttons[index].bitmapRect.bottom + 10)  /* if rectangleheight to small for bitmap (and 10 offset) */
         Rect.bottom = Rect.top + buttons[index].bitmapRect.bottom + 10 ;  /* adjust it */
      }

   if (Rect.bottom > buttons[index].Rect.bottom)                          /* adjust rectangle (according to bitmap size, */
      buttons[index].Rect.bottom = buttons[index].Rect.top + high + 10 ;
   if (Rect.right > buttons[index].Rect.right)                            /*      text size and offset values            */
      buttons[index].Rect.right = buttons[index].Rect.left + wide + 16 ;

   ReleaseDC(hWnd, hDC) ;                                                 /* release device context (free no longer used memory) */
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    PaintText (hWnd, index, id, rcPaint)

   PURPOSE:     Paint text
                hWnd:           window handle
                index:          set to -1, gets index of texttype for menu type
                id:             menu type
                rcPaint:        paint structure

*****************************************************************************/

void PaintText(HWND hWnd, int index, int id, LPRECT rcPaint)
   {
   DWORD   dwFG, dwBG;                                                    /* hold old FG/BG colors */
   HFONT   hFont;                                                         /* Used to create the font to display the text */
   HFONT   hOldFont;                                                      /* Used to hold the old font */
   HDC     hDC;                                                           /* Display context for showing the text */
   int oldmode, formindex ;
   RECT intrect, Rect ;                                                   /* used for rectangles */
   DWORD dwSize1 ;                                                        /* used for text size */

   if (index < 0)                                                         /* if index < 0 (was initially set to -1) */
      index = FindIndex(TEXT_TYPE, id) ;                                  /* set index to text type for menu type */
   formindex = FindIndex(FORM_TYPE, text[index].formid) ;                 /* set formindex for it */

   hDC = GetDC(hWnd) ;                                                    /* Get DC for window to show text */
   dwFG = SetTextColor(hDC, text[index].fgColor) ;                        /* set the foreground color */
   if (!text[index].NoBg)                                                 /* if background color not set */
      dwBG = SetBkColor(hDC, text[index].bgColor) ;                       /* set the background color */
   else                                                                   /* or set it transparent */
      oldmode = SetBkMode(hDC, TRANSPARENT) ;

#if !defined(__WC386__)
   hFont = CreateFontIndirect(&fonts[text[index].LogFont]) ;              /* load new font */
#else
   hFont = CreateFontIndirect((LPLOGFONT)&fonts[text[index].LogFont]) ;              /* load new font */
#endif
   hOldFont = SelectObject(hDC, hFont) ;                                  /* set the new display font */

#if !defined(__WC386__)
   dwSize1 = GetTextExtent(hDC, strings[text[index].string], lstrlen(strings[text[index].string])) ; /* calculate text size */
#else
   dwSize1 = GetTextExtent(hDC, (LPSTR)strings[text[index].string], lstrlen((LPSTR)strings[text[index].string])) ; /* calculate text size */
#endif
   Rect.top = Rect.bottom = text[index].Row ;                             /* adjust rectangle to text size: top */
   Rect.left = Rect.right = text[index].Col ;                             /* left */
   Rect.bottom += HIWORD(dwSize1) ;                                       /* bottom */
   Rect.right += LOWORD(dwSize1) ;                                        /* right */

   if (Rect.right > forms[formindex].xMaxPix)                             /* if window smaller than rectangle */
      forms[formindex].xMaxPix = Rect.right ;                             /* adjust horizontally */
   if (Rect.bottom > forms[formindex].yMaxPix)
      forms[formindex].yMaxPix = Rect.bottom ;                            /* adjust vertically */

   Rect.top += forms[formindex].yOffset ;                                 /* position the rectangle */
   Rect.left += forms[formindex].xOffset ;
   Rect.bottom += forms[formindex].yOffset ;
   Rect.right += forms[formindex].xOffset ;

   if (IntersectRect(&intrect, rcPaint, &Rect))                           /* if rectangle in redraw area */
      {
      TextOut(hDC,
              text[index].Col + forms[formindex].xOffset,
              text[index].Row + forms[formindex].yOffset,
#if !defined(__WC386__)
              strings[text[index].string],
              lstrlen(strings[text[index].string])) ;                     /* paint the text */
#else
              (LPSTR)strings[text[index].string],
              lstrlen((LPSTR)strings[text[index].string])) ;                     /* paint the text */
#endif
      if (rcPaint->top > Rect.top)                                        /* if top of painting area outside view rectangle */
         rcPaint->top = Rect.top ;                                        /* set top of painiting area at top of rectangle */
      if (rcPaint->left > Rect.left)                                      /* if left of painting area outside view rectangle */
         rcPaint->left = Rect.left ;                                      /* set left of painiting area at top of rectangle */
      if (rcPaint->bottom < Rect.bottom)                                  /* if bottom of painting area outside view rectangle */
         rcPaint->bottom = Rect.bottom ;                                  /* set bottom of painiting area at top of rectangle */
      if (rcPaint->right < Rect.right)                                    /* if right of painting area outside view rectangle */
         rcPaint->right = Rect.right ;                                    /* set right of painiting area at top of rectangle */
      }

   SelectObject(hDC, hOldFont) ;                                          /* reselect the old font */
   DeleteObject(hFont) ;                                                  /* delete the font */
   SetTextColor(hDC, dwFG) ;                                              /* restore the old foreground color */
   if (!text[index].NoBg)                                                 /* if background color changed */
      SetBkColor(hDC, dwBG) ;                                             /* restore the old background color */
   else
      SetBkMode(hDC, oldmode) ;                                           /* switch back from transparent */
   ReleaseDC(hWnd, hDC) ;                                                 /* release device context (free unused memory) */

   return ;                                                               /* return to calling function */
   }




/*****************************************************************************

   FUNCTION:    PaintRect (hWnd, index, id, rcPaint)

   PURPOSE:     Paint rectangle
                hWnd:           window handle
                index:          set to -1, gets index of recttype for menu type
                id:             menu type
                rcPaint:        paint structure

*****************************************************************************/

void PaintRect(HWND hWnd, int index, int id, LPRECT rcPaint)
   {
   HDC  hDC ;                                                             /* handle to device context */
   HRGN hRgn ;                                                            /* for creating clipping region */
   HPEN hOldPen, hPen ;                                                   /* handles for old & new pens */
   RECT Rect, intrect ;                                                   /* used for rectangles */
   HBRUSH hOldBrush, hBrush ;                                             /* handles for old & new brushes */
   int    iOldBkMode ;                                                    /* save the old background mode */
   int elipwide, formindex ;

   if (index < 0)                                                         /* if index < 0 (initially set to -1) */
      index = FindIndex(RECT_TYPE, id) ;                                  /* set index to rectangle type for menu type */
   formindex = FindIndex(FORM_TYPE, rects[index].formid) ;                /* set form type for it */

   if (rects[index].Rect.right > forms[formindex].xMaxPix)                /* if rectangles bigger than window */
      forms[formindex].xMaxPix = rects[index].Rect.right ;                /* adjust window horizontally */
   if (rects[index].Rect.bottom > forms[formindex].yMaxPix)
      forms[formindex].yMaxPix = rects[index].Rect.bottom ;               /* adjust window vertically */

   Rect.top = rects[index].Rect.top + forms[formindex].yOffset ;          /* adjust rectangle */
   Rect.left = rects[index].Rect.left + forms[formindex].xOffset ;
   Rect.bottom = rects[index].Rect.bottom + forms[formindex].yOffset ;
   Rect.right = rects[index].Rect.right + forms[formindex].xOffset ;

   if (!IntersectRect(&intrect, rcPaint, &Rect))                          /* if rectangle not in redraw area */
      return ;                                                            /* just return to calling function */
   if (rcPaint->top > Rect.top)                                           /* if top of painting area outside view rectangle */
      rcPaint->top = Rect.top ;                                           /* set top of painiting area at top of rectangle */
   if (rcPaint->left > Rect.left)                                         /* if left of painting area outside view rectangle */
      rcPaint->left = Rect.left ;                                         /* set left of painiting area at top of rectangle */
   if (rcPaint->bottom < Rect.bottom)                                     /* if bottom of painting area outside view rectangle */
      rcPaint->bottom = Rect.bottom ;                                     /* set bottom of painiting area at top of rectangle */
   if (rcPaint->right < Rect.right)                                       /* if right of painting area outside view rectangle */
      rcPaint->right = Rect.right ;                                       /* set right of painiting area at top of rectangle */

   if (rects[index].Rect.right == rects[index].Rect.left &&
       rects[index].Rect.top == rects[index].Rect.bottom)                 /* if rectangle is a point */
      return ;                                                            /* forget it, return to calling function */

   hDC = GetDC(hWnd) ;                                                    /* get window device context */

   iOldBkMode = SetBkMode(hDC, TRANSPARENT) ;                             /* remember old background mode */

   if ((rects[index].bRound == FALSE) && (rects[index].bOval == FALSE) &&
      (rects[index].bOutline == TRUE))                                    /* if region not round, not oval, but be outlined */
      {
      if (rects[index].bOpaque || !rects[index].bFill)                    /* if region opaque or not to be filled */
         {
         hBrush = PaintRectBack(index) ;                                  /* load brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* use the brush */
         Rectangle(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                        rects[index].Rect.top + forms[formindex].yOffset,
                        rects[index].Rect.right + forms[formindex].xOffset,
                        rects[index].Rect.bottom + forms[formindex].yOffset) ;      /* paint the rectangle */
         SelectObject(hDC, hOldBrush) ;                                   /* re-select old brush */
         DeleteObject(hBrush) ;                                           /* delete the brush */
         }
      hPen = CreatePen(PS_SOLID, rects[index].iWidth, rects[index].bordColor) ;     /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      hBrush = PaintRectFill(index) ;                                     /* load brush */
      hOldBrush = SelectObject(hDC, hBrush) ;                             /* select the brush */
      Rectangle(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                     rects[index].Rect.top + forms[formindex].yOffset,
                     rects[index].Rect.right + forms[formindex].xOffset,
                     rects[index].Rect.bottom + forms[formindex].yOffset) ;         /* paint the rectangle */
      SelectObject(hDC, hOldBrush) ;                                      /* re-select old brush */
      DeleteObject(hBrush) ;                                              /* delete newer brush */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */
      }

   /* now we need to process partial boxes (top, bottom, right, left) */
   if ((rects[index].bRound == FALSE) && (rects[index].bOval == FALSE) &&
      (rects[index].bOutline == FALSE))                                   /* if region not round, not oval, and not outlined */
      {
      if (rects[index].bOpaque || !rects[index].bFill)                    /* if region should be opaque, or not filled */
         {
         hBrush = PaintRectBack(index) ;                                  /* load brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* select brush */
         hRgn = CreateRectRgn(rects[index].Rect.left + forms[formindex].xOffset,
                              rects[index].Rect.top + forms[formindex].yOffset,
                              rects[index].Rect.right + forms[formindex].xOffset,
                              rects[index].Rect.bottom + 1 + forms[formindex].yOffset) ;      /* create a region */
         SelectClipRgn(hDC, hRgn) ;                                       /* select region */
         Rectangle(hDC, rects[index].Rect.left - 10 + forms[formindex].xOffset,
                        rects[index].Rect.top - 10 + forms[formindex].yOffset,
                        rects[index].Rect.right + 10 + forms[formindex].xOffset,
                        rects[index].Rect.bottom + 10 + forms[formindex].yOffset) ;           /* draw rectangle */
         SelectObject(hDC, hOldBrush) ;                                   /* re-select old brush */
         DeleteObject(hBrush) ;                                           /* delete newer brush */
         DeleteObject(hRgn) ;                                             /* delete region */
         }

      hPen = CreatePen(PS_SOLID, rects[index].iWidth, rects[index].bordColor) ;               /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      hBrush = PaintRectFill(index) ;                                     /* load brush */
      hOldBrush = SelectObject(hDC, hBrush) ;                             /* select brush */
      Rectangle(hDC, rects[index].Rect.left - 10 + forms[formindex].xOffset,
                     rects[index].Rect.top - 10 + forms[formindex].yOffset,
                     rects[index].Rect.right + 10 + forms[formindex].xOffset,
                     rects[index].Rect.bottom + 10 + forms[formindex].yOffset) ;              /* draw rectangle */
      GetClientRect(hwnd, &Rect) ;                                        /* get client coordinates */
      hRgn = CreateRectRgn(Rect.left + forms[formindex].xOffset,
                           Rect.top + forms[formindex].yOffset,
                           Rect.right + forms[formindex].xOffset,
                           Rect.bottom + forms[formindex].yOffset) ;      /* create a region for this coordinates plus offset */
      SelectClipRgn(hDC, hRgn) ;                                          /* select that region as current clipping region */
      SelectObject(hDC, hOldBrush) ;                                      /* re-select old brush */
      DeleteObject(hBrush) ;                                              /* delete newer brush */

      if (rects[index].bTop == TRUE)                                      /* if top should be drawn */
         {
         MoveTo(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                     rects[index].Rect.top + forms[formindex].yOffset) ;
         LineTo(hDC, rects[index].Rect.right + 1 + forms[formindex].xOffset,
                     rects[index].Rect.top + forms[formindex].yOffset) ;            /* we do it */
         }
      if (rects[index].bLeft == TRUE)                                     /* if the left side should be drawn */
         {
         MoveTo(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                     rects[index].Rect.top + forms[formindex].yOffset) ;
         LineTo(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                     rects[index].Rect.bottom + 1 + forms[formindex].yOffset) ;     /* we do it */
         }
      if (rects[index].bRight == TRUE)                                    /* if the right side should be drawn */
         {
         MoveTo(hDC, rects[index].Rect.right - 1 + forms[formindex].xOffset,
                     rects[index].Rect.top + forms[formindex].yOffset) ;
         LineTo(hDC, rects[index].Rect.right - 1 + forms[formindex].xOffset,
                     rects[index].Rect.bottom + 1 + forms[formindex].yOffset) ;     /* we do it */
         }
      if (rects[index].bBottom == TRUE)                                   /* if the bottom should be drawn */
         {
         MoveTo(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                     rects[index].Rect.bottom + forms[formindex].yOffset) ;
         LineTo(hDC, rects[index].Rect.right + 1 + forms[formindex].xOffset,
                     rects[index].Rect.bottom + forms[formindex].yOffset) ;         /* we do it */
         }

      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */
      DeleteObject(hRgn) ;                                                /* delete the region */
      }

   /* Now process the oval types  */
   if ((rects[index].bRound == FALSE) && (rects[index].bOval == TRUE) &&
      (rects[index].bOutline == TRUE))                                    /* if it's not round, but oval and outlined */
      {
      if (rects[index].bOpaque || !rects[index].bFill)                    /* if it's opaque, or not filled */
         {
         hBrush = PaintRectBack(index) ;                                  /* load brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* select brush */
         Ellipse(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                      rects[index].Rect.top + forms[formindex].yOffset,
                      rects[index].Rect.right + forms[formindex].xOffset,
                      rects[index].Rect.bottom + forms[formindex].yOffset) ;        /* draw the ellipse */
         SelectObject(hDC, hOldBrush) ;                                   /* re-select the old brush */
         DeleteObject(hBrush) ;                                           /* delete the newer brush */
         }
      hPen = CreatePen(PS_SOLID, rects[index].iWidth, rects[index].bordColor) ;     /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      hBrush = PaintRectFill(index) ;                                     /* load brush */
      hOldBrush = SelectObject(hDC, hBrush) ;                             /* select brush */
      Ellipse(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                   rects[index].Rect.top + forms[formindex].yOffset,
                   rects[index].Rect.right + forms[formindex].xOffset,
                   rects[index].Rect.bottom + forms[formindex].yOffset) ;           /* draw ellipse */
      SelectObject(hDC, hOldBrush) ;                                      /* re-select old brush */
      DeleteObject(hBrush) ;                                              /* delete newer brush */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */
      }

   /* Now process the Rounded rectangle types  */
   if ((rects[index].bRound == TRUE) && (rects[index].bOval == FALSE) &&
      (rects[index].bOutline == TRUE))                                    /* if round and outlined, and not oval */
      {                                                                   /* if rectangle smaller than high */
      if (rects[index].Rect.right - rects[index].Rect.left < rects[index].Rect.bottom - rects[index].Rect.top)
         elipwide = (rects[index].Rect.right - rects[index].Rect.left) / 3 ;   /* set elipwide to rectanglewidth / 3 */
      else
         elipwide = (rects[index].Rect.bottom - rects[index].Rect.top) / 3 ;   /* set elipwide to rectangleheight / 3 */
      if (rects[index].bOpaque || !rects[index].bFill)                    /* if rectangle opaque, or not filled */
         {
         hBrush = PaintRectBack(index) ;                                  /* load brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* select brush */
         RoundRect(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                        rects[index].Rect.top + forms[formindex].yOffset,
                        rects[index].Rect.right + forms[formindex].xOffset,
                        rects[index].Rect.bottom + forms[formindex].yOffset,
                        elipwide, elipwide) ;                             /* draw rectangle with rounded corners */
         SelectObject(hDC, hOldBrush) ;                                   /* re-select old brush */
         DeleteObject(hBrush) ;                                           /* delete newer brush */
         }
      hPen = CreatePen(PS_SOLID, rects[index].iWidth, rects[index].bordColor) ;     /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      hBrush = PaintRectFill(index) ;                                     /* load new brush */
      hOldBrush = SelectObject(hDC, hBrush) ;                             /* select brush */
      RoundRect(hDC, rects[index].Rect.left + forms[formindex].xOffset,
                     rects[index].Rect.top + forms[formindex].yOffset,
                     rects[index].Rect.right + forms[formindex].xOffset,
                     rects[index].Rect.bottom + forms[formindex].yOffset,
                     elipwide, elipwide) ;                                /* draw rectangle with rounded corners */
      SelectObject(hDC, hOldBrush) ;                                      /* re-select old brush */
      DeleteObject(hBrush) ;                                              /* delete newer brush */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */
      }

   SetBkMode(hDC, iOldBkMode) ;                                           /* reset the old background mode */
   ReleaseDC(hWnd, hDC) ;                                                 /* release the device context */
   }


/*****************************************************************************

   FUNCTION:    PaintLine (hWnd, index, id, rcPaint)

   PURPOSE:     Paint line
                hWnd:           window handle
                index:          set to -1, gets index of linetype for menu type
                id:             menu type
                rcPaint:        paint structure

*****************************************************************************/

void PaintLine(HWND hWnd, int index, int id, LPRECT rcPaint)
   {
   HDC  hDC ;                                                             /* handle to device context */
   HPEN hOldPen, hPen ;                                                   /* handles for old & new pens */
   RECT intrect, Rect ;                                                   /* used for rectangles */
   int formindex, i ;

   if (index < 0)                                                         /* if index < 0 (initially set to -1) */
      index = FindIndex(LINE_TYPE, id) ;                                  /* set index to line type for menu type */
   formindex = FindIndex(FORM_TYPE, lines[index].formid) ;                /* set formindex for it */

   if (lines[index].Rect.right > forms[formindex].xMaxPix)                /* if lines go beyond window area */
      forms[formindex].xMaxPix = lines[index].Rect.right ;                /* adjust window horizontally */
   if (lines[index].Rect.bottom > forms[formindex].yMaxPix)
      forms[formindex].yMaxPix = lines[index].Rect.bottom ;               /* adjust window vertically */

   Rect.top = lines[index].Rect.top + forms[formindex].yOffset ;          /* adjust rectangle */
   Rect.left = lines[index].Rect.left + forms[formindex].xOffset ;
   Rect.bottom = lines[index].Rect.bottom + forms[formindex].yOffset ;
   Rect.right = lines[index].Rect.right + forms[formindex].xOffset ;

   if (Rect.top > Rect.bottom)                                            /* if top lower than bottom */
      {
      i = Rect.top ;                                                      /* reverse values */
      Rect.top = Rect.bottom ;
      Rect.bottom = i ;
      }
   if (Rect.left > Rect.right)                                            /* if left right of right */
      {
      i = Rect.left ;                                                     /* reverse values */
      Rect.left = Rect.right ;
      Rect.right = i ;
      }
   if (Rect.top == Rect.bottom)                                           /* if top on bottom */
      Rect.bottom++ ;                                                     /* move the bottom one step down */
   if (Rect.left == Rect.right)                                           /* if left on right */
      Rect.right++ ;                                                      /* move right one step to the right */

   if (!IntersectRect(&intrect, rcPaint, &Rect))                          /* if nothing in active area */
      return ;                                                            /* just return to calling function */
   if (rcPaint->top > Rect.top)                                           /* if top of painting area outside view rectangle */
      rcPaint->top = Rect.top ;                                           /* set top of painiting area at top of rectangle */
   if (rcPaint->left > Rect.left)                                         /* if left of painting area outside view rectangle */
      rcPaint->left = Rect.left ;                                         /* set left of painiting area at top of rectangle */
   if (rcPaint->bottom < Rect.bottom)                                     /* if bottom of painting area outside view rectangle */
      rcPaint->bottom = Rect.bottom ;                                     /* set bottom of painiting area at top of rectangle */
   if (rcPaint->right < Rect.right)                                       /* if right of painting area outside view rectangle */
      rcPaint->right = Rect.right ;                                       /* set right of painiting area at top of rectangle */

   if (lines[index].Rect.right == lines[index].Rect.left &&
       lines[index].Rect.top == lines[index].Rect.bottom)                 /* if lines have no length */
      return ;                                                            /* forget it, return to calling function */

   hDC = GetDC(hWnd) ;                                                    /* get window device context */
   hPen = CreatePen(lines[index].Style, lines[index].Width, lines[index].Color) ;   /* create pen */
   hOldPen = SelectObject(hDC, hPen) ;                                    /* select pen */
   MoveTo(hDC, lines[index].Rect.left + forms[formindex].xOffset,
               lines[index].Rect.top + forms[formindex].yOffset) ;
   LineTo(hDC, lines[index].Rect.right + forms[formindex].xOffset,
               lines[index].Rect.bottom + forms[formindex].yOffset) ;     /* draw the line */
   SelectObject(hDC, hOldPen) ;                                           /* re-select the old pen back into the dc */
   DeleteObject(hPen) ;                                                   /* delete the newer pen */
   ReleaseDC(hWnd, hDC) ;                                                 /* release the device context */
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    PaintField (hWnd, index, id, rcPaint)

   PURPOSE:     Paint field
                hWnd:           window handle
                index:          set to -1, gets index of fieldtype for menu type
                id:             menu type
                rcPaint:        paint structure

*****************************************************************************/

void PaintField(HWND hWnd, int index, int id, LPRECT rcPaint)
   {
   HDC  hDC ;                                                             /* handle to device context */
   HRGN hRgn ;                                                            /* for creating clipping region */
   HPEN hOldPen, hPen ;                                                   /* handles for old & new pens */
   DWORD   dwFG ;                                                         /* hold old FG/BG colors */
   HFONT   hFont;                                                         /* Used to create the font to display the text */
   HFONT   hOldFont;                                                      /* Used to hold the old font */
   LOGBRUSH brush ;
   HBRUSH hOldBrush, hBrush ;                                             /* handles for old & new brushes */
   int  i, j, xoff, yoff, oldmode, unlock, formindex ;
   int listcount, listwide, listhigh, listvert, listhorz, xbutoff, ybutoff ;
   int twolines = 0 ;
   DWORD dwSize1, width ;
   RECT intrect, Rect ;

   if (index < 0)                                                         /* if index < 0 (initially set to -1) */
      index = FindIndex(FIELD_TYPE, id) ;                                 /* set index to fieldtype for menu type */
   formindex = FindIndex(FORM_TYPE, fields[index]->formid) ;               /* set formindex for it */
   if (fields[index]->Rect.right > forms[formindex].xMaxPix)               /* if fieldborders outside window */
      forms[formindex].xMaxPix = fields[index]->Rect.right ;               /* adjust window horizontally */
   if (fields[index]->Rect.bottom > forms[formindex].yMaxPix)
      forms[formindex].yMaxPix = fields[index]->Rect.bottom ;              /* adjust window vertically */

   Rect.top = fields[index]->Rect.top + forms[formindex].yOffset ;         /* adjust rectangle */
   Rect.left = fields[index]->Rect.left + forms[formindex].xOffset ;
   Rect.bottom = fields[index]->Rect.bottom + forms[formindex].yOffset ;
   Rect.right = fields[index]->Rect.right + forms[formindex].xOffset ;

   if (!IntersectRect(&intrect, rcPaint, &Rect))                          /* if field not in visible area */
      return ;                                                            /* forget it, return to calling function */
   if (PaintFieldValueOnly == WINEXE_PAINTCURFLDONLY &&
       forms[currentformindex].curfld != index)                           /* if fieldpaint only, and it's not this field */
      return ;                                                            /* don't bother, return to calling function */
   if (rcPaint->top > Rect.top)                                           /* if top of painting area outside view rectangle */
      rcPaint->top = Rect.top ;                                           /* set top of painiting area at top of rectangle */
   if (rcPaint->left > Rect.left)                                         /* if left of painting area outside view rectangle */
      rcPaint->left = Rect.left ;                                         /* set left of painiting area at top of rectangle */
   if (rcPaint->bottom < Rect.bottom)                                     /* if bottom of painting area outside view rectangle */
      rcPaint->bottom = Rect.bottom ;                                     /* set bottom of painiting area at top of rectangle */
   if (rcPaint->right < Rect.right)                                       /* if right of painting area outside view rectangle */
      rcPaint->right = Rect.right ;                                       /* set right of painiting area at top of rectangle */

   if (!PaintFieldValueOnly)
      {
      hDC = GetDC(hWnd) ;                                                 /* get window device context           */

      if (fields[index]->NoBg)                                             /* if no background color set */
         {
         fields[index]->ValueBg = GetPixel(hDC,
                                          fields[index]->Rect.right - 2 + forms[formindex].xOffset,
                                          fields[index]->Rect.top + 2 + forms[formindex].yOffset) ; /* get current bg color */
         fields[index]->LabelBg = fields[index]->ValueBg ;                  /* set it as label background color */
         fields[index]->NoBg = 0 ;                                         /* remember: background color set */
         }
      brush.lbStyle = BS_SOLID ;                                          /* define brush style */
      brush.lbColor = fields[index]->ValueBg ;                             /* define brush color */
      brush.lbHatch = NULL ;                                              /* define brush pattern */
      hBrush = CreateBrushIndirect(&brush) ;                              /* create brush */
      hOldBrush = SelectObject(hDC, hBrush) ;                             /* select brush */
      hPen = CreatePen(PS_NULL, 1, fields[index]->ValueBg) ;               /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      if (!fields[index]->HideLabel)                                       /* if label isn't hidden */
         Rectangle(hDC, fields[index]->Rect.left + forms[formindex].xOffset,
                        fields[index]->Rect.top + forms[formindex].yOffset,
                        fields[index]->Rect.right + forms[formindex].xOffset,
                        fields[index]->Rect.bottom + forms[formindex].yOffset) ;     /* paint the rectangle */
      SelectObject(hDC, hOldBrush) ;                                      /* re-select old brush */
      DeleteObject(hBrush) ;                                              /* delete newer brush */
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */

      hPen = CreatePen(PS_SOLID, 1, fields[index]->BorderColor) ;          /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */

      xoff = yoff = 0 ;                                                   /* clear variables */
      if (fields[index]->bTop == TRUE || fields[index]->bOutline == TRUE)   /* if top line or outline should be drawn */
         {
         yoff = 1 ;                                                       /* internal offset handling (for text) */
         MoveTo(hDC, fields[index]->Rect.left + forms[formindex].xOffset,
                     fields[index]->Rect.top + forms[formindex].yOffset) ;
         LineTo(hDC, fields[index]->Rect.right + forms[formindex].xOffset,
                     fields[index]->Rect.top + forms[formindex].yOffset) ;      /* paint the top line */
         }
      if (fields[index]->bLeft == TRUE || fields[index]->bOutline == TRUE)  /* if left line or outline should be drawn */
         {
         xoff = 1 ;                                                       /* internal offset handling (for text) */
         MoveTo(hDC, fields[index]->Rect.left + forms[formindex].xOffset,
                     fields[index]->Rect.top + forms[formindex].yOffset) ;
         LineTo(hDC, fields[index]->Rect.left + forms[formindex].xOffset,
                     fields[index]->Rect.bottom + forms[formindex].yOffset) ;   /* draw the left line */
         }
      if (fields[index]->bRight == TRUE || fields[index]->bOutline == TRUE)      /* if right line or outline should be drawn */
         {
         MoveTo(hDC, fields[index]->Rect.right - 1 + forms[formindex].xOffset,
                     fields[index]->Rect.top + forms[formindex].yOffset) ;
         LineTo(hDC, fields[index]->Rect.right - 1 + forms[formindex].xOffset,
                     fields[index]->Rect.bottom + forms[formindex].yOffset) ;   /* draw the right line */
         }
      if (fields[index]->bBottom == TRUE || fields[index]->bOutline == TRUE)     /* if bottom line or outline should be drawn */
         {
         MoveTo(hDC, fields[index]->Rect.left + forms[formindex].xOffset,
                     fields[index]->Rect.bottom - 1 + forms[formindex].yOffset) ;
         LineTo(hDC, fields[index]->Rect.right + forms[formindex].xOffset,
                     fields[index]->Rect.bottom - 1 + forms[formindex].yOffset) ;    /* draw the bottom line */
         }

      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */

      if (fields[index]->FieldType == WINEXE_FLD_BOOL)
         xoff += 16 ;
      if (!fields[index]->HideLabel && fields[index]->Label[0])             /* if label not hidden and not empty (show it!) */
         {
         hRgn = CreateRectRgn(fields[index]->Rect.left + forms[formindex].xOffset,
                     fields[index]->Rect.top + forms[formindex].yOffset,
                     fields[index]->Rect.right + forms[formindex].xOffset,
                     fields[index]->Rect.bottom + forms[formindex].yOffset) ;        /* create a rectangle region */
         SelectClipRgn(hDC, hRgn) ;                                       /* activate region */

         dwFG = SetTextColor(hDC, fields[index]->LabelFg) ;                /* set the foreground color */
         oldmode = SetBkMode(hDC, TRANSPARENT) ;                          /* save the old one */

#if !defined(__WC386__)
         hFont = CreateFontIndirect(&fonts[fields[index]->LabelFont]) ;           /* load font */
#else
         hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[index]->LabelFont]) ;           /* load font */
#endif
         hOldFont = SelectObject(hDC, hFont) ;                            /* set the new display font */
#if !defined(__WC386__)
         dwSize1 = GetTextExtent(hDC, fields[index]->Label, lstrlen(fields[index]->Label)) ;    /* calculate text size */
#else
         dwSize1 = GetTextExtent(hDC, (LPSTR)fields[index]->Label, lstrlen((LPSTR)fields[index]->Label)) ;    /* calculate text size */
#endif

         brush.lbStyle = BS_SOLID ;                                       /* define brush style */
         brush.lbColor = fields[index]->LabelBg ;                          /* define brush color */
         brush.lbHatch = NULL ;                                           /* define brush pattern */
         hBrush = CreateBrushIndirect(&brush) ;                           /* create the brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* select the brush */
         hPen = CreatePen(PS_NULL, 1, fields[index]->LabelBg) ;            /* create pen */
         hOldPen = SelectObject(hDC, hPen) ;                              /* select the pen into the device context */
         if (!fields[index]->NoBg)                                         /* if background selected */
            Rectangle(hDC, fields[index]->Rect.left + xoff + forms[formindex].xOffset,
                           fields[index]->Rect.top + yoff + forms[formindex].yOffset,
                           fields[index]->Rect.left + xoff + LOWORD(dwSize1) + forms[formindex].xOffset,
                           fields[index]->Rect.top + yoff + HIWORD(dwSize1) + forms[formindex].yOffset) ;     /* draw rectangle */
         SelectObject(hDC, hOldBrush) ;                                   /* re-select old brush */
         DeleteObject(hBrush) ;                                           /* delete newer brush */
         SelectObject(hDC, hOldPen) ;                                     /* re-select the old pen back into the dc */
         DeleteObject(hPen) ;                                             /* delete the newer pen */

         TextOut(hDC, fields[index]->Rect.left + xoff + forms[formindex].xOffset,
                      fields[index]->Rect.top + yoff + forms[formindex].yOffset,
#if !defined(__WC386__)
                      fields[index]->Label, lstrlen(fields[index]->Label)) ;          /* write text into rectangle */
#else
                      (LPSTR)fields[index]->Label, lstrlen((LPSTR)fields[index]->Label)) ;          /* write text into rectangle */
#endif

         if (abs(fonts[fields[index]->LabelFont].lfHeight) > HIWORD(dwSize1))     /* if font is higher than reserved space */
            yoff += abs(fonts[fields[index]->LabelFont].lfHeight) ;               /* adjust the offset */
         else
            yoff += HIWORD(dwSize1) ;                                     /* take the standard */
         xoff = 0 ;                                                       /* reset horizontal offset (intern) */
         twolines = 1 ;                                                   /* two lines */
         if (2 * yoff > fields[index]->Rect.bottom - fields[index]->Rect.top)    /* if not enough room for 2nd line of data entry */
            {
            yoff = 0 ;                                                    /* reset vertical offset (intern) */
            xoff = LOWORD(dwSize1) + 4 ;                                  /* adjust horizontal offset (data entry on same line as label) */
            twolines = 0 ;                                                /* not two lines */
            }
         SelectObject(hDC, hOldFont) ;                                    /* reselect the old font */
         DeleteObject(hFont) ;                                            /* Delete the font */
         SetTextColor(hDC, dwFG) ;                                        /* restore the old foreground color */
         SetBkMode(hDC, oldmode) ;                                        /* reset to old mode */
         DeleteObject(hRgn) ;                                             /* delete the region */
         }

      /* compute field value rectangle for data input; adjust fields with internal offset */
      fields[index]->ValueRect.top = fields[index]->Rect.top + yoff ;       /* top */
      fields[index]->ValueRect.left = fields[index]->Rect.left + xoff ;     /* left */
      fields[index]->ValueRect.bottom = fields[index]->Rect.bottom ;        /* no offset at bottom */
      fields[index]->ValueRect.right = fields[index]->Rect.right ;          /* no offset at right */
      if (fields[index]->bRight == TRUE || fields[index]->bOutline == TRUE)           /* if right line will be drawn */
         fields[index]->ValueRect.right-- ;                                /* reserve space for it (offset = 1) */
      if (fields[index]->bBottom == TRUE || fields[index]->bOutline == TRUE)          /* if bottom line will be drawn */
         fields[index]->ValueRect.bottom-- ;                               /* reserve space for it (offset = 1) */
      if (fields[index]->bLeft == TRUE || fields[index]->bOutline == TRUE)            /* if left line will be drawn */
         fields[index]->ValueRect.left++ ;                                 /* reserve space for it (xoff++) */
      if (!twolines && (fields[index]->bTop == TRUE || fields[index]->bOutline == TRUE))   /* if top line will be drawn and not two lines */
         fields[index]->ValueRect.top++ ;                                  /* reserve space for it (yoff++) */

      /* display the picture if no value is available to display */
      if (fields[index]->Buffer == NULL &&
          fields[index]->Picture[0] && (fields[index]->FieldType == WINEXE_FLD_NUMPIC ||
                              fields[index]->FieldType == WINEXE_FLD_PICTURE))
         {
         hRgn = CreateRectRgn(fields[index]->Rect.left + forms[formindex].xOffset,
                     fields[index]->Rect.top + forms[formindex].yOffset,
                     fields[index]->Rect.right + forms[formindex].xOffset,
                     fields[index]->Rect.bottom + forms[formindex].yOffset) ;        /* create rectangle region */
         SelectClipRgn(hDC, hRgn) ;                                       /* select it as clip region */

         dwFG = SetTextColor(hDC, fields[index]->ValueFg) ;                /* set the foreground color */
         oldmode = SetBkMode(hDC, TRANSPARENT) ;                          /* save the old mode */

#if !defined(__WC386__)
         hFont = CreateFontIndirect(&fonts[fields[index]->ValueFont]) ;    /* load font */
#else
         hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[index]->ValueFont]) ;    /* load font */
#endif
         hOldFont = SelectObject(hDC, hFont) ;                            /* set the new display font */
#if !defined(__WC386__)
         dwSize1 = GetTextExtent(hDC, fields[index]->Picture, lstrlen(fields[index]->Picture)) ;     /* calculate text size */
#else
         dwSize1 = GetTextExtent(hDC, (LPSTR)fields[index]->Picture, lstrlen((LPSTR)fields[index]->Picture)) ;     /* calculate text size */
#endif
         width = 1 + fields[index]->ValueRect.right - fields[index]->ValueRect.left ;                /* adjust width */
         switch(fields[index]->Alignment)                                  /* identify the alignment */
            {
            case WINEXE_FLD_ALGN_RIGHT:                                   /* if aligned to right */
               xoff = (int) (width - LOWORD(dwSize1) - 4) ;               /* correct the internal offset */
               break ;
            case WINEXE_FLD_ALGN_CENTER:                                  /* if centered */
               xoff = (int) ((width - LOWORD(dwSize1)) / 2) ;             /* correct the internal offset */
               break ;
            }
         TextOut(hDC, fields[index]->Rect.left + xoff + forms[formindex].xOffset,
                      fields[index]->Rect.top + yoff + forms[formindex].yOffset,
#if !defined(__WC386__)
                      fields[index]->Picture, lstrlen(fields[index]->Picture)) ;      /* paint the text */
#else
                      (LPSTR)fields[index]->Picture, lstrlen((LPSTR)fields[index]->Picture)) ;      /* paint the text */
#endif

         SelectObject(hDC, hOldFont) ;                                    /* reselect the old font */
         DeleteObject(hFont) ;                                            /* Delete the font */
         SetTextColor(hDC, dwFG) ;                                        /* restore the old foreground color */
         SetBkMode(hDC, oldmode) ;                                        /* reset to old mode */
         DeleteObject(hRgn) ;                                             /* delete the region */
         }

      if (fields[index]->FieldType == WINEXE_FLD_CHECK || fields[index]->FieldType == WINEXE_FLD_RADIO)
         {
#if !defined(__WC386__)
         hFont = CreateFontIndirect(&fonts[fields[index]->ValueFont]) ;
#else
         hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[index]->ValueFont]) ;
#endif
         hOldFont = SelectObject(hDC, hFont) ;      /* set the new display font */
         dwFG = SetTextColor(hDC, fields[index]->ValueFg) ;  /* set the foreground color */
         oldmode = SetBkMode(hDC, TRANSPARENT) ;

         listhigh = abs(fonts[fields[index]->ValueFont].lfHeight) ;
         listcount = listwide = 0 ;
         for (j = 0 ; j < FIELD_LIST_COUNT ; j++)
            {
            if (fieldlists[j].fieldid != fields[index]->id)
               continue ;
            listcount++ ;
            dwSize1 = GetTextExtent(hDC, (LPSTR) fieldlists[j].Label,
                                   lstrlen((LPSTR)fieldlists[j].Label)) ;
            if (LOWORD(dwSize1) > listwide)
               listwide = (int) LOWORD(dwSize1) ;
            }
         listwide += 12 + listhigh ;  /* allow room for checkbox/radio button */
         listvert = fields[index]->Rect.bottom - (fields[index]->Rect.top + yoff) ;
         listvert /= listhigh ;
         listhorz = fields[index]->Rect.right - (fields[index]->Rect.left + xoff) ;
         listhorz /= listwide ;
         if (listhorz < 1)
            listhorz = 1 ;
         if (listvert < 1)
            listvert = 1 ;

         i = 0 ;
         ybutoff = yoff ;
         xbutoff = xoff ;
         for (j = 0 ; j < FIELD_LIST_COUNT ; j++)
            {
            if (fieldlists[j].fieldid != fields[index]->id)
               continue ;
            TextOut(hDC, fields[index]->Rect.left + forms[formindex].xOffset + xbutoff + 6 +
                         abs(fonts[fields[index]->ValueFont].lfHeight),
                      fields[index]->Rect.top + forms[formindex].yOffset + ybutoff,
#if !defined(__WC386__)
                      fieldlists[j].Label, lstrlen(fieldlists[j].Label)) ;
#else
                      (LPSTR)fieldlists[j].Label, lstrlen((LPSTR)fieldlists[j].Label)) ;
#endif
            if (listvert >= listcount || !((i + 1) % listhorz))
               {
               xbutoff = xoff ;
               ybutoff += listhigh ;
               }
            else
               xbutoff += listwide ;
            i++ ;
            }

         SelectObject(hDC, hOldFont) ;  /* reselect the old font */
         DeleteObject(hFont) ;          /* Delete the font */
         SetTextColor(hDC, dwFG) ;      /* restore the old foreground color */
         SetBkMode(hDC, oldmode) ;
         }

      ReleaseDC(hWnd, hDC) ;                                              /* release device context */
      }

   if (fields[index]->Buffer != NULL)                                      /* if buffer isn't empty */
      {
      unlock = 0 ;                                                        /* reset variable */
      if (fields[index]->str == NULL)                                      /* check if string is lost */
         {
         fields[index]->str = GlobalLock(fields[index]->Buffer) ;           /* try to find it again and lock it */
         if (fields[index]->str != NULL)                                   /* if you found something */
            unlock = 1 ;                                                  /* remember it */
         }
      if (fields[index]->str != NULL)                                      /* so there is a string */
         PaintFieldValue(hWnd, index, fields[index]->id) ;                 /* paint it */
      if (unlock)                                                         /* we found something and locked it */
         {
         GlobalUnlock(fields[index]->Buffer) ;                             /* unlock it */
         fields[index]->str = NULL ;                                       /* and reset pointer */
         }
      }
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    PaintFieldValue (hWnd, index, id)

   PURPOSE:     Paint field value
                hWnd:           window handle
                index:          actual index
                id:             menu type

*****************************************************************************/

void PaintFieldValue(HWND hWnd, int index, int id)
   {
   HDC  hDC ;                                                             /* handle to device context */
   HRGN hRgn ;                                                            /* for creating clipping region */
   DWORD   dwFG ;                                                         /* hold old FG colors */
   HFONT   hFont;                                                         /* Used to create the font to display the text */
   HFONT   hOldFont;                                                      /* Used to hold the old font */
   int i, xoff, oldmode, charoff, savechar, formindex, j ;
   long sumcheck ;
   DWORD dwSize1, width ;                                                 /* used for text size */
   LOGBRUSH brush ;
   HBRUSH hBrush ;                                                        /* handles for old & new brushes */

   if (index < 0)                                                         /* if index isn't already set */
      index = FindIndex(FIELD_TYPE, id) ;                                 /* find field type index for menu type */
   formindex = FindIndex(FORM_TYPE, fields[index]->formid) ;               /* set form index for it */

   if (fields[index]->FieldType == WINEXE_FLD_RADIO)                       /* initialize the radio buttons */
      {
      for (i = 0 ; i < FIELD_LIST_COUNT ; i++)
         {
         if (fieldlists[i].fieldid != fields[index]->id)
            continue ;
#if !defined(__WC386__)
         if (!lstrcmpi(fieldlists[i].Label, fields[index]->str))
#else
         if (!lstrcmpi((LPSTR)fieldlists[i].Label, (LPSTR)fields[index]->str))
#endif
            SendMessage(fieldlists[i].hWnd, BM_SETCHECK, 1, 0L) ;
         else
            SendMessage(fieldlists[i].hWnd, BM_SETCHECK, 0, 0L) ;
         InvalidateRect(fieldlists[i].hWnd, NULL, FALSE) ;
         UpdateWindow(fieldlists[i].hWnd) ;
         }
      return ;
      }
   if (fields[index]->FieldType == WINEXE_FLD_CHECK)                       /* initialize the check box buttons */
      {
      sumcheck = atol(fields[index]->str) ;
      j = 0 ;
      for (i = 0 ; i < FIELD_LIST_COUNT ; i++)
         {
         if (fieldlists[i].fieldid != fields[index]->id)
            continue ;
         if ((long) sumcheck & (long) pow(2, j))
            SendMessage(fieldlists[i].hWnd, BM_SETCHECK, 1, 0L) ;
         else
            SendMessage(fieldlists[i].hWnd, BM_SETCHECK, 0, 0L) ;
         InvalidateRect(fieldlists[i].hWnd, NULL, FALSE) ;
         UpdateWindow(fieldlists[i].hWnd) ;
         j++ ;
         }
      return ;
      }
   if (fields[index]->FieldType == WINEXE_FLD_BOOL)                       /* initialize the boolean check box button */
      {
      if (fields[index]->hWnd == NULL)
         return ;
      j = fields[index]->str[0] ;
      if (j == 'Y' || j == 'y' || j == 'T' || j == 't' || j == '1' || j == 1)
         SendMessage(fields[index]->hWnd, BM_SETCHECK, 1, 0L) ;
      else
         SendMessage(fields[index]->hWnd, BM_SETCHECK, 0, 0L) ;
      return ;
      }

   hDC = GetDC(hWnd) ;                                                    /* get window device context           */
   hRgn = CreateRectRgn(fields[index]->ValueRect.left + forms[formindex].xOffset,
               fields[index]->ValueRect.top + forms[formindex].yOffset,
               fields[index]->ValueRect.right + forms[formindex].xOffset,
               fields[index]->ValueRect.bottom + forms[formindex].yOffset) ;         /* create region */
   SelectClipRgn(hDC, hRgn) ;                                             /* select region */

   if (!fields[index]->NoBg)                                               /* if backgroundcolor selected */
      {
      brush.lbStyle = BS_SOLID ;                                          /* set brush style */
      brush.lbColor = fields[index]->ValueBg ;                             /* set brush color */
      brush.lbHatch = NULL ;                                              /* set brush pattern */
      hBrush = CreateBrushIndirect(&brush) ;                              /* create that brush */
#if !defined(__WC386__)
      FillRect(hDC, &fields[index]->ValueRect, hBrush) ;                   /* fill the rectangle */
#else
      FillRect(hDC, (LPRECT)&fields[index]->ValueRect, hBrush) ;                   /* fill the rectangle */
#endif
      DeleteObject(hBrush) ;                                              /* delete the brush */
      }

   /* display the field value */
   dwFG = SetTextColor(hDC, fields[index]->ValueFg) ;                      /* set the foreground color */
   oldmode = SetBkMode(hDC, TRANSPARENT) ;                                /* select it */

#if !defined(__WC386__)
   hFont = CreateFontIndirect(&fonts[fields[index]->ValueFont]) ;          /* load font */
#else
   hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[index]->ValueFont]) ;          /* load font */
#endif
   hOldFont = SelectObject(hDC, hFont) ;                                  /* set the new display font */
   width = 1 + fields[index]->ValueRect.right - fields[index]->ValueRect.left ;       /* get field width */
   dwSize1 = GetTextExtent(hDC, fields[index]->str, lstrlen(fields[index]->str)) ;    /* get string length */
   charoff = 0 ;                                                          /* reset variable (caretposition left) */
   /* while there is something written, and the caret isn't very left (let the left of string vanish, if string to long) */
   while (LOWORD(dwSize1) > width - 10 && forms[formindex].caretpos > charoff)
      {
      charoff++ ;                                                         /* move caret one character to the right */
      dwSize1 = GetTextExtent(hDC, &fields[index]->str[charoff], lstrlen(&fields[index]->str[charoff])) ;   /* calculate stringlength right of caret */
      }
   xoff = 0 ;                                                             /* reset internal offset */
   if (LOWORD(dwSize1) <= width - 10)                                     /* if text has displayable size */
      {
      switch(fields[index]->Alignment)                                     /* identify alignment type */
         {
         case WINEXE_FLD_ALGN_RIGHT:                                      /* if right alignment */
            xoff = (int) (width - LOWORD(dwSize1) - 4) ;                  /* adjust offset */
            break ;
         case WINEXE_FLD_ALGN_CENTER:                                     /* if centered */
            xoff = (int) ((width - LOWORD(dwSize1)) / 2) ;                /* adjust offset */
            break ;
         }
      }
   TextOut(hDC, fields[index]->ValueRect.left + xoff + forms[formindex].xOffset,
                fields[index]->ValueRect.top + forms[formindex].yOffset,
                &fields[index]->str[charoff], lstrlen(&fields[index]->str[charoff])) ;          /* paint text */
   forms[formindex].charoff = charoff ;                                   /* remember character offset for caret */

   /* set caret position */
   if (formindex == currentformindex)
      {
      savechar = fields[index]->str[forms[formindex].caretpos] ;           /* get caret position */
      fields[index]->str[forms[formindex].caretpos] = 0 ;                  /* reset pointer */
      dwSize1 = GetTextExtent(hDC, &fields[index]->str[charoff], lstrlen(&fields[index]->str[charoff])) ;    /* get text size */
      forms[formindex].xcaretpos = fields[index]->ValueRect.left + xoff + LOWORD(dwSize1) ;     /* adjust x position of caret */
      forms[formindex].ycaretpos = fields[index]->ValueRect.top ;          /* adjust y position of caret */
      SetCaretPos(forms[formindex].xcaretpos + forms[formindex].xOffset,
                  forms[formindex].ycaretpos + forms[formindex].yOffset) ;   /* adjust caret to offsets */
      fields[index]->str[forms[formindex].caretpos] = savechar ;           /* remember caret position */
      }

   SelectObject(hDC, hOldFont) ;                                          /* reselect the old font */
   DeleteObject(hFont) ;                                                  /* Delete the font */
   SetTextColor(hDC, dwFG) ;                                              /* restore the old foreground color */
   SetBkMode(hDC, oldmode) ;                                              /* restore old mode */
   DeleteObject(hRgn) ;                                                   /* delete rectangle region */

   ReleaseDC(hWnd, hDC) ;                                                 /* release device context */
   return ;                                                               /* return to calling function */
   }


/*****************************************************************************

   FUNCTION:    PaintTable (hWnd, index, id, rcPaint)

   PURPOSE:     Paint table
                hWnd:           window handle
                index:          set to -1, gets index of tabletype for menu type
                id:             menu type
                rcPaint:        paint structure

*****************************************************************************/

void PaintTable(HWND hWnd, int index, int id, LPRECT rcPaint)
   {
   HDC  hDC ;                                                             /* handle to device context */
   HRGN hRgn ;                                                            /* for creating clipping region */
   HPEN hOldPen, hPen ;                                                   /* handles for old & new pens */
   DWORD   dwFG, dwBG;                                                    /* hold old FG/BG colors */
   HFONT   hFont;                                                         /* Used to create the font to display the text */
   HFONT   hOldFont;                                                      /* Used to hold the old font */
   LOGBRUSH brush, brushlbl ;
   HBRUSH hOldBrush, hBrush, hBrushlbl ;                                  /* handles for old & new brushes */
   int rc, xoff, yoff, i, colwidth, center, fudge, unlock, j, formindex ;
   DWORD dwSize1 ;                                                        /* used for text size */
   RECT intrect, Rect ;                                                   /* used for rectangles */
   LPHANDLE ph ;

   if (index < 0)                                                         /* if index < 0 (initially set to -1) */
      index = FindIndex(TABLE_TYPE, id) ;                                 /* find table type index for menu type */
   formindex = FindIndex(FORM_TYPE, tables[index].formid) ;               /* set formindex for it */

   if (tables[index].Rect.right > forms[formindex].xMaxPix)               /* if tablesize greater than window */
      forms[formindex].xMaxPix = tables[index].Rect.right ;               /* adjust window horizontally */
   if (tables[index].Rect.bottom > forms[formindex].yMaxPix)
      forms[formindex].yMaxPix = tables[index].Rect.bottom ;              /* adjust window vertically */

   Rect.top = tables[index].Rect.top + forms[formindex].yOffset ;         /* adjust rectangle according tablesize and offset */
   Rect.left = tables[index].Rect.left + forms[formindex].xOffset ;
   Rect.bottom = tables[index].Rect.bottom + forms[formindex].yOffset ;
   Rect.right = tables[index].Rect.right + forms[formindex].xOffset ;

   if (!IntersectRect(&intrect, rcPaint, &Rect))                          /* if it's not visible */
      return ;                                                            /* don't bother, return to calling function */

   if (!PaintFieldValueOnly || PaintFieldValueOnly == WINEXE_PAINTALLFIELDS)   /* paint of no special field, or all fields ordered */
      {
      if (rcPaint->top > Rect.top)                                        /* if top of painting area outside view rectangle */
         rcPaint->top = Rect.top ;                                        /* set top of painiting area at top of rectangle */
      if (rcPaint->left > Rect.left)                                      /* if left of painting area outside view rectangle */
         rcPaint->left = Rect.left ;                                      /* set left of painiting area at top of rectangle */
      if (rcPaint->bottom < Rect.bottom)                                  /* if bottom of painting area outside view rectangle */
         rcPaint->bottom = Rect.bottom ;                                  /* set bottom of painiting area at top of rectangle */
      if (rcPaint->right < Rect.right + 10)                               /* if right of painting area outside view rectangle */
         rcPaint->right = Rect.right + 10;                                /* set right of painiting area at top of rectangle */
      if (tables[index].ScrollBar && tables[index].ScrollhWnd != NULL)    /* if there is a scrollbar and it's active */
         InvalidateRect(tables[index].ScrollhWnd, NULL, FALSE) ;          /* delete old rectangle */
      hDC = GetDC(hWnd) ;                                                 /* get window device context */

      if (tables[index].ScrollBar && tables[index].ScrollhWnd != NULL)    /* if there is a scrollbar and it's active */
         {
         i = 0 ;
         while (databases[i].name != NULL)                                /* while you have database names */
            {
#if !defined(__WC386__)
            if (IsSameFile(databases[i].name, tables[index].Filename))    /* check if it's a filename */
#else
            if (IsSameFile((LPSTR)databases[i].name, (LPSTR)tables[index].Filename))    /* check if it's a filename */
#endif
               break ;
#if !defined(__WC386__)
            if (IsSameFile(databases[i].link, tables[index].Filename))    /* or the link is a filename */
#else
            if (IsSameFile((LPSTR)databases[i].link, (LPSTR)tables[index].Filename))    /* or the link is a filename */
#endif
               break ;
            i++ ;                                                         /* count checked databasenames */
            }
         if (databases[i].name != NULL && databases[i].recordcount > 0)   /* if you have a databasename, and it has entries */
            {
            databases[i].recordcount = RecordCount(i) ;                   /* update recordcount */
            if (databases[i].recordcount < 1)                             /* if there is no record */
               databases[i].recordcount = 1 ;                             /* let it have one */
            SetScrollRange(tables[index].ScrollhWnd, SB_CTL, 1, (int) databases[i].recordcount, FALSE) ;  /* adjust scrollbar range */
            if (databases[i].recordnumber > 0)                            /* if there are records */
               SetScrollPos(tables[index].ScrollhWnd, SB_CTL, (int) databases[i].recordnumber, TRUE) ;  /* draw scrollbox on scrollbar */
            }
         }

      yoff = 0 ;                                                          /* reset internal vertical offset */
      if (tables[index].TblLblSize > 0)                                   /* if there are table labels */
         {
         brush.lbStyle = BS_SOLID ;                                       /* set brush style */
         brush.lbColor = tables[index].TblLblBg ;                         /* set brush color */
         brush.lbHatch = NULL ;                                           /* set brush pattern */
         hBrush = CreateBrushIndirect(&brush) ;                           /* create brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* select the brush */
         hPen = CreatePen(PS_SOLID, tables[index].LineWidth, tables[index].BorderColor) ;     /* create pen */
         hOldPen = SelectObject(hDC, hPen) ;                              /* select the pen into the device context */
         /* draw rectangle and reserve space for labels on top */
         Rectangle(hDC, tables[index].Rect.left + forms[formindex].xOffset,
                        tables[index].Rect.top + forms[formindex].yOffset,
                        tables[index].Rect.right + forms[formindex].xOffset,
                        tables[index].Rect.top + tables[index].TblLblSize +
                                        (2 * tables[index].LineWidth) + forms[formindex].yOffset) ;
         SelectObject(hDC, hOldBrush) ;                                   /* re-select old brush */
         DeleteObject(hBrush) ;                                           /* delete newer brush */
         SelectObject(hDC, hOldPen) ;                                     /* re-select the old pen back into the dc */
         DeleteObject(hPen) ;                                             /* delete the newer pen */
         yoff += tables[index].TblLblSize ;                               /* adjust internal vertical offset to table labels */
         }

      xoff = 0 ;                                                          /* reset internal horizontal offset */
      i = 0 ;                                                             /* reset column counter */
      for (rc = 0 ; rc < FIELD_COUNT ; rc++)                              /* for all fields */
         {
         if (fields[rc]->tableid != tables[index].id)                      /* if table fields aren't for the menu type */
            continue ;                                                    /* don't bother, continue search */

         brush.lbStyle = BS_SOLID ;                                       /* set brush style */
         brush.lbColor = fields[rc]->ValueBg ;                             /* set brush color */
         brush.lbHatch = NULL ;                                           /* set brush pattern */
         hBrush = CreateBrushIndirect(&brush) ;                           /* create brush */
         hOldBrush = SelectObject(hDC, hBrush) ;                          /* select brush */

         brushlbl.lbStyle = BS_SOLID ;                                    /* set label brush style */
         brushlbl.lbColor = fields[rc]->LabelBg ;                          /* set label brush color */
         brushlbl.lbHatch = NULL ;                                        /* set label brush pattern */
         hBrushlbl = CreateBrushIndirect(&brushlbl) ;                     /* create label brush */

         hPen = CreatePen(PS_SOLID, tables[index].LineWidth, tables[index].BorderColor) ;     /* create pen */
         hOldPen = SelectObject(hDC, hPen) ;                              /* select the pen into the device context */
         colwidth = tables[index].AvgColSize ;                            /* set column width to average */
         if (fields[rc]->Rect.right > 0)                                   /* if field exists */
            colwidth = fields[rc]->Rect.right ;                            /* set column width to field width */
         fudge = 1 ;
         if (tables[index].ColLblSize > 0)                                /* display column label rectangle */
            {
            SelectObject(hDC, hBrushlbl) ;                                /* select label brush */
            Rectangle(hDC, tables[index].Rect.left + xoff + forms[formindex].xOffset,
                           tables[index].Rect.top + yoff + forms[formindex].yOffset,
                           tables[index].Rect.left + xoff + colwidth + fudge + forms[formindex].xOffset,
                           tables[index].Rect.top + yoff + tables[index].ColLblSize + 1 + forms[formindex].yOffset) ; /* draw rectangle */
            }

         SelectObject(hDC, hBrush) ;                                      /* select brush */
         Rectangle(hDC, tables[index].Rect.left + xoff + forms[formindex].xOffset,
                        tables[index].Rect.top + yoff + tables[index].ColLblSize + forms[formindex].yOffset,
                        tables[index].Rect.left + xoff + colwidth + fudge + forms[formindex].xOffset,
                        tables[index].Rect.bottom + 1 + forms[formindex].yOffset) ;           /* draw rectangle */
         SelectObject(hDC, hOldBrush) ;                                   /* re-select old brush */
         DeleteObject(hBrush) ;                                           /* delete newer brush */
         DeleteObject(hBrushlbl) ;                                        /* delete label brush */
         SelectObject(hDC, hOldPen) ;                                     /* re-select the old pen back into the dc */
         DeleteObject(hPen) ;                                             /* delete the newer pen */
         xoff += colwidth ;                                               /* add column width to horizontal offset */
         i++ ;                                                            /* increase column counter by 1 */
         }

      yoff += tables[index].AvgRowSize + tables[index].ColLblSize ;       /* add to vertical offset the averae row size and column label size */
      hPen = CreatePen(PS_SOLID, tables[index].LineWidth, tables[index].BorderColor) ;        /* create pen */
      hOldPen = SelectObject(hDC, hPen) ;                                 /* select the pen into the device context */
      for (i = 0 ; i < tables[index].Rows - 1 ; i++)                      /* as long as there are more than one rows */
         {
         MoveTo(hDC, tables[index].Rect.left + forms[formindex].xOffset,
                     tables[index].Rect.top + yoff + forms[formindex].yOffset) ;
         LineTo(hDC, tables[index].Rect.right + forms[formindex].xOffset,
                     tables[index].Rect.top + yoff + forms[formindex].yOffset) ;    /* draw a line as lower row border (underline) */
         yoff += tables[index].AvgRowSize ;                               /* go one row down */
         }
      SelectObject(hDC, hOldPen) ;                                        /* re-select the old pen back into the dc */
      DeleteObject(hPen) ;                                                /* delete the newer pen */

      /* display the table label */
      yoff = 0 ;                                                          /* reset internal vertical offset */
      if (tables[index].TblLblSize > 0)                                   /* if there are table labels */
         {
         hRgn = CreateRectRgn(tables[index].Rect.left + forms[formindex].xOffset,
                     tables[index].Rect.top + forms[formindex].yOffset,
                     tables[index].Rect.right + forms[formindex].xOffset,
                     tables[index].Rect.top + tables[index].TblLblSize + forms[formindex].yOffset) ;    /* create rectangle region */
         SelectClipRgn(hDC, hRgn) ;                                       /* select it as clip region */
         dwFG = SetTextColor(hDC, tables[index].TblLblFg) ;               /* set the foreground color */
         dwBG = SetBkColor(hDC, tables[index].TblLblBg) ;                 /* set the background color */

#if !defined(__WC386__)
         hFont = CreateFontIndirect(&fonts[tables[index].TableFont]) ;    /* load font */
#else
         hFont = CreateFontIndirect((LPLOGFONT)&fonts[tables[index].TableFont]) ;    /* load font */
#endif
         hOldFont = SelectObject(hDC, hFont) ;                            /* set the new display font */
#if !defined(__WC386__)
         dwSize1 = GetTextExtent(hDC, tables[index].Tablename, lstrlen(tables[index].Tablename)) ;  /* calculate tablename size */
#else
         dwSize1 = GetTextExtent(hDC, (LPSTR)tables[index].Tablename, lstrlen((LPSTR)tables[index].Tablename)) ;  /* calculate tablename size */
#endif
         center = tables[index].Rect.right - tables[index].Rect.left - LOWORD(dwSize1) ;      /* calculate center */
         center /= 2 ;                                                    /* offset to center */
         if (center < 1)                                                  /* if name beyond left border */
            center = 1 ;                                                  /* start at left border */
         TextOut(hDC, tables[index].Rect.left + tables[index].LineWidth + center + forms[formindex].xOffset,
                      tables[index].Rect.top + tables[index].LineWidth + forms[formindex].yOffset,
#if !defined(__WC386__)
                      tables[index].Tablename, lstrlen(tables[index].Tablename)) ;       /* paint the text */
#else
                      (LPSTR)tables[index].Tablename, lstrlen((LPSTR)tables[index].Tablename)) ;       /* paint the text */
#endif

         SelectObject(hDC, hOldFont) ;                                    /* reselect the old font */
         DeleteObject(hFont) ;                                            /* Delete the font */
         SetTextColor(hDC, dwFG) ;                                        /* restore the old foreground color */
         SetBkColor(hDC, dwBG) ;                                          /* restore the old background color */
         DeleteObject(hRgn) ;                                             /* delete rectangle region */
         yoff += tables[index].TblLblSize + tables[index].LineWidth ;     /* adjust internal vertical offset */
         }

      /* display the Column Headings */
      if (tables[index].ColLblSize > 0)                                   /* if there are column labels */
         {
         dwFG = SetTextColor(hDC, tables[index].ColLblFg) ;               /* set the foreground color */
         dwBG = SetBkColor(hDC, tables[index].ColLblBg) ;                 /* set the background color */

         xoff = 0 ;                                                       /* reset internal horizontal offset */

         for (rc = 0 ; rc < FIELD_COUNT ; rc++)                           /* as long as there are fields */
            {
            if (fields[rc]->tableid != tables[index].id)                   /* if the table fields aren't for this menu type */
               continue ;                                                 /* don't bother, continue search */

            colwidth = tables[index].AvgColSize ;                         /* set column width to average */
            if (fields[rc]->Rect.right > 0)                                /* if there is a field width */
               colwidth = fields[rc]->Rect.right ;                         /* take it as column width */
            if (fields[rc]->Label[0])                                      /* if there is a label */
               {
#if !defined(__WC386__)
               hFont = CreateFontIndirect(&fonts[fields[rc]->LabelFont]) ; /* load font */
#else
               hFont = CreateFontIndirect((LPLOGFONT)&fonts[fields[rc]->LabelFont]) ; /* load font */
#endif
               hOldFont = SelectObject(hDC, hFont) ;                      /* set the new display font */
#if !defined(__WC386__)
               dwSize1 = GetTextExtent(hDC, fields[rc]->Label, lstrlen(fields[rc]->Label)) ;    /* calculate text length */
#else
               dwSize1 = GetTextExtent(hDC, (LPSTR)fields[rc]->Label, lstrlen((LPSTR)fields[rc]->Label)) ;    /* calculate text length */
#endif
               hRgn = CreateRectRgn(tables[index].Rect.left + xoff + forms[formindex].xOffset,
                                    tables[index].Rect.top + tables[index].TblLblSize + forms[formindex].yOffset,
                                    tables[index].Rect.left + xoff + colwidth + forms[formindex].xOffset,
                                    tables[index].Rect.top +
                                    tables[index].TblLblSize + tables[index].ColLblSize + forms[formindex].yOffset) ; /* create a region for label */
               SelectClipRgn(hDC, hRgn) ;                                 /* activate region */
               center = colwidth - LOWORD(dwSize1) ;                      /* center label */
               center /= 2 ;                                              /* offset to center label */
               if (center < 1)                                            /* if label outside left border */
                  center = 1 ;                                            /* start at left border */
               SetTextColor(hDC, fields[rc]->LabelFg) ;                    /* set the foreground color */
               SetBkColor(hDC, fields[rc]->LabelBg) ;                      /* set the background color */
               TextOut(hDC, tables[index].Rect.left + xoff + center + forms[formindex].xOffset,
                      tables[index].Rect.top + yoff + tables[index].LineWidth + forms[formindex].yOffset,
#if !defined(__WC386__)
                      fields[rc]->Label, lstrlen((LPSTR) fields[rc]->Label)) ;        /* paint the text */
#else
                      (LPSTR)fields[rc]->Label, lstrlen((LPSTR) fields[rc]->Label)) ;        /* paint the text */
#endif
               SelectObject(hDC, hOldFont) ;                              /* reselect the old font */
               DeleteObject(hFont) ;                                      /* Delete the font */
               DeleteObject(hRgn) ;                                       /* delete the region */
               }
            xoff += colwidth ;                                            /* add columnwidth to offset (go to next column) */
            }
         SetTextColor(hDC, dwFG) ;                                        /* restore the old foreground color */
         SetBkColor(hDC, dwBG) ;                                          /* restore the old background color */
         }

      /* now paint the column/field values */
      for (i = 0 ; i < FIELD_COUNT ; i++)                                 /* as long as there are fields */
         {
         if (fields[i]->tableid != tables[index].id)                       /* if field isn't for this table */
            continue ;                                                    /* don't bother, continue search */
         if (fields[i]->ColBuffer == NULL)                                 /* if column buffer is empty */
            continue ;                                                    /* don't bother, continue search */
         if (fields[i]->str != NULL)                                       /* if there is a string */
            GlobalUnlock(fields[i]->Buffer) ;                              /* unlock it */
         fields[i]->str = NULL ;                                           /* delete it */
         ph = (LPHANDLE) GlobalLock(fields[i]->ColBuffer) ;                /* lock the column value and get address */
         for (j = 0 ; ph != NULL && j < tables[index].Rows ; j++)         /* if value exists, and row is displayed */
            {
            if (ph[j] == NULL)                                            /* if no column address */
               continue ;                                                 /* don't bother, continue search */
            fields[i]->ValueRect.top = tables[index].Rect.top +
                                      tables[index].TblLblSize + tables[index].ColLblSize +
                                      tables[index].LineWidth +
                                      (j * tables[index].AvgRowSize) ;    /* create field (top) */
            fields[i]->ValueRect.bottom = fields[i]->ValueRect.top + tables[index].AvgRowSize
                                      - (1 + tables[index].LineWidth) ;   /* create field (bottom) */
            fields[i]->Buffer = ph[j] ;                                    /* get the field data position */
            fields[i]->str = GlobalLock(fields[i]->Buffer) ;                /* get the string, lock it */
            if (fields[i]->str != NULL)                                    /* if the string isn't empty */
               {
               PaintFieldValue(hWnd, i, fields[i]->id) ;                   /* display the string */
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock string */
               }
            fields[i]->str = NULL ;                                        /* delete string */
            }
         if (ph != NULL)                                                  /*  if column address still exists */
            GlobalUnlock(fields[i]->ColBuffer) ;                           /* unlock the column buffer */
         }
      ReleaseDC(hWnd, hDC) ;                                              /* release device context */
      }
   else if (PaintFieldValueOnly == WINEXE_PAINTCURFLDONLY)                /* just paint current field */
      {
      for (i = 0 ; i < FIELD_COUNT ; i++)                                 /* as long as there is a field */
         {
         if (fields[i]->tableid != tables[index].id)                       /* if field is not for this table */
            continue ;                                                    /* don't bother, continue search */
         Rect.top = fields[i]->ValueRect.top + forms[formindex].yOffset ;  /* create rectangle for field */
         Rect.left = fields[i]->ValueRect.left + forms[formindex].xOffset ;
         Rect.bottom = fields[i]->ValueRect.bottom + forms[formindex].yOffset ;
         Rect.right = fields[i]->ValueRect.right + forms[formindex].xOffset ;
         if (IntersectRect(&intrect, rcPaint, &Rect) &&
             fields[i]->Buffer != NULL)                                    /* if field is in active part, and has a content */
            {
            unlock = 0 ;                                                  /* reset lock switch */
            if (fields[i]->str == NULL)                                    /* if string is empty */
               {
               fields[i]->str = GlobalLock(fields[i]->Buffer) ;             /* find it and lock it */
               if (fields[i]->str != NULL)                                 /* if you found it */
                  unlock = 1 ;                                            /* remember: it's locked */
               }
            if (fields[i]->str != NULL)                                    /* if there is a string */
               PaintFieldValue(hWnd, i, fields[i]->id) ;                   /* display it */
            if (unlock)                                                   /* if we just locked it */
               {
               GlobalUnlock(fields[i]->Buffer) ;                           /* unlock it */
               fields[i]->str = NULL ;                                     /* and delete it */
               }
            }
         }
      }
   return ;                                                               /* return to calling function */
   }

/*****************************************************************************

   FUNCTIONS:   PaintRectFill (index)      (fill)
                PaintRectBack (index)      (fill invisible)

   PURPOSE:     fill rectangle
                index:  type index

*****************************************************************************/

HBRUSH PaintRectFill(int index)
   {
   if (rects[index].bFill == FALSE)                                       /* if fill flag isn't set */
      return(GetStockObject(NULL_BRUSH)) ;                                /* return to calling function, set to no brush */

   switch(rects[index].wPattern)                                          /* identify pattern */
      {
      case 700:
      case 701:
      case 702:
      case 703:
      case 704:
      case 705:                                                           /* if one of these patterns */
      case 706:
      case 707:
      case 708:
      case 709:
      case 710:
      case 711:
      case 712:
      case 713:
#if !defined(__WC386__)
         return(CreateBrushIndirect(&rects[index].LogBrush)) ;            /* load that brush, and return to calling function */
#else
         return(CreateBrushIndirect((LPLOGBRUSH)&rects[index].LogBrush)) ;            /* load that brush, and return to calling function */
#endif

      default:                                                            /* no pattern found */
         return(GetStockObject(NULL_BRUSH)) ;                             /* return to calling function, no brush set */
      }                                                                   /* switch */
   }

HBRUSH PaintRectBack(int index)
   {
   LOGBRUSH brush ;

   if (rects[index].bTrans)                                               /* if transparent set */
      brush.lbStyle = BS_HOLLOW ;                                         /* set brush style hollow */
   else
      brush.lbStyle = BS_SOLID ;                                          /* set brush style solid */
   brush.lbColor = rects[index].bgColor ;                                 /* set brush color to background color */
   brush.lbHatch = NULL ;                                                 /* set brush pattern (none) */
   return(CreateBrushIndirect(&brush)) ;                                  /* return to calling function with created brush */
   }


/*****************************************************************************

   FUNCTION:    WinExeLoadBitmap (hWnd, inew, *fn, Rect, orgRect, *handle

   PURPOSE:     Load bitmap
                hWnd:     window handle
                inew:     1, if rectangle isn't prepared for bitmap, 0 else
                *fn:      bitmap filename
                Rect:     rectangle coordinates for bitmap
                orgRect:  rectangle coordinates actually used by bitmap
                *handle:  bitmap handle

*****************************************************************************/

BOOL WinExeLoadBitmap(HWND hWnd, int inew, char far *fn, LPRECT Rect, LPRECT orgRect, HBITMAP far *handle)
   {
   char buffer[81] ;
#if !defined(__WC386__)
   int fh ;
#else
   HFILE fh ;
#endif
   DWORD colors ;
   OFSTRUCT fs ;
   HCURSOR hOldCur ;
   BITMAPFILEHEADER bmfh ;
   HANDLE memhand ;
   LPSTR bits ;
   DWORD bmfsize ;
   BITMAPINFO far *pbminfo ;
   BITMAPCOREINFO far *pbmcore ;
   HDC hDC ;

   hOldCur = SetCursor(hCurWait) ;                                        /* 1, if cursor active; 0, if not */

#if !defined(__WC386__)
   fh = OpenFile(fn, &fs, OF_READ) ;                                      /* open file for reading */
#else
   fh = OpenFile((LPSTR)fn, (LPOFSTRUCT)&fs, OF_READ) ;                                      /* open file for reading */
#endif
   if (fh == -1)                                                          /* if opening of file failed */
      {
      lstrcpy(buffer, "Unable to open file: ") ;                          /* write that to buffer */
      lstrcat((LPSTR) buffer, (LPSTR) fn) ;                               /* add the filename to buffer */
      MessageBox(NULL, (LPSTR)buffer, NULL, MB_SYSTEMMODAL | MB_ICONHAND | MB_OK) ; /* display buffer in a message box */
      SetCursor(hOldCur) ;                                                /* re-select old cursor state */
      return(FALSE) ;                                                     /* return with no success message */
      }
#if defined(__C700__)                                                     /* to enable reading binares for different compilers */
   _setmode(fh, O_BINARY) ;                                               /* Microsoft C only */
#else
   setmode(fh, O_BINARY) ;                                                /* other C compilers */
#endif

#if defined(__C700__)                                                     /* and getting the file length */
   bmfsize = _filelength(fh) ;                                            /* Microsoft C only */
#else
   bmfsize = filelength(fh) ;                                             /* other C compilers */
#endif

   /* read in the bitmap file header */
   if (_lread (fh, (LPSTR) &bmfh, sizeof (bmfh)) != sizeof (bmfh))        /* if it hasn't the correct size */
      {
      SetCursor(hOldCur) ;                                                /* re-select old cursor state */
      _lclose(fh) ;                                                       /* close file */
      WinExeLoadBitmapError(hWnd, fn);                                    /* activate error message (loaderror) */
      return(FALSE) ;                                                     /* return to calling function with failure message */
      }

   if (bmfh.bfType != DIB_HEADER_MARKER)                                  /* if bitmap is the wrong type */
      {
      SetCursor(hOldCur) ;                                                /* re-select old cursor state */
      _lclose(fh) ;                                                       /* close file */
      WinExeLoadBitmapInvalid(hWnd, fn);                                  /* activate error message (invalid bitmap) */
      return(FALSE) ;                                                     /* return to calling function with failure message */
      }

   memhand = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, bmfsize - sizeof(BITMAPFILEHEADER));      /* check if bitmap can be loaded (enough memory?) */
   if (memhand == NULL)                                                   /* if not enough memory */
      {
      SetCursor(hOldCur) ;                                                /* re-select old cursor state */
      _lclose(fh) ;                                                       /* close file */
      WinExeLoadBitmapNoMem(hWnd, fn) ;                                   /* activate error message (out of memory) */
      return(FALSE) ;                                                     /* return to calling function with failure message */
      }

   bits = GlobalLock(memhand) ;                                           /* lock the memory for bitmap */
   if (bits == (LPSTR) NULL)                                              /* if memory couldn't be locked */
      {
      SetCursor(hOldCur) ;                                                /* re-select old cursor state */
      _lclose(fh) ;                                                       /* close file */
      GlobalFree(memhand) ;                                               /* free allocated memory (for bitmap) */
      WinExeLoadBitmapNoLock(hWnd, fn) ;                                  /* activate error message (memory lock failure) */
      return(FALSE) ;                                                     /* return to calling function with failure message */
      }

   if (!WinExeLoadBitmapRead(fh, bits, (DWORD) bmfsize - sizeof(BITMAPFILEHEADER))) /* load bitmap, if bitmap size doesn't correspond to size in header */
      {
      SetCursor(hOldCur) ;                                                /* re-select old cursor state */
      _lclose(fh) ;                                                       /* close file */
      GlobalUnlock(memhand);                                              /* unlock bitmap memory */
      GlobalFree(memhand);                                                /* free bitmap memory */
      WinExeLoadBitmapError(hWnd, fn) ;                                   /* activate error message (loaderror) */
      return(FALSE) ;                                                     /* return to calling function with failure message */
      }

   pbminfo = (BITMAPINFO far *) bits ;                                    /* read bitmap header */
   if (pbminfo->bmiHeader.biSize == 0x0C)                                 /* if it's a Windows bitmap */
      {
      pbmcore = (BITMAPCOREINFO far *) bits ;                             /* read the dimension and color information */
      if (inew)                                                           /* if no rectangle prepared */
         {
         Rect->top = 0 ;                                                  /* prepare rectangle */
         Rect->left = 0 ;
         Rect->bottom = (int) pbmcore->bmciHeader.bcHeight ;
         Rect->right = (int) pbmcore->bmciHeader.bcWidth ;
         orgRect->top = 0 ;                                               /* and remember the size */
         orgRect->left = 0 ;
         orgRect->bottom = (int) pbmcore->bmciHeader.bcHeight ;
         orgRect->right = (int) pbmcore->bmciHeader.bcWidth ;
         }
      else
         {
         orgRect->top = 0 ;                                               /* adjust bitmap to rectangle (and remember that) */
         orgRect->left = 0 ;
         orgRect->bottom = orgRect->top + (int) pbmcore->bmciHeader.bcHeight ;
         orgRect->right = orgRect->left + (int) pbmcore->bmciHeader.bcWidth ;
         }

      colors = 0L ;                                                       /* reset color variable (no colors) */
      switch(pbmcore->bmciHeader.bcBitCount)                              /* identify color counter */
         {
         case 1:                                                          /* count is 1 */
            colors = 2L ;                                                 /* monochrome (2 colors: background and foreground) */
            break ;
         case 4:                                                          /* count is 4 */
            colors = 16L ;                                                /* 16 colors */
            break ;
         case 8:                                                          /* count is 8 */
            colors = 256L ;                                               /* 256 colors */
            break ;
         }
      colors = colors * sizeof(RGBTRIPLE) ;                               /* check how many bytes are needed */
      colors += pbmcore->bmciHeader.bcSize ;                              /* and add headersize */
      hDC = GetDC(hWnd) ;                                                 /* get device context */
      *handle = CreateDIBitmap(hDC,                                       /* create bitmap (for display) and assign handle */
                                (LPBITMAPINFOHEADER) &(pbmcore->bmciHeader),
                                (DWORD) CBM_INIT,
                                (LPSTR) bits + (int) colors,
                                (LPBITMAPINFO) pbmcore,
                                (WORD) DIB_RGB_COLORS) ;

      ReleaseDC(hWnd, hDC) ;                                              /* release device context */
      }
   else                                                                   /* it should be an OS/2 */
      {
      if (inew)                                                           /* if rectangle isn't prepared for bitmap */
         {
         Rect->top = 0 ;                                                  /* prepare it */
         Rect->left = 0 ;
         Rect->bottom = (int) pbminfo->bmiHeader.biHeight ;
         Rect->right = (int) pbminfo->bmiHeader.biWidth ;
         orgRect->top = 0 ;                                               /* remember the size */
         orgRect->left = 0 ;
         orgRect->bottom = (int) pbminfo->bmiHeader.biHeight ;
         orgRect->right = (int) pbminfo->bmiHeader.biWidth ;
         }
      else
         {
         orgRect->top = 0 ;                                               /* adjust to rectangle, remember new size */
         orgRect->left = 0 ;
         orgRect->bottom = orgRect->top + (int) pbminfo->bmiHeader.biHeight ;
         orgRect->right = orgRect->left + (int) pbminfo->bmiHeader.biWidth ;
         }

      colors = pbminfo->bmiHeader.biClrUsed ;                             /* read color info (number of used colors) */
      if (colors == 0L)                                                   /* if no color info found */
         switch(pbminfo->bmiHeader.biBitCount)                            /* identify color counter */
            {
            case 1:                                                       /* color count is 1 */
               colors = 2L ;                                              /* monochrome (2 colors: background and foreground) */
               break ;
            case 4:                                                       /* color count is 4 */
               colors = 16L ;                                             /* 16 colors */
               break ;
            case 8:                                                       /* color count is 8 */
               colors = 256L ;                                            /* 256 colors */
               break ;
            }
      colors = colors * sizeof(RGBQUAD) ;                                 /* check how many bytes are needed */
      colors += pbminfo->bmiHeader.biSize ;                               /* and add header size */

      hDC = GetDC(hWnd) ;                                                 /* get device context */
      *handle = CreateDIBitmap(hDC,
                                (LPBITMAPINFOHEADER) &(pbminfo->bmiHeader),
                                (DWORD) CBM_INIT,
                                (LPSTR) bits + (int) colors,
                                (LPBITMAPINFO) pbminfo,
                                (WORD) DIB_RGB_COLORS) ;                  /* create bitmap for display and assign handle */

      ReleaseDC(hWnd, hDC) ;                                              /* release device context */
      }

   GlobalUnlock(memhand) ;                                                /* unlock the memory (loaded bitmap) */
   GlobalFree(memhand) ;                                                  /* free the unlocked memory */

   return(TRUE) ;                                                         /* return to calling function with success message */
   }


/*****************************************************************************

   FUNCTION:    WinExeLoadBitmapRead (fh, buffer, count)

   PURPOSE:     read the bitmap into memory
                fh:     file handle
                buffer: locked memory for bitmap
                count:  sizecount

*****************************************************************************/

BOOL WinExeLoadBitmapRead(int fh, LPSTR buffer, DWORD count)
   {
#if !defined(__ZTC__)
   char huge *ptr ;
#else
   char HUGE *ptr ;
#endif
   int i ;

#if !defined(__ZTC__)
   ptr = (char huge *)buffer ;                                            /* pointer to unlocked memory */
#else
   ptr = (char HUGE *)buffer ;
#endif

   while (count)                                                          /* while there is something to load */
      {
      i = (int) (count > (DWORD) 32767 ? 32767 : LOWORD (count)) ;        /* read in 32k chunks */
      if (_lread (fh, ptr, i) != (WORD) i)                                /* if not copied correctly into memory (wrong size) */
         return(FALSE) ;                                                  /* return to calling function with failure message */
      count -= i ;                                                        /* reduce sizecount by successfully copied amount */
      ptr += i ;                                                          /* set memory pointer to end of successfully copied part */
      }
   return(TRUE) ;                                                         /* return to calling function with success message */
   }


/*****************************************************************************

   FUNCTIONS:   WinExeLoadBitmapError   (hWnd, *fn)
                WinExeLoadBitmapInvalid (hWnd, *fn)
                WinExeLoadBitmapNoMem   (hWnd, *fn)
                WinExeLoadBitmapNoLock  (hWnd, *fn)

   PURPOSE:     manage bitmap load errors
                hWnd:   window handle
                *fn:    filename

*****************************************************************************/

void WinExeLoadBitmapError(HWND hWnd, char far *fn)                       /* wrong bitmap */
   {
   char buffer[81] ;
   wsprintf((LPSTR) buffer, (LPSTR) "%s:  Error reading from file", (LPSTR) fn) ;                  /* prepare error message */
   MessageBox(hWnd, buffer, NULL, MB_ICONEXCLAMATION | MB_OK) ;           /* display message in message box */
   }
void WinExeLoadBitmapInvalid(HWND hWnd, char far *fn)                     /* bitmap invalid */
   {
   char buffer[81] ;
   wsprintf((LPSTR) buffer, (LPSTR) "%s:  Invalid or Unknown bitmap file format.", (LPSTR) fn) ;   /* prepare error message */
   MessageBox(hWnd, buffer, NULL, MB_ICONEXCLAMATION | MB_OK) ;           /* display message in message box */
   }
void WinExeLoadBitmapNoMem(HWND hWnd, char far *fn)                       /* out of memory while loading bitmap */
   {
   char buffer[81] ;
   wsprintf((LPSTR) buffer, (LPSTR) "%s:  Not enough memory to load bitmap file.", (LPSTR) fn) ;   /* prepare error message */
   MessageBox(hWnd, buffer, NULL, MB_ICONEXCLAMATION | MB_OK) ;           /* display message in message box */
   }
void WinExeLoadBitmapNoLock(HWND hWnd, char far *fn)                      /* bitmap memory not locked */
   {
   char buffer[81] ;
   wsprintf((LPSTR) buffer, (LPSTR) "%s:  Unable to lock memory for bitmap file.", (LPSTR) fn) ;   /* prepare error message */
   MessageBox(hWnd, buffer, NULL, MB_ICONEXCLAMATION | MB_OK) ;           /* display message in message box */
   }




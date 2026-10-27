#include "order.h"


//-----------------------------------------------------------------------------
// WINDIB.C
//
// This is a collection of useful DIB manipulation/information gathering
// functions.  Many functions are supplied simply to take the burden
// of taking into account whether a DIB is a Win30 style or OS/2 style
// DIB away from the application.
//
// The functions in this module assume that the DIB pointers or handles
// passed to them point to a block of memory in one of two formats:
//
//       a) BITMAPINFOHEADER + color table + DIB bits (3.0 style DIB)
//       b) BITMAPCOREHEADER + color table + DIB bits (OS/2 PM style)
//
// The SDK Reference, Volume 2 describes these data structures.
//
// The functions in this module could be streamlined (made faster and
// smaller) by removing the OS/2 DIB specific code, and assuming all
// DIBs passed to it are Win30 style DIBs.  The DIB file reading code
// would need to be modified to always convert DIBs to Win30 style
// DIBs.  The only reason this isn't done in DIBView is because DIBView
// was written to test display and printer drivers (which are supposed
// to support OS/2 DIBs wherever they support Win30 style DIBs).  SHOWDIB
// is a great example of how to go about doing this.
//-----------------------------------------------------------------------------

/* define error messages */
static char far * far szErrors[] = {"Not a DIB file!",
                           "Couldn't allocate memory!",
                           "Error reading file!",
                           "Error locking memory!",
                           "Error opening file!",
                           "Error creating palette!",
                           "Error getting a DC!",
                           "Error creating MDI Child!",
                           "Error creating Device Dependent Bitmap",
                           "StretchBlt() failed!",
                           "StretchDIBits() failed!",
                           "Paint requires both DDB and DIB!",
                           "SetDIBitsToDevice() failed!",
                           "Printer: StartDoc failed!",
                           "Printing: GetModuleHandle() couldn't find GDI!",
                           "Printer: SetAbortProc failed!",
                           "Printer: StartPage failed!",
                           "Printer: NEWFRAME failed!",
                           "Printer: EndPage failed!",
                           "Printer: EndDoc failed!",
                           "Only one DIB can be animated at a time!",
                           "No timers available for animation!",
                           "Can't copy to clipboard -- no current DIB!",
                           "Clipboard is busy -- operation aborted!",
                           "Can't paste -- no DIBs or DDBs in clipboard!",
                           "SetDIBits() failed!",
                           "File Not Found!",
                           "Error writing DIB file!"
                          };

void DIBError (int ErrNo)
{
   if ((ErrNo < ERR_MIN) || (ErrNo >= ERR_MAX))                           /* if errornumber not in defined range */
   {
      MessageBox (GetFocus (), "Undefined Error!", NULL, MB_OK | MB_ICONHAND);      /* print it in a message box */
   }
   else                                                                   /* it's defined */
   {
#if !defined(__WC386__)
      MessageBox (GetFocus (), szErrors[ErrNo], NULL, MB_OK | MB_ICONHAND);         /* print error message in messagebox */
#else
      MessageBox (GetFocus (), (LPSTR)szErrors[ErrNo], NULL, MB_OK | MB_ICONHAND);         /* print error message in messagebox */
#endif
   }
}

//---------------------------------------------------------------------
//
// Function:   FindDIBBits
//
// Purpose:    Given a pointer to a DIB, returns a pointer to the
//             DIB's bitmap bits.
//
// Parms:      lpbi == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

LPSTR FindDIBBits (LPSTR lpbi)
   {
   int offset ;
   offset = (int) *(LPDWORD)lpbi + PaletteSize(lpbi) ;                    /* calculate length of header */
   return (lpbi + offset) ;                                               /* set pointer behind header */
   }


//---------------------------------------------------------------------
//
// Function:   DIBNumColors
//
// Purpose:    Given a pointer to a DIB, returns a number of colors in
//             the DIB's color table.
//
// Parms:      lpbi == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

WORD DIBNumColors (LPSTR lpbi)
{
   WORD wBitCount;

      // If this is a Windows style DIB, the number of colors in the
      //  color table can be less than the number of bits per pixel
      //  allows for (i.e. lpbi->biClrUsed can be set to some value).
      //  If this is the case, return the appropriate value.
   if (IS_WIN30_DIB (lpbi))
      {
      DWORD dwClrUsed;

      dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;                 /* read from header number of colors used */

      if (dwClrUsed)                                                      /* if found */
         return (WORD) dwClrUsed;                                         /* return with it to calling function */
      }

      // Calculate the number of colors in the color table based on
      //  the number of bits per pixel for the DIB.
   if (IS_WIN30_DIB (lpbi))                                               /* Windows bitmap */
      wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;                /* read bitcount */
   else                                                                   /* must be an OS/2 bitmap */
      wBitCount = ((LPBITMAPCOREHEADER) lpbi)->bcBitCount;                /* read bitcount */

   switch (wBitCount)                                                     /* identify bitcount */
      {
      case 1:                                                             /* bitcount 1 */
         return 2;                                                        /* return to calling function: monochrome (2 colors: background and foreground) */

      case 4:                                                             /* bitcount 4 */
         return 16;                                                       /* return to calling function: 16 colors */

      case 8:                                                             /* bitcount 8 */
         return 256;                                                      /* return to calling function: 256 colors */

      default:                                                            /* bitcount not found, not identified */
         return 0;                                                        /* return to calling function with failure message */
      }
}


//---------------------------------------------------------------------
//
// Function:   PaletteSize
//
// Purpose:    Given a pointer to a DIB, returns number of bytes
//             in the DIB's color table.
//
// Parms:      lpbi == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

WORD PaletteSize (LPSTR lpbi)
{
   if (IS_WIN30_DIB (lpbi))                                               /* Windows */
      return (DIBNumColors (lpbi) * sizeof (RGBQUAD));
   else                                                                   /* OS/2 */
      return (DIBNumColors (lpbi) * sizeof (RGBTRIPLE));
}


//---------------------------------------------------------------------
//
// Function:   CreateDIBPalette
//
// Purpose:    Given a handle to a DIB, constructs a logical palette,
//             and returns a handle to this palette.
//
//             Stolen almost verbatim from ShowDIB.
//
// Parms:      hDIB == HANDLE to global memory with a DIB header
//                     (either BITMAPINFOHEADER or BITMAPCOREHEADER)
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

HPALETTE CreateDIBPalette (HANDLE hDIB)
{
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i, wNumColors;
   LPSTR            lpbi;
   LPBITMAPINFO     lpbmi;
   LPBITMAPCOREINFO lpbmc;
   BOOL             bWinStyleDIB;

   if (!hDIB)
      return NULL;

   lpbi         = GlobalLock (hDIB);                                      /* lock the memory */
   lpbmi        = (LPBITMAPINFO) lpbi;                                    /* Windows info */
   lpbmc        = (LPBITMAPCOREINFO) lpbi;                                /* OS/2 info */
   wNumColors   = DIBNumColors (lpbi);                                    /* number of colors */
   bWinStyleDIB = IS_WIN30_DIB (lpbi);                                    /* is it Windows style? */

   if (wNumColors)                                                        /* if there are colors */
      {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * wNumColors);         /* allocate palette */

      if (!hLogPal)                                                       /* if palette couldn't be allocated */
         {
                 DIBError(ERR_CREATEPAL);                                 /* give an errormessage */
         GlobalUnlock (hDIB);                                             /* unlock memory */
         return NULL;                                                     /* return to calling function with failure code */
         }

      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);                        /* lock the palette memory */

      lpPal->palVersion    = PALVERSION;                                  /* remember pal version */
      lpPal->palNumEntries = wNumColors;                                  /* remember number of colors */

      for (i = 0;  i < wNumColors;  i++)                                  /* for all colors */
         {
         if (bWinStyleDIB)                                                /* if it's Windows style */
            {
            lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;   /* remember colors */
            lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
            }
         else                                                             /* it's OS/2 style */
            {
            lpPal->palPalEntry[i].peRed   = lpbmc->bmciColors[i].rgbtRed;      /* remember colors */
            lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
            lpPal->palPalEntry[i].peBlue  = lpbmc->bmciColors[i].rgbtBlue;
            lpPal->palPalEntry[i].peFlags = 0;
            }
         }

      hPal = CreatePalette (lpPal);                                       /* create palette */

      if (!hPal)                                                          /* if palette not created */
         DIBError (ERR_CREATEPAL);                                        /* give error message */

      GlobalUnlock (hLogPal);                                             /* unlock palette memory */
      GlobalFree   (hLogPal);                                             /* free palette memory */
   }

   GlobalUnlock (hDIB);                                                   /* unlock bitmap memory */

   return hPal;                                                           /* return to calling function with palette handle */
}


//---------------------------------------------------------------------
//
// Function:   DIBHeight
//
// Purpose:    Given a pointer to a DIB, returns its height.  Note
//             that it returns a DWORD (since a Win30 DIB can have
//             a DWORD in its height field), but under Win30, the
//             high order word isn't used!
//
// Parms:      lpDIB == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

DWORD DIBHeight (LPSTR lpDIB)
{
   LPBITMAPINFOHEADER lpbmi;
   LPBITMAPCOREHEADER lpbmc;
                                                                          /* remember pointer to DIB header */
   lpbmi = (LPBITMAPINFOHEADER) lpDIB;                                    /* Windows style */
   lpbmc = (LPBITMAPCOREHEADER) lpDIB;                                    /* OS/2 style */

   if (lpbmi->biSize == sizeof (BITMAPINFOHEADER))                        /* if size from header (Windows style) readable */
      return lpbmi->biHeight;                                             /* return to calling function with height */
   else                                                                   /* it's OS/2 */
      return (DWORD) lpbmc->bcHeight;                                     /* return to calling functio with height */
}


//---------------------------------------------------------------------
//
// Function:   DIBWidth
//
// Purpose:    Given a pointer to a DIB, returns its width.  Note
//             that it returns a DWORD (since a Win30 DIB can have
//             a DWORD in its width field), but under Win30, the
//             high order word isn't used!
//
// Parms:      lpDIB == pointer to DIB header (either BITMAPINFOHEADER
//                       or BITMAPCOREHEADER)
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

DWORD DIBWidth (LPSTR lpDIB)
{
   LPBITMAPINFOHEADER lpbmi;
   LPBITMAPCOREHEADER lpbmc;
                                                                          /* remember pointer to DIB header */
   lpbmi = (LPBITMAPINFOHEADER) lpDIB;                                    /* Windows style */
   lpbmc = (LPBITMAPCOREHEADER) lpDIB;                                    /* OS/2 style */

   if (lpbmi->biSize == sizeof (BITMAPINFOHEADER))                        /* if DIB size conforms with header size (Windows style) */
      return lpbmi->biWidth;                                              /* return to calling function with width */
   else                                                                   /* it's OS/2 */
      return (DWORD) lpbmc->bcWidth;                                      /* return to calling function with width */
}


//---------------------------------------------------------------------
//
// Function:   DIBToBitmap
//
// Purpose:    Given a handle to global memory with a DIB spec in it,
//             and a palette, returns a device dependent bitmap.  The
//             The DDB will be rendered with the specified palette.
//
// Parms:      hDIB == HANDLE to global memory containing a DIB spec
//                     (either BITMAPINFOHEADER or BITMAPCOREHEADER)
//             hPal == Palette to render the DDB with.  If it's NULL,
//                     use the default palette.
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

HBITMAP DIBToBitmap (HANDLE hDIB, HPALETTE hPal)
{
   LPSTR    lpDIBHdr, lpDIBBits;
   HBITMAP  hBitmap;
   HDC      hDC;
   HPALETTE hOldPal = NULL;

   if (!hDIB)                                                             /* if no DIB handle */
      return NULL;                                                        /* return to calling function with failure code */

   lpDIBHdr  = GlobalLock (hDIB);                                         /* lock the DIB memory */
   lpDIBBits = FindDIBBits (lpDIBHdr);                                    /* get the DIBBits */
   hDC       = GetDC (NULL);                                              /* get the device context */

   if (!hDC)                                                              /* if no device context */
      {
      GlobalUnlock (hDIB);                                                /* unlock the memory */
      return NULL;                                                        /* return to calling function with failure code */
      }

   if (hPal)                                                              /* if palette exists */
      hOldPal = SelectPalette (hDC, hPal, FALSE);                         /* select it */

   RealizePalette (hDC);                                                  /* map logical palette entries to system palette */

   hBitmap = CreateDIBitmap (hDC,
                             (LPBITMAPINFOHEADER) lpDIBHdr,
                             CBM_INIT,
                             lpDIBBits,
                             (LPBITMAPINFO) lpDIBHdr,
                             DIB_RGB_COLORS);                             /* create the bitmap for display */

   if (!hBitmap)                                                          /* if creation failed */
      DIBError (ERR_CREATEDDB);                                           /* give error message */

   if (hOldPal)                                                           /* if old palette existed */
      SelectPalette (hDC, hOldPal, FALSE);                                /* re-select it */

   ReleaseDC (NULL, hDC);                                                 /* release device context */
   GlobalUnlock (hDIB);                                                   /* unlock memory */

   return hBitmap;                                                        /* return to calling function (with new bitmap */
}


//---------------------------------------------------------------------
//
// Function:   InitBitmapInfoHeader
//
// Purpose:    Does a "standard" initialization of a BITMAPINFOHEADER,
//             given the Width, Height, and Bits per Pixel for the
//             DIB.
//
//             By standard, I mean that all the relevant fields are set
//             to the specified values.  biSizeImage is computed, the
//             biCompression field is set to "no compression," and all
//             other fields are 0.
//
//             Note that DIBs only allow BitsPixel values of 1, 4, 8, or
//             24.  This routine makes sure that one of these values is
//             used (whichever is most appropriate for the specified
//             nBPP).
//
// Parms:      lpBmInfoHdr == Far pointer to a BITMAPINFOHEADER structure
//                            to be filled in.
//             dwWidth     == Width of DIB (not in Win 3.0 & 3.1, high
//                            word MUST be 0).
//             dwHeight    == Height of DIB (not in Win 3.0 & 3.1, high
//                            word MUST be 0).
//             nBPP        == Bits per Pixel for the DIB.
//
// History:   Date      Reason
//            11/07/91  Created
//
//---------------------------------------------------------------------

void InitBitmapInfoHeader (LPBITMAPINFOHEADER lpBmInfoHdr,
                                        DWORD dwWidth,
                                        DWORD dwHeight,
                                          int nBPP)
{
#if !defined(__ZTC__)
   _fmemset (lpBmInfoHdr, 0, sizeof (BITMAPINFOHEADER));                  /* initialize a header */
#else
   memset (lpBmInfoHdr, 0, sizeof(BITMAPINFOHEADER));
#endif

   lpBmInfoHdr->biSize      = sizeof (BITMAPINFOHEADER);                  /* remember header size */
   lpBmInfoHdr->biWidth     = dwWidth;                                    /* remember bitmap width */
   lpBmInfoHdr->biHeight    = dwHeight;                                   /* remember bitmap height */
   lpBmInfoHdr->biPlanes    = 1;                                          /* remember: it's plane */

   if (nBPP <= 1)                                                         /* if less than two bit per pixel   */
      nBPP = 1;                                                           /* set it to one (monochrome)       */
   else if (nBPP <= 4)                                                    /* if less than five bits per pixel */
      nBPP = 4;                                                           /* set it to four (16 colors)       */
   else if (nBPP <= 8)                                                    /* if less than nine bits per pixel */
      nBPP = 8;                                                           /* set it to eight (256 colors)     */
   else                                                                   /* more than eight bits per pixel   */
      nBPP = 24;                                                          /* set it to 24                     */

   lpBmInfoHdr->biBitCount  = nBPP;                                       /* remember the bit per pixel code */
   lpBmInfoHdr->biSizeImage = WIDTHBYTES (dwWidth * nBPP) * dwHeight;     /* calculate and remember bitmap size */
}


//---------------------------------------------------------------------
//
// Function:   BitmapToDIB
//
// Purpose:    Given a device dependent bitmap and a palette, returns
//             a handle to global memory with a DIB spec in it.  The
//             DIB is rendered using the colors of the palette passed in.
//
// Parms:      hBitmap == Handle to device dependent bitmap compatible
//                        with default screen display device.
//             hPal    == Palette to render the DDB with.  If it's NULL,
//                        use the default palette.
//
// History:   Date      Reason
//             6/01/91  Created
//
//---------------------------------------------------------------------

HANDLE BitmapToDIB (HBITMAP hBitmap, HPALETTE hPal)
{
   BITMAP             Bitmap;
   BITMAPINFOHEADER   bmInfoHdr;
   LPBITMAPINFOHEADER lpbmInfoHdr;
   LPSTR              lpBits;
   HDC                hMemDC;
   HANDLE             hDIB;
   HPALETTE           hOldPal = NULL;

      // Do some setup -- make sure the Bitmap passed in is valid,
      //  get info on the bitmap (like its height, width, etc.),
      //  then setup a BITMAPINFOHEADER.
   if (!hBitmap)                                                          /* if there's no bitmap handle */
      return NULL;                                                        /* return to calling function with failure code */

   if (!GetObject (hBitmap, sizeof (Bitmap), (LPSTR) &Bitmap))            /* if we can't get the object */
      return NULL;                                                        /* return to calling function with failure code */

   InitBitmapInfoHeader (&bmInfoHdr,
                         Bitmap.bmWidth,
                         Bitmap.bmHeight,
                         Bitmap.bmPlanes * Bitmap.bmBitsPixel);           /* initialize bitmap header */


      // Now allocate memory for the DIB.  Then, set the BITMAPINFOHEADER
      //  into this memory, and find out where the bitmap bits go.
   hDIB = GlobalAlloc (GHND, sizeof (BITMAPINFOHEADER) +
             PaletteSize ((LPSTR) &bmInfoHdr) + bmInfoHdr.biSizeImage);

   if (!hDIB)                                                             /* if allocation failed */
      return NULL;                                                        /* return to calling function with failure code */

   lpbmInfoHdr  = (LPBITMAPINFOHEADER) GlobalLock (hDIB);                 /* lock the memory */
   *lpbmInfoHdr = bmInfoHdr;                                              /* remember header info */
   lpBits       = FindDIBBits ((LPSTR) lpbmInfoHdr);                      /* remember bit info */

      // Now, we need a DC to hold our bitmap.  If the app passed us
      //  a palette, it should be selected into the DC.
   hMemDC       = GetDC (NULL);

   if (hPal)                                                              /* if palette exits */
      {
      hOldPal = SelectPalette (hMemDC, hPal, FALSE);                      /* remember it */
      RealizePalette (hMemDC);                                            /* map logical palette entries to system palette */
      }

      // We're finally ready to get the DIB.  Call the driver and let
      //  it party on our bitmap.  It will fill in the color table,
      //  and bitmap bits of our global memory block.
   if (!GetDIBits (hMemDC,
                   hBitmap,
                   0,
                   Bitmap.bmHeight,
                   lpBits,
                   (LPBITMAPINFO) lpbmInfoHdr,
                   DIB_RGB_COLORS))                                       /* if we can't read the bitmap */
      {
      GlobalUnlock (hDIB);                                                /* unlock the memory */
      GlobalFree (hDIB);                                                  /* free it */
      hDIB = NULL;                                                        /* delete handle */
      }
   else
      GlobalUnlock (hDIB);                                                /* unlock memory */

      // Finally, clean up and return.
   if (hOldPal)                                                           /* if there was an old palette */
      SelectPalette (hMemDC, hOldPal, FALSE);                             /* re-select it */

   ReleaseDC (NULL, hMemDC);                                              /* release device context */

   return hDIB;                                                           /* return to calling function with DIB handle */
}

//----------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------


HANDLE BitmapToPmDIB (HBITMAP hBitmap, HPALETTE hPal)
{
   BITMAP               Bitmap;
   BITMAPCOREINFO       bmci;
   BITMAPCOREINFO FAR   *lpbi;
   BITMAPCOREHEADER             bmch;
   HDC                                  hDC;
   HANDLE                               hDIB;
   DWORD                                SizeImage, dwLen;
   int                                  nBPP;

      // Do some setup -- make sure the Bitmap passed in is valid,
      //  get info on the bitmap (like its height, width, etc.),
      //  then setup a BITMAPINFOHEADER.
   if (!hBitmap)                                                          /* if there's no bitmap */
      return NULL;                                                        /* return to calling function with failure code */

   if (!GetObject (hBitmap, sizeof (Bitmap), (LPSTR) &Bitmap))            /* if we can't get the bitmap */
      return NULL;                                                        /* return to calling function with failure code */

#if !defined(__ZTC__)
   _fmemset ((LPBITMAPCOREHEADER)&bmch, 0, sizeof(BITMAPCOREHEADER));     /* create bitmap header */
#else
   memset((LPBITMAPCOREHEADER)&bmch, 0, sizeof(BITMAPCOREHEADER));
#endif

   bmch.bcSize      = sizeof (BITMAPINFOHEADER);                          /* remember header size */
   bmch.bcWidth     = Bitmap.bmWidth;                                     /* remember bitmap width */
   bmch.bcHeight    = Bitmap.bmHeight;                                    /* remember bitmap height */
   bmch.bcPlanes    = 1;                                                  /* remember: it's plane */

   nBPP = Bitmap.bmPlanes * Bitmap.bmBitsPixel;                           /* get bit per pixel count */

   if (nBPP <= 1)                                                         /* if less than two bit per pixel   */
      nBPP = 1;                                                           /* set it to one (monochrome)       */
   else if (nBPP <= 4)                                                    /* if less than five bits per pixel */
      nBPP = 4;                                                           /* set it to four (16 colors)       */
   else if (nBPP <= 8)                                                    /* if less than nine bits per pixel */
      nBPP = 8;                                                           /* set it to eight (256 colors)     */
   else                                                                   /* more than eight bits per pixel   */
      nBPP = 24;                                                          /* set it to 24                     */

   bmch.bcBitCount  = nBPP;                                               /* remember bit per pixel count */

      // Now allocate memory for the DIB.  Then, set the BITMAPINFOHEADER
      //  into this memory, and find out where the bitmap bits go.
   SizeImage = WIDTHBYTES((DWORD)Bitmap.bmWidth * nBPP) * Bitmap.bmHeight;     /* calculate bitmap size */
   dwLen = bmch.bcSize + PaletteSize((LPSTR)&bmci) + SizeImage;           /* calculate total size (header + palette + image) */

   hDIB = GlobalAlloc (GHND, dwLen);                                      /* allocate memory for bitmap */

   if (!hDIB)                                                             /* if unable to allocate memory */
      return NULL;                                                        /* return to calling function with failure code */

   if (hPal == NULL)                                                      /* if no palette defined */
           hPal = GetStockObject(DEFAULT_PALETTE);                        /* set to default palette */

   hDC = GetDC(NULL);                                                     /* get device context */

   hPal = SelectPalette(hDC, hPal, FALSE);                                /* select the palette */
   RealizePalette(hDC);                                                   /* map logical palette entries to system palette */

   lpbi = (BITMAPCOREINFO FAR *)GlobalLock(hDIB);                         /* lock memory for header */
   *lpbi = bmci;                                                          /* remember core info */

   if (GetDIBits(hDC, hBitmap, 0, (WORD)bmci.bmciHeader.bcHeight, (LPSTR)lpbi + (WORD)bmci.bmciHeader.bcSize + PaletteSize((LPSTR)&bmci),
          (LPBITMAPINFO)(LPBITMAPCOREINFO)lpbi, DIB_RGB_COLORS) == 0)     /* if there's no header data */
          {
          GlobalUnlock(hDIB);                                             /* unlock bitmap memory */
          hDIB = NULL;                                                    /* reset bitmap handle */
          SelectPalette(hDC, hPal, FALSE);                                /* re-select old palette */
          ReleaseDC(hDC, NULL);                                           /* release device context */
          return NULL;                                                    /* return to calling function with failure code */
          }

   bmci = *lpbi;                                                          /* reset core info */
   GlobalUnlock(hDIB);                                                    /* unlock bitmap memory */

   SelectPalette(hDC, hPal, FALSE);                                       /* re-select old palette */
   ReleaseDC(hDC, NULL);                                                  /* release device context */
   return hDIB;                                                           /* return to calling function with DIB handle */

}


/*--------------  DIB header Marker Define -------------------------*/

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')


/*--------------  MyRead Function Define ---------------------------*/

// When we read in a DIB, we read it in in chunks.  We read half a segment
//  at a time.  This way we insure that we don't cross any segment
//  boundries in _lread() during a read.  We don't read in a full segment
//  at a time, since _lread takes some "int" type parms instead of
//  WORD type params (it'd work, but the compiler would give you warnings)...

#define BYTES_PER_READ  32767

/*--------------  Define for PM DIB  -------------------------------*/
// The constants for RGB, RLE4, RLE8 are already defined inside
// of Windows.h

/*-------------- Magic numbers -------------------------------------*/
// Maximum length of a filename for DOS is 128 characters.

#define MAX_FILENAME 129


/*--------------  TypeDef Structures -------------------------------*/

typedef struct InfoStruct
  {
  char  szName[13];
  char  szType[15];
  DWORD cbWidth;
  DWORD cbHeight;
  DWORD cbColors;
  char  szCompress[5];
  }  INFOSTRUCT;

typedef WORD (CALLBACK* FARHOOK)(HWND,UINT,WPARAM,LPARAM);
/*--------------  Global Variables ---------------------------------*/

extern char szFileName[256];        // Filename of DIB (valid iff (hDIB))
char   szDirName[256];              // Directory name
WORD   wDibType;                    // Type of Dib
DWORD  biStyle;                     // Type of Dib - RGB, RLE4, RLE8, PM
WORD   biBits;                      // bits per pixel

/*--------------  Local Function Prototypes ------------------------*/

BOOL     SaveDIBFile       (LPSTR, HBITMAP, WORD);
HANDLE   WinDibFromBitmap  (HBITMAP, DWORD, WORD, HPALETTE);
HANDLE   PMDibFromBitmap   (HBITMAP, DWORD, WORD, HPALETTE);
BOOL     WriteDIB          (LPSTR, HANDLE);
DWORD PASCAL lwrite        (int, VOID FAR *, DWORD);

/*--------------  Exported Function Prototypes ---------------------*/

BOOL FAR PASCAL FileOpenHookProc (HWND, WORD, WORD, LONG);
BOOL FAR PASCAL FileSaveHookProc (HWND, WORD, WORD, LONG);
BOOL FAR PASCAL InfoDlgProc      (HWND, WORD, WORD, LONG);


/****************************************************************************

 FUNCTION   : WriteDIB(LPSTR szFile,HANDLE hdib)

 PURPOSE    : Write a global handle in CF_DIB format to a file.

 RETURNS    : TRUE  - if successful.
              FALSE - otherwise

 ****************************************************************************/
BOOL WriteDIB (LPSTR szFile, HANDLE hdib)
{
    BITMAPFILEHEADER    hdr;
    LPBITMAPINFOHEADER  lpbi;
    int                 fh;
    OFSTRUCT            of;

    if (!hdib)                                                            /* if there's no bitmap */
    return FALSE;                                                         /* return to calling function with failure code */

    fh = OpenFile (szFile, &of, OF_CREATE|OF_READWRITE);                  /* open file to read and write */
    if (fh == -1)                                                         /* if file opening failed */
    return FALSE;                                                         /* return to calling function with failure code */

#if !defined(__WC386__)
    lpbi = (BITMAPINFOHEADER FAR *)GlobalLock (hdib);                     /* lock the bitmap memory */
#else
    lpbi = (LPBITMAPINFOHEADER)GlobalLock (hdib);                     /* lock the bitmap memory */
#endif

    /* Fill in the fields of the file header */
    hdr.bfType      = DIB_HEADER_MARKER;                                  /* type of bitmap */
    hdr.bfSize      = GlobalSize (hdib) + sizeof (BITMAPFILEHEADER);      /* bitmap size */
    hdr.bfReserved1     = 0;
    hdr.bfReserved2     = 0;
    hdr.bfOffBits       = (DWORD)sizeof(BITMAPFILEHEADER) + lpbi->biSize +
                          PaletteSize((LPSTR)lpbi);                       /* calculate offset bits (pointer to begin of bitmap data) */

    /* Write the file header */
    if (!_lwrite (fh, (LPSTR)&hdr, sizeof (BITMAPFILEHEADER)))            /* if write failure */
      {
      GlobalUnlock (hdib);                                                /* unlock bitmap memory */
      _lclose (fh);                                                       /* close file */
      return FALSE;                                                       /* return to calling function with failure code */
      }

    /* Write the DIB header and the bits */
    if (!lwrite (fh, (LPSTR)lpbi, GlobalSize (hdib)))                     /* if write failure */
      {
      GlobalUnlock (hdib);                                                /* unlock bitmap memory */
      _lclose (fh);                                                       /* close file */
      return FALSE;                                                       /* return to calling function with failure code */
      }

    GlobalUnlock (hdib);                                                  /* unlock bitmap memory */
    _lclose (fh);                                                         /* close file */
    return TRUE;                                                          /* return to calling function with success code */
}

/****************************************************************************

 FUNCTION   : lwrite(int fh, VOID FAR *pv, DWORD ul)

 PURPOSE    : Writes data in steps of 32k till all the data is written.

 RETURNS    : 0 - If write did not proceed correctly.
         number of bytes written otherwise.

 ****************************************************************************/
DWORD PASCAL lwrite (int fh, VOID FAR *pv, DWORD ul)
{
   DWORD     ulT = ul;                                                    /* remember lenth of data in bytes */
#if !defined(__ZTC__)
   BYTE huge *hp = (BYTE huge *)pv;                                       /* put a pointer to beginning of data */
#else
   BYTE HUGE *hp = (BYTE HUGE *)pv;
#endif

   while (ul > BYTES_PER_READ)                                            /* as long as there is data (full 32k-steps) */
      {
      if (_lwrite(fh, (LPSTR)hp, (WORD)BYTES_PER_READ) != BYTES_PER_READ) /* write it to file, if written bytes not equal to read bytes */
           return 0;                                                      /* return to calling function with failure code */

      ul -= BYTES_PER_READ;                                               /* decrement bytes to read by bytes read */
      hp += BYTES_PER_READ;                                               /* set pointer behind already read data */
      }

   if (_lwrite(fh, (LPSTR)hp, (WORD)ul) != (WORD)ul)                      /* write the (less than 32k) rest, if size of written bytes not equal to read bytes */
      return 0;                                                           /* return to calling function with failure code */

   return ulT;                                                            /* return to calling function with number of read bytes */
}


/****************************************************************************

 FUNCTION   : PMDibFromBitmap()

 PURPOSE    : Will create a global memory block in DIB format that
                 represents the Device-dependent bitmap (DDB) passed in.

                      biStyle -> DIB format     ==  PM
                      biBits  -> Bits per pixel ==  1,4,8,24

 RETURNS    : A handle to the DIB

 ****************************************************************************/

#pragma argsused                                                          /* don't mind to warn about unused arguments */
HANDLE PMDibFromBitmap (HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal)
{
    BITMAP               bm;
    BITMAPCOREINFO     bi;
    BITMAPCOREINFO FAR *lpbi;
    DWORD                dwLen;
    HANDLE               hdib;
    HDC                  hdc;
    DWORD                SizeImage;

    if (!hbm)                                                             /* if no bitmap exists */
        return NULL;                                                      /* return to calling function with failure code */

    if (hpal == NULL)                                                     /* if no palette defined */
        hpal = GetStockObject(DEFAULT_PALETTE);                           /* load default palette */

    GetObject(hbm,sizeof(bm),(LPSTR)&bm);                                 /* get bitmap definition */

    if (biBits == 0)                                                      /* if no bits per pixel defined */
        biBits =  bm.bmPlanes * bm.bmBitsPixel;                           /* set it to plane level (1) times color bits per pixel */

    bi.bmciHeader.bcSize               = sizeof(BITMAPCOREHEADER);        /* remember header size */
    bi.bmciHeader.bcWidth              = bm.bmWidth;                      /* remember bitmap width */
    bi.bmciHeader.bcHeight             = bm.bmHeight;                     /* remember bitmap height */
    bi.bmciHeader.bcPlanes             = 1;                               /* remember:it's plane */
    bi.bmciHeader.bcBitCount           = biBits;                          /* remember bits per pixel */

    SizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;     /* calculate size of bitmap */

    dwLen  = bi.bmciHeader.bcSize + PaletteSize((LPSTR)&bi) + SizeImage;  /* calculate file length (header + palette + image) */

    hdc = GetDC(NULL);                                                    /* get device context */
    hpal = SelectPalette(hdc,hpal,FALSE);                                 /* load palette */
         RealizePalette(hdc);                                             /* activate palette */

    hdib = GlobalAlloc(GHND,dwLen);                                       /* allocate memory for bitmap */

    if (!hdib)                                                            /* if memory allocation not successful */
      {
        SelectPalette(hdc,hpal,FALSE);                                    /* re-select old palette */
        ReleaseDC(NULL,hdc);                                              /* release device context */
        return NULL;                                                      /* return to calling function with failure code */
      }

    lpbi = (BITMAPCOREINFO FAR *)GlobalLock(hdib);                        /* lock bitmap memory */

    *lpbi = bi;                                                           /* place the bitmapcoreinfo into memory */

    if (GetDIBits( hdc,
                   hbm,
                   0,
                   (WORD)bi.bmciHeader.bcHeight,
                   (LPSTR)lpbi + (WORD)bi.bmciHeader.bcSize + PaletteSize((LPSTR)&bi),
                   (LPBITMAPINFO)(LPBITMAPCOREINFO) lpbi, DIB_RGB_COLORS) == 0)     /* load the bitmap, if that failes */
         {
         GlobalUnlock(hdib);                                              /* unlock the memory */
         hdib = NULL;                                                     /* empty it */
         SelectPalette(hdc,hpal,FALSE);                                   /* re-select old palette */
         ReleaseDC(NULL,hdc);                                             /* release device context */
         return NULL;                                                     /* return to calling function with failure code */
         }

    bi = *lpbi;                                                           /* refresh bitmapcoreinfo */
    GlobalUnlock(hdib);                                                   /* unlock bitmap memory */

    SelectPalette(hdc,hpal,FALSE);                                        /* re-select old palette */
    ReleaseDC(NULL,hdc);                                                  /* release device context */
    return hdib;                                                          /* return to calling function with bitmap handle */
}


/****************************************************************************

   FUNCTION:    CopyScreenToBitmap

****************************************************************************/

HBITMAP CopyScreenToBitmap(LPRECT lpRect)
{
  HDC hScrDC, hMemDC;           // screen DC and memory DC
  HBITMAP hBitmap, hOldBitmap;  // handles to deice-dependent bitmaps
  int nX, nY, nX2, nY2;         // coordinates of rectangle to grab
  int nWidth, nHeight;          // DIB width and height
  int xScrn, yScrn;             // screen resolution

  if (IsRectEmpty(lpRect))                                                /* if rectangle is empty */
    return NULL;                                                          /* return to calling function with failure code */

  hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);                         /* create a device context for screen */
  hMemDC = CreateCompatibleDC(hScrDC);                                    /* create a memory DC compatible to that */

  nX = lpRect->left;                                                      /* get points of rectangle to grab */
  nY = lpRect->top;
  nX2 = lpRect->right;
  nY2 = lpRect->bottom;

  xScrn = GetDeviceCaps(hScrDC, HORZRES);                                 /* get screen resolution */
  yScrn = GetDeviceCaps(hScrDC, VERTRES);

  if (nX < 0)                                                             /* make sure bitmap rectangle is visible */
    nX = 0;                                                               /*   and only visible parts shall be saved */
  if (nY < 0)
    nY = 0;
  if (nX2 > xScrn)
    nX2 = xScrn;
  if (nY2 > yScrn)
    nY2 = yScrn;
  nWidth = nX2 - nX;                                                      /* adjust width to screen */
  nHeight = nY2 - nY;                                                     /* adjust height to screen */

  hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);              /* create a bitmap compatible with the screen DC */
  hOldBitmap = SelectObject(hMemDC, hBitmap);                             /* select new bitmap into memory DC */
  BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY) ;        /* bitblt screen DC to memory DC */
  hBitmap = SelectObject(hMemDC, hOldBitmap);                             /* re-select old bitmap */

  /* clean up */
  DeleteDC(hScrDC);                                                       /* delete device context for screen */
  DeleteDC(hMemDC);                                                       /* delete device context for memory */

  return hBitmap;                                                         /* return to calling function with bitmap handle */
}



/****************************************************************************

   FUNCTION:    CopyWindowToBitmap

****************************************************************************/

HBITMAP CopyWindowToBitmap(HWND hWnd, LPRECT lpRect)
{
  HDC hScrDC, hMemDC;           // screen DC and memory DC
  HBITMAP hBitmap, hOldBitmap;  // handles to deice-dependent bitmaps
  int nX, nY, nX2, nY2;         // coordinates of rectangle to grab
  int nWidth, nHeight;          // DIB width and height

  if (IsRectEmpty(lpRect))                                                /* if rectangle is empty */
    return NULL;                                                          /* return to calling function with failure code */

  hScrDC = GetDC(hWnd) ;                                                  /* get window device context */
  hMemDC = CreateCompatibleDC(hScrDC);                                    /* create compatible memory DC to window DC */

  /* get points of rectangle to grab */
  nX = lpRect->left;                                                      /* get points of rectangle to grab */
  nY = lpRect->top;                                                       /*  only of the visible parts! */
  nX2 = lpRect->right;
  nY2 = lpRect->bottom;
  nWidth = nX2 - nX ;                                                     /* check width */
  nHeight = nY2 - nY ;                                                    /* check height */

  hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);              /* create a bitmap compatible with screen DC */
  hOldBitmap = SelectObject(hMemDC, hBitmap);                             /* select new bitmap into memory DC */
  BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY) ;        /* bitblt window DC into memory DC */
  hBitmap = SelectObject(hMemDC, hOldBitmap);                             /* re-select old bitmap */

  /* clean up */
  ReleaseDC(hWnd, hScrDC);                                                /* release device contexts */
  DeleteDC(hMemDC);                                                       /* delete memory DC */

  return hBitmap;                                                         /* return to calling function with bitmap handle */
}


/****************************************************************************

   FUNCTION:    CopyHandle

****************************************************************************/

HANDLE CopyHandle (HANDLE h)
   {
#if !defined(__ZTC__)
   BYTE huge *lpCopy;
   BYTE huge *lp;
#else
   BYTE HUGE *lpCopy;
   BYTE HUGE *lp;
#endif
   HANDLE     hCopy;
   DWORD      dwLen;

   if (!h)                                                                /* if there's no handle */
      return NULL;                                                        /* return to calling function with failure code */

   dwLen = GlobalSize (h);                                                /* get size of object of handle */

   hCopy = GlobalAlloc (GHND, dwLen) ;                                    /* allocate memory for object */
   if (hCopy)                                                             /* if memory allocation successful */
      {
#if !defined(__ZTC__)
      lpCopy = (BYTE huge *)GlobalLock (hCopy);                           /* lock that memory */
      lp     = (BYTE huge *)GlobalLock (h);                               /* lock memory of original */
#else
      lpCopy = (BYTE HUGE *)GlobalLock(hCopy);
      lp     = (BYTE HUGE *)GlobalLock(h);
#endif

      while (dwLen--)                                                     /* as long as there is something to copy */
         *lpCopy++ = *lp++;                                               /* copy element by element */

      GlobalUnlock (hCopy);                                               /* unlock copy memory */
      GlobalUnlock (h);                                                   /* unlock memory of original */
      }

   return hCopy;                                                          /* return to calling function with handle to copy */
   }




Adobe Type Manager (R) version 2.6    Release Notes
Windows version
June 10, 1993

Adobe Type Manager is a trademark of Adobe Systems
Incorporated registered in certain countries. Copyrights
1983-1993 Adobe Systems Incorporated. All Rights Reserved.
Patents Pending

This document supplements the Adobe Type Manager User
Guide. Topics include:

1.   Disk Contents
2.   Installation Requirements
3.   What's New in version 2.6
4.   ATM.INI
5.   ATM Control Panel
6.   Adding and Removing Fonts
7.   PostScript Fonts Listed in the ATM Control Panel
8.   PostScript Fonts and the Microsoft PostScript driver
9.   Soft Font Printing and the Print ATM Fonts as
     Graphics option
10.  Use Pre-built or Resident Fonts Option
11.  Install as Autodownload Fonts for the PostScript
     driver
12.  Using Third-party and Shareware fonts
13.  ATM and Printing
14.  Printer and Video Drivers
15.  Installing Fonts from the Adobe PlusPack and Adobe
     PostScript and Type Cartridges
16.  Installing ATM in the IBM OS/2 Windows Compatibility
     Box
17.  Application Notes
      - Adobe Illustrator 4.0
      - CorelDraw!
      - Designer 3.1 and Charisma 2.1
      - FaceLift
      - FastFonts
      - Harvard Draw 1.0
      - Harvard Graphics 1.0 for Windows
      - Micrografx PostScript Driver
      - Program Manager Replacements (Norton Desktop for
        Windows and PCTools Desktop)
      - Quattro Pro for Windows 1.0
      - TypeAlign 2.0
      - Various Applications with Draft Mode Printing
      - Various Font Conversion Utilities
      - Virus Protection Software
      - Word for Windows 2.0
      - WordPerfect for Windows 5.1 and 5.2
19.  Novell Netware Considerations
20.  ATM and IBM 4029 Series Printers
21.  Troubleshooting
22.  Acknowledgments
A.   ATM.INI parameters

1.   Disk Contents

The following files are found on your ATM disk(s):

ATM16.ND_            Compressed version of ATM16.DLL, the
                     program file for Windows Standard mode
ATM32.ND_            Compressed version of ATM32.DLL, the
                     program file for Windows 386 Enhanced
                     mode
ATMSYS.DR_           Compressed version of ATMSYS.DRV, the
                     ATM System driver
ATMCNTRL.EX_         Compressed version of ATMCNTRL.EXE,
                     the ATM Control Panel
INSTALL.EXE          ATM Installer
INSTALL.CNF          ATM installation configuration file                     
PROGDISK (FONTDISK)  Disk ID file
README.TXT           This README file
ATM.CNF              Configuration file (might not be
                     present on upgrade disks)

The compressed files can be manually expanded using the
Windows utility EXPAND.EXE. Refer to your Windows manual for
details on expanding files.

Your disk(s) might also contain a PSFONTS directory and a
PCLFONTS directory. The PSFONTS directory contains
PostScript Font Outline (PFB) files and Printer Font Metrics
(PFM) files. The PCLFONTS directory contains PCL bitmapped
font files, which have filename extensions of either SFP or
SFL. The PCLFONTS directory also contains Printer Font
Metrics (PFM) files.

If you received ATM with Adobe Acrobat Exchange, Reader, or
Distiller, there are also two MultipleMaster Substitution
Outline (PFB) files as well as their respective
MultipleMaster Font Metrics (MMM) files. These files are
located in the PSFONTS directory. MultipleMaster fonts are
required for font substitution in the Adobe Acrobat
products.

2.   Installation Requirements

To install ATM, you need a c:\ drive and sufficient disk
space for the ATM software and fonts. The amount of disk
space you need depends on the number of fonts included with
your ATM package. The standard retail ATM package requires
about 1.4 megabytes of free disk space. The default
directory for the PostScript fonts is c:\psfonts. You can,
however, install the font files in any directory on any
drive.

3.   What's New in version 2.6

New program functions where implemented in ATM version 2.6
to support the Adobe Acrobat Exchange, Reader, and Distiller
products.

4.   ATM.INI

ATM's initialization file, ATM.INI, is created during
installation. It contains a list of fonts installed in ATM
and other ATM program-related settings. See Appendix A at
the end of this README file for more technical information.

5.   ATM Control Panel

There is no longer a pre-defined limit to the number of
fonts you can install with the ATM Control Panel. Windows
does, however, limit initialization files (including the
ATM.INI and the WIN.INI) to a size of 64K. If this limit is
reached, Windows cannot open, read, or write properly to
these INI files.

For Windows 3.0 and 3.0a, Microsoft recommends keeping
initialization files smaller than 32k in size. Following
this recommendation, the maximum number of fonts that can be
listed in ATM.INI for Windows 3.0 or 3.0a is around 450
fonts. For Windows 3.1, the only limit is the 64K maximum
file size, which is large enough for about 900 fonts.
Many applications limit the number of fonts that can be
displayed in their font menus. For example, an application
font menu might show only 200 fonts even though you have
installed more than 200 ATM fonts.

The more fonts you add with ATM, the longer it takes Windows
to start. The exact time required for Windows to start
depends on the type of processor you have and your system
configuration. To reduce the time it takes Windows to start,
make more memory available to Windows by, for example,
removing Terminate and Stay Resident (TSR) programs. You can
also reduce the time it takes Windows to start by removing
ATM fonts you use infrequently.

6.   Adding and Removing Fonts

With ATM you can add and remove fonts without restarting
Windows. When you use ATM to add and remove fonts while
other applications are running, however, the ATM font
changes are not always automatically available in the
application font menus. Some applications, such as Windows
Write and Lotus 1-2-3 for Windows, update their available
font lists immediately, while others, such as Lotus AmiPro
3.0 and Aldus PageMaker 4.0, require that you re-select your
printer before the ATM font changes appear in font menus. A
few applications, such as Adobe Illustrator 4.0 and
QuattroPro for Windows 1.0, build their font menus when they
are started. You must restart these applications to see ATM
font changes in their font menus.

7.   PostScript Fonts Listed in the ATM Control Panel

Every PostScript font contains a specific Windows menu name,
which may differ from the true PostScript font name. For
example, when you see the font Bookman in the font menu of a
typical Windows application, you are actually using the
PostScript Bookman Light. Windows font menu names are
arranged in family groups of four styles (regular, bold,
bolditalic and italic). Each member of a family group is
listed in the ATM Control Panel individually by its family
name and its style link. For example, Helvetica with its
four different styles appears as Helvetica, Helvetica,BOLD,
Helvetica,BOLDITALIC, and Helvetica,ITALIC, although in a
typical Windows application you only see the one base style
Helvetica. The other styles are accessed by selecting the
bold and italic style attributes.

Some fonts have more than four styles to a family. For
example, Adobe Garamond has six different styles. In
conforming to the Windows standard of grouping fonts into
families of four, the font is broken up into two base font
groups, AGaramond and AGaramond Bold. The following table
illustrates how these different styles are used within
Windows applications:

Windows menu name with style link  True PostScript font name
AGaramond                          AGaramond
AGaramond,BOLD                     AGaramond Semibold
AGaramond,BOLDITALIC               AGaramond SemiboldItalic
AGaramond,ITALIC                   AGaramond Italic
AGaramond Bold                     AGaramond Bold
AGaramond Bold,ITALIC              AGaramond BoldItalic

The readme document on every Adobe font package lists how
the actual fonts are linked to their respective Windows font
menu names.

Some display or decorative fonts have only one style and
were designed with a specific style and weight in mind.
Although ATM provides you with the ability to synthesize
bold and italic styles when only one style is present, you
should refrain from doing so. Synthesizing styles may lead
to unexpected results when printing to high-resolutions
devices.

8.   PostScript Fonts and the Microsoft PostScript driver

For Windows 3.1, there is no longer a 150 font limit on the
number of soft font entries you can have for a PostScript
printer. For Windows 3.0 and 3.0a, however, the maximum
number of soft font entries per PostScript printer WIN.INI
entry is approximately 150. The exact number depends on your
particular system configuration. If you receive spurious
printer-related error messages when starting Windows or
switching to a PostScript printer, you might have too many
soft font entries in the printer's WIN.INI entry. To correct
the problem, try using the ATM Control Panel to remove soft
fonts you don't normally use.

If you manually remove soft font entries from the PostScript
printer section of the WIN.INI file, remember to adjust the
line "softfonts=nn" to show the new total number of soft
font entries. The ATM Control Panel automatically renumbers
soft font entries in WIN.INI when you remove fonts. ATM
cannot, however, correct misnumbered font entries.

The Microsoft Windows 3.1 PostScript driver lets you use a
synthesized italic style for a PostScript font even if the
outline font (PFB) file for the italic font is not present
on this system. Windows, however, cannot provide the
PostScript driver with the correct font metrics for a
synthesized italic face, so the font is displayed with
expanded character spacing. Also, PostScript printers print
the Roman (non-italic) face for the synthesized italic face
when you print.

9.   Soft Font Printing and the Print ATM Fonts as Graphics
     option

For Windows 3.1 and later, ATM takes advantage of a new
feature available with some printer drivers that lets ATM
use the driver to create soft fonts on-the-fly. Creating
soft fonts on-the-fly speeds up printing. Printer drivers
that create soft fonts on-the-fly include the drivers for
the HP LaserJet family of printers and the driver for the
IBM Lexmark printers.

This feature does not work with all printers that support
downloaded printer soft fonts. The printer's Windows 3.1
printer driver must also support on-the-fly soft font
creation for this feature to work. To determine whether a
Windows 3.1 printer driver supports on-the-fly soft font
creation, check the Printer Setup Options dialog box in the
Windows Printer Control Panel. If you do not see a "Print
TrueType as Graphics" option, the driver most likely cannot
create printer soft fonts on-the-fly.

If you set up your printer driver to print TrueType fonts as
graphics, ATM prints fonts as graphics regardless of how the
"Print ATM Fonts as Graphics" option is set.

The "Print ATM Fonts as Graphics" option controls whether or
not ATM uses the soft font creation feature when it is
available. By default, the "Print ATM Fonts as Graphics"
option is cleared and ATM uses the soft font creation
feature. When you select the "Print ATM Fonts as Graphics"
option, however, ATM prints characters as graphics.

Some applications, such as CorelDraw! 3.0, always print text
as graphics.

Selecting the "Print ATM Fonts as Graphics" option can solve
certain printing problems, such as when printing special
shadow or zoomed text effects or graphic objects placed on
top of text strings.

When you change the "Print ATM Fonts as Graphics" option
while other applications are running, the change usually
takes effect immediately. In a few applications, such as
Word for Windows 2.0 for example, you must re-select your
printer before the ATM print mode change takes effect.

10.   Use Pre-built or Resident Fonts

When you select the "Use Pre-built or Resident Fonts"
option, ATM lets PCL printer drivers use pre-built or
printer-resident fonts to print documents. When you clear
the Use Pre-built or Resident Fonts check box, ATM creates
characters for PCL printer drivers to print even when pre-
built or printer-resident fonts are available.

Although ATM does not require that you restart Windows after
you change the "Use Pre-built or Resident Fonts" option, you
might have to quit and restart applications for this option
to take effect.

If you are using Windows 3.1 with printer drivers that allow
soft fonts to be created on-the-fly (such as the PCL
drivers), you might want to turn this option off to speed up
printing. It is more efficient for a printer driver to
create a soft font on-the-fly than to download a pre-built
font. You can see if you have any pre-built fonts installed
for your PCL printer by checking the printer driver's Fonts
dialog box. To display the driver's Fonts dialog box, choose
Fonts in the driver's Printer Setup dialog box.

When the "Use Pre-built or Resident Fonts" option is
selected, you might find the characters of a printer-
resident or soft font that you place beneath a graphic image
print on top of the image. To see an example of this
problem, open an application that lets you draw images over
text, select the Courier font, type some text, and draw a
gray box on top of the text. The font is covered by the gray
box on the screen. Now print with the "Use Pre-built or
Resident Fonts" option selected to see if your printer
prints the text on top of the gray box. If the text prints
on top of the box, you will also have this problem when you
turn ATM off. To correct this problem, clear the Use Pre-
built or Resident Fonts check box.

11.   Install As Autodownload Fonts for the PostScript
      Driver

The Microsoft PostScript printer driver uses the
[PostScript, portname] section in the WIN.INI file to find
the PostScript soft fonts installed on the system. As an
example, the following [PostScript, portname] section shows
two soft fonts installed for a PostScript printer connected
to the parallel port LPT1.

[PostScript, LPT1]
softfonts=2
softfont1=c:\psfonts\pfm\tkrg______.pfm;c:\psfonts\tkrg____.pfb
softfont2=c:\psfonts\pfm\tkb_______.pfm;c:\psfonts\tkb_____.pfb

Each of these soft font entries is made up of two parts:
* The first part is the path and file name of the soft
  font's printer font metrics (PFM) file
* The second part is the path and file name of the soft
  font's outline (PFB) file

The PFM file part is necessary for all PostScript fonts
whose character metrics are not hard-coded in the PostScript
driver. The PFB file part is needed only when you want the
driver to download the font outline with every print job
that uses this font.

To set up the Microsoft PostScript driver to automatically
download a font at print time, install the font with the
"Install as autodownload fonts for the PostScript driver"
option selected. When this option is selected, ATM adds both
the PFM and PFB parts of the soft font entry.

You must also select the "Install as autodownload fonts for
the PostScript driver" option if you are installing the
standard Base35 fonts for a printer that does not contain
them. The Base35 fonts (available with the Adobe PlusPack
and the PostScript Type Cartridge) are: AvantGarde, Bookman,
Courier, Helvetica, HelveticaNarrow, NewCenturySchoolbook,
Palatino, Symbol, Times, ZapfChancery, and ZapfDingbats.
Most, but not all, newer PostScript printers contain the
Base35 fonts.

If you are adding fonts to ATM that are already resident in
the printer, you can uncheck this option to instruct the
printer driver not to send the font outline files to the
printer at print time. With the "Install as autodownload
fonts for the PostScript driver" option unchecked, ATM adds
only the PFM part of the soft font entries in WIN.INI. You
should also uncheck this option for fonts you plan to
manually download to the printer.

How ATM 2.6 Adds Base35 Fonts

When the "Install as autodownload fonts for the PostScript
driver" option is unchecked, ATM does not add soft font
entries to WIN.INI for the Base35 soft fonts. The soft font
entries for these fonts are not needed because the font
metrics for these fonts are hard-coded in the driver. If
soft font entries for the Base35 fonts are present in
WIN.INI, however, they are ignored by the PostScript driver
at print time if the Base35 fonts are resident in the
printer. In other words, the PostScript driver does not
download the outlines of these fonts if they are already
present on the printer. The only exception to this rule is
for the Bookman font. If a Bookman PFB entry is present in
the WIN.INI, the driver downloads the outline file at print
time. To correct this problem and to force the driver to use
the Bookman font resident in the printer, re-add the four
faces of Bookman with the "Install as autodownload fonts for
the PostScript driver" option unchecked

12.  Using Third-party and Shareware fonts

ATM has been tested with a large number of third-party and
shareware fonts. There are, however, certain limitations in
rasterizing fonts that have been converted using third-party
font converters. If you experience any problems with the
fonts you have, please contact Adobe Customer Support at
(408) 986-6520, FAX (408) 562-6767. Although we cannot solve
all fonts problems, we will do our best to solve any
incompatibilities you are experiencing.

13.  ATM and Printing

ATM uses PostScript outline fonts to produce a smooth
graphic display of your fonts on-screen. The fonts available
for use depend on the printer you have selected in your
application.

The fonts available for non-PostScript printers are the
printer resident fonts, the Windows internal bitmapped
fonts, the TrueType fonts installed on the system (for
Windows 3.1), and the PostScript fonts listed in the ATM
Control Panel.

For PostScript printers, the fonts available are the printer
resident fonts, the TrueType fonts installed on the system
(for Windows 3.1), and the PostScript soft font entries
listed in the printer's [PostScript, portname] section in
the WIN.INI file. When you print to a PostScript printer,
ATM does not have to be active because it does not have to
rasterize any fonts. Windows and the Microsoft PostScript
driver are responsible for downloading outline fonts to your
PostScript printer.

Print Resolution

For the highest print quality, Adobe recommends you set up
your printer to print at the highest resolution available.
Certain PCL printer drivers do not properly set the print
resolution. They might appear to be set at 300 dpi
resolution in the Windows Printer Control Panel even though
they are set to a lower resolution. Some applications fail
to print correctly when your PCL printer resolution is not
set to 300 dpi. Excel 3.0a, for example, prints blank cells
instead of ATM fonts when printing ATM fonts as graphics and
your PCL printer is not set to 300 dpi. If you suspect your
PCL printer driver is not set to 300 dpi, set the resolution
to 75 dpi, save this option, and then set the resolution
back to 300 dpi. When your printer resolution is set to 300
dpi, the line "prtresfac=0" appears in the PCL printer entry
in the WIN.INI file.

Colored Text

ATM generates a graphic image of colored text regardless of
the ATM print mode (soft font or graphics) set with the
"Print ATM Fonts as Graphics" option. To force ATM to
generate black or white soft fonts for monochrome printers
and color soft fonts for printers such as the HP PaintJet
300xl, add the following [Colors] section to your ATM.INI
file.

[Colors]
PrintColorGraphics=Off

See Appendix A for a description of this setting. Always
make a backup copy of the ATM.INI before making changes. If
you make a mistake, you can use the backup file to restore
ATM to its previous configuration.

14.  Printer and Video Drivers

For the highest font rasterization quality, Adobe recommends
you use the latest Windows printer and video drivers. If you
are using the Microsoft Windows 3.1 DeskJet driver version
1.2, make sure to use the Universal Printer driver dated
06/29/92. Otherwise, it is suggested to upgrade to the
Universal Printer driver version 3.1.2 when printing to HP
PCL4 and PCL5 printers and compatibles.

Adobe recommends using the LaserJet IV driver version
31.V1.18 or later. If you experiencing printing problems
when using this driver, try setting the Graphics options in
the Windows Control Panel Printer Setup to Raster mode or
reducing the print resolution from 600 dpi to 300 dpi.

15.  Installing Fonts from the Adobe PlusPack and Adobe
     PostScript and Type Cartridges

Installing fonts from the Adobe PlusPack or for the Adobe
PostScript or Adobe Type Cartridge requires that you insert
the different font disks many times. To avoid having to
repeatedly insert font disks, follow these steps:

1. Create a temporary directory on your hard disk.
2. Copy all the files from all the font disks to the
   temporary directory.
3. Start the ATM Control Panel and choose Add. The Add ATM
   Fonts dialog box appears.
4. Using the Directories list box, open the temporary
   directory. The names of all the fonts appear in the
   Available Fonts list.
5. Select the fonts you want to add and either accept the
   default target directories or enter the names of the
   PostScript font directories you are using.
6. Choose Add. The fonts are added and you are returned to
   the ATM Control Panel.
7. Choose OK to close the ATM Control Panel.
8. Delete all the files in the temporary directory; then
   delete the temporary directory.

16.  Installing ATM in the IBM OS/2 Windows Compatibility Box

Before installing ATM version 2.6 over an existing version
of ATM in the IBM OS/2 Windows Compatibility box, you must
first edit the Boot section of your Windows SYSTEM.INI file.
Replace the two lines

  SYSTEM.DRV=ATMSYS.DRV
  ATM.SYSTEM.DRV=SYSTEM.DRV

with this line

  SYSTEM.DRV=SYSTEM.DRV

After changing the SYSTEM.INI file, start Windows and
install ATM as instructed in the manual. Always make a
backup copy of the SYSTEM.INI before making changes. If you
make a mistake, you can use the backup file to restore
Windows to its previous configuration.

17.  Application Notes

When you add or remove a font while an application is
active, the application font menus are not always updated
automatically. If you add or remove a font while one of the
following applications is active,

  AmiPro 3.0, Freehand 3.0, Freelance 1.0, PageMaker 4.0,
  Ventura Publisher for Windows, and WordPerfect 5.2.for
  Windows
  
  
you must re-select your printer to update the application's
font menu.
  
Adobe Illustrator 4.0

If you have Illustrator 4.0 and add or remove a font, you
must restart Windows for font changes to take effect in
Illustrator.

If you are using newer versions of the Adobe Enumerator, you
need only restart Illustrator to see ATM font changes. Make
sure Illustrator is closed, however, before using ATM to add
or remove fonts.

The two enumerator files PSENUM.DLL and RUN_ENUM.EXE, which
Illustrator uses to register ATM font changes, are located
in the Windows system directory. If your enumerator files
are dated 7/17/92 or earlier, you need to obtain the most
recent versions of these files. You can receive the updates
to these files free of charge by contacting Adobe Customer
Support at (408) 986-6520. You can also download these files
from the Adobe CompuServe forum.

If you are using Enumerator files dated 7/17/92 or earlier,
you have to restart Windows for ATM font changes to take
effect.

CorelDraw!

CorelDraw! 3.0 now supports ATM fonts. Adobe strongly
recommends that you obtain the maintenance release version
3.0b. You must restart CorelDraw! for ATM font changes to
take effect. CorelDraw! version 2.0 does not support ATM
fonts.

Designer 3.1 and Charisma 2.1

The lines used for underlines and strikeouts may appear
broken when using an ATM font. This is due the way these
Micrografx products handle character placement.

If you rotate text at a 90 degree angle and then stretch it,
you might find that the font will not be properly resized. To
avoid this problem, first resize the font and then rotate it.
Rotating stretched text at a 180 degree angle might cause the
last letters in the text string to overlap. To correct this
problem, first rotate the text by 180 degrees and then
stretch it.

Rotated text at certain angles will sometimes print with
expanded character spacing on non-PostScript printers. To
correct this problem, try using the Print View option and
selecting the entire page.

FaceLift for Windows

FaceLift for Windows modifies the printer entries in the
WIN.INI. If FaceLift is installed, the ATM Installer and
Control Panel do not recognize the installed PostScript
printers and cannot add PostScript soft font entries for
these printers. Before adding fonts for PostScript printers,
you must first temporarily disable the "Print with FaceLift"
option in FaceLift Control Panel. After the fonts have been
installed, you can reenable "Print with FaceLift" option.

FastFonts

When using the type utility FastFonts, you must select the
"Print ATM text as Graphics" options in the ATM Control Panel
for ATM text to print properly.

Harvard Draw 1.01

Harvard Draw 1.01 supports ATM fonts. You must restart the
application for ATM font changes to take effect.

Harvard Graphics 1.01 for Windows

Harvard Graphics cannot rotate ATM fonts. Harvard Graphics
uses its own internal scalable fonts for all rotated text.

If you are using Harvard Graphics and ATM with a high-
resolution video driver, you must obtain the maintenance
upgrade version 1.021 or higher.

If you make ATM font changes while Harvard Draw is running,
you must select another printer driver and switch back to the
original driver for the ATM font changes to take effect.

Micrografx PostScript Driver

The Micrografx PostScript driver has a non-standard printer
section structure in the WIN.INI. PostScript soft fonts
cannot be installed to this driver using the ATM Installer
and Control Panel. You can, however, install your fonts
through the Micrografx Printer Setup menu. See your
Micrografx manual for instructions.

Program Manager replacements (Norton Desktop for Windows and
PCTools Desktop)

Due to the different structure of Norton Desktop's and
PCTools Desktop's application groups, the ATM Control Panel
icon might not be properly installed into the Main group
when you are running Norton or PCTools as your desktop
shell. If this occurs, you must manually add the ATM Control
Panel icon. The ATM Installer will, however, copy all of the
necessary files to your system and will configure Windows to
run ATM. See the Norton Desktop or PCTools manual for
instructions on how to install a new icon in a group.

Quattro Pro for Windows 1.0

You must restart this application for ATM font changes to
take effect.

PageMaker 4.0

For best results with ATM, set the "Vector text above" and
"Stretch text above" limits in the Preferences dialog box to
10000 pixels each.

If you make ATM font changes while PageMaker is running, you
must re-select your printer for the ATM font changes to take
effect.

TypeAlign 2.0

If you receive the error message "Application requested
abnormal termination" after installing TypeAlign under
Windows 3.1, you must manually remove the entry TALGNDLL.EXE
from the load line of the WIN.INI file. TALGNDLL.EXE is not
compatible with Windows 3.1. The Load= line in the WIN.INI
file is in the [windows] section. Be very careful when you
change WIN.INI. If you make a mistake, Windows will not work
correctly. TypeAlign 2.1 corrects this problem.

Ventura Publisher Windows Edition

Adobe recommends using version 4.1. For upgrade information,
please contact Ventura at (800) 822-8221.

The fonts Helvetica and Times do not display correctly in
Reduced View.

If you make ATM font changes while Ventura Publisher is
running, you must re-select your printer for the ATM font
changes to take effect.

Word for Windows 2.0

Double underlines may print as one thick underline at point
sizes larger than 18 points. This is due to the method Word
for Windows uses to perform double underlining.

Smart quotes (characters Alt+0147 and Alt+0148) might print
as double single quotes with some printer drivers. The
generic bullet character (Alt+0149) might print as a
lowercase o.

Various Applications with Draft Mode Printing Feature

Some applications which support draft mode printing do not
print graphics when they print in draft mode. When ATM is
set up to print ATM fonts as graphics, text might not print
correctly in draft mode. To correct this problem, clear the
"Print ATM Fonts as Graphics" option in the ATM Control
Panel.

Various Font Conversion Utilities

Some font conversion utilities, such as AllType,
automatically update the ATM.INI file. You must restart
Windows for font changes in the ATM.INI file introduced by
these utilities to take effect.

Virus Protection Software

Some virus protection software packages do not allow the ATM
Installer to replace previous versions of ATM software. If
you have this problem, first scan your hard drive for
viruses. Then temporarily disable your virus protection
software while you install ATM. Remember to re-enable your
virus protection software and re-scan your hard disk after
installing ATM.

WordPerfect for Windows 5.1 and 5.2

ATM requires the standard Windows printer drivers in order
to print ATM text. You cannot print ATM fonts with the
WordPerfect printer drivers, unless you also use a third-
party type utility such as PrimeType from LaserTools.

WordPerfect for Windows uses its own screen fonts for
preview, so ATM is not active in WordPerfect preview mode

Some special characters in the ANSI character set (with
values higher than 128) do not print as displayed. See your
Windows documentation for the ANSI character set.

17.  Novell Netware Considerations

Although ATM is not a network application, you can print to
network printers using ATM. However, you must configure the
File Contents section in your print job definition to
specify Byte stream instead of Text. The Byte stream option
is required because ATM sends raster graphics to non-
PostScript printers.

To add fonts from a network drive, you must first load a
SHELL.CFG file containing the line "show dots = on". Use
IPX.COM to load the SHELL.CFG file.

There is also a 34-character limit for the PostScript target
directories for the PFB and PFM files. If your target
directory name exceeds this limit, use the MAP ROOT command
(instead of the usual map command) to map the directory to a
simulated root directory.

18.  ATM and IBM 4029 Series Printers

To use ATM with one of the IBM 4029 series printers, you
must install version 3.01 or later of the 4029 printer
driver and version 1.65 or later of the Generic printer
driver. These versions are included with Windows 3.1.  If
you are using Windows 3.0, contact your printer dealer or
manufacturer for the latest drivers.

19.  Troubleshooting

Manually installing ATM

For detailed instructions on how to manually install ATM
version 2.6, call the Adobe Fax Request Line at (408)986-
6578 and request fax number 5120.

Justified Text Exceeds Right Margin On Screen

Justified text sometimes extends beyond the right margin on
screen, but prints correctly. This happens particularly at
small point sizes. Two situations can cause this problem:

* Windows uses a screen font for a font of a different
  size. Turning off the "Use Pre-built or Resident Fonts"
  option in the ATM Control Panel corrects this problem.
* The problem is caused by round-off errors occurring when
  an application calculates the total width of a line of
  text. This problem cannot be corrected.

Documents Containing the Arial MT and TimesNewRomanPS Fonts
Print Slowly

If, after upgrading to Windows 3.1, you find that documents
containing the fonts Arial MT and TimesNewRomanPS print more
slowly on a PostScript printer, you can take the following
steps to improve printing speed. These fonts are often used
as substitutes for the font Helvetica and Times when
included when ATM is bundled with another product.

To make documents containing Arial MT and TimesNewRomanPS
print faster, add the following lines to both the [Aliases]
section and [Synonyms] section of your ATM.INI file.

   Helvetica=Arial MT
   Times=TimesNewRomanPS

The following two lines

   Helv=Arial MT
   Tms Rmn=TimesNewRomanPS

should already be present in both ATM.INI sections.

Error removing the fonts Helvetica and Times
Helvetica and Times display upside-down

If you receive an error when removing the fonts Helvetica
and Times or if the fonts Helvetica and Times display upside-
down, check the ATM Control Panel to see if all of these
fonts are installed:

* Helvetica
* Times
* Arial MT
* TimesNewRomanPS.

If you have all these fonts, you must manually edit the
ATM.INI file and replace the following four lines in the
[Aliases] and [Synonyms] sections

   Helv=Arial MT
   Helvetica=Arial MT
   Tms Rmn=TimesNewRomanPS
   Times=TimesNewRomanPS

with these two lines

   Helv=Helvetica
   Tms Rmn=Times.

22.   Acknowledgments

The ATM team would like to acknowledge the following people
who have contributed to the testing of ATM version 2.6:  Jim
Al-Shamma, Geoff Arnold, Louis Fuh, Curtis Kunz, Ernie
March, Sasha Mobley, Grace Williams and Tokuro Yamashiro.

Appendix A. ATM.INI Parameters

The following section contains technical information on ATM
version 2.6's initialization file. This file is divided up
into six sections: Fonts, Setup, Settings, Mono, Aliases,
and Synonyms.

[Fonts]   This section contains a list of all of the fonts
          installed with ATM and the locations of the
          PostScript PFM and PFB files for each font. The
          fonts listed here are available to all non-
          PostScript printers for screen display and
          printing. The fonts available to PostScript
          printers are based on the PostScript soft font
          entries in the specific printer section of the
          WIN.INI file and the ROM-based fonts in the
          printer. If a font is listed in both WIN.INI and
          ATM.INI, ATM will rasterize the font to the
          screen. The printing of fonts to PostScript
          devices is solely the responsibility of the
          PostScript driver and Windows. ATM does not have
          to be active when printing to PostScript devices.

          Although a particular application will allow you
          to bold or italicize a certain font, your
          PostScript printer will not be able to render the
          bold or italic font unless an outline for the bold
          or italic font is available. On non-PostScript
          printers, however, ATM will synthesize a bold,
          italic or bold italic version of a font based on
          the roman outline if the outline of the desired
          font style is not present.
          
          When the ATM 2.6 Installer creates the [Fonts]
          section of the ATM.INI, it adds all the PostScript
          fonts found in the installation disk as well as
          any PostScript fonts listed as soft font entries
          for PostScript printers in the WIN.INI file. If
          you are upgrading from a previous version of ATM,
          the Installer also adds all the fonts listed in
          the current ATM.INI file.
          
[Setup]
PFM_Dir     Default PFM directory for the ATM Control Panel
            when adding PostScript fonts.
PFB_Dir     Default PFB directory for the ATM Control Panel
            when adding PostScript fonts.

[Settings]
FontCache=256       Specifies the size of the font cache in
                    kilobytes. This option is configurable
                    through the ATM Control Panel. The
                    default is 256k. Adobe recommends that
                    this value not be set to more than 64k
                    for each megabyte of physical memory.
ATM=On              Determines whether or not ATM is loaded
                    at Windows startup time.
BitmapFonts=On      This switch is set with the Use Pre-
                    Built or Resident Fonts check box in the
                    ATM Control Panel. When this switch is
                    on, ATM defers to screen fonts, resident
                    printer fonts, and printer soft fonts
                    rather than rasterizing the font itself
                    if the bitmap of the font requested in
                    available. If you change this switch,
                    you do not have to restart Windows for
                    the change to take effect. You might,
                    however, have to restart applications
                    for the setting to take effect. The
                    default setting is On.
SynonymPSBegin=9    This value specifies the point size at
                    which ATM starts using bitmap deferral
                    for font pairs listed in both the
                    Aliases and Synonyms sections. The
                    default is 9. Adobe recommends that you
                    do not change this setting.
QLCDir              Indicates the path of the ATM QuickLoad
                    file, ATMFONTS.QLC.
                    ATMFONTS.QLC contains a list of
                    installed fonts and font metrics, which
                    reduces Windows startup time. You can
                    force ATM to rebuild this file by
                    deleting the file with the MS-DOS DEL
                    command and restarting Windows.
Version=2.6         This value enables the ATM Installer to
                    determine the version of ATM when ATM is
                    not active; otherwise, the Installer
                    determines the version of ATM from the
                    ATM DLL.
DownloadFonts=On    This switch instructs ATM to print text
                    as soft fonts for Windows 3.1 printer
                    drivers that support this feature.

TmpDir=c:\psfonts\tmp    These settings are used for Adobe
TmpCount=00000000        Acrobat products and must not be
                         deleted.  The actual drive letter
                         and directories referenced in this
                         section must also exist.

[Mono]              This section contains a list of
Courier=Yes         monospaced fonts.
LetterGothic=Yes
PrestigeElite=Yes
Orator=Yes

[Aliases]   This section tells ATM to substitute the font on
            the right of the equal sign for the font on the
            left of the equal sign when an application
            requests the font on the left of the equal
            sign. The ATM Installer defines aliases for the
            fonts Helv, Tms Rmn, Courier, Roman, and
            Modern.

            When the "Use Pre-built or Resident Fonts"
            option is turned on, ATM uses the resident
            bitmapped fonts for point sizes for which
            bitmaps exist. When The Use Pre-built or
            Resident option is turned off, ATM uses the
            PostScript outline font to rasterize all sizes
            of the aliased bitmapped font.
            
Helv=Helvetica             The fonts left of the equals sign
Tms Rmn=Times              are Windows bitmapped fonts. By
Courier=Courier            using PostScript outline fonts,
                           ATM can rasterize a smooth font
                           when these fonts are requested at
                           sizes not available in bitmapped
                           form.
Roman=Times                These settings enable ATM to use
Modern=Helvetica           PostScript outlines instead of
                           the standard Windows vector
                           fonts.
Courier=Courier            This setting is needed if an
                           application requests the smallest
                           available fixed-pitch font. If
                           this line is not present, ATM
                           provides some applications with a
                           1-point Courier font.

Helv=Arial MT              If you have received ATM bundled
Helvetica=Arial MT         with another application, you
Tms Rmn=TimesNewRomanPS    might have the fonts Arial MT
Times=TimesNewRomanPS      and TimesNewRomanPS instead the
                           standard fonts Helvetica and
                           Times. You should have these
                           lines present in both the Aliases
                           and Synonyms sections. These
                           settings tell ATM to use internal
                           printer fonts when printing Arial
                           MT and TimesNewRomanPS to a
                           PostScript printer. This reduces
                           the time required to print
                           documents and improves the
                           quality of bold and italic text
                           styles.

                           If you have all four faces
                           (Helvetica, Times, Arial MT and
                           TimesNewRomanPS), you should use
                           the font pairs Helv=Helvetica and
                           Tms Rmn=Times in these sections.

[Synonyms]
Helv=Helvetica             Unlike the font pairs in the
Tms Rmn=Times              Aliases section, the font pairs
Courier=Courier            in the Synonyms are
                           interchangeable. This means that
                           when the font Helv is requested
                           at a point size not available in
                           bitmapped form, the font
                           Helvetica will be used.

                           In Windows 3.0 & 3.0a, when the
                           font Helvetica is requested and a
                           bitmapped Helv font of the exact
                           size is available, ATM will use
                           the Helv  bitmap to display the
                           font on the screen.

                           In Windows 3.1, the screen
                           fonts Helv and Tms Rmn have
                           been replaced by MS Sans Serif
                           and MS Serif, so screen font
                           deferral does not apply.

[Colors]                   This section may be added to
                           the ATM.INI to override
                           internal ATM defaults. The
                           switch is global for all
                           applications. While enabling
                           this switch might be useful for
                           a special purpose in a
                           particular application, it can
                           also produce unexpected results
                           in other applications.

PrintColorGraphics=Off     This switch instructs ATM to
                           print colored text as soft
                           fonts under Windows 3.1 rather
                           than as dithered graphics. The
                           printer driver must support
                           soft font creation for this
                           switch to work. For most
                           printer drivers, the resulting
                           soft fonts print as black or
                           white fonts.

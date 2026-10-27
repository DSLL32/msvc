echo off
cls
echo 	ษอออออออออออออออออออออออออออออออออออออออออป
echo 	บ    WELCOME TO THE VEDIT DEMO/TUTORIAL   บ
echo 	ศอออออออออออออออออออออออออออออออออออออออออผ
echo ÿ
echo To run the demo/tutorial, you must first copy the demo files to your hard disk
echo and de-compress them. While logged onto your hard disk, follow these steps:
echo ÿ
echo 	MD \VEDIT                    Alternatively, you can use the supplied
echo 	MD \VEDIT\DEMO               INSTALL.BAT file, where 'a' is the floppy
echo 	CD \VEDIT\DEMO               drive containing this disk and 'c' is your
echo 	COPY a:*.*                   hard disk:
echo 	DEMOFILE                                INSTALL a: c:
echo ÿ
echo To run the demo/tutorial, run the TUTOR.BAT file:
echo ÿ
echo 	TUTOR
echo ÿ
echo Feel free to copy the demo disk!  Make copies for your friends, etc.
echo Please copy the entire disk, so that all files are intact.
echo ÿ
pause
cls
echo Compiler Support:
echo ÿ
echo You will be able to compile and link the supplied example program ADDER.C
echo It purposely contains several obvious syntax errors for you
echo to experiment with.
echo ÿ
echo Print the file COMPILE.DEM which gives detailed instructions on the Compiler
echo Support portion of the demo. You can use VEDIT for this:
echo ÿ
echo 	VDEMO -X PRINT COMPILE.DEM
echo ÿ
pause
cls
echo To start up the Compiler Support, you can use the batch file "COMPILE.BAT":
echo ÿ
echo 	COMPILE ADDER.C
echo ÿ
echo Alternatively, if you have installed the V-SWAP program described below, you
echo can start up the Compiler Support while editing by selecting {MISC, Compiler
echo support} in the menu system. The default "hot-key" for this is ALT-I.
echo ÿ
echo Follow the instructions printed from COMPILE.DEM.
echo ÿ
pause
cls
echo VEDIT and VEDIT PLUS include modules to support these and other compilers:
echo ÿ
echo Microsoft:   C++, C, Quick C, Quick Basic, Fortran, Cobol, MASM Assembler
echo Borland:     C++, Turbo C, Turbo Pascal, Turbo Assembler
echo Avocet:      All Compilers, Assemblers and Cross-compilers
echo Lahey:       Fortran 77, Personal Fortran 77
echo Metaware:    High C
echo Micro Focus: Cobol
echo Microrim:    RBase Compiler
echo Nantucket:   Clipper Compiler
echo Realia:      Cobol
echo Watcom:      C Compiler
echo Zortech:     C++
echo Others:      2500 AD, Acucobol, Microtec, SVS, etc.
echo ÿ
echo The modules to support these compilers are available on our BBS. See below.
echo ÿ
pause
cls
echo The V-SWAP Program:
echo ÿ
echo V-SWAP is a small TSR program with which VEDIT can completely swap itself and
echo any desired TSRs out of memory in order to run the largest compilers and
echo debuggers. V-SWAP is normally run from your AUTOEXEC.BAT file, but for this
echo demo you can run it manually:
echo ÿ
echo 	VSWAP /E:1024
echo ÿ
echo (If your environment is larger than 1024 bytes, increase the "/E:nnn"
echo parameter.)
echo ÿ
echo Now edit the file ADDER.C: (you may need the "-M" option with B/W monitors)
echo ÿ
echo 	VDEMO ADDER.C           or          VDEMO -M ADDER.C
echo ÿ
echo To start up the Compiler Support, you now only need to press ALT-I.
echo ÿ
echo If desired, you can remove V-SWAP from memory after completing the demo:
echo ÿ
echo 	VSWAP /R
echo ÿ
pause
cls
echo To fully utilize V-SWAP, it should be run from within your AUTOEXEC.BAT file.
echo Place the command "VSWAP /E:1024" before other TSRs such as desktop utilities.
echo All TSRs following the VSWAP command will be swapped out of memory. In most
echo cases, V-SWAP can even swap network drives out of memory. (However, the VSWAP
echo command must follow any disk cache programs.)
echo ÿ
echo If you want to run VDEMO from other directories, and you have copied the files
echo to your hard disk, add the following line to your AUTOEXEC.BAT file:
echo ÿ
echo 	SET VEDPATH=C:\VEDIT\DEMO      (where "C" is your hard disk)
echo ÿ
echo After modifying your AUTOEXEC.BAT, you must re-boot for the changes to take
echo effect.
echo ÿ
echo When VEDIT shells out to DOS (runs DOS programs), V-SWAP will swap VEDIT and
echo other TSRs out of memory. This can give you more available memory than you had
echo before you entered VEDIT!
echo ÿ
echo For a summary of all V-SWAP options, give the command:
echo ÿ
echo 	VSWAP /?
echo ÿ
pause
cls
echo Description of files on disk:
echo ÿ
echo VDEMO.EXE	The demo version of VEDIT PLUS. It is limited to 1000
echo 		keystrokes before it exits. Alternatively, with the "-D"
echo 		option, you have unlimited keystrokes, but can only save very
echo 		small files. Configuration changes cannot be saved.
echo ÿ
echo VSWAP.EXE	A small TSR with which VEDIT can completely swap itself and any
echo 		desired TSRs out of memory in order to run large compilers and
echo 		debuggers. The command "VSWAP /?" displays all VSWAP options.
echo ÿ
echo VVHELP.HLP	Help file accessed by [HELP] function - (F1). You can print the
echo 		".HLP" files for more information about VEDIT.
echo ÿ
echo VPHELP.HLP	Help file for the Command Mode accessed by the "H" command.
echo ÿ
echo VPEHELP.HLP	Help file for Command Mode topics accessed by the "EH" command.
echo ÿ
echo WHATS.NEW	A summary of enhancements made to VEDIT PLUS since version 3.00
echo 		(1988). Primarily oriented towards previous VEDIT PLUS users.
echo ÿ
pause
cls
echo HELP.BAT	A very simple Help system that uses the DISPLAY.VDM macro. It
echo 		displays the file HELP.TXT.
echo ÿ
echo PRINT.VDM	Simple "macro" for printing files. It prints the filename date
echo 		and page number on each page and is a nice alternative to the
echo 		DOS "PRINT" command. To print the file "myfile" give the DOS
echo 		command:
echo ÿ
echo 		VDEMO -X PRINT myfile
echo ÿ
echo DISPLAY.VDM	Simple "macro" for displaying files. Uses a window to give
echo 		instructions and is a nice alternative to the DOS "TYPE"
echo 		command. To display the file "myfile" give the DOS command:
echo ÿ
echo 		VDEMO -X DISPLAY myfile
echo ÿ
echo 		Alternatively, you can create a batch file "D.BAT" containing
echo 		"VDEMO -B -X DISPLAY %%1". Then use the simpler command:
echo ÿ
echo 		D myfile
echo ÿ
pause
cls
echo BRIEF.KEY	These files are alternate keyboard layouts which emulate
echo WORDPERF.KEY	Brief, Word Perfect and WordStar. The emulations are
echo WORDPERF.KEY	described in the corresponding ".DOC" files. Instructions
echo 		for using the ".KEY" files are given below.
echo ÿ
echo DEMO.VDM	The DEMO.VDM and DEMOxx.VDM are the demo "macro" files.
echo ÿ
echo DEMOxx.TXT	Text files and instruction files which make up the demo.
echo 		Automatically loaded by DEMO.VDM.
echo ÿ
echo VCSD.EXE	When you run this program, it expands into all the files needed
echo 		to run the Compiler Support portion of the demo. This should be
echo 		done on a hard disk.
echo ÿ
pause
cls
echo Editing your own files:
echo ÿ
echo After running this demo/tutorial, you can try out the demo version of VEDIT
echo PLUS on your own files with the command:
echo ÿ
echo 	VDEMO filename      (also multiple files, e.g. "VDEMO *.C *.H")
echo ÿ
echo 	This will let you edit files of any size, but limits you to 1000
echo 	keystrokes - it will then exit after letting you save or abandon
echo 	your changes.
echo ÿ
echo Alternatively, you can run the demo with an unlimited number of keystrokes,
echo but cannot save files greater than 2K in size. The command is:
echo ÿ
echo 	VDEMO -D filename
echo ÿ
pause
cls
echo Browsing files:
echo ÿ
echo VEDIT has a special mode for browsing files; this prevents accidental
echo alterations. In browse mode you can instantly jump to the beginning or end
echo of the file, or to a relative position in the middle when using a mouse.
echo ÿ
echo VDEMO lets you browse and perform searches in files up to 2 Gigabytes in
echo size with the command:
echo ÿ
echo 	VDEMO -D -B filename
echo ÿ
pause
cls
echo Other invocation options: (Troubleshooting)
echo ÿ
echo If you have a B/W monitor, e.g. a "Monochrome VGA", you may need the "-M"
echo option; otherwise the pull-down menus may be blank. Use the command:
echo ÿ
echo 	VDEMO -M filename
echo ÿ
echo VEDIT normally increases the typematic (repeat) rate of the keyboard. This
echo causes problems (slow exiting, hanging) on some machines. Use the "-K"
echo option to have VEDIT leave the typematic rate unchanged:
echo ÿ
echo 	VDEMO -K filename
echo ÿ
echo If you are using a Microsoft or Logitech mouse, VEDIT will start up faster
echo (by a few seconds) if you use the "-P" option. (It skips the time consuming
echo "hardware" mouse reset.) However, "-P" will prevent the Mouse Systems and
echo possibly other mouse drivers from working properly.
echo ÿ
echo 	VDEMO -P filename
echo ÿ
echo (VEDIT can be configured so that these invocation options aren't needed.)
echo ÿ
pause
cls
echo For a summary of all VEDIT invocation options, give the command:
echo ÿ
echo 	VDEMO /?
echo ÿ
echo Notes:
echo ÿ
echo For the on-line help to work, the ".HLP" files must be in the current
echo directory, or in "\VEDIT\DEMO" of the current drive.
echo ÿ
echo VEDIT can be configured to find its help files in any desired directory of
echo any drive. (See also the "SET VEDPATH=..." command above.)
echo ÿ
pause
cls
echo Changing the Keyboard Layout:
echo ÿ
echo The {MISC, Keyboard layout} sub-menu lets you change the keyboard layout as
echo desired. Let the on-line help guide you. The "Save to disk" function lets you
echo save the entire keyboard layout as a .KEY file. "Load from disk" lets you load
echo a complete new keyboard layout at any time. The .KEY files are easily edited;
echo this gives you another (preferred) method of changing the layout. However,
echo this demo version does not let you permanently save any layout changes.
echo ÿ
echo To setup VEDIT to emulate Brief, Word Perfect or Wordstar, load the files
echo BRIEF.KEY, WORDPERF.KEY or WORDSTAR.KEY by selecting {MISC, Keyboard layout,
echo Load from disk}. The files BRIEF.DOC, WORDPERF.DOC and WORDSTAR.DOC document
echo these emulations. (VEDIT PLUS includes a macro to thoroughly emulate the UNIX
echo "vi" editor.)
echo ÿ
echo Note:	The instructions during the demo are set up for the default
echo 	keyboard layout. Therefore, we suggest finishing the demo before
echo 	changing the keyboard layout.
echo ÿ
pause
cls
echo The Greenview 24-Hour Bulletin Board System (BBS) for VEDIT:
echo ÿ
echo Our 24-hour BBS helps us service our customers in the fastest way possible.
echo Free services available on the BBS include:
echo ÿ
echo *  The latest version of our demo/tutorial disk and shareware VEDIT Jr.
echo *  Sales and update information; news of recent and upcoming versions.
echo *  Access to all VEDIT PLUS supplemental files. This includes all
echo    compiler support, help and macro library files.
echo *  Registered users can download minor updates at no charge.
echo *  Technical support Questions/Answers, application notes, etc.
echo ÿ
echo To download all VEDIT PLUS files including the full ".HLP" help files and
echo support for all compilers, select "F" (File library) and then download the
echo files VP-FILES.EXE and DISTCOMP.EXE. When you run them, they will expand
echo into all VEDIT PLUS files, except of course the "live" VEDIT.EXE.
echo ÿ
echo Also, if you are in a hurry to use one of our products, you can contact
echo our sales department with your ordering information and download the
echo "live" product via our BBS within a few hours.
echo ÿ
pause
cls
echo Enjoy the demo and be sure to contact us if you have any questions.
echo ÿ
echo Greenview Data Inc.
echo P.O. Box 1586
echo Ann Arbor, MI  48106
echo ÿ
echo Toll Free:   (800) 45-VEDIT  (U.S. and Canada)
echo Telephone:   (313) 996-1300
echo Fax:         (313) 996-1308
echo 24-Hour BBS: (313) 996-1304  (14,400/9600/2400/1200/300 baud, 8 Data bits)
echo 			     (Supports MNP, V.32, V.42bis)
echo ÿ
echo ÿ
echo NOTES:	To print this file, give the DOS command:
echo ÿ
echo 	VDEMO -X PRINT READ-ME.BAT
echo ÿ
echo 	If you notice letters missing from the pull-down menus when running
echo 	on a monochrome display, you may have the contrast control on your
echo 	monitor adjusted too low or may need to use the "-M" invocation
echo 	option described above.
echo ÿ

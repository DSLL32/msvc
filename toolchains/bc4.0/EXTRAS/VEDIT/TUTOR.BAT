echo off
cls
echo -
echo - This starts up the VEDIT PLUS demo/tutorial, but does not include the
echo - Compiler Support.  Run READ-ME.BAT for directions on how to run the
echo - Compiler Support.
echo -
echo - After running this demo/tutorial, you can try out the demo version of
echo - VEDIT PLUS on your own files with the command:
echo - 	VDEMO filename      (also multiple files, e.g. "VDEMO *.C *.H")
echo -
echo - 	This will let you edit files of any size, but limits you to 1000
echo - 	keystrokes - it will then exit after letting you save or abandon
echo - 	your changes.
echo -
echo - Alternatively, you can run the demo with an unlimited number of keystrokes,
echo - but cannot save files greater than 2K in size. The command is:
echo - 	VDEMO -D filename
echo -
echo - You can also quickly browse huge (up to 2 Gigabyte) files with the command:
echo - 	VDEMO -D -B filename
echo -
pause
echo - The demo\tutorial takes a few seconds to load...
echo -
vdemo -d -z -s196 -x demo

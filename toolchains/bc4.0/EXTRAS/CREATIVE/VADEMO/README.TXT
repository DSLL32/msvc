			VOICEASSIST - DEMO VERSION
		 Copyright Creative Technology Ltd. @1993
			    All rights reserved

This demo version provides much of the functionality of the full-feature 
retail version of VoiceAssist.  VoiceAssist works with all sound cards 
which can sample at an 8 kHz sampling rate.  The full-feature version of 
VoiceAssist is available at your local Sound Blaster dealer for a retail 
price of $99.95.


LIMITED FUNCTIONALITY

The demo version of VoiceAssist scales-down many of VoiceAssist's features:

			VoiceAssist Demo        VoiceAssist Retail Version

Number of Users                 1                       Up to 256
Number of App.'s                2                       30 per user
Commands per App.               20                      992
Max. commands at one time       52                      1024
User-defined Commands           yes                     yes
Automatic Menu Extraction       yes, up to 18 items     yes


INSTALLATION


1. Run INSTALL under Windows and specify the directory for VoiceAssist.   
   If you are using a card which has a SOUND environment string, you
   should install VoiceAssist as a subdirectory of your existing SOUND 
   environment string path.  Assuming you've installed a Sound Blaster 
   on drive C: this path will be one of the following:

	CARD            INSTALLATION DIRECTORY

	SB              C:\SB\V_ASSIST
	SBPRO           C:\SBPRO\V_ASSIST
	SB16            C:\SB16\V_ASSIST

   If your card does not have a SOUND environment string, you can install
   VoiceAssist off the root directory:
   
	
	C:\V_ASSIST
   
   You will also need to add the following SOUND environment string to your
   AUTOEXEC.BAT file:
	      
	SET SOUND=C:


2. Reboot your system.


USING THE ON-LINE HELP

The demo version of VoiceAssist ships with the retail version's on-line 
help. For instructions about using VoiceAssist refer to the on-line help 
by selecting "Help" from VoiceAssist's main menu or pressing the F1 key.

Since the demo version only supports one user file, there are different 
instructions when running VoiceAssist the first time.  Please refer to 
the following section when using getting started with the demo version 
of VoiceAssist:


DEMO QUICK START: RUNNING VOICEASSIST THE FIRST TIME

Before using VoiceAssist for the first time, a user file will have to 
be created in the following way:

1.  Run VoiceAssist.  Click the menu button and open the User dialog 
    or press "Alt-U".

2.  Click "new", type your first name for the user file name (8 character 
    max), and click "OK" to confirm.

3.  Click "Close" to close the User Dialog.


TRAINING THE GENERIC SET

To train VoiceAssist for the 32 generic commands which can be used at 
any time, do the following:

1.  Click the menu button and open the Training dialog or press "Alt-T".

2.  With "Generic commands" highlighted, click "All" to train the voice 
    for all 32 commands in the generic set.

3.  The Voice Training dialog will appear.  Click "OK" to begin training.
    Say each word once.  The Voice Training dialog will close when you 
    have finished training the generic set.

4.  Click "Save" to save your Voice trainings.

5.  Your generic set training can be tested by clicking the "Test" button 
    at upper right of the Training dialog.  Say each word and the 
    recognized word will appear to the left.  Click "Disable" when you 
    are finished testing the recognition.


TO ADD APPLICATIONS TO YOUR USER FILE

The demo version of VoiceAssist supports up to two applications.  To 
train one of the applications in the generic set do the following:

1.  In this example, open the training dialog, find the "clock" command 
    and double-click it.  The clock will launch and the Training Dialog 
    title will change to "Clock".

2.  Notice that the menu commands for clock have automatically been 
    extracted from the clock and now appear in the Commands list box.  
    Notice that the cursor in the Commands list box highlights "Clock 
    Commands".

3.  To train the Clock click the "All" button.  The Voice Training dialog 
    will appear.  Click "OK" and say each command.  When you have finished 
    training the clock, the Voice Training dialog will close automatically.

4.  Click "Save" to save your training for the Clock.

5.  Close the Training Dialog and return to the User dialog by selecting 
    "User" or pressing "Alt_U".  Notice that the Clock has been added as 
    an Included Application in your User file.  

NOTE: You will be unable to save more than two applications in the demo 
version of VoiceAssist.  To prevent you from saving a third application, 
the "Save" button will be disabled in the training dialog.

Refer to "Using VoiceAssist" and "Training VoiceAssist" in the on-line help 
for more information.


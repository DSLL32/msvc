/******************************************************************************
APICLI.H header file
Programmer : Virgil Dobjanschi
******************************************************************************/
/******************************************************************************/
/**************** Error codes for the action training *************************/
#define REC_OK          0       /* Operation was successful.*/
#define REC_ACTIVE      1       /* Attempt to start recording while already recording.*/
				/* Attempt to start play while already playing.*/
#define REC_INACTIVE    2       /* Attempt to stop recording while NOT recording.*/
#define REC_NOMEMORY    3       /* When attempting to start recording or during recording.*/
#define REC_NOEVENTS    4       /* Attempt playback with no events in memory block.*/
#define REC_TOOMANY     5       /* Attempt record more than 65535 events.*/
#define REC_SYSMODALON  6       /* Recording/playing halted because a system-modal*/
				/* dialog box appeared.*/
/******************************************************************************/
/************** Error codes in voice training *********************************/
#define ANSWER_NOERROR   0      /* Training, recognition OK.*/
#define ANSWER_REJECTION 1      /* Word was rejected ( not recognized).*/
#define ANSWER_SIMILAR   2      /* Word is to similar to another one.*/
#define ANSWER_NOISY     3      /* Bad recording.*/
/******************************************************************************/

// The following structure describes one command
typedef struct
{
	char szCommName[32];    // Command name
	WORD wData;             // Flags
}COMMINFO;
/* There are 3 flags in wData:
	Bit 0:  '0' for enable , '1' for disable
	Bit 1:  Voice training flag
	Bit 2:  Action training flag
*/
/******************************************************************************/

typedef struct
{
	char    szAppName[32];          // Application name
	char    szUserName[32];         // User name
	WORD    wNumCommands;           // Number of commands
	WORD    wOnOff;                 // Driver is on or off
	WORD    wAdvise;                // Advise is on or off
	WORD    wAction;                // VoiceAssist's macro playback is on or off
	WORD    wCapture;               // Captured or not
	COMMINFO sCommInfo[1024];
}SRCONTEXT, FAR *LPSRCONTEXT;

typedef struct
{
	char    szAppName[32];          // Application name
	char    szUserName[32];         // User name
	char    szCommName[32];         // Command name for SRAddTraining
	WORD    wIndex;                 // Index of the command to delete or train
	BOOL    bNew;
}SRWORD, FAR * LPSRWORD;

typedef struct {char szUserName[9];}    USERNAME;
typedef struct
{
	WORD     wNumUsers;
	USERNAME szTableName[256];
}SRUSERLIST, FAR *LPSRUSERLIST;

// Prototypes for API functions

#ifdef __cplusplus
extern "C" {
#endif /* Cplusplus */

	WORD WINAPI _export SRInitializeLink( HWND);
	WORD WINAPI _export SRTerminateLink( HWND);
	WORD WINAPI _export SRClose( VOID);
	WORD WINAPI _export SROnOff( HWND, BOOL);
	WORD WINAPI _export SRActionOnOff( HWND, BOOL);
	WORD WINAPI _export SRChangeUser( HWND, LPSTR);
	WORD WINAPI _export SREnableCommand( HWND, LPSRCONTEXT);
	WORD WINAPI _export SRRequest( HWND, LPSRCONTEXT);
	WORD WINAPI _export SRGetUserList( HWND, LPSRUSERLIST);
	WORD WINAPI _export SRNotify( HWND, BOOL);
	WORD WINAPI _export SRAddCommand( HWND, LPSRWORD);
	WORD WINAPI _export SRDeleteCommand( HWND, LPSRWORD);
	WORD WINAPI _export SRTrainVoice( HWND, LPSRWORD);
	WORD WINAPI _export SRTrainAction( HWND, LPSRWORD);
	WORD WINAPI _export SRSave( HWND, LPSTR);
	WORD WINAPI _export SRGetVersion( HWND, LONG FAR*);
	WORD WINAPI _export SRAbortVoiceTraining( HWND);
	WORD WINAPI _export SROpen( VOID);
	WORD WINAPI _export SRFreeze( HWND, BOOL);
	WORD WINAPI _export SREnableButtons( HWND,BOOL);
	WORD WINAPI _export SRRenameCommand( HWND, LPSRWORD);
#ifdef __cplusplus
}
#endif

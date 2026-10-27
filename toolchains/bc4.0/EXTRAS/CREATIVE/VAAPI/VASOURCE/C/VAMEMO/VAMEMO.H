#define VADLGOPEN 4
#define IDD_STATUS	627
#define ADD_EDIT	101
#define ADD_OK	102
#define ADD_CANCEL	103
#define WM_USER_MY_INIT		WM_USER+100

#define IDD_LISTBOX		101
#define IDD_REQUEST		113

#define IDD_ADD			120
#define IDD_EDIT 			121
#define IDD_TR_VOICE		122
#define IDD_RECORDMEMO 	123
#define IDD_TR_ACTION  	124

#define IDD_DELETE		125
#define IDD_GET			126
#define IDD_STOPRECORD	127


#define MAXMEMO 100

#define WM_PLAYVOICE WM_USER+192
#define WM_RECORDMEMO WM_USER+193
#define WM_SAVE WM_USER+194

#define DELETEACTION 32
#define ADDACTION		33
#define GETACTION		1
#define GOTOSLEEP	6
#define WAKEUP 23
struct MEMO
{

	char szVoiceFile[32];                  


};

long FAR PASCAL _export WndProc  (HWND, UINT, UINT, LONG);

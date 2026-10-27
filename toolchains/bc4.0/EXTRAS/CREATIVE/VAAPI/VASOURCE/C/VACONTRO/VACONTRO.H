
BOOL WINAPI _export VoiceTrainProcModel(HWND,UINT,
				UINT,LONG);

BOOL WINAPI _export AddCommProc(HWND hwnd, UINT message,
							 UINT wParam, LONG lParam);

BOOL WINAPI _export RenameCommProc(HWND hwnd, UINT message,
							 UINT wParam, LONG lParam) ;

static void myRegisterAPIWndMess(HWND hwnd);
static void myRequest(HWND hwnd);
static void myDeleteComm(HWND);
static void myBecomingActive(HWND,LPARAM);
static void myTrainVoice(HWND hwnd);
static void myMsgActionStart(HWND hwnd,UINT,LONG);
static void myMsgActionStop(HWND hwnd,UINT wParam,LONG lParam);
static void myMsgSpeechStop(HWND hwnd,UINT,LONG);
static void myDisableEnableComm(HWND,UINT wParam);
static void myGetUsers(HWND hwnd);
static void myChangeUser(HWND hwnd);
static void myDisableEnableComm(HWND,UINT wParam);
static void myAddComm(HWND hwnd);
static void myTrainAction(HWND hwnd);
static void myGetVersion(HWND hwnd);
static void myRenameComm(HWND hwnd);

HWND	hCommListBox, hUserListBox;

WORD	msgActionStart, msgActionStop, msgSpeechStart, msgSpeechStop;
WORD	msgLinkTerminated, msgMeter, msgAppSwitch;
ATOM aAtomID;

LPSRCONTEXT	lpSRContext;
LPSRUSERLIST	lpSRUserList;
SRWORD	sSRWord;
int gnIndex;
BOOL gbTrainAction = FALSE,gbVAStatus;

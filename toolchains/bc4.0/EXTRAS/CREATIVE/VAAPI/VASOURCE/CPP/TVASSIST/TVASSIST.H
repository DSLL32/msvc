#include "vasapi.h"
#define 	ACTIONOFF 0

#define ACTIONON 1
#define PREVACTION 2

#define NOAPPFILE 1

#define CM_CREATE 101
#define UNKNOWNERROR 9
#define WM_VATRAINVOICE WM_USER + 501

typedef struct    // for confidence
{
  WORD wRecognizedWord;
  WORD wRecognizedScore;
} SCONFIDENCE,FAR *LPSCONFIDENCE;

class  _export TVAssist
{
     	ATOM gaUserAtomID;
		BOOL  bVAStatus;
		BOOL bSingleComd,bPrevAct,bEnableButtons,bFirstTime;

	 public:
	 
		WPARAM wVAParam;
		LPARAM lVAParam;
		
		HWND hMain;
		LPSRCONTEXT lpSRContext;
		LPSRUSERLIST lpSRUserList;
		WORD msgTrainVoice;		
		SRWORD sSRWord;
		WORD  wCurTraining;
		BOOL bTrainAction,bTrainVoice,bLink;

		TVAssist();
		TVAssist(int ,BOOL);
		TVAssist(TVAssist far* VAParent/* = NULL*/,int ,BOOL );
		~TVAssist(void);

		/* ----------  Handlers for registered VA messages -----------------  */

		LRESULT  ActionStart(WPARAM,LPARAM );
		LRESULT ActionStop(WPARAM,LPARAM );
		LRESULT AppSwitch(WPARAM,LPARAM );
		LRESULT LinkTerminated(WPARAM,LPARAM );
		LRESULT Meter(WPARAM,LPARAM );
		virtual LRESULT SpeechStart(WPARAM,LPARAM );
		LRESULT SpeechStop(WPARAM,LPARAM );

		/*  high level functions */
		int AddCommand(LPSTR);
		virtual int ChangeUser(LPSTR);   //
		int DeleteCommand(WORD);  //
		virtual int EnableCommand();
		virtual void InitializeLink( HWND, UINT );
		virtual void ReInitializeLink( HWND);
		virtual  void SaveTrainAction( void);
		virtual int Save(LPSTR szUserName);    //
		virtual void SetCommandStatus(WORD nStart,WORD nEnd,BOOL bEnable);
		int TrainAction( int);
		int TrainVoice( WORD, WORD, BOOL,WORD);
		int RenameCommand(WORD,LPSTR);

		/* ----------   TVAssist virtual functions ---------------- */
		virtual void ActionStartError( );
		virtual void ActionStartSuccess( );
		virtual void ActionStopTrainAction( );
		virtual void ActionStopError( ){};
		virtual void ActionStopRecognition( ){};
		virtual void BecomingActive( );
		virtual void BecomingActivePostRequest( ){};
		virtual void BecomingInActive( );
		virtual void Confidence( LPSCONFIDENCE);
		virtual void DisplayCommand( WORD, WORD);
		virtual void FinishTrainVoice(void){};

		virtual void MeterTrainMode();
		virtual void MeterRecogMode();
		virtual void ProcessNoApp();
		virtual void SpeechStopError( );
		virtual void SpeechStopErrorRecognition();
		virtual void SpeechStopErrorTrainVoice( );
		virtual void SpeechStopSuccess( );
		virtual void SpeechStopSuccessTrainVoice( );
		virtual void SpeechStopSuccessRecognition();

		// VoiceAssist "Wrapper" functions have VA prefix and are not virtual

		int VAAbortVoiceTraining();
		int VAActionOnOff(BOOL);
		int VAAddCommand();

		int VAChangeUser(LPSTR);

		int VAClose(void) { return SRClose();};
		int VADeleteCommand();
		int VAEnableCommand();

		int VAEnableButtons(BOOL bEnableButtons);
		int VAFreeze(BOOL);
		int VAGetUserList();
		int VAGetVersion(LONG FAR* lplVersion) { return SRGetVersion(hMain,lplVersion);};
      int VAInitializeLink(HWND);
		int VANotify(BOOL);
		int VAOnOff(BOOL);
		int VAOpen(void) { return SROpen();};
		int VARenameCommand();
			//
		int VARequest();
		int VASave(LPSTR);

		int VATerminateLink();
		int VATrainAction();
		int VATrainVoice();

};

extern "C" BOOL far _export AddVADlg(HWND,HWND,TVAssist*);
extern "C" BOOL far _export DeleteVADlg(HWND,HWND ,TVAssist* );
extern "C" BOOL far _export RenameVADlg(HWND,HWND,TVAssist*,WORD );
extern "C" BOOL far _export TrainVoiceVADlg(HWND,HWND,TVAssist*,BOOL,WORD);


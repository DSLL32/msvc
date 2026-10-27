
#include <owl\dialog.h>

 class TVADelete: public TVAssist
{
	public:
		TVADelete(TVAssist* pParent):TVAssist(pParent,ACTIONOFF,FALSE){};
		void ShowComdInListBox(HWND );
		virtual void BecomingActivePostRequest();

};//{{TDialog = tdlg}}
class TVADlgDelete : public TDialog {
public:
	 TVADelete* VADeleteObject;
    TVADlgDelete (TVAssist* pParent,TWindow* parent, TResId resId, TModule* module = 0);
	 virtual ~TVADlgDelete ();
	 void ErrorMessage(int);
	void EvActivate(UINT active,BOOL minized,HWND hWndOther);
	LRESULT EvAppSwitch(WPARAM,LPARAM);
	LRESULT EvLinkTerminated(WPARAM,LPARAM);
	void CmOk();
	void CmCancel();
	DECLARE_RESPONSE_TABLE(TVADlgDelete);
};    //{{TVADlgDelete}}



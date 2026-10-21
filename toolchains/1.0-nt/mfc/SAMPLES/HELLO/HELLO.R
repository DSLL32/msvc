#include	"Types.r"
include     "hello.res";
include     "wlm.res";
include     "wlmcdlg.res";
resource    'SIZE' (-1)
{
	dontSaveScreen,
	acceptSuspendResumeEvents,
	enableOptionSwitch,
	canBackground,				
	multiFinderAware,			
	backgroundAndForeground,	
	dontGetFrontClicks,			
	ignoreChildDiedEvents,		
	is32BitCompatible,			
	isHighLevelEventAware,
	localAndRemoteHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	1500 * 1024,
	1200 * 1024	
};


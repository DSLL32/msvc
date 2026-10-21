#include	"Types.r"
#include	"ftab.r"
include     "chkbook.res";
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
	1800 * 1024,
	1500 * 1024	
};


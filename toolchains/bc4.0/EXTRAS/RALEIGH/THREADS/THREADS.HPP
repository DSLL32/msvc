/*  VCS_ID
 *  $Filename:   threads.hxx
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:57:26  $
 */
 
#include "threads.h"

// Frame window class.  All threads including the primary thread
// will create one of these objects

class TopWindow : public wFrameWindow
{
	private:
		wMenu *menubar;
		wIcon *icon;
	public:
		TopWindow();
		~TopWindow();
		void Paint();
		long MenuCommand(wCommandMsg m);
};


// The following derives a class from the libraries 'bThread' class.  
// When an instance of the library is initialized for an application
// The object library creates one instance of the bMainThread class.  This
// instance is for the primary thread.  Each time a new instance of the
// bThread class is created, the function Start() will be called in
// in a new thread.  To destroy a thread, either delete the object or 
// call the bThread::Quit() quit member 

class ChildThread : public bThread
{
	public:
		ChildThread(pbThread par, char *name) : bThread(par, name, 0, 16384) {};
		short Start();
};



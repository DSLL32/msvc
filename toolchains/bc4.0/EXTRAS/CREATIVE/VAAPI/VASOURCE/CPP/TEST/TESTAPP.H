#ifndef __testapp_H				// Sentry, use file only if it's not already included.
#define __testapp_H

/*  Project test

	 Copyright © 1993.  All Rights Reserved.

	 SUBSYSTEM:    test.exe Application
	 FILE:         testapp.h
	 AUTHOR:


	 OVERVIEW
	 ========
	 Class definition for testApp (TApplication).
*/


#include <owl\applicat.h>
#include <owl\decframe.h>
#include <owl\editfile.h>
#include <owl\opensave.h>
#include "testapp.rh"            // Definition of all resources.


class testApp : public TApplication {

public:
	 void InitMainWindow();

};


#endif 		// __testapp_H sentry.

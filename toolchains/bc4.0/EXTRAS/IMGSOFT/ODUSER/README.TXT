README.TXT for ODUSER.DLL
-------------------------

This readme file contains information detailing how to extend Object/Designer
through the use of a DLL.  You should be very familiar with Object/Designer,
source drivers, and DLLs before attempting to use this method of extending
Object/Designer. The DLL provides low-level access to the data structures
used within the program. It is possible to crash the application. 



Using ODUSER.DLL to extend Object/Designer
------------------------------------------

This release allows you to extend Object/Designer by adding your own DLL. 
This DLL can be used in two ways:

  1. You can easily add your own functions to the source driver language.
  2. You can write your own interface to edit attributes for any object 
     within the design stage of Object/Designer.
        			
The DLL must be named ODUSER.DLL and must be located within your path or in
your Object/Designer directory.


Testing the Sample ODUSER.DLL
-----------------------------
Object/Designer includes a simple version of ODUSER.DLL that you can 
experiment with and modify.  It was written and compiled with Borland
C, but it should be easy to adapt to other compilers.

To test this DLL, do the following:
  1. Copy ODUSER.DLL into your Object/Designer directory.
  2. Run Object/Designer
  3. Add an attribute definition to the application object with the name 
     Custom.
  4. Double-click on the Custom attribute for an application.
  5. A dialog will be displayed from the DLL that lets you modify several
     attributes at the same time.

To test the user-deined source driver function, modify a source driver
to use a function called StrUp(). It may look similar to this:

	Uppercase Test String = $StrUp("Test String")$

The output should look like this:

	Uppercase Test String = TEST STRING


Writing your own Source Driver Functions
-----------------------------------------
It's very easy and fast to write your own source driver functions. In your
ODUSER.DLL create your functions similar to the following prototype:

  LPSTR CALLBACK _export SampleFunction( HWND, LPSTR );

The following is a simple example of a custom function that converts a string
to uppercase. Of course you could use the built-in function $UP()$ but this
makes an easy example.

  LPSTR CALLBACK _export SampleFuntion(HWND hWnd, LPSTR str)
  {
     strupr(str);	// Can use str as a work buffer of 255 chars
     return str;	// Return the resulting string
  }
  

Within your source driver you can access this user-defined function just like
the built-in functions.

	$VAR VMenuID = SampleFunction("IDM_"+MenuItem.Name)$

This simple interface can allow you to do some very sophisticated things
from within the source drivers. You can embed several values into the
string that is passed into the function. Your function can then parse the
string to retrieve these values. Multiple values can also be returned in a
similar manner.

These functions are not just limited to manipulating a string either.  Your
function could display dialogs to retieve data from the user, or query an
external database.



Writing your own Interface to Edit Object/Designer Attributes
--------------------------------------------------------------
Object/Designer now allows you to use your own routines to edit attributes.
This is very helpful when integrating Object/Designer with external tools,
data, etc.  In your ODUSER.DLL create the following prototype for this 
custom routine:

  BOOL CALLBACK _export UserAttributeEditHook(HWND hWnd, LPSTR szObject, 
	   LPSTR szAttr);

You must follow this convention exactly. Object/Designer will call this 
function whenever you double-click on a user-defined attribute. It will 
pass to your function the object name and attribute name. Your function
can then decide what action to take.  You can return FALSE for the
default Object/Designer behavior.
The following is an example:

BOOL CALLBACK _export UserAttributeEditHook(HWND hWnd, LPSTR szObject, 
  LPSTR szAttr)
{
	// Must store handle to access attributes
	hAttrWnd = hWnd;

	if ( !strcmp(szAttr, "DLLs") && !strcmp(szObject, "Application") )
	{
		// Custom routine to allow user to select from available DLLs

		UserGetAttrValue(szAttr, szBuff);
		GetDLLNames(hWnd, szBuff);	 
		UserSetAttrValue(szAttr, szBuff);
		return TRUE;	 // Attribute has been edited
	}

	return NULL;   // No Action taken
}

In the simple example above, a single attribute value is set. You are free 
to modify as many attributes as needed. You may want to write a custom 
interface for all system objects. If the user clicks on an attribute you
could then allow the user to modify all attributes for the selected object.

Two functions have been provided to allow you to access the attribues for
the selected object. They are:

  UserSetAttrValue (LPSTR Attr, LPSTR Value);
  UserGetAttrValue (LPSTR Attr, LPSTR Value);

When providing your own edit routine you should format the attribute value
according to standard Object/Designer conventions. A list of attribute
values should be separated by semi-colons.

The ability to modify the interface to edit the attributes is most powerful
when you're integrating Object/Designer with other tools or data. This makes
it easy to add support for things like database access. The developer would
no longer have to key in table names, because you could display a list of 
available tables.


Developing Add-On Products for Object/Designer
----------------------------------------------
If you intend to market an add-on product for Object/Designer that utilizes
ODUSER.DLL you should be sure that the user still has the ability to add 
their own routines.  The add-on should exist within its own DLLs. That way,
you can distribute the source for ODUSER.DLL. It would only need to 
communicate with your DLL and would be open to user modifications.


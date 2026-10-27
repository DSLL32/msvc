:userdoc.
.* Extended help for FormDemo
:h1 res=501.Keys Help
:p.
Keys used in form editing:
:p.
Tab
:p.
        Move forward one field
:p.
Shift&plus.Tab
:p.
        Move backward one field
.*
.* Strings forms
.*
:h1 res=1000.String Fields
:i1 id=strings.String Fields
:p.
This form contains a number of entry fields for character strings.
Although strings are the simplest type of field, string fields can
be formatted to accept numerals only, letters only, or have their
case translated.
:p.
This form contains field types of both FtString and FtCharArray.  The
difference between these two types lies in the types of field
data objects that they interface to.  The FtString type reads and
writes it's string data from/to objects of type bString.  The
FtCharArray type uses standard character arrays (char[]). 
:h2 res=1001.String Field
:i2 refid=strings.String Field
:p.
This field is a standard character field that gets its data from, and
updates to, an object of type bString.
:h2 res=1002.Numeric Only String
:i2 refid=strings.Numeric String
:p.
This field is a character field that accepts only numeric input
values.  This is done by specifying the "%9" format string.  It's data 
object type is bString.
"%9".
:h2 res=1003.Letter Only String
:i2 refid=strings.Letter String
:p.
This field is a character field that accepts only alpha input
values (A-Z or a-z).  The format string used for this field is "%A".
It's data object type is bString.
:h2 res=1004.To-Upper String
:i2 refid=strings.To-Upper String
:p.
This field accepts all characters but translates the entered lower
case characters into upper case.  This behavior is specified using
the "%!" format option.  The field type for this field
is FtCharArray meaning that its associated data object is a standard
character array. 
:h2 res=1005.To-Lower String
:i2 refid=strings.To-Lower String
:p.
This field accepts all characters but translates the entered upper
case characters into lower case.  The field type for this field
is FtCharArray meaning that it's associated data object is standard
character array. 
.*
.* Characters form
.*
:h1 res=1100.Character Fields
:i1 id=chars.Character Fields
:p.
This form contains [single] character (FtChar) and boolean (FtBool) 
field types.  All of these fields allow only a single letter and use 
a single 'char' for their associated data objects. 
:h2 res=1101.Character
:i2 refid=strings.Character
:p.
This field is a standard single character field.  
:h2 res=1102.Numeric Character
:i2 refid=strings.Numeric Character
:p.
This field is a single character field that accepts only numeric
characters.  It uses the "%9" format character and a standard 'char'
as its associated data object.  
:h2 res=1103.To-Upper Character
:i2 refid=strings.To-Upper Character
:p.
This field accepts all characters but translates the entered lower
case character into upper case.  This behavior is specified using
the "%!" format option.  The field type for this field FtChar
meaning that its associated data object is a standard 'char' type.
:h2 res=1104.Boolean Character
:i2 refid=strings.Boolean Character
:p.
Boolean field types are those that accept only two valid characters. 
Examples are entries such as "Y" or "N", "T" or "F", and "A" or "B".
By default the two characters "T" and "F" are accepted.  This can be
changed using the "%r" format option.  For example "%rYN" changes the
field to accept either "Y" or "N". 
.*
.* MLE form
.*
:h1 res=1200.Multi-Line Edit Fields
:i1 id=mle.Multi-Line Edit Fields
:p.
This form conains a multi-line edit field.  In general multi-line
field types are very similar to single line types.  Both use bString
types for data objects.  However, case mapping and character
filtering have not been implemented for multi-line strings.
.*
.* Buttons form
.*
:h1 res=1300.Button Fields
:i1 id=buttons.Button Fields
:p.
This form contains a number of button fields.  The ObjectPM Forms
Manager supports Radio Buttons, CheckBoxes, and Push Buttons as 
Field types.  In the case of Radio Buttons and Checkboxes, objects of
type 'short int' are used as associated data objects.
:h2 res=1301.Radio Buttons
:i2 refid=buttons.Radio Buttons
:p.
This Radio Button cluster is considered a field.  A short integer is
used as its data object which determines which button, if any, are
on.  When creating these fields, make sure the ID's of the buttons
are consecutive and that they have the style of 'auto' turned on. 
The window ID specified in the application code should be the ID of
the first button in the cluster.
:h2 res=1302.Check Boxes
:i2 refid=buttons.Check Boxes
:p.
A check box specifies On/Off or On/Off/Unknown (tri-state) choices. 
The ObjectPM Forms Manager supports these controls as fields where
the associated data object type is 'short int'.
:h2 res=1303.Push Buttons
:i2 refid=buttons.Push Buttons
:p.
Push Button fields are used for issuing commands just as normal Push
Button controls do.  These field types do not edit or modify any form
data.  Instead, a method of a derived wFormWindow or wPushButtonField
can be specified to handle the 'Click' event, in which, you add your
code to process the event.  In this demo, one Push Button field is
used to 'Undo' any changes made since the button page was selected,
A check box specifies On/Off or On/Off/Unknown (tri-state) choices. 
and to pop-up an additional form.
.*
.* More form
.*
:h1 res=1400.Popup Forms
:i1 id=more.Popup Forms
:p.
This form was created by the code processing the 'More' button for
the buttons page.  It is put here to demonstrate that forms can be
nested with the Forms Manager.  Also notice that a text window is
used to show error strings instead of a pop-up message box and that 
the 'Return' key now closes the form the way normal dialogs work.
:h2 res=1401.Name 
:i2 refid=more.Name
:p.
Enter you name or alias.  This field is required.
:h2 res=1402.Company 
:i2 refid=more.Company
:p.
Enter the name of place of Employment.
:h2 res=1403.Phone 
:i2 refid=more.Phone
:p.
Enter your phone number.
:h2 res=1404.OK 
:i2 refid=more.OK
:p.
Select this button to close the form.  When this button is pressed,
all data in the fields will be 'converted' and written to the field
data objects.  This conversion process will be stopped by a
validation error (such as not filling in the name field).
:h2 res=1405.Cancel 
:i2 refid=more.Cancel
:p.
Select this button to abort the form.  When this button is pressed,
all changes to the data fields will be lost.  
.*
.* List form
.*
:h1 res=1500.List Regions
:i1 id=list.List Regions
:p.
This form demonstrates the Forms Manager's list support.  The 'List
Region' is a list box and a set of zero or more edit fields.  The
list box supports column formatting so that data can be left, right,
or centered aligned in columns.  Each column can contain static text
or can be associated with an edit field.
:p.
When the user selects an item in the list box, the item text is split
up into column strings and copied to the appropriate edit fields. 
The use can then modify the item.  When the user tabs off of the last
edit field, the item string is re-assembled and passed to a user-
supplied method.  To add a row, the user should click the right mouse
button over the list box, or press 'Esc' when the list box has focus. 
This clears the list region so that a new data row can be entered. 
To delete an item, press the 'Del' key.
:p.
All list region actions (Insert, Modify, and Delete), are passed to a
user-supplied method so that the application can take the appropriate
action.  This demo program supplies such a method for maintaining the
'Total' field.
:h2 res=1501.Listbox 
:i2 refid=list.Listbox
:p.
This is the list region's listbox.  
:h2 res=1502.Part Number 
:i2 refid=list.Part Number
:p.
Enter a numeric part number.  This field is required.
:h2 res=1503.Description 
:i2 refid=list.Description
:p.
Enter a description of the item.  This field is required.
:h2 res=1504.Amount
:i2 refid=list.Amount
:p.
Enter the dollar amount of the item.  This field is required.

  




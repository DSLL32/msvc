                   TEGL WINDOWS TOOKIT Release 3.0
             Copyright 1990-1993 TEGL SYSTEMS CORPORATION
                         All rights reserved.

                       TEGL SYSTEMS CORPORATION
                     780 - 789 West Pender Street
                       Vancouver, B.C., V6C 1H2
                                CANADA
              Phone: (604) 669-2577  FAX: (604) 688-9530

Welcome to the working model of the TEGL WINDOWS TOOLKIT. The
working model is not crippled, it contains all the functionality of
the full product with a couple of exceptions.

   * NO LIBRARY SOURCE CODE
   * Copyright Button Appears on all GUI programs
   * Copyright notice is displayed at the termination of any program.
   * Programs run for more than 20 minutes will display a copyright notice.

In creating this working model we have tried to give you a hands-on
approach, rather than just handing out an executable program to run.

To install you expand the TEGLDEMO.ZIP file in a directory (do not
expand it to the root directory). You can use PKUNZIP 2.0 or later or
UNZIP v5.0 or later. If you are using PKUNZIP be sure and use the
-d option so it will create the required directories.

Assuming you have used TEGLDEMO as the target directory you should
end up with this:

\TEGLDEMO
       cc.bat          batch file to invoke bcc (you will need to
                       edit this file for your own directory needs).
       any.prj         an example project file with the correct
                       compiler settings
       order.txt       order form
       toolkit.txt     information on TEGL WINDOWS features
       todemo.c        the demo program specified in the project
                         (it comes from \demos\windows ).
       \H              include files
       \LIB            the teglsys.lib is here
       \DEMOS
           \EX_TGI     examples from our graphics manual
           \EX_VIRT    examples from our virtual memory manual
           \EX_GUI     examples from our GUI manual
           \MENUS      menu demos (see menudemo.c)
           \WINDOWS    tw window demos (see twdemo.c)
           \OOPS       object demos (see todemo.c)

IMPORTANT!!!!

The first thing you must do is switch to the \H directory and run a
batch file called 'teglsys.bat'. This batch file build the teglsys.h
header file (just all the header files concantenated together in the
correct order). The teglsys.h file is included in many of the example
files

If you work with the command line compiler you should look at the
included 'cc.bat' file. This shows the correct command line settings
for compiling any of the demo programs.

The project file 'any.prj' can be loaded into the IDE and edited to
reflect how things are installed on your system. The make file
'any.mak' was generated using the "prj2mak" utility provided with
Borland C/C++.


USING THE IDE

If you use the IDE you will need to create a project file (an
example is included). This needs to specify the TEGLSYS.LIB as part
of the project and one of the example programs.

The IDE options must have the following settings:

   * LARGE MEMORY MODEL
   * word alignment OFF
   * frequent errors - Parameter 'ident' is never used - Turn this OFF
   * DO NOT LINK IN THE GRAPH.LIB, TEGL WINDOWS includes a
     complete graphics package that is BGI function compatible.

If the compiler complains of incorrect parameters then the memory
model has not been set to LARGE.

If you get a bunch of linker errors when compiling then probably the
TEGLSYS.LIB was not specified as part of the link or included in the
project file.

-----------------------------------------------------------------------

If you have any additional questions about the TEGL WINDOWS TOOLKIT
please call us. Our office hours are 9:30 - 4:30 Monday to Friday,
Pacific Standard Time.



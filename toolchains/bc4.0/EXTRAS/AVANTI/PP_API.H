/*
***************************************************************
***************************************************************
******                                                   ******
******  Copyright 1993 by Avanti Software Incorporated.  ******
******  Patents pending.                                 ******
******                                                   ******
***************************************************************
***************************************************************
*/

/*
**  File Name:  pp_api.h
**
**  Date:       08/20/93 12:00 pm
**
**  Function:   Include file to support PinPoint debugging from C/C++.
**
*/

#ifndef _PP_API_H
#define _PP_API_H                  /* make sure it only comes in once! */

/* this is for the DLL to create export names with proper call stacks  */
#define EXPORT_CV  _far _export __cdecl  /* for C (variable arguments) */
#define EXPORT_CPP _export               /* for C++                    */
#define EXPORT_P   _far _pascal _export  /* for all languages          */

#ifndef WINAPI              /* see if WINDOWS.H has already been included */
#include <windows.h>
#endif

/* 
** The symbols PINPOINT_ON and PINPOINT are NOT used internally by PinPoint;
**  feel free to modify them...
*/
#ifdef PINPOINT_ON
#define PINPOINT(a) a       /* enable  PinPoint macro */
#else
#define PINPOINT(a)         /* disable PinPoint macro */
#endif

#define _PINPOINT(a)        /* disable PinPoint macro with leading _ */

/* new defines for ppMessageBoxPrintf... */
enum {
    ppMB_INFO = 0,
    ppMB_ERROR
};

/* allow both C and C++ compilers to use this API... */
#ifdef __cplusplus
extern "C" {
#endif  

/*
********************************************
********************************************
****                                    ****
****  Define API for Non-C++ Compilers  ****
****                                    ****
********************************************
********************************************
*/       
void EXPORT_P  ppBeginProgram(void);
void EXPORT_P  ppEndProgram(void); 

void EXPORT_P  ppBeginFunction(LPSTR tagName, LPSTR functionName, LPSTR str);
void EXPORT_CV ppBeginFunctionf(LPSTR tagName, LPSTR functionName, LPSTR fmt, ...);
void EXPORT_P  ppEndFunction(LPSTR str);

void EXPORT_P  ppBeginSub(LPSTR tagName, LPSTR functionName, LPSTR str);
void EXPORT_P  ppEndSub(void);

void EXPORT_P  ppAssert(LPSTR tagName, BOOL test, short lineNo, 
    LPSTR fileName, LPSTR str);
void EXPORT_CV ppAssertf(LPSTR tagName, BOOL test, short lineNo, 
    LPSTR fileName, LPSTR fmt, ...);
void EXPORT_P  ppPrint(LPSTR tagName, LPSTR str);
void EXPORT_CV ppPrintf(LPSTR tagName, LPSTR fmt, ...);

/* utility routines */
LPSTR EXPORT_P ppConvert2Hex(LPSTR ptr, unsigned short count);
void EXPORT_P  ppFopen(LPSTR logFileName, BOOL bBuffered);
void EXPORT_P  ppFclose(void);
void EXPORT_P  ppMessageBoxPrint(short type, LPSTR title, LPSTR str);
void EXPORT_CV ppMessageBoxPrintf(short type, LPSTR title, LPSTR fmt, ...);

/* secondary monitor routines */
void EXPORT_P  ppMonoEnable(BOOL state);
void EXPORT_P  ppMonoPrint(LPSTR message);
void EXPORT_CV ppMonoPrintf(LPSTR format, ...);
BOOL EXPORT_P  ppMonoQuery(void);

/* for C programs to return function values */
void EXPORT_P ppReturnVoid(void);

void EXPORT_P ppReturnINT8(char var);
void EXPORT_P ppReturnINT16(short var);
void EXPORT_P ppReturnINT32(long var);
void EXPORT_P ppReturnBool(BOOL a);
void EXPORT_P ppReturnSuccessFail(short a);

void EXPORT_P ppReturnUINT8(unsigned char var);
void EXPORT_P ppReturnUINT16(unsigned short var);
void EXPORT_P ppReturnUINT32(unsigned long var);

void EXPORT_P ppReturnLPSTR(char FAR * var);
void EXPORT_P ppReturnLPCSTR(const char FAR * var);

void EXPORT_P ppReturnFloat(float var);
void EXPORT_P ppReturnDouble(double var);
void EXPORT_P ppReturnLongDouble(long double var);

void EXPORT_P ppReturnNearPtr(void NEAR * var);
void EXPORT_P ppReturnFarPtr(void FAR * var);

#ifdef __cplusplus
}
#endif  
  
/*
******************************************
******************************************
****                                  ****
****   Define API for C++ Compilers   ****
****                                  ****
******************************************
******************************************
*/       
#ifdef __cplusplus      /* turned on by C++ compilers... */                        

class EXPORT_CPP ppFunction {
  public:
    ppFunction(LPSTR tagName, LPSTR functionName, LPSTR fmt = NULL, ...);
    ~ppFunction();
}; // end of class ppFunction

class EXPORT_CPP ppMethod {
  public:
    ppMethod(LPSTR tagName, LPSTR className, LPSTR methodName, 
        LPSTR fmt = NULL, ...);
    ~ppMethod();
}; // end of class ppMethod

void EXPORT_CPP ppReturn(void);

void EXPORT_CPP ppReturn(char a);
void EXPORT_CPP ppReturn(short a);
void EXPORT_CPP ppReturn(int a);
void EXPORT_CPP ppReturn(long a);

void EXPORT_CPP ppReturn(unsigned char a);
void EXPORT_CPP ppReturn(unsigned short a);
void EXPORT_CPP ppReturn(unsigned int a);
void EXPORT_CPP ppReturn(unsigned long a);

void EXPORT_CPP ppReturn(LPSTR a);
void EXPORT_CPP ppReturn(LPCSTR a);

void EXPORT_CPP ppReturn(float a);
void EXPORT_CPP ppReturn(double a);
void EXPORT_CPP ppReturn(long double a);

void EXPORT_CPP ppReturn(void FAR *a);
void EXPORT_CPP ppReturn(void NEAR *a);

#endif /* __cplusplus */

#endif /* _PP_API_H */
                        
/* end of file pp_api.h */

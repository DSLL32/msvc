#if defined(__SW_BD)
/*

Default DLL initialization routine for OS/2 1.x.
Used if user does not write his own

*/

extern		int	main();

unsigned	__dll_initialize() {
/**********************************/

    return( main() );
}
#endif

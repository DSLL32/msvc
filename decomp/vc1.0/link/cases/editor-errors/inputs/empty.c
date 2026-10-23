/* Ordinary fixture-only DLL entry; no exported definitions or CRT imports. */
int __stdcall Empty(void *instance,unsigned reason,void *reserved)
{
    return 1;
}

int shared_value=7;
int *shared_address=&shared_value;
__declspec(dllexport) int imported_value(void)
{
    return *shared_address;
}
int __stdcall DllMain(void *module,unsigned long reason,void *reserved)
{
    return 1;
}

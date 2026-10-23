__declspec(dllimport) int imported_value(void);
int local_value=13;
int *local_address=&local_value;
int main(void)
{
    return imported_value()+*local_address;
}

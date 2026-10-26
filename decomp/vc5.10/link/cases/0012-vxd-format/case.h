/* Genuine x86 input for the VxD/LE format case. Compile only with the
 * original VC5 toolchain. This case checks image serialization, not OS loading. */
#pragma data_seg("_LDATA")
int device_state=7;
int *device_state_alias=&device_state;
char module_label[]="VxD format case";
#pragma bss_seg("_LDATA")
int zero_state[16];
#pragma code_seg("_LTEXT")
static int __cdecl ReadState(int index)
{
    zero_state[index&15]=*device_state_alias;
    return zero_state[index&15]+module_label[0];
}
int __cdecl DeviceControl(int request)
{
    return ReadState(request)+device_state;
}

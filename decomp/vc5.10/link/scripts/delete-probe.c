/* Explicit original-only canonical CRT allocation/delete probe, not target code. */
#include <windows.h>
#include <stdlib.h>
#include <errno.h>
#include <recon.h>

void *__cdecl crt_operator_new(unsigned);
void __cdecl crt_operator_delete(void *);

static HANDLE output;
static int put(const void *data,unsigned count)
{
    DWORD written;
    return WriteFile(output,data,count,&written,NULL) && written==count;
}

int main(void)
{
    static const unsigned sizes[3]={12288,4000,4096};
    BYTE *owned[3];
    DWORD header[2]={0x31504452,1};
    DWORD record[9],slot_header[2];
    unsigned scenario,slot,index,remaining;
    int target;
    output=CreateFileA("delete-probe.bin",GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE || !put(header,sizeof(header)))return 2;
    for(scenario=0;scenario<4;++scenario) {
        for(slot=0;slot<3;++slot) {
            owned[slot]=(BYTE *)crt_operator_new(sizes[slot]);
            if(!owned[slot])return 3;
            for(index=0;index<sizes[slot];++index)owned[slot][index]=(BYTE)(index*13+slot*17+scenario);
        }
        for(target=2;target>=0;--target) {
            record[0]=scenario;record[1]=(unsigned)target;record[2]=sizes[target];
            errno=(scenario&1)?111:0;
            SetLastError((scenario&2)?0x12345678:0);
            record[3]=errno;record[4]=GetLastError();
            crt_operator_delete(owned[target]);
            record[5]=errno;record[6]=GetLastError();
            owned[target]=NULL;
            record[7]=(1u<<target)-1;record[8]=(unsigned)target;
            if(!put(record,sizeof(record)))return 4;
            for(remaining=0;remaining<(unsigned)target;++remaining) {
                for(index=0;index<sizes[remaining];++index)
                    if(owned[remaining][index]!=(BYTE)(index*13+remaining*17+scenario))return 5;
                slot_header[0]=remaining;slot_header[1]=sizes[remaining];
                if(!put(slot_header,sizeof(slot_header)) || !put(owned[remaining],sizes[remaining]))return 4;
            }
        }
    }
    return CloseHandle(output)?0:6;
}

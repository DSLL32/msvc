/* Original-only debugger capture. Never linked into a reconstruction or fixture. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static HANDLE child;
static FILE *events;
static DWORD ids[128];
static HANDLE handles[128];
static unsigned count;
static DWORD entry[2048];
static BYTE entry_byte[2048];
static unsigned entries;
static DWORD mainargs_slots[5];
static DWORD message_slots[4], message_number, observed_environment;
static DWORD extended_step_thread;

static int read_memory(DWORD address, void *buffer, DWORD size)
{
    DWORD got;
    return ReadProcessMemory(child, (void *)address, buffer, size, &got) && got == size;
}
static void save_file(HANDLE source, const char *name)
{
    HANDLE destination;
    BYTE bytes[16384];
    DWORD got, wrote, size, high, offset = 0, count;
    OVERLAPPED position;
    size = GetFileSize(source, &high);
    if (high || !size || size == 0xffffffffUL) ExitProcess(124);
    destination = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (destination == INVALID_HANDLE_VALUE) ExitProcess(125);
    while (offset < size) {
        count = size - offset;
        if (count > sizeof(bytes)) count = sizeof(bytes);
        memset(&position, 0, sizeof(position));
        position.Offset = offset;
        got = 0;
        if (!ReadFile(source, bytes, count, &got, &position) || got != count)
            ExitProcess(126);
        if (!WriteFile(destination, bytes, got, &wrote, NULL) || wrote != got)
            ExitProcess(127);
        offset += got;
    }
    if (offset != size || GetFileSize(source, &high) != size || high)
        ExitProcess(124);
    CloseHandle(destination);
    fprintf(events, "file-identity output=%s actual-bytes=%lu explicit-offset=1\n", name, offset);
}
static void loaded(DWORD base, HANDLE file)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_EXPORT_DIRECTORY exports;
    char name[256];
    if (!read_memory(base, &dos, sizeof(dos)) ||
        !read_memory(base+dos.e_lfanew, &nt, sizeof(nt))) ExitProcess(128);
    strcpy(name, "<no-exports>");
    if (nt.OptionalHeader.DataDirectory[0].VirtualAddress) {
        if (!read_memory(base+nt.OptionalHeader.DataDirectory[0].VirtualAddress,
                         &exports, sizeof(exports)) ||
            !read_memory(base+exports.Name, name, sizeof(name))) ExitProcess(129);
        name[sizeof(name)-1] = 0;
    }
    fprintf(events, "module %08lx %08lx %s\n", base, nt.OptionalHeader.ImageBase, name);
    if (!_stricmp(name, "msvcrt.dll")) {
        save_file(file, "loaded-msvcrt.bin");
        fprintf(events, "provider-file loaded-msvcrt.bin\n");
    }
}
static void save_vector(DWORD address, const char *name)
{
    FILE *file = fopen(name, "wb");
    DWORD item, count = 0;
    char value;
    if (!file) ExitProcess(135);
    for (;;) {
        if (++count > 4096 || !read_memory(address, &item, 4)) ExitProcess(136);
        address += 4;
        if (!item) break;
        do {
            if (!read_memory(item++, &value, 1)) ExitProcess(137);
            fputc(value, file);
        } while (value);
    }
    fputc(0, file); fclose(file);
}
static void save_string(DWORD address, const char *name)
{
    FILE *file=fopen(name,"wb");
    DWORD i; char value;
    if (!file) ExitProcess(156);
    for (i=0;i<4096;++i) {
        if (!read_memory(address+i,&value,1)) ExitProcess(157);
        fputc(value,file); if (!value) break;
    }
    fclose(file); if (i==4096) ExitProcess(158);
}
static void save_contextual_data(void)
{
    FILE *plan=fopen("data-plan.txt","r"), *output;
    DWORD address,size; BYTE bytes[4096];
    if (!plan) ExitProcess(159);
    output=fopen("data-state.bin","wb");if (!output) ExitProcess(160);
    while (fscanf(plan,"%lx %lu",&address,&size)==2) {
        if (!size || size>sizeof(bytes) || !read_memory(address,bytes,size)) ExitProcess(161);
        if (fwrite(bytes,1,size,output)!=size) ExitProcess(162);
    }
    fclose(plan);fclose(output);
}
static void observe(CONTEXT *context, DWORD address)
{
    DWORD stack[16], value, i, pointer;
    static DWORD globals[] = {0x499708,0x4996d4,0x49975c,0x4980d4,0x499718,0x499694,0x4980d0,0x4980cc,0x48d064,0x7803fa90,0x7803fa88,0x7803fa94,0x7803f198,0x7803f928,0x7803fde8,0x499ac8,0x499adc,0x497e1c,0x497e08,0x497cac,0x49e2f4,0x4980d8,0x497fb0,0x490b7c,0x4972f8,0x497df8,0x491050,0x497f6c,0x497de8,0x48fb68,0x491040,0x491044,0x491048,0x49104c,0x491074,0x495f34,0x49745c,0x4973e8,0x490b84,0x4973d4,0x4973f0,0x4972fc,0x49d634,0x49739c,0x4973b0,0x4973a0,0x497390,0x497380,0x497c6c,0x4973c0,0x495f1c};
    fprintf(events,"observe %08lx eax=%08lx ecx=%08lx edx=%08lx esp=%08lx\n",
        address,context->Eax,context->Ecx,context->Edx,context->Esp);
    if (address==0x453df4 || address==0x453e02)
        fprintf(events,"directive-call-abi at=%08lx ecx=%08lx ebx=%08lx ebp=%08lx esi=%08lx edi=%08lx esp=%08lx\n",address,context->Ecx,context->Ebx,context->Ebp,context->Esi,context->Edi,context->Esp);
    if (read_memory(context->Esp,stack,sizeof(stack))) {
        fprintf(events,"observe-stack");
        for (i=0;i<16;++i) fprintf(events," %08lx",stack[i]);
        fprintf(events,"\n");
    }
    if (read_memory(context->Eax,&value,4)) fprintf(events,"eax-word %08lx %08lx\n",context->Eax,value);
    for (i=0;i<sizeof(globals)/sizeof(globals[0]);++i)
        if (read_memory(globals[i],&value,4)) fprintf(events,"global %08lx %08lx\n",globals[i],value);
    fprintf(events,"x87-control %08lx\n", context->FloatSave.ControlWord);
    if (address == 0x48b78c || address == 0x48b7ba || address == 0x48b3d0 || address == 0x48b6ce || address == 0x48b6ae) {
        DWORD initialized, cache[14];
        unsigned char buffer[384];
        FILE *output;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49e650,&initialized,4) || !read_memory(0x49e1d0,cache,56) || !read_memory(0x49e4d0,buffer,384)) ExitProcess(168);
        if (address == 0x48b78c)
            fprintf(events,"message-load id=%lu buffer=%08lx capacity=%lu allow=%lu initialized=%lu\n",context->Ecx,context->Edx,stack[1],stack[2],initialized);
        else if (address == 0x48b7ba)
            fprintf(events,"message-unescape buffer=%08lx initialized=%lu\n",context->Ecx,initialized);
        if (address == 0x48b6ce || address == 0x48b6ae) fprintf(events,"cached-message id=%lu\n",context->Ecx);
        if (address == 0x48b6ce || address == 0x48b6ae) {
            char name[64];
            for(i=0;i<6;++i) {
                sprintf(name,"cache-message-%lu.bin",cache[2*i]);
                if (!cache[2*i+1]) ExitProcess(172);
                save_string(cache[2*i+1],name);
            }
        }
        if (address == 0x48b3d0) {
            DWORD arguments[2], unexpected;
            char name[64];
            if (!read_memory(stack[4],arguments,8) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(170);
            fprintf(events,"diagnostic-emit category=%lu message=%lu format=%08lx arguments=%08lx argument=%08lx unexpected=%08lx\n",stack[1],stack[2],stack[3],stack[4],arguments[0],unexpected);
            save_string(stack[3],"diagnostic-format.bin");save_string(arguments[0],"fatal-argument.bin");save_string(arguments[1],"fatal-phase.bin");
            for(i=0;i<6;++i) {
                sprintf(name,"cache-message-%lu.bin",cache[2*i]);
                if (!cache[2*i+1]) ExitProcess(171);
                save_string(cache[2*i+1],name);
            }
        }
        output=fopen("message-cache.bin","wb");if(!output)ExitProcess(169);
        fwrite(&initialized,4,1,output);fwrite(cache,56,1,output);fwrite(buffer,384,1,output);fclose(output);
    }
    if (address == 0x48b708) {
        DWORD current, unexpected;
        if (!read_memory(0x49968c,&current,4) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(167);
        fprintf(events,"message-initialize current=%08lx unexpected=%08lx\n",current,unexpected);
        save_string(current,"current-option.bin");
        save_contextual_data();
    }
    if (address == 0x48b6e8) {
        DWORD current, unexpected;
        if (!read_memory(0x49968c,&current,4) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(166);
        fprintf(events,"message-lookup id=%lu allow=%lu current=%08lx unexpected=%08lx\n",context->Ecx,context->Edx,current,unexpected);
        save_string(current,"current-option.bin");
        save_contextual_data();
    }
    if (address == 0x48b566 || address == 0x48b57e) {
        DWORD arguments[2];
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(173);
        arguments[0]=stack[3];arguments[1]=stack[4];
        if (address == 0x48b57e && !read_memory(stack[3],arguments,8)) ExitProcess(174);
        fprintf(events,"format-prefix buffer=%08lx offset=%lu format=%08lx character=%lu number=%lu\n",stack[1],stack[1]-context->Esp,stack[2],arguments[0],arguments[1]);
        save_string(stack[2],"format-pattern.bin");save_string(stack[1]-11,"format-prefix.bin");
    }
    if (address == 0x4466a6) {
        char prefix[13];FILE *output;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(context->Edx-13,prefix,13)) ExitProcess(175);
        fprintf(events,"format-integer value=%lu buffer=%08lx base=%lu\n",context->Ecx,context->Edx,stack[1]);
        output=fopen("integer-prefix.bin","wb");if(!output)ExitProcess(176);fwrite(prefix,1,13,output);fclose(output);
    }
    if (address == 0x48b367) {
        DWORD current, unexpected;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49968c,&current,4) || !read_memory(0x49e284,&unexpected,4)) ExitProcess(165);
        fprintf(events,"fatal-option message=%lu argument=%08lx phase=%08lx current=%08lx unexpected=%08lx\n",stack[1],stack[2],stack[3],current,unexpected);
        save_string(stack[2],"fatal-argument.bin");
        save_string(stack[3],"fatal-phase.bin");
        save_contextual_data();
    }
    if (address == 0x452cfe) {
        DWORD current;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49968c,&current,4)) ExitProcess(164);
        fprintf(events,"apply-state option=%08lx value=%08lx mode=%lu current=%08lx\n",context->Ecx,context->Edx,stack[1],current);
        save_string(context->Edx,"option-value.bin");
        save_contextual_data();
    }
    if (address == 0x48b7f8) {
        DWORD current;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x49968c,&current,4)) ExitProcess(163);
        fprintf(events,"wildcard-state pattern=%08lx input=%08lx kind=%lu current=%08lx\n",context->Ecx,context->Edx,stack[1],current);
        save_string(context->Ecx,"wildcard-pattern.bin");
        save_string(context->Edx,"wildcard-input.bin");
        save_contextual_data();
    }
    if (address == 0x452bd9) {
        char value;
        FILE *file;
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(152);
        fprintf(events,"matcher-state table=%08lx input=%08lx callback=%08lx mode=%lu\n",context->Ecx,context->Edx,stack[1],stack[2]);
        file=fopen("matcher-input.bin","wb");if (!file) ExitProcess(153);
        for(i=0;i<4096;++i) {
            if (!read_memory(context->Edx+i,&value,1)) ExitProcess(154);
            fputc(value,file);if (!value)break;
        }
        fclose(file);if(i==4096)ExitProcess(155);
    }
    if (address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x42032b || address == 0x4283b4 || address == 0x4532d0 || address == 0x420350 || address == 0x420ebb || address == 0x420e74 || address == 0x420250 || address == 0x4206b7 || address == 0x420394 || address == 0x4532ae || address == 0x453e92 || address == 0x41f42e || address == 0x40d5a8 || address == 0x41f3f7 || address == 0x45312c || address == 0x4530fa || address == 0x4530a7 || address == 0x46575e || address == 0x453087 || address == 0x453036 || address == 0x401000 || address == 0x40d534 || address == 0x40d507 || address == 0x45302d || address == 0x453012 || address == 0x452f95 || address == 0x452f4b || address == 0x4200d1 || address == 0x452ea4 || address == 0x452335 || address == 0x452e9b || address == 0x452e27 || address == 0x452a22 || address == 0x452a98 || address == 0x452acb || address == 0x452bb7 || address == 0x452bd9 || address == 0x48b7f8 || address == 0x452cfe || address == 0x48b367 || address == 0x48b6e8 || address == 0x48b708 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD argc, argv;
        if (!read_memory(0x49972c,&argc,4) || !read_memory(0x499728,&argv,4)) ExitProcess(151);
        fprintf(events,"command-state argc=%lu argv=%08lx input=%08lx\n",argc,argv,context->Ecx);
        save_vector(argv,"command-argv.bin");
    }
    if (address == 0x452a18) {
        if (!read_memory(context->Esp,&value,4)) ExitProcess(150);
        fprintf(events,"onexit-input callback=%08lx\n",value);
    }
    if (address == 0x452a1e)
        fprintf(events,"onexit-output result=%08lx\n",context->Eax);
    if ((address == 0x4529f1 || address == 0x452a06)) {
        if (!read_memory(context->Esp+4,&value,4)) ExitProcess(149);
        fprintf(events,"exit-callback %08lx\n",value);
    }
    if (address == 0x452e9b || address == 0x452335 || address == 0x452ea4 || address == 0x4200d1 || address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000 || address == 0x453036 || address == 0x453087 || address == 0x46575e || address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD current, operand, argv, alignment=context->Ecx;
        if (address!=0x452e9b && !read_memory(0x499a04,&alignment,4)) ExitProcess(183);
        if (!read_memory(0x49968c,&current,4) ||
            !read_memory(0x4980c8,&operand,4) ||
            !read_memory(0x499728,&argv,4)) ExitProcess(182);
        fprintf(events,"alignment-option input=%08lx current=%08lx operand=%08lx argv=%08lx\n",
                alignment,current,operand,argv);
        save_string(current,"current-option.bin");
        save_string(operand,"option-operand.bin");
        save_contextual_data();
    }
    if (address == 0x452ea4 || address == 0x4200d1 || address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000 || address == 0x453036 || address == 0x453087 || address == 0x46575e || address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD input, globals, iob, active;
        BYTE data[64]; FILE *output;
        if (!read_memory(0x499698,&input,4) ||
            !read_memory(0x497fd4,&globals,4) ||
            !read_memory(0x48d08c,&iob,4) ||
            !read_memory(input,data,32) || !read_memory(globals,data+32,32)) ExitProcess(185);
        fprintf(events,"header-streams input=%08lx globals=%08lx iob=%08lx\n",input,globals,iob);
        if (address == 0x4200d1 || address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000 || address == 0x453036 || address == 0x453087 || address == 0x46575e || address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8) {
            if (!read_memory(0x490b80,&active,4)) ExitProcess(187);
            fprintf(events,"active-il-stream %08lx\n",active);
        }
        output=fopen("header-streams.bin","wb");
        if (!output || fwrite(data,1,64,output)!=64 || fclose(output)) ExitProcess(186);
        if (address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000 || address == 0x453036 || address == 0x453087 || address == 0x46575e || address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8) {
            DWORD words[8], valid;
            BYTE buffer[4096];
            memcpy(words,data+32,32);
            if (!words[2] || words[0]<words[2] || words[0]-words[2]>4096 ||
                words[1]>4096 || (valid=words[1]+words[0]-words[2])>4096 ||
                !read_memory(words[2],buffer,valid)) ExitProcess(188);
            output=fopen("il-buffer.bin","wb");
            if (!output || fwrite(buffer,1,valid,output)!=valid || fclose(output)) ExitProcess(189);
            fprintf(events,"il-buffer valid=%lu pointer-offset=%lu\n",valid,words[0]-words[2]);
            if(address==0x452f95)fprintf(events,"il-word-result %04lx\n",context->Eax&0xffff);
        }

    }
    if (address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000 || address == 0x453036 || address == 0x453087 || address == 0x46575e || address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8) {
        BYTE buckets[4096]; FILE *output;
        if (!read_memory(0x48fb6c,buckets,sizeof(buckets))) ExitProcess(190);
        output=fopen("symbol-hash.bin","wb");
        if (!output || fwrite(buckets,1,sizeof(buckets),output)!=sizeof(buckets) || fclose(output)) ExitProcess(191);
    }
    if (address == 0x40d507)
        fprintf(events,"allocate-node kind=%d pool=%ld\n",(signed char)context->Ecx,(long)context->Edx);
    if (address == 0x401000)
        fprintf(events,"pool-allocate pool=%ld bytes=%lu caller-kind=%d caller-pool=%ld\n",(long)context->Ecx,context->Edx,(signed char)context->Ebx,(long)context->Esi);
    if (address == 0x40d534)
        fprintf(events,"node-size kind=%ld caller-kind=%d pool=%ld\n",(long)context->Ecx,(signed char)context->Ebx,(long)context->Esi);
    if (address == 0x453036 || address == 0x453087 || address == 0x46575e || address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD pools[12], free_blocks, types;
        BYTE block[4096]; FILE *output;
        if (!read_memory(0x48e020,pools,sizeof(pools)) ||
            !read_memory(0x48e000,&free_blocks,4) ||
            !read_memory(0x497fd8,&types,4) || !pools[0] ||
            !read_memory(pools[0],block,sizeof(block))) ExitProcess(192);
        output=fopen("pool-descriptors.bin","wb");
        if (!output || fwrite(pools,1,sizeof(pools),output)!=sizeof(pools) || fclose(output)) ExitProcess(193);
        output=fopen("pool-block.bin","wb");
        if (!output || fwrite(block,1,sizeof(block),output)!=sizeof(block) || fclose(output)) ExitProcess(194);
        fprintf(events,"pool-result free=%08lx types=%08lx head=%08lx cursor=%08lx remaining=%ld\n",
                free_blocks,types,pools[0],pools[1],(long)pools[2]);
        if(address==0x453b0c || address==0x453c5a || address==0x453d75 || address==0x41eb8a || address==0x453dc6 || address==0x41eb96 || address==0x41ebca || address==0x4526ed || address==0x41ebe3 || address==0x453de7 || address==0x453e02 || address==0x453eab || address==0x41ec20 || address==0x41ec59 || address==0x4522ec || address==0x41ef35||address==0x41ef83||address==0x41f085||address==0x41f0c4) {
            if(!pools[3] || !read_memory(pools[3],block,sizeof(block)))ExitProcess(268);
            output=fopen("directive-pool.bin","wb");
            if(!output || fwrite(block,1,sizeof(block),output)!=sizeof(block) || fclose(output))ExitProcess(269);
            fprintf(events,"directive-pool head=%08lx cursor=%08lx remaining=%ld initialized=4096\n",pools[3],pools[4],(long)pools[5]);
        }
    }
    if (address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD pool[3], output_file, iob, writer, caller; BYTE data[4096]; FILE *output;
        if(!read_memory(0x48e074,pool,12)||!pool[0]||!read_memory(pool[0],data,4096))ExitProcess(270);
        output=fopen("writer-pool.bin","wb");if(!output||fwrite(data,1,4096,output)!=4096||fclose(output))ExitProcess(271);
        if(!read_memory(0x497e08,&output_file,4)||!read_memory(0x48d08c,&iob,4)||!read_memory(output_file,data,32))ExitProcess(272);
        output=fopen("output-file.bin","wb");if(!output||fwrite(data,1,32,output)!=32||fclose(output))ExitProcess(273);
        writer=context->Ecx;caller=context->Esi;
        if(address==0x453de7)writer=pool[0]+8;
        if(address==0x41ec20){writer=pool[0]+8;if(!read_memory(context->Esp+32,&caller,4))ExitProcess(279);}
        if(address==0x41ec59){writer=pool[0]+8;if(!read_memory(context->Esp+44,&caller,4))ExitProcess(280);}
        if(address==0x4522ec){writer=pool[0]+8;if(!read_memory(context->Esp+76,&caller,4))ExitProcess(281);}
        if(address==0x41ef35){writer=pool[0]+8;if(!read_memory(context->Esp+92,&caller,4))ExitProcess(282);}
        if(address==0x41ef83){writer=pool[0]+8;if(!read_memory(context->Esp+68,&caller,4))ExitProcess(283);}
        if(address==0x41f085||address==0x41f0c4){writer=pool[0]+8;if(!read_memory(context->Esp+92,&caller,4))ExitProcess(284);}
        if(address==0x453eab){writer=pool[0]+8;if(!read_memory(context->Esp+16,&caller,4))ExitProcess(278);}
        if(address==0x453e02){writer=pool[0]+8;if(!read_memory(context->Esp+4,&caller,4))ExitProcess(277);}
        if(address==0x453dc6 || address==0x41eb96 || address==0x41ebca || address==0x4526ed || address==0x41ebe3){writer=pool[0]+8;if(!read_memory(context->Esp+(address==0x41ebca?20:address==0x41ebe3?24:4),&caller,4))ExitProcess(274);if(address==0x453dc6)fprintf(events,"coff-initialized malloc-result=%08lx\n",context->Eax);}
        fprintf(events,"object-writer-input writer=%08lx caller=%08lx file=%08lx iob=%08lx\n",writer,caller,output_file,iob);
        if(address==0x4526ed)fprintf(events,"coff-source-input name=%08lx\n",context->Ecx);
        if(address==0x41ebe3)fprintf(events,"coff-symbol-input record=%08lx\n",context->Ecx);
        if(address==0x41ebca)fprintf(events,"coff-chunk-input buffered=%ld count-only=%ld list=%08lx\n",(long)context->Ebx,(long)context->Edi,context->Esi);
        if(address==0x41eb96)fprintf(events,"coff-list-input kind=%ld context=%08lx\n",(long)context->Ecx,context->Edx);
        if(address==0x41eb8a)fprintf(events,"coff-allocate-input bytes=%lu capacity=%lu\n",context->Ecx,context->Edi&0xffff);
    }
    if (address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD pointer, total, next, entries[3]; FILE *output;
        if(!read_memory(0x497d48,&pointer,4)||!read_memory(0x497d4c,&total,4)||
           !read_memory(0x497d54,&next,4)||total!=3||next!=3||!read_memory(pointer,entries,12))ExitProcess(275);
        output=fopen("coff-symbol-entries.bin","wb");
        if(!output||fwrite(entries,1,12,output)!=12||fclose(output))ExitProcess(276);
        fprintf(events,"coff-symbols-complete initialized-entries=3 bytes=12 remainder=UNMODELED\n");
    }
    if (address == 0x453087) {
        BYTE reals[36]; DWORD value; FILE *output;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(stack[1],&value,4) ||
            !read_memory(0x48f880,reals,12) || !read_memory(0x48fb08,reals+12,12) ||
            !read_memory(0x491058,reals+24,12)) ExitProcess(195);
        output=fopen("real-records.bin","wb");
        if (!output || fwrite(reals,1,sizeof(reals),output)!=sizeof(reals) || fclose(output)) ExitProcess(196);
        fprintf(events,"real-from-int input=%08lx output=%08lx value=%ld\n",stack[1],stack[2],(long)value);
    }
    if (address == 0x46575e) {
        BYTE destination[12]; FILE *output;
        if (!read_memory(context->Esi,destination,sizeof(destination))) ExitProcess(197);
        output=fopen("extended-destination.bin","wb");
        if (!output || fwrite(destination,1,sizeof(destination),output)!=sizeof(destination) || fclose(output)) ExitProcess(198);
        output=fopen("x87-state.bin","wb");
        if (!output || fwrite(&context->FloatSave,1,sizeof(context->FloatSave),output)!=sizeof(context->FloatSave) || fclose(output)) ExitProcess(199);
        fprintf(events,"extended-store destination=%08lx control=%04lx status=%04lx tag=%04lx instruction=%08lx\n",
                context->Esi,context->FloatSave.ControlWord&0xffff,context->FloatSave.StatusWord&0xffff,
                context->FloatSave.TagWord&0xffff,context->FloatSave.ErrorOffset);
    }
    if (address == 0x4530a7 || address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        BYTE reals[36]; FILE *output;
        if (!read_memory(0x48f880,reals,12) || !read_memory(0x48fb08,reals+12,12) ||
            !read_memory(0x491058,reals+24,12)) ExitProcess(205);
        output=fopen("real-records.bin","wb");
        if (!output || fwrite(reals,1,sizeof(reals),output)!=sizeof(reals) || fclose(output)) ExitProcess(206);
        output=fopen("x87-final.bin","wb");
        if (!output || fwrite(&context->FloatSave,1,sizeof(context->FloatSave),output)!=sizeof(context->FloatSave) || fclose(output)) ExitProcess(207);
        fprintf(events,"extended-results control=%04lx status=%04lx tag=%04lx\n",context->FloatSave.ControlWord&0xffff,
                context->FloatSave.StatusWord&0xffff,context->FloatSave.TagWord&0xffff);
    }
    if (address == 0x4530fa || address == 0x45312c || address == 0x41f3f7 || address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92 || address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        BYTE descriptions[344*16]; FILE *output;
        if (!read_memory(0x48e300,descriptions,sizeof(descriptions))) ExitProcess(208);
        output=fopen("intrinsic-table.bin","wb");
        if (!output || fwrite(descriptions,1,sizeof(descriptions),output)!=sizeof(descriptions) || fclose(output)) ExitProcess(209);
        fprintf(events,"intrinsics-initialized records=344 bytes=%lu\n",sizeof(descriptions));
    }
    if (address == 0x40d5a8 || address == 0x41f42e || address == 0x453e92) {
        BYTE node[86]; DWORD fields[7], name, class_name, global_name; FILE *output;
        DWORD locations[7]={0x499724,0x4999f0,0x497f54,0x499738,0x497d8c,0x497f4c,0x499740};
        if (!read_memory(context->Ecx,node,sizeof(node))) ExitProcess(215);
        memcpy(&name,node+24,4); memcpy(&class_name,node+54,4);
        if (!read_memory(0x4980c4,&global_name,4)) ExitProcess(216);
        for(i=0;i<7;++i)if(!read_memory(locations[i],&fields[i],4))ExitProcess(217);
        output=fopen("section-node.bin","wb");if(!output || fwrite(node,1,86,output)!=86 || fclose(output))ExitProcess(218);
        output=fopen("section-globals.bin","wb");if(!output || fwrite(fields,4,7,output)!=7 || fclose(output))ExitProcess(219);
        save_string(name,"section-name.bin");save_string(class_name,"section-class.bin");
        if(address==0x40d5a8)fprintf(events,"section-symbol node=%08lx local=%lu name=%08lx class=%08lx name-alias=%lu\n",context->Ecx,context->Edx,name,class_name,(DWORD)(name==global_name));
        else if(address==0x41f42e)fprintf(events,"section-linked node=%08lx name=%08lx class=%08lx name-alias=%lu\n",context->Ecx,name,class_name,(DWORD)(name==global_name));
    }
    if (address == 0x453e92) {
        DWORD list[2]; FILE *output;
        if(!read_memory(0x497dd0,list,4) || !read_memory(0x497db8,list+1,4))ExitProcess(220);
        output=fopen("section-list.bin","wb");if(!output || fwrite(list,4,2,output)!=2 || fclose(output))ExitProcess(221);
        fprintf(events,"section-defaults node=%08lx head=%08lx tail=%08lx\n",context->Ecx,list[0],list[1]);
        {
            DWORD at, amount, original_context, alignment;
            BYTE data[4096];
            if(!read_memory(0x497f58,&original_context,4) || !read_memory(0x495f24,&alignment,4))ExitProcess(222);
            fprintf(events,"section-default-input context=%08lx alignment=%lu\n",original_context,alignment);
            output=fopen("unmodeled-original-data.bin","wb");if(!output)ExitProcess(223);
            for(at=0;at<0x10700;at+=amount) {
                amount=0x10700-at;if(amount>sizeof(data))amount=sizeof(data);
                if(!read_memory(0x48e000+at,data,amount) || fwrite(data,1,amount,output)!=amount)ExitProcess(224);
            }
            if(fclose(output))ExitProcess(225);
            fprintf(events,"unmodeled-original-data base=0048e000 bytes=67328 NO-payload-or-extent-credit\n");
        }
    }
    if (address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8) {
        BYTE data[2059]; FILE *output;
        if(!read_memory(0x497308,data,64))ExitProcess(241);
        output=fopen("symbol-template.bin","wb");if(!output || fwrite(data,1,64,output)!=64 || fclose(output))ExitProcess(242);
        if(!read_memory(0x496a00,data,2058))ExitProcess(243);
        output=fopen("symbol-name.bin","wb");if(!output || fwrite(data,1,2058,output)!=2058 || fclose(output))ExitProcess(244);
        if(!read_memory(0x48e050,data,12))ExitProcess(245);
        output=fopen("source-name-pool.bin","wb");if(!output || fwrite(data,1,12,output)!=12 || fclose(output))ExitProcess(246);
        if(address!=0x420350 && address!=0x4532d0 && address!=0x4283b4 && address!=0x42032b && address!=0x4200f5 && address!=0x4201c5 && address!=0x4283e3 && address!=0x41f453 && address!=0x40d5d8 && address!=0x4285e8 && !read_memory(context->Esp+(address==0x420ebb?16:12),&value,4))ExitProcess(247);
        if(address==0x420350 || address==0x4532d0 || address==0x4283b4 || address==0x42032b || address==0x4200f5 || address==0x4201c5 || address==0x4283e3 || address==0x41f453 || address==0x40d5d8 || address==0x4285e8) {
            if(!read_memory(0x497460,data,2059))ExitProcess(249);
            output=fopen("decorated-name.bin","wb");if(!output || fwrite(data,1,2059,output)!=2059 || fclose(output))ExitProcess(250);
            if(address==0x4283b4 || address==0x42032b || address==0x4200f5 || address==0x4201c5 || address==0x4283e3 || address==0x41f453 || address==0x40d5d8 || address==0x4285e8) {
                BYTE symbol[110], filename[12]; DWORD head, decoded_kind, retention, argument=address==0x4285e8?context->Edx:(address==0x42032b || address==0x41f453 || address==0x40d5d8 || address==0x4285e8)?context->Esi:(address==0x4200f5 || address==0x4201c5)?context->Ebp:context->Ecx;
                if(!read_memory(argument,symbol,110) || !read_memory(0x4973b8,&head,4) || !read_memory(head,filename,12) || !read_memory(context->Esp+(address==0x42032b?32:24),&decoded_kind,4) || !read_memory(context->Esp+(address==0x42032b?20:12),&retention,4))ExitProcess(252);
                if(address==0x4283e3 || address==0x41f453 || address==0x40d5d8 || address==0x4285e8) {
                    decoded_kind=symbol[4];
                    if(address==0x41f453 || address==0x40d5d8){if(!read_memory(context->Esp+12,&retention,4))ExitProcess(255);if(address==0x41f453)fprintf(events,"lookup-input serial=%lu symbol=%08lx\n",context->Ecx,argument);
                        else fprintf(events,"next-serial-input selected-section=%08lx identifier=%lu\n",context->Eax,context->Ebp);}
                    else retention=context->Ebp;
                    fprintf(events,"storage-input symbol=%08lx caller-symbol=%08lx table=%08lx\n",argument,context->Esi,context->Edi);
                }
                if(address==0x4285e8){
                    DWORD pending,insertion;
                    if(!read_memory(0x4973d4,&pending,4)||!read_memory(0x4973f0,&insertion,4))ExitProcess(256);
                    fprintf(events,"put-input scope=%08lx symbol=%08lx caller-symbol=%08lx table=%08lx retention=%lu pending=%08lx insertion=%08lx\n",context->Ecx,argument,context->Esi,context->Edi,context->Ebp,pending,insertion);
                }
                output=fopen("function-symbol.bin","wb");if(!output || fwrite(symbol,1,110,output)!=110 || fclose(output))ExitProcess(253);
                output=fopen("filename-node.bin","wb");if(!output || fwrite(filename,1,12,output)!=12 || fclose(output))ExitProcess(254);
                fprintf(events,"type-input symbol=%08lx decoded-kind=%lu retention=%lu filenames=%08lx\n",argument,decoded_kind,retention,head);
            } else if(address==0x4532d0) {
                if(!read_memory(context->Ecx+24,&pointer,4) || !read_memory(context->Ecx+40,&value,4))ExitProcess(251);
                fprintf(events,"filename-input symbol=%08lx name=%08lx serial=%lu\n",context->Ecx,pointer,value);
            } else fprintf(events,"symbol-copy-input name=%08lx pool=%lu caller-symbol=%08lx\n",context->Ecx,context->Edx,context->Ebp);
        } else if(address==0x420ebb) {
            if(!read_memory(context->Esp+4,&pointer,4))ExitProcess(248);
            fprintf(events,"symbol-intern-input symbol=%08lx name=%08lx pool=%lu caller-kind=%ld retention=%lu\n",context->Ecx,context->Edx,pointer,(long)(signed char)context->Ebx,value);
        } else if(address==0x420e74)fprintf(events,"symbol-name-input buffer=%08lx capacity=%lu caller-kind=%ld retention=%lu\n",context->Ecx,context->Edx,(long)(signed char)context->Ebx,value);
        else fprintf(events,"symbol-serial-input kind=%ld retention=%lu formals=0 template=00497308 bytes=64 name-bytes=2058\n",(long)(signed char)context->Ebx,value);
    }
    if (address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        BYTE data[2059]; FILE *output;
        if(!read_memory(0x497308,data,64))ExitProcess(257);
        output=fopen("symbol-template.bin","wb");if(!output || fwrite(data,1,64,output)!=64 || fclose(output))ExitProcess(258);
        if(!read_memory(0x496a00,data,2058))ExitProcess(259);
        output=fopen("symbol-name.bin","wb");if(!output || fwrite(data,1,2058,output)!=2058 || fclose(output))ExitProcess(260);
        if(!read_memory(0x497460,data,2059))ExitProcess(261);
        output=fopen("decorated-name.bin","wb");if(!output || fwrite(data,1,2059,output)!=2059 || fclose(output))ExitProcess(262);
        if(address==0x4523d2){DWORD iob;if(!read_memory(0x48d08c,&iob,4))ExitProcess(263);
            fprintf(events,"globals-complete initializer-input file=%08lx iob=%08lx\n",context->Ecx,iob);
        }else if(address==0x453ad4) {
            fprintf(events,"globals-complete output-directive-input\n");
            fprintf(events,"output-directive-input kind=%lu value=%08lx result=%08lx\n",context->Ecx,context->Edx,context->Eax);
            save_string(context->Edx,"output-directive-value.bin");
        }else fprintf(events,"globals-complete no-formals\n");
        if(address==0x453305 || address==0x4538de || address==0x4538fc || address==0x453ab2 || address==0x453ad4 || address==0x453b0c || address==0x453c5a || address==0x453d75 || address==0x41eb8a || address==0x453dc6 || address==0x41eb96 || address==0x41ebca || address==0x4526ed || address==0x41ebe3 || address==0x453de7 || address==0x453e02 || address==0x453eab || address==0x41ec20 || address==0x41ec59 || address==0x4522ec || address==0x41ef35||address==0x41ef83||address==0x41f085||address==0x41f0c4){DWORD input,words[8];BYTE buffer[15];
            if(!read_memory(0x499698,&input,4)||!read_memory(input,words,32)||!words[2]||!read_memory(words[2],buffer,15))ExitProcess(264);
            output=fopen("initializer-buffer.bin","wb");if(!output||fwrite(buffer,1,15,output)!=15||fclose(output))ExitProcess(265);
            fprintf(events,"initializer-input-complete pointer=%08lx base=%08lx count=%ld flag=%08lx valid=15\n",words[0],words[2],(long)words[1],words[3]);
        }
    }
    if (address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD expression, symbols, iob; BYTE data[64]; FILE *output;
        if(!read_memory(0x4973dc,&expression,4)||!read_memory(0x4973b4,&symbols,4)||!read_memory(0x48d08c,&iob,4)||
           !read_memory(expression,data,32)||!read_memory(symbols,data+32,32))ExitProcess(266);
        fprintf(events,"backend-inputs-open expression=%08lx symbols=%08lx iob=%08lx\n",expression,symbols,iob);
        output=fopen("backend-streams.bin","wb");if(!output||fwrite(data,1,64,output)!=64||fclose(output))ExitProcess(267);
    }
    if (address == 0x4206b7) {
        BYTE scope[4100]; FILE *output;
        if(!read_memory(0x48fb68,scope,sizeof(scope)))ExitProcess(239);
        output=fopen("symbol-scope.bin","wb");if(!output || fwrite(scope,1,sizeof(scope),output)!=sizeof(scope) || fclose(output))ExitProcess(240);
        fprintf(events,"symbol-record-input retention=%lu caller-scope=%08lx table=%08lx caller-retention=%lu bytes=4100 buckets=4\n",context->Ecx,context->Esi,context->Edi,context->Ebp);
    }
    if (address == 0x420394) {
        DWORD active, globals;
        if(!read_memory(0x490b80,&active,4) || !read_memory(0x497fd4,&globals,4))ExitProcess(238);
        fprintf(events,"global-symbols-input scope=%08lx active=%08lx globals=%08lx\n",context->Ecx,active,globals);
    }
    if (address == 0x4532ae || address == 0x420394 || address == 0x4206b7 || address == 0x420250 || address == 0x420e74 || address == 0x420ebb || address == 0x420350 || address == 0x4532d0 || address == 0x4283b4 || address == 0x42032b || address == 0x4200f5 || address == 0x4201c5 || address == 0x4283e3 || address == 0x41f453 || address == 0x40d5d8 || address == 0x4285e8 || address == 0x4532fa || address == 0x4523d2 || address == 0x453305 || address == 0x4538de || address == 0x4538fc || address == 0x453ab2 || address == 0x453ad4 || address == 0x453b0c || address == 0x453c5a || address == 0x453d75 || address == 0x41eb8a || address == 0x453dc6 || address == 0x41eb96 || address == 0x41ebca || address == 0x4526ed || address == 0x41ebe3 || address == 0x453de7 || address == 0x453e02 || address == 0x453eab || address == 0x41ec20 || address == 0x41ec59 || address == 0x4522ec || address == 0x41ef35 || address == 0x41ef83 || address == 0x41f085 || address == 0x41f0c4) {
        DWORD fields[7], pointers[12], list[2], current, next, name, class_name, at, amount;
        DWORD count=(address==0x453de7||address==0x453e02||address==0x453eab||address==0x41ec20||address==0x41ec59||address==0x4522ec||address==0x41ef35||address==0x41ef83||address==0x41f085||address==0x41f0c4)?12:(address==0x4532fa || address==0x4523d2 || address==0x453305 || address==0x4538de || address==0x4538fc || address==0x453ab2 || address==0x453ad4 || address==0x453b0c || address==0x453c5a || address==0x453d75 || address==0x41eb8a || address==0x453dc6 || address==0x41eb96 || address==0x41ebca || address==0x4526ed || address==0x41ebe3 || address==0x453de7 || address==0x453e02 || address==0x453eab || address==0x41ec20 || address==0x41ec59 || address==0x4522ec || address==0x41ef35||address==0x41ef83||address==0x41f085||address==0x41f0c4)?10:address==0x4285e8?6:5;
        DWORD locations[7]={0x499724,0x4999f0,0x497f54,0x499738,0x497d8c,0x497f4c,0x499740};
        BYTE node[86], data[4096]; char file_name[64]; FILE *output;
        for(i=0;i<7;++i)if(!read_memory(locations[i],fields+i,4))ExitProcess(226);
        if(!read_memory(0x497dd0,list,4) || !read_memory(0x497db8,list+1,4))ExitProcess(227);
        output=fopen("section-globals.bin","wb");if(!output || fwrite(fields,4,7,output)!=7 || fclose(output))ExitProcess(228);
        output=fopen("section-list.bin","wb");if(!output || fwrite(list,4,2,output)!=2 || fclose(output))ExitProcess(229);
        output=fopen("sections.bin","wb");if(!output)ExitProcess(230);
        current=list[0];
        for(i=0;i<count;++i) {
            if(!current || !read_memory(current,node,86))ExitProcess(231);
            pointers[i]=current;memcpy(&name,node+24,4);memcpy(&class_name,node+54,4);memcpy(&next,node+50,4);
            sprintf(file_name,"section-name-%lu.bin",i);save_string(name,file_name);
            sprintf(file_name,"section-class-%lu.bin",i);save_string(class_name,file_name);
            if(fwrite(node,1,86,output)!=86)ExitProcess(232);current=next;
        }
        if(current || fclose(output))ExitProcess(233);
        output=fopen("section-addresses.bin","wb");if(!output || fwrite(pointers,4,count,output)!=count || fclose(output))ExitProcess(234);
        output=fopen("unmodeled-original-data.bin","wb");if(!output)ExitProcess(235);
        for(at=0;at<0x10700;at+=amount) {
            amount=0x10700-at;if(amount>sizeof(data))amount=sizeof(data);
            if(!read_memory(0x48e000+at,data,amount) || fwrite(data,1,amount,output)!=amount)ExitProcess(236);
        }
        if(fclose(output))ExitProcess(237);
        fprintf(events,"sections-initialized records=%lu head=%08lx tail=%08lx UNMODELED-context=0049e300\n",count,list[0],list[1]);
    }
    if (address == 0x41f3f7) {
        DWORD name, sections[7], addresses[7]={0x499724,0x4999f0,0x497f54,0x499738,0x497d8c,0x497f4c,0x499740};
        FILE *output;
        if (!read_memory(context->Esp,stack,sizeof(stack)) || !read_memory(0x4980c4,&name,4)) ExitProcess(210);
        for(i=0;i<7;++i)if(!read_memory(addresses[i],sections+i,4))ExitProcess(211);
        save_string(context->Ecx,"section-name.bin");save_string(context->Edx,"section-class.bin");
        output=fopen("section-globals.bin","wb");
        if(!output || fwrite(sections,1,sizeof(sections),output)!=sizeof(sections) || fclose(output))ExitProcess(212);
        fprintf(events,"section-constructor name=%08lx class=%08lx serial=%lu name-alias=%d\n",context->Ecx,context->Edx,stack[1],context->Ecx==name);
    }
    if (address == 0x452335) {
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(184);
        fprintf(events,"intermediate-open base=%08lx suffix=%08lx mode=%08lx description=%08lx\n",
                context->Ecx,context->Edx,stack[1],stack[2]);
        save_string(context->Ecx,"intermediate-base.bin");
        save_string(context->Edx,"intermediate-suffix.bin");
        save_string(stack[1],"intermediate-mode.bin");
        save_string(stack[2],"intermediate-description.bin");
    }
    if (address == 0x452e27) {
        DWORD current, operand, argv, mb_address, mb_value, table_address, table;
        short character; unsigned short flags;
        if (!read_memory(0x49968c,&current,4) ||
            !read_memory(0x4980c8,&operand,4) ||
            !read_memory(0x499728,&argv,4)) ExitProcess(179);
        fprintf(events,"integer-option input=%08lx current=%08lx operand=%08lx argv=%08lx\n",
                context->Ecx,current,operand,argv);
        save_string(context->Ecx,"integer-input.bin");
        save_string(current,"current-option.bin");
        if (!read_memory(0x48d104,&mb_address,4) ||
            !read_memory(mb_address,&mb_value,4) ||
            !read_memory(0x48d0d4,&table_address,4) ||
            !read_memory(table_address,&table,4) ||
            !read_memory(context->Ecx,&character,1)) ExitProcess(180);
        character=(signed char)character;
        if (!read_memory(table+character*2,&flags,2)) ExitProcess(181);
        fprintf(events,"integer-ctype mb-address=%08lx mb=%lu table-address=%08lx table=%08lx character=%d flags=%04x\n",
                mb_address,mb_value,table_address,table,character,flags);
        save_contextual_data();
    }
    if (address == 0x452a98) {
        fprintf(events,"environment-copy flags=%08lx\n",context->Esi);
        save_string(context->Esi,"copy-flags.bin");
        save_contextual_data();
    }
    if (address == 0x452a22) {
        DWORD slot=observed_environment, entry, alias=0;
        char name[14];
        if (!slot) ExitProcess(177);
        for (i=0;i<4096;++i,slot+=4) {
            if (!read_memory(slot,&entry,4)) ExitProcess(178);
            if (!entry)break;
            if (read_memory(entry,name,sizeof(name)) && !memcmp(name,"MSC_CMD_FLAGS=",14)) {
                alias=context->Ecx==entry+14;
                fprintf(events,"flags-environment entry=%08lx returned=%08lx alias=%lu\n",entry,context->Ecx,alias);
                save_string(context->Ecx,"environment-flags.bin");
                break;
            }
        }
    }
    if (address == 0x4528b0) {
        if (!read_memory(context->Esp,stack,sizeof(stack))) ExitProcess(147);
        fprintf(events,"main-input argc=%lu argv=%08lx environment=%08lx\n",stack[1],stack[2],stack[3]);
        observed_environment=stack[3];
        save_vector(stack[2],"main-argv.bin");
        save_vector(stack[3],"main-environment.bin");
        if (!read_memory(0x7803fde8,&value,4)) ExitProcess(148);
        fprintf(events,"main-initenv-alias %lu\n",value==stack[3]);
    }
    if (address == 0x48b79f) {
        DWORD wrote;
        BYTE trap = 0xcc;
        if (!read_memory(context->Esp,message_slots,sizeof(message_slots))) ExitProcess(142);
        fprintf(events,"loadstring-input module=%08lx id=%lu buffer=%08lx capacity=%lu\n",message_slots[0],message_slots[1],message_slots[2],message_slots[3]);
        if (!WriteProcessMemory(child,(void *)0x48b7a5,&trap,1,&wrote) || wrote != 1) ExitProcess(143);
        FlushInstructionCache(child,(void *)0x48b7a5,1);
    }
    if (address == 0x48b7a5) {
        DWORD wrote;
        BYTE trap=0xcc, bytes[384];
        char name[80];
        FILE *file;
        if (message_slots[3] != sizeof(bytes) || !read_memory(message_slots[2],bytes,sizeof(bytes))) ExitProcess(144);
        sprintf(name,"message-%02lu-%lu.bin",message_number++,message_slots[1]);
        file=fopen(name,"wb"); if (!file) ExitProcess(145);
        fwrite(bytes,1,sizeof(bytes),file);fclose(file);
        fprintf(events,"loadstring-output id=%lu result=%ld bytes=%s\n",message_slots[1],(long)context->Eax,name);
        if (!WriteProcessMemory(child,(void *)0x48b79f,&trap,1,&wrote) || wrote!=1) ExitProcess(146);
        FlushInstructionCache(child,(void *)0x48b79f,1);
    }
    if (address == 0x452855 && !read_memory(context->Esp,mainargs_slots,sizeof(mainargs_slots))) ExitProcess(138);
    if (address == 0x45285b) {
        for (i=0;i<5;++i) {
            if (i == 3) value=mainargs_slots[i];
            else if (!read_memory(mainargs_slots[i],&value,4)) ExitProcess(139);
            fprintf(events,"mainargs %lu %08lx %08lx\n",i,mainargs_slots[i],value);
        }
        if (!read_memory(mainargs_slots[1],&pointer,4)) ExitProcess(140);
        save_vector(pointer,"argv.bin");
        if (!read_memory(mainargs_slots[2],&pointer,4)) ExitProcess(141);
        save_vector(pointer,"environment.bin");
    }
}
int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE out, err, input;
    DWORD disposition, address, code, wrote, base = 0;
    BYTE trap = 0xcc;
    char command[4096];
    unsigned i;
    int loader_break = 0, completed = 0;
    FILE *plan;
    char child_environment[] = "PATH=R:\\toolchains\\5.10\\bin\0SystemRoot=C:\\windows\0TEMP=.\0TMP=.\0WINEDLLOVERRIDES=msvcrt=n\0WINEPATH=R:\\toolchains\\5.10\\bin\0windir=C:\\windows\0";
    char *environment=child_environment;
    DWORD environment_size=sizeof(child_environment);
    const char *working_directory="S:\\vc5.10\\c2\\original";
    if (argc != 5 && argc != 7) return 120;
    if (argc == 7) {
        FILE *input_file=fopen(argv[5],"rb");
        long size;
        if(!input_file || fseek(input_file,0,SEEK_END))return 179;
        size=ftell(input_file);
        if(size<2 || size>65536 || fseek(input_file,0,SEEK_SET))return 180;
        environment=(char *)malloc(size);if(!environment)return 181;
        if(fread(environment,1,size,input_file)!=(DWORD)size || fclose(input_file))return 182;
        if(environment[size-1] || environment[size-2])return 183;
        environment_size=(DWORD)size;working_directory=argv[6];
    }
    events = fopen("events.txt", "wb");
    plan = fopen(argv[4], "r");
    if (!events || !plan) return 121;
    while (entries < 2048 && fscanf(plan, "%lx %x", &entry[entries], &i) == 2)
        entry_byte[entries++] = (BYTE)i;
    fclose(plan);
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security); security.bInheritHandle = TRUE;
    out = CreateFileA("stdout.bin", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                      &security, CREATE_NEW, 0, NULL);
    err = CreateFileA("stderr.bin", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                      &security, CREATE_NEW, 0, NULL);
    input = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        &security, OPEN_EXISTING, 0, NULL);
    memset(&startup, 0, sizeof(startup)); startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input; startup.hStdOutput = out; startup.hStdError = err;
    if(*argv[3])sprintf(command, "\"%s\" %s", argv[2], argv[3]);
    else sprintf(command,"\"%s\"",argv[2]);
    {
        FILE *input_file = fopen("child-environment-input.bin", "wb");
        if (!input_file) return 147;
        fwrite(environment, 1, environment_size, input_file); fclose(input_file);
        input_file = fopen("child-command-input.bin", "wb");
        if (!input_file) return 148;
        fwrite(command, 1, strlen(command)+1, input_file); fclose(input_file);
    }
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
                        environment, working_directory, &startup, &process)) return 122;
    child = process.hProcess;
    while (!completed && WaitForDebugEvent(&event, 30000)) {
        disposition = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            fprintf(events, "image-base %08lx\n", base);
            ids[count] = event.dwThreadId; handles[count++] = event.u.CreateProcessInfo.hThread;
            CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (count == 128) return 123;
            ids[count] = event.dwThreadId; handles[count++] = event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            loaded((DWORD)event.u.LoadDll.lpBaseOfDll, event.u.LoadDll.hFile);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == EXCEPTION_BREAKPOINT && !loader_break && (address < base || address >= base+0xa3000)) {
                loader_break = 1;
            } else {
                for (i=0; i<count && ids[i]!=event.dwThreadId; ++i) {}
                if (i == count) return 130;
                memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
                if (!GetThreadContext(handles[i], &context)) return 131;
                if (code == EXCEPTION_SINGLE_STEP && event.dwThreadId == extended_step_thread) {
                    BYTE destination[12]; FILE *output;
                    if (context.Eip != 0x465760 || !read_memory(context.Esi,destination,sizeof(destination))) return 200;
                    output=fopen("extended-after.bin","wb");
                    if (!output || fwrite(destination,1,sizeof(destination),output)!=sizeof(destination) || fclose(output)) return 201;
                    output=fopen("x87-after.bin","wb");
                    if (!output || fwrite(&context.FloatSave,1,sizeof(context.FloatSave),output)!=sizeof(context.FloatSave) || fclose(output)) return 202;
                    fprintf(events,"extended-after destination=%08lx control=%04lx status=%04lx tag=%04lx instruction=%08lx\n",context.Esi,
                        context.FloatSave.ControlWord&0xffff,context.FloatSave.StatusWord&0xffff,context.FloatSave.TagWord&0xffff,context.FloatSave.ErrorOffset);
                    extended_step_thread=0;context.EFlags&=~0x100;
                    if (!SetThreadContext(handles[i],&context)) return 203;
                    fflush(events);ContinueDebugEvent(event.dwProcessId,event.dwThreadId,disposition);
                    continue;
                }
                for (i=0; i<entries && entry[i]!=address; ++i) {}
                if (code == EXCEPTION_BREAKPOINT && i < entries) {
                    fprintf(events, "entry %08lx\n", address);
                    observe(&context,address);
                    if (!WriteProcessMemory(child, (void *)address, entry_byte+i, 1, &wrote) || wrote!=1) return 132;
                    FlushInstructionCache(child, (void *)address, 1);
                    context.Eip = address;
                    if (address == 0x46575e) {
                        if (context.EFlags&0x100) return 204;
                        extended_step_thread=event.dwThreadId;context.EFlags|=0x100;
                    }
                    for (i=0; i<count && ids[i]!=event.dwThreadId; ++i) {}
                    if (!SetThreadContext(handles[i], &context)) return 133;
                } else {
                    DWORD stack[16];
                    fprintf(events, "trap %08lx %08lx eip=%08lx esp=%08lx ecx=%08lx edx=%08lx\n",
                        code, address, context.Eip, context.Esp, context.Ecx, context.Edx);
                    if (read_memory(context.Esp, stack, sizeof(stack))) {
                        fprintf(events,"stack");
                        for (i=0; i<16; ++i) fprintf(events," %08lx",stack[i]);
                        fprintf(events,"\n");
                    }
                    observe(&context,address);
                    TerminateProcess(child, 237); completed = 1;
                }
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            fprintf(events, "exit %08lx\n", event.u.ExitProcess.dwExitCode); completed = 1;
        }
        fflush(events);
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
    }
    if (!completed) { fprintf(events, "timeout\n"); TerminateProcess(child, 238); }
    fclose(events); CloseHandle(out); CloseHandle(err); CloseHandle(input);
    return completed ? 0 : 134;
}

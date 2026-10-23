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

static int read_memory(DWORD address, void *buffer, DWORD size)
{
    DWORD got;
    return ReadProcessMemory(child, (void *)address, buffer, size, &got) && got == size;
}
static void save_file(HANDLE source, const char *name)
{
    HANDLE destination;
    BYTE bytes[16384];
    DWORD got, wrote, previous;
    previous = SetFilePointer(source, 0, NULL, FILE_CURRENT);
    if (previous == 0xffffffffUL || SetFilePointer(source, 0, NULL, FILE_BEGIN) == 0xffffffffUL)
        ExitProcess(124);
    destination = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    if (destination == INVALID_HANDLE_VALUE) ExitProcess(125);
    for (;;) {
        if (!ReadFile(source, bytes, sizeof(bytes), &got, NULL)) ExitProcess(126);
        if (!got) break;
        if (!WriteFile(destination, bytes, got, &wrote, NULL) || wrote != got) ExitProcess(127);
    }
    CloseHandle(destination);
    SetFilePointer(source, previous, NULL, FILE_BEGIN);
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
    static DWORD globals[] = {0x499708,0x4996d4,0x49975c,0x4980d4,0x499718,0x499694,0x4980d0,0x4980cc,0x48d064,0x7803fa90,0x7803fa88,0x7803fa94,0x7803f198,0x7803f928,0x7803fde8,0x499ac8,0x499adc,0x497e1c,0x497e08,0x497cac,0x49e2f4,0x4980d8,0x497fb0,0x490b7c,0x4972f8,0x497df8,0x491050,0x497f6c,0x497de8,0x48fb68,0x491040,0x491044,0x491048,0x49104c};
    fprintf(events,"observe %08lx eax=%08lx ecx=%08lx edx=%08lx esp=%08lx\n",
        address,context->Eax,context->Ecx,context->Edx,context->Esp);
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
    if (address == 0x401000 || address == 0x40d534 || address == 0x40d507 || address == 0x45302d || address == 0x453012 || address == 0x452f95 || address == 0x452f4b || address == 0x4200d1 || address == 0x452ea4 || address == 0x452335 || address == 0x452e9b || address == 0x452e27 || address == 0x452a22 || address == 0x452a98 || address == 0x452acb || address == 0x452bb7 || address == 0x452bd9 || address == 0x48b7f8 || address == 0x452cfe || address == 0x48b367 || address == 0x48b6e8 || address == 0x48b708) {
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
    if (address == 0x452e9b || address == 0x452335 || address == 0x452ea4 || address == 0x4200d1 || address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000) {
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
    if (address == 0x452ea4 || address == 0x4200d1 || address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000) {
        DWORD input, globals, iob, active;
        BYTE data[64]; FILE *output;
        if (!read_memory(0x499698,&input,4) ||
            !read_memory(0x497fd4,&globals,4) ||
            !read_memory(0x48d08c,&iob,4) ||
            !read_memory(input,data,32) || !read_memory(globals,data+32,32)) ExitProcess(185);
        fprintf(events,"header-streams input=%08lx globals=%08lx iob=%08lx\n",input,globals,iob);
        if (address == 0x4200d1 || address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000) {
            if (!read_memory(0x490b80,&active,4)) ExitProcess(187);
            fprintf(events,"active-il-stream %08lx\n",active);
        }
        output=fopen("header-streams.bin","wb");
        if (!output || fwrite(data,1,64,output)!=64 || fclose(output)) ExitProcess(186);
        if (address == 0x452f4b || address == 0x452f95 || address == 0x453012 || address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000) {
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
    if (address == 0x45302d || address == 0x40d507 || address == 0x40d534 || address == 0x401000) {
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
                for (i=0; i<entries && entry[i]!=address; ++i) {}
                if (code == EXCEPTION_BREAKPOINT && i < entries) {
                    fprintf(events, "entry %08lx\n", address);
                    observe(&context,address);
                    if (!WriteProcessMemory(child, (void *)address, entry_byte+i, 1, &wrote) || wrote!=1) return 132;
                    FlushInstructionCache(child, (void *)address, 1);
                    context.Eip = address;
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

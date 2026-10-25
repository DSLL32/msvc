/* Authored native i386 protocol probe, not a Win32 broker or application code. */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

enum { MAP_BYTES=16384, MAX_PAYLOAD=4096, PAYLOAD=256, MAX_SEQUENCE=1000000 };
static uint8_t *area;
static uint32_t load(unsigned offset)
{
    return __atomic_load_n((uint32_t *)(area+offset),__ATOMIC_ACQUIRE);
}
static void publish(unsigned offset,uint32_t value)
{
    __atomic_exchange_n((uint32_t *)(area+offset),value,__ATOMIC_RELEASE);
}
static double now(void)
{
    struct timespec value;
    if(clock_gettime(CLOCK_MONOTONIC,&value)) exit(99);
    return value.tv_sec+value.tv_nsec/1000000000.0;
}
static void pause_ms(unsigned ms)
{
    struct timespec delay={ms/1000,(ms%1000)*1000000};
    while(nanosleep(&delay,&delay) && errno==EINTR) {}
}
static int finish(int status,const char *reason,uint32_t seen)
{
    printf("status=%d reason=%s processed=%u pointer_bits=%zu\n",status,reason,
           seen,sizeof(void *)*8);
    munmap(area,MAP_BYTES);
    return status;
}
int main(int argc,char **argv)
{
    int fd; struct stat facts;
    uint32_t seen=0,lease,command,length,operation,i;
    double start,last_lease,deadline_ms,lease_ms;
    const char *mode;
    uint8_t copy[MAX_PAYLOAD];
    if(argc!=5) return 90;
    mode=argv[2];deadline_ms=strtoul(argv[3],NULL,10);lease_ms=strtoul(argv[4],NULL,10);
    if(deadline_ms<1 || deadline_ms>300000 || lease_ms<1 || lease_ms>10000) return 91;
    fd=open(argv[1],O_RDWR);
    if(fd<0 || fstat(fd,&facts) || facts.st_size!=MAP_BYTES) return 92;
    area=mmap(NULL,MAP_BYTES,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);close(fd);
    if(area==MAP_FAILED || ((uintptr_t)area&3)) return 93;
    if(load(0)!=0x50434757 || load(4)!=1 || load(8)!=MAP_BYTES)
        return finish(94,"header",seen);
    lease=load(20);
    if(!lease || lease>MAX_SEQUENCE) return finish(75,"initial_lease",seen);
    start=last_lease=now();
    while(1) {
        double current=now();
        uint32_t next_lease=load(20);
        if(next_lease!=lease) {
            if(next_lease<=lease || next_lease>MAX_SEQUENCE)
                return finish(75,"lease_sequence",seen);
            lease=next_lease;last_lease=current;
        }
        if((current-start)*1000>=deadline_ms) return finish(73,"global_deadline",seen);
        if((current-last_lease)*1000>=lease_ms) return finish(74,"owner_lease",seen);
        if(load(0)!=0x50434757 || load(4)!=1 || load(8)!=MAP_BYTES)
            return finish(94,"header",seen);
        command=load(12);
        if(command==seen) {pause_ms(1);continue;}
        if(command!=seen+1 || command>MAX_SEQUENCE || load(16)!=seen)
            return finish(76,"command_sequence",seen);
        operation=load(24);length=load(28);
        if(length>MAX_PAYLOAD) return finish(77,"command_length",seen);
        memcpy(copy,area+PAYLOAD,length);
        if(load(12)!=command || load(24)!=operation || load(28)!=length)
            return finish(78,"command_changed",seen);
        if(operation!=1 && operation!=2) {
            *(uint32_t *)(area+32)=79;*(uint32_t *)(area+36)=0;
            publish(16,command);return finish(79,"unknown_operation",seen);
        }
        if(operation==2) {
            *(uint32_t *)(area+32)=0;*(uint32_t *)(area+36)=0;
            publish(16,command);return finish(0,"requested_exit",command);
        }
        /* Fault modes are deliberate protocol violations in this disposable peer. */
        if(!strcmp(mode,"silent")) {pause_ms(1);continue;}
        if(!strcmp(mode,"late")) pause_ms(250);
        if(!strcmp(mode,"partial")) {
            memset(area+PAYLOAD,0x5a,length);
            *(uint32_t *)(area+32)=0;*(uint32_t *)(area+36)=length;
            pause_ms(50); /* No publication: client must not consume the partial payload. */
        }
        for(i=0;i<length;i++) area[PAYLOAD+i]=copy[i]^0xa5;
        *(uint32_t *)(area+32)=0;
        *(uint32_t *)(area+36)=!strcmp(mode,"oversized")?MAX_PAYLOAD+1:length;
        if(!strcmp(mode,"header")) *(uint32_t *)area=0;
        publish(16,!strcmp(mode,"skip")?command+1:command);
        seen=command;
    }
}

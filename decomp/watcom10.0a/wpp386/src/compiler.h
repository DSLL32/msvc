#ifndef WPP_COMPILER_H
#define WPP_COMPILER_H
#include <recon.h>
typedef struct initfini INITFINI;
typedef struct exit_point EXIT_POINT;
struct initfini {
    void (*init_rtn)(INITFINI *);
    void (*fini_rtn)(INITFINI *);
};
/* The 1994 trailing registration array ends with a NULL sentinel.
 * It predates the subsequent bit introduced in the released source. */
struct exit_point {
    EXIT_POINT *previous;
    INITFINI *registered[];
};
extern EXIT_POINT ExitPoint_program;
extern unsigned StartupWord;
void CtxSetContext(unsigned char curr);
void ExitPointAcquireRtn(EXIT_POINT *point);
void ExitPointReleaseRtn(EXIT_POINT *point);
int front_end(char **argv);
void addPerm(unsigned size);
void *CMemAlloc(unsigned size);
void CMemFree(void *ptr);
void *CPermAlloc(unsigned size);
void *alloc_from_cleanup(unsigned size);
struct perm_blk;
void linkPerm(struct perm_blk *block, unsigned size);
void *cutPerm(struct perm_blk *block, unsigned size);
void CErr1(unsigned code);
unsigned CErr2p(unsigned code, const char *text);
void CSuicide(void);
void CMemRegisterCleanup(void (*cleanup)(void));
void reserveRelease(void);
void *RingPush(void **head, void *entry);
void *RingAlloc(void **head, unsigned size);
void RingAppend(void **head, void *entry);
void RingInsert(void **head, void *entry, void *insert);
typedef struct ringname { struct ringname *next; char name[1]; } RINGNAME;
typedef struct ringnamectl { RINGNAME *ring; unsigned case_sensitive : 1; } RINGNAMECTL;
void RingNameInit(RINGNAMECTL *control);
void setPaths(char **paths);
int pathExists(char *path);
typedef struct vbuf { char *buf; unsigned len, used; } VBUF;
void VbufInit(VBUF *buffer);
char *NameCreateNoLen(char *name);
typedef struct name_record NAME_RECORD;
unsigned nameHash(char *id, unsigned *length);
char *nameAdd(NAME_RECORD **head, unsigned bucket, char *id, unsigned length);
void cmemInit(INITFINI *entry);
#endif

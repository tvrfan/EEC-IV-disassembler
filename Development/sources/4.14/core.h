
/*******************************************************************
common declarations for 'core' code modules (disassembly)
**********************************************************************

 * NOTE - This code assumes 'int' is at least 32 bit (4 bytes),
 *
 * On some older 16 bit compilers this would require the use of LONG instead of INT.
 * code here also uses a lot of pointer casts. Should be fine on any 'C' compiler,
 * but stated here just in case
 *
 *
 * 8065 manual says that bank is an extra 4 bits in CPU, (bits 16-19) to give a 20 bit address (0-0xFFFFF)
 * which could therefore be 1 Mb (1048576), but biggest bins are 256k (= 0x40000), or 4 banks, using 2 bits.
 *
 * Bin addresses used throughout this code are all 20 bit, = bank in 0xf0000 + 16 bit address. Mostly as unsigned int.
 * This code uses a pointer in a BANK structure to map into each malloced memory block (= bank) for best speed.
 * This design is therefore bank order independent. To make logic simpler, banks are actually set as +1, so
 * the standard 0,1,8,9 banks are handled as 1,2,9,10 in the code. this gives easy way to spot values with no bank.
 * only registers can have no bank.
 *
 * This code can handle 15 banks with only changes to bank setup subroutines required (detect and check).
 * Apart from this, all code uses bank+address everywhere, but not tested beyond 4 banks
 *
 *********************************************************/

#ifndef _XCOREX_H
#define _XCOREX_H 1

#include  "shared.h"


#define PATWINTP         11   // word table interpolate (for signed/unsigned checks)
#define PATBINTP         10    // byte table interpolate (for signed/unsigned checks)

SIG* do_sig (uint pat, uint ofst);

void  show_prog    (int);

SIG*  scan_sigs    (int);
SIG*  scan_asigs   (int);
void  prescan_sigs (void);
int   do_jumpsig   (int, uint, uint);
void  do_jumps     (void);

uint fmask(uint fstart, uint fend);
void *mem          (void*,size_t, size_t);
void  mfree        (void *, size_t);

void free_structs (void);
void set_rgsize   (RST *,uint);

int adtchnsize    (void*,  uint);

void* vconv (uint);

ADT *append_adt      (void *, uint);
LBK *add_cmd      (uint, uint, uint, uint);
LBK *add_aux_cmd  (uint, uint, uint, uint);
RBT *add_rbase    (uint, uint, uint, uint);
SYM *add_sym       (cchar *,    uint , uint , uint, uint,  uint);
SUB *add_subr     (uint);
SBK *add_scan     (uint, int, SBK *);
SPF *append_spf   (void *, uint, uint);
SBK *add_escan    (uint, SBK *);
JMP *add_jump     (SBK *, int, int);
RST *add_rgstat   (uint, uint, uint, uint);
PSW *add_psw      (uint, uint);
TRK* add_dtk      (SBK *,INST *);
BNKF *add_bnkf    (uint add, uint pc);

ADT *get_adt       (void *fid, uint bix);
SBK *get_scan      (uint, uint);
RST *get_rgarg     (uint, uint);
LBK *get_cmd       (uint, uint);
LBK *get_aux_cmd   (uint, uint);
SBK *get_scan      (uint);

LBK *get_prt_cmd   (LBK *dflt, BANK *b, uint ofst);
RST* get_rgstat    (uint);
SIG* get_sig       (uint);
uint get_tjmp_ix   (uint);
JMP *get_fjump     (uint);
uint get_jump      (CHAIN *,uint);
SUB *get_subr      (uint);
RBT* get_rbt       (uint, uint);
SYM* get_sym       (uint, uint , uint , uint);
SYMLIST* get_symlist  (uint, uint, uint);
SPF *get_spf       (void*) ;
PSW* get_psw       (uint);
TRK* get_dtk       (uint);
DTD* get_dtkd      (CHAIN *, uint, uint );


TOPC *get_opc(uint addr);
TOPC *add_opc(uint addr);

void chupdate(CHAIN *x, uint ix);

SPF *find_spf(void *fid, uint spf);
int find_tjump (uint);
JMP * find_fjump (uint , int*);

SIG* match_sig     (PAT *, uint);


SIG* inssig(SIG *blk);

void fixbitfield(uint *, uint *, uint *);

//void scan_dc_olaps(void);
//void scan_code_gaps(void);
void scan_cmd_gaps(uint (*pgap) (LBK *s, LBK *n));

uint scan_cgap(LBK *s, LBK *n);
uint scan_fillgap(LBK *s, LBK *n);
//void scan_gaps(void);


void check_dtk_links(void);

//void scan_lpdata(void);

void do_sigproc (SIG* g, SBK* s);


void chdelete(CHAIN *, uint, int);
void clearchain(CHAIN *);



extern CHAIN chjpt;
extern CHAIN chjpf;
extern CHAIN chcmd;
extern CHAIN chaux;
extern CHAIN chadnl;
//extern CHAIN chadcm;
// extern CHAIN chans;
extern CHAIN chbase;
extern CHAIN chscan;
extern CHAIN chsubr;
extern CHAIN chsig;
extern CHAIN chsym;
extern CHAIN chrgst;
extern CHAIN chemul;
extern CHAIN chsbcn;
extern CHAIN chpsw;
extern CHAIN chdtko;
extern CHAIN chdtk;
extern CHAIN chbkf;
extern CHAIN chopcd;


#ifdef XDBGX

extern const char *chntxt[];
extern const char *jtxt[];

uint DBGPRT (uint, const char *, ...);
void DBG_stack (SBK *,FSTK *);
void DBG_rgchain(void);
void DBG_rgstat(RST *);
void DBG_sbk(const char *t, SBK *s);
void DBGPRTFGS(int ,LBK *,LBK *);

void DBG_banks(void);

void DBG_sigs(uint);
void DBG_data(void);

extern  int DBGmcalls;            // malloc calls
extern  int DBGmrels;             // malloc releases
extern  int DBGmaxalloc;          //  TEMP for malloc tracing
extern  int DBGcuralloc;


#endif




#endif

// end of header

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


#define PATWINTP         15    // word table interpolate (for signed/unsigned checks)
#define PATBINTP         14    // byte table interpolate (for signed/unsigned checks)


//void clear_pops(void);


void   DBG_scans();




char *calcfiles(char *fname);
int openfiles(void);
void closefiles(void);

void p_indl (uint fno);   // TEMP

void set_cmdopt(uint x, uint y);
void pp_comment (uint ofst, uint flags);


 void do_listing (void);

void getudir (void);

uint val_bank(uint bk);

CSTR *get_cstr (uint s, uint ix);


HDATA* get_flhdr(void);
FDATA* get_fldata(uint ix);

void* chimem    (uint);

SIG* do_sig (uint pat, uint ofst);

CHAIN *get_chain(uint);
uint get_lastix(uint);
uint get_lasterr(uint);

uint get_cmdopt(uint x);
uint get_anlpass(void);

//SYM *new_autosym (uint ix, int addr);
void add_autosym (uint, uint);

void  show_prog    (void);             //int);

void scan_opcodes(void);
SIG*  scan_sigs    (int);
SIG*  scan_asigs   (int);
void  prescan_sigs (void);
int   do_jumpsig   (int, uint, uint);
void  do_jumps     (void);

uint fmask(uint fstart, uint fend);
void *mem          (void*,size_t, size_t);
void  mfree        (void *, size_t);

void free_structs (void);
//void set_rgsize   (RST *,uint);

int adtchnsize    (void*,  uint);

void* vconvi (uint);

ADT *append_adt     (ADT*);     // (void *, uint);  to allow duplicate checks.
 ADT* append_adt_mult (ADT *x);       //test for adt
int cmp_adt (ADT *a, ADT *b);
LBK *add_cmd      (uint, uint, uint);
LBK *add_aux_cmd  (uint, uint, uint);
RBT *add_rbase    (uint, uint, uint, uint);
// SYM *add_sym       (CSTR *,    uint , uint , uint, uint,  uint);
SUB *add_subr     (SBK *, uint);
SBK *add_scan     (uint, int, SBK *);
SPF *append_spf   (void *, uint, uint);
SBK *add_escan    (uint, SBK *);
JMP *add_jump     (SBK *, int, int);
//RST *add_rgstat   (uint, uint, uint, uint);
PSW *add_psw      (uint, uint);
void add_dtr      (SBK *,INST *);
BNKF *add_bnkf    (uint add, uint pc);

MATHX *add_encode(uint chn, void *fid, uint type, uint reg);

  FKL *add_link(uchar chsce, void *sce, uchar chdst, void *dst, uchar type);

RST *add_rgstat     (uint, uint, uint);
RST* get_next_rgstat(uint reg);

ADT* get_last_adt (void *fid, uint fix);

//ARG* append_arg(uint addr, uint reg);

//POP* add_pop (SBK *, uint);        //, SBK*);
//POP* get_pop (uint,uint) ;

SBK* copy_scanchain(SBK* base, uint addr, uint ch);

void cleantestdata(SBK*);

ADT *get_adt       (void *fid, uint bix);
SBK *get_scan      (uint, uint);
//SBK *get_scansub      (uint,uint);
RST *get_rgstat     (uint);
RST *get_rgstata     (uint);
LBK *get_cmd       (uint, uint);
LBK *get_aux_cmd   (uint, uint);
SBK *get_scan      (uint);

FKL *get_link  (uchar chsce, void *sce, uchar chdst, void *dst, uchar type);

LBK *get_prt_cmd   (LBK *dflt, BANK *b, uint ofst);
//RST* get_rgstat    (uint);
SIG* get_sig       (uint);
uint get_tjmp_ix   (uint);
uint get_fjmp_ix   (uint ch, uint ofst);
JMP *get_fjump     (uint);
JMP* get_tjump     (uint, uint*);
SUB *get_subr      (uint);
RBT* get_rbt       (uint, uint);
SYM* get_sym       (uint, uint , uint , uint);
SYMLIST* get_symlist  (uint, uint, uint);
SPF *get_spf       (void*) ;
PSW* get_psw       (uint);
//TRK* get_dtk       (uint);
DTD* get_dtkd      (uint, uint, uint );


CINST *get_copcode(uint addr);
uint   get_mopcode(INST *dest);
CINST *get_nearcopcode(uint addr);
INST *add_opcode(INST *);
CINST *find_opcode (uint ofst, uint up);

uint get_opcode_ix(uint);
uint valid_ix(uint , uint);
void* get_chitem(uint, uint);
void* get_next_item(uint ch);


void chupdate(uint ch, uint ix);
void chinsert(uint ch, int ix, void *blk);

SPF *find_spf(void *fid, uint spf);
int get_tjump_bkts (uint ofst);
JMP * find_fjump (uint , int*);


SIG* inssig(SIG *blk);

void fixbitfield(uint *, uint *, uint *);

//void clr_rbflg(uint index);
//char get_rbflg(uint index);

//void scan_dc_olaps(void);
//void scan_code_gaps(void);
void scan_cmd_gaps(INST *c, uint (*pgap) (INST *c,LBK *s, LBK *n));

uint scan_cgap(INST *, LBK *s, LBK *n);
uint scan_fillgap(INST *,LBK *s, LBK *n);
//void scan_gaps(void);

// uint check_altscan_chain(void);
//void copy_alt_chains(void);
//void clear_alt_chains(void);

void check_dtk_links(void);

//void scan_lpdata(void);

void do_sigproc (SIG* g, SBK* s);


void chdelete(CHAIN *, uint, int);
void emptychain(uint);

// ________________-new print stuff here...........

uint get_pfwdef(ADT *a);

int pp_subargs (CINST *c) ;

//int decode_addr(ADT *a, uint ofst);
 MHLD *do_adt_calc(ADT *a, uint ofst);

uint xprt (uint, uint, cchar *, ...);
void p_pad(uint, uint);


void list_banks(uint);
void paddr(uint, uint,uint);
void prt_adt(uint, void *, int);          //, uint (*prt) (uint,const char *, ...))
void pchar(uint,char);
uint pbin(uint, uint, uint);
void wrap(uint);
void p_run(uint, uint, char c, uint);

uint prtfl(uint,float, uint, uint);

void prtflgs(uint,uint);

void pp_hdr (int ofst, cchar *com,  int cnt);

//void prt_glo(uint, SUB *);
//void prt_opts(uint);
void list_dirs(uint);
//void list_scans(uint);
//void list_psw(uint);
//void list_cmds(uint);

//-------------------------------------


#ifdef XDBGX

uint DBGPRT (uint, const char *, ...);
void DBG_stack (uint);        //,FSTK *);
void DBG_rgchain(void);
void DBG_rgstat(RST *);
void DBG_sbk(const char *t, SBK *s);
void DBGPRTFGS(int ,LBK *,LBK *);

void DBG_banks(void);
void DBG_rst(void);       //TEMP


void DBG_sigs(uint);
void DBG_data(void);
void DBG_emuargs(void);

#endif




#endif

// end of header
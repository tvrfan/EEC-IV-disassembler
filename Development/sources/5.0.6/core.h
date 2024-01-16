
/*******************************************************************
common declarations for 'core' code modules (disassembly)
**********************************************************************

 * NOTE - This code assumes 'int' is at least 32 bit (4 bytes),
 *
 * On some older compilers this would require the use of LONG instead of INT.
 * code here also uses a lot of pointer casts. Should be fine on modern compiler,
 * but stated here just in case
 *
 * On Win32 builds (Codeblocks via mingw) int=long=void* all at 32 bits.
 * On Linux builds (CodeBlocks via gcc amd64)   int at 32 bits, long=void* at 64 bits.
 *
 * 8065 manual says that bank is an extra 4 bits in CPU, (bits 16-19) to give a 20 bit address (0-0xFFFFF)
 * which could be 1 Mb (1048576), but biggest bins seem to be 256k (= 0x40000), or 4 banks, using 2 bits.
 *
 * Bin addresses used throughout are 20 bit, = bank in 0xf0000 + 16 bit address. Sometimes as unsigned int.
 * This code uses a pointer in a BANK structure to map into each malloced memory block (= bank) for best speed.
 * This design is therefore bank order independent.
 *
 * This code can handle all 16 banks with only changes to bank setup subroutines required (detect and check).
 * Apart from this, all code uses bank+address everywhere, but not tested beyond 4 banks
 *
 * NB. Changed above to store (bank+1) internally, so that 'bank present' checks are much easier, which
 * means that code now would support 15 banks, as 1-15 internally or 0-14 in bin terms, but only 0,1,8,9
 * are used, and are therefore internally mapped to 1,2,9,10
 *********************************************************/

#ifndef _XCOREX_H
#define _XCOREX_H 1

#include  "shared.h"


#define EMUGSZ    5                  // size of emulate arguments list



/* loop struct ??

typedef struct loop
{
uint  start      : 20;         // start address
uint  end        : 20;         // end address
//uint  lastimd    : 20;         // last immediate assigned. NO !!
uint  incr       : 4;          // at least one incr (count)
uint incr2       : 4;          // for inc at same address (diouble check)
uint  inrx       : 1;          // at least one inx or inr
uint  increg     : 10;         // register incremented
int   sval ;
uint datastart   : 20;

} LOOP;
*/


// structure finder (tabs and funcs)
// argument holder for branch analysis subrs

typedef struct  sfx
 {
   SPF* spf;
   uint datbank;
   uint ans[2];
   uint rg [2];               // also in spf....
   uint lst [2];
 }  SFIND;





/**********************************************************************************
external subroutines
***********************************************************************************/
void chupdate(uint ch, uint ix);              //temp

void *mem          (void*,size_t, size_t);
void *cmem         (void*,size_t, size_t);
void  mfree        (void *, size_t);
void  free_structs (void);
void *chmem        (uint, uint);

HDATA  *get_fldata   (void);

ADT*   get_adt         (void *, uint );

ADT*   get_last_adt    (void *fid);

SBK*   get_scan       (uint,uint);
uint   get_scan_range (uint,uint);

SUB*   get_subr       (uint);
RST*   get_rgstat     (uint);
RPOP*   get_rgpop     (uint);
LBK*   get_cmd        (uint, uint, uint);

RBT*   get_rbase        (uint, uint);

SIG*   get_sig        (uint);
SPF*   get_spf        (void*);
SPF*   find_spf       (void *, uint, uint) ;

uint   get_tjmp_ix    (uint);
uint   get_lastix     (uint);
uint   get_lasterr      (uint);
uint   get_chnum         (uint );
CHAIN* get_chain      (uint);
void*  get_next_item   (uint, uint*);
void*  get_item        (uint,uint);
cchar *get_cmd_str      (uint cmd);

FKL *get_link         (void *, void *, uint);

STACK * get_stack(SBK *s, uint ptr);           //, SBK **ans, uint *psp);
void put_stack(SBK *s, uint type, uint addr, uint f);     //SBK *blk, uint psp);
uint cnt_levels(SBK*, uint, SBK*);

void fake_push(SBK *s, uint type, SBK *newblk);
uint fake_pop(SBK *s, OPS *o);

uint find_scan_olaps(uint addr);
uint find_argblk(SBK *, uint, OPS *);

void set_rgsize   (RST *,uint);
uint get_cmdopt   (uint);
void set_cmdopt   (uint);

SIG*  scan_asigs  (int);
SIG*  scan_sigs   (int);

void do_sigproc   (SIG* , SBK* );

int  adn_totsize  (void *);

ADT *add_adt      (void *, uint);
int  add_args     (LBK *, int , int );
LBK *add_aux_cmd  (uint, uint, uint, uint);
LBK *add_cmd      (uint, uint, uint, uint);
SBK *add_escan    (uint, SBK *);
JMP *add_jump     (SBK *, uint, int);
RBT *add_rbase    (uint, uint, uint, uint);
SBK *add_scan     (uint, uint, uint, int, SBK *);
SUB *add_subr     (uint);
TRK *add_dtk      (SBK *,INST *);
BNKF *add_bnkf    (uint , uint);

FKL *add_link     (uchar chsce, void *sce,  uchar chdst, void *dst, uchar type);

SPF *add_spf      (void *, uint, uint);
SYM *add_sym      (cchar *, uint, uint, uint, uint, uint);

MATHN *add_encode(uint chn, void *fid,uint type, uint reg);

MATHX* add_smterm (void *, uint, MNUM *, MNUM *);

MHLD* do_tcalc(void *fid, int pix, ADT *a, int ofst) ;

void new_symname (SYM *xsym, cchar *fnam);

SPF* copy_spf(void *, SPF * , int );
void copy_adt_chain(void *, void *);

int adtchnsize(void *fid, uint nl);

void reconcile_emuscans(void);

void cleareptrs(void);

void *vconv(uint address);

void  prescan_sigs (void);
SIG*  do_sig     (uint, uint);

void init_prt(void);
void prt_dirs(void);
void prt_listing(void);
void prt_banks(uint (*prt) (uint,cchar *, ...));

uint pstr (uint, cchar *, ...);

uint wnprt (uint, cchar *, ...);
void set_pfwdef    (ADT *);

void set_scan_banks(SBK *, SBK *);

int getudir (int);

void do_dtk_links(void);
void do_jumps (void);


void chdelete(uint, uint, uint);
void clearchain(uint);

void  show_prog    (int);
void closefiles(void);
void openfiles(void);


void turn_dtk_into_data(void);
void turn_scans_into_code(void);

void scan_gaps(void);

#ifdef XDBGX

void DBG_init     ();
uint DBGPRT       (uint, cchar *, ...);
//void DBG_stack    (SBK *, FSTK *);
void DBG_rgchain  (void);
void DBG_sbk      (cchar *t, SBK *s);
void DBGLBK       (int, LBK *, cchar *, ...);
void DBG_sigs     (void);
void DBG_data     (void);
void DBG_STK      (uint,SBK *);
#endif



#endif

// end of header

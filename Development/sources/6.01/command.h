

#ifndef _XCMND_H

#define _XCMND_H 1

#include "shared.h"


#include  "shared.h"


void* mem(void *ptr, size_t osize, size_t nsize);

CHAIN *get_chain(uint chn);

HDATA* get_flhdr(void);

void show_prog (void);

int g_byte (uint addr);

FDATA* get_fldata(uint ix);
BANK *get_bankmap(uint ix);
BASP *get_basepars(void);
uint get_anlpass(void);

MATHN *get_mname(void *fid, CSTR *name);
uint get_lasterr(uint ch);
uint get_lastix(uint ch);

uint get_pfwdef(ADT *a);

ADT *get_adt(void *fid, uint bix);

uint bytes(uint fend);
uint valid_reg(uint reg);

void chupdate(uint ch, uint ix);
void chdelete(CHAIN *x, uint ix, int num);


BANK *mapbank(uint addr);

int get_numbanks(void);
void set_numbanks(int n);


void *vconvi(uint addr);


uint val_rom_addr(uint addr);

int g_val (uint addr, uint fstart, uint fend);
uint g_word (uint addr);


int totsize(void *x);
int listsize(void *x);

uint get_startval(uint fend);

MATHX *get_mterm(void *fid, int pix);


void *chimem(uint ch);

void add_iname(int from, int ofst);
RBT *add_rbase(uint reg, uint addr, uint rstart, uint rend);
MATHN *add_mname(void *fid, CSTR *name);
FKL *add_link(uchar chsce, void *sce, uchar chdst, void *dst, uchar type);


MATHX* add_mterms (MATHX *a);
LBK* add_cmd(uint start, uint end, uint com);

SYM *add_sym  (CSTR *fnam, uint add, uint fstart, uint fend, uint rstart, uint rend);
SUB *add_subr (SBK *s,uint addr);
 SBK *add_scan (uint add, int type, SBK *caller);
 LBK* add_aux_cmd (uint start, uint end, uint com);
 PSW *add_psw(uint jstart, uint pswaddr);


ADT* append_adt (ADT *);          //void *fid, uint x);
SPF* append_spf (void *fid, uint spf, uint from);



void p_pad(uint fno, uint c);
void pchar(uint fno, char z);
uint xprt (uint fno, uint nl, cchar *fmt, ...);
void prtcmdopts(uint fno, uint flgs);
void paddr(uint fno, uint addr,uint pfw);


uint DBGPRT (uint nl, cchar *fmt, ...);
void DBG_adt (ADT *, uint);

#endif
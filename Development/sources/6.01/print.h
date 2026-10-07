


#ifndef _XPRINT_H

#define _XPRINT_H 1

#include "shared.h"





#include  "shared.h"

void* mem(void *ptr, size_t osize, size_t nsize);
void* cmem(void *ptr, size_t osize, size_t nsize);

void mfree(void *addr, size_t size);
void show_prog (void);

FDATA* get_fldata(uint ix);
HDATA *get_flhdr(void);

uint get_numbanks(void);
DIRS *get_dirs(uint ix);

BANK *get_bankmap(uint bank);
uint get_cmdopt(uint x);

CHAIN *get_chain(uint chn);

MXF * get_mathfunc(uint ix);

void* get_next_item(uint ch);

int cellsize(ADT *a);

uint valid_reg(uint reg);

cchar *get_spfstr (uint ix);

CSINX *get_strinx(uint ix);
CSTR *get_cstr (uint s, uint ix);

ADT *get_adt(void *fid, uint bix);
SPF *get_spf(void* fid);
PSW *get_psw(uint addr);

SYM* get_sym(uint add, uint fstart, uint fend, uint pc);
SUB *get_subr(uint addr);

FKL *get_link (uchar chsce, void *sce, uchar chdst, void *dst, uchar type) ;
MATHX *get_mterm(void *fid, int pix);

void *vconvi(uint addr);

RBT* get_rbt(uint reg, uint pc);

LBK *get_aux_cmd (uint start, uint fcom);

LBK *get_prt_cmd(LBK *dflt, BANK *b, uint ofst);
SYMLIST * get_symlist(uint add, uint fend, uint pc);

CINST *get_copcode(uint addr) ;
uint get_mopcode(INST *dest);


CINST *find_opcode (uint ofst, uint up);

JMP* get_tjump(uint ofst, uint *rix);
JMP * find_fjump (uint ofst, int *x);
int get_tjump_bkts (uint ofst);

int g_byte (uint addr);
uint g_word (uint addr);
int g_val (uint addr, uint fstart, uint fend);

int scale_val (int val, uint fstart, uint fend);

uint bytes(uint fend);

uint fmask(uint fstart, uint fend);

const OPC* get_opc_entry(uchar ix);

int getpx(CPS *c, int ix, int l);
int getpd(CPS *c, int ix, int limit);
int readpunc(CPS *c);
uint fix_input_addr_bank(CPS *c, int ix);

uint get_sizemask(uint fend);
int get_cmnt (CPS *c);


int listsize(void *x);
int totsize(void *x);


uint DBGPRT (uint nl, cchar *fmt, ...);

#endif
#ifndef _XPRINTX_H

#define _XPRINTX_H 1

#include "shared.h"


cchar*  get_mathfname     (uint);

uint    codebank          (uint, INST *);
uint    databank          (uint, INST *);
uint    get_anlpass       (void);
const OPC* get_opc_entry  (uchar);
cchar  *get_cmd_str       (uint);
cchar  * get_spfstr      (uint);

CHAIN*  get_chain         (uint);
void *  chmem              (uint,uint);

uint fmask(uint fstart, uint fend);

MXF * get_mathfunc(uint ix);
void    show_prog    (int);

int     g_byte       (uint);
uint    g_word       (uint);
int     g_val        (uint, uint, uint);

uint is_special_reg  (uint);

uint get_sizemask    (uint);
uint bytes           (uint);

uint zero_reg        (OPS *);
uint valid_reg       (uint);
uint val_rom_addr    (uint);

void *vconv(uint addr);

SPF *get_spf         (void *) ;
SPF* get_next_spf    (SPF *);
uint get_flag        (uint , uint *);
uint get_pfw         (ADT *);
SYM *get_sym         (uint, uint, uint, uint);
SYM **get_syms       (uint, uint,uint, uint);
ADT *get_adt         (void *,uint);

FILE *get_file_handle    (uint);
HDATA *get_fldata      (void);
SBK  * get_scan        (uint, uint);

RST *get_rgstat      (uint);
SUB *get_subr        (uint);
LBK *get_prt_cmd     (LBK *, BANK *, uint);
LBK *get_cmd         (uint, uint,uint);
PSW* get_psw         (uint);
JMP *get_fjump       (uint);
RBT* get_rbase         (uint, uint);
uint get_cmdopt      (uint);
uint get_cmdoptstr   (uint, uint, cchar **);
uchar get_ftype      (uint);
uint get_numbanks    (void);
BANK* get_bkdata     (uint);

int getpx            (CPS *c, int ix, int limit);
int getpd            (CPS *c, int ix, int limit);
int readpunc         (CPS *c);

uint fix_input_addr_bank  (CPS *c, int ix);

DIRS*  get_cmd_def   (uint);

void set_scan_banks  (SBK *, SBK*);

FKL   *get_link      (void *, void *, uint);
MATHX *get_mterm     (void *, int);


uint find_opcode     (int, uint, const OPC **);
int find_tjump       (uint);
JMP * find_fjump     (uint , int*);


int cellsize         (ADT *);
int adn_totsize      (void *fid);
int adn_listsize     (void *fid);       //, int seq);

void fixbitfield      (uint*, uint*, uint*);
void do_one_opcode    (uint, SBK*, INST *);
void do_code          (SBK *, INST *);

void clr_inst         (INST*);

ADT *add_adt      (void *, uint);

void* mem          (void *, size_t, size_t);              //TEMP !!
void mfree         (void *, size_t);


#ifdef XDBGX

uint DBGPRT (uint, cchar *, ...);
void DBGLBK(int , LBK *, cchar *, ...);

#endif

#endif





#ifndef _XCHAINX_H
#define _XCHAINX_H 1

#include  "shared.h"



HDATA  *get_fldata(void);
BANK   *get_bkdata(uint);

int     g_byte (uint);
int     g_word (uint);
int     g_val (uint, uint, uint);

uint    get_flag (uint, uint *);
uint    get_cmdopt(uint);
cchar  *get_cmd_str(uint cmd);
uint    get_anlpass();
uint    get_pn_opstart (uint, int);

uchar   get_ftype (uint);

RST    *get_rgstat(uint);
RST    *add_rgstat(uint);

//RPOP   *get_rgpop(uint);
RPOP*   get_rpop(uint);
//void add_pop_reg(uint reg);

MXF    *get_mathfunc(uint);

DIRS   *get_cmd_def(uint cmd);
cchar  *get_mathfname(uint ix);

int    get_numbanks(void);
uint   get_opdata(uint);
AUTON * get_autonames(void);

uint   wnprt (uint, cchar *, ...);

uint   val_rom_addr(uint);
uint   valid_reg(uint);

uint   codebank(uint, INST *);
uint   databank(uint, INST *);

uint   maxromadd (uint);
uint   minromadd (uint);

uint   bytes(uint);

int    cellsize(ADT *);

//uint get_signmask(uint);

SYM   *new_autosym (uint, uint);

int    check_sfill(uint, uint);

uint   find_opcode     (int d, uint xofst, const OPC **opl);
void   do_one_opcode   (uint, SBK*, INST *);


void   scan_blk(SBK*, INST*);
void   scan_loop(SBK *, INST *, uint);

void set_ftype(uint, uchar);
cchar *get_jtext(uint);
cchar  *get_chn_name     (uint);


 #ifdef XDBGX

  uint DBGPRT     (uint, cchar *, ...);
  void DBGPRTFGS  (int ,LBK *,LBK *);
  void DBGLBK     (int, LBK *, cchar *, ...);
  void DBG_patt   (uint*);
  void DBG_sbk    (cchar*, SBK*);

#endif

















#endif

#ifndef _XCMNDX_H

#define _XCMNDX_H 1


#include "shared.h"




/********************************
 * Interrupt subr sym names structure
 ********************************/

typedef const struct
{
  uint  start : 8;     // start offset
  uint  end   : 8;     // end (0 - 0x5f)
  uint  p5    : 1;     // 8061 or 8065
  uint  num   : 1;     // with auto number
  cchar *name;
} INAMES;



/********************************
 * Preset sym names structure
 ********************************/

 //need access mode in here too for 8065.
 // fend - size as field (0-31), sign (+32), write (+64)

typedef const struct
{
 // uint wrt    : 1;       // write symbol
  uint fstart : 3;       // start bit number (0-7)
  uint fend   : 7;       // end bit number
  uint addr  : 20;       // address
  uint no_olap : 1;      // SFR word does not overlap into next byte
  cchar *symname;        // name

} DFSYM;



uint     wnprt(uint, cchar *, ...);
uint     maxadd (uint );
SIG*     match_sig     (uint, uint,uint);

void*    mem(void *ptr, size_t osize, size_t nsize);

int      g_val (uint, uint, uint);
int      g_byte(uint);

FILE     *get_file_handle(uint);

HDATA    *get_fldata(void);

cchar    *get_cmd_str(uint);

uint     get_startval(uint);

FKL     *get_link (void *, void *)   ;


RBT *add_rbase    (uint, uint, uint, uint);

void prtcopts     (uint (*) (uint,cchar *, ...));
void prt_adt      (void *, int, uint (*) (uint,cchar *, ...));

void prtaddr      (uint, uint, uint (*) (uint,cchar *, ...) );
uint paddr        (uint);

ADT *get_adt        (void *, uint);                      //, uint);

uint get_lasterr    (uint ch);
void clear_lasterr  (uint);

CHAIN *get_chain     (uint);
BANK  *get_bkdata    (uint);
AUTON *get_autonames (void);


ADT *add_adt      (void *,uint);
LBK* add_cmd      (uint, uint, uint, uint);
SPF* add_spf      (void *, uint, uint);
SBK *add_scan     (uint,uint, uint, int, SBK *);
LBK* add_aux_cmd  (uint start, uint end, uint com, uint from);
PSW *add_psw      (uint jstart, uint pswaddr);
FKL *add_link     (uchar chsce, void *sce, uchar chdst, void *dst, uchar type);

MATHN *add_mname        (void *, cchar *);
MATHN *add_sys_mname    (void *, uint);
MATHN *get_mname        (void *,char *);

MATHX* add_mterm  (void *, int);
MATHX* get_mterm  (void *, int);

void new_symname  (SYM *, cchar *);

int get_numbanks(void);
void set_numbanks(int);

uint val_rom_addr(uint addr);

uint bytes(uint);

SYM *add_sym (cchar *, uint, uint, uint, uint, uint);
SUB * add_subr (uint);

int adn_totsize(void *);

void chdelete(uint ch, uint ix, uint num);

void *vconv(uint addr);

uint valid_reg(uint);

uint fix_addr_bank(uint addr) ;
void show_prog (int );
uint get_lastix(uint ch);

void chupdate(uint ch, uint ix);

uint g_word (uint);

int cellsize(ADT *a);

#ifdef XDBGX
uint DBGPRT (uint, cchar *, ...);
#endif

#endif
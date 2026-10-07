
#include  "core.h"



extern  cchar *chntxt[];                //TEMP
FDATA* get_fldata   (uint);
DIRS *get_dirs      (uint ix);
OPC*  get_opc_entry  (uchar);
uint get_cmdopt     (uint x);
uint get_anlpass    (void);


uint scan_blk(SBK*, INST*);

//void scan_loop(SBK *, OPCDH *, uint);

uint get_flag (uint, uint *);

uint xprt(uint, uint, const char *, ...);

uint val_rom_addr(uint);
uint valid_reg(uint);
uint get_opdata(uint);
uint codebank(uint, CINST *);
uint databank(uint, CINST *);


uint maxadd (uint);
uint minadd (uint);
int g_byte (uint addr);
int g_word (uint addr);
int g_val (uint, uint, uint);
uint bytes(uint);

int cellsize(ADT *);

uint get_signmask(uint);

uint fix_input_addr(uint);

int new_symname (SYM *, CSTR *);

//SYM *new_autosym (uint, int);

void add_autosym (uint, uint);

int check_sfill(uint addr);

uchar* get_signame(uint patno);

void do_one_opcode( uint xofst);

uint max_reg(void);
uint val_stack_reg(uint);

#ifdef XDBGX

extern  int DBGrecurse;
extern  int DBGnumops;

extern const char *jtxt[];

  uint DBGPRT (uint, const char *, ...);
  void DBGPRTFLGS(int ,LBK *,LBK *);
  void DBGLBK(uint, LBK *);


void DBG_chn(uint ch);


#endif
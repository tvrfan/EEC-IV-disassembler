
#include  "core.h"

extern HDATA fldata;             // files data holder (in shared.h)
extern DIRS dirs[];

extern BASP basepars;            // min and max ram register address (for checking)
extern BANK bkmap[];
extern OPC opctbl[];
extern uchar opcind[];

extern int anlpass;
extern int cmdopts;
extern int numbanks;
extern uint lowscan;

extern CSTR cmds[];

extern INST cinst;
extern INST sinst;

void scan_blk(SBK*, INST*);

void scan_loop(SBK *, INST *, uint);

uint get_flag (uint, uint *);

uint wnprt (uint, const char *, ...);

uint val_rom_addr(uint);
uint val_general_reg(uint);
uint is_special_reg(uint);
uint get_opdata(uint);
uint codebank(uint, INST *);
uint databank(uint, INST *);


uint maxadd (uint);
uint minadd (uint);
int g_byte (uint addr);
int g_word (uint addr);
int g_val (uint, uint, uint);
uint bytes(uint);

int cellsize(ADT *);

uint get_signmask(uint);

uint fix_input_addr(uint);

int new_symname (SYM *, const char *);

SYM *new_autosym (uint, int);

int check_sfill(uint addr);

uint find_opcode(int d, uint xofst, OPC **opl);
void do_one_opcode( uint xofst);

uint max_reg(void);
uint val_stack_reg(uint);

#ifdef XDBGX

extern  int DBGrecurse;
extern  int DBGnumops;

extern  int DBGmcalls;             // malloc calls
extern  int DBGmrels;              // malloc releases
extern  int DBGmaxalloc;           //  TEMP for malloc tracing
extern  int DBGcuralloc;

extern const char *jtxt[];
  uint DBGPRT (uint, const char *, ...);
  void DBGPRTFGS(int ,LBK *,LBK *);
  void DBGPBK(int, LBK *, const char *, ...);

#endif
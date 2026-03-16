
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version 4.0.6
 ******************************************************
 * BUILD NOTES
 * This code assumes INT and UNSIGNED INT are at least 32 bits (4 bytes).
 * and WILL NOT WORK with 16 bit compilers.
 * sizes (in bits) for information for a different platform
 *
 * OS     compiler     int     float   long   void*    double
 * Win32  mingw        32      32      32     32       64
 * Linux  amd64-gcc    32      32      64     64       64
 *
 * This code built with CodeBlocks and/or CodeLite IDE environments
 * on both Win and Linux
 *
 ******************************************************
 *  Declarations and includes
 *******************************************************/

//#include  <time.h>             // if printing date and time

#include  "core.h"             // this calls shared.h
#include  "sign.h"


/**********************************************************************************
external subroutines, variables
***********************************************************************************/

extern PAT intign;                // sig 'ignore interrupt'  (for naming)
extern PAT hdr;                   // sig 'start jump' (and vects);
extern PAT fnlp;
extern PAT tblp;

void  show_prog    (int);
SIG*  find_sig     (PAT *, int);
SIG*  scan_sigs    (int);
SIG*  scan_asigs   (int);
void  prescan_sigs (void);
int   do_jumpsig   (int, int);
void  do_jumps(void);

void *mem(void*,size_t, size_t);
void mfree(void *, size_t);

SBK *get_scan(uint, uint);
RST* find_rgstat(uint);
void free_structs(void);
void set_rgsize(uint, uint);

int adtchnsize(void *, int, int);

ADT *add_adt      (CHAIN *x, void *fid, int seq);
LBK *add_data_cmd (uint start, uint end, uint com, uint from);
LBK *add_code_cmd (uint start, uint end, uint com);
RBT *add_rbase    (uint reg, uint addr, uint start, uint end);
SYM *add_sym      (int rw, const char *fnam, uint add, int bitno, uint start, uint end);
SUB *add_subr     (int addr);
SBK *add_scan     (uint add, int type, SBK *caller);
SPF *add_spf      (void *fid)   ;
SBK *add_escan    (uint add, SBK *caller);
JMP *add_jump     (SBK *s, int to, int type);
RST *add_rgstat   (uint reg, uint ssize);
PSW *add_psw      (uint jstart, uint pswaddr);


SIG* inssig(SIG *blk);
void *find(CHAIN *x, void *b);

RST * find_lowest_rgarg(uint reg);
LBK *find_data_cmd (int start, int fcom);
SBK *find_scan(uint addr);
uint find_tjix(uint ofst);
LBK *find_prt_cmd(LBK *dflt, BANK *b, uint ofst);

void* chmem(CHAIN *);
void* schmem(void);

SUB *get_subr      (int addr);
RBT* get_rbt       (int reg, int pc);
SYM* get_sym       (int rw, uint add, int bitno, int pc);
ADT *get_adnl      (CHAIN *x, void *fid, int seq);
ADT *get_next_adnl (CHAIN *, ADT *);
SPF *get_spf       (void *fid)    ;
PSW* get_psw(int addr);


void scan_dc_olaps(void);
void scan_code_gaps(void);

void free_adnl(CHAIN *,void *);
ADT *set_adnl(ADT* a);

void chdelete(CHAIN *x, uint ix, int num);
void clearchain(CHAIN *);

extern CHAIN chjpt;
extern CHAIN chjpf;
extern CHAIN chdata;
extern CHAIN chcode;
extern CHAIN chadnl;
extern CHAIN chans;
extern CHAIN chbase;
extern CHAIN chscan;
extern CHAIN chsubr;
extern CHAIN chsig;
extern CHAIN chsym;
extern CHAIN chrgst;
extern CHAIN chemul;
extern CHAIN chsbcn;
extern CHAIN chpsw;


#ifdef XDBGX

extern const char *chntxt[];

extern const char *jtxt[];

int  DBGPRT (int, const char *, ...);
void DBGPBK(int, LBK *, const char *, ...);
void DBG_stack (FSTK *);
void DBG_rgchain(void);
void DBG_rgstat(RST *);
void DBG_sbk(const char *t, SBK *s);
void DBGPRTFGS(int ,LBK *,LBK *);

void DBG_sigs(CHAIN*);
void DBG_data(void);

extern  int DBGmcalls;            // malloc calls
extern  int DBGmrels;            // malloc releases
extern  int DBGmaxalloc;           //  TEMP for malloc tracing
extern  int DBGcuralloc;








#endif


/**********************************************************************************
local global declarations
***********************************************************************************/

void scan_blk(SBK *, INST *);    // used in several places
void do_code (SBK *, INST *);



const char *empty = "";                                    // gets around compiler warnings with ""





//for lasterr errors
const char *etxts[] = {empty, "\002Duplicate Command", "\001Invalid Address","\001Banks Not Match", "\001Odd address\003",
   "\001Commands\006", "\001Ranges Overlap","\002XCODE bans Command", "\002Symname replaces previous \"%s\"" 

 };

//for compressing common strings
const char *cmptxts[16] =
 {empty, " Error - "," Warning - ", " Invalid"," Required"," Not Allowed"," Overlap"," Missing", " Ignored" ,         // use \001 upwards ??
 //  0     1           2              3            4          5             6            7          10        (OCTAL)
 " Parameters", "Data Item(s)", empty,empty,empty,empty,empty   };
//      11,        12          13-15

/*****************************************************************************
* declarations for subroutines in command structure declaration
***************************************************************************/

// print processors

uint pp_dflt  (uint, LBK *); uint pp_wdbl  (uint, LBK *); uint pp_text  (uint, LBK *);
uint pp_vect  (uint, LBK *); uint pp_code  (uint, LBK *); uint pp_stct  (uint, LBK *);
uint pp_timer (uint, LBK *); uint pp_dmy   (uint, LBK *);


// command processors

uint set_vect (CPS*); uint set_code (CPS*); uint set_scan (CPS*); uint set_cdih (CPS*);
uint set_args (CPS*); uint set_data (CPS*); uint set_rbas (CPS*); uint set_sym  (CPS*);
uint set_subr (CPS*); uint set_bnk  (CPS*); uint set_time (CPS*); uint set_tab  (CPS*);
uint set_func (CPS*); uint set_stct (CPS*); uint set_opts (CPS*); uint clr_opts (CPS*);
uint psw_set  (CPS*);
/********** COMMAND Option letters -
*
* Global opts
  A   Args mode print (one per line)  default for ARGS and subrs
  C   Compact mode print default for all data structs 
  F   special Function (subr)    
  Q   Quit, terminator byte(s) 1-3
  
* Standard, data items

 =   Answer definition ?
 A
 B   Bit symbol (in sym) 
 C               Chunk/Cell ?? subword fields ?
 D   Displacement - Data offset
 E   Encoded address
 F   Flags word or byte (symbol)    
 G  
 H 
 I
 J
 K   banK  replace bank where applicable
 L   Long
 M         [ mask ? ](byte or word pair) not done yet
 N   Name lookup, find symbol
 O   Count (cols etc)
 P   Print fieldwidth
 Q   
 R   Reference pointer (vector) in struct
 S   Signed
 T                   biT symbol (deprecated) Type ?? [ text, mS, Volts, A/d Time/mS/scale etc ?]
 U   Unsigned 
 V   diVisor - Float value
 W   Word,  Write (symbol)
 X   Swop Decimal print
 Y   bYte
 Z
 ***************************************************************


 *********************
 main command structure -
 params are -


  merge is with same cmd, override is score, higher wins

******/


// NEW command "pswdep" addr addr  for where the PSW is set for a cond jump.

// string options for SETOPTS and CLROPTS

const char *optstrs[14] =

{ "default"    ,"sceprt", "tabnames" , "funcnames", "ssubnames" , "8065" ,
  "labelnames" ,"manual", "intrnames", "subnames" , "signatures", "acomments",
  "sympresets" ,"8065D"};

struct zxz
{
char key;
int  copt;
} opts [14] = {
 {0   ,0x1C7   },     // default
 {'C' ,1       },     // sceprt  print source code
 {'T' ,2       },     // auto table names
 {'F' ,0x200   },     // auto func names
 {'G' ,4       },     // auto special func subnames
 {'H' ,8       },     // set 8065
 {'L' ,0x10    },     // auto labelnames
 {'M' ,0x20    },     // full manual operation
 {'N' ,0x40    },     // auto interrupt handler names
 {'P' ,0x80    },     // auto subrotuine names
 {'S' ,0x100   },     // signatures
 {0   , 0x400  },     // do autocomments
 {0   , 0x800  },     // preset symbols
 {0   ,0x1008   }     // set 8065 Step D
 
};



// what about decimal versus hex prints for each data type ???
// other dfault layouts and types
// symbol types and print options ?

// default compact/args layout ??
//  one for sturcts/tabs/funcs  default C
//  one for subrs dafault A


//special function strings for subroutines

const char *spfstrs[13] =

{ empty    ,"uuyflu", "usyflu" , "suyflu", "ssyflu" ,
            "uuwflu", "uswflu" , "suwflu", "sswflu" ,
            "uytlu" , "sytlu"
};











// NEW command "pswdep" addr addr  for where the PSW is set for a cond jump.

const char *cmds[22] =

{"fill", "byte", "word", "long", "text", "vect", "args", "table","func","struct",
 "timer", "code", "xcode", "subr", "scan", "opts", "rbase","symbol", "bank", "setopt", "clropt", "pswset"};




// Params -
// command processor, command printer, max numbers expected (addresses),
// single start address posn, pair start addr posn,  (7 = none)
// name allowed,  max addnl levels, min addnl levels, default size,
//  override score, merge allowed, default to decimal, options string (scanf)


DIRS dirs[22] = {

{ set_data, pp_dflt,  2, 7, 0, 0,    0,  0, 1,   0, 1, 0, 0, 0    },                      // fill    ,default fill  -> MUST be zero ix
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 1,   2, 0, 0, 0, "%1[:PSUXV]%n" },            // byte    ,group 1,  mergeable, lowest prio
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 2,   2, 0, 0, 0, "%1[:PSUXV]%n" },            // word    ,temp take away merge
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 3,   2, 0, 0, 0, "%1[:PSUXV]%n" },            // long    ,
{ set_data, pp_text,  2, 7, 0, 0,    0,  0, 1,   1, 1, 0, 0, 0  } ,                       // text    ,
{ set_vect, pp_vect,  2, 7, 0, 1,    1,  0, 2,   3, 0, 0, 0, "%1[:DKQ]%n"},               // vect    ,but must check bank !!
{ set_args, pp_dflt,  2, 7, 0, 0,    15, 1, 0,   5, 0, 0, 0, "%1[:|DELNOPSUVWXY]%n" },     // args    ,not mergeable without ADT handling....

{ set_tab,  pp_stct,  2, 7, 0, 1,    1,  1, 0,   4, 0, 1, 0, "%1[:OPSUVWXY]%n" },         // table   ,data structures, can override group 1
{ set_func, pp_stct,  2, 7, 0, 1,    2,  2, 0,   4, 0, 1, 0, "%1[:LPSUVWXY]%n" } ,        // func    ,NOT mergeable
{ set_stct, pp_stct,  2, 7, 0, 1,    15, 1, 0,   4, 0, 0, "%1[QAC]%n", "%1[:|DELNOPRSUVWXY]%n" },  // struct  ,
{ set_time, pp_timer, 2, 7, 0, 1,    2,  0, 0,   4, 0, 0, 0, "%1[:NTUWY]%n" },            // timer   ,

{ set_code, pp_code,  2, 7, 0, 0,    0,  0, 0,   0, 1, 0, 0, 0  } ,                       // code    ,separate CODE command chain, mergeable
{ set_cdih, pp_dmy,   2, 7, 0, 0,    0,  0, 0,   1, 1, 0, 0, 0  } ,                       // xcode   ,

{ set_subr, pp_dmy,   1, 0, 7, 1,    15, 0, 0,   0, 0, 0, "%1[ACF]", "%1[:|DELNOPSUVWXY=]%n" },    // subr    ,others
{ set_scan, pp_dmy ,  1, 0, 7, 0,    0,  0, 0,   0, 0, 0, 0, 0   },                       // scan    ,
{ set_opts, pp_dmy,   0, 7, 7, 0,    1,  0, 0,   0, 0, 0, 0, "%1[:CFGHLMNPST]%n" },       // opts    old style - deprecated
{ set_rbas, pp_dmy,   4, 1, 2, 0,    0,  0, 0,   0, 0, 0, 0, 0   },                       // rbase   ,has a start AND an address pair

{ set_sym,  pp_dmy,   3, 0, 1, 1,    1,  0, 0,   0, 0, 0, 0, "%1[:BFITW]%n"    },         // sym     ,flags,bit (B and T),write
{ set_bnk,  pp_dmy,   4, 7, 7, 0,    0,  0, 0,   0, 0, 0, 0, 0 },                         // bank    ,

{ set_opts, pp_dmy,   0, 7, 7, 0,    1,  0, 0,   0, 0, 0, 0, "\1" },                       // set  NEW style with strings
{ clr_opts, pp_dmy,   0, 7, 7, 0,    1,  0, 0,   0, 0, 0, 0, "\1" },                       // clear  NEW style with strings
{ psw_set,  pp_dmy,   2, 0, 7, 0,    0,  0, 0,   0, 0, 0, 0, 0    }                        // psw setter for '0=0' jumps
};






/**********************
 *  opcode index and definition table
 * index for opcodes first - points into main opcode table - 0 is invalid
 * reordered to put main opcode types together.
 * **********************/

#define LDWIX  41          // for 3 op src replacements, "x = y" - print as LDW
#define JMPIX  75          // for 'goto X' jump output (for conditionals)


uchar opcind[256] =
{
// 0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
   80,  89,  96,  91,  0,   98, 100,  93,  4,   1,   5,   0,   6,   3,   7,   94,    // 00 - 0f
// skip,clrw,cplw,negw,     decw,sexw,incw,shrw,shlw,asrw,     shrd,shld,asrd,norm
   112, 88,  95,  90,  0,   97,  99,  92,  8,   2,   9,   0,   0,   0,   0,   0,     // 10 - 1f
// RBNK,CLRB,CPLB,NEGB,----,DECB,SEXB,INCB,SHRB,SHLB,ASRB
   81,  81,  81,  81,  81,  81,  81,  81,  82,  82,  82,  82,  82,  82,  82,  82,    // 20 - 2f
// sjmp                                    scall
   70,  70,  70,  70,  70,  70,  70,  70,  71,  71,  71,  71,  71,  71,  71,  71,    // 30 - 3f
// jnb                                     jb
   13,  13,  13,  13,  25,  25,  25,  25,  29,  29,  29,  29,  20,  20,  20,  20,    // 40 - 4f
// AN3W                AD3W                SB3W                ML3W
   12,  12,  12,  12,  24,  24,  24,  24,  28,  28,  28,  28,  18,  18,  18,  18,    // 50 - 5f
// AN3B                AD3B                SB3B                ML3B
   11,  11,  11,  11,  23,  23,  23,  23,  27,  27,  27,  27,  16,  16,  16,  16,    // 60 - 6f
// AN2W                AD2W                SB2W                ML2W
   10,  10,  10,  10,  22,  22,  22,  22,  26,  26,  26,  26,  14,  14,  14,  14,    // 70 - 7f
// AN2B                AD2B                SB2B                ML2B
   31,  31,  31,  31,  33,  33,  33,  33,  35,  35,  35,  35,  38,  38,  38,  38,    // 80 - 8f
// ORW                 XRW                 CMPW                DIVW
   30,  30,  30,  30,  32,  32,  32,  32,  34,  34,  34,  34,  36,  36,  36,  36,    // 90 - 9f
// ORB                 XORB                CMPB                DIVB
   41,  41,  41,  41,  43,  43,  43,  43,  45,  45,  45,  45,  46,  46,  46,  46,    // a0 - af
// LDW                 ADCW                SBBW                LDZBW
   40,  40,  40,  40,  42,  42,  42,  42,  44,  44,  44,  44,  47,  47,  47,  47,    // b0 - bf
// LDB                 ADCB                SBBB                LDSBW
   49,   0,  49,  49,  48,   0,  48,  48,  50,  50,  50,  50,  52,   0,  52,  52,    // c0 - cf
// STW (not IM)        STB (not IM)        PUSHW               POPW (not IM)
   54,  63,  64,  56,  60,  58,  66,  69,  55,  62,  65,  57,  61,  59,  67,  68,    // d0 - df
// JNST,JLEU,JGT, JNC, NVT, JNV, JGE, JNE, JST, JGTU,JLE, JC,  JVT, JV,  JLT, JE
   72,   0,   0,   0,   0,   0,   0,  75,   0,   0,   0,   0,   0,   0,   0,  73,    // e0 - ef
// DJNZ                               JUMP                                    CALL
   76,  78,  84,  86, 107, 108, 109,   0, 102, 101, 103, 104, 105, 110, 111, 106     // f0 - ff
// RET, RETI,PSHP,POPP,Bnk0,Bnk1,Bnk2,    CLC, STC, DI,  EI,  CLVT,Bnk3,SIGN,NOP
};


/*

 Signature index value for like instructions - MAX 63
 0  special   1  clr    2  neg/cpl   3 shift l   4 shift R   5 and
 6  add       7  subt   8  mult      9 or        10 cmp     11 divide
 12 ldx       13 stx    14 push     15 pop       16 sjmp    17 subr
 18 djnz      19 cjmp   20 ret      21 carry     22 other
 24 dec       25 inc

 (sjmp is static jump , cjmp is conditional jump)

 * complicated -
 23 jnb/jb - can cross match to 19 with extra par

 12 and 13 (ldx, stx) can be regarded as equal with params swopped

 40+ are optional and skipped by default, but detectable with -

 40 sig      41 pushp    42 popp    43 di       44 ei       45 (RAM)bnk
 46 (ROM)bnk 47 nop      48 others
*/

// code emulation procs ref'ed in opcode struct

void clr  (SBK *, INST *); void ldx  (SBK *, INST *); void stx  (SBK *, INST *); void orx  (SBK *, INST *);
void add  (SBK *, INST *); void andx (SBK *, INST *); void neg  (SBK *, INST *); void cpl  (SBK *, INST *);
void sub  (SBK *, INST *); void cmp  (SBK *, INST *); void mlx  (SBK *, INST *); void dvx  (SBK *, INST *);
void shl  (SBK *, INST *); void shr  (SBK *, INST *); void popw (SBK *, INST *); void pshw (SBK *, INST *);
void inc  (SBK *, INST *); void dec  (SBK *, INST *); void bka  (SBK *, INST *); void cjm  (SBK *, INST *);
void bjm  (SBK *, INST *); void djm  (SBK *, INST *); void ljm  (SBK *, INST *); void cll  (SBK *, INST *);
void ret  (SBK *, INST *); void scl  (SBK *, INST *); void skj  (SBK *, INST *); void sjm  (SBK *, INST *);
void php  (SBK *, INST *); void ppp  (SBK *, INST *); void nrm  (SBK *, INST *); void sex  (SBK *, INST *);
void clc  (SBK *, INST *); void stc  (SBK *, INST *); void die  (SBK *, INST *); void nop  (SBK *, INST *);
void clv  (SBK *, INST *);


// OPC table items - signature index, 'fe' prefix allowed (and then skips to next entry),
// 8065 opcode only, opcode changes PSW, number of ops, write op index, write size,
// operand 1,2,3 read sizes, opcode name, pseudo source

// NB. in pseudo source - Octal escape sequences (\nnn)  are limited to three digits,
// hex (\xnn..) format is not limited (as per ansi C rules).
// so use octal in strings so that 'C' of "CY =" is not interpreted as a number.
// '\2x' in the sring means add size/sign flags to the operand 'x' (1-3).


OPC opctbl[] = {                                                       // 113 entries
{ 0 ,  0, 0,   0, 0, 0,0, 0,0,0, 0     ,"!INV!" , empty },             // zero for invalid opcodes

{ 3 ,  0, 0,   1, 2, 2,2, 1,2,0, shl   ,"shlw"  , "\2 <<= \1" },       // 1
{ 3 ,  0, 0,   1, 2, 2,1, 1,1,0, shl   ,"shlb"  , "\2 <<= \1" },       // 2
{ 3 ,  0, 0,   1, 2, 2,3, 1,3,0, shl   ,"shldw" , "\22 <<= \1" },      // 3

{ 4 ,  0, 0,   1, 2, 2,2, 1,2,0, shr   ,"shrw"  , "\2 >>= \1" },       // 4
{ 4 ,  0, 0,   1, 2, 2,2, 1,6,0, shr   ,"asrw"  , "\22 >>= \1" },      // 5
{ 4 ,  0, 0,   1, 2, 2,3, 1,3,0, shr   ,"shrdw" , "\22 >>= \1" },      // 6
{ 4 ,  0, 0,   1, 2, 2,7, 1,7,0, shr   ,"asrdw" , "\22 >>= \21" },     // 7
{ 4 ,  0, 0,   1, 2, 2,1, 1,1,0, shr   ,"shrb"  , "\2 >>= \1" },       // 8
{ 4 ,  0, 0,   1, 2, 2,5, 1,5,0, shr   ,"asrb"  , "\22 >>= \1" },      // 9

{ 5 ,  0, 0,   1, 2, 2,1, 1,1,0, andx  ,"an2b"  , "\2 &= \1"  },       // 10
{ 5 ,  0, 0,   1, 2, 2,2, 2,2,0, andx  ,"an2w"  , "\2 &= \1" },        // 11
{ 5 ,  0, 0,   1, 3, 3,1, 1,1,1, andx  ,"an3b"  , "\3 = \2 & \1"  },   // 12
{ 5 ,  0, 0,   1, 3, 3,2, 2,2,2, andx  ,"an3w"  , "\3 = \2 & \1" },    // 13

{ 8 ,  1, 0,   1, 2, 2,2, 1,1,0, mlx   ,"ml2b"  , "\22 = \2 * \1" },   // 14
{ 8 ,  0, 0,   1, 2, 2,6, 5,5,0, mlx   ,"sml2b" , "\22 = \2 * \1"},
{ 8 ,  1, 0,   1, 2, 2,3, 2,2,0, mlx   ,"ml2w"  , "\22 = \2 * \1"  },
{ 8 ,  0, 0,   1, 2, 2,7, 6,6,0, mlx   ,"sml2w" , "\22 = \2 * \1"},

{ 8 ,  1, 0,   0, 3, 3,2, 1,1,1, mlx   ,"ml3b"  , "\23 = \2 * \1"  },   // 18
{ 8 ,  0, 0,   0, 3, 3,6, 5,5,2, mlx   ,"sml3b" , "\23 = \2 * \1" },    // 19
{ 8 ,  1, 0,   0, 3, 3,3, 2,2,2, mlx   ,"ml3w"  , "\23 = \2 * \1"  },   // 20
{ 8 ,  0, 0,   0, 3, 3,7, 6,6,2, mlx   ,"sml3w" , "\23 = \2 * \1" },

{ 6 ,  0, 0,   1, 2,2, 1,1,1,0, add   ,"ad2b"  , "\2 += \1" },         // 22
{ 6 ,  0, 0,   1, 2,2, 2,2,2,0, add   ,"ad2w"  , "\2 += \1" },
{ 6 ,  0, 0,   1, 3,3, 1,1,1,1, add   ,"ad3b"  , "\3 = \2 + \1"  },
{ 6 ,  0, 0,   1, 3,3, 2,2,2,2, add   ,"ad3w"  , "\3 = \2 + \1" },     // 25

{ 7 ,  0, 0,   1, 2,2, 1,1,1,0, sub   ,"sb2b"  , "\2 -= \1" },         // 26
{ 7 ,  0, 0,   1, 2,2, 2,2,2,0, sub   ,"sb2w"  , "\2 -= \1" },
{ 7 ,  0, 0,   1, 3,3, 1,1,1,1, sub   ,"sb3b"  , "\3 = \2 - \1"  },
{ 7 ,  0, 0,   1, 3,3, 2,2,2,2, sub   ,"sb3w"  , "\3 = \2 - \1" },

{ 9 ,  0, 0,   1, 2,2, 1,1,1,0, orx   ,"orb"   , "\2 |= \1" },         //30
{ 9 ,  0, 0,   1, 2,2, 2,2,2,0, orx   ,"orw"   , "\2 |= \1" },
{ 9 ,  0, 0,   1, 2,2, 1,1,1,0, orx   ,"xorb"  , "\2 ^= \1" },
{ 9 ,  0, 0,   1, 2,2, 2,2,2,0, orx   ,"xrw"   , "\2 ^= \1" },         //33

{ 10,  0, 0,   1, 2,0, 1,1,1,0, cmp   ,"cmpb"  , empty },               //34
{ 10,  0, 0,   1, 2,0, 0,2,2,0, cmp   ,"cmpw"  , empty },

{ 11,  1, 0,   1, 2,2, 1,1,2,0, dvx   ,"divb"  , "\2 = \22 / \1" },    //36
{ 11,  0, 0,   1, 2,2, 5,5,6,0, dvx   ,"sdivb" , "\2 = \22 / \1"},
{ 11,  1, 0,   1, 2,2, 2,2,3,0, dvx   ,"divw"  , "\2 = \22 / \1" },
{ 11,  0, 0,   1, 2,2, 6,6,7,0, dvx   ,"sdivw" , "\2 = \22 / \1"},     //39

{ 12,  0, 0,   0, 2,2, 1,1,1,0, ldx   ,"ldb"   , "\2 = \1" },          //40
{ 12,  0, 0,   0, 2,2, 2,2,2,0, ldx   ,"ldw"   , "\2 = \1" },

{ 6 ,  0, 0,   1, 2,2, 1,1,1,0, add   ,"adcb"  , "\2 += \1 + CY" },    //42
{ 6 ,  0, 0,   1, 2,2, 2,2,2,0, add   ,"adcw"  , "\2 += \1 + CY" },
{ 7 ,  0, 0,   1, 2,2, 1,1,1,0, sub   ,"sbbb"  , "\2 -= \1 - CY" },
{ 7 ,  0, 0,   1, 2,2, 2,2,2,0, sub   ,"sbbw"  , "\2 -= \1 - CY" },

{ 12,  0, 0,   0, 2,2, 2,1,1,0, ldx   ,"ldzbw" , "\22 = \21" },        // 46
{ 12,  0, 0,   0, 2,2, 6,1,1,0, ldx   ,"ldsbw" , "\22 = \21" },

{ 13,  0, 0,   0, 2,1, 1,1,1,0, stx   ,"stb"   , "\1 = \2" },
{ 13,  0, 0,   0, 2,1, 2,2,2,0, stx   ,"stw"   , "\1 = \2" },          //49

{ 14,  1, 0,   0, 1,0, 0,2,0,0, pshw  ,"push"  , "push(\1)" },        // 50  or [STACK++] = \1
{ 14,  0, 1,   0, 1,0, 0,2,0,0, pshw  ,"pusha" , "pusha(\1)" },
{ 15,  1, 0,   0, 1,1, 2,2,0,0, popw  ,"pop"   , "\1 = pop()" },      // 52 or \1 = [STACK--]
{ 15,  0, 1,   0, 1,1, 2,2,0,0, popw  ,"popa"  , "\1 = popa()" },

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jnst"  , "if (STK = 0) \x9" },     // 54 (->71) cond jumps
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jst"   , "if (STK = 1) \x9" },     // 55

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jnc"   , "\10CY = 0) \x9" },      // 56
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jc"    , "\10CY = 1) \x9" },      // 57

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jnv"   , "\10OVF = 0) \x9" },     // 58
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jv"    , "\10OVF = 1) \x9" },     // 59

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jnvt"  , "if (OVT = 0) \x9" },     // 60
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jvt"   , "if (OVT = 1) \x9" },     // 61

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jgtu"  , "\7>"   },                // 62
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jleu"  , "\7<="  },                // 63

{ 19,  0, 0,   0, 1,0, 0,2,0,1, cjm   ,"jgt"   , "\7>"       },            // 64  rsz3 add "signed" comment
{ 19,  0, 0,   0, 1,0, 0,2,0,1, cjm   ,"jle"   , "\7<="      },            // 65

{ 19,  0, 0,   0, 1,0, 0,2,0,1, cjm   ,"jge"   , "\7>="      },            // 66
{ 19,  0, 0,   0, 1,0, 0,2,0,1, cjm   ,"jlt"   , "\7<"       },            // 67

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"je"    , "\7="       },            // 68
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm   ,"jne"   , "\7!="      },            // 69

{ 23,  0, 0,   0, 2,0, 0,2,1,1, bjm   ,"jnb"   , "if (\3\2 = 0) \x9" },    // 70
{ 23,  0, 0,   0, 2,0, 0,2,1,1, bjm   ,"jb"    , "if (\3\2 = 1) \x9" },    // 71

{ 18,  0, 0,   0, 2,2, 1,1,1,0, djm   ,"djnz"  , "\2--;\xbif (\2 != 0) \x9" },  //72

{ 17,  1, 0,   0, 1,0, 0,2,0,0, cll   ,"call"  , "\5" },        // 73   Long jumps
{ 17,  0, 1,   0, 2,0, 0,0,0,0, cll   ,"calla" , "\5" },        // 74
{ 16,  0, 0,   0, 1,0, 0,2,0,0, ljm   ,"jump"  , "\x9" },          // 75

{ 20,  1, 0,   0, 0,0, 0,2,0,0, ret   ,"ret"   , "return" },      // 76
{ 20,  0, 1,   0, 0,0, 0,0,0,0, ret   ,"reta"  , "return" },
{ 20,  1, 0,   0, 0,0, 0,2,0,0, ret   ,"reti"  , "return" },
{ 20,  0, 1,   0, 0,0, 0,0,0,0, ret   ,"retia" , "return" },      // 79

{ 16,  0, 0,   0, 0,0, 0,2,0,0, skj   ,"skip"  , "\x9" },          // 80     skip = sjmp [pc+2]
{ 16,  0, 0,   0, 1,0, 0,2,1,0, sjm   ,"sjmp"  , "\x9" },          // 81     short jumps
{ 17,  1, 0,   0, 1,0, 0,2,1,0, scl   ,"scall" , "\5" },        // 82
{ 17,  0, 0,   0, 1,0, 0,0,0,0, scl   ,"sclla" , "\5" },        // 83

{ 41,  1, 0,   0, 0,0, 0,0,0,0, php   ,"pushp" , "push(PSW)" },   //84
{ 41,  0, 1,   0, 0,0, 0,0,0,0, php   ,"pshpa" , "pusha(PSW)" },
{ 42,  1, 0,   1, 0,0, 0,0,0,0, ppp   ,"popp"  , "PSW = pop()" },     //86
{ 42,  0, 1,   1, 0,0, 0,0,0,0, ppp   ,"poppa" , "PSW = popa()" },


{ 1 ,  0, 0,   0, 1,1, 1,1,0,0, clr  ,"clrb"  ,  "\1 = 0" },        // 88
{ 1 ,  0, 0,   0, 1,1, 2,2,0,0, clr  ,"clrw"  ,  "\1 = 0" },

{ 2 ,  0, 0,   1, 1,1, 1,1,0,0, neg  ,"negb"  ,  "\1 = -\1" },       //90
{ 2 ,  0, 0,   1, 1,1, 2,2,0,0, neg  ,"negw"  ,  "\1 = -\1" },

{ 25,  0, 0,   1, 1,1, 1,1,0,0, inc  ,"incb"  ,  "\1++"  },
{ 25,  0, 0,   1, 1,1, 2,2,0,0, inc  ,"incw"  ,  "\1++" },           //93
{ 22,  0, 0,   1, 2,1, 1,1,3,0, nrm  ,"norm"  ,  "\1 = normalize(\22)" },
{ 2 ,  0, 0,   1, 1,1, 1,1,0,0, cpl  ,"cplb"  ,  "\1 = ~\1" },          // 95
{ 2 ,  0, 0,   1, 1,1, 2,2,0,0, cpl  ,"cplw"  ,  "\1 = ~\1" },

{ 24,  0, 0,   1, 1,1, 1,1,0,0, dec  ,"decb"  ,  "\1--" },           // 97
{ 24,  0, 0,   1, 1,1, 2,2,0,0, dec  ,"decw"  ,  "\1--"  },
{ 22,  0, 0,   1, 1,1, 6,1,0,0, sex  ,"sexb"  ,  "\21 = \1" },
{ 22,  0, 0,   0, 1,1, 7,2,0,0, sex  ,"sexw"  ,  "\21 = \1" },      // 100


{ 21,  0, 0,   1, 0,0, 0,0,0,0, stc  ,"stc"   ,  "CY = 1" },         // 101
{ 21,  0, 0,   1, 0,0, 0,0,0,0, clc  ,"clc"   ,  "CY = 0" },         // 102
{ 43,  0, 0,   0, 0,0, 0,0,0,0, die  ,"di"    ,  "disable intps" },
{ 44,  0, 0,   0, 0,0, 0,0,0,0, die  ,"ei"    ,  "enable intps" },
{ 48,  0, 0,   0, 0,0, 0,0,0,0, clv  ,"clrvt" ,  "OVT = 0" },
{ 47,  0, 0,   0, 0,0, 0,0,0,0, nop  ,"nop"   ,  empty },

{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk0"  ,  empty },              // 107
{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk1"  ,  empty },
{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk2"  ,  empty },              //109
{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk3"  ,  empty },               // 110

{ 46,  0, 1,   0, 1,0, 0,1,0,0, 0    ,"rbnk"  ,  empty },  // 111 bank prefix (8065)
{ 40,  0, 0,   0, 0,0, 0,0,0,0, 0    ,"!PRE!" ,  empty }   // 112 signed prefix

};


// special modded opcode pseudo sources for carry
const char *scespec[] = {
  "\x2 += CY",
  "\x2 -= CY"
 };

//  for swopping over cmp R0, x operands

const char *swopcmpop[] = {

 "<" ,        // jgtu '>'  (62) to 'jltu' and JC std
 ">=",        // jleu '<=  (63) to 'jgeu' and JNC std
 "<" ,        // jgt  '>'  (64) to 'jlt'
 ">=",        // jle  '<=' (65) to 'jge'
 "<=",        // jge  '>=' (66) to 'jle'
 ">" ,        // jlt  '<'  (67) to 'jgt'
 "=" ,        // je  remains same
 "!=",        // jne remains same
 ">",         // JNC
 "<="         // JC

};

// for the dummy cmp R0,0

const char *zerocmpop[] = {

 "false" ,        // jgtu 
 "true"  ,        // jleu
 "false" ,        // jgt
 "true"  ,        // jle
 "true"  ,        // jge
 "false" ,        // jlt
 "true"  ,        // je  
 "false"          // jne
};



 /* interrupt subroutine names array, for auto naming
  * "HSI_" and default name is output as prefix in code
  * switched on/off with Opt N
  * params -
  * start, end   address of interrupt (+ 0x2010)
  * I8065  1 = 8065 interrupt, 0 is both 8065 and 8061
  * num    1 = name has number appended (8065)
  * name
  */


NAMES inames[] = {
{ 0,   0, 0, 0, "HSO_2" },            //  8061 ints - actual at 2010
{ 1,   1, 0, 0, "Timer_OVF" },
{ 2,   2, 0, 0, "AD_Rdy" },
{ 3,   3, 0, 0, "HSI_Data" },
{ 4,   4, 0, 0, "External" },
{ 5,   5, 0, 0, "HSO_1" },
{ 6,   6, 0, 0, "HSI_1" },
{ 7,   7, 0, 0, "HSI_0" },            // 201f  - end 8061 list

{ 0,  15, 1, 1, "HSO_" },           // 8065 ints - 2010-201f (plus int num)
{ 16, 16, 1, 0, "HSI_FIFO" },
{ 17, 17, 1, 0, "External" },
{ 18, 18, 1, 0, "HSI_0" },
{ 19, 19, 1, 0, "HSI_Data" },
{ 20, 20, 1, 0, "HSI_1" },
{ 21, 21, 1, 0, "AD_Imm_Rdy" },
{ 22, 22, 1, 0, "AD_Timed_Rdy" },
{ 23, 23, 1, 0, "ATimer_OVF" },
{ 24, 24, 1, 0, "AD_Timed_Start" },
{ 25, 25, 1, 0, "ATimer_reset" },
{ 26, 29, 1, 1, "Counter_" },            // add num
{ 30, 39, 1, 1, "Software_" }          // add num 2040-205f    end 8065
};


// Auto names - appended with address.
// option cmd mask, size in, size out, name
// sizes are for function and table subroutine names

AUTON anames [] = {
 { 0,      0, 0, empty     },          // 0 null entry
 { 0x80 ,  0, 0, "Sub"     },          // 1   PPROC   proc names (xfunc, vfunc, intfunc) and default syms
 { 0x10 ,  0, 0, "Lab"     },          // 2   PLABEL  label names (lab)
 { 0x2  ,  0, 0, "Func"    },          // 3   PFUNC   function data names
 { 0x2  ,  0, 0, "Table"   },          // 4   PFUNC   table data names

 { 0x4  ,  1, 1, "UUYFuncLU" },        // 5   auto subroutine names for function and table lookup procs
 { 0x4  ,  1, 5, "USYFuncLU" },
 { 0x4  ,  5, 1, "SUYFuncLU" },
 { 0x4  ,  5, 5, "SSYFuncLU" },

 { 0x4  ,  2, 2, "UUWFuncLU" },         // 9
 { 0x4  ,  2, 6, "USWFuncLU" },
 { 0x4  ,  6, 2, "SUWFuncLU" },
 { 0x4  ,  6, 6, "SSWFuncLU" },

 { 0x4  ,  1, 1, "UYTabLU"  },          //  13
 { 0x4  ,  5, 5, "SYTabLU"  },
 { 0    ,  0, 0, "Timer"    }           // 15   used as a SPECIAL for timers.
 };



/*
 {0   ,0       },     // lufunc used for spf funcs
 {0   ,0       },     // lutable
*/


// bits   7 is Most significant 0 is least.

/***** Auto default symbols *******

 * Always added to analysis, fixed nas 'base' names for special registers
 * Params in order -
 * 8061  name
 * 8065  name     (both can be set)
 * read 0, write 1
 * whole 0, bit 1
 * bitnumber (0-7)
 * address
 * name
*/


DFSYM defsyms [] = {

 {1, 1, 0, 1, 6,     2, "CPU_OK"      },    // both 8061 and 8065
 {1, 1, 0, 0, 0,     2, "LSO_Port"    },
 {1, 1, 0, 0, 0,     3, "LIO_Port"    },
 {1, 1, 0, 0, 0,     4, "AD_Low"      },
 {1, 1, 0, 0, 0,     5, "AD_High"     },
 {1, 1, 0, 0, 0,     6, "IO_Timer"    },
 {1, 0, 0, 0, 0,     7, "IO_Timer_Hi" },       // 8061 only
 {0, 1, 0, 0, 0,     7, "AD_Timed_Lo"  },      // 8065 only
 {1, 1, 0, 0, 0,     8, "INT_Mask"     },
 {1, 1, 0, 0 ,0,     9, "INT_Pend"     },

 {1, 1, 0, 1, 0,   0xa, "HSO_OVF"      },      // both, bit 0
 {1, 1, 0, 1, 1,   0xa, "HSI_OVF"      },
 {1, 1, 0, 1, 2,   0xa, "HSI_Ready"    },
 {1, 1, 0, 1, 3,   0xa, "AD_Ready"     },      // both, bit 3
 {1, 0, 0, 1, 4,   0xa, "INT_Handling" }  ,           // 8061 only
 {0, 1, 0, 1, 4,   0xa, "MEM_Expand"    },            // 8065 only
 {1, 0, 0, 1, 5,   0xa, "INT_Priority"  },
 {0, 1, 0, 1, 5,   0xa, "HSO_Pcnt_OVF"  },

 {0, 1, 0, 1, 6,   0xa, "Timed_AD_Ready"},  // bits 6,7 not used in 8061
 {0 ,1, 0, 1, 7,   0xa, "HSO_Port_OVF"  },


 {1, 1, 0, 0, 0,   0xa, "IO_Status"     },
 {1, 1, 0, 0, 0,   0xb, "HSI_Sample"    },
 {1, 1, 0, 0, 0,   0xc, "HSI_Mask"  },
 {1, 1, 0, 0, 0,   0xd, "HSI_Data"  },
 {1, 1, 0, 0, 0,   0xe, "HSI_Time"  },
 {1, 0, 0, 0, 0,  0x10, "StackPtr"   },        // 8061 stack

 {0, 1, 0, 0, 0,   0xf, "AD_Timed_HI" },       // with 007 above


 {0, 1, 0, 0, 0,  0x10, "HSO_IntPend1" },        // 8065 only
 {0, 1, 0, 0, 0,  0x11, "BANK_Select"  },
 {0, 1, 0, 0, 0,  0x12, "HSO_IntMask1" },
 {0, 1, 0, 0, 0,  0x13, "IO_Timer_Hi"  },
 {0, 1, 0, 0, 0,  0x14, "HSO_IntPend2" },
 {0, 1, 0, 0, 0,  0x15, "LSSI_A"       },
 {0, 1, 0, 0, 0,  0x16, "HSO_IntMask2" },
 {0, 1, 0, 0, 0,  0x17, "LSSI_B"       },
 {0, 1, 0, 0, 0,  0x18, "HSO_Status"   },
 {0, 1, 0, 0, 0,  0x19, "LSSI_C"       },
 {0, 1, 0, 0, 0,  0x1a, "HSI_Mode"     },
 {0, 1, 0, 0, 0,  0x1b, "HSO_UsedCnt"  },
 {0, 1, 0, 0, 0,  0x1c, "HSO_TimeCnt"  },
 {0, 1, 0, 0, 0,  0x1d, "LSSI_D"       },
 {0, 1, 0, 0, 0,  0x1e, "HSO_LastUsed" },
 {0, 1, 0, 0, 0,  0x1f, "HSO_SlotSel"  },
 {0, 1, 0, 0, 0,  0x20, "StackPtr"     },        // 8065 stacks
 {0, 1, 0, 0, 0,  0x22, "StackPtr2"    },

 {1, 1, 1, 0, 0,   0x4, "AD_Cmd"       },           // Write symbols from here
 {1, 1, 1, 0, 0,   0x5, "WDG_Timer"    },
 {0, 1, 1, 0, 0,   0x7, "AD_Timed_Cmd" },
 {0, 1, 1, 0, 0,   0xb, "IDDQ_Test"    },
 {1, 1, 1, 0, 0,   0xd, "HSO_Cmd"      },
 {0, 1, 1, 0, 0,   0xe, "HSO_Time"     },
 {0, 1, 1, 0, 0,   0x15, "LSSO_A"      },
 {0, 1, 1, 0, 0,   0x17, "LSSO_B"      },
 {0, 1, 1, 0, 0,   0x19, "LSSO_C"      },
 {0, 1, 1, 0, 0,   0x1d, "LSSO_D"      },

};


 /**************************************************
 * main global variables
 **************************************************/

time_t  timenow;

HDATA fldata;                 // files data holder (in shared.h)

// IBUF - generic memory array.
// First 8192 (0x2000) reserved for RAM and register data.  !! WRONG !!
// Then 4*0x10000 banks, referenced by 'nbuf' pointers in bkmap structure (with PCORG offset).
// Bank order independent. bkmap[BMAX] is used for RAM [0-1fff].

// need to change this so that only 0-3ff is bankless. (what about RAMBNK?)
// must change file read mapping to sort this out as well.........
// this would mess up my neat contiguous file read, unless we can have
// multiple 0-2000 slots as well.............
// so technically, 4 * 0-ff, 4 * 400-1fff, 4*2000-ffff   (= 4 * 400 - ffff)

// or put ram in bank 1 for multibanks (DATBNK)

//first, split the front 2k off ibuf...........

//uchar rbuf[0x2000];           // for starters.

//now malloced.

uchar *ibuf;                   //[0x42000];              // 0-2000 ram, + 4 banks of 0-0xffff ROM
uint  *opbit;                  //[0x2000];    0x8000 bytes          // opcode start flag - 4 banks of 32 flags

// bit array for RBINV. Allows for 0-0x2000
// speeds up rbase searches, as most are invalid
uint  rbinv[0x100];

BANK bkmap[BMAX+1];               // bank map - 16 banks +1 for ram/reg data.
BANK *lastbmap;                   // speedup for mapping

// lookup arrays for 'ssize' parameter used in many places
// ssize is 1,2,3 = usigned byte,word,long    4,5,6 signed byte,word,long

uint casz[8] = {0,1,2,4,0,1,2,4};                                   // size in bytes
uint casg[8] = {0,0,0,0,0,0x80,0x8000,0x8000000};                   // sign bit mask (and minimum)
uint cafw[8] = {0,3,5,7,0,4,6,8};                                   // print field width
uint camk[8] = {0,0xff,0xffff,0xffffffff,0,0xff,0xffff,0xffffffff}; // size mask (and maximum)

const char *calo[8] = {empty, "UY", "UW", "UL", empty, "SY", "SW", "SL"};  // description in commands
// const char *calo[8] = {empty, "Y", "W", "L", empty, "SY", "SW", "SL"};  // description in commands


/* positions of columns in printout - may allow changes to these at some point....
*
* 28      opcode mnemonic starts (after header)  [26 if no bank prefix]
* 34      operands start             (+6)
* 51      pseudo source code starts  (+17)
* 85      comment starts             (+34)
* 180     max width of page - wrap here
*/

int cposn[5] = {26, 32, 49, 83, 180};

#ifdef XDBGX
  // debug counters various

  int DBGrecurse  = 0;
  int DBGnumops   = 0;          // num of opcodes
#endif

uint  tscans  = 0;            // total scan count, for safety
int   xscans  = 0;            // main loop (for investigation)
uint  lowscan = 100000;       // rescan from here in scan_all
int   opcnt   = 0;            // opcode count for emulate
int   cmdopts    = 0;        // command options
int   anlpass    = 0;        // number of passes done so far

int   numbanks;              // number of banks-1  (-> 8061 is always 0)

int indent;               // TEST for code print layout
int   gcol;                  // where main print is at (no of chars = colno)
int   lastpad;               // last padded to (column)
int   wcol;                  // where warning output is at

uint  datbnk;                // currently selected data bank - always 8 if single bank
uint  codbnk;                // currently selected code bank - always 8 if single bank
uint  rambnk;                // currently selected RAM bank  - always 0 for 8061

CMNT  fcmnt  ;               // comments holder from cmnts file
CMNT  acmnt  ;               // auto comments holder


// instance holders (each hold one analysed opcode and all its operand data)

INST  cinst;                 // current (default) instance for std scans
INST  sinst;                 // search instance (for conditionals, params, etc )
INST  einst;                 // current emulate instance

// special scan blocks, outside standard scan chain
SBK   tmpscan;                // scan block for searches
SBK   prtscan;                // scan block for printing
SBK   *emuscan;               // scan block for imd pushes in emulate

LBK   prtblk;                 // dummy LBK block for printing adjustments
LBK  *emuargs[EMUGSZ];        // tracking argument LBKs, created in emulate, for size fixup


uint minwreg, maxwreg;        // min and max ram register address (for checking)
uint stkreg1, stkreg2;        // stack registers, 2 stacks for 8065


char  flbuf[256];             // for directives and comment file reads
char  nm [128];               // for temp name construction, etc.
int  plist[32];              // generic param list and column positions for structs



FSTK  scanstack[STKSZ];       // fake stack holder for previous subr calls (for arg tracking)
FSTK  emulstack[STKSZ];       // fake stack for emulation - emulate all parameter/argument getters
CPS  cmnd;                    // command structure
ADT  cmdadnl[31];             // temp ADT block, used for command and prints
//ADT  ansadnl;                 // one entry for possible subr answer

int recurse  = 0;             // recurse count check



//********** file extensions and flags for linux and Win32 ********************

#if defined(__linux__) || defined(__ANDROID__)

  HOP fd[] ={{".bin","r"},    {"_lst.txt","w"}, {"_msg.txt","w"},
             {"_dir.txt","r"},{"_cmt.txt","r"}, {"sad.ini","r"} , {"_dbg.txt","w"} };

 #define PATHCHAR  '/'

#endif

#if defined __MINGW32__ || defined __MINGW64__

// windows (CodeBlocks, Mingw)

   HOP fd[] ={{".bin","rb"},    {"_lst.txt","wt"}, {"_msg.txt","wt"},
              {"_dir.txt","rt"},{"_cmt.txt","rt"}, {"sad.ini","rt"} , {"_dbg.txt","wt"} };

 #define PATHCHAR  '\\'

#endif


/************************
 * chaining and admin stuff
 **************************/

void wnpad(int c)
{
// warning file. pad out to required column number
int i;

if  (c <= wcol) c = wcol+1;                // always add at least one space...
for (i = wcol; i < c; i++)  fprintf(fldata.fl[2], " ");
wcol = c;
}


int wnprt (int nl, const char *fmt, ...)
{  // warning file prt

  int chars;
  chars = 0;
  if (fmt)
    {
     va_list args;
     va_start (args, fmt);
     chars = vfprintf (fldata.fl[2], fmt, args);
     va_end (args);
    }
  wcol += chars;  
  if (nl) wcol = 0;         // newline  
  while (nl--) fprintf(fldata.fl[2], "\n");
  return chars;             // useful for if statements
}


int cmdaddr(int addr)
{
  // for printing of command addresses with or without bank
 if (!numbanks) return nobank(addr);
 return addr;
}


int bprtfl(float fv, int pfw)
{
  // print float to nm holder array
  char *tx, *s, p;
  int cs;

  if (pfw) p = ' '; else p = '\0';  // zero or space
  s = nm+64;                        //move after other strings
  cs = sprintf(s,"%*.3f", pfw, fv);
  tx = s + cs-1;
  
  // trim/replace trailing zeroes and dot if no digits.
  
  while (*tx == '0') *tx-- = p;
  if (*tx == '.') *tx-- = p;
  if (pfw) return cs;
  return tx-s;                   // chars 'printed'
}



int wflprt(const char *fmt, float fv, int (*prt) (int,const char *, ...))
 {
  // separate float print, to control width
  // the 'prt' argument allows print to debug or message files

  int cs;
  prt(0,fmt);
  cs = bprtfl(fv,0);
  prt(0,"%s ",nm+64);
  return cs;
}




void p_pad(int c)
{
// list file. pad out to required column number, if not already there
int i;

if  (c <= gcol) c = gcol+1;                // always add at least one space...
for (i = gcol; i < c; i++)  fprintf(fldata.fl[1], " ");
gcol = c;
lastpad = c;                         // last padded column
}

void newl (int c)
{                  // add return(s) and reset colno
  if (c <= 0) c = 1;

  while (c > 0) {fprintf(fldata.fl[1], "\n"); c--;}
  gcol = 0;
}

void wrap()
 {
  // newline and wrap back to comments column if already greater,
  // or lastpad column if not
  fprintf(fldata.fl[1], "\n");
  gcol = 0;
  if (lastpad > cposn[3])  lastpad = cposn[3];
  p_pad(lastpad);
 }

void pchar(char z)
{
 fputc(z,fldata.fl[1]);
 gcol++;
 if (gcol >= cposn[4]) wrap();
}

void pstr (const char *fmt, ...)
{   // output file print
  va_list argptr;

  if (!fmt || !*fmt ) return ;
  va_start (argptr, fmt);
  gcol += vfprintf (fldata.fl[1], fmt, argptr);
  va_end (argptr);
  if (gcol >= cposn[4]) wrap();
}

void paddr(int addr)
{
  // printer for addresses with or without bank.
  // if < 0x2000 (PCORG) always drop bank
  // For listing output

  // NOT with 0x3ff CHANGES !!

 if (nobank(addr) >= PCORG)
   {
    if (numbanks) gcol += fprintf(fldata.fl[1],"%05x", addr);
    else          gcol += fprintf(fldata.fl[1],"%4x", nobank(addr));
    }
 else    gcol += fprintf(fldata.fl[1],"%x", nobank(addr));
}



 int check_bank(int bk)
{
  if (bk == 0 || bk == 1 || bk == 8 || bk == 9) return 1;
  return 0;
}

 // master exit and cleanup routines

void closefiles(void)
{
    // flush and close all files
  int i;

  for (i=0; i < 6; i++)         // include sad.ini
   {
    if (fldata.fl[i])         // close file
       {
         fflush (fldata.fl[i]);
         fclose (fldata.fl[i]);
         fldata.fl[i] = NULL;
       }
   }
  fldata.fillen = 0;             //flag marker.

#ifdef XDBGX

   fflush (fldata.fl[6]);    // flush debug file too
   fclose (fldata.fl[6]);
   fldata.fl[6] = NULL;
#endif

}



// address handling subrs  0xfffff  (4 bits bank, 16 bits addr in bank)


BANK *map_bank(uint addr)
{
 // and is always valid pointer into bkmap, even if RUBBISH
 return bkmap+((addr >> 16) & 0xf);
}

BANK *mapdbank(uint addr)
{
  // get data bank. allows for less than PCORG

 if (nobank(addr) < PCORG) return bkmap+BMAX;     // register and ram bank.
 return bkmap+((addr >> 16) & 0xf);          // and return as an int 0-16
}


uint minadd(uint addr)
{
 BANK *b;
 b = map_bank(addr);
 if (!b->bok) return 0xffff;           // invalid
 return b->minpc;
}                                 // min address is bank|PCORG

uint maxadd (uint addr)
{
 BANK *b;
 b = map_bank(addr);
 if (!b->bok) return 0;           // invalid

 return b->maxpc;
}                                 // max address for this bank

uint maxbkadd (uint addr)
{
 BANK *b;
 b = map_bank(addr);
 if (!b->bok) return 0;           // invalid

 return b->maxbk;
}


bool val_code_addr(uint addr)
 {
 // return 1 if valid 0 if invalid.
 // CODE addresses - no data fixup

 //int bank;
 //bank = (addr >> 16) & 0xf;        // NOT dbank call
 BANK *b;


 b = bkmap+((addr >> 16) & 0xf);

 if (!b->bok) return 0;           // invalid

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // now where fill starts

 return 1;        // within valid range, OK
 }





bool val_jmp_addr(uint addr)
 {
 // return 1 if valid 0 if invalid.
 // JUMP addresses only, NOT scan

 // NOT allows some extra adds as OK (for console)

 int x;
 BANK *b;

 x = (addr >> 16) & 0xf;        // NOT dbank call
 b = bkmap+x;

 if (addr == 0x1f1c) return 1;   // DUCE chip ??

 if (!b->bok) return 0;           // invalid bank

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // now where fill starts

 return 1;        // within valid range, OK
 }


bool val_data_addr(uint addr)

{
 // return 1 if valid 0 if invalid.
 // DATA addresses

 BANK *b;

 b = mapdbank(addr);             // handles < PCORG
 if (!b->bok) return 0;          // invalid

 if (addr <= b->minpc) return 0;  // PCORG (and zero) is invalid
 if (addr > b->maxpc) return 0;

 return 1;        // within valid range, OK
 }


uchar* map_addr(uint *addr)
 {
  // only used by g_word, g_byte, g_val.  Allow maxbk as limit (so fill can be read)
  // demands that b->fbuf points to bank|PCORG

  uint bank, x;
  BANK *b;

  x = *addr;
  *addr = nobank(*addr);            // ALWAYS strip bank for lookup

  // speedup check to save a redundant remap
  if (x >= lastbmap->minpc && x <= lastbmap->maxbk) return lastbmap->fbuf;

  if (x < minwreg) return NULL;
  if ((*addr) < PCORG) return bkmap[BMAX].fbuf;           //always RAM or regs

  bank = (x >> 16) & 0xf;
  b = bkmap+bank;

  if (!b->bok || x > b->maxbk) return NULL;  // invalid addr, assume start at PCORG

  lastbmap = b;
  return b->fbuf;         // valid address, return pointer

 }


//----------------------------------------------------------


uint codebank(uint addr, INST *c)
 {
   // get BANK opcode to override default

  if (addr >= PCORG)
    {
     addr = nobank(addr);
     if (c && c->bnk) addr |= (c->bank << 16);
     else addr |= codbnk;
    }
  return addr;
 }

uint databank(uint addr, INST *c)
 {
   // get BANK opcode to override default
   // NEGATIVE offsets can come here

  if (addr >= PCORG)
    {
     addr = nobank(addr);
     if (c && c->bnk) addr |= (c->bank << 16);
     else addr |= datbnk;
    }
  return addr;
 }


int g_byte (uint addr)
{                    // addr is  encoded bank | addr
  uchar *b;

  b = map_addr(&addr);
  if (!b) return 0;        // safety
  return (int) b[addr];
}


int g_word (uint addr)
{         // addr is  encoded bank | addr
  int val;
  uchar *b;

  b = map_addr(&addr);                      // map to required bank
  if (!b)  return 0;

  val =  b[addr+1];
  val =  val << 8;
  val |= b[addr];
  return val;

 }

/*************************************
Multiple scale generic get value.
 size  1=byte, 2=word, 3=long. unsigned
       5=byte, 6=word, 7=long. signed
Auto negates via bit mask if required
***********************************/

int g_val (uint addr, uint ssize)
{                                           // addr is  encoded bank | addr
  int val, mask;                            // scale is -ve for no sign
  uchar *b;

  b = map_addr(&addr);
  if (!b)  return 0;                        // map to required bank

  ssize &= 7;                                // safety
  val = 0;
  mask = casg[ssize];            // where sign bit resides
  ssize = casz[ssize] - 1;        // get true size in bytes

  if (addr <= ssize) return 0;   // safety for unsigned loop

  for (ssize = ssize+addr; ssize >= addr; ssize--)
    val = (val << 8) | b[ssize];

  if (mask & val) val |= ~(mask - 1);      // if signed

  return val;
}


int negval (int val, int mask)
{  // negate value according to sign bit position - used for jumps

  if (mask & val) val |= ~(mask - 1);        // negate if sign bit set
  return val;
}


int upd_ram(uint add, int val, int size)
 {
  int ans;
  uint i;
  uchar x;

  add = nobank(add);
  if (add <= minwreg)  return 0;        //cannot write to specials
  if (add >= PCORG)    return 0;        // cannot write to ROM....

  ans = 0;
  size &= 7;                            // safety
  size = casz[size];                    // get true size in bytes

  for (i = add; i < add+size; i++)
    {
      x = ibuf[i];                      // old value
      ibuf[i] = val & 0xff;
      if (x != ibuf[i])  ans = 1;       // value has changed
      val >>= 8;
    }

  return ans;
 }








void set_flag(uint reg, uint *x)
 {
  uint bit;

  if (reg > PCORG) return;

#ifdef XDBGX
  DBGPRT(1,"set inv rbase R%x", reg);
#endif

  bit = reg & 0x1F;       // 32 bits
  reg = reg / 32;
  x[reg] |= (1 << bit);
 }

int get_flag (uint reg, uint *x)
{
  uint bit;

  if (reg > PCORG) return 1;

  bit = reg & 0x1F;
  reg = reg / 32;
  return x[reg] & (1 << bit);
}



int check_argdiff(int nv, int ov)
{
  // nv is new value, ov is original

  int cnt;

  if (! bankeq(nv, ov))  nv |= g_bank(ov);

  cnt = nv - ov;
  if (cnt > 0 && cnt < 32) return cnt;

  #ifdef XDBGX
  if (nv != ov) DBGPRT(1,"Argdiff reject %x %x", nv,ov);
  #endif
  return 0;
}








int get_pn_opstart (uint ofst, int d)
{
  // get previous or next opstart
  // d = 0 for previous, 1 for next

  int bit,bank,cnt;
  uint xofst,end;
  BANK *b;
  uint *z;

  bank = (ofst >> 16) & 0xf;
  b = bkmap + bank;
  z = b->opbt;
  end = maxadd(ofst);
  cnt = 0;

  xofst  = nobank(ofst);
  end    = nobank(end) - PCORG;     // map to bit array
  xofst -= PCORG;

// MUCH faster to do the shift and calc BACK....
// CAN scan THIS opcode if forwards, but not if backwards
// max possible code length is 8 (bank+fe+5)

  while (xofst > 0 && xofst <= end)
   {
    if (d) xofst++;  else xofst--;
    cnt++;
    bit = xofst & 0x1F;
    bank = xofst / 32;
    if (z[bank] & (1 << bit)) break;
    if (cnt > 8) break;          // no opcodes here
   }

  if (xofst > end)  return 0;
  if (cnt > 8) return 0;

  xofst += PCORG;
  xofst |= g_bank(ofst);  // bank
  return xofst;

}

uint find_opcode(int d, uint xofst, OPC **opl)
 {
  // would be simple lookup if it weren't for the prefixes.
  // d = 0 for previous, 1 for next
  int x, indx;

  *opl = 0;
  xofst = get_pn_opstart (xofst, d);      // previous or next

  if (!xofst) return 0;
  if (!val_code_addr(xofst)) return 0;

  x = g_byte(xofst);
  indx = opcind[x];                       // opcode table index
  *opl = opctbl + indx;

 // Ford book states opcodes can have rombank(bank) AND fe as prefix (= 3 bytes)

   if (indx == 112)                     // bank swop - do as prefix
     {
      if (!P8065) indx = 0;             // invalid for 8061
      else
       {                                // skip op & bank, 2 bytes
        x = g_byte(xofst+2);            // get true opcode
        indx = opcind[x];
       }
     }

  if (indx == 111)                      // SIGN prefix
     {                                  // skip prefix, 1 byte
      x = g_byte(xofst + 1);
      indx = opcind[x];                 // get base opcode
      *opl = opctbl + indx;
      if ((*opl)->sign)  indx += 1;     // get prefix opcode
      else indx = 0;                    // prefix not valid
     }

  if ((*opl)->p65 && !P8065) return 0;     // 8065 check only
  if (!indx) return 0;
  *opl = opctbl + indx;                 // set opl for return
  return xofst;
 }




int find_pn_opcodes(int d, uint addr, int num, uint n, ...)     // number back from start
 {
   // finds previous or next opcodes in list (sig ix) max num is NUMPARS (16)
   // use get_pn_opcode above, may be merged in here later

   // add a check for block terminators - jumps, rets etc.
   // d = 0 for previous, 1 for next
   // num = num of opcodes to scan (n and list is the opcodes)
   // results in sinst instance

    int xofst, ans, cnt;
    uint i;
    OPC *opl;
    va_list ix;

    // assemble opcode sigixs into array
    va_start(ix,n);

    memset(plist,0,sizeof(plist));

    if (n > NC(plist)) n = NC(plist);

    i = n;
    while (i--)
     {
      plist[i] = va_arg(ix, int);
     }

    va_end(ix);

    // OK -now see if we can find a match

    xofst = addr;
    if (d)  xofst--;        // allow use of supplied xofst if going forwards, not backwards
    ans = 0;
    cnt = 0;
    // x->ofst must fall within specifed range (backwards from current). Allow for curinst
    // being fed in so <=

    while (!ans)
     {
      xofst = find_opcode(d, xofst, &opl);
      if (!xofst) break;

      cnt ++;
      if (cnt > num) {break;}

     // 16  jump, 20 ret.
      if (d && opl->sigix == 20) break;   // not through a RET (forwards)

      if (d && opl->sigix == 16)
        {        //allow jumps forwards
          tmpscan.curaddr = xofst;
          tmpscan.start = xofst;
          do_code(&tmpscan,&sinst);
          xofst = sinst.opr[1].addr;
          xofst--;
        }

      for (i = 0; i < n; i++)
        {
         if (opl->sigix == plist[i])  ans = xofst;
        }
     }

    if (ans)
      {
       tmpscan.curaddr = xofst;
       tmpscan.start = xofst;
       do_code(&tmpscan,&sinst);
      }
    else
      {
       memset(&sinst,0,sizeof(INST));
       sinst.opr[1].rgf = 1;
       sinst.opr[2].rgf = 1;
       sinst.opr[3].rgf = 1;
      }
    return ans;
 }





int mark_emu(SBK *s, uint ofst)
 {
  // get the calling proc from opcode before any new args
  // and flag it as emureqd

   OPC *opl;
   SBK *x;
   SUB * sub;
   uint ans;

    #ifdef XDBGX
      DBGPRT(1,"In Mark_emu (for ARGS) for %x", s->start);
    #endif


   ans = 0;

   if (!ofst) return ans;

   ofst = find_opcode(0, ofst, &opl);   // opcode before args

   if (!ofst || opl->sigix != 17)  return ans;

   // CALL,
   tmpscan.curaddr = ofst;
   tmpscan.start = ofst;
   do_code(&tmpscan,&sinst);
   x = get_scan(sinst.opr[1].addr, sinst.opr[1].addr);

   sub = get_subr(sinst.opr[1].addr);

   if (sub && sub->size)
      {
#ifdef XDBGX
        DBGPRT(1,"args ignored - preset size");           //preset size, no emul
#endif
        return x->start;
      }

      if (x && !x->emulreqd)
        {
         #ifdef XDBGX
           DBGPRT(1,"SET EMUREQD for sub %x", x->start);
         #endif
         x->emulreqd = 1;            // this is where args attached
         ans = x->start;            // address of sub
         s->args = 1;               // and mark caller
        }
     
   return ans;
  }


void check_args(SBK *s, INST *c)
  {
    // check if register qualifies subroutine for EMULATE
    // i.e. was popped and modified....
    // only used in scanning mode. Called from upd_watch.

   int i, args;
   FSTK *t;
   RST *r;
   OPS *o;

// for autoinc and indirect only at present

   if (!s->scanning) return;

   o = c->opr+1;                     // target for increments
   if (!o->addr) return;             // not for R0

#ifdef XDBGX
   DBGPRT(1,"R%x check args", o->addr);
#endif

   // subroutine immediately before the ARGS is the one which needs to be
   // flagged as EMUREQD and THIS subr flagged as an ARGS getter

   r = find_rgstat(o->addr);
   if (!r || !r->popped) return;       // not found or reg not popped
   set_rgsize(2,chrgst.lastix);                   // incs are ALWAYS word
   args = 0;
   
   for (i=STKSZ-1; i >= 0; i--)
      {
        t = scanstack+i;          // this is UP the scan mode call chain
        if (t->type == 1 && t->reg == o->addr && t->popped)
          {
            args = check_argdiff((nobank(o->val) | g_bank(t->origadd)), t->origadd);
             if (args)
                {
                  #ifdef XDBGX
                   DBGPRT(1,"found FSTK for %x at %d", o->addr, i);
                   #endif
                   break;
                }
           }
      }

   // don't actually have to find a stack entry, which when sub is specified
   // and has a lower level getargs (e.g. A9L)

   // default check first, if stack not found. May be valid.

   
   if (!args) args = check_argdiff(o->val, 0);

   if (args)
    {
     // flag this subr doing the arg get so that
     // calling it will trigger emu of the caller.
     #ifdef XDBGX
      if (i < 0) DBGPRT(1,"no FSTK %x", s->curaddr);
     #endif
     if (i >=0) mark_emu(s, t->origadd);  //not if FSTK not found (by i)
    }
  }





void log_emuargs(LBK *l)
{
int i;

if (!l) return;

for (i = 0; i < EMUGSZ; i++)
  {
   if (emuargs[i] == l) break;     // already logged
   if (!emuargs[i])
     {
      emuargs[i] = l;
      #ifdef XDBGX
      DBGPRT(1,"log_emu %x = %d",l,i);
      #endif
      break;
     }
  }
     #ifdef XDBGX
if (i >= EMUGSZ) DBGPRT(1,"NO SLOTS FOR LOG EMU");
  #endif

}


int cellsize(ADT *a)
{
  // single additional entry
 if (!a) return 0;
 return a->cnt * casz[a->ssize];
}



int listsize(void *fid, int seq)
{
 // continue from seq to next newline
 // but must save lastix in case it hits end

 return adtchnsize(fid, seq, 1);

}

int totsize(void *fid)
{
 return adtchnsize(fid, -1, 0);
}


int add_args(LBK *l, int dreg, int args)
{
  // add args in pairs (= words) to make easier sizing later
  // assume l is set up as C_ARGS already
  ADT *a;
  RST  *r;
  int size, reg;

  if (!l) return -1;

  size = args - totsize(l);          // new args required
  reg = dreg;

  if (!size) return 0;
  while (size > 0)
   {
      a = add_adt(&chadnl, l,128);   //new one
      if (a)
          {
           r = find_lowest_rgarg(dreg);
           if (r) reg = r->reg;
           if ((r && (r->reg & 1)) || size < 2) ; else a->cnt = 2;  // i.e. for each byte so far
    //       a->fnam = 1;}                          // No name lookup for bytes
           a->dreg = reg;
           dreg += a->cnt;                       // next lookup
           reg  += a->cnt;                       // next reg if list
           size -= a->cnt;
          }
      else break;                                   // safety
    // size = args - asize(l);
   }

   l->end  = l->start+args-1;               // set size correctly
   l->size = totsize(l);                      // safety check
  return 1;
}


void do_args(SBK *s, FSTK *t)
 {
    // called from pushw or stx in emulate.

    // but not if subr has args attached ?? how to get this ???

   LBK  *l;
   SUB *sub;
   int args, addr;

   args = check_argdiff(t->newadd, t->origadd);        // safety check.....
   if (!args) return;

                 // return addr is modified
// #ifdef XDBGX
//   DBGPRT(1," ARGS REQD %x for %d bytes", t->origadd, args);
//   #endif

 //  s->args = 1;     // mark block (emulate mode)
   addr = mark_emu(s,t->origadd);                     // mark calling subr for args

   if (addr)
      {       // check if subr is manually defined
       sub = get_subr(addr);
       if (sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"ARGS IGNORED, SUB %x has cmd args (%d)", addr, sub->size);
           #endif
           return;
         }
      }

   // probably, data entries will have been created for reqd args.
   // Delete them first.........

   for (addr = t->origadd; addr < t->origadd+args; addr++)
   {
     l = find_data_cmd(addr, 0);
     if (l)
       {
        addr = l->end; // skip to end of this block
        if (l->fcom <= C_LONG) chdelete(&chdata,chdata.lastix,1);
       }
   }

   l = find_data_cmd(t->origadd,0);                   // any command here ?

if (l && l->cmd) 
   {
     #ifdef XDBGX  
      DBGPRT(0,"ARGS CMD SET for %x  (%x-%x)!!", t->origadd, l->start, l->end);
     #endif
   return;
   }
   /// NOT IF CMD SET !!!
   if (l)
     {
      if (l->fcom == C_ARGS)
        {    // a further set, probably call same getter again
        // BUT THIS DOESN'T CHECK FOR OVERLAPS !!!

         if (add_args(l, t->reg, args) > 0)
         {
         #ifdef XDBGX
         DBGPRT(1,"ARGS-extend to %d, %x-%x", args, l->start, l->end);
         #endif
         }

        }
     }
   else
     {    // if cmd not found, add a new args command
       #ifdef XDBGX
        DBGPRT(1,"ARGS-create %d, %x-%x", args, t->origadd, t->newadd-1);
       #endif
      l = add_data_cmd(t->origadd, t->newadd-1, C_ARGS,0);
      add_args(l, t->reg, args);
     }
   log_emuargs(l);
 }



void do_rbcheck(SBK *s, INST *c)
{
  OPS *o;
  RBT *b;
  uint val;

   if (!s->scanning) return;      // only in scan mode
   if (!c->wop) return;           // no writes

   o = c->opr + c->wop;           // destination operand (wop)

   if (get_flag(o->addr, rbinv)) return;    // already marked as invalid

   b = get_rbt(o->addr,0);                 // is it an rbase ?

   if (b && b->cmd)
     {
       #ifdef XDBGX
        DBGPRT(1,"cmd bans rbase update %x", o->addr);
       #endif
       return;
     }


   val = databank(o->val,c);          // if val is an address, must add bank

   // only add an RBASE if ldw or stw and immediate and not zero and not invalid
   // AD3w allowed if other op is an rbase (A9L 7096 ad3w)

   if (!b)
   {
   if (c->opcsub == 1 && o->val)
      {
       // ldw, stw or ad3w and immediate
       if (c->opcix == 41 || c->opcix == 49 || (c->opcix == 25 && c->opr[2].rbs))
        b = add_rbase(o->addr, val,0,0);
      }
   }
   else
       {   // rbase already exists, invalidate if not same value and (or?) not immediate

        if (opctbl[c->opcix].sigix == 13 && c->opcsub == 3)   return;   // not indexed stx

        if (b->val != val && c->opcsub != 1)
           {
             b->inv = 1;      // not same value
             set_flag(o->addr, rbinv);
       //      if (c->opcsub == 1) b->val = val;    // but keep it for vect lists ?
             }
       }         // end check rbase
   }


void upd_watch(SBK *s, INST *c)
 {
  OPS *o;

  if (anlpass >= ANLPRT) return;            // ignore if in print phase

  o = c->opr + c->wop;                      // destination entry (wop)

  do_rbcheck(s,c);

  if (s->scanning || s->emulating)
    {
       if (c->wop)
       {
        upd_ram(o->addr,o->val,o->wsize);

        #ifdef XDBGX

        if (anlpass < ANLPRT)
          {
           DBGPRT(0,"%05x ",s->curaddr);
           if (s->emulating) DBGPRT(0, "E");
           DBGPRT(0, "RAM %x = %x (%d)", o->addr, o->val, o->wsize);
           if (c->opcsub == 2)  DBGPRT(0, "[%x]", c->opr[4].addr);      // for indirect

//only place wop = 1 is for stx....

           if (c->opcsub == 3 && c->wop != 1)  DBGPRT(0, "[%x]", c->opr[1].addr);      // for indexed
           if (nobank(o->addr) >= PCORG) DBGPRT(0,"  ROM !!");

           DBGPRT(1,0);
          }
        #endif
       }                // end if wop (write op)

     // NOW check for autoinc ....always opr[1]

     o = c->opr+1;                     // this operand gets incremented

     if (c->opcsub == 2)               // indirect
       {    // addr & val replaced with indirect ones.  Put originals vals back for increment, but keep indirect address in [0]
        o->val = o->addr;                // value = address accessed
        o->addr = c->opr[4].addr;        // restore orig register address
        c->opr[4].val = o->val;          // save this value
       }

     if (c->inc)    // can only be set within opcsub == 2
       {     // auto increment set - update value
        o->val += casz[o->ssize];       // autoinc is by op size
                    // o->val &= camk[o->ssize];    // allow for overflow ?
        upd_ram(o->addr,o->val,2);
        check_args(s,c);            // check if qualifies as subr argument

        #ifdef XDBGX
         if (anlpass < ANLPRT)
          {
           DBGPRT(0, "%05x RAM++ %x = %x (%d)",s->curaddr,  o->addr, o->val, 2);
           if (nobank(o->addr) >= PCORG)    DBGPRT(0,"  ROM !!");
           DBGPRT(1,0);
          }
        #endif
      }
    }
}


//------------------------------------------------------------------------



void set_opstart (uint ofst)
{
  int bit, bank;
  uint *z;
  BANK *x;

  bank = (ofst >> 16) & 0xf;
  ofst = nobank(ofst);

  ofst -= PCORG;
  bit = ofst & 0x1F;       // 32 bits
  ofst = ofst / 32;

  x = bkmap+bank;
  z = x->opbt;
  z[ofst] |= (1 << bit);
  #ifdef XDBGX
    DBGnumops++;
  #endif
}

int get_opstart (uint ofst)
{
  int bit, bank;
  BANK *x;
  uint *z;

  bank = ofst >> 16;
  ofst = nobank(ofst);
  if (ofst < PCORG) return 0;

  ofst -= PCORG;
  bit = ofst & 0x1F;
  ofst = ofst / 32;

  x = bkmap+bank;
  z = x->opbt;
  return z[ofst] & (1 << bit);
}






void prt_bnks(void)
{
int i;
BANK *x;

wnprt(1,"# Banks Found. For information but can uncomment if necessary");

for (i = 0; i < BMAX; i++)
 {
  x = bkmap+i;

  if (x->bok)
    {
     wnprt(0,"# bank  %d %5x %5x %5x", i, x->filstrt, x->filend, cmdaddr(x->maxpc));
     #ifdef XDBGX
    wnprt(0,"  #  %05x-%05x (%05x)",x->minpc,x->maxbk, x->maxpc);


    if (x->cbnk) wnprt(0,"  code start (bank 8)");
    #endif
     wnprt(1,0);
    }
 }
   wnprt(1,0);
}


void prt_adt(CHAIN *x, void *fid, int dft, int (*prt) (int,const char *, ...))
{
  // print additional params for any attached ADT chain, LBK mainly
  // the 'prt' argument allows print to debug or message files
  // dcm is 1 bit

ADT *a;
int nl;

a = (ADT*) chmem(x);
a->fid = fid;

if (dft > 1) a->seq = dft; else  a->seq = -1;
nl = 0;
dft &= 1;

while ((a = get_next_adnl(x,a)))
  {
    if (a->cnt || prt != wnprt)       // use cnt, not ssize
     {    // ignore if zero size, if not debug
       if (nl) prt(0," | "); else prt(0," : ");
       if (a->newl) nl = 1; else nl = 0;      // newline is for END of block

if (a->ans)
   {
    prt(0," = ");
  //  if (a->data > 0xff ) prt(0,"[");   //not here, in print !!
    prt(0,"%x ", a->data);
   //  if (a->data > 0xff ) prt(0,"]");   
 }

       if (a->cnt > 1) prt(0,"O %d ", a->cnt);
       if (a->foff) prt(0,"D %x ", a->data);

       if (a->enc)  prt(0,"E %d %x ", a->enc, a->data & 0xff);
       else
       if (a->vaddr)
        {
         prt(0,"R ");
         //   if (numbanks)   // what about external banks ?
         //   prt(" %x", (a->data >> 16) & 0xf);
         //     prt(" ");
        }
       else
       if (a->bank) prt(0,"K %x", a->data >> 16);
       else prt(0,"%s ",calo[a->ssize]);      // neater than a case statement

       // print fieldwidth if wider than default

       if (a->pfw && a->pfw > cafw[a->ssize])  prt(0,"P %x ",a->pfw);

       if (a->dcm != dft) prt(0,"X ");       // decimal/hex
       if (a->fdat) wflprt("V ", a->fdat, prt);
       if (a->fnam) prt(0,"N ");

       #ifdef XDBGX
         if (prt == DBGPRT) prt (0,"[R%x, csz %d] ", a->dreg, cellsize(a));
       #endif
     }

  }
}


void prtflgs(int flgs,  int (*prt) (int,const char *, ...))
{

  // NEW Style.............
 uint i;

// skip default, start at 1
for (i = 0; i < NC(opts); i++)
   {
     if (flgs & opts[i].copt) prt(0, ": %s ", optstrs[i]);
     flgs &= ~opts[i].copt;      //stop dupls
   }

 }


void prt_opts(void)
{
 //print opt flags and banks
 wnprt(0,"setopts  " );
 prtflgs(cmdopts, wnprt);
 wnprt(2,0);
 
}



void prt_glo(SUB *sub, int (*prt) (int,const char *, ...))
{
  // AUTON *n;
   SPF * s;

  // print special func in style of adt
      /* special subr function
               // f 1 <reg1> <size>  <size>    func reg in , out
               // f 2 <reg1> <reg2>  <size>    tab add, cols, size

               // f 3 <reg1> <size>           variable ?
               // f 4 <?>     <size1> < size2>  switch between 2 sizes args ?
      */

  if (sub->cptl) prt (0,"$ C");

  s = get_spf(sub);

  if (!s) return;          // no special funcs

  if (!sub->cptl) prt(0,"$"); 

  prt(0," F %s %x", spfstrs[s->spf-4],s->addrreg);
 
  // tab subroutines need extra par
  if (s->spf > 12) prt (0," %x", s->sizereg);

}







void prt_rbt (void)
{
  RBT *r ;
  uint ix;

  for (ix = 0; ix < chbase.num; ix++)
   {
    r = (RBT*) chbase.ptrs[ix];

   if (!r->inv)
     {
    wnprt(0,"rbase %x %x" , r->reg , cmdaddr(r->val));
    if (r->start)  wnprt(0,"  %x %x", cmdaddr(r->start), cmdaddr(r->end));
    if (r->cmd) wnprt(0,"       # cmd");
    wnprt(1,0);
    }

   }
     wnprt(1,0);
}



void prt_psw (void)
{
  PSW *r ;
  uint ix;

  for (ix = 0; ix < chpsw.num; ix++)
   {
    r = (PSW*) chpsw.ptrs[ix];

    wnprt(0,"pswset  %x %x" , cmdaddr(r->jstart), cmdaddr(r->pswop));
    wnprt(1,0);
   }
     wnprt(1,0);
}








/************************************************
 *  Symbols
 ************************************************/

int new_symname (SYM *xsym, const char *fnam)
 {
   // replace symbol name
   // symname cannot be NULL

  int oldsize;

  if (xsym->cmd) return 0;                  // can't rename if by CMD
  if (!fnam || !*fnam) return 0;

  oldsize = xsym->tsize;                    // old size
  xsym->tsize = strlen (fnam) + 1;          // include null, strlen does not...

  if (xsym->tsize > 64) xsym->tsize = 64;   // max storage size is 32

  xsym->name = (char *) mem (xsym->name,oldsize,xsym->tsize);  // get new sym size

  if (xsym->name)  strncpy (xsym->name, fnam, xsym->tsize);    // and replace it
  xsym->name[xsym->tsize-1] = '\0';                            // safety

  return xsym->tsize;
 }





char* get_sym_name(int rw, int add, int pc)
 {
  SYM* x;    // whole 'read' symbols only

  x = get_sym(rw, add, -1, pc);
  if (x) return x->name;
  return NULL;
 }

void prt_cmds(CHAIN *z, int (*prt) (int,const char *, ...))
{                // sub command printer for both cmd and auxcmd
 uint ix, f;
 LBK *c;
 SYM *x;
 DIRS *t;


 for (ix = 0; ix < z->num; ix++)
    {
     c = (LBK*) z->ptrs[ix];
     t = dirs + c->fcom;

     if (prt == wnprt)
       {         // real printout
         if (nobank(c->start) < PCORG) continue;
         if (c->fcom == C_TIMR) prt (0,"# ");
         prt(0,"%-7s %x %x", cmds[c->fcom], cmdaddr(c->start),cmdaddr(c->end));
       }
     else
      {   // debug printout
          prt(0,"%-7s %x %x", cmds[c->fcom], c->start,c->end);
      }

     if (t->namex)         // name expected
      {
       x = get_sym(0, c->start,-1,0);
       if (x)
        {
         prt(0,"  %c%s%c  ", '\"', x->name, '\"');
         x->noprt = 1;        // printed this symbol
        }
      }

   // global options here
 
   f = 0;
   if (c->term)
      {
       prt (0," $");    
       prt (0," Q");
       if (c->term > 1) prt(0," %d", c->term);
       f = 1;
      }

   if (c->argl)
      {
       if (!f) prt (0," $"); 
       prt (0," A");
      } 

  // if (c->cptl) prt (0," C");
// now do addnlt levels
     prt_adt(&chadnl, c,t->decdft, prt);


  
     #ifdef XDBGX

     if (prt == DBGPRT)
      {
       if (c->opcsub) prt(0,  "# opcsub %d, off %x", c->opcsub, c->soff);
       if (c->from) prt(0,"         # from %x", c->from);
       if (c->cmd) prt(0,"         # cmd");
      }
  #endif
  
  if (c->sys) { prt(0,"     # autogenerated by SAD");}
     prt(1,0);
    }
   prt(2,0);
}


/*
void dbglbk(CHAIN *x,int ix, void *blk, const char *txt)
{
 LBK *k;

    DBGPRT(1,txt);

   k = (LBK*) x->ptrs[ix];

   DBGPRT(0," %d", ix);
   DBGPRT(1," %s, %x %x", cmds[k->fcom], k->start, k->end);



}
*/





SYM *new_autosym (uint nix, int addr)
{

// add auto sym with address
// all of these are whole read symbols

  SYM *xsym;
  int c;

  if (anlpass >= ANLPRT) return 0;
  if (nix >= NC(anames)) return NULL;    // safety check

  if (!(cmdopts & anames[nix].mask)) return NULL;    // autoname option not set

  // make name first

  c =  sprintf(nm,   "%s", anames[nix].pname);
  if (!numbanks) c += sprintf(nm+c, "_%x", nobank(addr));
  else           c += sprintf(nm+c, "_%x", addr);

  xsym = add_sym(C_SYS, nm, addr,-1, 0,0);     // add new (read) symbol

  if (xsym->cmd) return xsym;               // can't change a cmd sym
    
  if (nix > xsym->xnum)
    {     //only if higher level autonumber
      new_symname (xsym, nm);
      xsym->xnum = nix;
    }

  return xsym;
}


int do_tab_data_pattern(uint start, int scols, uint *apnd, uint maxend)

{

  // assume a table for now...............
// so a rectangle of numbers...
// last byte may be a filler.
// last byte/word by command may or may not be correct end....
// start should be OK though.

// how to 'trim' the sizes ???


// to decide on table structure more closely, need an array of rows-1 x cols-1
// and keep ALL the interdigit differences to get best fit.   this should also
// allow splitting up unknown blocks into probable tables.............

// func finder should be child's play compared to this..........




//looks like we need a function finder too........................
// and then a struct finder..............

  int i,j;
  uint apend;
  int size, addr;
  int rmax;
  int cmax;
  int rscore, cscore;
  int row, col, lval, val, dif;
  int cols, rsize;        // bit array  ??

  int ansrow, anscol;

  int cdif, rdif, tot, ansscore;        //test

  ansscore = 20000;
  anscol = 0;
  ansrow = 0;

  apend = *apnd;
  cols = 0;


#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"do data table patt for %x-%x cols %d, max %x", start, apend, scols, maxend);
 #endif

 for (j = 0; j < 2; j++)
 {            // for apparent and maxend

   if (!j)  size = apend-start;
   else
     {
       if (maxend == apend) break;  else  size = maxend-start;
     }

  for (i = 3; i < 32; i++)
    {
      row = size/i;
      if (row*i == size && row < 32) cols |= (1<<i);
      if (i == scols) cols |= (1<<i);                      // add suggested col
    }

#ifdef XDBGX
  if (!cols) DBGPRT(1,"no valid rows/cols for %d", size);
#endif


  for (i = 1; i < 17; i++)
   {
     cols >>= 1;
     if (cols & 1)
     {
      rmax = 0;
      rscore = 0;
      cmax = 0;
      cscore = 0;
      cdif = 0;
      rdif = 0;
      rsize = size/i;

//but this may be too many rows..........
  #ifdef XDBGX
      DBGPRT(1, "- - - - %d x %d (%d)", i, rsize, size );
  #endif

      // do rows first  (cols within each row)

      for (row = 0; row < i; row++) // for each row
        {
         addr = start+(row*rsize);              // start of this row
         lval = g_byte(addr);                   // first col of row

         for(col = 1; col < rsize; col++)   // col in row
          {
            val = g_byte(addr+col);
            dif =  abs(val-lval);
            if (dif > rmax) rmax = dif;
            rscore += dif;                     //all rows;
            lval = val;                       //update last value
          }

        }


      // do cols (rows in each col)

      for (col = 0; col < rsize; col++)   // col in row
        {
         addr = start+col;                // start of his row
         lval = g_byte(addr);               // first row of col

         for (row = 1; row < i; row++) // for each row
          {
            val = g_byte(addr+(row*rsize));
            dif =  abs(val-lval);
            if (dif > cmax) cmax = dif;
            cscore += dif;
            lval = val;
          }

         }

// max diff appears to be the real deal.

        dif = (rsize-1)*(i-1);    // no of elements
        if (dif) {
        rdif = (rscore*10) / dif;
        cdif = (cscore*10) / dif;
        }

    tot = cdif+rdif;
  #ifdef  XDBGX
       DBGPRT(1,"%dx%d  rav+cav= (%d+%d)/%d = %d", i,rsize,rdif,cdif,dif, tot);
  #endif

      if (tot >= 0 && tot < ansscore)
      {
      ansscore = tot;
      ansrow = rsize;
      anscol = i;
      }

     if (!tot && anscol < 4 && i > 4 and i < 8)
        {  // choose a middle value for small cols and all zeroes
          ansscore = -1;
          ansrow = rsize;
          anscol = i;
        }
    }     // end possible colcnt

  }  // end col count loop
}
#ifdef XDBGX

  DBGPRT(1,"LOWEST %dx%d TOT %d ",anscol, ansrow, ansscore );
#endif

if (ansrow > 16 || ansscore == 0) ansrow = 0;

 *apnd = (ansrow*anscol);
 return ansrow;
}




void do_code_scans (void)
 {

  // turn scan list into code commands.

  uint ix;
  SBK *s, *t;

  for (ix = 0; ix < chscan.num; ix++)
   {
    s = (SBK*) chscan.ptrs[ix];
    if (s->inv)
     {
       // inv block check if another block overlaps this one.
       // may be a run-on due to faulty args or a stack shortcut, etc.
       // also for debugs, can add code up to end, if it doesn't overlap with anything
       // ...THIS IS A HORRIBLE KLUDGE !!! but it works...
       if (ix < chscan.num-1)
         {
          t = (SBK*) chscan.ptrs[ix+1];
          if (!t->inv && t->start < s->nextaddr)                     // s->end
            {
             // next block not invalid and overlaps this one
             #ifdef XDBGX

             DBGPRT(1,"Invalid scan %x-%x overlapped by %x-%x",s->start, s->nextaddr,t->start, t->nextaddr);
             #endif
             add_code_cmd (s->start,t->nextaddr,C_CODE);
             s = NULL;                  //continue;                                 // return to loop
           }
       }        // ix < chscan.num
     }         // s->inv
    else
    if (s && (s->stop))            //ch || s->stopcd))
      {   // valid block to turn to code, even if inv
       add_code_cmd (s->start,s->nextaddr,C_CODE);
      }
   }            // for loop
 }


void end_scan (SBK *s)            //, int x)                    //, uint end)
{
  SUB *b;
  if (anlpass >= ANLPRT || PMANL ) return;

  s->stop = 1;
  if (s->nextaddr > s->start)
    {
     s->nextaddr--;  //is now end of block

     b = get_subr(s->substart);
     if (b && s->nextaddr > b->end) b->end = s->nextaddr;
    }

  #ifdef XDBGX
   else  DBGPRT(1," Start > End");
  #endif


  #ifdef XDBGX
   DBGPRT(0,"END scan %x at %x", s->start, s->nextaddr);
   if (s->inv)  DBGPRT (0," Invalid Set");
   DBGPRT(1,0);
  #endif

}








int decode_addr(ADT *a, uint ofst)
  {
  int rb,val,off;
  RBT *r;

  val = g_val (ofst, a->ssize);   // offsets may be SIGNED
  rb  = val;
  off = 0;

  if (a->foff)
    {            // fixed offset (as address)
     val += a->data;
     val = nobank(val);          //why ??
     return val;
    }

  switch (a->enc)
   {
    case 1:           // 1 and 3 - only if top bit set
    case 3:
     if (! (rb & 0x8000)) return val;
     off = val & 0xfff;
     rb >>= 12;            // top nibble
     if (a->enc == 3) rb &= 7;
     rb *=2;
     break;

    case 2:       // 2 and 4 Always decode
     off = val & 0xfff;
     rb >>= 12;     // top nibble
     rb &= 0xf;
     break;

    case 4:
     off = val & 0x1fff;
     rb >>= 12;     // top nibble
     rb &= 0xe;
     break;

    default:
     break;
   }

  if (a->enc)
   {
    r = get_rbt((a->data & 0xff) + rb, ofst);
    if (r)   val = off + r->val;        // reset op
    #ifdef XDBGX
    else
    DBGPRT(1,"No rbase for address decode! %x (at %x)", a->data+rb, ofst);
    #endif
   }

  if (a->vaddr)
   {    // value is  address
    val |= g_bank(ofst);   // add bank from start addr....not necessarily right
   }

  return val;
}


void avcf (SIG *z, SBK *blk)
  {
    int start, addr, val, bk;
    LBK *s;
    ADT *c;

    if (!blk->scanning) return;

    addr = z->v[4];
    start = addr;
    bk = z->start & 0xf0000;
    #ifdef XDBGX
    DBGPRT(1,"in avcf, start = %05x", addr);
   #endif
    while (1)
     {
        val = g_word(addr);
        val |= bk;
        if (!val_code_addr(val)) break;
        add_scan(val, J_SUB,0);
        addr -= 2;
     }
    s = add_data_cmd (addr, start, C_VECT, z->start);

    if (s)
     {
     // check banks here......
       c = add_adt(&chadnl, s,0);    // only one
       if (c) {
       if (bk == g_bank(addr)) c->ssize = 0;
       c->bank = 1;
       c->data = bk;         // add bank
       }
     }

    #ifdef XDBGX
    DBGPRT(1,"avcf stop at %05x", addr);
   #endif
  }



uint reg_eq(uint ra, uint rb, uint size)
{

 if (ra == rb) return 1;     // straight match

 if (ra & 1)
  {       // odd register (in) - check for word ops to match
   if ((size & 3) > 1 && (ra - 1) == rb) return 2;
  }

return 0;         // no match
}


void match_opnd(uint xofst, OPC *opl, SFIND *f)
{
   OPS *s;
   uint m, ix;

   for (ix = 0; ix < 2; ix++)
    {
     if (f->rg[ix] >= minwreg && !f->ans[ix])
      {

       if (opl->sigix == 12)
        {  // ldx - get operands
         tmpscan.curaddr = xofst;
         tmpscan.start   = xofst;
         do_code(&tmpscan,&sinst);
         s = sinst.opr + sinst.wop;       // where the write op is

         m = reg_eq(f->rg[ix], s->addr,s->wsize);
         if (m)
           {        // found register [2] = [1] for ldx
            if (sinst.opcsub == 1)
              {   // immediate, look for wop override
               f->ans[ix] = sinst.opr[1].addr;
               #ifdef XDBGX
                 DBGPRT(0," Found %d R%x = %x [%x]", ix, f->rg[ix], f->ans[ix], xofst);
               #endif
              }
            else
            if (!sinst.opcsub)
              {  // ldx from another register, change register, but NOT
                #ifdef XDBGX
                  DBGPRT(0," Chg R%x = R%x [%x]", f->rg[ix], sinst.opr[1].addr, xofst);
                #endif
                f->rg[ix] &= 1;                       // is it odd ?
                f->rg[ix] += sinst.opr[1].addr;       // replace with new reg (may be odd)
              }          // end if match reg
           }


        }    //end ldx
       else

       if (opl->sigix == 6 && opl->nops == 3)
         {      // ad3w
          tmpscan.curaddr = xofst;
          tmpscan.start = xofst;
          do_code(&tmpscan,&sinst);

          if (sinst.opr[3].addr == f->rg[ix])
           {
            if (sinst.opr[2].rbs)
             {     // ad3w with an rbase, so [3].val will be correct. can't be odd...
              f->ans[ix] = sinst.opr[3].val;
              #ifdef XDBGX
               DBGPRT(0," ad3w R%x = %x at %x", f->rg[ix], f->ans[ix], xofst);
              #endif
             }
            else
             {
              #ifdef XDBGX
               DBGPRT(0," ad3w NRB R%x at %x", f->rg[ix],  xofst);
              #endif
             }
           }
         }             // end ad3w

           // may be other pointer stuff here xdt2 does ad2w
      }     // end if f->x
    }    // end for i
 }



void set_xfunc(uint anum, SFIND f)
 {
  // set function start and one row.
  // extended later with extend_func
  // check start value for sign/unsign

  uint startval, val, szin;
  int rowsize;       // increment, entry (read) size for g_val

  ADT *a;
  LBK  *k;
  AUTON *nm;

  nm = anames+anum;

  #ifdef XDBGX
  DBGPRT(1,"Check Func %x, IS%x, OS%x", f.ans[0], nm->sizein, nm->sizeout);
  #endif

  szin = nm->sizein;      //local as it may change
  startval = (~casg[szin]) & camk[szin];     // start value

  val = g_val(f.ans[0], szin);         // read first input value (as unsigned)

  if (val != startval)  // try alternate sign....
    {
     szin ^= 4;          // swop sign flag
     #ifdef XDBGX
     DBGPRT(1,"require Swop sign in - val %x (%x)", val, startval);
     #endif

     startval = (~casg[szin]) & camk[szin];     // start value

     if (val != startval)
      {
       #ifdef XDBGX
         DBGPRT(1,"func %x Invalid start value %x, expect %x", f.ans[0], val, startval);
       #endif
       return ;
      }
    }

    rowsize = casz[szin] + casz[nm->sizeout];

    k = add_data_cmd (f.ans[0], f.ans[0] + rowsize-1, C_FUNC, 0);

    if (k && !k->cmd)                    // && !k->adnl)
      {                                     // cmd OK, add size etc (2 sets for func)

       new_autosym(3,f.ans[0]);                    // auto func name, can fail
       k->size = rowsize;                   // size of one row

       a = add_adt(&chadnl,k,0);                    // t(&(k->adnl));
       a->ssize = szin;                      // size, word or byte
       a->pfw =  cafw[a->ssize];             // allow for signed value
       a->dcm = dirs[k->fcom].decdft;        // default decimal print

       a = add_adt(&chadnl,k,1);
       a->cnt = 1;
       a->ssize = nm->sizeout;               // size out
       a->pfw =  cafw[a->ssize];             // allow for signed value
       a->dcm = dirs[k->fcom].decdft;        // default decimal print
      }
 }





void fnplu (SIG *s,  SBK *blk)
 {
    // func lookup subroutine
    // set up subroputine only - func added later by jump trace

   SUB *subr;
   SBK *t;
   SPF *f;
   int size;
   uint i, taddr;
   uint xofst, imask, omask, isize, osize;

   if (!s || !blk)     return;
   if (!blk->scanning) return;
   if (!blk->substart) return;
   if (!s->v[3])       return ;

   subr = add_subr(blk->substart);  // ok if already exists
   if (!subr) return;

   f = add_spf(subr);
   if (f->spf) return;               // already processed, assume f always OK

   size  = s->v[2]/2;                // size from sig (unsigned)
   taddr = s->v[4];                  // address register

   #ifdef XDBGX
     DBGPRT(1,"in fnplu for %x", blk->substart);
   #endif

   blk->nodata = 1;        // ALWAYS set this block (it's where sig found)
   #ifdef XDBGX
     DBGPRT(1,"set nodata %x", blk->start);
   #endif
   if (blk->start != blk->substart)
     {
       t = find_scan(blk->substart);
       if (t)
         {
          t->nodata = 1;
          #ifdef XDBGX
            DBGPRT(1,"set nodata %x", t->start);
          #endif
         }
     }     // stop any data additions in func


   osize = (s->v[10] & 0x1000) ? 4 : 0;              // signed out if bit SET, 1000 = signed if NOT set
   isize = (s->v[11] & 0x1000) ? 4 : 0;              // signed in  if bit SET, 1000 = signed if NOT set
   osize |= size;
   isize |= size;

   omask = (s->v[10] & 0xff);                         // bit mask value, sign out flag
   imask = (s->v[11] & 0xff);                         // bit mask value, sign in  flag

// look for OR opcodes if not in 'main' subroutine
// also look for changed register for address holder (ldx or stx)

   xofst = blk->substart;
   while (xofst)
    {
      if (xofst >= s->start && xofst <= s->end) break;

     // look for an OR, LDX, STX in 22 opcodes from front of subr
     xofst = find_pn_opcodes (1, xofst, 22, 3, 9, 12, 13);

     if (xofst)
       {
        if (opctbl[sinst.opcix].sigix == 9 && sinst.opr[2].addr == s->v[3])
          {
           #ifdef XDBGX
             DBGPRT(1,"find OR at %x", sinst.ofst);
           #endif
           if (omask & sinst.opr[1].val)  osize ^= 4;    // flip sign out if match;
           if (imask & sinst.opr[1].val)  isize ^= 4;    // flip sign in  if match
          }

        if (!sinst.opcsub && opctbl[sinst.opcix].sigix == 12 && sinst.opr[2].addr == taddr)
           {
             // this may also work for args, as they must be loaded via ldx ....
             taddr = sinst.opr[1].addr;
           }

        xofst++;                                      // skip to next instruction
       }
    }

   // set function type

   for (i = 5; i < 13; i++)
     {
      if (anames[i].sizein == isize && anames[i].sizeout == osize)
        {
         new_autosym(i,blk->substart);             // add subr name acc. to type
         f->spf = i;
         f->sizein  = isize;
         f->sizeout = osize;
         f->addrreg = taddr;  //address

         #ifdef XDBGX
              DBGPRT(1,"func spf=%d (%x)", i, blk->substart);
          #endif
          break;
         }
     }
 }


void set_xtab(uint ainx, SFIND f)
{

 // add table from col and rows size.
 // can't fully check, do sanity check on sizes

  uint xend;
  LBK *k;
  ADT *a;
  int ofst, rows, cols;

// probably need a maxend safety here..... and a sign check....

  ofst = f.ans[0];
  rows = 2;
  cols = f.ans[1];

  #ifdef XDBGX
   DBGPRT(1,"SET TAB Add %x cols %x Rows %x inx %x ss %d", ofst, cols, rows, ainx,anames[ainx].sizeout);
   #endif



  //rows++;                 // answer from funcs is MAX row.
  //if (rows < 2 || rows > 32) rows = 1;    // safety
  if (cols < 2 || cols > 32) cols = 1;

  xend = ofst;
  xend += (rows*cols)-1;

  k = add_data_cmd(ofst, xend, C_TABLE, 0);

  if (k && !k->cmd)                     // in case tab by user cmd....
    {
     new_autosym(4,ofst);               // auto table name, can fail
     a = add_adt(&chadnl,k,0);                 // add ONE at front
     k->size = cols;                    // size for one line ( = cols)
     a->cnt = cols;
     a->dcm = dirs[k->fcom].decdft;     // default decimal print
     a->ssize = anames[ainx].sizeout;
     a->pfw =  cafw[a->ssize];          // allow for signed value
    }

  // probably need some more checks, but simple version works for now...

}



void tbplu (SIG *s,  SBK *blk)

{
  // uint adr;
 //  int  cols, rows;
   uint xofst, taddr, tcols, osign, omask;
   SIG *t;
   SUB *subr;
   SBK *x;
   SPF *f;

   if (!s || !blk)     return;
   if (!blk->scanning) return;
   if (!blk->substart) return;
   if (!s->v[3])       return ;

   subr = add_subr(blk->substart);  // ok if it already exists

   if (!subr)       return;

   #ifdef XDBGX
     DBGPRT(0,"In TBPLU ");
   #endif

   f = add_spf(subr);
   if (f->spf) return;               // already processed, assume f always OK

   osign = 0;
   omask = 0;

   taddr = s->v[4];            // table address
   tcols = s->v[3];            // num cols

   // In tab lookup, sign flag is in the INTERPOLATE subroutine
   // subr call address saved in s->v[0] so look for tabinterp sig there

   t = (SIG*) schmem();
   t->start = s->v[0];
   t = (SIG*) find(&chsig,t);               // is it there already ?
   if (!t) t = scan_asigs (s->v[0]);        // no, so scan for it

   if (t)
     {           //   found tab signed interpolate
      #ifdef XDBGX
        DBGPRT(1," SIGN Flag = %x", t->v[3]);
      #endif

      osign = (t->v[10] & 0x1000) ? 0 : 1;
      omask = (t->v[10] & 0xff);
     }


   xofst = blk->substart;
   while (xofst)
    {
      if (xofst >= s->start && xofst <= s->end) break;
      if (t && xofst >= t->start && xofst <= t->end) break;   // safety checks
     // look for an OR, LDX, STX in 22 opcodes from front of subr
     xofst = find_pn_opcodes (1, xofst, 22, 3, 9, 12, 13);

     if (xofst)
       {
        if (t && opctbl[sinst.opcix].sigix == 9 && sinst.opr[2].addr == t->v[3])
          {                       // found register to match 't'
            #ifdef XDBGX
               DBGPRT(1,"find OR at %x", sinst.ofst);
            #endif
            if (omask & sinst.opr[1].val)  osign ^= 1;    // flip signout to match
          }

        if (!sinst.opcsub && opctbl[sinst.opcix].sigix == 12)
           {
               // ldx reg-reg this may also work for args, as they must be loaded via ldx ....
               // opcsub = 0 and set opr[1].addr
             if (sinst.opr[2].addr == taddr) taddr = sinst.opr[1].addr;
             if (sinst.opr[2].addr == tcols) tcols = sinst.opr[1].addr;
           }
        xofst++;                                      // skip to next instruction
       }
    }

   blk->nodata = 1;        // ALWAYS set this block (it's where sig found)
   #ifdef XDBGX
      DBGPRT(1,"set nodata %x", blk->start);
   #endif

   if (blk->start != blk->substart)
     {
       x = find_scan(blk->substart);
       if (x)
         {
           x->nodata = 1;
           #ifdef XDBGX
             DBGPRT(1,"set nodata %x (%x)", x->start, x->nextaddr);
           #endif
         }
     }

   // subr is (blk->substart) from above
   new_autosym(13 + osign, blk->substart);             // add subr name acc. to type

   f->spf = 13 + osign;
   f->sizein  = 0;
   f->sizeout = 1;                // need better than this ??
   if (osign) f->sizeout |=4;
   f->addrreg = taddr;           //reg holding address
   f->sizereg = tcols;           // reg holding cols

   #ifdef XDBGX
     DBGPRT(1,"tab spf=%d (%x)", f->spf, blk->substart);
   #endif
}


void encdx (SIG *s,  SBK *blk)
{

  int enc, reg;
  RST *r;

  if (!blk || !s) return;
  if (!blk->emulating) return;

  #ifdef XDBGX

   DBGPRT(0,"in E encdx");
   DBGPRT(0," for R%x (=%x) at %x", s->v[8], g_word(s->v[8]), blk->curaddr);
   DBGPRT(1,0);
  #endif

   enc = s->ptn->vflag + 2;
   if (enc == 4 && s->v[3] == 0xf)  enc = 2;            // type 2, not 4
   if (enc == 3 && s->v[3] == 0xfe) enc = 1;            // type 1, not 3


  if (s->v[9])
  {
     reg = s->v[9];
     if (s->v[1] == 2) reg = g_word(reg);     //is this always indirect ??
     // and may need to create an rgstat.....
  }
  else reg = s->v[8];

  r = find_rgstat(reg);

  if (r)
   {       // mark rg status as encoded first
       #ifdef XDBGX
   DBGPRT(1,"set enc for %x", reg);
   #endif
   r->enc = enc;
   r->data  = s->v[5];                            // 'start of data' base reg
   }
}



void ptimep(SIG *x, SBK *blk)
{
    // don't have to have a subr for this to work, so can do as
    // a preprocess, but logic works as main process too (but only once)


  LBK *k;
  ADT *a;
  uint start;
  int val,z;
  uint ofst;

  if (!blk->scanning) return;
  if (x->done) return ;

 #ifdef XDBGX
  DBGPRT(1,"in ptimep");          //move this later
 #endif

  x->v[0] = g_word(x->v[1]);

 // size in v[14].  do a 'find end' for the zero terminator
 // cmd size (Y or W) is (2 or 4)=byte (3 or 5)=word

   start = databank(x->v[0],0);            // |= datbank;
   z = x->v[14];

   #ifdef XDBGX
    DBGPRT(0,"Timer list %x size %d", x->v[0]-1, z+3);
    //if (s) DBGPRT(0," Sub %x", s->addr);
    DBGPRT(1,0);
   #endif

   val = 1;
   a = 0;

 // should also check for next cmd....and put names in here....

   ofst = start;

   while (ofst < maxadd(ofst))
    {
      val = g_byte(ofst);
      if (!val) break;
      ofst += z;
      if (val & 1) ofst+=3; else ofst+=1;
    }

    k = add_data_cmd(start, ofst, C_TIMR, 0);   // this basic works, need to add symbols ?....
    if (k) a = add_adt(&chadnl,k,0);                   // only one (for now)
    if (a) {a->ssize = z;
// a->fnam = 1;
}

  x->done = 1;

}



 /*
 *  for move into auto timer above.
 * void set_time(CPS *cmnd)
 {
  LBK *blk;
  int val,type,bank;
  int xofst;
  short b, m, sn;
  SYM *s;

  char *z;

  blk = set_cmd (cmnd->start,cmnd->end,cmnd->fcom,1);
  set_adnl(cmnd,blk);
  DBGPRT(1,0);
  new_sym(2,cmnd->symname,cmnd->start, -1);  // safe for null name

  // up to here is same as set_prm...now add names

  if (!blk) return;          // command didn't stick
  a = blk->addnl;
  if (!a) return;                     //  nothing extra to do
  sn = 0;                             // temp flag after SIGN removed !!
  if (a->addr) sn = 1;
  if (!a->name && !sn) return;       // nothing extra to do

  if (a->ssize < 1) a->ssize = 1;  // safety

  xofst = cmnd->start;
  bank = xofst &= 0xf0000;                  //cmnd->start >> 16;
  while (xofst < maxadd(xofst))
   {
    type = g_byte (xofst++);                  // first byte - flags of time size
    if (!type) break;                         // end of list;

    val = g_val (xofst, a->ssize);            // address of counter
    val |= bank;

    if (a->name) // && (cmdopts & T)
    {
     z = nm;
     z += sprintf(z, "%s", anames[0].pname);
     z += sprintf(z, "%d", anames[0].nval++);
     if (type& 0x4)  z += sprintf(z,"D"); else z+=sprintf(z,"U");
     if (type&0x20) z += sprintf(z,"_mS");
     else
     if (type&0x40) z += sprintf(z,"_8thS");
     else
     if (type&0x80) z += sprintf(z,"_S");
     s = new_sym(0,nm,a->addr,type);     // add new (read) sym
    }


    xofst+= casz[a->ssize];                           // jump over address (word or byte)

    z = nm;
    if (type&1)      // int entry
     {
      if (s && sn)
        {                      // add flags word name too
        m = g_byte (xofst++); // bit mask
        b = 0;
        while (!(m&1)) {m /= 2; b++;}    // bit number
        val = g_byte (xofst++);    // address of bit mask
        val |= bank;
        if (type & 8) z+= sprintf(z,"Stop_"); else z += sprintf(z,"Go_");
        sprintf(z,"%s",s->name);
        new_sym(0,nm, val, b);
        }
      else     xofst+=2;                // jump over extras
     }
   }


      // upd sym to add flags and times
 }



*/





int check_sfill(uint addr)
{
  int val, ofst;
  // 4 or more repeated values cannot be code....

  ofst = addr;
  val = g_byte(ofst);
  for (ofst = 0; ofst < 4; ofst++)
   {
     if (g_byte(ofst+addr) != val) break;
   }
 if (ofst < 4) return 0;

 return 1;
}





void build_fake_stack(SBK *s)
{

 // build a fake stack from caller list of s.
 // from the stack point of view, a pushp will always be
 // Higher (= older) than its call out.

  int i;
 // SBK *x;
  FSTK *c, *d;

  c = emulstack;
  d = scanstack;


   memset(c, 0, sizeof(emulstack));

   s = s->caller;

   i = 0;

   while (i < STKSZ && s)
      {
    //    if (!s->popped)
        {
         c[i].origadd = s->nextaddr;
         c[i].newadd = s->nextaddr;
         c[i].psw = 0;
         c[i].type = 1;

        if (d[i].origadd == c[i].origadd && d[i].popped)  c[i].popped = 1;  // temp fixup for A9L, seems to work ??


         if (s->php)
          {
           i++;
           c[i].type = 4;
           c[i].origadd = s->nextaddr;
           c[i].newadd = s->nextaddr;
          }
        }
        s = s->caller;
        i++;
      }
          #ifdef XDBGX
DBG_stack(d);
DBG_stack(c);
#endif
}



void emulate_blk(SBK *s)
 {
   SBK *emu, *x, *y;

   if (!s) return;               // safety
   if (anlpass >= ANLPRT) return;

   show_prog (anlpass);

   #ifdef XDBGX
     DBGPRT(2,0);
     DBGPRT(0,"In Emulate");
     DBG_sbk(0,s);
   #endif

   // first, copy the call chain from scans into the emuchain

   x = s;
   emu = add_escan(0, s);      // copy block for emu (this is actual emu)
   x = x->caller;              // next (in) block
   y = emu;                    // start of emu chain

   while (x)
    {
      y->caller = add_escan(0, x);   // add new copy of scanblock
      y = y->caller;                 // move to new emu chain end
      y->curaddr  = x->curaddr;      // correct spot for subr call
      y->nextaddr = x->nextaddr;
      x = x->caller;                 // and around for next scan block/caller
    }


// now, build a fake stack for emulate.

 build_fake_stack(emu);

 memset(emuargs, 0, sizeof(emuargs));

  opcnt = 0;               // global so it survives
  emuscan = 0;
  scan_blk(emu, &einst);

  // but if a push(imd) somewhere, then this won't be called
  // ....so fiddle it here by calling it LAST

  if (emuscan) scan_blk(emuscan, &einst);

     #ifdef XDBGX
             DBGPRT(1,"Emulate run STOP %x at %x", emu->start, emu->nextaddr);
             if (emu->inv) DBGPRT(1," with Invalid at %x", emu->curaddr);
          #endif

 clearchain(&chemul);          // clear all emulate scans

}



int check_backw(uint addr, SBK *caller, int type)
{
// check if jump backwards for possible loop condition

  int ans, ofst;
//  OPC *opl;

  ans = 0;           // continue

// basic address stuff first................
   ofst = addr - caller->curaddr;

  if (bankeq(addr, caller->curaddr))
     {                                                    // in same bank
      if (addr == caller->nextaddr) ans = 1;              // dummy jump, ignore for scans
      if (addr == caller->curaddr)  ans = 3;              // loopstop ofst = 0

      // backward jump, scanned already and conditional
      if (type == J_COND && ofst < 0 && get_opstart(addr)) ans = 2;

      if (type == J_STAT)
       {
        // check for bank start safety loopstops and opening jumps.
        if (nobank(addr) < 0x2010) end_scan (caller);               //, caller->nextaddr-1);   // END of this block

         if (ofst == -1)
           {
            //some bins do a loopstop with di,jmp...............
            if (g_byte(addr) ==  0xfa) ans = 3;
           }
       }
     }

  if (ans > 2 && caller->emulating)
    {    // only check for real loopstops in emulate
        #ifdef XDBGX
     DBGPRT(1,"Loopstop detected (%d) at %x", ans, caller->curaddr);
     #endif
     caller->stop = 1;
    }

  return ans;
}



void do_signatures(SBK *s)
 {
  SIG *g;

  if (anlpass >= ANLPRT) return;

  if (get_opstart(s->curaddr))                // has this been scanned already ?
    {
     g = (SIG*) chmem(&chsig);
     g->start = s->curaddr;
     g = (SIG*) find(&chsig,g);              // Yes, check for sig at this address
    }
  else
    {
     g = scan_sigs(s->curaddr);               // scan sig if not code scanned before (faster)
     set_opstart(s->curaddr);                 // include any prefixes in opcode flag
    }

  if (g && g->ptn->pr_sig)
    {
     g->ptn->pr_sig(g,s);         // process signature
    }
 }

void fix_args(uint addr)
{
// check argument sizes and amend
// first draft.....called when args are skipped
// to ensure all processing has been done

  RST *r;
  LBK *k;
  ADT *a;
  int i, size;

// need to check that overall size does not change


#ifdef XDBGX
 DBGPRT(1,"\nFIX args");
 DBG_rgchain();
 #endif

 a = (ADT*) chmem(&chadnl);

 for (i = 0; i < EMUGSZ; i++)
 {
   k = emuargs[i];            // next args command

   if (k && k->start == addr)
   {
    size = k->size;
    a->fid = k;
    a->seq = -1;

    while ((a = get_next_adnl(&chadnl,a)))
     {          // for each level
      r = find_rgstat(a->dreg); // find register

      if (r)
        {
            #ifdef XDBGX
         DBGPRT(0,"R%x match %x", a->dreg, addr);
         #endif
         if (r->enc)
          {     // convert to encoded...........
                 #ifdef XDBGX
           DBGPRT(1," Convert to encoded %d", r->enc);
#endif
           a->enc  = r->enc;
           a->data = r->data;
           a->fnam = 1;
           a->cnt  = 1;      // as args added in twos anyway
           a->ssize = 2;
          }
         else
          {        // not encoded, adnl in twos anyway, simple change
           if ((r->ssize & 3) > 1 && a->cnt > 1)
             {
              a->cnt  = 1;
              a->fnam = 1;
              if (a->ssize & 4) a->ssize = 6;     // signed word
              else a->ssize = 2;
              #ifdef XDBGX
              DBGPRT(1,"Change size to %d", a->ssize);
              #endif
             }
          }
        }
                  #ifdef XDBGX
      else  DBGPRT(0,"R%x not found for %x", a->dreg, addr);
      #endif
   //   a = a->next;
     }

       // sanity check here
     if (size != totsize(k))           //->adnl))
       {
                #ifdef XDBGX
        DBGPRT(1,"ADDNL SIZE NOT MATCH - FIXED");
        #endif
        // emergency fix !!
        free_adnl(&chadnl,k);            //all chain ??
        a = add_adt(&chadnl,k,0);
        a->cnt = size;
       }

   }

 }
}

void skip_args(SBK *s)
 {
   LBK *k, *e;
   int i;

   k = (LBK*) 1;   // to start loop..

   while (k)
    {
     k = find_data_cmd(s->curaddr,C_ARGS);
     if (k)
       {
        s->curaddr += k->size;
        #ifdef XDBGX
         DBGPRT(1,"SKIP %d args -> %x", k->size, s->curaddr);
        #endif

        // fix args with new sizes, encode, etc
        // cleanup reg status and arg holders
        // clear rgargs matching k->start
        // clear emuargs matching k->start

          fix_args(k->start);

          for (i = 0; i < EMUGSZ; i++)
             {
               e = emuargs[i];
                   #ifdef XDBGX
               if (e) DBGPRT(1,"EMUARGS = %x (%d)", e->start, i);
               #endif
               if (e && e->start == k->start)
               {
                       #ifdef XDBGX
                DBGPRT(1,"drop EMUARG %d = %x", i, e->start);
                #endif
                emuargs[i] = 0;
               }
             }
        }
       }
    }











void scan_blk(SBK *s, INST *c)
 {
   // used for both scan AND emulate
   if (!s) return;              // safety

   show_prog (anlpass);

   // need to check that it's not a faulty filler block......

   if (!s->stop && check_sfill(s->curaddr))
     {
      s->stop = 1;
      s->inv = 1;
      #ifdef XDBGX

       DBGPRT(1,"Invalid (rpt) %05x from %05x",s->start, s->curaddr);
      #endif
      return;
     }

     if (s->stop || s->inv)
       {
         #ifdef XDBGX
         DBGPRT(1,"Ignore Scan %x", s->start);
         #endif
         return;
        }

     #ifdef XDBGX
       DBG_sbk(0,s);
       #endif

   tscans++;

  //  if (s->scnt > 2000)
  //    {
  //    s->scanned = 1;
  //    DBGPRT(0,"too many scans !"); return;
  //   }

   s->scnt++;                                   // scan count by block

   while (!s->stop && !s->inv)                 // inv seems to stop loops, but should it be overridden with args detection ?
    {
     skip_args(s);                             // check for ARGS command(s)
     do_signatures(s);                         // moved to here

     do_code (s, c);                           // do opcode
     s->curaddr = s->nextaddr;                 // next opcode address

     if (s->emulating)
      {                    // safety check for code loops
       opcnt++;

       if (opcnt > 10000)
       {
        s->stop = 1;
            #ifdef XDBGX
        DBGPRT(1,"Hit opcode limit %x  %x", s->start, s->curaddr);
            #endif
       }
      }
    }

    #ifdef XDBGX
     DBGPRT(0,"Stop ");
     if (s->scanning)  DBGPRT(0,"scan");
     if (s->emulating) DBGPRT(0,"Emu");
     DBGPRT(1," blk %x at %x", s->start, s->nextaddr);
    #endif

}



void scan_subbs(SBK* caller, INST *c)
 {
  // scan proc sub branches too see if stack fiddling
  // is in branches.  Not reqd if already flagged
  // ADAPT for twin role as subr-sub or gap check ??

  SBK *s;
  uint ix, num;

  if (!caller) return;
  ix = 0;

  num = chsbcn.num;

  while (ix < chsbcn.num)
   {

    s = (SBK*) chsbcn.ptrs[ix];

    if (!s->stop && !s->inv && s->scnt < 10)
      {
        if (s->substart == caller->substart)
         {
          #ifdef XDBGX
             DBGPRT(1,0);
             DBGPRT(0,"Subb Scan %d ", ix);
          #endif
          scan_blk(s, c);
          if (chsbcn.num != num)
            {
             #ifdef XDBGX
             DBGPRT(1,"NEWCH %d %d", chsbcn.num, num);
             #endif
             ix = -1;   // rescan if changed
             num = chsbcn.num;
            }
         }
      }
    ix++;
   }

  clearchain(&chsbcn);       // clear all temp subdiv branches
 }


void fakepop(SBK *s)
{
  // drop newest stack entry - underflow safe
  // as push, copy whole stack if necessary,
  // less efficient, but better for keeping track
  // KEEP last pop in entry zero, which is useful for
  // stack shuffling par getters

  FSTK *t;

  if (s->emulating)  t = emulstack; else t = scanstack;

  memmove (t, t+1, sizeof (FSTK) * (STKSZ-1));    // shuffle sce down, (over [0])
  memset  (t+ (STKSZ-1),0, sizeof(FSTK));          // and clear last entry
}


int get_stack(SBK *s, FSTK *t, int types)
{
 // get stack entry, with bit mask of types allowed
  int match, ans;

  match = 0;
  ans = t->newadd;   // default return;

  if (t->type & types) match = 1;        // OK

  switch(t->type)
    {

     default:
     case 1:    // std call, default
     break;

     case 2:    // imd
       ans = t->psw;          // return address as pushed
       break;

     case 4:                  // pushp addr = psw bank goes in 10-13
       ans = t->psw;         // psw
       ans |= ((t->origadd >> 6) & 0x3c00);   // add callers bank in correct place
       break;
   }

if (!match && s->emulating)
  {
    ans = t->origadd;

    #ifdef XDBGX
    DBGPRT(1,"STACK TYPE not match %x != %x at %x", t->type, types, s->curaddr);
    DBG_stack(emulstack);
    #endif
  }

return ans;
}


void fakepush(SBK *caller, int addr, int type)
{

// add stack entry - overflow safe
// stack shuffle is less efficient than a pointer,
// but will always have newest PUSH at zero which is neater.
// also right way round for [R22+x] style multibanks
// type 1 = std call, 2=imd, 4 = pushp as bit
// always have newaddr and origaddr as real addresses, so if it is
// used incorrectly for a RET, it will just continue.

  FSTK *t;

   if (!caller) return;

   if (caller->emulating)  t = emulstack;
   else
    {
      // don't do immediate pushes in scan mode
      if (type == 2) return;
      t = scanstack;
     }

   memmove (t+1, t, sizeof (FSTK) * (STKSZ-1));  // shuffle stack up
   memset(t,0, sizeof(FSTK));       // clear entry[1]

   t->type    = type;
   t->newadd  = caller->nextaddr;
   t->origadd = caller->nextaddr;
   t->psw     = addr;

}


int check_caller_chain(int addr, SBK *caller)
 {
  SBK * x;
  int ans;

  ans = 0;
  x = find_scan(addr);         //,0);
  if (!x) return ans;          // no block

  while (caller)
    {
      if (x == caller)
        {
         ans = 1;
         break;
        }
      caller = caller->caller;
    }
 return ans;
}


void do_sjsubr(SBK *caller, INST *c, int type)
{
  /* do subroutine or jump
  * new scanned block (or found one) may become holder of this block
  * for subroutine special funcs (and any args)
  */

  SBK *newsc, *x;                        // new scan, or temp holder
  SUB *sub;
  ADT *a;
  int addr, saveddbnk;

  if (PMANL) return;                // no scans, use default
  if (anlpass >= ANLPRT) return;

  addr = c->opr[1].addr;


  switch (type)                                // requested scan type
    {

     default:
       #ifdef XDBGX
       DBGPRT(0,"Error at %x unknown type =%d", caller->curaddr,type);
       #endif
       break;

     case J_RET:                     // return

         // end block and drop back to caller emulate and scan
         if (caller->caller) add_jump(caller, caller->caller->nextaddr, J_RET);
         else add_jump (caller, caller->curaddr, J_RET);
// mark end of subr ??


         end_scan (caller);              //, caller->nextaddr-1);        // END of this scan block
         break;


     case J_COND:                        // conditional jump

        /* don't necessarily need to add another scan here as code CAN continue, but
         * necessary to get any alternate addresses etc. via parameter saves (e.g. for tables)
         * backwards conditionals are probably always local loops
         * duplicates screened out by add_scan
        */
          add_jump(caller, addr, type);             //jumps BEFORE checks (for loopstops)
          if (check_backw(addr, caller, type)) break;     // backward jump probably a loop



          if (PLABEL) new_autosym (2, addr);                 // add autolabel if reqd
          add_scan (addr,type, caller);                   // where cond continues
          break;

     case J_STAT:                       // static jump
        /* backwards static jumps are also probably a code loop as well, but some bins
         * have a common 'return' point, so can't assume this.
         * also a static jump can go to a subroutine start, as per signed and
         * unsigned function and table lookups, and so on.
         */

         add_jump(caller, addr, type);
         if (check_backw(addr, caller, type)) break;     // backward jump probably a loop
        
         if (PLABEL) new_autosym (2, addr);                 // add autolabel if reqd

         end_scan (caller);                              // END of this block
         newsc = add_scan (addr, type, caller);          // new or duplicate scan
         scan_blk(newsc,c);
         break;

     case J_SUB:

         add_jump(caller, addr, type);

         if (check_backw(addr,caller,type)) break;
         if (check_caller_chain(addr, caller)) break;

         newsc = add_scan (addr,type,caller);                // nsc = new or zero if dupl/invalid

         fakepush(caller, addr,1);                     // need this for later emulation check

         scan_blk(newsc,c);

        // scan subbranches of this subr for emulation before subr exit
        scan_subbs(newsc, c);

        if (newsc)
         {
           sub = get_subr(newsc->substart);
           a = get_adnl(&chadnl,sub,0);
           if (a)                    //sub && sub->adnl)
            {            // args set by user command no args, just skip
                  //note that cannot do multi level args in a command...................
               #ifdef XDBGX
               DBGPRT(0,"CMD ARGS ");
               #endif
               caller->nextaddr += sub->size;          //sub->adnl);
            }
           else
            {
             if (newsc->args)
              {                  // or here - need a level indicator ???
           //    #ifdef XDBGX
          //     DBGPRT(1," ARGS FOUND %x (s%x) set emulrqd for %x (sub %x)", newsc->start, newsc->substart, caller->start, caller->substart);
          //     #endif

              x = find_scan(caller->substart); // this is only true if single level args getter ?? and not if static jump....
              
               if (x)
                 {

        //           sub = get_subr(x->start);
          //         if (sub && sub->size)
            //       {
        //            #ifdef XDBGX
        //              DBGPRT(1," ignore %x args set", x->start);
        //            #endif
        //            newsc
        //           }
        //           else        // check x for defined args *here* ??

           
                  #ifdef XDBGX
                   if (!x->emulreqd && x!= newsc) {DBGPRT(0,"%x - ARGS Set emulreqd for %x (%x)", c->ofst, x->start, x->substart);
                      DBGPRT(1," new %x (%x) C %x (s%x) %x", newsc->start, newsc->substart, caller->start, caller->substart, caller->curaddr);
                   }
         //          DBGPRT(0,"XZXZ N %x", newsc->start);
        //           if (x != newsc) DBGPRT(0, " != "); else DBGPRT(0," = ");
        //           DBGPRT(0, "E %x", x->start);
        //           if (x != caller) DBGPRT(0, " != "); else DBGPRT(0," = ");
       //            DBGPRT(1,"C %x",caller->start);
                  #endif
                        x->emulreqd = 1;  
                 }               // from substart
              
  
                 // but if this is in the currrent caller chain, need to go back to it ???
                 // find scan doesn't check if scan is in current chain....and just assumes args are in CALLER,
                 // which is NOT always true........
                 
                 
              }  


             if (newsc->emulreqd)     // && !caller->emulating)
               {
                 newsc->caller = caller;         // reset for stack rebuild
                 saveddbnk = datbnk;
                 emulate_blk(newsc);             // and emulate it
                 datbnk = saveddbnk;
               }
           }
         }
    #ifdef XDBGX
        DBGPRT(1,"Resume at %x",caller->nextaddr);
        DBGPRT(1,0);
    #endif
        fakepop(caller);

    }        // end switch

}



/***********************************************
 *  calculate operand parameter number 'ix'
 *  from 'pc' (normally the register address)
 *  assumes read size already set in operand cell (OPS)
 *  sym names and special fixups done in print phase
 ************************************************/

void op_calc (int pc, int ix, INST *c)
  {
  OPS *o;
  RBT *b;

  o = c->opr+ix;

  o->addr = pc;
  o->rbs = 0;
  b = 0;

  if (o->addr < 0x100)   o->addr += rambnk;     // always zero if not 8065

//but if later 8065, odd o_addr in word mode = + 0x100 !!     opt   if (PSTPD)


  o->addr = databank(o->addr,c);                // add bank



  if (o->rgf)
     {                                          // if register (default)
      o->addr &= 0xff;                          // safety - should be redundant
      o->addr += rambnk;                        // rambank always zero if not 8065
      b = get_rbt(o->addr,c->ofst);
     }

  if (o->imd || o->bit) o->val = pc;            // immediate or bit flag, so pc is absolute value
  else o->val = g_val(o->addr,o->ssize);        // get value from reg address, sized by op

 if (b && c != &einst)
   {                                            // don't do rbase if in emu mode
    o->rbs = 1;
    o->val = b->val;                            // get value from RBASE entry instead
   }
  // indirect and indexed etc cases handled in calc mult_ops (do code)
}

void op_imd (INST *c)
{
  // convert op[1] to imd
  OPS *o;

  o = c->opr + 1;
  o->imd  = 1;
  o->rgf  = 0;
  o->rbs  = 0;
// o->sym ??
  if (rambnk) o->addr -=  rambnk;               // remove any register bank
  o->val  = o->addr;
}



void do_vect_list(INST *c, uint start, uint end) // SBK *caller
 {

// no checks..................
  uint pc, vcall;
  LBK  *s;
  ADT *a;

  if (anlpass >= ANLPRT) return;

  pc = start;

  // would need to do PC+subpars for A9L proper display

   #ifdef XDBGX
    DBGPRT(1,"do vect list %x-%x", start,end);
   #endif

  for (pc = start; pc < end; pc +=2)
    {
     vcall = g_word (pc);                            // address from list
     vcall = codebank(vcall, c);                     // assume always in CODE not databank
     if (val_code_addr(vcall))
     add_scan (vcall, J_SUB, 0);                     // vect subr (checked inside cadd_scan)
    }


  s = add_data_cmd (start, end, C_VECT,0);
  if (s && codbnk != datbnk)
    {
     // bank goes in ONE addn block.
       a = add_adt(&chadnl,s,0);
       if (a) {
       a->bank = 1;
       a->data = codbnk;          // add current codbank
       if (g_bank(vcall) == g_bank(start)) a->ssize = 0;
       }
    }
     #ifdef XDBGX
     DBGPRT(1,"END vect list");
     #endif
 }

/******************************************
attempt to decode vector or vector list from push word
probably need to split off the vect list (for multibanks)
****************************************/

int check_vect_list(SBK *caller, uint start, uint sz, INST *c)
{
   uint score;
   uint i, addr, lowcadd, lastiadd, end;
   SBK *s, *t;
   LBK *l, *k;

   // assume bank is correct in start
   if (nobank(start) < PCORG) return 0;

   if (sz) end = start + sz;               // 0 to sz is an extra pointer sometimes.
   else   end = start + 0xff;             // 128 vectors max (A9L is 0xa4)


   score = 0;                     // max score
   lowcadd = maxadd(c->ofst);              // bank max for call addresses - multibank ?
   lastiadd = 0;

  // check start against curinst+cnt to see if an extra offset needs to be added ?
   //this needs probably more checks

   if (start & 1) start++;             // can't start on odd byte, but end should be
   if (!(end & 1)) end --;             // drop last byte if even

   #ifdef XDBGX
    DBGPRT(0,"In check_vect %x sz=%x lc=%x", start, sz, lowcadd);
    DBGPRT(1," from %x", c->ofst);
   #endif

   t = (SBK*) chmem(&chscan);
   k = (LBK*) chmem(&chdata);


   for(i = start; i <= end; i+=2)       // always word addresses  (score > 0?)
     {

      if (i >= lowcadd)
       {
         #ifdef XDBGX
         DBGPRT(1,"hit lc_add %x", i);
         #endif
         break;
       }

      t->start = i;  // for check of SBK
      k->start = i;  // for check of commands

      s = (SBK*) find(&chscan,t);
      if (s)
        {
         #ifdef XDBGX
          DBGPRT(1,"hit scan %x", i);
         #endif
         end = i-1;
         break;
        }

      l = (LBK *) find(&chcode,k);
      if (l)
        {
         #ifdef XDBGX
           DBGPRT(1,"hit code %x %s",i,cmds[l->fcom]);
         #endif
         end = i-1;
         break;
        }


//CHECK VECT !!!



      l = (LBK *) find(&chdata, k);
      if (l && l->fcom > C_VECT)
        {                  // just score this one
         score +=2;
         #ifdef XDBGX
          DBGPRT(1,"hit Data %x=%x",i, l->start);
         #endif
        }


      // now check the vect address itself

      // add extra check for end address, as size can be >,<, <=

      addr = g_word (i);
      addr = codebank(addr, c);

      if (!val_jmp_addr(addr))
        {
         #ifdef XDBGX
           DBGPRT(1,"iadd %x (%x)",i, addr);
         #endif
         score +=2;
         if (i == start) start +=2;    // first address is invalid, move start

         if (!lastiadd) lastiadd = i;           // keep last invalid address
         else
           {
            // end = i-1;                            // not allow 2 invalids
            end = lastiadd-1;
            break;
           }
         addr = 0;                               // remove address
        }

       // lowest call address - but this doesn't work for multibanks !!

      if (addr > start && addr < lowcadd) {lowcadd = addr; }       // keep lowest found addr

      if (addr && (addr & 0xff) == 0)     // suspicious, but possible
        {
         score ++;                 // just add 1
         #ifdef XDBGX
          DBGPRT(1,"Suspicious %x = %x",i, addr);
         #endif
         }
       // more scores ??

   }  // end main loop

  if (lowcadd < end) end = lowcadd-1;


 #ifdef XDBGX
   DBGPRT(1,"end at %x", end);
   DBGPRT(1,"Check_vect %x-%x (%d)|lowc %x|score %d", start, i, i-start, lowcadd,score);
  #endif



// add a SIZE check

   if (end - start < 15)
   {                // is a minimum of 8 pointers
     #ifdef XDBGX
     DBGPRT (1,"REJECT LIST %x from %x, %d ptrs",start, c->ofst, (end-start)/2);
     #endif
     return 0;
   }

  addr = i-start;
  if (addr > 0)
   {
     score = (score*100)/addr;
      #ifdef XDBGX
     DBGPRT (0,"FINAL SCORE %d",score);
      if (score < 10)   DBGPRT (0,"PASS"); else DBGPRT(0,"FAIL");
     DBGPRT(1,0);
     #endif

     if (score < 10) do_vect_list(c, start,end);
     return 1;
   }
  return 0;
}


JMP * find_fjump (uint ofst, int *x)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 8 for 'else', 4 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JMP * j;

  if (anlpass < ANLPRT) return 0;

  *x = 0;

  j = (JMP*) chmem(&chjpf);
  j->fromaddr = ofst;
  j = (JMP*) find(&chjpf,j);         // jump from

  if (!j) return 0;

  if (!j->jtype)  *x = 0;   // true return

    if (j->retn)  *x = 2;      // jump to a return

//  if (j->jtype == J_ELSE) *x = 4;
  if (j->obkt) *x = 1;     // bracket - reverse condition in source

if (j->jelse) *x |= 4;          //pstr(" } else { ");
if (j->jor)   pstr(" ** OR **");

  return j;
}






int find_tjump (uint ofst)
{
  // find trailing jump, for brackets, matches fjump
  // prints close bracket for each jump found

 JMP * j;
 uint ix, ans;

 if (anlpass < ANLPRT) return 0;
 if (!PSSRC) return 0;

 ans = 0;
 ix = find_tjix(ofst);   // find first to jump
 
 while (ix < chjpt.num)
  {
   j = (JMP*) chjpt.ptrs[ix];
   if (j->toaddr == ofst)
    {
     if (j->cbkt)
       {
       p_pad(cposn[2]);
       pstr ("}");
       ans ++;
indent--;                        //test
      }
     ix++;
    }
   else break;    //stop as soon as different address
  }
  return ans;
}


void pp_hdr (int ofst, const char *com, short cnt)    //, short pad)
{
  char *tt;
  int sym;

  if (cnt & P_NOSYM) sym = 0; else sym = 1;
  cnt &= (P_NOSYM-1);         //max count 255

  if (!cnt) return;

  newl(1);                                         // always start at a new line for hdr
  if (sym)
    {                                            // name allowed
     tt = get_sym_name(0,ofst,0);                  // is there a symbol here ?
     if (tt)
      {
       p_pad(3);
       pstr ("%s:", tt);
       newl(1);                                  // output newline and reset column after symbol
      }
    }

  if (!numbanks) pstr ("%4x: ", nobank(ofst));
  else pstr ("%05x: ", ofst);

  while (--cnt) pstr ("%02x,", g_byte(ofst++));
  pstr ("%02x", g_byte(ofst++));

  if (com)
   {
    if (gcol > cposn[0]) p_pad(gcol+2); else p_pad(cposn[0]);
    pstr ("%s",com);
    if (gcol > cposn[1]) p_pad(gcol+2); else p_pad(cposn[1]);
   }

 }

/*************************************
get next comment line from file
move to combined bank+addr (as single hex)
**************************************/

int get_cmnt (CMNT *f)
{
  int ans;
  int maxlen, rlen;
  uint pcz;
  char *t;

  ans = 0;
  rlen = 0;
  maxlen = 0;
  pcz = 0;

  f->ctext = flbuf;

  if (fldata.fl[4] == NULL || feof(fldata.fl[4]))
   {
    f->ofst = 0xfffff;          // max possible address with max bank
    f->ctext = (char *) empty;
    return 0;                 // no file, or end of file
   }

  while (ans < 1 && fgets (flbuf, 255, fldata.fl[4]))
    {
     maxlen = strlen(flbuf);   // safety check
     ans = sscanf (flbuf, "%x%n", &pcz, &rlen);    // n is actual no of chars read to %n
    }

  if (rlen > maxlen) rlen = maxlen;

  if (ans > 0)
    {
      if (!numbanks  && pcz < 0xffff)  pcz += 0x80000;  // force bank 8

      f->ofst = pcz;
      f->ctext = flbuf+rlen;
      while (*(f->ctext) == 0x20) (f->ctext)++;     // consume any spaces after number
      t = f->ctext;
      if (*t == '\\' && *(t+1) == 'n') f->nl = 1;    // newline at front of cmt
      else f->nl = 0;
      return 1;
    }

  f->ctext = (char *) empty;    //anything else is dead and ignored...
  f->ofst  = 0xfffff;           // max possible address with max bank
  return 0;

}

// incorrectly stacked comments cause filled spaces (cos of print...)


char* symfromcmt(char *s, int *c, int ofst, int *bit)
 {
   int ans;
   uint add, p;
   SYM *z;

  if (!isxdigit(*s))
   {
    *c = 0;
    return 0;   // anything other than digit after special char not allowed
   }
   *bit = -1;
   z = 0;

   ans = sscanf(s,"%x%n:%x%n",&add,c,&p,c);

   if (ans <= 0) return NULL;

   if (ans <= 1) *bit = -1; else *bit = p;
   // add,{bit}  with add or add,bit allowed


   if (add < PCORG) {}                //add |= RAMBNK;
   else
   if (!numbanks && add <= 0xffff)
     {
      add |= 0x80000;                   // default to bank 8
     }

   z =  get_sym (0, add, *bit, ofst);

   //try without bit if not found
   if (!z && *bit >= 0) z =  get_sym (0, add, -1, ofst);
   else *bit = -1;

   if (z) return z->name;
   return NULL;
 }

// print indent line for extra psuedo code lines

void p_indl (void)
{
  if (gcol) newl(1);
  p_pad(cposn[2]);

}


int pp_adt (int ofst, ADT *a, int pfwo)
 {

// print a SINGLE ITEM from *a, even if a->cnt is set
// done this way for ARGS printouts.

  int pfw, val;
  SYM* sym;

  sym = NULL;
  pfw = 1;                               // min field width

  // pfw is still not always right in a struct, especially if names appear.
  // also args always needs min fieldwidths, so allow for a->pfw at zero

  if (pfwo) pfw = plist[pfwo];
  else
  {  
  if (a->pfw)
   {                    // NOT FOR ARGS !!
    if (a->pfw < cafw[a->ssize]) pfw = cafw[a->ssize];    // default to preset min
    else pfw = a->pfw;
   }
  }
  val = decode_addr(a, ofst);                        // val with decode if nec.  and sign - READS ofst
  if (a->fnam) sym = get_sym(0, val,-1,ofst);        // READ sym AFTER any decodes - was xval

  if (sym) pstr("%*s",-pfw,sym->name);
   else
  if (a->fdat)
     {            // float - probably need to extend pfw to make sense...but what if you DON'T want a float ??
            // this will also fill spaces if pfw set
      bprtfl((float) val/a->fdat, pfw+4);         // add 4 for float part
      pstr("%s", nm+64);
     }
   else
  if (a->dcm) pstr("%*d",pfw, val);   // decimal print
    else
     { // hex prt
      //if (!numbanks) don't KNOW whether this is a value or an address...vaddr
      val = nobank(val);      // but NOT for bank addresses !
      pstr("%*x", pfw, val);
     }

  return casz[a->ssize];
}



int pp_lev (void *fid, int ofst, int *seq, int *mitem)
{

// ALWAYS stops at a newline.

// need to add CUTOFF at an end point so it does not overrun
// unless command verifies and fixes size first (better)

//start indent at plist[1] and go up from there.
// in here is Ok as it always does one row

 int i, item;
 ADT *a;

 i = 0;
 item = 0;
 
 a = (ADT*) chmem(&chadnl);
 a->fid = fid;
 if (!seq)  a->seq = -1; else a->seq = *seq;

 while ((a = get_next_adnl(&chadnl,a)))
  {
   if (i) pstr(", ");       // true if looping on a

   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     if (i) pstr(", ");
     if (mitem) item = *mitem;           // for column pads
     pp_adt(ofst, a, item);
     if (mitem)
       {
        (*mitem)++;   
        item = *mitem;           // for column pads
       }
     ofst += casz[a->ssize];
    }

//probably need a wrap check here too.............


   if (a->newl)
    {
      if (mitem)       // all cols
       {
        *mitem = 1;   
         item = 1;           // restart column pads
       }  
      pchar(',');
      break;
    }
  }
  
 if (seq)
  {
   if (a) *seq = a->seq; else *seq = -1;
  }
  // return size....?
  return 0;
}


















//*********************************

uint pp_text (uint ofst, LBK *x)
{
  int cnt;
  short i, v;
  int xofst;

  xofst = x->end;
  cnt = xofst-ofst+1;

  if (cnt > 15) cnt = 15;              // was 16, for copyright notice
  if (cnt < 1)  cnt = 1;

  pp_hdr (ofst, cmds[x->fcom], cnt);

  xofst = ofst;

  p_pad(cposn[3]);
  pstr("\"");
  for (i = cnt; i > 0; i--)
     {
      v = g_byte (xofst++);
      if (v >=0x20 && v <= 0x7f) pchar (v); else  pchar('.');
     }
  pchar('\"');

  return ofst+cnt;
}

//***********************************
// byte, word, long  - change name position
//***********************************

uint pp_wdbl (uint ofst, LBK *x)
{
  ADT *a;
  char *tt;
  DIRS *d;

  // local declares for possible override
  uint com, ssize, pfw, val;


  a = get_adnl(&chadnl,x,0);

  com = x->fcom;
  d = dirs + com;

  if (!a)
   {    // no addnl for pp-lev
    ssize = x->fcom & 3;     // safe for byte,wd,long, unsigned
    pfw   = cafw[2];        // WORD, so that bytes and word values line up
   }
  else
   {
    ssize = a->ssize;
    pfw   = a->pfw;
   }


  if (((x->end - ofst)+1) < casz[ssize])
    {
     // can't be right if it overlaps end, FORCE to byte
     ssize = 1;
     com = C_BYTE;
  }

  // no symbol print, do at position 2
  pp_hdr (ofst, cmds[com], casz[ssize] | P_NOSYM);

  if (pfw < cafw[d->defsze])  pfw = cafw[d->defsze]; // not reqd ??

  if (a) pp_lev(x, ofst, 0, 0);       // use addnl block(s)
  else
   {
     // default print
     val = g_val (ofst, ssize);
     if (dirs[x->fcom].decdft)
        pstr("%*d", pfw, val);   // decimal print
     else
        pstr("%*x", pfw, val);
   }

  // add symbol name in source code position
  // but this should only be for SINGLE entries ?

  tt = get_sym_name(0, ofst,0);

  if (tt)
    {
     p_pad(cposn[2]);
     pstr ("%s", tt);
    }

  ofst += casz[ssize];

  return ofst;
}




uint pp_vect(uint ofst, LBK *x)
{
  /*************************
  * A9l has param data in its vector list
  * do a find subr to decide what to print
  * ALWAYS have an adnl blk for bank, even if default
  * ***************/

  int val, size, bank;
  char *n;
  ADT *a;
  LBK  *y;

  a = get_adnl(&chadnl,x,0);

  size = 0;
  val = g_word (ofst);

  if (a) bank = a->data; else bank = g_bank(x->start);

  val |= bank;               // add bank;

  y = (LBK*) chmem(&chcode);
  y->start = val;

  y = (LBK*) find(&chcode,y);                 // look for CODE...

  if (!y) size = 2;

  if (ofst & 1)  size = 1;

  if (size)
      {
      pp_hdr (ofst, cmds[size], size);         // replace with WORD or BYTE
      }
   else
    {
     pp_hdr (ofst, cmds[x->fcom], 2);
     size = 2;                                      // move to next entry
    }
  n = get_sym_name (0, val,0);

  paddr(val);

  if (n)
     {
     p_pad(cposn[2]);
     pstr ("%s", n);
     }
  return ofst+size;
}

//  print a line as default

uint pp_dmy(uint ofst, LBK *x)
{
 return ofst+1;       // don't print anything.
}


uint pp_dflt (uint ofst, LBK *x)
{
  int i, cnt, sval;       // for big blocks of fill.....

  cnt = x->end-ofst+1;

  sval = g_byte(ofst);            // start value for repeats

 for (i = 1; i < cnt; ++i)        // count equal values
    {
      if (g_byte(ofst+i) != sval) break;
    }

  if (i > 32)   // more than 2 rows of fill - compress
     {
      newl  (1);
      paddr (ofst);
      pstr  (" -> ");
      paddr (ofst+i-1);
      pstr  (" = 0x%x  ## fill ## ", sval );
      newl  (1);
      return ofst+i;
    }

  if (cnt <= 0 ) cnt = 1;
  if (cnt > 16) cnt = 16;      // 16 max in a line


  if (ofst + cnt > x->end) cnt = x->end - ofst +1;

 // if (i >= cnt)
  //   {
  //    pp_hdr (ofst, "???", cnt);
  //    return ofst+cnt;
  //  }

  if (x->cmd)
   pp_hdr (ofst, cmds[0], cnt); 
  else   
   pp_hdr (ofst, "???", cnt);

  return ofst+cnt;
}

// this works for actual structure - need to sort up/down etc

uint pp_timer(uint ofst, LBK *x)
{
  int val,cnt;
  int xofst;

  SYM *sym;
  short bit, i;
  ADT *a;

  sym = 0;
  a = get_adnl(&chadnl,x,0);           //x->adnl;
  if (!a)  a = set_adnl(cmdadnl);      // safety

  xofst = ofst;

  val = g_byte (xofst++);  // first byte
  if (!val)
    {
      pp_hdr (ofst,cmds[C_BYTE],1);    // end of list

      return xofst;
     }

  cnt = (val & 1) ? 3 : 1;     // int or short entry
  pp_hdr (ofst, cmds[C_TIMR], cnt+casz[a->ssize]);
  pstr("%2x, ", val);
  val = g_val (xofst, a->ssize);
  if (a->fnam) sym = get_sym(0, val,-1,xofst);           // syname AFTER any decodes
  if (sym) pstr("%s, ",sym->name);
  else pstr("%5x, ",val);
  xofst += casz[a->ssize];

  if (cnt == 3)
   {                // int entry
    i = g_byte (xofst++);
    bit = 0;
    while (!(i&1) && bit < 16) {i /= 2; bit++;}          // LOOP without bit check !!
    val = g_byte (xofst);

     if (a->fnam) {sym = get_sym(0, val,bit,xofst);
     if (!sym) sym = get_sym(0,val,-1,xofst); }
    if (sym) pstr("%s, ",sym->name);
    else  pstr(" B%d_%x",bit, val);
  //  xofst++;                            // more later ??

   }


  return ofst+cnt+casz[a->ssize];
}



void p_op (OPS *o)
{
 // bare single operand - print type by flag markers

 if (o->imd) pstr ("%x", o->val & camk[o->ssize]);      // immediate (with size)
 if (o->rgf) pstr ("R%x",o->addr);                      // register, already 0xff | rambank
 if (o->inc) pstr ("++");                               // autoincrement
 if (o->bit) pstr ("B%d",o->val);                       // bit value
 if (o->adr) paddr(o->addr);                            // address - jumps
 if (o->off) pstr ("+%x", o->addr & camk[o->ssize]);     // address, as offset (indexed)

}


void p_oper (INST *c, int ix)
{
  // bare op print by instance and index - top level
  // if indexed op. append "+op[0]" with op[1]

  OPS *o;

  ix &= 0x3;                      // safety check

  o = c->opr+ix;

  if (o->bkt)  pchar ('[');
  p_op (o);
  if (ix == 1 && c->opcsub == 3) p_op (c->opr);
  if (o->bkt)  pchar (']');
  return;
}


void p_sc (INST *x, int ix)
{
// single op print for source code for supplied instance index
// ans = 1 if printed something. 0 otherwise

// special check for jumps, which use codeblock by default

  int v;
  OPS *o;
  SYM *s;
  o = x->opr + ix;
  s = 0;

  if (o->sym)
   {                                     // if symbol was set/found earlier
  //   if (o->astr) pchar('@');            // Test for 'address of'

     if (opctbl[x->opcix].sigix == 17)
        v = codebank(o->addr, x);        //jump
     else
        v = databank(o->addr, x);

     if (o->bit)
        s = get_sym (o->wsize, v, o->val, x->ofst);
     else
        s = get_sym (o->wsize, v, -1, x->ofst);

     if (s)
      {
        if (o->off) pchar('+');       // if offset with sym
        pstr("%s", s->name);
        return;
      }
    }

 if (o->imd)                               // immediate operand
    {
     pstr("%x", o->val & camk[o->ssize]);
     return;
    }

 if (o->rgf)
    {
     if (o->addr || o->wsize) pstr ("R");           // non zero reg, or write op
     pstr ("%x", o->addr);
     if (o->inc) pstr ("++");
     return;
    }

 if (o->bit)
     {
       pstr ("B%d_", o->val);
       return;
     }

 if (o->adr)
    {
     paddr(o->addr);
     return;
    }

if (o->off)
    {            //offset address - signed
     v = o->addr;
      //     pchar(0x20);
     if (v < 0)
     {
      v = -v;
      pchar('-');
     }
     else pchar('+');
  //     pchar(0x20);

     if (!numbanks) v &= camk[2];   /// remove bank
     pstr ("%x", v);               // & camk[2]);
     return;
    }
}

void fix_sym_names(const char **, INST *);


void clr_inst(INST *x)
{
       memset(x,0,sizeof(INST));       // set all zero
       x->opr[1].rgf = 1;
       x->opr[2].rgf = 1;
       x->opr[3].rgf = 1;
}


int find_last_psw(INST *c, int syms)
 {
   // finds last PSW changing opcode for printout, always print phase
    int ans, cnt,chkclc;
    uint ofst;
    OPC *opl;
    PSW *p;

    ans = 0;
    cnt = 0;
    ofst = c->ofst;   //from current instance

    // is a psw setter defined by user ??
    p = get_psw(c->ofst);
    if (p)
       {
          ans = 1;            // found !!
          ofst = p->pswop;
       }

    /// ignore clc,stc as psw setters unless JLEU, JGTU, JC, JNC
    chkclc = 0;
    if (c->opcix > 61 && c->opcix < 64) chkclc = 1;  // JLEU/JGTU
    if (c->opcix > 55 && c->opcix < 58) chkclc = 1;   // JC/JNC

    // This is used by carry and ovf as well

    // what about a popp ??

    while (!ans)
     {
      ofst = find_opcode(0, ofst, &opl);

      if (cnt > 32) break;
      if (!ofst) break;

      if (opl->sigix == 17)
       {
        // CALL - find jump and go to end of subr
        JMP *j;
        SUB *sub;
        j = (JMP*) chmem(&chjpf);
        j->fromaddr = ofst;
        j = (JMP*) find(&chjpf, j);
        if (j && j->jtype == J_SUB)
           { 
            sub = get_subr(j->toaddr);
            ofst = sub->end;
           }
        else break;
       }

   if (opl->sigix == 20)  break;      // ret


// must check for  rets too !!
// can't go back through rets, or stat jumps ??






      if (opl->pswc) 
        {    // FOUND PSW setter, if set/clear carry, check flag.
           if (opl->sigix == 21)
            {
             if (chkclc) ans = 2; 
            }
         else
            { 
             ans = 1;
            } 
        } 
        cnt++;
     }

 if (ans)
       {
        tmpscan.curaddr = ofst;
        tmpscan.start = ofst;
        do_code(&tmpscan,&sinst);
        if (syms)  fix_sym_names(0,&sinst);
       }
   else
      {
       clr_inst(&sinst);
//       memset(&sinst,0,sizeof(INST));       // set all zero
  //     sinst.opr[1].rgf = 1;
   //    sinst.opr[2].rgf = 1;
   //    sinst.opr[3].rgf = 1;
      }

    return ans;
 }


void p_opsc (INST *x, int ix, int ws)
{
 // source code printout - include sign and size markers.
 // ws = use write size
 // ix+0x10 flag specifies extra sizes

  OPS *o;
  int i;
   if (!x)
    {
       pchar('0');
       return;
    }

   if (ix >= 0x11 && ix <= 0x13)
      {                 // extra size/sign requested
       i = ix & 3;
       if (i == 1 && x->opcsub > 1)  i = 0;      //(x->inx || x->inr)) i = 0;     // not index or indirect
       if (x->opr[i].sym) i = 0;
       if (!x->opr[i].rgf) i = 0;

       if (i)
         {
           if (x->opr[i].wsize && ws) i = x->opr[i].wsize; else i = x->opr[i].ssize;
           // after an equals, always use read size (ws = 0)
           if (i & 4)  pstr("s");
           i &= 3;
           if (i == 1)  pstr("y");
           if (i == 2)  pstr("w");
           if (i == 3)  pstr("l");
         }
      }

  ix &= 3;
  o = x->opr + ix;                            // set operand

  if (o->bkt)  pchar ('[');
  p_sc (x, ix);
  if (ix == 1 && x->opcsub == 3) p_sc (x, 0);  // indexed, extra op
  if (o->bkt)  pchar (']');
  return;
}



void do_comment(CMNT *m)
{
  // process any special chars in a comment line

  char *c, *x;
  int z, col, bit;

   if (!m->ctext)  return;                     // safety

   c = m->ctext;

   if (!*c) return;

   if (*c != '\\' )                       // no special at front, so pad to cmt col
     {
      if (cposn[3] - gcol <= 2) pstr("   ");  // always have a space before comment
      else  p_pad(cposn[3]);
     }

   while (*c)
    {
     switch (*c)
       {   // top level char check

         default:
           pchar(*c);                     // print anything else
           break;

         case 1:                          //  for autocomment
         case 2:
         case 3:
           p_opsc (m->minst,*c,0);
           break;

         case '\n' :                      // '\n',  ignore it
         case '\r' :
           break;

         case '\\' :
           c++;                     // skip the backslash, process next char.

           switch (*c)
             {  // special sequences

               default:
                 pchar(*c);                              // print anything else
                 break;

               case 0x31:
               case 0x32:
               case 0x33:
                 // for user embedded operands 1-3
                 z = *c - 0x30;   // char to number
                 if (z >= 1 && z <= 3)
                   {
                    p_opsc(m->minst,z,0);
                    c++;
                   }
                 break;

               case 'n' :                                // newline

                 newl(1);
                 break;

               case 'P' :                                // '@' symname with padding, drop through
               case 'p' :
               case 'S' :                                // '$' symname with no padding
               case 's':

                 x = symfromcmt(c+1, &z, m->ofst, &bit);
                 col = gcol;                             // remember this column

                 if (x)
                   {
                    if (bit >= 0) pstr("B%d_",bit); 
                    pstr("%s",x);
                    if (*c == 'P' || *c == 'p') p_pad(col+21);           // pad 21 chars from col (or not)
                   }
                 c = c+z;                                   // skip the sequence
                 break;

               case '\n' :                               // '\n',  ignore it
               case '\r' :
                 break;

              case 'w' :                                // wrap comment, or pad to comment column
                 if (lastpad >= cposn[3]) wrap();
                 else p_pad(cposn[3]);
                 break;

             }   // end switch for '\'
       }        // end switch for char
     c++;
    }

  m->ctext = (char *) empty;        // clear printed string
}

void pp_comment (uint ofst, uint ret)
{

  // ofst is the START of an opcode statement so comments
  // less than and equal should be printed.

  if (anlpass < ANLPRT) return;        // safety check

//if (ofst > 0x824b6)
//{
//DBGPRT(1,0);
//}



 // if 'return' set, then add a newline.
 // newline should be AFTER comments not beginning with newline
 // but BEFORE ones that do.

// MORE if ret set, and first cmnt is not newline, then break at first newline ???
// this allows insertion of newline as splitter ??

  if (!fcmnt.nl)
    {
     while (ofst > fcmnt.ofst)
       {  //not newline 
        do_comment(&fcmnt);
        if (!get_cmnt(&fcmnt)) break;
        if (fcmnt.nl) break;
       }
    }

     if (ret)  { newl(1); ret = 0;}
     while (ofst > fcmnt.ofst)
       { // newline 
        do_comment(&fcmnt);
        if (!get_cmnt(&fcmnt)) break;
       }
    

// no comments, newline anyway.
// if (ret)  newl(1);

}

/*  while (ofst > fcmnt.ofst)
   {
     

    if (ret && fcmnt.nl)  { newl(1); ret = 0;}
    do_comment(&fcmnt);
    if (ret && !fcmnt.nl)  { newl(1); ret = 0;} 
    if (!get_cmnt(&fcmnt)) break;
   }

// no comments, newline anyway.
 if (ret)  newl(1);

}  */


int maxsize(LBK *x)
{
 // return max fieldwidth of a segment between newlines
 
 ADT *a;
 SYM *sym;
 int i,item,val,sz;
 uint ofst, end;

 ofst = x->start;
 memset(&plist,0,sizeof(plist)); // zero the whole thing
 
 if (x->argl) return 0;
 end = x->end - x->term ;  // looks OK
 
 while (ofst < end)
 {   
 
 sz = 0;
 item = 1;

// do this for every row (if struct), but only once for tabs and funcs
 
 a = (ADT*) chmem(&chadnl);
 a->fid = x;
 a->seq = -1;

 while ((a = get_next_adnl(&chadnl,a)))
   {
    sz += cellsize(a);               // which is a->cnt * casz[a->ssize];
    
    // now try to work out widths
 
    for (i = 0; i < a->cnt; i++)             // count within each level
      {
       sym = 0;   
       if (a->fnam) 
          {
           val = decode_addr(a, ofst);           // val with decode if nec.  and sign - READS ofst
           sym = get_sym(0, val,-1,ofst);        // READ sym AFTER any decodes
          } 
       if (sym)
         {
           val = sym->tsize -1;     
           if (val > plist[item]) plist[item] = val;
         }  
       else
         {
           // check pfw in HERE !!
           if (a->pfw) val = a->pfw;
           else val = cafw[a->ssize];       //sz[a->ssize]; 
           if (a->fdat) val+= 4;    // floating point
           if (val > plist[item]) plist[item] = val; 
         } 
       item++; 
       ofst += casz[a->ssize]; 
      }
 
// this to line up ALL columns even with split lines  
    if (a->newl)
     {
      if (sz > plist[0])  plist[0] = sz;     // for header
      sz = 0;                                // for next segment
      item = 1;                              // restart items
     }
  
   }
  
 if (sz > plist[0])  plist[0] = sz;     // for last (or only) segment
 
 if (x->fcom != C_STCT) break;        //only need one pass unless struct
 
 }
 
  //now add up for pad position.
  
 plist[0] = (plist[0]*3) + 7;      //first pad posn, allow for addr
 if (numbanks) plist[0]++;         // for exta address digit

 if (plist[0] < cposn[0]) plist[0] = cposn[0];   //minimum at mnemonic 

 return item;
}


int pp_cmpl(uint ofst, void *fid, int fcom)
{
  // print 'compact' layout, all args on one line
  // unless | char, when it's split  
    
  uint tsize, size, end;
  int sq, itn;
  
  tsize = totsize(fid); 
  end = ofst + tsize;

// this is stct format, need a different one for subr (with args afterwards) 
 sq = -1;                 // new query
 itn = 1; 
 
 while (ofst < end)
  {
   size = listsize(fid,sq);
   
  if (fcom != C_ARGS)
   {
    pp_hdr (ofst,0,size);        // no title
    p_pad(plist[0]);
    pstr ("%s", cmds[fcom]);
    if (gcol < cposn[1]) p_pad(cposn[1]); else p_pad(gcol+2);
    pp_lev(fid,ofst,&sq, &itn);    // continue from newline
    ofst += size;
    if (ofst < end) pp_comment (ofst,0);    //NOT last item
   }
  else
   { // arguments for subr on same line
    pp_lev(fid,ofst,&sq, 0);
    ofst += size;
   }
  }

 
  
 return tsize;
}

/* this is original one
int pp_subargs (INST *c, int ofst)
{
    // ofst is effectively nextaddr
  
  SUB  *xsub;
  LBK  *ablk;
  int size, xofst;

  size = 0;

  if (PSSRC)   pchar('(');   // open args bracket

  xofst = ofst;
  
  while(1)           //look for args first.........in a loop
   {  
    ablk = find_data_cmd(xofst, C_ARGS);
     if (!ablk) break;          // none found
    
    if (PSSRC && xofst > ofst)   pchar(','); 
    if (PSSRC && ablk->size) pp_lev(xofst,ablk->adnl,0,ablk->size);     // do params at min width, no trail comma
    xofst += ablk->size;                     // look for next args
    size += ablk->size;
    }

  if (!size)
    {
     // didn't find any ARGS commands - look for subr arguments  

     xsub = get_subr(c->opr[1].addr);
  
     if (xsub)
      {
       size = sizew(xsub->adnl);
       if (PSSRC && size) pp_lev(ofst,xsub->adnl,0,size);     // do params at min width, no trail comma
      }  
    }
 
  if (PSSRC) pstr(");");       // close args bracket

  if (size)
    {
     pp_comment (ofst);                         // do any comment before args print
     pp_hdr (ofst, "#args ", size,0);           // do args as sep line
    }
       
  return size;
}

*/

















int pp_argl (uint ofst, void *fid, int fcom, int *argno)
{

// Print 'argument' layout, 1 arg per line
// can get SUB or LBK as fid

int i;
uint tend;
ADT *a;

 pp_comment (ofst,0);            //for any comments BEFORE args

 tend = ofst + totsize(fid)-1;  // where this 'row' ends
 i = 0;
 a = (ADT*) chmem(&chadnl);
 a->fid = fid;
 a->seq = -1;

 while ((a = get_next_adnl(&chadnl,a)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     if (fcom == C_ARGS) sprintf(nm, "      #%s %d", "arg", (*argno)++); 
     else                sprintf(nm, "      #%s %d", cmds[fcom], (*argno)++);
     
     pp_hdr(ofst, nm, casz[a->ssize]);
     p_pad(cposn[2]+3);
     a->pfw = 0;                         // no print field in argl
     pp_adt(ofst, a,0);

     ofst += casz[a->ssize];
     if (!casz[a->ssize]) ofst++;     // safety

     if (ofst < tend)
       {
         pchar(',');
         pp_comment (ofst,0);        
       }
     else
       {
        if (fcom != C_ARGS) pp_comment (ofst,0); // not for subrs,args
       }
    }

   if (a->newl) break;       //this breaks seq no ??
  }

  // return size....?
  return ofst;
}



int pp_subargs (INST *c, int ofst)
{
  // ofst is effectively nextaddr

  // change pp-lev for pp_args to do
 // a line by line print of attached arguments

  SUB  *xsub;
  LBK  *k;
  int size, xofst, argno;

  size = 0;
  argno = 1;

  if (PSSRC)   pstr(" (");   // args open bracket

  xofst = ofst;
  while(1)           //look for args first.........in a loop
   {
    k = find_data_cmd(xofst, C_ARGS);

    if (!k) break;
    if (PSSRC && xofst > ofst)   pchar(',');
    if (PSSRC && k->size)
      {  
        if (k->cptl) pp_cmpl(xofst,k, k->fcom);
        else  pp_argl(xofst, k, k->fcom, &argno);
      }
    xofst += k->size;                     // look for next args
    size += k->size;
    }

  if (!size)
    {
     // didn't find any ARGS commands - look for subr arguments

     xsub = get_subr(c->opr[1].addr);

     if (xsub)
      {
       size = xsub->size;
       if (PSSRC && size)
         {  
          if (xsub->cptl) pp_cmpl(xofst,xsub, C_ARGS);
          else pp_argl(xofst, xsub, C_ARGS, &argno);
         }
      }
    }

  if (PSSRC && size) pchar(' '); // add space if args
  if (PSSRC) pstr(");");       // args close bracket

  if (size)
    {
     pp_comment (ofst,0);                               // do any comment before next args print
     if (xsub->cptl) pp_hdr (ofst, "#args ", size);   // do args as sep line if compact
    }

//  if (!size) size++;       // stop infinite loops !! size can be zero
  return size;
}
// print subroutine params - simpler version
//func, table, struct go here

uint pp_stct (uint ofst, LBK *x)
{
  uint size, end;
  int argno;

  end =  ofst + x->size;
  argno = 1;                               // can't reset argno !!
 
  if (end == ofst) end++;     // safety
 
  if (ofst == x->start) maxsize(x);  // do only once.

  size = x->end - x->term + 1;    // where terminator starts
  
  if (x->term && end > size)
    {      // print terminator
     pp_hdr (ofst, 0, x->term);
     p_pad(cposn[1]);
     pstr("## terminator");
     ofst += x->term;
     return ofst;
    }

  while (ofst < end)              // TEST !!
    { 
      if (x->argl)
        {
         ofst = pp_argl(ofst,x,x->fcom,&argno);
         newl(1);
        }
      else  ofst += pp_cmpl(ofst,x,x->fcom);
    }
  
 return ofst;
}

void shuffle_ops(const char **tx, INST *c, int ix)
{
  // shuffle ops down by one [1]=[2],[2]=[3] acc. ix
  // change pseudo code to ldx string "[2] = [1]"

  while (ix < 3) {c->opr[ix] = c->opr[ix+1]; ix++;}
  if (tx) *tx = opctbl[LDWIX].sce;
}

void fix_sym_names(const char **tx, INST *c)
{
  // adjust operands for correct SOURCE code printout, rbases, indexes etc.
  // This CHANGES addresses for indexed and RBS for printout purposes

  OPS *b, *o;
  RBT *r;
  SYM *s;
  int i;
  int addr;
  const OPC* opl;

  opl = opctbl + c->opcix;

  // get and flag all sym names FIRST, then can drop them out later as reqd

  for (i = 1; i <= opl->nops; i++)
   {
    o = c->opr+i;
    o->sym = 0;            // no sym
    s = NULL;

    if (opctbl[c->opcix].sigix == 17)
       addr = codebank(o->addr,c);           // calls need CODE address
    else
       addr = databank(o->addr,c);

    s = get_sym(o->wsize, addr,-1, c->ofst);      // write sym or read sym
    if (s)
      {
       o->sym = 1;  // symbol found
       if (s->immok) o->symi = 1;            // immediates OK
      } 
   }

  if (c->opr[3].bit)            // bit jump - if bitname, drop register name
     {
        o = c->opr+2;
        b = c->opr+3;               // b = bit number entry, get bit/flag name
        b->sym = 0;                 // clear bit name flag
        s = get_sym (0, databank(o->addr,c), b->val, c->ofst);  // read sym
        if (s)
          {
           o->rgf = 0;
           o->sym = 0;
           b->sym = 1;
           b->addr = o->addr;
          }
     }

  //  Shifts, drop sym if immediate
  if (c->opcix < 10 && c->opr[1].val < 16)  c->opr[1].sym = 0;

  // indexed - check if RBS, or zero base (equiv of = [imd])

  if (c->opcsub == 3)           //indexed, use addr in cell [0] first
     {
     b = c->opr;                // probably redundant, but safety - cell [0]
     o = c->opr+1;

     if (o->addr < 2) o->val = 0;       // safety

     if (!o->addr || o->rbs)
      {
        // Rbase+off  or R0+off
        // merge the two operands to fixed addr  and drop register operand
        // can't rely on o->val always being correct (pop screws it up...) so reset here

        if (o->rbs)
          {           // rbase - get true value from rb chain
           r = get_rbt(o->addr, c->ofst);
           o->val = r->val;                     // but already done ?
          }

        addr = c->opr->addr + o->val;          // addr may be NEGATIVE at this point !!
        if (addr < 0) addr = -addr;                // effective wrap around ? never true.......

//        o->addr = o->val + c->opr->addr;                 // add [0] to get true address
        o->addr = databank(addr, c);                  // need bank for index addition

        o->rgf = 0;
        o->sym = 0;
        o->adr = 1;                                      // [o] is now an address
        b->off = 0;                                      // no following offset reqd

        s = get_sym(o->wsize, o->addr,-1,c->ofst);       // and sym for true inx address

        if (s)     //symbol found at (true) addr
         {
          o->bkt = 0;       // drop brackets (from [1+0])
          o->sym = 1;       // mark symbol found
         }

      } // end merge (rbs or zero)
     else
      {
        // this is an [Rx + offset]. Sort op[0] symbol if not a small offset
        if (b->addr > minwreg && !b->neg)
          {
            // not negative, not reserved register, add databank if > PCORG
                
            s = get_sym(0, databank(b->addr,c),-1,c->ofst);
            if (s) 
              {
               if (nobank(b->addr) > PCORG || s->immok)  b->sym = 1;
              }
            if (numbanks) b->addr = databank(b->addr,c);         // do bank for printout
          }
      }
    }         // end inx

  if (c->opcsub == 1)       // immediate - check for removing symbol names
     {
      if (c->opr[1].val < 0xff)  c->opr[1].sym = 0;      // not (normally) names for registers
      
      if (opl->sigix == 10) c->opr[1].sym = 0;           // compare - must be an abs value
      else if (opl->sigix == 8)  c->opr[1].sym = 0;           // mult    - must be an abs value
      else if (opl->sigix == 11) c->opr[1].sym = 0;           // divide  - must be an abs value
      else if (opl->sigix == 12 && c->opr[2].rbs) c->opr[1].sym = 0; // ldx rbase - no sym

     }

  // 3 operands.  look for rbase or zero in op[1] or op [2] and convert or drop
  if (opl->nops > 2)
    {    //rbase and R0
     if (c->opr[1].rbs && c->opr[2].addr == 0)  shuffle_ops(tx,c,2);

     if (c->opr[2].rbs && c->opcsub == 1 && c->opcix > 23 && c->opcix < 29)
      {   // RBASE + offset by addition or subtraction, only if imd and ad3w or sb3w
       b = c->opr+1;
       addr =  b->val + c->opr[2].val; // addr may be NEGATIVE at this point !!
       if (addr < 0) addr = -addr;

       b->addr = addr;         //b->val + c->opr[2].val;              // convert to true address....
       // databank ?
       b->imd = 0;
       b->adr = 1;
       s = get_sym(0, b->addr,-1,c->ofst);

       if (s) b->sym = 1;
       shuffle_ops(tx,c,2);                  // drop op [2]
      }

    if (c->opr[2].addr == 0)   shuffle_ops(tx,c,2);      // x+0, drop R0
    if (c->opr[1].addr == 0)   shuffle_ops(tx,c,1);      // 0+x, drop R0

    if (!c->opcsub && (c->opr[1].rbs || c->opr[2].rbs))
      {  //     R3 = R2 + R1 style.  is anything rbase ?? replace with address as an imd
        for (i = 1; i < 3; i++)
          {  //          ops 1 and 2
            b = c->opr + i;
            if (b->rbs)
              {
               b->imd = 1;
               b->rgf = 0;
               b->sym = 0;
              }
          }  
       }
   }

    // CARRY opcodes - can get 0+x in 2 op adds as well for carry
    // "\x2 += \x1 + CY;"
  if (c->opr[1].addr == 0 && c->opcix > 41 && c->opcix < 46)
    {
     if (tx) *tx = scespec[opl->sigix-6];     // drop op [1] via special array
    }
}


int chk_mask(inst *c, int v)
{
  // v is 1 for or, and 0 for and


  int i, bcnt, ccnt, bit, mask, size;
  OPS * o;

  // if already flagged as bit word/byte
  // no, need to FIND symbols, for address range

  o = c->opr+1;
  mask = o->val;
  size = casz[o->ssize] * 8;      // number of bits;

  // check for contiguous bits....BEFORE mask is swopped

  ccnt = 0;                         // change count
  bcnt = 0;                         // matching bit count
  bit = mask & 1;                   // lowest bit state

  for (i = 0; i < size; i++)
     {
      if ((mask & 1) == v) bcnt++;    // count bit matches
      if ((mask & 1) != bit)
        {
         bit ^= 1;               // swop last bit state
         ccnt++;                 // and count change
        }
      mask >>= 1;
     }

  if (bcnt < 4 || ccnt > 2)
   {
     return 0;  // ok as bit mask
   }
  return 1;
 }

void prt_bitwise(INST *c, SYM *w, SYM *b, int bit)
{
  SYM *s;
  uint addr;

  // does a bit name exist ??
  addr = c->opr[2].addr;  //stored locally so it can be changed

  s = get_sym(1, addr, bit, c->ofst);

  if (s)
    {
    pstr ("%s ",s->name);      //  print matched sym bit name
    return;
    }

  if (bit > 7)
    {
     if (w && !b)
      {      // word name exists but not byte name
        pstr ("B%d_", bit);     //  unchanged bit number
        pstr ("%s ",w->name);   //  print word name
        return;
      }
     else
      {  // otherwise, either a byte name or no name
        bit  -= 8;
        addr += 1;
      }
    }

  pstr ("B%d_", bit);            // no name match
  if (b) pstr ("%s ",b->name);   //  print matched byte name
  else   pstr ("R%x ", addr);
}


void bitwise_replace(const char **tx, INST *c)

// check for bitwise operations - not for 3 operand instructions
// must be immediate op (as flags mask) and at least one name found
// WRITE symbols for first try.
{

  int i, ix, k, v, size;
  uint  mask, addr;
  SYM *w, *b;

  ix = c->opcix;

  if (anlpass < ANLPRT)    return;     // only for  print phase
  if (opctbl[ix].nops > 2) return;     // not 3 op instructions
  if (!c->opr[1].imd)      return;     // immediate mask only

  // check must be AND,OR, XOR
  // works for ldx only if or, and set the flag.
  // WHAT ABOUT CMP  ??????

  // find out if reg is marked as a flags word/byte.

  addr = c->opr[2].addr;

  if (addr&1) w = get_sym(1,addr-1,-1, c->ofst); // try word name
  else w = 0;

  b = get_sym(1,addr,-1, c->ofst);       // try byte name

  if      (opctbl[ix].sigix == 5)  v = 0;     // AND
  else if (opctbl[ix].sigix == 9)  v = 1;     // or, XOR

  else if (opctbl[ix].sigix == 12 || opctbl[ix].sigix == 10)
      {    // ldx or cmpx
       v = 0;
       if (w && w->flags) v = 2;
       if (b && b->flags) v = 2;
       if (v != 2) return;
      }
  else return;

  // MORE HERE !!

  if (chk_mask(c, v)) return;

  mask = c->opr[1].val;
  if (!mask) return;

   // sigix is 5 for AND and 9 for OR, XOR
  // and 12 for ldx

  if (ix >= 10 && ix <= 13)
    {                      // AND opcodes v = 0
     mask = ~mask;              // reverse mask for AND
    }

  // check here if mask is qualifier for separate flags.

  mask &= camk[c->opr[1].ssize];
  size = casz[c->opr[1].ssize] * 8;       // number of bits

  k = 0;                                     // first output flag

  for (i = 0; i < size; i++)
   {
    if (mask & 1 || v > 1) // would not be mask & 1 for a v=2;
      {
       if (k++)  {pchar(';'); p_indl (); }   // new line and indent if not first
       prt_bitwise(c,w,b,i);
       if (ix >= 32 && ix <= 33) pstr("^");
       if (v > 1) pstr ("= %d", (mask & 1)); else
       pstr ("= %d" ,v);
      }
    mask  = mask >> 1;
   }
  *tx = empty;                  // at least one name substituted return a ptr to null
 }


void shift_comment(INST *c)
 {
  /*  do shift replace here - must be arith shift and not register */
  int ps;

  if (anlpass < ANLPRT) return ;              // redundant ?

  if (c->opcix < 10 && c->opr[1].imd)
   {
    if (!c->opr[2].addr)
      {  // R0, timewaster
       ps = sprintf(nm, "#[ Time delay ?] ");
      }
    else
     {
      ps = sprintf(nm, "#[ \x2 ");
      if (c->opcix < 4)
       ps += sprintf(nm+ps,"*");
      else
       ps += sprintf(nm+ps,"/");
      ps += sprintf(nm+ps, "= %d] ", 1 << c->opr[1].val);
     }
    acmnt.ofst = c->ofst;
    acmnt.ctext = nm;
    acmnt.minst = c;
  }
}



void do_carry(INST *c, const char **tx, int ainx)
{

// Replace "if (CY)" with various sources
// possible opcodes add,shift R, shift L, cmp, inc, dec, neg, sub

   int ans;
   const OPC *opl;

   pstr ("if (");


   // get last psw changer
   ans = find_last_psw(c,1);  // returns 1 if found...op in sinst
   if (!ans) return;          // not found, revert to "if (CY"   style

   opl = opctbl + sinst.opcix;              // mapped to found inst

   if (opl->sigix == 7)
        {
          // subtract. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s ",*tx);
          p_opsc(0,1,0);
          pstr(") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

  if (opl->sigix == 10)
        {
          // compare. Same as subtratc, but no wop
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];

          if (sinst.opr[2].addr == 0)
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(&sinst,1,0);                   // operand 1 of compare
              pstr(" %s ", swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(&sinst,2,0);                   // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(&sinst,2,0);
              pstr(" %s ",*tx);
              p_opsc(&sinst, 1,0);
            }
          pstr(") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 6  || opl->sigix == 25)
        {       //  add or inc STANDARD CARRY -  have wop ?  
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<="); 
          pstr(" %x) ", camk[sinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


   if (opl->sigix == 3 || opl->sigix == 4)
        {   //  shl (3)   or shr (4) 

          // check no of shifts is imd, otherwise return 0=0 style.
           if (!sinst.opr[1].imd)
            {
             clr_inst(&sinst);       // can't do variables
             return;
            }

           if (opl->sigix == 3)
            {   //shl
             ans = casz[sinst.opr[opl->wop].wsize] * 8;          // number of bits
             ans -= sinst.opr[1].val;                            //shift from MSig

             if (ans > 16)
               { // dl shifts ??
                 ans -= 16;
                 sinst.opr[opl->wop].addr+=2;
               }
            }
          else 
            {  //shl
              ans = sinst.opr[1].val-1;
            }

          pstr("B%d_", ans);

          p_opsc(&sinst,opl->wop,0);
          pstr(" = %d) ", ainx & 1); 
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

 /*  if (opl->sigix == 11 || opl->sigix == 8)
        {       //  mult,div STANDARD OVERFLOW all have wop ?
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<=" );
          pstr(" %x) ", camk[sinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }
*/

     if (opl->sigix == 24)
      {       // dec
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s 0) ", (ainx & 1) ?  ">=" : "<" );
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
      }

}


void do_ovf(INST *c, const char **tx, int ainx)
{

// Replace if (OVF) with various sources
// possible opcodes add, shift L, cmp, inc, dec, neg, sub, div.

   int ans;
   const OPC *opl;

   pstr ("if (");
   // get last psw changer


   ans = find_last_psw(c,1);  // returns 1 if found...op in sinst
   if (!ans) return;          // not found

   opl = opctbl + sinst.opcix;              // mapped to found inst


   if (opl->sigix == 7)
        {
          // subtract. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s ",*tx);
          p_opsc(0,1,0);
          pstr(") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


  if (opl->sigix == 10)
        {
          // compare. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];

          if (sinst.opr[2].addr == 0)
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(&sinst,1,0);                        // operand 1 of compare
              pstr(" %s ", swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(&sinst,2,0);                        // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(&sinst,2,0);
              pstr(" %s ",*tx);
              p_opsc(&sinst, 1,0);
            }
          pstr(") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 6  || opl->sigix == 25)
        {       //  add or inc STANDARD CARRY -  have wop ?  
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<="); 
          pstr(" %x) ", camk[sinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


   if (opl->sigix == 3)
        {       //  shl STANDARD CARRY all have wop ? 

// need to find out how many shifts      clr_inst(&sinst); for early exit.
 
// sinst.opr[1].val is shift amount.  wop is shifted value cant do it for other than imm (opr[1].imd)

          if (!sinst.opr[1].imd)
            {
             clr_inst(&sinst);       // can't do variables
             return;
            }

          //shl
          
          ans = casz[sinst.opr[opl->wop].wsize] * 8;          // number of bits
          ans -= sinst.opr[1].val;                            //shift from MSig

// dl shifts ??

if (ans > 16)
  {
  ans -= 16;
  sinst.opr[opl->wop].addr+=2;
}

//shl     ans = sinst.opr[1].val-1;

//fiddle opr as a bit ??
          pstr("B%d_", ans);

          p_opsc(&sinst,opl->wop,0);
          pstr(" = %d) ", ainx & 1);           // ? "= 1)" : "= 0)"); 
     //     pstr(" %x) ", camk[sinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

/*
   if (opl->sigix == 4)
        {       //  shr STANDARD CARRY all have wop ? 

          if (!sinst.opr[1].imd)
            {
             clr_inst(&sinst);       // can't do variables
             return;
            }

          //shl
          
 // dl shifts ??


//shl 
    ans = sinst.opr[1].val-1;








// need to find out how many shifst
          pstr("B%d_", ans);
          p_opsc(&sinst,opl->wop,0);

          pstr(" = %d) ", ainx & 1);           // ? "= 1)" : "= 0)"); 
      //    pstr(" %s", (ainx & 1) ? ">" : "<="); 
      //    pstr(" %x) ", camk[sinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }
*/







   if (opl->sigix == 11 || opl->sigix == 8)
        {       //  mult,div STANDARD OVERFLOW all have wop ?
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<=" );
          pstr(" %x) ", camk[sinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


     if (opl->sigix == 24)
      {       // dec
          p_opsc(&sinst,opl->wop,0);
          pstr(" %s 0) ", (ainx & 1) ?  ">=" : "<" );
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
      }

}

void do_cond_ops(INST *c, const char **tx, int ainx)
{
   // Conditional ops replacements.
   // last is ZERO and lwop < 0 if no candidate found.
   // lwop is zero ONLY for R0 as wop

   // note that tx is ALREADY SWOPPED for "if{}" or "goto"

   OPC *opl;
   const char *t, *s, *x;
   int ans;

   t = *tx+1;    // skip '\7', t points to operator string

   ans = find_last_psw(c,1);  // returns 1 if found...op in sinst

   pstr ("if (");

   // if (!ans) return;          // not found
   opl = opctbl + sinst.opcix;  // mapped to found instance




   if (opl->wop)
     {            // not a compare
      if (!sinst.opr[opl->wop].addr)
        {   //wop is R0, style"(op) = 0"
         s = opctbl[sinst.opcix].sce;               // source code of psw op
         x = strchr(s, '=');
         if (x)
           {  // do opcode after = in brackets
            x += 3;                                // should now be op
            pstr("(");
            p_opsc(&sinst,2,0);                        // operand 2 of compare
            while (*x > 0x1f) pchar(*x++);             // operator
            p_opsc(&sinst,1,0);                        // operand 1 of compare
            pstr(")");
            pstr(" %s ",t);                            // 
            p_opsc(0,1,0);
           }
        }
      else
       {   // no compare real wop , style [wop] == 0
        p_opsc(&sinst,opl->wop,0);                     //lwop,0);
        pstr(" %s ",t);
        p_opsc(0,1,0);
       }
     }

 
   if (!opl->wop)
     {      // compare found, style R2 = R1
      if (sinst.opr[2].addr == 0 )
       {
         if (sinst.opr[1].addr == 0 && ans)
          {       //both ops zero ??
            pstr(" %s ", zerocmpop[ainx - 62]);       //c->opcix - 62]);
          }
         else
          {
           // cmp has zero first, so swop operands over via special array.
           // NB. NOT same as 'goto' to {} logical swop
           // so R1 <reversed> 0
           p_opsc(&sinst,1,0);                        // operand 1 of compare
           pstr(" %s ", swopcmpop[ainx - 62]);        //c->opcix - 62]);
           p_opsc(&sinst,2,0);                        // operand 2 of compare (zero)
          }
       }
     else
       {  
         p_opsc(&sinst,2,0);                        // operand 2 of compare
         pstr(" %s ",t);
         p_opsc(&sinst,1,0);                        // operand 1 of compare
       }
     }

    pstr(") ");
    *tx = opctbl[JMPIX].sce - 1;             //set \9 for 'goto' processing
}


void pp_answer(INST *c)
{
  // print answer, if there is one
  uint addr;
  SUB *s;
  ADT *a;
  SYM *sym;
  OPS *o;

  addr = c->opr[1].addr;
  s = get_subr(addr);

  a = get_adnl(&chans,s,0);

  if (a)
   { // found an answer set up phantom opr[3] (rgf set)
     o = c->opr+3;
     sym = 0;

     o->ssize = a->ssize;
     o->addr = a->data;

      if (o->addr > 0xff)
       {
         o->rgf = 0;
         o->adr = 1;
         o->bkt = 1;
       }

     if (a->fnam) sym = get_sym(0, o->addr,-1,c->ofst);           // syname AFTER any decodes
     if (sym) {o->sym = 1; o->bkt = 0;}            // pstr("%s, ",sym->name);

     p_opsc(c,0x13,0);

  pstr(" = ");
  }
}




void do_regstats(INST *c)
 {
   int i;
   OPS *o;
   RST *r;

// only if emulating - register size if argument

   for (i = 1; i <= c->numops; i++)
    {
     o = c->opr+i;
     r = find_rgstat(o->addr);
     if (r && r->arg)
     {
       set_rgsize(chrgst.lastix, o->ssize);
           #ifdef XDBGX
       DBGPRT(1,"argsize r%x=%d at %x", r->reg, r->ssize, c->ofst);
       #endif

     }
    }

  if (c->opcsub == 2)
  { // indirect use opr[4] for reg details
     o = c->opr+4;
     r = find_rgstat(o->addr);
  if (r && r->arg)
  {
    // arg used as address, so will always be WORD
    set_rgsize(chrgst.lastix,2);
  }
  }

}


 void do_dflt_ops(SBK *s, INST *c)
 {  // default - any operands as registers
    int i;
    for (i = 1; i <= c->numops; i++) op_calc (g_byte(s->nextaddr + i), i,c);
    s->nextaddr += c->numops + 1;
    do_regstats(c);
 }


void add_opr_data(SBK *s, INST *c)
{
  OPS *o;
  LBK *k;
  int addr, sig;             //x

  if (anlpass >= ANLFFS) return;
  if (s->nodata) return;

  addr = 0;
  sig = opctbl[c->opcix].sigix;

  if (sig == 14)  return;    // not push



 /* opcsub == 1 for immediates with valid ROM / RAM address ??
 // how to tell what's real though ?


  if (c->opcsub == 1)
  {     //ldx or add with immediate only. ALSO check it's not a cmp limit check ?
    if (sig == 12 || sig == 6)
      {
       o = c->opr+1;
       if (!o->wsize) addr = o->addr;

       if (nobank(addr) > PCORG)
         {
          x = find_last_psw(c->ofst,0);
          if (x && sinst.opcix > 33 && sinst.opcix < 36)
           {  // found  a cmp
              if (sinst.opr[2].addr == c->opr[2].addr
               && sinst.opr[2].val == c->opr[2].val)
             {  // cmp matches the ldx, i.e. a LIMIT CHECK
              addr = 0;
             }
           }
         }
       }
    }           */


// indirect but NOT for writes
  if (c->opcsub == 2)
   {
     o = c->opr+1;
     if (!o->wsize) addr = o->addr;

  //   if (c->inc)
 //    { //struct ?
 //    DBGPRT(1,"INC++ STRUCT?");
 //    }
   }

 // indexed

  if (c->opcsub == 3)
  {
   o = c->opr+4;                         // the saved opr[1]

  if (!o->wsize)
  {
  // if [0] is a valid ROM address, then use that.
  // if R[1] is a valid ROM address with small offset, then use R[1]
  // if R[1] is an rbase, use combined address in [1]

   if (c->opr[1].rbs) addr = c->opr[1].addr;    // combined address
   else
    {
     addr = c->opr->addr;                 // address of offset [0]
     if (addr >= (PCORG+0x8))
       {
        addr = databank(addr,c);       // use fixed offset
       }
     else
      {
       addr = o->addr; // fixed offset is too small, use register ?
       if (addr >= (PCORG+0x8)) addr = databank(addr,c); else addr = 0;
      }
    }
   }
  }


  if (addr)
     {
      k = add_data_cmd (addr,  addr+casz[o->ssize]-1, o->ssize & 3, c->ofst);       // add data entry
      if (k)
       {    // add offset and type for later checks
        k->opcsub = c->opcsub;
        if (c->opcsub == 3 && c->opr[4].addr)
        if (c->opr[4].val > 0 && c->opr[4].val < 17)
        k->soff = c->opr[4].val;                // offset 0-16
       }
     }
 }

void calc_mult_ops(SBK *s, INST *c)
{
  /******************* multiple subtype instructions *****************
    * opcsub values   0 direct  1 immediate  2 indirect  3 indexed
    * order of ops    3 ops       2 ops      1 op
    *                 dest reg    dest reg   sce/dest reg
    *                 sce  reg    sce  reg
    *                 sce2 reg
    * NB indexed mode (3) is a SIGNED offset
    *******************************************
    * Adjust xofst for true size of opcode
    * +2   opcsub 3, if 'long index' flag set
    * +1   opcsub 3, if not set (='short index')
    * +1   opcsub 1  Word op (immediate)
    ******************************************************/

     int firstb,i, xofst,addr;
     OPS *o;

     xofst = s->nextaddr;

     c->opcsub = g_byte(xofst) & 3;

     firstb = g_byte(xofst+1);             // first data byte after (true) opcode
     o = c->opr+1;                         // most action is for opr[1]


     switch (c->opcsub)
      {
       case 1:                                          // first op is immediate value, byte or word
         op_imd(c);                                     // op[1] is immediate
         if (casz[o->ssize] == 2)
          {
           xofst++;
           op_calc (g_word(xofst), 1,c);                // recalc op1 as word
          }
         else
          {
           op_calc (firstb, 1,c);                       // calc for op[1]
          }

// but here MGHT be required for a table base address ?? But how to tell it's a real start ??
// as in -
     //     add_opr_data(s,c);

       break;

       case 2:     // indirect, optionally with increment - add data cmd if valid
                   // ALWAYS WORD address. Save pre-indirect to [0] for increment by upd_watch

         if (firstb & 1)
          {                                         // auto increment
           firstb &= 0xFE;                          // drop flag marker
           c->inc = 1;
           o->inc = 1;
          }
         op_calc (firstb, 1,c);                     // calc [1] for new value
         o->bkt = 1;
         c->opr[4] = c->opr[1];                     // keep register details

         if (anlpass < ANLPRT)
          {                                         // get actual indirect values for emulation (restore in upd_watch)
           o->addr = g_word(o->addr);               // get address from register, always word
           o->addr = databank(o->addr, c);          // add bank
           o->val = g_val(o->addr,o->ssize);        // get value from address, sized by op
     //      add_opr_data(s,c);
          }
         break;

       case 3:                                          // indexed, op0 is a fixed offset

         if (firstb & 1)
            {
             c->opr->ssize = 2;                        // offset is word (long, unsigned)
             firstb &= 0xFE;                           // and drop flag
            }
          else
            {
             c->opr->ssize = 5;                        // short, byte offset (signed)
            }

         c->opr->off = 1;                              // op [0] is an address offset
         o->bkt = 1;
         op_calc (firstb, 1,c);                        // calc op [1]
         c->opr[4] = c->opr[1];                        // keep register details
         c->opr->addr = g_val(xofst+2,c->opr->ssize);          // get offset or base value CAN BE NEGATIVE OFFSET
         addr = c->opr->addr;
         if (addr < 0 && (c->opr->ssize & 4)) c->opr->neg = 1;    // flag negative if signed
         
         if (anlpass < ANLPRT)
          {                                            // get actual values for emulation
           addr = c->opr->addr + o->val;
           //o->addr may be NEGATIVE at this point !!    probably never..........
           if (addr < 0)
           {
             addr = -addr;                             // wrap around ?
           }
           o->addr = databank(addr, c);                // get address and bank
           o->val = g_val(o->addr,o->ssize);           // get value from address, sized by op
       //    add_opr_data(s, c);
          }
         xofst += casz[c->opr->ssize];                  // size of op zero;
         break;

      default:      // direct, all ops are registers
         op_calc (firstb, 1,c);                // calc for op[1]
         break;
      }

     //calc rest of ops - register 1 done

     for (i = 2; i <= c->numops; i++)   op_calc (g_byte(xofst + i), i,c);
     s->nextaddr = xofst + c->numops + 1;

     if (anlpass < ANLPRT && c == &cinst && c->opcsub)    add_opr_data(s, c);

     if (s->emulating) do_regstats(c);

  }


void pset_psw(INST *c, int t, int mask)
 {
  // preset ones or zeroes

   mask &= 0x3f;                     // safety

   if (t)
    {
      c->psw |= mask;     // set defined bits
    }
  else
    {
      mask = ~mask;       // flip bits
      c->psw &= mask;     // clear defined bits
    }

 }

void set_psw(INST *c, int val, int size, int mask)
 {

    c->psw &= (~mask);     // clear defined bits

  /* set bits according to actual state
    #define PSWZ(x)   (x->psw & 32)        // Z    zero flag
    #define PSWN(x)   (x->psw & 16)        // N    negative
    #define PSWOV(x)  (x->psw & 8)         // OVF  overflow
    #define PSWOT(x)  (x->psw & 4)         // OVT  overflow trap
    #define PSWCY(x)  (x->psw & 2)         // CY    carry
    #define PSWST(x)  (x->psw & 1)         // STK  sticky
*/

    if ((mask & 32)  && !val)      c->psw |= 32;       // zero
    if ((mask & 16)  &&  val < 0)  c->psw |= 16;       // negative

    if (mask & 2)
        {           // carry
         c->psw &= 0x3d;          // clear CY
         if (val > (int) camk[size]) c->psw |= 0x2;      // set CY
        }
    if (mask & 8)
        {           // OVF
         c->psw &= 0x37;          // clear OVF
         if (val > (int) camk[size]) c->psw |= 0x8;      // set OVF
        }

     if (mask & 4)
        {           // OVT - don't clear, just set it.
         if (val > (int) camk[size]) c->psw |= 0x4;      // set OVT
        }
}



int find_vect_size(SBK *s, INST *c, int ix)
{
    // see if a cmp can be found with register.
    // go back to see if we can find a size
    // allow for later ldx to swop registers

// IX is always 4 ??

// could add nrml here too (would be max 32)

      int size, xofst;
      uint reg;

      size = 0;
      reg = c->opr[ix].addr;
      xofst = c->ofst;

      while (xofst)
         {         // look for a cmp or ldx (max 10 instructions back)
          xofst = find_pn_opcodes (0, xofst, 10, 2, 10, 12);

          if (xofst)
            {
             if (opctbl[sinst.opcix].sigix == 12 && sinst.opr[2].addr == reg)
              {  // ldx from another register, change register
                reg = sinst.opr[1].addr;
              }

             if (opctbl[sinst.opcix].sigix == 10 && sinst.opr[2].addr == reg)
              {  // cmp
               if (sinst.opcsub == 1)
                 {                             // found cmp for register
                  size = sinst.opr[1].val;     // immediate compare, use size
                  return size;
                 }
              }
            }
         }

return size;
}


int find_vect_base(INST *c, int ix)
{
    // if an indirect push, try to find the base address.
    // expect an add of an immediate value
    // allow for later ldx to swop registers

      int addr, xofst;
      uint reg;

      addr = 0;
      //ix &= 3;                // safety
      reg = c->opr[ix].addr;
      xofst = c->ofst;

      while (xofst)
         {         // look for an add or ldx (max 10 instructions back)
          xofst = find_pn_opcodes (0, xofst, 10, 2, 6, 12);

          if (xofst)
            {
             if (opctbl[sinst.opcix].sigix == 12 && sinst.opr[2].addr == reg)
              {  // ldx from another register, change register
                reg = sinst.opr[1].addr;
              }

             if (opctbl[sinst.opcix].sigix == 6 && sinst.opr[2].addr == reg)
              {  // add
               if (sinst.opcsub == 1)
                 {                              // found add for register
                  addr = sinst.opr[1].addr;     // immediate add, use as base
                  return addr;
                 }
              }
            }
         }
  // but even if not found, may still have a valid address already in register.
    if (val_code_addr(c->opr[ix].val))  return c->opr[ix].val;

return addr;
}



void pshw (SBK *s, INST *c)
{
  OPS  *o;
  int  size, i;
 // SIG *g;
  FSTK *t;
  RST *r;

  calc_mult_ops(s,c);

  // does not change PSW

  // if scanning, check for vect pushes first (list or single)
  // immediate value is normally a direct call if valid code addr
  // if indexed or indirect do list check
  // it std register, assume it's an argument getter

  if (anlpass >= ANLPRT)  return;

  o = c->opr + 1;

  if (s->scanning)
    {
        // change to a switch on opcsub ?

     if (o->imd && val_code_addr(o->addr))  // c->opcsub = 1
       {   // treat as a subr call, but this could be a return address
           // this uses CODE bank, not data
           add_scan(codebank(o->addr,c), J_SUB,s);
       }

     if (c->opcsub == 3)
         {
           // indexed - try to find size and 'base' of list
           // ops[0] is base (fixed) addr.  ops[4] (copy of ops[1]) is saved register.
           // go backwards to see if we can find a size via a cmp (find_vect_size)
           // only look for list if [0] is a valid address,

           // try [1] too ?? a combined address may be the right one

           i = databank(c->opr->addr,c);               // add bank first
           if (val_code_addr(i))
            {            // vect list from (valid) fixed addr part of index
             size = find_vect_size(s, c, 4);
             check_vect_list(s, i, size, c);
            }
         }
     if (c->opcsub == 2)
         {
             // indirect - try to find size and base of list
             // size is same as indexed above, (via cmp)
             // but base must have been added earlier, so look for an ADD, X
             // where x is immediate
             // [4] is saved register

            i = find_vect_base(c, 4);
            size = find_vect_size(s, c, 4);
            if (val_code_addr(i))
              {
               check_vect_list(s, i, size, c);
              }
           }

     if (o->rgf &&  !c->opcsub)
         {
          // register - check for argument getter

          for (i=0; i < STKSZ; i++)
            {
             t = scanstack+i;
             if (t->type == 1 && t->reg == o->addr)
              { // found
                r = find_rgstat(o->addr);
                if (r) r->popped = 0;
                t->popped = 0;        // mark as pushed.

              }
            }
         }
    }    // end of if scanning

  if (s->emulating)
    {   // similar to push above, but do args if necessary
        // note that as this is done after each scan or rescan,
        // the stack is already built by sj subr....

     if (o->imd && val_code_addr(o->addr))    //opcsub = 1
       {   // immediate value (address) pushed
          emuscan = add_escan(codebank(o->addr,c),s);            // add imd as a special scan
          fakepush(s, o->addr, 2);                      // imd push
       }
     else
       {
        for (i=0; i < STKSZ; i++)
            {
             t = emulstack+i;
             if (t->type == 1 && t->reg == o->addr)
              { // found
                t->popped = 0;        // clear popped.
                t->newadd = o->val | g_bank(t->origadd);
                do_args(s,t);
                r = find_rgstat(o->addr);
                if (r)
                  { r->popped = 0;
                    if (t->newadd == t->origadd)
                     { // no change, delete it - how ?
                      #ifdef XDBGX
                      DBGPRT(1,"delete rgstat %x", o->addr);
                      #endif
                      chdelete(&chrgst,chrgst.lastix,1);
                    }
                  }
                break;
              }
            }
       }
    }


  #ifdef XDBGX
 //   DBG_stack(s->emulating);
    DBGPRT(0,"%x PUSH a=%x v=%x",c->ofst, o->addr, o->val);
    if (s->substart) DBGPRT(0," sub %x", s->substart);
    DBGPRT(1,0);
 #endif

}



void popw(SBK *s, INST *c)
{

   OPS  *o;
   FSTK *t;
   int i;
   RST *r;
   // does not set PSW

   calc_mult_ops(s,c);

   if (anlpass >= ANLPRT)  return;

   o = c->opr+1;      // w operand
   o->val = 0;         // safety

   // can get throwaway POP to R0. Ignore it.
   if (!o->addr) return;

// if scanning, don't actually care that there is no stack entry.
// this only really matters in emulate.

 if (s->emulating) t = emulstack; else t = scanstack;

   #ifdef XDBGX
   DBG_stack(t);
   #endif

     for (i=0; i < STKSZ; i++)
      {           // scan UP callers
       if (!t->type)
         {
           if (s->scanning)
             {
              r = add_rgstat(o->addr, 2);
              r->popped = 1;
             }
           break;
         }

       if (!t->popped)
         {   // not popped yet(or pushed back)
          t->reg = o->addr;
          t->popped = 1;
          o->val = get_stack(s, t, 3);      // not popp
          r = add_rgstat(o->addr, 2);       // find or add (was origadd)
          r->popped = 1;                    // set popped for emu detect
          #ifdef XDBGX
          DBGPRT(1,"set Popped, size = 2, at %x", c->ofst);
          #endif
          break;
         }
        t++;
      }

 #ifdef XDBGX
   DBGPRT(0,"%x POP a=%x v=%x",c->ofst, o->addr, o->val);
   DBGPRT(1,0);
 #endif

}




void clr (SBK *s, INST *c)
  {
    // clear
    do_dflt_ops(s,c);

  c->opr[c->wop].val = 0;

   if (s->emulating)
      {
       pset_psw(c, 0, 0x1a);      // clear bits N,V,C
       pset_psw(c, 1, 0x20);      // set Z
      }
  }

void neg  (SBK *s, INST *c)
  {
      // negate

    do_dflt_ops(s,c);
    c->opr[1].val = -c->opr[1].val;
    if (s->emulating) set_psw(c, c->opr[1].val, c->opr[1].wsize,0x2e);
 }


void cpl  (SBK *s, INST *c)
  {
    // complement bits
    do_dflt_ops(s,c);
    c->opr[1].val = (~c->opr[1].val) & 0xffff;         // size ?

    if (s->emulating)
     {
      pset_psw(c, 0, 0xa);      // clear bits V,C
      set_psw(c, c->opr[1].val, c->opr[1].wsize, 0x30);     // N and Z
     }
  }



void check_R11(INST *c)
{
  // extra handler for write to Reg 11 for both stx and ldx
  // done AFTER assignment, but before upd_watch
  // R11 is not written to anyway by upd_watch
  int val;
  OPS *o;

  if (c->opcsub != 1)  return;     // immediate only

  o = c->opr + c->wop;              // always 2 ??

  if (o->addr != 0x11) return;

  // specifically write to ibuf, to keep it for reads ?

  val = c->opr[1].val & 0xf;
  ibuf[0x11] = val;

  // do this only if it's an imd as this is reliable, else ignore it ???

  if (bkmap[val].bok)
    {              // valid bank
     datbnk = val << 16;
     #ifdef XDBGX
      DBGPRT(1,"%x R11 Write imd, DataBank=%d",c->ofst, val);
     #endif
    }
 }



void stx (SBK *s, INST *c)
  {
   /* NB. this can WRITE to indexed and indirect addrs
    * but addresses not calc'ed in print phase
    * does not change PSW
    */

  FSTK *t, *x;
  OPS  *o;
  RST  *r, *b;
  int val, inx;

  calc_mult_ops(s,c);

  c->opr[1].val = c->opr[2].val;

  if (anlpass >= ANLPRT)  return;

  // now check if this is a stack operation
  // if it is a stack register, treat similarly to a PUSH

  o = c->opr + 4;             // to check inx register

  if (s->scanning) x = scanstack; else x = emulstack;

  if (o->addr ==  stkreg1 && s->emulating)
    {
      inx = -1;

      if (c->opcsub == 2) inx = 0;                    // inr
      if (c->opcsub == 3) inx =  c->opr[0].addr/2;    // inx

      if (inx >=0 && inx < STKSZ)
        {
         t = x+inx;
         val =  c->opr[2].val |= g_bank(t->origadd);       // for stack checks
         t->newadd = val;  //update both scan and emu
         do_args(s,t);
   #ifdef XDBGX
         DBGPRT(1,"PUSH [%d] via STX R%x=%x over %x at %x", inx, c->opr[2].addr, val, t->origadd, c->ofst);
#endif
          r = find_rgstat(c->opr[2].addr);
          if (r) r->popped = 0;               // clear popped from sce reg

         if (t->type != 1)
          {
                 #ifdef XDBGX
           DBGPRT(0,"PSW INV PUSH!! %d ",t->type);
           DBG_stack(x);
           #endif
           return;
          }
        }
    }

// now need to do args check here, jst like pop/push
// clear popped marker if scanning but nothing else - DONE

// 1 = 2 for stx (if not stack reg),


  if (s->emulating)
    {
          // 1 = 2 for ldx (if not stack reg),
          // so get source register from 1 (or 4 if ind or inx)

          o = c->opr + 1;
          r = find_rgstat(c->opr[2].addr);    //  t->origadd);

          if (r && r->arg)
          {
           b = add_rgstat(o->addr, o->ssize);
           b->arg = 1;
           b->sreg  = r->sreg;
   #ifdef XDBGX
           DBGPRT(1,"ADD ARG STX r%x=%d sr%xat %x", b->reg, r->ssize, r->sreg,c->ofst);
           #endif
          }
     }

check_R11(c);
 }





void ldx(SBK *s, INST *c)
  {
   // does not change psw

   int inx;
   RST *org, *nrg;
   OPS *o, *sce;
   FSTK *t, *x;

   calc_mult_ops(s,c);

   o = c->opr+2;

   o->val = c->opr[1].val;        // [2] = [1] as default op

   if (anlpass >= ANLPRT)  return;

   if (s->scanning) x = scanstack; else x = emulstack;


  // check if [4] =  stack register. If so this is a load from stack,
  // analogous to a POP, and will always be indirect or indexed

  if (c->opr[4].addr == stkreg1)
    {
           #ifdef XDBGX
    DBG_stack(x);
    #endif
      inx = -1;
      if (c->opcsub == 2) inx = 0;                    // inr
      if (c->opcsub == 3) inx =  c->opr[0].addr/2;    // inx

      if (inx >=0 && inx < STKSZ)
        {
         t = x + inx;
         o->val = get_stack(s, t, 7);        // anything
         org = add_rgstat(o->addr, 2);            // find or add
         t->reg = o->addr;
         t->popped = 1;                      // same as a pop
         org->popped = 1;                      // set popped for emu detect
         // org->ssize = 2;                       // word
            #ifdef XDBGX
         DBGPRT(1,"%x POP via LDX R%x=%x [%d] T%d", c->ofst, c->opr[2].addr, c->opr[2].val, inx, t->type);
         DBGPRT(1,"set popped, size = 2");
    #endif
         if (t->type == 1 && g_bank(o->val) != datbnk)
          {

           datbnk = g_bank(o->val);
           #ifdef XDBGX
            DBGPRT(1,"set dbnk %d, %x", datbnk, datbnk);
            #endif

            // set databank here for correct [Rx++] calls ? if t->type == 1
          }
        }
    }

  check_R11(c);

  if (s->emulating)
    {
          // 2 = 1 for ldx (if not stack reg),
          // so get source register from 1 (or 4 if ind or inx)
      org = 0;
      sce = 0;
      if (!c->opcsub) sce = c->opr + 1;
      if (c->opcsub > 1) sce = c->opr + 4;

      if (sce) org = find_rgstat(sce->addr);

      if (org)
        {     // found source reg
          if (org->arg)
              // reg is flagged as arg, so use size
           {
             // really what we are interested in for now is ONLY the SIZE
             set_rgsize(chrgst.lastix, o->ssize);

             org = find_rgstat(org->sreg);
             if (org) set_rgsize(chrgst.lastix, o->ssize);
           }
         else
         if (org->popped)
              // reg flagged as popped so make [wop] an arg - would expect this to be indirect or indexed
           {
            nrg = add_rgstat(o->addr, o->ssize);
            if (nrg)
             {
              nrg->arg = 1;
              nrg->sreg  = org->reg;

                 #ifdef XDBGX
              DBGPRT(1,"ADD ARGP %x at %x z%d", nrg->reg, s->start, nrg->ssize);
              #endif
             }
           }
        }

      }

  }


void orx(SBK *s, INST *c)
 {

  calc_mult_ops(s,c);

  if (c->opcix < 32)    c->opr[2].val |=  c->opr[1].val;   // std or
  else  c->opr[2].val ^=  c->opr[1].val;                   // xor

  if (s->emulating)
     {
      pset_psw(c, 0, 0xa);      // clear bits V,C
      set_psw(c, c->opr[2].val, c->opr[2].wsize, 0x30);     // N and Z
     }
 }

void add(SBK *s, INST *c)
 {
      // 2 or 3 op
   calc_mult_ops(s,c);

   c->opr[c->numops].val =  c->opr[2].val + c->opr[1].val;

   if (s->emulating)
   set_psw(c, c->opr[c->numops].val, c->opr[c->numops].wsize, 0x3e);
 }


void andx(SBK *s, INST *c)
 {
       // 2 or 3 op
  calc_mult_ops(s,c);
  c->opr[c->numops].val =  c->opr[2].val & c->opr[1].val;

 if (s->emulating)  pset_psw(c, 0, 0xa);
 // set_psw(2, 0x30);

 }


void sub(SBK *s, INST *c)
 {      // 2 or 3 op
    calc_mult_ops(s,c);

    c->opr[c->numops].val =  c->opr[2].val - c->opr[1].val;

 // sub is a BORROW not a carry
    if (s->emulating)
      {
       set_psw(c, c->opr[c->numops].val, c->opr[c->numops].wsize, 0x3e);
       c->psw ^= 2;       // complement of carry
      }
 }


void cmp(SBK *s, INST *c)
 {
   // use curops[0] as temp holder for subtract.
   // sub is a BORROW not a carry

  calc_mult_ops(s,c);

  if (s->emulating)
    {
     c->opr[0].val =  c->opr[2].val - c->opr[1].val;
     set_psw(c, c->opr[0].val, c->opr[1].ssize,0x3e);
     c->psw ^= 2;       // complement of carry
    }
 }

void mlx(SBK *s, INST *c)
 {     //psw not clear - assume no changes; 2 or 3 op
    calc_mult_ops(s,c);
    c->opr[c->numops].val =  c->opr[2].val * c->opr[1].val;  // 2 and 3 ops
 // PSW ???

 }

void dvx(SBK *s, INST *c)
 {
   // quotient to low addr, and remainder to high addr
   // always 2 op ??
    long x;
    int ix;

    calc_mult_ops(s,c);

    ix = c->numops;

    x = c->opr[ix].val;  // keep original
    if (c->opr[1].val != 0) c->opr[ix].val /= c->opr[1].val;    // if 2 ops (wop = 2)

    x -= (x * c->opr[ix].val);             // calc remainder

    if (c->opr[ix].ssize & 1)              // long/word
        {
         c->opr[ix].val  &= 0xffff;
         c->opr[ix].val  |= (x << 16);       // remainder
        }
    else
       {          // word/byte
         c->opr[ix].val  &= 0xff;
         c->opr[ix].val  |= (x << 8);       // remainder
        }

// PSW ??
 }


void calc_shift_ops(SBK *s, INST *c)
{
    short b;
     b = g_byte(s->nextaddr+1);             // first data byte after (true) opcode
     op_calc (b, 1,c);                      // 5 lowest bits only from firstb, from hbook

     if (b < 16) op_imd(c);                 // convert op 1 to absolute (= immediate) shift value
     op_calc (g_byte(s->nextaddr+2),2,c);
     s->nextaddr += c->numops + 1;
    }

void shl(SBK *s, INST *c)
 {

   /* manual - The right bits of the result are filled with zeroes.
     The last bit shifted out is saved in the carry flag.
      * sticky appears to be last carry ?
   */

  long mask, v;

  calc_shift_ops(s,c);
  v = c->opr[2].val;
  v <<= c->opr[1].val;  // as a 64 long, for DW shifts

  if (s->emulating)
    {
     set_psw(c, v, c->opr[2].wsize,0x30);             // Z and N
     mask =  camk[c->opr[1].ssize] + 1;         // get to 'size + 1' bit (= last shifted)
     if (v & mask) c->psw |= 2;  else c->psw &= 0x3d;   // set or clear carry flag
    }
  c->opr[2].val =  v;
 }

void shr(SBK *s, INST *c)
 {
// arith shift vs uns ??

 /* manual  The last bit shifted out is saved in the carry flag.
   The sticky bit flag is set to indicate that, during a right shift, a “1” has been shifted into the
   carry flag and then shifted out.
*/

  calc_shift_ops(s,c);


 // c->psw &= 0x3b;         // clear sticky first

  c->opr[2].val >>=  c->opr[1].val;    // always 2

// if (!s->scanning) return;
  // and need to sort out carry and sticky here....
 }

void inc(SBK *s, INST *c)
 {
   do_dflt_ops(s,c);
   // ALWAYS 1 (only autoinc is one or two)
   c->opr[1].val++;

   if (s->emulating)
     {

      set_psw(c, c->opr[1].val, c->opr[1].ssize, 0x3e);
     }

 }

void dec(SBK *s, INST *c)
 {
    do_dflt_ops(s,c);
   // ignore for scans
      c->opr[1].val--;
    if (s->emulating)
     {

      set_psw(c, c->opr[1].val, c->opr[1].ssize, 0x3e);
     }
 }


int calc_jump_ops(SBK *s, int jofs, INST *c)
{
    // s->nextaddr must correctly point to next opcode.
    // returns jump address

     OPS *o;

     jofs += s->nextaddr;                            // offset as calc'ed from NEXT opcode
     jofs = nobank(jofs);                            // pcj allow for wrap in 16 bits

     o = c->opr+1;                                   // set up opr[1] as a jump (=address)
     o->adr = 1;
     o->rgf = 0;
     o->rbs = 0;
     o->addr = codebank(jofs, c);                        // add CODE bank
     o->val = g_val(o->addr,o->ssize);             // get value from jump address, sized by op

     jofs = c->opr[1].addr;                          // now is full address with bank

     return jofs;
  }


void cjm(SBK *s, INST *c)
 {    // conditional jump, (short)
    // all types here based upon psw

   s->nextaddr += 2;
   calc_jump_ops(s,negval(g_byte(s->nextaddr-1),0x80),c);


   if (s->scanning)
    {
    do_sjsubr(s, c, J_COND);
    return;
    }

if (s->emulating) {

// do actual jump if emulating
switch (c->opcix)
   {

    case 54:            // JNST      STK sticky is 1
       if (!(c->psw & 1))   s->nextaddr = c->opr[1].addr;
       break;

    case 55:            // JST
      if ((c->psw & 1))   s->nextaddr = c->opr[1].addr;
       break;

    case 56:            // JNC       CY carry is 2
       if (!(c->psw & 2))   s->nextaddr = c->opr[1].addr;
       break;

    case 57:            // JC
       if (c->psw & 2)   s->nextaddr = c->opr[1].addr;
       break;

    case 58:            // JNV      OV overflow is 8
       if (!(c->psw & 8))   s->nextaddr = c->opr[1].addr;
       break;

    case 59:            // JV
      if (c->psw & 8)   s->nextaddr = c->opr[1].addr;
       break;

    case 60:            // JNVT  OVT overflow trap is 4
       if (!(c->psw & 4))   s->nextaddr = c->opr[1].addr;
       break;

    case 61:            // JVT
       if (c->psw & 4)   s->nextaddr = c->opr[1].addr;
       break;

    case 62:            // JGTU  C (2) set and Z (0x20) unset
       if ((c->psw & 0x22) == 2)   s->nextaddr = c->opr[1].addr;
       break;

    case 63:            // JLEU  C unset OR Z set
       if ((c->psw & 0x22) != 2)   s->nextaddr = c->opr[1].addr;
       break;

    case 64:            // JGT  N (16) unset and Z (32) unset
       if (!(c->psw & 0x30))   s->nextaddr = c->opr[1].addr;
       break;

    case 65:            // JLE  N set OR Z set
      if (c->psw & 0x30)   s->nextaddr = c->opr[1].addr;
       break;

    case 66:            // JGE  N not set N is 16
       if (!(c->psw & 16))   s->nextaddr = c->opr[1].addr;
       break;

    case 67:            // JLT  N (negative) is set
      if (c->psw & 16)   s->nextaddr = c->opr[1].addr;
       break;

    case 68:            // JE   (Z set)  Z is 32, zero flag
       if (c->psw & 32)   s->nextaddr = c->opr[1].addr;
       break;

    case 69:            // JNE    (Z not set)
      if (!(c->psw & 32))   s->nextaddr = c->opr[1].addr;
       break;

    default:
       break;
   }

#ifdef XDBGX
if (s->nextaddr == c->opr[1].addr)      // debug confirm jump taken
  {
   DBGPRT(1,"cjumpE %x->%x (psw=%x)", c->ofst, c->opr[1].addr, c->psw);
  }
#endif

}   // end of emulating
 }

void bjm(SBK *s, INST *c)
 {     // Bit Jump.  JB, JNB,
     //  add bit operand but don't change opcode size
    int xofst;
    xofst = s->nextaddr;
    s->nextaddr += 3;

    c->opr[3].bit = 1;                      // setup bit number in op3,
    c->opr[3].rgf = 0;
    c->numops++;                            // local change, NOT opcode size.
    op_calc (g_byte(xofst) & 7, 3,c);       // bit number
    op_calc (g_byte(xofst+1), 2,c);         // register
    calc_jump_ops(s, g_val(xofst+2,5),c);   // jump destination

    if (s->scanning)
      {
       do_sjsubr(s, c, J_COND);
       return ;
      }

  //  0 is LS bit .... jnb is 70, jb 71

if (s->emulating)
    {

    if (check_backw(c->opr[1].addr, s, J_STAT)) return;

    xofst = (1 << c->opr[3].val);    // set mask
  // bit set and JB
  if ((c->opr[2].val & xofst) && (c->opcix & 1)) s->nextaddr = c->opr[1].addr;
  // bit not set and JNB
  if (!(c->opr[2].val & xofst) && !(c->opcix & 1)) s->nextaddr = c->opr[1].addr;
   }

 }

void sjm(SBK *s, INST *c)
 { //  a short jump
    int jofs;

    jofs = g_byte(s->nextaddr) & 7;
    jofs <<= 8;
    jofs |= g_byte(s->nextaddr+1);
    jofs =  negval(jofs,0x400);                     // signed 11 bit value

    s->nextaddr += 2;

    calc_jump_ops(s, jofs,c);

    if (s->scanning)
      {
       do_sjsubr(s, c, J_STAT);
       return;
      }

if (s->emulating)
    {
     if (check_backw(c->opr[1].addr, s, J_STAT)) return;
     s->nextaddr = c->opr[1].addr;      // go direct in emu mode
    }

 }


void ljm(SBK *s, INST *c)
 {      //long jump
   int jofs;

   s->nextaddr += 3;
   jofs = g_val(s->nextaddr-2,6);
   calc_jump_ops(s, jofs,c);

   if (s->scanning)
     {
       do_sjsubr(s, c, J_STAT);
       return;
     }

if (s->emulating)
    {
 if (check_backw(c->opr[1].addr, s, J_STAT)) return;
    s->nextaddr = c->opr[1].addr;      // go direct in emu mode
    }
  }



void cll(SBK *s, INST *c)
 {     // long call
    SBK *n;
    s->nextaddr += 3;
    calc_jump_ops(s,g_val(s->nextaddr-2,6),c);
   #ifdef XDBGX
    if (anlpass < ANLPRT) DBGPRT(1,"call %x from %x", c->opr[1].addr, s->curaddr);
    #endif
    if (s->scanning)
      {
       do_sjsubr(s, c, J_SUB);
       return;
      }

if (s->emulating)
    {
       SUB *sub;
       sub = get_subr(c->opr[1].addr);
       if (sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opr[1].addr, sub->size);
           #endif
           s->nextaddr += sub->size;
           return;
         }   


     n = add_escan(c->opr[1].addr, s);
     fakepush(s, c->opr[1].addr,1);            // std subr call, push return details onto stack

   #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opr[1].addr, s->curaddr);
     #endif
     scan_blk(n, &einst);                      // recursive

// break here ???  if args set ??

    }
 }


void scl(SBK *s, INST *c)
 { // short call
   int jofs;
   SBK *n;
   jofs = g_byte(s->nextaddr) & 7;
   jofs <<= 8;
   jofs |= g_byte(s->nextaddr+1);
   jofs =  negval(jofs,0x400);                     // signed 11 bit value
   s->nextaddr+=2;
   calc_jump_ops(s,jofs,c);
   #ifdef XDBGX
  if (anlpass < ANLPRT) DBGPRT(1,"call %x from %x", c->opr[1].addr, s->curaddr);
  #endif
   if (s->scanning)
     {
      do_sjsubr(s, c, J_SUB);
      return;
     }

 if (s->emulating)
    {
       SUB *sub;
       sub = get_subr(c->opr[1].addr);
       if (sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opr[1].addr, sub->size);
           #endif
           s->nextaddr += sub->size;
           return;
         }   








     n = add_escan(c->opr[1].addr, s);
     fakepush(s, c->opr[1].addr,1);              // std subr call, push return details onto stack
   #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opr[1].addr, s->curaddr);
     #endif
     scan_blk(n, &einst);                          // recursive (as true core)
    }

 }


void ret(SBK *s, INST *c)
 {           // return

   s->nextaddr++;
   calc_jump_ops(s, -1,c);      // do as loopstop jump


   if (s->scanning)
    {
     clearchain(&chrgst);         //clear regstat if not emulating
     do_sjsubr(s, c, J_RET);
     return;
    }

   if (anlpass < ANLPRT) {
    if (s->emulating)
    {
     fakepop(s);                     // check & drop the stack call
    }

   #ifdef XDBGX
   DBGPRT(0,"%x RET", s->curaddr);
  if (s->emulating) DBGPRT(0," E");
  DBGPRT(1,0);
 DBGPRT(1,0);
#endif


// always end this block
   end_scan(s);

 }

}



void djm(SBK *s, INST *c)
 { // djnz
   int xofst;


   xofst = s->nextaddr;
   s->nextaddr += 3;
   op_calc (g_byte(xofst+1), 2,c);              // do register

   calc_jump_ops(s,g_val(xofst+2,5),c);

   if (s->scanning)
     {
      do_sjsubr(s, c, J_COND);
      return ;
     }

   if (s->emulating)
     {

       RST *r;
       OPS *o;
       o = c->opr+2;
       c->opr[2].val--;
       if (c->opr[2].val > 0) s->nextaddr = c->opr[1].addr;
       else
        {
         r = find_rgstat(o->addr);
         if (r)
             {chdelete(&chrgst,chrgst.lastix,0);    // delete this - loop variable
                 #ifdef XDBGX
                      DBGPRT(1,"delete rgstat %x", o->addr);
                      #endif
}
        }
     }
 }

void skj(SBK *s, INST *c)
 {  // skip, = jump(pc+2) as static jump
    s->nextaddr++;

    calc_jump_ops(s,1,c);

    if (s->scanning)
      {
       do_sjsubr(s, c, J_STAT);
       return;
      }

  if (s->emulating)
   {
      s->nextaddr++;      // skip next byte
   }
 }




void php(SBK *s, INST *c)
 {
     // push flags (i.e. psw)

   s->nextaddr++;
   s->php = 1;

     if (numbanks)
    {
      fakepush(s, c->psw, 4);
         #ifdef XDBGX
      if (anlpass < ANLPRT) DBGPRT(0,"%x PUSHP ", c->ofst);
  //    DBG_stack(s->emulating);
      #endif
    }

 }

void ppp(SBK *s, INST *c)
 {
     // pop flags (i.e. psw)
     // ignore in scan

   s->nextaddr++;
   s->php = 0;

   FSTK *x;
   if (s->emulating)
      {
        x = emulstack;       //else  x = fstack;

       if (numbanks)
      {
       get_stack(s, x, 4);       // for error report
       fakepop(s);
      }
}


 }

void nrm(SBK *s, INST *c)
 {
      do_dflt_ops(s,c);

      // from Ford hbook shift left double until b31 = 1
      // and result size is double too....as it's shifted...

      // TO DO
 }


void sex(SBK *s, INST *c)
 {
     // sign extend - not necessary in this setup ?

      do_dflt_ops(s,c);
 }


void clc(SBK *s, INST *c)
 {
     // clear carry
   c->psw &= 0x3d;
   s->nextaddr++;
 }

void stc(SBK *s, INST *c)
 {
     // set carry
   c->psw |= 2;
   s->nextaddr++;
 }

void die(SBK *s, INST *c)
 {
     // enable & disable ints

    s->nextaddr++;
 }

void clv(SBK *s, INST *c)
 {   // clear trap
  c->psw &= 0x3b;
  s->nextaddr++;
 }

void nop(SBK *s, INST *c)
 {
      s->nextaddr++;
 }

void bka(SBK *s, INST *c)
 {          // RAM bank swopper 8065 only
   #ifdef XDBGX
   int bk;
   //   According to Ford book, this only sets PSW, so could be cancelled by a POPP
   //   or PUSHP/POPP save/restore,  so may need a PREV_BANK

   bk = c->opcix - 107;                   // new RAM bank
   //     rambank = bk * 0x100;                 // establish new offset

   if (s->scanning) DBGPRT(1,"New Rambank = %x at %x", bk, s->curaddr);
   #endif
   op_imd(c);                                    // mark operand 1 as immediate
   s->nextaddr++;
 }


void do_code (SBK *s, INST *c)

{
     /* analyse next code statement from ofst and print operands
      * s is a scan blk, c is holding instance for decoded op.
      * pp-code calls this with a dummy scan block in print phase
      */

  int x, xofst, indx, psw;
  OPC *opl;
  OPS *o;

  codbnk = g_bank(s->curaddr);                // current code bank from ofst

  if (s->stop)
   {
    #ifdef XDBGX
     DBGPRT(1,"Already Scanned s %x c %x e %x", s->start, s->curaddr, s->nextaddr);
    #endif
    return;
   }

    if (s->curaddr > maxadd(s->start) || s->curaddr < minadd(s->start))
    {
      #ifdef XDBGX
        DBGPRT(1,"Scan > Illegal address %x from blk %x, STOP", s->curaddr, s->start);
      #endif
      s->stop = 1;
      return;
    }


  if (!val_code_addr(s->curaddr))
    {
     #ifdef XDBGX
     DBGPRT(1,"Invalid address %05x", s->curaddr);
     #endif
     s->inv = 1;
     s->stop = 1;
     s->nextaddr = s->curaddr+1;
     return;
    }

  xofst = s->curaddr;                             // don't modify curaddr, use temp instead

//----------------------------

  psw = c->psw;                                     // keep psw from supplied instance
  memset(c,0,sizeof(INST));                       // clear entry

  x = g_byte(xofst);
  indx = opcind[x];                       // opcode table index
  opl = opctbl + indx;

 // Ford book states opcodes can have rombank(bank) AND fe as prefix (= 3 bytes)

   if (indx == 112)                     // bank swop - do as prefix
     {
      if (!P8065) indx = 0;             // invalid for 8061
      else
       {
        int tbk;
        xofst++;
        tbk = g_byte(xofst++);                         // bank number (next byte)
        tbk &= 0xf;

        if (bkmap[tbk].bok)
          {
            c->bnk  = 1;                                 // mark bank instruction
            c->bank = tbk;                               // and keep it for this opcode
          }
        #ifdef XDBGX
        else
         {
          if (anlpass < ANLPRT) DBGPRT(1,"ROMBNK - Invalid bank %d (%x)", tbk, c->ofst);
         }
        #endif

        x = g_byte(xofst);                // get true opcode
        indx = opcind[x];
       }
     }

  if (indx == 111)
     {                                  // this is the SIGN prefix, find alternate opcode
      xofst++;
      x = g_byte(xofst);
      indx = opcind[x];                 // get opcode
      opl = opctbl + indx;
      if (opl->sign)  indx += 1;        // get prefix opcode index instead
      else indx = 0;                    // or prefix not valid
     }

  if (opl->p65 && !P8065) indx = 0;     // 8065 only

  if (indx > 110) indx = 0;         // may happen if totally lost.....

  opl = opctbl + indx;

  if (!indx)
     {
       s->inv = 1;
       s->nextaddr = xofst + 2;      //to get nextaddr right for end_scan
       #ifdef XDBGX
          DBGPRT(1,"Invalid opcode (%x) at %x [%d]", x, s->curaddr, anlpass);
       #endif
       if (s->scanning) end_scan (s);
       else wnprt(1,"Invalid opcode at %x [%d]", s->curaddr, anlpass); // report only once
       return;
     }

  c->opcix = indx;                                // opcode index
  c->ofst  = s->curaddr;                          // original start of opcode
  if (!c->bnk) c->bank  = codbnk >> 16;           // bank to be used (codebank)

  // These items can be changed by opcodes, this is default setup

  c->numops = opl->nops;
  c->psw   = psw;                                 // carry over psw from last instance
  o = c->opr;                                     // shortcut to operands (4 off)
  c->wop = opl->wop;                              // write destination index (or zero)
  o[opl->wop].wsize = opl->wsz;                   // set write size (or zero)
  o[1].ssize = opl->rsz1;                         // operands - read size
  o[2].ssize = opl->rsz2;
  o[3].ssize = opl->rsz3;
  o[1].rgf  = 1;                                  // set register ops by default
  o[2].rgf  = 1;
  o[3].rgf  = 1;

  s->nextaddr = xofst;                      // address of actual opcode - for eml call

  opl->eml(s, c);                           // opcode handler - modifies nextaddr
                                            // s->nextaddr now points to next opcode
  upd_watch(s, c);
 // c->nextaddr = s->nextaddr;

 }



/**************************************************************************
* opcode printout - calls do_code
***************************************************************************/

uint pp_code (uint ofst, LBK *k)
{
  int i, cnt, inx, ainx, eqp, jf;

  const char  *tx ;
  const OPC* opl;
  INST *c;
  JMP *j;

  memset(&prtscan,0, sizeof(SBK));     // clear it first
  c = &cinst;
  prtscan.curaddr = ofst;
  prtscan.start = ofst;
  do_code(&prtscan, c);

//  preset_comment(ofst);  // begin and end reqd ??

  if (prtscan.inv)
    {
     pp_hdr (ofst, opctbl->name, 1);   // single invalid opcode (= !INV!)
     return prtscan.nextaddr;
     }

  // BANK - For printout, show bank swop prefix as a separate opcode
  // This is neatest way to show addresses correctly in printout
  // internally, keep 'ofst' at original address, but update curaddr for printout.

  if (c->bnk)
    {
     pp_hdr (ofst, "bank", 2);
     pstr ("%d", c->bank);
     prtscan.curaddr +=2;
    }

  inx = c->opcix;
  ainx = inx;                        // safety
  opl = opctbl + inx;

  cnt = prtscan.nextaddr - prtscan.curaddr;
  pp_hdr (prtscan.curaddr, opl->name, cnt);

  i = c->numops;
  while (i)
     {
      p_oper (c, i);              // code printout
      if (--i) pchar (',');
     }

  /**********************************************
  * source code generation from here
  * check and do substitutes and cleanups, name resolution etc.
  *********************************************/


  if (PSSRC)
    {
      if (opl->sce)
       {               // skip if no pseudo source
        p_pad(cposn[2]);
   //     pstr("|%d ", indent);         // experiment, for lining up and new brackets

        tx = opl->sce;                 // source code, before any mods
        eqp = 1;                       // equals not printed yet

        // any mods to source code go here

        fix_sym_names(&tx,c);           // for rbases, 3 ops, names, indexed fixes etc.
        shift_comment(c);               // add comment to shifts with mutiply/divide  - if (curinst.opcix < 10 && ops->imd)
        bitwise_replace(&tx, c);        // replace bit masks with flag names - ANDs and ORs  if (ix >  9 && ix < 14) ||  (ix > 29 && ix < 34)

        j = find_fjump (ofst, &jf);

        if ((jf & 1) && *tx)
         {                             // invert jump logic for source printout
           if (inx > 53 && inx < 72)   // all conditional jumps (except djnz)
           {
             ainx = inx ^ 1;           // get alternate opcode index if jump found.
             tx = opctbl[ainx].sce;    // and source code
indent++;
           }
           else jf = 0;
        }

       //  do 'psuedo source code' output

       while (*tx)
        {

         switch (*tx)   // 1-12 used  1-31 available (0x1 to 0x1f)
          {
           case 1:
           case 0x11:           // print size/sign
           case 2:
           case 0x12:
           case 3:
           case 0x13:

               p_opsc (c, *tx, eqp);
               break;

       case 5:
             // subr call, may have answer attached.
             pp_answer(c);
             p_opsc (c, 1, eqp);
             prtscan.nextaddr += pp_subargs(c,prtscan.nextaddr);
             jf = 10;                   // stop semicolon
             break;


//           case 4 6 8 not used

       case 6:     // overflow
            do_ovf(c, &tx, ainx);
            break;


       case 7:     // conditonal jumps
            do_cond_ops(c, &tx, ainx);
            break;

       case 8:       // carry 

            do_carry(c, &tx, ainx);   // JC and JNC
            break;





       case 9:
              // if (j<3)  always true at present
// make this bitmap instead of switch ???

             switch (jf)    //jump handling
             {
              case 1 :
                pstr (" {");   // [if] case
                break;
              case 2 :
// but this can break the {} sequences !!
                pstr ("return;");  // static jump to a return
                break;
              case 3:
                pchar(';');
                // fall through
              case 4:
                pstr("} else {");
                break;
              default:
                pstr ("goto ");
                p_opsc (c,1,0);
                //pchar (';');
                break;

             }
/*if (j && j->retn)
    { 
 //     if (jf == 1) { pstr(" }");  j->cbkt = 0; }


           acmnt.minst = c;
                acmnt.ctext = nm;
                acmnt.ofst = prtscan.nextaddr;
                sprintf(nm, "## -> Return");

}   */

        break;

       case 11:
          p_indl();             // 0xb  print new line with indent (DJNZ etc)
        break;

    //   case 12:
            // subroutine arguments, if any.
      //      prtscan.nextaddr += pp_subargs(c,prtscan.nextaddr);
      //      jf = 10;                   // stop semicolon
    //        break;

       case '=' :
             eqp = 0;                 //     equals printed
              // fall through
       default:
            pchar(*tx);
      }

     tx++;
    }
      }
    }
     else      // this is for embedded arguments without source code option - must still skip any operands
     {
      if (!PSSRC && (inx == 73 || inx == 74 || inx == 82 || inx == 83))          // CALL opcodes
         {
           prtscan.nextaddr += pp_subargs(c,prtscan.nextaddr);
            jf = 10;      // stop semicolon print
         }
     }

     if (*opl->sce && !jf) pchar(';');         // add semicolon if no open or close backets
     find_tjump (prtscan.nextaddr);         // add any reqd close bracket(s)

      do_comment(&acmnt);                   // print auto comment if one created, but need to check ofst !!

/*
  {
    OPS *s;                              // extra debug printout

  //  pp_comment(ofst+cnt);              // do comment first
    if (opctbl[curinst.opcix].nops)
     {
      p_pad(cposn[3]);                          //CDOSTART);
      pstr(" SVA ");

      for (i=curinst.numops;  i>=0; i--)
        {
         s = curops+i;
         if (s->addr + s->val)
          {
   //        if(curinst.opl->wop == i && i)  pstr("*");
           pstr("%c[",i+0x30);

           if (s->sym) pstr("%s ",get_sym_name(0,s->addr,0));       // temp fix.....

           pstr("%x %x",s->val, s->addr);
//           if (!i) pstr (" T%x", curinst.tval);          //new temp value
           pstr("]%x, ",s->ssize);
          }
        }
     }
  }                 // end dubug         */

  // add an extra blank line after END BLOCK opcodes, via comments
  // 75, 81 are jumps 76->79 are rets

if (prtscan.nextaddr <= k->end )
{
  if (inx > 75 && inx < 80) pp_comment(prtscan.nextaddr,1);   // Rets
 // if ((inx == 75 || inx == 81) && !j && !j->jelse) pp_comment(prtscan.nextaddr,1); //jumps
if (inx == 75 || inx == 81)  pp_comment(prtscan.nextaddr,1); //jumps
//  {
//   if (!j || (j && !j->jelse)) pp_comment(prtscan.nextaddr,1); //jumps CRASH FIX
//  }
}
  return prtscan.nextaddr;
}



// ---------------------------------------------------------------------------------------------

/// chain errors start at bit 2
// 0 no error
// 1 warning
// 2 error     (bit mask 3)

// 4,5 duplicate cmd
// 6,7 invalid address
// 8,9 banks don't match
// NO - change this to a struct lookup............much easier..............

//compress common terms






int do_error(CPS *c, const char *err, ...)
  {
    // cmnd string in flbuf....
    // use vfprintf directly as it does not work
    // if called via wnprt

   const char *t, *x;
   va_list args;
   int val;
   
   if (!err)  return 0;             // safety
   if (!*err)  return 0;             // safety
 
   if (!c->firsterr)
     {
      fprintf (fldata.fl[2], "## %*s",c->maxlen,flbuf);  //print cmd string once
      c->firsterr = 1;
     }

   va_start (args, err);
   t = err;
   
   // fist char expected to be a 1 or a 2 for error and warning.
   
   while (*t)
     {  //go down err string and look for special chars
       
       if (*t < 16)
         { //print compressed str
           val = *t;
           fprintf(fldata.fl[2],"%s", cmptxts[val]);
         }
       else
       if (*t == '%')
           {
            x = t+1;      //next char
            if (*x == 'd')
            {  
              val = va_arg(args, int);
              fprintf(fldata.fl[2],"%d", val);
              t+=1;
            }
            else
            if (*x == 'x')
            {  
              val = va_arg(args, int);
              fprintf(fldata.fl[2],"%x", val);
              t+=1;
            } 
            else
            if (*x == 's')
            {  
              x = va_arg(args, char*);
              fprintf(fldata.fl[2],"%s", x);
              t+=1;
            }             
            

           } 
        else fputc(*t,fldata.fl[2]);
        t++;
     }
    
           
   va_end(args);
   wnprt(1,0);                               // newline
   c->error = 1;
   return 1;           // answers one for use as 'return do_error('
}


uint set_rbas (CPS *c)
{
 RBT *x;

//should check databank for rbases p[1] 

 x = add_rbase(c->p[0]&0xFF, c->p[1], c->p[2], c->p[3] );
 if (chbase.lasterr) return do_error(c,etxts[chbase.lasterr]);
 x->cmd = 1;              // by command
 return 0;
}


uint set_opts (CPS *c)
{
  //opts bitmask in p[0]  
 // int x;
 // if (P8065) x = 1; else x = 0;

  cmdopts |= c->p[0];

  if (numbanks)  cmdopts |= 0x8;

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(0,"SetOpts = ");
    prtflgs(cmdopts, DBGPRT);
    DBGPRT(2,0);
  #endif
return 0;

}

uint clr_opts (CPS *c)
{
  cmdopts &= (~c->p[0]);

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(0,"ClrOpts = ");
    prtflgs(cmdopts, DBGPRT);
    DBGPRT(2,0);
  #endif
return 0;
    
}







void set_data_vect(LBK *blk)
{
 int ofst, xofst, end, pc;
 int bank;
 ADT *a;

 // for structs only
 if (PMANL)  return;
 if (!blk || blk->fcom != C_STCT) return;

 bank = g_bank(blk->start);      // what if bank is NOT same as struct
 
 for (ofst = blk->start; ofst < blk->end; ofst += blk->size)
 {   
  a = (ADT*) chmem(&chadnl);
  a->fid = blk;
  a->seq = -1;

  end = ofst + blk->size-1;
  xofst = ofst;
  while ((a = get_next_adnl(&chadnl,a)) && xofst < end)
    {
      if (a->vaddr)
      {
       pc = g_word(xofst);
       pc |= bank;                  // this assumes vect is always in same bank
       add_scan (pc, J_SUB, 0);      // auto adds subr
      }
     xofst += cellsize(a);        // always 2 No........
    }
  }  
}

void add_ans(void *fid, ADT *ca)
{  //special adt copy for subr answers
 ADT *a;

// insert ino answer chain
  a = add_adt(&chans, fid, 0);        // add new one
  *a = *ca;
  a->fid = fid;      // reset FK
  a->seq = 0;      // and renumber sequence

    #ifdef XDBGX
    DBGPRT(0,"      with answer params");
    prt_adt(&chans, fid, 0, DBGPRT);
    DBGPRT(1,0);
  #endif

}

void cpy_adt(CPS *c, void * fid)                 //, int start)
{  // move adt blocks from cmnd to new LBK

  int i, seq;
  ADT *a, *ca;
  if (!fid) return;

  if (c->levels < 0) return;                  // no addnl block

  seq = 0;

  for (i = 0; i <= c->levels; i++)            //start
    {
      ca = cmdadnl + i;
      if (c->fcom == C_ARGS || c->fcom == C_SUBR) ca->pfw = 0;    // clear all pfw

      if (ca->ans)
       {
        add_ans(fid,ca);
       }
      else

      if (ca->cnt)      // use cnt, not ssize)
       {     //ignore unsized (not data) entries
        a = add_adt(&chadnl,fid,128);      // add new one
        *a = *ca;
        a->fid = fid;        // reset FK
        a->seq = seq++;      // and renumber sequence
       }
    }


    #ifdef XDBGX
    DBGPRT(0,"      with addnl params");
    prt_adt(&chadnl,fid, dirs[c->fcom].decdft, DBGPRT);
    DBGPRT(1,0);
  #endif

}



// amend this for everything - change end if relevant
// to command


int chk_csize(CPS *c)
 {
  int bsize, tsize, row, bytes;
  DIRS *d;

 // if (!c->fcom) return 0;       // not for fill cmd
  
  d = dirs + c->fcom;
 // check addnl levels allowed

  row = d->maxadt -1;      //max levels
  bytes = d->minadt -1;      // min levels

  if (row < 0 && c->levels >= 0)
    {     //no extra allowed
     return do_error(c,"\001Extra\012\005");
    }

  if (c->levels < bytes )
    { // not enough levels
     return do_error(c,"\001Extra\012\004");
    }

   if (c->levels > row)
     {
        do_error(c,"\002Extra\012\010");
        c->levels = row;
     }


// check extra levels and sizes 

  bsize = c->p[1] - c->p[0]+1-c->term;    // byte size of cmd entry in total

  if (!d->defsze) tsize = c->tsize;  else tsize = d->defsze; 
  
  if (!tsize)
    {
      do_error(c,"Row size is ZERO !!");
      tsize = 1;
     }

   if (tsize > bsize)
     {
      if (c->p[0] == c->p[1] || d->defsze)
        {  // if end = start, or preset sizes, allow change
         c->p[1] = c->p[0] + c->tsize-1;
         bsize = c->tsize;
         do_error(c,"\002End increased to %x",c->p[1]);
        }
         else  return do_error(c,"\001End too low for start+data definition");
     }

   row = bsize/tsize;                  // number of whole rows
   bytes = row * tsize;                // number of bytes for whole rows

   if (bytes != bsize)
     {

// if close to right ??
       bytes += c->p[0] + c->term - 1;
       c->p[1] = bytes;
       
       // check y against end ?

       do_error(c,"\002End inconsistent with size, reduced to %x", bytes);
      
     }

   return 0;
 }


uint set_func (CPS *c)
{
  // for functions.  2 levels only

  int val, ssize, startval;
  LBK *blk;

  // size row vs. start>end check first

  if (c->levels < 1) cmdadnl[1] = cmdadnl[0];
  c->levels = 1;

  if (chk_csize(c)) return 1;

  ssize = cmdadnl[0].ssize;                   //size and sign as specified

  // check start value consistent with sign

  val   = g_val(c->p[0], ssize);
  startval = (~casg[ssize]) & camk[ssize];     // start value from SIGN

  if (val != startval)  // try alternate sign....does not change command though
    {
     ssize ^= 4;          // swop sign flag
     startval = (~casg[ssize]) & camk[ssize];     // new start value
     if (val != startval) do_error(c,"\002Function Start value\003");
     else  do_error(c,"\002First value (%x) indicates wrong sign specified", val);
    }

  blk = add_data_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chdata.lasterr) return  do_error(c,etxts[chdata.lasterr]);

  cpy_adt(c,blk);
  blk->size = totsize(blk);

  return 0;
}

uint set_tab (CPS *c)
{
   // tables, ONE level only

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_data_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chdata.lasterr) return do_error(c,etxts[chdata.lasterr]);      //do_error(c,2,0);

  cpy_adt(c,blk);
  blk->size = totsize(blk);

 return 0;
}


uint set_stct (CPS *c)
{
   // structures

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_data_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chdata.lasterr) return do_error(c,etxts[chdata.lasterr]);      //do_error(c,2,0);

  cpy_adt(c,blk);
  blk->size = totsize(blk);

  blk->term = c->term;                    // terminating byte flag (struct only)
  blk->argl = c->argl;

  set_data_vect(blk);   // for Refs out in structs
  return 0;
}





uint set_data (CPS *c)
{
   // change for word, byte, long
   // no addnl is fine

  if (chk_csize(c)) return 1;

  LBK *blk;
  blk = add_data_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chdata.lasterr) return do_error(c,etxts[chdata.lasterr]);

  cpy_adt(c,blk);
  blk->size = totsize(blk);

  return 0;
}


uint set_sym (CPS *c)
{
  int bitno,write, flags, i;
  ADT *a;
  SYM  *s;

// flagsword.
// allow/inhibit if immediate...


  write = 0;           // read or write
  flags = 0;           // flags word
  bitno = -1;

  if (c->npars < 3)
   {
    c->p[1] = 0;         // default address range to ALL (zeroes) otherwise done by cmd
    c->p[2] = 0;
   }

 for (i = 0; i <= c->levels; i++)
   {
    a = cmdadnl + i;           //->adnl;
    if (a)
     {
      if (a->enc == 7)
        {        // bit marker 'T' or 'F'
         if (a->data < 16)  bitno = (a->data & 0xf);    // 0-15 (bitno)
         else flags = 1;
        }

      if (a->ssize & 2)  write = 1;      // write 'W'
     }
   }
   // OK now add symbol to right chain (=w)

  s = add_sym(write|C_CMD,c->symname, c->p[0], bitno, c->p[1], c->p[2]);
  
 /* if (chsym.lasterr == E_DUPL)
   {      
     if (*c->symname)
       {
         strncpy(nm,s->name, 63);  
         new_symname (s, c->symname); // may change sym if it's not a cmd
         chsym.lasterr = 0;
         if (!s->sys) do_error(c,etxts[E_SYMNAM], nm);
         s->sys = 0;
         s->cmd = 1;        //clear system, set user cmd
       }
   }   
  if (chsym.lasterr)   return do_error(c,etxts[chsym.lasterr]);
*/
  if (chsym.lasterr && chsym.lasterr != E_DUPL)   return do_error(c,etxts[chsym.lasterr]);

     
     
  if (flags) s->flags = 1;          // with flags (not for bits...)
  if (c->argl) s->immok = 1;
  return 0;
}




uint set_subr (CPS *c)
{
 // int ix;
  SUB *xsub;
 // ADT *a;
  SPF *f;

  // if (chk_csize(c)) return 1;  can't do this !!

  xsub = add_subr(c->p[0]);

  // can do name here, even if a duplicate.............

  if (!xsub || xsub->cmd)
    {   //allow def of existsing system one
     if (chsubr.lasterr) return do_error(c,etxts[chsubr.lasterr]);
    } 
  xsub->cmd = 1;

  // do any special functions first

  f = 0;

  if (c->spf)
     {            // special func is marked

      f = add_spf(xsub);
      f->spf = c->spf;   // start of funcs
      f->sizein  = anames[f->spf].sizein;
      f->sizeout = anames[f->spf].sizeout;
      f->addrreg = c->adreg;          // register
      f->sizereg = c->szreg;             // cols

      #ifdef XDBGX
        DBGPRT(0, "  with spf = %x ", f->spf);
      #endif
    }  // end special func

     add_scan (c->p[0], J_SUB|C_CMD,0);

     cpy_adt(c, xsub);

  //   add_ans(xsub);

     xsub->size = totsize(xsub);
     xsub->cptl = c->cptl;

 return 0;
 }


uint set_time(CPS *c)
 {
  LBK *blk;
 // int val,type,bank;
 // uint xofst;
 // short b, m, sn;
 // SYM *s;
 // char *z;

  blk = add_data_cmd (c->p[0],c->p[1],c->fcom|C_CMD,0);
  if (chdata.lasterr) return do_error(c,etxts[chdata.lasterr]); 
  cpy_adt(c,blk);
  //s =
 // add_sym(C_CMD,c->symname,c->p[0],-1, 0,0);  // safe for null name
 // if (s) s->cmd = 1;

  /* up to here is same as set_prm...now add names

  if (!blk) return 1;          // command didn't stick
  a = blk->adnl;
  if (!a) return 0;                     //  nothing extra to do
  sn = 0;                             // temp flag after SIGN removed
  if (a->data) sn = 1;
  if (!a->name && !sn) return 0;       // nothing extra to do

  if (a->ssize < 1) a->ssize = 1;  // safety

  xofst = c->p[0];
  bank = g_bank(xofst);
  while (xofst < maxadd(xofst))
   {
    type = g_byte (xofst++);                  // first byte - flags of time size
    if (!type) break;                         // end of list;

    val = g_val (xofst, a->ssize);            // address of counter
    val |= bank;

    if (a->name)
    {
     z = nm;
     z += sprintf(z, "%s", anames[0].pname);
     z += sprintf(z, "%d", anames[0].nval++);
     if (type& 0x4)  z += sprintf(z,"D"); else z+=sprintf(z,"U");
     if (type&0x20) z += sprintf(z,"_mS");
     else
     if (type&0x40) z += sprintf(z,"_8thS");
     else
     if (type&0x80) z += sprintf(z,"_S");
     s = add_sym(0,nm,a->data,0,0);     // add new (read) sym
    }

    xofst+= casz[a->ssize];                           // jump over address (word or byte)

    z = nm;
    if (type&1)      // int entry
     {
      if (s && sn)
        {                      // add flags word name too
        m = g_byte (xofst++); // bit mask
        b = 0;
        while (!(m&1)) {m /= 2; b++;}    // bit number
        val = g_byte (xofst++);    // address of bit mask
        val |= bank;
        if (type & 8) z+= sprintf(z,"Stop_"); else z += sprintf(z,"Go_");
        sprintf(z,"%s",s->name);
        add_symbit(0 ,nm, val, b, 0, 0);
        }
      else     xofst+=2;                // jump over extras
     }
   }
*/

      // upd sym to add flags and times
      return 0;
 }


int find_text(BANK *b)
{
  // check for text in end filler block
  // text blocks have either "Copyright at ff63" or ".HEX* ff06"  in them...
  // check address above ff00 for copyright and then check blocks....
  // works for now, more or less

  // BUT 0FAB has NO COPYRIGHT STRING !!!

  int ans, val, start, ofst, bk, cnt;

  if (nobank(b->maxbk) < 0xdfff)  return 0;          // not full bank

  if (strncmp("Copyright", (char *) b->fbuf + 0xff63, 9)) return 0;  // not found

  // matched Copyright string
  bk = g_bank(b->minpc);

  start = (0xff63 | bk);
  ofst = start;

  ans = 0;
  val = g_byte(ofst);

  while (!val || (val >=0x20 && val <= 0x7f))
   {
      ofst++;
      val = g_byte(ofst);
   }
  add_data_cmd (start,ofst, C_TEXT |C_SYS,0);
  add_data_cmd (ofst+1,b->maxbk, C_DFLT|C_SYS,0);    // treat as cmd (cannot override)

  ans = start - 1;

  ofst = (0xff06 | bk);
  cnt = 0;
  val = g_byte(ofst);

  while (ofst < (0xff63 | bk))
   {     // allow a few non printers...6 ?
     if (val < 0x20 || val > 0x7f)
      {
        if (cnt > 5) add_data_cmd (ofst-cnt,ofst-1, C_TEXT |C_SYS,0);
        cnt = 0;
      }
     else
      {
       cnt ++;
       start = ofst;
      }
     ofst++;
     val = g_byte(ofst);
   }

    ans = start-1;

  return ans;
 }












void find_filler(BANK *b)
  {

// look for filler at end of bank.

   uint ofst, last;
   int cnt, val;

   last = b->maxbk;
   b->maxpc = b->maxbk;             // default and safety

   ofst = find_text(b);             // any text found ?

   if (!ofst) ofst = b->maxbk; else last = ofst;

   cnt = 0;
   for (; ofst > b->minpc; ofst--)
     {
      val = g_byte(ofst);

      if (val != 0xff)
         {
          cnt++;                  // count non 0xff
         }
      if (cnt > 3) break;         // 2 non 0xff and no text match
     }


   ofst += 4;                     // 2 count plus one - may need more for a terminator byte ?

   if (ofst < b->maxbk)
      {
        // fill at end
       add_data_cmd (ofst,last, C_DFLT|C_SYS,0);
       add_code_cmd (ofst,b->maxpc,C_XCODE|C_SYS);       // and xcode to match
       b->maxpc = ofst;
      }
  }

/*
void setflgbnk (int ix, int bk)
{
 BANK *b;
 b = bkmap + ix;
 if (bk == 0) b->b0 = 1; else b->b0 = 0;
 if (bk == 1) b->b1 = 1; else b->b1 = 0;
 if (bk == 8) b->b8 = 1; else b->b8 = 0;
 if (bk == 9) b->b9 = 1; else b->b9 = 0;
}*/

void mkbanks(int num, ...)
{
 va_list ixs;
 int i, j, k;
 BANK *b;

 va_start(ixs,num);

 //for (i = 2; i < 2+num; i++) bkmap[i].dbnk = va_arg(ixs,int); original
 // change to allow for a deleted bank
 j = 0;
 for (i = 2; i < 6; i++)
    { //if (bkmap[i].btype)
      {
       k = va_arg(ixs,int);
       b = bkmap + i;
       b->dbank = k;

   //    setflgbnk(i,b);
  //     if (b == 0) bkmap[i].b0 = 1; else bkmap[i].b0 = 0;
  //     if (b == 1) bkmap[i].b1 = 1; else bkmap[i].b1 = 0;
  //     if (b == 8) bkmap[i].b8 = 1; else bkmap[i].b8 = 0;
  //     if (b == 9) bkmap[i].b9 = 1; else bkmap[i].b9 = 0;

 //      dbnk = va_arg(ixs,int);
       j++;

       # ifdef XDBGX
         DBGPRT(1,"Set temp bank %d to bank %d",i, k);     //kmap[i].dbnk);
       #endif

      }
     if (j >= num) break;
    }

 va_end(ixs);

}


void set_banks(void)
{
 int i;
 BANK *b;

  codbnk = 0x80000;                         // code start always bank 8
  rambnk = 0;                               // start at zero for RAM locations
  if (numbanks) datbnk = 0x10000;           // data bank default to 1 for multibanks
  else   datbnk = 0x80000;                  // databank 8 for single

  for (i = 0; i < BMAX; i++)
     {
         // maxpc used as flag to check if user entered....
      b = bkmap + i;
      if (b->bok && !b->maxpc) find_filler(b);
     }


  if (P8065)
     {
      minwreg = 0x1f;    // min allowed update RAM - allow stack updates
      maxwreg = 0x3ff;   // max register
      stkreg1 = 0x20;    // stack
      stkreg2 = 0x22;    // altstack

     }
  else
     {
      minwreg = 0xf;    // same for 8061, no altstack
      maxwreg = 0xff;
      stkreg1 = 0x10;
      stkreg2 = 0;
     }
 }














uint set_bnk (CPS *c)
{
  int ix, bank, xx;
  BANK *b;

   // bank no , fileoffset, endfileoffset, maxpc (optional);

   // use bk[2].cmd as marker to indicate bank command used already for
   // following bank commands.

   // validate all params before overwriting anything

   if (c->npars < 2) return do_error(c,"\001need two\011");

   ix = c->p[0];          // first param
   if (!check_bank(ix)) return do_error(c,"\001\003 Bank Number");
   if (bkmap[ix].cmd)   return do_error(c,"\001Bank already defined");

   bank = ix << 16;          // bank in correct form

   // file offset can be negative for missing bytes
   if (c->p[1] < -10 || c->p[1] > (int)fldata.fillen) return do_error(c,"\001\003 File Offset");

   if (ix > 0 && bkmap[ix-1].bok)
      {
       if (c->p[1] <= bkmap[ix-1].filend) return do_error(c,"\001File offsets\006");
      }

   if (c->npars > 2)
     {     // end file offset specified
      if (c->p[2] <= c->p[1])             return do_error(c,"\001End < start");
      if (c->p[2] - c->p[1] > 0xdfff)     return do_error(c,"\001Start -> End gap too large");
      if (c->p[1] >= (int) fldata.fillen) return do_error(c,"\001Start > File Size");
      if (c->p[2] >= (int) fldata.fillen)
         {
          c->p[2] = fldata.fillen-1;
          do_error(c,"\002Bank end > File size, reduced to match");
         }
     }
    else
     {
      c->p[2] = c->p[1] + 0xdfff;     // default to max bank size
      if (c->p[2] >= (int) fldata.fillen) c->p[2] = fldata.fillen-1;
     }

  if (c->npars > 3)
  {        // filstart specified
    c->p[3] = nobank(c->p[3]) | bank;
    xx = c->p[2] - c->p[1] + PCORG;    // max pc
    xx |= bank;

    if (c->p[3] < PCORG || c->p[3] > xx ) do_error(c,"\002Fill start address\003 - ignored");
  }

   if (!bkmap[2].cmd)
      {    // clear whole map, except entry 16, once.
       memset(bkmap,0,sizeof(bkmap)- sizeof(BANK));     // leave entry 16 (RAM)
       bkmap[0].opbt = opbit;                   // bit flag array reset
       bkmap[1].opbt = opbit + 0x800;           // = 32 bit   0x1000 = 16 bit ints
       bkmap[8].opbt = opbit + 0x1000;
       bkmap[9].opbt = opbit + 0x1800;
       numbanks = -1;                           // numbanks 0 is single bank
       bkmap[2].cmd = 1;
      }

   b = bkmap+ix;
   b->cmd = 1;
   b->bok = 1;
   b->filstrt = c->p[1];
   b->fbuf  = ibuf + b->filstrt;       // ibuf + file offset - allows for PCORG
   b->filend =  c->p[2];
   b->minpc = PCORG | bank;
   b->maxbk = (b->filend-b->filstrt + PCORG) | bank;

   if (c->p[3]) b->maxpc = nobank(c->p[3]) | bank;

   numbanks++;

   set_banks();         // sets filler(maxpc) if not already done

   #ifdef XDBGX
     DBGPRT(0,"Bank Set %d %x %x ",c->p[1], c->p[2],c->p[3]);
     DBGPRT(1," #( = %05x - %05x fill from %05x)",b->minpc, b->maxbk, b->maxpc);
   #endif
   return 0;
 }


uint set_vect (CPS *c)
{                          // assumes vect subrs in same bank as pointers
  uint  ofst,bank;
  int i;
  LBK *blk;
  ADT *a;

  if (chk_csize(c))   return 1;

  blk = add_data_cmd (c->p[0],c->p[1], C_VECT|C_CMD,0);   // by cmd

  if (PMANL) return 0;
  if (chdata.lasterr)  return do_error(c,etxts[chdata.lasterr]);

  if (c->levels == 0)
   {
     a = cmdadnl;             // check addn entry
     bank = g_bank(a->data);
 //    name = a->name;
     if (!bkmap[a->bank].bok) return do_error(c,"\001Bank\003");
     cpy_adt(c,blk);
   }
  else bank = g_bank(c->p[0]);

  for (i = c->p[0]; i < c->p[1]; i += 2)
    {
      ofst = g_word (i);
      ofst |= g_bank(bank);
      add_scan (ofst, J_SUB|C_SYS,0);      // adds subr
      
    }
  return 0;
}

uint set_code (CPS *c)
{

  add_code_cmd (c->p[0], c->p[1], c->fcom|C_CMD);   // by cmd
  if (chcode.lasterr) do_error(c,etxts[chcode.lasterr]);
  return 0;
}

uint set_scan (CPS *c)
{
  add_scan (c->p[0], J_STAT|C_CMD,0);
  if (chscan.lasterr) do_error(c,etxts[chscan.lasterr]);
 return 0;
}


uint set_args(CPS *c)
{
  LBK *blk;

 if (chk_csize(c)) return 1;

  blk = add_data_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // by cmd

  if (chdata.lasterr)  return  do_error(c,etxts[chdata.lasterr]);

  // must clear all fieldwidths for args command

  cpy_adt(c,blk);
  set_data_vect(blk);

  blk->size = totsize(blk);

  #ifdef XDBGX
  DBGPRT(1,0);
  #endif
return 0;
}

uint set_cdih (CPS *c)
{
    // xcode commands
  add_code_cmd(c->p[0],c->p[1],c->fcom|C_CMD);
  if (chcode.lasterr) do_error(c,etxts[chcode.lasterr]);
  return 0;
}

uint psw_set (CPS *c)
{
    // psw setter p[0] checked, but not p[1]
   if (!numbanks && (c->p[1] <= 0xffff && c->p[1] >= PCORG)) c->p[1] |= 0x80000;
  add_psw(c->p[0],c->p[1]);
  if (chpsw.lasterr) do_error(c,etxts[chpsw.lasterr]);
  return 0;
}






 void do_listing (void)
{
  uint ofst;
  int i, lastcom;
  LBK *x;
  BANK *b;
  anlpass = ANLPRT;                   // safety

  newl(1);
  fprintf (fldata.fl[1], "####################################################################################################");
  newl(1);
  fprintf (fldata.fl[1],"#\n# Disassembly listing of binary '%s' ",fldata.bare);
  if (P8065) pstr("(8065, %d banks)",numbanks+1 ); else pstr("(8061, 1 bank)");
  fprintf (fldata.fl[1], " by SAD Version %s (%s)", SADVERSION, SADDATE);
  fprintf (fldata.fl[1],"\n# See '%s%s' for warnings, results and other information", fldata.bare, fd[2].suffix);
  fprintf (fldata.fl[1],"\n\n# Explanation of extra flags and formats - ");
  fprintf (fldata.fl[1],"\n#   R general register. Extra prefix letters shown for mixed size opcodes (e.g DIVW)");
  fprintf (fldata.fl[1],"\n#   l=long (4 bytes), w=word (2 bytes), y=byte, s=signed. Unsigned is default.");
  fprintf (fldata.fl[1],"\n#   [ ]=use value as an address   (addresses are always word) ");
  fprintf (fldata.fl[1],"\n#   @=value is an address (to symbol),  '++' increment register after operation." );
  fprintf (fldata.fl[1],"\n# Processor status flags (for conditional jumps)" );
  fprintf (fldata.fl[1],"\n#   CY=carry, STC=sticky, OVF=overflow, OVT=overflow trap\n");
#ifdef  XDBGX
fprintf (fldata.fl[1],"\n# - - - -  DEBUG  - - - - - ");   
#endif
  fprintf (fldata.fl[1],"#\n####################################################################################################");
  newl(2);


 for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;

     if (!b->bok) continue;

     if (numbanks)
      {
       newl(2);
       pstr ("########################################################################");
       newl(1);
       pstr ("# Bank %d  file offset %x-%x, (%x - %x)", i, b->filstrt, b->filend, b->minpc, b->maxbk);
       newl(1);
       pstr ("########################################################################");
       newl(2);
      }

     indent = 0;
     lastcom = 0;

     for (ofst = b->minpc; ofst < b->maxbk; )
      {
         // first, find command for this offset

       x = find_prt_cmd(&prtblk, b, ofst);

       // add newline if changing block print types
       if (dirs[x->fcom].prtcmd != dirs[lastcom].prtcmd || dirs[x->fcom].prtcmd == pp_stct) {pp_comment(ofst,1); indent = 0;}
       lastcom = x->fcom;

       while (ofst <= x->end)
         {
          show_prog (anlpass);
          pp_comment(ofst,0);
          ofst = dirs[x->fcom].prtcmd (ofst, x);
         }
       }      // ofst loop

     pp_comment(b->maxbk+1,0);             // any trailing comments at bank or listing end
    }        // bank
}



void scan_all (void)
{
  // check whole scan chain for unscanned items
  // a scan may add new entries so track lowest added scan block
  SBK *s, *saved;
  uint ix;

  ix = 0;

  while (ix < chscan.num)
   {
    s = (SBK*) chscan.ptrs[ix];
    xscans++;

       if (!s->stop)       // && !s->caller)            //experiment..NO........
         {
   //       if (anlpass >= ANLPRT) newl(1);            // new line for new block print
          saved = s->caller;
          s->caller = NULL;                          // set caller zero as safety for emulate
          recurse = 0;                               // reset recurse level
          #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(0,"Chain Scan");
          #endif
          scan_blk(s, &cinst);
          s->chscan = 1;
          s->caller = saved;                          // and restore caller

          if (tscans > chscan.num * 32)               // loop safety check
           {
            wnprt(2,"*********");
            wnprt(1," Abandon - too many scans - a loop somewhere ?");
            wnprt(1,"*********");
            printf("FAILED - too many scan loops\n");
            break;
           }

        }         // end if not scanned

      if (lowscan <= ix)  ix = lowscan;            // recheck from lowest new scan inserted
      else  ix++;
      lowscan = chscan.num;                       // reset to highest value for next scan
   }

 }






/*
void scan_data_gaps(void)
{
  int ix, sx, ofst, ndata, zz;
  SBK *s;
  LBK *d, *n;

  // look for data words or bytes with gaps after them
  // still experimental


  ndata = chdata.num;
  for (ix = 0; ix < ndata-1; ix++)
   {
    d = (LBK*) chdata.ptrs[ix];
    n = (LBK*) chdata.ptrs[ix+1];  // next data block

    if (nobank(d->start) <= 0x201e) continue;

    ofst = d->end+1;      // ofst after end of THIS scan

    if (ofst < n->start)
     {       // GAP, could be code or data in here..............
   //   if ((ofst & 1) && g_byte(ofst) == 0xff) ofst++;   // filler ??

         s = (SBK*) chmem(&chscan);
         s->start = ofst;
         sx = bfindix(&chscan, s, 0);

         if (sx < chscan.num)
          {
           s = (SBK *) chscan.ptrs[sx];
           if (s->start > ofst)
           {        // potential gap to investigate d->start (or end)->ofst and beyond


      //     if (d->start > n->start)
      //      {
                // code gap with nothing else in it.
                // see if overlapping data access first though
                // need more here as index may actually work for a data item
                // OR , go through data chain and look for code overlapped ones ??

            //    d = (LBK*) chmem(&chdata);
            //    d->start = ofst-10;        // for a forward index
            //    dx = bfindix(&chdata, d, 0);
            //    d = (LBK *) chdata.ptrs[dx];
            //    if (d->start < ofst)
            //    {
             //       DBGPRT(1,"z"); // may be indexed data
           //     }

                // try a scan.

                // need some way to do jumps and calls in here !!!!
                zz = s->start;
                if (n->start < zz)  zz = n->start;

                 #ifdef XDBGX
                DBGPRT(1,"******************************************* Data GAP s %x-%x (opc %d off %x)", d->start, zz-1, d->opcsub, d->soff); // not if found something before it.
                #endif

      //          scan_sgap(s->nextaddr+1);
            }
else
{  // NOT found scan block

    //  #ifdef XDBGX
     //           DBGPRT(1,"******************************************* Data GAP d %x-%x", d->start, n->start-1); // not if found something before it.
     //           #endif
}

          }
     }

   }

  } // end func
*/




// ********** command processing section **************


void add_iname(int from, int ofst)
{
  // add interrupt autonames
  uint i, x;
  char *z;

  // ignore_int can be at the end of a jump in multibanks....

  if (!PINTF) return;
  from &= 0xffff;            // drop bank
  from -= 0x2010;
  from /= 2;                 // correct index for table lookup

  x  = (P8065) ? 1 : 0;     // 8061 or 8065
  z = nm;

  if (numbanks)               // sort out bank number
   {
    i = ofst >> 16;             // add bank number after the I
    z += sprintf(nm, "I%u_",i);
   }
 else
    z += sprintf(nm, "I_");         // no bank number

 if (find_sig(&intign, ofst))       // this is 'ignore int' signature
    {
      sprintf(z,"Ignore");
      add_sym(C_SYS, nm, ofst,-1,0,0);

      return;
    }

 for (i = 0; i < NC(inames); i++)
    {
    if (from <= inames[i].end && from >= inames[i].start && inames[i].p5 == x)
       {
       z += sprintf(z,"%s",inames[i].name);                      // number flag set
       if (inames[i].num) z+=sprintf(z, "%u", from-inames[i].start);
       add_sym(C_SYS, nm, ofst,-1,0,0);
       break;    // stop loop once found for single calls
       }
    }
return;
 }



/*******************command stuff ***************************************************/

int readpunc(CPS *c, int uc)
 {
  // read punctuation between commands and numbers etc.
  // keeps track of position (for errors)
  // changes lowercase to uppercase if uc set
  while (c->posn <= c->maxlen && (flbuf[c->posn] == ' ' || flbuf[c->posn] == ',')) c->posn++;
  if (c->posn >= c->maxlen) return 0;            // hit end of line

  if (!uc) return 1;
  
  //lower case fix
  if (flbuf[c->posn] >= 'a' && flbuf[c->posn] <= 'z') flbuf[c->posn] -= 0x20;           // A is 0x41 a is 0x61  
  return 1;
 }

int chkpunc(CPS *c, int l)
 {
  // check end letter of numbers to make sure this isn't
  // a string which happens to begin a-f

  char *x;

  x = flbuf+c->posn+l;

  if (*x == ' ') return 0;
  if (*x == ',') return 0;
  if (*x == ':') return 0;
  if (*x == '|') return 0;
  if (c->posn+l >= c->maxlen)  return 0;            // hit end of line
  return 1;
 }







int getpx(CPS *c, int ix)
 {
  // ix is where to start in p params array,
  // for max of 8-ix parameters
  // answer is params read OK

  int ans, rlen;
  uint v;

  ans = 0;

  while(ix < 8)
   {
     if (sscanf(flbuf+c->posn, "%5x%n", &v, &rlen) > 0)
       {
        if (chkpunc(c, rlen)) break;
        c->p[ix] = v; 
        c->posn += rlen;
        ans++;
        }
     else break;
    readpunc(c,1);
    ix++;
   }

  return ans;
 }


int getpd(CPS *c, int ix)
 {
  // ix is where to start in p params array, 2 max decimal pars
  // answer is params read - only ever 1 param ??
   int ans, rlen;

  ans = 0;

  while(ix < 8)
   {
     if (sscanf(flbuf+c->posn, "%d%n", c->p+ix, &rlen) > 0)
       {
        if (chkpunc(c, rlen)) break;
        c->posn += rlen;
        ans++;
        }
     else break;
    readpunc(c,1);
    ix++;
   }

  return ans;
 }



int getstr(CPS *c, const char **s, int size)
 {
     // get string max of 12 chars at the moment, may use for opts later
     // only ever 1 param ??
  int ans, rlen, j;

  ans = sscanf (flbuf+c->posn, "%64s%n", nm, &rlen);    // n is no of chars read, to %n

  if (ans <= 0) return ans;

  ans = -2;         // string not found.


  for (j = 0; j < size; j++)
   {
    if (!strncasecmp (*(s+j), nm, 3) )      // 3 chars min match
      {
       ans = j;                 // found
       break;
      }
   }

   if (ans >= 0) c->posn += rlen;
   return ans;
 }



int chk_cmdaddrs(CPS *c, DIRS* d)
 {
   // check ans fixup any address issues (bank etc)
   // can have a pair (start-end) and/or a single start (e.g. rbase has both)

  int start, end;

  if (d->sapos < d->maxpars)
    {  // single address, with or without a start/end pair following
    start = c->p[d->sapos];
    if (!numbanks && (start <= 0xffff && start >= PCORG)) start |= 0x80000;
    c->p[d->sapos] = start;
   }

 if (d->appos < d->maxpars)
    {        // address pair
     start = c->p[d->appos];
     end   = c->p[d->appos+1];

     if (!numbanks && (start <= 0xffff && start >= PCORG)) start |= 0x80000;

     if (!end) end = start;
     if (end <= 0xffff) end |= g_bank(start);      // add start bank
     if (!bankeq(start,end)) return do_error(c,"\001Banks don't match");
     if (end < start)  return do_error(c,"\001End < Start");

     c->p[d->appos] = start;
     c->p[d->appos+1]   = end;
    }


 return 0;
 }



int do_adnl_letter (CPS* c)
{
   // switch by command letter - moved out of parse loop for clarity
   ADT *a;
   DIRS *d;
   int ans, rlen;
   float f1;
   char optl;

   d =  dirs + c->fcom;

   ans = sscanf(flbuf+c->posn,d->opts, &optl, &rlen);       // only one char at a time
   if (ans <= 0) return do_error(c,"\001\003 Option");     // illegal option char (4)
   if (ans) c->posn += 1;                                   // only a single char here

   if (c->levels < 0 && optl != ':') return do_error(c,"\001Colon\007");

   if (c->levels >= 0) a = cmdadnl + c->levels; // not for -1 (start point)

   switch(optl)
     {
           // case 'A':                    // AD voltage ?? / RPM/ IOtime....
           //    cadd->volt = 1;
           //    break;

// B with T
           // case 'C' :
           //    break;

       case '|' :                    // split printout for large structs, not for argl

        if (!c->argl) a->newl = 1;                  // mark split printout here (end of block)

         // fall through

       case ':' :


// need max level check !!!! or change to get blocks from chain but would need to delete and rechain
// with new FK.....

// NB. pp-lev etc uses ->cnt so set cnt = 0, ssize = 1 for hidden entries

         // Add a new ADT level
         if (!readpunc(c,1)) break;   // not if a trailing colon (or new line marker)
         c->levels++;
         if (c->levels > 31) return do_error(c,"\001Too Many data items");
         a = cmdadnl+c->levels;
         a->seq = c->levels;
         a->dcm = d->decdft;                 // default o/P
         cmdadnl[c->levels+1].seq = -1;      // safety marker
         break;

       case '=' :       // an ANSWER definition. 

         ans = getpx(c,4);
         if (!ans) return do_error(c,"\001Address\004");

         a->ans = 1;
         if (!a->cnt)   a->cnt = 1;
         if (!a->ssize) a->ssize = 1;           // default
         a->data  = c->p[4];    // item + fixed address offset (funcs) -  bank?
         break;

       case 'D' :

         // 1 hex value (address offset with bank)
         ans = getpx(c,4);
         if (!ans) return do_error(c,"\001Address\004");
         a->foff = 1;
         a->data  = c->p[4];    // item + fixed address offset (funcs) -  bank?
         if (!a->pfw) a->pfw = cafw[2];     //word sized for offsests
         break;

       case 'E' :

         // ENCODED - always size 2
         ans = getpx(c,4);
         if (ans != 2 )  return do_error(c,"\001 %d values\004",2);

         a->enc   = c->p[4];
         a->data  = c->p[5];
         a->ssize = 2;            // encoded implies word
         if (!a->pfw) a->pfw = cafw[2];
         if (!a->cnt) a->cnt   = 1;
         break;

       case 'F' :

         // flags for symbol

        if (c->fcom == C_SYM)
         {             // flags word/byte for SYM.
           a->enc = 7;                // mark it
           a->data = 0x100;           // for just a single 'F'
           break;
         }
        break;

    

  // G H I J

       case 'I':               // immediate fixup for syms
        if (c->fcom == C_SYM)
         {             
          c->argl = 1;            // reuse flag
         }
         else do_error(c,"\001Option\010");
         
         break;


       case 'L':               // long int (= double)
         a->ssize &= 4;        // keep sign
         a->ssize |= 3;
         if (!a->cnt) a->cnt = 1;
         if (!a->pfw) a->pfw = cafw[a->ssize];
         break;

 // case 'M':                // mask pair ?, use esize to decide word or byte
 //    cadd->mask = 1;
 //    break;

       case 'N':
         a->fnam = 1;       // Name lookup
         break;

       case 'O':                // Cols or count - 1 decimal value

         ans = getpd(c, 4);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, "\002 Repeat value\007, 1 assumed");
           }  
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c,"\001 Repeat value\003 (1-32)");

         a->cnt = c->p[4];
         break;

       case 'P':               // Print fieldwidth   1 decimal
         ans = getpd(c, 4);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, "\002 Filed Width\007, 1 assumed");
           }  
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c,"\001 Field width\003 (1-32)");

         a->pfw = c->p[4];
         break;       
         
    /*   case 'Q' :              // terminator byte MOVED TO GLOBAL

         // read optional number of bytes max 3.
         ans = getpd(c, 4);
         if (!ans) c->term = 1;
         else
          {
            if (c->p[4] < 1 || c->p[4] > 3) return do_error(c,2,"Invalid number");
            c->term = c->p[4];
          }
         break;           */

       case 'R':               // indirect pointer to subroutine (= vect)
         a->ssize = 2;         // safety, lock to WORD
         a->vaddr = 1;
     //    a->name  = 1;          // with name by default  NOT DEFAULT
         if (!a->cnt) a->cnt   = 1;

         if (!a->pfw) a->pfw = cafw[a->ssize];
         break;

       case 'K' :
         // 1 decimal value (bank, 0,1,8,9)
         ans = getpd(c, 4);
         if (!ans) return do_error(c,"\001Bank number\004");

         if (!check_bank(c->p[4])) return do_error(c,"\001Bank Number\003");
         a->bank = 1;
         a->data  = c->p[4];       // Temp bank holder
         if (!a->cnt) a->cnt   = 1;     // but ssize stays at zero
         break;

       case 'S':              // Signed
          a->ssize |= 4;
          break;

       case 'T':                 // biT
       case 'B':                 // Bit    (future ??)
         // read one decimal val  T only allowed in sym and timer commands
         ans = getpd(c, 4);
         if (!ans) return do_error(c,"\001Bit Number\004");
         if (c->p[4] < 0 || c->p[4] > 15) return do_error(c,"\001Bit Number\003");
         a->data = c->p[4];
         a->enc = 7;           // temp marker for  bitno
         break;

       case  'U' :               //unsigned (default so optional)
         a->ssize &= 3;         // clear sign
         break;

       case 'V':                  // diVisor
         // read one FLOAT val
         ans = sscanf(flbuf+c->posn, "%f%n ", &f1, &rlen);       // FLOAT
         if (ans <=0) return do_error(c,"\001Divsior value\004");
         c->posn+=rlen;
         a->fdat = f1;                              // Full FLOAT variable
         break;

       case 'W':                  // Word   - or write for syms
         a->ssize &= 4;           // keep sign
         a->ssize |= 2;
         if (!a->cnt) a->cnt   = 1;
         if (!a->pfw) a->pfw = cafw[a->ssize];
         break;

       case 'X':                  // Print in decimal /
         a->dcm ^= 1;             // swop over when specified
         break;

       case 'Y':                  // bYte
         a->ssize &= 4;           // keep sign, but ditch rest
         a->ssize |= 1;
         if (!a->cnt) a->cnt   = 1;
         if (!a->pfw) a->pfw = cafw[a->ssize];
         break;

         // Z

       default:
           return do_error(c,"\001\003Option");

    }          // end switch
 return 0;
    }


int do_global_opts(CPS *c)
{
 
   DIRS *d;
   int ans, rlen;
   char optl;   
   
   d =  dirs + c->fcom;   
   if (!d->gopts)  return 0;

   ans = 1;
     if (!readpunc(c,0)) return 0;
     
   ans = sscanf(flbuf+c->posn,"%1[$:]", &optl);      // only a $ to start with
   if (ans <= 0 || optl == ':') return 0;                   // no global options
   c->posn += 1;                                   // only a single char here

 
   while (ans)
    { 

     if (!readpunc(c,0)) break;
     ans = sscanf(flbuf+c->posn,d->gopts, &optl, &rlen);     // nm NOT  only one char at a time
     if (ans < 0) return do_error(c,"\003Option");           // illegal option char (4)
     if (ans) c->posn += 1;                                  // Only single char here
     if (!ans) break;
     
     switch(optl)
     {
       case 'Q' :              // terminator byte

         // read optional number of bytes max 3.
         ans = getpd(c, 4);
         if (!ans) c->term = 1;
         else
          {
            if (c->p[4] < 1 || c->p[4] > 3) return do_error(c,"\001Size\003 (1-3)");
            c->term = c->p[4];
          }
         break;
      
       case  'A' :
         c->argl = 1;        // set args layout
         break;
         
       case  'C' :
         c->cptl = 1;        // set compact layout
         break;


       case  'F' :
        // f <str> <reg1> <reg2>    func add/ tab add, cols

         ans = getstr(c,spfstrs, NC(spfstrs));
         if (!ans)  return do_error(c,"\001Subr type reqd");
         c->spf = ans + 4;
         ans = getpx(c,5);

         // read colons in here and cheat ???

         if (c->spf < 13) 
          {
           if (ans < 1)   return do_error(c,"\001 At least %d values\004",1);
           if (ans > 1)   do_error(c,"002Extra Values\010");
          } 
         else
          {   
           if (ans < 2)   return do_error(c,"\001 At least %d values\004",2);  
           if (ans > 2)   do_error(c,"002Extra Values\010");
          }  

         c->adreg = c->p[5];                  // struct address register
         c->szreg = c->p[6];                  // tab column register
         break;
      
       default:
         do_error(c,"\001\003Option");
         break;
     }
     optl = 0;
    }  
   return ans;
}
    
    
int do_opt_letter (CPS* c)
{
    // this is a "cmd: ABCD " type (no params)

  uint j,f;

  DIRS *d;
  char optl;
  int ans, rlen;

  d =  dirs + c->fcom;
  f = 0;

  ans = sscanf(flbuf+c->posn,d->opts, &optl, &rlen);       // only one char at a time

  if (ans <= 0) return do_error(c,"\001\003Option");      // illegal option char (4)
  c->posn += 1;                                           // Not rlen, only single char here

  if (optl >= 'A' && optl <= 'Z')
    {    //ignore colons etc,  then must find letter in opts array.
       for (j = 0; j < NC(opts); j++)
       {
         if (opts[j].key == optl) c->p[0] |= opts[j].copt;
         f = 1;
       }
     if (!f) do_error(c,"Unknown Option!!");                //should not happen
    }
  return 0;
 }

int do_opt_str (CPS* c)
{
    int ans,sz;
    char *t;

    c->p[0] = 0;    
    ans = 1;
    sz = NC(optstrs);   //this stops stupid unsigned comparison error

   // if (!readpunc(c)) break;   // not if a trailing colon (or new line marker) 
   while (ans)
    { 
      if (!readpunc(c,0)) break;
      t = flbuf+c->posn;  
      if (*t == ':') c->posn++;
   //   if (!readpunc(c,0)) break;
      
      ans = getstr(c,optstrs, sz);
             
      if (ans < -1 || ans >= sz) do_error(c,"\001\003Option");       // not found
      
      if (ans >= 0)
         {  // a valid option 
           c->p[0] |= opts[ans].copt;
         } 
    }     
return 0;
      
}    

int parse_com(CPS *c, char *flbuf)
{
  // parse (and check) cmnd return error(s) as bit flags
  // return 1 for next line (even if error) and 0 to stop (end of file)

  int rlen, ans;
  char *t, *e;                        // where we are up to
  DIRS* d;

  memset(c, 0, sizeof(CPS));          // clear entire struct
  memset(cmdadnl,0,sizeof(cmdadnl));
 
  c->levels = -1;                     // no additional yet

  if (!fgets (flbuf, 255, fldata.fl[3])) return 0; 
 
  c->maxlen = strlen(flbuf);          // safety check

  e = flbuf + c->maxlen;              // end of input line

  t = strchr(flbuf, '\n');            // check for LF
  if (t && t < e) e = t;              // mark it

  t = strchr(flbuf, '\r');            // check for CR
  if (t && t < e) e = t;              // mark it

  t = strchr(flbuf, '#');             // check for comment
  if (t && t < e) e = t;              // mark it

  c->maxlen = e - flbuf;              // shorten line as reqd

  if (c->maxlen == 0) return 1;        // this is a null line;

  c->posn = 0;

  if (!readpunc(c,0)) return 1;         // null line after punc removed

  ans = getstr(c,cmds, NC(cmds));     // string match against cmds array
  if (ans < 0)  return do_error(c,"\001\004 Command");             // cmnd not found

  d = dirs+ans;
  c->fcom = ans;
  readpunc(c,0);

  if (d->maxpars)       // read up to 4 following addresses into p[0] (if any)
    {
     c->npars = getpx(c,0);

     if (c->npars < 1) return do_error(c,"\001\011\004");        // params reqd
     if (c->npars > d->maxpars) do_error(c,"\002Extra\011\010"); // extra pars ignored (continue)

      // verify addresses, single and/or start-end pair
      // but not whther valid for data or code
     ans = chk_cmdaddrs(c, d);
     if (c->error) return 1;
    }

  // read name here, if allowed and present

  readpunc(c,0);

  if (d->namex && c->posn < c->maxlen &&  flbuf[c->posn] == '\"')
   {
    c->posn++;   // skip quote
    ans = sscanf(flbuf+c->posn," %65s%n",c->symname, &rlen);     //max 64 char names.

    if (rlen > 63)
      {       // truncate with warning, but keep rlen to what was actually read
          do_error(c,"\002 Name truncated to %d chars",63);
          c->symname[64] = '\0';
      }

    if (ans > 0) c->posn += rlen;

    t = strchr(c->symname, '\"');                 // drop trailing quote from symname
    if (t) *t = 0;
    else return do_error(c,"\001\007 close quote");  // missing quote
   }

  // read global opts here ???  Separate 'global' string in dirs ???

  do_global_opts(c);


  // now read levels


  if (d->opts)
   {        // if no opts, skip this
     // read option chars and level markers whilst data left
     // from here, all lower case should converted to upper ?
      while (c->posn < c->maxlen)
       {
        if (!readpunc(c,1)) break;         // safety check

        if (!d->maxpars)
        {  
          if (*d->opts == 1) ans = do_opt_str(c);
           else   ans = do_opt_letter (c);                 // cmd : ABC  style
        }  
        else 
        {
            ans = do_adnl_letter (c);                // adnl style
        }
        if (c->error) return 1;
       }      //  end get more options (colon levels)
   }  // end if options allowed

 // any further error reporting is responsibility of handler command

 c->tsize = 0;

 for (rlen = 0; rlen <= c->levels; rlen++) c->tsize += cellsize(cmdadnl+rlen);

 d->setcmd (c);                  // do setup procedure

 if (d->namex && c->fcom != C_SYM && *c->symname)
 {  // default add sym name for cmds other than SYM
   SYM *s;
   s = add_sym(C_CMD,c->symname,c->p[0],-1, 0,0);
   if (chsym.lasterr == E_DUPL)
       {
         strncpy(nm,s->name, 63);  
         new_symname (s, c->symname); // change sym
         chsym.lasterr = 0;
         if (!s->sys) do_error(c,etxts[E_SYMNAM], nm);
         s->sys = 0;
         s->cmd = 1;        //clear system, set user cmd
         s->xnum = 0;      // ans any autonum
         if (chsym.lasterr)   return do_error(c,etxts[chsym.lasterr]);
       }
 }





 if (c->firsterr) wnprt(1,0);    // extra LF after any errors
 show_prog(0);
 return 1;                   // next line
}


void do_preset_syms(void)
 {
   uint i, bitno;
   DFSYM *z;

   for (i = 0; i < NC(defsyms); i++)
     {
      z = defsyms+i;
      if (((P8065) && z->p85) || (!(P8065) && z->p81))
         {
          if (z->bit) bitno = z->bitno; else bitno =-1;
          add_sym(z->wrt|C_SYS,z->symname,z->addr,bitno,0,0);
         }
      }
  }



/***************************************************************
 *  read directives file
 ***************************************************************/
void getudir (void)
{
  int addr,addr2, i, j, bank;
  BANK *b;
 // ADT *a;
 // LBK *k;

  // cmdopts will be zero or 'H' here (before dir read)

  cmdopts |= PDEFLT;




  if (fldata.fl[3] == NULL)
    {
     wnprt(2,"# ----- No directive file. Set default options");
    }
  else
    {
      wnprt(2,"# Read commands from directive file at '%s'", fldata.fn[3]);

      while(parse_com(&cmnd, flbuf));
     
     //while (1)
     // {
     //  if (!parse_com(&cmnd, flbuf)) break;
     // }
    }

  do_preset_syms();

  if (PMANL) return;

  // now setup each bank's first scans and interrupt subr cmds

  for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;
     if (!b->bok) continue;

     #ifdef XDBGX
      DBGPRT(1,"--- auto setup for bank %d ---",i);
     #endif

     bank = i << 16;
     addr = b->minpc;
     addr |= bank;

     add_scan (addr,J_INIT|C_SYS,0);                  // inital scan at PCORG (cmd)

     j = (P8065) ? 0x205f : 0x201f;
     j |= bank;

     add_data_cmd (0x200a|bank, 0x200f|bank, C_WORD|C_SYS,0);     // from Ford handbook
     add_data_cmd (0x2010|bank, j, C_VECT|C_SYS,0);     // interrupt vects with cmd

     for (j -=1; j >= (0x200f|bank); j-=2)      // counting DOWN to 2010
        {
         addr2 = g_word (j);
         addr2 |= bank;
         add_iname(j,addr2);
         add_scan (addr2, J_SUB|C_SYS,0);      // interrupt handling subr

        }
     }

   #ifdef XDBGX
      DBGPRT(1," END auto setup");
     #endif

 }



void prt_subs (void)
{
  SUB *s;
  SYM *x;
  uint ix;

  wnprt(1,"# ------------ Subroutine list----------");

  for (ix = 0; ix < chsubr.num; ix++)
      {
        s = (SUB*) chsubr.ptrs[ix];

    //    wnprt(0,"sub  %x  ", s->start);
      wnprt(0,"sub  %x %x  ", s->start, s->end);
        x = get_sym(0, s->start,-1,0);
        if (x)
          {
           wnprt(0,"%c%s%c  ", '\"', x->name, '\"');
           x->noprt = 1;        // printed sym, don't repeat it
          }

        prt_glo(s, wnprt);
        prt_adt(&chadnl,s,0,wnprt);
        prt_adt(&chans ,s,0,wnprt);             // answer 
        if (s->cmd)  wnprt(0," #cmd");
        wnprt(1,0);
      }

   wnprt(2,0);
  }

/*
void DBG_subs (void)
{
  SUB *s;
  SYM *x;
  uint ix;

  DBGPRT(1,0);
  DBGPRT(1,"# ------------ Subroutine list");
  DBGPRT(1,"# num subs = %d", chsubr.num);

  for (ix = 0; ix < chsubr.num; ix++)
      {
        s = (SUB*) chsubr.ptrs[ix];

        DBGPRT(0,"sub  %x  ", s->start);
        x = get_sym(0, s->start,-1, 0);
        if (x) {DBGPRT(0,"%c%s%c  ", '\"', x->name, '\"');
               //  if (x->xnum > 1)   DBGPRT(0," N%d", x->xnum);
        }
        prt_glo(s, DBGPRT);
        prt_adt(s,0,DBGPRT);
        if (s->cmd)   DBGPRT(0,"# cmd");
        DBGPRT(1,0);
      }

   DBGPRT(2,0);
  }

*/












void prt_syms (void)
{
 SYM *t;
 uint ix, b;

 wnprt(1,"# ------------ Symbol list ----");

 for (ix = 0; ix < chsym.num; ix++)
   {
   t = (SYM*) chsym.ptrs[ix];

   if (!t->noprt)     // not printed as subr or struct
     {
     wnprt(0,"sym %x ", cmdaddr(t->addb >> 4));
     if (t->rstart)  wnprt (0," %x %x ", cmdaddr(t->rstart), cmdaddr(t->rend));
     wnprt(0," \"%s\"",t->name);

     b = t->addb & 0xf;
     if (b || t->writ || t->flags || t->immok) wnprt(0," :");

     //if (b) wnprt(0,"T %d" , (b-1));
     if (b) wnprt(0,"B %d" , (b-1));
     if (t->writ)  wnprt(0," W");
     if (t->flags) wnprt(0," F");
     if (t->immok) wnprt(0," I");
     if (t->sys) {wnpad(cposn[2]); wnprt(0,"     # autogenerated by SAD");}
     
     wnprt(1,0) ;
     }
   }
  wnprt(1,0);
 }


void prt_dirs()
{                 // print all directives for possible re-use to msg file
  prt_opts();
  prt_bnks();
  prt_rbt();
  prt_psw();
  prt_cmds(&chcode,wnprt);
  prt_cmds(&chdata, wnprt);
  prt_subs();
  prt_syms();
  
  // prt_scans ?
}

short init_structs(void)
 {

  ibuf  = (uchar*) mem(0,0, 0x42000);                   //[0x42000];              // 0-2000 ram, + 4 banks of 0-0xffff ROM
  opbit = (uint *) mem(0,0, 0x8000);                  //[0x2000]; ints             // opcode start flag - 4 banks of 32 flags

  memset(bkmap,0,sizeof(bkmap));
  bkmap[2].opbt = opbit;                   // bit flag array start
  bkmap[3].opbt = opbit + 0x800;           // = 32 bit   0x1000 = 16 bit ints
  bkmap[4].opbt = opbit + 0x1000;
  bkmap[5].opbt = opbit + 0x1800;

  memset(opbit,0, 0x8000);                 // clear all opcode markers
  memset(ibuf, 0, PCORG);                  // clear RAM and regs at front of ibuf
  anlpass  = 0;                            // number of passes done so far
  gcol     = 0;
  lastpad  = 0;
  cmdopts  = 0;

  #ifdef XDBGX
    DBGmaxalloc = 0;          // malloc tracking
    DBGcuralloc = 0;
    DBGmcalls   = 0;
    DBGmrels    = 0;
  #endif

  tscans    = 0;
  xscans    = 0;
  rambnk    = 0;

  return 0;
}


void openfiles(void)
{
  int i;

  fldata.error = 0;

  for (i = 0; i < 5; i++)
   {
    fldata.fl[i] = fopen(fldata.fn[i], fd[i].fpars);           // open precalc'ed names
    if(i < 3 && fldata.fl[i] == NULL) {fldata.error = i+1; return;}  // failed to open a file
   }

#ifdef XDBGX
  fldata.fl[6] = fopen(fldata.fn[6], fd[6].fpars);
#endif
}


void check_bank_intv (SIG *s, int addr, int ofs)
 {
    uint bank, taddr, x;
    int val;
    if (!s)  return;                     // safety
    taddr = 0;
    bank = g_bank(s->start);               // FAKE bank from sig
    if (nobank(s->end) >= 0x2010 || s->v[1] < (0x2000|bank))  return;

    // sig can't overlap interrupt vects (2010) and must have a jump
    // can use s->v[2] onwards as markers
    if (s->v[1] <= s->start+s->skp)  s->v[3] = 2;                          // loopstop    DATA (use xend?)
    if ( bank == 0x20000 && nobank(s->v[1]) > 0x201f)   s->v[3] = 1;       // jump over int regs - CODE.
    if ( bank >  0x20000 && nobank(s->v[1]) > 0x205f)   s->v[3] = 1;       // jump over int regs - CODE.

    // now check intr vects 8061 - counts into [4] and [5]
    for (x = (bank|0x2010);  x < (bank|0x201f);  x+=2)
      {
       taddr = g_word(x);             //pointer
       if (taddr <= 0xfff0 && taddr >= 0x2020) s->v[4]++;         // 8061 count
       else break;

       // check pointer value for return or pushp

       taddr |= bank;            // real address
       val = g_byte(taddr);

       if (val == 0xe7 || ( val >=0x20 && val <= 0x27))
         {
          // a jump - calc destination and check
          val = do_jumpsig (opcind[val], taddr);
          val = g_byte(val);
         }

       if (val >= 0xf0 && val <= 0xf2) s->v[5]++;     // ret, retei or pushp

        // could add rbase check here too......??
      }    // end vect check 8061 loop


    //8065 check [6] and [7]
    for (x = (bank|0x2010);  x < (bank|0x205f);  x+=2)
      {
       taddr = g_word(x);             //pointer
       if (taddr <= 0xfff0  && taddr >= 0x2060)  s->v[6]++;         // 8065 count
       else break;

       // check pointer value for return (ignore interrupt)
       taddr |= bank;            // real address
       val = g_byte(taddr);

       if (val == 0xe7 || ( val >=0x20 && val <= 0x27))
         {
          // a jump - calc destination and check
          val = do_jumpsig (opcind[val], taddr);
          val = g_byte(val);
         }

       if (val >= 0xf0 && val <= 0xf2) s->v[7]++;     // ret, retei or pushp
       // for 8065, can be a bank swop.  Check next byte is 0,1,8,9 with mask (and THEN a jump ??)
       if (val == 0x10)
         {
          val = g_byte(bank|(val+1));
          if (!(val & 0xf6)) s->v[7]++;        // mark as OK  (0,1,8,9)
         }

      }    // end vect check 8065 loop


    s->v[15] = addr;
    s->v[10] = ofs;               // but not right for -ves
    if (ofs < 0) s->v[9] = 1;     // mark negative
    s->v[13] = s->start;
    s->v[14] = s->end;
    taddr = chsig.num * 0x1000;
    s->start += taddr;                   // fiddle to stop overlaps
    s->end += taddr;
    inssig(s);              // sig jump matched, so keep it for now

}


int scan_start (int bankix, int mapaddr)
{
    // file is read in to ibuf [0x2000] onwards - look for start jump and interrupt vectors
    // new version - assemble temp sigs into chain before assessing. (should stop near misses)
    // CANNOT just scan whole file - will get far too many pssible matches and it will be slow.
    // try around defined known likely addresses. if this fails, file is probably trash anyway


 // mapaddr is fed in from last bank + 0xe0000 (+0x2000)

    BANK *b;
    SIG* s;
    uint max, addr, taddr, bank, val, pass, tix;
    int ofs;

    b = bkmap+bankix;
    bank = g_bank(b->minpc);      // FAKE TEMP bank from bkmap
    b->cbnk = 0;
    pass = 0;

    #ifdef XDBGX
     DBGPRT(1,"scan for bank %d start", bankix-1);
    #endif


    while (pass < 15)
     {
        // gives 0 to 32000 in 7 steps
       addr = mapaddr;        //-2;                  // allow ff fa missing
       if (pass & 1) addr += 0x2000;
       if (pass & 2) addr += 0xe000;      // was 10000
       if (pass & 4) addr += 0x20000;
       if (pass & 8) addr += 0x40000;

    //   if (max > fldata.fillen) max = fldata.fillen;
       if (addr >= fldata.fillen)
         {
          #ifdef XDBGX
            DBGPRT(1,"Stop - End of file (%x)", fldata.fillen);
          #endif
          break;
         }


       #ifdef XDBGX
          DBGPRT(1,"try%d <=> %x",pass, addr);
       #endif

       // keep all and then review best candidate...
       // to get this right, need to use PC of 2000-2003 (so that int vects are correct) and NOT addr in negative.
       // fbuf MUST point to 0x2000 whether or not it's a real fileaddr
       // missing bytes - fileaddr zero is say 2001 or 2002 - so move 0x2001 (2) for sig, addr, and set minpc to match
       // extra bytes   - fileaddr zero is say 1ffe or 1fff - move addr only and stick to 0x2000 for sig

       for (ofs = -2; ofs < 3;  ofs++)
        {
         b->fbuf = ibuf + addr + ofs;                   // this maps file address as virtual 0x2000 (inc. negative)
         if (ofs < 0)
            {
             s = find_sig(&hdr,(0x2000|bank)- ofs);          // hdr is pattern ref - but this is just a jump, really....
            }
         else
            {
             s = find_sig(&hdr,0x2000|bank);
            }
         check_bank_intv (s, addr, ofs);                // do scoring and checks in signature x is v[10} ??
        }
     pass++;
   }             // end of for pass

// available for totals
  val = 0;
  addr = 0;

  max = 0;
  taddr = 0;

  // look for highest valid int count first....then vect count. [4] & 5 for 8061, 6 & 7 8065

  for (tix = 0; tix < chsig.num; tix++)
    {
      s = (SIG*) chsig.ptrs[tix];

      // interrupt vect cnt MUST be all OK otherwise not correct binary
      if (s->v[3])
       {
         if (s->v[6] == 40 && s->v[7] > val)
          {
           val = s->v[7]; // 8065
           addr = tix;
          }

         if (s->v[4] == 8 && s->v[5] > max)
          {
           max = s->v[5];   // 8061
           taddr = tix;
          }
       }
    }

  s = 0;

  if (val > 0)
    {
     // highest 8065 found
     s = (SIG*) chsig.ptrs[addr];
     b->P65 = 1;                             // set 8065
    }
  else
    {
     if (max > 0)
       {
        s = (SIG*) chsig.ptrs[taddr];
        b->P65 = 0;                         // set 8061
       }
    }

  if (s)
    {
      b->bok = 1;                          // found a bank start
      if (s->v[3] < 2) b->cbnk = 1;

      if (s->v[10] < 0)
         { // missing bytes (from x)   CAN NEVER BE TRUE ????
          s->v[15] += s->v[10];
          b->minpc -= s->v[10];
         }

      if (s->v[10] > 0)
         {
          // extra bytes
          s->v[15] += s->v[10];
         }

      b->filstrt = s->v[15];                       // actual bank start (buffer offset) for 0x2000
      b->fbuf = ibuf + b->filstrt;                 // this maps file address as virtual 0x2000
      addr = s->v[15];
    }
  else b->bok = 0;




#ifdef XDBGX
   DBG_sigs(&chsig);
#endif
   clearchain(&chsig);                    // clear out any sigs so far (otheriwse get dupl !!)

   return addr;

 }



void find_banks(void)
 {
  // file is in ibuf[0x2000] onwards
  // set TEMP banks [2]-[5] as these are not used in main decode

  uint faddr, i, bank;
  BANK *b;

  lastbmap = bkmap;                            // for later on...bank 0
  numbanks = -1;

  i = 2;                                       // use 2,3,4,5 as temp holders
  faddr = 0;                                   // fileadd [0] (pc = 0x2000 equiv)

  while (faddr < fldata.fillen && i < 6)       // 4 banks max (for now)
    {
     bank = i << 16;
     b = bkmap+i;
     b->maxbk = (bank | 0xffff);               // temp bank setup so that g_val (sigs) etc. work
     b->maxpc = b->maxbk;                      // need for signature scan
     b->minpc = (bank | PCORG);                // must become value from sig
     b->bok = 1;                               // temp mark as OK
     b->cmd = 0;
     faddr = scan_start(i, faddr);             // test for start jump at or around around faddr

     if (b->bok)
      {                                        // found !! 8061 mem map indicates 0xc000 (+a000), but 8065 to ffff
       faddr += 0xe000;                        // assume end of this bank at 0xffff, as a file offset - but might not be !!

// OK have now found a format which does NOT conform, so need to CHANGE this.







       b->filend = faddr-1;                    // this may be wrong, but this is default for now (0x2000 - 0xffff).

       if (faddr >= fldata.fillen) b->filend = fldata.fillen-1;                  // end of file

       b->maxbk = b->filend - b->filstrt + PCORG;
       b->maxbk |= bank;
       b->bok = 0;
       numbanks++;

       #ifdef XDBGX
        DBGPRT(1,"temp Bank %05x-%05x, type %d", b->filstrt, b->filend, b->cbnk);
        DBGPRT(1,0);
       #endif

       if (i == 2 && (b->filstrt & 3))  wnprt(2,"!! WARNING !! file appears to have missing or extra byte(s) at front");
       if (faddr >= fldata.fillen) break;

       i++;
     }
    else break;
    }
}


void clrbkbit(int ix, int bk)
{
  BANK *b;

  b = bkmap+ix;

  switch (bk)
  {
    case 0:
      b->bkmask &= 0xe;      //clear bit 0
      break;
      
    case 1:
      b->bkmask &= 0xd;      //clear bit 1
      break;

   case 8:
      b->bkmask &= 0xb;      //clear bit 2
      break;

    case 9:
      b->bkmask &= 7;        //clear bit 3
      break;
     
    default:
      #ifdef XDBGX
      DBGPRT(0,"unknown bank %d", bk);
      #endif
      break;
  } 
}

void cpy_banks(void)
 {
  int i, j, bk;
  BANK *b;

// for (i = 2; i < 3+numbanks; i++) original
// change to allow for a deleted bank

  j = 0;
  for (i = 2; i < 6; i++)
   {
    bk = bkmap[i].dbank;             //getflagbnk(i);
    if (bk >=0)
      {
       b = bkmap + bk;                // preset destination
       bk <<= 16;
       *b = *(bkmap+i);
       b->minpc &= 0xffff;
       b->minpc |= bk;    // substitute bank no
       b->maxpc = 0;       // do fill check
       b->maxbk &= 0xffff;
       b->maxbk |= bk;
       b->bok = 1;
       j++;
      }

     if (j >= numbanks+1) break;
   }

// clear ALL 2-6 ALWAYS
  for (i = 2; i < 6; i++)
   {
    bkmap[i].bok = 0;
   }


}


void drop_banks_via_ints (int bk)
 {
    // check outwards from here for other banks
    // from int vect pointers via jumps and bank prefixes.
    // numbanks is 4, 2 to 6 of bkmap valid, the temp slots
    // but real bank nos (to be) assigned to them
    // NB. some bins don't have cross bank int jumps (e.g. 22CA)

    uint tbank, taddr, x;
    int  val, rbank;

  //  bkmap[bk].numbk = 3;              // 3 possible banks
    bkmap[bk].bkmask = 0xb;         // 9, 1, 0  not 8
    bkmap[bk].bok = 1;

    tbank = bk << 16;                 // TEMP bank no (so it maps OK)

    for (x = (tbank|0x2010);  x < (tbank|0x205f);  x+=2)
      {
       taddr = g_word(x);             // pointer
       taddr |= tbank;                 // make true address
       val = g_byte(taddr);
       // for 8065, can be a bank swop.  Check next byte is 0,1,8,9 with mask (and THEN a jump ??)

       if (val == 0x10)
         {           // bank swop prefix.
          taddr++;
          rbank = g_byte(taddr);            // get true bank (from code)
          /* check this is not wrong already !! CAN'T !!
          if (getflagbnk(cbk) == rbank)
                 {
                  #ifdef XDBGX
                    DBGPRT(1," Bank self ref (%d) %d. Clear", cbk, rbank);
                  #endif
                  setflgbnk(cbk,-1);
                 }    */
          // assume this bank CANNOT be rbank.............
          clrbkbit(bk,rbank);

          taddr++;
          val = g_byte(taddr);             // get opcode after bank prefix

          if (val == 0xe7 || ( val >=0x20 && val <= 0x27))
           {
            // a jump - calc destination
            taddr = do_jumpsig (opcind[val], taddr);
           // now try to find a valid opcode in one of the other banks.
           // have a guessed order at this point

          /* for (i = 2; i < 6; i++)
             {
               taddr = nobank(taddr);
               taddr |= (i << 16);
               val = g_byte(taddr);
               if (val >= 0xf0 && val <= 0xf2)
                  {
                   if (getflagbnk(i) != rbank)
                    {       // not match !!
                     #ifdef XDBGX
                        DBGPRT(1,"Change (%d) from %d to %d",i, getflagbnk(i),rbank);
                     #endif
                     setflgbnk(i,rbank);
                    }

                #ifdef XDBGX
                else
                {
                 DBGPRT(1,"confirm bank (%d) %d via intr jump",i, rbank);
                }
                #endif

               }
             } */
           } 
         }    // end bank check 8065 loop
      }

// now check for an allocated bank after shuffle....



 }

int chk_bank_order (int cbk)
 {
    // cbk = bank location - so check outwards for other banks
    // from int vect pointers

/* cannot change bank 8 !!
   GCM2 is fked up by this code ....
  must check if there IS a clash first....
 */ 

// 22CA all point to bank 8 !!

  int i, x, ans;
  BANK *b;

  ans = 1;  
  // first use interrupt handlers to identify any bank jumps
  // and know that THIS bank cannot be any that are jumped to
  // b->bkmask is bitmask of possible banks

  
  for (i = 2; i < 6; i++)
    {
     b = bkmap + i;
     if (i != cbk) drop_banks_via_ints(i);
     else
      {    
       b->dbank = 8;       // set up bank 8 directly
       b->bkmask = 4;
      } 
    }       
      
   // now see if we can definitely find more banks
   // if any bank has single bit, can take that as definite ID
   // and drop that bit from all other banks

   for (i = 2; i < 6; i++)
    {  
      b = bkmap + i; 
      
      if (i != cbk)
        {  // not for bank 8
         if (b->bkmask == 1 || b->bkmask == 2 || b->bkmask == 8)
         {  // found a single bit, drop form other banks
           for (x = 2; x < 6; x++)
             { 
               if (x != i) bkmap[x].bkmask &= (~b->bkmask);   
             }
         }
        }
    }   
        
  // now, is there one bit per bank ?? if so have got the
 // bank order.  if not, then we'll have to guess.      
 
   for (i = 2; i < 6; i++)
    {  
      b = bkmap + i; 
      switch (b->bkmask)
        {
          case 1 :
            b->dbank = 0;
            break;
           
          case 2 :
            b->dbank = 1;
            break;
           
          case 4 :
            b->dbank = 8;
            break;
     
          case 8 :
            b->dbank = 9;
            break;
          
          default :
           ans = 0;       // not single bits
           break;
        }
      
      if (!ans)
       {
        wnprt(2, "WARNING - Cannot confirm bank order is correct, selected order is best guess only");
        break;
       } 
    }

return ans;
}



int do_banks(void)
 {
  // set up banks after find_banks, or user command.

  // set up RAM bank [16] first 0x2000 of ibuf for ram and regs
  // NOTE - not technically safe if file has missing front bytes, but probably OK

  int ltj,bank,i;
  BANK *b;

  b = bkmap+BMAX;
  b->maxbk = PCORG-1;               // 0-0x1fff in bank 16 for registers
  b->opbt = 0;
  b->maxpc = PCORG-1;
  b->minpc = 0;
  b->fbuf  = ibuf;                  // first 0x2000  in bank 16 for regs and RAM
  b->bok = 1;

  // now do the checks and bank ordering.....

  if (numbanks)  cmdopts |= 0x8;          // multibank bin, mark as 8065
  else
   {
     if (bkmap[2].P65) {cmdopts |= 0x8; bkmap[2].P65 = 0;}    // single bank 8065 detected from int.vects
   }

  bank = -1;                           // bank with the start jump (= code)
  ltj  = 0;                            // temp number of loopback jumps

  for (i = 0; i <= numbanks; i++)
      {
       b = bkmap+i+2;                // 2 to 5
//       b->P65 = 0;                    //dbnk = -1;                  // clear any flags

       if (b->cbnk)
          {
           if (bank >= 0)
             {
               wnprt(1,"Found multiple Start banks [bank 8]");
               wnprt(1,"Ignoring extra bank");
               b->cbnk = 0;        // set to a loopback
               numbanks--;
            }
           else bank = i;

          }
       else ltj++;       // no of other banks
      }

  if (bank < 0)
     {
      wnprt(1,"Cannot find a Start bank [bank 8]");
      wnprt(1,"Probably binary file is corrupt");
      return -4;
     }

   #ifdef XDBGX
   DBGPRT(1,"%d Banks, start in %d", numbanks+1, bank+1);
   #endif

  switch (numbanks)
    {

     default:
       wnprt(1,"0 or >4 banks found. File corrupt?");
       return -2;
       break;

     case 0:                      // copybanks autostarts at [2] for source list
       mkbanks(1,8);
       break;

     case 1:                      // 2 bank, only 2 options
       if (bank == 1) mkbanks(2,1,8);
       else mkbanks(2,8,1);
       break;

     // no 3 bank bins (case=2) so far .....

     case 3:
       // do best guess bank order and then check it
       // can shuffle numbers before the cpy_banks which moves them.
       if (!chk_bank_order(bank+2)) // attempt to sort banks by intrpt handlers
       switch(bank)
        {
         // Alternate bank orders - best guess from bank 8 position

         case 0:
           mkbanks(4,8,9,0,1);
           break;

        case 1:
           mkbanks(4,9,8,0,1);
           break;

         case 2:
           mkbanks(4,0,1,8,9);
           break;

         case 3:
           mkbanks(4,0,1,9,8);
           break;
        }

       break;
      }

   // final copy and fill detection. this subr recalled if user command
  cpy_banks();
  set_banks();
  #ifdef XDBGX
  DBGPRT(1,0);
  #endif

  return 0;
}


int readbin(void)
{
  /* Read binary into main block 0x2000 onwards (ibuf[0x2000])
  *  ibuf  [0]-[2000]     RAM  (faked and used in this disassembly)
  *  ibuf  [2000]-[42000] up to 4 banks ROM, at 0x10000 each
  */

  int x;

  fldata.fillen = fseek (fldata.fl[0], 0, SEEK_END);
  fldata.fillen = ftell (fldata.fl[0]);
  fseek (fldata.fl[0], 0, SEEK_SET);                // get file size & restore to start

  wnprt(2,"# Input file is '%s'",fldata.fn[0]);
  wnprt(2,"# File is %dK (0x%x) bytes",fldata.fillen/1024, fldata.fillen);

//  fldata.error = 0;

  if (fldata.fillen > 0x40000)
   {
    wnprt(1,"# Bin file > 256k. Truncated to 256k");  // is this always right ??
    fldata.fillen = 0x40000;
   }

  wnprt(1,0);

  fread (ibuf+PCORG, 1, fldata.fillen, fldata.fl[0]);

  #ifdef XDBGX
   DBGPRT(1,"Read input file (0x%x bytes)", fldata.fillen);
  #endif

// but if sailorbob right, will need to load 4 parts of file into 4 parts of buffer....
// which is totally different to now, with one contiguous write............



// check for file error ?
//  end = fldata.fillen;

  find_banks();
  x = do_banks();               // auto, may be modded later by command
  return x;
}


void rbasp(SIG *sx, SBK *b)
{

  //add rbase entries from signature match

  uint add, add2, reg, i, cnt;   //,xcnt,rcnt;
  RBT *r;
  uint  pc;

  if (b) return;                  // no processing if subr set

 // rcnt = chbase.num;              // may have rbases already. start at 'NEW' point
  reg = (sx->v[1] & 0xff);
  add = sx->v[3];
  cnt = g_byte(add);              // no of entries in list

  #ifdef XDBGX
  DBGPRT(1,"In rbasp - begin reg %x at %x, cnt %d", reg, add, cnt);
  #endif

  add_data_cmd(add, add+1, C_BYTE|C_SYS,0);
  add_data_cmd(add+2, add+(cnt*2)+1, C_WORD|C_SYS, 0);

  if (chdata.lasterr)
   {
     #ifdef XDBGX
      DBGPRT(1,"Rbase Already Processed");
     #endif
    return ;
   }

  pc= add+2;

 //  check bank is correct ?
    add = g_word(pc);                       // register pointer
    add |= datbnk;                          // + default data bank
    add2 = g_word(add);                     // check pointer is consistent with next entry
    i = g_word(pc+2);                       // next in list

  #ifdef XDBGX
  if (i != add2)
    {
     // rbases point to different bank
     DBGPRT(1,"RBASEDBNK");
    }
  #endif

  for (i = 0; i < cnt; i++)
   {
    add = g_word(pc);                       // register pointer
    add |= datbnk;                          // add + default data bank
    add_data_cmd(add, add+1, C_WORD|C_SYS,0);
    add2 = g_word(add);                     // check pointer is consistent with next entry
    // also valid for last pointer for xcode

    r = add_rbase(reg,add,0,0);                     // message in here....
    if (r) r->cmd = 1;
    add_code_cmd(add,add2-1, C_XCODE|C_SYS);          // and add an XCODE as this is data

    reg+=2;
    pc+=2;
   }

 /* now check pointers for XCODE directives for newly added entries

 if (rcnt == chbase.num)  return ;     // nothing added

 for (i = rcnt; i < rcnt+cnt; i++)
   {
     if (i >= chbase.num)  break;
    r = (RBT*) chbase.ptrs[i];
    n = (RBT*) chbase.ptrs[i+1];
    pc = n->rval & 0xffff;

    xcnt =0;

    if (i >= rcnt+cnt-1)
     {
      n = (RBT*) chbase.ptrs[i-1];                  // get bank of last pointer
      r->rval = (r->rval & 0xffff) | (n->rval & 0xf0000);
      pc = g_word(r->rval);
     }
    else
     {
       while (pc != g_word(r->rval) && xcnt < BMAX && i < rcnt+cnt-1)
       {
        xcnt++;             // try to find matching bank...
        if (bkmap[xcnt].bok)  r->rval = (r->rval & 0xffff) | (xcnt << 16);
       }
     }

    if (xcnt >= BMAX)
    {
		#ifdef XDBGX
        DBGPRT(1,"Matching rbase addr not found !");
		#endif
    }
    else
      {
   //     if (xcnt) DBGPRTN("Rbase %x = %x",r->radd, r->rval);
      //  set_cmd(r->rval, r->rval+1, C_WORD);
     //   set_auxcmd(r->rval,pc-1, C_XCODE);          // and add an XCODE as this is data
        add_data_cmd(val, val+1, C_WORD|C_CMD,0);
        add_code_cmd(val,val2-1, C_XCODE);          // and add an XCODE as this is data
      }
   }
   sx->done = 1;
   #ifdef XDBGX
   DBGPRT(1,"end rbasp");
   #endif
   return ;*/
}


/*
void check_rbase(void)       //SIG *sx, SBK *dummy)
{

// rewrite to ditch the sig scan
// CAN'T DO THIS - 0FAB does not use f0 as start point !!!
 // must go back to signature !!

  //check if rbase caclcon pointer entries are present
  // 2020 num of levels (always 8)
  // 2021 no of calibrations (1)
  // 2022-20A1 cal pointers = 7f 0r 64 pointers
  // presume 2060 for 8065.

  int i, cnt,bank, bf;
  RBT *r;
  uint  pc, addr, val, val2;

 #ifdef XDBGX
  DBGPRT(1,"In chk_rbasp");
  #endif

// do NOT use numbanks, as CAN have single bank 8065 !!
  if (P8065) addr = 0x82060; else addr = 0x82020;

  cnt = g_byte(addr);                     // no of entries
  bf = 0;
  pc = addr+2;
  val2 = g_word(pc);

  if (cnt != 8)
   {             // may be a bigger number somewhere....
    #ifdef XDBGX
     DBGPRT(1,"Not Found");
    #endif
   return;
   }

  bank = 8;

  for (i = 1; i < cnt; i++)
   {
    addr = val2 | (bank << 16);
    if (!val_data_addr(addr)) break;  // cancel all

    val = g_word(addr);                    // value at pointed to
    pc += 2;                               // still in bank 8
    val2 = g_word(pc);                     // get next match (was addr)

    if (val != val2)
     {
      if (!numbanks)  break;
      // check if rbase points to another bank (typically b1)
      if (!bf)
        {
         // not checked yet
          for (bank = 0; bank < BMAX; bank++)
              {
                if (bkmap[bank].bok)
                 {          // bank is OK
                   addr = nobank(addr) | (bank << 16);
                   val = g_word(addr);                    // value at pointed to
                  if (val == val2) {bf = 1; break;}                      // bank found
                 }
              }
        }
     }
   }

  if (i < cnt)
   {
    #ifdef XDBGX
    DBGPRT(1,"Not Found");
    #endif
    return;
   }


  #ifdef XDBGX
    DBGPRT(1,"FOUND RBASE, points to bank %d", bank);
  #endif

  // OK, matched list and bank................

  if (P8065) addr = 0x82062; else addr = 0x82022;

  add_data_cmd(addr-2, addr-1, C_BYTE|C_CMD, 0);
  add_data_cmd(addr, addr+(cnt*2)-1, C_WORD|C_CMD, 0);

  pc = 0xf0;                                // always 0xf0 ?? NO !!!!
  for (i = 0; i < cnt; i++)
   {
    val   = g_word(addr);                    // value at pointed to
    val  |= (bank << 16);
    val2  = g_word(val);
    val2 |= (bank << 16);

    r = add_rbase(pc,val,0,0);             // message in here....
    if (r)
      {
       r->cmd = 1;
       upd_ram(pc,val,2);                           // and set register
       add_data_cmd(val, val+1, C_WORD|C_CMD,0);
       add_code_cmd(val,val2-1, C_XCODE);          // and add an XCODE as this is data
      }

    pc+=2;
    addr+=2;
   }

   #ifdef XDBGX
   DBGPRT(1,"end rbasp");
   #endif
   return ;
}*/


// do branch has a FULL copy of SFIND to ensure that parallel branches have their own
// parameter set established at start of each new branch (= a jump to here)
// this is because the registers may CHANGE in each branch.

void (*set_str) (uint , SFIND);
int anix;

void do_branch(uint addr, int cnt, SFIND f)
 {
    // do a branch from a jump. if jump(s) to here found, do as separate branch
   int xcnt;
   uint xofst, ix;
   JMP *j;
   OPC *opl;

   if (cnt <= 0) return;
      #ifdef XDBGX
       DBGPRT(1,0);
       DBGPRT(0,"Branch start %x %d [%x %x] (%x %x)", addr,cnt, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
      #endif
   xcnt = cnt;
   xofst = addr;

   while (xcnt)
     {
       // find_opcode just goes down/up list, no checks (pn_opcodes does check).
       xofst = find_opcode(0, xofst, &opl);
       if (!xofst) break;
       xcnt --;

       if (!xcnt && f.ans[0])
         {
          #ifdef XDBGX
            DBGPRT(0,"END, set %x at %x", f.ans[0], xofst);
          #endif
          set_str(anix, f);  // add table with one row if run out of opcodes
         }

       if (xcnt <= 0)
         {
          #ifdef XDBGX
            DBGPRT(0,"END %x ", xofst);
          #endif
          break;
         }

       match_opnd(xofst,opl,&f);

       if (f.ans[0] && f.ans[1])
         {
          #ifdef XDBGX
           DBGPRT(1," MATCH R%x=%x R%x=%x", f.rg[0], f.ans[0], f.rg[1], f.ans[1]);
          #endif
          // do the func or tab set here

          set_str(anix,f);   // add func or tab (add override)

          f.ans[0] = 0;
          f.ans[1] = 0;
          break;
         }
       #ifdef XDBGX
         else DBGPRT(0," %x[%d]", xofst, xcnt);
       #endif

       // see if any jumps in to xofst. if so, launch new branches.

       ix = find_tjix(xofst);     // start point

       while(ix < chjpt.num)
        {
          j = (JMP*) chjpt.ptrs[ix];
          if (j->toaddr != xofst) break;
          #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(1,"JMP %x<-%x %d", j->toaddr, j->fromaddr, cnt);
          #endif

         //j->from-j->cmpcnt to skip the cmp
         do_branch(j->fromaddr,xcnt,f);
         ix++;
        }  // end jump loop
      
     }        // end find opcode loop

}


void do_calls(SUB * sub, SFIND f)    //uint rg1, uint rg2, uint rg3, uint ans1, int ans2, int ans3)
{
  // do the 'base' (i.e. the subr calls outwards) separately to reset params
  // was    do_branch(sub->start,20, sub->sig->v[4], 1, 0, 0, 1, sub->sig->v[7]);

 JMP *j, *k;
 LBK *g;
 ADT *a;
 uint jx, ix, ofst;
      #ifdef XDBGX
        DBGPRT(1,0);
        DBGPRT(0,"Base calls for sub %x", sub->start);
      #endif
      
 ix = find_tjix(sub->start);     // start point

 while(ix < chjpt.num)
   {
    j = (JMP*) chjpt.ptrs[ix];

    if (j->toaddr != sub->start) break;

    if (j->jtype == J_SUB)              // sub only for from FIRST call (why?) for call back to sub......
        {
          if (f.rg[1] == 1) f.ans[1] = j->fromaddr + j->size;     //func only, for override
     #ifdef XDBGX
  DBGPRT(1,0);
          DBGPRT(0,"To %x - [%x %x] (%x %x) ", j->fromaddr , f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
#endif
        //j->from-j->cmpcnt to skip the cmp
        //j->from+j->size to check for arguments
        // defined args are same as 'args' data command.....

        ofst = j->fromaddr + j->size;

        a = get_adnl(&chadnl,sub,0);
        if (a)
          {    // same as data command, but user specified
            f.ans[0] = decode_addr(a, ofst);
             #ifdef XDBGX
            DBGPRT(1,"do args at %x = %x", g->start, f.ans[0]);    // assume first parameter ???
             #endif
           // need datbank ?? from the
          }

        else
        {
          g = find_data_cmd(ofst, C_ARGS);
          if (g)
           {         // assume first parameter ???
            a = get_adnl(&chadnl,g,0);
            f.ans[0] = decode_addr(a, ofst);

            #ifdef XDBGX
             DBGPRT(1,"do args at %x = %x", g->start, f.ans[0]);    // assume first parameter ???
            #endif
            // need datbank ??
           }
        }

      do_branch(j->fromaddr, 20,f);

          // do_branch scans back from j->from FIRST before checking for jumps,
          // so add an extra check and loop for jumps to j->from

          jx = find_tjix(j->fromaddr);     // start point
          while(jx < chjpt.num)
            {
             k = (JMP*) chjpt.ptrs[jx];
             if (k->toaddr != j->fromaddr) break;
             #ifdef XDBGX
              DBGPRT(1,0);
              DBGPRT(1,"To+ %x - [%x %x] (%x %x) ", j->fromaddr, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
             #endif
             //j->from-j->cmpcnt to skip the cmp

             do_branch(k->fromaddr,20,f);                //rg1,rg2,rg3,ans1,ans2,ans3);
             jx++;
            }  // end jump loop
 
            
          #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(1,"End call branch %x", j->fromaddr);
          #endif
       }
    ix++;

   }

}


LBK* del_olaps(uint addr, uint ix)
{
 LBK *n;
 n = 0;
 if (ix < chdata.num) n = (LBK *) chdata.ptrs[ix];
 while (n && n->start < addr)
     {
      // n MUST be wrong - delete it. keep if higher or cmd
      if (n->cmd) break;
      if (n->fcom < C_TEXT) chdelete(&chdata,ix,1); else break;
      if (ix < chdata.num) n = (LBK *) chdata.ptrs[ix]; else n = 0;
     }
return n;
}


void extend_func(uint ix)
{

  uint eadr, size, rsize, adr, val, eval;
  ADT *a, *b;
  LBK *k, *n;

  k = (LBK *) chdata.ptrs[ix];

  ix++;
  if (ix < chdata.num) n =  (LBK *) chdata.ptrs[ix]; else n = 0;
      // check end of func

  eadr = maxadd(k->end);          // safety
  a = get_adnl(&chadnl,k,0);     // entry zero   //k->adnl;
  b = get_next_adnl(&chadnl,a);     // entry one    //a->next;

 // extend func to zero or max negative here

  size  = casz[a->ssize];                 // input size
  rsize = size + casz[b->ssize];          // whole row size
  eval  = casg[a->ssize];                 // end value
  adr   = k->start;

  // need to check next command as well....

  while (adr < eadr)                  // check input value for each row (UNSIGNED)
    {
     adr += rsize;
     val = g_val(adr, size);            // next input value
     if (val == eval)
        {
         eadr = adr + rsize-1;            // hit end input value, set end
         break;
        }
    }

  k->end = eadr;                        // stage 1 of extend

  //check here if any command occurs inside the function and delete them
  n = del_olaps(eadr, ix);

  //adr is now at first zero row, and val is last value

  eadr = maxadd(k->end);           // reset end

  if (n && n->cmd) eadr = n->start;  // can't go past a user command.

  rsize = (a->ssize + 1) & 3;      // row size (unsigned)
  eval = g_val(adr,rsize);         // and its full row value

  while (adr < eadr)
    {       // matching final rows
     adr += casz[rsize];
     val = g_val(adr,rsize);
     if (val != eval) break;
     if (n && !eval && n->start == adr) break;   // match next cmd for extended zeroes
    }

  // adr is now start of next item (end of extend values)
  // if end value is zero need extra check
  // otherwise need to delete any skipped entries........
  n = del_olaps(adr, ix);


  // need any more here ???

  if (eval != 0 || n == 0 || n->start >= adr)
    {  // end of list or not zero value or no overlap
     if (adr > k->end) k->end = adr-1;
    }
  else
    {     // check next command

      // n is < and value is 0
                   // probably wrong, but safe for now
                   // extend if only one word/byte
   //             k->end = n->start-1;          // TEMP !!
                 k->end = adr-1;
          }
      }


void extend_table(uint ix)
{
  uint apeadr, maxadr, size, val, eval;
  ADT *a;
  LBK *k, *n;           //, *t;

  k = (LBK *) chdata.ptrs[ix];

 // options -

 // could just fit nicely to n.
 // some bytes are unknown before n
 // another cmd(s) before true n
 // + colsize is wrong for this table
 // then possible single byte filler

// make tab_data_patt return only stats by addr, end, (->size),
// and do changes in here.  (then can call for unknowns)
// stats will be 'score' (by col x row ?), next row range (somehow)
// to decide how to proceed.


  if (ix >= chdata.num)
   {
     apeadr = maxadd(k->end)+1;
     maxadr = apeadr;
   }

  else
   {
    ix++;
    n = (LBK *) chdata.ptrs[ix];
    apeadr = n->start;             // next apparent data cmd

    while (n->fcom < C_TEXT && !n->cmd)
     {  // struct of user command
       ix++;
       n = (LBK *) chdata.ptrs[ix];    //skip all byte and word
     }

    maxadr = n->start;            // max not skippable
   }

  a = get_adnl(&chadnl,k,0);          //k->adnl;                 // cols in a->cnt

  size = apeadr - k->start;    // size in bytes to apparent end
  eval = size/a->cnt;          // max rows
  val  = eval * a->cnt;        // size in whole rows

 if (a->cnt > 2)
  {  // must have valid rows
  if (val == size)
    {
      // MATCH with extra rows.... eadr is next cmd
      k->end = apeadr-1;
      return;
    }

  if (val+1 == size)
   {   // could be a filler - need to check next data cmd ?
      if (g_byte(apeadr-1) == 0xff) k->end = apeadr-2;
      return;
   }
  }
   // default - move table to nearest whole row ?? not perfect, but OK for now.
   // then investigate for sizes
   k->end = k->start+val-1;       // temp

  size = do_tab_data_pattern(k->start, a->cnt, &apeadr, maxadr);

  //need to get end address out as well.......use apeadr

 if (size)
     {
      a->cnt = size;
      k->size = size;  // should do a dsize() really         // 1st test
     if (k->start+apeadr > k->end) k->end = k->start+apeadr-1;
     }
}


void do_structs()

 {
   uint i;
   SUB *sub;
   LBK *k;
   SFIND f;
   SPF *x;
   // go down tab and func subroutines and find params (structs)

   // func subroutines

   for (i = 0; i < chsubr.num; i++)
   {
    sub =  (SUB *) chsubr.ptrs[i];
    x = get_spf(sub);

    if (x)
    {
    if (x->spf > 4 && x->spf < 13)
      {
       set_str = &set_xfunc;
       anix = x->spf;
       f.rg[0] = x->addrreg;
       f.rg[1] = 1;
       f.ans[0] = 0;
       f.ans[1] = 1;
       do_calls(sub,f);
      }

   // table subroutines

    if (x->spf > 12 && x->spf < 15)      //sub->spf > 12  && sub->spf < 15)
    {
      set_str = &set_xtab;
      anix = x->spf;                   //sub->spf;
      f.rg[0] = x->addrreg;            //sub->sig->v[4];  //address
      f.rg[1] = x->sizereg;            //sub->sig->v[3];  //cols from sig
      f.ans[0] = 0;
      f.ans[1] = 0;
      do_calls(sub,f);
    }
   }
   }
// now extend the end of funcs for the extra filler rows.....
// probably need to check tables too.

   for (i = 0; i < chdata.num; i++)
   {
     k = (LBK *) chdata.ptrs[i];
     if (!k->cmd)
       {
        if (k->fcom == C_FUNC)  extend_func(i);
        if (k->fcom == C_TABLE) extend_table(i);
       }
   }
}



char *calcfiles(char *fname)
 {
  // do filenames
  char *t, *x;
  int i;

  t = strrchr(fname, '\n');          // DOS and Linux versions may have newline at end
  if (t) {*t = '\0';}

  x = fname;                           // keep possible pathname

  t = strrchr(fname, PATHCHAR);         // stop at last backslash
  if (t)
   {          // assume a path in filename, replace path.
    *t = '\0';
    fname = t+1;
    sprintf(fldata.path, "%s%c", x,PATHCHAR);
   }


  x = strrchr(fname, '.');                         // ditch any extension
  if (x) *x = '\0';

  strcpy(fldata.bare,fname);                        // bare file name

#ifdef XDBGX
  strncpy(fldata.fn[6], fldata.fn[2], 255);             // copy wrn path to debug
#endif

  for (i = 0; i < 5; i++)
   {
    if (! *(fldata.fn[i])) strcpy(fldata.fn[i],fldata.path);   // use default path name

    strcat(fldata.fn[i],fldata.bare);                 // bare name
    strcat(fldata.fn[i],fd[i].suffix);                // plus suffix
   }

#ifdef XDBGX
 // debug file

 if (! *(fldata.fn[6])) strcpy(fldata.fn[6],fldata.path);   // use default path name

    strcat(fldata.fn[6],fldata.bare);                 // bare name
    strcat(fldata.fn[6],fd[6].suffix);                // plus suffix

#endif


 return fldata.bare;
 }


/***************************************************************
 *  disassemble EEC Binary
 ***************************************************************/
short disassemble (char *fstr)
{

  init_structs();
  calcfiles(fstr);
  openfiles();

 // time(&timenow);
 // sprintf(nm, "%s", ctime(&timenow));

/*


#include <stdio.h>
#include <time.h>

void main()
{
    time_t t;
    time(&t);
    clrscr();

    printf("Today's date and time : %s",ctime(&t));
    getch();
}


*/






  if (fldata.error)
    {
     printf("File not found or File error\n");
     return fldata.error;
    }

  wnprt(1,0);
  wnprt(1,"# ----------------------------");
  wnprt(1,"# SAD Version %s (%s)", SADVERSION, SADDATE);
  wnprt(2,"# ----------------------------");

  if (readbin() < 0)               // and does init_structs in here
  {
    wnprt(1,"Abandoned !!");
    printf("Abandon - can't process - see warnings file\n");
    return 0;
  }

  show_prog(++anlpass);            //anlpass = 1  cmds and setup
  getudir();                       // AFTER readbin...

  show_prog(++anlpass);            //anlpass = 2  signature pass
  prescan_sigs();

   //check_rbase();                     // check static rbase setup (2020 etc, not a sig)
  wnprt(2,"# ----- Start Disassembly phase 1 -----");

  scan_all ();        // do all the analysis

 // ----------------------------------------------------------------------

  show_prog(++anlpass);             //anlpass = 3  scan phase

  do_jumps();

  wnprt(2,"# ----- End   Disassembly phase 1 -----");
  wnprt(2,"# ----- Start Disassembly phase 2 -----");

   // ----------------------------------------------------------------------
   // and do a whole new pass here for the data ??? loops,structs, tables....
   // ----------------------------------------------------------------------


   scan_dc_olaps();            // check and tidy code and data overlaps
   scan_code_gaps();           // scan for code gaps

  //scan_data_gaps();


  show_prog (++anlpass);            //anlpass = 4

  wnprt(2,"# ----- End   Disassembly phase 2  -----");

  do_code_scans();      //  turn all scans into code

  do_structs();


  // ----------------------------------------------------------------------


 

   wnprt(2,"# ----- Output Listing to file %s", fldata.fn[1]);

   show_prog (++anlpass);                 //anlpass = 5 = ANLPRT

  #ifdef XDBGX
    DBG_data();
  #endif

  do_listing ();
  newl(2);     // flush output file

  wnprt(2,0);
  wnprt(1,"# ---------------------------------------------------------------------------------------------");
  wnprt(1,"# The disassembler has scanned the binary and arrived at the following final command list.");
  wnprt(1,"# This list is not guaranteed to be perfect, but should be a good base.");
  wnprt(1,"# Extra gap and data scans (Phase 2) are shown above as they may be incorrect");
  wnprt(1,"# This following list can be copied and pasted into a directives file from here.");
  wnprt(3,"# ---------------------------------------------------------------------------------------------");

  prt_dirs();
  free_structs();
  mfree(ibuf, 0x42000);
  mfree(opbit, 0x2000);
  wnprt(2,0);
  wnprt(2, "# ----- END of disassembly run -----");
  printf ("\n END of run\n");
  closefiles();
  return 0;
}


int get_config(char** pts)
 {
  /* pts [0] = exe path (and default) ,
   * [1] = config file READ path or zero.  (use default path)
   * both from command line
   */

  short i;
  char *t;

  // PATHCHAR defined at beginning with file option flags

  t = pts[0];
  if (*t == '\"' || *t == '\'') t++;      // skip any opening quote
  strcpy(fldata.dpath, t);

 // if (!t) printf("error in cmdline !!");    // never true ??

  t = strrchr(fldata.dpath, PATHCHAR);           // stop at last backslash to get path
  if (t) *t = '\0';

  if (pts[1])
  {
     strncpy(fldata.fn[5],pts[1], 255);           // copy max of 255 chars
     i = strlen(fldata.fn[5]);                    // size to get last char
     t = fldata.fn[5]+i-1;                        // t = last char
     if (*t == PATHCHAR) *t = 0; else t++;

     sprintf(t, "%c%s",PATHCHAR,fd[5].suffix);    // and add sad.ini to it...

     fldata.fl[5] = fopen(fldata.fn[5], fd[5].fpars);
     if (!fldata.fl[5])
       {
           printf("\ncan't find config file %s\n", fldata.fn[5]);
           return 1;                // no config file in -c
       }
  }

 else
  {
    // else use bin path to make up file file name of SAD.INI
    sprintf(fldata.fn[5], "%s%c%s",fldata.dpath,PATHCHAR,fd[5].suffix);
    fldata.fl[5] = fopen(fldata.fn[5], fd[5].fpars);         //open it if there
  }


   if (fldata.fl[5])
     {                                           // file exists, read it
     for (i = 0; i< 5; i++)
      {
       if (fgets (flbuf, 256, fldata.fl[5]))
        {
         t = strchr(flbuf, ' ');                 // first space
         if (!t) t = strchr(flbuf,'#');          // or comment
         if (!t) t = strchr(flbuf, '\n');        // or newline
         if (t)
            {
             *t = '\0';
             t--;
             if (*t != PATHCHAR) sprintf(t+1, "%c", PATHCHAR);
            }
         strcpy(fldata.fn[i], flbuf);

       }
      }
     fclose(fldata.fl[5]);                 //close leaves handle addr
     fldata.fl[5] = NULL;
     strcpy(fldata.path, fldata.fn[0]);
     printf("\n");
    }
  else printf(" - not found, assume local dir\n");

  return 0;
 }

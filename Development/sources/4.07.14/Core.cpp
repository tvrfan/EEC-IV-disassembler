
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version 4.0.7
 ******************************************************
 * BUILD NOTES
 * This code assumes INT and UNSIGNED INT are at least 32 bits (4 bytes).
 * and WILL NOT WORK with 16 bit compilers.
 * sizes here (in bits) for information for different platforms
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

// #include  <time.h>             // if printing date and time

#include  "core.h"             // this calls shared.h
#include  "sign.h"


/**********************************************************************************
external subroutines, variables
***********************************************************************************/

extern PAT intign;                // sig 'ignore interrupt'  (for naming)
extern PAT hdr;                   // sig 'start jump' (and vects);
extern PAT fnlp;
extern PAT tblp;
extern PAT inthd;

void  show_prog    (int);

SIG*  scan_sigs    (int);
SIG*  scan_asigs   (int);
void  prescan_sigs (void);
int   do_jumpsig   (int, uint, uint);
void  do_jumps     (void);

void *mem          (void*,size_t, size_t);
void  mfree        (void *, size_t);

void free_structs (void);
void set_rgsize   (RST *,uint);

int adtchnsize    (CHAIN *, uint, uint, uint);

ADT *add_adt      (CHAIN *, uint, uint);
LBK *add_cmd      (uint, uint, uint, uint);
LBK *add_aux_cmd  (uint, uint, uint, uint);
RBT *add_rbase    (uint, uint, uint, uint);
SYM *add_sym      (const char *, uint, uint, uint, uint, uint, uint);
SUB *add_subr     (uint);
SBK *add_scan     (uint, int, SBK *);
SPF *add_spf      (uint);
SBK *add_escan    (uint, SBK *);
JMP *add_jump     (SBK *, int, int);
RST *add_rgstat   (uint, uint, uint, uint);
PSW *add_psw      (uint, uint);
TRK* add_dtk      (SBK *,INST *); 

SBK *get_scan      (uint, uint);
RST *get_rgarg     (uint, uint);
LBK *get_cmd  (uint, uint);
LBK *get_aux_cmd  (uint, uint);
SBK *get_scan      (uint);

LBK *get_prt_cmd   (LBK *dflt, BANK *b, uint ofst);
RST* get_rgstat    (uint);
SIG* get_sig       (uint);
uint get_tjmp_ix   (uint);
JMP *get_fjump     (uint);
uint get_jump      (CHAIN *,uint);
SUB *get_subr      (uint);
RBT* get_rbt       (uint, uint);
SYM* get_sym       (uint , uint , uint , uint);
ADT *get_adnl      (CHAIN *, uint, uint );
ADT *get_next_adnl (CHAIN *, ADT *);
SPF *get_spf       (uint) ;
PSW* get_psw       (uint);
TRK* get_dtk       (uint);
DTD* get_dtkd      (CHAIN *, uint, uint );


int find_tjump (uint);
JMP * find_fjump (uint , int*);

SIG* match_sig     (PAT *, uint,uint);

ADT *start_adnl_loop(CHAIN *, uint);

SIG* inssig(SIG *blk);

void fixbitfield(uint *, uint *, uint *);

//void scan_dc_olaps(void);
//void scan_code_gaps(void);
void scan_gaps(void);
void check_dtk_links(void);

//void scan_lpdata(void);

void free_adnl(CHAIN *,void *);
ADT *set_adnl(ADT* a);
uint set_adt_fkey(uint,uint);

void chdelete(CHAIN *, uint, int);
void clearchain(CHAIN *);



extern CHAIN chjpt;
extern CHAIN chjpf;
extern CHAIN chcmd;
extern CHAIN chaux;
extern CHAIN chadnl;
extern CHAIN chadcm;
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
extern CHAIN chdtko;
extern CHAIN chdtk;


#ifdef XDBGX

extern const char *chntxt[];
extern const char *jtxt[];

uint DBGPRT (uint, const char *, ...);
void DBG_stack (FSTK *);
void DBG_rgchain(void);
void DBG_rgstat(RST *);
void DBG_sbk(const char *t, SBK *s);
void DBGPRTFGS(int ,LBK *,LBK *);

void DBG_banks(void);

void DBG_sigs(CHAIN*);
void DBG_data(void);

extern  int DBGmcalls;            // malloc calls
extern  int DBGmrels;             // malloc releases
extern  int DBGmaxalloc;          //  TEMP for malloc tracing
extern  int DBGcuralloc;


#endif



/**********************************************************************************
local global declarations
***********************************************************************************/

void scan_blk(SBK *, INST *);      // used in several places
void do_code (SBK *, INST *);

const char *empty = "\0\0\0";      // safety and allows increments, fixes compiler warnings too

//for chain.lasterr errors
const char *etxts[] = {empty, "\002Duplicate Command", "\001Invalid Address","\001Banks Not Match", "\001Odd address\003",
   "\001Commands\006", "\001Ranges Overlap","\002XCODE bans Command", "\002Symname replaces previous \"%s\"" 

 };

//for compressing common strings
const char *cmptxts[16] =
 {empty, " Error - "," Warning - ", " Invalid "," Required"," Not Allowed"," Overlap"," Missing", " Ignored" ,         // use \001 upwards ??
 //  0     1           2              3            4          5             6            7          8  \10 (OCTAL)
 " Parameters", "Data Item(s)", empty,empty,empty,empty, "FATAL - " };
//   9  \11,      10  \12        11-15  \13 -17

/*****************************************************************************
* declarations for subroutines in command structure declaration
***************************************************************************/

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
 C   Cell  for subword fields [start,width]
 D   Displacement - Data offset
 E   Encoded address
 F   Flags byte/word (symbol)    
 G    Gen Type ?? [ text, mS, Volts, A/d Time/mS/scale etc ?] or Hybrid ??
 H 
 I   imd name OK for syms
 J
 K   banK  replace bank where applicable
 L   Long
 M         [ mask ? ](byte or word pair) not done yet
 N   Name lookup, find symbol
 O   Count (cols etc)
 P   Minimum Print fieldwidth
 Q   
 R   Reference pointer (vector) in struct
 S   Signed
 T   Triple                 biT symbol (deprecated) 
 U   Unsigned 
 V   diVisor - Float value
 W   Word,  Write (symbol)
 X   Print RadiX
 Y   bYte
 Z
 ***************************************************************/

// print processors

uint pp_dflt  (uint, LBK *); uint pp_wdbl  (uint, LBK *); uint pp_text  (uint, LBK *);
uint pp_vect  (uint, LBK *); uint pp_code  (uint, LBK *); uint pp_stct  (uint, LBK *);
uint pp_timer (uint, LBK *); uint pp_dmy   (uint, LBK *);

// command processors

uint set_vect (CPS*); uint set_code (CPS*); uint set_scan (CPS*); uint set_cdih (CPS*);
uint set_args (CPS*); uint set_data (CPS*); uint set_rbas (CPS*); uint set_sym  (CPS*);
uint set_subr (CPS*); uint set_bnk  (CPS*); uint set_time (CPS*); uint set_tab  (CPS*);
uint set_func (CPS*); uint set_stct (CPS*); uint set_opts (CPS*); uint clr_opts (CPS*);
uint set_psw  (CPS*); uint set_lay  (CPS*);

// string options for SETOPTS and CLROPTS

const char *optstrs[15] =

{ "default"    ,"sceprt", "tabnames"  , "funcnames", "ssubnames" ,
  "labelnames" ,"manual", "intrnames" , "subnames" , "signatures", "acomments",
  "sympresets" ,"8065"  , "cptargs"   , "dataxtnd" };

// add param options ??

int mopts [15] = {
  PDEFLT  ,     // default
  1       ,     // sceprt  print source code
  2       ,     // auto table names
  0x200   ,     // auto func names
  4       ,     // auto special func subnames
  0x10    ,     // auto labelnames
  0x20    ,     // full manual operation
  0x40    ,     // auto interrupt handler names
  0x80    ,     // auto subroutine names
  0x100   ,     // signatures/fingerprint
  0x400   ,     // do autocomments
  0x800   ,     // preset symbols
  0x8     ,     // set 8065 (not opt D)
  0x2000  ,     // set arg compact layout
  0x4000        // set data extend layout
};


// what about decimal versus hex prints for each data type ??? (too much ?)
// other default layouts and types DONE A & C
// symbol types and print options ?

//special function strings for subroutines

const char *spfstrs[13] =

{ empty    ,"uuyflu", "usyflu" , "suyflu", "ssyflu" ,
            "uuwflu", "uswflu" , "suwflu", "sswflu" ,
            "uytlu" , "sytlu"
};


const char *cmds[23] =

{"fill", "byte", "word", "triple", "long", "text", "vect",  "table","func","struct",
 "timer", "code", "args", "xcode", "subr", "scan", "rbase","symbol", "bank", "setopt", "clropt", "pswset", "layout"};

// Params -
// command processor subr, command printer subr,
// max pars expected, single start address posn, pair start addr posn (7 = none), name allowed,
// <gap> max addnl levels, min addnl levels, default size,
// <gap>  merge allowed, string opts,
// <gap>  global options (scanf), data options string (scanf)


DIRS dirs[23] = {

// these go in cmd chain
{ set_data, pp_dflt,  2, 7, 0, 0,    0,  0, 1,   1, 0,   0,           0    },               // fill  (default) MUST be entry zero
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 1,   1, 0,   0,           "PSUXV" },            // byte
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 2,   1, 0,   0,           "PSUXV" },            // word
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 2,   0, 0,   0,           "PSUXV" },            // triple
{ set_data, pp_wdbl,  2, 7, 0, 1,    1,  0, 3,   0, 0,   0,           "PSUXV" },            // long
{ set_data, pp_text,  2, 7, 0, 0,    0,  0, 1,   1, 0,   0,           0  } ,                // text
{ set_vect, pp_vect,  2, 7, 0, 1,    1,  0, 2,   0, 0,   0,           "DKQ"},               // vect
{ set_tab,  pp_stct,  2, 7, 0, 1,    1,  1, 0,   0, 0,   0,           "OPSUVWXY" },         // table
{ set_func, pp_stct,  2, 7, 0, 1,    2,  2, 0,   0, 0,   0,           "LPSUVWXY" } ,        // func
{ set_stct, pp_stct,  2, 7, 0, 1,    15, 1, 0,   0, 0,   "QAC",       "DELNOPRSUVWXY|" },   // struct
{ set_time, pp_timer, 2, 7, 0, 1,    2,  0, 0,   0, 0,   0,           "NTUWY" },            // timer
{ set_code, pp_code,  2, 7, 0, 0,    0,  0, 0,   1, 0,   0,           0  } ,                // code

//these go in aux chain
{ set_args, pp_dflt,  2, 7, 0, 0,    15, 1, 0,   0, 0,   0,           "DELNOPSUVWXY|" },    // args
{ set_cdih, pp_dmy,   2, 7, 0, 0,    0,  0, 0,   1, 0,   0,           0  } ,                // xcode


{ set_subr, pp_dmy,   1, 0, 7, 1,    15, 0, 0,   0, 0,   "ACF",       "DELNOPSUVWXY|=" },   // subr
{ set_scan, pp_dmy ,  1, 0, 7, 0,    0,  0, 0,   0, 0,   0,           0   },                // scan
{ set_rbas, pp_dmy,   4, 1, 2, 0,    0,  0, 0,   0, 0,   0,           0   },                // rbase

{ set_sym,  pp_dmy,   3, 0, 1, 1,    1,  0, 0,   0, 0,   0,           "BFITWN"    },        // sym
{ set_bnk,  pp_dmy,   4, 7, 7, 0,    0,  0, 0,   0, 0,   0,           0 },                  // bank

{ set_opts, pp_dmy,   0, 7, 7, 0,    1,  0, 0,   0, 1,   0,           0 },                  // set options (external strings array)
{ clr_opts, pp_dmy,   0, 7, 7, 0,    1,  0, 0,   0, 1,   0,           0 },                  // clear options
{ set_psw,  pp_dmy,   2, 0, 7, 0,    0,  0, 0,   0, 0,   0,           0 },                  // psw setter for '0=0' jumps
{ set_lay,  pp_dmy,   3, 7, 7, 0,    0,  0, 0,   0, 0,   0,           0 }                   // layout for prints
};


/**********************
 * opcode index to opcode definition table
 * index for opcodes first - points into main opcode table - 0 is invalid
 * main table ordered to put main opcode types together.
 * **********************/


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


/**********************************************************************

 Signature index value for like instructions - MAX 63
 0  special   1  clr    2  neg/cpl   3 shift l   4 shift R   5 and
 6  add       7  subt   8  mult      9 or        10 cmp     11 divide
 12 ldx       13 stx    14 push     15 pop       16 sjmp    17 subr
 18 djnz      19 cjmp   20 ret      21 carry     22 other
 24 dec       25 inc

 (sjmp is static jump , cjmp is conditional jump)

 * more complicated -
 23 jnb/jb - can cross match to 19 with extra par
 12 and 13 (ldx, stx) can be regarded as equal with params swopped

 40+ are optional and skipped by default, but detectable with -

 40 sig      41 pushp    42 popp    43 di       44 ei       45 (RAM)bnk
 46 (ROM)bnk 47 nop      48 others
*/

//*****************************************************************************
// code emulation procs for opcode struct

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



// NB. in pseudo source - Octal escape sequences (\nnn)  are limited to three digits,
// whereas hex (\xnn..) format is not limited (ansi C rules).
// Octal used in strings below so that letters not interpreted as part of hex number (e.g. 'C' of "CY =").
// '\2x' in the sring means add size and sign flags to the operand 'x' (1-3).

// bit 7 is most significant, 0 is least, so order is 7,6,5,4,3,2,1,0  as read.

// OPC table items across - signature index, 'fe' prefix allowed (= use next entry),
// 8065 opcode only, opcode changes PSW, number of ops, write op index, write size,
// read sizes (operand 1,2,3), opcode name, pseudo source

// op sizes are by 'field end' with an implied field start of 0 
//  7,15,23,31 = unsigned byte,word,triple,long
// 39,47,55,63 =   signed byte,word,triple,long  (+32 signed)


OPC opctbl[] = {                                                               // 113 entries
{ 0 ,  0, 0,   0, 0, 0,  0,  0,  0,  0, 0     ,"!INV!" , empty },             // zero for invalid opcodes

{ 3 ,  0, 0,   1, 2, 2, 15,  7, 15,  0, shl   ,"shlw"  , "\2 <<= \1;" },                // 1
{ 3 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, shl   ,"shlb"  , "\2 <<= \1;" },
{ 3 ,  0, 0,   1, 2, 2, 31,  7, 31,  0, shl   ,"shldw" , "\22 <<= \1;" },

{ 4 ,  0, 0,   1, 2, 2, 15,  7, 15,  0, shr   ,"shrw"  , "\2 >>= \1;" },                // 4
{ 4 ,  0, 0,   1, 2, 2, 47,  7, 47,  0, shr   ,"asrw"  , "\22 >>= \1;" },
{ 4 ,  0, 0,   1, 2, 2, 31,  7, 31,  0, shr   ,"shrdw" , "\22 >>= \1;" },               // 6
{ 4 ,  0, 0,   1, 2, 2, 63,  7, 63,  0, shr   ,"asrdw" , "\22 >>= \21;" },
{ 4 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, shr   ,"shrb"  , "\2 >>= \1;" },                // 8
{ 4 ,  0, 0,   1, 2, 2, 39,  7, 39,  0, shr   ,"asrb"  , "\22 >>= \1;" },

{ 5 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, andx  ,"an2b"  , "\4\2 &= \1;"  },              // 10
{ 5 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, andx  ,"an2w"  , "\4\2 &= \1;" },
{ 5 ,  0, 0,   1, 3, 3,  7,  7,  7,  7, andx  ,"an3b"  , "\4\3 = \2 & \1;"  },
{ 5 ,  0, 0,   1, 3, 3, 15, 15, 15, 15, andx  ,"an3w"  , "\4\3 = \2 & \1;" },

{ 8 ,  1, 0,   1, 2, 2, 15,  7,  7,  0, mlx   ,"ml2b"  , "\22 *= \1;" },                // 14
{ 8 ,  0, 0,   1, 2, 2, 47, 39, 39,  0, mlx   ,"sml2b" , "\22 *= \1;"},
{ 8 ,  1, 0,   1, 2, 2, 31, 15, 15,  0, mlx   ,"ml2w"  , "\22 *= \1;"  },
{ 8 ,  0, 0,   1, 2, 2, 63, 47, 47,  0, mlx   ,"sml2w" , "\22 *= \1;"},

{ 8 ,  1, 0,   0, 3, 3, 15,  7,  7,  7, mlx   ,"ml3b"  , "\23 = \22 * \1;"  },          // 18
{ 8 ,  0, 0,   0, 3, 3, 47, 39, 39, 15, mlx   ,"sml3b" , "\23 = \22 * \1;" },
{ 8 ,  1, 0,   0, 3, 3, 31, 15, 15, 15, mlx   ,"ml3w"  , "\23 = \22 * \1;"  },
{ 8 ,  0, 0,   0, 3, 3, 63, 47, 47, 15, mlx   ,"sml3w" , "\23 = \22 * \1;" },

{ 6 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, add   ,"ad2b"  , "\2 += \1;" },                 // 22
{ 6 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, add   ,"ad2w"  , "\2 += \1;" },
{ 6 ,  0, 0,   1, 3, 3,  7,  7,  7,  7, add   ,"ad3b"  , "\3 = \2 + \1;"  },
{ 6 ,  0, 0,   1, 3, 3, 15, 15, 15, 15, add   ,"ad3w"  , "\3 = \2 + \1;" },

{ 7 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, sub   ,"sb2b"  , "\2 -= \1;" },                 // 26
{ 7 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, sub   ,"sb2w"  , "\2 -= \1;" },
{ 7 ,  0, 0,   1, 3, 3,  7,  7,  7,  7, sub   ,"sb3b"  , "\3 = \2 - \1;"  },
{ 7 ,  0, 0,   1, 3, 3, 15, 15, 15, 15, sub   ,"sb3w"  , "\3 = \2 - \1;" },

{ 9 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, orx   ,"orb"   , "\4\2 |= \1;" },               // 30
{ 9 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, orx   ,"orw"   , "\4\2 |= \1;" },
{ 9 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, orx   ,"xorb"  , "\4\2 ^= \1;" },
{ 9 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, orx   ,"xrw"   , "\4\2 ^= \1;" },

{ 10,  0, 0,   1, 2, 0,  0,  7,  7,  0, cmp   ,"cmpb"  , empty },                       // 34
{ 10,  0, 0,   1, 2, 0,  0, 15, 15,  0, cmp   ,"cmpw"  , empty },

{ 11,  1, 0,   1, 2, 2,  7,  7, 15,  0, dvx   ,"divb"  , "\22 /= \1;" },                // 36
{ 11,  0, 0,   1, 2, 2, 39, 39, 47,  0, dvx   ,"sdivb" , "\22 /= \1;"},
{ 11,  1, 0,   1, 2, 2, 15, 15, 31,  0, dvx   ,"divw"  , "\22 /= \1;" },
{ 11,  0, 0,   1, 2, 2, 47, 47, 63,  0, dvx   ,"sdivw" , "\22 /= \1;"},

{ 12,  0, 0,   0, 2, 2,  7,  7,  7,  0, ldx   ,"ldb"   , "\4\2 = \1;" },                // 40
{ 12,  0, 0,   0, 2, 2, 15, 15, 15,  0, ldx   ,"ldw"   , "\4\2 = \1;" },

{ 6 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, add   ,"adcb"  , "\2 += \1 + CY;" },            // 42
{ 6 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, add   ,"adcw"  , "\2 += \1 + CY;" },
{ 7 ,  0, 0,   1, 2, 2,  7,  7,  7,  0, sub   ,"sbbb"  , "\2 -= \1 - CY;" },
{ 7 ,  0, 0,   1, 2, 2, 15, 15, 15,  0, sub   ,"sbbw"  , "\2 -= \1 - CY;" },

{ 12,  0, 0,   0, 2, 2, 15,  7,  7,  0, ldx   ,"ldzbw" , "\22 = \21;" },                // 46
{ 12,  0, 0,   0, 2, 2, 47,  7,  7,  0, ldx   ,"ldsbw" , "\22 = \21;" },

{ 13,  0, 0,   0, 2, 1,  7,  7,  7,  0, stx   ,"stb"   , "\1 = \2;" },                  // 48
{ 13,  0, 0,   0, 2, 1, 15, 15, 15,  0, stx   ,"stw"   , "\1 = \2;" },

{ 14,  1, 0,   0, 1, 0,  0, 15,  0,  0, pshw  ,"push"  , "push(\1);" },                 // 50
{ 14,  0, 1,   0, 1, 0,  0, 15,  0,  0, pshw  ,"pusha" , "pusha(\1);" },
{ 15,  1, 0,   0, 1, 1, 15, 15,  0,  0, popw  ,"pop"   , "\1 = pop();" },               // 52
{ 15,  0, 1,   0, 1, 1, 15, 15,  0,  0, popw  ,"popa"  , "\1 = popa();" },

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jnst"  , "if (STK = 0) \x9" },          // 54  to 71 cond jumps
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jst"   , "if (STK = 1) \x9" },          // 55

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jnc"   , "\10CY = 0) \x9" },            // 56
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jc"    , "\10CY = 1) \x9" },            // 57

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jnv"   , "\10OVF = 0) \x9" },           // 58
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jv"    , "\10OVF = 1) \x9" },           // 59

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jnvt"  , "if (OVT = 0) \x9" },          // 60
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jvt"   , "if (OVT = 1) \x9" },          // 61

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jgtu"  , "\7>"   },                     // 62
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jleu"  , "\7<="  },                     // 63

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jgt"   , "\7>"       },                 // 64
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jle"   , "\7<="      },                 // 65

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jge"   , "\7>="      },                 // 66
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jlt"   , "\7<"       },                 // 67

{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"je"    , "\7="       },                 // 68
{ 19,  0, 0,   0, 1, 0,  0,  0, 0, 0, cjm   ,"jne"   , "\7!="      },                 // 69

{ 23,  0, 0,   0, 2, 0,  0, 15,  7,  7, bjm   ,"jnb"   , "if (\3\2 = 0) \x9" },         // 70
{ 23,  0, 0,   0, 2, 0,  0, 15,  7,  7, bjm   ,"jb"    , "if (\3\2 = 1) \x9" },         // 71

{ 18,  0, 0,   0, 2, 2,  7,  7,  7, 0, djm   ,"djnz"  , "\2--;\xbif (\2 != 0) \x9" },  // 72

{ 17,  1, 0,   0, 1, 0,  0, 0, 0, 0, cll   ,"call"  , "\5" },                        // 73   Long jumps
{ 17,  0, 1,   0, 2, 0,  0, 0, 0, 0, cll   ,"calla" , "\5" },                        // 74
{ 16,  0, 0,   0, 1, 0,  0, 0, 0, 0, ljm   ,"jump"  , "\x9" },                       // 75

{ 20,  1, 0,   0, 0, 0,  0, 0, 0, 0, ret   ,"ret"   , "return;" },                   // 76  returns
{ 20,  0, 1,   0, 0, 0,  0, 0, 0, 0, ret   ,"reta"  , "return;" },
{ 20,  1, 0,   0, 0, 0,  0, 0, 0, 0, ret   ,"reti"  , "return;" },
{ 20,  0, 1,   0, 0, 0,  0, 0, 0, 0, ret   ,"retia" , "return;" },                   // 79

{ 16,  0, 0,   0, 0, 0,  0, 0, 0, 0, skj   ,"skip"  , "\x9" },                       // 80 skip (= sjmp [pc+2])
{ 16,  0, 0,   0, 1, 0,  0, 0, 0, 0, sjm   ,"sjmp"  , "\x9" },                       // 81     short jumps
{ 17,  1, 0,   0, 1, 0,  0, 0, 0, 0, scl   ,"scall" , "\5" },                        // 82
{ 17,  0, 0,   0, 1, 0,  0, 0, 0, 0, scl   ,"sclla" , "\5" },                        // 83

{ 41,  1, 0,   0, 0, 0,  0, 0, 0, 0, php   ,"pushp" , "push(PSW);" },                // 84 push psw
{ 41,  0, 1,   0, 0, 0,  0, 0, 0, 0, php   ,"pshpa" , "pusha(PSW);" },
{ 42,  1, 0,   1, 0, 0,  0, 0, 0, 0, ppp   ,"popp"  , "PSW = pop();" },              // 86
{ 42,  0, 1,   1, 0, 0,  0, 0, 0, 0, ppp   ,"poppa" , "PSW = popa();" },             // 87


{ 1 ,  0, 0,   0, 1, 1,  7,  7, 0, 0, clr   ,"clrb"  ,  "\4\1 = 0;" },                // 88
{ 1 ,  0, 0,   0, 1, 1, 15, 15, 0, 0, clr   ,"clrw"  ,  "\4\1 = 0;" },

{ 2 ,  0, 0,   1, 1, 1,  7,  7, 0, 0, neg   ,"negb"  ,  "\1 = -\1;" },                // 90
{ 2 ,  0, 0,   1, 1, 1, 15, 15, 0, 0, neg   ,"negw"  ,  "\1 = -\1;" },

{ 25,  0, 0,   1, 1, 1,  7,  7, 0, 0, inc   ,"incb"  ,  "\1++;"  },                   // 92
{ 25,  0, 0,   1, 1, 1, 15, 15, 0, 0, inc   ,"incw"  ,  "\1++;" },                    // 93
{ 22,  0, 0,   1, 2, 1,  7,  7, 31, 0, nrm   ,"norm"  ,  "\1 = normalize(\22);" },
{ 2 ,  0, 0,   1, 1, 1,  7,  7, 0, 0, cpl   ,"cplb"  ,  "\1 = ~\1;" },                // 95
{ 2 ,  0, 0,   1, 1, 1, 15, 15, 0, 0, cpl   ,"cplw"  ,  "\1 = ~\1;" },

{ 24,  0, 0,   1, 1, 1,  7,  7, 0, 0, dec   ,"decb"  ,  "\1--;" },                    // 97
{ 24,  0, 0,   1, 1, 1, 15, 15, 0, 0, dec   ,"decw"  ,  "\1--;"  },
{ 22,  0, 0,   1, 1, 1, 47,  7, 0, 0, sex   ,"sexb"  ,  "\21 = \1;" },
{ 22,  0, 0,   0, 1, 1, 63, 15, 0, 0, sex   ,"sexw"  ,  "\21 = \1;" },                // 100


{ 21,  0, 0,   1, 0, 0,  0, 0, 0, 0, stc   ,"stc"   ,  "CY = 1;" },                  // 101
{ 21,  0, 0,   1, 0, 0,  0, 0, 0, 0, clc   ,"clc"   ,  "CY = 0;" },                  // 102
{ 43,  0, 0,   0, 0, 0,  0, 0, 0, 0, die   ,"di"    ,  "interrupts OFF;" },
{ 44,  0, 0,   0, 0, 0,  0, 0, 0, 0, die   ,"ei"    ,  "interrupts ON;" },
{ 48,  0, 0,   0, 0, 0,  0, 0, 0, 0, clv   ,"clrvt" ,  "OVT = 0;" },
{ 47,  0, 0,   0, 0, 0,  0, 0, 0, 0, nop   ,"nop"   ,  empty },

{ 45,  0, 1,   0, 0, 0,  0, 0, 0, 0, bka   ,"regbk"  ,  empty },                     // 107
{ 45,  0, 1,   0, 0, 0,  0, 0, 0, 0, bka   ,"regbk"  ,  empty },
{ 45,  0, 1,   0, 0, 0,  0, 0, 0, 0, bka   ,"regbk"  ,  empty },                     // 109
{ 45,  0, 1,   0, 0, 0,  0, 0, 0, 0, bka   ,"regbk"  ,  empty },                     // 110

{ 46,  0, 1,   0, 1, 0,  0, 7, 0, 0, 0     ,"rombk"  ,  empty },                     // 111 bank prefix (8065)
{ 40,  0, 0,   0, 0, 0,  0, 0, 0, 0, 0     ,"!PRE!"  ,  empty }                      // 112 signed prefix

};


// extra opcode pseudo sources for replace of R0 opcodes,
// carry, ldx, goto

const char *scespec[] = {
  "\x2 += CY;",         // R0 + carry replace
  "\x2 -= CY;",
  "\2 = \1;",           // LDX for reduced 3 op, XOR
  "\x9"                 // 'bare' goto 
 };

//  for swopping over cmp (R0, x) operands

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

// for the dummy cmp R0,0 or cmp Rx Rx etc

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
  * start, end   address(es) of interrupt (+ 0x2010)
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
// sizes are for function and table subroutine names as opcode table

AUTON anames [] = {
 { 0,      0, 0, empty     },          // 0 null entry
 { 0x80 ,  0, 0, "Sub"     },          // 1   PPROC   auto proc names (func)
 { 0x10 ,  0, 0, "Lab"     },          // 2   PLABEL  label names (lab)
 { 0x200,  0, 0, "Func"    },          // 3   PFUNC   function data names
 { 0x2  ,  0, 0, "Table"   },          // 4   PTABL   table data names

 { 0x4  ,   7,  7, "UUYFuncLU" },      // 5   PSTRS   auto subroutine names for function and table lookup procs
 { 0x4  ,   7, 39, "USYFuncLU" },
 { 0x4  ,  39,  7, "SUYFuncLU" },
 { 0x4  ,  39, 39, "SSYFuncLU" },

 { 0x4  ,  15, 15, "UUWFuncLU" },        // 9
 { 0x4  ,  15, 47, "USWFuncLU" },
 { 0x4  ,  47, 15, "SUWFuncLU" },
 { 0x4  ,  47, 47, "SSWFuncLU" },

 { 0x4  ,   7,  7, "UYTabLU"  },         // 13
 { 0x4  ,  39, 39, "SYTabLU"  },

 { 0    ,   7,  7, "Timer"    }          // 15   used as a SPECIAL for timers.
 };



/***** Auto default symbols *******

 * added to analysis if PRSYM set
 * fixed names for special regs as Ford Handbook
 * Params in order -
 * for 8061
 * for 8065 (both can be set)
 * read sym 0, write 1
 * whole 0, bit 1
 * bitnumber (0-7)
 * address
 * name
*/


DFSYM defsyms [] = {

 {1, 0, 0,  6,  6, 0x2, "CPU_OK"   },      // 8061 only and not sure if this should be kept.
 {1, 1, 0, 32, 32, 0x2, "LSO_Port"    },
 {1, 1, 0, 32, 32, 0x3, "LIO_Port"    },
 {1, 1, 0, 32, 32, 0x4, "AD_Low"      },
 {1, 1, 0, 32, 32, 0x5, "AD_High"     },
 {1, 1, 0, 32, 32, 0x6, "IO_Timer"    },
 {1, 0, 0, 32, 32, 0x7, "IO_Timer_Hi" },       // 8061 only
 {0, 1, 0, 32, 32, 0x7, "AD_Timed_Lo"  },      // 8065 only
 {1, 1, 0, 32, 32, 0x8, "INT_Mask"     },
 {1, 1, 0, 32, 32, 0x9, "INT_Pend"     },

 {1, 1, 0,  0,  0, 0xa, "HSO_OVF"      },      // both, bit 0
 {1, 1, 0,  1,  1, 0xa, "HSI_OVF"      },
 {1, 1, 0,  2,  2, 0xa, "HSI_Ready"    },
 {1, 1, 0,  3,  3, 0xa, "AD_Ready"     },      // both, bit 3
 {1, 0, 0,  4,  4, 0xa, "INT_Handling" }  ,           // 8061 only
 {0, 1, 0,  4,  4, 0xa, "MEM_Expand"    },            // 8065 only
 {1, 0, 0,  5,  5, 0xa, "INT_Priority"  },
 {0, 1, 0,  5,  6, 0xa, "HSO_Pcnt_OVF"  },

 {0, 1, 0,  6,  6, 0xa, "Timed_AD_Ready"},  // bits 6,7 not used in 8061
 {0 ,1, 0,  7,  7, 0xa, "HSO_Port_OVF"  },


 {1, 1, 0, 32, 32, 0xa, "IO_Status"     },
 {1, 1, 0, 32, 32, 0xb, "HSI_Sample"    },
 {1, 1, 0, 32, 32, 0xc, "HSI_Mask"  },
 {1, 1, 0, 32, 32, 0xd, "HSI_Data"  },
 {1, 1, 0, 32, 32, 0xe, "HSI_Time"  },
 {1, 0, 0, 32, 32, 0x10, "StackPtr"   },        // 8061 stack

 {0, 1, 0, 32, 32, 0xf, "AD_Timed_HI" },       // with 007 above


 {0, 1, 0, 32, 32, 0x10, "HSO_IntPend1" },        // 8065 only
 {0, 1, 0, 32, 32, 0x11, "BANK_Select"  },
 {0, 1, 0, 32, 32, 0x12, "HSO_IntMask1" },
 {0, 1, 0, 32, 32, 0x13, "IO_Timer_Hi"  },
 {0, 1, 0, 32, 32, 0x14, "HSO_IntPend2" },
 {0, 1, 0, 32, 32, 0x15, "LSSI_A"       },
 {0, 1, 0, 32, 32, 0x16, "HSO_IntMask2" },
 {0, 1, 0, 32, 32, 0x17, "LSSI_B"       },
 {0, 1, 0, 32, 32, 0x18, "HSO_Status"   },
 {0, 1, 0, 32, 32, 0x19, "LSSI_C"       },
 {0, 1, 0, 32, 32, 0x1a, "HSI_Mode"     },
 {0, 1, 0, 32, 32, 0x1b, "HSO_UsedCnt"  },
 {0, 1, 0, 32, 32, 0x1c, "HSO_TimeCnt"  },
 {0, 1, 0, 32, 32, 0x1d, "LSSI_D"       },
 {0, 1, 0, 32, 32, 0x1e, "HSO_LastUsed" },
 {0, 1, 0, 32, 32, 0x1f, "HSO_SlotSel"  },
 {0, 1, 0, 32, 32, 0x20, "StackPtr"     },        // 8065 stacks
 {0, 1, 0, 32, 32, 0x22, "StackPtr2"    },

 {1, 1, 1, 32, 32, 0x4, "AD_Cmd"       },           // Write symbols from here
 {1, 1, 1, 32, 32, 0x5, "WDG_Timer"    },
 {0, 1, 1, 32, 32, 0x7, "AD_Timed_Cmd" },
 {0, 1, 1, 32, 32, 0xb, "IDDQ_Test"    },
 {1, 1, 1, 32, 32, 0xd, "HSO_Cmd"      },
 {0, 1, 1, 32, 32, 0xe, "HSO_Time"     },
 {0, 1, 1, 32, 32, 0x15, "LSSO_A"      },
 {0, 1, 1, 32, 32, 0x17, "LSSO_B"      },
 {0, 1, 1, 32, 32, 0x19, "LSSO_C"      },
 {0, 1, 1, 32, 32, 0x1d, "LSSO_D"      },

};


 /**************************************************
 * main global variables
 **************************************************/

//time_t  timenow;

HDATA fldata;                 // files data holder (declared in shared.h)

/* bank layout.
* Full bank malloced to each relevant bkmap[bankno].fbuf, as 0-0xffff, for max of 16 banks.
* Regs (< 0x400) are mapped to Bank 8. Single banks are mapped completely in bank 8.
*/

BANK bkmap[BMAX];               // 16 banks max.

// opcode and data bit arrays - each 0x8000 bytes malloced

uint  *opcbit;       // opcode start
//uint  *opdbit;       // opcode data

// bit array for RBINV. Allows for 0-0x400 only
// speeds up rbase searches, as most are invalid

uint  rbinv[0x20];

/* positions of print columns in printout
*
* 26      opcode mnemonic starts (after header)
* 32      operands start             (+6)
* 49      pseudo source code starts  (+17)
* 86      comment starts             (+37)
* 180     max width of page - wrap here
*/

#define MNEMPOSN   cposn[1]
#define OPNDPOSN   cposn[2]
#define PSCEPOSN   cposn[3]
#define CMNTPOSN   cposn[4]
#define WRAPPOSN   cposn[5]

// [0] is for 'modified' marker for print

const uint stdpos[6] = {0, 26, 32, 49, 83, 180};         // standard posns

uint cposn [6] = {0, 26, 32, 49, 83, 180};               // modifiable by cmd

#ifdef XDBGX
  // debug counters various
  int DBGrecurse  = 0;
  int DBGnumops   = 0;          // num of opcodes
#endif

// number files, does not include sad.ini (= entry [6]), but does include debug file.

#ifdef XDBGX
  int numfiles = 6;
#else
  int numfiles = 5;
#endif


uint  tscans  = 0;            // total scan count, for safety
int   xscans  = 0;            // main loop (for investigation)
uint  lowscan = 100000;       // rescan from here in scan_all
int   opcnt   = 0;            // opcode count for emulate
int   cmdopts    = 0;         // command options
int   anlpass    = 0;         // number of passes done so far

int   numbanks;               // number of banks-1  (so 8061 is always 0)

//int indent;               // TEST for code print layout
uint  gcol;                   // where main print is at (no of chars = colno)
uint  lastpad;                // last padded to (column)
uint  lastcmt;                // last cmt state for end
uint  wcol;                   // where warning output is at

BASP basepars;

CMNT  fcmnt;               // comments holder from cmnts file
//CMNT  acmnt;               // auto comments holder

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

char  flbuf [256];            // for directives and comment file reads
char  nm    [128];            // for temp name construction, string hacking, etc.
uint  plist [32];             // generic param list and column positions for structs
SYM  *bitnames[16];           // sym names for bitwise ops

FSTK  scanstack[STKSZ];       // fake stack holder for previous subr calls (for arg tracking)
FSTK  emulstack[STKSZ];       // fake stack for emulation - emulate all parameter/argument getters
CPS   cmnd;                   // command structure


int recurse  = 0;             // recurse count check



//********** file extensions and flags for linux and Win32 ********************

#ifdef __linux__

// linux (gcc)

  HOP fd[] ={{".bin","r"},    {"_lst.txt","w"}, {"_msg.txt","w"},
             {"_dir.txt","r"},{"_cmt.txt","r"}, {"_dbg.txt","w"}, {"sad.ini","r"}};

 #define PATHCHAR  '/'

#endif

#if defined __MINGW32__ || defined __MINGW64__

// windows (CodeBlocks, mingw)

   HOP fd[] ={{".bin","rb"},    {"_lst.txt","wt"}, {"_msg.txt","wt"},
              {"_dir.txt","rt"},{"_cmt.txt","rt"}, {"_dbg.txt","wt"}, {"sad.ini","rt"}  };

 #define PATHCHAR  '\\'

#endif


/************************
 * print routines
 **************************/

uint wnprt (uint nl, const char *fmt, ...)
{  // warning file prt

  uint chars;
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

float rnd(float fv)
{

//  round a float in 3 decimal places
// need varable places............
// for digits d=1; (none), d *10; etc.

  int c, r, m;

  m = fv * 1000;   // round down to integer (= NO OF DIGITS)
  c = m%10;        // lowest digit
  r = m/10;        // div by ten for round up
  if(c >= 5)       // round up
  r++;
  fv = (float) r/100;    //restore float   
  return fv;
}

int sprtfl(float fv, int pfw)
{
  // print float to nm holder array
  char *tx, *s, p;
  int cs;

  if (pfw) p = ' '; else p = '\0';  // zero or space
  s = nm+64;                        //move after other strings
  cs = sprintf(s,"%*.3f", pfw, rnd(fv));
  tx = s + cs-1;
  
  // trim/replace trailing zeroes and dot if no digits.
  
  while (*tx == '0') *tx-- = p;
  if (*tx == '.') *tx-- = p;
  if (pfw) return cs;
  return tx-s;                   // chars 'printed'
}

uint flprt(const char *fmt, float fv, uint (*prt) (uint,const char *, ...))
 {
  // separate float print, to control width
  // 'prt' argument allows print to debug or message files

  uint cs;
  prt(0,fmt);
  cs = sprtfl(fv,0);
  prt(0,"%s ",nm+64);
  return cs;
}

void p_pad(uint c)
{
// List file.  Pad out to required column number if not already there
// and not already padded to. Add spaces if pstr (gcol) > lastpad 
uint i;

  if  (c <= gcol && gcol > lastpad) c = gcol+2;          // add 2 spaces...
  for (i = gcol; i < c; i++)  fputc(' ',fldata.fl[1]);   // fprintf(fldata.fl[1], " ");
  gcol = c;
  lastpad = c;                         // last padded column
}

void wrap()
 {
  // newline and wrap back to comments column if already greater,
  // or lastpad column if not
  fprintf(fldata.fl[1], "\n");
  gcol = 0;
  if (lastpad > CMNTPOSN)  lastpad = CMNTPOSN;
  p_pad(lastpad);
 }

void p_run(char c, uint num)
{
 uint i;
 for (i = 0; i < num; i++) fprintf(fldata.fl[1], "%c",c);
 gcol +=num;
 if (gcol >= WRAPPOSN) wrap();
}






void pchar(char z)
{
 fputc(z,fldata.fl[1]);
 gcol++;
 if (gcol >= WRAPPOSN) wrap();
}

uint pstr (uint nl,const char *fmt, ...)
{   // List file print
  va_list argptr;
  uint chars;
  chars = 0;

  if (fmt)
   {
    va_start (argptr, fmt);
    chars = vfprintf (fldata.fl[1], fmt, argptr);
    va_end (argptr);
    if (gcol >= WRAPPOSN) wrap();
   }
  gcol += chars;

  if (nl) gcol = 0;         // newline  
  while (nl--) fprintf(fldata.fl[1], "\n");
  return chars;
}

uint pbin(uint val, uint fend)
{  // print val in binary
   // not sure about pfw yet ...
  uint i, tval, pfw;    //, places;

/*  tval = val;
  i = 1;
  places = 0;

// where is 
 while (tval)
  {  // find ms '1' bit
    if (tval & 1)  places = i;
    tval >>= 1;
    i++;
  }

 if (pfw < cnv[size].bytes*8) pfw = cnv[size].bytes*8;  //min size
 if (pfw < places) pfw = places;   */

//simpler

if (val & 0xf000)  pfw = 16; 
else if (val & 0x0f00)  pfw = 12;
else if (val & 0x00f0)  pfw = 8;
//else if (val & 0x000f)  pfw = 4;
else pfw = 4;                // temp

 tval = val;
 for (i = 16; i > 0; i--)
  {  // find ms '1' bit
    if (i <= pfw)
      {
       if ((i/4*4) == i) pchar(' ');    //space every 4
       if (tval & 0x8000)  pchar('1'); else pchar('0');
      } 
    tval <<= 1;
  }

return pfw; 

}

/*
uint pdec(int val, uint size)
{
  // print val in decimal can be -ve
   // not sure about pfw yet ...
  uint i, pfw, base;    //, places;
  int tval;
 const char digits[] = "01234567890ABCDEF";
 char *p;

*  tval = val;
  i = 1;
  places = 0;

// where is 
 while (tval)
  {  // find ms '1' bit
    if (tval & 1)  places = i;
    tval >>= 1;
    i++;
  }

 if (pfw < cnv[size].bytes*8) pfw = cnv[size].bytes*8;  //min size
 if (pfw < places) pfw = places;   /

//simpler


 base = 10;
 tval = val;
 p = nm+127;
 *p-- = 0;

 do
  {  // have to build digits backwards !!
    i = tval % base;
    tval /= base;
    *p-- = digits[i];
  } while (tval);
 pstr (0,"%s",p+1);


*
if (i <= pfw)
      {
       if ((i/4*4) == i) pchar(' ');    //space every 4
       if (tval & 0x8000)  pchar('1'); else pchar('0');
      } 
    tval <<= 1;
  }

return pfw; /

}
*/



void paddr(uint addr,uint sp, uint (*prt) (uint,const char *, ...) )
{
  // printer for addresses with or without bank.
  // if single bank or no bank drop bank
  // For listing or debug output

 if (!numbanks || g_bank(addr) == 0)
     prt(0,"%x", nobank(addr));       // no leading zero
 else
    {    // bank+1 used internally, leading zeroes
     prt(0,"%0x", (addr >> 16) -1);
     prt(0,"%04x", nobank(addr));
    }
  while (sp--) prt(0," ");
}



 // master exit and cleanup routines

void closefiles(void)
{
    // flush and close all files
  int i;

  for (i=0; i < numfiles; i++)         // include sad.ini
   {
    if (fldata.fl[i])         // close file
       {
         fflush (fldata.fl[i]);
         fclose (fldata.fl[i]);
         fldata.fl[i] = NULL;
       }
   }
  fldata.fillen = 0;             //flag marker.

}

// 32 bit flag setters (int array)

void set_flag(uint addr, uint *x)
 {
  uint bit;

  addr = nobank(addr);
  bit = addr & 0x1F;       // 32 bits
  addr = addr / 32;
  x[addr] |= (1 << bit);
 }

uint get_flag (uint addr, uint *x)
{
  uint bit;

  addr = nobank(addr);
  bit = addr & 0x1F;
  addr = addr / 32;
  return x[addr] & (1 << bit);
}



uint val_bank(uint bk)
{
 //for bank number
  BANK *b;

  if (bk > 16) return 0;

  b = bkmap + bk;
  if (b->bok) return 1;
  return 0;
}


BANK *mapbank(uint addr)
{
 // if register, force to bank 8

  if (addr < 0x400) return bkmap + 9;
  return bkmap + ((addr >> 16) & 0xf);
}


uint bytes(uint fend)
{  //size in bytes. From field end (assumes start at 0)
 fend &= 31;         //drop sign
 if (!fend) return 0;
 
 return (fend / 8) + 1;
}


void set_opstart (uint ofst)
{
  // 1 for code start
  BANK *b;
  b = mapbank(ofst);
  if (!b->bok) return;     // invalid
  set_flag(ofst,b->opcbt);
}

uint get_opstart (uint ofst)
{
  // for code start
  BANK *b;

  b = mapbank(ofst);
  if (!b->bok) return 0;     // invalid
  return get_flag(ofst, b->opcbt);
}


/*
void set_opdata (uint ofst, uint fend)
{
  // for opcode data
  BANK *b;
  uint i;

  b = mapbank(ofst);
  if (!b->bok) return;     // invalid
  for (i = 0; i < bytes(fend); i++)
      set_flag(ofst+i,b->opdbt);
}


void set_opdatar (uint start, uint end)
{
  // for opcode data
  BANK *b;
  uint i;

  b = mapbank(start);
  if (!b->bok) return;     // invalid
  for (i = start; i <= end; i++)
      set_flag(i,b->opdbt);
}


uint get_opdatar (uint start, uint end)
{
  // for all data and opcodes
  BANK *b;
  uint i;

  b = mapbank(start);
  if (!b->bok) return 0;     // invalid
  for (i = start; i <= end; i++)
     {
      if (get_flag(i,b->opdbt)) break;
     } 
  if (i <= end) return i;
  return 0;
}

uint get_opdata (uint ofst)
{
  // for opcode data
  BANK *b;

  b = mapbank(ofst);
  if (!b->bok) return 0;     // invalid
  return get_flag(ofst, b->opdbt);
}
*/


// address handling subrs  0xfffff  (4 bits bank, 16 bits addr in bank)


uint minadd(uint addr)
{  // min address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b->bok) return 0xffff;           // invalid
 return b->minpc;
}

uint maxpcadd (uint addr)
{   // max program address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b->bok) return 0;           // invalid
 return b->maxpc;
}

uint maxbkadd (uint addr)
{   // max bank address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b->bok) return 0;           // invalid

 return b->maxbk;
}


uint val_rom_addr(uint addr)
 {
 // return 1 if valid 0 if invalid.
 // allows ROM addresses only.
 // check Code, jumps, immds

 BANK *b;

 b = mapbank(addr);

 if (!b->bok) return 0;           // invalid

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // where fill starts

 return 1;        // within valid range, OK
 }

uint max_reg(void)
{
 if (P8065) return 0x3ff;
 else return 0xff;
}

uint zero_reg(OPS *o)
{
  // answers 1 if zero register
  // 8061 is R0 and R1 (max reg 0xff)
  // 8065 is 0/1, 100/1,200/1,300/1

if (!o->rgf) return 0;

if (nobank(o->addr) > max_reg()) return 0; // 8061 and safety check

if ((o->addr & 0xfe) == 0) return 1;       // allow 0 or 1 options
return 0;

}

uint val_stack_reg(uint addr)
{
 addr = nobank(addr);
 if (P8065)
    {
     if (addr == 0x20 || addr == 0x22)  return 1;
    }
  else
    {
     if (addr == 0x10)  return 1;
    }

return 0;
}  

uint is_special_reg(uint reg)
{
  reg = nobank(reg);
  if (P8065 && reg <= 0x22) return 1; 
  if (reg <= 0x10) return 1;
  return 0;
}

uint val_general_reg(uint reg)
 {
  // can't have special regs or stack
  // and must be valid register  
  reg = nobank(reg);
  if (P8065)
    {
     if (reg > 0x3ff) return 0;
     if (reg <= 0x22) return 0; 
    }
  else
    {
     if (reg > 0xff)  return 0;
     if (reg <= 0x10) return 0;
    }
  return 1;
 }

uint fix_addr_bank(uint addr)  // INST *c   ??
{
  // sort out bank number and default
  // remember bank+1 throughout

  uint x;
  x = nobank(addr);

  // < 0x400 is a register, no bank
  if (x <= max_reg()) return x;

  // single banks always 9 (databank)
  if (!numbanks)      return x | basepars.datbnk;

  // no bank and multibank, default to databank
  if (!g_bank(addr))  return x | basepars.datbnk;

  return addr;
}


uint fix_input_addr(uint addr)
{
  // force single bank addrs to bank 9,
  //otherwise add one to bank
  // almost same as fix_addr_bank

  uint x;
  x = nobank(addr);

  // < 0x400 is a register, no bank
  if (x <= max_reg()) return x;

  // single banks always 9 (databank)
  if (!numbanks)      return x | basepars.datbnk;

  // no bank and multibank, default to databank - wait, is this right ??
  // leading zero marked as bank 0x20  so  05486 is 205486
  // can't use g_bank

  if ((addr & 0xff0000) == 0x200000)
     {       // bank 0 marker
      return x | 0x10000;             // bank 0 
     }

  if (!g_bank(addr))  return x | basepars.datbnk;

  x = g_bank(addr);
  x = x + 0x10000;      // add one to bank
  return nobank(addr) | x;

}


uint val_input_bank(uint addr)
 {
  uint bk;
  // registers always OK.
  // uses internal bank, already +1
  if (nobank(addr) < max_reg()) return 1;

  bk = addr >> 16;

  if (bkmap[bk].bok) return 1;
  return 0;
 }

// ---------------------------- fieldwidth and masks etc


uint get_pfwdef(ADT *a)
{   // default print field width
 uint ans, sz, sign;

 sign = (a->fend & 32);
 sz = (a->fend & 31) - a->fstart;

// need more here for variable fields. Probably lookup array ?

 ans = 0;          // min default size

 if (sz == 7) ans = 3;
 if (sz == 15) ans = 5;
 if (sz == 23) ans = 6;
 if (sz == 31) ans = 8;

 if (sign) ans += 1;      //for -ve sign
 // if (a->flt) val+= 4;           // floating point

 return ans;
}

uint get_signmask(uint fend)
{  // calc mask for sign bit

 if (!(fend & 32)) return 0;      // not signed

 return (1 << fend);

}


uint get_sizemask(uint fend)
{  // calc size mask
 uint ans;

 // or is this (1 << (fend+1)) - 1; ??

 fend &= 31;
 ans = 1;

 while (fend--) 
  {
    ans <<= 1;
    ans |= 1;
  }

 return ans;
}


uint get_startval(uint fend)
{  // calc startval for functions
 uint ans;

 ans = get_sizemask(fend);

 if (fend < 32) return ans;

 ans >>= 1;    // one bit less for sign bit

return ans;

}



uchar* map_addr(uint *addr)
 {
  // only used by g_word, g_byte, g_val.  Allow maxbk as limit (so fill can be read)
  // registers are always mapped to bank 8

  uint x;
  BANK *b;

  x = *addr;
  *addr = nobank(*addr);   // return addr without bank

  if (*addr < 0x400) return bkmap[9].fbuf;

  b = mapbank(x);

  if (!b->bok || x > b->maxbk) return NULL;

  return b->fbuf;         // valid address, return pointer

 }


//----------------------------------------------------------
// probably need more smarts in here need to know if register address ??


uint addbank(uint addr, INST *c, uint bkset)
  {
   addr = nobank(addr);
   if (addr <= max_reg()) return addr;

// redundant ???
   if (!numbanks) return addr |= basepars.datbnk;  //0x90000;  // force bank 8    //basepars.datbank ??

   if (c && c->bank) return addr | (c->bank << 16);
   return  addr | bkset;
 }


uint codebank(uint addr, INST *c)
 {
   // get BANK opcode to override default
   return addbank(addr,c,basepars.codbnk);
 }

uint databank(uint addr, INST *c)
 {
   // get BANK opcode to override default
   return addbank(addr,c,basepars.datbnk);
 }


int g_byte (uint addr)
{                    // addr is  encoded bank | addr
  uchar *b;

  b = map_addr(&addr);
  if (!b) return 0;        // safety
  return (int) b[addr];
}


uint g_word (uint addr)
{         // addr is  encoded bank | addr
  uint val;
  uchar *b;

  b = map_addr(&addr);                      // map to required bank
  if (!b)  return 0;

  val =  b[addr+1];
  val =  val << 8;
  val |= b[addr];
  return val;

 }


int sjmp_ofst (uint addr)
{  // negate value according to sign bit position
   // used ONLY for jumps
   // special 11 bit signed field.....

  int ofs;

  ofs = g_byte(addr) & 7;                // bottom 3 bits
  ofs <<= 8;
  ofs |= g_byte(addr+1);                 // and next byte
  if (ofs & 0x400) ofs |= 0xfffffc00;    // negate if sign bit set          

  return ofs;
}


int upd_ram(uint add, int val, uint fend)
 {
  int ans;
  uint i, sz;
  uchar *b;

  // not right for later bins ??

  if (add > minadd(add)) return 0;    // No writes to ROM
  if ((add & 0xfe) == 0) return 0;    // R0/1, 100/1, 200, 300 all fixed at zero

  b = map_addr(&add);
  if (!b) return 0;        // safety

  add = nobank(add);

  ans = 0;
  sz = bytes(fend);                            // size in bytes

  for (i = add; i < add + sz; i++)
    {
      uchar x;
      x = b[i];                      // old value
      b[i] = val & 0xff;
      if (x != b[i])  ans = 1;       // value has changed
      val >>= 8;
    }

  return ans;
 }


int check_argdiff(int nv, int ov)
{
  // value difference for arguments
  // nv is new value, ov is original

  int cnt;

  cnt = nobank(nv) - nobank(ov);    //is this wrong for multibanks ?
  if (cnt > 0 && cnt < 32) return cnt;

  #ifdef XDBGX
  if (nv != ov) DBGPRT(1,"Argdiff reject %x %x", nv,ov);
  #endif
  return 0;
}

uint get_pn_opstart (uint ofst, int d)
{
  // get previous or next opcode start
  // d = 0 for previous, 1 for next

  int cnt;
  uint end;
  BANK *b;

  b = mapbank(ofst);
  end = maxpcadd(ofst);
  cnt = 0;

// CAN scan THIS opcode if forwards, but not if backwards
// max possible code length is 8 (bank+fe+long index)

  while (ofst > 0 && ofst <= end)
   {
    if (d) ofst++;  else ofst--;
    cnt++;
    if (get_flag(ofst,b->opcbt)) break;
    if (cnt > 8) break;          // no opcodes here
   }

  if (ofst > end)  return 0;
  if (cnt > 8) return 0;

  return ofst;

}

uint find_opcode(int d, uint xofst, OPC **opl)
 {
  // would be simple lookup if it weren't for the prefixes.
  // d = 0 for previous, 1 for next
  int x, indx;

  *opl = 0;
  xofst = get_pn_opstart (xofst, d);      // previous or next

  if (!xofst) return 0;
  if (!val_rom_addr(xofst)) return 0;

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

void clr_inst(INST *x)
{
       memset(x,0,sizeof(INST));       // set all zero
       x->opr[1].rgf = 1;
       x->opr[2].rgf = 1;
       x->opr[3].rgf = 1;
}


void do_one_opcode(uint xofst)
  {
   tmpscan.curaddr = xofst;
   tmpscan.start   = xofst;
   tmpscan.stop    = 0;
   do_code(&tmpscan,&sinst);
  }



int find_pn_opcodes(int d, uint addr, int num, uint n, ...)     // number back from start
 {
   // finds previous or next opcodes in list (sigix) max num is NUMPARS (16)
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
          do_one_opcode(xofst);
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
       do_one_opcode(xofst);
      }
    else
      {
       clr_inst(&sinst);

//       memset(&sinst,0,sizeof(INST));
//       sinst.opr[1].rgf = 1;
//       sinst.opr[2].rgf = 1;
//       sinst.opr[3].rgf = 1;
      }
    return ans;
 }


int mark_emu(SBK *s, uint ofst)
 {
  // get the calling proc from opcode before any new args
  // and flag it as emulate reqd

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

   if (!ofst || opl->sigix != 17) 
     {
    #ifdef XDBGX
      DBGPRT(1,"Not found CALL !!");
    #endif

 return ans;
}
   // CALL,
   do_one_opcode(ofst);
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
         x->emulreqd = 1;           // this is where args attached
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



   o = c->opr+1;                     // target for increments
   if (zero_reg(o)) return;          // >addr)) return;             // not for R0

   r = get_rgstat(o->addr);

  if (!r) return;       // not found

#ifdef XDBGX
   DBGPRT(1,"R%x found pop=%d", o->addr, r->popped);
#endif

   r->ofst = o->val;                   // update to inc'ed address
   r->inc = 1;                         // set inc
   set_rgsize(r,15);                   // incs are ALWAYS word

   if (!s->scanning) return;

#ifdef XDBGX
   DBGPRT(1,"R%x check args", o->addr);
#endif

   // subroutine immediately before the ARGS is the one which needs to be
   // flagged as EMUREQD and THIS subr flagged as an ARGS getter

   if (!r->popped) return;       // not popped



   args = 0;
   
   for (i=STKSZ-1; i >= 0; i--)
      {
        t = scanstack+i;          // this is UP the scan mode call chain
        if (t->type == 1 && t->reg == o->addr && t->popped)
          {
            args = check_argdiff(o->val, t->origadd);
             if (args)
                {
                  #ifdef XDBGX
                   DBGPRT(1,"found FSTK for %x at %d", o->addr, i);
                   #endif
                   break;
                }
           }
      }

   // don't actually have to find a stack entry
   // check against zero address
   
   if (!args) args = check_argdiff(o->val, 0);

   if (args)
    {
     // flag this subr doing the arg get so that
     // calling it will trigger emu of the caller.
     #ifdef XDBGX
       if (i < 0) DBGPRT(1,"no FSTK %x (but %d args)", s->curaddr, args);
     #endif
     mark_emu(s, t->origadd);  // seems to be OK even if not found (by i)
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
  // byte size of single ADT entry
 if (!a) return 0;
 return a->cnt * (bytes(a->fend));
}


int listsize(CHAIN *x, uint start, int seq)
{
 // list size to next newline (last par).
 // may continue (via seq) from last newline
 // use chain.lastix in case it hits end

 return adtchnsize(x, start, seq, 1);

}

int totsize(CHAIN *x, uint start)
{
 // total size of aditional data chain
 // ignore newlines
 return adtchnsize(x, start, 0, 0);
}





int add_args(LBK *l, int dreg, int args)
{
  // add args in pairs (= words) to make easier sizing later
  // assume l is set up as C_ARGS already

  RST  *r;
  int size, reg;
  uint addr;

  if (!l) return -1;

  #ifdef XDBGX
  DBGPRT(1,"in add_args");
  #endif

  size = args - totsize(&chadnl,l->start);          // new args required
  reg = dreg;
  addr = l->start;

  if (size <= 0) return 0;       // bigger or match

  while (size > 0)
   {
      ADT *a;
      a = add_adt(&chadnl, l->start,256);   //new one
      if (a)
          {
#ifdef XDBGX
           DBGPRT(1,"add_adt size %d", size);
#endif
           r = get_rgarg(dreg, addr);
           if (r) reg = r->reg;
           if ((r && (r->reg & 1)) || size < 2) a->cnt = 1;
           else a->cnt = 2;  // i.e. for each byte so far

           a->dreg = reg;
           a->data |= basepars.datbnk;          // g_bank(l->start);
           dreg += a->cnt;           // next lookup
           addr += a->cnt; 
           reg  += a->cnt;           // next reg if list
           size -= a->cnt;
          }
      else break;                    // not found
   }

   l->end  = l->start+args-1;        // set size correctly
   l->size = totsize(&chadnl,l->start);             // safety check
  return 1;
}


void do_args(SBK *s, FSTK *t)
 {
    // called from pushw or stx in emulate.

    // but not if subr has args attached ?? how to get this ???

   LBK  *l;

   int args, addr;

   args = check_argdiff(t->newadd, t->origadd);        // safety check.....
   if (!args) return;

   addr = mark_emu(s,t->origadd);                     // mark calling subr for args

   if (addr)
      {       // check if subr is manually defined
       SUB *sub;
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
     l = get_cmd(addr, 0);
     if (l)
       {
        addr = l->end; // skip to end of this block
        if (l->fcom <= C_LONG) chdelete(&chcmd,chcmd.lastix,1);
       }
   }

   l = get_aux_cmd(t->origadd,0);                   // any command here ?

   if (l && l->cmd) 
    {
      #ifdef XDBGX  
        DBGPRT(0,"ARGS CMD SET for %x (%x-%x)", t->origadd, l->start, l->end);
      #endif
      return;
    }

   if (l)
     {
      if (l->fcom == C_ARGS)
        {    // a further set, probably call same getter again
             // THIS DOESN'T CHECK FOR OVERLAPS !!!
         int tsz;
         #ifdef XDBGX  
          DBGPRT(1,"Found ARGS %x-%x (%d)", l->start, l->end, l->size);
        #endif
        tsz = totsize(&chadnl,l->start);         //debug test
         if (tsz != l->size)
           {
#ifdef XDBGX
            DBGPRT(1,"PANIC - sz no match, %d %d", l->size, tsz);
#endif
           }


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
      l = add_aux_cmd(t->origadd, t->newadd-1, C_ARGS,s->curaddr);
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

// BUT with STW, c->wop could be indirect or indexed and be in [4]
// but the 'find' will always be full default range...........

  if (c->wop == 1) 
     {
       if (c->opcsub == 2 && c->opr[1].inc) o = c->opr + 4;    // c->inc move to true register
       if (c->opcsub == 3) return;                    // ignore indexed stx
     }

   if (!o->rgf) return;
   if (!val_general_reg(o->addr)) return;
   if (get_flag(o->addr, rbinv)) return;    // already marked as invalid

   b = get_rbt(o->addr,0);                 // is it an rbase (default)?

   if (b && b->cmd)
     {
       #ifdef XDBGX
        DBGPRT(1,"cmd bans rbase update %x", o->addr);
       #endif
       return;
     }

    val = databank(o->val,c);          // if val is an address, add bank

   // only add an RBASE if ldw or stw, immediate,WORD, not zero and not invalid
   // AD3w allowed for NEW rbase if other op is an rbase already (A9L 7096 ad3w) 

   if (!b)
   {
   if (c->opcsub == 1 && o->val && bytes(o->wfend) == 2)           //o->wsize == 2)
      {
       // ldw, stw or ad3w and immediate - new rbase candidate
       if (c->opcix == 41 || c->opcix == 49 || (c->opcix == 25 && c->opr[2].rbs))
        b = add_rbase(o->addr, val,0,0xfffff);
      }
   }
   else
       {   // rbase already exists, invalidate if not same value or not immediate

        if (b->val != val || c->opcsub != 1)
           {
             b->inv = 1;      // not same value
             set_flag(o->addr, rbinv);
             #ifdef XDBGX
               DBGPRT(1,"Inv rbase R%x (%x)", o->addr, c->ofst);
             #endif
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
        upd_ram(o->addr,o->val,o->wfend);

        #ifdef XDBGX

        if (anlpass < ANLPRT)
          {
           DBGPRT(0,"%05x ",s->curaddr);
           if (s->emulating) DBGPRT(0, "E");
           DBGPRT(0, "RAM %x = %x (%d)", o->addr, o->val, o->wfend);
           if (c->opcsub == 2)  DBGPRT(0, "[%x]", c->opr[4].addr);      // for indirect

           //only place wop = 1 is for stx....

           if (c->opcsub == 3 && c->wop != 1)  DBGPRT(0, "[%x]", c->opr[1].addr);      // for indexed
           if (nobank(o->addr) >= PCORG) DBGPRT(0," WROM !!");

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

     if (o->inc)    // c->inc can only be set within opcsub == 2
       {     // auto increment set - update value

        o->val += bytes(o->rfend);       // autoinc is by op size

        // o->val &= getsizemask...camk[o->ssize];    // allow for overflow ?
        upd_ram(o->addr,o->val,15);         //word update
        check_args(s,c);            // check if qualifies as subr argument

        #ifdef XDBGX
         if (anlpass < ANLPRT)
          {
           DBGPRT(0, "%05x RAM++ %x = %x (%d)",s->curaddr,  o->addr, o->val, 15);
           if (nobank(o->addr) >= PCORG)    DBGPRT(0,"  ROM !!");
           DBGPRT(1,0);
          }
        #endif
      }
    }
}


//------------------------------------------------------------------------

void prt_banks(uint (*prt) (uint,const char *, ...))
{
  int i;
  BANK *x;

  // bank no , file offset,  (opt) pcstart, (opt)  pcend  (opt) end file offset, (opt) fillstart

  if (prt == wnprt)
  {
   prt(2,"# Banks Found.  For information, can uncomment to manually override");
  // prt(1,"# bank_num, file_offset, start_addr, end addr, [filler_start]");

   for (i = 0; i < BMAX; i++)
    {
     x = bkmap+i;
     if (x->bprt) 
       {
        prt(0,"# bank  %d %5x ", x->dbank-1, x->filstrt);
        paddr(x->minpc,1,prt);
        paddr(x->maxbk,1,prt);
        if (x->cmd) prt (0,"  # by user command");

        if (!x->bok) prt (0,"** ignored **");
        prt(1,0);
       }
    }
   prt(1,0);
  }

#ifdef XDBGX
else
  {
 //  debug printout

  DBGPRT(1,0);
  DBGPRT(1,"-------DEBUG BANKS---------");
  DBGPRT(1,"bank filstart, minpc, maxbk, maxpc, filend. bok,cmd,bkmask, cbnk, P65 dbank" );

  for (i = 0; i < BMAX; i++)
    {
      x = bkmap + i;
      if (x->bok)
       {
         DBGPRT(0,"%2d %5x %5x %5x %5x (%5x)", i, x->filstrt, x->minpc, x->maxbk,x->maxpc, x->filend);
         DBGPRT(1,"  %x %x %x %x %x %x", x->bok, x->cmd,x->bkmask, x->cbnk, x->P65, x->dbank);
       }
    }
  DBGPRT(1,0);
  DBGPRT(1," Non bok ---");

  for (i = 0; i < BMAX; i++)
    {
      x = bkmap + i;
      if (!x->bok && x->minpc)
        {
      DBGPRT(0,"%2d %5x %5x %5x %5x (%5x)", i, x->filstrt, x->minpc, x->maxbk,x->maxpc, x->filend);
         DBGPRT(1,"  %x %x %x %x %x %x", x->bok, x->cmd,x->bkmask, x->cbnk, x->P65, x->dbank);
        }
     }
 DBGPRT(1,0);
}

#endif

}



void prt_cellsize(ADT *a, uint (*prt) (uint,const char *, ...))
{
char c;
uint sign, fend;
if (a->fstart)
{
}
else
{
 c = 'Y';
sign = (a->fend & 32);
fend = a->fend & 31;
 prt (0,"%c", sign ? 'S' : 'U');

 if (fend == 15) c = 'W';
 if (fend == 23) c = 'T';
 if (fend == 31) c = 'L';
 prt(0,"%c ", c);
}

}

void prt_pfw(ADT *a, uint (*prt) (uint,const char *, ...))
{
// will have subfield sizes in here
 if (a->pfw && a->pfw > get_pfwdef(a)) prt(0,"P %x ",a->pfw);
}

void prt_radix(ADT *a, int drdx, uint (*prt) (uint,const char *, ...))
{
        if ((a->prdx != drdx) || prt != wnprt)
         {      // print radix
          if (a->prdx == 1) prt(0,"X 16 ");  
          if (a->prdx == 2) prt(0,"X 10 "); 
          if (a->prdx == 3) prt(0,"X 2 "); 
         }
}


void prt_adt(CHAIN *x, uint start, int drdx, uint (*prt) (uint,const char *, ...))
{
  // print additional params for any attached ADT chain, LBK mainly
  // the 'prt' argument allows print to debug or message files
  // dcm is 1 bit

ADT *a;
int nl;

  a = start_adnl_loop(x,start);       //(ADT*) chmem(x,0);


//if (start == 0x9279c)
//{
//DBGPRT(1,0);
//}


  nl = 0;

while ((a = get_next_adnl(x,a)))
  {
    if (a->cnt || prt != wnprt)
     {    // ignore if zero size, and not debug
       if (nl) prt(0,"| "); else prt(0,": ");
       if (a->newl) nl = 1; else nl = 0;      // newline is for END of block

       if (a->ans)
        {        //subroutine answer definition
         prt(0," = %x ", a->data);
        }

       if (a->cnt > 1) prt(0,"O %d ", a->cnt);
       if (a->foff) prt(0,"D %x ", a->data);

       if (a->enc)  prt(0,"E %d %x ", a->enc, a->data & 0xff);
       else
       if (a->vaddr)
        {
         prt(0,"R ");
        }
       else
       if (a->bank) prt(0,"K %x ", a->bank-1);
       else prt_cellsize(a,prt);

       prt_pfw(a,prt);                         // print fieldwidth if not default
       prt_radix(a,drdx,prt);                  // print radix if not default

       if (a->flt) flprt("V ", a->fldat, prt);
       if (a->fnam) prt(0,"N ");

       #ifdef XDBGX
         if (prt == DBGPRT) prt (0,"[R%x, csz %d] ", a->dreg, cellsize(a));
       #endif
     }

  }
}


void prtflgs(int flgs,  uint (*prt) (uint,const char *, ...))
{
  // stop dupls by removing matched flags 
 uint i;

 for (i = 0; i < NC(mopts); i++)
   {
     if ((flgs & mopts[i]) == mopts[i])
     {
       prt(0, ": %s ", optstrs[i]);
       flgs &= ~mopts[i];
      } 
   }
}


void prt_opts(void)
{
 // print opt flags to msg file
 wnprt(0,"setopts  " );
 prtflgs(cmdopts, wnprt);
 wnprt(2,0);
}

void prt_layout(void)
{
 // print layout options
 int i;
 if (!cposn[0]) return;    //no changes

 wnprt(0,"layout  " );
 for (i = 3; i < 6; i++)
   {
    wnprt(0," %d",cposn[i]);
   }
 wnprt(2,0);
}

void prt_glo(SUB *sub, uint (*prt) (uint,const char *, ...))
{
   SPF * s;
   int x;
 
  // print special func in style of adt
  x = 0;
 
  if (!PARGC && sub->cptl)
       {
        prt (0," $ C");
        x = 1;
       }

  s = get_spf(sub->start);

  if (!s) return;          // no special funcs

  if (!x) prt(0," $"); 

  prt(0," F %s %x", spfstrs[s->spf-4],s->addrreg);
 
  // tab subroutines need extra par
  if (s->spf > 12) prt (0," %x", s->sizereg);

}


void prt_rbt ( uint (*prt) (uint,const char *, ...))
{
  RBT *r ;
  uint ix, v;

  for (ix = 0; ix < chbase.num; ix++)
   {
    r = (RBT*) chbase.ptrs[ix];

    v = get_flag(r->reg,rbinv);
// if (get_flag(o->addr, rbinv)) return;    // already marked as invalid

        if (prt == wnprt)
          {
           if (!r->inv && !v)
             {      //not invalid or flagged invalid
              prt(0,"rbase %x " , r->reg);
              paddr(r->val,1,prt);
              if (r->rstart) 
               {
                prt(0,"  ");
                paddr(r->rstart,1,prt);
                paddr(r->rend,0,prt);
               }
              if (r->cmd) prt(0,"       # cmd");
              prt(1,0);
            }
         }
        else
          {
           prt(0,"rbase %x %x" , r->reg , r->val);
           prt(0,"  %x %x", r->rstart, r->rend);
           if (r->inv) prt (0, "  INV");
           if (v) prt(0,"  INVFLAG");
           if (r->cmd) prt(0,"       # cmd");
        prt(1,0);
          }
   }
    prt(1,0);
}

void prt_scans(void)
{
  // not merged with DBG version, as it's very different.
  uint ix;
  SBK *s;

   for (ix = 0; ix < chscan.num; ix++)
    {
     s = (SBK*) chscan.ptrs[ix];
     if (s->cmd) {wnprt(0,"scan ");
     paddr(s->start,0,wnprt); 
     wnprt(1,0);}
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

    wnprt(0,"pswset ");
    paddr(r->jstart,1,wnprt);
    paddr(r->pswop ,0,wnprt);
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

  if (xsym->cmd) return 0;                  // can't rename if set by CMD
  if (!fnam || !*fnam) return 0;

  oldsize = xsym->tsize;                    // old size
  xsym->tsize = strlen (fnam) + 1;          // include null, strlen does not...

  if (xsym->tsize > SYMSZE) xsym->tsize = SYMSZE;   // max storage size is 64

  xsym->name = (char *) mem (xsym->name,oldsize,xsym->tsize);  // get new sym size

  if (xsym->name)  strncpy (xsym->name, fnam, xsym->tsize);    // and replace it
  xsym->name[xsym->tsize-1] = '\0';                            // safety

  return xsym->tsize;
 }



void prt_cmd(LBK *c, uint (*prt) (uint,const char *, ...))
{
  uint f;
  // print single LBK command

     if (prt == wnprt)
       {         // real printout
         if (nobank(c->start) < PCORG) return;    // safety
         if (c->fcom == C_TIMR) prt (0,"# ");
         prt(0,"%-7s ", cmds[c->fcom]);
         paddr(c->start,1,prt);
         paddr(c->end  ,0,prt);
       }
     else
      {   // debug printout
          prt(0,"%-7s %x %x", cmds[c->fcom], c->start,c->end);
      }

/*     if (t->namex)         // name expected
      {
       x = get_sym(0, c->start,-1,0);
       if (x)
        {
         prt(0,"  %c%s%c  ", '\"', x->name, '\"');  // TEMP
         x->noprt = 1;        // printed this symbol
        }
      }  */

   // global options here
 
   f = 0;
   if (c->term)
      {
       prt (0,"  $");    
       prt (0," Q");
       if (c->term > 1) prt(0," %d", c->term);
       f = 1;
      }

   if (c->argl)
      {
       if (!f) prt (0,"  $"); 
       prt (0," A");
      } 

   if (c->fcom == C_TABLE || c->fcom == C_FUNC) prt_adt(&chadnl, c->start,1, prt); 
   else   prt_adt(&chadnl, c->start,0, prt);

   if (c->sys) { prt(0,"     # auto");}
   prt(1,0);
  }



void prt_cmds(uint (*prt) (uint,const char *, ...))
{ 
    // command printer for both cmd and auxcmd, debug or real
    // do both chains in one pass in start order 

 uint ixc, ixa;
 LBK *cmd, *aux;

 ixa = 0;

 aux = (LBK*) chaux.ptrs[ixa];           // first aux

 for (ixc = 0; ixc < chcmd.num; ixc++)
    {
     cmd = (LBK*) chcmd.ptrs[ixc];

     while (aux && aux->start <= cmd->start)
        {
         prt_cmd(aux,prt);
         ixa++;
         if (ixa < chaux.num) aux = (LBK*) chaux.ptrs[ixa];    
         else aux = 0;
        }

     prt_cmd(cmd,prt);

    }
    prt(2,0);
}    


SYM *new_autosym (uint ix, int addr)
{

// add auto sym with address
// all of these are whole read symbols

  SYM *xsym;
  int c;

  if (anlpass >= ANLPRT) return 0;

  if (ix >= NC(anames)) return NULL;              // safety check

  if (!(cmdopts & anames[ix].mask)) return NULL;  // autoname option not set

  // make name first

  c =  sprintf(nm,   "%s", anames[ix].pname);
  // add address
  c += sprintf(nm+c, "_");
  if (numbanks)  c += sprintf(nm+c, "%x", (addr >> 16) -1);     
  c += sprintf(nm+c, "%04x", nobank(addr));
  //else           c += sprintf(nm+c, "_%05x", addr);                 // include bank

  xsym = add_sym(nm, C_SYS, addr,32, 32, 0,0xfffff);     // add new (read) symbol

  if (xsym->cmd) return xsym;               // can't change a cmd sym
    
  if (ix > xsym->xnum)
    {     //only if higher level autonumber
      new_symname (xsym, nm);
      xsym->xnum = ix;
    }

  return xsym;
}


int do_table_sizes(uint start, int scols, uint *apnd, uint maxend)
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
// so algorthm must be size independent ??


//or find a central value ? (mean/median) or modal value (most common)




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
  scols = nobank(scols);


#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"do data table sizes for %x-%x cols %d, max %x", start, apend, scols, maxend);
 #endif

 for (j = 0; j < 2; j++)
 {            // for apparent and maxend

   if (!j)  size = apend-start;
   else
     {
       if (maxend == apend) break;  else  size = maxend-start;
     }


// first find candidate sizes


  for (i = 3; i < 32; i++)
    {
      row = size/i;
      if (row*i == size && row < 32) cols |= (1<<i);
      if (i == scols) cols |= (1<<i);                      // add suggested col
    }


  if (!cols)
      {
       row = g_byte(start+size);
       if (row == 0xff)
         {  // could be terminator
           size--;
           #ifdef XDBGX
            DBGPRT(1,"terminator removed");
           #endif
 
// temp repeat fix this later............but this works so far

  for (i = 3; i < 32; i++)
    {
      row = size/i;
      if (row*i == size && row < 32) cols |= (1<<i);
      if (i == scols) cols |= (1<<i);                      // add suggested col
    }




#ifdef XDBGX
 DBGPRT(1,"no valid rows/cols for %d", size);
#endif
      } }



// find most common value first ??


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
      rsize = size/i;     //possible row size

//but this may be too many rows..........
  #ifdef XDBGX
      DBGPRT(1, "- - - - %d x %d (%d)", i, rsize, size );
  #endif

      // do rows first  (cols within each row)

      for (row = 0; row < i; row++) // for each row
        {
         addr = start+(row*rsize);              // start of this row (col zero)
         lval = g_byte(addr);                   // first col of row

         for(col = 1; col < rsize; col++)   // col in row
          {
            val = g_byte(addr+col);
            dif =  abs(val-lval);
            if (dif > rmax) rmax = dif;         // max dif for this row
            rscore += dif;                      // all rows;
            lval = val;                         // update last value
          }

        }


      // do cols (rows in each col)

      for (col = 0; col < rsize; col++)   // col in row
        {
         addr = start+col;                // start of this row
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

    tot = rdif +cdif;
     

  #ifdef  XDBGX
       DBGPRT(1,"%dx%d  rdf,cdf %d+%d/%d = %d", i,rsize,rdif,cdif,dif, tot);
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

// if score = 0 then default to common sizes.

if (ansrow > 16 || ansscore == 0) ansrow = 0;

 *apnd = (ansrow*anscol);
 return ansrow;
}

LOOP l;

void turn_dtk_into_data(void)
{

  uint ix, cix, dix, dcmd, start,lstart;    //, end;
  uint push;
  TRK *d, *n;
  DTD *a;
  LBK *k, *p;
  JMP* j, *m;
 
#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"dtk to data");
#endif

//start with all loops must be spotted via autoinc

/* for (ix = 0; ix < chdtk.num; ix++)
   {
    d = (DTK*) chdtk.ptrs[ix];
    if (d->opcsub == 1 )
     {
      lstart = 1;
       while ((a = get_dtkd(&chdtka, d, lstart)))
         {
          lstart = a->stdat+1;          //next start
          start = a->stdat;
          if (val_rom_addr(start))
             {
              p = get_cmd(start,0);
              DBGPRT(0,"%x IMD = %x ", d->ofst,a->stdat);
 if (p) DBGPRT(1," IGNORED %x-%x %s",p->start, p->end,cmds[p->fcom]); else DBGPRT(1,0);
//get cmd first.................
              
          //    DBGPRT(1,"%x-%x %s",p->start, p->end,cmds[p->fcom]);
            //  else DBGPRT(1, "***");
             }
         }
      //  DBGPRT(1,0);
      }
   }

/pushes first ??
// these are ALL INCORRECT except for 79f9 on A9L

 for (ix = 0; ix < chdtk.num; ix++)
   {
    d = (DTK*) chdtk.ptrs[ix];
  //  dcmd = bytes(d->fend);       //may add an fcom instead...

    if (d->psh)       // && d->opcsub == 1)
       { //imd push
    //    DBGPRT(0, "%x is push ", d->ofst);
        lstart = 1;

        while ((a = get_dtkd(&chdtka, d, lstart)))
         {
          lstart = a->stdat+1;          //next start
          start = a->stdat;
          if (val_rom_addr(start))
             {

              get_cmd(start,0);
              cix = chcmd.lastix;
              while (cix < chcmd.num && start) 
                { 
                  p = (LBK*) chcmd.ptrs[cix--];
                  if (p->end < start) break;             //no overlap
                  if (p->start <= start && p->end >= start) start = 0;
                } 
              if (start) DBGPRT(1,"push %x from %x",start, d->ofst);
             }
//get cmd to make sure it's not used.
         }
   //     DBGPRT(1,0);
       }
   } 
  //      add_cmd(start, start+bytes(d->fend)-1,bytes(d->fend), d->ofst);    
  //scan dtk chain and add data entries
        DBGPRT(2,0);
*/


  for (ix = 0; ix < chdtk.num; ix++)
   {
    d = (TRK*) chdtk.ptrs[ix];
    dcmd = bytes(opctbl[d->opcix].rfend1);       //may add an fcom instead...
    if (opctbl[d->opcix].sigix == 14) push = 1; else push = 0;  

    // find attached data start(s)

    lstart = 1;

#ifdef XDBGX
    if (d->ainc) DBGPRT(1, "%x is INC++ sz %d", d->ofst, d->ainc);
    #endif   

    while ((a = get_dtkd(&chdtko, d->ofst, lstart)))
       {
        lstart = a->stdat+1;          //next start
        start = a->stdat;


  //      add_cmd(start, start+bytes(d->fend)-1,bytes(d->fend), d->ofst);       // add data entry TEMP

#ifdef XDBGX
        DBGPRT(1,"%x dat=%x opc %d",d->ofst, a->stdat, d->opcsub);
#endif
        // get nearest command, for overlap checks with data

      //  end = start + bytes(d->fend)-1;      //end of this data item
        p = get_cmd(start,0);
 
            if (p && p->start <= start && p->end >= start)
             {
               //overlap - kill if matches criteria

               if (p->fcom == dcmd)
                {
#ifdef XDBGX
                 DBGPRT(1,"duplicate command %x %s", start, cmds[dcmd]);
#endif
                 start = 0;                         // kill insert
                 break;
                }
             // words/bytes/longs within func/tab (+ struct/timer for now). args ?? - different chain)
               if ( dcmd >= C_BYTE && dcmd <= C_LONG && p->fcom >= C_TABLE && p->fcom <= C_TIMR)
                {
#ifdef XDBGX
                 DBGPRT(1,"overlap struct %x %s", start, cmds[dcmd]);
#endif
                 start = 0;
           //      a->olp = 1;
                 break;
                } 
               else
                {
#ifdef XDBGX
                 DBGPRT(1,"overlap %x %s from %x with %x-%x %s", a->stdat, cmds[dcmd], d->ofst, p->start, p->end,cmds[p->fcom]);
#endif
                 if (p->fcom == C_CODE && !push)         //push
                   {
#ifdef XDBGX
                     DBGPRT(1, " **** CODE ***");
#endif
//a->inv = 1;                  // temp..........
if (p->end-start < 16 )  //&& start > 0x9201f)
      {
#ifdef XDBGX
        DBGPRT(1,"inx?");
#endif
       }

                   }
                //and extra work to go here..............
                 break;
                }
             }
     //     }          //  end overlap check
      


// if imd, then look for a To at d->ofst+d->ocnt, for a loop with start at...
// AA has start and END, to be careful of...check by using inc....



        if (start && d->ainc) 
          { 
            // inc is set, look for a loop. may be better after an imd ??? will be before the inc ??
            j = 0;
            cix = get_jump(&chjpf,d->ofst);       // from jump (after inc opcode)
            while (cix < chjpf.num)
             {
               m = (JMP*) chjpf.ptrs[cix];
               if (m->toaddr > d->ofst) break;      // must span d->ofst
               if (m->toaddr <= d->ofst && m->toaddr != m->fromaddr &&m->fromaddr >= d->ofst && m->back) j = m;
               cix++;



    // next jump ? prev jump?
          //          if (dix < chjpt.num) j = (JMP*) chjpt.ptrs[dix]; else break;
                   
             }

           //    j = (JMP*) chjpt.ptrs[cix];   // candidate jump


               if (j && j->back)          // && j->fromaddr > d->ofst)
                 {  //loop spans this increment.
                    //new subr ??
                    //need to check loop size too, A9L JUMPS to SUBRS....
#ifdef XDBGX
                   DBGPRT(1,"Loop? %x (%x-%x)", d->ofst, j->toaddr,j->fromaddr);
#endif
                  // set up loop holder
                   memset(&l,0,sizeof(LOOP));              //clear loop struct
                   l.increg = d->rgvl[1];
                   l.incr = d->ainc;
                   l.start = j->toaddr;                  // where loop starts
                   l.end = j->fromaddr;

                   dix = ix;

while (dix && dix < chdtk.num)
 {
  DTD *b; 
   n = (TRK*) chdtk.ptrs[dix--];

   if (n->ofst < l.start-32) break;
   if (n->opcsub == 1 && n->rgvl[2] == l.increg)
     {
       b = get_dtkd(&chdtko, n->ofst, 1);
     if (b) {  l.datastart = b->stdat;
#ifdef XDBGX
       DBGPRT(1,"loop imd %x = %x", n->ofst, b->stdat);
#endif
       break;}
     }     
  }



}

}
/*go back to find start address of designated register..............
// so this must be FROM ordered..............



// can use ix here for further loop
  //     n = (DTK*) chdtk.ptrs[ix];





 
       while (d->ofst < l.end && ix < chdtk.num)
          {     //inside loop
            ix++;
            n = d;
            d = (DTK*) chdtk.ptrs[ix];  
             if (d->ainc && d->ofst != n->ofst && d->rreg == n->rreg) l.incr += d->ainc;
             else
               {
                 if (!l.incr2) l.incr2 = d->start[0]-n->start[0];
               //  else if (l.incr2 != d->start[0]-n->start) DBGPRT(1,"inc mismatch");
                }
      //       DBGPRT(1,"L %x", d->from);
          }
          
            DBGPRT(1,"loop analyse %x-%x for R%x inc %x %x start %x",l.start,l.end, l.increg, l.incr, l.incr2, l.datastart);         
     //  ix--;   // to stay at end of loop

// and HERE is where to find start, end etc conditions ??
// inc should be FIRST in loop.............
// if jump is STAT then need to find exit condition
        }
     }


*/
// else DBGPRT(1,"no p");



    if (val_rom_addr(start) && !push && d->opcsub > 1)
       {
//ignore reg to reg and immds
          //split out add_cmd for ix value fo range checks ? or just do bfindix ??

        k = add_cmd(start, start+dcmd-1,dcmd, d->ofst);       // add data entry

        if (!k)
          {
#ifdef XDBGX
           DBGPRT(1,"no cmd [%d] (%x %s)", ix, start, cmds[dcmd]);
#endif
          }
       }

   } // end while 'a'


}          //master 'for'



}       // end of subr









/*
void add_opr_data(INST *c)
{
  OPS *o;
  LBK *k;

// THIS ONE  to change to TRACKING data ??


 // LBK *k;
  int addr, sig;             //x

  if (anlpass >= ANLFFS) return;
 // if (s->nodata) return;


  addr = 0;
  sig = c->sigix;

  if (sig == 14)  return;    // not push



 --- opcsub == 1 for immediates with valid ROM / RAM address ??
 // how to tell what's real though ? Assume it always calls a indirect ??


  if (c->opcsub == 1)
  {     //ldx or add with immediate only. ALSO check it's not a cmp limit check ?
    if (sig == 12 || sig == 6)
      {
       o = c->opr+1;

    //   if (val_rom_addr(o->addr))  addr = o->addr ; 

    //   if (c->wop == 1) addr = o->addr;

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
    }         /


// indirect but NOT for writes (STX) or zero wops

  if (c->opcsub == 2)
   {
     o = c->opr+1;
     if (c->wop > 1) addr = o->addr;

  //   if (c->inc)
 //    { //struct ?
 //    DBGPRT(1,"INC++ STRUCT?");
 //    }
   }

 // indexed

  if (c->opcsub == 3)
  {
   o = c->opr+4;                         // = saved opr[1]

 // if (!o->wsize)
 // {
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
  // }
  }


  if (addr)
     {            // addr must be val_rom_addr
      k = add_cmd (addr,  addr+bytes(o->rfend)-1, bytes(o->rfend), c->ofst);       // add data entry
  //    if (k)
 //      {    // add offset and type for later checks
 //        k->opcsub = c->opcsub;
    //    if (c->opcsub == 3 && c->opr[4].addr)
    //    if (c->opr[4].val > 0 && c->opr[4].val < 17)
    //    k->soff = c->opr[4].val;                // offset 0-16
  //     }
     }
 }


}
}
}
}
*/






void turn_scans_into_code (void)
 {

  // turn scan list into code commands.

  uint ix;
  SBK *s, *t;

  for (ix = 0; ix < chscan.num; ix++)
   {
    s = (SBK*) chscan.ptrs[ix];
    if (s->inv)
     {
       // inv block check.  if another block overlaps this one.
       // may be a run-on due to faulty args or a stack shortcut, etc.
       // also for debugs, can add code up to end, if it doesn't overlap with anything
       //s->nextaddr will be address AFTER the inv
       // ...THIS IS A HORRIBLE KLUDGE !!! but it works...
       if (ix < chscan.num-1)
         {
          t = (SBK*) chscan.ptrs[ix+1];
          if (!t->inv && t->start < s->nextaddr)
            {
             // next block not invalid and overlaps this one
             #ifdef XDBGX

             DBGPRT(1,"Invalid scan %x-%x overlapped by %x-%x",s->start, s->nextaddr,t->start, t->nextaddr);
             #endif
             add_cmd (s->start,t->nextaddr,C_CODE,0);
             s = NULL;                  //continue;                                 // return to loop
           }
       }        // ix < chscan.num
     }         // s->inv
    else
    if (s && (s->stop))
      {   // valid block to turn to code, not inv
       add_cmd (s->start,s->nextaddr,C_CODE,0);
      }
   }            // for loop
 }


void end_scan (SBK *s)
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

/*************************************
Multiple scale generic get value.
for varibale bit fields (0-31)
Auto negates via sign (fend & 32) if required
***********************************/

int g_val (uint addr, uint fstart, uint fend)
{                   // addr is encoded bank | addr
  int val, start,stop;
  uint mask, sign;
  uchar *b;
  uchar *t;

  b = map_addr(&addr); 

  if (!b)  return 0;               // map to required bank
  t = (uchar*) &val;

  fstart &= 31;                    // max field
  sign = (fend & 32);
  fend &= 31;

  if (fstart > fend) return 0;     // safety

  stop   = fend   /8;
  start  = fstart /8;

  if (fend == fstart) sign = 0;         // no sign for single bit

  val = 0;

  while (start <= stop)
    {
     t[start] = b[addr+start];
     start++;
    }

  // do the field extract and sign

 // size = (fend - fstart) + 1;      // field width
 // mask = 0xffffffff >> (32 - size);              // make mask


  if (fstart) val >>= fstart;                  // shift down (start bit -> bit 0)
  mask = 0xffffffff >> (31 + fstart - fend);   // make mask
  val &= mask;                                 // and extract required field

  // set sign, reusing mask 
  
  if (sign)
    {
     mask = ~mask;                           // flip for set negative
     sign = 1 << fend;                       // sign mask
     if (val & sign) val = val | mask;
    }

return val;

}



int decode_addr(ADT *a, uint ofst)
  {
  int rb,val,off;
  RBT *r;

  // HERE would be where we go to the subfields setup

// ALSO have to assume that reqd bank is in (top of) data

  val = g_val (ofst, a->fstart, a->fend);   // offsets may be SIGNED

  rb  = val;
  off = 0;

  if (a->foff)
    {   // fixed offset (assume bank in a->data)
     val += a->data;
     return val;
    }

  switch (a->enc)
   {
    case 1:           // 1 and 3 - only if top bit set
    case 3:
     if (! (rb & 0x8000)) 
       {
        if (val > (int) max_reg()) val |= g_bank(a->data); 
        return val;
       }
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

 else
  if (a->vaddr)
   {    // value is  address
   // val |= g_bank(a->data);   // add bank from start addr....not necessarily right
 if (val > (int) max_reg()) val |= g_bank(a->bank); 
     // g-bank(a->addr);  ?
   }

// if (val > max_reg()) val |= g_bank(a->data); 
 // if (!numbanks && val > max_reg() ) < 0x400) val = nobank(val) | 0x80000;    //   force bank 8 for regs

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
    bk = g_bank(z->start);
    #ifdef XDBGX
    DBGPRT(1,"in avcf, start = %05x", addr);
   #endif
    while (1)
     {
        val = g_word(addr);
        val |= bk;
        if (!val_rom_addr(val)) break;
        add_scan(val, J_SUB,0);
        addr -= 2;
     }
    s = add_cmd (addr, start, C_VECT, z->start);

    if (s)
     {
     // do bank here......
       c = add_adt(&chadnl, s->start,1);    // only one
       if (c) {
       if (bk == g_bank(addr)) c->cnt = 0;
       c->bank = bk >> 16;                 // always add bank
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

int find_list_base(INST *c, uint ix, uint num)
{
    // try to find a list 'base address'.
    // expect an add of an immediate value
    // allow for ldx to swop registers

      int ans, xofst;
      uint reg;

      ans = 0;
      reg = c->opr[ix].addr;
      xofst = c->ofst;

      while (xofst)
         {         // look for an add,ad3w or ldx (max 'num' instructions back)
          xofst = find_pn_opcodes (0, xofst, num, 2, 6, 12);

          if (xofst)
            {          //check word ?
             if (sinst.sigix == 12 && sinst.opr[2].addr == reg)
              {  // ldw
                if (!sinst.opcsub) // from another register, change register
                reg = sinst.opr[1].addr;
                if (sinst.opcsub == 1)    // immediate
                  {
                  ans = sinst.opr[1].addr;     // addr is val+databank
                  if (val_rom_addr(ans)) return ans;
                  }
              }

             if (sinst.sigix == 6 && sinst.opr[sinst.wop].addr == reg)   //2].addr == reg)
              {  // ad2w, ad3w
               if (sinst.opcsub == 1)
                 {                              // found add for register
                  ans = sinst.opr[1].addr;     // addr is val+databank
                  if (val_rom_addr(ans)) return ans;
                 }
              }
            }
         }
  // but even if not found, may still have a valid address already in register.
  if (xofst)
    {
    ans = c->opr[ix].val;
    if (val_rom_addr(ans)) return ans;
    }
  else ans = 0;

return ans;
}

void match_opnd(uint xofst, OPC *opl, SFIND *f)
{
   OPS *s;
   uint m, ix;

   for (ix = 0; ix < 2; ix++)
    { // for each possible answer/register
     if (val_general_reg(f->rg[ix]) && !f->ans[ix])
      {

       if (opl->sigix == 12)
        {  // ldw - get operands
         do_one_opcode(xofst);
         s = sinst.opr + sinst.wop;       // where the write op is

         m = reg_eq(f->rg[ix], s->addr,s->wfend);

         if (m)
           {        // found register [2] = [1] for ldx
            if (sinst.opcsub == 1)
              {   // immediate, look for wop override
               f->ans[ix] = sinst.opr[1].addr;               //val;
               #ifdef XDBGX
                 DBGPRT(0," Found imm R%x = %x [%x]", f->rg[ix], f->ans[ix], xofst);
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
         {      // ad3w (ad2w ??)
          do_one_opcode(xofst);

          if (sinst.opr[3].addr == f->rg[ix])
           {
            if (sinst.opr[2].rbs)
             {     // ad3w with an rbase. 
               if (sinst.opcsub == 1)
                 {  // immediate.  op[3].val is correct as bank in rbase.
                   f->ans[ix] = sinst.opr[3].val;
                   #ifdef XDBGX
                      DBGPRT(0," ad3w R%x = %x at %x", f->rg[ix], f->ans[ix], xofst);
                   #endif
                 }
               else
                 {
                  // if it's a register, or indirect, then need to find a list base for multiple structs (tabs or funcs) 
                  // this might be widened to non_rbase ??
                  // and check spacing for tabs ??? op[4] is orig register, same subr as vects
                  f->lst[1] = sinst.opr[2].val;                // this is rbase address for possible list start
                  m = find_list_base(&sinst,4,15);
                  if (m)
                    {
                     f->lst[0] = m;
                     f->ans[ix] = m;       // for stopping the search
                     #ifdef XDBGX
                       DBGPRT(0," Found LIST R%x -> %x [%x]", f->rg[ix], m, xofst);
                     #endif
                    }
                  else
                   {
                  #ifdef XDBGX
                      DBGPRT(0," ad3w unknown R%x at %x", f->rg[ix], xofst);
                   #endif
                   }
                 }
             }
            else
             {         // not rbase
              #ifdef XDBGX
               DBGPRT(0," ad3w !rbase R%x at %x", f->rg[ix],  xofst);
              #endif
             }
           }
         }             // end ad3w

           // may be other pointer stuff here xdt2 does ad2w
      }     // end if f->x
    }    // end for i
 }



void set_xfunc(SFIND *f)
 {
  // set function start and one row.
  // extended later with extend_func
  // check start value for sign/unsign

  uint startval, val, fendin;
  int rowsize;       // increment, entry (read) size for g_val

  ADT *a;
  LBK  *k;
  AUTON *nm;

  nm = anames+f->spf;

  #ifdef XDBGX
  DBGPRT(1,"Check Func %x, I %x, O %x", f->ans[0], nm->fendin, nm->fendout);
  #endif

 

  fendin = nm->fendin;      //local as it may change
  startval = get_startval(fendin);     //cnv[szin].startval;      //(~casg[szin]) & camk[szin];     // start value unsigned

  val = g_val(f->ans[0],0, fendin);         // read first input value (as unsigned)
  val &= get_sizemask(fendin);     //  cnv[szin].sizemask;            //camk[szin];                     // and fit to startval

  if (val != startval)  // try alternate sign....
    {
     fendin ^= 32;          // swop sign flag
     #ifdef XDBGX
     DBGPRT(1,"require Swop sign in - val %x (%x)", val, startval);
     #endif

     startval = get_startval(fendin);       //cnv[szin].startval;  // (~casg[szin]) & camk[szin];     // start value

     if (val != startval)
      {
       #ifdef XDBGX
         DBGPRT(1,"func %x Invalid start value %x, expect %x", f->ans[0], val, startval);
       #endif
       return ;
      }
    }

    rowsize = bytes(fendin) + bytes (nm->fendout);

    k = add_cmd (f->ans[0], f->ans[0] + rowsize-1, C_FUNC, 0);

    if (k && !k->cmd)
      {                                     // cmd OK, add size etc (2 sets for func)

       new_autosym(3,f->ans[0]);            // auto func name, can fail
       k->size = rowsize;                   // size of one row

       a = add_adt(&chadnl,k->start,1);
       a->cnt = 1;
       a->fend = fendin;                     // size, word or byte
       a->pfw   = get_pfwdef(a);
       a->prdx = 2;                         // default decimal print

       a = add_adt(&chadnl,k->start,2);
       a->cnt = 1;
       a->fend = nm->fendout;              // size out
       a->pfw =  get_pfwdef(a);
       a->prdx = 2;                         // default decimal print
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
   if (!s->v[3])       return ;         //sign holder

   subr = add_subr(blk->substart);  // ok if already exists
   if (!subr) return;

   f = add_spf(subr->start);
   if (f->spf) return;               // already processed, assume f always OK

   size  = s->v[2]/2;                // size from sig (unsigned) as BYTES
   taddr = s->v[4];                  // address register (need +80000 ??)


   #ifdef XDBGX
     DBGPRT(1,"in fnplu for %x", blk->substart);
   #endif

   blk->logdata = 0;        // no data to collect (it's where sig found)
   #ifdef XDBGX
     DBGPRT(1,"set nodata %x", blk->start);
   #endif
   if (blk->start != blk->substart)
     {
       t = get_scan(blk->substart);
       if (t)
         {
          t->logdata = 0;
          #ifdef XDBGX
            DBGPRT(1,"set nodata %x", t->start);
          #endif
         }
     }     // stop any data additions in func

   size = (size*8) -1;               // convert to fend style

   osize = (s->v[10] & 0x1000) ? 32 : 0;              // signed out if bit SET, 1000 = signed if NOT set
   isize = (s->v[11] & 0x1000) ? 32 : 0;              // signed in  if bit SET, 1000 = signed if NOT set
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
        if (sinst.sigix == 9 && sinst.opr[2].addr == s->v[3])
          {
           #ifdef XDBGX
             DBGPRT(1,"find OR at %x", sinst.ofst);
           #endif
           if (omask & sinst.opr[1].val)  osize ^= 32;    // flip sign out if match;
           if (imask & sinst.opr[1].val)  isize ^= 32;    // flip sign in  if match
          }

        if (!sinst.opcsub && sinst.sigix == 12 && sinst.opr[2].addr == taddr)
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
      if (anames[i].fendin == isize && anames[i].fendout == osize)
        {
         new_autosym(i,blk->substart);             // add subr name acc. to type
         f->spf = i;
         f->fendin  = isize;
         f->fendout = osize;
         f->addrreg = taddr;  //address

         #ifdef XDBGX
              DBGPRT(1,"func spf=%d (%x)", i, blk->substart);
          #endif
          break;
         }
     }
 }



void set_xtab(SFIND *f)
{

 // add table from col and rows size.
 // can't fully check, do sanity check on sizes

  uint xend;
  LBK *k;
  ADT *a;
  int ofst, rows, cols;

// probably need a maxend safety here..... and a sign check....

  ofst = f->ans[0];
  rows = 2;
  cols = nobank(f->ans[1]);

  #ifdef XDBGX
   DBGPRT(1,"SET TAB Add %x cols %x Rows %x inx %x ss %d", ofst, cols, rows, f->spf,anames[f->spf].fendout);
   #endif

  if (cols < 2 || cols > 32) cols = 1;

  xend = ofst;
  xend += (rows*cols)-1;

  k = add_cmd(ofst, xend, C_TABLE, 0);

  if (k && !k->cmd)                       // in case tab by user cmd....
    {
     if (PSTCA) k->argl = 1;              // arg layout by default
     new_autosym(4,ofst);                 // auto table name, can fail
     a = add_adt(&chadnl,k->start,1);            // just ONE adt
     k->size = cols;                      // size for one line ( = cols)
     a->cnt = cols;
     a->prdx = 2;                         // default decimal print
    // cnvsstofld(a, anames[f->spf].sizeout);     

     a->fend = anames[f->spf].fendout;   // includes sign
     a->pfw =  get_pfwdef(a);

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

   f = add_spf(subr->start);
   if (f->spf) return;               // already processed, assume f always OK

   taddr = s->v[4] ;            // table address register
   tcols = s->v[3] ;            // num cols in this register
   osign = 0;
   omask = 0;


   // In tab lookup, sign flag is in the INTERPOLATE subroutine
   // subr call address saved in s->v[0] so look for tabinterp sig there

   t = get_sig(s->v[0]);
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
        if (t && sinst.sigix == 9 && sinst.opr[2].addr == t->v[3])
          {                       // found register to match 't'
            #ifdef XDBGX
               DBGPRT(1,"find OR at %x", sinst.ofst);
            #endif
            if (omask & sinst.opr[1].val)  osign ^= 1;    // flip signout to match
          }

        if (!sinst.opcsub && sinst.sigix == 12)
           {
               // ldx reg-reg this may also work for args, as they must be loaded via ldx ....
               // opcsub = 0 and set opr[1].addr
             if (sinst.opr[2].addr == taddr) taddr = sinst.opr[1].addr;
             if (sinst.opr[2].addr == tcols) tcols = sinst.opr[1].addr;
           }
        xofst++;                                      // skip to next instruction
       }
    }

   blk->logdata = 0;        // no data logging (it's where sig found)
   #ifdef XDBGX
      DBGPRT(1,"set nodata %x", blk->start);
   #endif

   if (blk->start != blk->substart)
     {
       x = get_scan(blk->substart);
       if (x)
         {
           x->logdata = 0;
           #ifdef XDBGX
             DBGPRT(1,"set nodata %x (%x)", x->start, x->nextaddr);
           #endif
         }
     }

   // subr is (blk->substart) from above
   new_autosym(13 + osign, blk->substart);             // add subr name acc. to type

   f->spf = 13 + osign;
   f->fendin  = 7;                 //1;
   f->fendout = 7;             //sizeout = 1;                // need better than this ??
   if (osign) f->fendout |= 32;         //sizeout |= 8;
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

  reg = nobank(reg);
  r = get_rgstat(reg);

  if (r)
   {       // mark rg status as encoded first
       #ifdef XDBGX
   DBGPRT(1,"set enc for %x", reg);
   #endif
   r->enc = enc;
   r->fend = 15;                   // enc is always a word
   r->data  = nobank(s->v[5]);     // 'start of data' base reg
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
 // cmd size (Y or W) is (2 or 4)= byte  (3 or 5)=word

   start = databank(x->v[0],0);
   z = x->v[14];

   #ifdef XDBGX
    DBGPRT(0,"Timer list %x size %d", x->v[0], z+3);
    //if (s) DBGPRT(0," Sub %x", s->addr);
    DBGPRT(1,0);
   #endif

   val = 1;
   a = 0;

 // should also check for next cmd....and put names in here....

   ofst = start;

   while (ofst < maxpcadd(ofst))
    {
      val = g_byte(ofst);
      if (!val) break;
      ofst += z;
      if (val & 1) ofst+=3; else ofst+=1;
    }

    k = add_cmd(start, ofst, C_TIMR, 0);      // this basic works, need to add symbols ?....
    if (k) a = add_adt(&chadnl,k->start,1);                   // only one (for now)
    if (a) {a->fend = (z*8) -1;  a->cnt = 1;
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
  // ....so fiddle it here by calling it LAST, and hope...

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

  int ans, jump;
//  OPC *opl;

  ans = 0;           // continue

// basic address stuff first................

   jump = addr - caller->curaddr;                         //difference

  if (bankeq(addr, caller->curaddr))
     {                                                    // in same bank
      if (addr == caller->nextaddr) ans = 1;              // dummy jump, ignore for scans
      if (addr == caller->curaddr)  ans = 3;              // loopstop HERE

      // backward jump, scanned already and conditional
      if (type == J_COND && jump < 0 && get_opstart(addr)) ans = 2;

      if (type == J_STAT)
       {
        // check for bank start, safety loopstops and opening jumps.
        if (nobank(addr) < 0x2010) end_scan (caller);                // END of this block

         if (jump == -1)
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
 //    if (!caller->lscan)
 caller->stop = 1;
    }

  return ans;
}



void do_signatures(SBK *s)
 {
  SIG *g;

  if (anlpass >= ANLPRT) return;

  if (get_opstart(s->curaddr))                // code, has this been scanned already ?
    {
 //    g = (SIG*) chmem(&chsig,0);
  //   g->start = s->curaddr;

/*
void *find(CHAIN *x, void *b)
{
 // find item with optional extra pars in b

 uint ix;
 ix = bfindix(x, b, 0);
 if (x->equal(x,ix,b)) return NULL;    // no match
 return x->ptrs[ix];
}

*/



 //    g = (SIG*) find(&chsig,g);              // Yes, check for sig at this address

       g = get_sig(s->curaddr);
    }
  else
    {
     g = scan_sigs(s->curaddr);               // scan sig if not code scanned before (faster)
     #ifdef XDBGX
       DBGnumops++;
     #endif
    }

  if (g && g->ptn->pr_sig)
    {
     g->ptn->pr_sig(g,s);         // process signature
    }
 }

void fix_args(LBK *l, SBK *s)
{
// check argument sizes and amend
// first draft.....called when args are skipped
// to ensure all processing has been done

  RST *r;
  LBK *k;
  ADT *a;
  int i, size;

  #ifdef XDBGX
    DBGPRT(1,"\nFIX args");
  #endif

 a = start_adnl_loop(&chadnl,l->start); 

//if (!a)
//{
//DBGPRT(1,0);
//}

 for (i = 0; i < EMUGSZ; i++)
 {
   k = emuargs[i];            // next args command (LBK)

   if (k && k->start == l->start)
   {
    size = k->size;
    if (a) a->fkey = set_adt_fkey(k->start,0);

    while ((a = get_next_adnl(&chadnl,a)))
     {          // for each level
      r = get_rgstat(a->dreg); // find register

      if (r)
        {
            #ifdef XDBGX
         DBGPRT(0,"R%x match %x", r->reg, l->start);
         DBGPRT(1," fend rg(%d) ad(%d)", r->fend, a->fend);
         #endif
         if (r->enc)
          {     // convert to encoded...........
            #ifdef XDBGX
              DBGPRT(1," Convert R%x to encoded %d", r->reg,r->enc);
            #endif
           a->enc  = r->enc;
           a->data = r->data;
           a->data |= basepars.datbnk;
           a->fnam = 1;
           a->cnt  = 1;      // as args added in twos anyway
           a->fend = 15;          // word
          }
         else
          {     // not encoded, adnl in twos anyway, simple change
                // if rgstat size bigger (word) (use 3 for byte and word)
           if ((r->fend & 31) > (a->fend & 31) && a->cnt > 1)
             {         //byte
              a->cnt  = 1;
              a->fnam = 1;

              a->fend = (r->fend & 15);        //no bigger than word
         //     a->fend |= (r->fend & 32);       // ignore sign

              #ifdef XDBGX
              DBGPRT(1,"Change R%x size to %d", r->reg, r->fend);
              #endif
             }
          }


// add a data item here ??
// add = decode_addr(a, s->curaddr);       no, need separate subr                 // val with decode if nec.  and sign - READS ofst
        }
                  #ifdef XDBGX
      else  DBGPRT(1,"R%x not found for %x", a->dreg, l->start);
      #endif
     }

       // sanity check here
     if (size != totsize(&chadnl,k->start))
       {
         #ifdef XDBGX
           DBGPRT(1,"ADDNL SIZE NOT MATCH %d %d- FIX", size, totsize(&chadnl,k->start) );
         #endif
   //     free_adnl(&chadnl,k);            //all chain ??
    //    a = add_adt(&chadnl,k,1);
    //    a->cnt = size;
       }

   }

 }      /// for emuargs loop

  #ifdef XDBGX
//    if (changed) 
DBG_rgchain();
    DBGPRT(1,"\nEnd FIX args");

  #endif



}

/*
int do_arg_data (int ofst, ADT *a, int pfwo)
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
    if (a->pfw < cnv[a->ssize].pfwdef) pfw = cnv[a->ssize].pfwdef;      // default to preset
    else pfw = a->pfw;                                                  // user specified
   }
  }
  val = decode_addr(a, ofst);                        // val with decode if nec.  and sign - READS ofst
  if (a->fnam) sym = get_sym(0, val,-1,ofst);        // READ sym AFTER any decodes - was xval

  if (sym) pstr(0,"%s",sym->name);  // pstr(0,"%*s",-pfw,sym->name); -ve fw doesn't work ??
   else
  if (a->fdat)
     {      // float - probably need to extend pfw to make full sense - allow override with X ?
      sprtfl((float) val/a->fdat, pfw+4);         // add 4 for float part
      pstr(0,"%s", nm+64);
     }
   else
     {
       if (a->prdx == 0) pstr(0,"%*x", pfw, nobank(val));   // HEX   nobank ??
       if (a->prdx == 1) pstr(0,"%*d", pfw, val);           // decimal print
       if (a->prdx == 2) pbin(val, a->ssize);           // binary  print, need size
 //   else
 //    { // hex prt
 //     //if (!numbanks) don't KNOW whether this is a value or an address...vaddr
 //     val = nobank(val);      // but NOT for bank addresses !
 //     pstr(0,"%*x", pfw, val);
 //    }
     }
  return cnv[a->ssize].bytes;         //casz[a->ssize];
}
*/


 void add_arg_data(uint start, uint ofst)
  {
   ADT *a;
   uint val, i,sz;
// LBK *k;

 //  DBGPRT(1,"XZARG DATA for %x", ofst);

   a = start_adnl_loop(&chadnl,start);    //    (ADT*) chmem(&chadnl,0);
 //  a->fid = fid;
 //  a->seq = 0;

 while ((a = get_next_adnl(&chadnl,a)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     val = decode_addr(a, ofst); 
     sz = bytes(a->fend);

     add_cmd (val,  val+sz -1, sz, ofst);     //change to dtk ???
     ofst += sz;
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
     k = get_aux_cmd(s->curaddr,C_ARGS);
     if (k)
       {
          fix_args(k,s);
          add_arg_data(k->start, s->curaddr);

        s->curaddr += k->size;
        #ifdef XDBGX
         DBGPRT(1,"SKIP %d args -> %x", k->size, s->curaddr);
        #endif

        if (!k->size)
         {
          #ifdef XDBGX
           DBGPRT(1,"ARGSIZE IS ZERO !! %x", s->curaddr);
         #endif
         s->curaddr ++;
         }

        // fix args with new sizes, encode, etc
        // cleanup reg status and arg holders
        // clear rgargs matching k->start
        // clear emuargs matching k->start



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


                if (e->end != k->end)


                DBGPRT(1,"size mismatch EMUARG %d = %x", i, e->start);
                #endif

         //       emuargs[i] = 0;
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

       DBGPRT(1,"Invalid (scan) %05x from %05x",s->start, s->curaddr);
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

/*
void scan_loop(SBK *s, INST *c, uint ofst)
 {
   // used for emulate of loop, has a stop ofst...

   int lscans = 0;

   if (!s) return;              // safety

    #ifdef XDBGX
     DBGPRT(0,"Emu");
     DBGPRT(0," Loop");
     DBGPRT(1," blk %x at %x", s->start, s->nextaddr);
    #endif





   show_prog (anlpass);

   s->scnt++;                                   // scan count by block

   while (!s->stop && !s->inv)                 // inv seems to stop loops, but should it be overridden with args detection ?
    {
     skip_args(s);                             // check for ARGS command(s)

     do_code (s, c);                           // do opcode
     s->curaddr = s->nextaddr;                 // next opcode address

     if (s->curaddr > ofst) break;

     lscans++;

     if (lscans > 2000)        // > 10000)
       {
        s->stop = 1;
        #ifdef XDBGX
         DBGPRT(1,"Hit opcode limit %x  %x", s->start, s->curaddr);
        #endif
       }
    }

    #ifdef XDBGX
     DBGPRT(0,"Stop ");
     DBGPRT(0,"Emu");
     DBGPRT(1," blk %x at %x", s->start, s->nextaddr);
    #endif

}
*/


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
  x = get_scan(addr);         //,0);
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

         end_scan (caller);        // END of this scan block
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

         newsc = add_scan (addr,type,caller);          // nsc = new or zero if dupl/invalid

         fakepush(caller, addr,1);                     // need this for later emulation check

         scan_blk(newsc,c);

        // scan subbranches of this subr for emulation before subr exit
        scan_subbs(newsc, c);

        if (newsc)
         {
           sub = get_subr(newsc->substart);
           a = get_adnl(&chadnl,sub->start,1);
           if (a)
            {     // args set by user command no args, just skip
                  //note that cannot do multi level args in a command...................
               add_arg_data(sub->start, caller->nextaddr);

               #ifdef XDBGX
               DBGPRT(0,"CMD ARGS ");
               #endif
               caller->nextaddr += sub->size;          //sub->adnl);
            }
           else
            {
             if (newsc->args)
              { 
                   // this is already in chain somewhere ??  need to ESCAPE and restart
               x = get_scan(caller->substart); // this is only true if single level args getter ?? and not if static jump....
               if (x)
                 {
                  #ifdef XDBGX
                   if (!x->emulreqd && x != newsc)
                     {
                       DBGPRT(0,"%x - ARGS Set emulreqd for %x (%x)", c->ofst, x->start, x->substart);
                       DBGPRT(1," new %x (%x) C %x (s%x) %x", newsc->start, newsc->substart, caller->start, caller->substart, caller->curaddr);
                     }
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
                 saveddbnk = basepars.datbnk;
                 emulate_blk(newsc);             // and emulate it
                 basepars.datbnk = saveddbnk;
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

  if (o->rgf)
     {                                          // if register (default)
      o->addr &= 0xff;                          // strip bank and safety check
      o->addr += basepars.rambnk;               // rambank always zero if 8061

      // for 8065 direct address word, or greater if odd,
      // change to bank+1 if currently even bank

      if (P8065 && (o->addr & 1) && bytes(o->rfend) > 1 )
        {  // add 100 for even banks only
         // if (!is_special_reg(o->addr) &&             // ALL registers........
         if ((o->addr & 0x100) == 0) o->addr += 0x100;  // loop 3 -> 0 ??
         o->addr ^= 1;         // drop odd bit, even if no bank change
        }

      b = get_rbt(o->addr,c->ofst);
     }         // end rgf
  else   o->addr = databank(o->addr,c);         // generic add (data) bank

  if (o->imd || o->bit) o->val = pc;            // immediate or bit flag, so pc is absolute value

  else o->val = g_val(o->addr,0, o->rfend);     // get value from reg address, sized by op

  if (b && c != &einst)
   {                                            // don't do rbase if in emu mode
    o->rbs = 1;
    o->val = b->val;                            // get value from RBASE entry instead
   }
  // indirect and indexed modes handled in calc mult_ops (called by do code)
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
  if (basepars.rambnk) o->addr -=  basepars.rambnk;  // remove any register bank
  o->val  = nobank(o->addr);                         // and any ROM bank
}



void do_vect_list(INST *c, uint start, uint end) // SBK *caller
 {

// no checks..................
  uint pc, vcall;
  LBK  *s;
  ADT *a;

  if (anlpass >= ANLPRT) return;

  pc = start;

  // would need to do PC + subpars for A9L proper display

   #ifdef XDBGX
    DBGPRT(1,"do vect list %x-%x", start,end);
   #endif

  for (pc = start; pc < end; pc +=2)
    {
     vcall = g_word (pc);                            // address from list
     vcall = codebank(vcall, c);                     // assume always in CODE not databank
     if (val_rom_addr(vcall))
     add_scan (vcall, J_SUB, 0);                     // vect subr (checked inside cadd_scan)
    }


  s = add_cmd (start, end, C_VECT,0);
  if (s && basepars.codbnk != basepars.datbnk)
    {
     // bank goes in ONE addn block.
       a = add_adt(&chadnl,s->start,1);
       if (a) {
       a->bank = basepars.codbnk >> 16;                  // add current codbank
       if (g_bank(vcall) == g_bank(start)) a->cnt = 0;  //don't print if match
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
   SBK *s;
   LBK *l;

   // assume bank is correct in start
   if (nobank(start) < PCORG) return 0;

   if (sz) end = start + sz;               // 0 to sz is an extra pointer sometimes.
   else   end = start + 0xff;             // 128 vectors max (A9L is 0xa4)


   score = 0;                     // max score
   lowcadd = maxpcadd(c->ofst);              // bank max for call addresses - multibank ?
   lastiadd = 0;

  // check start against curinst+cnt to see if an extra offset needs to be added ?
   //this needs probably more checks

   if (start & 1) start++;             // can't start on odd byte, but end should be
   if (!(end & 1)) end --;             // drop last byte if even

   #ifdef XDBGX
    DBGPRT(0,"In check_vect %x sz=%x lc=%x", start, sz, lowcadd);
    DBGPRT(1," from %x", c->ofst);
   #endif

   for(i = start; i <= end; i+=2)       // always word addresses  (score > 0?)
     {

      if (i >= lowcadd)
       {
         #ifdef XDBGX
         DBGPRT(1,"hit lc_add %x", i);
         #endif
         break;
       }

      s = (SBK*) get_scan (i);        //  find(&chscan,t);
      if (s)
        {
         #ifdef XDBGX
          DBGPRT(1,"hit scan %x", i);
         #endif
         end = i-1;
         break;
        }

      l = (LBK *) get_cmd(i,0);      //find(&chcode,k);
      if (l)
        {
         #ifdef XDBGX
           DBGPRT(1,"hit code %x %s",i,cmds[l->fcom]);
         #endif
         end = i-1;
         break;
        }


//CHECK VECT !!!



      l = (LBK *) get_cmd(i,0);       //find(&chdata, k);
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

      if (!val_rom_addr(addr))
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






void pp_hdr (int ofst, const char *com,  int cnt)    //, short pad)
{
  SYM *s;     //char *tt;
  int sym;

  if (cnt & P_NOSYM) sym = 0; else sym = 1;
  cnt &= (P_NOSYM-1);         // max count is 255

  if (!cnt) return;

  pstr(1,0);                                         // always start at a new line for hdr
  if (sym)
    {                                              // name allowed
     s = get_sym (0,ofst,32,ofst);                 // is there a symbol here ?(int rw, uint add, uint bitno, uint pc)
     if (s)
      {
       p_pad(3);
       pstr (1,"%s:", s->name);                      // output newline and reset column after symbol
      }
    }

  paddr(ofst,0,pstr);
  pstr(0,": ");
//       if (!numbanks) pstr (0,"%4x: ", nobank(ofst));
//  else pstr (0,"%05x: ", ofst); old, replaced by paddr

  while (--cnt) pstr (0,"%02x,", g_byte(ofst++));
  pstr (0,"%02x", g_byte(ofst++));

  if (com)
   {
    if (gcol > MNEMPOSN) p_pad(gcol+2); else p_pad(MNEMPOSN);
    pstr (0,"%s",com);
    if (gcol > OPNDPOSN) p_pad(gcol+2); else p_pad(OPNDPOSN);
   }

 }

/*************************************
get next comment line from file
move to combined bank+addr (as single hex)
**************************************/

int get_cmnt (CMNT *f, uint flags)
{
  int ans;
  int maxlen, rlen;
  uint pcz;
  char *t;

  if (f->ctext) return 0;

  ans = 0;
  rlen = 0;
  maxlen = 0;
  pcz = 0;

  f->ctext = flbuf;

  if (fldata.fl[4] == NULL || feof(fldata.fl[4]))
   {
    f->ofst = 0xfffff;          // max possible address with max bank
    f->ctext = NULL;          //(char *) empty;
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
      uint x;
      x = nobank(pcz);
      if (!numbanks)      pcz = x | basepars.datbnk;       //force 9
      else  pcz += 0x10000;      // otherwise add one to bank

      f->ofst = pcz;
      f->ctext = flbuf+rlen;
      while (*(f->ctext) == 0x20) (f->ctext)++;     // consume any spaces after number
      t = f->ctext;
      if (*t == '\\' && *(t+1) == 'n') f->newl = 1;    // newline at front of cmt
      else f->newl = 0;
   //   f->nflag = flags;
      return 1;
    }

  f->ctext = NULL;              // anything else is dead and ignored...
  f->ofst  = 0xfffff;           // max possible address with max bank
  return 0;

}

/* incorrectly stacked comments cause filled spaces (cos of print...)


SYM* symfromcmt(char *s, int *c, uint ofst, uint *bit)
 {
   int ans;
   uint add, p;
   SYM *z;

  if (!isxdigit(*s))
   {
    *c = 0;
    return 0;   // anything other than digit after special char not allowed
   }

   *bit = 32;
   z = 0;

   ans = sscanf(s,"%x%n:%x%n",&add,c,&p,c);

   if (ans <= 0) return NULL;

   if (ans > 1) *bit = p;

   z =  get_sym (0, add, *bit, ofst);

   return z;
 } */

// print indent line for extra psuedo code lines

void p_indl (void)
{
  if (gcol) pstr(1,0);
  p_pad(PSCEPOSN);

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
  // also args always needs min fieldwidths, so allow for a->pfw of zero

  if (pfwo) pfw = plist[pfwo];
  else
  {  
  if (a->pfw)
   {  // Not set for ARGS, default to preset if too small.
    val = get_pfwdef(a);
    if (a->pfw < val) pfw = val;
    else pfw = a->pfw;
   }
  }
  val = decode_addr(a, ofst);                             // val with decode if nec.  and sign - READS ofst
  

  if (a->fnam) sym = get_sym(0, val,32,ofst);        // READ sym AFTER any decodes - was xval
  if (sym) pstr(0,"%*s",-pfw,sym->name);
   else
  if (a->flt)
     {      // float - probably need to extend pfw to make full sense - allow override with X ?
      sprtfl((float) val/a->fldat, pfw+4);         // add 4 for float part
      pstr(0,"%s", nm+64);
     }
   else
                // original is %*d, or RIGHT justified (make this a prt_radix ??)
     {
       if (a->prdx == 2)
         { //if (val >= 0) {pchar(' '); pfw--;}           // for negative
           //pdec(val, a->ssize);
            pstr(0,"%*d", pfw, val);           // decimal print, need full width if negative
         }
       else
        {
         if (a->foff) val &= get_sizemask(15);            // word sized if offset
         else val &= get_sizemask(a->fend);  
         if (a->prdx < 2) pstr(0,"%*x", pfw, val);       // HEX default nobank ?? what about addresses ??
         if (a->prdx == 3) pbin(val, a->fend);           // binary print, need size.
        }
 //   else
 //    { // hex prt
 //     //if (!numbanks) don't KNOW whether this is a value or an address...vaddr
 //     val = nobank(val);      // but NOT for bank addresses !
 //     pstr(0,"%*x", pfw, val);
 //    }
     }
  return bytes(a->fend);
}



int pp_lev (uint start, int ofst, uint *seq, uint *mitem)
{

// ALWAYS stops at a newline.

// need to add CUTOFF at an end point so it does not overrun
// unless command verifies and fixes size first (better)

//start indent at plist[1] and go up from there.
// in here is Ok as it always does one row

 uint i, item;
 ADT *a;

 i = 0;
 item = 0;
 
 a = start_adnl_loop(&chadnl,start);     //(ADT*) chmem(&chadnl,0);
 //  a->fid = fid;

 a->fkey = set_adt_fkey(start, seq ? *seq : 1);

 while ((a = get_next_adnl(&chadnl,a)))
  {
   if (i) pstr(0,", ");       // true if looping on a

   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     if (i) pstr(0,", ");
     if (mitem) item = *mitem;           // for column pads
     pp_adt(ofst, a, item);              // field width is plist[item]
     if (mitem)
       {
        (*mitem)++;   
        item = *mitem;           // for column pads
       }
     ofst += bytes(a->fend);
    }

//probably need a wrap check here too.............and comment aligner


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
  }      // end of while (a)

// breaks to here for both newline and end.

//i = 1;
//item = lastpad;

//while (plist[i] > 0 && i < 32) {item +=plist[i]; i++;}


//for (i = 1; i < 

  
 if (seq)
  {
   if (a) *seq = a->fkey & 0xff; else *seq = 0;
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

  p_pad(CMNTPOSN);
  pstr(0,"\"");
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
  SYM *s;   //char *tt;
 // DIRS *d;

  // local declares for possible override
  uint com, fend, pfw, val;

  a = get_adnl(&chadnl,x->start,1);

  com = x->fcom;
 // d = dirs + com;

  if (!a)
   {    // no addnl for pp-lev
    fend = 7;                   // byte, safety.
    if (x->fcom < C_TEXT) fend = (x->fcom *8) -1;   // safe for byte to long
    pfw  = 5;       //get_pfwdef(15);           //cnv[2].pfwdef;                       // UNS WORD, so that bytes and word values line up
   }
  else
   {
    fend = a->fend;
    pfw   = a->pfw;
   }


  if (((x->end - ofst) +1 ) < bytes(fend))
    {
     // can't be right if it overlaps end, FORCE to byte
     fend = 7;
     com = C_BYTE;
  }

  // no symbol print, do at position 2
  pp_hdr (ofst, cmds[com], bytes(fend) | P_NOSYM);

  //if (pfw < cnv[cafw[d->defsze])  pfw = cafw[d->defsze]; // not reqd ??

  if (a) pp_lev(x->start, ofst, 0, 0);       // use addnl block(s)
  else
   {
     // default print HEX
     val = g_val (ofst,0, fend);
     pstr(0,"%*x", pfw, val);
   }

  // add symbol name in source code position
  // but this should only be for SINGLE entries ?

  s = get_sym(0, ofst, 32, ofst);  //(int rw, uint add, uint bitno, uint pc)

  if (s)
    {
     p_pad(PSCEPOSN);
     pstr (0,"%s", s->name);
    }

  ofst += bytes(fend);

  return ofst;
}




uint pp_vect(uint ofst, LBK *x)
{
  /*************************
  * A9l has param data in its vector list
  * do a find subr to decide what to print
  * ALWAYS have an adnl blk for bank, even if default
  * ***************/

  int val, bytes, bank;
  SYM *s;           //char *n;
  ADT *a;

  s = NULL;
  a = get_adnl(&chadnl,x->start,1);

  bytes = 2;                   // word (for now)

  val = g_word (ofst);          //always word

  if (a) bank = a->bank << 16; else bank = g_bank(x->start);

  val |= bank;               // add bank to value

  if (get_opstart(val))
    {   // valid code address
      pp_hdr (ofst, cmds[x->fcom], bytes);
      s = get_sym (0, val, 32, val);
    }
  else
    {
      if (ofst & 1)  bytes = 1;        // byte, safety
      pp_hdr (ofst, cmds[bytes], bytes); 
    }

  paddr(val,0,pstr);

  if (s)
     {
     p_pad(PSCEPOSN);
     pstr (0,"%s", s->name);
     }

  return ofst+bytes;
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
      pstr  (1,0);            // newline first
      paddr (ofst,0,pstr);
      pstr  (0," -> ");
      paddr (ofst+i-1,0, pstr);
      pstr  (1," = 0x%x  ## fill ## ", sval );
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

  if (!x->dflt)            //(x->cmd || !x->dflt)
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
  a = get_adnl(&chadnl,x->start,1);

  if (!a)  a = add_adt(&chadnl,x->start,1);     // safety

  xofst = ofst;

  val = g_byte (xofst++);  // first byte

  if (!val)
    {
      pp_hdr (ofst,cmds[C_BYTE],1);    // end of list
      pstr(0,"# Terminator", val);
      return xofst;
     }

 // a->fnam = 1;                      // TEMP TEST !!

  cnt = (val & 1) ? 3 : 1;           // long or short entry
  pp_hdr (ofst, cmds[C_TIMR], bytes(a->fend) + cnt);
  pstr(0,"%2x, ", val);
  val = g_val (xofst,0, a->fend); 
//val = fix_sbk_addr(val);                    //2nd entry.
  if (a->fnam) sym = get_sym(0, val,32,xofst);       // syname AFTER any decodes
  if (sym) pstr(0,"%s, ",sym->name);
  else pstr(0,"%5x, ",val);

  xofst += bytes(a->fend);

  if (cnt == 3)
   {                // int entry
    i = g_byte (xofst++);
    bit = 0;
    while (!(i&1) && bit < 16) {i /= 2; bit++;}          // LOOP without bit check !!
    val = g_byte (xofst);
//val = fix_sbk_addr(val);  
     if (a->fnam) {sym = get_sym(0, val,bit,xofst);
     if (!sym) sym = get_sym(0,val,32,xofst); }
    if (sym) pstr(0,"%s, ",sym->name);
    else  pstr(0," B%d_%x",bit, val);
  //  xofst++;                            // more later ??

   }

  return ofst+cnt+bytes(a->fend);
}



void p_op (OPS *o)
{
 // bare single operand - print type by flag markers

 if (o->imd) pstr (0,"%x", o->val & get_sizemask(o->rfend));    // immediate (with size)
 if (o->rgf) pstr (0,"R%x",nobank(o->addr));                    // register
 if (o->inc) pstr (0,"++");                                     // autoincrement
 if (o->bit) pstr (0,"B%d",o->val);                             // bit value
 if (o->adr) paddr(o->addr,0,pstr);                               // address - jumps
 if (o->off) pstr (0,"+%x", o->addr & get_sizemask(o->rfend));  // address, as raw offset (indexed)

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


void p_sc (INST *x, int ix, int ws)
{
// single op print for source code for supplied instance index
// note that cannot use same print as bits, bitno and reg stored 
// in separate ops


  int v;
  OPS *o;
  SYM *s;
  o = x->opr + ix;
  s = 0;


  if (o->sym)
   {                                     // if symbol was set/found earlier
     s = get_sym (ws, o->addr, o->bit ? o->val : 32, x->ofst);

     if (s)
      {
        if (o->off) pchar('+');                    // if offset with symname

        if (s->prtbit) pstr (0,"B%d_", o->val);   // default 'base' name
        pstr(0,"%s", s->name);
        return;
      }
    }

 if (o->imd)                               // immediate operand
    {
     pstr(0,"%x", o->val & get_sizemask(o->rfend));
     return;
    }

 if (o->rgf)
    {
   //  if (!zero_reg(o->addr) || ws) pstr (0,"R");           // non zero reg, or write op

     if (!zero_reg(o) || ws) pstr (0,"R");           // non zero reg, or write op

     pstr (0,"%x", nobank(o->addr));
     if (o->inc) pstr (0,"++");
     return;
    }

 if (o->bit)
     {
       pstr (0,"B%d_", o->val);
       return;
     }

 if (o->adr)
    {
     paddr(o->addr,0,pstr);
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
     // may need more smarts here
     // if single bank or < register
     if (!numbanks || nobank (v) < 0x400) v &= get_sizemask(15);
    // can't do paddr here..............but have to............
    // pstr (0,"%x", v);
       paddr(v,0,pstr);
     return;
    }
}

void fix_sym_names(const char **, INST *);




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

        j = get_fjump(ofst);       //(JMP*) chmem(&chjpf,0);
      //  j->fromaddr = ofst;
     //   j = (JMP*) find(&chjpf, j);

        if (j && j->jtype == J_SUB)
           { 
            sub = get_subr(j->toaddr);
            if (sub)  ofst = sub->end; else break;
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
        do_one_opcode(ofst);
        if (syms)  fix_sym_names(0,&sinst);
       }
   else
      {
       clr_inst(&sinst);
      }

    return ans;
 }


void p_opsc (INST *x, int ix, int ws)
{
 // source code printout - include sign and size markers.
 // ws = if set, use write size (equals not printed yet)
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
       i = ix & 3;      // safety
       if (i == 1 && x->opcsub > 1)  i = 0;
       if (x->opr[i].sym) i = 0;
       if (!x->opr[i].rgf) i = 0;

       if (i)
         {
           if (ws) i = x->opr[i].wfend; else i = x->opr[i].rfend; //(x->opr[i].wsize && ws)
           // after an equals, always use read size (ws = 0)
           if (i & 32)  pstr(0,"s");
           i &= 31;
           if (i < 8)  pstr(0,"y");
           else if (i < 16)  pstr(0,"w");
           else if (i < 24)  pstr(0,"t");
           //if (i == 4)
           else pstr(0,"l");
         }
      }

  ix &= 3;
  o = x->opr + ix;                            // set operand

//check if index is to a register
  if (ix == 1 && x->opcsub == 3 && nobank(o->addr) <= max_reg() && !o->rgf)
      {     // present as register - seems to work OK
       o->bkt = 0;
       o->rgf = 1;
       p_sc (x, ix, ws);
       return;
      }

// push check, turn immediate into address printout
//probably need better check here, to see if 'codebank(addr)' is a code command

  if (ix == 1 && o->imd && x->sigix == 14)
        {  // immediate - assume current code bank for printout 
           // if valid address, and change print to address
          o->addr = codebank(o->addr,x);
          if (val_rom_addr(o->addr))
            {
             o->imd = 0;
             o->adr = 1;
             o->sym = 1;       //look for symname too
            }
        }

  if (o->bkt)  pchar ('[');
  p_sc (x, ix, ws);
  if (ix == 1 && x->opcsub == 3) p_sc (x, 0,ws);  // indexed, extra op
  if (o->bkt)  pchar (']');
  return;
}

void cmtwrapcheck(char *c)
{
  char *t;
  uint col;
  // check if next 'word' would wrap after a space

  if (*c != ' ' && *c != ',') return; // not at space

  t = c+1;         // next char
  while (*t)
   {
     if (*t == ' ' || *t == ',' || *t == '\n' || *t == '\r' ) break;
     t++;
   }

// but this could be end of line and still overlap....
  if (!(*t)) return;         // no space
  col = t - c;
  if (col > 40) return;      // safety ....
  col += gcol;   // where next 'word' would end 

  if (col >= WRAPPOSN)
    {
     wrap();
    }
}

/*
char * do_seq(char *c,CMNT *m)
{
int ans, rs, bit,col,chars,charx, go;
uint ofst, addr;
SYM *x;

     //    case '\\' :                // special sequences
           c++;                     // skip the backslash, process next char.

           rs = toupper(*c);
           switch (rs)
             {  // special sequences

               default:
                 pchar(*c);             // print anything else
                 break;

// more opcode/operand options ??

               case 0x30:                  //allow zero - NOT TESTED !!
               case 0x31:
               case 0x32:                 // for user embedded operands 1-3
               case 0x33:

                 bit = *c - 0x30;   // char to number
                 if (bit >= 1 && bit <= 3)
                   {
                    p_opsc(m->minst,bit,0);
                    //c++;  done below..........
                   }
                 break;

               case 'n' :                                // newline
                 pstr(1,0);
                 break;

       //        case 'P' :                                // 'P' symname with padding, drop through
         //      case 'p' :
               case 'S' :                                // 'S' symname with no padding
               case 's':
                 col = gcol;                             // where print is now.
           //      if (*c == 'P' || *c == 'p') col+=21;    // pad 21 chars from here (deprecated)
                 c++;            //next char
                 chars = 0;
                 rs = 0;                                 // read sym by default
                 ofst = m->ofst;
                 x = NULL;

                 if (*c == 'w' || *c == 'W') {rs = 1; c++;}  // write sym
                 if (isxdigit(*c))
                   {                        // must be hex digit
                    bit = 32;
                    ans = sscanf(c,"%x%n",&addr,&chars);    //main symbol

                    if (ans > 0) c+=chars; else chars = 0;    // valid read

                    if (chars)
                      {      // bit.  this can fail
                       ans = sscanf(c,":%x%n",&bit,&charx);
                       // check bit valid ??
                       if (ans > 0) c+=charx;    // valid read

                       // range.  this can fail
                       ans = sscanf(c,"@%x%n",&rs,&charx);
                       if (ans > 0)
                        {
                         if (val_rom_addr(rs))  ofst = rs; 
                         c += charx;
                        }
                      
                       if (chars) x =  get_sym (rs, addr, bit, ofst);
                     }
                   

                    if (x)
                     {
                      if (bit < 32 && x->prtbit) pstr(0,"B%d_",bit); // not right for WORDS ?
                      pstr(0,"%s",x->name);
                      if (col > gcol) p_pad(col);           // pad to col (from above)
                     }
                  }
                    else
                     {  pstr(0,"??");  // failed sym read, print ??? and sequence ??for Bx_Rn dsiplay
                      //if (ans > 1 && bit < 32) pstr(0,"B%d_",bit);
                      //if (ans > 0) pstr (0,"%x ", addr);
                     }  //     /
                   

//if field read, print ??

                 c = c+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'T' :
               case 't' :
                                // tab to 'x'
                 c++;            //next char
                 chars = 0;

                 if (isxdigit(*c))
                   {                  // must be digit
                    ans = sscanf(c,"%u%n",&addr,&chars);
                    if (ans > 0) { if (addr < WRAPPOSN) p_pad(addr); else wrap();}
                   }
                 c = c+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'R' :        // runout
               case 'r' :

                 c++;
                 chars = 0;
                 rs = ' ';

                 if (!isdigit(*c))
                   { 
                    rs = *c;
                    c++; 
                   }

                 // runour of spaces only
                   ans = sscanf(c,"%u%n",&addr,&chars);
                   if (ans <= 0) addr =1;
                   p_run(rs, addr);
                 c = c+addr-1;                                   // skip the sequence + whatever read
                 break;

               case 'M' :      //next tab posn
               case 'm' :
                 chars = gcol;       //current col
                 chars = gcol/4 * 4;     //nearest prev tab
                 chars += 4; 
                 p_pad(chars);
                 break;


               case '\n' :                               // '\n',  ignore it
               case '\r' :
                 break;

              case 'w' :                                // wrap comment, or pad to comment column
              case 'W' :
                  // allow wrap to to other columns ?
                 if (lastpad >= CMNTPOSN) wrap();
                 else p_pad(CMNTPOSN);
                 break;

              case '|'  :        // multiple comment lines ? this seems to work............
                 if (m->nflag & 2) go = 0;
                 break;

             }   // end switch for '\' sequences

}


*/





void parse_comment(CMNT *m, uint flags)
{
  // process any special chars in a comment line

  char *c;     //, *n;
  int go, rs, chars, charx, ans;
  uint addr, col, bit, ofst;
  SYM *x;

   if (!m->ctext)  return;                     // safety

   c = m->ctext;

   if (!*c) return;                            //empty

   // if no special char at front, pad to cmt column
   if (*c != '\\' ) p_pad(CMNTPOSN);

   go = 1;
   while (*c && go)
    {
     switch (*c)
       {   // top level char check

         default:
           pchar(*c);                     // print anything else - auto wraps
           cmtwrapcheck(c);               // wrap check for whole word.
           break;

         case 1:                          //  for autocomment embedded - NOTE not chars !!
         case 2:
         case 3:
           p_opsc (m->minst,*c,0);
           break;

         case '\n' :                      // '\n',  ignore it
         case '\r' :
           break;


         case '\\' :                // special sequences
           c++;                     // skip the backslash, process next char.

           rs = toupper(*c);
           switch (rs)
             {  // special sequences

               default:
                 pchar(*c);             // print anything else
                 break;

// more opcode/operand options ??

               case 0x30:                  //allow zero - NOT TESTED !!
               case 0x31:
               case 0x32:                 // for user embedded operands 1-3
               case 0x33:

                 bit = *c - 0x30;   // char to number
                 if (bit >= 1 && bit <= 3)
                   {
                    p_opsc(m->minst,bit,0);
                    //c++;  done below..........
                   }
                 break;

               case 'N' : 
                 pstr(1,0);
                 break;

       //        case 'P' :                                // 'P' symname with padding, drop through
         //      case 'p' :
    
               case 'S' :                                // 'S' symname with no padding
        //       case 's':
                 col = gcol;                             // where print is now.
           //      if (*c == 'P' || *c == 'p') col+=21;    // pad 21 chars from here (deprecated)
                 c++;            //next char
                 chars = 0;
                 rs = 0;                                 // read sym by default
                 ofst = m->ofst;
                 x = NULL;

                 if (toupper(*c) == 'W') {rs = 1; c++;}  // write sym
                 if (isxdigit(*c))
                   {                        // must be hex digit
                    bit = 32;
                    ans = sscanf(c,"%x%n",&addr,&chars);    //main symbol

                    if (ans > 0) c+=chars; else chars = 0;    // valid read

                    if (chars)
                      {      // bit.  this can fail
                       ans = sscanf(c,":%x%n",&bit,&charx);
                       // check bit valid ??
                       if (ans > 0) c+=charx;    // valid read

                       // range.  this can fail
                       ans = sscanf(c,"@%x%n",&ofst,&charx);
                       if (ans > 0)
                        {
                         if (!val_rom_addr(ofst))  ofst = m->ofst; 
                         c += charx;
                        }
                      
                       x =  get_sym (rs, addr, bit, ofst);
                     }
                   

                    if (x)
                     {
                      if (bit < 32 && x->prtbit) pstr(0,"B%d_",bit); // not right for WORDS ?
                      pstr(0,"%s",x->name);
                      if (col > gcol) p_pad(col);           // pad to col (from above)
                     }
                  }
                    else
                     {  pstr(0,"??");  // failed sym read, print ??? and sequence ??for Bx_Rn dsiplay
                      //if (ans > 1 && bit < 32) pstr(0,"B%d_",bit);
                      //if (ans > 0) pstr (0,"%x ", addr);
                     }  //     
                   

//if field read, print ??

                 c--;         //= c+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'T' :
         //      case 't' :
                                // tab to 'x'
                 c++;            //next char
                 chars = 0;

                 if (isxdigit(*c))
                   {                  // must be digit
                    ans = sscanf(c,"%u%n",&addr,&chars);
                    if (ans > 0) { if (addr < WRAPPOSN) p_pad(addr); else wrap();}
                   }
                 c = c+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'R' :        // runout
                 c++;
                 chars = 0;
                 rs = ' ';     //space is default

                 if (!isdigit(*c))
                   { 
                    rs = *c;   //new runout char
                    c++; 
                   }
                 ans = sscanf(c,"%u%n",&addr,&chars);
                 if (ans <= 0) addr =1;
                 p_run(rs, addr);
                 c = c+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'M' :      //next tab posn
           //      chars = gcol;       //current col
                 chars = gcol/4 * 4;     //nearest prev tab
                 chars += 4; 
                 p_pad(chars);
                 break;


               case '\n' :                               // '\n',  ignore it
               case '\r' :
                 break;

 
              case 'W' :     // wrap comment, or pad to comment column
                 if (lastpad >= CMNTPOSN) wrap();
                 else p_pad(CMNTPOSN);
                 break;

              case '|'  :        // multiple comment lines ? this seems to work............
                 if (flags & 2) go = 0;
                 break;

             }   // end switch for '\' sequences



// case '[' :
// new sequences probably SWAP to different reader ??  






       }        // end switch for char
     if (c) c++;
    }

// save char position if in opcode,  else clear string

  if ((flags & 2) && *c)  m->ctext = c;  
  else m->ctext = NULL;         // clear string for next cmt
}

void pp_comment (uint ofst, uint flags)
{
  // ofst is the START of an opcode statement so comments
  // less than ofst should be printed.

  if (anlpass < ANLPRT) return;        // safety check


 // if (flags&1) set, then add a newline.
 // if (flags&2) set, stop on a '|' char (split comment for bit names)
 // BUT newline should be AFTER trailing comments (i.e. not beginning with newline)
 // but BEFORE ones that do. Therefore, if end set, break at first cmnt with newline at front
 // and remember if newline printed so multiple 'end' set doesn't cause multiple newlines
 // clear flag if end not set

   while (ofst > fcmnt.ofst)
       {
        if ((flags & 1) && fcmnt.newl)  break;  // do on NEXT call
        parse_comment(&fcmnt,flags);            //prints it;
        if (!get_cmnt(&fcmnt,flags)) break;
       }

   if (flags & 1)
      {
       if (!lastcmt)
         {
           pstr(1,0);
           lastcmt = 1;    // remember
         }
       }
   else lastcmt = 0;      //clear if not end
}



int maxsizes(LBK *x)
{
 // return max fieldwidth of a segment between newlines
 // currently for symbols, but need it for floats too.
 
 ADT *a;
 SYM *sym;
 int i,item;
 uint ofst, end, val,sz;

 ofst = x->start;
 memset(&plist,0,sizeof(plist)); // zero the whole thing
 
 if (x->argl) return 0;
 end = x->end - x->term ;  // looks OK
 
 while (ofst < end)
 {   
 
 sz = 0;
 item = 1;

// do this for every row (if struct), but only once for tabs and funcs
 
 //a = (ADT*) chmem(&chadnl,0);
 //a->fid = x;

 a = start_adnl_loop(&chadnl,x->start);
 while ((a = get_next_adnl(&chadnl,a)))
   {
    sz += cellsize(a);
    
    // now try to work out widths -not only symsize, but floats as well ?
 
    for (i = 0; i < a->cnt; i++)             // count within each level
      {
       sym = 0;   
       if (a->fnam) 
          {
           val = decode_addr(a, ofst);           // val with decode if nec.  and sign - READS ofst
           sym = get_sym(0, val,32,ofst);        // READ sym AFTER any decodes
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
           else val = get_pfwdef(a);
           if (a->flt) val+= 4;           // floating point
           if (val > plist[item]) plist[item] = val; 
         } 
       item++; 
       ofst += bytes(a->fend);
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
 
  // now add up for pad position.
  // ADD a comment column here ?
  
 plist[0] = (plist[0]*3) + 7;      //first pad posn, allow for addr
 if (numbanks) plist[0]++;         // for exta address digit

 if (plist[0] < MNEMPOSN) plist[0] = MNEMPOSN;   //minimum at mnemonic 

 for (i = 0; i <= item; i++)
   {
     plist[31] += plist[i]+2;            // space and comma for each item
   }

plist[31] += 5;          // for cmnd string
 return item;
}


int pp_cmpl(uint ofst, uint start, int fcom)
{
  // print 'compact' layout, all args on one line
  // unless | char, when it's split  
    
  uint tsize, size, end, seq, itn;
  
  tsize = totsize(&chadnl,start); 
  end = ofst + tsize;

// this is stct format, need a different one for subr (with args afterwards) 
 seq = 0;                 // new query
 itn = 1; 
 
 while (ofst < end)
  {      //for newline split............
   size = listsize(&chadnl,start,seq);
   
  if (fcom != C_ARGS)
   {
    pp_hdr (ofst,0,size);        // no title
    p_pad(plist[0]);
    pstr (0,"%s", cmds[fcom]);
    if (gcol < OPNDPOSN) p_pad(OPNDPOSN); else p_pad(gcol+2);
    pp_lev(start,ofst,&seq, &itn);    // continue from newline
    ofst += size;
    if (gcol >= CMNTPOSN) p_pad(plist[31]);
    if (ofst < end) pp_comment (ofst,0);    // NOT last item
   }
  else
   { // arguments for subr on same line, no split
    pp_lev(start,ofst,&seq, 0);
    ofst += size;
   }
  }

 if (!tsize) return 1;
  
 return tsize;
}



int pp_argl (uint ofst, uint start, int fcom, int *argno)
{

// Print 'argument' layout, 1 arg per line
// can get SUB or LBK as fid

int i;
uint tend;
ADT *a;

 pp_comment (ofst,0);            //for any comments BEFORE args

 tend = ofst + totsize(&chadnl,start)-1;  // where this 'row' ends
 i = 0;
 a = start_adnl_loop(&chadnl, start);    //(ADT*) chmem(&chadnl,0);
// a->fid = fid;

 while ((a = get_next_adnl(&chadnl,a)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     if (fcom == C_ARGS) sprintf(nm, "      #%s %d", "arg", (*argno)++); 
     else                sprintf(nm, "      #%s %d", cmds[fcom], (*argno)++);
     
     pp_hdr(ofst, nm, bytes(a->fend));
     p_pad(PSCEPOSN+3);
     a->pfw = 0;                         // no print field in argl
     pp_adt(ofst, a,0);

     ofst += bytes(a->fend);
     if (!bytes(a->fend)) ofst++;     // safety

     if (ofst <= tend)
       {
         pchar(',');
         pp_comment (ofst,0);        
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
  int size, xofst, argno,postprt;

  size = 0;
  argno = 1;
  postprt = 0;

  if (PSSRC)   pstr(0," (");   // args open bracket

  xofst = ofst;

  while(1)           //look for args first.........in a loop
   {
    k = get_aux_cmd(xofst, C_ARGS);

    if (!k || !k->size) break;
    if (PSSRC && xofst > ofst)   pchar(',');
    if (PSSRC && k->size)
      {  
        if (k->cptl)
           {
            pp_cmpl(xofst,k->start, k->fcom);
            postprt = 1;
           } 
        else  pp_argl(xofst, k->start, k->fcom, &argno);
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
          if (xsub->cptl) 
             {
              pp_cmpl(xofst,xsub->start, C_ARGS);
              postprt = 1;
             }
          else pp_argl(xofst, xsub->start, C_ARGS, &argno);
         }
      }
    }

  if (PSSRC && size) pchar(' '); // add space if args
  if (PSSRC) pstr(0,");");       // args close bracket

  if (size)
    {
     pp_comment (ofst,0);                               // do any comment before next args print
     if (postprt) pp_hdr (ofst, "#args ", size);        // do args as sep line if compact
    }

  return size;
}
// print subroutine params - simpler version
//func, table, struct go here

uint pp_stct (uint ofst, LBK *x)
{
  uint size, end;
  int argno;


if (x->start == 0x9c0f8)
 {
DBGPRT(1,0);
}



  end =  ofst + x->size;
  argno = 1;                               // can't reset argno !!
 
  if (end == ofst) end++;     // safety
 
  if (ofst == x->start) maxsizes(x);  // do only once.

  size = x->end - x->term + 1;    // where terminator starts
  
  if (x->term && end > size)
    {      // print terminator
     pp_hdr (ofst, 0, x->term);
     p_pad(OPNDPOSN);
     pstr(0,"## terminator");
     ofst += x->term;
     return ofst;
    }

  while (ofst < end)
    { 
      if (x->argl)
        {
         ofst = pp_argl(ofst,x->start,x->fcom,&argno);
         pp_comment (ofst, 1);                  // last item
         pstr(1,0);
        }
      else 
        {
         ofst += pp_cmpl(ofst,x->start,x->fcom);
         if (gcol >= CMNTPOSN) p_pad(plist[31]);    //before comments.....
        }
    }
  
 return ofst;
}

void shuffle_ops(const char **tx, INST *c, int ix)
{
  // shuffle ops down by one [1]=[2],[2]=[3] acc. ix
  // change pseudo code to ldx string "[2] = [1]"

  c->opr[4] = c->opr[ix];  // save original
  while (ix < 3)
    {
     c->opr[ix] = c->opr[ix+1];
     ix++;
    }
  if (tx) *tx = scespec[2];   //opctbl[LDWIX].sce;
}







void fix_sym_names(const char **tx, INST *c)
{
  // adjust operands for correct SOURCE code printout, rbases, indexes etc.
  // This CHANGES addresses for indexed and RBS for printout purposes

  OPS *b, *o;
  RBT *r;
  SYM *s;
  int i,w;
  int addr;
  const OPC* opl;

  opl = opctbl + c->opcix;

  // get and flag all sym names FIRST, then can drop them out later as reqd

  for (i = 1; i <= opl->nops; i++)
   {
    o = c->opr+i;
    o->sym = 0;            // no sym
    s = NULL;
    if (i == c->wop) w = 1; else w = 0;

    s = get_sym(w, o->addr,32, c->ofst);      // write sym or read sym
    if (s)
      {
       o->sym = 1;  // symbol found
      } 
   }

  if (c->opr[3].bit)            // bit jump - if bitname, drop register name
     {
        o = c->opr+2;
        b = c->opr+3;               // b = bit number entry, get bit/flag name
        b->sym = 0;                 // clear bit name flag
        s = get_sym (0, o->addr, b->val, c->ofst);  // read sym
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
     if (c->wop == 1) w = 1; else w = 0;
     if ((o->addr&0x3ff) < 2) o->val = 0;       // safety

     //if (zero_reg(o->addr) || o->rbs)
     if (zero_reg(o) || o->rbs)
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

        s = get_sym(w, o->addr,32,c->ofst);       // and sym for true inx address

        if (s)     //symbol found at (true) addr
         {
          o->bkt = 0;       // drop brackets (from [1+0])
          o->sym = 1;       // mark symbol found
         }

      } // end merge (rbs or zero)
     else
      {
        // this is an [Rx + offset]. Sort op[0] symbol if not a small offset
        if (!is_special_reg(b->addr) && !b->neg)
          {
            // not negative, not reserved register, add databank if > PCORG
                
            s = get_sym(0, databank(b->addr,c),32,c->ofst);
            if (s) 
              {
               if (nobank(b->addr) > PCORG || s->immok)  b->sym = 1;
              }
            if (numbanks) b->addr = databank(b->addr,c);         // do bank for printout
          }
      }
    }         // end inx

  if (c->opcsub == 1)       // immediate - check opcodes etc. for removing symbol names
     {
      // not small values (register names ??)
      if (nobank(c->opr[1].addr) < max_reg())  c->opr[1].sym = 0;      // not (normally) names for registers
      // sub ??
      // not 5 and, 8 mult, 9 or,xor, 10 cmp, 11 div  - all must be an abs value
      // ldx 12 with rbase - no symname
      else if (opl->sigix == 5)  c->opr[1].sym = 0;
      else if (opl->sigix > 7 && opl->sigix < 12)  c->opr[1].sym = 0;  
      else if (opl->sigix == 12 && c->opr[2].rbs)  c->opr[1].sym = 0;
     }


// TEST !!

if (opl->nops == 2 && c->wop && zero_reg(c->opr + c->wop))
   {   //   R0 = R0 + something;
      if (tx) *tx = scespec[2];           //opctbl[LDWIX].sce;
    }

  // 3 operands.  look for rbase or zero in op[1] or op [2] and convert or drop
  if (opl->nops > 2)
    {    //rbase and R0
     if (c->opr[1].rbs && zero_reg(c->opr + 2))  shuffle_ops(tx,c,2);

     if (c->opr[2].rbs && c->opcsub == 1 && c->opcix > 23 && c->opcix < 29)
      {   // RBASE + offset by addition or subtraction, only if imd and ad3w or sb3w
       b = c->opr+1;
       addr =  b->val + c->opr[2].val; // addr may be NEGATIVE at this point !!
       if (addr < 0) addr = -addr;

       b->addr = addr;         //b->val + c->opr[2].val;              // convert to true address....
       // databank ?
       b->imd = 0;
       b->adr = 1;
       s = get_sym(0, b->addr,32,c->ofst);

       if (s) b->sym = 1;
       shuffle_ops(tx,c,2);                  // drop op [2]
      }

    if (zero_reg(c->opr + 2))   shuffle_ops(tx,c,2);      // x+0, drop R0
    if (zero_reg(c->opr + 1))   shuffle_ops(tx,c,1);      // 0+x, drop R0



   }   // end op 3 op checks

 //     R3 = R2 + R1 style.  is anything rbase ?? replace with address as an imd
  if (!c->opcsub && (c->opr[1].rbs || c->opr[2].rbs))
      { 
        for (i = 1; i < c->numops; i++)
          {  //  op 1 for 2 ops, 1 & 2 for 3 ops
            b = c->opr + i;
            if (b->rbs)
              {
               b->imd = 1;
               b->rgf = 0;
               b->sym = 0;
              }
          }  
       }

    // CARRY opcodes - can get 0+x in 2 op adds as well for carry
    // "\x2 += \x1 + CY;"   nobank is for immediates...

  if (nobank(c->opr[1].addr) == 0 && c->opcix > 41 && c->opcix < 46)
    {
     if (tx) *tx = scespec[opl->sigix-6];     // drop op [1] via special array
    }
}




void make_mask(uint ix)
{    //make mask of where matching bits are

  SYM *s;
  uint  i, start, end;
  plist[ix] = 0;
  s = bitnames[ix];

  if (!s) plist[ix] |= (1 << ix);
  else
    {
     if (s->prtbit) plist[ix] |= (1 << ix);
     else
       {
         // check for upper byte matches first.
         start = s->addb & 7;
         end = s->fend;
         if (ix > 7)
          {
            start += (ix/8) *8;
            end +=   (ix/8) * 8;;
          }

         for (i = start; i <= end; i++) plist[ix] |= (1 << i);
       }
    }
}




uint get_bitnames(INST *c)
 {
  // get all bit names, count print mask
  // for later checks.
  uint pcnt, i, x, sz;      //ncnt
  OPBIT *m;
  SYM *s;

  m = (OPBIT*) c->opr + 4;      

  x = m->pmask;                 // print mask
  sz = c->opr[1].rfend;        // bits to check - not right for variable fields
  if (sz > 15) sz = 15;        // safety !!

  // first, check for 'F' or 'N' in no bit symbol(s)

  pcnt = c->opr[m->sceix].addr;    // base addr of op

  for (i = 0; i <= sz/8; i++)
    {
     s = get_sym(m->rw,pcnt+i,32, c->ofst);    // master base name(s) 

     if (s)
      {
       if (s->flags) m->flags = 1;           // flag byte bit
       if (s->names) m->names = 1;
      }
    }

  pcnt = 0;

// x = m->pmask;                 // print mask

  for (i = 0; i <= sz; i++)
   {

    bitnames[i] = get_sym(m->rw, c->opr[m->sceix].addr, i, c->ofst);
    make_mask(i);                     // preset matching mask

    if (bitnames[i])    
      { 
       if ((x & plist[i]) && !bitnames[i]->prtbit) m->nmfnd  |= (1 << i);      // matching bit names mask
       if (bitnames[i]->flags) m->flags = 1;           // 'F' flagsword 
       if (bitnames[i]->names) m->names = 1;           // 'N' names only 
      } 
    if (x & plist[i]) {pcnt++;                // count of matching syms
    x ^= plist[i];}
   }

 return pcnt;
}


uint prt_bitsym(INST *c, uint ix)
{ 
  // print bitname or BX_Rn as last resort
  // ix is bitno in here.... 
  uint addr;
  SYM *s;
  OPBIT *m;
  
  m = (OPBIT*) c->opr + 4;      
  addr = c->opr[m->sceix].addr;

  if (bitnames[ix])
    {
      s = bitnames[ix];
      if (s->prtbit) pstr (0,"B%d_", ix);
      pstr (0,"%s",s->name);      //  print matched sym bit name
      return 1;
    }

  if (m->names) return 0;            // names only
 
  fixbitfield(&addr, &ix, 0);               // fix anything over B7
  pstr (0,"B%d_R%x", ix, addr);
  return 1;
}


void prt_bitline(INST *c)
{
 // print bitnames.
 // use 'm' struct

  uint i, x, sz, vmask;
  uint pmask;
  OPBIT *m;
  
  m = (OPBIT*) c->opr + 4;     

  x = 0;
  // pmask might be REVERSE of vmask (for ands)

  vmask = c->opr[1].val;          // orig (value) mask
  pmask = m->pmask;               // print mask
  sz = c->opr[1].rfend + 1;
  if (sz > 16) sz = 16;            // safety !!

  for (i = 0; i < sz; i++)
   {
      if (pmask & plist[i]) 
      {
       if (x)
        {   // indent+new line if not first sym
         p_indl ();
         x = 0;
        }

       x = prt_bitsym(c, i);    //print bit or sym

       if (x)
        {
           pchar(' '); 
           if (m->fxor) pstr(0,"^");
           pstr (0,"= %d;", (vmask & plist[i]) >> i); // ** HERE would be a bit FIELD

           if (pmask == plist[i]) break;          // exit and stop comment check (last entry)
           pp_comment(c->ofst+1,2);               // otherwise check for comment
        }
       pmask ^= plist[i];
      }
   }
}


void bitwise_replace(const char **tx, INST *c)

 /* check for bit flag printouts (and or operations)
 * must be immediate op (as flags mask) and at least one name found
 * for ops with a \4 at the front.  WRITE symbols for first try.
 * ops referenced are -
 * c->opr[1].val   is actual mask
 * c->opr[2].addr  is base (sym lookup) addr - same for 2 or 3 op
 * ops[4] overlaid with different struct 'm' for ease of reading
 * m->pmask          // print mask
 * m->nfnd           // names found mask
 * m->fxor           // this is an XOR opcode
 * m->flags          // flags word 'F' in sym or bitsyms
 * m->names          // names      'N' in sym
 * m->sceix          // opc source index
 * m->rw             // look for read or write syms
 */
// need to rework for fields wider than one bit................
{

  uint i, x, pcnt, sig;
  OPBIT *m;
  
  if (anlpass < ANLPRT)    return;              // only for  print phase
  sig = c->sigix;                 // opcode sigix

  // only continue if either imd instruction or clrx

  if (c->opcsub != 1  &&  sig != 1) return;      // only if immediate or clr  
  if (c->opr[4].rbs) return;                     // NOT if a shuffled out rbs operand

  memset(c->opr+4,0,sizeof(OPS));                // clear ops[4], safety

  m = (OPBIT*) c->opr + 4;                       // overlay opbit onto ops[4]
  m->pmask = c->opr[1].val;                      // print mask
  m->sceix = (sig == 1) ? 1 : 2;                 // source op index, 1 for clr
  m->rw = 1;                                     // write syms by default
  if (c->opcix >= 32 && c->opcix <= 33) m->fxor = 1;  // XOR Flag

  memset(&bitnames,0,sizeof(bitnames));          // clear bitnames

  // OR, XOR with zero mask 
  if (sig == 9 && !c->opr[1].val)
    { 
      c->opr[1] = c->opr[2];
      *tx = scespec[2]-1;
      // add "redundant" comment ??
      return;
    } 

  // AND. reverse print mask if not a 3 op, otherwise set READ

  if (sig == 5)
    {
     if (c->numops < 3)
       {    // not an3x
        m->pmask = ~m->pmask;
        m->pmask &= get_sizemask(c->opr[1].rfend);    // safety
       }
     else m->rw = 0;                        // read sym names for an3x
    }

  pcnt =  get_bitnames(c);         // pcnt is no of bits in print mask 

  if (sig == 12 || sig == 10 || sig == 1)
      {    // ldx, cmp, clr
       if (m->flags)
        {
         m->pmask = get_sizemask(c->opr[1].rfend);  // set all bits to print (wrong ??)
        } 
       else return;     // do standard print of these ops
      }

// STX ????

//  if (pcnt > 1) c->opr[4].addr = c->opr[4].val;  // printmask = namesmask - only prt sym names

  if (pcnt > 1 && !m->nmfnd)  return;


  // now print names, one per line

  if (c->numops < 3)
   {
    prt_bitline(c);       // one bitname per line, with =
    *tx = empty; 
   }
 else
  {        // an3x a = b|c|d  format
   int ppos;

   if (sig != 5) return;

   p_opsc(c,3,1);
   pstr(0," = ");
   if (pcnt > 1) {pchar ('('); ppos = gcol;}

// newline for | for long lines ?

   sig = c->opr[1].val;      // temp vmask
   x = 0;
   i = 0;

   while (i <= c->opr[1].rfend)       //;  i++)
    {
     if (!sig) break;    //to stop comment ?? not reqd ??
     if (sig & plist[i])
      {
       if (x)
         {
           pchar('|');
           pp_comment(c->ofst+1,2); // and need a pp_comment here too..........
           pstr(1,0);
           p_pad(ppos);
         }
       x = prt_bitsym(c, i); //name or Bx_Rn
       sig ^= plist[i];
      }
     i++;
    }

   if (pcnt > 1) pchar (')');
   if (pcnt) { pchar(';');  *tx = empty; }                 // at least one name substituted return a ptr to null
  } 
  
 }



/*
void shift_comment(INST *c)
 {
  //  do shift replace here - must be arith shift and not register 
  int ps;

  if (anlpass < ANLPRT) return ;              // redundant ?

  if (c->opcix < 10 && c->opr[1].imd)
   {     // immediate shift
    if (zero_reg(c->opr + 2))
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

*/

void do_carry(INST *c, const char **tx, int ainx)
{

// Replace "if (CY)" with various sources
// possible opcodes add,shift R, shift L, cmp, inc, dec, neg, sub

   int ans;
   const OPC *opl;

   pstr (0,"if (");


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
          p_opsc(&sinst,opl->wop,1);           // write
          pstr(0," %s ",*tx);
          p_opsc(0,1,0);
          pstr(0,") ");
          *tx = scespec[3] -1;     //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

  if (opl->sigix == 10)
        {
          // compare. Same as subtratc, but no wop
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];

          if (zero_reg(sinst.opr + 2))                //sinst.opr[2].addr == 0)
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(&sinst,1,0);                   // operand 1 of compare
              pstr(0," %s ", swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(&sinst,2,0);                   // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(&sinst,2,0);
              pstr(0," %s ",*tx);
              p_opsc(&sinst, 1,0);
            }
          pstr(0,") ");
          *tx = scespec[3] -1;    //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 6  || opl->sigix == 25)
        {       //  add or inc STANDARD CARRY -  have wop ?  
          p_opsc(&sinst,opl->wop,1);
          pstr(0," %s", (ainx & 1) ? ">" : "<="); 
          pstr(0," %x) ", get_sizemask(sinst.opr[opl->wop].wfend));
          *tx = scespec[3] -1;         //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
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
             ans =  sinst.opr[opl->wop].wfend + 1;       //  (sinst.opr[opl->wop].wsize & 7) * 8;        //   casz[sinst.opr[opl->wop].wsize] * 8;          // number of bits
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

          pstr(0,"B%d_", ans);

          p_opsc(&sinst,opl->wop,1);
          pstr(0," = %d) ", ainx & 1); 
          *tx = scespec[3] -1;         //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
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
          p_opsc(&sinst,opl->wop,1);
          pstr(0," %s 0) ", (ainx & 1) ?  ">=" : "<" );
          *tx = scespec[3] -1;             //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
      }

}


void do_ovf(INST *c, const char **tx, int ainx)
{

// Replace if (OVF) with various sources
// possible opcodes add, shift L, cmp, inc, dec, neg, sub, div.

 //  int ans;
 //  const OPC *opl;

   pstr (0,"if (");
   // get last psw changer

return;            //temp ignore extras

/*
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

          if (!nobank(sinst.opr[2].addr))
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
          // need to find out how many shifts
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

          //fiddle as a bit
          pstr("B%d_", ans);
          p_opsc(&sinst,opl->wop,0);
          pstr(" = %d) ", ainx & 1);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

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
*/
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

   pstr (0,"if (");

   // if (!ans) return;          // not found
   opl = opctbl + sinst.opcix;  // mapped to found instance

   if (opl->wop)
     {            // not a compare
      if (!nobank(sinst.opr[opl->wop].addr))
        {
         // wop is R0, style"(op) = 0"
         // does not work for shortened ops, i.e. "+="


         if (nobank(sinst.opr[2].addr))
           {  // print only [1] and op if zero register in [2]  
            s = opctbl[sinst.opcix].sce;               // source code of psw op
            x = strchr(s, '=');
            if (x)
             {  // do opcode after = in brackets
              x += 3;                                // should now be op
              pstr(0,"(");
              p_opsc(&sinst,2,1);                    // operand 2 of compare (write)
              while (*x > 0x1f) pchar(*x++);         // operator (space & up)
              p_opsc(&sinst,1,0);                    // operand 1 of compare
              pstr(0,")");
             }              
           }
         else
           {
             p_opsc(&sinst,1,1);                 // operand 1 of compare (write)
           }
         pstr(0," %s ",t);                      // orig compare op  
         p_opsc(0,1,0);                         // orig compare value
        }
      else
       {   // no compare real wop , style [wop] == 0
        p_opsc(&sinst,opl->wop,1);         // write
        pstr(0," %s ",t);
        p_opsc(0,1,0);
       }
     }

 
   if (!opl->wop)
     {      // compare found, style R2 = R1
      if (!nobank(sinst.opr[2].addr))
       {
         if (!nobank(sinst.opr[1].addr) && ans)
          {       //both ops zero ??
            pstr(0," %s ", zerocmpop[ainx - 62]);       //c->opcix - 62]);
          }
         else
          {
           // cmp has zero first, so swop operands over via special array.
           // NB. NOT same as 'goto' to {} logical swop
           // so R1 <reversed> 0
           p_opsc(&sinst,1,0);                        // operand 1 of compare
           pstr(0," %s ", swopcmpop[ainx - 62]);        //c->opcix - 62]);
           p_opsc(&sinst,2,0);                        // operand 2 of compare (zero)
          }
       }
     else
       {  
         p_opsc(&sinst,2,0);                        // operand 2 of compare (reads)
         pstr(0," %s ",t);
         p_opsc(&sinst,1,0);                        // operand 1 of compare
       }
     }

    pstr(0,") ");
    *tx = scespec[3] -1; //opctbl[JMPIX].sce - 1;             //set \9 for 'goto' processing
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

  if (!s) return;
  a = get_adnl(&chans,s->start,1);

  if (!a) return;

//   { // found an answer set up phantom opr[3] (rgf set)
     o = c->opr+3;
     sym = 0;

     o->rfend = a->fend;
     o->addr = a->data;

      if (o->addr > 0x3ff)
       {
         o->rgf = 0;
         o->adr = 1;
         o->bkt = 1;
       }

     if (a->fnam) sym = get_sym(0, o->addr,32,c->ofst);           // syname AFTER any decodes
     if (sym) {o->sym = 1; o->bkt = 0;}

     p_opsc(c,0x13,0);

  pstr(0," = ");
//  }
}




void do_regstats(INST *c)
 {
   int i;
   OPS *o;
   RST *r;

// register size arguments

   for (i = 1; i <= c->numops; i++)
    {
     // not wop !
     if (i != c->wop)
      {
     o = c->opr+i;
     r = get_rgstat(o->addr);
     if (r && r->arg)
     {
       set_rgsize(r, o->rfend);  
  //      r->used = 1;

     }
     }
    }

  if (c->opcsub == 2)
  { // indirect use opr[4] for reg details
     o = c->opr+4;
     r = get_rgstat(o->addr);

  if (r && r->arg)
  {
    // arg used as address, so will always be WORD
    r->inr = 1;
    set_rgsize(r, 15);
 //       r->used = 1;


// add here instead ??
  }
  }

}


 void do_dflt_ops(SBK *s, INST *c)
 {  // default - any operands as registers
    int i;
    c->opcsub = 0;         // no multiple mode
    for (i = 1; i <= c->numops; i++)
        {
         op_calc (g_byte(s->nextaddr + i), i,c);
     //    set_opdata(s->nextaddr + i,7);
        }
    s->nextaddr += c->numops + 1;
//    if (s->emulating)
 do_regstats(c);
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

  //   c->opcsub = g_byte(xofst) & 3;       //move outside to save an access

     firstb = g_byte(xofst+1);             // first data byte after (true) opcode
  //   set_opdata(xofst+1,7);
     o = c->opr+1;                         // most action is for opr[1]


     switch (c->opcsub)
      {
       case 1:                                          // first op is immediate value, byte or word
         op_imd(c);                                     // op[1] is immediate
         if (o->rfend > 7)
          {                             // recalc op1 as word
           xofst++;
           op_calc (g_word(xofst), 1,c);
     //      set_opdata(xofst,o->rfend);
          }
         else
          {
           op_calc (firstb, 1,c);                       // calc for op[1]
          }

       break;

       case 2:     // indirect, optionally with increment - add data cmd if valid
                   // ALWAYS WORD address. Save pre-indirect to [0] for increment by upd_watch

         if (firstb & 1)
          {                                         // auto increment
           firstb &= 0xFE;                          // drop flag marker
      //     c->inc = 1;
           o->inc = 1;                              // always op[1]
          }
         op_calc (firstb, 1,c);                     // calc [1] for new value, always even
         o->bkt = 1;
         c->opr[4] = c->opr[1];                     // keep register details
         if (anlpass < ANLPRT)
          {                                         // get actual indirect values for emulation (restore in upd_watch)
           o->addr = g_word(o->addr);               // get address from register, always word
           o->addr = databank(o->addr, c);          // add bank
           o->val = g_val(o->addr, 0, o->rfend);        // get value from address, sized by op
     //      add_opr_data(s,c);
          }
         break;

       case 3:                                          // indexed, op0 is a fixed offset

         if (firstb & 1)
            {
             c->opr->rfend = 15;                        // offset is word (unsigned)
             firstb &= 0xFE;                           // and drop flag
            }
          else
            {
             c->opr->rfend = 39;                        // short, byte offset (signed)
            }

         c->opr->off = 1;                              // op [0] is an address offset
         o->bkt = 1;
         op_calc (firstb, 1,c);                        // calc op [1] (always even) 
         if (!o->rbs) o->val = g_word(o->addr);                     // ALWAYS a WORD for indexed
         c->opr[4] = c->opr[1];                          // keep register details - is this right for STW ??
         c->opr->addr = g_val(xofst+2,0,c->opr->rfend);  // get offset or base value CAN BE NEGATIVE OFFSET
     //    set_opdata(xofst+2,c->opr->rfend);
         addr = c->opr->addr;
         if (addr < 0 && (c->opr->rfend & 32)) c->opr->neg = 1;    // flag negative if signed
         
         if (anlpass < ANLPRT)
          {                                            // get actual values for emulation
           addr = c->opr->addr + o->val;
           //o->addr may be NEGATIVE at this point !!    probably never..........
           if (addr < 0)
           {
             addr = -addr;                             // wrap around ?
           }
           o->addr = databank(addr, c);                // get address and bank
           o->val = g_val(o->addr,0, o->rfend);           // get value from address, sized by op
          }
         
         xofst += bytes(c->opr->rfend);                  // size of op zero;

         break;

      default:      // direct, all ops are registers
         op_calc (firstb, 1,c);                // calc for op[1]
         break;
      }

     //calc rest of ops - register 1 done

     for (i = 2; i <= c->numops; i++)
       {
        op_calc (g_byte(xofst + i), i,c);
       } 
     s->nextaddr = xofst + c->numops + 1;

     if (s->scanning) add_dtk(s,c);

//     if (s->emulating) 
do_regstats(c);

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
         if (val > (int) get_sizemask(size)) c->psw |= 0x2;            //cnv[size].sizemask) c->psw |= 0x2;      // set CY  camk[size]
        }
    if (mask & 8)
        {           // OVF
         c->psw &= 0x37;          // clear OVF
         if (val > (int) get_sizemask(size)) c->psw |= 0x8;      // set OVF
        }

     if (mask & 4)
        {           // OVT - don't clear, just set it.
         if (val > (int) get_sizemask(size)) c->psw |= 0x4;      // set OVT
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
             if (sinst.sigix == 12 && sinst.opr[2].addr == reg)
              {  // ldx from another register, change register
                reg = sinst.opr[1].addr;
              }

             if (sinst.sigix == 10 && sinst.opr[2].addr == reg)
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

  o = c->opr + 1;

  if (anlpass >= ANLPRT) return;

  if (s->scanning)
    {
        // change to a switch on opcsub ?

     if (o->imd && val_rom_addr(o->addr))  // c->opcsub = 1
       {   // treat as a subr call, but this could be a return address
           // this uses CODE bank, not data
           //change display to addr, not val 
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
           if (val_rom_addr(i))
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

            i = find_list_base(c, 4,10);
            size = find_vect_size(s, c, 4);
       //     i = databank(i,c);
            if (i)            //val_rom_addr(i)) done in find_list
              {
               check_vect_list(s, i, size, c);
              }
           }

     if (o->rgf &&  !c->opcsub)
         {
          // register - check for argument getter
          r = 0;
          for (i=0; i < STKSZ; i++)
            {
             t = scanstack+i;
             if (t->type == 1 && t->reg == o->addr)
              { // found
                r = get_rgstat(o->addr);
                if (r) r->popped = 0;
                t->popped = 0;        // mark as pushed.
              }
            }

/*           if (!r && o->addr && nobank(o->val) > PCORG)
            { //push goes to different scan queue ??
             i = codebank(o->val,c);
             // add gapscan ??
           DBGPRT(1,0); // consider codebank(o->val) as scan_addr...... 
            }
*/

         }
    }    // end of if scanning

  if (s->emulating)
    {   // similar to push above, but do args if necessary
        // note that as this is done after each scan or rescan,
        // the stack is already built by sj subr....

     if (o->imd && val_rom_addr(o->addr))    //opcsub = 1
       {   // immediate value (address) pushed
          emuscan = add_escan(codebank(o->addr,c),s);            // add imd as a special scan
          fakepush(s, o->addr, 2);                               // imd push
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
                r = get_rgstat(o->addr);
                if (r)
                  { r->popped = 0;
                    if (t->newadd == t->origadd)
                     { // no change, delete it
                      #ifdef XDBGX
                      DBGPRT(1,"delete rgstat R%x", o->addr);
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
   if (zero_reg(o)) return;

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
              r = add_rgstat(o->addr, 15,0, 0);
              if (r) {
              r->popped = 1;
         //     r->used   = 0;
              }
             }
           break;
         }

       if (!t->popped)
         {   // not popped yet(or pushed back)
          t->reg = o->addr;
          t->popped = 1;
          o->val = get_stack(s, t, 3);                   // not pop flags
          r = add_rgstat(o->addr, 15, 0, o->val);       // find or add (was origadd)
           if (r) {
          r->popped = 1;                    // set popped for emu detect
          #ifdef XDBGX
          DBGPRT(1,"set Popped, WR%x=%x at %x", o->addr, o->val, c->ofst);
          #endif 
         }
          break;
         }
        t++;
      }
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
    if (s->emulating) set_psw(c, c->opr[1].val, c->opr[1].wfend,0x2e);
 }


void cpl  (SBK *s, INST *c)
  {
    // complement bits
    do_dflt_ops(s,c);
    c->opr[1].val = (~c->opr[1].val) & 0xffff;         // size ?

    if (s->emulating)
     {
      pset_psw(c, 0, 0xa);      // clear bits V,C
      set_psw(c, c->opr[1].val, c->opr[1].wfend, 0x30);     // N and Z
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

  if ((o->addr & 0x3ff) != 0x11) return;

  // specifically write to ibuf, to keep it for reads ?

  val = c->opr[1].val & 0xf;
  val++;
  upd_ram(0x11,c->opr[1].val,7);

  // do this only if it's an imd as this is reliable, else ignore it ???

  if (bkmap[val].bok)
    {              // valid bank
     basepars.datbnk = val << 16;
     #ifdef XDBGX
      DBGPRT(1,"%x R11 Write imd, DataBank=%d",c->ofst, val);
     #endif
    }
 }



void do_rgargs(INST *c)
{

   RST *org, *nrg;
//   OPS *sce, *dst;
   uint screg, scfend, scaddr, scval; 
   uint dsreg;


   if (c->opcsub == 1) return;    //ignore immediates

   screg = 0;

   if (c->sigix == 12)
     {                   //ldx
      if (!c->opcsub) screg = c->opr[1].addr;
      else screg = c->opr[4].addr;  
      scfend = c->opr[1].rfend;
      scval =  c->opr[1].val;
      scaddr = c->opr[1].addr;
      dsreg  = c->opr[2].addr;
     }

   if (c->sigix == 13)
     {                   //stx
      screg  = c->opr[2].addr;
      scfend = c->opr[2].rfend;
      scval =  c->opr[2].val;
      scaddr = c->opr[2].addr;
      dsreg = c->opr[1].addr;
     }


//   if (!c->opcsub)
//   sce = c->opr + six;                   //ldx
//   else sce = c->opr + 4;
//   dst = c->opr + c->wop;                //dst


// sce = c->opr + 2            // for stx
//   if (!c->opcsub) dst = c->opr + 1;                   //stx
 //  else dst = c->opr + 4;


      org = get_rgstat(screg);

      if (org)
        {     // found source reg - check if popped or arg
          if (org->arg || org->sarg)
              // reg is flagged as arg, so use size
           {
             // firstly set size, and reflect back to source.
             set_rgsize(org,scfend);
             nrg = get_rgstat(org->sreg);
             set_rgsize(nrg,scfend);             

              // if first level arg (next to popped)
             if (org->sarg)
               {
                // this 2nd level arg, copy sce from first level
                 //  how to stop the arg flag when left subr......... 
                nrg = add_rgstat(dsreg, scfend, org->reg, org->ofst); 
                if (nrg) {nrg->arg = 1; org->arg = 0;
                 if (org->enc) { 
                    nrg->enc = org->enc;   //src is encoded
                    nrg->fend = 15;                   // enc is always a word
                    nrg->data  = org->data;     // 'start of data' base reg
#ifdef XDBGX
DBGPRT(1,"ORG ENC");
#endif
} }
               }

             #ifdef XDBGX
              if (org) DBGPRT(1,"RG %x=%x to %x", screg, scval, dsreg);
             #endif

           }
         else
         if (org->popped)
              // reg flagged as popped so make [wop] an arg - would expect this to be indirect or indexed
           {
            nrg = add_rgstat(dsreg, scfend, org->reg, scaddr);          // [1] for address if indirect
            if (nrg)
             {
              nrg->arg = 1;
              nrg->sarg = 1;                // source level from pop

                 #ifdef XDBGX
              DBGPRT(1,"ADD ARG RG %x at %x sz%d", nrg->reg, c->ofst, nrg->fend);
              #endif
             }
           }
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
  RST  *r;    //, *b;
  int val, inx;

  calc_mult_ops(s,c);

  c->opr[1].val = c->opr[2].val;

  if (anlpass >= ANLPRT)  return;

  // now check if this is a stack operation
  // if it is a stack register, treat similarly to a PUSH

  o = c->opr + 4;             // to check inx register

  if (s->scanning) x = scanstack; else x = emulstack;

  if (val_stack_reg(o->addr) && s->emulating)
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
          r = get_rgstat(c->opr[2].addr);
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

  do_rgargs(c);

check_R11(c);
 }





void ldx(SBK *s, INST *c)
  {
   // does not change psw

   int inx;
   RST *org;    //, *nrg;
   OPS *o;    //, *sce;
   FSTK *t, *x;

   calc_mult_ops(s,c);

   o = c->opr+2;

   o->val = c->opr[1].val;        // [2] = [1] as default op

   if (anlpass >= ANLPRT)  return;

   if (s->scanning) x = scanstack; else x = emulstack;


  // check if [4] =  stack register. If so this is a load from stack,
  // analogous to a POP, and will always be indirect or indexed

  if (val_stack_reg(c->opr[4].addr))
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
         org = add_rgstat(o->addr, 15, 0, o->val);      //s->substart);          // find or add (word sized)
         t->reg = o->addr;
         t->popped = 1;                      // same as a pop
         if (org) org->popped = 1;                      // set popped for emu detect

            #ifdef XDBGX
         DBGPRT(1,"%x POP via LDX R%x=%x [%d] T%d", c->ofst, c->opr[2].addr, c->opr[2].val, inx, t->type);
         DBGPRT(1,"set popped, size = 2");
    #endif
         if (t->type == 1 && g_bank(o->val) != basepars.datbnk)
          {

           basepars.datbnk = g_bank(o->val);
           #ifdef XDBGX
            DBGPRT(1,"set dbnk %d", basepars.datbnk);
            #endif

            // set databank here for correct [Rx++] calls ? if t->type == 1
          }
        }
    }

  check_R11(c);

 // if (s->emulating)
  //  {
          // 2 = 1 for ldx (if not stack reg),
          // so get source register from 1 (or 4 if ind or inx)


//if (s->curaddr == 0x87aaa)
//{
//DBGPRT(1,0);
//}


     do_rgargs(c);
}
/*
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
             set_rgsize(org,o->rfend);

             org = find_rgstat(org->sreg);
             set_rgsize(org,o->rfend);             

             // but now, arg value is copied to a new register/addr, which MAY be used.....
             #ifdef XDBGX
              if (org) DBGPRT(1,"LDX %x=%x to %x", sce->addr, sce->val, c->opr[c->wop].addr);
             #endif

           }
         else
         if (org->popped)
              // reg flagged as popped so make [wop] an arg - would expect this to be indirect or indexed
           {
            nrg = add_rgstat(o->addr, o->rfend, c->opr[1].addr);          //org->ofst);     //s->substart);
            if (nrg)
             {
              nrg->arg = 1;
           //   nrg->sarg = 1;
              nrg->sreg  = org->reg;

                 #ifdef XDBGX
              DBGPRT(1,"ADD ARG LDX %x at %x sz%d", nrg->reg, s->start, nrg->fend);
              #endif
             }
           }
        }

  //    }

  }
*/



void orx(SBK *s, INST *c)
 {

  calc_mult_ops(s,c);

  if (c->opcix < 32)    c->opr[2].val |=  c->opr[1].val;   // std or
  else  c->opr[2].val ^=  c->opr[1].val;                   // xor

  if (s->emulating)
     {
      pset_psw(c, 0, 0xa);      // clear bits V,C
      set_psw(c, c->opr[2].val, c->opr[2].wfend, 0x30);     // N and Z
     }
 }

void add(SBK *s, INST *c)
 {
      // 2 or 3 op
   calc_mult_ops(s,c);

   c->opr[c->numops].val =  c->opr[2].val + c->opr[1].val;

   if (s->emulating)
   set_psw(c, c->opr[c->numops].val, c->opr[c->numops].wfend, 0x3e);
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
       set_psw(c, c->opr[c->numops].val, c->opr[c->numops].wfend, 0x3e);
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
     set_psw(c, c->opr[0].val, c->opr[1].rfend,0x3e);
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

    if (c->opr[ix].rfend > 7)              // long/word
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
  //   set_opdata(s->nextaddr+1,7);
     op_calc (b, 1,c);                      // 5 lowest bits only from firstb, from hbook

     if (b < 16) op_imd(c);                 // convert op 1 to absolute (= immediate) shift value
     op_calc (g_byte(s->nextaddr+2),2,c);
 //    set_opdata(s->nextaddr+2,7);
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
     set_psw(c, v, c->opr[2].wfend,0x30);               // Z and N
     mask =  get_sizemask(c->opr[1].rfend) + 1;         // get to 'size + 1' bit (= last shifted)
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

   if (s->scanning) add_dtk(s,c);
   if (s->emulating)
     {
      set_psw(c, c->opr[1].val, c->opr[1].rfend, 0x3e);
     }

 }

void dec(SBK *s, INST *c)
 {
    do_dflt_ops(s,c);
   // ignore for scans
      c->opr[1].val--;
    if (s->emulating)
     {

      set_psw(c, c->opr[1].val, c->opr[1].rfend, 0x3e);
     }
 }


int calc_jump_ops(SBK *s, int jofs, INST *c)
{
    // s->nextaddr must correctly point to next opcode.
    // returns jump address

     OPS *o;

     c->opcsub = 0;                           //no multimode
     jofs += s->nextaddr;                     // offset as calc'ed from NEXT opcode
     jofs = nobank(jofs);                     // pcj allow for wrap in 16 bits

     o = c->opr+1;                            // set up opr[1] as a jump (=address)
     o->adr = 1;
     o->rgf = 0;
     o->rbs = 0;
     o->addr = codebank(jofs, c);             // add CODE bank for jumps
     if (val_rom_addr(o->addr)) o->val = g_val(o->addr,0, 15);     // get value of jump address for display ??
     else o->val = 0;
     jofs = c->opr[1].addr;                   // now is full address with bank
     return jofs;
  }


void cjm(SBK *s, INST *c)
 {    // conditional jump, (short)
    // all types here based upon psw

   s->nextaddr += 2;
   calc_jump_ops(s,g_val(s->nextaddr-1,0,39),c);   //signed byte offset

  // set_opdata(s->nextaddr-1,7);

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

    c->opr[3].bit = 1;                      // setup bit number in op3
    c->opr[3].rgf = 0;
    c->numops++;                            // local change, NOT opcode size.
    op_calc (g_byte(xofst) & 7, 3,c);       // bit number (in opcode)
    op_calc (g_byte(xofst+1), 2,c);         // register
 //   set_opdata(xofst+1,7);
    calc_jump_ops(s, g_val(xofst+2,0,39),c);   // jump destination, signed byte
 //   set_opdata(xofst+2,39);
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
   //  CANNOT do this as part bit field as it is REVERSE of std word

    int jofs;

    jofs = sjmp_ofst(s->nextaddr);

 //   set_opdata(s->nextaddr+1,7);
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
   jofs = g_val(s->nextaddr-2,0,47);        //signed word
   calc_jump_ops(s, jofs,c);
 //  set_opdata(s->nextaddr-2,47);
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
    calc_jump_ops(s,g_val(s->nextaddr-2,0,47),c);         //signed word
 //   set_opdata(s->nextaddr-2,47);


 /*  if (s->lscan)
    {
      #ifdef XDBGX
       if (anlpass < ANLPRT) DBGPRT(1,"ignore call %x from %x", c->opr[1].addr, s->curaddr);
      #endif
      return;
    }
*/

    if (s->scanning)
      {
   #ifdef XDBGX
    if (anlpass < ANLPRT) DBGPRT(1,"call %x from %x", c->opr[1].addr, s->curaddr);
    #endif
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

     #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opr[1].addr, s->curaddr);
     #endif

     n = add_escan(c->opr[1].addr, s);
     fakepush(s, c->opr[1].addr,1);            // std subr call, push return details onto stack

     scan_blk(n, &einst);                      // recursive

// break here ???  if args set ??

    }
 }


void scl(SBK *s, INST *c)
 { // short call
   int jofs;
   SBK *n;

   jofs = sjmp_ofst(s->nextaddr);

  // set_opdata(s->nextaddr+1,7);
   s->nextaddr+=2;
   calc_jump_ops(s,jofs,c);

 /*  if (s->lscan)
    {
      #ifdef XDBGX
       if (anlpass < ANLPRT) DBGPRT(1,"ignore call %x from %x", c->opr[1].addr, s->curaddr);
      #endif
      return;
    }
*/

   if (s->scanning)
     {

   #ifdef XDBGX
  if (anlpass < ANLPRT) DBGPRT(1,"call %x from %x", c->opr[1].addr, s->curaddr);
  #endif

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




   #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opr[1].addr, s->curaddr);
     #endif



     n = add_escan(c->opr[1].addr, s);
     fakepush(s, c->opr[1].addr,1);              // std subr call, push return details onto stack

     scan_blk(n, &einst);                          // recursive (as true core)
    }

 }


void ret(SBK *s, INST *c)
 {           // return

   s->nextaddr++;
   calc_jump_ops(s, -1,c);      // do as loopstop jump


   if (s->scanning)
    {
 //    #ifdef XDBGX
 //      if (chrgst.num > 0) DBGPRT(1,"Clear rgstats");
  //   #endif

  //   clearchain(&chrgst);         // clear regstat want to ditch this !!
     basepars.rambnk = 0;         // safety
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
     DBGPRT(2,0);
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
 //  set_opdata(xofst+1,7);
   calc_jump_ops(s,g_val(xofst+2,0,39),c);      //signed byte
 //  set_opdata(xofst+2,39);
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
         r = get_rgstat(o->addr);
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

   int bk;
   //   According to Ford book, this only sets PSW, so could be cancelled by a POPP
   //   or PUSHP/POPP save/restore,  so may need a PREV_BANK

   bk = c->opcix - 107;                   // new RAM bank
   basepars.rambnk = bk * 0x100;                 // establish new offset

   op_imd(c);                                    // mark operand 1 as immediate
   c->opr[1].val = bk;
   c->opr[1].rfend = 7;                           // byte sized; 
   c->numops = 1;                                // fake a single byte [imd] op
   s->nextaddr++;

   #ifdef XDBGX
   if (s->scanning) DBGPRT(1,"New Rambank = %x at %x", bk, s->curaddr);
   #endif
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

  basepars.codbnk = g_bank(s->curaddr);                // current code bank from ofst

  if (s->stop)
   {
    #ifdef XDBGX
     DBGPRT(1,"Already Scanned s %x c %x e %x", s->start, s->curaddr, s->nextaddr);
    #endif
    return;
   }

    if (s->curaddr > maxpcadd(s->start) || s->curaddr < minadd(s->start))
    {
      #ifdef XDBGX
      if (anlpass)  DBGPRT(1,"Scan > Illegal address %x from blk %x, STOP", s->curaddr, s->start);
      #endif
      s->stop = 1;
      return;
    }


  if (!val_rom_addr(s->curaddr))
    {
     #ifdef XDBGX
     if (anlpass) DBGPRT(1,"Invalid code address %05x", s->curaddr);
     #endif
     s->inv = 1;
     s->stop = 1;
     s->nextaddr = s->curaddr+1;
     return;
    }

  xofst = s->curaddr;                             // don't modify curaddr, use temp instead

//----------------------------

  psw = c->psw;                                     // keep psw from supplied instance
  memset(c,0,sizeof(INST));                         // clear entire INST entry
 
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
        tbk++;

        #ifdef XDBGX
        if (anlpass && anlpass < ANLPRT && !bkmap[tbk].bok)
                    DBGPRT(1,"ROMBNK - Invalid bank %d (%x)", tbk, c->ofst);
        #endif
        c->bank = tbk;                               // set bank for this opcode
        x = g_byte(xofst);                           // get true opcode
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
          if (anlpass) DBGPRT(1,"Invalid opcode (%x) at %x [%d]", x, s->curaddr, anlpass);
       #endif

       if (s->scanning) end_scan (s);
       else  if (anlpass) wnprt(1,"Invalid opcode at %x [%d]", s->curaddr, anlpass); // report only once

       return;
     }

  c->opcsub = x & 3;                               // this is valid only for multimode opcodes
  c->opcix = indx;                                 // opcode index
  c->sigix = opl->sigix;                           // sig index
  c->ofst  = s->curaddr;                           // original start of opcode

  set_opstart(s->curaddr);                         // code start flag
 
  // These next items can be changed by opcodes, this is default setup

  c->numops = opl->nops;
  c->psw   = psw;                                 // carry over psw from last instance
  o = c->opr;                                     // shortcut to operands (4 off)
  c->wop = opl->wop;                              // write destination index (or zero)
  o[opl->wop].wfend = opl->wfend;                 // set write size (or zero)
  o[1].rfend = opl->rfend1;                       // operands - read size
  o[2].rfend = opl->rfend2;
  o[3].rfend = opl->rfend3;
  o[1].rgf  = 1;                                  // set register ops by default
  o[2].rgf  = 1;
  o[3].rgf  = 1;


//if (s->curaddr == 0x9342a)
//if (s->curaddr == 0x9224e)
//{
//DBGPRT(1,0);
//}


  s->nextaddr = xofst;                      // address of actual opcode - for eml call

  opl->eml(s, c);                           // opcode handler - modifies nextaddr
                                            // s->nextaddr now points to next opcode
  upd_watch(s, c);


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
 // JMP *j;

//if (ofst == 0x92ba9)
//{                         //xdt2
//DBGPRT(1,0);
//}



  memset(&prtscan,0, sizeof(SBK));     // clear it first
  c = &cinst;
  prtscan.curaddr = ofst;
  prtscan.start = ofst;
  do_code(&prtscan, c);

  fcmnt.minst = c;           // for later comments
//  preset_comment(ofst);  // begin and end reqd ??

  if (prtscan.inv)
    {
     pp_hdr (ofst, opctbl->name, 1);   // single invalid opcode (= !INV!)
     return prtscan.nextaddr;
     }

  // BANK - For printout, show bank swop prefix as a separate opcode
  // This is neatest way to show addresses correctly in printout
  // internally, keep 'ofst' at original address, but update curaddr for printout.

  if (c->bank)
    {
     pp_hdr (ofst, "rombk", 2);
     pstr (0,"%d", c->bank-1);
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


if (ofst == 0x92ba9)
{                         //xdt2
DBGPRT(1,0);
}



  if (PSSRC)
    {
      if (opl->sce)
       {               // skip if no pseudo source
        p_pad(PSCEPOSN);
   //     pstr("|%d ", indent);         // experiment, for lining up and new brackets

        tx = opl->sce;                 // source code, before any mods
        eqp = 1;                       // equals not printed yet

        // any mods to source code go here

        fix_sym_names(&tx,c);           // for rbases, 3 ops, names, indexed fixes etc.
    //    shift_comment(c);               // add comment to shifts with mutiply/divide  - if (curinst.opcix < 10 && ops->imd)

//        j = 
        find_fjump (ofst, &jf);

        if ((jf & 1) && *tx)
         {                             // invert jump logic for source printout
           if (inx > 53 && inx < 72)   // all conditional jumps (except djnz)
           {
             ainx = inx ^ 1;           // get alternate opcode index if jump found.
             tx = opctbl[ainx].sce;    // and source code
//indent++;
           }
           else jf = 0;
        }

       //  do 'psuedo source code' output

       while (*tx)
        {

         switch (*tx)   // 1-12 used  1-31 available (0x1 to 0x1f)
          {
           case 1:
           case 2:
           case 3:
           case 0x11:           // + 0x10 = print size,sign
           case 0x12:
           case 0x13:

             p_opsc (c, *tx, eqp);
             break;

           case 4:
             // replace bit masks with flag names - ANDs and ORs  //MORE//  if (ix >  9 && ix < 14) ||  (ix > 29 && ix < 34)
             bitwise_replace(&tx, c);
             break;

           case 5:
             // subr call, may have answer attached.
             pp_answer(c);
             p_opsc (c, 1, eqp);
             prtscan.nextaddr += pp_subargs(c,prtscan.nextaddr);
             break;

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
                pstr (0," {");   // [if] case
                break;
              case 2 :
                pstr (0,"return;");  // static jump to a return
                break;
              case 3:
                pchar(';');
                // fall through
              case 4:
                pstr(0,"} else {");
                break;
              default:
                pstr (0,"goto ");
                p_opsc (c,1,0);
                pchar (';');
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

           case '=' :
             eqp = 0;                 // equals printed, read syms after this
              // fall through
           default:
             pchar(*tx);
          }         // end switch tx
          tx++;
        }           // end while tx
      }             // end if opl->sce
   }                // end if PSSRC
  else
     {      // subr arguments without source code option - must still print and skip operands
      if (!PSSRC && (inx == 73 || inx == 74 || inx == 82 || inx == 83))          // CALL opcodes
         {
           prtscan.nextaddr += pp_subargs(c,prtscan.nextaddr);
         }
     }

     i = find_tjump (prtscan.nextaddr);         // add any reqd close bracket(s)

     while (i > 0)
      {               // i = number of brackets (=jumps) found
       pstr (0," }");
       i--;
      }

  //   if (PACOM) parse_comment(&acmnt,0);                   // print auto comment if one created, but need to check ofst !!

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

 if (prtscan.nextaddr <= k->end)
  {
   if (inx > 75 && inx < 80) pp_comment(prtscan.nextaddr,1);   // Rets
   if (inx == 75 || inx == 81)  pp_comment(prtscan.nextaddr,1); //jumps
  }
 return prtscan.nextaddr;
}



// ---------------------------------------------------------------------------------------------


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

   c->error = 0;   // warning (default), continue
   if (*t == 15)  c->error = 2;      // fatal error
   if (*t == 1)   c->error = 1;        // error

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
   return 1;
}


uint set_rbas (CPS *c)
{
 RBT *x;

//addresses checked already in parse_cmd 

  if (c->npars < 3)
   {
    c->p[2] = 0;         // default address range to ALL (zeroes) otherwise done by cmd
    c->p[3] = 0xfffff;   // max possible address
   }

 x = add_rbase(c->p[0], c->p[1], c->p[2], c->p[3]);

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
  a = start_adnl_loop(&chadnl, blk->start); //(ADT*) chmem(&chadnl,0);
 // a->fid = blk;

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

void add_ans(uint start, ADT *ca)
{  //special adt copy for subr answers
 ADT *a;

// insert ino answer chain
  a = add_adt(&chans, start, 1);        // add new one
  *a = *ca;
  a->fkey = set_adt_fkey(start,1);

    #ifdef XDBGX
    DBGPRT(0,"      with answer params");
    prt_adt(&chans, start, 0, DBGPRT);
    DBGPRT(1,0);
  #endif

}

void cpy_adt(CPS *c, uint start)                 //, int start)
{ 
 // move adt blocks from cmnd to new LBK
 // NB. create NEW ones as fid will be in worng place otherwise in adt chain

//  int i;
  uint seq;
  ADT *a, *cp;
  if (!start) return;                      // must have an fid
  if (!c->seq) return;                   // no addnl blocks

  a = start_adnl_loop(&chadcm, 0);    //c->start);      //(ADT*) chmem(&chadnl,0);
 // a->fid = c->fid;                  //command adnl entries

  while ((a = get_next_adnl(&chadcm,a)))
    {
 //     if (a->seq > c->seq) break; don't need it, chain cleared each command
      if (a->ans)
       {
        add_ans(start,a);        // add answer (in different chain)
       }
      else

      if (a->cnt)
       {          // use cnt as null checker
        if (c->fcom == C_ARGS || c->fcom == C_SUBR) a->pfw = 0;    // clear all pfw
        cp = add_adt(&chadnl,start,256);      // add new one with correct fid and seq
        seq = cp->fkey;       // save key 
        *cp = *a;             //copy block
        cp->fkey = seq;       // restore key
        //set decimal for tabs and funcs
        if ((c->fcom == C_TABLE || c->fcom == C_FUNC) && cp->prdx == 0) cp->prdx = 2;
       }
    }

    #ifdef XDBGX
    DBGPRT(0,"      with addnl params");
    prt_adt(&chadnl,start, 0, DBGPRT);
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

//  row = d->maxadt;      //max levels
//  bytes = d->minadt -1;      // min levels

  if (!d->maxadt && c->seq)
    {     //no extra allowed
     return do_error(c,"\001Extra\012\005");
    }

  if (c->seq < d->minadt )
    { // not enough levels
     return do_error(c,"\001Extra\012\004");
    }

   if (c->seq > d->maxadt)
     {
        do_error(c,"\002Extra\012\010");
        c->seq = d->maxadt;
     }


// check extra levels and sizes 

  bsize = c->p[1] - c->p[0]+1-c->term;    // byte size of cmd entry in total

  if (d->defsze)  tsize = d->defsze; else tsize = c->tsize; 
  
  if (!tsize)
    {
      do_error(c,"Row size is ZERO !!");
      tsize = 1;
     }

   if (tsize > bsize)
     {
      if (c->p[0] == c->p[1] || d->defsze)
        {  // if end = start, or preset sizes, allow change
         c->p[1] = c->p[0] + tsize-1;
         bsize = tsize;
         do_error(c,"\002End increased to ");
         paddr(c->p[1],0,wnprt);
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

  int val, fend, startval;
  LBK *blk;
  ADT *a, *b;
  // size row vs. start>end check first

  a = get_adnl(&chadcm, 0, 1);

  if (c->seq < 2)
    {   // add another level if only one specified
     b = add_adt(&chadcm, 0, 256);
     *b = *a;               // copy all data
     b->fkey = 2;           // but reset seq 
     c->seq = 2;            // levels = 1;         // up levels
     c->tsize = totsize(&chadcm, 0);
    }

  // c->tsize must be correct first

  if (chk_csize(c)) return 1;

  fend = a->fend;                   //size and sign as specified

  // check start value consistent with sign

  val   = g_val(c->p[0], 0, fend);
  startval = get_startval(fend);              // start value from fend SIGN

  if (val != startval)  // try alternate sign....does not change command though
    {
     fend ^= 32;                             // swop sign flag
     startval = get_startval(fend);          // new start value
     if (val != startval) do_error(c,"\002Function Start value\003");
     else  do_error(c,"\002First value (%x) indicates wrong sign specified", val);
    }

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chcmd.lasterr) return  do_error(c,etxts[chcmd.lasterr]);

  cpy_adt(c,blk->start);
  blk->size = totsize(&chadnl, blk->start);

  return 0;
}

uint set_tab (CPS *c)
{
   // tables, ONE level only

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chcmd.lasterr) return do_error(c,etxts[chcmd.lasterr]);


  cpy_adt(c,blk->start);
  blk->size = totsize(&chadnl,blk->start);

 if (c->argl) blk->argl = 1;     // default is argl OFF
 return 0;
}


uint set_stct (CPS *c)
{
   // structures

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chcmd.lasterr) return do_error(c,etxts[chcmd.lasterr]);      //do_error(c,2,0);

  cpy_adt(c,blk->start);
  blk->size = totsize(&chadnl,blk->start);

  blk->term = c->term;                    // terminating byte flag (struct only)
  if (c->argl) blk->argl = 1;             // default is arg layout

  set_data_vect(blk);   // for Refs out in structs
  return 0;
}





uint set_data (CPS *c)
{
   // change for word, byte, long
   // no addnl is fine

  LBK *blk;
  ADT *a;

  if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // start,end, cmnd. by cmd
  if (chcmd.lasterr) return do_error(c,etxts[chcmd.lasterr]);

  a = c->adnl;

  if (a && bytes(a->fend) < c->fcom)  a->fend = (c->fcom * 8) -1;     // safety for command sizes

  //if (a && a->bsize < c->fcom)  a->bsize = c->fcom;

  cpy_adt(c,blk->start);
  blk->size = totsize(&chadnl,blk->start);

  return 0;
}


uint set_sym (CPS *c)
{
  int fstart,fend,write;    //,i, flags
  ADT *a;
  SYM  *s;

// flagsword.
// allow/inhibit if immediate...


  write = 0;           // read or write
 // flags = 0;           // flags word
  fstart = 32;
  fend = 31;

  if (c->npars < 3)
   {
    c->p[1] = 0;         // default address range to ALL (zeroes) otherwise done by cmd
    c->p[2] = 0xfffff;   // max possible address
   }

// only one adt level allowed

// for (i = 0; i <= c->levels; i++)
 //  {
    a = get_adnl(&chadcm, 0, 1);     //cmdadnl + i;           //->adnl;
    if (a)
     {
      if (a->fend == 15)  write = 1;      // write 'W'

      if (a->enc == 7 && a->fstart < 31)
        {        // bit marker 'B'
         fstart = a->fstart;
         fend   = a->data & 0x1f;
        }
     }

  s = add_sym(c->symname, write|C_CMD, c->p[0], fstart, fend, c->p[1], c->p[2]);
  
  if (chsym.lasterr && chsym.lasterr != E_DUPL)   return do_error(c,etxts[chsym.lasterr]);
     
  if (c->flags) s->flags = 1;          // with flags (not for bits...)
  if (c->names) s->names = 1;          // with flags (not for bits...)
  if (c->imm)   s->immok = 1;
  return 0;
}




uint set_subr (CPS *c)
{

  SUB *xsub;
  SPF *f;

  // if (chk_csize(c)) return 1;  can't do this !!

  xsub = add_subr(c->p[0]);

  // can do name here, even if a duplicate.............

  if (!xsub || xsub->cmd)
    {   //allow def of existsing system one
     if (chsubr.lasterr) return do_error(c,etxts[chsubr.lasterr]);
    } 
  else xsub->cmd = 1;

  // do any special functions first

  f = 0;

  if (c->spf)
     {            // special func is marked

      f = add_spf(xsub->start);
      f->spf = c->spf;   // start of funcs
      f->fendin  = anames[f->spf].fendin;
      f->fendout = anames[f->spf].fendout;
      f->addrreg = c->adreg;          // register
      f->sizereg = c->szreg;             // cols

      #ifdef XDBGX
        DBGPRT(0, "  with spf = %x ", f->spf);
      #endif
    }  // end special func

  add_scan (c->p[0], J_SUB,0);

  cpy_adt(c, xsub->start);

  xsub->size = totsize(&chadnl,xsub->start);
  if (c->cptl) xsub->cptl = 1;     // default is argl ON
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

  blk = add_cmd (c->p[0],c->p[1],c->fcom|C_CMD,0);
  if (chcmd.lasterr) return do_error(c,etxts[chcmd.lasterr]); 
  cpy_adt(c,blk->start);
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

// need more than this........................

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
  add_cmd (start,ofst, C_TEXT |C_SYS,0);
  add_cmd (ofst+1,b->maxbk, C_DFLT|C_SYS,0);    // treat as cmd (cannot override)

  ans = start - 1;

  ofst = (0xff06 | bk);
  cnt = 0;
  val = g_byte(ofst);

  while (ofst < (0xff63 | bk))
   {     // allow a few non printers...6 ?
     if (val < 0x20 || val > 0x7f)
      {
        if (cnt > 5) add_cmd (ofst-cnt,ofst-1, C_TEXT |C_SYS,0);
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












void find_fill_txt(BANK *b)
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
       add_cmd (ofst,last, C_DFLT|C_SYS,0);
       add_aux_cmd (ofst,b->maxpc,C_XCODE|C_SYS,0);       // and xcode to match
       b->maxpc = ofst;
      }
  }


void mkbanks(int num, ...)
{
 va_list ixs;
 int i, j, k;
 BANK *b;

 va_start(ixs,num);

 j = 0;
 for (i = 3; i < 7; i++)
    {
     k = va_arg(ixs,int);
     b = bkmap + i;
     b->dbank = k;
//     b->bprt = 1;  //print in msg file
     j++;

     # ifdef XDBGX
       DBGPRT(1,"Set temp bank %d to %d",i, k);
     #endif

     if (j >= num) break;
    }
 va_end(ixs);

}


/*
uint val_cmdbank(uint bk)
{  // 0,1,8,9 only allowed, val by bitmask
  if (bk > 15) return 0;
  if (bk & 6) return 0;
  return 1;
}*/


uint set_bnk (CPS *c)
{
  int i, bk, bank, add;
  BANK *b;

  // bank no , file offset,  (opt) pcstart, (opt)  bkend  , (opt) fillstart

   // use bk[2].cmd as marker to indicate bank command used already for
   // following bank commands, as this would not get set otherwise.

   // validate all params before overwriting anything

   if (c->npars < 2) return do_error(c,"\001need two\011");

   bk = c->p[0];          // first param is BANK

   //if (!val_cmdbank(bk)) return do_error(c,"\001\003 Bank Number");

   if ((bk & 6) || bk > 15) return do_error(c,"\001\003 Bank Number");

   if (bkmap[bk].cmd)   return do_error(c,"\001Bank already defined");

   bk++;
   bank = bk << 16;          // bank in correct form

   // 2nd is file offset
   if (c->p[1] < 0 || c->p[1] > (int) fldata.fillen) return do_error(c,"\001\003 File Offset");

   if (bk > 0 && bkmap[bk-1].bok)
      {        //check overlap in file offset
       if (c->p[1] <= (int) bkmap[bk-1].filend) return do_error(c,"\001File offsets\006");
      }

   if (c->npars > 2)
     {     // pc start specified (for non std offset) (p[0] bank, p[1] start file offset)
      add = nobank(c->p[2]);
      if (add < PCORG-2 ||  add > PCORG+3 ) return do_error(c,"\001 Bank Start");
     }

   if (c->npars > 3)
     {     // bank end specified. p[0] bank, p[1] start file offset)
      add = nobank(c->p[3]);
      if (add > 0xffff) return do_error(c,"\001 Bank End");

      add -= nobank(c->p[2]);       // size, end-start.
      add++;

      if ((c->p[1] + add) > (int) fldata.fillen) return do_error(c,"\001Bank end > File Size"); 

     }


//  if (c->npars > 4)      //fill specified zero otherwise


// then sort fillstart

// NEED MORE CHECKS HERE TO STOP CRASHES and set up data flag markers ..............


// clear valid map entries

   if (!bkmap[3].cmd)
      {    //first use of 'bank' command - clear whole map.
  #ifdef XDBGX
     DBGPRT(1,"Banks CLEARED ***************");
   #endif
       for (i = 0; i < BMAX; i++)
          {
           b = bkmap+i;
           b->bok = 0;
           b->cbnk = 0;
           b->bprt = 0;
           b->bkmask = 0;
          } 
       numbanks = -1;                // no banks
       bkmap[3].cmd = 1;
      }

   b = bkmap+bk;
   b->cmd = 1;
   b->bok = 1;
   b->bprt = 1;    // print in msg file
   b->filstrt = c->p[1];

   if (c->npars > 2) b->minpc = nobank(c->p[2]) | bank;
   else  b->minpc = PCORG | bank;

   if (c->npars > 3) b->maxbk = nobank(c->p[3]) | bank;
   else  b->maxbk = 0xffff;

   b->filend =  b->filstrt + (b->maxbk - b->minpc);

   // fldata is still valid...
   if (b->filend > fldata.fillen)
      {
       b->filend = fldata.fillen-1;
       b->maxbk = (b->filend-b->filstrt) + b->minpc;
      }

    b->maxpc = b->maxbk;

// file read ???     anything ??

  b->fbuf = (uchar*) mem(0,0, 0x10000);          // big enough for file TOO BIG

  fseek (fldata.fl[0], b->filstrt,SEEK_SET);        // set file start - CHECK ERROR

  fread (b->fbuf + 0x2000, 1, b->filend-b->filstrt+1, fldata.fl[0]);

/*  #ifdef XDBGX
   DBGPRT(1,"Read input file (0x%x bytes)", fldata.fillen);
  #endif
  for (i = 3; i < 7; i++)
   {
     from = bkmap + i;            // temp bank
     if (from->bok)
      {        // valid to transfer 
       from->dbank++;
       to   = bkmap + from->dbank;        // new bank
       bk =  from->dbank << 16;

       *to = *from;             // lazy, may change this
       size = to->filend-to->filstrt+1;

       to->fbuf = (uchar*) mem(0,0, 0x10000);   //malloc a full bank.

       memcpy(to->fbuf+nobank(from->minpc), from->fbuf+0x2000, size);
       to->minpc &= 0xffff;
       to->minpc |= bk;    // substitute bank no

       to->maxbk &= 0xffff;
       to->maxbk |= bk;
       to->bok = 1;
       to->maxpc = 0;       // do the fill check
       find_fill_txt(to);
       from->bok = 0;        // clear old bank
      }


/ cross check banks too for maxbk ????
 bkmap[3].opcbt = opcbit;                   // bit flag array start (uints)
  bkmap[4].opcbt = opcbit + 0x800;           // = 800*20 = 10000 (32 bit uints)
  bkmap[5].opcbt = opcbit + 0x1000;
  bkmap[6].opcbt = opcbit + 0x1800;

  bkmap[3].opdbt = opdbit;                   // bit flag array start
  bkmap[4].opdbt = opdbit + 0x800;           // = 800*20 = 10000 (32 bit uints)
  bkmap[5].opdbt = opdbit + 0x1000;
  bkmap[6].opdbt = opdbit + 0x1800;

*/
   numbanks++;

b->opcbt = opcbit + (0x800*numbanks);
//b->opdbt = opdbit + (0x800*numbanks);

  basepars.codbnk = 0x90000;                         // code start always bank 8
  basepars.rambnk = 0;                               // start at zero for RAM locations
  if (numbanks) basepars.datbnk = 0x20000;           // data bank default to 1 for multibanks
  else   basepars.datbnk = 0x90000;                  // databank 8 for single

   
   #ifdef XDBGX
     DBGPRT(0,"Bank Set %d %x %x ",bk, b->minpc,b->maxbk);
     DBGPRT(1," #(%x - %x  fill %x)",b->filstrt, b->filend, b->maxpc);
   #endif
   return 0;
 }


uint set_vect (CPS *c)
{                          // assumes vect subrs in same bank as pointers
  uint  ofst, bank;
  int i;
  LBK *blk;
  ADT *a;

  if (chk_csize(c))   return 1;

  blk = add_cmd (c->p[0],c->p[1], C_VECT|C_CMD,0);   // by cmd

  if (PMANL) return 0;
  if (chcmd.lasterr)  return do_error(c,etxts[chcmd.lasterr]);

  if (c->seq)                 //levels == 0)
   {
     a = get_adnl(&chadcm,0, 1);             // check addn entry
     bank = a->bank << 16;
     if (!bkmap[a->bank].bok) return do_error(c,"\001Bank\003");
     cpy_adt(c,blk->start);
   }
  else bank = g_bank(c->p[0]);

  for (i = c->p[0]; i < c->p[1]; i += 2)
    {
      ofst = g_word (i);
      ofst |= g_bank(bank);
      add_scan (ofst, J_SUB,0);      // adds subr
      
    }
  return 0;
}

uint set_code (CPS *c)
{

  add_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);   // by cmd
  if (chcmd.lasterr) do_error(c,etxts[chcmd.lasterr]);
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

  blk = add_aux_cmd (c->p[0], c->p[1], c->fcom|C_CMD,0);  // by cmd

  if (chaux.lasterr)  return  do_error(c,etxts[chaux.lasterr]);

  // must clear all fieldwidths for args command

  cpy_adt(c,blk->start);
  set_data_vect(blk);

  blk->size = totsize(&chadnl,blk->start);

  #ifdef XDBGX
  DBGPRT(1,0);
  #endif
return 0;
}

uint set_cdih (CPS *c)
{
    // xcode commands
  add_aux_cmd(c->p[0],c->p[1],c->fcom|C_CMD,0);
  if (chaux.lasterr) do_error(c,etxts[chaux.lasterr]);
  return 0;
}

uint set_psw (CPS *c)
{
    // psw setter p[0] checked, but not p[1]
  c->p[1] = fix_addr_bank(c->p[1]);      // force bank 9 if single bank
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

  pstr(1,0);            //newline
  pstr(1,"####################################################################################################");
  pstr(0,"#\n# Disassembly listing of binary '%s'  ",fldata.bare);

  //pstr(0,"  806%d%c %d bank%c ", P8065 ? 5 : 1, PSTPD ? 'D' : ' ',  numbanks+1, numbanks ? 's' : ' ');
  pstr(0,"  806%d  %d bank%c ", P8065 ? 5 : 1, numbanks+1, numbanks ? 's' : ' ');

  pstr(0,"   SAD Version %s (%s)", SADVERSION, SADDATE);
  pstr(0,"\n# See '%s%s' for warnings, results and other information", fldata.bare, fd[2].suffix);
  pstr(0,"\n\n# Explanation of extra flags and formats - ");
  pstr(0,"\n#   R general register. Extra prefix letters shown for mixed size opcodes (e.g DIVW)");
  pstr(0,"\n#   l=long (4 bytes), w=word (2 bytes), y=byte, s=signed. Unsigned is default.");
  pstr(0,"\n#   [ ]=use value as an address   (addresses are always word) ");
  pstr(0,"\n#   @=value is an address (to symbol),  '++' increment register after operation." );
  pstr(0,"\n# Processor status flags (for conditional jumps)" );
  pstr(1,"\n#   CY=carry, STC=sticky, OVF=overflow, OVT=overflow trap");
#ifdef  XDBGX
pstr(0,"\n# - - - -  DEBUG  - - - - - ");   
#endif
  pstr(1,"#\n####################################################################################################");


 for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;

     if (!b->bok) continue;

     if (numbanks)
      {
       pstr (2,0);
       pstr (1,"###########################################################################");
       pstr (0,"# Bank %d  file offset %x-%x, ", i-1, b->filstrt, b->filend);
       paddr(b->minpc,0,pstr);  pstr(0," - "); paddr(b->maxbk,0,pstr);
       if (i == 9) pstr(0, "  CODE/BOOT starts HERE"); 
       pstr(1,0);
       pstr (2,"###########################################################################");
      }

  //   indent = 0;
     lastcom = 0;

     for (ofst = b->minpc; ofst < b->maxbk; )
      {
         // first, find command for this offset

       x = get_prt_cmd(&prtblk, b, ofst);

       // add newline if changing block print types
       if (dirs[x->fcom].prtcmd != dirs[lastcom].prtcmd || dirs[x->fcom].prtcmd == pp_stct) {pp_comment(ofst,1);}
       lastcom = x->fcom;

       while (ofst <= x->end)
         {
          show_prog (anlpass);
          pp_comment(ofst,0);
          ofst = dirs[x->fcom].prtcmd (ofst, x);
         }
       }      // ofst loop

     pp_comment(b->maxbk,0);     //+1,0);             // any trailing comments at bank or listing end
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


// ********** command processing section **************


void add_iname(int from, int ofst)
{
  // add interrupt autonames
  uint i, x;
  char *z;

  // ignore_int can be at the end of a jump in multibanks....

 // if (!PINTF) return;      // moved out to call
  from &= 0xffff;            // drop bank
  from -= 0x2010;
  from /= 2;                 // correct index for table lookup

  x  = (P8065) ? 1 : 0;     // 8061 or 8065
  z = nm;

  if (numbanks)               // sort out bank number
   {
    i = ofst >> 16;             // add bank number after the I
    z += sprintf(nm, "I%u_",i-1);
   }
 else
    z += sprintf(nm, "I_");         // no bank number

 if (match_sig(&intign, ofst,0))       // this is 'ignore int' signature
    {
      sprintf(z,"Ignore");
      add_sym(nm, C_SYS, ofst,32,32, 0,0xfffff);

      return;
    }

 for (i = 0; i < NC(inames); i++)
    {
    if (from <= inames[i].end && from >= inames[i].start && inames[i].p5 == x)
       {
       z += sprintf(z,"%s",inames[i].name);                      // number flag set
       if (inames[i].num) z+=sprintf(z, "%u", from-inames[i].start);
       add_sym(nm, C_SYS, ofst,32,32,0,0xfffff);
       break;    // stop loop once found for single calls
       }
    }
return;
 }



/*******************command stuff ***************************************************/

int ismypunc(char *c)
{
  if (*c == ' ')  return 1;
  if (*c == ',')  return 1;
  if (*c == '\t') return 1;  
  return 0;
}

int readpunc(CPS *c)
 {
  // read punctuation between commands and numbers etc.
  // keeps track of position (for errors)

    while (c->posn <= c->maxlen)
     {
       if (ismypunc(flbuf+c->posn)) c->posn++;
       else break;
     }

  if (c->posn >= c->maxlen) return 0;            // hit end of line

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
  char *t;

  ans = 0;

  while(ix < 8)
   {
     if (sscanf(flbuf+c->posn, "%5x%n", &v, &rlen) > 0)
       {
        if (chkpunc(c, rlen)) break;
        // special check for leading zero addresses
        // flag as special bank 32 for input bank fix 
        t = flbuf + c->posn;
        if (*t == '0' && rlen == 5) v |= 0x200000;

        c->p[ix] = v; 
        c->posn += rlen;
        ans++;
        }
     else break;
    readpunc(c);
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
    readpunc(c);
    ix++;
   }

  return ans;
 }



int getstr(CPS *c, const char **s, uint size)
 {
     // get string max of 63 chars and compare to string array 's'
  int ans, rlen;
  uint j;

  ans = sscanf (flbuf+c->posn, "%64s%n", nm, &rlen);    // n is no of chars read, to %n
  if (ans >= 0) c->posn += rlen;

  if (ans <= 0) return ans;          // nothing to read

  ans = -2;         // string not found.

  for (j = 0; j < size; j++)
   {
    if (!strncasecmp (*(s+j), nm, 3) )      // 3 chars min match
      {
       ans = j;                 // found
       break;
      }
   }

   return ans;
 }

uint set_lay (CPS *c)
{
// set layout columns
/*  c->npars is what's read in (

// npars = 1 p = 5    6-npars 0->5
// npars = 2 p = 4,5          0->4, 1 ->5
// npars = 3 p = 3,4,5        0->3, 1->4, 2->5

// address and header bytes from zero
// could cut one from these for single bins
#define MNEMPOSN   cposn[1]   where opcode mneomic starts (26)
#define OPNDPOSN   cposn[2]   where operands start    (32)
#define PSCEPOSN   cposn[3]   where pseudo code starts (49) 
#define CMNTPOSN   cposn[4]   where comment starts (83)
#define WRAPPOSN   cposn[5]   where line finishes (180)
//must verify too from stdpos

rules -

cannot change first 2, so max of 3 values.
if no psudocode, can move comment start down to 49.
49 is minimum column for everything.
columns must be > than previous entry.

const int stdpos[6] = {0, 26, 32, 49, 83, 180};         // standard posns

*/

int i, j, x;

for (i = 0; i < c->npars; i++)
{
//must be >= psuedocode column

 if ((unsigned) c->p[i] < stdpos[3]) return do_error(c,"\001Column < minimum");
 if (i && c->p[i] <= c->p[i-1]) return do_error(c,"\001 Column <= prevous Col");
}


x = 6 - c->npars;     // where to start

for (i = 0; i < c->npars; i++)
  {
    for (j = x; j < 6; j++) cposn[j] = c->p[i];
    x++;
  }
  return 0;
}





int chk_cmdaddrs(CPS *c, DIRS* d)
 {
   // check and fixup any address issues (bank etc)
   // can have a pair (start-end) and/or a single start (e.g. rbase has both)

   // check for valid bank and remember to add 1 
   // banks set up before here 

// can check numbanks < 0 here abaondon if no banks present.

  int start, end;

  if (d->sgpos < d->maxpars)
    {  // single address, may have a start/end pair following
     start = fix_input_addr(c->p[d->sgpos]);

     if (numbanks < 0) return do_error(c,"\017No valid banks found/defined");
     if (!val_input_bank(start)) return do_error(c,"\001Invalid Bank");
     c->p[d->sgpos] = start;
    }

 if (d->prpos < d->maxpars)
    {        // address pair
     start = fix_input_addr(c->p[d->prpos]);
     end   = fix_input_addr(c->p[d->prpos+1]);

     // if pair not first, optional.
        if (numbanks < 0) return do_error(c,"\017No valid banks found/defined");
     if (!val_input_bank(start)) return do_error(c,"\001Invalid Bank");

     if (!end) end = start;
     if (end <= 0xffff) end |= g_bank(start);      // add start bank
     if (!bankeq(start,end)) return do_error(c,"\001Banks don't match");
     if (end < start)  return do_error(c,"\001End < Start");

     c->p[d->prpos]   = start;
     c->p[d->prpos+1] = end;
    }


// do error returns 1;

 return 0;
 }

int check_opt(CPS *c, uint set)
  {
   char *t;
   const char *x;
   DIRS *d;
   int ans;

   ans = 0;
   d =  dirs + c->fcom;   
   t = flbuf+c->posn;       // where we are in cmd string

   // check for delimiters first, then options allowed  :|

   if (set == 1 && *t == ':')  ans = ':';
 //  if (*t == '|')  ans = '|';        //not yet, only some cmds ??

   if (*t == '[' || *t == ']') ans = *t;

   if (ans) return ans;

   x = 0;
   if (set == 1) x = d->opts;        // allowed letter main options
   if (set == 2) x = d->gopts;       // allowed letter global options

   while (x)
    {
      if (*t == *x) 
        {
         ans = *t;
         break;
        }
     x++;
   }
 return ans;
}

int do_adnl_letter (CPS* c)
{
   // switch by command letter - moved out of parse loop for clarity

   ADT *a;
   int ans, rlen, opt;
   float fx;
 //  char *t;

 // get next letter and check, main options
   opt = check_opt(c,1);

// BUT HERE, SWOP TO [] Mode if starts with [


   if (opt <= 0) return do_error(c,"\001\003 Option");      // Illegal option
   else c->posn +=1;

   if (!c->seq && opt != ':') return do_error(c,"\001Colon\007");

   if (c->seq > 0) a = c->adnl;                            //current adt block

   switch(toupper(opt))
     {
           // case 'A':                    // AD voltage ?? / RPM/ IOtime....
           //    cadd->volt = 1;
           //    break;

// B with T
           // case 'C' :

// to become cell <x>,<x>  start bit , size.    from zero bit (LS)

           //    break;

       case '|' :                    // split printout for large structs, not for argl

        if (!c->argl) a->newl = 1;                  // mark split printout here (end of block)

         // fall through

       case ':' :


// need max level check !!!! or change to get blocks from chain but would need to delete and rechain
// with new FK.....

// NB. pp-lev etc uses ->cnt so set cnt = 0, ssize = 1 for hidden entries



// add case '[' :         // treat as colon if not 'new' mode.
//                       open new 'level' based on fid of current adt - meaning adt cannot be in current form.... 
// add case ']' :         // if 'new mode' must check levels all closed.



         // Add a new ADT level
      //   t = flbuf+ c->posn;
      //   while (*t


         if (!readpunc(c)) break;   // not if a trailing colon (or new line marker)

// check for double colons too..........



  //       c->levels++;
    //     if (c->levels > 31) return do_error(c,"\001Too Many data items");
         if (c->seq > 32) return do_error(c,"\001Too Many data items");
         a = add_adt(&chadcm,0, 256);                   // add new adt
         c->adnl = a;                                        // and remember it
         c->seq = a->fkey & 0xff;                                     
         break;

       case '=' :       // an ANSWER definition. 

         ans = getpx(c,4);
         if (!ans) return do_error(c,"\001Address\004");

         a->ans = 1;
          //   check........ MUST BE VALID REGISTER !!
         if (!a->cnt)   a->cnt = 1;
         a->data  = c->p[4];    // item + fixed address offset (funcs) -  bank?
         break;

       case 'B' :                 // Bit
         // read one decimal val  b only allowed in sym and timer commands
         ans = getpd(c, 4);
         if (!ans) return do_error(c,"\001Bit Number\004");
         if (c->p[4] < 0 || c->p[4] > 31) return do_error(c,"\001Bit Number\003");
         a->fstart = c->p[4];
          // can't use a->fend , which is for SIZE (=W)
         if (ans > 1) a->data = c->p[5];         // TEMP !!
         else a->data = a->fstart;               // single bit
         a->enc = 7;           // temp marker for  bitno
         break;

       case 'D' :

         // 1 hex value (address offset with bank) WHAT ABOUT NEGATIVES !!
         ans = getpx(c,4);
         if (!ans) return do_error(c,"\001Address\004");
         a->foff = 1;

         //a->data  = fix_input_addr(c->p[4]);    // item + fixed address offset (funcs) -  bank?

         a->data  = c->p[4];  
         // if (a->data & 0x8000)
         //     {  a->data &= 0xffff;
         //        a->data =  -a->data;
         //     }




         if (!a->pfw) a->pfw = get_pfwdef(a);  //word sized for offsests
         break;

       case 'E' :

         // ENCODED - always size 2
         ans = getpx(c,4);
         if (ans != 2 )  return do_error(c,"\001 %d values\004",2);

         a->enc   = c->p[4];
         a->data  = c->p[5];
         a->fend = 15;            // encoded implies word
         //a->bsize = 2;            // word sized

         if (!a->pfw) a->pfw = get_pfwdef(a); //cnv[1].pfwdef;    setdefpfw(a);     //         if (!a->pfw) a->pfw = cnv[1].pfwdef;    //cafw[2];
         if (!a->cnt) a->cnt   = 1;
         break;

       case 'F' :

         // flags for symbol

        if (c->fcom == C_SYM)
         {             // flags word/byte for SYM.
           c->flags = 1;
           break;
         }
        break;

    

  // G H J

       case 'I' :               // immediate fixup for syms

        if (c->fcom == C_SYM)  c->imm = 1;
        break;

       case 'K' :

         // 1 decimal value (bank, 0,1,8,9)
         ans = getpd(c, 4);
         if (!ans) return do_error(c,"\001Bank number\004");

         if (!val_bank(c->p[4])) return do_error(c,"\001Bank Number\003");
         a->bank  = c->p[4]+1;
         if (!a->cnt) a->cnt = 1;     // but size stays at zero
         break;

       case 'L' :               // long int (= double)

         a->fend &= 32;        // keep sign
         a->fend |= 31;
       //  a->bsize = 4;
         if (!a->cnt) a->cnt = 1;
         if (!a->pfw) a->pfw = get_pfwdef(a);
         break;

 // case 'M':                // mask pair ?, use esize to decide word or byte
 //    cadd->mask = 1;
 //    break;

       case 'N' :

         a->fnam = 1;       // Name lookup
          if (c->fcom == C_SYM) c->names = 1;
         break;

       case 'O' :                // Cols or count - 1 decimal value

         ans = getpd(c, 4);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, "\002 Repeat value\007, 1 assumed");
           }  
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c,"\001 Repeat value\003 (1-32)");

         a->cnt = c->p[4];
         break;

       case 'P' :               // Print fieldwidth   1 decimal

         ans = getpd(c, 4);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, "\002 Field Width\007, 1 assumed");
           }  
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c,"\001 Field width\003 (1-32)");

         a->pfw = c->p[4];
         break;       
         
       case 'R' :               // indirect pointer to subroutine (= vect)

         a->fend = 15;         // safety, lock to WORD
       //  a->bsize = 2;      
         a->vaddr = 1;
     //    a->name  = 1;          // with name by default  NOT DEFAULT
         if (!a->cnt) a->cnt   = 1;

         if (!a->pfw) a->pfw = get_pfwdef(a);
         break;


       case 'S':              // Signed
          a->fend |= 32;
          break;

       case 'T':                 // biT ->Triple
         a->fend &= 32;           // keep sign
         a->fend |= 23;
         if (!a->cnt) a->cnt   = 1;
         if (!a->pfw) a->pfw = get_pfwdef(a);
         break;




       case 'U' :               //unsigned (default so optional)
         a->fend &= 31;         // clear sign
         break;

       case 'V':                  // diVisor

         // read one FLOAT val
         ans = sscanf(flbuf+c->posn, "%f%n ", &fx, &rlen);       // FLOAT
         if (ans <=0) return do_error(c,"\001Divsior value\004");
         c->posn+=rlen;
         a->flt = 1;                            //mark float variable
         a->fldat = fx;                              // Full FLOAT variable
         break;

       case 'W':                  // Word   - or write for syms

         a->fend &= 32;           // keep sign
         a->fend |= 15;

         if (!a->cnt) a->cnt   = 1;
         if (!a->pfw) a->pfw = get_pfwdef(a);
         break;

       case 'X':                  // Print radix  ( Hex default)
         ans = getpd(c, 4);
         if (!ans)
           {
             a->prdx = 0;
             do_error(c, "\002Radix \007, Hex assumed");
           }

// change to :m 10.z  for float, where z is num places.
// t = flbuf+c->posn;       // where we are in cmd string 
// if (t = '.')    go float


         else if (c->p[4] == 2 ) a->prdx = 3;
         else if (c->p[4] == 10) a->prdx = 2;
         else if (c->p[4] == 16) a->prdx = 1;
         else return do_error(c,"\001 Radix value\003 (2,10,16)");
         break;

       case 'Y':                  // bYte
         a->fend &= 32;           // keep sign, set size to 1
         a->fend |= 7;
         if (!a->cnt) a->cnt   = 1;
         if (!a->pfw) a->pfw = get_pfwdef(a);
         break;

         // Z

       default:
           return do_error(c,"\001\003Option");

    }          // end switch
 return 0;
    }


int do_global_opts(CPS *c)
{
   int inglo, ans, opt;
   char *t;

   if (!readpunc(c)) return 0;
     
   t = flbuf+c->posn;
   if (*t != '$') return 0;       // no '$', exit

   c->posn += 1;                            // skip the '$'

   inglo = 1;

   while (inglo)
    { 
     if (!readpunc(c)) break;
 
     // get next letter and check, main options
     opt = check_opt(c,2);

     if (opt <= 0) return do_error(c,"\003Option");      // illegal option char (4)
     else c->posn += 1;                                  // Only single char here
 
     switch(toupper(opt))
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
         c->cptl = 0;
         break;
         
       case  'C' :

         c->cptl = 1;        // set compact layout
         c->argl = 0;
         break;


       case  'F' :
        // f <str> <reg1> <reg2>    func add/ tab add, cols

         ans = getstr(c,spfstrs, NC(spfstrs));  // zero is a dummy
         if (ans <= 0)  return do_error(c,"\001Subr type reqd");
         c->spf = ans + 4;
         ans = getpx(c,5);

         // read colons in here and cheat ???
// register check ??

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

       case ':'  :
         inglo = 0;            // end of global options
         c->posn--;            // go back to colon
         break;
      
       default:
         do_error(c,"\001\003Option");
         break;
     }
    }  
   return 0;
}
    

int do_opt_str (CPS* c)
{
    int ans;
    char *t;

    c->p[0] = 0;    
    ans = 1;

   while (ans)
    { 
      if (!readpunc(c)) break;
      t = flbuf+c->posn;  
      if (*t == ':') c->posn++;
      
      ans = getstr(c,optstrs, NC(optstrs));            // zero is valid answer (default)            
      if (ans < 0) do_error(c,"\001\003 Option");       // not found
      c->p[0] |= mopts[ans];
    }     
return 0;
      
}    

int parse_cmd(CPS *c, char *flbuf)
{
  // parse (and check) cmnd return error(s) as bit flags
  // return 1 for next line (even if error) and 0 to stop (end of file)

  int rlen, ans;
  char *t, *e;                        // where we are up to
  DIRS* d;

  memset(c, 0, sizeof(CPS));          // clear entire struct
  clearchain(&chadcm);                // clear command adt chain
//  c->fid = (void *) 0x100;            // dummy fid for adt command chain

  if (!fgets (flbuf, 255, fldata.fl[3])) return 0; 

  flbuf[255] = '\0';
  c->maxlen = strlen(flbuf);          // safety

  e = flbuf + c->maxlen;              // end of input line

  t = strchr(flbuf, '\n');            // check for LF
  if (t && t < e) e = t;              // mark it

  t = strchr(flbuf, '\r');            // check for CR
  if (t && t < e) e = t;              // mark it

  //t = strchr(flbuf, '#');             // check for comment - after names
  //if (t && t < e) e = t;              // mark it

  c->maxlen = e - flbuf;              // shorten line as reqd

  if (c->maxlen == 0) return 1;        // this is a null line;

  c->posn = 0;

  if (!readpunc(c)) return 1;         // null line after punc removed

  t = flbuf+c->posn;
  if (*t == '#')  return 1;           // # comment at front 

  ans = getstr(c,cmds, NC(cmds));     // string match against cmds array - zero is valid
  if (ans < 0)  return do_error(c,"\001 \003 Command");             // cmnd not found

  d = dirs+ans;
  c->fcom = ans;

  if (PARGC)   c->cptl = 1;     // set compact argument layout default
  if (PSTCA)   c->argl = 1;     // set data struct argument layout default

  readpunc(c);

  if (d->maxpars)       // read up to 8 following addresses into p[0-n] (if any)
    {
     c->npars = getpx(c,0);

     if (c->npars < 1) return do_error(c,"\001\011\004");        // params reqd
     if (c->npars > d->maxpars) do_error(c,"\002Extra\011\010"); // extra pars ignored (continue)

     // verify addresses, single and/or start-end pair
     // but not whether valid for data or code (yet)
     ans = chk_cmdaddrs(c, d);

// ans is not used ???
// ans id currently 0 or 1
// (do_error sets c->error to 1 if error. c->error is full int

     if (c->error > 1) return 0;
     
     if (c->error) return 1;

//c->error is 0 for warning, 1 for error

// return 0 if serious bank problem
// "no banks defined, cannot continue" ..................


    }

  // read name here, if allowed and present - sort out spaces in names ??

  readpunc(c);

  if (d->namex && c->posn < c->maxlen &&  flbuf[c->posn] == '\"')
   {
    t = flbuf+c->posn;
    while (*t == '\"')
      {
       c->posn++;   // skip quote(s)
       t = flbuf+c->posn;
      }
    rlen = 0;
// readpunc first, and allow multiple quotes ??
   readpunc(c);                    // safety check

   while (c->posn < c->maxlen)
     {
      t = flbuf+c->posn;
      if (rlen < SYMSZE) c->symname[rlen] = *t;
      if (*t == '\"') break;            // end of name
      if (!isspace(*t)) c->posn++;      // use system isspace not mypunct 
      else break;
      rlen++;
     } 

    while (*t == '\"')
      {
       c->posn++;   // skip quote(s)
       t = flbuf+c->posn;
      }

 //  if (*t == '\"')
 //     {         // end of name
 //      c->posn++;
 //     } 
   // else do_error(c,"\002\007 close quote (assumed at first space)");  // missing quote

   if (rlen > SYMSZE-1)
      {       // truncate with warning, but keep c->posn to what was actually read
          do_error(c,"\002 Name truncated at %d chars",SYMSZE);
          rlen = SYMSZE-1;
      }
   c->symname[rlen] = '\0';   
   }

   // NOW check for comment, AFTER name
   t = strchr(flbuf+c->posn, '#');
   if (t) c->maxlen = t - flbuf;              // shorten line as reqd


   if (d->gopts) do_global_opts(c);

  // now read addnl levels

  if (d->opts || d->stropt)
   {
     // read option chars/strings/markers whilst data left
      while (c->posn < c->maxlen)
       {
        if (!readpunc(c)) break;                    // safety check

        if (d->stropt) ans = do_opt_str(c);        // option strings
        if (d->opts)   ans = do_adnl_letter (c);   // adnl letters style
        if (c->error) return 1;
       }      //  end get more options (= colon levels)

   }  // end if options allowed

 // any further error reporting is responsibility of each handler command

 c->tsize = totsize(&chadcm, 0);          //safety.

  d->setcmd (c);                  // do setup procedure

 if (d->namex && c->fcom != C_SYM && *c->symname)
 {  // default add sym name for cmds other than SYM
   SYM *s;
 //  uint add;
 //  add = nobank(c->p[0]);
 //  add = fix_sbk_addr(add);      // force bank 9 if single bank
   s = add_sym(c->symname,C_CMD,c->p[0],32,32, 0,0xfffff);

// may need error for bank 

   if (chsym.lasterr == E_DUPL)
       {
   //      do_error(c,etxts[chsym.lasterr]);     //TEST !!
         strncpy(nm,s->name, SYMSZE);    // current sym
         new_symname (s, c->symname);    // change sym
         chsym.lasterr = 0;
         if (!s->sys) do_error(c,etxts[E_SYMNAM], nm);
         s->sys = 0;
         s->cmd = 1;        //clear system, set user cmd
         s->xnum = 0;      // and clear any autonum
         if (chsym.lasterr)   return do_error(c,etxts[chsym.lasterr]);
       }
 }



 if (c->firsterr) wnprt(1,0);    // extra LF after any errors
 show_prog(0);
 return 1;                   // next line
}


void do_preset_syms(void)
 {
   uint i;
   DFSYM *z;

   for (i = 0; i < NC(defsyms); i++)
     {
      z = defsyms+i;
      if (((P8065) && z->p85) || (!(P8065) && z->p81))
         {
          add_sym(z->symname,z->wrt|C_SYS,z->addr,z->fstart,z->fend,0,0xfffff);
         }
      }
  }



/***************************************************************
 *  read directives file
 ***************************************************************/
int getudir (int ans)
{
  int addr,addr2, i, j, bank;
  BANK *b;

  // cmdopts will be zero or P8065 here (before dir read)
  // and banks should be sorted (auto detect anyway)

  cmdopts |= PDEFLT;            // add default

  if (fldata.fl[3] == NULL)
    {
     wnprt(2,"# ----- No directive file. Use default options");
    }
  else
    {
      wnprt(2,"# Read commands from directive file at '%s'", fldata.fn[3]);
      while (parse_cmd(&cmnd, flbuf));
    }




  if (PRSYM) do_preset_syms();

  if (PMANL) return 0;         // entirely manual

  // now setup each bank's first scans and interrupt subr cmds
// ERROR IF NO BANKS !!


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

     add_scan (addr,J_INIT,0);                  // inital scan at PCORG (cmd)

     j = (P8065) ? 0x205f : 0x201f;
     j |= bank;

     add_cmd (0x200a|bank, 0x200f|bank, C_WORD|C_SYS,0);     // from Ford handbook
     add_cmd (0x2010|bank, j, C_VECT|C_SYS,0);               // interrupt vects with cmd

     for (j -= 1; j >= (0x200f|bank); j-=2)      // counting DOWN to 2010
        {
         addr2 = g_word (j);
         addr2 |= bank;
         if (PINTF) add_iname(j,addr2);
         add_scan (addr2, J_SUB,0);      // interrupt handling subr

        }
     }

   #ifdef XDBGX
      DBGPRT(1," END auto setup");
     #endif

return 0;
 }



void prt_subs (uint (*prt) (uint,const char *, ...))
{
  SUB *s;
  SYM *x;
//  SPF *p;
 // ADT *a;  
  uint ix, go;

  prt(1,"# ------------ Subroutine list----------");

  for (ix = 0; ix < chsubr.num; ix++)
      {
        s = (SUB*) chsubr.ptrs[ix];

        // decide on print
        if (prt == wnprt)
           {
            go = 0;
            if (s->cmd)  go = 1;
            if (!go && get_spf(s->start)) go = 1;
            if (!go && get_adnl(&chadnl,s->start,1)) go = 1;
            if (!go && get_adnl(&chans,s->start,1))  go = 1;
           }
        else go = 1;

        if (go)
          { 
           if (prt == wnprt) {prt(0,"sub  ");  paddr(s->start,2,prt);}
           else prt(0,"sub  %x %x  ", s->start, s->end);

           x = get_sym(0, s->start,32,0);
           if (x)
             {
              prt(0,"%c%s%c  ", '\"', x->name, '\"');
              if (prt == wnprt) x->noprt = 1;        // printed sym, don't repeat it
             }

           prt_glo(s, prt);
           prt_adt(&chadnl,s->start,0,prt);
           prt_adt(&chans ,s->start,0,prt);             // answer 
           if (s->cmd)  prt(0," #cmd");
           prt(1,0);
          }
      }

   prt(2,0);
  }



void prt_syms (uint (*prt) (uint,const char *, ...))       //void)
{
 SYM *t;
 uint ix, b;

 prt(1,"# ------------ Symbol list ----");

 for (ix = 0; ix < chsym.num; ix++)
   {
   t = (SYM*) chsym.ptrs[ix];

   // decide on print
   if (prt != wnprt || !t->noprt)
     {

      if (prt == wnprt)
         {
          prt(0,"sym ");
          paddr(t->addb >> 5,1,prt);
          if (t->rstart)
           {
            paddr(t->rstart,1,prt);
            paddr(t->rend,0,prt);
           }
         }
      else
        {         // debug
         prt(0,"[%x] sym %x ", t->addb, t->addb >> 5);
         prt (0," %5x %5x ", t->rstart, t->rend);
        }

     prt(0," \"%s\"",t->name);

     b = t->addb & 0x8;
     if (!b || t->flags || !(t->addb & 0x10) || t->immok) prt(0," :");

     if (!(t->addb & 0x8))
       {
        prt(0,"B %d" , t->addb & 7);
        if (t->fend != (t->addb & 7)) prt(0, " +%d", t->fend);
       }

     if (!(t->addb & 0x10))  prt(0," W");
     if (t->flags) prt(0," F");
     if (t->names) prt(0," N");
     if (t->immok) prt(0," I");
     if (t->sys) prt(0,"                  # auto");
     
     prt(1,0);
     }
   }
  prt(1,0);
 }


void prt_dirs()
{                 // print all directives for possible re-use to msg file
  prt_opts ();
  prt_layout();
  prt_banks(wnprt);
  prt_rbt  (wnprt);
  prt_psw  ();
  prt_scans();
  prt_cmds (wnprt);
//  prt_cmds (&chaux, wnprt);
  prt_subs (wnprt);
  prt_syms (wnprt);


}

short init_structs(void)
 {

  // 4 banks of 16k as bit flags
  opcbit = (uint *) mem(0,0, 0x8000);       // opcode start bit flags (bytes)
 // opdbit = (uint *) mem(0,0, 0x8000);       // 8*0x8000 = 0x40000

  memset(bkmap,0,sizeof(bkmap));
  bkmap[3].opcbt = opcbit;                   // bit flag array start (uints)
  bkmap[4].opcbt = opcbit + 0x800;           // = 800*20 = 10000 (32 bit uints)
  bkmap[5].opcbt = opcbit + 0x1000;
  bkmap[6].opcbt = opcbit + 0x1800;

//  bkmap[3].opdbt = opdbit;                   // bit flag array start
//  bkmap[4].opdbt = opdbit + 0x800;           // = 800*20 = 10000 (32 bit uints)
 // bkmap[5].opdbt = opdbit + 0x1000;
//  bkmap[6].opdbt = opdbit + 0x1800;

  memset(opcbit, 0, 0x8000);                 // clear all opcode flags
//  memset(opdbit, 0, 0x8000);                 // clear all opdata flags

  anlpass  = 0;                              // current pass number
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
  basepars.rambnk    = 0;

  return 0;
}


int openfiles(void)
{
  int i;

  fldata.error = -1;

  for (i = 0; i < numfiles; i++)
   {
    fldata.fl[i] = fopen(fldata.fn[i], fd[i].fpars);           // open precalc'ed names
    if(i < 3 && fldata.fl[i] == NULL)
      {
       fldata.error = i;
       return i;   // failed to open a file
      }
   }

return -1;
}








/* old
void check_ihandler(SIG *s, uint taddr, uint ix)
 {
   // check pointer values (int handler) for valid jump, return etc
   // ix = 7 for 8065, 5 for 8061
  int val;
  uint jp,cx;

  val = g_byte(taddr);

  //allow up to 7 nops
  jp = taddr+7;
  while (taddr < jp)
    {
     if (val == 0xff) val = g_byte(++taddr); else break;
    } 

  do_one_opcode(taddr);      // do THIS way !! handles 0x10 cmdopts set

// sinst, banks will be +1 from do_code

  if (ix == 7 && val == 0x10)            // sinst.sigix = 16   (jump)
    {    // bank swop 8065 only, verify bank is valid,
         // and check for a valid jump after it, return
      jp = g_byte(++taddr);

      if (!(jp & 0xf6)) 
        {     // bank OK  (0,1,8,9)
          jp = g_byte(++taddr);                           
          jp = do_jumpsig (opcind[jp], taddr, 0x100000);
          if (jp) { s->v[ix]++; return;} 
        }        
    }


  // check if valid jump, reload val if so
  jp = do_jumpsig (opcind[val], taddr, 0x100000);
  if (nobank(jp) >= PCORG) val = g_byte(jp);

// could also be a further jump here (xdt2)

  if (val >= 0xf0 && val <= 0xf2) s->v[ix]++;      // ret, retei or pushp
   else
  if (ix == 7 && val == 0x10) s->v[ix]++;         // jump to a bank swop (8065)
   else
    {
     cx = opcind[val];                                   //get index
     if ((cx > 47 && cx < 50) || (cx > 39 && cx < 41))    // ldx, stx 
       {
        val = g_byte(taddr + 3);
        if (val >= 0xf0 && val <= 0xf2) s->v[ix]++;     // ret, retei or pushp
       }
    }

} */

void check_ihandler(SIG *s, uint taddr, uint ix)
 {
   // check pointer values (int handler) for valid jump, return etc
   // ix = 7 for 8065, 5 for 8061
  int val;
  uint jp, cx;


// allow taddr == d004 as a special ???

  val = g_byte(taddr);

  // allow up to 7 nops
  jp = taddr+7;
  while (taddr < jp)
    {
     if (val == 0xff) val = g_byte(++taddr); else break;
    } 

  if (val >= 0xf0 && val <= 0xf2)
    {  // ret, retei or pushp
     s->v[ix]++;
     return;
    } 

  do_one_opcode(taddr);

  if (sinst.sigix == 16 || sinst.sigix == 23)
    {    // jump, jnb
     jp = sinst.opr[1].addr;
     cx = g_bank(jp);
     if (cx != g_bank(taddr))
        {    // bank swop
          cx-= 0x10000;
          if (!(cx & 0xf6))
           {
            s->v[ix]++;   // bank OK  (0,1,8,9)
            return;       // can't go any further
           }
        }

     taddr = jp;
     if (val_rom_addr(taddr))
       {
        val = g_byte(taddr);  //not bank swop, get value
        if (val >= 0xf0 && val <= 0xf2) {s->v[ix]++; return;}    // ret, retei or pushp
       }

    }

  taddr = tmpscan.nextaddr;        //try after one unmatchd opcode
  val = g_byte(taddr);             // get value
  if (val >= 0xf0 && val <= 0xf2) {s->v[ix]++; return;}    // ret, retei or pushp

 } 



void check_bank_intv (SIG *s, uint *mx)
 {
    uint bank, taddr, x;
    if (!s)  return;                     // safety, no jump
    taddr = 0;
    bank = g_bank(s->start);               // FAKE bank from sig

    // sig below int vects and a valid jump
    if (nobank(s->end) >= 0x2010 || s->v[1] < (0x2000|bank))  return;

    // check interrupt vects 8065
    // counts valid adrress into [6]
    // and valid contents into [7]

    cmdopts |= 8;            //set 8065 for bank swops

    for (x = (bank|0x2010);  x < (bank|0x205f);  x+=2)
      {
       taddr = g_word(x);             //pointer
       //if (taddr <= 0xffff  && taddr >= 0x2060)  s->v[6]++;         // 8065 count (valid address)
       if (taddr > 0x205f)  s->v[6]++;         // 8065 count (valid address)
       else break;
       check_ihandler(s, taddr|bank, 7);
      }

  if (s->v[6] > *mx) *mx = s->v[6];                    //  keep highest match in outer loop

  cmdopts = 0;                      //clear 8065

  if (*mx <= 11)
   {
    // check interrupt vects 8061 if max 8065 count below 10
    // counts valid adrress into [4]
    // and  valid contents into [5]

    for (x = (bank|0x2010);  x < (bank|0x201f);  x+=2)
      {
       taddr = g_word(x);             //pointer
     //  if (taddr <= 0xffff && taddr >= 0x2020) s->v[4]++;         // valid adress
       if (taddr > 0x201f) s->v[4]++;         // valid adress
       else break;
       check_ihandler(s, taddr|bank, 5);

      }
   }

  taddr = chsig.num * 0x100;
  s->start += taddr;                   // fiddle to stop dupls & overlaps
  s->end += taddr;

  // check for code bank, bank 8
  // sig can't overlap interrupt vects (2010) and must have a jump

  if (s->v[1] <= s->start+s->skp)  s->v[3] = 2;                      // loopstop = Data bank

  if (s->v[6] > 39 && nobank(s->v[1]) > 0x205f)   s->v[3] = 1;       // jump over intr regs - 8065.
  else
  if (s->v[4] > 7  && nobank(s->v[1]) > 0x201f)   s->v[3] = 1;       // jump over intr regs - 8061.

 if (s->v[3])
   {   //sig must have a matched jump
    if (s->v[4] > 7 || s->v[6] > 35)  inssig(s);                     // matched enough, so keep it for now
   }
 if (s->v[6] > *mx) *mx = s->v[6];             // keep highest score
}




void find_banks (uchar *fb)
{
    // file is read in to ibuf [0x2000] onwards - look for start jump and interrupt vectors
    // assemble temp sigs into chain and then check interupt vectors.  scan around each address
    // for missing or extra bytes

    BANK *b, *l;
    SIG *s, *t, *x;
    uint addr, bank, tix, mx;
    int ofs;

    // temp bank setup so accesses work

    bank = 0x30000;      //bank 3, temp
    b = bkmap+3;
    b->maxbk = (bank | 0xffff);
    b->maxpc = b->maxbk;
    b->minpc = (bank | PCORG);
    b->bok = 1;                               // temp mark as OK
    b->cmd = 0;
    b->cbnk = 0;

    #ifdef XDBGX
       DBGPRT(1,"scan for all banks");
    #endif

    // check full file in loop for ALL banks 

    mx = 0;                //move this inside loop ???
    numbanks = -1;

    for (addr = 0; addr < fldata.fillen;  addr += 0x1000)
      {
       // keep all matches and then review best candidates
       // bank->fbuf MUST point to 0x2000 whether real or virtual
       // for missing bytes - mapaddr fixed, PC changes, 0x2002, 0x2001
       // for extra bytes   - mapaddr changes +0 to +3, PC set at 0x2000
 
       b->filstrt = addr;                       // move file offset
       b->filend  = addr + 0xe000-1;            // max possible end point 

       if (b->filend >= fldata.fillen)
         {        // make range safe for file end, adjust downwards
          b->filend = fldata.fillen-1;                         // end of file (no bank)
          b->maxbk = b->filend - b->filstrt + (PCORG | bank);
          b->maxpc = b->maxbk;
         }

       b->fbuf = fb + addr - 0x2002;     // start addr maps to 0x1ffe, then 1fff

       for (ofs = 0; ofs < 2; ofs++)
         {
          s = match_sig(&hdr,((0x2002-ofs)|bank),0);
          if (s) 
            {
             s->v[15] = addr;                 // file start addr 
             s->v[10] = 0x2002-ofs;           // code start addr 
             check_bank_intv (s,&mx);
            }
          b->fbuf++;
         }

       b->fbuf = fb + addr - 0x2000;     // start addr fixed 0x2000, file start moves

       for (ofs = 0; ofs < 3; ofs++)
        {
         s = match_sig(&hdr,(0x2000|bank),0);
         if (s)
          {
             s->v[15] = addr+ofs;        // file start addr
             s->v[10] = 0x2000;          // code start addr
             check_bank_intv (s, &mx);
          }
         b->fbuf++;
        }
    }

  // look for highest valid intr count. [4] & 5 for 8061, 6 & 7 8065
  // addresses (in s->v[15]) will be grouped. v[2] is bank (and highest)

  t = (SIG*) chsig.ptrs[0];             // to be 8061 highest match
  x = t;                                // to be 8065 highest match
  bank = 3;                             // (fake) start bank
  if (t->v[5] > 5)  t->v[2] = bank;     // start as match if qualifies
  if (x->v[7] > 30) x->v[2] = bank;

  for (tix = 1; tix < chsig.num; tix++)
    {
      s = (SIG*) chsig.ptrs[tix];

      // interrupt vect cnt MUST be all OK otherwise not correct binary
      // check address ranges in s[15] so no interbank overlaps

      if ((s->v[15] - x->v[15]) < 0x800)
       {
         if (s->v[6] == 40) // 8065
           {
             if (s->v[7] > x->v[7])
               {
                if (s->v[7] > 30) s->v[2] = bank;           // new match
                x->v[2] = 0;                                // kill old match
                x = s;
               }
           }

         if (s->v[4] == 8 && s->v[6] < 10)     // 8061
           {
              if (s->v[5] > t->v[5])
                {
                 if (s->v[5] > 6) s->v[2] = bank;           // new match
                 t->v[2] = 0;                               // kill old match
                 t = s;
                }
           } 
       }
      else
       {
         if (x->v[2] || t->v[2]) bank++;
         t = (SIG*) chsig.ptrs[tix];            // to be 8061 highest match
         x = t;
         if (t->v[5] > 6)  t->v[2] = bank;      // add bank if qualifies
         if (x->v[7] > 30) x->v[2] = bank;  
       }
     }

// done, now sort out temp banks.
// clear all bank OK markers

 for (tix = 3; tix < 7;  tix++)
    {
     b = bkmap+tix;
     b->bok = 0;
    }

  for (tix = 0; tix < chsig.num; tix++)
    {
     s = (SIG*) chsig.ptrs[tix];
     if (s->v[2]) 
       {        //bank set (temp)
        bank = s->v[2];               //reset bank.............
        b = bkmap+bank;

        if (s->v[6] == 40) b->P65 = 1; else  b->P65 = 0;   // mark bank as 8065
        b->bok = 1;                                  // bank is valid start
        numbanks++;
        if (s->v[3] < 2) b->cbnk = 1;                // code bank (8)
        b->filstrt = s->v[15];                       // actual bank start as buffer offset for 0x2000
        b->filend = b->filstrt + 0xe000-1;           // default (max) end

        // beyond end of file ???
        if (b->filend >= fldata.fillen) b->filend = fldata.fillen-1;

       // short bank check .............
       l = bkmap + (bank-1);                      // last bank 
       if (bank > 3 && b->filstrt < l->filend)
         {   // shorten previous bank if it overlaps start.

          l->filend = b->filstrt-1;
          l->maxbk = l->filend - l->filstrt + l->minpc;
          l->maxbk |= g_bank(l->minpc);
          l->maxpc = l->maxbk; 
           #ifdef XDBGX
           DBGPRT(1,"** Short bank %d, %x-%x", bank-1, l->filstrt, l->filend);
           #endif  
         }

        b->fbuf = fb + b->filstrt-0x2000;            // set correct fbuf
        b->minpc = s->v[10] | (bank << 16);                  // 2000, 2001 or 2002
        b->maxbk = b->filend - b->filstrt + b->minpc; 

        #ifdef XDBGX
        DBGPRT(1,"bank found at %5x, PC %x", b->filstrt, s->v[10]);
        if (b->filstrt & 3)           DBGPRT(1," WARNING - file/bank appears to have %s byte(s) at front", "extra");
        if (nobank(b->minpc) > PCORG) DBGPRT(1," WARNING - file/bank appears to have %s byte(s) at front", "missing");
        #endif

        if (b->filstrt & 3)           wnprt(1," WARNING - file/bank appears to have %s byte(s) at front", "extra");
        if (nobank(b->minpc) > PCORG) wnprt(1," WARNING - file/bank appears to have %s byte(s) at front", "missing");

      }
    }

   #ifdef XDBGX
   DBG_sigs(&chsig);
   #endif
   clearchain(&chsig);                    // clear out all sigs

}




void clrbkbit(int ix, int bk)
{
  BANK *b;

  b = bkmap+ix;

  switch (bk)
  {
    case 0:

      #ifdef XDBGX
      if (b->bkmask & 1)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
      #endif
      b->bkmask &= 0xe;      //clear bit 0
      break;
      
    case 1:

      #ifdef XDBGX
       if (b->bkmask & 2)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
       #endif
      b->bkmask &= 0xd;      //clear bit 1
      break;

   case 8:

      #ifdef XDBGX
       if (b->bkmask & 4)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
       #endif
      b->bkmask &= 0xb;      //clear bit 2
      break;

    case 9:

      #ifdef XDBGX
       if (b->bkmask & 8)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
       #endif
      b->bkmask &= 7;        //clear bit 3
      break;
     
    default:
      #ifdef XDBGX
      DBGPRT(0,"unknown bank %d", bk);
      #endif
      break;
  } 
}

void copy_banks(uchar * fbuf)
 {
  int i, bk, size;
  BANK *to, *from;

// copy to bank+1  for bankfix with sig names 

  for (i = 3; i < 7; i++)
   {
     from = bkmap + i;            // temp bank
     if (from->bok)
      {        // valid to transfer 
       from->dbank++;
       to   = bkmap + from->dbank;        // new bank
       bk =  from->dbank << 16;

       *to = *from;             // lazy, may change this
       size = to->filend-to->filstrt+1;

       to->fbuf = (uchar*) mem(0,0, 0x10000);   //malloc a full bank.

       memcpy(to->fbuf+nobank(from->minpc), from->fbuf+0x2000, size);
       to->minpc &= 0xffff;
       to->minpc |= bk;    // substitute bank no

       to->maxbk &= 0xffff;
       to->maxbk |= bk;
       to->bok = 1;
       to->bprt = 1;         // print his bank
       to->maxpc = 0;       // do the fill check
       find_fill_txt(to);
       from->bok = 0;        // clear old bank
       from->bprt = 0;       // and print
      }

   }


  basepars.codbnk = 0x90000;                         // code start always bank 8
  basepars.rambnk = 0;                               // start at zero for RAM locations
  if (numbanks) basepars.datbnk = 0x20000;           // data bank default to 1 for multibanks
  else   basepars.datbnk = 0x90000;                  // databank 8 for single

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
          // assume this bank CANNOT be rbank.............
          clrbkbit(bk,rbank);

          taddr++;
          val = g_byte(taddr);             // get opcode after bank prefix

          if (val == 0xe7 || ( val >=0x20 && val <= 0x27))
           {
            // a jump - calc destination
            taddr = do_jumpsig (opcind[val], taddr,0x100000);
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

  
  for (i = 3; i < 7; i++)
    {
     b = bkmap + i;
     if (i != cbk) drop_banks_via_ints(i);
     else
      {    
       b->dbank = 8;       // set up bank 8 directly
       b->bkmask = 4;
     #ifdef XDBGX
       DBGPRT(1, "bank %d is 8", i);
       #endif
      } 
    }       
      
   // now see if we can definitely find more banks
   // if any bank has single bit, can take that as definite ID
   // and drop that bit from all other banks

   for (i = 3; i < 7; i++)
    {  
      b = bkmap + i; 
      
      if (i != cbk)
        {  // not for bank 8
         if (b->bkmask == 1 || b->bkmask == 2 || b->bkmask == 8)
         {  // found a single bit, drop from other banks
           for (x = 3; x < 7; x++)
             { 
               if (x != i) 
                  {
                    #ifdef XDBGX
                   if (bkmap[x].bkmask & b->bkmask)
                     {
                      DBGPRT(1, "bank %d cannot be %d", x, b->bkmask < 8 ? b->bkmask-1 : 9);
                     }
                   #endif
                   bkmap[x].bkmask &= (~b->bkmask); 
                  }  
             }
         }
        }
    }   
        
  // now, is there one bit per bank ?? if so have got the
 // bank order.  if not, then we'll have to guess.      
 
   for (i = 3; i < 7; i++)
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



int do_banks(uchar *fbuf)
 {
  // set up banks after find_banks, or user command.

  // NO set up RAM bank [16] first 0x2000 of ibuf for ram and regs
  // NOTE - not technically safe if file has missing front bytes, but probably OK

// COPY  file into correct banks layout to c

  int ltj,bank,i, ans;
  BANK *b;

  //  do the checks and bank ordering.....

  bank = -1;                           // bank with the start jump (= code)
  ltj  = 0;                            // temp number of loopback jumps
  ans = 0;

  for (i = 0; i <= numbanks; i++)
      {
       b = bkmap+i+3;                // 3 to 6
       // b->fbuf points to bank start..................

       // any bank with P65 means all banks are 8065
       if (b->P65) cmdopts |= 0x8;

       if (b->cbnk)
          {
           if (bank >= 0)
             {
               wnprt(1," Warning - ");
               wnprt(1,"Found multiple Start Code banks [bank 8]");
               wnprt(1,"Choosing first bank found, others ignored - use bank command to override");
               #ifdef XDBGX
                 DBGPRT(1," ** Found multiple code banks");
               #endif
               b->bprt = 1;    //keep the print
               b->bok = 0;
               b->dbank = 9;
               numbanks--;
            }
           else bank = i;

          }
       else ltj++;       // no of other banks
      }


  if (bank < 0)
     {
      wnprt(1,"Cannot find a Start/Code bank [bank 8]");
      wnprt(1,"Is this file an 8061 or 8065 Binary ??");
      ans = -4;       //return -4;
     }


   #ifdef XDBGX
   DBGPRT(1,"%d Banks, start in %d", numbanks+1, bank+3);
   #endif

  switch (numbanks)
    {

     default:
       wnprt(1,"0 or > 4 banks found. File corrupt?");
       ans = -2;          //return -2;
       break;

     case 0:                      // copybanks autostarts at [3] for source list
       mkbanks(1,8);
       break;

     case 1:                      // 2 bank, only 2 options
       if (bank == 1) mkbanks(2,1,8);
       else mkbanks(2,8,1);
       break;

     // no 3 bank bins (case=2) so far .....

     // MUST handle 3 banks !!!



     case 3:
       // do best guess bank order and then check it
       // can shuffle numbers before the cpy_banks which moves them.
       if (!chk_bank_order(bank+3)) // attempt to sort banks by intrpt handlers

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

// what about multiple 8061 bank 8 ??
   // final copy and fill detection. this subr recalled if user command
 // DBG_banks();

  copy_banks(fbuf);

  #ifdef XDBGX
  prt_banks(DBGPRT);
  DBGPRT(1,0);
  #endif

  return ans;
}


int readbin(void)
{
  /* Read binary into main block 0x2000 onwards (ibuf[0x2000])
  *  ibuf  [0]-[2000]     RAM  (faked and used in this disassembly)
  *  ibuf  [2000]-[42000] up to 4 banks ROM, at 0x10000 each
  */

  int x;

  uchar *fbuf;            //file goes in here first.

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

  fbuf = (uchar*) mem(0,0, fldata.fillen);          // big enough for file
  fread (fbuf, 1, fldata.fillen, fldata.fl[0]);

  #ifdef XDBGX
   DBGPRT(1,"Read input file (0x%x bytes)", fldata.fillen);
  #endif

  // file in temp buffer, now read and layout as per banks.

// check for file error ?
//  end = fldata.fillen;

  find_banks(fbuf);
  x = do_banks(fbuf);               // auto, may be modded later by command
  mfree(fbuf, fldata.fillen);
  return x;
}


void rbasp(SIG *sx, SBK *b)
{

  //add rbase entries from signature match

  uint add, add2, reg, i, cnt;   //,xcnt,rcnt;
//  RBT *r;
  uint  pc;

  if (b) return;                  // no processing if subr set

 // rcnt = chbase.num;              // may have rbases already. start at 'NEW' point
  reg = (sx->v[1] & max_reg());
  add = sx->v[3];
  cnt = g_byte(add);              // no of entries in list

  #ifdef XDBGX
  DBGPRT(1,"In rbasp - begin reg %x at %x, cnt %d", reg, add, cnt);
  #endif

  add_cmd(add, add+1, C_BYTE|C_SYS,0);
  add_cmd(add+2, add+(cnt*2)+1, C_WORD|C_SYS, 0);

  if (chcmd.lasterr)
   {
     #ifdef XDBGX
      DBGPRT(1,"Rbase Already Processed");
     #endif
    return ;
   }

  pc= add+2;

 //  check bank is correct ?
    add = g_word(pc);                       // register pointer
    add |= basepars.datbnk;                 // + default data bank
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
    add |= basepars.datbnk;               // add + default data bank

   if (nobank(add) < 0x2020) break; 
   add_cmd(add, add+1, C_WORD|C_SYS,0);
   add2 = g_word(add);                     // check pointer is consistent with next entry
    // also valid for last pointer for xcode
     
//   r = 
add_rbase(reg,add,0,0xfffff);                     // dbg message in here....
 //  if (r) r->cmd = 1;
   upd_ram(reg,add,15);
   add_aux_cmd(add,add2-1, C_XCODE|C_SYS,0);          // and add an XCODE as this is data

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

// void (*set_str) (SFIND);
          //int anix;            // spf number of func moved to SFIND




void set_struct(SFIND *f)
{
 // check for bank first, and then if truly valid
 if (!val_rom_addr(f->ans[0]))  f->ans[0] = databank(f->ans[0],0);

 if (!val_rom_addr(f->ans[0])) 
     {
   //    #ifdef XDBGX
     //    DBGPRT(1,"Invalid Address %x", f->ans[0]);
  //     #endif
       return ;
      }





   // function  
   if (f->spf > 4 && f->spf < 13) set_xfunc(f);
   // table

    if (f->spf > 12 && f->spf < 15) set_xtab(f);
}

void do_list_struct(SFIND *f)
{
 uint m, i, addr;       // temp !!
 uint start, gap;      // for pointer list
 LBK *k; 
 ADT *a;


// this sort of works, problem is it proceeds down list and can get size and sign worng.......

// and overlaps don't work right either............





#ifdef XDBGX
 DBGPRT(1," LIST START %x for rbase %x", f->lst[0], f->lst[1]);
#endif

  start = f->lst[0];
// add_data_cmd (m,  m+1, C_WORD, 0);       // add data entry - TEST !!

  m = start;

  if (!f->rg[1])
    {  // func, check first byte is valid.

     while (1)
      {
       addr = g_word(m);
       addr = addr + f->lst[1];
#ifdef XDBGX
         DBGPRT(0," %x ", addr);
#endif

       if (!val_rom_addr(addr)) break;
       i = g_byte(addr);
       if ( i != 0x7f && i != 0xff) break;
       f->ans[0] = addr;        //set func address
       set_struct(f);
       m += 2;
       if (get_cmd(m,0)) break;        // is this any good ? (next ptr not done !)
      }
    } 
 else
    {  // Table - check size first
     addr = g_word(m);      //start
     i = g_word(m+2);       // next pointer 
     gap = i - addr;        // table size
     addr = addr + f->lst[1];
     i = addr;
     f->ans[0] = addr; 
     m +=2;
     set_struct(f);

     while(1)
      { 
       addr = g_word(m);
       addr = addr + f->lst[1];
#ifdef XDBGX
       DBGPRT(0,"  %x", addr);
#endif
       if ((addr - i) != gap)  break;      //assume all tabs are same size  
       i = addr;    
       f->ans[0] = addr; 
       set_struct(f);
       m +=2;
      if (get_cmd(m,0)) break;       
       }
    }

   if (m >= start+2) {  k = add_cmd (start,  m-1, C_WORD, 0); 

   if (k)
    {
      a = add_adt(&chadnl,k->start,0);    // only one
   if (a)
     {
      a->fend = 15;         //word
  //    a->bsize = 1;
      a->foff = 1;
      a->data = f->lst[1];
      a->fnam = 1;
      a->cnt = 1;
     }   
}}
}


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
          set_struct(&f);        // add table with one row if run out of opcodes
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

          if (f.lst[0]) do_list_struct(&f);
          else          set_struct(&f);   // add func or tab (add override)
          f.ans[0] = 0;
          f.ans[1] = 0;  // are these necessary ??
          f.lst[0] = 0;
          break;
         }
       #ifdef XDBGX
         else DBGPRT(0," %x[%d]", xofst, xcnt);
       #endif

       // see if any jumps in to xofst. if so, launch new branches.

       ix = get_tjmp_ix(xofst);     // start point

       while(ix < chjpt.num)
        {
          j = (JMP*) chjpt.ptrs[ix];
          if (j->toaddr != xofst) break;
          #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(1,"JMP %x<-%x %d", j->toaddr, j->fromaddr, cnt);
          #endif

         //j->from-j->cmpcnt to skip the cmp
// copy f for each new branch ??
         do_branch(j->fromaddr,xcnt,f);
         ix++;
        }  // end jump loop
      
     }        // end find opcode loop

}



// ** abandon this for funcs and do a liear search and verify ???
// MUCH easier than tables are.....................
// but STIL need a decent table recognizer................



void do_calls(SUB * sub, SFIND f)
{
  // do the 'base' (i.e. the subr calls outwards) separately to reset params

 JMP *j, *k;
 LBK *g;
 ADT *a;
 uint jx, ix, ofst;
      #ifdef XDBGX
        DBGPRT(1,0);
        DBGPRT(0,"Base calls for sub %x", sub->start);
      #endif
      
 ix = get_tjmp_ix(sub->start);     // start point

 while(ix < chjpt.num)
   {
    j = (JMP*) chjpt.ptrs[ix];

    if (j->toaddr != sub->start) break;

    if (j->jtype == J_SUB)              // sub only for from FIRST call
        {
     //     if (f.rg[1] == 1) f.ans[1] = j->fromaddr + j->size;     //func only, for override
     #ifdef XDBGX
  DBGPRT(1,0);
          DBGPRT(0,"To %x - [%x %x] (%x %x) ", j->fromaddr , f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
#endif
        //j->from-j->cmpcnt to skip the cmp
        //j->from+j->size to check for arguments
        // defined args are same as 'args' data command.....

        ofst = j->fromaddr + j->size;

        a = get_adnl(&chadnl,sub->start,1);
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
          g = get_aux_cmd(ofst, C_ARGS);
          if (g)
           {         // assume first parameter ???
            a = get_adnl(&chadnl,g->start,1);
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

          jx = get_tjmp_ix(j->fromaddr);     // start point
          while(jx < chjpt.num)
            {
             k = (JMP*) chjpt.ptrs[jx];
             if (k->toaddr != j->fromaddr) break;
             #ifdef XDBGX
              DBGPRT(1,0);
              DBGPRT(1,"To+ %x - [%x %x] (%x %x) ", j->fromaddr, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
             #endif
             //j->from-j->cmpcnt to skip the cmp
// copy f for each new branch ??
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
 if (ix < chcmd.num) n = (LBK *) chcmd.ptrs[ix];
 while (n && n->start < addr)
     {
      // n MUST be wrong - delete it. keep if higher or cmd
      if (n->cmd) break;
      if (n->fcom < C_TEXT) chdelete(&chcmd,ix,1); else break;
      if (ix < chcmd.num) n = (LBK *) chcmd.ptrs[ix]; else n = 0;
     }
return n;
}


//uint getfunceval(uint size)
//{


//}

void extend_func(uint ix)
{

  uint eadr, rsize, adr, val, eval;     //size
  ADT *a, *b;
  LBK *k, *n;

  k = (LBK *) chcmd.ptrs[ix];

//if (k->start == 0x22336)
//{
//DBGPRT(1,0);
//}








  ix++;
  if (ix < chcmd.num) n =  (LBK *) chcmd.ptrs[ix]; else n = 0;
      // check end of func

  eadr = maxpcadd(k->end);                 // safety
  a = get_adnl(&chadnl,k->start,1);        // entry 1
  if (!a) return;
  b = get_next_adnl(&chadnl,a);            // entry 2
  if (!b) return;

 // extend func to zero or max negative here

 // size = bytes(a->fend);
  rsize = bytes(a->fend) + bytes(b->fend);    // whole row size in BYTES
  eval  = get_signmask(a->fend);              // end value
  adr   = k->start;

  // need to check next command as well....

  while (adr < eadr)                      // check input value for each row (UNSIGNED)
    {
     adr += rsize;
     val = g_val(adr,0, (a->fend & 31));      // next input value as UNSIGNED
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

  eadr = maxpcadd(k->end);           // reset end

  if (n)
    {
      if (n->cmd) eadr = n->start;  // can't go past a user command.
      if (n->fcom > C_TEXT) eadr = n->start;  // or past funcs or tabs (etc)
    }
  // set row size for whole row for extend check.
  // word ->long, byte->word

  rsize = (a->fend & 31) + (b->fend & 31) +1;        // row size (unsigned)

  eval = g_val(adr,0, rsize);         // and its full row value

// must check overlap with data HERE

  while (adr < eadr)
    {       // any matching final whole rows
     adr += bytes(rsize);
     val = g_val(adr,0, rsize);
     if (val != eval) break;
     if (n && !eval && n->start == adr) break;   // match next cmd for extended zeroes
    }

  // adr is now start of next item (end of extend values)
  // if end value is zero need extra check
  // otherwise need to delete any skipped entries........

  n = del_olaps(adr, ix);

 //check again for overlaps as del-olaps may have moved n

 if (n && adr > n->start)
    {
      if (n->cmd) adr = n->start;  // can't go past a user command.
      if (n->fcom > C_TEXT) adr = n->start;  // or past funcs or tabs (etc)
    }


  // need any more here ???

//  if (eval != 0 || n == 0 || n->start >= adr)
//    {  // end of list or not zero value or no overlap
//     if (adr > k->end) k->end = adr-1;
//    }
//  else
//    {     // check next command

      // n is < and value is 0
                   // probably wrong, but safe for now
                   // extend if only one word/byte
   //             k->end = n->start-1;          // TEMP !!


                 k->end = adr-1;
 //   }


//now check if all outputs are in upper byte and < 16
// if so this is dimension for table, and can autosize it.


  rsize = bytes(a->fend) + bytes(b->fend);    // whole row size in BYTES

if (rsize == 4)
  {          //word table - check for table dim style (top byte only)
   uint lval, chg;
// prob need more rules (like eval should always be <= lval...but val must change also
// and end up at zero ??)

// need to check not signed as well !!

  adr  = k->start + bytes(b->fend);
  eadr = k->end;
  eval = 1;                         // flag
  chg  = 0;                       //value changed
  lval = 0x2000;                     // 32 in top byte 
  while (adr < eadr)                      // check input value for each row (UNSIGNED)
    {
     val = g_val(adr,0, (b->fend & 31));      // next input value as UNSIGNED
     if (val & 0xff)
        { //bottom byte set
         eval = 0;
         break;
        }
     if (val != lval) chg = 1;

     if (val > lval)
       {  // downward trend only
         eval = 0;
         break;
        }
      adr += rsize;
      lval = val;
    }

if (eval && chg)
 {       //passes test as dimension func for table
   b->flt = 1;
   b->fldat = 256.0;           //set divisor at 256
//change name ??
 }

}




 }


/* experimentals

uint test_func(uint start, uint ssize)
{
uint addr, end, rsz;
int val,lval;
if (start == 0x827b4)
{
DBGPRT(1,0);
}



rsz = casz[ssize] * 2;          // one row

addr = start;
end = start + 128;      // safety

lval = g_val(addr, ssize);   //start value


val = (~casg[ssize]) & camk[ssize];     // start value

if (lval != val) return start;


// need row extender in here for some short tables

addr += rsz;

while (addr < end)
{ 
val = g_val(addr, ssize);
if (val >= lval) break;
lval = val;
addr += rsz;
}

// 3 rows = 6 * size
if ( (addr - start) >= (rsz * 3) && (lval & camk[ssize]) == casg[ssize] )
{
DBGPRT (1,"possible func at %x-%x (%d)", start, addr, ssize);  
}

return start;
}




void find_func(uint start, uint end)
{
uint addr;
int val;

addr = start;

while (addr < end)
  {
   val = g_byte(addr);
   if (val == 0xff || val == 0x7f)
    {
     test_func(addr,1);
     test_func(addr,5);      // byte  
     if (!(addr &1))
      {
       test_func(addr,2);
       test_func(addr,6);  // word
      }
     }
    addr++;

 }
}
*/




void extend_table(uint ix)
{
  uint apeadr, maxadr, size, val, eval, temp,jx;
  ADT *a;
  LBK *k, *n;

  k = (LBK *) chcmd.ptrs[ix];


 // attempt to extend by data,use external proc
 // to do best match by data for row and col sizes

  if (ix >= chcmd.num)
   {
     apeadr = maxpcadd(k->end)+1;
     maxadr = apeadr;
   }
  else
   {
    jx = ix+1;
    n = (LBK *) chcmd.ptrs[jx];
    apeadr = n->start;             // apparent end from next cmd

    while ( n->fcom < C_TEXT && !n->cmd)
     {  // struct of user command
       jx++;
       if (jx >= chcmd.num) break;
       n = (LBK *) chcmd.ptrs[jx];    //skip all byte and word
     }

    maxadr = n->start;            // max end. not skippable
   }

  a = get_adnl(&chadnl,k->start,1);          //k->adnl;                 // cols in a->cnt

  if (!a) return;
  size = apeadr - k->start;    // size in bytes to apparent end
  eval = size/a->cnt;          // max rows
  val  = eval * a->cnt;        // size in whole rows

 temp = apeadr;
 do_table_sizes(k->start, a->cnt, &temp, maxadr);  // temp, for all tables


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
   // then investigate actual data for sizes
   k->end = k->start+val-1;       // temp

  size = do_table_sizes(k->start, a->cnt, &apeadr, maxadr);

  //need to get end address out as well.......use apeadr

 if (size)
     {
      a->cnt = size;
      k->size = size;  // should do a dsize() really         // 1st test
     if (k->start+apeadr >= k->end) k->end = k->start+apeadr-1;
     }
 else
     {
// er....
    }

 del_olaps(k->end,ix+1);       //remove any spanned entries

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
    x = get_spf(sub->start);

    if (x)
     {
      // common to both
      f.lst[0] = 0;
      f.spf    = x->spf;
      f.rg[0]  = x->addrreg;        // address
      f.ans[0] = 0;

      if (x->spf > 4 && x->spf < 13)
       {
        f.rg[1]  = 0;              // no size reg
        f.ans[1] = 1;              // mark as found
       }

      // table subroutines

      if (x->spf > 12 && x->spf < 15)
       {
        f.rg[1]  = x->sizereg;        // cols
        f.ans[1] = 0;
       }

      do_calls(sub,f);
    }

   }
// now extend the end of funcs for the extra filler rows.....
// probably need to check tables too.

   for (i = 0; i < chcmd.num; i++)
   {
     k = (LBK *) chcmd.ptrs[i];
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

  for (i = 0; i < numfiles; i++)
   {
    if (! *(fldata.fn[i])) strcpy(fldata.fn[i],fldata.path);   // use default path name

    strcat(fldata.fn[i],fldata.bare);                 // bare name
    strcat(fldata.fn[i],fd[i].suffix);                // plus suffix
   }


 return fldata.bare;
 }


/***************************************************************
 *  disassemble EEC Binary
 ***************************************************************/
short disassemble (char *fstr)
{
  int ans;

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


  if (fldata.error >= 0)
    {
     printf("File '%s' not found or cannot open\n",fldata.fn[fldata.error] );
     return fldata.error;
    }

  wnprt(1,0);
  wnprt(1,"# ----------------------------");
  wnprt(1,"# SAD Version %s (%s)", SADVERSION, SADDATE);
  wnprt(2,"# ----------------------------");

  ans = readbin();

  /* read bin file and sort out banks
   but allow getudir in case of manual set banks */

  show_prog(++anlpass);            //anlpass = 1  cmds and setup
  ans = getudir(ans);                       // AFTER readbin...

  if (numbanks < 0 || ans < 0 || cmnd.error > 1)
  {
     // wrong if no banks or fault
    wnprt(1,"Abandoned !!");
    printf("Abandon - can't process - see warnings file\n");
    return 0;
  }  

  show_prog(++anlpass);            //anlpass = 2  signature pass
  prescan_sigs();

   //check_rbase();                     // check static rbase setup (2020 etc, not as sig)

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



  // scan_dc_olaps();            // check and tidy code and data overlaps


// scan funcs
// scan tabs ??

  // scan_data_gaps();

 //  scan_lpdata();



  show_prog (++anlpass);            //anlpass = 4 ANLFFS

  wnprt(2,"# ----- End   Disassembly phase 2  -----");

  turn_scans_into_code();      // turn all valid scans into code

//  scan_code_gaps();           // scan for code gaps  TEMP !!!

  do_structs();                // then structs

//  turn_dtk_into_data();        // add data found fom dtk

 check_dtk_links();

 scan_gaps();                 // BEFORE data, so only code, tabs,vects,funcs

 turn_dtk_into_data();        // add data found fom dtk



  // ----------------------------------------------------------------------


 



   show_prog (++anlpass);                 //anlpass = 5 = ANLPRT

  #ifdef XDBGX
    DBG_data();
  #endif

  wnprt(2,"# ----- Output Listing to file %s", fldata.fn[1]);

  do_listing ();
  pstr(2,0);     // flush output file
  pstr(2, " ##########   END of Listing   ##########");



// could put messages in here with wnprt.................


  wnprt(2,0);
  wnprt(1,"# ---------------------------------------------------------------------------------------------");
  wnprt(1,"# The disassembler has scanned the binary and produced the following command list.");
  wnprt(1,"# This list is not guaranteed to be perfect, but should be a good base.");
  wnprt(1,"# Commented lines for information bu may be uncommented for use (e.g. banks)");
  wnprt(1,"# This following list can be copied and pasted into a directives file.");
  wnprt(3,"# ---------------------------------------------------------------------------------------------");

  prt_dirs();
  free_structs();
  mfree(opcbit, 0x2000);
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
     strncpy(fldata.fn[6],pts[1], 255);           // copy max of 255 chars
     i = strlen(fldata.fn[6]);                    // size to get last char
     t = fldata.fn[6]+i-1;                        // t = last char
     if (*t == PATHCHAR) *t = 0; else t++;

     sprintf(t, "%c%s",PATHCHAR,fd[6].suffix);    // and add sad.ini to it...

     fldata.fl[6] = fopen(fldata.fn[6], fd[6].fpars);

     if (!fldata.fl[6])
       {
           printf("\ncan't find config file %s\n", fldata.fn[5]);
           return 1;                // no config file in -c
       }
  }

 else
  {
    // else use bin path to make up file file name of SAD.INI
    sprintf(fldata.fn[6], "%s%c%s",fldata.dpath,PATHCHAR,fd[6].suffix);
    fldata.fl[6] = fopen(fldata.fn[6], fd[6].fpars);         //open it if there
  }


   if (fldata.fl[6])
     {                                           // file exists, read it
     for (i = 0; i< 5; i++)
      {
       if (fgets (flbuf, 256, fldata.fl[6]))
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
         flbuf[255] = '\0';                    // safety 
         strcpy(fldata.fn[i], flbuf);
       }
      }
     fclose(fldata.fl[6]);                 //close leaves handle addr
     fldata.fl[6] = NULL;
     strcpy(fldata.path, fldata.fn[0]);
     strcpy(fldata.fn[5], fldata.fn[2]);   //copy wrn path to dbg path

 //    printf("config file OK\n");
    }
  else printf(" - not found, assume local dir\n");

  return 0;
 }

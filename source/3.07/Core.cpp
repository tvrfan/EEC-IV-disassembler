
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version 3.07
 ******************************************************
 * NB - clean up all extra test stuff and recheck all bins for correct (as much as possible) output
 
 * This code assumes INT is at least 32 bits, on some older compilers this would require
 * the use of LONG instead of INT.
 * On Win32 builds (Codeblocks mingw)      int = long = void* = 32 bits.
 * On Linux builds (CodeBlocks gcc amd64)  int = 32 bits, long = void* = 64 bits.

 ******************************************************
 *  Declarations and includes
 *******************************************************/

#include  "core.h"             // this calls shared.h
#include  "sign.h"

// debug defines - when switched on, this causes a LOT of output to wrn file
// DBGPRT is kept to make debug code more obvious to view

// #define XDBGX

#ifdef XDBGX
#define DBGPRT    wnprt        // for debug printing to xx_wrn.txt file
#define DBGPRTN   wnprtn
#endif

/**********************************************************************************
external subroutine and variable declarations
***********************************************************************************/

extern PAT intign;                // sig 'ignore interrupt'  (for naming)
extern PAT hdr;                   // sig 'start jump' (and vects);
extern PAT fnlp;                  // function lookup
extern PAT tblp;
extern PAT tblx;                   // table lookups type 1 and 2
 
void  show_prog    (int);
SIG*  do_sig       (PAT *, int);
SIG*  scan_sigs    (int);
SIG*  scan_asigs   (int);
void  prescan_sigs (void);

/*****************************************************************************
* declarations for subroutines in command structure declaration
***************************************************************************/

// print processors, 

int pp_dft   (int, int); int pp_wdbt  (int, int); int pp_text  (int, int);
int pp_vect  (int, int); int pp_code  (int, int); int pp_stct  (int, int);
int pp_timer (int, int);


// command processors

int set_vect (CPS*); int set_code (CPS*); int set_scan (CPS*); int set_cdih (CPS*);
int set_args (CPS*); int set_data (CPS*); int set_opts (CPS*); int set_rbas (CPS*);
int set_sym  (CPS*); int set_subr (CPS*); int set_bnk  (CPS*); int set_time (CPS*);
int set_ordr (CPS*);


// CHAIN subroutines - free blocks for various chain types

void fsym(void *); void fscn(void *); void fsub(void *); 
void fcmd(void *); void fblk(void *);

// CHAIN subroutines - compares, for binary chop searches

int cpfjp  (int, void *); int cptjp  (int, void *); int cpsym  (int, void *);
int cpbase (int, void *); int cpsig  (int, void *); int cpcmd  (int, void *);
int cpacmd (int, void *); int cpscan (int, void *); int cpsub  (int, void *);
int cpddt  (int, void *);

// CHAIN subroutines - equals. where extra logic is needed over a compare

int eqcmd  (int, void *); int eqacmd (int, void *); int eqbase (int, void *);
int eqsym  (int, void *);

// CHAIN structures.  These use common find and insert mechanisms with defined subroutines for each chain.
// params = num entries, num ptrs, allocsize, entrysize, maxptrs, num mallocs, ptr array,
// subroutines - free, compare, equals

CHAIN jpfch  = {0,0,500,sizeof(JLT),   0,0,0,0    ,cpfjp  , cpfjp };      // jump from
CHAIN jptch  = {0,0,500,0          ,   0,0,0,0    ,cptjp  , cptjp };      // jump to REMOTE ptrs to jpfch
CHAIN symch  = {0,0,400,sizeof(SYT),   0,0,0,fsym ,cpsym  , eqsym };      // syms read
CHAIN basech = {0,0, 20,sizeof(RBT),   0,0,0,0    ,cpbase , eqbase};      // rbase
CHAIN sigch  = {0,0, 40,sizeof(SIG),   0,0,0,0    ,cpsig  , cpsig };      // signature
CHAIN auxch  = {0,0, 10,sizeof(LBK),   0,0,0,fcmd ,cpacmd , eqacmd};      // aux cmd (xcode, args)
CHAIN cmdch  = {0,0,200,sizeof(LBK),   0,0,0,fcmd ,cpcmd  , eqcmd };      // commands
CHAIN scanch = {0,0,750,sizeof(SBK),   0,0,0,fscn ,cpscan , cpscan};      // scan  bring back eqscan ?
CHAIN subch  = {0,0,300,sizeof(SXT),   0,0,0,fsub ,cpsub  , cpsub };      // subr
CHAIN datch  = {0,0,500,sizeof(DDT),   0,0,0,0    ,cpddt  , cpddt };      // data addresses detected

CHAIN mblk   = {0,0,100,sizeof(MBL),   0,0,0,fblk ,0      , 0    };       // tracks CHAIN mallocs for cleanup

// pointer array for neater start and tidyup loops.

CHAIN *chlist[] = { &jpfch, &jptch, &symch, &basech, &sigch, &auxch, &cmdch, &scanch,
                    &subch, &datch, &mblk };
					

/********** COMMAND Option letters -

      as opt            in cmd extension
 A
 B
 C    C code            Class ? (not done yet)
 D                      Fixed offset for param (like rbase) (word or byte)   or 2nd BANK   (vect)
 E                      ENCODED  type, rbase
 F                      special function type  - type, addr, cols sign
 G    Auto sub names
 H    set 8065
 I
 J
 K
 L    label names       int
 M    manual mode       ? mask ?(byte or word pair) not done yet  MULTIPLIER ??  
 N    Int proc names    Name lookup
 O                      Count (cols etc)
 P    proc names        Print fieldwidth
 Q                      Quit (terminator) Byte (or Word) for structs
 R                      Indirect/pointer (vector) in struct   write symbol in SYM ?
 S    find signatures   Signed    (unsigned otherwise)
 T                      BiT (with symbol)           (table if data cmd?)
 U                                  [ Unit - text mS Volts etc ?]
 V                      Divisor (Scaler) - Float 
 W                      Word
 X    Decimal print     Decimal print
 Y                      bYte
 Z
 ***************************************************************/


/*********************
main command structure -
params are -
command string, command processor, command printer, max addnl levels, max addresses/numbers expected,
single adress posn, 'start/end' address pair posn, name expected, overrides (bit mask), merge allowed,  options string (scanf)

merge is with same cmd, override is bitmask

******/


DIRS dirs[] = {
{"fill"    , set_data,  pp_dft,   0,  2, 0, 1, 0, 0, 1, 0},                      // fill - default -> MUST be zero ix
{"byte"    , set_data,  pp_wdbt,  2,  2, 0, 1, 0, 3, 1, "%[:PSX]%n" },           // pfw (min print field width), signed, decimal
{"word"    , set_data,  pp_wdbt,  2,  2, 0, 1, 0, 1, 1, "%[:LPSX]%n" },          // + Long
{"text"    , set_data,  pp_text,  0,  2, 0, 1, 0, 7, 1, 0  } ,

{"table"   , set_data,  pp_stct,  2,  2, 0, 1, 1, 7, 0, "%[:OPSVWXY]%n" },
{"func"    , set_data,  pp_stct,  3,  2, 0, 1, 1, 7, 0, "%[:LPSVWXY]%n" } ,
{"struct"  , set_data,  pp_stct,  15, 2, 0, 1, 1, 7, 0, "%[:|DLMNOPRSVWXYQ]%n" },

{"timerl"  , set_time, pp_timer,  2,  2, 0, 1, 1, 7, 0, "%[:YWNT]%n" },           // timer list
{"vect"    , set_vect, pp_vect,   2,  2, 0, 1, 1, 7, 1, "%[:DQ]%n"} ,             // bank, quit byte (always hex) N ? names
{"args"    , set_args, pp_dft,   15,  2, 0, 1, 0, 0, 0, "%[:DELNOPSWXY]%n" },     // call address, not subr

{"subr"    , set_subr, pp_dft,   15,  1, 0, 1, 1, 0, 0, "%[:DEFLNOPSUWXY]%n" },
{"xcode"   , set_cdih, pp_dft,   0,   2, 0, 1, 0, 0, 1, 0  } ,
{"code"    , set_code, pp_code,  0,   2, 1, 0, 0, 7, 1, 0  } ,                    // may be more than 7 for ovrdes

{"scan"    , set_scan, pp_dft ,  0,   1, 1, 0, 0, 0, 0, 0   },
{"opts"    , set_opts, pp_dft,   2,   0, 0, 0, 0, 0, 0, "%[:CDHLGMNPS]%n" },      // add more here....
{"rbase"   , set_rbas, pp_dft,   0,   4, 2, 3, 0, 0, 0, 0   },

{"sym"     , set_sym,  pp_dft,   2,   3, 1, 2, 1, 0, 0, "%[:FTW]%n"    },         // flags,bit,write

{"banks"   , set_bnk,  pp_dft,   0,   4, 0, 2, 0, 0, 0, 0 },                      //  file map/offsets
{"order"   , set_ordr, pp_dft,   2,   4, 0, 0, 0, 0, 0, 0 }
};


const char *empty = "";                                    // gets around compiler warnings

/**********************
 *  opcode definition table
 **********************/

// index for opcodes - points into main opcode table - 0 is invalid 
// rordered to put main opcode types together.

#define LDWIX  41          // for 3 op replacements to use "op = op"


uchar opcind[256] =
{
// 0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f 
   80,  89,  96,  91,  0,   98, 100,  93,  4,   1,   5,   0,   6,   3,   7,   94,    // 00 - 0f
// skip,clrw,cplw,negw,----,decw,sexw,incw,shrw,shlw,asrw,----,shrd,shld,asrd,norm
   112, 88,  95,  90,  0,   97,  99,  92,  8,   2,   9,   0,   0,   0,   0,   0,     // 10 - 1f
// RBNK,CLRB,CPLB,NEGB,----,DECB,SEXB,INCB,SHRB,SHLB,ASRB,----
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
   55,  63,  64,  57,  61,  59,  66,  69,  54,  62,  65,  56,  60,  58,  67,  68,    // d0 - df 
// JNST,JLEU,JGT, JNC, NVT, JNV, JGE, JNE, JST, JGTU,JLE, JC,  JVT, JV,  JLT, JE
   72,   0,   0,   0,   0,   0,   0,  75,   0,   0,   0,   0,   0,   0,   0,  73,    // e0 - ef 
// DJNZ                               JUMP                                    CALL
   76,  78,  84,  86, 107, 108, 109,   0, 102, 101, 103, 104, 105, 110, 111, 106     // f0 - ff 
// RET, RETI,PSHP,POPP,Bnk0,Bnk1,Bnk2,    CLC, STC, DI,  EI,  CLVT,Bnk3,SIGN,NOP
};


/*

 Signature index value for like instructions - MAX 63 
 0  special   1  clr    2  neg/cpl     3  shift l   4  shift R   5 and
 6  add       7  subt   8  mult    9  or       10 cmp       11 divide
 12 load      13 sto    14 push    15 pop      16 jumps     17 subr
 18 djnz      19 condjmp 20 ret     21 carry   22 other

 * complicated ones -
 23 jnb/jb - can cross match to 19 with extra par

 24 dec     25 inc
 12 and 13 (ldx, stx) can be regarded as equal with params swopped over...
  
 40+ are optional and skipped, but detectable with -

 40 sig      41 pushp    42 popp    43 di       44 ei       45 (RAM)bnk
 46 (ROM)bnk 47 nop      48 others
*/

// code emulation procs ref'ed in opcode struct

void clr  (SBK *); void ldx  (SBK *); void stx  (SBK *); void orx  (SBK *);
void add  (SBK *); void xand (SBK *); void neg  (SBK *); void cpl  (SBK *);
void sub  (SBK *); void cmp  (SBK *); void mlx  (SBK *); void dvx  (SBK *);
void shl  (SBK *); void shr  (SBK *); void popw (SBK *); void pshw (SBK *);
void inc  (SBK *); void dec  (SBK *); void bka  (SBK *); void cjm  (SBK *);
void bjm  (SBK *); void djm  (SBK *); void ljm  (SBK *); void cll  (SBK *);
void ret  (SBK *); void scl  (SBK *); void skj  (SBK *); void sjm  (SBK *);
void php  (SBK *); void ppp  (SBK *); void nrm  (SBK *); void sex  (SBK *);
void clc  (SBK *); void stc  (SBK *); void die  (SBK *); void nop  (SBK *);
void clv  (SBK *);


// reordered to put main types together
// signature ix, sign, p65, alters psw, num ops, write op, write size ,read size op1,2, 3/flag, extra flags,name, source code

OPC opctbl[] = {                                                               // 113 entries
{ 0 ,  0, 0,   0, 0,0, 0,0,0,0, 0     ,"!INV!" ,  "" },                        // zero entry is for invalid opcodes

{ 3 ,  0, 0,   1, 2,2, 2,1,2,0, shl   ,"shlw"  ,  "\x2 <<= \x1;" },         // 1
{ 3 ,  0, 0,   1, 2,2, 1,1,1,0, shl   ,"shlb"  ,  "\x2 <<= \x1;" },         // 2 
{ 3 ,  0, 0,   1, 2,2, 3,1,3,0, shl   ,"shldw" ,  "\x7\x2 <<= \x1;" },      // 3

{ 4 ,  0, 0,   1, 2,2, 2,1,2,0, shr   ,"shrw"  ,  "\x2 >>= \x1;" },         // 4
{ 4 ,  0, 0,   1, 2,2, 2,1,6,0, shr   ,"asrw"  ,  "\x7\x2 >>= \x1;" },      // 5
{ 4 ,  0, 0,   1, 2,2, 3,1,3,0, shr   ,"shrdw" ,  "\x7\x2 >>= \x1;" },      // 6
{ 4 ,  0, 0,   1, 2,2, 7,1,7,0, shr   ,"asrdw" ,  "\x7\x2 >>= \x7\x1;" },   // 7
{ 4 ,  0, 0,   1, 2,2, 1,1,1,0, shr   ,"shrb"  ,  "\x2 >>= \x1;" },         // 8
{ 4 ,  0, 0,   1, 2,2, 5,1,5,0, shr   ,"asrb"  ,  "\x7\x2 >>= \x1;" },      // 9

{ 5 ,  0, 0,   1, 2,2, 1,1,1,0, xand  ,"an2b"  , "\x2 &= \x1;"  },          // 10
{ 5 ,  0, 0,   1, 2,2, 2,2,2,0, xand  ,"an2w"  , "\x2 &= \x1;" },           // 11
{ 5 ,  0, 0,   1, 3,3, 1,1,1,1, xand  ,"an3b"  , "\x3 = \x2 & \x1;"  },     // 12
{ 5 ,  0, 0,   1, 3,3, 2,2,2,2, xand  ,"an3w"  , "\x3 = \x2 & \x1;" },      // 13

{ 8 ,  1, 0,   1, 2,2, 2,1,1,0, mlx   ,"ml2b"  , "\x7\x2 = \x2 * \x1;" },   // 14
{ 8 ,  0, 0,   1, 2,2, 6,5,5,0, mlx   ,"sml2b" , "\x7\x2 = \x2 * \x1;"},
{ 8 ,  1, 0,   1, 2,2, 3,2,2,0, mlx   ,"ml2w"  , "\x7\x2 = \x2 * \x1;"  },
{ 8 ,  0, 0,   1, 2,2, 7,6,6,0, mlx   ,"sml2w" , "\x7\x2 = \x2 * \x1;"},

{ 8 ,  1, 0,   1, 3,3, 2,1,1,1, mlx   ,"ml3b"  , "\x7\x3 = \x2 * \x1;"  },   // 18
{ 8 ,  0, 0,   1, 3,3, 6,5,5,2, mlx   ,"sml3b" , "\x7\x3 = \x2 * \x1;" },    // 19
{ 8 ,  1, 0,   1, 3,3, 3,2,2,2, mlx   ,"ml3w"  , "\x7\x3 = \x2 * \x1;"  },   // 20
{ 8 ,  0, 0,   1, 3,3, 7,6,6,2, mlx   ,"sml3w" , "\x7\x3 = \x2 * \x1;" }, 

{ 6 ,  0, 0,   1, 2,2, 1,1,1,0, add   ,"ad2b"  , "\x2 += \x1;" },            // 22
{ 6 ,  0, 0,   1, 2,2, 2,2,2,0, add   ,"ad2w"  , "\x2 += \x1;" },
{ 6 ,  0, 0,   1, 3,3, 1,1,1,1, add   ,"ad3b"  , "\x3 = \x2 + \x1;"  },
{ 6 ,  0, 0,   1, 3,3, 2,2,2,2, add   ,"ad3w"  , "\x3 = \x2 + \x1;" },       // 25

{ 7 ,  0, 0,   1, 2,2, 1,1,1,0, sub   ,"sb2b"  , "\x2 -= \x1;" },            // 26
{ 7 ,  0, 0,   1, 2,2, 2,2,2,0, sub   ,"sb2w"  , "\x2 -= \x1;" },
{ 7 ,  0, 0,   1, 3,3, 1,1,1,1, sub   ,"sb3b"  , "\x3 = \x2 - \x1;"  },
{ 7 ,  0, 0,   1, 3,3, 2,2,2,2, sub   ,"sb3w"  , "\x3 = \x2 - \x1;" },

{ 9 ,  0, 0,   1, 2,2, 1,1,1,0, orx   ,"orb"   , "\x2 |= \x1;" },            // 30
{ 9 ,  0, 0,   1, 2,2, 2,2,2,0, orx   ,"orw"   , "\x2 |= \x1;" },
{ 9 ,  0, 0,   1, 2,2, 1,1,1,0, orx   ,"xorb"  , "\x2 ^= \x1;" },        //32
{ 9 ,  0, 0,   1, 2,2, 2,2,2,0, orx   ,"xrw"   , "\x2 ^= \x1;" },       //33

{ 10,  0, 0,   1, 2,0, 1,1,1,0, cmp   ,"cmpb"  , "" },                //34
{ 10,  0, 0,   1, 2,0, 0,2,2,0, cmp   ,"cmpw"  , "" },

{ 11,  1, 0,   1, 2,2, 2,1,2,0, dvx   ,"divb"  , "\x2 = \x7\x2 / \x1;" },     //36
{ 11,  0, 0,   1, 2,2, 6,5,6,0, dvx   ,"sdivb" , "\x2 = \x7\x2 / \x1;"},
{ 11,  1, 0,   1, 2,2, 3,2,3,0, dvx   ,"divw"  , "\x2 = \x7\x2 / \x1;" },
{ 11,  0, 0,   1, 2,2, 7,6,7,0, dvx   ,"sdivw" , "\x2 = \x7\x2 / \x1;"},      //39

{ 12,  0, 0,   0, 2,2, 1,1,1,0, ldx   ,"ldb"   , "\x2 = \x1;" },            //40
{ 12,  0, 0,   0, 2,2, 2,2,2,0, ldx   ,"ldw"   , "\x2 = \x1;" },

{ 6 ,  0, 0,   1, 2,2, 1,1,1,0, add   ,"adcb"  , "\x2 += \x1 + CY;" },       //42
{ 6 ,  0, 0,   1, 2,2, 2,2,2,0, add   ,"adcw"  , "\x2 += \x1 + CY;" },
{ 7 ,  0, 0,   1, 2,2, 1,1,1,0, sub   ,"sbbb"  , "\x2 -= \x1 - CY;" },
{ 7 ,  0, 0,   1, 2,2, 2,2,2,0, sub   ,"sbbw"  , "\x2 -= \x1 - CY;" },

{ 12,  0, 0,   0, 2,2, 2,1,1,0, ldx   ,"ldzbw" , "\x7\x2 = \x7\x1;" },  // 46
{ 12,  0, 0,   0, 2,2, 6,1,1,0, ldx   ,"ldsbw" , "\x7\x2 = \x7\x1;" },

{ 13,  0, 0,   0, 2,1, 1,1,1,0, stx   ,"stb"   , "\x1 = \x2;" },
{ 13,  0, 0,   0, 2,1, 2,2,2,0, stx   ,"stw"   , "\x1 = \x2;" },            //49

{ 14,  1, 0,   0, 1,0, 0,2,0,0, pshw  ,"push"  , "push(\x1);" },        // 50  or [STACK++] = \x1
{ 14,  0, 1,   0, 1,0, 0,2,0,0, pshw  ,"pusha" , "pusha(\x1);" },
{ 15,  1, 0,   0, 1,1, 2,2,0,0, popw  ,"pop"   , "\x1 = pop();" },      // 52 or \x1 = [STACK--];
{ 15,  0, 1,   0, 1,1, 2,2,0,0, popw  ,"popa"  , "\x1 = popa();" },
 

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jst"   ,  "\x8STC = 1) \x9" },            // 54  cond jumps
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jnst"  ,  "\x8STC = 0) \x9" },            // 55

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jc"    ,  "if (CY = 1) \x9" },            // 56
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jnc"   ,  "if (CY = 0) \x9" },            // 57

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jv"    ,  "\x8OVF = 1) \x9" },            // 58
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jnv"   ,  "\x8OVF = 0) \x9" },            // 59

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jvt"   ,  "\x8OVT = 1) \x9" },            // 60
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jnvt"  ,  "\x8OVT = 0) \x9" },            // 61

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jgtu"  ,  "\x8(uns) \x6 > \x5 ) \x9" },   // 62
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jleu"  ,  "\x8(uns) \x6 <= \x5) \x9" },   // 63

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jgt"   ,  "\x8\x6 > \x5) \x9" },          // 64
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jle"   ,  "\x8\x6 <= \x5) \x9" },         // 65

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jge"   ,  "\x8\x6 >= \x5) \x9" },         // 66
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jlt"   ,  "\x8\x6 < \x5) \x9" },          // 67

{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"je"    ,  "\x8\x6 = \x5) \x9" },          // 68
{ 19,  0, 0,   0, 1,0, 0,2,0,0, cjm  ,"jne"   ,  "\x8\x6 != \x5) \x9" },         // 69

{ 23,  0, 0,   0, 2,0, 0,2,1,1, bjm  ,"jnb"   ,  "\x8\x3\x2 = 0) \x9" },                //70
{ 23,  0, 0,   0, 2,0, 0,2,1,1, bjm  ,"jb"    ,  "\x8\x3\x2 = 1) \x9" },                //71

{ 18,  0, 0,   0, 2,2, 2,2,1,0, djm  ,"djnz"  ,  "\x2--;\xb\x8\x2 != 0) \x9" },         //72
          
{ 17,  1, 0,   0, 1,0, 0,2,0,0, cll  ,"call"  ,  "\x1\xc" },        // 73   Long jumps
{ 17,  0, 1,   0, 2,0, 0,0,0,0, cll  ,"calla" , "\x1\xc" },        // 74  
{ 16,  0, 0,   0, 1,0, 0,2,0,0, ljm  ,"jump"  ,  "\x9" },           // 75
 
{ 20,  1, 0,   0, 0,0, 0,2,0,0, ret  ,"ret"   ,  "return;" },       //76
{ 20,  0, 1,   0, 0,0, 0,0,0,0, ret  ,"reta"  , "return;" },
{ 20,  1, 0,   0, 0,0, 0,2,0,0, ret  ,"reti"  ,  "return;" },       //78
{ 20,  0, 1,   0, 0,0, 0,0,0,0, ret  ,"retia" , "return;" },

{ 16,  0, 0,   0, 0,0, 0,2,0,0, skj  ,"skip"  ,  "\x9" },           // 80     skip = sjmp [pc+2]
{ 16,  0, 0,   0, 1,0, 0,2,1,0, sjm  ,"sjmp"  ,  "\x9" },           // 81     short jumps
{ 17,  1, 0,   0, 1,0, 0,2,1,0, scl  ,"scall" ,  "\x1\xc" },        // 82
{ 17,  0, 0,   0, 1,0, 0,0,0,0, scl  ,"sclla" , "\x1\xc" },        // 83

{ 41,  1, 0,   0, 0,0, 0,0,0,0, php  ,"pushp" ,  "push(PSW);" },     //84
{ 41,  0, 1,   0, 0,0, 0,0,0,0, php  ,"pshpa" , "pusha(PSW);" },
{ 42,  1, 0,   1, 0,0, 0,0,0,0, ppp  ,"popp"  ,  "PSW = pop();" },     //86
{ 42,  0, 1,   1, 0,0, 0,0,0,0, ppp  ,"poppa" , "PSW = popa();" },   


{ 1 ,  0, 0,   0, 1,1, 1,1,0,0, clr  ,"clrb"  ,  "\x1 = 0;" },        // 88
{ 1 ,  0, 0,   0, 1,1, 2,2,0,0, clr  ,"clrw"  ,  "\x1 = 0;" }, 

{ 2 ,  0, 0,   1, 1,1, 1,1,0,0, neg  ,"negb"  ,  "\x1 = -\x1;" },       //90
{ 2 ,  0, 0,   1, 1,1, 2,2,0,0, neg  ,"negw"  ,  "\x1 = -\x1;" }, 

{ 25,  0, 0,   1, 1,1, 1,1,0,0, inc  ,"incb"  ,  "\x1++;"  },
{ 25,  0, 0,   1, 1,1, 2,2,0,0, inc  ,"incw"  ,  "\x1++;" },           //93
{ 22,  0, 0,   1, 2,1, 1,1,3,0, nrm  ,"norm"  ,  "\x7\x1 = nrml(\x7\x2);" },
{ 2 ,  0, 0,   1, 1,1, 1,1,0,0, cpl  ,"cplb"  ,  "\x1 = ~\x1;" },          // 95
{ 2 ,  0, 0,   1, 1,1, 2,2,0,0, cpl  ,"cplw"  ,  "\x1 = ~\x1;" },

{ 24,  0, 0,   1, 1,1, 1,1,0,0, dec  ,"decb"  ,  "\x1--;" },                // 97
{ 24,  0, 0,   1, 1,1, 2,2,0,0, dec  ,"decw"  ,  "\x1--;"  },
{ 22,  0, 0,   1, 1,1, 6,1,0,0, sex  ,"sexb"  ,  "\x7\x1 = \x7\x1;" },
{ 22,  0, 0,   0, 1,1, 7,2,0,0, sex  ,"sexw"  ,  "\x7\x1 = \x7\x1;" },      //100


{ 21,  0, 0,   1, 0,0, 0,0,0,0, stc  ,"stc"   ,  "CY = 1;" },                //101
{ 21,  0, 0,   1, 0,0, 0,0,0,0, clc  ,"clc"   ,  "CY = 0;" },                // 102
{ 43,  0, 0,   0, 0,0, 0,0,0,0, die  ,"di"    ,  "disable ints;" },
{ 44,  0, 0,   0, 0,0, 0,0,0,0, die  ,"ei"    ,  "enable ints;" }, 
{ 48,  0, 0,   0, 0,0, 0,0,0,0, clv  ,"clrvt" ,  "OVT = 0" },
{ 47,  0, 0,   0, 0,0, 0,0,0,0, nop  ,"nop"   ,  "" },

{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk0"  ,  "" },
{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk1"  ,  "" },
{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk2"  ,  "" },                    //109
{ 45,  0, 1,   0, 0,0, 0,0,0,0, bka  ,"bnk3"  ,  "" },    // 110
 
{ 46,  0, 1,   0, 1,0, 0,1,0,0, 0    ,"rbnk"  ,  "" },  // 111 bank prefix. here for signatures
{ 40,  0, 0,   0, 0,0, 0,0,0,0, 0    ,"!PRE!" ,  "" }   // 112 signed prefix op

};



// special modded opcode sources
const char *scespec[] = {
"\x2 += CY;", 
"\x2 -= CY;"

 };
 
 /* interrupt function names in array for auto naming
  * "HSI_" and default name is output as prefix in code
  * switched on/off with Opt N

params -
start, end   address of interrupt (+ 0x2010)
I8065  1 = 8065 interrupt, 0 is both 8065 and 8061
num    1 if name has number appended (8065)
name
 */

 
NAMES inames[] = {
 0,   0, 0, 0, "HSO_2",            //  8061 ints - actual at 2010
 1,   1, 0, 0, "Timer_OVF",
 2,   2, 0, 0, "AD_Rdy",
 3,   3, 0, 0, "HSI_Data",
 4,   4, 0, 0, "External",
 5,   5, 0, 0, "HSO_1",
 6,   6, 0, 0, "HSI_1",
 7,   7, 0, 0, "HSI_0",            // 201f  - end 8061 list

 0,  15, 1, 1, "HSO_",           // 8065 ints - 2010-201f (plus num)
 16, 16, 1, 0, "HSI_FIFO",
 17, 17, 1, 0, "External",
 18, 18, 1, 0, "HSI_0",
 19, 19, 1, 0, "HSI_Data",
 20, 20, 1, 0, "HSI_1",
 21, 21, 1, 0, "AD_Imm_Rdy",
 22, 22, 1, 0, "AD_Timed_Rdy",
 23, 23, 1, 0, "ATimer_OVF",
 24, 24, 1, 0, "AD_Timed_Start",
 25, 25, 1, 0, "ATimer_reset",
 26, 29, 1, 1, "Counter_",            // add num
 30, 39, 1, 1, "Software_"           // add num 2040-205f    end 8065
};


// Other auto names, which have numbered suffix.
// current num, mask (to switch on/ff) name (base) string

AUTON anames [] = {
 {1, 0, "Timer"     },        // used as a SPECIAL for timers.
 {1, P, "Sub"     },          // 1   PPROC   proc names (xfunc, vfunc, intfunc)
 {1, L, "Lab"     },          // 2   PLABEL  label names (lab)
 {1, F, "Func"    },          // 3   PFUNC   function data names
 {1, F, "Table"   },          // 4   PFUNC   table data names

 {1, G, "UUBFunLU" },         // 5   auto subroutine names for function and table lookup procs
 {1, G, "USBFunLU" },  
 {1, G, "SUBFunLU" },  
 {1, G, "SSBFunLU" },  
 
 {1, G, "UUWFunLU" },         // 9
 {1, G, "USWFunLU" },  
 {1, G, "SUWFunLU" },  
 {1, G, "SSWFunLU" },  
 
 {1, G, "UTabLU"  },          //  13
 {1, G, "STabLU"  }
 

 };


/***** Auto default symbols *******/

// name, addr, bitno, flag (8065 = 2,8061 = 1), write flag

DFSYM defsyms [] = {
{ "CPU_OK"        ,    2,   6, 3,0 },     //both 61 and 65
{ "LSO_Port"      ,    2,  -1, 3,0 },
{ "LIO_Port"      ,    3,  -1, 3,0 },
{ "AD_Low"        ,    4,  -1, 3,0 },
{ "AD_High"       ,    5,  -1, 3,0 },
{ "IO_Timer"      ,    6,  -1, 3,0 },
{ "IO_Timer_Hi"   ,    7,  -1, 1,0 },     // 61 only
{ "IO_Timer_Mid"  ,    7,  -1, 2,0 },     // 65 only
{ "INT_Mask"      ,    8,  -1, 3,0 },
{ "INT_Pend"      ,    9,  -1, 3,0 },
 {"HSO_OVF"       ,  0xA ,  0, 3,0 },
 {"HSI_OVF"       ,  0xA ,  1, 3,0 },
 {"HSI_Ready"     ,  0xA ,  2, 3,0 },
 {"AD_Ready"      ,  0xA ,  3, 3,0 },
 {"INT_Service"   ,  0xA ,  4, 1,0 },           // 8061 only
 {"INT_Priority"  ,  0xA ,  5, 1,0 },
 {"MEM_Expand"    ,  0xA ,  4, 2,0 },           // 8065 only
 {"HSO_PCNT_OVF"  ,  0xA ,  5, 2,0 },
 {"AD_TMD_RDY"    ,  0xA ,  6, 2,0 },
 {"HSO_PORT_OVF"  ,  0xA ,  7, 2,0 },
 {"IO_Status"     ,  0xA , -1, 3,0 },
 {"HSI_Sample"    ,  0xB , -1, 3,0 },
 {"HSI_Mask"      ,  0xC , -1, 3,0 },
 {"HSI_Data"      ,  0xD , -1, 3,0 },
 {"HSI_Time"      ,  0xE , -1, 3,0 },
 {"HSI_Time_Hi"   ,  0xF , -1, 1,0 },
 {"AD_TMD_HI"     ,  0xF , -1, 2,0 },

 {"STACK"         ,  0x10, -1, 1,0 },        // 8061 stack
 {"HSO_IntPend1"  ,  0x10, -1, 2,0 },        // 8065 only
 {"HSO_IntMask1"  ,  0x12, -1, 2,0 },
 {"IO_Timer_Hi"   ,  0x13, -1, 2,0 },
 {"HSO_IntPend2"  ,  0x14, -1, 2,0 },
 {"LSSI_A"        ,  0x15, -1, 2,0 },
 {"HSO_IntMask2"  ,  0x16, -1, 2,0 },
 {"LSSI_B"        ,  0x17, -1, 2,0 },
 {"HSO_Status"    ,  0x18, -1, 2,0 },
 {"LSSI_C"        ,  0x19, -1, 2,0 },
 {"HSI_MODE"      ,  0x1A, -1, 2,0 },
 {"HSO_UCNT"      ,  0x1B, -1, 2,0 },
 {"HSO_TIMEC"     ,  0x1C, -1, 2,0 },
 {"LSSI_D"        ,  0x1D, -1, 2,0 },
 {"HSO_Last"      ,  0x1E, -1, 2,0 },
 {"HSO_Select"    ,  0x1F, -1, 2,0 },
 {"STACK"         ,  0x20, -1, 2,0 },        // 8065 stacks
 {"ALTSTACK"      ,  0x22, -1, 2,0 },

 {"AD_Cmd"        , 4    , -1, 3,1 },           // Write symbols from here
 {"WDG_Timer"     , 5    , -1, 3,1 },
 {"AD_TMD_CMD"    , 7    , -1, 2,1 },
 {"HSO_Cmd"       , 0xd  , -1, 3,1 },
 {"HSO_Time"      , 0xe  , -1, 3,1 },
 {"HSO_Time_Hi"   , 0xf  , -1, 3,1 },
 {"BANK_SEL"      , 0x11,  -1, 2,1 },
 {"LSSO_A"        , 0x15 , -1, 2,1 },
 {"LSSO_B"        , 0x17 , -1, 2,1 },
 {"LSSO_C"        , 0x19 , -1, 2,1 },
 {"LSSO_D"        , 0x1D , -1, 2,1 },
};

/**************************************************/
// main variables

HDATA fldata;                 // files data holder
char pathchar;

// IBUF - FIRST 8192 (0x2000) reserved for RAM and reg data, then 4*0x10000 banks,
// ref'ed by nbuf pointers in bkmap  (with PCORG offset).  bkmap[BMAX] is used for RAM [0-1fff]
// opbit is used for opcode start flags

uchar ibuf[0x42000];              // 4 banks of 0xffff + 2000 ram at front
uint  opbit[0x2000];              // 4 banks of 32 flags (0x800 each = 0x10000 flags)

BANK bkmap[BMAX+1];               // bank map - allow for full range of 16 banks PLUS 1 for register data.

BANK *lastbmap;                   // test for speedup of mapping


// size (bytes), print fieldwidth, size mask for opcode variable sizes. 

int  casz[8] = {0,1,2,4,0,1,2,4};                      // data size, byte,short,long, unsigned, signed
int  cafw[8] = {0,3,5,7,0,4,6,8};                      // print field width, byte,short,long, unsigned, unsigned
uint camk[8] = {0,0xff,0xffff,0xffffffff,0,0xff,0xffff,0xffffffff};     //size mask


/* positions of columns in printout - may allow changes to these later....
 * 
28      where opcode mnemonic starts (after header)  [26 if no bank prefix]
34      where opcode data starts (+6)
51      where pseudo source code starts  (+17)
85      where comment starts      (+34)
180     max width of page - wrap here
*/

int cposn[5] = {26, 32, 49, 83, 180};

#ifdef XDBGX
  int DBGxcalls   = 0;            // progress bar updates to screen - ref'ed externally
  int DBGmcalls   = 0;            // malloc calls
  int DBGmaxholds = 0;           // for debug.
  int DBGholdsp   = 0;
  int DBGmrels    = 0;            // malloc releases
  int DBGwtcnt    = 0;            // watch count
  int DBGmaxalloc = 0;           //  TEMP for malloc tracing
  int DBGcuralloc = 0;
#endif

int tscans = 0;             // total scan count, for safety

int  anlpass    = 0;        // number of passes done so far
int  cmdopts    = 0;        // command options

int  numbanks;              // number of 8065 banks-1

SYT  *snam[16];             // sym names temp holder for bitwise flags
char flbuf[256];            // for directives and comment file reads
char nm [128];              // for temp name construction, etc.

int  gcol;                  // where print is at (no of chars = colno)

int lastpad;                // last padded to (column)

int  cmntofst;              // next addr for comment from cmt file
char *cmtt;                 //  next comment text as read from file
int  cmntnl;                //  newlines for next comment
char autocmt [128];         // auto added comment(s)
INST *cmntinst;             // auto added comment for operands;

//int psw;                    // processor status word

int datbank;                // currently selected data bank - always 8 if single bank
int codbank;                // currently selected code bank - always 8 if single bank
int rambank;                // currently selected RAM bank  - always 0 for 8061

// opcode and operands pile

#define PILESZ  128

INST instpile[PILESZ];       // last opcodes list (instances)
OPS  *curops;                // pointer to current operands
INST *curinst;               // current instruction instance

int lpix;                    // index for instance pile searches

int minwreg, maxwreg;        // min and max ram register (for checking)
int stkreg1, stkreg2;        // stack registers, 2 stacks for 8065

int schblk[128];              // search block, used for find - overlayed with whatever struct
ushort ramlist[NPARS];          // (16) last updated ram address list for HOLD (address & size)

void *shuffle[16];           // for chain reorganises, a block of void pointers

/********** file open flags for linux and Win32 ********************/

#ifdef __linux__
   HOP fd[] ={{".bin","r"},    {"_lst.txt","w"}, {"_msg.txt","w"},
              {"_dir.txt","r"},{"_cmt.txt","r"}, {"sad.ini","r"}  };

#else

   HOP fd[] ={{".bin","rb"},    {"_lst.txt","wt"}, {"_msg.txt","wt"},
              {"_dir.txt","rt"},{"_cmt.txt","rt"}, {"sad.ini","rt"}  };
#endif


/************************
 * chaining and admin stuff
 **************************/




int wnprt (const char *fmt, ...) 
{  // warning file prt
  va_list args;

  if (!fmt ) return 0;
  va_start (args, fmt);
  vfprintf (fldata.fl[2], fmt, args);
  va_end (args);
  return 1;             // useful for if statements
}

int wnprtn (const char *fmt, ...) 
{  // warning file prt with newline
  va_list args;

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[2], fmt, args);
     va_end (args);
	} 
  fprintf(fldata.fl[2], "\n");
  return 1;             // useful for if statements
}

int cmdaddr(int addr)
{
  // for printing of command addresses with or without bank 
 if (!numbanks) return (addr &0xffff);
 return addr;
}


int wflprt(const char *fmt, float fv)
 {           // separate float print, to control width
  char *tx;
  int cs;

  wnprt(fmt);
  cs = sprintf(nm+32,"%.3f",fv);
  tx = nm+31+cs;
  while (*tx == '0') tx--;
  if (*tx == '.') tx--;
  *(tx+1) = '\0';
  wnprt(nm+32);
  wnprt(" ");
 return tx-nm-32;
}



#ifdef XDBGX

void DBGPBK(LBK *b, const char *fmt, ...)
{       // debug print warn blk
  va_list args;

  if (!b) return;

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[2], fmt, args);
     va_end (args);
    }

  DBGPRT(" %s %05x %05x ",  dirs[b->fcom].com, b->start, b->end);
  va_end (args);
}


void DBGPBKN(LBK *b, const char *fmt, ...)
{
  va_list args;
  if (!b) return;
  va_start (args, fmt);
  DBGPBK(b,fmt,args);
  va_end (args);
  DBGPRTN(0);
}

#endif

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

  if (!fmt || !*fmt ) return;
  va_start (argptr, fmt);
  gcol += vfprintf (fldata.fl[1], fmt, argptr);
  va_end (argptr);
  if (gcol >= cposn[4]) wrap();
}


void paddr(int addr)
{
  // printer for address with or without bank. 
 if (numbanks) gcol += fprintf(fldata.fl[1],"%05x ", addr);
 else gcol += fprintf(fldata.fl[1],"%4x ", addr & 0xffff);
}






 // master exit and cleanup routines

 void closefiles(void)
{
  int i;

  for (i=0; i<5; i++)         // include sad.ini
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


void mfree(void *addr, size_t size)
 {
  if (!addr) return;     // safety

  #ifdef XDBGX
   DBGcuralloc -= size;    // keep track of mallocs
   DBGmrels++;
  #endif

  free (addr);
  addr = 0;
 }


void* mem(void *ptr, size_t osize, size_t nsize)
{  // use realloc, as is same as malloc with zero osize ptr
  void *ans;
  if (nsize < 1) wnprtn ("Invalid malloc size");
  
  #ifdef XDBGX
  if (ptr)  DBGcuralloc -= osize;
  DBGmcalls++;
  DBGcuralloc += nsize;
  if (DBGcuralloc > DBGmaxalloc) DBGmaxalloc = DBGcuralloc;
  #endif
  
  ans = realloc(ptr, nsize);

  if (!ans) wnprtn("Run out of malloc space");
  return ans;
}


short chk_cadt(CADT *a)
{

 int safe, safe2;
 CADT *b;

 safe = 0;

 while (a && safe < 50 )
  {
   b = a->next;
   safe2 = 0;
   while (b && safe2 < 50)
     {
      if (b == a) return 1;       //loop found
      b = b->next;                // next item to check
      safe2++;
     }
   if (safe2 >=50) return 2;
   a = a->next;
   safe++;
  }
 if (safe >=50) return 2;
 return 0;
 }

int freecadt(CADT **z)
 {
  CADT *n,*x;
  int a;

  x = *z;
  a = chk_cadt(x);
  if (a)
    {
	 #ifdef XDBGX
      DBGPRTN("CADT chain err %d",a);
	 #endif
     return 1;
    }

  while (x)
   {
    n = x->next;
    mfree (x, sizeof(CADT));
    x = n;
   }
 *z = NULL;
 return 0;
 }

int freesigc(CSIG **z)
 {

  CSIG *n, *x;

  x = *z;
  while (x)
   {
    n = x->next;
    mfree (x, sizeof(CSIG));
    x = n;
   }
 *z = NULL;
 return 0;
 }


int freehlds(HOLDS **z)
 {
	 // free any holds chain (SBK)
  HOLDS *n, *x;
  
  x = *z;
  while (x)
   {
    n = x->next;
    mfree (x, sizeof(HOLDS));
    x = n;
   }
 *z = NULL;
 return 0;
 }


void fsym(void *x) 
{            // free symbol
 SYT *s;
 int z;
 s = (SYT*) x;
 z = 0;
 if (s->name) z = strlen(s->name) + 1;    // string plus 0 byte
 if (z)  mfree (s->name, z);              // free symbol name
 s->name = NULL;
}

void fsub(void *x)
{             // free subr
  SXT *p;

  p = (SXT*) x;

  if (p->sdat)
   {
    freecadt (&(p->sdat->cadt));                   //cadt and sigs first
    freesigc (&(p->sdat->sigc));
    mfree    (p->sdat, sizeof(SXDT));              // then ptr stct
    p->sdat = NULL;
   }
 }

void fcmd(void *x)
{       // free cmd
  LBK *k;
  k = (LBK*) x;
  freecadt (&(k->adnl));
  }


void fscn(void *x) 
{      // free scan
  SBK *k;
  k = (SBK *) x;
//  mfree(k->pars, sizeof(SPARS));    // Pars array
//  k->pars = 0;
  freehlds(&k->hchn);           // HOLDS chain;  
}

void fblk(void *x) 
{           // malloced blocks tracker (free'd last)
  MBL *k;
  k = (MBL *) x;
  mfree(k->blk, k->size);
}

void clearchain(CHAIN *x)
{                          // clear pointer list contents. free any attached structures
 //  NB.  count DOWN so that for mblks first entry (which points to itself) is released last.

 int i;

 for (i = x->num-1; i >=0; i--)    
    {
     if (x->cfree)  x->cfree(x->ptrs[i]);
    }
    
 x->num = 0;
 x->DBGmax = 0;
 
 }


void freeptrs(CHAIN *x)
{
    // free the master pointer block of chain x (after clearchain does entries)
 if (x->pnum)
   {
    mfree(x->ptrs, x->pnum * sizeof(void*));
    x->ptrs = NULL;
    x->pnum = 0;
   }
}

void freechain(CHAIN *x)
{
  #ifdef XDBGX
    int size;
    size =  x->pnum * sizeof(void *);     // num ptrs of void*
    size += x->pnum * x->esize;          // + structs pointed to
    DBGPRTN("max=%4d tot=%4d allocs=%2d esize=%2d tsize=%d)", x->DBGmax, x->pnum, x->DBGallocs, x->esize, size);
  #endif

  clearchain(x);
  freeptrs(x);
}

void  free_structs ()
{                        // complete tidyup for closing or restart
 uint i;

 for (i = 0; i < sizeof(chlist)/sizeof(chlist[0]); i++)
  {
   freechain(chlist[i]);           // this includes mblock 
  }
   #ifdef XDBGX
   DBGPRTN ("max alloc = %d", DBGmaxalloc);
   DBGPRTN ("cur alloc = %d", DBGcuralloc);
   DBGPRTN ("mcalls = %d", DBGmcalls);
   DBGPRTN ("mrels  = %d", DBGmrels);
   DBGPRTN ("prog bar updates = %d", DBGxcalls);
   DBGPRTN ("watch reg updates = %d", DBGwtcnt);
   DBGPRTN ("max holds = %d", DBGmaxholds);
   #endif
   
   fflush (fldata.fl[1]);            // flush output files (safety)
   fflush (fldata.fl[2]);
}


void adpch(CHAIN *);    // declared for logblk  

void logblk(void *blk, size_t size)
{
 MBL *b;

 if (!blk) return;

 if (mblk.num >= mblk.pnum-1) adpch(&mblk);      // need more pointers     

 b = (MBL*) mblk.ptrs[mblk.num];  // no point in using chmem
 b->blk = blk;
 b->size = size;
 mblk.num ++;
 if (mblk.num > mblk.DBGmax) mblk.DBGmax = mblk.num;      // for debug really
}

 //----------------------------------------------------------------
 // add pointers for chains. malloced entries
 // pointer block is realloced, so is continuous...
 // but data blocks are NOT....
 //----------------------------------------------------------------

void adpch(CHAIN *x) 
 {   // add pointers for more chain pointers (malloced entries)
     // pointer block IS realloced, so IS continuous...
 int old, nw, msz,i;
 char *z, *n;
                       // grow pointer array - always as single block for arrays
    old = x->pnum;
    x->pnum += x->asize;
    x->asize += x->asize/4;      // add 25% for next alloc
    x->DBGallocs++;

    nw = x->pnum;            // new size
    x->ptrs = (void**) mem(x->ptrs, old*sizeof(void*), nw*sizeof(void*));

    // allocate new entries if not a remote chain, and keep them in mbl chain
    
    n = NULL;

    if (x->esize)
     {
      msz = x->esize*x->asize;           // size of new data block to go with ptrs
      n =  (char *) mem(0,0,msz);
      if (!n) wnprt("FATAL - MALLOC ERROR ");
     }

    z = n;
    for (i = old;  i < nw; i++)
     {
      x->ptrs[i] = z;
      z += x->esize;
     }
     logblk(n, msz); 
 
 }


void* chmem(CHAIN *x)
 {
    // get a free entry of the chain for search or insert.
  void *ptr;

  if (!x->esize) return NULL;         // can't have memory on a remote chain...

  if (x->num >= x->pnum-1) adpch(x);     // need more pointers

  ptr = x->ptrs[x->num];              // use next free block (at end of list)
  memset(ptr,0, x->esize);            // and clear it
  return ptr;
 }


void* schmem(void)
 {
    // get a free block for chain searches, from local reserved block.
  memset(schblk,0, sizeof(int)*30);           // clear block
  return schblk;
 }
 
 
void chinsert(CHAIN *x, int ix, void *blk)
{
  // insert new pointer (= block) at ix
  // relies on main pointer block being contiguous

 if (!blk) return ;

 if (x->num >= x->pnum-1) adpch(x);      // need more pointers

 // move entries up one from ix to make a space.
 memmove(x->ptrs+ix+1,x->ptrs+ix, sizeof(void*) * (x->num-ix));  // fastest for contiguous

 x->ptrs[ix] = blk;                         // insert new block at ix
 x->num ++;
 if (x->num > x->DBGmax) x->DBGmax = x->num;      // for debug really
}


void chrelease(CHAIN *x, int ix, int num)
{
 // release num block(s) from ix, shuffle down and restore ptr to top after num
 // relies on main pointer block being contiguous
 // shuffle array to hold pointers (max 16)
 int i;

 if (ix < 0 || ix >= x->num) return;  //// + num?
 
 if (x->cfree)
   {
    for (i = ix;  i < ix+num; i++) x->cfree(x->ptrs[i]);
   }
 
 memcpy(shuffle, x->ptrs+ix, sizeof(void*) * num);                           // save 'num' pointers

 // move entries down by 'num' from ix+num and then restore pointers to top of pointers, after (new) num.
 memmove(x->ptrs+ix,x->ptrs+ix+num, sizeof(void*) * (x->num-num-ix));  // fastest for contiguous
 memcpy(x->ptrs + x->num - num, shuffle, sizeof(void*) * num);
 x->num -= num;
}


int minadd(int addr)
{
 return bkmap[(addr >> 16) & 0xf].minpc;
}                                 // min address is bank|PCORG

int maxadd (int addr)
{
 return bkmap[(addr >> 16) & 0xf].maxpc;
}                                 // max address for this bank

int g_bank(int addr)
{
	return addr & 0xf0000;
}

int bankeq(int addr1, int addr2)
{
	// true if bank matches
	if ((addr1 &0xf0000) == (addr2 &0xf0000))   return 1;
	return 0;
}

int nobank(int addr)
{
	return addr & 0xffff;
}




bool val_code_addr(int addr)
 {
 // return 1 if valid 0 if invalid.
 // CODE addresses - no data fixup
 
 int bank;
 BANK *b;

 bank = (addr >> 16) & 0xf;        // NOT dbank call
 b = bkmap+bank;

 if (!b->bok) return 0;           // invalid

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // now where fill starts

 return 1;        // within valid range, OK
 }

int dbank(int addr)
{
  // get data bank. has below PCORG fixup  
 int bank;
 
 if (nobank(addr) < PCORG) return BMAX;  // register and ram fixup.
 bank = (addr >> 16) & 0xf;
 return bank;                             // and return as an int 0-16
}


bool val_jmp_addr(int addr)
 {
 // return 1 if valid 0 if invalid.
 // JUMP addresses

 // allows some extra adds as OK (for console)
 
 int x;
 BANK *b;

 x = (addr >> 16) & 0xf;        // NOT dbank call
 b = bkmap+x;

 if (addr >= 0x8d000 && addr <= 0x8d010) return 1; // cal console
 if (addr == 0x1f1c) return 1; // DUCE chip ??
 
 if (!b->bok) return 0;           // invalid

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // now where fill starts

 return 1;        // within valid range, OK
 }


bool val_data_addr(int addr)

{
 // return 1 if valid 0 if invalid.
 // DATA addresses

 int bank;
 BANK *b;

 bank = dbank(addr);
 b = bkmap+bank;                 // handles < PCORG

 if (!b->bok) return 0;          // invalid

 if (addr <= b->minpc) return 0;  // PCORG (and zero) is invalid
 if (addr > b->maxpc) return 0;

 return 1;        // within valid range, OK
 }


uchar* map_addr(int addr)
 {
  // only used by g_word, g_byte, g_val.  Allow maxbk as limit (so fill can be read)

  int bank;
  BANK *b;

  // speedup check to save a redundant remap ?

  if (nobank(addr) < PCORG) return bkmap[BMAX].fbuf;            //not in ROM
  
  if (addr >= lastbmap->minpc && addr <= lastbmap->maxbk) return lastbmap->fbuf;

  bank = (addr >> 16) & 0xf;
  b = bkmap+bank;

  if (!b->bok || addr > b->maxbk) return NULL;  // invalid

  lastbmap = b;
  return b->fbuf;         // valid address, return pointer

 }

int icaddrse(int *start, int *end)
{                                      // for start and end checks of COMMANDS - not below PCORG
 int addr;

 if (!g_bank(*end)) *end |= g_bank(*start);     // use bank from start addr

 if (!bankeq(*end, *start))
    {
	  #ifdef XDBGX	
      DBGPRTN ("Invalid - diff banks %05x - %05x", *start, *end);
      #endif
      return 1;
    }


 if (val_code_addr(*start) && val_code_addr(*end))
    {
     addr = maxadd(*start);
     if (*end > addr)   *end = addr;
     if (*end < *start) *end = *start;
     return 0;
    }
 return 1;
}


//----------------------------------------------------------


int opbank(int xbank)
 {
   // simple way to get BANK opcode to override default  
  if (curinst->bnk) return curinst->bank <<16;
 return xbank;
 }


//---------------------------------------------------------------------------------------/

// compare (cp)  procs used by bfindix for binary chop chain searches
// NB.  straight subtract works nicely for chain ordering, even for pointers!
// < 0 less than, 0 equals, > 0 greater than  

int cpfjp (int ix, void *d)
{
    // 'from' jump chain
  int ans;
  JLT *j, *t;
 
  if (ix >= jpfch.num) return -1; 
  
  j = (JLT*)jpfch.ptrs[ix];
  t = (JLT*) d;

  ans = j->from - t->from;

  if (ans) return ans;

  if (t->jtype) ans = j->jtype - t->jtype;

  return ans;
}


int cptjp (int ix, void *d)
{
  // 'to' jump chain  
  int ans;
  JLT *j, *t;
  
  if (ix >= jptch.num) return -1;
  
  j = (JLT*)jptch.ptrs[ix];
  t = (JLT*) d;

  ans = j->to - t->to;
  
  if (ans) return ans;

  if (t->jtype) ans = j->jtype - t->jtype;

  return ans;
}


int cpsym (int ix, void *d)
{
 SYT *s, *t;
 int ans;
 if (ix >= symch.num) return -1;

 s = (SYT*) symch.ptrs[ix];
 t = (SYT*) d;

 ans = s->add - t->add;
 if (ans) return ans;
 
 // address check here ?

 ans = s->bitno - t->bitno;
 if (ans) return ans;

 ans = s->writ - t->writ;
 if (ans) return ans;

 return s->end-t->start;

}


int cpbase (int ix, void *d)
{
 RBT *b, *t;
 int ans;
 
 if (ix >= basech.num) return -1;
 b = (RBT*)basech.ptrs[ix];
 t = (RBT*) d;
 
  ans = b->reg - t->reg;
  if (ans) return ans;

  ans = b->end - t->start;
  return ans;
}


int cpsig (int ix, void *d)
{
 SIG *g, *t;
 int ans;

 if (ix >= sigch.num) return -1;
 g = (SIG*) sigch.ptrs[ix];
 t = (SIG*) d;
 ans =  g->ofst - t->ofst;
 if (ans) return ans;

 if (t->ptn) ans = g->ptn - t->ptn;
 return ans;
}


int cpddt (int ix, void *d)
{
 DDT *g, *t;
 int ans;

 if (ix >= datch.num) return -1;

 g = (DDT*) datch.ptrs[ix];
 t = (DDT*) d;
 ans = g->start - t->start;

// if (ans) return ans;
// if (t->from) ans = g->from - t->from;   // disable for now.
 
 return ans;
}



int cpscan (int ix, void *d)
{
  // need to check FROM (retaddr) so that alternative scan params (i.e. tables etc) get picked
  // blk is what you would insert (or find match to)

 SBK *s, *t;
 int ans;

 if (ix >= scanch.num) return -1;

 s = (SBK*) scanch.ptrs[ix];
 t = (SBK*) d;
 
 ans = s->start - t->start;
 if (ans) return ans;


// if start matches and cksum is same, can skip

 if (t->cksum == s->cksum)  return 0;        // shortcut

 if (t->retaddr)  ans = s->retaddr - t->retaddr;        // return address
 if (ans) return ans;

 if (t->sub)  ans = s->sub - t->sub;               // and subr

 return ans;
 
} 


int cpsub (int ix, void *d)
{
 // add a check here for pushp subrs.
 SXT *s, *t;

 if (ix >= subch.num) return -1;

 s = (SXT*) subch.ptrs[ix];
 t = (SXT*) d;

 if (s->psp && t->addr == s->addr+1)  return 0;     // pushp match

 return s->addr - t->addr;
}



int cpcmd (int ix, void *d)
{
 // this compare checks END address, not start
 // so that if d.start > [ix].end then next search candidate
 // becomes ix+1 (and might be the matching block)	
 	
 LBK *b, *t; 
 int ans;
 
 if (ix >= cmdch.num) return -1;
 b = (LBK*) cmdch.ptrs[ix];
 t = (LBK*) d;

 ans = b->end - t->start;

 return ans;
 
}

int cpacmd (int ix, void *d)
{
	// check END address, as per cpcmd
 LBK *b, *t;
 int ans;
 
 if (ix >= auxch.num) return -1;
 b = (LBK*) auxch.ptrs[ix];
 t = (LBK*) d;
 
 ans = b->end - t->start;

 return ans;
 
}

int eqbase (int ix, void *d)
{
 RBT *b, *t;
 int ans;
 
 if (ix >= basech.num) return -1;
 b = (RBT*)basech.ptrs[ix];
 t = (RBT*) d;
 
 ans = b->reg - t->reg;
 if (ans) return ans;

 if (t->start >= b->start && t->start <= b->end ) return 0;

 return b->end-t->start;
}

int eqsym (int ix, void *d)
{
 SYT *s, *t;
 int ans;
 if (ix >= symch.num) return -1;

 s = (SYT*) symch.ptrs[ix];
 t = (SYT*) d;

 ans = s->add - t->add;
 if (ans) return ans;
 
 ans = s->bitno - t->bitno;
 if (ans) return ans;

 ans = s->writ - t->writ;
 if (ans) return ans;

 if (t->start >= s->start && t->start <= s->end ) return 0;
  
 return s->end-t->start;

}

int eqcmd (int ix, void *d)
{
 // compare checks END address, not start
 // so that end is always less than start
 // in previous block (= ix-1)
 // so this block is right candidate to check
 
 LBK *b, *t;

 if (ix >= cmdch.num) return -1;

 b = (LBK*) cmdch.ptrs[ix];
 t = (LBK*) d;
 
 // t->start will always > b->end ??

 if (t->start >= b->start && t->start <= b->end )
    {
     if (!t->fcom || t->fcom == b->fcom) return 0;
    }
 return 1;
}

int eqacmd (int ix,  void *d)
{
 LBK *b, *t;

 if (ix >= auxch.num) return -1;

 b = (LBK*) auxch.ptrs[ix];
 t = (LBK*) d;
 
 if (t->start >= b->start && t->start <= b->end )
    {
     if (!t->fcom || t->fcom == b->fcom) return 0;
    }
 return 1;
}


int bfindix(CHAIN *x, void *blk)
{
  // generic binary chop subr
  // send back INDEX value to allow access to adjacent blocks.
  // use a compare subr by chain, so candidate block is void *
  // answers so that min-1 is always smaller than blk,
  // therefore min is blk match or larger.

  int mid, min, max;

  min = 0;
  max = x->num;

  while (min < max)
    {
      mid = min + ((max - min) / 2);      // safest way to do midpoint (imin, imax);
      if(x->comp(mid,blk) < 0)            // blk < mid, so this becomes new 'bottom' (min)
         min = mid + 1; else  max = mid;  // else mid is new 'top' (max)
    }
  return min;
}


// special checking for LBK commands

int olchk(CHAIN *x, int i, int ix)
 {

   /***** overlap and overwrite check if - commands can be merged/overwrite etc
   * b is 'master' (new) block at ix , t is existing (test) block at i
   * set answer as bit mask -
     L   overlaps - this is flag then used to do something
     S   block is spanned - t within blk
     W   overwrite (command) allowed
     V   overwrite (other way) allowed
     C   contains - blk within t (reverse of spanned)
     M   merge allowed
     Q   equal
   ************/

   LBK *t, *b;
   int ans;
   
   if (i < 0 || ix < 0) return 0;
   if (i > x->num || ix > x->num) return 0;

   t = (LBK*) x->ptrs[i];
   b = (LBK*) x->ptrs[ix];     // block just inserted

   ans = 0;   //clear all flags;

   if (t == b) return 0;                     // safety

   if (t->start == b->start && t->end == b->end && t->fcom == b->fcom) return Q;                      // deemed equal


// now check for overlaps

   if (b->start >= t->start && b->end <= t->end)  {ans |= L; ans |= C;}  // contains
   else
   if (b->start < t->start && b->end > t->end)  {ans |= L; ans |= S;}  // spans
   else
   if (t->start >= b->start && t->start <= b->end)  ans |= L;        // overlap (start)
   if (t->end   >= b->start && t->end   <= b->end)  ans |= L;        //overlap (end)

   if (ans & L)
    {
     if (!t->cmd && (dirs[b->fcom].ovrde & (1 << t->fcom))) ans |= W;  // if overwrite flag set
     if (!b->cmd && (dirs[t->fcom].ovrde & (1 << b->fcom))) ans |= V;
    }

   if (dirs[t->fcom].merge && t->fcom == b->fcom)
    {
      // check there are no cadt extras first.
      if (!t->adnl && !b->adnl)  
       {
        if (ans & L) ans |= M;          // overlap set, merge allowed
        if (b->end == t->start-1 || b->start == t->end+1) {ans |= (L|M);}       //adjacent
       } 
    }

 return ans;
}

void merge(LBK *t, LBK *s)
 { // merge two blocks, t = s update BOTH
  if (s->start < t->start) t->start = s->start;
  if (s->end   > t->end)   t->end   = s->end;

  if (t->start < s->start) s->start = t->start;
  if (t->end   > s->end)   s->end   = t->end;
 }

int split(LBK *m, LBK *s)
{
  // m is kept original and s is trimmed
  // s is either higher or lower, trim as reqd

  if (s->start >= m->start && s->start <= m->end)
    {
      s->start = m->end+1;
      return 1;
    } 
  if (s->end  >= m->start && s->end   <= m->end)
    {
     s->end   = m->start-1;

    }
  return 0;  
}

#ifdef XDBGX
void DBGPRTFGS(int x)
{
    // temp for testing
  if (x & U) DBGPRT ("U");
  if (x & D) DBGPRT ("D");
  if (x & L) DBGPRT ("L");
  if (x & C) DBGPRT ("C");
  if (x & M) DBGPRT ("M");
  if (x & Q) DBGPRT ("Q");
  if (x & S) DBGPRT ("S");
  if (x & W) DBGPRT ("W");
  if (x & V) DBGPRT ("V");
  DBGPRT(" ");
}
 
#endif


int inschk(CHAIN *x, int *ix, LBK *b)
{

   // check if insert is allowed, fix any overlaps later.
   // b is insert candidate,   ix is where it would go
   // so [ix-1].end should always be less than b->start

  LBK *l, *n;               // last, block for insert, next
  
  int chkn, chkl, min, max, ans;           // TEST


  // for olchk by pointer x->num is insert candidate....

  ans = 1;            // insert set

  min = (*ix)-1;
  
  //check upwards FIRST, backwards should only ever have an adjacent ?   
  

  max = *ix;            //block returned from compare(find)
  chkn = 1;
  
  while (chkn)
    {
     chkn = olchk(x, max, x->num);
     if  (!chkn) break;
  
     n = (LBK*) x->ptrs[max];   
     
     if (chkn & Q)                          // chkl won't be a Q
      {
	   #ifdef XDBGX  
         DBGPBKN(n,"Dupl");
	   #endif
       ans = 0;                           // exact match, don't insert
      }
     else
     if (chkn & L)
      {
      // overlaps with next block
       if (chkn & C)
         {
         // b already contained in l - do nothing
		  #ifdef XDBGX  
           DBGPBK(b,"Ins Chk N");
           DBGPRTFGS(chkn);
           DBGPBKN(n,"contained in");
		  #endif 
          ans = 0;
         }
       else
       if (chkn & M)
        {
         // merge allowed
		 #ifdef XDBGX  
         DBGPBK(b,"Ins Chk N");
         DBGPRTFGS(chkn);
         DBGPBKN(n,"Merge with");
		 #endif
         merge(n,b);
         ans = 0;
        }
       else
        if (chkn & W)
        {
         // overwrite allowed
         //  DBGPBK(b,"Ins Chk N");
         //  DBGPRTFGS(chkn);
         //  DBGPBKN(n,"Split with");
         split(b,n);
         (*ix)++;                //  but insert now one up
         // and can still insert
        }
       else
        {
          // default catch
          // DBGPBK(b,"Ins Chk N");
          // DBGPRTFGS(chkn);
          // DBGPBKN (n, "REJECTn");
          ans = 0;           // temp stop insert
        } 
      }
    
     max++;
     if (!ans) return ans;
    }

  if (! ans) return ans;
  
  chkl = 1;
  while (chkl)
   {  
     chkl = olchk(x, min, x->num);
     if  (!chkl) break;
      
     l = (LBK*) x->ptrs[min];   
     if (chkl & L)
        {
         // overlaps with last block
          if (chkl & M)
          {
           // merge allowed 
           //  DBGPBK(b,"Ins Chk L");
           //  DBGPRTFGS(chkl);
           //  DBGPBKN(l,"Merge with");
           merge(l,b);
           ans = 0;
          }
        else
          {
            ans = 0;
           //  DBGPBK(b,"Ins Chk L");
           //  DBGPRTFGS(chkl);
           //  DBGPBKN(l, "REJECT l!");
          }
        }
     min--;    // loop back for all overlaps
    }




 return ans;                    // insert 
}



LBK* inscmd(CHAIN *x, LBK *blk)
 {
   /* change to insert THEN fixup ?

   
    * at this call 'blk is always x->ptrs[x->num] ....
    * do a simple check,   THEN do more complex merging as necessary

   this works but does not merge anything yet.  THis assumes you never get an END overlap ?
   probably true for current scan setup, but not safe.....

   eqcmd only checks that start falls between start&end of chained block....

   NB.   Probably don't care about overlaps right now, can do later, but must capture
   any extended END markers.

   so have to check for exact match, and non_allowed overlaps/overrides ?

    */

  int ans, ix;      //, s;
   
  ans = 1;

  ix = bfindix(x, blk);                 // where to insert (from cpcmd)

  ans = inschk(x,&ix,blk);              // found - is insert allowed and can mod ix

  if (ans)
      {
       chinsert(x, ix, blk);            // do insert at i
       return blk;
      } 
  return (LBK*) x->ptrs[ix];             // duplicate

}

// match for signature blocks

SIG* inssig(SIG *blk)
 {

  int ans, ix;
  SIG *s;
  ans = 1;                  // insert by default

  ix = bfindix(&sigch, blk);

  ans = cpsig(ix,blk);

  if (!ans) return 0;             // straight duplicate
  
// check for duplicates with different skips at front...
// and check for overlap with next sig - but sign sig may be OK ?
// may need better check here for overlaps
  
  if (ix < sigch.num)
   {
    s = (SIG*) sigch.ptrs[ix];
   
   if (s->ptn == blk->ptn && s->ofst+s->skp == blk->ofst+blk->skp)
     {      // duplicate, only skips differ...
       return 0;
     }
   }


  if (ix > 0)
  {

   s = (SIG*) sigch.ptrs[ix-1];
   
   if (s->ptn == blk->ptn && s->ofst+s->skp == blk->ofst+blk->skp)
     {      // duplicate, only skips differ...
       return 0;
     }


   if (s->ofst <= blk->ofst && s->xend > blk->ofst)
     {
	  #ifdef XDBGX  
      DBGPRT("SIG OLP %05x %s with %05x ", blk->ofst, blk->ptn->name, s->ofst);
	  #endif
      if (!s->ptn->olap) return 0;
     }
     
  }

  chinsert(&sigch, ix, blk);      // do insert
  return blk;

}



DDT* add_ddt(int addr, int size)
 {
  DDT *x;
  int ans,ix;

  if (anlpass >= ANLPRT)  return 0;

  ans = nobank(addr);
  if (ans < PCORG)  addr = ans;     // strip bank for < PCORG
  if (addr <= maxwreg) return NULL;

  if (!val_data_addr(addr))  return NULL;

  x = (DDT *) chmem(&datch);      // next free block
  x->start  = addr;

  ix = bfindix(&datch, x);
  ans = cpddt(ix, x);

  if (ans)
    {          // no match

     x->from = curinst->ofst;
     x->imd = curinst->imd;
     if( curinst->inx)
       { if (curops[1].addr) x->inx = 1; else x->imd = 1; } 
     x->inr = curinst->inr;

     x->inc = curinst->inc;
     x->ssize = size;             // will not override tab or func....
     chinsert(&datch, ix, x);
     return x;
    }
 
   // Match - return ZERO
 return 0;             //(DDT *) datch.ptrs[ix];
}

// used in lots of places....general chain finder

void *find(CHAIN *x, void *b)
{
 // find item with optional extra pars in b

 int ix;
 ix = bfindix(x, b);
 if (x->equal(ix,b)) return NULL;    // no match
 return x->ptrs[ix];

}


int g_byte (int addr)
{                    // addr is  encoded bank | addr
  uchar *b;

  b = map_addr(addr);
  if (!b) return 0;        // safety

  return (int) b[nobank(addr)];
}


int g_word (int addr)
{         // addr is  encoded bank | addr
  int val;
  uchar *b;

  b = map_addr(addr);                      // map to required bank
  if (!b)  return 0;

  addr = nobank(addr);

  val =  b[addr+1];
  val =  val << 8;
  val |= b[addr];
  return val;

 }

/*************************************
Multiple scale generic get value.
scale  1=byte, 2=word, 3=long. unsigned
       5=byte, 6=word, 7=long. signed
Auto negates via bit mask if required
***********************************/

int g_val (int addr, int size)
{                                           // addr is  encoded bank | addr
  int val, mask;                            // scale is -ve for no sign
  uchar *b;

  b = map_addr(addr);

  if (!b)  return 0;                        // map to required bank

  addr = nobank(addr);

  val = 0;
  mask = 0;

    switch(size)
     {

      case 5:                    //signed byte
        mask = 0x80;
      case 1 :                  // uns byte

       val = b[addr];
       break;

      case 6:                  // word
        mask = 0x8000;
      case 2:
        val =  b[addr+1];
        val =  val << 8;
        val |= b[addr];
        break;


      case 7:                    //int, or long in 806x terms
      //  mask = 0x8000;        // not reqd !!
      case 3:
        val =  b[addr+3];
        val =  val << 8;
        val |= b[addr+2];
        val =  val << 8;

        val |= b[addr+1];
        val =  val << 8;
        val |= b[addr];
        break;

      default:
        val = 0;
        mask = 0;
		#ifdef XDBGX
         DBGPRTN ("g_val inv size %d", size);
		#endif
        break;
    }

   if (mask & val) val |= ~(mask - 1);

  return val;

}


int negval (int val, int mask)
{  // negate value according to sign bit position - used for some jumps

  if (mask & val) val |= ~(mask - 1);        // negate if sign bit set
  return val;
}


int find_ramlist(int addr)
 {
  int i,ans;

  // does register exist in list ?
  // i.e. was it recently updated ?
  // for signed funcs and tabs. Returns size

  ans = 0;        // not found
  for (i = 0; i < NPARS; i++)
     if ((ramlist[i] & 0x1fff) == addr) 
       { 
        if (ramlist[i] & 0x8000) ans = 2;
        else ans = 1;      // found 
        break;
       }
return ans;
 
 }


void upd_ramlist(int addr, int size)
 {
  int i,j;

  // straight memory move down list with check for duplicate.
  // [0] = newest entry

  if (addr <= minwreg || addr >= PCORG) return;       // only keep from general regs to PCORG
  size &=3;                                         // no longs (yet)  

  j = NPARS-1;                                      // move 15 entries if not found

  for (i = 0; i < NPARS-1; i++)
     if ((ramlist[i] & 0x1fff) == addr) 
       {                                            // no point checking last entry....
        j = i; 
        break;
       }

    memmove (ramlist+1, ramlist, j * sizeof(ushort));
    ramlist[0] = addr;
    ramlist[0] |= (size << 14);
 }




/*
RBT *ins_rbase(RBT *x)
 {
	int ix, ans; 
	 RBT *z;
     ix = bfindix(&basech, x);
  //  ans = cpbase(ix, x);

    // check for overlap here - this becomes an ins_rbase ?
	ans = 1;                 // insert OK
	
 //  if (ix < basech.num)
  //  { 	
	 z = (RBT *) basech.ptrs[ix];         // chain block found
	// if (z->start && z->end < 0xfffff)
    //   {    // z is address block with range
    //     if (x->start) 
	//   	  {             // both addresses - check overlap
	//       if (x->start >= z->start && x->start <=z->end)  ans = 0;  // overlap
   //        if (x->end   >= z->start && x->end   <=z->end)  ans = 0;  // overlap    
	//      }
  //     else
	  //   {  // x is a default (no addresses) put default in 
		 
		 
 //   if (z->start)
 //   { 
		
     if (ix < basech.num)
    { 
     z = (RBT *) basech.ptrs[ix];         // next block
     if (x->end >= z->start)  ans = 0;
	}

 if (ix > 0) 
    {
      z = (RBT *) basech.ptrs[ix-1];      // previous block
      if (x->start <= z->end)  ans = 0;   // overlap
    } 
//	}
	
  if (ans)
   {
	 #ifdef XDBGX  
     DBGPRTN(" OK");
     #endif

    chinsert(&basech, ix, x);
    return x;
   }
	 #ifdef XDBGX  
     DBGPRTN(" DUP or OLAP");
     #endif
    return 0;   // duplicate

}
*/


RBT *add_rbase(int reg, int val, int start, int end)
{
  RBT *x, *z;     // val includes bank
  int ix, ans;

  if (reg & 1) return 0;          // can't have odd ones
  if (reg <= stkreg1) return 0;   // can't have special regs or stack
  if (val <= maxwreg) return 0;   // can't have register to register adds

  if (val < 0xffff && val > PCORG) val |= opbank(datbank);     // safety check for imd values

  if (!val_data_addr(val)) return 0;
 
  x = (RBT *) chmem(&basech);
  x->reg   = reg;
  x->val   = val;
  x->start = start;
  x->end   = end;

  #ifdef XDBGX  
  DBGPRT("add rbase R%x = %05x", reg, val);
  if (x->start) DBGPRT("(%05x - %05x)", x->start, x->end);
  #endif

     ix = bfindix(&basech, x);
     ans = eqbase(ix, x);

    // check for overlap here - this becomes an ins_rbase ?
   // must check x->end does not overlap next block....
   // start MUST be > than end of previous block...

	
    if (ix < basech.num)
     { 	
	 z = (RBT *) basech.ptrs[ix];         // chain block found

     if (x->reg == z->reg) 
	  { 
		  if (x->end > z->start) ans = 0;

	  }
	 }

 
	

  if (ans)
   {
	 #ifdef XDBGX  
     DBGPRTN(" OK");
     #endif

    chinsert(&basech, ix, x);
    return x;
   }
	 #ifdef XDBGX  
     DBGPRTN(" DUP or OLAP");
     #endif
    return 0;   // duplicate
	


}



int upd_ram(int add, uint val, int size)
 {

  add = nobank(add);
  if (add <= 0 )  return 0;

   switch(size)
    {

     case 1 :
     case 5 :
        if (add > PCORG) return 0;

        ibuf[add] = (val & 0xff);
        break;

     case 6:
     case 2:
        if (add > (PCORG-1)) return 0;

        ibuf[add] = val & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        break;

     case 7:
     case 3:
        if (add > (PCORG-3)) return 0;
        ibuf[add] = val & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        break;

     default:
	   #ifdef XDBGX  
        DBGPRTN ("Upd_reg inv size %d", size);
	   #endif
       return 0;
    }
   return 1;        // updated OK
 }

RBT* get_rbt(int reg, int pc)
{
  RBT *r, *x;
	
  r = (RBT*) schmem();
  r->reg = reg;
  r->start = pc;  
  x = (RBT*) find(&basech, r);

// if not found, look for default (add 0).....

  if (!x && pc)
    { 
     r->start = 0;
     x = (RBT*) find(&basech, r);
	} 
	
  if (x && x->inv) x = 0; 
 
return x; 
  
  
}


void upd_watch(SBK *s)
 {
  OPS *o;
  int w ;
  RBT *b;
  DDT *d;

if (anlpass >= ANLPRT) return;       // don't do in print phase !!

  w = opctbl[curinst->opcix].wop;
  o = curinst->opr + w;                    // destination entry

  if (o->rbs && o->rbs->cmd)
     {
	  #ifdef XDBGX  
       DBGPRTN ("Rbase bans update %x", o->addr);
	  #endif
      return;
     }

   if (w)
     {
 	  b = get_rbt(o->addr,0);
	   // only add in first place if ldx or stw and not zero
      if (!b)
	   {  
		if(curinst->imd && opctbl[curinst->opcix].sigix >= 11 && opctbl[curinst->opcix].sigix <= 13 && o->val) b = add_rbase(o->addr, o->val,0,0);
	   }	
      else 
       {
         if (!b->cmd)
           {
            if (b->cnt < 15) b->cnt++;             // extra trick !!
            if (b->val != o->val)
             {
               b->inv = 1;      // not same value
			   #ifdef XDBGX
               DBGPRTN ("Inv rbase (V) %x", b->reg);
			   #endif
             }  
            if (!curinst->imd)
               {
                b->inv = 1;    // not immediate value - this seems to work
				#ifdef XDBGX  
                DBGPRTN ("Inv rbase (!I) %x", b->reg);
				#endif
               }   
           }
       }

     if (upd_ram(o->addr,o->val,o->wsize))  upd_ramlist(o->addr,o->wsize);
     #ifdef XDBGX  
      if (anlpass < ANLPRT) DBGPRTN( "%05x RAM %x = %x (%d)",s->curaddr,  o->addr, o->val, o->wsize);
     #endif
    }

// check for autoinc (can be on the read register....always [1])


  o = curinst->opr+1;

  if (w && curinst->inc)
    {           // autoinc set - update value, check for subr params
     
     //check for DDT here
       d = (DDT*) chmem(&datch);
       d->start = o->addr;
       d->from = curinst->ofst;
        
       d = (DDT*) find(&datch,d);
       if (d) d->inc =1; 
 
    } 

//   wtcnt++;

 // return o->addr;
}



//------------------------------------------------------------------------

void set_opstart (int ofst)
{
  short bit;
  uint *z;
  BANK *x;
  int bank;

  bank = (ofst >> 16) & 0xf;
  ofst = nobank(ofst);

  x = bkmap+bank;
  z = x->opbt;
  ofst -= PCORG;
  bit = ofst & 0x1F;       // 32 bits
  ofst = ofst / 32;
  z[ofst] |= (1 << bit);
}

int get_opstart (int ofst)
{
  short bit;
  int bank;
  BANK *x;
  uint *z;

  bank = ofst >> 16;
  x = bkmap+bank;
  z = x->opbt;
  ofst = nobank(ofst);
  ofst -= PCORG;
  bit = ofst & 0x1F;
  ofst = ofst / 32;

  return z[ofst] & (1 << bit);
}

int get_prev_opstart (int ofst)

{
  short bit;
  int bank,xofst;
  BANK *b;
  uint *z;

  bank = (ofst >> 16) & 0xf;
  b = bkmap + bank;
  z = b->opbt;
  xofst = nobank(ofst);
  xofst -= PCORG;

  while (xofst-- > 0)
   {
    bit = xofst & 0x1F;
    bank = xofst / 32;
    if (z[bank] & (1 << bit)) break;
   }

  xofst += PCORG;
  xofst |= g_bank(ofst);  // bank
  return xofst;

}


int outside (JLT* b, JLT *tst)
 {
	 // b is base, tst is test

   if (b == tst)        return 0;      // don't compare same jump... 
   if (!tst->jtype)     return 0;      // ignore returns
   if ( tst->jtype > 3) return 0;      // ignore subr jumps

   
// outside jumps in	 
   if (tst->to > b->from && tst->to < b->to)
     { 
	  if (tst->from < b->from)  return 1;
      if (tst->from > b->to) return 1;
     }

// inside jumps out

  if (tst->from > b->from && tst->from < b->to)
     { 
	  if (tst->to < b->from)  return 2;
      if (tst->to > b->to) return 2;
     }

   return 0;
   
   
 }

void do_jumps (void)
{

/******* jumps checklist ***************
 set conditions flags for various things
 whether jumps overlap, multiple end points etc etc.
*****************************************/

 int ix, zx, a;    // have to use indexes here as interleaved scan

 JLT *f, *t;

for (ix = 0; ix < jpfch.num-1; ix++)
    {
     f = (JLT*) jpfch.ptrs[ix];                  // down full list of forward jumps
     if (f->jtype == S_CJMP && !f->retn)
        {                          // only for conditional jumps (as base)
          // check any jumps IN, from outside of f
          zx = ix+1;                                  // next jump
          t = (JLT*) jpfch.ptrs[zx]; 
          while (t->from <= f->to && zx < jpfch.num)
            {
 	         a = outside(f, t);
             if (a == 2) f->uci = 1;     // jumps outside in 
             if (a == 1) f->uco = 1;     // jumps inside to out
             zx++;
             t = (JLT*) jpfch.ptrs[zx];
            }
        }

     if (!f->back && !f->uci && !f->uco && f->jtype == S_CJMP) f->cbkt = 1;  	// conditional, forwards, clean.
    }
	
}

#ifdef XDBGX
void DBG_CSIG(CSIG *s)
 {                // prt single csig
  if (!s) return;
  DBGPRT  (s->sig->ptn->name);
  if (s->disb) DBGPRT (" !");
  DBGPRT  (" L1=%d", s->lev1);
  if (s->l1d) DBGPRT ("*");
  DBGPRT  (" L2=%d", s->lev2);
  if (s->l2d) DBGPRT ("*");
  DBGPRT  (" P=%x",  s->par1);
  DBGPRT  (" D=%x",  s->dat1);
  DBGPRT  (" A=%05x",s->caddr);
 }
#endif



void prt_csig_list(CSIG *x)
 {                   // special funcs for subrs - COMMAND output
  short levs;        // safety count
  CSIG *s;

// check for manual sig in here....for the F: option, and print it as a CADT fake....

  levs = 0;
  if (!x) return;
  
  s = x;

  while (s && levs < MAXLEVS)
   {
    if (s->sig->ptn == &tblp || s->sig->ptn == &tblx) 
    {
      wnprt(" : F 2");
      wnprt (" %x %x", s->sig->v[4],s->sig->v[3]);
      if (s->sig->v[2] < 2) wnprt (" Y");  else wnprt(" W");
	  if (s->par1 & 1) wnprt (" S");  else wnprt(" U");
    }  

    if (s->sig->ptn == &fnlp) 
    {
     wnprt(" : F 1");
     wnprt (" %x", s->sig->v[4]);
     if (s->sig->v[2] == 2) wnprt (" Y");  else wnprt(" W");
     if (s->par1 & 1)    wnprt (" S");  else wnprt(" U");
     if (s->par1 & 0x10) wnprt (" S");  else wnprt(" U");
    }

    if (s == s->next) {
		 #ifdef XDBGX  
         DBGPRT(" __CSIG LOOP__");
		 #endif
        break; }         // safety
    s = s->next;
    levs++;
   }
   
   //   DEBUG only - add all sigs
 #ifdef XDBGX  
  levs = 0;
   s = x; 
  while (s && levs < MAXLEVS)
   {
	DBGPRT(" :");
    DBG_CSIG(s);
	
    if (s == s->next) {
         DBGPRT(" __CSIG LOOP__");
        break; }         // safety
    s = s->next;
    levs++;
   }
 #endif  
   
   
 }


int dsize(CADT *s)
{            // size of a single CADT entry
 if (!s) return 0;
 if (s->disb) return 0;        // disabled entry
 return s->cnt * casz[s->ssize];
}


void prt_cadt(CADT *s)
{                  // additional params in LBK (commands) or SXT  (subs)

ushort flg, levs;

levs = 0;

while (s && levs < MAXLEVS)
  {
   if (!s->nodis && !s->disb)
   {
   if (s->newl) wnprt("| "); else wnprt(": ");
   if (s->ssize > 4) wnprt("S ");
   flg = s->ssize & 3;

    switch (flg)
     {
      case 1:
        wnprt("Y ");
        break;
      case 2:
        if (s->enc) wnprt("E %d %x ", s->enc, s->addr);
        else        wnprt("W ");
        break;
      case 3:
        wnprt("L ");
        break;
      default:
        break;
        
     }
    

  // print fieldwidth if not zero and not equal to default
  
   if (s->pfw)
    {
      if (s->pfw != cafw[s->ssize])  wnprt("P %x ",s->pfw);
    }

   if (s->dcm) wnprt("X ");
   if (s->cnt > 1) wnprt("O %d ", s->cnt);
   if (s->foff) wnprt("D %x ", s->addr);
   if (s->divd) wflprt("V ", s->divd);
   if (s->vect) wnprt("R ");
   if (s->name) wnprt("N ");
 //  if (s->disb) wnprt("! ");
    #ifdef XDBGX  
    DBGPRT ("[A%x D%x] ", s->addr, s->dreg);
	#endif
 //  }
   
   if (s == s->next)
      {
       #ifdef XDBGX  
	   DBGPRT("__LOOP__");
	   #endif
        return;
      }        // safety
   
}
   s = s->next;
   levs++;
  }
}

void prt_bnks(void)
{
int i;
BANK *x;
for (i = 0; i < BMAX; i++)
 {
  x = bkmap+i;

  if (x->bok)
    {
     wnprt("bank %d  %5x  %5x", i, x->filstrt, x->filend);
	 #ifdef XDBGX  
    DBGPRT("  # %05x fill %05x-%05x, type %d",x->maxpc,x->minpc,x->maxbk,x->btype);
	#endif
     wnprtn(0);
    }
 }
   wnprtn(0);
}

void prtflgs(int flgs)
{

 int fl;
 short i;

 fl = flgs;

 for (i = 0; i < 26; i++)
   {
   if (!fl) break;
   if (fl & 1) wnprt("%c ", i+'A');
   fl >>=1;
  }

 }

void prt_opts(void)
{
// print opt flags and banks
 wnprt("opts   :" );
 prtflgs(cmdopts);
 wnprtn(0);
 wnprtn(0);
}


#ifdef XDBGX
//void DBG_scanpars(SPARS *k)
void DBG_scanpars(SBK *s)
{
 int i, z;
 
 if (!s) return;
 z = 1;
 
 for (i = 0; i < NPARS; i++)
   {
     if (s->preg[i]) 
         {
			if (z) {DBGPRT("\n\t  PARS ck=%x :", s->cksum); z = 0;} 
             DBGPRT(" %d|", s->preg[i] & 0x8000 ? 2 : 1); 
             DBGPRT("%x=%x",s->preg[i] & 0x1fff, s->pval[i]);
         }
    
   }
}

/*
void DBG_cksum(SPARS *k, SPARS *z)
{
  int i,j, reg, dat, reg2, dat2;

 //  compare for debug k is chained, z is new
 if (!k || !z)  return;
  
 for (i = 0; i < NPARS; i++)
   {
	   
    reg = k->preg[i] & 0x1fff;
	dat = k->pval[i];
	
	for (j = 0; j < NPARS; j++)
      {
		 reg2 = z->preg[j] & 0x1fff;
	     dat2 = z->pval[j];        // new ones
		if (i != j)
           {
            if (reg2 == reg)
              { // regs match
			    
                if (dat2 != dat)
                    {DBGPRT (" R%x [%x|%x]",reg, dat2,dat);}
				reg = 0;	
				break;
			  }
		   }
	  }

    if (reg)
      { // no reg match
        DBGPRT (" !R%x [%x]",reg, dat);}
      }
   
 
}
 */

void DBG_sbk(SBK *s)
{
 HOLDS *x;
 int z;
	  
 DBGPRT  ("scan %05x ", s->start);
 DBGPRT  ("end=%05x  ret=%05x t=%d",s->end, s->retaddr, s->ctype);
 if (s->sub)  DBGPRT (" sub %05x", s->sub->addr);   else DBGPRT("         ");
 if (s->caller)
 { 
  DBGPRT (" callst %05x r %05x", s->caller->start, s->caller->retaddr);
  if (s->caller->sub) DBGPRT (" csub %05x", s->caller->sub->addr);
 }
 if (s->cmd) DBGPRT (" cmd");
 if (s->inv) DBGPRT (" INV");
 if (!s->scanned) DBGPRT (" !Scan");
 if (s->held) DBGPRT(" Held %05x", s->held->start);
 
 x = s->hchn;
 
 
 z = 1;
 
 while (x)
   {
     if (x->hblk && z) { z = 0; DBGPRT("\n\tHOLDS ");} 
	 if (x->hblk) DBGPRT(" %05x", x->hblk->start);
     x = x->next;
   }
  //if (s->hblk) DBGPRT (" HOLD to %5x", s->hblk->start);
 //if (s->holder) DBGPRT (" Holder");
 
 DBG_scanpars(s);               //->pars);
 DBGPRTN(0);   
}


void DBG_scans(void)
{

  short ix;


   DBGPRTN("# dbg scans num = %d tscans = %d", scanch.num, tscans);

   for (ix = 0; ix < scanch.num; ix++)
    {
    DBG_sbk((SBK*) scanch.ptrs[ix]);

    }
 wnprtn(0);
} 

#endif




SYT* get_sym(int rw, int add, int bit, int pc)
{
  SYT *s, *t;
  int a;

  if (bit > 7)
   {                // always do on a byte basis
    add++;
    bit -= 8;
   }
  
  a = nobank(add);
  if (a < PCORG)  add = a;     // fiddle for low syms

  if (!val_data_addr(add)) return 0;

  s = (SYT*) schmem();      // dummy block for search
  s->add = add;             // symbol addr
  s->bitno = bit;
  s->start = pc;            // current addr for range check
  if (rw) s->writ = 1;
  
  t = (SYT*) find(&symch,s);

if (!t && s->start)
   {         // try default (write)
    s->start = 0;
    t = (SYT*) find(&symch,s);
   }
   
  if (rw && !t)
    {
	 s->start = pc;      
     s->writ = 0;     // try a read symbol at address
     t = (SYT*) find(&symch,s);
	 
	 if (!t && s->start)
      {         // try default read symbol
       s->start = 0;
       t = (SYT*) find(&symch,s);
      }
    } 

  if (t) t->used =1;               // for printout & debug

  return t;
}



char* get_sym_name(int rw, int add, int pc)
 {
  SYT* x;    // whole 'read' symbols only

  x = get_sym(rw,add,-1, pc);
  if (x) return x->name;
  return NULL;
 }


void prt_cmds(CHAIN *z)
{                // sub command printer for both cmd and auxcmd
 short ix;
 LBK *c;
 SYT *x;

 for (ix = 0; ix < z->num; ix++)
    {
     c = (LBK*) z->ptrs[ix];
     
	 wnprt("%-7s %x %x", dirs[c->fcom].com, c->start,c->end);
  
     if (dirs[c->fcom].namee)         // name allowed
      {
       x = get_sym(0,c->start,-1,0);
       if (x)
        {
         wnprt("  %c%s%c  ", '\"', x->name, '\"');
         x->prtd = 1;        // printed this symbol
        }
      }

     prt_cadt(c->adnl);

     if (c->cmd) wnprt("         # cmd");
     wnprtn(0);
    }
   wnprtn(0);
     wnprtn(0);
}



void prt_subs (void)
{
  SXT *s;
  SYT *x;
  SXDT *d;
  short ix;

  wnprtn("# ------------ Subroutine list");
  #ifdef XDBGX  
  DBGPRTN("# num subs = %d", subch.num);
  DBGPRTN("# addr name [ psize cmd psp ]");
  #endif

  for (ix = 0; ix < subch.num; ix++)
      {
       s = (SXT*) subch.ptrs[ix];
       d = s->sdat;       

       wnprt("sub  %x  ", s->addr);
       x = get_sym(0,s->addr,-1, 0);    
       if (x)
         {
          wnprt("%c%s%c  ", '\"', x->name, '\"');
          x->prtd = 1;        // printed sym, don't repeat
         }
       if (d)
         {       
           prt_cadt(d->cadt);
           prt_csig_list(d->sigc);
		   #ifdef XDBGX  
            DBGPRT (" #sz=%d", s->nargs1);
            DBGPRT (" #sz2=%d", s->nargs2);
            DBGPRT(" XN=%d", s->xnam);	 
            DBGPRT(" CN=%d", s->cname);	 
		   #endif
		   
           if (s->cmd) wnprt ("            # by cmd");
         }
          wnprtn(0);  
      }
      
  wnprtn(0);
  wnprtn(0);  
  }


void prt_syms (void)
{
 SYT *t;
 short ix;

 wnprtn("# ------------ Symbol list ----");
#ifdef XDBGX  
   DBGPRTN("# num syms = %d", symch.num);
#endif
 for (ix = 0; ix < symch.num; ix++)
   {
   t = (SYT*) symch.ptrs[ix];
   if (!t->prtd)     // not printed as subr or struct
     {
     wnprt("sym %x ", cmdaddr(t->add));

	 if (t->start)  wnprt (" %x %x ", cmdaddr(t->start), cmdaddr(t->end));
	 
     wnprt(" \"%s\"",t->name);

    if (t->bitno >= 0 || t->writ) wnprt(" :");  
    if (t->bitno >= 0) wnprt("T %d " , t->bitno);
    if (t->writ) wnprt("W");

  //  if (!t->cmd)  DBGPRT("    # auto-added");
  //  if (!t->used) DBGPRT("      # Not used");
    #ifdef XDBGX  
     DBGPRT ("           ## A %x", t->add);
    #endif
     wnprtn(0) ;
     }
   }
  wnprtn(0);
 }

#ifdef XDBGX

void DBG_ddt (void)
{
  DDT *r ;
  short ix;

  wnprtn("DDT num=%d", datch.num);
  for (ix = 0; ix < datch.num; ix++)
   {
    r = (DDT*) datch.ptrs[ix];

 //  if (!r->inv)
 //    {
    wnprt("ddt %x ", r->start);
    wnprt("sz %d ", r->ssize);
    wnprt("FROM %x", r->from);
    if (r->inx)  wnprt(" inx");
    if (r->inr)  wnprt(" inr");
    if (r->imd)  wnprt(" imd");
    if (r->inc)  wnprt(" inc %d", r->inc);       //not quite useful, YET.
    if (r->rbt)  wnprt(" rbs");
    if (r->vect) wnprt(" vec");
    if (r->psh)  wnprt(" psh");
    
    if (r->tab)
    {
      wnprt(" tab");
      wnprt(" C%d R%d" ,r->p1, r->p2);
    }
    else
    if (r->func)
      {
        wnprt(" func %d", r->p2);
      }
    else
     { 
      if (r->inx)
      {if (r->p1 > -32 && r->p1 < 32)    
      wnprt(" A %x V %d T%x" ,r->p2, r->p1, r->p3); 
      else
      wnprt(" A %x V %x t%x" ,r->p2, r->p1, r->p3); }
     }
  //    if (r->p1)  wnprt(" p1 = %x", r->p1) ;
  //    if (r->p2)  wnprt(" p2 = %x", r->p2) ;
     
//   wnprtP(r->inv, " INV");
 //   wnprtP(r->cmd, "  # CMD");
    wnprtn(0);
 //   }

   }
}

#endif


void prt_rbt (void)
{
  RBT *r ;
  short ix;

  for (ix = 0; ix < basech.num; ix++)
   {
    r = (RBT*) basech.ptrs[ix];

   if (!r->inv)
     {
    wnprt("rbase %x %x" , r->reg , r->val);
	if (r->start)  wnprt("  %x %x", r->start, r->end);
    if (r->cmd) wnprt("       # cmd");
    wnprt("\n");
    }

   }
     wnprtn(0);
}


void prt_dirs(int x)
{                 // print all directives for possible re-use
  #ifdef XDBGX  
  DBG_scans();
  #endif
  prt_opts();
  prt_bnks();
  prt_rbt();
  prt_cmds(&cmdch);
  prt_cmds(&auxch);
  prt_subs();
  if (x) prt_syms(); 
}


#ifdef XDBGX

void DBG_jumps (CHAIN *x)
{
 JLT *j;
 short ix;

  DBGPRT("------------ Jump list \n");
  DBGPRT("num jumps = %d\n", x->num);

  for (ix = 0; ix < x->num; ix++)
    {
      j = (JLT*) x->ptrs[ix];

      DBGPRT("%5x (%5x)->%5x t%d",j->from, j->from+j->jcnt,j->to, j->jtype);

	  if (j->bswp)  DBGPRT (" bswp");
      if (j->back)  DBGPRT (" back");
      if (j->uci)   DBGPRT (" uci");
      if (j->uco)   DBGPRT (" uco");
      if (j->retn)  DBGPRT (" retn");
      if (j->cbkt)  DBGPRT (" }");
      DBGPRTN (0);
    }
    DBGPRTN (0);
}

#endif

SXDT *get_sxdt(SXT *s, int create)
{
    // get sxdt from subr, create if not exist
  SXDT *d;

  if (!s) return NULL;               // safety, no subr....

  d = s->sdat;

  if (!d && create)
    {
     d = (SXDT*) mem (0,0,sizeof(SXDT));
     memset(d,0, sizeof(SXDT));             //clear block
     s->sdat = d;
    }

  return d;
}

CSIG *get_csig(SXT *s)
{
  SXDT *d;

  if (!s) return NULL;
  d = s->sdat;
  if (!d) return NULL;
  return d->sigc;
}

CADT *get_cadt(SXT *s)
{
  SXDT *d;

  if (!s) return NULL;
  d = s->sdat;
  if (!d) return NULL;
  return d->cadt;
}


CADT* add_cadt (CADT **x)
  {
   // add cadt block to CADT chain at end

  CADT *ans, *z;

  ans = (CADT *) mem (0,0,sizeof(CADT));
  memset(ans,0,sizeof(CADT));                // clear block
  
  ans->cnt = 1;
  ans->ssize = 1;                            // single byte unsigned is default

  z = *x;                                    // chain ptr (can be zero)

  if (!z) *x = ans;      // new chain
  else
   {    
    while(z->next) z = z->next;   // end of chain
    z->next = ans;                // add to end
   }

  return ans;
}


CSIG *add_csig(SXT *s)           //, int forb)
 {
  CSIG *f, *x;
  SXDT *d;

  if (!s) return NULL;               // safety, no subr....

  d = get_sxdt(s,1);

  x = (CSIG*) mem (0,0,sizeof(CSIG));
  memset(x,0,sizeof(CSIG));            //new zero block

  if (!d->sigc)
   {
     d->sigc = x;            // front of chain
   }
  else
    {                 // end of chain
      f = d->sigc; 
      while(f->next) f = f->next;
      f->next = x;
    }

return x;
 }

// may not need both s and patn here ...

CSIG* find_csig(SXT *sub, SIG *s, PAT *patn, int addr)
{
 CSIG *f;                     // return block which matches or zero if no match
 int ans;
 SIG *z;

 ans = 0;

 if (!sub) return 0;

 f = get_csig(sub);

 while(f)
   {
    ans = 0;
    z = f->sig;
    if (!s    || s == z) ans ++;
    if (!addr || addr == f->caddr) ans++;
    if (!patn || patn == z->ptn) ans++;
    if (ans > 2) break;
    f = f->next;
   }

  return f;
 }

CSIG* find_csigp(SXT *s)
{
   // find any sig which is a param getter (for encodes and grnf)
   // used for func and tab too !! (and address match ?)

 CSIG *f;

 if (!s)  return 0;

 f = get_csig(s);

  while(f)
   {
    if (f->gpar && f->disb == 0 && f->par1 > 1)  break;
    f = f->next;
   }

  return f;
 }

CSIG* find_csige(CSIG *z, int reg)
{
   // find any sig which is an encoder

 CSIG *f;

 if (!z)  return 0;

 f = z->next;              //all csigs AFTER z

  while(f)
   {
    if (f->enc && f->disb == 0 && f->dat1 == reg)  break;
    f = f->next;
   }

  return f;
 }


void get_scanpars(SBK *s)                             //SPARS **k)
{             //get latest updated registers from ramlist
  int i,j, csk;
 
 /* if (!(*k))               //s->pars)
   {
 	*k = (SPARS*) mem(0,0,sizeof(SPARS));     // space for addr and its value
   } */
  
//  add a simple add type checksum 
  csk = 0;
  
  for (i = 0; i < NPARS; i++)
   {
     s->preg[i] = ramlist[i];                    //(*k)->preg[i] = ramlist[i];
     j = ramlist[i] & 0x1fff;
     if (j) 
      {
	   s->pval[i] = ibuf[j];
       if (ramlist[i] & 0x8000) s->pval[i] |= (ibuf[j+1] << 8);
	   csk += s->pval[i];
	   csk += j << 19;
	  }
   }
  s->cksum = csk;
//  (*k)->sptr = stkp; 
//   DBGPRT("\nGET ");
//   DBG_scanpars(*k);
//   DBGPRTN(0);

}

void put_scanpars(SBK *s)
{            // restore saved values from hold
 int i,j;

 if (!s) return;
 
 // stkp = k->sptr; 
 for (i = 0; i < NPARS; i++)
   {
	ramlist[i] = s->preg[i];                      // Restore ram list as well as the data
    if (s->preg[i])
       {
        j = s->preg[i] & 0x1fff;
        ibuf[j] = s->pval[i] & 0xff;
        if (s->preg[i] & 0x8000) ibuf[j+1] = (s->pval[i] >> 8);
       }
   }
 //    DBGPRT("\nPUT ");
 //    DBG_scanpars(k);
//	   DBGPRTN(0);
}















LBK *set_auxcmd (int start, int end, int com)
{
   LBK *b, *z;

   if (icaddrse(&start,&end)) return NULL;

   b = (LBK *) chmem(&auxch);
   b->start = start;
   b->end   = end;
   b->fcom  = com & 0x1f;    // max 31 as set
   if (com & 32) b->cmd = 1;

   z = inscmd(&auxch,b);
   #ifdef XDBGX
   DBGPBKN (b,"set auxcmd");
   #endif
   if (z != b) return 0;                       // fail

   return z;
}

int check_xcode(int start)
{
  LBK n;                  // local block
  
  n.start = start;
  n.fcom  = C_XCODE;                  // search for xcodes
  if (find(&auxch,&n)) return 1;
  return 0;
}


LBK* set_cmd (int start, int end, int com)
 {          // this for user commands only, no called params
  LBK *n, *z;

  if (anlpass >= ANLPRT)    return NULL;
  if (icaddrse(&start,&end)) return NULL;

  n = (LBK *) chmem(&cmdch);
  n->start = start;
  n->end   = end;
  n->fcom  = com & 0x1f;    // max 31 as set
  if (com & C_CMD) n->cmd = 1;


  if (n->fcom == C_CODE && check_xcode(start))
     {
	  #ifdef XDBGX  
       DBGPRTN ("XCODE bans set_cmd %x", start);
	  #endif 
      return NULL;
     }

  z = inscmd(&cmdch,n);  
      
  if (z != n) return 0;                        // fail
  #ifdef XDBGX  
  DBGPBKN (n, "Add cmd");   
  #endif
  return n;   // successful
 }


/************************************************
 *   add symbol
 * addr       address to assoc. with name
 * bitno      bit number PLUS 1  (0 is no bits)
 * fnam       name to add
 ************************************************/

int do_symname (SYT *xsym, const char *fnam)
 {
  int z, s;

  if (!fnam || !*fnam)  return 0;
  z = strlen (fnam) + 1;  // include null, strlen does not...
  if (xsym->name)
  s = strlen (xsym->name) + 1;  // include null, strlen does not...
  else s = 0;
  
  if (z > 31) z = 31;    // max storage size

  xsym->name = (char *) mem (xsym->name,s,z);
  if (xsym->name)  strncpy (xsym->name, fnam,z);
  return z;

 }


SYT *new_sym (int rw, const char *fnam, int add, short bitno, int start, int end)
{
   // rw is 0 for read sym, 1 for write, |=2 for cmd set.
   // Changed this so that all bits are 0-7 and addressed by byte (-1 is full word)

  SYT *s, *z;
  int ans,ix;

  if (anlpass >= ANLPRT) return 0;
  if (!fnam || !(*fnam)) return 0;              // safety

  if (bitno > 7)
   {
    add++;
    bitno -= 8;
   }

   s = (SYT *) chmem(&symch);
   s->add = add;
   s->bitno = bitno;
   s->writ = rw & 1;
   s->start = start;
   s->end = end;
   
   ix = bfindix(&symch, s);
   ans = cpsym(ix, s);              // zero if matches


 // check for overlap here

   if (ix < symch.num)
    { 
     z = (SYT *) symch.ptrs[ix];         // next block
     if (s->add == z->add) 
	  { 
		  if (s->end > z->start) ans = 0;
	  }
	}

   if (ans)
   {
 
     // if (bitno >=0) xsym->flags = 1;       // at least one flags bit.
     if ((rw & 2)) s->cmd = 1;
     if (do_symname (s, fnam)) chinsert(&symch, ix, s);

     #ifdef XDBGX       
      DBGPRT("add sym %6x %s",add,fnam);
	  if (s->start)      DBGPRT (" %05x - %05x" , s->start, s->end);
      if (s->bitno >= 0) DBGPRT (" B%d", s->bitno);
	  
      DBGPRT("\n");
	 #endif 
    return s;
   }   // do insert

 return (SYT*) symch.ptrs[ix]; // duplicate

}


// add numbered auto sym and increment save value

SYT *add_nsym (uint ix, int addr)
{
  SYT *xsym;
  AUTON *n;
  int z;

  if (anlpass >= ANLPRT) return 0;
  if (ix >= NC(anames)) return NULL;    // safety check

  if (!(cmdopts & anames[ix].mask)) return NULL;    // autoname option not set

  xsym = get_sym(0,addr,-1,0);

  if (xsym)
   {                  // symbol found
    if (xsym->cmd) return xsym;              // set by cmd
    if (xsym->anum) return xsym;       // already autonumbered
   }

  // now get symbol name

  n = anames + ix;

  z = sprintf(nm, "%s", n->pname);
  z += sprintf(nm+z, "%d", n->nval++);

   // name now set in nm

  if (xsym) do_symname (xsym, nm);   //update name
  else xsym = new_sym(0,nm,addr,-1,0,0);     // add new (read) sym

  if (!xsym)
   {
    if (anames[ix].nval > 1) anames[ix].nval--;
   }
  else
   {
    xsym->anum = 1;                 // remember autonumber
   }
  return xsym;
}


void do_subnames(void)
{
  // add names as required - do at end of dissas from
  // subr chain

 int ix;
 SXT  *sub;
 SYT  *z;

 for (ix = 0; ix < subch.num; ix++)
    {
     sub = (SXT*) subch.ptrs[ix];

     if (!sub->cmd && !sub->cname && sub->xnam)
	   { // not cmd, name change allowed....
	   
        z = (SYT*) schmem(); 
        z->add = sub->addr;
        z->bitno = -1;
        z = (SYT*) find(&symch,z);
        if (!z)  
	    {  // check if sym exists alreaady....
          add_nsym(sub->xnam,sub->addr);             // add subr name acc. to type
	    }
	  }	
    }
   #ifdef XDBGX  	
   DBGPRTN("end dosubnames");
   #endif
 }


void set_size(SXT *s)
{
 CADT *x;

 if (!s) return;
 s->nargs1 = 0;

 x = get_cadt(s);

 while(x)
   {
    s->nargs1 += dsize(x);
    x = x->next;
   }
    #ifdef XDBGX  
    DBGPRTN("set size = %d for %x",s->nargs1, s->addr);
	#endif
}

void fix_scans(int addr)
 {
   // addr is a subroutine start.
   // after a sig added to subroutine, the sig may change no of args for 'sub'
   // so reset all caller scans of start address (=addr) to be redone. 
   // NB. this may not work for sigs which do multiple level tricks !!



// should really drop any blocks which are children of the rescanned blocks......


   int ix;
   SBK  *s, *x;

   x = (SBK*) schmem();
   x-> start = addr; 

   ix = bfindix(&scanch, x);            // find any scans for subr start
   if (ix >= scanch.num) return;        // none found

   while (ix < scanch.num)
    {
      s = (SBK*) scanch.ptrs[ix];
      if (s->start > addr+1) break;      // end of scans to 'sub'- allow for PUSHP

      s = s->caller;                         // caller (of this sub)
 
      if (s)
	  {  
        if (s->scanned && !s->held)
        {
         s->scanned = 0;                     // rescan this caller
		 s->curaddr = s->start;
		 s->end = maxadd(s->start);
         #ifdef XDBGX  
         DBGPRT("Set rescan");
		 #endif
         if (s->inv)
           {
             s->inv = 0;                     // rescan even if invalid (may have had wrong arguments before this)
			 #ifdef XDBGX  
             DBGPRT(" INV=0");
			 #endif
           }
		   #ifdef XDBGX  
         DBGPRT(" %x (sub %x)", s->start,addr);
		 #endif
        }
	    if (s->ctype == S_SJMP )
	    { 
         // reset any jumps to specials for this subr
         s->ctype = S_JSUB;
	     #ifdef XDBGX  
         DBGPRT(" Reset type to SUB %x", s->start);
		 #endif
	    }
	     #ifdef XDBGX  
         DBGPRTN(0);
		 #endif	
	  }	
      ix++;
    }
 }

CSIG* init_csig(SIG *x, SBK *b)
{
   CSIG *c;

   c = add_csig(b->sub);               // sub from scan block
   c->sig = x;                         // this sig add to chain
   c->caddr = b->curaddr;              // and current addr  = curinst->ofst
   #ifdef XDBGX  
   DBGPRT (" Sub %x init csig ", b->sub->addr);
   //   DBG_CSIG(c);
   //  DBGPRTN(0);  
   #endif  
 return c;
}


void nldf (SIG *x, SBK *b)
{
  // default preprocess - no levels
  
 if (!b) return;
 
 init_csig(x,b); 
  //   DBG_CSIG(c);
 //  DBGPRTN(0);
}

void fldf (SIG *x, SBK *b)
{
  // preprocess for funclookups - no levels
  // pick up signed flag(s)
  
   CSIG *c;
   int val;
  
   if (!b) return;
   
   c = init_csig(x,b); 

   c->dat1 = x->v[3]; 
   #ifdef XDBGX  
   DBGPRT(" SIGN Flag = %x", c->dat1);
   #endif	
 
  if (!find_ramlist(c->dat1))
         {
			#ifdef XDBGX   
          DBGPRTN(" Not found");
		  #endif
		  c->par1 = 0;
		 } 
     else 
		 {
		  c->par1 = g_byte(c->dat1);      // will have been restored from list 
		  #ifdef XDBGX  
          DBGPRTN(" = %x", c->par1);
		  #endif
      val = g_byte(x->v[3]);          // flag value 
	  val &= (~x->v[10]);
 	  val &= (~x->v[11]);
      upd_ram(x->v[3],val,1);
      upd_ramlist(x->v[3],1);


         }
 
 // TEST !! clear the flag(s) here



  //   DBG_CSIG(c);
  //  DBGPRTN(0);
}


void tldf (SIG *x, SBK *b)
{
  // preprocess for tablookup - no levels
  // pick up signed flag(s)
  
   CSIG *c;
   SIG  *t;
   int val;
  
   if (!b) return;
   
   c = init_csig(x,b); 

   // In tables , flag is in the INTERPOLATE signature - so go and find it
   // tabinterp at saved subr call address = x->v[0] (jump addr)

      t = (SIG*) schmem();   
      t->ofst = x->v[0]; 
      
      t = (SIG*) find(&sigch,t);               // is it there already ?
      if (!t) t = scan_asigs (x->v[0]);        // no, so scan for it
 
      if (t) 
       {
        c->dat1 = t->v[3];
		#ifdef XDBGX  
        DBGPRT(" SIGN Flag = %x", c->dat1);
		#endif
        
        if (!find_ramlist(c->dat1))
         {
		  #ifdef XDBGX   
          DBGPRTN(" Not found");
		  #endif
		 } 
        else 
		 {
			 
		  c->par1 = g_byte(c->dat1);          // flag value 
		  c->dat1 |= (t->v[10] << 8);         // and mask in next byte
		  #ifdef XDBGX  
          DBGPRTN(" = %x m%x", c->par1, c->dat1);
		  #endif
		       val = g_byte(t->v[3]);          // flag value 
	  val &= (~t->v[10]);
      upd_ram(t->v[3],val,1);
      upd_ramlist(t->v[3],1);
		  
		  
		  
		  
		  
         }

// TEST - clear sign flag
 

 	   }
	   #ifdef XDBGX  
       else DBGPRTN(" SIGNED TABINTERP Not found");
       #endif
	   
  //   DBG_CSIG(c);
 //  DBGPRTN(0);
}





void plvdf (SIG *x, SBK *b)
{
  // default preprocess for 'pop' type levels
   CSIG *c;
  
   if (!b) return;
   
   c = init_csig(x,b); 
         
   c->lev1 = x->v[1];
   c->lev2 = x->v[2];
   c->lev1 += c->lev2;

//   DBG_CSIG(c);
//   DBGPRTN(0);
}



void process_sig(SIG *x, SBK *b) 
 {
  // does this check AFTER the relevant do-code
  // so that curinst, codebank, etc are sorted
  // assumes only one sig can exist for one ofst
 
  CSIG *c;

  if (b->sub && b->sub->cmd && !x->ptn->spf)
     {
	   #ifdef XDBGX  
       DBGPRTN("Ignore sig %s subr %x cmd",x->ptn->name, b->sub->addr);
	   #endif
      return;      // subr specified by command, no overrides (EXCEPT an F!)
     }

  if (!b->sub)
     {          // no sub, set zero  scanblk
      x->ptn->sig_prep(x,0);
      return;
     }

   // correct that sigs are chained at the back (i.e. in order),

     c = find_csig(b->sub,x,0,0);

     if (!c)  
         {                          // not a duplicate....
		  x->ptn->sig_prep(x,b);     // add csig to sub and preprocess it
		  #ifdef XDBGX
           DBGPRT ("(PRCS) ");
           DBGPRTN ("%x (PRC) ADD csig to sub %x ",b->curaddr, b->sub->addr);
          #endif
         }
    }

/*
void do_data_pat(int ix)
{
  int i, j, start, addr,gap, val;
  int score [32];                  // = max gap
  int mtype [32];                  // temp for now whilst testing
  uchar vl  [32];               // value difference
  uchar a, b;
  // uint c,d; ? or ushort ?
  LBK *s; //, *n;
  s = (LBK*) cmdch.ptrs[ix];
//  n = (LBK*) cmdch.ptrs[ix+1];


// data struct may scan beyond end.....

if (s->fcom == C_CODE) start = s->end+1;
else start = s->start;

DBGPRTN ("start gap data analysis at %x", start);

 for (gap =1 ; gap < 32; gap++)
 {
   score[gap] = 0;
   for (i = 0; i < gap; i++)       // establish base row of 'gap' elements
     {
      addr = start+i;
      a = g_byte(addr);
      b = g_byte(addr+gap);
      vl[i] = a-b;
      mtype[i] = 0;
                             // compile value difference.
      if (a == b)  mtype[i] |= M;         // match
      else if (__builtin_popcount(a) == 1 && __builtin_popcount(b) == 1) mtype[i] = B;  // bit mask ?
      if (a < b )  mtype [i] |= I;        // increment
      if (a > b )  mtype [i] |= D;        // decrement
 //     j = __builtin_popcount(a);          // no of bits
     }

// now compare niew rows with 'base' row

   for (j = start+gap; j < (start+127)-gap; j+=gap)       // row by row until 128 max
     {
         for (i = 0; i < gap; i++)       // compare this row to the 'base'
          {
           addr = j+i;
           a = g_byte(addr);
           b = g_byte(addr+gap);
           val = a-b;                              // value difference.
           if (val == vl[i])  score[gap] +=3;

           if (mtype[i] & M)
           {
             if (a == b)  score[gap] += 2; else score[gap]--;        // match
           }
           if (mtype[i] & I)
           {
           if (a < b)  score[gap] += 2; else score[gap]--;       // decrement
           }
           if (mtype[i] & D)
           {
           if (a > b)  score[gap] += 2; else score[gap]--;       // increment
           }
           if (mtype[i] & B)
           {
            if (a != b && __builtin_popcount(a) == 1 && __builtin_popcount(b) == 1 ) score[gap] +=2;
           }

           }
     }

DBGPRTN("gap %d, score %d", gap, score[gap]);

 }

 addr = 0;                         // use for highest score
 gap = 0;                          // where it is
 for (i=1 ; i < 32; i++)
 {
   if (score[i] > addr)
   { addr = score[i];
      gap = i;
   }
 }

DBGPRTN("end analysis at %x", s->end+1);
DBGPRTN("highest score is %d for gap %d", addr, gap);

}
*/













void code_scans (void)
 {

  // turn scan list into code, if scans complete and valid
  // needs a DONE flag if called more then once....
  int ix;
  SBK *s;
 // LBK *k;          //, *n;

  for (ix = 0; ix < scanch.num; ix++)
   {
    s = (SBK*) scanch.ptrs[ix];
    if (s->inv)
       {
		#ifdef XDBGX
        DBGPRTN("INV scan ",s->start, s->end,0);
		#endif
       }

    if (s->scanned && s->end >= s->start)  // 2nd check redundant ?
     {
      set_cmd(s->start,s->end,C_CODE);
     }
   }
 }


int unhold_scans (SBK *blk)
 {
	// more efficient version - chain held blocks on holder 
  SBK *s;
  int ans;
  HOLDS *x;

  ans = 0; 
  
  if (!blk->hchn)  return 0;            // no chain,  no holds

  x = blk->hchn;                        // chain of holds
  while (x)
   {
     s = x->hblk;
     if (s)
	  { 
	    #ifdef XDBGX	
          DBGPRT("UNHOLD %x cur=%x ct=%d", s->start, s->curaddr, s->ctype);
          if (blk->sub) DBGPRT(" sub %x", blk->sub->addr);
		  if (blk->caller) DBGPRT(" caller %x", blk->caller->start);
	  	  DBGPRTN(0);
	    #endif
        s->held = 0;                    // Do NOT update scanpars here
	    x->hblk = 0;                    // clear ref to this block, but keep chain entry
        ans = 1;                        // at least one block unheld
      }
	x = x->next;
   }
 return ans;        // set if anything unheld
}

 
void end_scan (SBK *s, int end)
{
   // allow zero end value
  if (anlpass >= ANLPRT || PMANL ) return;

  #ifdef XDBGX
   if (s->held) DBGPRT ("Held "); 
   if (s->inv)  DBGPRT ("Invalid ");
   DBGPRT("END scan %05x t=%d",s->start, s->ctype);
   if (!s->held && end) DBGPRT(" end = %05x", end);
   DBGPRTN(0);
  #endif
 
   if (!s->held)  
    {
	  if (end) s->end = end;
      s->stop = 1;
      if (s->end <= maxadd(s->end) && s->end >= s->start)
        {
         s->scanned = 1;
        }
		
    }
}



SXT* add_subr (int addr)
{
  // type is 1 add name, 0 no name

  int ans, ix;
  SXT *x;

  if (anlpass >= ANLPRT) return NULL;
  if (!val_code_addr(addr)) return NULL;

  if (check_xcode(addr))
     {
	   #ifdef XDBGX	 
       DBGPRTN("XCODE bans add_subr %x", addr);
	   #endif
      return NULL;
     }

  ans = 1;

// insert into subroutine chain
  x = (SXT *) chmem(&subch);
  x->addr = addr;

  ix = bfindix(&subch, x);
  ans = cpsub (ix,x);

  if (ans)
   {
    x->xnam = 1;                   // default subname ('subnnn')
    chinsert(&subch, ix, x);       // do insert
    #ifdef XDBGX
      DBGPRT("Add sub %x\n",  addr);
    #endif
    if (g_byte(addr) == 0xf2) {x->psp = 1; addr++;}   // subr begins with pushflags
   // return x;
   }
  else
  {	  
   // match - do not insert, free block. Duplicate.
    #ifdef XDBGX
     DBGPRT("Dup sub %x\n",addr);
    #endif
  x = (SXT*) subch.ptrs[ix];
  }

  return x;                   //(SXT*) subch.ptrs[ix];
  }


// only sj_subr uses the answer from add_scan..and sets caller.


SBK *add_scan (int add, int type, SBK *caller)
{

 /* add code scan.  Will autocreate a subr for scans > 3 so that sub ptr set, and comparisons work.
  * types 0-15. +16 indicates a command.....+32 for return zero if not added (for vects)
  * 
  * CHANGE CURRENT BLOCK - TEST TEST
  * 
  * scan type  =
   1 init jump    -- is a static jump without a hold
   2 static jump
   3 cond jump
   4 int subr
   5 vect subr
   6 called subr
   7 converted jmp to subr
 */

  SBK *s;             //, *t;
  int ans, ix;

  if (anlpass >= ANLPRT)   return 0;

// TEMP add a safety barrier ...
if (scanch.num > 20000) return 0;

  if (!val_code_addr(add))
    {
	 #ifdef XDBGX	
     if (val_jmp_addr(add)) DBGPRT ("Ignore scan %x", add);
     else        DBGPRT ("Invalid scan %x", add);
     if (caller) DBGPRT (" from %x", caller->curaddr);
     DBGPRTN(0);
	 #endif 
     return 0;
    }

  if (check_xcode(add))
     {
	  #ifdef XDBGX	
      DBGPRT("XCODE bans scan %x", add);
      if (caller) DBGPRT (" from %x", caller->curaddr);
      DBGPRTN(0);
	  #endif
      return 0;
     }

  if (PMANL)
      {
	   #ifdef XDBGX
       DBGPRTN("no scan %x, manual mode", add);
	   #endif
       return 0;
      }

  s = (SBK *) chmem(&scanch);             // new block (zeroed)
  s->start = add;
  s->end   = maxadd(add);                 // safety
  s->curaddr = s->start;                  // important !! 
  if (type & C_CMD) s->cmd = 1;           // added by user command

  s->ctype = type & 0xf;                  // scan type 0-15

  // retaddr is actually the START of calling opcode, need to add cnt to get true return addr
  // need to keep start of calling opcode so that UNHOLD resumes at the call or jump to redo any subr arguments

  get_scanpars(s);                              // Always, for a checksum
	
  if (caller)
   {
    s->caller   = caller;
    s->retaddr  = caller->curaddr;                        // return addr for tracking
    s->sub      = caller->sub;
	s->stk      = caller->stk;
   }

   if (s->ctype == S_CSUB)
      { 
		s->sub = add_subr(s->start);           // add_sub returns 'sub' even if a duplicate
		s->ctype = S_SJMP;
		s->stk = 0;
	  }	

   ix = bfindix(&scanch, s); 
   ans = cpscan(ix,s);

   if (!ans) 
   {              // match - so is a duplicate

	 #ifdef XDBGX     
	  s = (SBK*) scanch.ptrs[ix];                  // block which matches
	  DBGPRT ("Add scan DUPL of %x t=%d", s->start, s->ctype);
      DBGPRT (" from %x", s->retaddr);
      if (s->cmd)  DBGPRT(" +cmd"); 
      if (s->scanned) DBGPRT(" Scanned");
	  DBG_scanpars(s);
	  DBGPRTN(0);
	 #endif

	  return (SBK*) scanch.ptrs[ix];              // duplicate scan
   }   

   chinsert(&scanch, ix, s);            // do insert

   #ifdef XDBGX
    DBGPRT ("Add scan %x t=%d", s->start, s->ctype);
    if (s->ctype == S_CSUB) DBGPRT(" +sub");
    if (s->cmd)  DBGPRT(" +cmd");
 
    if (caller) 
      {
       if (caller->sub) DBGPRT(" +sub %x", caller->sub->addr);
       DBGPRT (" from %x", s->retaddr);
      }
   DBGPRTN(0);
  #endif
 return s;
}

/*
void scan_all (void);

/
void scan_gaps(void)
{
  int ix, i, ofst;
  LBK *t, *n, *l;    // this, next, last

  for (ix = 0; ix < cmdch.num-1; ix++)
   {
    if (ix <= 0) l = 0; else l = (LBK*) cmdch.ptrs[ix-1];
    
    t = (LBK*) cmdch.ptrs[ix];
    i = ix+1;
    n = (LBK*) cmdch.ptrs[i];

    if ((t->start & 0xffff) <= 0x201e) continue;

    // check for filler (can be anywhere !!)
    ofst = t->end+1;
    while (ofst < n->start)
     {  
      if (g_byte(ofst) != 0xff)  break;
      ofst++;
     } 

     if (ofst < n->start)
      {     
       DBGPRTN("GAP detected %x->%x type %s and %s", t->end+1, n->start-1, cstr[t->fcom], cstr[n->fcom]);
       
       if (l && l->end == t->start-1 && t->fcom == C_WORD && n->fcom == C_CODE && l->fcom == C_CODE &&
           t->end - t->start == 1)
             {
                 DBGPRT("*A ");
              add_scan(t->start,S_SJMP|C_TST,0);       // add test marker
              scan_all(); 
             }
                  
       if (t->fcom == C_CODE && n->fcom == C_CODE)
          {
            DBGPRT("*B ");
            add_scan(t->end+1,S_SJMP|C_TST,0);       // add test marker
            scan_all(); 
          }
     // or if only one word right at front (a push...) and then code on both sides, this also for test scan. 


      }
  //  }
   }
 // justprint at the moment

} */




int decode_addr(CADT *a, int ofst)
  {
  int rb,val,off;
  RBT *r;

  val = g_val (ofst, a->ssize);   // offsets may be SIGNED
  rb  = val;

  if (a->foff)
    {            // fixed offset (as address)
     val += a->addr;
     val = nobank(val);
     return val;
    }

  switch (a->enc)
     {
      case 1:           // only if top bit set
      case 3:
        if (! (rb & 0x8000)) return val;
        off = val & 0xfff;
        rb >>= 12;            // top nibble
        if (a->enc == 3) rb &= 7;
        rb *=2;
        break;

      case 2:       // Always decode
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
  
	r = get_rbt((a->addr & 0xff) + rb, ofst);   
    if (r)   val = off + r->val;        // reset op
	#ifdef XDBGX
    else
    DBGPRTN("No rbase for address decode! %x (at %x)", a->addr+rb, ofst);
	#endif
   }
   
  if (a->vect)
  {    // pointer to subr - may do this elsewhere too... 
   val |= g_bank(ofst);   // add bank from start addr....not necessarily right
  } 
  return val;

}

void chg_subpars (SXT* sub, int reg, int size)
{
 // or merge reg and size ?
 //subr (bytes to words)  here and other places too. (reg, ssize) ?

 int tsz, ls;   // total size, local size
 CADT *a;

 if (sub && anlpass < ANLPRT) a = get_cadt(sub);
 else  a = 0;

 #ifdef XDBGX 
   DBGPRT("In chg_subpars sub %x  p%x to %d", sub->addr,reg, size);
 #endif
 
 while (a)
  {
   ls = casz[a->ssize];   // saves some lookups
   if (a->dreg == reg && ls != size)
     {
      tsz = ls * a->cnt;      // size of entry
      a->ssize = size;
      a->cnt = tsz/ls;         // reset count
       #ifdef XDBGX 
	   DBGPRT (" changed ");
	   #endif
      break;                             // found
     }
   a = a->next;
  }
 #ifdef XDBGX 
  DBGPRTN(" exit chg");
 #endif 
}



int get_subpars(SXT* sub, SBK* blk, int pno)
{
  // pno is which param gets returned in answer, used for tab and funcs
  // or is register with param in it (marked by flag) 
  // This gets whole chain at the moment.....put break back after debug.
  // blk is caller

 int i,n, size, reg, addr, val, ans;
 CADT *a;


 if (!sub) return 0;
 if (anlpass >= ANLPRT) return 0;

 a = get_cadt(sub);
 if (!a) return 0;   // no additional
 
 addr = blk->nextaddr;
 
  #ifdef XDBGX 
  DBGPRT("In get_subpars sub %x at %x", sub->addr,addr);
  #endif
 reg = 0;
 
 if (pno & 0x100)
   {
     reg = pno & 0xff;
     pno = 0;
   }
   
  #ifdef XDBGX 	 
   if (pno & 0x100) DBGPRTN(" Reg %x", reg); else DBGPRTN(" pno %d",  pno);  
  #endif
  
 n   = 1;
 ans = 0;

 while (a)
  {
   for (i = 0; i < a->cnt; i++)
     {
       size = casz[a->ssize];              // TEST set up data correctly...
       val = decode_addr(a, addr);

       if (val_data_addr(val))
        {
          add_ddt(val,size);            //ddt is there, it's the cmd merge....
        }
   
       if (pno && n == pno)
          {
           ans = val;
		   #ifdef XDBGX
           DBGPRT("*");
           #endif 
          }
       
       if (reg && a->dreg == reg) {
           ans = val; 
		   #ifdef XDBGX
            DBGPRT("*");
		   #endif
           }
       #ifdef XDBGX
       DBGPRT ("P%d = %x, ",n, val);
	   #endif
       n++;
       addr += size;
     }

  a = a->next;
}
  #ifdef XDBGX
  DBGPRTN(0);
  #endif
 return ans;
}


/*
void DBGPSUB(SXT *csub, SBK *blk)
{
  DBGPRT("Resume at %x ", blk->nextaddr + csub->nargs1);
  if (csub->cmd) DBGPRT("(cmd) ");
  if (csub->nargs1 > 0) DBGPRT("%d numargs ",csub->nargs1);
}

*/

CSIG* cpy_csig_levels(CSIG *z, SBK *cblk)
{
// copy any new sigs to calling subr, if addr matches and levels high enough....

CSIG *a;

if (!cblk->sub) return 0;

if (z->disb) return 0;                // DON'T copy disabled csigs.
  #ifdef XDBGX
  DBGPRTN("Cpy csig lvs [%s %d %d] to %x ",z->sig->ptn->name, z->lev1, z->lev2,cblk->sub->addr); 
  #endif

a = find_csig(cblk->sub,0,z->sig->ptn,cblk->curaddr);

if (!a && (z->lev1 > 1 || z->lev2 > 1))
    {           // not match addr or pattern and > lev 1

     a = add_csig(cblk->sub);               //,0);            //z->sig->ptn->forb);
     *a = *z;                           // copy to new block
     a->next = 0;                       // clear next
     if (a->lev1 > 0) a->lev1--;        // safety so it don't wrap
     if (a->lev1 > 0) a->l1d = 0;       // > 0, mark as active again
     if (a->lev2 > 0) a->lev2--;        // safety so it don't wrap
     if (a->lev2 > 0) a->l2d = 0;
    
     a->caddr = cblk->curaddr;
	 #ifdef XDBGX
     DBG_CSIG(a);
     DBGPRTN(0);
	 #endif
     return a;
    }

 return 0;
  }

void sdumx (CSIG *z, SBK *c, SBK *b)
{
  // dummy handler
  #ifdef XDBGX
   DBGPRTN(" SKIP %x (%s)",b->curaddr, z->sig->ptn->name);
   #endif
}

void avcf   (SIG *z, SBK *blk) 
{
	int start, addr, val, bk;
	LBK *s;
	CADT *c;
	
	addr = z->v[4];
	start = addr;
    bk = z->ofst & 0xf0000;
    wnprtn("in avcf, start = %05x", addr);
	
	while (1)
	{
		val = g_word(addr);
		val |= bk;
		if (!val_code_addr(val)) break;
        add_scan(val, S_CSUB, 0);
		addr -=2;
	}	
    s = set_cmd (addr, start, C_VECT);	// need to add the bank....
	
	if (s)
    {
     // check banks here......
       c = s->adnl;
       if (!c) c = add_cadt(&s->adnl);
       c->foff = 1;
       c->ssize = 0;         // no size
       c->addr = bk >> 16;   // add bank
      }
 
	wnprtn("stop at %05x", addr);
}

void fnplu (CSIG *z, SBK* nsc, SBK *blk)
 {

    // func lookup - don't do anything to SUB itself
    // added check for signed sigs at front, if not done already....

   int adr, size, mask, val, val2, sign;
   SIG   *s;
   DDT   *d;
   CSIG  *x;
   SXT *csub;
   
   csub = nsc->sub;
   
   s = z->sig;

   size = s->v[2]/2;                 // size from sig (unsigned)
  #ifdef XDBGX  
  DBGPRT("In FNPLU %d (%x)", size, blk->curaddr);
  #endif

   if (size < 1 || size > 4)
       {
		#ifdef XDBGX  
          DBGPRTN("FUNC Invalid size - set default =1");
		#endif 
        size = 1;
       }

   x = find_csigp(csub);

   if (x)
     {
       // this subr has an inline par getter
       adr = get_subpars(csub,blk, s->v[4] | 0x100);      // reqd register
     }

   else
     {
      adr = g_word(s->v[4]);        // address from sig
	  #ifdef XDBGX
       DBGPRTN("Direct addr R%x = %x", s->v[4], adr); 
	  #endif 

     }
   if (adr < 256 && adr > minwreg)
     {
	  #ifdef XDBGX
      DBGPRTN("FUNC addr is reg %x = %x ", adr, g_word(adr));
	  #endif
      adr = g_word(adr);
     }

  if (adr == 0)
     {
      #ifdef XDBGX	
      DBGPRTN("FUNC Invalid address ZERO (%x)",blk->curaddr );
	  #endif
      return;
     }
  
  if (size > 1 && (adr & 1))
       {
		 #ifdef XDBGX  
         DBGPRTN("FUNC odd start addr for word func - adjust PC+1");
		 #endif
        adr+= 1;
       }

   adr = nobank(adr);
   adr |= datbank;       // need this as get_reg only does 16 bit
  
      if (!val_data_addr(adr))
     {
	   #ifdef XDBGX
       DBGPRTN("FUNC Invalid addr %x", adr);
	   #endif
      return;
     }


// correct flag value in z->par1 (from v[3])

    if (!z->l1d)
      {
        mask = (s->v[10] & 0xff);                         // mask of bit
	    sign = (s->v[10] & 0x1000) ? 1 : 0;               // sign if bit SET, otherwise bit set is UNSIGNED 
	    val = z->par1 & mask;                             // is bit set ?
	    if (!sign) val ^= mask;                           // swop flag.....  
	    if (val)  val2 = 1; else val2 = 0;

        mask = (s->v[11] & 0xff);                         // mask of bit
	    sign = (s->v[11] & 0x1000) ? 1 : 0;               // sign if bit SET, otherwise bit set is UNSIGNED 
	    val = z->par1 & mask;                             // is bit set ?
	    if (!sign) val ^= mask;                           // swop flag.....  
	    if (val)  val2 |= 0x10;


        if ((size & 3) < 2) val = 5; else val = 9;    // main name (UU size)
        if (val2 & 1) val += 2;                       // UU to SU
        if (val2 & 0x10) val += 1;                       // UU to SU

        z->par1 = val2;
        if (csub && !csub->cmd)
          {
           // may need more checks than this 
           csub->xnam = val;
          } 
	    z->l1d = 1;
      }

    #ifdef XDBGX
    if (z->par1 & 1) DBGPRT(" SIGNED IN " ); else DBGPRT( " UNSIGNED IN " );	
    if (z->par1 & 0x10) DBGPRT(" SIGNED OUT " ); else DBGPRT( " UNSIGNED OUT " );		  
    #endif





// do search here...................


    d = (DDT *) chmem(&datch);      // next free block
    d->start  = adr;
  
    d = (DDT *) find(&datch, d);
  
    if (!d) d = add_ddt(adr, size);
  
    if (d)
      {
       d->func = 1;
       d->ssize = size;                // both cols in func are same size....
       d->p2 = size;
       d->from = csub->addr;
	   
         if (s->v[0] == 0x100000)
          { 
           // by user command
           d->cmd = 1;             // no changes
           if (s->v[14] == 2) d->ssize |= 4;  // signed out
           if (s->v[15] == 2) d->p2 |= 4;  // signed in
          }
	     else
		 {	
		    if (z->par1 & 1)    d->p2 |= 4;
            if (z->par1 & 0x10) d->ssize |= 4;  // signed out
	
		 }
	   #ifdef XDBGX
        DBGPRTN("ADD FUNC %x isize=%d osize=%d", adr, d->ssize, d->p2);
	   #endif	
      }

    s->done = 1;                           // for sig tracking only
      val = g_byte(s->v[3]);          // flag value 
	  val &= (~s->v[10]);
 	  val &= (~s->v[11]);
      upd_ram(s->v[3],val,1);
      upd_ramlist(s->v[3],1);
}



void tbplu (CSIG *z, SBK *nsc, SBK *blk)

{
   int adr, cols, val;

   SIG *s; 
   DDT *d;
   CSIG *x;
   SXT *csub;

   s = z->sig;
   csub = nsc->sub;

  #ifdef XDBGX
  DBGPRT("In TBPLU ");
  #endif
 
   x = find_csigp(csub);
   
   if (x)
   {
       // this subr has a par getter
       chg_subpars(csub, s->v[4], 2);                    // change reg to a WORD
       adr = get_subpars(csub,blk,s->v[9] | 0x100);      // first param

   }

   else
    adr = g_word(s->v[4]);

   if (adr < 255 && adr > minwreg)
       {
		 #ifdef XDBGX  
          DBGPRTN("TABLE addr embedded %x = %x", adr, g_word(adr));
	     #endif
        adr = g_word(adr);
       }

   // abandon scan ? add reg as a monitor ?

   adr = nobank(adr);
   adr |= datbank;                            //wbnk(datbank));

     // get extended block for tab or func commands

   if (!val_data_addr(adr))
      {
		#ifdef XDBGX  
        DBGPRTN("TABLE Invalid addr %x", adr);
		#endif
        return;
      }


   cols = g_byte(s->v[3]);

   #ifdef XDBGX
   DBGPRT("1 add %x cols (%x)=%x ", adr, s->v[3],cols);
   #endif
   
   if (s->v[3] < 0) cols = - s->v[3];          // fixed size

	//  c->dat1 = g_byte(c->par1);          // flag value 
	//	  c->par1 |= (t->v[10] << 8);         // and mask in next byte

 if (!z->l1d)
    {
     // mask in z->dat1, signed flag val in z->par1, if found..........
	 val = 0;	
	 if (z->dat1)
	   {        // mask set
    	 val = z->par1 & (z->dat1 >> 8);
       }

      if (csub && !csub->cmd)
        {
               // may need more checks than this 
		 if (val) z->par1 = 1; else z->par1 = 0;
		 csub->xnam = 13 + z->par1;
        }
     z->l1d = 1;
	}

   #ifdef XDBGX 
   if (z->par1) DBGPRT( " SIGNED " ); else DBGPRT( " UNSIGNED " );		  
   DBGPRT("add %x cols %d ", adr, cols);
   #endif
   
   if (cols <= 0 || cols > 64)
       {
		   #ifdef XDBGX
          DBGPRT(" inv colsize %x. set 1",cols);
		  #endif
        cols = 1;
       }

   
    // tables are BYTE so far, do an UPD_CMD, to replace directly

    d = (DDT *) chmem(&datch);      // next free block
    d->start  = adr;
  
    d = (DDT *) find(&datch, d);
  
    if (!d) d = add_ddt(adr, cols);
   
   
   if (d)
     {                          // multiple updates ?
      d->p1 = cols;
      d->p2 = 1;
      d->ssize = 1;
      if (z->par1) d->ssize |= 4;          // signed marker
      d->from = csub->addr;
      d->tab = 1;
     }

   s->done = 1;     // for display only
   #ifdef XDBGX
   DBGPRTN(0);
   #endif
}



void pencdx (SIG *x, SBK *b)
{
  CSIG *y, *z;                       // sub's sig chain


  if (!b) return;                     // safety - proc expected
   #ifdef XDBGX
   DBGPRT ("In pencdx");
   #endif
  y = init_csig(x,b);

  if (x->v[9]) y->dat1 = x->v[9];
  else y->dat1 = x->v[8];           // where data is (for decoding)
  y->enc = 1;

  #ifdef XDBGX
  DBG_CSIG(y);
  DBGPRTN(" + Set dat %x, recalc", y->dat1);
  #endif
  b->sub->sgclc = 1;                // set recalc....


// can't just stop here as we need a LEVEL from somewhere.....

z = find_csigp(b->sub);                // find prev param getter - may need more checks

 if (z)
   {
    y->lev1 = z->lev1;            // set encode level from param getter
	
	#ifdef XDBGX
     DBGPRTN(" lev=%d (<- %s)", y->lev1, z->sig->ptn->name);
	#endif
 
    if (x->v[9])               // has indirect assign at front....
    {
      y->lev1++;                // so assume one level extra
	  #ifdef XDBGX
       DBGPRT(" ++");
	  #endif 
    }
	#ifdef XDBGX
    DBGPRTN(0); 
	#endif
   }
}




void encdx (CSIG *z, SBK *nsc, SBK *blk)
{


  CSIG *c;
  SIG *s;
  SXT *csub;

  s = z->sig;
  csub = nsc->sub;

#ifdef XDBGX
  DBGPRTN("in encdx");
#endif

  if (z->disb)
    {
	  #ifdef XDBGX
        DBGPRTN ("encdx disb");
	  #endif	
      return;
    }


  // if direct, sig gets replaced in preprocess
    
   if (z->lev1 > 1)
      {                 // not yet, copy down a level, but get data dest if indirect
       c = find_csigp(csub);
       if (c && s->v[9]) 
        {  
		 #ifdef XDBGX
         DBGPRTN(" rpar %x",c->dat2);
		 #endif
         z->dat1 = c->dat2;
        }
       cpy_csig_levels(z,blk);
       return;
      }

}



void pfparp(SIG *s, SBK *b)
{
    //preprocess for fparp -- add multiple fparp here ?

    CSIG *c;

    if (!b) return;
    #ifdef XDBGX
     DBGPRT("In pfparp");
	#endif 

    c = init_csig(s,b);  

    c->dat1 = s->v[4];     // first data register move from 4 to 16
    c->par1 = s->v[3];     // no of pars
    c->gpar = 1;
    c->lev1 = s->v[1];
    c->lev2 = 0;
    
    if (s->v[12] == 2)  // variable patt match...
      {
       c->dat2 = g_byte(s->v[14]);
      }
    
  #ifdef XDBGX    
  DBG_CSIG(c);
  DBGPRTN(0);
  #endif
  }



void pfp5z(SIG *x, SBK *b)
{
     CSIG *c;

     if (!b) return;
     // only for 8065 ..... levels are actually FIXED by the   ldw   R40,[R20+2] style instead of popw

     #ifdef XDBGX
     DBGPRT("In pfp5Z ");
     #endif
     c = init_csig(x,b);           //,0);

     c->par1 = x->v[3];
     c->dat1 = x->v[4];
     c->gpar = 1;                  // this is a param getter
     c->lev1 = 2;                   // fiddle !!
  //   c->lev2 = 0;
     #ifdef XDBGX
     DBG_CSIG(c);
     DBGPRTN(0);     
	 #endif
}



void pfp52 (SIG *x, SBK *b)
  {
     CSIG *c;

     if (!b) return;
     // only for 8065 ..... levels are actually FIXED by the   ldw   R40,[R20+2] style instead of popw
   
   #ifdef XDBGX  
   DBGPRT("In pfp52 ");
   #endif
   
     c = init_csig(x,b);           //,0);

     c->par1 = x->v[3];
     c->dat1 = x->v[4];
     c->gpar = 1;                  // this is a param getter
     
     c->lev1 = x->v[15]/2;                   // temp fiddle !!
   
     #ifdef XDBGX  
      DBG_CSIG(c);
      DBGPRTN(0);
     #endif  
  }




void do_pgcd(CSIG *z, SXT *s)
{
  // add cadt for par getters, fpars and vpars, with check for encodes 
  // do in sets of 2, so that any subsequent encdx can be resolved into it.
// assume that sequential registers will form WORDS  

  int i,j;
  CADT *a;
  SXDT *d;
  CSIG *t;
  SIG *x;  

     d = get_sxdt(s,0);
     
     i = z->par1;       // no of bytes
     j = z->dat1;       // start register
  //   y = z->sig;

     while (i > 0)
      {
       a = add_cadt(&(d->cadt));            // at end of subr chain

       if (a)
        {  
         // check for following encoder first !!
         t = find_csige(z,j);       // find any encoders after THIS csig for j (data reg)

         if (t)
          {
          // add an encoded cadt instead  
           x = t->sig;
           a->enc = 3;                                   // ENC13 (default)

           if (*x->ptn->name == 0x32) a->enc = 4;        // ENC24 from name
   
           if (a->enc == 4 && x->v[3] == 0xf)  a->enc = 2;            // type 2, not 4
           if (a->enc == 3 && x->v[3] == 0xfe) a->enc = 1;             // type 1, not 3

           a->addr  = x->v[5];                            // 'start of data' base reg
           a->dreg  = j; 
           a->ssize = 2;                  // word based
           a->cnt   = 1;                    // zero words
           a->name  = 1;                    // find symbol names
           x->done = 1;

           #ifdef XDBGX
            DBGPRT(" enc cadt %d (%x) ",a->enc, z->caddr);
	       #endif
          } 
       else
        {
          // cnt = 1 and ssize = 1 by default
          if (i >=2 && (j & 1) == 0) {a->ssize = 2;}    // even start and at least 2 left    
         else a->cnt = 1;                          // otherwise 1 byte
         a->dreg = j;                             // data destination
         a->name = 1;                              // find sym names
         a->pget = 1;                              // this is a par getter
		 
		 #ifdef XDBGX
          DBGPRTN("pget cadt (%x)",z->caddr);
		 #endif 
        }
       }  
       i-= a->cnt*a->ssize;           // doesn't work if SIGNED
       j+= a->cnt*a->ssize;
      }

}


void fparp (CSIG *z, SBK *nsc, SBK *caller)
   {
    // fixed pars subr - THIS IS SUBSET OF VPAR ?   ONLY NEED ONE PROCESSOR ?
    /* VPAR IS
    * [15] first pop level
    * [14] first num pars (bytes)
    * [1] second pop level        [1-3] to match fpar
    * [2] second num pars
    * [3] param list (incremented) */

    // this is -
    // s->v[1] is level (1)
    // s->v[2] is no of bytes
    // s->v[3] is addr/reg which then may be used by encx

    // if levels > 1 then copy this sig to caller subr (via blk->caller)
    
 



    SIG *s;
	SXT *csub;
	
//    SXDT *d;

    s = z->sig;
	csub = nsc->sub;
 //   d = get_sxdt(csub,0);

    #ifdef XDBGX
     DBGPRTN("In fparp %x (%d)", caller->curaddr, z->lev1);
	#endif 

    if (z->lev1 > 1)                 // not yet, copy down a level
      {
       cpy_csig_levels(z,caller);
       return;
      }


    // split into sets of 2 bytes for a possible encode to follow
    // dat1 is param list start, par1 is number. (dreg is start in each CADT)

    if (!z->l1d)
    {
        
     if (s->v[12] > 1) 
      {
        int m,v;
        
        csub->nargs2 = s->v[12];
        
        if (s->v[13] == 2)  // bit jump
        {
        if (s->v[11] & 0x100) v = 0; else v = 1;      // mask value
        m = s->v[11] & 0xf;                    // mask

        if ((z->dat2 & m) == v)  z->par1 += s->v[12];
        }
      #ifdef XDBGX
       DBGPRT("init z");
      #endif

      }        
        
        
        
     do_pgcd(z,csub);

     if (csub->nargs2) z->par1 -= csub->nargs2; 

 
     set_size(csub);
     s->done = 1;
     z->l1d = 1;
	 
	 #ifdef XDBGX
	   DBGPRT("subr %x add cadt (%x)",csub->addr,caller->curaddr);
       //PRT_CADT(d->cadt);
       DBGPRTN ("  fparp done");
	 #endif  
    }

 //   get_subpars(csub,blk,0);

   }


void feparp (CSIG *z, SBK *nsc, SBK *caller)
   {
    // fixed pars subr with embedded encode
 
    // s->v[1] is level (1)
    // s->v[2] is no of bytes
    // s->v[3] is addr/reg which then may be used by encx

    // if levels > 1 then copy this sig to caller subr (via blk->caller)
    
    SIG *s;
    SXDT *d;
    CADT *a;
	SXT *csub;
    int i;
    
    s = z->sig;
	csub = nsc->sub;
    d = get_sxdt(csub,0);

    #ifdef XDBGX
     DBGPRTN("In feparp %x (%d)", caller->curaddr, z->lev1);
	#endif 

    if (z->lev1 > 1)                 // not yet, copy down a level
      {
       cpy_csig_levels(z,caller);
       return;
      }


    // split into sets of 2 bytes for a possible encode to follow
    // dat1 is param list start, par1 is number. (dreg is start in each CADT)

    if (!z->l1d)
    {
     // encoded par is FIRST, then ordinary par.
   
      d = get_sxdt(csub,0);
     
      a = add_cadt(&(d->cadt));            // at end of subr chain
      if (a)
       {  // encode
        a->enc = 4;   
        if (s->v[15] == 0xf)  a->enc = 2;            // type 2, not 4
        a->addr  = s->v[11];                         // 'start of data' base reg
        a->dreg  = s->v[4]; 
        a->ssize = 2;                  // word based
        a->cnt   = 1;                    // zero words
        a->name  = 1;                    // find symbol names
       }
 
      i = s->v[3];         // total par count  // cnt = 1 and ssize = 1 by default
      i -= 2;               // encode from above;
      
      a = add_cadt(&(d->cadt));            // at end of subr chain
      if (a)
        {           // standard par
         a->dreg = s->v[6];                        // data destination
//         a->name = 1;                              // find sym names
         a->pget = 1;                              // this is a par getter
//         if (i & 1) 
//           {
//            a->ssize = 2;
//            a->cnt = i/2;
//           }
//         else
          { 
            a->ssize = 1;
            a->cnt = i;
          }    // even start and at least 2 left   
 
        }  // if (a) 
    } 
        

     set_size(csub);
     s->done = 1;
     z->l1d = 1;
	 
	 #ifdef XDBGX
	    DBGPRT("subr %x add cadt (%x)",csub->addr,caller->curaddr);
        //PRT_CADT(d->cadt);
        DBGPRTN ("    feparp done");
	 #endif	
    }


void pvparp (SIG *x, SBK *b)
{
  CSIG *c;

  if (!b) return;

  #ifdef XDBGX
  DBGPRT("In pvparp ");
  #endif

  c = init_csig(x,b);

  c->dat1 = g_word(x->v[9]);             // address of 2nd level data (assumes some emulation...)
  c->gpar = 1;                     // x->v[3]
  c->lev1 = x->v[1];
   
  c->lev2 = x->v[2];              //p->lv2];
  c->lev1 += c->lev2;

  #ifdef XDBGX
    DBG_CSIG(c);
    DBGPRTN(0);
  #endif	
  // num pars not available yet.....
  
}

void pvpar5 (SIG *x, SBK *b)
{
  CSIG *c;

  if (!b) return;

  #ifdef XDBGX
  DBGPRT("In pvpar5 ");
  #endif

  c = init_csig(x,b);

  c->dat1 = g_word(x->v[9]);             // address of 2nd level data (assumes some emulation...)
  c->gpar = x->v[3];
  c->lev1 = 2;
   
  c->lev2 = 1; 
 // c->lev1 += c->lev2;

  #ifdef XDBGX
    DBG_CSIG(c);
    DBGPRTN(0);
  #endif	
  // num pars not available yet.....
  
}



void vparp (CSIG *z, SBK *nsc, SBK *blk)
  {
     /* variable args at various levels
      * a double level param getter
     [15] first pop level
     [14] first num pars (bytes)
     [1] second pop level        [1-3] to match fpar
     [2] second num pars
     [3] param list (incremented) */

   // current value of v[9] done in preprocess
   // safe to use addr here (used with offset and encode rbases)

   // fix this first and then .......

   CADT *a;      //, *n;
   SIG *s;
   SXDT *d;
   SXT *csub;
 //  CSIG *c;
 //  int i,j;

   csub = nsc->sub;
   d = get_sxdt(csub,1);
   s = z->sig;

   #ifdef XDBGX
     DBGPRT("In Vparp %x L1=%d L2=%d P=%x", blk->curaddr, z->lev1, z->lev2, z->par1);
     //  if (z->l1d ) DBGPRT (" L1 Done");
     //  if (z->l2d ) DBGPRT (" L2 Done");
     DBGPRTN(0);
   #endif


   if (z->lev2 > 1)          // >1 not this proc yet, copy to caller
      {
       cpy_csig_levels(z,blk); 
       return;
      }
 
   if (z->lev2 == 1)
     {                  // process at this level (numof pars reqd)
       if (!z->l2d)
        {                 // not processed yet - add CADT    
          a = add_cadt(&(d->cadt));
          if (a)                                 // safety
           {                                     // add LOCAL param as a CADT    (this is number of pars to get from caller)
            a->ssize = 1;                       // NOT redundant (as rewrite)
            a->cnt = s->v[4];                  // num of pars first level
            a->dreg = s->v[5];                  // FIRST data location (for first level)
            set_size(csub);
			#ifdef XDBGX
              DBGPRTN("sub %x add cadt L=%d", csub->addr, csub->nargs1);
              //PRT_CADT(d->cadt);
			#endif  
            z->l2d = 1;                // local level done  ->done =1;
            s->done = 1;
           }
        }
       z->par1 = get_subpars(csub, blk, 1);     // get any subpars when l2 = 1 - this gets TARGET values !!
	   #ifdef XDBGX
         DBGPRTN(" VPARP Varg = %x [%x]", z->par1, blk->nextaddr);
	   #endif	 
    }

   if (z->lev1 > 1)         // >1 not pars for this proc yet, copy to caller
      { 
       cpy_csig_levels(z,blk);        // change this for multiple fpars ?
       return;
      }



   if (z->lev1 == 1)
    {
    if (!z->l1d)
    {
     do_pgcd(z,csub);               // must add an 'assume consecutive' !
     set_size(csub);
     s->done = 1;
     z->l1d = 1;
	 
     #ifdef XDBGX  
       DBGPRT("subr %x add cadt R (%x)",csub->addr,blk->curaddr);
       //PRT_CADT(d->cadt);
       DBGPRTN(0);
	 #endif  
 
    } 
   z->dat2 = get_subpars(csub, blk, 1);        //TEST - first param. seems to work for encdx replace (later on)
    }
    // get subpars ?
 }  


void grnfp(CSIG *z, SBK *nsc, SBK *blk)
{
  CSIG *c;
  SXT *csub;
 // SIG *s;



  if (z->l1d)
     {
		#ifdef XDBGX  
        DBGPRTN("GRNF done");
		#endif
        return;
	 }

  csub = nsc->sub;
  #ifdef XDBGX 
  DBGPRTN("In GRNF subr %x (%d) for %x", csub->addr, csub->nargs1, blk->curaddr );
  #endif

  //  extra POP before param getter.  Increase level1 by no of pops
  // NB. this currently pops but then calls getpar LOCALLY so only ups l1, NOT l2

  c = get_csig(csub);

  while (c)
  {
    if (c != z)
     {      // skip GRNF sig itself
       c->lev1 += z->lev1;
	   #ifdef XDBGX 
        DBGPRTN("Sig %s +lev = %d and %d",c->sig->ptn->name, c->lev1, c->lev2);
	   #endif	
     }
    c = c->next;
  }

z->l1d = 1;
z->sig->done = 1;

}


void ptimep(SIG *x, SBK *b)
{
    // don't have to have a subr for this to work, so can do as
    // a preprocess, but logic works as main process too (but only once)


 LBK *k;
 CADT *a;
 int start,ofst, val,z;

 if (x->done) return;
 
 #ifdef XDBGX 
  DBGPRTN("in ptimep");          //move this later
 #endif 
  
 if (b)                        // may be sub, but possibly not....
  {
    init_csig(x, b);
    #ifdef XDBGX   
     DBGPRTN ("%x Add csig to sub %x ",b->curaddr, b->sub->addr); 
    #endif  
  }

  x->v[0] = g_word(x->v[1]);

 // size in v[14].  do a 'find end' for the zero terminator
 // cmd size (Y or W) is (2 or 4)=byte (3 or 5)=word

   start = x->v[0]; 
   start |= datbank;
   z = x->v[14];
   
   #ifdef XDBGX
    DBGPRT("Timer list %x size %d", x->v[0]-1, z+3);
    //if (s) DBGPRT(" Sub %x", s->addr);
    DBGPRTN(0);
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

    k = set_cmd(start, ofst, C_TIMR);    // this basic works, need to add symbols ?....
    if (k) a = add_cadt(&k->adnl);
    if (a) {a->ssize = z; a->name = 1;}

  x->done = 1;
}



 /* 
 * 
 * void set_time(CPS *cmnd)
 {
  LBK *blk;
  int val,type,bank;
  int xofst;
  short b, m, sn;
  SYT *s;
  CADT *a;
  char *z;

  blk = set_cmd (cmnd->start,cmnd->end,cmnd->fcom,1);
  set_adnl(cmnd,blk);
  DBGPRTN(0);
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



void do_substructs (SBK* nsc, SBK *caller)
  {
   // nsc is new block, for sub 
   // caller is caller of nsc (and sub), which is where args are
   // and is NOT always nsc->caller !!

   SXT *sub; 
   CSIG *z;                  // for sub's sig chain
   int args;

  if (anlpass >= ANLPRT) return;

   #ifdef XDBGX
   DBGPRT("substructs ");
   #endif
 
 if (!nsc)
   {
	 #ifdef XDBGX
     DBGPRTN("nsc Null");         // no called sub - should never happen
	 #endif
     return;
    }
 
 sub = nsc->sub;
 args = sub->nargs1;              // remember args before any new sigs
 
 if (!nsc->sub)
   {
	#ifdef XDBGX
    DBGPRTN("Sub Null");         // no called sub - should never happen
	#endif
    return;
   }

 //if (sub->cmd) DBGPRTN("Sub CMD");
   #ifdef XDBGX
   DBGPRTN("at %x for sub %x", caller->curaddr, sub->addr);
   #endif
  z = get_csig(sub);                // any sigs in called sub ?

  // nsc->sub is target sub.  Do_sigs may add sigs at different levels for fpar,vpar, GRNF, etc
  // will be found when sub scanned

 if (sub->sgclc && sub->sdat) freecadt(&sub->sdat->cadt);         //recalc set - clear ALL cadts so far

  while (z)                          // process each sig in order along chain
   {
    if (!sub->cmd || z->sig->ptn->spf)    // not cmd or specific override
      {  
       if (sub->sgclc) z->l1d = 0;      // reset 
       z->sig->ptn->sig_pr(z,nsc,caller);
      } 
    z = z->next;
   }

   sub->sgclc = 0;
 
   get_subpars(sub,caller,0);                // for data markers...
   caller->nextaddr += sub->nargs1;
   if (args != sub->nargs1) fix_scans(sub->addr);   // rescan blocks if params have changed.
  
      #ifdef XDBGX
    // DBGPSUB(sub,caller);
     DBGPRTN("Exit substr, next = %x (%d)", caller->nextaddr, sub->nargs1);
	#endif 
   
 
}




INST *get_prev_instance(INST *x)
 {

  int i, ofst;

  ofst = get_prev_opstart (x->ofst);        // previous opcode

  i = lpix;                                // current index

  while (1)
    {
     i--; 
     if (i < 0) i = PILESZ-1;       // loop

     if (i == lpix)             // not found
       {

        x = 0;
        break;
       }

     x = instpile + i;
     if (x->ofst == ofst) return x;       // found
  }

  return 0;

 }


JLT *add_jump (SBK *b, int type)
{
  JLT *j,*z;
  OPS *o;               // assumes destination is in curops[1].addr
  
  int ix, a;

  if (anlpass >= ANLPRT) return NULL;

  o = curops + 1;

  if (!val_jmp_addr(o->addr))
    {
	 #ifdef XDBGX
     DBGPRTN("Invalid jump to %x from %x", o->addr, b->curaddr);
	 #endif
     return NULL;
    }

  j = (JLT *) chmem(&jpfch);
  j->from   = b->curaddr;
  j->jcnt   = b->nextaddr - b->curaddr;               // jump size
  j->to     = o->addr;
  j->jtype  = type;

  if (!bankeq(j->to, j->from))  j->bswp = 1;        // bank change
  else
  if (j->jtype > 0 && j->jtype < S_CSUB && j->from >= j->to)
   {   // swop and mark as backwards jump for loops - not for sub calls or rets
	  j->back = 1;
      j->from = o->addr;
      j->to  = b->curaddr;        // reverse order so jump is always low->high
  }

  // check for jump to a ret opcode.
  // keep orig type (for now) for debug.
 
     a = g_byte(j->to);
	 if (a == 0xf0 || a == 0xf1) 
       {
        j->retn = 1;
        j->to = j->from + j->jcnt;  
	   }	

  if (!j->jtype) j->retn = 1;          // return jump (flag can be inherited to other jumps)


  // now insert into the two jump chains
  
  ix = bfindix(&jpfch, j);      // FROM chain

  if (ix < jpfch.num)
    {                            // check for duplicate jump
     z = (JLT *) jpfch.ptrs[ix];
     if (j->to == z->to && j->from == z->from && j->jtype == z->jtype) return z;
    }

  chinsert(&jpfch, ix, j);                    // do insert in FROM chain
  ix = bfindix(&jptch, j);                    // and in TO chain
  chinsert(&jptch, ix, j);

  return j;
}


int chk_hchain(SBK *blk,  SBK *c)
{
	// does c exist anywhere in 'blk' chain or its subordinates ?
	
  HOLDS *x;
  int ans;
  ans = 0;
   
  if (!blk) return 0;    //empty chain
 
  x = blk->hchn;
  while (x)	
    {
     if (x->hblk == c) 
	    {
		 ans = 1;	
     	 #ifdef XDBGX
         DBGPRTN("DEADLOCK %x and %x ", blk->start, c->start);
		 #endif
        }
	 else ans = chk_hchain(x->hblk,c);         // check subs chain too....

	 if (ans) break;
	 x = x->next;
    }   
return ans;	
}





int add_hblk (SBK *newsc, SBK *caller)
  {
   // add 'caller' to 'newsc' chain and put 'caller' on hold
   // make sure 'newsc' does not have 'caller' in its chain already.
   // or caller has new ??
   
   // have to do the RECURSIVELY as chain is a full tree structure

   HOLDS *x, *z;

   if (newsc == caller) return 0;                    // safety - same block
   if (newsc->scanned)  return 0;                    // block has been scanned, so no hold necessary

   // if newsc in caller's tree, would cause a deadlock (probably a backwards jump)

   if (chk_hchain(caller, newsc))  return 0; 
      
   // add caller to newsc chain
   // if it's there already, then reset held status and exit
 
   x = newsc->hchn;
   z = 0;

   while (x)	
      {
       if (x->hblk == caller) 
		   {
			 caller->held = newsc;      // was 1
             return 2;                  // already in chain
		   } 
	   if (!x->hblk && !z)  z = x;      // remember first zero block
       x = x->next;
	  }

   // caller is not in newsc chain, so use an empty slot, or add it 

   if (!z)
     {
       // no empty slots, add a new entry to the chain

       z = (HOLDS *) mem (0,0,sizeof(HOLDS));
       memset(z,0,sizeof(HOLDS));                         // clear block
 
       if (!newsc->hchn) newsc->hchn = z;                // start new chain
       else
         {
	      x = newsc->hchn;
          while(x->next) x = x->next;                     // get to end of chain
          x->next = z;                                    // add to end
         } 
     } 
   z->hblk = caller;                                      // add caller to chain
   caller->held = newsc;
   return 1;
  } 

  
int do_hold(SBK *newsc, SBK *caller)
 {
  // return 1 if on hold, zero otherwise.
  // caller is put on hold to await scan of newsc.
  // if newsc has already been scanned, don't require a hold.
  
  int ans;
  
  ans = 0;                    // no hold;
 
  if (!newsc || newsc->scanned) return 0;        // scan block invalid, or scanned already (don't need hold)
  if (!caller) return 0;                         // safety, probably redundant
  
  get_scanpars(caller);        //&caller->pars);                   // doesn't work without this...reqd for scanned (= reused) subr with new params
  ans = add_hblk(newsc, caller);                 // add caller to newsc list, and check for deadlocks

  #ifdef XDBGX
   if (ans == 2) DBGPRT("2");
   if (ans) DBGPRTN("HOLD %x at %x -> %x", caller->start, caller->curaddr,newsc->start);
  #endif
  return ans;
 }



void do_sjsubr(SBK *caller, int type)
{
  /* do subroutine or jump
  * new scanned block (or found one) may become holder of this block
  * for subroutine special funcs (and any args)
  */

  SBK *newsc;                        // new scan, or temp holder
  int addr;

  if (PMANL) return;                // no scans, use default 
  if (anlpass >= ANLPRT) return;
   
  addr = curops[1].addr;

  switch (type)                                // requested scan type
    {

     default:
	   #ifdef XDBGX
       DBGPRT("Error at %x unknown type =%d", caller->curaddr,type);
	   #endif
       break;
        
     case S_CJMP:                        // conditional jump

        /* don't necessarily need to add another scan here as code CAN continue, but
         * necessary to get any alternate addresses etc. via parameter saves (e.g. for tables)
		 * backwards conditionals are probably always local loops
		 * duplicates screened out by add_scan
		*/
        
          add_scan (addr,type, caller);      // where this continues
          break; 

     case S_SJMP:                       // static jump
	    /* backwards static jumps are also probably a code loop is mall, but some bins
		 * have a common 'return' point, so can't assume this.
		 * also a static jump can go to a subroutine start, as per signed and
		 * unsigned function and table lookups, and so on.
		 * for this reason, all static jumps must go on HOLD until target processed
		 * just like a subroutine CALL
		 */ 
 
     case S_CSUB:                       // std subr
     case S_JSUB:
	 
           newsc = add_scan (addr,type,caller);           // nsc = new or zero if dupl/invalid
           if (do_hold(newsc,caller)) break;              // this can hold, or not and fall through.
    }        // end switch           

    if (type == S_CSUB && !caller->held)  do_substructs(newsc,caller);       // always check for subr arguments 

}


/***********************************************
 *  get label name for pc and assign operand parameter
 *  returns:    str ptr if found     0 otherwise
 *  get op value too
 ************************************************/

void op_val (int pc, int ix)
  // PC is SIGNED - so can be NEGATIVE
  // negative ix signifies use of CODE bank
  // assumes read size already set in operand cell (OPS)
  // NB. sym names and special fixups in print phase

  {
  int bank;
  OPS *o;

  if (ix < 0)
   {
    bank = opbank(codbank);        // codebank for jumps
    ix = -ix;
   }
  else bank = opbank(datbank);   // data bank otherwise

  o = curops+ix;
  o->stk = 0;
  o->addr = pc;
  o->rbs = 0;

  if (o->addr < 0x100)   o->addr += rambank;     // always zero if not 8065
  if (o->addr >= PCORG)  o->addr |= bank;        // add bank

  if (o->rgf)
     {                                          // if register (default)
      o->addr &= 0xff;                          // safety - should be redundant
      o->addr += rambank;                       // rambank always zero if not 8065
      if (o->addr == stkreg1)  o->stk = 1;
      if (o->addr == stkreg2)  o->stk = 1;      // 'This is stack reg' flag
      o->rbs = get_rbt(o->addr,curinst->ofst);
     }

  if (o->imd || o->bit) o->val = pc;            // immediate or bit flag, so pc is absolute value
  else o->val = g_val(o->addr,o->ssize);        // get value from reg address, sized by op

  if (o->rbs) o->val = o->rbs->val;            // get value from RBASE entry. safety, should be same, but....
  
  // NB. indirect and indexed etc cases  handled elswehere
}



void op_imd (void)
{
  // convert op[1] to imd
  OPS *o;

  o = curops+1;
  o->imd  = 1;
  o->rgf  = 0;
  o->rbs  = 0;
  if (rambank) o->addr -=  rambank;               // remove any register bank
  o->val  = o->addr;
}

void do_vect_list(int start, int end)
 {
  int pc, vcall;
  LBK  *s;
  CADT *c;
  DDT  *d;

  if (anlpass >= ANLPRT) return;

  pc = start;

  d = add_ddt(pc,2);
  if (d) {d->vect = 1; d->dis = 1;}

  // would need to do PC+subpars for A9L proper display
   #ifdef XDBGX
    DBGPRTN("do vect list %x-%x", start,end);
   #endif
  for (pc = start; pc < end; pc +=2)
    {
     vcall = g_word (pc);                            // address from list
     vcall |= opbank(codbank);                       // assume always in CODE not databank
//  ?     add_ddt(vcall);
      #ifdef XDBGX
      DBGPRT(" v %x ",vcall);
	  #endif
     add_scan (vcall, S_CSUB,0);     // vect subr (checked inside cadd_scan)
    }


  s = set_cmd (start, end, C_VECT);
  if (s)
    {
     // check banks here......
     if (datbank != codbank)
      {
       c = s->adnl;
       if (!c) c = add_cadt(&s->adnl);
       c->foff = 1;
       c->ssize = 0;         // no size
       c->addr = codbank >> 16;   // add current codbank
      }
    }
	 #ifdef XDBGX
     DBGPRTN("END vect list");
	 #endif
 }

/******************************************
attempt to decode vector or vector list from push word
probably need to split off the vect list (for multibanks)
****************************************/

int check_vect_list(int start, int sz, int retaddr)
{
   int addr, i, score, lowcadd, end;
   SBK *s, *t;
   LBK *l, *k;

   // assume bank is correct in start
   if (nobank(start) < PCORG) return 0;


   end = start + 0x300; 
   score = 0;                     // max score
   lowcadd = maxadd(curinst->ofst);              // bank max for call addresses - multibank ?

   // check start against curinst+cnt to see if an extra offset needs to be added....
   //this needs more checks as well

   //allow for sz = 0 !!
   #ifdef XDBGX
    DBGPRTN("in check_vect %x sz=%x R=%x lc=%x", start, sz, retaddr, lowcadd);
   #endif
   if (!sz) sz = 0x300;
   t = (SBK*) chmem(&scanch);
   k = (LBK*) chmem(&auxch);


   for(i = start; i < end; i+=2)       // always word addresses  (score > 0?)
     {
      if (i >= start+sz && score < 10)
        {
		 end=i-1;
	     #ifdef XDBGX
          DBGPRTN("end %x", end);
	     #endif 
         break;
		}

     if (i >= lowcadd)
       {
		 #ifdef XDBGX
         DBGPRTN("hit lc_add %x", i);
		 #endif
         break;
	   }
     
     t->start = i;  // for check of SBK
     k->start = i;  // for check of auxcommands
     
     s = (SBK*) find(&scanch,t);
     if (s)
       {
		 #ifdef XDBGX
          DBGPRTN("scan %x t%d", i, s->ctype);
         #endif
         end = i-1;
         break;
       }

     l = (LBK *) find(&cmdch,k); 
     if (l)
       {
         if (l->fcom > C_WORD)
           {
			 #ifdef XDBGX   
              DBGPRTN("cmd %x %d",i,l->fcom);
			 #endif 
             end = i-1;
             break;
           }
	   }

     addr = g_word (i);
     
     addr |= g_bank(retaddr);    // bank from return addr

     if (!val_jmp_addr(addr))
        {
         score +=2;
		 #ifdef XDBGX
          DBGPRTN("iadd %x (%x)",i, addr);
		 #endif 
         addr = 0;
        }

     if (addr)
     {
       // lowest call address - but this doesn't work for multibanks !!
       if (addr > start && addr < lowcadd) {lowcadd = addr; }


       l = (LBK *) find(&auxch, k);
       if (l)
        {
         score +=2;
		 #ifdef XDBGX
          DBGPRTN("Aux %x=%x",i, addr);
		 #endif 
        }

       if ((addr & 0xff) == 0)     // suspicious, but possible
          {
         score ++;
		 #ifdef XDBGX
          DBGPRTN("Suspicious val %x=%x",i, addr);
		 #endif
         }
       // more scores ??
     }
    else if (i == start) start +=2;
   }
  #ifdef XDBGX
   DBGPRTN("Check_vect %x-%x (%d)|lowc %x|score %d", start, i, i-start, lowcadd,score);
  #endif
  if (lowcadd < end) end = lowcadd-1;

  addr = i-start;
  if (addr >0)
   {
     score = (score*100)/addr;
	  #ifdef XDBGX
     DBGPRT ("FINAL SCORE %d",score);
      if (score < 10)   DBGPRT ("PASS"); else DBGPRT("FAIL");
     DBGPRTN(0);
	 #endif

     if (score < 10) do_vect_list(start,end);
     return 1;
   }
  return 0;
}






 

 void vct1  (SIG *x, SBK *b)
  {

   int start, end;

   if (anlpass >= ANLPRT)  return;
   // assume bank now included in all addresses

     // a LIST, from indirect pushw

   #ifdef XDBGX
    DBGPRTN("In vct1 from %x for %s (%x)",x->ofst, x->ptn->name, x->v[4]);
   #endif
   
   start = x->v[4] ;

   if (start > PCORG)
     {                                // ignore register offsets

      if (start > x->ofst && start <= x->xend) start = x->xend+1;        //overlaps , so list must start at [1] or more
      if (start & 1) { start++;}                     // odd address not allowed

      end = start + x->v[1];

      // but size may be unknown !!
      #ifdef XDBGX
       DBGPRTN("Do LIST %x-%x",start, end);
      #endif
	 
      if (x->v[6]) end = x->v[6];
      else
      if (x->v[8]) end = x->v[8]; 
      else   
      end = start;    
            
      check_vect_list(start,x->v[1], end);
     }

     // Immediate, return addr pushed before list

   if (x->v[6])
     {
      start = x->v[6];
      if (val_code_addr(start))
       {
		#ifdef XDBGX
         DBGPRTN("Do IMD1 %x",start);
		#endif
        add_scan (start,S_CSUB,0);          // treat as vect subr
       }
     }
     
   if (x->v[8])
     {
      start = x->v[8];
      if (val_code_addr(start))
       {
		 #ifdef XDBGX   
          DBGPRTN("Do IMD2 %x",start);
		 #endif 
        add_scan (start,S_CSUB,0);          // treat as vect subr
       }
     }

     x->done = 1;
  }






short find_fjump (int ofst)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 4 for 'else', 3 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JLT * j;
 // int i;

  if (anlpass < ANLPRT) return 0;


  j = (JLT*) chmem(&jpfch);
  j->from = ofst;
  j = (JLT*) find(&jpfch,j);         // jump from

  if (!j) return 0;

  if (j->retn) return 2;    // end of subr, check this FIRST
  if (j->cbkt) return 1;     // bracket - reverse condition in source
  return 0;
}


void find_tjump (int ofst)
{
  // find trailing jump, for brackets, matches fjump
  // prints close bracket for each jump found

  // need to put loop detection here ?


 JLT * j;
 short ix;

 if (anlpass < ANLPRT) return;
 if (!PSSRC) return;

 // not find, as needs to check more than one
  j = (JLT*) chmem(&jpfch);
  j->to = ofst;

 ix = bfindix(&jptch, j);     // a jump TO here

 while (ix >= 0 && ix < jptch.num)
  {
   j = (JLT*) jptch.ptrs[ix];
   if (j->to == ofst)
    {
     if (j->cbkt)
       {
       p_pad(cposn[2]);
       pstr ("}");
      }
     ix++;
    }
   else break;    //stop as soon as different address
  }
}



void pp_hdr (int ofst, const char *com, short cnt, short pad)
{
  char *tt;
  if (!cnt) return;
 
  newl(1);                                         // always start at a new line for hdr
  if (pad >= 0)
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

  p_pad(pad > 0 ? pad : cposn[0]);
  if (com) pstr ("%s",com);
  p_pad(cposn[1]);

}

/*************************************
get next comment line from file
move to combined bank+addr (as single hex)
**************************************/

short getcmnt (int *ofst, char **cmnt)
{
  short ans;
  int maxlen, rlen;
  int pcz;

  ans = 0;
  rlen = 0;
  maxlen = 0;
  pcz = 0;
  
  *cmnt = flbuf;               // safety

  if (fldata.fl[4] == NULL || feof(fldata.fl[4]))
   {
    *ofst = 0xfffff;          // max possible address with bank
    **cmnt = '\0';
    return 0;                 // no file, or end of file
   }

  // bank,pc

  while (ans < 1 && fgets (flbuf, 255, fldata.fl[4]))
    {
     maxlen = strlen(flbuf);   // safety check
     ans = sscanf (flbuf, "%x%n", &pcz, &rlen);    // n is no of chars read to %n
    }

  if (rlen > maxlen) rlen = maxlen;

  if (ans)
    {
      if (!numbanks  && pcz < 0xffff)  pcz += 0x80000;  // force bank 8

      *ofst = pcz;
      *cmnt = flbuf+rlen;
      while (**cmnt == 0x20) (*cmnt)++;     // consume any spaces after number
      return 1;
    }

  **cmnt = '\0';    //anything else is dead and ignored...
  return 0;

}

// incorrectly stacked comments cause filled spaces (cos of print...)


char* symfromcmt(char *s, int *c)
 {
   int ans,add, bit;
   SYT *z;

  if (!isxdigit(*s))
   {
    *c = 0;
    return 0;   // anything other than digit after special char not allowed
   }
   bit = -1;
   z = 0;

   ans = sscanf(s,"%x%n:%x%n",&add,c,&bit,c);

   if (!ans) return NULL;

   // add,bit  with add or add,bit allowed


   if (add < PCORG) {}                //add |= RAMBNK;
   else
   if (!numbanks && add <= 0xffff)
     {
      add |= 0x80000;
     }

   if (ans > 1) z =  get_sym (0,add, bit,0);

   if (!z)      z = get_sym(0,add,-1,0);

   if (z) return z->name;
   return NULL;
 }


 // could allow a wrap() here for multiline comments by design also.



// print indent line for extra psuedo code lines

void p_indl (void)
{
  if (gcol) newl(1);
  p_pad(cposn[2]);

}

/***************************
signature pattern detection
**************************/

// will only print max of 'size' bytes
// add CHECK for incorrect table or struct overlapping next cmd.

CADT *pp_lev (int ofst, CADT *a, int pad, int max)
{

 int pfw, val, end, i, lev;
 float fval;
 SYT* sym;

 if (max)
   end = ofst + max;
 else
   end = ofst + 64;      /* safety stop 64 bytes */
 
 lev = 0;
 while (a && lev < MAXLEVS)
  {
   if (ofst >= end) break;                 // safety - force exit   
   if (a->disb) { a = a->next; continue; } // skip this entry

   for (i = 0; i < a->cnt; i++)      // count within each level
    {
     if (i) pchar(',');
     sym = NULL;
     pfw = 1;                               // min field width
     if (ofst > end) break;                // safety - force exit
     if (pad)
       {                                   // for table layouts etc, pad entries
        pfw = a->pfw;
        if (!pfw) pfw = cafw[a->ssize];          // default to preset
        if (a->divd)   pfw+=4;                   // 4 extra for ".000"
       }

     val = decode_addr(a, ofst);               // val with decode if nec.  and sign
     if (a->name)
       {
         sym = get_sym(0,val,-1, ofst);    // syname AFTER any decodes - was xval
       }

     if (sym) pstr("%s",sym->name);
     else
     if (a->divd)
       {
        char *tx;
        int cs;
        fval = val/a->divd;
        cs = sprintf(nm,"%*.3f", pfw, fval);
        tx = nm+cs-1;
        while (*tx == '0') *tx-- = ' ';
        if (*tx == '.') *tx = ' ';
        pstr("%s", nm);
       }
     else
     if (a->dcm)
      pstr("%*d", pfw, val);   // decimal print
     else
        { // hex prt
         val = nobank(val);
         pstr("%*x", pfw, val);
        }
     ofst += casz[a->ssize];
     }
   lev++;
   a = a->next;
   if (a) pchar(',');
  }
return a;

}

//*********************************

int pp_text (int ofst, int ix)
{
  int cnt;
  short i, v;
  int xofst;
  LBK *x;
  x = (LBK*) cmdch.ptrs[ix];

  xofst = x->end;
  cnt = xofst-ofst+1;

  if (cnt > 16) cnt = 16;
  if (cnt < 1)  cnt = 1;

  pp_hdr (ofst, dirs[x->fcom].com, cnt,0);

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
// word and byte only - fiddle name position

int pp_wdbt (int ofst, int ix)
{
  CADT *a;
  char *tt;

  LBK *x;
  
  x = (LBK *) cmdch.ptrs[ix];

  a = x->adnl;

  if (!a)
   {       // add  block so can use pp_lev
    a = add_cadt(&(x->adnl));         // size 1 by default
    a->nodis = 1;           // don't print cadt in command prt
   }
  // always addnl now...
  if (x->fcom != C_BYTE) a->ssize = 2; 

  if (ofst + casz[a->ssize] > x->end+1)
    {
     // can't be a word if overlaps end, KLUDGE to byte
     a->ssize = 1;
     x->fcom = C_BYTE; 
  } 

  pp_hdr (ofst, dirs[x->fcom].com, casz[a->ssize],-1);
  pp_lev(ofst,x->adnl,1,0);

  // add symbol name in source code position

  tt = get_sym_name(0,ofst,0);

  if (tt)
    {
     p_pad(cposn[2]);
     pstr ("%s", tt);
    }

  return ofst+casz[a->ssize];
}


//***********************************

int pp_vect(int ofst, int ix)
{
  /*************************
  * A9l has param data in its vector list
  * do a find subr to decide what to print
  ***************/

  int val;
  char *n;
  CADT *a;
  LBK  *x;
  SXT * sub;
  int bank, size;

  x = (LBK*) cmdch.ptrs[ix];
  
  a = x->adnl;

  size = 0;
  val = g_word (ofst);

  if (a && a->foff && bkmap[a->addr].bok)
      bank = a->addr << 16;         //different bank specified in cadt
  else
     bank = g_bank(ofst);         // use bank from ofst
     
  val |= bank;

  sub = (SXT*) schmem();
  sub->addr = val;
  
  if (!find(&subch, sub)) size = 2;  // not correct for cal console etc
  
  if (ofst & 1)  size = 1;

  if (size)
      {
      pp_hdr (ofst, dirs[size].com, size,0);         // replace with WORD
      }
   else
    { pp_hdr (ofst, dirs[x->fcom].com, 2,0);
    size = 2;
    }
  n = get_sym_name (0,val,0);

  paddr(val);

  if (n)
     {
     p_pad(cposn[2]);
     pstr ("%s", n);
     }
  return ofst+size;
}

//  print a line as default

int pp_dft (int ofst, int ix)
{
  int i, cnt, sval;       // for big blocks of fill.....


  LBK *x;
  x = (LBK*) cmdch.ptrs[ix];

  cnt = x->end-ofst+1;

  sval = g_byte(ofst);            // start value for repeats
 
 for (i = 0; i < cnt; ++i)
    {
      if (g_byte(ofst+i) != sval) break;
    }

  if (i > 32)   // more than 2 rows of fill     PUT THIS BACK FOR MULTIBANKS (LARGE BLKS OF FILL)
     {
      newl  (2);
      pstr  ("## ");
	  paddr (ofst);
      pstr  ("to ");
      paddr (ofst+i-1);
      pstr  ("= %x (filler ?) ## ", sval );
      newl(2);
      return ofst+i;
    }

  if (cnt <= 0 ) cnt = 1;
  if (cnt > 16) cnt = 16;      // 16 max in a line


  if (ofst + cnt > x->end) cnt = x->end - ofst +1;

  if (i >= cnt)
     {
      pp_hdr (ofst, "???", cnt,0);
      return ofst+cnt;
    }

  if (cnt > 8) cnt = 8;        // data up to 8 for UNKNOWN

  pp_hdr (ofst, "???", cnt,0);

  return ofst+cnt;
}

// this works for actual structure - need to sort up/down etc
// looks like may be standard set...

int pp_timer(int ofst, int ix)
{
  int val,cnt;
  int xofst;
//
//  int bank;
  SYT *sym;
  short bit, i;
  CADT *a;
  LBK *x;
  x = (LBK*) cmdch.ptrs[ix];



  sym = 0;
  a = x->adnl;
  if (!a)  a = add_cadt(&(x->adnl));       // just in case


  xofst = ofst;


  val = g_byte (xofst++);  // first byte
  if (!val)
    {
      pp_hdr (ofst,dirs[C_BYTE].com,1,0);    // end of list
 
      return xofst; 
     }


  cnt = (val & 1) ? 3 : 1;     // int or short entry
  pp_hdr (ofst, dirs[C_STCT].com, cnt+casz[a->ssize],0);
  pstr("%2x, ", val);
  val = g_val (xofst, a->ssize);
  if (a->name) sym = get_sym(0,val,-1, xofst);           // syname AFTER any decodes
  if (sym) pstr("%s, ",sym->name);
  else pstr("%5x, ",val);
  xofst += casz[a->ssize];
  
  if (cnt == 3)
   {                // int entry
    i = g_byte (xofst++);
    bit = 0;
    while (!(i&1) && bit < 16) {i /= 2; bit++;}          // LOOP without bit check !!
    val = g_byte (xofst);

     if (a->name) {sym = get_sym(0,val,bit,xofst);
     if (!sym) sym = get_sym(0,val,-1,xofst); }
    if (sym) pstr("%s, ",sym->name);
    else  pstr(" B%d_%x",bit, val);
	xofst++;
	
   }


  return ofst+cnt+casz[a->ssize];
}



void p_op (OPS *o)
{
 // bare single operand print

 if (o->imd) pstr ("%x", o->val & camk[o->ssize]);      // immediate (with size)
 if (o->rgf) pstr ("R%x",o->addr);                      // register, already 0xff | rambank
 if (o->inc) pstr ("++");                               // autoincrement
 if (o->bit) pstr ("B%d",o->val);                       // bit value 
 if (o->adr) paddr(o->addr);                            // address - jumps
 if (o->off) pstr ("%x", o->addr & camk[o->ssize]);     // address, as offset (inx)

}


void p_oper (int ix)
{
  // bare op print - top level

  OPS *o;

  ix &= 0x3;                      // safety check
  
  o = curops+ix;

  if (o->bkt)  pchar ('[');
  p_op (o);

  if (curinst->inx && ix == 1)        // indexed op - use op [0] as well.
    {                                // indexed ops are SIGNED offset, in [0]
      pchar ('+');
      p_op (curops);
    }
  if (o->bkt)  pchar (']');
  return;
}


int p_sc (OPS *o)
{
// single op print for source code from supplied instance
// ans = 1 if printed something. 0 otherwise

int v;

 if (o->sym)  {pstr (o->sym->name); return 1; }

 if (o->imd)                               // immediate operand
    {
     pstr("%x", o->val & camk[o->ssize]);
     return 1;
    }

 if (o->rgf)
    {
     if (o->addr || o->wsize) pstr ("R");           // non zero reg, or write op
     pstr ("%x", o->addr);
     if (o->inc) pstr ("++");
     return 1;
    }

 if (o->bit) {pstr ("B%d_", o->val); return 1;}

 if (o->adr)
    {
     paddr(o->addr);
     return 1;
    }
    
if (o->off)
    {
     v = o->addr;
     if (v < 0)  v = -v;
     pstr ("%x", v & camk[2]);  
     return 1;
    }
    
    
return 0;
}


int find_last_psw(INST **z)
 {
   // finds last PSW changing opcode for printout
    int ans, cnt;
    INST *x;
    const OPC *opl;

    x = curinst;
    cnt = 0;
    ans = -1;           // not found
    *z = 0;             // not found
 
    while (x && cnt < 16)
     {
      x = get_prev_instance(x);
      cnt++;                         // test max 16 opcodes

       // probably need more checks in here.
 
      if (!x) break;
      opl = opctbl + x->opcix;

      if (opl->sigix == 17)  break;              // call
      if (opl->sigix == 21)  break;              // set or clear carry
      
      if (opl->pswc)                  // sets PSW, allow target
         {
           *z = x;                   // instance found
           ans = opl->wop;           // return wop (zero for compare)
		   if (!x->opr[opl->wop].addr) ans = 0;  // but not if wop is R0
           break;
         }
     }

    return ans;
 }




void p_opsc (INST *x, int ix)
{
 // source code printout

  OPS *o, *b;

   if (!x)
   {
       pchar('0');
       return;
   } 

  o = x->opr + ix;                            // set operand

  if (x->inx && ix == 1)
    {                                        // indexed ops are SIGNED offset
      b = x->opr;
      if (o->bkt) pchar ('[');
      if (p_sc (o))
        {
         pchar( (b->addr < 0) ? '-' : '+');      // if first op printed
        }
      p_sc (b);
      if (o->bkt) pchar (']');
      return;
    }                     // end indexed

  if (o->bkt)  pchar ('[');
  p_sc (o);
  if (o->bkt)  pchar (']');
  return;
}

void do_comment(char *c, INST *l)
{
  char *x;
  int z, col;

   if (!c)  return;                               // safety
   if (*c != '|' ) p_pad(cposn[3]);               // '|' = newline at front, so no pad

     while (*c)
      {
       switch (*c)
        {
          // autocomment operands - a fiddle really ....

          case 1:
	      case 2:
	      case 3:
              if (l) p_opsc (l,*c);
    	      break;


         case '|' :                                // newline
           newl(1);
           break;

         case '~' :             // runout 80
           if (!gcol)
             {                // newline just issued ( '|')
              x = c+1;        // next char is the runout one
              for (z = 0; z < 80; z++) pchar(*x);
              c++;
             }
           else
            pchar(*c);
           break;

         case '@' :                                // '@' symname with padding, drop through
         case '$' :                                // '$' symname with no padding
           x = symfromcmt(c+1, &z);                // c+z+1 seems to work
           col = gcol;                             // remember this column
           if (x) pstr("%s",x);                                // if (x) pstr(x);
           if (*c == '@') p_pad(col+21);           // pad 21 chars from col (or not)
           c = c+z;                                // skip the sym numbers
           break;

         case '\n' :                               // '\n',  ignore it
		 case '\r' :
           break;

         case '^' :                                // '^' wrap comment, or pad to comment column
           if (lastpad >= cposn[3]) wrap();
           else p_pad(cposn[3]);
           break;

         default:
           pchar(*c);                              // print anything else
           break;
        }
       c++;
      }
}



void pp_comment (int ofst, int nl)
{
 
  // ofst , for convenience, is for the START of a statement so comments
  // up to ofst -1 should be printed....
  
  //nl is newlines, but if comment does not have a '|' (newline) at front,
  // print this first to allow a trailing cmt and then a block cmt


  if (anlpass < ANLPRT) return;
  

  
  if (ofst < 0) ofst = 1;
  if (ofst > maxadd(ofst)) ofst = maxadd(ofst)+1;         // safety checks
 
// do any trailing cmnts first if newlines reqd
 
  if (nl)
    { 
     while (cmntofst < ofst && nl)
       {
        if ( *cmtt == '|') break;                       // a block comment
        do_comment(cmtt,0);
        if (!getcmnt(&cmntofst, &cmtt)) break;
       }
     newl(nl);                     // do newlines
	} 

 // do any other comments (for a block)
  while (cmntofst < ofst)
    {
     do_comment(cmtt,0);
     if (!getcmnt(&cmntofst, &cmtt)) break;
    }
 }



void auto_comment (void)
{
// put auto generated in HERE
    if (*autocmt) { 
    do_comment(autocmt, cmntinst);
    *autocmt = 0;
    cmntinst = 0;             //safety
    }

}

// print subroutine params
int pp_stct (int ofst, int ix)
{
 int size, indent, end;
 LBK *x;
 CADT *a, *c;

 x = (LBK*) cmdch.ptrs[ix]; 
 a = x->adnl;

 // must safety check size versus end !!

 if (ofst+x->size > x->end) size = x->end-ofst+1;        // overlap !!   
 else size = x->size;
 
 if (!x->newl)
  {
   // single line printout	
   indent = size*3+8;
   pp_hdr (ofst, dirs[x->fcom].com,size,indent);
   pp_lev(ofst,a,1, size);
   ofst += size;
  } 
 else
 {
	 // multiline printout, look for split flag(s) 
	end = ofst + size;
	indent = 0;
    while (a && ofst < end)
	 {
	  size = 0;
      c = a; 
	  while(a)
	   {
	    if (size && a->newl) break; 
	    size += dsize(a);
	    a = a->next;
	   }
      if ((size*3+8) > indent) indent = size*3+8;      // temp lashup 
      pp_hdr (ofst, dirs[x->fcom].com, size,indent);
      pp_lev(ofst,c,1,size);
      ofst += size;
	 }
 }	 

 if (x->term && ofst == x->end)     // terminator byte(s) ?
  {
   pp_hdr (ofst, "byte", x->term, indent);
   pstr("##  terminator");
   return ofst+x->term;           // only if term byte printed
  }
  return ofst;
}


int pp_subpars (int ofst, short cnt)
{
  SXT  *xsub;
  CADT *add;
  LBK  *sblk;
  short size;
  OPS *t;
  //short xpc,xbnk;

  add = NULL;
  size = 0;
  t = curops+1;

  xsub = (SXT*) chmem(&subch);
  xsub->addr = t->addr;

  xsub = (SXT*) find(&subch, xsub);

  if (xsub)
   {
    add = get_cadt(xsub);
    size = xsub->nargs1;
   }
 
  sblk = (LBK*) chmem(&auxch);
  sblk->start = ofst;
  sblk->fcom = C_ARGS;

  sblk = (LBK *) find(&auxch,sblk);
  if (sblk)
     {
     add =  sblk->adnl;      // ARGS command overrides proc settings
     size = sblk->size;
     }

   if (!add)
     {                 // no additional params
      if (PSSRC) pstr("();");
      return 0;
     }

     // 'addnl' must be set from here
   if (PSSRC)
    {
     pchar('(');

     if (size)
       {
        pp_lev(ofst+cnt,add,0,size);     // do params at min width, drop trail comma
       }
     pstr(");");
     }

     if (size)
       {
        pp_comment (ofst+cnt,0);                       // do any comment before args
        pp_hdr (ofst+cnt, "#args ", size,0);           // do args as sep line
       }
  return size;
}


void fixup_sym_names(const char **tx)
{
  // adjust ops for correct SOURCE code printout, rbases, indexes etc.

  // sort of OK so far ....

  OPS *b, *o;
  SYT *s;
  int i, addr;
  const OPC* opl;

  opl = opctbl + curinst->opcix;
  //  BIT JUMP
  
  
  if (curops[3].bit)            // bit jump - if bitname, drop register name
     {
        o = curops+2;
        b = curops+3;               // b = bit number entry, get bit/flag name
        b->sym = get_sym (0,o->addr, b->val,curinst->ofst);
        if (b->sym)   {o->rgf = 0; o->sym = 0; return; }
     }

// get sym names FIRST, then can drop them later

  for (i = 1; i <= opl->nops; i++)
   {
    o = curops+i;
    if (i == opl->wop)
     o->sym = get_sym (1,o->addr,-1,curinst->ofst);
    else
     o->sym = get_sym (0,o->addr,-1,curinst->ofst);              // READ symbols
   }


  if (curinst->opcix < 10)                         //  Shifts, drop sym if immediate
   { // shift  
     if (curops[1].val < 16)  curops[1].sym = 0;
   } 





// indexed - check if RBS, or zero base (equiv of =[imd])

  if (curinst->inx)                             //indexed, use addr in cell [0] first
     {
     b = curops;        // probably redundant, but safety - cell [0]
     o = curops+1;

     if (!o->addr || o->rbs)
      {
        // drop register part and merge two terms into one as fixed addr
        // can't rely on o->val always being correct (pop screws it up...) so reset here
        if (!o->addr)  addr = 0;
        if (o->rbs) addr = o->rbs->val; 
        
        addr += curops->addr;                   // should give true address....
             
        if (addr >= PCORG) addr |= opbank(datbank);                        // do need this for inx additions !!

        s = get_sym (o->wsize, addr,-1,curinst->ofst);           // and sym for true inx address

        curops->val = addr;                    // temp !!

        o->rgf = 0;
        o->sym = 0;         // drop any sym and register print

        if (s)               //symbol found at (true) addr
         {
          o->bkt = 0;       // drop brackets (from [1])
          curops->sym = s;   
         }
       else
         {  // update for printout as a single address
           b->addr = addr;
         }

      } // end merge
     else
      {  
        // this is an [Rx + off], or [Rx - off], just add [0] symbol if not a small offset
        if (curops->addr > 0xff) curops->sym = get_sym (o->wsize, curops->addr,-1,curinst->ofst);           // and sym for true inx address
      }

    }         // end inx

  if (curinst->imd)       // clear symbol if register
     {
      if (curops[1].val < 0xff)  curops[1].sym = 0;      // not (normally) names for registers
      if (opl->sigix == 10) curops[1].sym = 0;           // must be an abs value for imd compare !!
     }


    // 3 operands, look for rbase or zero in [1] or [2] convert or drop

  if (opl->nops > 2)
    {
     if (curops[1].rbs && curops[2].addr == 0)
        {       // if true, RBS address and sym already sorted
         curops[2] = curops[3];                // op [2] becomes wop and dropped 
         *tx = opctbl[LDWIX].sce;                  // use ldx string TEMP  ([2] = [1] )
        }
      //   need to use ALT for this for full solution !!
     
     if (curops[2].rbs && !curinst->inx)        // RBASE by addition, not indexed
      {
       b = curops+1;
       b->val = b->val + curops[2].val;                   // should give true address....
       b->sym = get_sym (0, b->val,-1,curinst->ofst);     // redo symbol check

       curops[2] = curops[3];                // drop op [2]
       *tx = opctbl[LDWIX].sce;        // use ldx string TEMP  ([2] = [1] )
      }

    if (curops[2].addr == 0)         // 0+x,   drop x
     {
      curops[2] = curops[3];                // drop op [2]
      *tx = opctbl[LDWIX].sce;        // use ldx string TEMP  ([2] = [1] )
     } 

    if (curops[1].addr == 0)      // 0+x drop x
     {
      curops[1] = curops[2];
      curops[2] = curops[3];              // shuffle ops up
      *tx = opctbl[LDWIX].sce;        // use ldx string TEMP  ([2] = [1] )
     }
   }
  else
   {
    // + or - CARRY opcodes - can get 0+x in 2 op adds as well for carry
    // "\x2 += \x1 + CY;"
    if (curops[1].addr == 0 && curinst->opcix > 41 && curinst->opcix < 46)
    {
     *tx = scespec[opl->sigix-6];   // drop op [1] via array of alts
    }  

   }

}



short get_bitsyms (SYT **snam)
 {

 // for mask bit conversion to multiple sym names
 // ans no of syms found for given address and mask
 // this search assumes syms are in strictly correct order, bits then -1 (0->7)
 // and size is now known !!

   int i,ix,hsym,add, maxbit, maxadd;
   SYT *xsym;

   add =  curops[2].addr;
   maxbit = 7;                  // default to byte size
   maxadd = add;

   hsym = -1;  // highest bit number found
   for (i = 0; i < 16; i++) snam[i] = NULL;   // clear names

   xsym = (SYT*) chmem(&symch);
   xsym->add = add;
   xsym->bitno = -1;

   ix = bfindix(&symch, xsym);   // first entry for add
   if (ix >= symch.num) return 0;

   if (casz[curops[2].ssize] > 1)
   {
       maxbit = 15;       // this is a word op
       maxadd++;
   }

    // now ix is at first xsym which matches pc (probably whole sym)

   xsym = (SYT *) symch.ptrs[ix];    // could be NULL ?

   while (ix < symch.num && xsym->add <= maxadd)
     {
      i = xsym->bitno;

      if (i >= 0)
       {
        if (xsym->add > add) i += 8;    // for 2nd byte of a word

        if (i <= maxbit)
          {
           snam[i] = xsym;
           if (i > hsym)  hsym =  i;      // keep highest bitno
          }
       }
      xsym = (SYT *) symch.ptrs[++ix];    // could be NULL ?
     }
  return hsym;
}

//const char *
void bitwise_replace(const char **tx)

/* check for bitwise operations - not for 3 operand instructions or where no symname ?*/
{

  int lg, i, k, mask, ix;
  char eq[3];
  const OPC * opl;

  ix = curinst->opcix;
  
  opl = opctbl + ix;

  if (anlpass < ANLPRT) return;               // don't care
  if (opl->nops > 2)    return;               // not 3 op instructions
  if (!curops[1].imd)   return;               // only allow immediate vals

  lg = -1;
  if (ix >  9 && ix < 14) lg = 0;            // AND opcodes
  if (ix > 29 && ix < 34) lg = 1;            // OR  opcodes

  if (lg < 0) return;

  mask = curops[1].val;

  if (get_bitsyms (snam) < 0) return;        // no bit names found

  // names are in bit order (1 << offset);

  k = 0;		                             // first output flag
  if (ix > 31 && ix < 34)                    // XOR opcodes
     strcpy(eq,"^=");
  else
     strcpy(eq,"=");
   // xor or standard

  if (ix >  9 && ix < 14)  mask = ~mask;              // reverse mask for AND

  if (!mask) return;                                  // no flags, so print orig

  for (i = 0; i < casz[curops[1].ssize]* 8; i++)
   {
    if (mask & 1)
      {
       if (k++)  p_indl ();    // new line and indent if not first
       if (snam[i])
          {
           pstr ("%s",snam[i]->name);   //  print each matched sym name
          }
       else
         {
          pstr ("B%d_", i);            // no name match
          p_opsc (curinst, 2);
         }
       pstr (" %s %d;",eq, lg);
      }
    mask  = mask >> 1;
   }
  *tx = empty;                        // at least one name substituted  so return a ptr to null
 }



void shift_comment(void)
 {
  /*  do shift replace here - must be arith shift and not register */
  int j, ps;
  OPS *ops;

  if (anlpass < ANLPRT) return ;      // s;         // redundant ?

  ops = curops+1;
  
  j = 1 << ops->val;         // equiv maths divide or multiply 

  if (curinst->opcix < 10 && ops->imd)
   {
     cmntinst = curinst;  
     ps = sprintf(autocmt, "## \x2 "); 
     if (curinst->opcix < 4)
      ps += sprintf(autocmt+ps,"*");
     else  
      ps += sprintf(autocmt+ps,"/");
     ps += sprintf(autocmt+ps, "= %d ", j); 
   }
}

 /*
void do_std_ops(const char **tx)
{ // use 0x10 ?
  const OPC* opl;
  
  opl = curinst->opl;

   if (**tx == 15)  // 0xf
   {
     // replace with std type A = B + C;
     if (opl->nops == 3) p_opsc(curinst,3);
     else p_opsc (curinst,2);
     pstr(" = ");

     p_opsc (curinst,2);
     pstr(" %s ",*tx+1);
     p_opsc (curinst,1);
     pstr(";");
     *tx = empty;  
   }  
    
}
*/




void carry_src(const char **tx, int jf)
{
    // jf is jump found (=1 if so)  JC and JNC only
    
   int jc,ps,inx;         
   INST *last;            // last instance ptr
   const OPC *opl;
   
   inx = curinst->opcix; 

   if (inx != 56 && inx != 57) return;     // not a carry related op
 
   jc = (inx & 1) ? 0 : 1;                 // if jnc 56 yes 57 no
 
   // JC or JNC to get here
 
   find_last_psw(&last);                // get last psw changer (last is always valid)
      
   if (!last)  return;              // valid one not found, or subr call etc.
         
   opl = opctbl + last->opcix;      // mapped to last inst

      if (opl->sigix == 10 || opl->sigix == 7)
        {
           pstr ("if (");          // compare or sub borrow cleared if a=b
           pstr("(uns) ");
           p_opsc(last,2); 
           if (jc != jf) pstr(" >= "); else
           pstr(" < ");               // right for JNC if no bkts            // this to swop ?
           p_opsc(last, 1);
         
        
           if (jf == 1)
             {
              pstr(" )  {");
             }
           else
             { 
              pstr(") goto ");
              p_opsc(curinst,1); 
              pstr(";  ");
             }
       //    pstr("x-x");
           *tx = empty;            // nothing to follow
        }

// these two don't really work in a maths sense....so embed a comment instead.

      if (opl->sigix == 6  || opl->sigix == 3)
        {                 //  add
     
 
           if (opl->sigix == 6)
            {  
             ps = sprintf(autocmt, "## jump if (\x2+\x1");
             if (last->opcix > 41 && last->opcix < 47) ps+= sprintf(autocmt+ps,"+CY");
             ps += sprintf(autocmt+ps, ")");
            } 
           
           else
           ps = sprintf(autocmt, "## jump if (\x2<<\x1)"); 
           
           cmntinst = last;
           
           ps += sprintf(autocmt+ps, jc ? " > " : " <= ");
           
           
           if (casz[last->opr[1].ssize] == 1 )
           sprintf(autocmt + ps, "0xff " );   else
           sprintf(autocmt + ps, "0xffff ");
        }
    
      if (opl->sigix == 25)
      {       // inc
          cmntinst = last;
          ps = sprintf(autocmt, "## jump if \x1"); 
          ps += sprintf(autocmt+ps, jc ? " > " : " <= ");
          if (casz[last->opr[1].ssize] == 1 ) sprintf(autocmt + ps, "0xff " );
          else sprintf(autocmt + ps, "0xffff ");  
      } 

    if (opl->sigix == 24)
      {       // dec
         cmntinst = last;
         ps = sprintf(autocmt, "## jump if \x1");
         ps += sprintf(autocmt+ps, jc ? " >= " : " < ");
         sprintf(autocmt + ps, "0 ");  
      } 

} 

 void do_dflt_ops(SBK *s)
 {  // default - any operands as registers  
	int i; 
	for (i = 1; i <= curinst->numops; i++) op_val (g_byte(s->nextaddr + i), i);
	s->nextaddr += curinst->numops + 1; 
 }


void calc_mult_ops(SBK *s)
{
  /******************* instructions M *****************
    * multiple address multiple op opcodes
    * opcsub values   0 direct  1 immediate  2 indirect  3 indexed
    * order of ops    3 ops       2 ops      1 op
    *                 dest reg    dest reg   sce reg
    *                 sce  reg    sce  reg
    *                 sce2 reg
    * NB indexed mode is SIGNED offset
    *
    *******************************************
    * Adjust cnt of opcode bytes for 'M' opcodes
    * opcsub 3 - +1 byte if int index flag set
    * Word op - opcsub 1 and 3 - +1 byte
    * Byte op - opcsub 3 - +1 byte
    ******************************************************/

     int opcsub, firstb,i , xofst;
     OPS *o;
     DDT *d;

     xofst = s->nextaddr;
	 
     opcsub = g_byte(xofst);
     opcsub &= 3;                           // op subtypes

     firstb = g_byte(xofst+1);             // first data byte after (true) opcode
     op_val (firstb, 1);        

     o = curops+1;                             // most action is for [1]
     
     
     switch (opcsub)
	  {
	   case 1:                                          // first op is immediate value, byte or word
         curinst->imd = 1;
         op_imd();                                      // op[1] is immediate

	     if (casz[o->ssize] == 2)
          {
           xofst++;                                    // word,  recalc
           op_val (g_word(xofst), 1);                  // recalc op1 as word
          }

         d = add_ddt(o->addr, o->ssize);               // may be an address of struct...

		 if (d && curinst->opcix > 49 && curinst->opcix < 52)    // a PUSH - might be code ?
         d->psh = 1;
		 
         break;

       case 2:     // indirect, optionally with increment - add data cmd if valid
                   //  indirects are ALWAYS WORD address.
         curinst->inr = 1;
         if (firstb & 1)
	      {                                         // auto increment
           firstb &= 0xFE;                          // drop flag marker
           curinst->inc = 1;
           o->inc = 1;
	      }
         op_val (firstb, 1);                       // recalc [1] for inr
         o->bkt = 1;

         if (anlpass < ANLPRT)
          { // get actual values for emulation
           o->addr = g_word(o->addr);                // get address from register, always word
           if (o->addr >= PCORG) o->addr |= opbank(datbank);
           o->val = g_val(o->addr,o->ssize);  // get value from address, sized by op
           d = add_ddt(o->addr, o->ssize);
          }
	     break;


	   case 3:                                          // indexed, op0 is a fixed SIGNED offset

	     curinst->inx = 1;                             // mark op1 as indexed
         if (firstb & 1)
            {
             curops->ssize = 2;                        // offset is long (TRY unsigned word)
             firstb &= 0xFE;                           // and drop flag
            }
          else
            {
             curops->ssize = 5;                        // short, = byte offset signed
            }

         curops->off = 1;                               // op [0] is an address offset
         o->bkt = 1;
         op_val (firstb, 1); 
         
         curops->addr = g_val(xofst+2,curops->ssize);
         if (curops->addr >= PCORG) curops->addr |= opbank(datbank); 

        if (anlpass < ANLPRT)
          { // get actual values for emulation
           opcsub = o->val;
           o->addr = curops->addr+o->val;                   // get address from register, always word
           o->val = g_val(o->addr,o->ssize);                // get value from address, sized by op
           d = add_ddt(o->addr, o->ssize);
           
           if (d)
            {
            d->p1 = curops->addr;     // add_ddtx( ); with inx offset
            d->p2 = opcsub;
          }  

          }         
         opcsub = casz[curops[0].ssize];                  // size of op zero
         xofst += opcsub;
    
         break;

       default:      // direct, all ops are registers
         break;
      }

     //calc rest of ops - always register
     for (i = 2; i <= curinst->numops; i++) op_val (g_byte(xofst + i), i);

     s->nextaddr = xofst + curinst->numops + 1; 
  }


 void pshw (SBK *s)
{

  OPS *z;
//  int addr;
//  DDT *d;

  calc_mult_ops(s);


  if (anlpass >= ANLPRT) return;

  // now, check for single vect or vect list
  // immediate may be a vect list (but not always)
  // MUST REWORK THIS TO CHECK FOR A RET.. otherwise can give invalid addresses
// but PE has a push (30de) which does not get  used till much later....
// check the RET ?

  z = curinst->opr + 1;

// POPW must occurr first.....

 // if (s->emulate) stkp--;
  
//  if (!stkp)
// {
//     s->emulate = 0;   
// }
// else      
// {
	 // do arg check here !!
// }
  
  // but detect difference here !!
  s->stk--;
 
  #ifdef XDBGX
   DBGPRT("PUSH R%x from (%x) ", z->addr, curinst->ofst);
   if (z->rgf) DBGPRT ("=%x", z->val);
   DBGPRT(" stk = %d", s->stk);
   DBGPRTN(0);
  #endif
 // if (s->sub && s->type > 3) s->sub->popl--;

 // addr = nobank(z->addr);

  //if (curinst->inx)  addr = curinst->opr->val;      // [0]

  //if (curinst->inr)  addr = z->val;

  //addr |= opbank(datbank);               // if not in vect sig, assume it's DATA - NO THIS DOESN'T WORK !!
  //d = add_ddt(addr,2);
  //if (d) d->psh = 1;
//  return xofst;
}



void popw(SBK *s)
{
   int i;
   SBK *c;
   
	calc_mult_ops(s);
	
	if (!curops[opctbl[curinst->opcix].wop].addr) return;
	if (s->stk >= 0) 
		{
		 s->stk++;
		 c = s;
	    for (i = 0; i < s->stk; i++)	if (c) c = c->caller;
		
		
 #ifdef XDBGX
  DBGPRT("POPW %x %d r%x = %x",s->curaddr,s->ctype,curops[opctbl[curinst->opcix].wop].addr, c ? c->nextaddr: 0);
  if (s->sub) DBGPRT(" in sub %x", s->sub->addr);
   DBGPRT(" stk = %d", s->stk);
  DBGPRTN(0);
#endif
		}
//stkp++;

// if (stkp)
// {
//     s->emulate = 1;     
	 // and add where addr gets popped
// }
 
 
curops[opctbl[curinst->opcix].wop].val = 0;         // test
 
}


void  clr (SBK *s)
  {
	do_dflt_ops(s);  
    curops[opctbl[curinst->opcix].wop].val = 0;
  }

void neg  (SBK *s)
  {
	do_dflt_ops(s); 
    if  (anlpass < ANLPRT) curops[1].val = -curops[1].val;
 }

void  cpl  (SBK *s)
  {
	do_dflt_ops(s); 
    if  (anlpass < ANLPRT) curops[1].val = (~curops[1].val) & 0xffff;         // size ?
  }

void  stx (SBK *s)
  {
   /* NB. this can WRITE to indexed and indirect addrs
    * but addresses not calc'ed in print phase
    */
	
	calc_mult_ops(s);
    if (anlpass < ANLPRT) curops[1].val = curops[2].val;
	
  }

void ldx (SBK *s)
  {
 
   int ix, w;
   calc_mult_ops(s);
   ix = 1;
   w = opctbl[curinst->opcix].wop;



   if (w) curops[w].val = curops[ix].val;

   // SPECIAL for bank change via LDB

    if (anlpass < ANLPRT && P8065 && curops[w].addr == 0x11 && curinst->imd)
      { // This is BANK SWOP    via LDB
        // top nibble is STACK (we don't care) bottom is DATA BANK
        // may need to ADD REGISTER load
        int i;        //,j;
        i = curops[1].val & 0xf;
        //j = curops[1].val >> 4;

        //need to check if this is register or immediate
		#ifdef XDBGX
          DBGPRT("%x LDX R11 Data Bank = %d, Stack Bank =%d" ,curinst->ofst, i, curops[1].val >> 4);
		#endif  
        if (bkmap[i].bok) datbank = i << 16;
   //     else  DBGPRT("- Invalid data");
  //      if (!bkmap[j].bok) DBGPRT("- Invalid stack bank");
        #ifdef XDBGX
          DBGPRTN(0);
        #endif
      }
  /*

  // SPECIAL for STACK move
   for (i = opl->nops; i > 0; i--)
      {
       if (curops[i].stk)        // && curops[i].reg)
          {
          if (curops[i].wsize)  DBGPRT( "*STACK WRITE at %x\n",ofst);
          else DBGPRT( "*STACK READ at %x\n",ofst);
          }
    }
 }
        */

  }

void orx(SBK *s)
 {
  
  calc_mult_ops(s);
  if (anlpass >= ANLPRT)   return; 
  if (curinst->opcix < 32)	curops[opctbl[curinst->opcix].wop].val |=  curops[1].val;    // std
  else  curops[opctbl[curinst->opcix].wop].val ^=  curops[1].val;                        // xor
  
 }

void add(SBK *s)
 {
 	 
   calc_mult_ops(s);
   if (anlpass < ANLPRT) curops[curinst->numops].val =  curops[2].val + curops[1].val;

   
 }


void xand(SBK *s)
 {
  	 
	calc_mult_ops(s);
  if (anlpass < ANLPRT)  curops[curinst->numops].val =  curops[2].val & curops[1].val;
   
 }

 
void sub(SBK *s)
 { 	 
	calc_mult_ops(s);
 if (anlpass < ANLPRT) curops[curinst->numops].val =  curops[2].val - curops[1].val;
  
 }


void cmp(SBK *s)
 {
   
  calc_mult_ops(s);
	
	
//   int x;
   
 //  s->nextaddr = calc_mops(xofst);
   //x =  curops[2].val - curops[1].val;      // 2 - 1 for psw set
 //  DBGPRTN("cmp %x-%x",curops[2].val,curops[1].val);
 //  set_psw(0x3e, x);         // special call here (no writes)
   
//   if (x >=0) psw |= 2; else psw &= 0x3d;      // set carry if positive clear if not
     
 }

void mlx(SBK *s)
 { //int xofst;	 
	calc_mult_ops(s);
   if (anlpass < ANLPRT)curops[curinst->numops].val =  curops[2].val * curops[1].val;  // 2 and 3 ops
    
 }

 void dvx(SBK *s)
 {  
	calc_mult_ops(s);
  if (curops[1].val != 0)
    if (anlpass < ANLPRT)curops[opctbl[curinst->opcix].wop].val /=  curops[1].val;    // if 2 ops (wop = 2)
	
 }


void calc_shift_ops(SBK *s)
{
    short b;    
     b = g_byte(s->nextaddr+1);             // first data byte after (true) opcode
     op_val (b, 1);                   // 5 lowest bits only from firstb, from hbook
     if (b < 16) op_imd();            // convert op 1 to absolute (= immediate) shift value
     op_val (g_byte(s->nextaddr+2),2);
	 s->nextaddr += curinst->numops + 1; 
    }










void shl(SBK *s)
 {
  calc_shift_ops(s); 
  if (anlpass < ANLPRT)curops[opctbl[curinst->opcix].wop].val <<=  curops[1].val;
  
 }

 void shr(SBK *s)
 {
  calc_shift_ops(s); 
  if (anlpass < ANLPRT)curops[opctbl[curinst->opcix].wop].val >>=  curops[1].val;
  
 }

void inc(SBK *s)
 {
	 do_dflt_ops(s);
// NOT ssize, but ALWAYS 1 (only autoinc is one or two)
// if (anlpass < ANLPRT) curops[curinst->opl->wop].val++;
  //curinst->inc = 1;                  // trick as autoinc ?
	
 }

void dec(SBK *s)
 {
	  do_dflt_ops(s);
 // if (anlpass < ANLPRT)curops[curinst->opl->wop].val--;
   }


int calc_jump_ops(SBK *s, int jofs, int type)
{ 
	// s->nextaddr must correctly point to next opcode for this to work
	// returns jump address
	JLT *j;
	
     jofs += s->nextaddr;                            // offset as calc'ed from NEXT opcode 
     jofs = nobank(jofs);                            // pcj allow for wrap in 16 bits
	 
     curops[1].adr = 1;
     curops[1].rgf = 0;
 
     op_val (jofs, -1);                              // jump destination in [1] with CODE bank
     jofs = curops[1].addr;                          // now is full address with bank
 
     if (anlpass >= ANLPRT) return jofs;         // still calced, but no need to add jump for print phase
     j = add_jump (s,type);
  
     if (!j)
        {
         s->inv = 1;                                 // mark as invalid scan
         end_scan (s, s->nextaddr-1);
		 return 0;
        }
     return jofs;
  }


void cjm(SBK *s)
 {    // conditional jump, (short)
   s->nextaddr += 2;
   calc_jump_ops(s,negval(g_byte(s->nextaddr-1),0x80), S_CJMP);
   do_sjsubr(s, S_CJMP);
 }

void bjm(SBK *s)
 {     // JB, JNB, add bit operand, but don't change opcode size
    int xofst;
    xofst = s->nextaddr;
	s->nextaddr += 3;

    curops[3].bit = 1;
    curops[3].rgf = 0;
    curinst->numops++;                       // setup bit number in op3, no change to opcode size.
    op_val (g_byte(xofst) & 7, 3);           // bit number
    op_val (g_byte(xofst+1), 2);             // register

    calc_jump_ops(s, g_val(xofst+2,5),S_CJMP);
	do_sjsubr(s,S_CJMP);

 }

void sjm(SBK *s)
 { //  a short jump
	int jofs;

    jofs = g_byte(s->nextaddr) & 7;
    jofs <<= 8;
    jofs |= g_byte(s->nextaddr+1);
    jofs =  negval(jofs,0x400);                     // signed 11 bit value

	s->nextaddr += 2; 

	calc_jump_ops(s, jofs,S_SJMP);
   
	do_sjsubr(s,S_SJMP);
/*	jofs = curops[1].addr;                 // jump addr	
     if (xofst <= s->curaddr  && xofst >= s->start) return;   // 0;     // local loop - temp ignore.    
         if (xofst == s->nextaddr) return;      // 0;                           // dummy jump - safety
        } 
     add_scan(xofst,S_SJMP,s);                        // add new scan 2
	
  */
 
	end_scan (s, s->nextaddr-1);                  // END of this block                      // END of this block   	 
 }



void ljm(SBK *s)
 {      //long jump

    s->nextaddr += 3;

	calc_jump_ops(s,g_val(s->nextaddr-2,6),S_SJMP);            //g_val(xofst+1,6));

//end_scan (s, s->nextaddr-1);                  // END of this block	  
	do_sjsubr(s,S_SJMP);

 //   xofst = curops[1].addr;                 // jump addr	
/*    if (xofst <= s->curaddr  && xofst >= s->start) return;   // 0;     // local loop - temp ignore.    
         if (xofst == s->nextaddr) return;      // 0;                           // dummy/loopstop jump
        } 
     add_scan(xofst,S_SJMP,s);                        // add new scan 2
	
  */ 
	       	
	end_scan (s, s->nextaddr-1);                  // END of this block	(subr?)       
 }

void cll(SBK *s)
 {     // long call
 
    s->nextaddr += 3;
    calc_jump_ops(s,g_val(s->nextaddr-2,6),S_CSUB);
	do_sjsubr(s, S_CSUB);
 }

void ret(SBK *s)
 {
    s->nextaddr++;
    calc_jump_ops(s, -1, S_RETJ );              // add jump but no scan block
    end_scan (s, s->nextaddr-1);        // END of this scan block 
 }


void djm(SBK *s)
 { // djnz
   int xofst;
   xofst = s->nextaddr;
   s->nextaddr += 3;
   op_val (g_byte(xofst+1), 2);              // do register

   calc_jump_ops(s,g_val(xofst+2,5), S_CJMP);
   do_sjsubr(s, S_CJMP);	
 }

void skj(SBK *s)
 {  // skip, = pc+2 forwards op 0, static jump
	s->nextaddr++;

    calc_jump_ops(s,1, S_SJMP);
	do_sjsubr(s, S_SJMP);
    end_scan (s, s->nextaddr-1);        // opcodes which mark END of block       
 }

void scl(SBK *s)
 { // short call
   int jofs;

   jofs = g_byte(s->nextaddr) & 7;
   jofs <<= 8;
   jofs |= g_byte(s->nextaddr+1);
   jofs =  negval(jofs,0x400);                     // signed 11 bit value
   s->nextaddr+=2;

   calc_jump_ops(s,jofs, S_CSUB);
   do_sjsubr(s, S_CSUB);
  
 }


void php(SBK *s)
 {
	//  do_dflt_ops(s);
 // curops[curinst->opl->wop].val--;
  	s->nextaddr++;
 }

void ppp(SBK *s)
 {
	//  do_dflt_ops(s);
 // curops[curinst->opl->wop].val--;
  	 s->nextaddr++;
 }

void nrm(SBK *s)
 {
	  do_dflt_ops(s);
 // curops[curinst->opl->wop].val--;
  	// return s->nextaddr;
 }


void sex(SBK *s)
 {
	  do_dflt_ops(s);
 // curops[curinst->opl->wop].val--;
  	//return s->nextaddr;
 }

void clc(SBK *s)
 {
 // curops[curinst->opl->wop].val--;
  	s->nextaddr++;
 }

void stc(SBK *s)
 {
 // curops[curinst->opl->wop].val--;
  	s->nextaddr++;
 }

void die(SBK *s)
 {
 // curops[curinst->opl->wop].val--;
  	 s->nextaddr++;
 }

void clv(SBK *s)
 {
 // curops[curinst->opl->wop].val--;
  	s->nextaddr++;
 }

void nop(SBK *s)
 {
 // curops[curinst->opl->wop].val--;
  	s->nextaddr++;
 }

void bka(SBK *s)
 {          // RAM bank swopper 8065 only
   int bk;
   //   According to Ford book, this only sets PSW, so could be cancelled by a POPP
   //   or PUSHP/POPP save/restore,  so may need a PREV_BANK 

   bk = curinst->opcix - 107;                   // new RAM bank
   //     rambank = bk * 0x100;                 // establish new offset
   #ifdef XDBGX
    DBGPRTN("New Rambank = %x", bk);
   #endif
   op_imd();                                    // mark operand 1 as immediate
   s->nextaddr++;
 }








void do_code (SBK *s)

{
     /* analyse next code statement from ofst and print operands
      * s is a scan blk, pp-code calls this with a dummy scan block in print phase
      */

  int tbk, x, xofst, indx;
  OPC *opl;
  SIG *g;

  codbank = g_bank(s->curaddr);                // current code bank from ofst

   if (s->scanned) 
  {
   #ifdef XDBGX 
    DBGPRTN("Already Scanned s %x c %x e %x", s->start, s->curaddr, s->end);  
   #endif	
    return;
  }  

  if (s->stop || s->held) 
  { 
	#ifdef XDBGX
     DBGPRTN("Stop/hold set s %x c %x e %x", s->start, s->curaddr, s->end);  
	#endif 
    return;
  }  

    if (anlpass < ANLPRT && s->curaddr >= s->end)
    {
	  #ifdef XDBGX
        DBGPRTN("Scan >= End, STOP");
	  #endif	
      end_scan (s, 0); 
      return; 
    }


  if (!val_code_addr(s->curaddr))
    {
	 #ifdef XDBGX	
     DBGPRTN("Invalid address %05x", s->curaddr);
	 #endif 
     s->inv = 1;
     s->stop = 1;
	 s->nextaddr = s->curaddr+1;
	 return;
    }

   /***************************************************************
  * one instruction at a time.
  * this is opcode analysis. also used for print routine
  ***************************************************************/

  xofst = s->curaddr;                     // don't modify original ofst, use this instead
  tbk = -1;                                 // temp bank holder

  x = g_byte(xofst);
  indx = opcind[x];                       // opcode table index
  opl = opctbl + indx;


 // Ford book states opcodes can have rombank(bank)+fe as prefix (= 3 bytes)

   if (indx == 112)                     // bank swop prefix 
     {
      if (!P8065) indx = 0;
      else
	   {
        xofst++;
        tbk = g_byte(xofst++);            // bank number (next byte)
        tbk &= 0xf;
        x = g_byte(xofst);                // get opcode
        indx = opcind[x];
//       opl = opctbl + indx;              // this could be the SIGN prefix (check next)
	   }
     }

  if (indx == 111)
     {                                  // this is the SIGN prefix, find alternate opcode
      xofst++;
      x = g_byte(xofst);
      indx = opcind[x];                 // get opcode
      opl = opctbl + indx;
      if (opl->sign)  indx += 1;         // get prefixed opcode index instead
      else indx = 0;                    // or prefix not valid
     }

  if (opl->p65 && !P8065) indx = 0;     // 8065 only

  if (!opctbl[indx].eml)  indx = 0;     // safety check
  
  opl = opctbl + indx;

  if (!indx)
     {
       s->inv = 1;
	   s->nextaddr = xofst + 1;
       if (anlpass < ANLPRT)
         {

        //  wnprtn("Invalid opcode (%x) %x", j, s->curaddr);
          end_scan (s, xofst+1);
         }
       else  
            wnprtn("Invalid opcode at %05x", s->curaddr); // report only once
      return;
     }

  lpix++;                                               // increment Instances stack ptr 
  if (lpix >=PILESZ) lpix = 0;                          // loop around if necessary
  curinst = instpile + lpix;
  memset(curinst,0,sizeof(INST));                       // clear next entry for use

  curinst->opcix = indx;                                // opcode index
  curinst->ofst = s->curaddr;                           // original start of opcode
  curops = curinst->opr;                                // shortcut to operands (4 off)
  
  curinst->numops = opl->nops;                          // These items can be changed by opcodes
  curops[opl->wop].wsize = opl->wsz;                    // set write size (or zero)
  curops[1].ssize = opl->rsz1;                          // operands - read size
  curops[2].ssize = opl->rsz2;
  curops[3].ssize = opl->rsz3;
  curops[1].rgf  = 1;                                   // operands - set register by default
  curops[2].rgf  = 1;
  curops[3].rgf  = 1;
  curinst->bank = codbank>>16;                          // bank to be used (codebank)

  if (tbk >=0)
  {  // bank opcode
    if (bkmap[tbk].bok)
     {
      curinst->bnk  = 1;                               // mark bank instruction
      curinst->bank = tbk;                               // and keep it for this opcode 
     }
	#ifdef XDBGX 
    else
     {
	   if (anlpass < ANLPRT) DBGPRTN("ROMBNK - Invalid bank %d", tbk);
     }
	#endif 
  }

  // need an emulation approach to get number of subr arguments...
  
  if (get_opstart(s->curaddr))                // has this been scanned already ?
    { 
      g = (SIG*) chmem(&sigch);
      g->ofst = s->curaddr;
      g = (SIG*) find(&sigch,g);              // Yes, check for sig at this address
    }  
  else
    {
     g = scan_sigs(s->curaddr);               // scan sig if not code scanned before (faster)
     set_opstart(s->curaddr);                 // include any prefixes in opcode flag
    }

  if (g) process_sig(g, s);                   // process sig if reqd

  s->nextaddr = xofst;                        // address of this opcode - for operands 
  opl->eml(s);                                // opcode handler
                                              // s->nextaddr is now next opcode address
  curinst->nextaddr = s->nextaddr;            // update current instance
  if  (anlpass < ANLPRT) upd_watch(s);
 }



/**************************************************************************
* opcode printout - calls do_code
***************************************************************************/

int pp_code (int ofst, int ix)
{
  int i, j, lwop, cnt, inx, eqp;
  SBK s;                          // DON'T use a temp one, this is safer....
  const char  *tx ;
  INST *last;
  const OPC* opl;
 
  memset(&s,0, sizeof(SBK));     // clear it first
  s.curaddr = ofst;
  do_code(&s);
  
  if (s.inv)                            // cnt < 0
    {
     pp_hdr (ofst, opctbl->name, 1,0);   // single invalid opcode
     return s.nextaddr;
     }

  // For printout, add an extra line with bank swop prefix/operand
  // as there is no other way to neatly show indirect refs addrs correctly (i.e. [x])

  if (curinst->bnk)
    {
     pp_hdr (ofst, "bank", 2,0);
     pstr ("%d", curinst->bank);
     s.curaddr +=2;
    } 

// NB. keep 'ofst' at original address so that symbols, vects etc work correctly

  inx = curinst->opcix;
  opl = opctbl + inx;

  cnt = s.nextaddr - s.curaddr;
  pp_hdr (s.curaddr, opl->name, cnt,0);

  i = curinst->numops;
  while (i)
     {
      p_oper (i);              // code printout
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

        tx = opl->sce;                    // source code, before any mods
		eqp = 0;                          // equals not printed

        // any mods to source code go here

        j = find_fjump (ofst);
        
        fixup_sym_names(&tx);         // for rbases, 3 ops, names, indexed fixes
        shift_comment();              // add comment to shifts with mutiply/divide
        bitwise_replace(&tx);         // replace bit masks with flag names
        carry_src(&tx, j);            // sort out carry cases
        
     //   do_std_ops(&tx);              // shortcut std opcode


    
        if ((j & 1) && *tx) 
         {
           if (inx > 53 && inx < 72)                 // cond jumps
           {
             if (inx & 1) tx = opctbl[inx-1].sce;
             else tx = opctbl[inx+1].sce;
           }
           else j = 0;
        }

       //*** do psuedo source output

       while (*tx)
	    {
            
         switch (*tx)   // 1-12 used  1-31 available
          {
           case 1:
	       case 2:
	       case 3:
           	    p_opsc (curinst,*tx);
    	        break;
            

//           case 4:  not used ???

	       case 5:
            lwop = find_last_psw(&last);                 // get last psw changer 
            if (lwop > 0) p_opsc(0,1);                   // zero if not a compare
            else p_opsc(last, 1);                        // operand 1 of compare (or zero op)
            break;

	       case 6:
            lwop = find_last_psw(&last);                // get last psw changer
            if (lwop > 0) p_opsc(last,lwop);            // write op if not a compare
            else p_opsc(last,2);                        // operand 2 of compare
            break;
            
          case 7:                                       // add size or SIGN value
            i = *(tx+1);                                // get operand
            if (curops[i].sym) break;
            if (!curops[i].rgf) break;
            if (i == 1 && (curinst->inx || curinst->inr)) break;     // not for indexes or indirect
            
            if (curops[i].wsize && !eqp) i = curops[i].wsize; else i = curops[i].ssize;
            
            // after an equals, always use read size
            
            if (i & 4)  pstr("S");
            i &= 3;
            if (i == 1)  pstr("Y");
            if (i == 2)  pstr("W");
            if (i == 3)  pstr("L");
            break;

         case 8:
            if (j<3) pstr ("if (");
            break;

	   case 9:

  	       switch (j)  //jump handling
             {
              case 1 :
	            pstr (" {");   // [if] cases
                break;
              case 2 :
                pstr ("return;");
                break;
              case 3:
                pchar(';');
                // drop through
              case 4:
                pstr("} else {");
                break;     
              default:
                pstr ("goto ");
	            p_opsc (curinst,1);
	            pchar (';');
                break; 

             }
	    break;

	   case 11:
	      p_indl();             // print new line with indent
	    break;

       case 12:
	        s.nextaddr += pp_subpars(ofst,cnt);
            break;

       case '=' :
	   
             eqp = 1;          //     equals printed
              // fall through 
	   default:
            pchar(*tx);
	  }
      
	 tx++;
	}
      }
     }
     else      // this is for embedded arguments without source code option - must still skip any operands
          
      if (!PSSRC && (inx == 73 || inx == 74 || inx == 82 || inx == 83))          // CALL opcodes
          
           s.nextaddr += pp_subpars(ofst, cnt);

     find_tjump (s.nextaddr);       // add any reqd close bracket(s)

/*
  {
    OPS *s;                            // debug

  //  pp_comment(ofst+cnt);              // do comment first
    if (curinst->opl->nops)
     {
      p_pad(cposn[3]);                          //CDOSTART);
      pstr(" SVA ");

      for (i=curinst->numops;  i>=0; i--)
        {
         s = curops+i;
         if (s->addr + s->val)
          {
   //        if(curinst->opl->wop == i && i)  pstr("*");
           pstr("%c[",i+0x30);
           if (s->sym) pstr("%s ",s->sym->name);
           pstr("%x %x",s->val, s->addr);
//           if (!i) pstr (" T%x", curinst->tval);          //new temp value
           pstr("]%x, ",s->ssize);    
          }
        }
     }
  }                 // end dubug         */


  auto_comment();                   // print auto comment if one created



  if (inx > 74 && inx < 82)         // add an extra blank line after END BLOCK opcodes ...
   {
     cmntnl = 1;
   }

  return s.nextaddr;
}

int do_error(const char *err,int posn)
  {                                 // cmnd string in flbuf....
   int i; 

   wnprtn(flbuf);                 // put newline on end.
   for (i = 0; i < posn; i++) fputc(0x20, fldata.fl[2]);
   wnprt("^ ");
   wnprtn(err);
   return 1;
}



int set_rbas (CPS *c)
{
 RBT *x;
 x = add_rbase(c->p[0]&0xFF, c->p[1], c->p[2], c->p[3] );
 if (x) x->cmd = 1;              // by command
 return 0;
}



int set_opts (CPS *c)
{
  // check first, for single bank 8065
  int x;
  if (P8065) x = 1; else x = 0;
  
  cmdopts = c->p[0];
  
  if (numbanks || x)  cmdopts |= H;
 
  #ifdef XDBGX 
    DBGPRT("\nOpts = ");
    prtflgs(cmdopts);
    DBGPRTN(0);
    DBGPRTN(0);
  #endif	
  return 0;
}



void set_adnl(CPS *cmnd, LBK *z)
{           // move cadt blocks from cmnd to new LBK 

  if (!z) return;

  if (cmnd->levels <= 0 || !cmnd->adnl)
    {
	  #ifdef XDBGX	
       DBGPRTN(0);
	  #endif 
     return;       // no addnl block
    }

  freecadt(&(z->adnl));            // free any existing CADT, probably redundant ?

  z->size = cmnd->size;

  z->adnl = cmnd->adnl;          // add chain to command blk
  cmnd->adnl = NULL;             // and drop from cmnd structure

  #ifdef XDBGX
    DBGPRT("      with addnl params");
    prt_cadt(z->adnl);
    DBGPRTN(0);
  #endif

}

void set_data_vect(LBK *blk)
{
 int ofst, end,pc;
 int bank;
 short i;
 CADT *addnl;

 if (blk && blk->fcom == C_STCT)
  {
   ofst = blk->start;
   end =  blk->end;
   bank = g_bank(ofst);

   while (ofst < end)
     {
      i = 0;
      addnl = blk->adnl;
 
     while (addnl && i < 32 )
       {
        if (addnl->vect && !PMANL)
         {
          pc = g_word(ofst);
          pc |= bank;                 // this assumes vect is always in same bank
          add_scan (pc, S_CSUB, 0);   // adds subr
         }
        ofst += dsize(addnl);
        i++;
        addnl = addnl->next;
       }
     }
  }

}

int set_data (CPS *c)
{
  LBK *blk;
  blk = set_cmd (c->p[0], c->p[1], c->fcom|C_CMD);  // start,end, cmnd. by cmd

  set_adnl(c,blk);
   #ifdef XDBGX
     DBGPRTN(0);
   #endif
   
  if (blk) 
	  {
       blk->term = c->term;                    // terminating byte flag
       blk->newl = c->newl;                    // split printout (newline)
	  } 
  new_sym(2,c->symname,c->p[0], -1,0,0);           // +cmd. safe if symname null
  set_data_vect(blk);
  return 0;
}


int set_sym (CPS *c)
{
  int bitno,w;
  CADT *a;

  a = c->adnl;
  bitno = -1;

  if (c->levels)
   {
    if (a->addr >= 0 && a->addr < 16) bitno = a->addr;   // bit
   }

  w = (a && a->vect);                       // W option, write symbol (via size = word)
  w +=2;                                    // with cmd flag

  if (c->npars < 3)
  {
	c->p[1] = 0;         // address fix....
	c->p[2] = 0;  
  }  
  new_sym(w,c->symname, c->p[0], bitno, c->p[1], c->p[2]);
 return 0;
}


int set_subr (CPS *c)
{
  SXT *xsub;
  SXDT *d;
  xsub = add_subr(c->p[0]);

  if (xsub)
   {
     if (*c->symname)
        {
         new_sym(2,c->symname,c->p[0], -1,0,0);     // cmd set if sym specified
        }
     add_scan (c->p[0], S_CSUB|C_CMD, 0);

     if (c->levels <= 0 || !c->adnl) return 0;    // no addnl block

     d = get_sxdt(xsub,1);

     if (d)
      {
       xsub->cmd = 1;              // only set if extra cmds
       d->cadt = c->adnl;          // add chain to subroutine
       c->adnl = NULL;             // and drop from cmnd structure
      }
     xsub->nargs1 = c->size;   // and set sub size 
   }
  
  return 0;
 }

void set_banks(void);
void cpy_banks(void);


 int check_bank(int bk)
{
  if (bk == 0 || bk == 1 || bk == 8 || bk == 9) return 1;
  return 0;  
}

int set_ordr(CPS *cmnd)
 {
   int i,ans;
  // not allowed if single bank !!
  
  if (!numbanks)
    {
     do_error("Order IGNORED for 1 bank", cmnd->posn);
     return 1;
    }

  if (cmnd->npars != numbanks+1)
     {do_error("Num params not equal to banks detected", cmnd->posn); 
     return 1; }

  ans = 1;
  for (i = 0; i < numbanks+1; i++)
   {
    if (!check_bank(cmnd->p[i]))
       {
        do_error("Illegal Bank Number", cmnd->posn);        
        ans = 0;
       }
   }    

 if (!ans )  return 1;
            
// check bank nums first.....

  #ifdef XDBGX
   DBGPRT("set order = %d,%d,%d,%d\n", cmnd->p[0],cmnd->p[1],cmnd->p[2],cmnd->p[3]);
  #endif
  
  bkmap[2].dbnk = cmnd->p[0];
  bkmap[3].dbnk = cmnd->p[1];
  bkmap[4].dbnk = cmnd->p[2];
  bkmap[5].dbnk = cmnd->p[3];
  
  // and reset bok flags for copy....
  bkmap[2].bok = 1;
  bkmap[2].bok = 1;
  bkmap[2].bok = 1;
  bkmap[2].bok = 1;
  
  cpy_banks();
  set_banks();
  return 0;
 }

int set_time(CPS *c)
 {
  LBK *blk;
  int val,type,bank;
  int xofst;
  short b, m, sn;
  SYT *s;
  CADT *a;
  char *z;

  blk = set_cmd (c->p[0],c->p[1],c->fcom|C_CMD);
  set_adnl(c,blk);
  new_sym(2,c->symname,c->p[0], -1,0,0);  // safe for null name

  // up to here is same as set_prm...now add names

  if (!blk) return 1;          // command didn't stick
  a = blk->adnl;
  if (!a) return 0;                     //  nothing extra to do
  sn = 0;                             // temp flag after SIGN removed
  if (a->addr) sn = 1;
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
     s = new_sym(0,nm,a->addr,type,0,0);     // add new (read) sym
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
        new_sym(0,nm, val, b, 0, 0);
        }
      else     xofst+=2;                // jump over extras
     }
   }

return 0;
      // upd sym to add flags and times
 }


int set_bnk (CPS *cmnd)
{
  int bk;
  BANK *b;

   
   bk = cmnd->p[0];          // first param
   if (!check_bank(bk)) {do_error("Illegal Bank Number", cmnd->posn); return 1;} 
   
   if (bkmap[bk].cmd)
   {
     do_error("Bank already defined",cmnd->posn);
     return 1;
   }

   if (cmnd->p[1] < 0 || cmnd->p[1] > fldata.fillen) {do_error("Illegal File Offset", cmnd->posn); return 1;}
   
   if (cmnd->p[2] - cmnd->p[1] > 0xdfff) {do_error("start->end gap too large", cmnd->posn); return 1;}
   
   
   if (bkmap[2].btype)
   { 
   // kill all banks ready for new commands
   memset(bkmap,0,sizeof(bkmap));
   bkmap[0].opbt = opbit;                   // bit flag array reset
   bkmap[1].opbt = opbit + 0x800;           // = 32 bit   0x1000 = 16 bit ints
   bkmap[8].opbt = opbit + 0x1000;
   bkmap[9].opbt = opbit + 0x1800;
   numbanks = -1;          
   }
   
   // now add one bank

   b = bkmap+bk;
    
   b->fbuf = ibuf + cmnd->p[1];          // file offset
   b->filstrt = cmnd->p[1];
   b->minpc = PCORG;
   b->maxbk = cmnd->p[2]-PCORG;                  //
   if (cmnd->p[3])  b->maxpc = cmnd->p[3];

   if (cmnd->p[3] < PCORG || cmnd->p[3] > b->maxbk)
   {  do_error("fill start outside start and end", cmnd->posn); return 1;}
   
   b->cmd = 1;
   b->bok = 1;

   numbanks++; 


   set_banks();              // on final bank layout, not 2,3,4,5  no harm called for each bank.....
 
   #ifdef XDBGX
     DBGPRT("Bank Set %d %x %x ",cmnd->p[1], cmnd->p[2],cmnd->p[3]);
     DBGPRTN(" #( = %05x - %05x fill from %05x)",b->minpc, b->maxbk, b->maxpc);
   #endif
   return 0;
 }


int set_vect (CPS *c)
{                          // assumes vect subrs in same bank as pointers
  int i, ofst,bank;
  LBK *blk;
  CADT *a;

  a = c->adnl;
  blk = set_cmd (c->p[0],c->p[1], C_VECT|C_CMD);   // by cmd
   
  if (PMANL) return 0;
  set_adnl(c,blk);

  bank = g_bank(c->p[0]);

  if (a && a->foff && bkmap[a->addr].bok) bank = a->addr << 16;

  for (i = c->p[0]; i < c->p[1]; i += 2)
    {
      ofst = g_word (i);
      ofst |= bank;
      add_scan (ofst, S_CSUB, 0);      // adds subr
    }
    
    return 0;
}

int set_code (CPS *c)
{
  set_cmd (c->p[0], c->p[1], c->fcom|C_CMD);   // by cmd
  return 0;
}

int set_scan (CPS *c)
{
  add_scan (c->p[0], S_SJMP|C_CMD, 0);
  return 0;
}


int set_args(CPS *c)
{
  LBK *blk;
  blk = set_auxcmd (c->p[0], c->p[1], c->fcom|C_CMD);  // by cmd
  set_adnl(c,blk);
  set_data_vect(blk);
  #ifdef XDBGX
  DBGPRTN(0);
  #endif
  return 0;
}

int set_cdih (CPS *c)
{
   set_auxcmd(c->p[0],c->p[1],c->fcom|C_CMD);
   return 0;
}



/***************************************************************
 *  do last phase listing
 *
 *  loop within loop for faster speed
 ***************************************************************/


 void do_listing (void)
{
  int ofst;
  short i, ix;
  LBK *x, *y;
  BANK *b;
  anlpass = ANLPRT;                   // safety
  getcmnt(&cmntofst, &cmtt);          // initialise comments
  *autocmt = 0;
  cmntinst = 0;

  // print full bank profile first ??
  newl(1);
  fprintf (fldata.fl[1], "####################################################################################################");
  newl(1);
  fprintf (fldata.fl[1],"#\n# Disassembly listing of binary '%s' ",fldata.bare); 
  if (P8065) pstr("(8065, %d banks)",numbanks+1 ); else pstr("(8061, 1 bank)");
  fprintf (fldata.fl[1], " by SAD Version %s", SADVERSION);
  fprintf (fldata.fl[1],"\n# See '%s%s' for warnings, results other info", fldata.bare, fd[2].suffix);
  fprintf (fldata.fl[1],"\n\n# Key  R general register, with extra letters if opcode has mixed sizes (e.g DIVW)");
  fprintf (fldata.fl[1],"\n#      L long (4 bytes), W word (2 bytes), Y byte, S signed. Unsigned is default");
  fprintf (fldata.fl[1],"\n# Others"); 
  fprintf (fldata.fl[1],"\n#      [ ] = address of (always word), '++' increment register after operation" );
  fprintf (fldata.fl[1],"\n# Processor Status flags (for conditional jumps)" );
  fprintf (fldata.fl[1],"\n#      CY = carry, STC = sticky, OVF = overflow, OVT = overflow trap\n");
  fprintf (fldata.fl[1],"####################################################################################################");
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

     for (ofst = b->minpc; ofst < b->maxbk; )
      {
                // first, find command for this offset
       y = (LBK*) schmem();
       y->start = ofst;
       ix = bfindix(&cmdch,y);             // search for ofst

       x = (LBK*) cmdch.ptrs[ix];

       if (ix >= cmdch.num || cmdch.equal(ix,y))
         {                            // NOT found, set up a dummy block
         x = (LBK*) chmem(&cmdch);
         x->start = ofst;
         x->end   = b->maxbk;         // safety catch
         x->fcom  = C_DFLT;

         if (ix < cmdch.num)
             {                    // next block is 'found' block - safety.
              y = (LBK*) cmdch.ptrs[ix];
              if (x->end >= y->start) x->end = y->start -1;
             }
          ix = cmdch.num;                 // where temp 'x' is...........
         }

       if (x->start > x->end)          // safety check
         {
   //       PBKN(x, "Invalid cmd/order - set 1");
          x->end = x->start+1;
          ofst++;
          continue;
         }

       memset(curops,0, sizeof(OPS)*4);               // clear all ops

       while (ofst <= x->end)
         {
          show_prog (anlpass);
          pp_comment(ofst, 0);                          // before, and any auto ones
          ofst = dirs[x->fcom].prtcmd (ofst, ix);
          if (!val_data_addr(ofst)) break;             // safety - probably redundant
	      if (ofst > x->end)  cmntnl = 1;
  	      pp_comment(ofst, cmntnl);                    // after, with newlines
	      cmntnl = 0;
         }
      }      // ofst
    }        // bank
}


void scan_blk(SBK *blk)
 {

  blk->stop = 0;                           // just in case...
  put_scanpars(blk);                       // restore saved values

 while (!blk->stop)
  {
   do_code (blk);                         // do op decode first to get curinst, codebank, etc
   if (blk->held) break;                  // scan block is on HOLD, so exit
   blk->curaddr = blk->nextaddr;          // next opcode
  }

}


void scan_all (void)
{
  SBK *s;
  int ix, nx;   // nx to check if scans changed

  ix = 0;

  nx = scanch.num;                     // no of scan blocks
  while (ix < scanch.num)
   {

    s = (SBK*) scanch.ptrs[ix];

    if (!s->scanned && !s->held)
      {
       if (anlpass >= ANLPRT) newl(1);                // new line for new block print
       show_prog (anlpass);

        #ifdef XDBGX
          DBGPRTN(0);
    //   if (s->tst) DBGPRT("TEST ");
	      DBGPRTN(0);
          DBGPRT("SCAN %05x from %05x t=%d ret=%05x",s->start, s->curaddr, s->ctype,s->retaddr);
          DBG_scanpars(s);      //->pars);
          DBGPRTN(0);
		#endif  

     
       if (PLABEL)
        {          // static or cond jump, add auto label name (goto)
        if (s->ctype != S_CSUB)  add_nsym (2, s->start);
        }
        
       scan_blk(s);
       
       if (scanch.num != nx) ix = -1;                 // scan chain changed somewhere, restart loop at ix = zero
       if (!s->held)
          {
           if (unhold_scans (s)) ix = -1;            // unlock any held scan blocks, restart
          }
       tscans++;

     if (tscans > scanch.num * 16)               // loop safety check
       {
        wnprtn("\n *********");   
        wnprtn(" abandoned - too many scans - a loop somewhere ?");
        wnprtn("\n *********"); 
        printf("FAILED - too many scan loops\n");
        break;
       }
    }
   ix++;
   }
 }


/* find functions */

void incpc (int *ofst, short inc)
{
  *ofst += inc;
  if (*ofst >= maxadd(*ofst))
    {
	  #ifdef XDBGX
       DBGPRTN("increment outside ROM [%05x] ignored", *ofst);
	  #endif 
    }
}



void set_tab(int ix)
{

 /* sort out table to full size.
 * doesnt work if table abuts large FILL area !!
 * must check next entry starts on table row boundary....
 * row size in adnl->cnt
 * single word skip was for errant addresses which appear in multiplies etc...
 */

  int cols, ofst, rows, xend;
  DDT *d, *n;
  LBK *blk;
  CADT *a;
  
// probably need a maxend safety here....., and a sign check....

  d = (DDT*) datch.ptrs[ix];

  cols = d->p1;                   // cols size;
  ofst = d->start;
  rows = 0;
  ix++;

  n = (DDT*) datch.ptrs[ix];    // next data entry
   
  while (ofst < maxadd(ofst))
    {
     ofst+=cols;                // row by row
     rows++;                    // row count
     
     if (ofst >= n->start)
      {
       if (n->tab || n->func || n->p1 > 1)
         {
           if (ofst > n->start)  rows--;
           break;
           }     // fits to next struct
       else
       {
        // decide if this can be skipped
        if (!n->tab && !n->func) 
        {        // ignore - MORE PROBABLY REQUIRED 
         ix++;
         n = (DDT*) datch.ptrs[ix];    // next data entry
        }
       } 
      }
    }  

// need to fix ofst up to whole rows and cols in case of a filler byte...

xend = d->start;
xend += (rows*cols) -1;
 
  blk = set_cmd(d->start, xend, C_TABLE);

     if (blk && !blk->cmd)                // in case tab by user cmd....
       {
        add_nsym(4,d->start);                  // auto table name, can fail
        a = add_cadt(&(blk->adnl)); //,0);    // add at front
        blk->size = d->p1;                // size for one line ( = cols)
        a->cnt = d->p1;
        a->dcm = 1;                       // default to decimal (PDCM)? 1: 0;
        a->ssize = d->ssize;
       }

       // probably need some more checks, but simple version works for now...
 
}


int set_stval(int size, int *sval, int *nval)
{

  if (size == 1)
    {                      // unsigned byte
      *sval = 0xff;
      *nval = 0;
     return 1;          // ok   
    }

  if (size == 2)
    {  
      *sval = 0xffff;
      *nval = 0;
      return 1;          //ok
    }
    
  if (size == 5)
    {  
     *sval = 0x7f;        // signed byte
     *nval = 0x80;  
     return 1;
    }
    
  if (size == 6) 
    {      
     *sval = 0x7fff;
     *nval = 0x8000;
     return 1;
    }

  return 0;
  }   


void set_func(int ix)
{
 /*  extend functions to true 'end'.  Funcs should be
 *  unsigned in - start 0xffff or 0xff,  end 0    (word,byte)
 *  signed in   - start 0x7fff or 0x7f,  end 0x8000 or 0x80
 *  Output - use better flag detect, so no checks.
 */


  int sval, nval, val3, xofst, xend, maxend, err;
 
  int cnt, osze, isze;      //, odir;        // start and end values, and sign bit
  int incr, esize;          // increment, entry (read) size for g_val

  CADT *a;
  LBK *blk;
  DDT *d, *n;

  d = (DDT*) datch.ptrs[ix];
  #ifdef XDBGX
   DBGPRTN("DDT FUNC %05x, sz %x %x", d->start, d->ssize, d->p1);
  #endif 
 
  maxend = maxadd(d->start);

  // get right sign first.....

  xofst = d->start;
  isze = d->ssize & 3;                     // start as unsigned
  osze = isze;                             
 
  val3 = g_val(xofst, isze);         // read first input value (as unsigned)

  if (!set_stval(isze, &sval, &nval)) { wnprt("invalid size in func"); return ;}

  if (val3 != sval)  // try alternate sign....
    {
     isze ^= 4;          // swop sign flag
 //    wnprtn("Swop sign in - val %x (%x)", val3, sval);
     set_stval(isze, &sval, &nval);  // and try again
          
     if (val3 != sval)
      {
        wnprtn("Func %05x Invalid start value %x", d->start, val3);
        return;
      }
    }
    
  // val3 = first row

 esize = isze & 3;
  n = (DDT*) datch.ptrs[ix+1];  // next cmd

 // esize &= 3;             // drop sign
  xend  = 0;
  cnt = 0;
  err = 0;
  incr = casz[esize] *2;                  // whole row increment

  while (xofst += incr)                   // check input value of each row (UNSIGNED)
    {
     if (xofst > maxend) err = 3;
     val3 = g_val(xofst, esize);          // next input value

     if (xend)
       {           // track WHOLE ROW from end value
        val3 = g_val(xofst, esize+1);     // get whole row
        if (val3 != sval) {err=4; break;}
        cnt++;
        if (cnt > 32) {err=4; break;}          // safety
        if (xofst >= n->start)
           { if (!val3 || n->func || n->tab){err=5; break;}}  // and check for next block (any if zero)
        xend = xofst+incr-1;                   // update end  
       }
     else
     if (val3 == nval)
       { 
        xend = xofst+incr-1;                        // hit end value, set end
        sval = g_val(xofst, esize+1);               // get whole row for next check
       }
     else
       {
        if (xend) break;                            // have run over struct end
        if (val3 == sval) cnt++; else cnt = 0;      // count any matching rows
        if (cnt > 16) err = 2;                       // 8 identical data rows - STOP
        sval = val3;                                // save this value for next row check
       }

     if (err)
       {
		 #ifdef XDBGX
           DBGPRT(" err=%d ", err); 
		 #endif  
         if (!xend) xend = xofst-1;
         break;
	   }
    }          // while

 
  // if (d->cmd)
     {  // user defined - restore originals
      isze = d->ssize;
      osze = d->p2;
     }


// OK now add function cmd

    blk = set_cmd (d->start, xend, C_FUNC);

    if (blk && !blk->cmd)
      {                                 // cmd OK, add size etc (2 sets for func)

       add_nsym(3,d->start);            // auto func name, can fail
       blk->size = incr;               // size of one row

       a = add_cadt(&(blk->adnl)); //,0);                       // at front
       if (a) add_cadt(&(blk->adnl)); //,1);                    // at end
       a->cnt = 1;
       a->ssize = isze;                         // size, word or byte
       a->dcm = 1;                              // default to decimal print

       a->next->cnt = 1;
       a->next->ssize = osze;                    // size out
       a->next->dcm = 1;                        // default to decimal
      }

}

void set_ddt(int ix)
{

DDT *d;
//LBK *blk;

int sz;

d = (DDT*) datch.ptrs[ix];

if (d->psh) return;                       //ignore pushed addresses (code)

sz = casz[d->ssize];

if (sz == 1)
{

set_cmd (d->start, d->start, C_BYTE);


}

if (sz == 2)
{

set_cmd (d->start, d->start+1, C_WORD);

}


/*

if (d->ssize  & 3)


blk = set_cmd (d->start, xend, C_FUNC, 0);

    if (blk && !blk->cmd)
      {                                 // cmd OK, add size etc (2 sets for func)

       add_nsym(3,d->start);            // auto func name, can fail
       blk->size1 = incr;               // size of one row

       a = add_cadt(&(blk->addnl),0);                       // at front
       if (a) add_cadt(&(blk->addnl),1);                    // at end
       a->cnt = 1;
       a->ssize = d->ssize;                    // size, word or byte
       a->dcm = 1;                              // default to decimal print

       a->next->cnt = 1;
       a->next->ssize = d->sz2;              // size
       a->next->dcm = 1;                        // default to decimal
      }

}


*/


}









 void do_ddt(int type)
 {
  int ix;
  DDT *d;
  LBK *k;

  if (PMANL) return;

  #ifdef XDBGX
   DBGPRTN("do_ddt");
  #endif
 
  for (ix = 0; ix < datch.num; ix++)
   {
    d = (DDT*) datch.ptrs[ix];  // this func
    if (!d->dis)
    {  
     if (type == 1)
	 { // do vects, but not 'return to here'  LA3 needs this, but buggers up A9L.....
	  if (d->psh && d->imd && d->start != d->from)
	   {
		 k = (LBK*) schmem();  
		 k->start = d->start;
		 k = (LBK*) find( &cmdch,k);
         if (!k) add_scan(d->start, S_CSUB, 0);
	   }
     }   
    else	
	   {	
        if (d->tab) set_tab (ix);
        else
        if (d->func)  set_func(ix);
        else set_ddt(ix);
       }
   } 
  }
 }



//********** command processing section **************







void DBG_sigs(CHAIN *x)
{
 int i, ix;
 SIG *y;

 wnprtn("\n-----------Signatures---------[%d]--", x->num);
 wnprtn("d ofst  end    k    name      pars" );

 for (ix = 0; ix < x->num; ix++)
   {
     y = (SIG*) x->ptrs[ix];
     wnprt("%d %5x %5x %2d %7s  ", y->done, y->ofst, y->xend, y->skp, y->ptn->name);
     for (i = 0; i < NSGV; i++) wnprt("%5x," , y->v[i]);
     wnprtn(0);
    }

 wnprtn(0);
}





//*********************************************************************************


void add_iname(short ix, int ofst)
{
uint i, x;
char *z;

 // ignore_int can be at the end of a jump in multibanks....

 if (!PINTF) return;

 ix -= 0x10;
 ix /= 2;          // get correct index for table lookup


 x  = (P8065) ? 1 : 0;
 z = nm;

 if (numbanks)               // sort out name
   {
    i = ofst >> 16;             // add bank number after the I
    z += sprintf(nm, "I%d_",i);
   }
 else
  z += sprintf(nm, "I_");

 if (do_sig(&intign, ofst))  // this is 'ignore int' signature, use 'ignore' name
    {
      sprintf(z,"Ignore");
      new_sym(0,nm, ofst, -1,0 ,0);
      return; // nm;
    }

 for (i = 0; i < NC(inames); i++)
    {
    if (ix <= inames[i].end && ix >= inames[i].start && inames[i].p5 == x)
       {
       z += sprintf(z,"%s",inames[i].name);                      // number flag set
       if (inames[i].num) z+=sprintf(z, "%d", ix-inames[i].start);
       new_sym(0,nm, ofst, -1, 0, 0);
       break;    // stop loop once found for single calls
       }
    }
return;
 }




/*******************command stuff ***************************************************/





int readpunc(int posn, int end)
{
 while (posn <= end && (flbuf[posn] == ' ' || flbuf[posn] == ',')) posn++;
 return posn;
 }



int getp(int *v, int *posn)
 {
  int ans, rlen;   
  ans = sscanf(flbuf+*posn, "%5x%n %5x%n %5x%n %5x%n", v, &rlen, v+1, &rlen, v+2, &rlen, v+3, &rlen);
  if (ans) *posn += rlen;
  return ans;
 }




int chk_startaddr(CPS *c, DIRS* dir)
 {
   // check ans fixup any address issues (bank etc) 
  int start, end, ix;

  ix = dir->vpos;
  if (ix)
   {            // single address, with or without a start/end pair following  
    start = c->p[ix-1]; 
    if (!numbanks && (start <= 0xffff && start >= PCORG)) start |= 0x80000;
    c->p[ix-1] = start;
   }	

  ix = dir->spos;
  if (ix <= 0) return 0;
                                  // address pair
  start = c->p[ix-1];             // so zero is ignored....
  end   = c->p[ix];
     
  // autofix single banks to bank 8 
  if (!numbanks && (start <= 0xffff && start >= PCORG)) start |= 0x80000;

  if (!end) 
    {
      // if fixed size command (word byte etc) then calc end automatically
      end = start;
    }

  if (end <= 0xffff) end |= g_bank(start);      // add start bank

  if (!bankeq(start,end))  {do_error("Banks don't match",c->posn);return 8;}   // banks don't match
  
  if ( end < start) {do_error("End less than Start",c->posn);return 8;}   // banks don't match
    
  c->p[ix-1] = start;
  c->p[ix]   = end;  

 return 0;      
 }



int do_adnl_letter (CPS* cmnd, char optl)

{
     CADT *c;
     int ans, rlen;
     int v[4];
     float f1;
     SIG * z;     
     // switch by command letter - moved out of parse loop for clarity
 
     c = cmnd->adnl;
     z = 0;                // safety....

     while(c && c->next) c = c->next;          // make sure at end of chain
    
	 if (!c && optl != ':') return do_error("Missing Colon", cmnd->posn);
	
     switch(optl)
          {
           // case 'A':                    // AD voltage ?? / RPM/ IOtime....
           //    cadd->volt = 1;
           //    break;

           // case 'B' :
           //    cadd->f.foff = 1;
           //    cadd->divisor  = ans;    // Byte + fixed address offset (funcs)
           //    break;

           // case 'C' :
           //    break;
		   
           case '|' :
		      c = add_cadt(&(cmnd->adnl));
	          cmnd->levels++;
              c->newl = 1;                    // mark split printout here
			  cmnd->newl = 1;                  // flag in main cmd struct too
              break;		   
		   
		   case ':' :
 
              // colon add a new CADT struct (= level) at end of chain
              c = add_cadt(&(cmnd->adnl));
	          cmnd->levels++;
              break;
   
			  
           case 'D' :

             // 1 hex value (address offset)
             ans = sscanf(flbuf+cmnd->posn, "%5x%n ", v, &rlen);
             if (!ans) return do_error("Param reqd", cmnd->posn);
             cmnd->posn+=rlen;
             c->foff = 1;
             c->addr  = v[0];    // Word + fixed address offset (funcs) - bank swop vect
             break;


           case 'E' :
             // ENCODED 2 hex vals
             ans = sscanf(flbuf+cmnd->posn, "%5x%n %5x%n", v, &rlen, v+1, &rlen);
             if (ans != 2 )  return do_error("2 Params reqd", cmnd->posn);
              
             cmnd->posn+=rlen;
             c->enc  = v[0]; 
             c->addr = v[1];
             c->ssize = 2;   // encoded, implies word
              
             break;

           case 'F' :
               // special subr function read but ignored - need to put back
               // f 1 <reg1>   <size+sign> <sign>    func
               // f 2 <reg1> <reg2>  <size+sign>    tab

               // f 3 <reg1> <size>           variable ?
               // f 4 <?>     <size1> < size2>  switch between 2 sizes args ?
               
               // cmnd.fcom to check ..

               ans = sscanf(flbuf+cmnd->posn, "%5x%n", v, &rlen);
                  
               if (ans != 1)   return do_error("Invalid Func type", cmnd->posn);
               cmnd->posn+=rlen;
               if (v[0] < 1 || v[0] > 2) return do_error("Invalid Func type", cmnd->posn);
                { 
                        //    add a SIG here to do the job
                  c->disb = 1;                          // disable this cadt entry
                  cmnd->z = (SIG *) chmem(&sigch);
                  z =  cmnd->z;
                  z->ofst = cmnd->p[0];
                  z->xend = cmnd->p[0];        // start and end address of subr
                  z->v[0] = 0x100000;          // special address as marker
                  z->v[14] = 1;
                  z->v[15] = 1;               // unsigned
               
               if (v[0] == 1)
                {
                 ans = sscanf(flbuf+cmnd->posn, "%5x%n", v, &rlen);
                 if (ans != 1)   return do_error("Invalid address register", cmnd->posn);
                 cmnd->posn+=rlen;   
                    
                 z->ptn = &fnlp ;           // function
                 z->v[2] = 1;                // default to byte
                 z->v[4] = v[0];            // register for address
                } 
               else
                {                           // table  
                 ans = sscanf(flbuf+cmnd->posn, "%5x%n %5x%n", v, &rlen, v+1, &rlen);
                 // error ?
                 cmnd->posn+=rlen;
                 z->ptn = &tblp;
                 z->v[4] = v[0];             // address register
                 z->v[3] = v[1];             // tab column register

             
                }
              inssig(z);            // insert sig into chain
  
             }
             break;

  // G H I J K

           case 'L':                // long int
             c->ssize &= 4;        // keep sign
             c->ssize |= 3;
             break;

 // case 'M':                // mask pair ?, use esize to decide word or byte
 //    cadd->mask = 1;
 //    break;

           case 'N':
             c->name = 1;       // Name lookup
             break;

           case 'O':                // Cols or count
             // 1 decimal value

             ans = sscanf(flbuf+cmnd->posn, "%d%n ", v, &rlen);
             if (ans>0)
               {
                 cmnd->posn+=rlen;
                 c->cnt = v[0];            // must be positive ?
               }     else return do_error("Param reqd", cmnd->posn);  
             break;

           case 'P':               // Print fieldwidth   1 decimal
             ans = sscanf(flbuf+cmnd->posn, "%5d%n ", v, &rlen);
             if (ans>0){ cmnd->posn+=rlen;
             c->pfw = v[0];
               }     else return do_error("Param reqd", cmnd->posn);
              break;

           case 'Q' :              // terminator byte
             cmnd->term = 1;
             break;

           case 'R':               // indirect pointer to subroutine (= vect)
             c->ssize = 2;        // safety, lock to WORD
             c->vect = 1;
             c->name = 1;         // with name by default
             break;

           case 'S':              // Signed
             c->ssize |= 4;
             if (cmnd->z)
                {
                  z = cmnd->z;  
                  if (z->v[1]) z->v[14] = 2;   // signed out
                  else
                    { z->v[15] = 2; // signed in
                     if (z->ptn == &fnlp) z->v[1] = 1;
                    }
  
                } 
             break;

           case 'T':                 // biT
               // read one decimal val
             ans = sscanf(flbuf+cmnd->posn, "%5d%n ", v, &rlen);
             if (ans > 0) {
               cmnd->posn+=rlen;
               c->addr = v[0];
               }    else return do_error("Param reqd", cmnd->posn);
             break;

           case  'U' :  //not used , except for funcs (default uns)
              c->ssize &= 3;         // clear sign
              if (cmnd->z)
               {
                 z = cmnd->z;  
                 if (z->v[1])  z->v[14] = 1;   // unsigned out
                 else
                     {z->v[15] = 1; // unsigned out
                      if (z->ptn == &fnlp) z->v[1] = 1;
                     }
                }   
                     
              break;

           case 'V':                  // diVisor
             // read one FLOAT val
             ans = sscanf(flbuf+cmnd->posn, "%f%n ", &f1, &rlen);       // FLOAT
             if (ans > 0)
             {cmnd->posn+=rlen;
               c->divd = f1;                                       // now go FULL FLOAT
             }     else return do_error("Param reqd", cmnd->posn);
             break;

           case 'W':                 // Word   - or write for syms     /
             c->ssize &= 4;          // keep sign
             c->ssize |= 2;
             if (cmnd->z)  cmnd->z->v[2] = 4;      // set size of func to word
             break;

           case 'X':                 // Print in decimal /
             c->dcm = 1;
             break;

           case 'Y':                   // bYte/
             c->ssize &= 4;           // keep sign, but ditch rest
             c->ssize |= 1;
             if (cmnd->z)  cmnd->z->v[2] = 2;      // set size of func to byte
             break;

         // Z
 
           default:
              return do_error("Illegal Option", cmnd->posn);

          }          // end switch
 return 0;
    }   



void  do_opt_letter (CPS* cmnd, char optl)
{
    // this is a  cmd: ABCD type (no params)
    
    int j;
    
    if (optl >= 'A' && optl <= 'Z')
	  {
       j = optl - 'A';                      // convert letter to bit offset
       cmnd->p[0] |= (1 << j) ;
	 }
 }



int parse_com(char *flbuf)
{
  // parse (and check) cmnd return error(s) as bit flags;

  uint j;
  int maxlen,rlen,ans;

  char rdst[32];
  char *t, *e;                                  // where we are up to

  DIRS* crdir;
  CPS  cmnd;                                            // for parse command
  CADT *c;

  memset(&cmnd, 0, sizeof(CPS));                       // clear entire struct
  c = 0;
  
  crdir = NULL;
  maxlen = strlen(flbuf);    // safety check


// ans = sscanf(flbuf, "[\n\r#]");      // alternate method ?
// if (ans)  flbuf[end] = '\0';

  e = flbuf + maxlen;

  t = strchr(flbuf, '\n');            // check for end of line
  if (t && t < e) e = t;                // remove it

  t = strchr(flbuf, '\r');            // check for end of line
  if (t && t < e) e = t;                 // remove it

  t = strchr(flbuf, '#');            // stop at comment
  if (t && t < e) e = t;                  // remove any comment

  maxlen = e - flbuf;
  
  if (maxlen <=0) return 0;                // null line;

  cmnd.posn = 0;

  cmnd.posn = readpunc(cmnd.posn,maxlen);
  ans = sscanf (flbuf, "%7s%n", rdst, &rlen);    // n is no of chars read, to %n

  if (ans <= 0) return 0;                // no command - null line;

  // get command

  for (j = 0; j < NC(dirs); j++)
   {
    if (!strncasecmp (dirs[j].com, rdst, 3) )      // 3 chars min
      {
       cmnd.fcom = j;                 // found
       crdir = dirs+j;                // ptr to command entry
       break;
      }
   }
  cmnd.posn += rlen;
  if (!crdir)  return do_error("Invalid Command",cmnd.posn);             // cmnd not found


  cmnd.posn = readpunc(cmnd.posn, maxlen);


  if (crdir->maxps)       // read up to 4 following addresses into p (if any)
    {
     ans = getp(cmnd.p, &cmnd.posn);

     if (ans < 1) return do_error("Parameters Required",cmnd.posn);
     if (ans > crdir->maxps) return do_error("Too many Parameters",cmnd.posn);
     cmnd.npars = ans;
     
     // verify addresses here - change to allow single start for everything....

     ans = chk_startaddr(&cmnd, crdir);
 
     if (ans) return ans;
    }
    
  // possible name here

  cmnd.posn = readpunc(cmnd.posn, maxlen);

  if (crdir->namee && cmnd.posn < maxlen &&  flbuf[cmnd.posn] == '\"')
   {
    cmnd.posn++;   // skip quote
    ans = sscanf(flbuf+cmnd.posn," %32s%n", cmnd.symname,&rlen);     //max 32 char names.
    if (ans) cmnd.posn += rlen;
    
    t = strchr(cmnd.symname, '\"');                 // drop trailing quote from syname
    if (t) *t = 0;
	else return do_error("Missing close quote", cmnd.posn);
   }

  // now read levels


  if (crdir->opts) 
   {        // if no opts, skip this
      
     // read option chars and level markers whilst data left

      while (cmnd.posn < maxlen)
       {
        cmnd.posn = readpunc(cmnd.posn,maxlen);
        if (cmnd.posn >= maxlen) break;         // safety
     
        ans = sscanf(flbuf+cmnd.posn,crdir->opts, rdst, &rlen);
        t = flbuf+cmnd.posn;                                               // remember char
	
        if (ans <= 0) return do_error("Illegal Option", cmnd.posn);        // illegal option char (4)
   
	    cmnd.posn += 1;                                                    // Not rlen, only single char here
    
        if (!crdir->maxps) do_opt_letter(&cmnd, rdst[0]);                  // cmd : ABC  style
        else
         {
          if (do_adnl_letter (&cmnd, rdst[0]))  return 1;
         } 
         
      }      //  end get more options (colon levels

   }  // end if options/adnl

 
  c = cmnd.adnl;

  while(c)
   {
    cmnd.size += dsize(c);   // set final size
    if (cmnd.fcom >= C_TABLE && cmnd.fcom <= C_FUNC) c->dcm = 1;  // decimal default
    c = c->next;
   }

  c = cmnd.adnl;

   if (cmnd.fcom == C_FUNC &&  cmnd.levels  < 2)    // func must always have 2 levels
   { c->next = (CADT *) mem (0,0,sizeof(CADT));
     *(c->next) = *c;        // copy entry
     c->next->next = NULL;
     cmnd.levels ++;
     cmnd.size *= 2;             //csize
   }


dirs[cmnd.fcom].setcmd (&cmnd);

//  DBGPRTN("%s - Parse OK ", flbuf);
freecadt(&(cmnd.adnl));                                 // safety
show_prog(0);
return 0;
}




/***************************************************************
 *  read directives file
 ***************************************************************/
void getudir (void)
{
  int addr,addr2;
  uint i;
  int  j,bank;
  BANK *b;
  DFSYM *z;

  cmdopts = PDEFLT;           // always reset to default

  if (fldata.fl[3] == NULL)
    {
     wnprt("\n --- No directive file. Set default opts = C N P S");
      if (numbanks)
          {
           cmdopts |= H;        // 8065 multibank
           wnprt(" H");
          }
      wnprtn(0);
    }
  else
    {
      wnprtn("\n\n --- Read commands from directive file = %s", fldata.fn[3]);

     while (1)
	  {
	    if (!fgets (flbuf, 255, fldata.fl[3])) break;

        addr = parse_com(flbuf);
        show_prog(0);
	  }
    }

   j = (P8065)? 2:1;

  for (i = 0; i < NC(defsyms); i++)
    {
     z = defsyms+i;
     if (j & z->pcx) new_sym (z->wrt+2,z->symn, z->addr, z->bit1,0,0);
    }

  if (PMANL) return;


  // now setup each bank's first scans and interrupt subr cmds

  for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;
     if (!b->bok) continue;
	 
	 #ifdef XDBGX
      DBGPRTN("--- auto setup for bank %d ---",i);
	 #endif
 
     bank = i << 16;
     addr = PCORG;
     addr |= bank;

     add_scan (addr,S_SJMP|C_CMD,0);                    // inital scan at PCORG (cmd)
  
    // set_cmd (addr+0xa , addr+0xe, C_WORD,1);    // PCORG+0xa, cal pars and cmd
     j = (P8065) ? 0x5f : 0x1f;
     
     set_cmd (addr+0x10, addr+j, C_VECT|C_CMD);    // interrupt vecs, by cmd
     
     for (j -=1; j >= 0xf; j-=2)      // counting DOWN
	    {
	     addr2 = g_word (addr+j);
         addr2 |= bank;
         add_iname(j,addr2); 
         add_scan (addr2, S_CSUB|C_CMD,0);         // interrupt handling subrs
        }
     }
 }


short init_structs(void)
 {
  uint i;                                              // NO file stuff
  for (i = 0; i < NC(anames); i++) anames[i].nval = 1;
 
  memset(ramlist,0,NPARS*2);

  memset(bkmap,0,sizeof(bkmap));
  bkmap[2].opbt = opbit;                   // bit flag array start
  bkmap[3].opbt = opbit + 0x800;           // = 32 bit   0x1000 = 16 bit ints
  bkmap[4].opbt = opbit + 0x1000;
  bkmap[5].opbt = opbit + 0x1800; 
 
  memset(opbit,0, 0x8000);                 // clear all opcode markers
  memset(ibuf, 0, PCORG);                  // clear RAM and regs at front of ibuf 
  anlpass  = 0;                             // number of passes done so far
  gcol     = 0;
  lastpad  = 0;
  cmdopts  = 0;
  
  #ifdef XDBGX 
    DBGmaxalloc = 0;          // malloc tracing
    DBGcuralloc = 0;
    DBGmcalls   = 0;
    DBGmrels    = 0;
    DBGxcalls   = 0;
  #endif

  tscans    = 0;
  cmntofst  = 0;
  cmntnl    = 0;
  lpix      = 0;

  curinst = instpile;
  curops = curinst->opr;
  rambank = 0;

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

}




 /* first jump should start at 0 or 2000 within file for each bank.
  * fiddle banks ptrs temporarily to get sigs to scan the whole file
  * use a nominal bank 2,3,4,5 and fiddle the endpoint -

  * If we assume that bytes may be missing or extra then scan around the start
  * first, then scan whole front

  * 1 - at zero
  * 2 - at 0x2000          (1 & 2 are official starts)
  * 3 - -8 to +8           (missing or extra bytes)
  * 4 - full 0-2000        (last ditch effort)
  */
  

int scan_start (int ix, int faddr)
{
     // file is read in to ibuf[0x2000] onwards - look for start jump and interrupt vectors
 
    BANK *b;    
    SIG* s;
    int max, addr, bank, val, pass, x;

    b = bkmap+ix;
    bank = g_bank(b->minpc);      // FAKE bank from sig
    b->btype = 0;
    
    pass = 0;
    
	#ifdef XDBGX
     DBGPRTN("scan for bank start");
	#endif 
    while (pass < 4 && b->btype == 0)
     {
       switch(pass)
       {
          case 0:
            addr = faddr;                     // try at zero offset in this block, once
		    #ifdef XDBGX
              DBGPRTN("try0 = %x",addr);
            #endif
            max = faddr;
            break;
            
          case 1:
            addr = faddr + 0x2000;            // try a block along, once (0x2000 of front fill)
 		    #ifdef XDBGX
              DBGPRTN("try1 = %x",addr);
            #endif
			max = addr;
			break;
			
          case 2: 
            addr = faddr-8;                   // try +-8 around a zero pointer
            max = faddr + 8;
			#ifdef XDBGX
              DBGPRTN("try2 = %d to %d", addr,max);     // so minus works...
            #endif
            break;
          
          case 3:
            addr = faddr;
            max = addr + 0x2000;                        // search whole block ?
			#ifdef XDBGX
              DBGPRTN("try3 = %x to %x", addr,max);
            #endif
       } 


       while (addr <= max && b->btype == 0)
        {
         clearchain(&sigch);                    // clear out any sigs so far
         b->fbuf = ibuf + addr;
         s = do_sig(&hdr,0x2000|bank);             //hdr is pattern ref - but this is just a jump, really.... 

        if (s)
         {
          bank = g_bank(s->ofst);               // FAKE bank from sig
          if (s->v[1] <= s->ofst+s->skp)
           {
             b->btype = 2;                          // loopstop 
             b->filstrt = addr;                  // actual bank start (buffer offset) for 0x2000
             s->v[3] = 2;
           }
          if (nobank(s->v[1]) > 0x201f)
           {
             b->btype = 1;                         // jump over int regs.
             b->filstrt = addr;                  // actual bank start (buffer offset) for 0x2000
             s->v[3] = 1;
           }

          // now check that the int vects look OK.....if 8065 this can be longer...
          // possibly check for ignint too ??


          //if (s->skp > 2) b->btype = 0;                                    // TEST !! for extra front chars
		  if (nobank(s->xend) > 0x2009) b->btype = 0;
          for (x = (bank|0x2010);  x < (bank|0x201f);  x+=2)
           {               // 8061 ints
            val = g_word(x);
            if (val < 0x2020) b->btype = 0;              //    need more here !!  
           
   //         val = g_byte(bank|val); 
 
//            if (val && (val < 0xf0 || val > 0xf2) && val != 0x10) 
  //            {     
    //           b->btype = 0;                 // 0,ret, retei, pushp, bnk allowed  CARD !!
    //          }
           }
           
          // if it's a bnk, then the jump should go to one of the other existing int subrs....

         b->dbnk = 1;                        // temp flag
         for (x = (bank|0x2020);  x < (bank|0x205f);  x+=2)
           {
            val = g_word(x);
            if (val < 0x2060) b->dbnk = 0;                 // need more here !!  
           }

           if (b->btype)
             {
              faddr = addr; 
			  #ifdef XDBGX
                DBGPRTN("match at = %05x", faddr);    
			  #endif	
             } 
         }           // end of if (s)
         addr++;   
        
      }          // for cnt
     pass++; 
    }             // for pass
      
   return faddr;  

 }




void find_banks(void)
 {
  // file is in ibuf[0x2000] onwards
  // set TEMP banks [2]-[5] as these are not used in main decode
     
  int faddr, i, end, bank;
  BANK *b;

  lastbmap = bkmap;                         // for later on...
  numbanks = -1;   
  
  i = 2;                                    // use 2,3,4,5 as temp holders 
  end = fldata.fillen;
  faddr = 0;
 
  while (faddr < end && i < 6)           // 4 banks max for now
    {
     bank = i << 16;   
     b = bkmap+i;
     b->maxbk = bank | 0xffff;          // temp bank setup so that g_val (sigs) etc. work
     b->maxpc = b->maxbk;               // need for signature scan
     b->minpc = bank | PCORG;
     b->bok = 1;

     faddr = scan_start(i, faddr);             // test for start jump at or around around faddr

     if (b->btype > 0)
      {                                        // found !!
       faddr += 0xe000;                        // assume end of this bank, as a file offset
       b->filend = faddr-1;                 // this may be wrong, but this is default for now (0x2000 - 0xffff).
           
       if (faddr >= end) b->filend = end-1;                  // end of file
       
       b->maxbk = b->filend - b->filstrt + PCORG;
       b->maxbk |= bank;
       numbanks++;

       #ifdef XDBGX 
        DBGPRTN("temp Bank %05x-%05x, type %d", b->filstrt, b->filend, b->btype);
        DBGPRTN(0);
	   #endif	
       if (faddr >= end) break;
       i++;
     }
    else break;
    }
}
 
int find_text(BANK *b)
{
  // check for text in end filler block
  // text blocks have either "Copyright at ff63" or ".HEX* ff06"  in them...
  // check address above ff00 for copyright and then check blocks....
  // works for now, more or less
 
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
  set_cmd (start,ofst, C_TEXT |C_CMD);
  set_cmd (ofst+1,b->maxbk, C_DFLT|C_CMD);    // treat as cmd (cannot override)   
 
  ans = start - 1;

  ofst = (0xff06 | bk);
  cnt = 0;
  val = g_byte(ofst);
  
  while (ofst < (0xff63 | bk)) 
   {            // allow a couple of non printers...use 6....
    // if (!strncmp(".HEX", (char *) b->fbuf + (ofst & 0xffff), 4)) start = (0xff06 | bk);  // found in blk somewhere
     if (val < 0x20 || val > 0x7f) 
      {
        if (cnt > 5) set_cmd (ofst-cnt,ofst-1, C_TEXT |C_CMD);
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

   int ofst, cnt, val, last;
   
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
       set_cmd (ofst,last, C_DFLT|C_CMD);    // treat as cmd (cannot override)
       set_auxcmd (ofst,b->maxpc,C_XCODE);       // and xcode to match
       b->maxpc = ofst;
      }
  }  

void mkbanks(int num, ...)
{
 va_list ixs;
 int i;
 va_start(ixs,num);

 for (i = 2; i < 2+num; i++) bkmap[i].dbnk = va_arg(ixs,int);

 va_end(ixs);

}

void cpy_banks(void)
 {
  int i, bk;
  BANK *b;
  
 for (i = 2; i < 3+numbanks; i++)
 {
   b = bkmap + bkmap[i].dbnk;                // preset destination
   bk = (bkmap[i].dbnk << 16);
   *b = *(bkmap+i); 
   b->minpc &= 0xffff;
   b->minpc |= bk;    // substitute bank no
   b->maxpc = 0;       // do fill check
   b->maxbk &= 0xffff;
   b->maxbk |= bk;
   bkmap[i].bok = 0;
 }

}

void set_banks(void)
{
 int i;
 BANK *b;
 
  codbank = 0x80000;                         // code start always bank 8
  rambank = 0;                               // start at zero for RAM locations
  if (numbanks) datbank = 0x10000;           // data bank default to 1 for multibanks
  else   datbank = 0x80000;                  // databank 8 for single

  for (i = 0; i < BMAX; i++)
     {
         // maxpc used as flag to check if user entered....
         
      b = bkmap + i;
      if (b->bok && !b->maxpc) find_filler(b);
     }

// this is a temp fiddle for single bank 8065...

    if (!numbanks)
     {  
      i = g_word(0x82060);
      if  (i == 0x108) cmdopts |= H;        // single bank 8065, rbase at 2060
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
      stkreg2 = -1;
     }
 }



int do_banks(void)
 {
  // set up banks after find_banks, or user command.
    
  // set up RAM bank [16] first 0x2000 of ibuf for ram and regs
  // NOTE - not technically safe if file has missing front bytes, but probably OK

  int end,ofst,bank,i;
  BANK *b;

  b = bkmap+BMAX;
  b->maxbk = PCORG-1;               // 0-0x1fff in bank 16 for registers
  b->maxpc = PCORG-1;
  b->minpc = 0;
  b->fbuf  = ibuf;                  // first 0x2000  in bank 16 for regs and RAM
  b->bok = 1;

  // now do the checks and bank ordering.....
 
  if (numbanks)  cmdopts |= H;          // multibank bin, mark as 8065
  else
   {  
     if (bkmap[2].dbnk) cmdopts |= H;   // single bank 8065 detected from int.vects
   }

  bank = -1;                           // temp bank with the start jump (=code)
  end = 0;                             // temp number of banks with start jumps
  ofst = 0;                            // temp number of loopback jumps

  for (i = 0; i <= numbanks; i++)
      {
       b = bkmap+i+2;                // 2 to 5
       b->dbnk = 0;                  // clear any flags 
       if (b->btype == 1) { end++; bank = i; }              // a real jump
       if (b->btype == 2) ofst++;                           // loopback jump
      }

  if (!end)
     {
      wnprtn("Cannot find Code start bank [8]");
      return -4;
     }

  if (end > 1)
     {
      wnprtn("Found multiple Code start banks [8]");
      return -5;
     }

   #ifdef XDBGX 
   DBGPRT("%d Banks, start %d\n", numbanks+1, bank+1);
   #endif

  // this is a bit ugly, but it allows commands to override autosetup easily...
  // just set which bank is which 
 
  switch (numbanks)
    {

     default:
       wnprtn("No banks, or >4 banks found. File corrupt?");
       return -2;
       break;

     case 0:                      // copybanks autostarts at [2] for source
       mkbanks(1,8);   
       break;

     case 1:                      // 2 bank;
       if (bank == 1) mkbanks(2,1,8); else mkbanks(2,8,1);    
       break;

     // no 3 bank bins so far .....

     case 3:

       switch(bank)
        {
         // Alternate bank orders - best guess

         case 0:
           mkbanks(4,8,9,0,1);  
           break;

         case 2:
         default:
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

  return 0;  
}


int readbin(void)
{
  /* Read binary into main block 0x2000 onwards (ibuf[0x2000])
  *  ibuf  [0]-[2000]     RAM  (faked and used in this disassembly)
  *  ibuf  [2000]-[42000] up to 4 banks ROM, at 0x10000 each
  */

 // int end;

  fldata.fillen = fseek (fldata.fl[0], 0, SEEK_END);
  fldata.fillen = ftell (fldata.fl[0]);
  fseek (fldata.fl[0], 0, SEEK_SET);	            // get file size & restore to start

  wnprtn(" --- Input file = '%s'. It is %d (0x%x) bytes",fldata.fn[0], fldata.fillen, fldata.fillen);

//  fldata.error = 0;

  if (fldata.fillen > 0x40000)
   {
    wnprtn("BIN file > 256k. Truncated at 256k");  // is this always right ??
    fldata.fillen = 0x40000;
   }

  //end = 
  fread (ibuf+PCORG, 1, fldata.fillen, fldata.fl[0]);

  #ifdef XDBGX 
   DBGPRTN("Read input file (0x%x bytes)", fldata.fillen);
  #endif 

// check for file error ?
//  end = fldata.fillen;

  find_banks();
  do_banks();               // auto, may be modded later by command

  return 0;
}

void rbasp(SIG *sx, SBK *b)
{

  //add rbase entries from signature match
 
  int add, reg, i,j, cnt,bnk;
  RBT *r,*n;
  int  pc;

  if (b) return;                 // no process if subr set

  reg = (sx->v[1] & 0xff);
  add = sx->v[3];
  
  cnt = g_byte(add);          // no of entries
 
  #ifdef XDBGX 
  DBGPRTN("In rbasp - begin reg %x at %x, cnt %d", reg, add, cnt);
  #endif

  if (!set_cmd (add, add+1,C_BYTE))
   {
	 #ifdef XDBGX   
      DBGPRTN("Rbase Already Processed");
	 #endif 
    return ;
   }
  
  set_cmd (add+2, (add+cnt*2)+1, C_WORD);

  pc= add+2;

  for (i = 0; i < cnt; i++)
   {
	 if (reg > 0xff) break;                  // safety 
    add = g_word(pc);                       // register pointer
    r = add_rbase(reg,add,0,0);             // message in here....
    if (r) {r->cmd = 1; r->blk = 1;}
    reg+=2;
    pc+=2;
   }

 // now check pointers for XCODE directives for newly added entries
 // and don'tknow for sure which bank these point to yet

 
 for (i = 0; i < basech.num; i++)
   {
    r = (RBT*) basech.ptrs[i];
	if (r->blk)
	{   	 // auto added from above
	 cnt = 0;
                             // now match marker 
     for (j = i+1; j < basech.num; j++)
	    { 
	     n = (RBT*) basech.ptrs[j];
		 if (n->blk) 
		   {
            for (bnk = 0; bnk < BMAX; bnk++)
			  { 
				if (bkmap[bnk].bok)
				 {          // bank is OK  
				  r->val &= 0xffff;                  // strip bank  
		          pc = g_word(r->val | (bnk << 16));
			      if ((n->val & 0xffff) == pc) 
			        {
					 cnt = 1;                        // STOP, match found	
                     r->val |= (bnk << 16);	
			         set_cmd(r->val, r->val+1, C_WORD);
                     set_auxcmd(r->val,pc-1, C_XCODE);          // and add an XCODE as this is data 
			         break;
			        }
                 }               
			  }              // bank loop
             
	        #ifdef XDBGX 
              if (bnk >= BMAX)  DBGPRTN("Match address fail, rbase %x", r->reg);
		    #endif 
	
		   }
          if (cnt) break; 
		}                       // end j loop
	}
   } 	 
	 

   sx->done = 1;
   #ifdef XDBGX 
   DBGPRTN("end rbasp");
   #endif
   return ;
}



char *calcfiles(char *fname)
 {
  // do filenames
  char *t, *x;
  int i;

  t = strrchr(fname, '\n');          // DOS and Linux versions may have newline at end
  if (t) {*t = '\0';}

  x = fname;                           // keep possible pathname

  t = strrchr(fname, pathchar);         // stop at last backslash
  if (t)
   {          // assume a path in filename, replace path.
    *t = '\0';
    fname = t+1;
    sprintf(fldata.path, "%s%c", x,pathchar);
   }


  x = strrchr(fname, '.');                         // ditch any extension
  if (x) *x = '\0';

  strcpy(fldata.bare,fname);                        // bare file name

  for (i = 0; i < 5; i++)
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
short dissas (char *fstr)
{
  init_structs();
  calcfiles(fstr);
  openfiles();

  if (fldata.error)
    {
     printf("File not found or File error\n");
     return fldata.error;
    }

  wnprtn("\n ----------------------------");
 
  wnprtn(" SAD Version %s", SADVERSION);
 wnprtn(" ----------------------------");
 wnprtn(0);
   
  if (readbin() < 0)               // and does init_structs in here
  {
    wnprtn("abandoned !!");
    printf("can't process");
    return 0;  
  }

  //verify_chk ();
  show_prog(++anlpass);            //anlpass = 1  cmds and setup
  getudir();                       // AFTER readbin...

  show_prog(++anlpass);            //anlpass = 2  signature pass
  prescan_sigs();                  // full pass for signatures


 
  wnprt("\n\n --- Start Disassembly -----\n");

  #ifdef XDBGX
   DBGPRT("-- num commands before scan\n");
   DBGPRTN("# aux = %d, cmds = %d", auxch.num,cmdch.num);
  #endif 

  scan_all ();

  do_ddt(1);               // scan any vect pushes
                           // should check if anything on HOLD !!
  scan_all ();

 // wnprt("\n\n------------Post scan ------------\n");

 // ----------------------------------------------------------------------

  do_jumps();
 
  code_scans();    // turn all scans into code - after any hole fills.....
  
  do_ddt(0);        // code, data, subnames  (need to add any scans from pushes here too....)
  
  do_subnames();


  show_prog(++anlpass);             //anlpass = 3  scan phase
  // ----------------------------------------------------------------------
  show_prog (++anlpass);            //anlpass = 4

  #ifdef XDBGX
   DBGPRT("max alloc = %d\n", DBGmaxalloc);
   DBGPRT("cur alloc = %d\n", DBGcuralloc);
   DBGPRT("mcalls = %d\n", DBGmcalls);
   DBGPRT("mrels  = %d\n", DBGmrels);
   DBGPRT("xcalls = %d\n", DBGxcalls);
   DBGPRT("tscans = %d\n", tscans);               // tscans used as safety check
  #endif 

   wnprtn("\n\n --- Outputting Listing to %s------------\n",fldata.fn[1]);

  anlpass = ANLPRT;

  do_listing ();
  newl(2);     // flush output file

  // must remember to drop subs with no name ...

  #ifdef XDBGX
    DBGPRTN("\n\n -- DEBUG INFO --");
    DBG_jumps(&jpfch);
    DBG_scans();
    DBG_ddt();
    DBG_sigs(&sigch);
  #endif	

 

  wnprtn("\n\n---------------------------------------------------------------------------------------------");
  wnprtn("The disassembler has scanned the binary and arrived at the following final command list.");
  wnprtn("This list is not guaranteed to be perfect, but should be a good basic start.");
  wnprtn("This following list can be copied and pasted into a dir file.  ( #cmd = user command/directive)");
  wnprtn("---------------------------------------------------------------------------------------------\n\n");
  
  prt_dirs(1);
  free_structs();
  wnprtn( "\n\n------ END Disassembly run ------\n");
  printf ("\n End of run\n");
  closefiles();
  return 0;
}

/*void put_config(char* p)
{
 // put config if sad.ini is null

  short i;
  char *t;

  if (!fldata.fl[5])      // double check
   {
    fldata.fl[5] = fopen(fldata.fn[5], "w");    //open for write
    if (!fldata.fl[5])
      {
       fldata.error = 6;                        // error opening file
       return;
      }

    for (i =0; i < 5; i++)
      {
       t = strrchr(fldata.fn[i], pathchar);          // is there a path ?
       if (t)
        {
         *t = '\0';                     // terminate the path
         t = fldata.fn[i];               // set path
        }
       else
        {
         t = fldata.dpath;                // set default path
         sprintf(fldata.fn[i],"%s%c", t,pathchar);   // and put in file path
        }
       fprintf (fldata.fl[5], "%s%c    #%s location \n", t,pathchar,fd[i].suffix);
      }
    fclose(fldata.fl[5]);

   }
}*/

int get_config(char** pts)
 {
  /* pts [0] = exe path (and default) ,
   * [1] = config file READ path or zero.  (use default path)
   * [2] = config WRITE path or zero
   */

  short i;
  char *t;

  // strchr(cmdline, '\')   for first occurence   DOS (and 'x:'
  // strchr(cmdline, '/')   for first occurence   UNIX

  t = pts[0];
  if (*t == '\"' || *t == '\'') t++;      // skip any opening quote
  strcpy(fldata.dpath, t);

  t = strchr(fldata.dpath, '\\');
  if (t) pathchar = '\\';                    // assume DOS or WIN
  else
   {
    t = strchr(fldata.dpath, '/');
    if (t) pathchar = '/';                   // assume UX
   }
   
  if (!t) printf("error in cmdline !!");

  t = strrchr(fldata.dpath, pathchar);           // stop at last backslash to get path
  if (t) *t = '\0';

  if (pts[1])
  {
     strncpy(fldata.fn[5],pts[1], 255);           // copy max of 255 chars
     i = strlen(fldata.fn[5]);                    // size to get last char
     t = fldata.fn[5]+i-1;                        // t = last char
     if (*t == pathchar) *t = 0; else t++;      

     sprintf(t, "%c%s",pathchar,fd[5].suffix);    // and add sad.ini to it...

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
    sprintf(fldata.fn[5], "%s%c%s",fldata.dpath,pathchar,fd[5].suffix);
    fldata.fl[5] = fopen(fldata.fn[5], fd[5].fpars);         //open it if there
  }

 // printf ("\nconfig file = %s", fldata.fn[5]);

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
             if (*t != pathchar) sprintf(t+1, "%c", pathchar);
			}
		 strcpy(fldata.fn[i], flbuf);
		 
       }
      }
     fclose(fldata.fl[5]);                 //close leaves handle addr
     strcpy(fldata.path, fldata.fn[0]);
     printf("\n");
    }
  else printf(" - not found, assume local dir\n");
  
//  if (pts[2])  put_config(pts[2]);
  return 0;
 };
